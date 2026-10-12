/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

/*!
 * @file
 * @brief DSIAttention requests
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <atalk/afp.h>
#include <atalk/dsi.h>
#include <atalk/util.h>

/*!
 * @brief Send a DSIAttention request carrying flags to the client
 *
 * The attention is a separate 16-byte header plus the user bytes, limited by
 * the attention quantum. The block lives on the stack, because a signal
 * handler may trigger it while the DSI buffers are in use. A sleeping session
 * skips the send. A session in the middle of a reply refuses it, because the
 * handler must not interleave with that write.
 *
 * @param[in,out] dsi    DSI session
 * @param[in]     flags  AFP attention user bytes, in host order
 * @returns bytes sent, 1 for a sleeping session, or -1 when a reply is in
 *          progress or the send failed
 */
int dsi_attention(DSI *dsi, AFPUserBytes flags)
{
    /* header + AFPUserBytes */
    char block[DSI_BLOCKSIZ + sizeof(AFPUserBytes)];
    uint32_t len, nlen;
    uint16_t id;

    if (dsi->flags & DSI_SLEEPING) {
        return 1;
    }

    if (dsi->in_write) {
        return -1;
    }

    id = htons(dsi_serverID(dsi));
    flags = htons(flags);
    len = MIN(sizeof(flags), dsi->attn_quantum);
    nlen = htonl(len);
    memset(block, 0, sizeof(block));
    /* sending a request */
    block[0] = DSIFL_REQUEST;
    /* it's an attention */
    block[1] = DSIFUNC_ATTN;
    memcpy(block + 2, &id, sizeof(id));
    /* code = 0 */
    memcpy(block + 8, &nlen, sizeof(nlen));
    memcpy(block + 16, &flags, sizeof(flags));
    /* reserved = 0 */
    /* send an attention */
    return dsi_stream_write(dsi, block, DSI_BLOCKSIZ + len, DSI_NOWAIT);
}
