/*
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief ASP tickles
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <atalk/logger.h>
#include <sys/types.h>
#include <errno.h>

#include <sys/socket.h>
#include <atalk/atp.h>
#include <atalk/asp.h>

/*!
 * @brief Send one ASP tickle for a session, with a single try and no wait for a
 * reply
 *
 * @param[in] asp  server object whose ATP socket sends the tickle
 * @param[in] sid  session ID
 * @param[in] sat  address of the workstation
 * @returns 1, or 0 when the send fails
 */
int asp_tickle(ASP asp, const uint8_t sid, struct sockaddr_at *sat)
{
    struct atp_block atpb;
    char buf[ASP_HDRSIZ];
    buf[0] = ASPFUNC_TICKLE;
    buf[1] = sid;
    buf[2] = buf[3] = 0;
    atpb.atp_saddr = sat;
    atpb.atp_sreqdata = buf;
    atpb.atp_sreqdlen = sizeof(buf);
    atpb.atp_sreqto = 0;
    atpb.atp_sreqtries = 1;

    if (atp_sreq(asp->asp_atp, &atpb, 0, 0) < 0) {
        LOG(log_error, logtype_default, "atp_sreq: %s", strerror(errno));
        return 0;
    }

    return 1;
}
