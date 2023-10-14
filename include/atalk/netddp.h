/*
 * $Id: netddp.h,v 1.4 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 * this provides a generic interface to the ddp layer. with this, we
 * should be able to interact with any appletalk stack that allows
 * direct access to the ddp layer. right now, only os x server's ddp
 * layer and the generic socket based interfaces are understood.  
 */

#ifndef _ATALK_NETDDP_H
#define _ATALK_NETDDP_H 1

#ifndef NO_DDP

#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netatalk/at.h>

extern int netddp_open   (struct sockaddr_at *, struct sockaddr_at *);

#include <unistd.h>
#include <sys/types.h>

#define netddp_close(a)  close(a)
#define netddp_sendto    sendto
#define netddp_recvfrom  recvfrom

#endif  /* NO_DDP */
#endif /* netddp.h */

