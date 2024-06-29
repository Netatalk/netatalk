/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/compat.h>
#include <atalk/dsi.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/paths.h>
#include <atalk/server_child.h>
#include <atalk/server_ipc.h>
#include <atalk/util.h>

#include "afp_config.h"
#include "afpstats.h"
#include "fork.h"
#include "status.h"
#include "uam_auth.h"

#define ASEV_THRESHHOLD 10

unsigned char nologin = 0;

static AFPObj dsi_obj;
static AFPObj asp_obj;
static server_child_t *server_children;
static sig_atomic_t reloadconfig = 0;
static sig_atomic_t gotsigchld = 0;
static struct asev *asev;

static afp_child_t *dsi_start(AFPObj *obj, DSI *dsi, server_child_t *server_children);
static int asp_start(AFPObj* obj, server_child_t* server_children);
static void asp_cleanup(const AFPObj* obj);

static void afp_exit(int ret)
{
    exit(ret);
}


/* ------------------
   initialize fd set we are waiting for.
*/
static bool init_listening_sockets(const AFPObj *dsiconfig, const AFPObj *aspconfig)
{
    DSI *dsi;
    int numlisteners;

    for (numlisteners = 0, dsi = dsiconfig->dsi; dsi; dsi = dsi->next) {
        numlisteners++;
    }
    asev = asev_init(dsiconfig->options.connections + numlisteners + ASEV_THRESHHOLD);
    if (asev == NULL) {
        return false;
    }

    for (dsi = dsiconfig->dsi; dsi; dsi = dsi->next) {
        if (!(asev_add_fd(asev, dsi->serversock, LISTEN_FD, dsi, AFPPROTO_DSI))) {
            return false;
        }
    }
#ifndef NO_DDP
        if (!(asev_add_fd(asev, aspconfig->fd, LISTEN_FD, 0, AFPPROTO_ASP))) {
            return false;
        }
#endif /* no afp/asp */
    return true;
}

static bool reset_listening_sockets(const AFPObj *dsiconfig, const AFPObj *aspconfig)
{
    const DSI *dsi;

    for (dsi = dsiconfig->dsi; dsi; dsi = dsi->next) {
        if (!(asev_del_fd(asev, dsi->serversock))) {
            return false;
        }
    }
#ifndef NO_DDP
    if (aspconfig->fd) /* only attempt to reset asp socket if we have one */
    {
        if (!(asev_del_fd(asev, aspconfig->fd))) {
            return false;
        }
    }
#endif /* no afp/asp */
    return true;
}

/* ------------------ */
static void afp_goaway(int sig)
{
#ifndef NO_DDP
    asp_kill(sig);
#endif /* ! NO_DDP */

    switch( sig ) {

    case SIGTERM:
    case SIGQUIT:
        LOG(log_note, logtype_afpd, "AFP Server shutting down");
        if (server_children)
            server_child_kill(server_children, SIGTERM);
#ifndef NO_DDP
        if ((asp_obj.handle))
        {
            asp_cleanup(&asp_obj);
        }
#endif /* ! NO_DDP */
        _exit(0);
        break;

    case SIGUSR1 :
        nologin++;
        auth_unload();
        LOG(log_info, logtype_afpd, "disallowing logins");

        if (server_children)
            server_child_kill(server_children, sig);
        break;

    case SIGHUP :
        /* w/ a configuration file, we can force a re-read if we want */
        reloadconfig = 1;
        break;

    case SIGCHLD:
        /* w/ a configuration file, we can force a re-read if we want */
        gotsigchld = 1;
        break;

    default :
        LOG(log_error, logtype_afpd, "afp_goaway: bad signal" );
    }
    return;
}

static void child_handler(void)
{
    int fd;
    int status;
    pid_t pid;

#ifndef WAIT_ANY
#define WAIT_ANY (-1)
#endif /* ! WAIT_ANY */

    while ((pid = waitpid(WAIT_ANY, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status))
                LOG(log_info, logtype_afpd, "child[%d]: exited %d", pid, WEXITSTATUS(status));
            else
                LOG(log_info, logtype_afpd, "child[%d]: done", pid);
        } else {
            if (WIFSIGNALED(status))
                LOG(log_info, logtype_afpd, "child[%d]: killed by signal %d", pid, WTERMSIG(status));
            else
                LOG(log_info, logtype_afpd, "child[%d]: died", pid);
        }

        fd = server_child_remove(server_children, pid);
        if (fd == -1) {
            continue;
        }
        if (!(asev_del_fd(asev, fd))) {
            LOG(log_error, logtype_afpd, "child[%d]: asev_del_fd: %d", pid, fd);
        }
    }
}

