/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <stdio.h>

#include <atalk/dsi.h>
#include <atalk/logger.h>

/*!
 * @brief Send the reply to the current DSICommand with result code err
 *
 * The reply reuses the request header, so command, request ID, and reserved
 * field stay as the request left them. The payload is the datalen bytes of
 * the data buffer.
 *
 * @param[in,out] dsi  DSI session with the reply payload in its data buffer
 * @param[in]     err  AFP result code
 * @returns 1, or 0 when the send failed
 */
int dsi_cmdreply(DSI *dsi, const int err)
{
    int ret;
    LOG(log_maxdebug, logtype_dsi, "dsi_cmdreply(DSI ID: %u, len: %zd): START",
        dsi->clientID, dsi->datalen);
    dsi->header.dsi_flags = DSIFL_REPLY;
    dsi->header.dsi_len = htonl(dsi->datalen);
    dsi->header.dsi_data.dsi_code = htonl(err);
    ret = dsi_stream_send(dsi, dsi->data, dsi->datalen);
    LOG(log_maxdebug, logtype_dsi, "dsi_cmdreply(DSI ID: %u, len: %zd): END",
        dsi->clientID, dsi->datalen);
    return ret;
}
