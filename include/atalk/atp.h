/*
 * $Id: atp.h,v 1.5 2009-10-13 22:55:37 didg Exp $
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

#ifndef _ATALK_ATP_H
#define _ATALK_ATP_H 1
 
#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netatalk/at.h>
#include <netatalk/endian.h>

/* ATP packet format

 |----------------|
 | link header    |
 |      ...       |
 |----------------|
 | DDP header     |
 |      ...       |
 |   type = 3     |
 |----------------|
 | control info   | --> bits 7,6: function code
 |----------------|            5: XO bit
 | bitmap/seq no. |            4: EOM bit
 |----------------|            3: STS bit
 | TID (MSB)      |        2,1,0: release timer code (ignored under phase I)
 |----------------|
 | TID (LSB)      |
 |----------------|
 | user bytes (4) |
 |----------------|
 | data (0-578)   |
 |      ...       |
 |----------------|
*/
struct atphdr {
    u_int8_t	atphd_ctrlinfo;	/* control information */
    u_int8_t	atphd_bitmap;   /* bitmap or sequence number */
    u_int16_t	atphd_tid;	/* transaction id. */
};

/* ATP protocol parameters
*/
#define ATP_MAXDATA	(578+4)		/* maximum ATP data size */
#define ATP_BUFSIZ	587		/* maximum packet size */
#define ATP_HDRSIZE	5		/* includes DDP type field */

#define ATP_TRELMASK	0x07		/* mask all but TREL */
#define ATP_RELTIME	30		/* base release timer (in secs) */

#define ATP_TREL30	0x0		/* release time codes */
#define ATP_TREL1M	0x1		/* these are passed in flags of */
#define ATP_TREL2M	0x2		/* atp_sreq call, and set in the */
#define ATP_TREL4M	0x3		/* packet control info. */
#define ATP_TREL8M	0x4

#define ATP_TRIES_INFINITE	-1	/* for atp_sreq, etc */

struct atpxobuf {
    u_int16_t		atpxo_tid;
    struct timeval	atpxo_tv;
    int			atpxo_reltime;
    struct atpbuf	*atpxo_packet[8];
};

struct atpbuf {
    struct atpbuf	*atpbuf_next;		/* next buffer in chain */
    size_t		atpbuf_dlen;		/* data length <= ATP_BUFSIZ */
    struct sockaddr_at	atpbuf_addr;		/* net address sent/recvd */
    union {
	char		atpbuf_data[ ATP_BUFSIZ ];	/* the data */
	struct atpxobuf	atpbuf_xo;			/* for XO requests */
    } atpbuf_info;
};

struct atp_handle {
    int			atph_socket;		/* ddp socket */
    struct sockaddr_at	atph_saddr;		/* address */
    u_int16_t		atph_tid;		/* last tid used */
    u_int16_t		atph_rtid;		/* last received (rreq) */
    u_int8_t		atph_rxo;		/* XO flag from last rreq */
    int			atph_rreltime;		/* release time (secs) */
    struct atpbuf	*atph_sent;		/* packets we send (XO) */
    struct atpbuf	*atph_queue;		/* queue of pending packets */
    int			atph_reqtries;		/* retry count for request */
    int			atph_reqto;		/* retry timeout for request */
    int			atph_rrespcount;	/* expected # of responses */
    u_int8_t		atph_rbitmap;		/* bitmap for request */
    struct atpbuf	*atph_reqpkt;		/* last request packet */
    struct timeval	atph_reqtv;		/* when we last sent request */
    struct atpbuf	*atph_resppkt[8];	/* response to request */
};

typedef struct atp_handle *ATP;

#define atp_sockaddr( h )	(&(h)->atph_saddr)
#define atp_fileno(x)		((x)->atph_socket)

struct sreq_st {
    char	    *atpd_data;		/* request data */
    int		    atpd_dlen;
    int		    atpd_tries;		/* max. retry count */
    int		    atpd_to;		/* retry interval */
};

struct rres_st {
    struct iovec    *atpd_iov;		/* for response */
    int		    atpd_iovcnt;
};

struct rreq_st {
    char	    *atpd_data;		/* request data */
    int		    atpd_dlen;
};

struct sres_st {
    struct iovec    *atpd_iov;		/* for response */
    int		    atpd_iovcnt;
};

struct atp_block {
    struct sockaddr_at	*atp_saddr;		/* from/to address */
    union {
	struct sreq_st	sreqdata;
#define atp_sreqdata	atp_data.sreqdata.atpd_data
#define atp_sreqdlen	atp_data.sreqdata.atpd_dlen
#define atp_sreqtries	atp_data.sreqdata.atpd_tries
#define atp_sreqto	atp_data.sreqdata.atpd_to

	struct rres_st	rresdata;
#define atp_rresiov	atp_data.rresdata.atpd_iov
#define atp_rresiovcnt	atp_data.rresdata.atpd_iovcnt

	struct rreq_st	rreqdata;
#define atp_rreqdata	atp_data.rreqdata.atpd_data
#define atp_rreqdlen	atp_data.rreqdata.atpd_dlen

	struct sres_st	sresdata;
#define atp_sresiov	atp_data.sresdata.atpd_iov
#define atp_sresiovcnt	atp_data.sresdata.atpd_iovcnt
    } atp_data;
    u_int8_t		atp_bitmap;	/* response buffer bitmap */
};


/* flags for ATP options (and control byte)
*/
#define ATP_STS		(1<<3)		/* Send Transaction Status */
#define ATP_EOM		(1<<4)		/* End Of Message */
#define ATP_XO		(1<<5)		/* eXactly Once mode */

/* function codes
*/
#define ATP_FUNCMASK	(3<<6)		/* mask all but function */

#define ATP_TREQ	(1<<6)		/* Trans. REQuest */
#define ATP_TRESP	(2<<6)		/* Trans. RESPonse */
#define ATP_TREL	(3<<6)		/* Trans. RELease */

extern ATP		atp_open  (u_int8_t, 
				       const struct at_addr *);
extern int		atp_close (ATP);
extern int		atp_sreq  (ATP, struct atp_block *, int, 
				       u_int8_t);
extern int		atp_rresp (ATP, struct atp_block *);
extern int		atp_rsel  (ATP, struct sockaddr_at *, int);
extern int		atp_rreq  (ATP, struct atp_block *);
extern int		atp_sresp (ATP, struct atp_block *);

#endif
