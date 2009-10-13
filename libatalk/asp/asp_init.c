/*
 * $Id: asp_init.c,v 1.4 2009-10-13 22:55:37 didg Exp $
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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netatalk/at.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <stdio.h>
#include <stdlib.h>

ASP asp_init(ATP atp)
{
    ASP		asp;

    if (( asp = (struct ASP *)calloc(1, sizeof( struct ASP ))) == NULL ) {
	return( NULL );
    }

    asp->asp_atp = atp;
#ifdef BSD4_4
    asp->asp_sat.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    asp->asp_sat.sat_family = AF_APPLETALK;
    asp->asp_sat.sat_addr.s_net = ATADDR_ANYNET;
    asp->asp_sat.sat_addr.s_node = ATADDR_ANYNODE;
    asp->asp_sat.sat_port = ATADDR_ANYPORT;
    asp->asp_status = NULL;
    asp->asp_slen = 0;
    asp->asp_sid = 0;
    asp->asp_flags = ASPFL_SLS;
    asp->cmdlen = asp->datalen = 0;
    asp->read_count = asp->write_count = 0;
    asp->commands = asp->cmdbuf + 4;

    return( asp );
}

void asp_setstatus(ASP asp, char *status, const int slen)
{
    asp->asp_status = status;
    asp->asp_slen = slen;
}
