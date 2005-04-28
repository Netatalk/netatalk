/*
 * $Id: dsi_cmdreply.c,v 1.4 2005-04-28 20:50:02 bfernhomberg Exp $
 *
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <atalk/dsi.h>
#include <netatalk/endian.h>

/* this assumes that the reply follows right after the command, saving
 * on a couple assignments. specifically, command, requestID, and
 * reserved field are assumed to already be set. */ 
int dsi_cmdreply(DSI *dsi, const int err)
{
int ret;
  dsi->header.dsi_flags = DSIFL_REPLY;
  /*dsi->header.dsi_command = DSIFUNC_CMD;*/
  dsi->header.dsi_len = htonl(dsi->datalen);
  dsi->header.dsi_code = htonl(err);

  ret = dsi_stream_send(dsi, dsi->data, dsi->datalen);
  if (dsi->sigblocked)  {
      sigprocmask(SIG_SETMASK, &dsi->oldset, NULL);
      dsi->sigblocked = 0;
  }
  return ret;
}
