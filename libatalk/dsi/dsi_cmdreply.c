/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <atalk/dsi.h>
#include <netatalk/endian.h>

/* this assumes that the reply follows right after the command, saving
 * on a couple assignments. specifically, command, requestID, and
 * reserved field are assumed to already be set. */ 
int dsi_cmdreply(DSI *dsi, const int err)
{
  dsi->header.dsi_flags = DSIFL_REPLY;
  /*dsi->header.dsi_command = DSIFUNC_CMD;*/
  dsi->header.dsi_len = htonl(dsi->datalen);
  dsi->header.dsi_code = htonl(err);

  return dsi_stream_send(dsi, dsi->data, dsi->datalen);
}
