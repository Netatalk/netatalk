/*
 * $Id: main.c,v 1.18 2002-03-24 01:23:41 sibaz Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <atalk/logger.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <errno.h>

#include <netatalk/endian.h>
#include <netatalk/at.h>
#include <atalk/compat.h>
#include <atalk/dsi.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/afp.h>
#include <atalk/adouble.h>
#include <atalk/paths.h>
#include <atalk/util.h>
#include <atalk/server_child.h>

#include "globals.h"
#include "afp_config.h"
#include "status.h"
#include "fork.h"
#include "uam_auth.h"

#ifdef TRU64
#include <sys/security.h>
#include <prot.h>
#include <sia.h>

static int argc = 0;
static char **argv = NULL;
#endif /* TRU64 */

#ifdef DID_MTAB
#include "parse_mtab.h"
#endif /* DID_MTAB */

unsigned char	nologin = 0;

#ifdef DID_MTAB
/* global mount table; afpd_st_cnid uses this to lookup the right entry.  */
static struct afpd_mount_table *afpd_mount_table = NULL;
#endif /* DID_MTAB */

struct afp_options default_options;
static AFPConfig *configs;
static server_child *server_children;
static fd_set save_rfds;

#ifdef TRU64
void afp_get_cmdline( int *ac, char ***av)
{
    *ac = argc;
    *av = argv;
}
#endif /* TRU64 */

static void afp_exit(const int i)
{
    server_unlock(default_options.pidfile);
    exit(i);
}

static void afp_goaway(int sig)
{
#ifndef NO_DDP
    asp_kill(sig);
#endif /* ! NO_DDP */
    dsi_kill(sig);
    switch( sig ) {
    case SIGTERM :
        LOG(log_info, logtype_afpd, "shutting down on signal %d", sig );
        break;
    case SIGHUP :
        /* w/ a configuration file, we can force a re-read if we want */
        nologin++;
        if ((nologin + 1) & 1) {
            AFPConfig *config;

            LOG(log_info, logtype_afpd, "re-reading configuration file");
            for (config = configs; config; config = config->next)
                if (config->server_cleanup)
                    config->server_cleanup(config);

            configfree(configs, NULL);
            if (!(configs = configinit(&default_options))) {
                LOG(log_error, logtype_afpd, "config re-read: no servers configured");
                afp_exit(1);
            }
            FD_ZERO(&save_rfds);
            for (config = configs; config; config = config->next) {
                if (config->fd < 0)
                    continue;
                FD_SET(config->fd, &save_rfds);
            }
        } else {
            LOG(log_info, logtype_afpd, "disallowing logins");
            auth_unload();
        }
        break;
    default :
        LOG(log_error, logtype_afpd, "afp_goaway: bad signal" );
    }
    if ( sig == SIGTERM ) {
        AFPConfig *config;

        for (config = configs; config; config = config->next)
            if (config->server_cleanup)
                config->server_cleanup(config);

        afp_exit(0);
    }
    return;
}

static void child_handler()
{
    server_child_handler(server_children);
}

int main( ac, av )
int		ac;
char	**av;
{
    AFPConfig           *config;
    fd_set              rfds;
    struct sigaction	sv;
    sigset_t            sigs;

#ifdef TRU64
    argc = ac;
    argv = av;
    set_auth_parameters( ac, av );
#endif /* TRU64 */

    afp_options_init(&default_options);
    if (!afp_options_parse(ac, av, &default_options))
        exit(1);

    umask( default_options.umask );

    switch(server_lock("afpd", default_options.pidfile,
                       default_options.flags & OPTION_DEBUG)) {
    case -1: /* error */
        exit(1);
    case 0: /* child */
        break;
    default: /* server */
        exit(0);
    }

#ifdef DID_MTAB
    /* if we are going to use afpd.mtab, load the file */
    afpd_mount_table = afpd_mtab_parse ( AFPD_MTAB_FILE );
#endif /* DID_MTAB */

    /* install child handler for asp and dsi. we do this before afp_goaway
     * as afp_goaway references stuff from here. 
     * XXX: this should really be setup after the initial connections. */
    if (!(server_children = server_child_alloc(default_options.connections,
                            CHILD_NFORKS))) {
        LOG(log_error, logtype_afpd, "main: server_child alloc: %s", strerror(errno) );
        afp_exit(1);
    }

    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = child_handler;
    sigemptyset( &sv.sa_mask );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGCHLD, &sv, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "main: sigaction: %s", strerror(errno) );
        afp_exit(1);
    }

    sv.sa_handler = afp_goaway;
    sigemptyset( &sv.sa_mask );
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGTERM);
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGHUP, &sv, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "main: sigaction: %s", strerror(errno) );
        afp_exit(1);
    }
    if ( sigaction( SIGTERM, &sv, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "main: sigaction: %s", strerror(errno) );
        afp_exit(1);
    }

    /* afpd.conf: not in config file: lockfile, connections, configfile
     *            preference: command-line provides defaults.
     *                        config file over-writes defaults.
     *
     * we also need to make sure that killing afpd during startup
     * won't leave any lingering registered names around.
     */

    sigemptyset(&sigs);
    sigaddset(&sigs, SIGHUP);
    sigaddset(&sigs, SIGTERM);
    sigprocmask(SIG_BLOCK, &sigs, NULL);
    if (!(configs = configinit(&default_options))) {
        LOG(log_error, logtype_afpd, "main: no servers configured: %s\n", strerror(errno));
        afp_exit(1);
    }
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);

    /* watch atp and dsi sockets. */
    FD_ZERO(&save_rfds);
    for (config = configs; config; config = config->next) {
        if (config->fd < 0) /* for proxies */
            continue;
        FD_SET(config->fd, &save_rfds);
    }

    /* wait for an appleshare connection. parent remains in the loop
     * while the children get handled by afp_over_{asp,dsi}.  this is
     * currently vulnerable to a denial-of-service attack if a
     * connection is made without an actual login attempt being made
     * afterwards. establishing timeouts for logins is a possible 
     * solution. */
    while (1) {
        rfds = save_rfds;
        if (select(FD_SETSIZE, &rfds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR)
                continue;
            LOG(log_error, logtype_afpd, "main: can't wait for input: %s", strerror(errno));
            break;
        }

        for (config = configs; config; config = config->next) {
            if (config->fd < 0)
                continue;
            if (FD_ISSET(config->fd, &rfds))
                config->server_start(config, configs, server_children);
        }
    }

    return 0;
}
