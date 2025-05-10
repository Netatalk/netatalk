/*
 * AppleTalk MacIP Gateway
 *
 * (c) 2013, 1997 Stefan Bethke. All rights reserved.
 * (c) 2015 Jason King. All rights reserved.
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
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <netatalk/at.h>
#include <atalk/aep.h>
#include <atalk/ddp.h>
#include <atalk/atp.h>
#include <atalk/zip.h>

#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include "atp_input.h"
#include "nbp_lkup_async.h"

#include "common.h"
#include "macip.h"
#include "util.h"

#define DDPTYPE_MACIP (22)
#define MACIP_ASSIGN (1)
#define MACIP_SERVER (3)
#define MACIP_PORT (72)

#define MACIP_MAXMTU (586)

#define MACIP_NODETYPE "IPADDRESS"
#define MACIP_GATETYPE "IPGATEWAY"

#define MACIP_ATPRETRIES (5)
#define MACIP_ATPWAIT (5)
#define MACIP_NBPRETRIES (5)
#define MACIP_NBPWAIT (5)

#define ARPTIMEOUT (30)
#define ARPRETRIES (10)
/* ping every 30secs, give up after 5mins */

#define ASSIGN_FREE   (0)
#define ASSIGN_LEASED (1)
#define ASSIGN_FIXED  (2)

struct macip_req_control {
    uint16_t mipr_version;
    uint16_t _mipr_pad1;
    uint32_t mipr_function;
} __attribute__((__packed__));

struct macip_req_data {
    uint32_t mipr_ipaddr;
    uint32_t mipr_nameserver;
    uint32_t mipr_broadcast;
    uint32_t _mipr_pad2;
    uint32_t mipr_subnet;
} __attribute__((__packed__));

struct macip_req {
    struct macip_req_control miprc;
    struct macip_req_data miprd;
} __attribute__((__packed__));

struct ipent {
    int assigned;
    long timo;
    int retr;
    struct sockaddr_at sat;
};

struct macip_data {
    uint32_t net;
    uint32_t mask;
    uint32_t broadcast;
    uint32_t nameserver;
    uint32_t addr;
    int nipent;
    struct ipent *ipent;

    int sock;
    ATP atp;

    char name[32];
    char type[32];
    char zone[32];
};

#define IPADDR(e) (e - gMacip.ipent + gMacip.net + 1)

#define MAXZONES 256

struct zones {
    int n;
    char *z[MAXZONES];
};

static struct macip_data gMacip;
static struct zones gZones;
static outputfunc_t gOutput;


static uint16_t cksum(char *buffer, int len)
{
    uint16_t *b = (uint16_t *) buffer;
    uint32_t sum = 0;
    len /= 2;

    while (len--) {
        sum += *b++;
    }

    sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
    sum = sum > 65535 ? sum - 65535 : sum;
    return ~sum & 0xffff;
}


static void icmp_echo(uint32_t src, uint32_t dst)
{
    char buffer[500];
    struct ip *ip = (struct ip *) buffer;
    struct icmp *icmp = (struct icmp *)(buffer + 20);
    bzero(buffer, sizeof(buffer));
    ip->ip_v = IPVERSION;
    ip->ip_hl = 5;
    ip->ip_ttl = 255;
    ip->ip_src.s_addr = htonl(src);
    ip->ip_dst.s_addr = htonl(dst);
    ip->ip_id = 1;
    ip->ip_p = IPPROTO_ICMP;
    ip->ip_len = htons(20 + 8);
    ip->ip_sum = 0;
    ip->ip_sum = cksum((char *) ip, 20);
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = cksum((char *) icmp, 8);
    macip_output(buffer, ntohs(ip->ip_len));
}



static long now(void)
{
    static struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec;
}


struct ipent *get_ipent(uint32_t ip)
{
    return (ip - gMacip.net > 0 && ip - gMacip.net < gMacip.nipent) ?
           &gMacip.ipent[ip - gMacip.net - 1] : 0;
}

/*
 * aquire a new ip address from the pool
 */

