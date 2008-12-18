/*
 * $Id: at.h,v 1.6 2008-12-18 17:31:31 morgana Exp $
 *
 * Copyright (c) 1990,1991 Regents of The University of Michigan.
 *
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifndef __AT_HEADER__
#define __AT_HEADER__

#if defined(linux) /* pull in the linux header */
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/atalk.h>

#ifdef HAVE_ATALK_ADDR
#define at_addr atalk_addr
#define netrange atalk_netrange
#endif /* HAVE_ATALK_ADDR */

#else /* linux */

#include <sys/types.h>
#include <netinet/in.h> /* so that we can deal with sun's s_net #define */

#ifdef MACOSX_SERVER
#include <netat/appletalk.h>
#endif /* MACOSX_SERVER */

/*
 * Supported protocols
 */
#ifdef ATPROTO_DDP
#undef ATPROTO_DDP
#endif /* ATPROTO_DDP */
#define ATPROTO_DDP	0
#define ATPROTO_AARP	254

/*
 * Ethernet types, for DIX.
 * These should really be in some global header file, but we can't
 * count on them being there, and it's annoying to patch system files.
 */
#ifndef ETHERTYPE_AT
#define ETHERTYPE_AT	0x809B		/* AppleTalk protocol */
#endif
#ifndef ETHERTYPE_AARP
#define ETHERTYPE_AARP	0x80F3		/* AppleTalk ARP */
#endif

#define DDP_MAXSZ	587

/*
 * If ATPORT_FIRST <= Port < ATPORT_RESERVED,
 * Port was created by a privileged process.
 * If ATPORT_RESERVED <= Port < ATPORT_LAST,
 * Port was not necessarily created by a
 * privileged process.
 */
#define ATPORT_FIRST	1
#define ATPORT_RESERVED	128
#define ATPORT_LAST	254 /* 254 is reserved on ether/tokentalk networks */

/*
 * AppleTalk address.
 */
#ifndef MACOSX_SERVER
struct at_addr {
#ifdef s_net
#undef s_net
#endif /* s_net */
    u_short	s_net;
    u_char	s_node;
};
#endif /* MACOSX_SERVER */

#define ATADDR_ANYNET	(u_short)0x0000
#define ATADDR_ANYNODE	(u_char)0x00
#define ATADDR_ANYPORT	(u_char)0x00
#define ATADDR_BCAST	(u_char)0xff		/* There is no BCAST for NET */

/*
 * Socket address, AppleTalk style.  We keep magic information in the 
 * zero bytes.  There are three types, NONE, CONFIG which has the phase
 * and a net range, and IFACE which has the network address of an
 * interface.  IFACE may be filled in by the client, and is filled in
 * by the kernel.
 */
#ifndef MACOSX_SERVER
struct sockaddr_at {
#ifdef BSD4_4
    u_char		sat_len;
    u_char		sat_family;
#else /* BSD4_4 */
    short		sat_family;
#endif /* BSD4_4 */
    u_char		sat_port;
    struct at_addr	sat_addr;
#ifdef notdef
    struct {
	u_char		sh_type;
# define SATHINT_NONE	0
# define SATHINT_CONFIG	1
# define SATHINT_IFACE	2
	union {
	    char		su_zero[ 7 ];	/* XXX check size */
	    struct {
		u_char		sr_phase;
		u_short		sr_firstnet, sr_lastnet;
	    } su_range;
	    u_short		su_interface;
	} sh_un;
    } sat_hints;
#else /* notdef */
    char		sat_zero[ 8 ];
#endif /* notdef */
};
#endif /* MACOSX_SERVER */

struct netrange {
    u_char		nr_phase;
    u_short		nr_firstnet;
    u_short		nr_lastnet;
};

#ifdef KERNEL
extern struct domain	atalkdomain;
extern struct protosw	atalksw[];
#endif /* KERNEL */

#endif /* linux */
#endif /* __AT_HEADER__ */

