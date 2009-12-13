/*
 * $Id: interface.h,v 1.6 2009-12-13 02:21:47 didg Exp $
 * Copyright (c) 1990,1992 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifndef ATALKD_INTERFACE_H
#define ATALKD_INTERFACE_H 1

#include <sys/cdefs.h>

struct interface {
    struct interface	*i_next;
    char		i_name[ IFNAMSIZ ];
    int			i_flags;
    int			i_time;
    int                 i_group; /* for isolated appletalk domains */
    struct sockaddr_at	i_addr;
    struct sockaddr_at	i_caddr;
    struct ziptab	*i_czt;
    struct rtmptab	*i_rt;
    struct gate		*i_gate;
    struct atport	*i_ports;
};

#define IFACE_PHASE1	0x001
#define IFACE_PHASE2	0x002
#define IFACE_LOOPBACK	0x004		/* is the loopback interface */
#define IFACE_SEED	0x008		/* act as seed */
#define IFACE_ADDR	0x010		/* has an address set */
#define IFACE_CONFIG	0x020		/* has been configured */
#define IFACE_NOROUTER	0x040		/* no router on interface */
#define IFACE_LOOP	0x080		/* has a loopback route */
#define IFACE_RSEED     0x100           /* almost the same as seed. RSEED
					   says that we should try to 
					   do routing. */
#define IFACE_DONTROUTE 0x200           /* don't route this interface */
#define IFACE_ISROUTER  0x400           /* act as a router. */
#define IFACE_ALLMULTI  0x800		/* set allmulti on this interface, linux only */
#define IFACE_WASALLMULTI 0x1000	/* don't unset allmulti on this interface on shutdown, linux only */
#define IFACE_ERROR 	0x2000		/* sendto returned an error */

#define UNSTABLE	2
#define STABLE		0
#define STABLEANYWAY	-2

#define IFBASE		2	/* base number of interfaces */

#ifdef linux
#define LOOPIFACE	"lo"
#else /* !linux */
#define LOOPIFACE	"lo0"
#endif /* linux */

extern struct interface	*interfaces;
extern struct interface	*ciface;
struct interface	*newiface (const char *);

#endif /* ATALKD_INTERFACE_H */