static int setlimits(void)
{
    struct rlimit rlim;

    if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
        LOG(log_warning, logtype_afpd, "setlimits: reading current limits failed: %s", strerror(errno));
        return -1;
    }
    if (rlim.rlim_cur != RLIM_INFINITY && rlim.rlim_cur < 65535) {
        rlim.rlim_cur = 65535;
        if (rlim.rlim_max != RLIM_INFINITY && rlim.rlim_max < 65535)
            rlim.rlim_max = 65535;
        if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
            LOG(log_warning, logtype_afpd, "setlimits: increasing limits failed: %s", strerror(errno));
            return -1;
        }
    }
    return 0;
}

int main(int ac, char **av)
{
    struct sigaction	sv;
    sigset_t            sigs;
    int                 ret;

    /* Parse argv args and initialize default options */
    afp_options_parse_cmdline(&dsi_obj, ac, av);

    if (!(dsi_obj.cmdlineflags & OPTION_DEBUG) && (daemonize(0, 0) != 0))
        exit(EXITERR_SYS);

    /* Log SIGBUS/SIGSEGV SBT */
    fault_setup(NULL);

    if (afp_config_parse(&dsi_obj, "afpd") != 0)
        afp_exit(EXITERR_CONF);

    /* Save the user's current umask */
    dsi_obj.options.save_mask = umask(dsi_obj.options.umask);

    /* install child handler for asp and dsi. we do this before afp_goaway
     * as afp_goaway references stuff from here.
     * XXX: this should really be setup after the initial connections. */
    if (!(server_children = server_child_alloc(dsi_obj.options.connections))) {
        LOG(log_error, logtype_afpd, "main: server_child alloc: %s", strerror(errno) );
        afp_exit(EXITERR_SYS);
    }

#ifndef NO_DDP
    /* initialize appletalk protocol support if enabled */
    if (dsi_obj.options.flags & OPTION_DDP)
    {
        afp_options_parse_cmdline(&asp_obj, ac, av);
        if (afp_config_parse(&asp_obj, "afpd") != 0)
            afp_exit(EXITERR_CONF);
    }
#endif /* no afp/asp */

    sigemptyset(&sigs);
    pthread_sigmask(SIG_SETMASK, &sigs, NULL);

    memset(&sv, 0, sizeof(sv));
    /* linux at least up to 2.4.22 send a SIGXFZ for vfat fs,
       even if the file is open with O_LARGEFILE ! */
#ifdef SIGXFSZ
    sv.sa_handler = SIG_IGN;
    sigemptyset( &sv.sa_mask );
    if (sigaction(SIGXFSZ, &sv, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "main: sigaction: %s", strerror(errno) );
        afp_exit(EXITERR_SYS);
    }
#endif

    sv.sa_handler = SIG_IGN;
    sigemptyset( &sv.sa_mask );
    if (sigaction(SIGPIPE, &sv, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "main: sigaction: %s", strerror(errno) );
        afp_exit(EXITERR_SYS);
    }

    sv.sa_handler = afp_goaway; /* handler for all sigs */

    sigemptyset( &sv.sa_mask );
    sigaddset(&sv.sa_mask, SIGALRM);
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGTERM);
    sigaddset(&sv.sa_mask, SIGUSR1);
    sigaddset(&sv.sa_mask, SIGQUIT);
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGCHLD, &sv, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "main: sigaction: %s", strerror(errno) );
        afp_exit(EXITERR_SYS);
    }

    sigemptyset( &sv.sa_mask );
    sigaddset(&sv.sa_mask, SIGALRM);
    sigaddset(&sv.sa_mask, SIGTERM);
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGCHLD);
    sigaddset(&sv.sa_mask, SIGQUIT);
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR1, &sv, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "main: sigaction: %s", strerror(errno) );
        afp_exit(EXITERR_SYS);
    }

    sigemptyset( &sv.sa_mask );
    sigaddset(&sv.sa_mask, SIGALRM);
    sigaddset(&sv.sa_mask, SIGTERM);
    sigaddset(&sv.sa_mask, SIGUSR1);
    sigaddset(&sv.sa_mask, SIGCHLD);
    sigaddset(&sv.sa_mask, SIGQUIT);
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGHUP, &sv, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "main: sigaction: %s", strerror(errno) );
        afp_exit(EXITERR_SYS);
    }

    sigemptyset( &sv.sa_mask );
    sigaddset(&sv.sa_mask, SIGALRM);
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGUSR1);
    sigaddset(&sv.sa_mask, SIGCHLD);
    sigaddset(&sv.sa_mask, SIGQUIT);
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGTERM, &sv, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "main: sigaction: %s", strerror(errno) );
        afp_exit(EXITERR_SYS);
    }

    sigemptyset( &sv.sa_mask );
    sigaddset(&sv.sa_mask, SIGALRM);
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGUSR1);
    sigaddset(&sv.sa_mask, SIGCHLD);
    sigaddset(&sv.sa_mask, SIGTERM);
    sv.sa_flags = SA_RESTART;
    if (sigaction(SIGQUIT, &sv, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "main: sigaction: %s", strerror(errno) );
        afp_exit(EXITERR_SYS);
    }

    /* afp.conf:  not in config file: lockfile, configfile
     *            preference: command-line provides defaults.
     *                        config file over-writes defaults.
     *
     * we also need to make sure that killing afpd during startup
     * won't leave any lingering registered names around.
     */

    sigemptyset(&sigs);
    sigaddset(&sigs, SIGALRM);
    sigaddset(&sigs, SIGHUP);
    sigaddset(&sigs, SIGUSR1);
    sigaddset(&sigs, SIGCHLD);

    pthread_sigmask(SIG_BLOCK, &sigs, NULL);
