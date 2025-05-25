/*
 * Copyright (C) Rob Mitchelmore 2025
 * All Rights Reserved.  See COPYING.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netatalk/at.h>

#include <atalk/ddp.h>
#include <atalk/netddp.h>
#include <atalk/rtmp.h>
#include <atalk/util.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int setup_ddp_socket(const struct at_addr *local_addr,
                     struct at_addr **remote_addr);
void do_rtmp_rdr(int sockfd, const struct sockaddr_at *sa_remote, bool get_all,
                 int secs_timeout);
void print_rtmp_data_packet(const uint8_t *buf, size_t len);
void do_rtmp_request(int sockfd, struct sockaddr_at *sa_remote);

static void usage(char *s)
{
    fprintf(stderr, "usage:\t%s [-a | -r] [ -A local_address ] [remote_address]\n",
            s);
    exit(1);
}

int main(int argc, char** argv)
{
    int c;
    int error_flag = 0;
    struct at_addr local_addr = { 0 };
    struct at_addr *remote_addr = NULL;
    int sockfd;
    bool all_routes = false;
    bool send_request = false;

    while ((c = getopt(argc, argv, "arA:")) != EOF) {
        switch (c) {
        case 'a':
            if (send_request) {
                error_flag++;
            } else {
                all_routes = true;
            }

            break;

        case 'r':
            if (all_routes) {
                error_flag++;
            } else {
                send_request = true;
            }

            break;

        case 'A':
            if (!atalk_aton(optarg, &local_addr)) {
                fprintf(stderr, "Bad address.\n");
                exit(1);
            }

            break;

        default:
            error_flag++;
        }
    }

    if (error_flag || argc - optind > 1) {
        usage(argv[0]);
    }

    /* set remote address from command line if we have one */
    if (argc > optind) {
        remote_addr = calloc(1, sizeof(struct at_addr));

        if (remote_addr == NULL) {
            perror("calloc");
            exit(1);
        }

        if (!atalk_aton(argv[optind], remote_addr)) {
            fprintf(stderr, "Bad address.\n");
            exit(1);
        }
    }

    /* set up DDP socket - fills in the remote address if none was explicitly
       provided */
    sockfd = setup_ddp_socket(&local_addr, &remote_addr);
    /* Construct remote sockaddr */
    struct sockaddr_at sa_remote = { 0 };
#ifdef BSD4_4
    sa_remote.sat_len = sizeof(struct sockaddr_at);
#endif /* BSD4_4 */
    sa_remote.sat_family = AF_APPLETALK;
    /* RIS socket is always 1 */
    sa_remote.sat_port = 1;
    memcpy(&sa_remote.sat_addr, remote_addr, sizeof(struct at_addr));

    if (send_request) {
        do_rtmp_request(sockfd, &sa_remote);
    } else {
        do_rtmp_rdr(sockfd, &sa_remote, all_routes, 2);
    }
    
    free(remote_addr);
}

/* Set up a DDP socket ready for sending RTMP packets.  The remote_addr is a pointer
   to a pointer so that if it's null we can fill it in with the address the kernel
   provides, as we do in libatalk/nbp/nbp_lkup.c for example. */
int setup_ddp_socket(const struct at_addr *local_addr,
                     struct at_addr **remote_addr)
{
    struct sockaddr_at local = { 0 };
    int sockfd;
    memcpy(&local.sat_addr, local_addr, sizeof(struct at_addr));
    sockfd = netddp_open(&local, NULL);

    if (sockfd < 0) {
        perror("netddp_open");
        exit(1);
    }

    /* Do we have a remote address? */
    if (*remote_addr == NULL) {
        /* Nope, fill it in with the address the kernel has bound us to. */
        *remote_addr = malloc(sizeof(struct at_addr));
        memcpy(*remote_addr, &local.sat_addr, sizeof(struct at_addr));
    }

    return sockfd;
}

/* Send an RTMP Request and wait for a reply.  An RTMP request just requests details
   of the network this node is connected to; in general, these aren't used on extended
   networks (instead, a ZIP GetNetInfo is used) but they should be supported anyway. */
