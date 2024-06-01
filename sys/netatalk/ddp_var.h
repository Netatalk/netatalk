/*
 * Copyright (c) 1990,1994 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef _NETATALK_DDP_VAR_H
#define _NETATALK_DDP_VAR_H 1

#include <netatalk/endian.h>

struct ddpcb {
    struct sockaddr_at	ddp_fsat, ddp_lsat;
    struct route	ddp_route;
    struct socket	*ddp_socket;
    struct ddpcb	*ddp_prev, *ddp_next;
    struct ddpcb	*ddp_pprev, *ddp_pnext;
};

#define sotoddpcb(so)	((struct ddpcb *)(so)->so_pcb)

struct ddpstat {
    uint32_t	ddps_short;		/* short header packets received */
    uint32_t	ddps_long;		/* long header packets received */
    uint32_t	ddps_nosum;		/* no checksum */
    uint32_t	ddps_badsum;		/* bad checksum */
    uint32_t	ddps_tooshort;		/* packet too short */
    uint32_t	ddps_toosmall;		/* not enough data */
    uint32_t	ddps_forward;		/* packets forwarded */
    uint32_t	ddps_encap;		/* packets encapsulated */
    uint32_t	ddps_cantforward;	/* packets rcvd for unreachable dest */
    uint32_t	ddps_nosockspace;	/* no space in sockbuf for packet */
};

#ifdef KERNEL
struct ddpcb		*ddp_ports[ ATPORT_LAST ];
struct ddpcb		*ddpcb;
struct ddpstat		ddpstat;
struct ddpcb		*ddp_search();
#endif /* KERNEL */

#endif /* netatalk/ddp_var.h */
