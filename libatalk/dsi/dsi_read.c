/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

/*!
 * @file
 * @brief Streamed FPRead replies over DSI
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <atalk/dsi.h>
#include <atalk/logger.h>
#include <atalk/util.h>

/*!
 * @brief Start a streamed FPRead reply of size bytes and send its first buflen
 * bytes
 *
 * The reply header carries the full size and the result code err. DSI_NOREPLY
 * tells the command loop that the handler sends the reply itself, and in_write
 * blocks attentions until dsi_readdone. The remaining size minus buflen bytes
 * follow through dsi_read or dsi_stream_read_file.
 *
 * @param[in,out] dsi     DSI session
 * @param[in]     buf     first part of the reply payload
 * @param[in]     buflen  bytes in buf
 * @param[in]     size    total reply payload length
 * @param[in]     err     AFP result code
 * @returns bytes to send next, at most buflen, or -1 when the send failed
 */
ssize_t dsi_readinit(DSI *dsi, void *buf, const size_t buflen,
                     const size_t size, const int err)
{
    LOG(log_maxdebug, logtype_dsi,
        "dsi_readinit: sending %zd bytes from buffer, total size: %zd",
        buflen, size);
    dsi->flags |= DSI_NOREPLY; /* we will handle our own replies */
    dsi->header.dsi_flags = DSIFL_REPLY;
    dsi->header.dsi_len = htonl(size);
    dsi->header.dsi_data.dsi_code = htonl(err);
    dsi->in_write++;

    if (dsi_stream_send(dsi, buf, buflen)) {
        dsi->datasize = size - buflen;
        LOG(log_maxdebug, logtype_dsi, "dsi_readinit: remaining data for sendfile: %zd",
            dsi->datasize);
        return MIN(dsi->datasize, buflen);
    }

    return -1; /* error */
}

/*!
 * @brief End a streamed FPRead reply that dsi_readinit started
 *
 * @param[in,out] dsi  DSI session whose in_write count drops
 */
void dsi_readdone(DSI *dsi)
{
    dsi->in_write--;
}

/*!
 * @brief Send the next buflen bytes of a streamed FPRead reply
 *
 * @param[in,out] dsi     DSI session after dsi_readinit
 * @param[in]     buf     payload bytes
 * @param[in]     buflen  bytes in buf
 * @returns bytes to send next, at most buflen, or -1 when the send failed
 */
ssize_t dsi_read(DSI *dsi, void *buf, const size_t buflen)
{
    size_t len;
    len  = dsi_stream_write(dsi, buf, buflen, 0);

    if (len == buflen) {
        dsi->datasize -= len;
        return MIN(dsi->datasize, buflen);
    }

    return -1;
}
