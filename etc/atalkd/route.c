/*
 * Copyright (c) 1990,1996 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <sys/ioctl.h>
#include <netatalk/at.h>

#include "rtmp.h"
#include "route.h"

#ifndef BSD4_4
int route(int message, struct sockaddr *dst, struct sockaddr *gate, int flags)
{
    struct rtentry	rtent;
    memset(&rtent, 0, sizeof(struct rtentry));
    rtent.rt_dst = *dst;
    rtent.rt_gateway = *gate;
    rtent.rt_flags = flags;
    return ioctl(rtfd, message, &rtent);
}

#else /* BSD4_4 */

struct sockaddr_m {
    unsigned char	sam_len;
    unsigned char	sam_family;
    unsigned short	sam_pad;
    unsigned short	sam_mask;
    unsigned short	sam_pad2;
} mask = { sizeof(struct sockaddr_m), 0, 0, 0xffff, 0 };

struct rt_msg_at {
    struct rt_msghdr	rtma_rtm;
    struct sockaddr_at	rtma_dst;
    struct sockaddr_at	rtma_gate;
    struct sockaddr_m	rtma_mask;
} rtma;

int route(int message, struct sockaddr_at *dst, struct sockaddr_at *gate,
          int flags)
{
    int			rc;
    memset(&rtma, 0, sizeof(struct rt_msg_at));
    rtma.rtma_rtm.rtm_msglen = sizeof(struct rt_msg_at);
    rtma.rtma_rtm.rtm_version = RTM_VERSION;
    rtma.rtma_rtm.rtm_type = message;
    rtma.rtma_rtm.rtm_pid = getpid();
    rtma.rtma_rtm.rtm_addrs = RTA_DST | RTA_GATEWAY;

    if (flags & RTF_HOST) {
        rtma.rtma_rtm.rtm_msglen = sizeof(struct rt_msg_at) -
                                   sizeof(struct sockaddr_m);
    } else {
        rtma.rtma_rtm.rtm_msglen = sizeof(struct rt_msg_at);
        rtma.rtma_rtm.rtm_addrs |= RTA_NETMASK;
        rtma.rtma_mask = mask;
    }

    rtma.rtma_rtm.rtm_flags = flags;
    rtma.rtma_dst = *dst;
    rtma.rtma_gate = *gate;

    if ((rc = write(rtfd, &rtma, rtma.rtma_rtm.rtm_msglen)) !=
            rtma.rtma_rtm.rtm_msglen) {
        return -1;
    }

    return 0;
}
#endif /* BSD4_4 */