static uint32_t lease_ip()
{
    int i;

    for (i = 0; i < gMacip.nipent; i++) {
        if (gMacip.ipent[i].assigned == ASSIGN_FREE) {
            gMacip.ipent[i].assigned = ASSIGN_LEASED;
            bzero(&gMacip.ipent[i].sat,
                  sizeof(struct sockaddr_at));
            gMacip.ipent[i].retr = ARPRETRIES;
            gMacip.ipent[i].timo = ARPTIMEOUT;
            return gMacip.net + i + 1;
        }
    }

    return 0;
}


/*
 * find AT address for an IP address
 */

static int arp_lookup(struct sockaddr_at *sat, uint32_t ip)
{
    int i;
    char s[32];
    struct ipent *e = get_ipent(ip);

    if (e && e->assigned) {
        if (gDebug & DEBUG_MACIP)
            printf("found arp entry: %s -> %d.%d\n",
                   iptoa(ip), ntohs(e->sat.sat_addr.s_net),
                   e->sat.sat_addr.s_node);

        bcopy(&e->sat, sat, sizeof(*sat));
        return 1;
    }

    strcpy(s, iptoa(ip));

    for (i = 0; i < gZones.n; i++) {
        nbp_lookup_req(gMacip.sock, s, MACIP_NODETYPE,
                       gZones.z[i]);
    }

    return 0;
}


/*
 * Set AT address from received packet
 */

static void arp_set(uint32_t ip, struct sockaddr_at *sat)
{
    struct ipent *e;
    e = get_ipent(ip);

    if (e) {
        bcopy(sat, &e->sat, sizeof(struct sockaddr_at));
        e->timo = now() + ARPTIMEOUT;
        e->retr = ARPRETRIES;

        if (e->assigned == ASSIGN_FREE) {
            e->assigned = ASSIGN_LEASED;
        }

        if (gDebug & DEBUG_MACIP)
            printf("arp_set: %s -> %d.%d\n", iptoa(ip),
                   ntohs(sat->sat_addr.s_net),
                   sat->sat_addr.s_node);
    }
}

/*
 *	handle name lookup replies, add to arp table
 */

static void arp_input(struct sockaddr_at *sat, char *buffer, int len)
{
    struct nbpnve *nve;
    char s[32];
    uint32_t ip;

    while ((nve = nbp_parse_lkup_repl(buffer, len)) != NULL) {
        if (gDebug & DEBUG_MACIP)
            printf("received nbp entry '%.*s:%.*s@%.*s'\n",
                   nve->nn_objlen, nve->nn_obj,
                   nve->nn_typelen, nve->nn_type,
                   nve->nn_zonelen, nve->nn_zone);

        if (nve->nn_typelen != strlen(MACIP_NODETYPE) ||
                strncasecmp(nve->nn_type, MACIP_NODETYPE,
                            strlen(MACIP_NODETYPE)) != 0) {
            continue;
        }

        bcopy(nve->nn_obj, s, nve->nn_objlen);
        s[nve->nn_objlen] = 0;
        ip = atoip(s);

        if (ip == 0) {
            continue;
        }

        arp_set(ip, &nve->nn_sat);
    }

    /* send packets waiting */
}

/*
 *	handle incoming IP packet
 */

static void ip_input(struct sockaddr_at *sat, char *buffer, int len)
{
    struct ip *p = (struct ip *) buffer;

    if (gDebug & DEBUG_MACIP) {
        printf("got IP packet from %d.%d\n",
               ntohs(sat->sat_addr.s_net), sat->sat_addr.s_node);
        printf("\tsource=%s, ", iptoa(ntohl(p->ip_src.s_addr)));
        printf("dest=%s\n", iptoa(ntohl(p->ip_dst.s_addr)));
    }

    arp_set(ntohl(p->ip_src.s_addr), sat);
    gOutput(buffer, len);
}

/*
 *	send IP packet through AT
 */

void macip_output(char *buffer, int len)
{
    struct sockaddr_at sat;
    struct ip *ip = (struct ip *) buffer;
    char ob[MACIP_MAXMTU + 1];

    if (len > MACIP_MAXMTU) {
        if (gDebug & DEBUG_MACIP)
            printf
            ("macip_output: packet to large, dropped.\n");

        return;
        /*      actually, we could fragment here, but this should never happen,
           as the MTU for the tunnel should be small enough.
         */
    }

    if (arp_lookup(&sat, ntohl(ip->ip_dst.s_addr))) {
        bcopy(ip, ob + 1, len);
        *ob = DDPTYPE_MACIP;
        len++;

        if (sendto(gMacip.sock, ob, len, 0,
                   (struct sockaddr *) &sat, sizeof(sat)) <= 0) {
            if (gDebug & DEBUG_MACIP) {
                perror("macip_output: sendto");
            }
        }
    } else {
        /* XXX we could queue the packet here for delivery when the
           address has been looked up */
    }
}

