/*
 * $Id: dsi_tickle.c,v 1.8 2009-10-25 06:13:11 didg Exp $
 *
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>

#include <atalk/dsi.h>
#include <netatalk/endian.h>

/* server generated tickles. as this is only called by the tickle handler,
 * we don't need to block signals. */
int dsi_tickle(DSI *dsi)
{
  char block[DSI_BLOCKSIZ];
  u_int16_t id;
  int ret;
  
  if (dsi->asleep || dsi->in_write)
      return 1;

  id = htons(dsi_serverID(dsi));

  memset(block, 0, sizeof(block));
  block[0] = DSIFL_REQUEST;
  block[1] = DSIFUNC_TICKLE;
  memcpy(block + 2, &id, sizeof(id));
  /* code = len = reserved = 0 */

  ret = dsi_stream_write(dsi, block, DSI_BLOCKSIZ, DSI_NOWAIT);
  /* we don't really care if we can't send a tickle, it will fail
   * elsewhere
  */
  ret = (ret == -1 || ret == DSI_BLOCKSIZ);
  return ret;
}

