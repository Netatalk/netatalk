/*
 * $Id: asp_attn.c,v 1.7 2002-12-04 10:59:37 didg Exp $
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <atalk/logger.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>

#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/afp.h>

/* attentions can get sent at any time. as a consequence, we don't
 * want to touch anything that might be used elsewhere. */
int asp_attention(ASP asp, AFPUserBytes flags)
{
    char cmds[ASP_HDRSIZ], data[ASP_HDRSIZ];
    struct sockaddr_at  sat;
    struct atp_block	atpb;
    struct iovec	iov[ 1 ];

    cmds[0] = ASPFUNC_ATTN;
    cmds[1] = asp->asp_sid;
    flags = htons(flags);
    memcpy(cmds + 2, &flags, sizeof(flags));

    sat = asp->asp_sat;
    sat.sat_port = asp->asp_wss;
    atpb.atp_saddr = &sat;
    atpb.atp_sreqdata = cmds;
    atpb.atp_sreqdlen = sizeof(cmds);
    atpb.atp_sreqto = 2;
    atpb.atp_sreqtries = 5;

    if ( atp_sreq( asp->asp_atp, &atpb, 1, 0 ) < 0 ) {
	LOG(log_error, logtype_default, "atp_sreq: %s", strerror(errno) );
	return 0;
    }

    iov[ 0 ].iov_base = data;
    iov[ 0 ].iov_len = sizeof( data );
    atpb.atp_rresiov = iov;
    atpb.atp_rresiovcnt = sizeof( iov )/sizeof( iov[ 0 ] );
    if ( atp_rresp( asp->asp_atp, &atpb ) < 0 ) {
	LOG(log_error, logtype_default, "atp_rresp: %s", strerror(errno) );
	return 0;
    }

    return 1;
}
