/*
 * $Id: atp_open.c,v 1.5 2001-08-15 02:17:57 srittau Exp $
 *
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation, and that the name of The University
 * of Michigan not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. This software is supplied as is without expressed or
 * implied warranties of any kind.
 *
 *	Research Systems Unix Group
 *	The University of Michigan
 *	c/o Mike Clark
 *	535 W. William Street
 *	Ann Arbor, Michigan
 *	+1-313-763-0525
 *	netatalk@itd.umich.edu
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <netatalk/at.h>
#include <netatalk/endian.h>

#include <atalk/netddp.h>
#include <atalk/ddp.h>
#include <atalk/atp.h>

#include "atp_internals.h"

ATP atp_open(u_int8_t port, const struct at_addr *saddr)
{
    struct sockaddr_at  addr;
    int			s;
    ATP			atp;
    struct timeval	tv;
    int			pid;

#ifdef DEBUG
    printf( "<%d> atp_open\n", getpid());
#endif /* DEBUG */

    memset(&addr, 0, sizeof(addr));
    addr.sat_port = port;
    if (saddr) 
      memcpy(&addr.sat_addr, saddr, sizeof(struct at_addr));
    if ((s = netddp_open(&addr, NULL)) < 0)
        return NULL;

    if (( atp = (ATP) atp_alloc_buf()) == NULL ) {
        netddp_close(s);
	return NULL;
    }

    /* initialize the atp handle */
    memset(atp, 0, sizeof( struct atp_handle ));
    memcpy(&atp->atph_saddr, &addr, sizeof(addr));

    atp->atph_socket = s;
    atp->atph_reqto = -1;
    gettimeofday( &tv, (struct timezone *) 0 );
    pid = getpid();
    atp->atph_tid = tv.tv_sec ^ ((( pid << 8 ) & 0xff00 ) | ( pid >> 8 ));

#ifdef EBUG
srandom( tv.tv_sec );
#endif /* EBUG */

    return atp;
}
