/*
 * $Id: dsi_getsess.c,v 1.3 2001-06-29 14:14:46 rufustfirefly Exp $
 *
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
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
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <syslog.h>

#include <atalk/dsi.h>
#include <atalk/server_child.h>

static server_child *children = NULL;

void dsi_kill(int sig)
{
  if (children)
    server_child_kill(children, CHILD_DSIFORK, sig);
}

/* hand off the command. return child connection to the main program */
DSI *dsi_getsession(DSI *dsi, server_child *serv_children, 
		    const int tickleval)
{
  pid_t pid;
  
  /* do a couple things on first entry */
  if (!dsi->inited) {
    if (!(children = serv_children))
      return NULL;
    dsi->inited = 1;
  }
  
  switch (pid = dsi->proto_open(dsi)) {
  case -1:
    /* if we fail, just return. it might work later */
    syslog(LOG_ERR, "dsi_getsess: %m");
    return dsi;

  case 0: /* child. mostly handled below. */
    dsi->child = 1;
    break;

  default: /* parent */
    /* using SIGQUIT is hokey, but the child might not have
     * re-established its signal handler for SIGTERM yet. */
    if (server_child_add(children, CHILD_DSIFORK, pid) < 0) {
      syslog(LOG_ERR, "dsi_getsess: %m");
      dsi->header.dsi_flags = DSIFL_REPLY;
      dsi->header.dsi_code = DSIERR_SERVBUSY;
      dsi_send(dsi);
      dsi->header.dsi_code = DSIERR_OK;
      kill(pid, SIGQUIT);
    }

    dsi->proto_close(dsi);
    return dsi;
  }
  
  /* child: check number of open connections. this is one off the
   * actual count. */
  if ((children->count >= children->nsessions) &&
      (dsi->header.dsi_command == DSIFUNC_OPEN)) {
    syslog(LOG_INFO, "dsi_getsess: too many connections");
    dsi->header.dsi_flags = DSIFL_REPLY;
    dsi->header.dsi_code = DSIERR_TOOMANY;
    dsi_send(dsi);
    exit(1);
  }

  /* get rid of some stuff */
  close(dsi->serversock);
  server_child_free(children); 
  children = NULL;

  switch (dsi->header.dsi_command) {
  case DSIFUNC_STAT: /* send off status and return */
    {
      /* OpenTransport 1.1.2 bug workaround: 
       *
       * OT code doesn't currently handle close sockets well. urk.
       * the workaround: wait for the client to close its
       * side. timeouts prevent indefinite resource use. 
       */
      
      static struct timeval timeout = {120, 0};
      fd_set readfds;
      
      dsi_getstatus(dsi);

      FD_ZERO(&readfds);
      FD_SET(dsi->socket, &readfds);
      free(dsi);
      select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);    
      exit(0);
    }
    break;
    
  case DSIFUNC_OPEN: /* setup session */
    /* set up the tickle timer */
    dsi->timer.it_interval.tv_sec = dsi->timer.it_value.tv_sec = tickleval;
    dsi->timer.it_interval.tv_usec = dsi->timer.it_value.tv_usec = 0;
    signal(SIGPIPE, SIG_IGN); /* we catch these ourselves */
    dsi_opensession(dsi);
    return dsi;
    break;

  default: /* just close */
    syslog(LOG_INFO, "DSIUnknown %d", dsi->header.dsi_command);
    dsi->proto_close(dsi);
    exit(1);
  }
}
