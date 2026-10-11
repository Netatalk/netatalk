/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>

#include <atalk/dsi.h>

/*!
 * @brief Reply to a DSIGetStatus request with the cached server status block
 *
 * The reply reuses the request header and sends the statuslen bytes of status
 * through the command buffer.
 *
 * @param[in,out] dsi  DSI session holding the status block
 */
void dsi_getstatus(DSI *dsi)
{
    dsi->header.dsi_flags = DSIFL_REPLY;
#if 0
    dsi->header.dsi_command = DSIFUNC_STAT;
#endif
    dsi->header.dsi_data.dsi_code = dsi->header.dsi_reserved = 0;
    memcpy(dsi->commands, dsi->status, dsi->statuslen);
    dsi->cmdlen = dsi->statuslen;
    dsi_send(dsi);
}
