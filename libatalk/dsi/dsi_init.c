/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 2012, Frank Lahm <franklahm@googlemail.com>
 * All rights reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <atalk/dsi.h>

/*!
 * @brief Allocate a DSI listener and bind it to the TCP transport
 *
 * The attention quantum starts at DSI_DEFQUANT, and the server quantum,
 * read-ahead buffer size, and recvfile pipe size come from the AFPObj options.
 * The recvfile pipe stays closed until first use. dsi_tcp_init creates the
 * listening socket, and its failure frees the structure again.
 *
 * @param[in] obj       server object supplying the options
 * @param[in] hostname  server host name
 * @param[in] address   address to listen on, or NULL for the wildcard address
 * @param[in] port      TCP port to listen on
 * @returns the listener, or NULL when allocation or the transport setup failed
 */
DSI *dsi_init(AFPObj *obj, const char *hostname, const char *address,
              const char *port)
{
    DSI		*dsi;

    if ((dsi = (DSI *)calloc(1, sizeof(DSI))) == NULL) {
        return NULL;
    }

    dsi->attn_quantum = DSI_DEFQUANT;
    dsi->server_quantum = obj->options.server_quantum;
    dsi->dsireadbuf = obj->options.dsireadbuf;
    dsi->splice_pipe[0] = dsi->splice_pipe[1] = -1;
    dsi->splice_size = obj->options.splice_size;

    /* currently the only transport protocol that exists for dsi */
    if (dsi_tcp_init(dsi, hostname, address, port) != 0) {
        free(dsi);
        dsi = NULL;
    }

    return dsi;
}
