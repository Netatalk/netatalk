/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

/*!
 * @file
 * @brief Closing a DSI session with a DSICloseSession request to the client
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>

#include <atalk/dsi.h>

/*!
 * @brief Send a DSICloseSession request, close the transport, and free the DSI
 * session
 *
 * A sleeping or disconnected session gets neither the request nor the
 * transport close, and a session whose reply is still under way gets the
 * transport close alone. The call frees the structure in every case.
 *
 * @param[in,out] dsi  DSI session to end, invalid afterwards
 */
void dsi_close(DSI *dsi)
{
    if (!(dsi->flags & DSI_SLEEPING) && !(dsi->flags & DSI_DISCONNECTED)) {
        /* server generated. need to set all the fields; nothing follows a
         * reply still under way */
        if (!dsi->in_write) {
            dsi->header.dsi_flags = DSIFL_REQUEST;
            dsi->header.dsi_command = DSIFUNC_CLOSE;
            dsi->header.dsi_requestID = htons(dsi_serverID(dsi));
            dsi->header.dsi_data.dsi_code = htonl(0);
            dsi->header.dsi_reserved = htonl(0);
            dsi->cmdlen = 0;
            dsi_send(dsi);
        }

        dsi->proto_close(dsi);
    }

    free(dsi);
}
