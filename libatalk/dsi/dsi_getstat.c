/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#include <stdio.h>
#include <string.h>

#include <atalk/dsi.h>
#include <netatalk/endian.h>

/* return the status and then delete the connection. most of the
 * fields are already set. */
void dsi_getstatus(DSI *dsi)
{
  dsi->header.dsi_flags = DSIFL_REPLY;
  /*dsi->header.dsi_command = DSIFUNC_STAT;*/
  
  memcpy(dsi->commands, dsi->status, dsi->statuslen);
  dsi->cmdlen = dsi->statuslen; 
  dsi_send(dsi);
}