/*
 *	handle configuration ATP request
 */

static void config_input(ATP atp, struct sockaddr_at *faddr, char *packet,
                         int len)
{
    struct atp_block atpb;
    struct sockaddr_at sat;
    struct iovec iov;
    char buffer[600];
    struct macip_req *rq;
    int f;
    uint32_t ip;

    if (atp_input(atp, faddr, packet, len) < 0) {
        if (gDebug & DEBUG_MACIP) {
            printf("atp_input: packet rejected.\n");
        }

        return;
    }

    bzero(&sat, sizeof(sat));
    sat.sat_family = AF_APPLETALK;
    sat.sat_addr.s_net = ATADDR_ANYNET;
    sat.sat_addr.s_node = ATADDR_ANYNODE;
    sat.sat_port = ATADDR_ANYPORT;

    if (atp_rsel(atp, &sat, ATP_TREQ) != ATP_TREQ) {
        return;		/* something other than a new request */
    }

    atpb.atp_saddr = &sat;
    atpb.atp_rreqdata = buffer;
    atpb.atp_rreqdlen = sizeof(buffer);

    if (atp_rreq(atp, &atpb) < 0) {
        if (gDebug & DEBUG_MACIP) {
            perror("macip: config_input: atp_rreq");
        }

        return;
    }

    rq = (struct macip_req *)(atpb.atp_rreqdata);
    f = ntohl(rq->miprc.mipr_function);
    len = atpb.atp_rreqdlen;

    if (gDebug & DEBUG_MACIP)
        printf("\nMacIP req: %d from %d.%d\n", f,
               ntohs(atpb.atp_saddr->sat_addr.s_net),
               atpb.atp_saddr->sat_addr.s_node);

    rq = (struct macip_req *) buffer;
    bzero(rq, sizeof(struct macip_req));

    switch (f) {
    case MACIP_ASSIGN:
        rq->miprc.mipr_version = htons(1U);

        if ((ip = lease_ip())) {
            rq->miprc.mipr_function = htonl(1U);
            rq->miprd.mipr_ipaddr = htonl(ip);
            rq->miprd.mipr_nameserver =
                htonl(gMacip.nameserver);
            rq->miprd.mipr_broadcast = htonl(gMacip.broadcast);
            rq->miprd.mipr_subnet = htonl(gMacip.mask);
            len = sizeof(struct macip_req);
            arp_set(ip, &sat);

            if (gDebug & DEBUG_MACIP) {
                printf("assigned %s.\n", iptoa(ip));
            }
        } else {
            rq->miprc.mipr_function = htonl(0);
            len = sizeof(struct macip_req);
        }

        break;

    case MACIP_SERVER:
        rq->miprc.mipr_version = htons(1U);
        rq->miprc.mipr_function = htonl(3U);
        len = sizeof(struct macip_req_control);
        break;

    default:
        if (gDebug & DEBUG_MACIP)
            printf
            ("macip_input: unknown request #%d received; ignored.\n",
             f);

        rq->miprc.mipr_version = htons(1U);
        rq->miprc.mipr_function = htonl(0U);
        len = sizeof(struct macip_req_control);
    }

    iov.iov_base = buffer;
    iov.iov_len = len;
    atpb.atp_sresiov = &iov;
    atpb.atp_sresiovcnt = 1;

    if (atp_sresp(atp, &atpb) < 0 && gDebug & DEBUG_MACIP) {
        perror("macip_input: atp_sresp");
    }
}

/*
 *	Handle incoming AT packet
 */

