/*
 * $Id: asp_close.c,v 1.5 2009-10-13 22:55:37 didg Exp $
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
#include <sys/uio.h>
#include <sys/socket.h>

#include <netatalk/at.h>
#include <atalk/atp.h>
#include <atalk/asp.h>

int asp_close(ASP asp)
{
    struct atp_block	atpb;
    struct iovec	iov[ 1 ];
    int err = 0;

    memset(asp->commands, 0, sizeof(u_int32_t));

    atpb.atp_saddr = &asp->asp_sat;
    iov[ 0 ].iov_base = asp->commands;
    iov[ 0 ].iov_len = sizeof(u_int32_t);
    atpb.atp_sresiov = iov;
    atpb.atp_sresiovcnt = 1;

    if (atp_sresp( asp->asp_atp, &atpb ) < 0)
      err = -1;

    if (atp_close( asp->asp_atp ) < 0)
      err = -1;

    free( asp );
    return err;
}