void do_rtmp_request(int sockfd, struct sockaddr_at *sa_remote)
{
    /* See Inside Appletalk 2nd ed p. 5-18 for packet layout for both request and
       reply. */
    uint8_t buf[600];
    buf[0] = DDPTYPE_RTMPR;
    buf[1] = RTMPROP_REQUEST;
    int ret = netddp_sendto(sockfd, buf, 2, 0, (struct sockaddr*)sa_remote,
                            (int)sizeof(struct sockaddr_at));

    if (ret < 0) {
        perror("netddp_sendto");
        exit(1);
    }

    for (;;) {
        int length = netddp_recvfrom(sockfd, buf, (int)sizeof(buf), 0, NULL, NULL);

        if (length < 0) {
            perror("netddp_recvfrom");
            exit(1);
        }

        /* Is this a RTMP response?  If not, it's just some random
           packet that's fallen into our socket; ignore it.  */
        if (length < 5) {
            /* 4 is DDP type + network number + id length + node.  No valid reply will be
               shorter than this */
            continue;
        }

        if (buf[0] != DDPTYPE_RTMPRD) {
            continue;
        }

        /* Skip the DDP type which we checked above */
        uint8_t *cursor = buf + 1;
        uint16_t router_net = ntohs(*(uint16_t*)cursor);
        cursor += 2;
        uint8_t id_len = *cursor++;

        if (id_len != 8) {
            fprintf(stderr, "RTMP packet contained bad id length %"PRIu8" - are you "
                            "living in an alternate timeline where DDP was extended?", id_len);
            continue;
        }

        uint8_t router_node = *cursor++;
        /* If this is a nonextended network, we'll now have run out of packet.  If
           this is extended, we'll have six more bytes. */
        uint16_t net_start = 0;
        uint16_t net_end = 0;
        bool extended_network = false;

        if (((cursor + 6) - buf) <= length) {
            extended_network = true;
            net_start = ntohs(*(uint16_t*)cursor);
            cursor += 2;

            /* magic number.  Why 0x80?  Who knows! */
            if ((*cursor++) != 0x80) {
                fprintf(stderr, "Bad magic byte in RTMP response; giving up.");
                exit(1);
            }

            net_end = ntohs(*(uint16_t*)cursor);
            cursor += 2;
            /* Another magic byte: I assume this is the RTMP version as it's the same
               as bytes documented as RTMP version in other packets, but IA isn't
               actually very forthcoming here. */
            uint8_t rtmp_version = *cursor++;

            if (rtmp_version != 0x82) {
                fprintf(stderr, "Bad RTMP version 0x"PRIx8" in response; bailing out.");
                exit(1);
            }
        }

        /* At this point, we should have extracted all the data from the packet.
           Now print it and quit. */
        printf("Router: %"PRIu16".%"PRIu8"\n", router_net, router_node);

        if (!extended_network) {
            printf("Network(s): %"PRIu16"\n", router_net);
        } else {
            printf("Network(s): %"PRIu16" - %"PRIu16"\n", net_start, net_end);
        }

        break;
    }
}

/* Send an RTMP Route Data Request (RDR) and wait for a reply. An RTMP RDR solicits
   RTMP data packets - generally, these are sent out broadcast on a timer, but one can
   specifically ask for them to be sent to a non-standard socket by means of an RDR.
   An RDR can also specify whether or not for the router to do split horizon
   processing (or whether to return all routes). */
void do_rtmp_rdr(int sockfd, const struct sockaddr_at *sa_remote, bool get_all,
                 int secs_timeout)
{
    uint8_t buf[600];
    /* First send the RDR */
    buf[0] = DDPTYPE_RTMPR;

    if (!get_all) {
        buf[1] = RTMPROP_RDR;
    } else {
        buf[1] = RTMPROP_RDR_NOSH;
    }

    int ret = netddp_sendto(sockfd, buf, 2, 0, (struct sockaddr*)sa_remote,
                            (int)sizeof(struct sockaddr_at));

    if (ret < 0) {
        perror("netddp_sendto");
        exit(1);
    }

    /* Now, wait at least timeout_secs for replies.  We could do the whole dance that
       the NBP code does to precisely wait for the right number of seconds - but we
       really don't need that kind of precision, and nobody is going to really care
       whether we wait for 2 or 2.9 seconds.  So instead we're just going to set the
       timeout to secs_timeout + 1 so we always wait >= secs_timeout and
       <= secs_timeout + select_timeout. */
    /* Set up timeouts */
    int secs_to_wait = secs_timeout + 1;
    struct timeval select_timeout;
    struct timeval end_time;
    select_timeout.tv_sec = 0;
    select_timeout.tv_usec = 250000;
    ret = gettimeofday(&end_time, NULL);

    if (ret < 0) {
        perror("gettimeofday");
        exit(1);
    }

    end_time.tv_sec += secs_to_wait;

    /* Go around in small circles until we run out of time */
    for (;;) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        ret = select(sockfd + 1, &fds, NULL, NULL, &select_timeout);

        if (ret < 0) {
            perror("select");
            exit(1);
        }

        if (ret > 0) {
            ret = netddp_recvfrom(sockfd, buf, (int)sizeof(buf), 0, NULL, NULL);

            if (ret < 0) {
                perror("netddp_recvfrom");
                exit(1);
            }

            print_rtmp_data_packet(buf, ret);
        }

        /* have we run out of time? */
        struct timeval now;
        ret = gettimeofday(&now, NULL);

        if (ret < 0) {
            perror("gettimeofday");
            exit(1);
        }

        if (now.tv_sec >= end_time.tv_sec) {
            break;
        }
    }
}

