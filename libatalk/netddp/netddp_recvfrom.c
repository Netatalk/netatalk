/* 
 * $Id: netddp_recvfrom.c,v 1.5 2003-02-17 02:02:25 srittau Exp $
 *
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 *
 * receive data.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef NO_DDP
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>

#ifdef MACOSX_SERVER
#include <netat/appletalk.h>
#include <netat/ddp.h>
#endif /* MACOSX_SERVER */

#include <netatalk/at.h>
#include <netatalk/endian.h>
#include <netatalk/ddp.h>
#include <atalk/netddp.h>

#ifndef MAX
#define MAX(a, b)  ((a) < (b) ? (b) : (a))
#endif /* ! MAX */

#ifdef MACOSX_SERVER
int netddp_recvfrom(int fd, void *buf, int buflen, unsigned int dummy, 
		     struct sockaddr *addr, unsigned int *addrlen)
{
    ssize_t i;
    struct ddpehdr ddphdr;
    struct sockaddr_at *sat = (struct sockaddr_at *) addr;
    struct iovec iov[2];

    iov[0].iov_base = (void *) &ddphdr;
    iov[0].iov_len = sizeof(ddphdr);
    iov[1].iov_base = buf;
    iov[1].iov_len = buflen;

    while ((i = readv(fd, iov, 2)) < 0) {
      if (errno != EINTR)
	return -1;
    }

    if (addr) {
      sat->sat_addr.s_net = ddphdr.deh_snet;
      sat->sat_addr.s_node = ddphdr.deh_snode;
      sat->sat_port = ddphdr.deh_sport;
    }

    return MAX(0, i - sizeof(ddphdr));
}

#endif /* os x server */
#endif /* no ddp */
