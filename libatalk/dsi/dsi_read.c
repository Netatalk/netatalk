/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>

#include <atalk/dsi.h>

#ifndef min
#define min(a,b)   ((a) < (b) ? (a) : (b))
#endif

/* streaming i/o for afp_read. this is all from the perspective of the
 * client. it basically does the reverse of dsi_write. on first entry,
 * it will send off the header plus whatever is in its command
 * buffer. it returns the amount of stuff still to be read
 * (constrained by the buffer size). */
ssize_t dsi_readinit(DSI *dsi, void *buf, const size_t buflen,
		    const size_t size, const int err)
{
  const struct itimerval none = {{0, 0}, {0, 0}};

  dsi->noreply = 1; /* we will handle our own replies */
  dsi->header.dsi_flags = DSIFL_REPLY;
  /*dsi->header.dsi_command = DSIFUNC_CMD;*/
  dsi->header.dsi_len = htonl(size);
  dsi->header.dsi_code = htonl(err);

  sigprocmask(SIG_BLOCK, &dsi->sigblockset, NULL);
  setitimer(ITIMER_REAL, &none, &dsi->savetimer);
  if (dsi_stream_send(dsi, buf, buflen)) {
    dsi->datasize = size - buflen;
    return min(dsi->datasize, buflen);
  }

  return -1; /* error */
}

void dsi_readdone(DSI *dsi)
{
  setitimer(ITIMER_REAL, &dsi->savetimer, NULL);
  sigprocmask(SIG_UNBLOCK, &dsi->sigblockset, NULL);
}

/* send off the data */
ssize_t dsi_read(DSI *dsi, void *buf, const size_t buflen)
{
  size_t len = dsi_stream_write(dsi, buf, buflen);

  if (len == buflen) {
    dsi->datasize -= len;
    return min(dsi->datasize, buflen);
  }

  return -1;
}
