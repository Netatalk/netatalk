/*
 * Asnychrounous NBP lookup
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
 *
 * Copyright (c) 1990,1996 Regents of The University of Michigan.
 * All Rights Reserved.
 *
 *     Permission to use, copy, modify, and distribute this software and
 *     its documentation for any purpose and without fee is hereby granted,
 *     provided that the above copyright notice appears in all copies and
 *     that both that copyright notice and this permission notice appear
 *     in supporting documentation, and that the name of The University
 *     of Michigan not be used in advertising or publicity pertaining to
 *     distribution of the software without specific, written prior
 *     permission. This software is supplied as is without expressed or
 *     implied warranties of any kind.
 */


#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <netatalk/at.h>
#include <atalk/ddp.h>
#include <netdb.h>

#include "nbp_lkup_async.h"

extern int gDebug;
extern int nbp_parse(char *, struct nbpnve *, int);

static char *nbp_addstring(char *p, char *s) {
    int i;

    if (s) {
        if ((i = strlen(s)) > NBPSTRLEN) {
            i = 31;
        }

        *p++ = i;
        bcopy(s, p, i);
        p += i;
    } else {
        *p++ = 0;
    }

    return p;
}

static char *nbp_addtuple(char *p, struct sockaddr_at addr,
                          char *name, char *type, char *zone) {
    struct nbptuple nt;
    nt.nt_net = addr.sat_addr.s_net;
    nt.nt_node = addr.sat_addr.s_node;
    nt.nt_port = addr.sat_port;
    bcopy(&nt, p, SZ_NBPTUPLE);
    p += SZ_NBPTUPLE;
    p = nbp_addstring(p, name);
    p = nbp_addstring(p, type);
    p = nbp_addstring(p, zone);
    return p;
}

int nbp_lookup_req(int s, char *name, char *type, char *zone) {
    static int rqid = 1;
    static int nbp_port = 0;
    struct sockaddr_at addr;
    socklen_t i;
    char buffer[500];
    char *p = buffer;
    struct nbphdr nh;
    struct servent *se;
    i = sizeof(struct sockaddr_at);

    if (getsockname(s, (struct sockaddr *) &addr, &i) < 0) {
        return -1;
    }

    *p++ = DDPTYPE_NBP;
    nh.nh_op = NBPOP_BRRQ;
    nh.nh_cnt = 1;
    nh.nh_id = rqid++;
    bcopy(&nh, p, SZ_NBPHDR);
    p += SZ_NBPHDR;
    p = nbp_addtuple(p, addr, name, type, zone);
    bzero(&addr, sizeof(struct sockaddr_at));
#ifdef BSD4_4
    addr.sat_len = sizeof(struct sockaddr_at);
#endif
    addr.sat_family = AF_APPLETALK;
    addr.sat_addr.s_net = ATADDR_ANYNET;
    addr.sat_addr.s_node = ATADDR_ANYNODE;

    if (nbp_port == 0) {
        if ((se = getservbyname("nbp", "ddp")) == NULL) {
            nbp_port = 2;
        } else {
            nbp_port = ntohs(se->s_port);
        }
    }

    addr.sat_port = nbp_port;

    if (gDebug) {
        printf("looking up '%s:%s@%s'\n", name, type, zone);
    }

    if (sendto(s, buffer, p - buffer, 0, (struct sockaddr *) &addr,
               sizeof(struct sockaddr_at)) < 0) {
        return -1;
    }

    return 0;
}

struct nbpnve *nbp_parse_lkup_repl(char *buffer, int len) {
    static char *lastbuffer = NULL;
    static char *nexttuple = NULL;
    static struct nbpnve nve;
    struct nbphdr *nh;
    int i;

    if (lastbuffer != buffer) {	/* new packet */
        lastbuffer = buffer;
        nexttuple = 0;

        if (*buffer++ != DDPTYPE_NBP) {
            return 0;
        }

        len--;
        nh = (struct nbphdr *) buffer;
        buffer += SZ_NBPHDR;

        if (nh->nh_op != NBPOP_LKUPREPLY) {
            return 0;
        }

        len -= SZ_NBPHDR;
        nexttuple = buffer;
    } else {
        len -= nexttuple - buffer;
    }

    i = nbp_parse(nexttuple, &nve, len);

    if (i >= 0) {
        nexttuple += len - i;

        if (gDebug)
            printf("received nbp entry '%.*s:%.*s@%.*s'\n",
                   nve.nn_objlen, nve.nn_obj,
                   nve.nn_typelen, nve.nn_type,
                   nve.nn_zonelen, nve.nn_zone);

        return &nve;
    }

    lastbuffer = 0;
    nexttuple = 0;
    return 0;
}
