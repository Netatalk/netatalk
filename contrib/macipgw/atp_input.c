/*
 * Add an AT packet to ATPs input queue
 *
 * (c) 1997 Stefan Bethke. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#undef s_net
#include <netatalk/at.h>
#include <atalk/ddp.h>
#include <atalk/atp.h>
#include <atalk/util.h>

#include "atp_internals.h"

extern int gDebug;

static void print_func(u_int8_t ctrlinfo)
{
    switch (ctrlinfo & ATP_FUNCMASK) {
    case ATP_TREQ:
        printf("TREQ");
        break;

    case ATP_TRESP:
        printf("TRESP");
        break;

    case ATP_TREL:
        printf("ANY/TREL");
        break;

    case ATP_TIDANY:
        printf("*");
        break;

    default:
        printf("%x", ctrlinfo & ATP_FUNCMASK);
    }
}

void atp_print_addr(char *s, struct sockaddr_at *saddr)
{
    printf("%s ", s);
    saddr->sat_family == AF_APPLETALK ? printf("at.") :
    printf("%d.", saddr->sat_family);
    saddr->sat_addr.s_net == ATADDR_ANYNET ? printf("*.") :
    printf("%d.", ntohs(saddr->sat_addr.s_net));
    saddr->sat_addr.s_node == ATADDR_ANYNODE ? printf("*.") :
    printf("%d.", saddr->sat_addr.s_node);
    saddr->sat_port == ATADDR_ANYPORT ? printf("*") :
    printf("%d", saddr->sat_port);
}


int atp_input(ATP ah, struct sockaddr_at *faddr, char *rbuf, int recvlen)
{
    struct atpbuf *pq, *cq;
    struct atphdr ahdr;
    uint16_t rfunc;
    uint16_t rtid;
    int i;
    struct atpbuf *inbuf;
    bcopy(rbuf + 1, (char *) &ahdr, sizeof(struct atphdr));

    if (recvlen >= ATP_HDRSIZE && *rbuf == DDPTYPE_ATP) {
        /* this is a valid ATP packet -- check for a match */
        rfunc = ahdr.atphd_ctrlinfo & ATP_FUNCMASK;
        rtid = ahdr.atphd_tid;

        if (gDebug) {
            printf("<%d> got tid=%hu func=", getpid(),
                   ntohs(rtid));
            print_func(rfunc);
            atp_print_addr(" from", faddr);
            putchar('\n');
            bprint(rbuf, recvlen);
        }

        if (rfunc == ATP_TREL) {
            /* remove response from sent list */
            for (pq = NULL, cq = ah->atph_sent; cq != NULL;
                    pq = cq, cq = cq->atpbuf_next) {
                if (at_addr_eq(faddr, &cq->atpbuf_addr) &&
                        cq->atpbuf_info.atpbuf_xo.atpxo_tid ==
                        ntohs(rtid)) {
                    break;
                }
            }

            if (cq != NULL) {
                if (gDebug)
                    printf
                    ("<%d> releasing transaction %hu\n",
                     getpid(), ntohs(rtid));

                if (pq == NULL) {
                    ah->atph_sent = cq->atpbuf_next;
                } else {
                    pq->atpbuf_next = cq->atpbuf_next;
                }

                for (i = 0; i < 8; ++i) {
                    if (cq->atpbuf_info.atpbuf_xo.
                            atpxo_packet[i] != NULL) {
                        atp_free_buf(cq->
                                     atpbuf_info.
                                     atpbuf_xo.
                                     atpxo_packet
                                     [i]);
                    }
                }

                atp_free_buf(cq);
            }
        } else {
            /* add packet to incoming queue */
            if (gDebug)
                printf("<%d> queuing incoming...\n",
                       getpid());

            if ((inbuf = atp_alloc_buf()) == NULL) {
                if (gDebug)
                    printf("<%d> can't alloc buffer\n",
                           getpid());

                return -1;
            }

            bcopy((char *) faddr, (char *) &inbuf->atpbuf_addr,
                  sizeof(struct sockaddr_at));
            inbuf->atpbuf_next = ah->atph_queue;
            ah->atph_queue = inbuf;
            inbuf->atpbuf_dlen = recvlen;
            bcopy((char *) rbuf,
                  (char *) inbuf->atpbuf_info.atpbuf_data,
                  recvlen);
        }

        return 0;
    }

    return -1;		/* invalid packet */
}
