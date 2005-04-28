/*
 * $Id: ad_sendfile.c,v 1.8 2005-04-28 20:49:52 bfernhomberg Exp $
 *
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 *
 * NOTE: the following uses the fact that sendfile() only exists on
 * machines with SA_RESTART behaviour. this is all very machine specific. 
 *
 * sendfile chainsaw from samba.
 Unix SMB/Netbios implementation.
 Version 2.2.x / 3.0.x
 sendfile implementations.
 Copyright (C) Jeremy Allison 2002.
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef WITH_SENDFILE

#include <atalk/adouble.h>

#include <stdio.h>

#include <sys/socket.h>
#include <sys/uio.h>

#include <errno.h>  

#include <atalk/logger.h>

#include "ad_private.h"

#if defined(LINUX_BROKEN_SENDFILE_API)

extern int32_t sendfile (int fdout, int fdin, int32_t *offset, u_int32_t count);

ssize_t sys_sendfile(int tofd, int fromfd, off_t *offset, size_t count)
{
u_int32_t small_total = 0;
int32_t small_offset;

    /*
     * Fix for broken Linux 2.4 systems with no working sendfile64().
     * If the offset+count > 2 GB then pretend we don't have the
     * system call sendfile at all. The upper layer catches this
     * and uses a normal read. JRA.
     */
 
     if ((sizeof(off_t) >= 8) && (*offset + count > (off_t)0x7FFFFFFF)) {
         errno = ENOSYS;
         return -1;
     }
     small_total = (u_int32_t)count;
     small_offset = (int32_t)*offset;
 
     while (small_total) {
         int32_t nwritten;
         do {
             nwritten = sendfile(tofd, fromfd, &small_offset, small_total);
         } while (nwritten == -1 && errno == EINTR);
         if (nwritten == -1)
             return -1;
         if (nwritten == 0)
             return -1; /* I think we're at EOF here... */
         small_total -= nwritten;
    }
    *offset += count;
    return count;
}

#elif defined(SENDFILE_FLAVOR_LINUX)
#include <sys/sendfile.h>

ssize_t sys_sendfile(int tofd, int fromfd, off_t *offset, size_t count)
{
size_t total=0;

    total = count;
    while (total) {
        ssize_t nwritten;
        do {
            nwritten = sendfile(tofd, fromfd, offset, total);
        } while (nwritten == -1 && errno == EINTR);
        if (nwritten == -1)
            return -1;
        if (nwritten == 0)
            return -1; /* I think we're at EOF here... */
        total -= nwritten;
    }
    return count;
}


#elif defined(SENDFILE_FLAVOR_BSD )
/* FIXME untested */
#include <sys/sendfile.h>
ssize_t sys_sendfile(int tofd, int fromfd, off_t *offset, size_t count)
{
size_t total=0;
int    ret;

    total = count;
    while (total) {
        ssize_t nwritten;
        do {
           ret = sendfile(fromfd, tofd, offset, count, NULL, &nwritten, 0);
        while (ret == -1 && errno == EINTR);
        if (ret == -1)
            return -1;
        total -= nwritten;
        offset += nwritten;
    }
    return count;
}

#else

ssize_t sys_sendfile(int out_fd, int in_fd, off_t *_offset, size_t count)
{
    /* No sendfile syscall. */
    errno = ENOSYS;
    return -1;
}
#endif

/* ------------------------------- */
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


/* --------------------------------
 * read from adouble file and write to socket. sendfile doesn't change
 * the file pointer position. */
ssize_t ad_readfile(const struct adouble *ad, const int eid, 
		    const int sock, off_t off, const size_t len)
{
  off_t cc;
  int fd;

  fd = ad_sendfile_init(ad, eid, &off, 0);
  cc = sys_sendfile(sock, fd, &off, len);
  return cc;
}

/* ------------------------ */
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
  if ((cc = sys_sendfile(fd, sock, &off, len)) < 0)
    return -1;

  if ((eid != ADEID_DFORK) && (off > ad_getentrylen(ad, eid))) 
    ad_setentrylen(ad, eid, off);

  return cc;
#endif /* __linux__ */
}
#endif /* HAVE_SENDFILE_WRITE */
#endif /* 0 */
#endif
