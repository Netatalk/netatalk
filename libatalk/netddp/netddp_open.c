/* 
 * $Id: netddp_open.c,v 1.9 2005-04-28 20:50:02 bfernhomberg Exp $
 *
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 *
 * open a ddp socket and return the port and address assigned. return
 * various address info if requested as well.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netatalk/at.h>
#include <atalk/netddp.h>

int netddp_open(struct sockaddr_at *addr, struct sockaddr_at *bridge)
{

#ifdef NO_DDP
    return -1;
#else /* !NO_DDP */

    int s;
    socklen_t len;

    if ((s = socket( AF_APPLETALK, SOCK_DGRAM, 0 )) < 0) 
	return -1;
    
    if (!addr)
	return s;

    addr->sat_family = AF_APPLETALK;
    /* rest of address should be initialized by the caller */
    if (bind(s, (struct sockaddr *) addr, sizeof( struct sockaddr_at )) < 0 ) {
        close(s);
	return -1;
    }

    /* get the real address from the kernel */
    len = sizeof( struct sockaddr_at);
    if ( getsockname( s, (struct sockaddr *) addr, &len ) != 0 ) {
        close(s);
	return -1;
    }

    return s;
#endif /* NO_DDP */
}
