/*
 * Copyright (c) 1996 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief ASP CloseSession requests to the workstation
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
 * @brief Ask the workstation to close a session with an ASP CloseSession
 * request
 *
 * The request goes to the session socket of the workstation as an exactly
 * once transaction, with 5 tries 2 seconds apart. The call waits for the
 * reply.
 *
 * @param[in,out] asp  session, its command buffer reused for the request
 * @returns 0, or -1 when the request or the reply fails
 */
int asp_shutdown(ASP asp)
{
    struct atp_block	atpb;
    struct iovec	iov;
    char		*p;
    uint16_t		seq;
    uint8_t		oport;
    p = asp->commands;
    *p++ = ASPFUNC_CLOSE;
    *p++ = asp->asp_sid;
    seq = 0;
    memcpy(p, &seq, sizeof(seq));
    p += sizeof(seq);
    oport = asp->asp_sat.sat_port;
    atpb.atp_saddr = &asp->asp_sat;
    atpb.atp_saddr->sat_port = asp->asp_wss;
    atpb.atp_sreqdata = asp->commands;
    atpb.atp_sreqdlen = p - asp->commands;
    atpb.atp_sreqto = 2;
    atpb.atp_sreqtries = 5;

    if (atp_sreq(asp->asp_atp, &atpb, 1, ATP_XO) < 0) {
        asp->asp_sat.sat_port = oport;
        return -1;
    }

    iov.iov_base = asp->commands;
    iov.iov_len = ASP_CMDSIZ;
    atpb.atp_rresiov = &iov;
    atpb.atp_rresiovcnt = 1;

    if (atp_rresp(asp->asp_atp, &atpb) < 0) {
        asp->asp_sat.sat_port = oport;
        return -1;
    }

    asp->asp_sat.sat_port = oport;
    return 0;
}
