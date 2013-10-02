/*
 *
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 *
 * 7 Oct 1997 added checks for 0 data.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/* this streams writes */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>

#include <atalk/dsi.h>
#include <atalk/util.h>
#include <atalk/logger.h>

size_t dsi_writeinit(DSI *dsi, void *buf, const size_t buflen _U_)
{
    size_t bytes = 0;
    dsi->datasize = ntohl(dsi->header.dsi_len) - dsi->header.dsi_data.dsi_doff;

    if (dsi->eof > dsi->start) {
        /* We have data in the buffer */
        bytes = MIN(dsi->eof - dsi->start, dsi->datasize);
        memmove(buf, dsi->start, bytes);
        dsi->start += bytes;
        dsi->datasize -= bytes;
        if (dsi->start >= dsi->eof)
            dsi->start = dsi->eof = dsi->buffer;
    }

    LOG(log_maxdebug, logtype_dsi, "dsi_writeinit: remaining DSI datasize: %jd", (intmax_t)dsi->datasize);

    return bytes;
}


/* fill up buf and then return. this should be called repeatedly
 * until all the data has been read. i block alarm processing 
 * during the transfer to avoid sending unnecessary tickles. */
size_t dsi_write(DSI *dsi, void *buf, const size_t buflen)
{
  size_t length;

  LOG(log_maxdebug, logtype_dsi, "dsi_write: remaining DSI datasize: %jd", (intmax_t)dsi->datasize);

  if ((length = MIN(buflen, dsi->datasize)) > 0) {
      if ((length = dsi_stream_read(dsi, buf, length)) > 0) {
          LOG(log_maxdebug, logtype_dsi, "dsi_write: received: %ju", (intmax_t)length);
          dsi->datasize -= length;
          return length;
      }
  }
  return 0;
}

/* flush any unread buffers. */
void dsi_writeflush(DSI *dsi)
{
  size_t length;

  while (dsi->datasize > 0) { 
    length = dsi_stream_read(dsi, dsi->data,
			     MIN(sizeof(dsi->data), dsi->datasize));
    if (length > 0)
      dsi->datasize -= length;
    else
      break;
  }
}
