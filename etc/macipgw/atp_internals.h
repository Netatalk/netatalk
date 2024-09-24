/*
 * $Id: atp_internals.h,v 1.4 2009-10-13 22:55:37 didg Exp $
 *
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
 */

#ifndef ATP_INTERNALS_H
#define ATP_INTERNALS_H 1

#include <sys/types.h>
#include <sys/cdefs.h>
#include <netatalk/at.h>
#include <netatalk/endian.h>
#include <atalk/atp.h>

/*
 * masks for matching incoming packets
 */
#define ATP_FUNCANY	ATP_TREQ | ATP_TRESP | ATP_TREL
#define ATP_TIDANY	0xffff

/* in atp_bufs.c */
extern struct atpbuf *atp_alloc_buf (void);
extern void atp_print_bufuse        (ATP, char *);
extern int atp_free_buf             (struct atpbuf *);

/* in atp_packet.c */
extern int at_addr_eq               (struct sockaddr_at *, 
					 struct sockaddr_at *);
extern void atp_build_req_packet    (struct atpbuf *, u_int16_t, 
					 u_int8_t, struct atp_block *);
extern void atp_build_resp_packet   (struct atpbuf *, u_int16_t,
					 u_int8_t, struct atp_block *,
					 u_int8_t);
extern int atp_recv_atp             (ATP, struct sockaddr_at *, 
					 u_int8_t *, u_int16_t, char *,
					 int);
#ifdef EBUG
extern void atp_print_addr          (char *, struct sockaddr_at *);
#endif /* EBUG */

#endif /* ATP_INTERNALS_H */
