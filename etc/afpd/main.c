/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/syslog.h>
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
#include "config.h"
#include "status.h"
#include "fork.h"
#include "uam_auth.h"

unsigned char	nologin = 0;

static struct afp_options default_options;
static AFPConfig *configs;
static server_child *server_children;
static fd_set save_rfds;

#ifdef CAPDIR
int addr_net, addr_node, addr_uid;
char addr_name[32];
#endif CAPDIR

static void afp_exit(const int i)
{
  server_unlock(default_options.pidfile);
  exit(i);
}

static void afp_goaway(int sig)
{
#ifndef NO_DDP
    asp_kill(sig);
#endif
    dsi_kill(sig);
    switch( sig ) {
    case SIGTERM :
      syslog( LOG_INFO, "shutting down on signal %d", sig );
      break;
    case SIGHUP :
      /* w/ a configuration file, we can force a re-read if we want */
      nologin++;
      if ((nologin + 1) & 1) {
	AFPConfig *config;

	syslog(LOG_INFO, "re-reading configuration file");
	for (config = configs; config; config = config->next)
	  if (config->server_cleanup)
	    config->server_cleanup(config);

	configfree(configs, NULL);
	if (!(configs = configinit(&default_options))) {
	  syslog(LOG_ERR, "config re-read: no servers configured");
	  afp_exit(1);
	}
	FD_ZERO(&save_rfds);
	for (config = configs; config; config = config->next) {
	  if (config->fd < 0)
	    continue;
	  FD_SET(config->fd, &save_rfds);
	}
      } else {
	syslog(LOG_INFO, "disallowing logins");
	auth_unload();
      }
      break;
    default :
	syslog( LOG_ERR, "afp_goaway: bad signal" );
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

    umask( 0 );		/* so inherited file permissions work right */

    afp_options_init(&default_options);
    if (!afp_options_parse(ac, av, &default_options))
      exit(1);
    
    switch(server_lock("afpd", default_options.pidfile, 
		       default_options.flags & OPTION_DEBUG)) {
    case -1: /* error */
      exit(1);
    case 0: /* child */
      break;
    default: /* server */
      exit(0);
    }

    /* install child handler for asp and dsi. we do this before afp_goaway
     * as afp_goaway references stuff from here. 
     * XXX: this should really be setup after the initial connections. */
    if (!(server_children = server_child_alloc(default_options.connections,
					       CHILD_NFORKS))) {
      syslog(LOG_ERR, "main: server_child alloc: %m");
      afp_exit(1);
    }
      
    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = child_handler;
    sigemptyset( &sv.sa_mask );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGCHLD, &sv, 0 ) < 0 ) {
	syslog( LOG_ERR, "main: sigaction: %m" );
	afp_exit(1);
    }

    sv.sa_handler = afp_goaway;
    sigemptyset( &sv.sa_mask );
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGTERM);
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGHUP, &sv, 0 ) < 0 ) {
	syslog( LOG_ERR, "main: sigaction: %m" );
	afp_exit(1);
    }
    if ( sigaction( SIGTERM, &sv, 0 ) < 0 ) {
	syslog( LOG_ERR, "main: sigaction: %m" );
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
      syslog(LOG_ERR, "main: no servers configured: %m\n");
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
	syslog(LOG_ERR, "main: can't wait for input: %m");
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
