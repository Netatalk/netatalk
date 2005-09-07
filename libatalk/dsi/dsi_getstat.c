/*
 * $Id: dsi_getstat.c,v 1.4 2005-09-07 15:27:29 didg Exp $
 *
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

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
  dsi->header.dsi_code = dsi->header.dsi_reserved = 0;
  
  memcpy(dsi->commands, dsi->status, dsi->statuslen);
  dsi->cmdlen = dsi->statuslen; 
  dsi_send(dsi);
}
