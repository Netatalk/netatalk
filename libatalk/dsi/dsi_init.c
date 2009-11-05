/*
 * $Id: dsi_init.c,v 1.10 2009-11-05 14:38:08 franklahm Exp $
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
	      const char *port, const int proxy, const u_int32_t quantum)
{
    DSI		*dsi;

    if ((dsi = (DSI *) calloc(1, sizeof(DSI))) == NULL) {
      return( NULL );
    }
    dsi->attn_quantum = DSI_DEFQUANT; /* default quantum size */
    dsi->server_quantum = quantum; /* default server quantum */
    dsi->program = program;

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
