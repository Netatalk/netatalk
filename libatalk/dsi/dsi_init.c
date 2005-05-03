/*
 * $Id: dsi_init.c,v 1.5 2005-05-03 14:55:15 didg Exp $
 *
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <atalk/dsi.h>
#include "dsi_private.h"

DSI *dsi_init(const dsi_proto protocol, const char *program, 
	      const char *hostname, const char *address,
	      const int port, const int proxy, const u_int32_t quantum)
{
    DSI		*dsi;

    if ((dsi = (DSI *) calloc(1, sizeof(DSI))) == NULL) {
      return( NULL );
    }
    dsi->attn_quantum = DSI_DEFQUANT; /* default quantum size */
    dsi->server_quantum = quantum; /* default server quantum */
    dsi->program = program;

    /* signals to block. we actually disable timers for "known" 
     * large transfers (i.e., dsi_read/write). */
    sigemptyset(&dsi->sigblockset);
    sigaddset(&dsi->sigblockset, SIGTERM);
    sigaddset(&dsi->sigblockset, SIGHUP);
    sigaddset(&dsi->sigblockset, SIGALRM);
    sigaddset(&dsi->sigblockset, SIGUSR1);
    /* always block SIGUSR2 even if SERVERTEXT is not defined */
    sigaddset(&dsi->sigblockset, SIGUSR2);
    switch (protocol) {
      /* currently the only transport protocol that exists for dsi */
    case DSI_TCPIP: 
      if (!dsi_tcp_init(dsi, hostname, address, port, proxy)) {
	free(dsi);
	dsi = NULL;
      }
      break;

    default: /* unknown protocol */
      free(dsi);
      dsi = NULL;
      break;
    }

    return dsi;
}

void dsi_setstatus(DSI *dsi, char *status, const size_t slen)
{
    dsi->status = status;
    dsi->statuslen = slen;
}