void macip_input(void)
{
    struct sockaddr_at sat;
    char buffer[800];
    ssize_t len;
    socklen_t flen;
    bzero(&sat, sizeof(sat));
#ifdef BSD4_4
    sat.sat_len = sizeof(struct sockaddr_at);
#endif
    sat.sat_family = AF_APPLETALK;
    sat.sat_addr.s_net = ATADDR_ANYNET;
    sat.sat_addr.s_node = ATADDR_ANYNODE;
    sat.sat_port = ATADDR_ANYPORT;
    flen = sizeof(struct sockaddr_at);

    if ((len = recvfrom(gMacip.sock, buffer, ATP_BUFSIZ,
                        0, (struct sockaddr *) &sat, &flen)) > 0) {
        if (gDebug & DEBUG_MACIP)
            printf("macip_input: packet: DDP=%d, len=%zu\n",
                   *buffer, len);

        switch (*buffer) {
    /*DDPTYPE*/ case DDPTYPE_NBP:
            arp_input(&sat, buffer, len);
            break;

        case DDPTYPE_ATP:
            config_input(gMacip.atp, &sat, buffer, len);
            break;

        case DDPTYPE_MACIP:
            ip_input(&sat, buffer + 1, len - 1);
            break;
        }
    } else {
        perror("recvfrom");
    }
}


static int init_ip(uint32_t net, uint32_t mask, uint32_t nameserver)
{
    bzero(&gMacip, sizeof(gMacip));
    gMacip.net = net;
    gMacip.mask = mask;
    gMacip.nameserver = nameserver;
    gMacip.broadcast = net | ~mask;
    gMacip.nipent = (~mask) - 1;

    if ((gMacip.ipent =
                calloc(gMacip.nipent, sizeof(struct ipent))) == 0) {
        if (gDebug & DEBUG_MACIP) {
            perror("init_ip: calloc");
        }

        return -1;
    }

    gMacip.addr = net + 1;	/* my address */
    gMacip.ipent[0].assigned = ASSIGN_FIXED;

    if (gDebug & DEBUG_MACIP) {
        printf("init_ip: network %s/", iptoa(gMacip.net));
        printf("%s, ", iptoa(gMacip.mask));
        printf("broadcast %s\n\t%d addresses available\n",
               iptoa(gMacip.broadcast), gMacip.nipent - 1);
        printf("\tmy ip=%s\n", iptoa(gMacip.net + 1));
    }

    return 0;
}


static void add_zones(short n, char *buf)
{
    char s[32];

    for (; n--; buf += (*buf) + 1) {
        if (gDebug & DEBUG_MACIP) {
            printf("add_zones: %.*s\n", *buf, buf + 1);
        }

        bcopy(buf + 1, s, *buf);
        s[(int) *buf] = 0;

        if (gZones.n < MAXZONES) {
            gZones.z[gZones.n++] = strdup(s);
        }
    }
}

static int get_zones(void)
{
    struct atp_handle *ah;
    struct atp_block atpb;
    struct sockaddr_at saddr;
    struct servent *se;
    char reqdata[4], buf[ATP_MAXDATA];
    struct iovec iov;
    short temp, index = 0;
    int i;
    gZones.n = 0;
    reqdata[0] = ZIPOP_GETZONELIST;

    if ((ah = atp_open(0, NULL)) == NULL) {
        perror("atp_open");
        return -1;
    }

    bzero((char *) &saddr, sizeof(struct sockaddr_at));
#ifdef BSD4_4
    saddr.sat_len = sizeof(struct sockaddr_at);
#endif
    saddr.sat_family = AF_APPLETALK;

    if ((se = getservbyname("zip", "ddp")) == NULL) {
        saddr.sat_port = 6;
    } else {
        saddr.sat_port = ntohs(se->s_port);
    }

    saddr.sat_addr.s_net = ATADDR_ANYNET;
    saddr.sat_addr.s_node = ATADDR_ANYNODE;
    index = 1;
    reqdata[1] = 0;

    do {
        atpb.atp_saddr = &saddr;
        temp = htons(index);
        bcopy(&temp, &reqdata[2], 2);
        atpb.atp_sreqdata = reqdata;
        atpb.atp_sreqdlen = 4;
        atpb.atp_sreqto = 2;
        atpb.atp_sreqtries = 5;

        /* send getzone request zones (or get my zone)
         */
        if (atp_sreq(ah, &atpb, 1, 0) < 0) {
            perror("atp_sreq");
            return -1;
        }

        iov.iov_base = buf;
        iov.iov_len = ATP_MAXDATA;
        atpb.atp_rresiov = &iov;
        atpb.atp_rresiovcnt = 1;
        i = atp_rresp(ah, &atpb);

        if (i < 0) {
            if (errno == ETIMEDOUT) {
                gZones.n = 1;
                gZones.z[0] = "*";

                if (gDebug & DEBUG_MACIP) {
                    printf("no zones on network\n");
                }

                return 0;
            }

            perror("get_zones: atp_rresp");
            return -1;
        }

        bcopy(&((char *) iov.iov_base)[2], &temp, 2);
        temp = ntohs(temp);
        add_zones(temp, iov.iov_base + 4);
        index += temp;
    } while (!((char *) iov.iov_base)[0]);

    if (atp_close(ah) != 0) {
        perror("atp_close");
        return -1;
    }

    return 0;
}


