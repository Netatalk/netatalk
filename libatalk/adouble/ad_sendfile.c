/*
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 *
 * NOTE: the following uses the fact that sendfile() only exists on
 * machines with SA_RESTART behaviour. this is all very machine specific. 
 *
 * 
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <atalk/adouble.h>

#include <syslog.h>

#include "ad_private.h"

static int _ad_sendfile_dummy;

#if defined(HAVE_SENDFILE_READ) || defined(HAVE_SENDFILE_WRITE)
static __inline__ int ad_sendfile_init(const struct adouble *ad, 
				       const int eid, off_t *off,
				       const int end)
{
  int fd;

  if (end) 
    *off = ad_size(ad, eid) - *off;

  if (eid == ADEID_DFORK) {
    fd = ad_dfileno(ad);
  } else {
    *off += ad_getentryoff(ad, eid);
    fd = ad_hfileno(ad);
  }

  return fd;
}
#endif


/* read from adouble file and write to socket. sendfile doesn't change
 * the file pointer position. */
#ifdef HAVE_SENDFILE_READ
ssize_t ad_readfile(const struct adouble *ad, const int eid, 
		    const int sock, off_t off, const size_t len)
{
  off_t cc;
  int fd;

  fd = ad_sendfile_init(ad, eid, &off, 0);
#ifdef __linux__
  cc = sendfile(sock, fd, &off, len);
#endif

#ifdef BSD4_4
  if (sendfile(fd, sock, off, len, NULL, &cc, 0) < 0)
    return -1;
#endif

  return cc;
}
#endif

#if 0
#ifdef HAVE_SENDFILE_WRITE
/* read from a socket and write to an adouble file */
ssize_t ad_writefile(struct adouble *ad, const int eid, 
		     const int sock, off_t off, const int end,
		     const size_t len)
{
#ifdef __linux__
  ssize_t cc;
  int fd;

  fd = ad_sendfile_init(ad, eid, &off, end);
  if ((cc = sendfile(fd, sock, &off, len)) < 0)
    return -1;

  if ((eid != ADEID_DFORK) && (off > ad_getentrylen(ad, eid))) 
    ad_setentrylen(ad, eid, off);

  return cc;
#endif
}
#endif
#endif
