/*
 * $Id: rtmp.h,v 1.5 2009-10-13 22:55:37 didg Exp $
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved. See COPYRIGHT.
 *
 * We have an rtmptab circular linked list for each gateway.  Entries
 * are inserted in the order we get them.  The expectation is that
 * we will get a complexity of N for the stable case.  If we have N
 * existing entries, and M new entries, we'll have on the order of
 * N + ( M * N ) complexity (really it will be something more than
 * that, maybe N + ( M * ( N + 1/2 M )).  Note that having a list to
 * search is superior to a hash table if you are expecting bad data:
 * you have the opportunity to range-check the incoming data.
 *
 * We keep several ZIP related flags and counters here.  For ZIP Extended
 * Replies, we must keep a flag indicating that the zone is up or down.
 * This flag is necessary for ZIP Extended Replies which cross packet
 * boundaries: even tho the rtmptab entry has data, it is not yet
 * complete.  For ZIP in general, we keep a flag indicating that we've
 * asked for a ZIP (E)Reply.  If this flag is not set, we won't process
 * ZIP Reply data for given rtmptab entries.  Lastly, we keep a count of
 * the number of times we've asked for ZIP Reply data.  When this value
 * reaches some value (3?), we can optionally stop asking.
 */

#ifndef ATALKD_RTMP_H
#define ATALKD_RTMP_H 1

#include <sys/cdefs.h>

struct rtmptab {
    struct rtmptab	*rt_next,
			*rt_prev;
    struct rtmptab	*rt_inext,
			*rt_iprev;
    u_short		rt_firstnet, rt_lastnet;
    u_char		rt_hops;
    u_char		rt_state;
    u_char		rt_flags;
    u_char		rt_nzq;		/* number of zip queries issued */
    struct gate		*rt_gate;	/* gate is NULL for interfaces */
    struct list		*rt_zt;
    const struct interface    *rt_iface;
};

struct rtmp_head {
    u_short	rh_net;
    u_char	rh_nodelen;
    u_char	rh_node;
};

struct rtmp_tuple {
    u_short	rt_net;
    u_char	rt_dist;
};
#define SZ_RTMPTUPLE	3

#define RTMPTAB_PERM	0
#define RTMPTAB_GOOD	1
#define RTMPTAB_SUSP1	2
#define RTMPTAB_SUSP2	3
#define RTMPTAB_BAD	4

#define RTMPTAB_ZIPQUERY	0x01
#define RTMPTAB_HASZONES	0x02
#define RTMPTAB_EXTENDED	0x04
#define RTMPTAB_ROUTE		0x08

#ifndef BSD4_4
#define RTMP_ADD	SIOCADDRT
#define RTMP_DEL	SIOCDELRT
#else /* BSD4_4 */
#define RTMP_ADD	RTM_ADD
#define RTMP_DEL	RTM_DELETE
#endif /* BSD4_4 */

#define STARTUP_FIRSTNET	0xff00
#define STARTUP_LASTNET		0xfffe

extern int	rtfd;
struct rtmptab	*newrt (const struct interface *);
void rtmp_delzonemap  (struct rtmptab *);

int rtmp_request ( struct interface * );
void rtmp_free ( struct rtmptab * );
int rtmp_replace ( struct rtmptab * );
int looproute ( struct interface *, unsigned int );
int gateroute ( unsigned int, struct rtmptab * );

struct atport;

int rtmp_packet(struct atport *ap, struct sockaddr_at *from, char *data, int len);

#endif /* atalkd/rtmp.h */
