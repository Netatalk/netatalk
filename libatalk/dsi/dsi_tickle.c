/*
 * $Id: dsi_tickle.c,v 1.5 2003-03-12 15:07:07 didg Exp $
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
 * we don't need to block signals. well, actually, we might get it during
 * a SIGHUP. */
int dsi_tickle(DSI *dsi)
{
  char block[DSI_BLOCKSIZ];
  sigset_t oldset;
  u_int16_t id;
  int ret;
  
  if (dsi->asleep)
      return 1;

  id = htons(dsi_serverID(dsi));

  memset(block, 0, sizeof(block));
  block[0] = DSIFL_REQUEST;
  block[1] = DSIFUNC_TICKLE;
  memcpy(block + 2, &id, sizeof(id));
  /* code = len = reserved = 0 */

  sigprocmask(SIG_BLOCK, &dsi->sigblockset, &oldset);
  ret = dsi_stream_write(dsi, block, DSI_BLOCKSIZ) == DSI_BLOCKSIZ;
  sigprocmask(SIG_SETMASK, &oldset, NULL);
  return ret;
}