/* Print an RTMP tuple pointed to by 'cursor' within the buffer 'buf' with length 'len'.
   Returns the next value of 'cursor' or NULL if no more tuples exist. */
const uint8_t *print_rtmp_tuple(const uint8_t *buf, const uint8_t *cursor,
                                size_t len, uint16_t router_net,
                                uint8_t router_node)
{
    /* A tuple is at least 3 bytes long */
    if ((cursor + 3) - buf > len) {
        return NULL;
    }

    /* Is this an extended network tuple?  If so, it's actually 6 bytes long. */
    bool tuple_extended = false;

    if (cursor[2] & 0x80) {
        tuple_extended = true;

        if ((cursor + 6) - buf > len) {
            return NULL;
        }
    }

    uint16_t net_start = ntohs(*(uint16_t*)cursor);
    cursor += 2;
    /* top bit of distance is the extended network bit we used above */
    uint8_t distance = (*cursor++) & 0x7f;

    /* If this is an extended tuple, we also have an end network and an RTMP version */
    if (tuple_extended) {
        uint16_t net_end = ntohs(*(uint16_t*)cursor);
        cursor += 2;
        uint8_t rtmp_version = *cursor++;

        if (rtmp_version != 0x82) {
            fprintf(stderr, "Bad RTMP version 0x"PRIx8" in tuple; ignoring tuple.");
            return cursor;
        }

        printf("%"PRIu16" - %"PRIu16" distance %"PRIu8
               " via %"PRIu16".%"PRIu8"\n", net_start, net_end, distance, router_net,
               router_node);
    } else {
        printf("%"PRIu16" distance %"PRIu8" via %"PRIu16".%"PRIu8"\n",
               net_start, distance, router_net, router_node);
    }

    return cursor;
}

void print_rtmp_data_packet(const uint8_t *buf, size_t len)
{
    const uint8_t *cursor = buf;

    /* The RTMP data packet format is in Inside Appletalk 2nd ed, p. 5-14. */

    /* check we have enough packet to be an RTMP packet */
    if (len < 5) {
        /* 5 = length of RTMP packet with no tuples */
        return;
    }

    /* check that we have RTMP... */
    if (*cursor != DDPTYPE_RTMPRD) {
        /* Nope, a random packet has wandered into our socket */
        return;
    }

    cursor++;
    uint16_t router_net_num = ntohs(*(uint16_t*)cursor);
    cursor += 2;
    uint8_t id_len = *cursor++;

    if (id_len != 8) {
        fprintf(stderr, "RTMP packet contained bad id length %"PRIu8" - are you "
                        "living in an alternate timeline where DDP was extended?", id_len);
        return;
    }

    uint8_t router_node = *cursor++;
    bool network_is_extended = true;

    /* If we're on a nonextended network, we'll have three further bytes, the first
       two of which are 0 and the third is the RTMP version (0x82). */
    if ((cursor + 3) - buf <= len) {
        if (cursor[0] == 0 && cursor[1] == 0) {
            network_is_extended = false;
        }

        if (!network_is_extended && cursor[2] != 0x82) {
            fprintf(stderr,
                    "Invalid RTMP version 0x%"PRIx8"; disregarding rest of packet\n",
                    cursor[2]);
            return;
        }

        if (!network_is_extended) {
            cursor += 3;
        }
    } else {
        /* We've run out of packet. */
        return;
    }

    /* The cursor will now point to the first tuple. */
    for (;;) {
        cursor = print_rtmp_tuple(buf, cursor, len, router_net_num, router_node);

        if (cursor == NULL) {
            break;
        }
    }

    printf("\n");
}
