/* 
 * $Id: netddp_sendto.c,v 1.7 2005-04-28 20:50:02 bfernhomberg Exp $
 *
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 *
 * send data.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef NO_DDP
#include <string.h>
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
int netddp_sendto(int fd, void *buf, size_t buflen, unsigned int dummy, 
		  const struct sockaddr *addr, unsigned int addrlen)
{
    ssize_t i;
    struct ddpehdr ddphdr;
    const struct sockaddr_at *sat = (const struct sockaddr_at *) addr;
    struct iovec iov[2];

    iov[0].iov_base = (void *) &ddphdr;
    iov[0].iov_len = sizeof(ddphdr);
    iov[1].iov_base = buf;
    iov[1].iov_len = buflen;

    if (!addr)
      return -1;

    memset(&ddphdr, 0, sizeof(ddphdr));
    ddphdr.deh_len = htons(sizeof(ddphdr) + (u_int16_t) buflen);
    ddphdr.deh_dnet = sat->sat_addr.s_net;
    ddphdr.deh_dnode = sat->sat_addr.s_node;
    ddphdr.deh_dport = sat->sat_port;
    while ((i = writev(fd, iov, 2)) < 0) {
      if (errno != EINTR)
	return -1;
    }

    return MAX(0, i - sizeof(ddphdr));
}

#endif /* os x server */
#endif /* no ddp */