#ifdef HAVE_DBUS_GLIB
    /* Run dbus AFP statistics thread */
    if (dsi_obj.options.flags & OPTION_DBUS_AFPSTATS)
        (void)afpstats_init(server_children);
#endif
    if (configinit(&dsi_obj, &asp_obj) != 0) {
        LOG(log_error, logtype_afpd, "main: no servers configured");
        afp_exit(EXITERR_CONF);
    }
    pthread_sigmask(SIG_UNBLOCK, &sigs, NULL);

    /* Initialize */
    cnid_init();

    /* watch atp, dsi sockets and ipc parent/child file descriptor. */
    if (!(init_listening_sockets(&dsi_obj, &asp_obj))) {
        LOG(log_error, logtype_afpd, "main: couldn't initialize socket handler");
        afp_exit(EXITERR_CONF);
    }

    /* set limits */
    (void)setlimits();

    afp_child_t *child;
    int saveerrno;

    /* wait for an appleshare connection. parent remains in the loop
     * while the children get handled by afp_over_{asp,dsi}.  this is
     * currently vulnerable to a denial-of-service attack if a
     * connection is made without an actual login attempt being made
     * afterwards. establishing timeouts for logins is a possible
     * solution. */
    while (1) {
        pthread_sigmask(SIG_UNBLOCK, &sigs, NULL);
        ret = poll(asev->fdset, asev->used, -1);
        pthread_sigmask(SIG_BLOCK, &sigs, NULL);
        saveerrno = errno;

        if (gotsigchld) {
            gotsigchld = 0;
            child_handler();
            continue;
        }

        if (reloadconfig) {
            nologin++;

            if (!(reset_listening_sockets(&dsi_obj, &asp_obj))) {
                LOG(log_error, logtype_afpd, "main: reset socket handlers");
                afp_exit(EXITERR_CONF);
            }

            LOG(log_info, logtype_afpd, "re-reading configuration file");
            configfree(&dsi_obj, NULL);
            afp_config_free(&dsi_obj);
#ifndef NO_DDP
            afp_config_free(&asp_obj);
            if ((asp_obj.handle))
            {
                asp_cleanup(&asp_obj);
                configfree(&asp_obj, NULL);
            }
#endif /* no afp/asp */

            if (afp_config_parse(&dsi_obj, "afpd") != 0)
                afp_exit(EXITERR_CONF);

#ifndef NO_DDP
            /* initialize appletalk protocol support if enabled */
            if (dsi_obj.options.flags & OPTION_DDP)
            {
                afp_options_parse_cmdline(&asp_obj, ac, av);
                if (afp_config_parse(&asp_obj, "afpd") != 0)
                    afp_exit(EXITERR_CONF);
            }
#endif /* no afp/asp */

            if (configinit(&dsi_obj, &asp_obj) != 0) {
                LOG(log_error, logtype_afpd, "config re-read: no servers configured");
                afp_exit(EXITERR_CONF);
            }

            if (!(init_listening_sockets(&dsi_obj, &asp_obj))) {
                LOG(log_error, logtype_afpd, "main: couldn't initialize socket handler");
                afp_exit(EXITERR_CONF);
            }

            nologin = 0;
            reloadconfig = 0;
            errno = saveerrno;

            if (server_children) {
                server_child_kill(server_children, SIGHUP);
            }

            continue;
        }

        if (ret == 0)
            continue;

        if (ret < 0) {
            if (errno == EINTR)
                continue;
            LOG(log_error, logtype_afpd, "main: can't wait for input: %s", strerror(errno));
            break;
        }

        for (int i = 0; i < asev->used; i++) {
            if (asev->fdset[i].revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL)) {
                switch (asev->data[i].fdtype) {

                case LISTEN_FD:
                    switch (asev->data[i].protocol) {

                    case AFPPROTO_DSI:
                        if ((child = dsi_start(&dsi_obj, (DSI*)(asev->data[i].private), server_children))) {
                            if (!(asev_add_fd(asev, child->afpch_ipc_fd, IPC_FD, child, 0))) {
                                LOG(log_error, logtype_afpd, "out of asev slots");

                                /*
                                 * Close IPC fd here and mark it as unused
                                 */
                                close(child->afpch_ipc_fd);
                                child->afpch_ipc_fd = -1;

                                /*
                                 * Being unfriendly here, but we really
                                 * want to get rid of it. The 'child'
                                 * handle gets cleaned up in the SIGCLD
                                 * handler.
                                 */
                                kill(child->afpch_pid, SIGKILL);
                            }
                        }
                        break;
#ifndef NO_DDP
                    case AFPPROTO_ASP:
                        asp_start(&asp_obj, server_children);
                        break;
#endif /* no afp/asp */ 
                    default:
                        LOG(log_debug, logtype_afpd, "main: unknown protocol type");
                        break;
                    }
                    break;

                case IPC_FD:
                    child = (afp_child_t *)(asev->data[i].private);
                    LOG(log_debug, logtype_afpd, "main: IPC request from child[%u]", child->afpch_pid);

                    if (ipc_server_read(server_children, child->afpch_ipc_fd) != 0) {
                        if (!(asev_del_fd(asev, child->afpch_ipc_fd))) {
                            LOG(log_error, logtype_afpd, "child[%u]: no IPC fd");
                        }
                        close(child->afpch_ipc_fd);
                        child->afpch_ipc_fd = -1;
                    }
                    break;

                default:
                    LOG(log_debug, logtype_afpd, "main: IPC request for unknown type");
                    break;
                } /* switch */
            }  /* if */
        } /* for (i)*/
    } /* while (1) */

    return 0;
}