/*
 * time out old arp entries
 */

void macip_idle(void)
{
    struct ipent *e;
    int i;
    long n = now();

    for (i = gMacip.nipent, e = gMacip.ipent; i--; e++) {
        if ((e->assigned == ASSIGN_LEASED) && (e->timo < n)) {
            if (e->retr--) {
                icmp_echo(gMacip.addr, IPADDR(e));
                e->timo = n + ARPTIMEOUT;

                if (gDebug & DEBUG_MACIP)
                    printf
                    ("macip_idle: sending probe to %s.\n",
                     iptoa(IPADDR(e)));
            } else {
                e->assigned = ASSIGN_FREE;

                if (gDebug & DEBUG_MACIP)
                    printf
                    ("macip_idle: arp entry %s/%d.%d free'd.\n",
                     iptoa(IPADDR(e)),
                     ntohs(e->sat.sat_addr.s_net),
                     e->sat.sat_addr.s_node);
            }
        }
    }
}


int
macip_open(char *zone, uint32_t net, uint32_t mask, uint32_t ns,
           outputfunc_t o)
{
    int i;

    if (init_ip(net, mask, ns)) {
        if (gDebug & DEBUG_MACIP) {
            printf("macip_open: init_ip failed.\n");
        }

        return -1;
    }

    for (i = 0; i < MACIP_ATPRETRIES; i++) {
        if (i == MACIP_ATPRETRIES - 1) {
            printf("macip_open: too many retries\n");
            return -1;
        }

        if ((gMacip.atp = atp_open(MACIP_PORT, NULL)) == NULL) {
            if (gDebug & DEBUG_MACIP) {
                perror("macip_open: atp_open");
            }

            printf("macip_open: retrying in %d seconds\n", MACIP_ATPRETRIES);
            sleep(MACIP_ATPWAIT);
        } else {
            break;
        }
    }

    gMacip.sock = gMacip.atp->atph_socket;
    strcpy(gMacip.name, iptoa(gMacip.addr));
    strcpy(gMacip.type, MACIP_GATETYPE);
    strcpy(gMacip.zone, zone);

    for (i = 0; i < MACIP_NBPRETRIES; i++) {
        if (i == MACIP_NBPRETRIES - 1) {
            printf("macip_open: too many retries\n");
            return -1;
        }

        if (gDebug & DEBUG_MACIP) {
            printf("macip_open: registering %s:%s@%s...",
                   gMacip.name, gMacip.type, gMacip.zone);
            fflush(stdout);
        }

        if (nbp_rgstr(atp_sockaddr(gMacip.atp),
                      gMacip.name,
                      gMacip.type,
                      gMacip.zone) < 0) {
            if (gDebug & DEBUG_MACIP) {
                perror("failed");
            }

            printf("macip_open: retrying in %d seconds\n", MACIP_NBPRETRIES);
            sleep(MACIP_NBPWAIT);
        } else {
            break;
        }
    }

    if (gDebug & DEBUG_MACIP) {
        printf("done.\n");
    }

    if (get_zones()) {
        return -1;
    }

    nbp_lookup_req(gMacip.sock, "=", "IPADDRESS", "*");
    gOutput = o;
    return gMacip.sock;
}


void macip_close(void)
{
    nbp_unrgstr(gMacip.name, gMacip.type, gMacip.zone, NULL);
    close(gMacip.sock);
}
