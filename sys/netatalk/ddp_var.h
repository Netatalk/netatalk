/*
 * $Id: ddp_var.h,v 1.2 2001-06-29 14:14:47 rufustfirefly Exp $
 *
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
    u_int32_t	ddps_short;		/* short header packets received */
    u_int32_t	ddps_long;		/* long header packets received */
    u_int32_t	ddps_nosum;		/* no checksum */
    u_int32_t	ddps_badsum;		/* bad checksum */
    u_int32_t	ddps_tooshort;		/* packet too short */
    u_int32_t	ddps_toosmall;		/* not enough data */
    u_int32_t	ddps_forward;		/* packets forwarded */
    u_int32_t	ddps_encap;		/* packets encapsulated */
    u_int32_t	ddps_cantforward;	/* packets rcvd for unreachable dest */
    u_int32_t	ddps_nosockspace;	/* no space in sockbuf for packet */
};

#ifdef KERNEL
struct ddpcb		*ddp_ports[ ATPORT_LAST ];
struct ddpcb		*ddpcb;
struct ddpstat		ddpstat;
struct ddpcb		*ddp_search();
#endif /* KERNEL */

#endif /* netatalk/ddp_var.h */