static afp_child_t *dsi_start(AFPObj *obj, DSI *dsi, server_child_t *server_children)
{
    afp_child_t *child = NULL;

    if (dsi_getsession(dsi, server_children, obj->options.tickleval, &child) != 0) {
        LOG(log_error, logtype_afpd, "dsi_start: session error: %s", strerror(errno));
        return NULL;
    }

    /* we've forked. */
    if (child == NULL) {
        configfree(obj, dsi);
	afp_over_dsi(obj); /* start a session */
        exit (0);
    }

    return child;
}
#ifndef NO_DDP
static int asp_start(AFPObj* obj, server_child_t *server_children)
{
    ASP asp;

    if (!(asp = asp_getsession(obj->handle, server_children, obj->options.tickleval))) {
        LOG(log_error, logtype_afpd, "main: asp_getsession: %s", strerror(errno));
	exit(EXITERR_CLNT);
    }

    if (asp->child) {
        afp_over_asp(obj);
        exit(0);
    }
    return 0;
}
static void asp_cleanup(const AFPObj* obj)
{
    /* we need to stop tickle handler */
    asp_stop_tickle();
    nbp_unrgstr(obj->Obj, obj->Type, obj->Zone,
        &obj->options.ddpaddr);
}
#endif /* no afp/asp */ 
