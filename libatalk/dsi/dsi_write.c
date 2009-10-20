/*
 * $Id: dsi_write.c,v 1.5 2009-10-20 04:31:41 didg Exp $
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <string.h>

#include <atalk/dsi.h>
#include <netatalk/endian.h>

#ifndef MIN
#define MIN(a,b)     ((a) < (b) ? (a) : (b))
#endif /* ! MIN */

/* initialize relevant things for dsi_write. this returns the amount
 * of data in the data buffer. the interface has been reworked to allow
 * for arbitrary buffers. */
size_t dsi_writeinit(DSI *dsi, void *buf, const size_t buflen _U_)
{
  size_t len, header;

  /* figure out how much data we have. do a couple checks for 0 
   * data */
  header = ntohl(dsi->header.dsi_code);
  dsi->datasize = header ? ntohl(dsi->header.dsi_len) - header : 0;
  if (dsi->datasize > 0) {
    len = MIN(sizeof(dsi->commands) - header, dsi->datasize);
    
    /* write last part of command buffer into buf */
    memcpy(buf, dsi->commands + header, len);
    
    /* recalculate remaining data */
    dsi->datasize -= len;
  } else
    len = 0;

  return len;
}

/* fill up buf and then return. this should be called repeatedly
 * until all the data has been read. i block alarm processing 
 * during the transfer to avoid sending unnecessary tickles. */
size_t dsi_write(DSI *dsi, void *buf, const size_t buflen)
{
  size_t length;

  if (((length = MIN(buflen, dsi->datasize)) > 0) &&
      ((length = dsi_stream_read(dsi, buf, length)) > 0)) {
    dsi->datasize -= length;
    return length;
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
