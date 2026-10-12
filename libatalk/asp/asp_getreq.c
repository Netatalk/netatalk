/*
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
 *
 */

/*!
 * @file
 * @brief Reading an ASP request from the session socket
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <netatalk/at.h>
#include <atalk/atp.h>
#include <atalk/asp.h>

/*!
 * @brief Read one ASP request from the session socket without blocking
 *
 * The call reads no more than one datagram. ASP_NOREQUEST means that a datagram
 * went by without a usable request, and the caller waits for the socket to
 * turn readable again. A request shorter than the ASP header, or a command or
 * write without a payload, counts as no request. Every function except
 * ASPFUNC_CLOSE has to carry the next sequence number.
 *
 * The session ID and the function byte compare as unsigned bytes. A session ID
 * of 0x80 or above therefore matches, and a function byte from 0xFD to 0xFF
 * never reads as an error code.
 *
 * @param[in,out] asp  session, receives the request in cmdbuf and its payload
 *                     length in cmdlen
 * @returns the ASP function byte, ASP_NOREQUEST, ASP_ERR_READ, ASP_ERR_SEQ for
 *          a wrong sequence number, or ASP_ERR_SID for another session
 */
int asp_getrequest(ASP asp)
{
    struct atp_block	atpb;
    uint16_t		seq;
    int			rc;
    asp->asp_sat.sat_port = ATADDR_ANYPORT;
    atpb.atp_saddr = &asp->asp_sat;
    atpb.atp_rreqdata = asp->cmdbuf;
    atpb.atp_rreqdlen = sizeof(asp->cmdbuf);
    rc = atp_rreq_try(asp->asp_atp, &atpb);

    if (rc < 0) {
        return ASP_ERR_READ;
    }

    if (rc == 0) {
        return ASP_NOREQUEST;
    }

    /* cmdlen is size_t, so a request shorter than the ASP header would wrap it
     * to near SIZE_MAX and be passed to the command handlers as their input
     * length. Such a packet is malformed; discard it. */
    if (atpb.atp_rreqdlen < ASP_HDRSIZ) {
        return ASP_NOREQUEST;
    }

    asp->cmdlen = (size_t) atpb.atp_rreqdlen - ASP_HDRSIZ;

    /* The header alone is a valid frame for the control functions, but not for
     * the two that carry an AFP call: the dispatcher reads commands[0] to pick
     * it, so with no payload it would select the call from whatever the previous
     * request left in the buffer and run it with an input length of 0. */
    if (asp->cmdlen < 1
            && ((unsigned char) asp->cmdbuf[0] == ASPFUNC_CMD
                || (unsigned char) asp->cmdbuf[0] == ASPFUNC_WRITE)) {
        return ASP_NOREQUEST;
    }

    asp->read_count += asp->cmdlen;
    memcpy(&seq, asp->cmdbuf + 2, sizeof(seq));
    seq = ntohs(seq);

    if (((unsigned char) asp->cmdbuf[0] != ASPFUNC_CLOSE)
            && (seq != asp->asp_seq)) {
        return ASP_ERR_SEQ;
    }

    if ((unsigned char) asp->cmdbuf[1] != asp->asp_sid) {
        return ASP_ERR_SID;
    }

    return (unsigned char) asp->cmdbuf[0]; /* the command */
}
