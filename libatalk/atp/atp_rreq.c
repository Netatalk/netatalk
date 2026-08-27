/*
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netatalk/at.h>
#include <atalk/atp.h>

#include "atp_internals.h"

#ifdef EBUG
#include <stdio.h>
#endif /* EBUG */


/*
 * atp_rreq() waits for a request, consuming as many datagrams as it takes.
 * atp_rreq_try() makes exactly one attempt, so a caller that has already
 * established the socket is readable never blocks in it, and stays in charge of
 * its own wait. That matters for a caller driving other descriptors in the same
 * loop: a readable socket does not promise a request, because the datagram may
 * belong to another transaction or another node, and the waiting caller would
 * otherwise be parked here with those descriptors unwatched.
 */
int atp_rreq_try(ATP ah, struct atp_block *atpb)
{
    struct atpbuf	*req_buf;	/* for receiving request packet */
    struct atphdr	req_hdr;	/* request header overlay */
    struct sockaddr_at	faddr;		/* sender's address */
    int			recvlen;	/* length of received packet */
    uint16_t		tid;
    int			rc;
    uint8_t		func;
#ifdef EBUG
    atp_print_bufuse(ah, "atp_rreq");
#endif /* EBUG */
    rc = atp_rsel(ah, atpb->atp_saddr, ATP_TREQ);

    if (rc == 0) {
        return 0;		/* consumed, but not a new request */
    }

    if (rc != ATP_TREQ) {
#ifdef EBUG
        printf("<%d> atp_rreq: atp_rsel returns err %d\n", getpid(), rc);
#endif /* EBUG */
        return -1;
    }

    /* allocate a buffer for receiving request
    */
    if ((req_buf = atp_alloc_buf()) == NULL) {
        return -1;
    }

    memcpy(&faddr, atpb->atp_saddr, sizeof(struct sockaddr_at));
    func = ATP_TREQ;

    if ((recvlen = atp_recv_atp(ah, &faddr, &func, ATP_TIDANY,
                                req_buf->atpbuf_info.atpbuf_data, 1)) < 0) {
        atp_free_buf(req_buf);
        return -1;
    }

    memcpy(&req_hdr, req_buf->atpbuf_info.atpbuf_data + 1,
           sizeof(struct atphdr));
    tid = ntohs(req_hdr.atphd_tid);
    ah->atph_rtid = tid;

    if ((ah->atph_rxo = req_hdr.atphd_ctrlinfo & ATP_XO) != 0) {
        ah->atph_rreltime = ATP_RELTIME *
                            (1 << (req_hdr.atphd_ctrlinfo & ATP_TRELMASK));
    }

    memcpy(atpb->atp_saddr, &faddr, sizeof(struct sockaddr_at));

    if (recvlen - ATP_HDRSIZE > atpb->atp_rreqdlen) {
        atp_free_buf(req_buf);
        errno = EMSGSIZE;
        return -1;
    }

    atpb->atp_rreqdlen = recvlen - ATP_HDRSIZE;
    memcpy(atpb->atp_rreqdata,
           req_buf->atpbuf_info.atpbuf_data + ATP_HDRSIZE,
           recvlen - ATP_HDRSIZE);
    atpb->atp_bitmap = req_hdr.atphd_bitmap;
    atp_free_buf(req_buf);
    return 1;
}

/*!
 * @brief wait for a transaction service request
 * @param ah open atp handle
 * @param atpb parameter block
 */
int atp_rreq(ATP ah, struct atp_block *atpb)
{
    int rc;

    while ((rc = atp_rreq_try(ah, atpb)) == 0) {
        ;
    }

    return (rc == 1) ? 0 : rc;
}
