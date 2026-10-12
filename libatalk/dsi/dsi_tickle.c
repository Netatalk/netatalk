/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

/*!
 * @file
 * @brief DSITickle requests
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <atalk/dsi.h>

/*!
 * @brief Send a DSITickle request to the client
 *
 * Its callers run in the main loop, so it blocks no signals. A sleeping
 * session or one in the middle of a reply skips the tickle and yields 1.
 * Otherwise the result is that of dsi_stream_write, the bytes sent or -1.
 *
 * @param[in,out] dsi  DSI session
 */
int dsi_tickle(DSI *dsi)
{
    char block[DSI_BLOCKSIZ];
    uint16_t id;

    if ((dsi->flags & DSI_SLEEPING) || dsi->in_write) {
        return 1;
    }

    id = htons(dsi_serverID(dsi));
    memset(block, 0, sizeof(block));
    block[0] = DSIFL_REQUEST;
    block[1] = DSIFUNC_TICKLE;
    memcpy(block + 2, &id, sizeof(id));
    /* code = len = reserved = 0 */
    return dsi_stream_write(dsi, block, DSI_BLOCKSIZ, DSI_NOWAIT);
}

