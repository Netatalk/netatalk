#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/socket.h>
#include <sys/time.h>
#include <netatalk/at.h>

#include <atalk/ddp.h>
#include <atalk/netddp.h>
#include <atalk/rtmp.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int setup_ddp_socket(struct at_addr *local_addr, struct at_addr **remote_addr);
void do_rtmp_rdr(int sockfd, struct sockaddr_at *sa_remote, bool get_all,
                 int secs_timeout);

static void usage(char *s)
{
    fprintf(stderr, "usage:\t%s [ -A local_address ] [remote_address]\n", s);
    exit(1);
}

void main(int argc, char** argv)
{
    int c;
    int error_flag = 0;
    struct at_addr local_addr = { 0 };
    struct at_addr *remote_addr = NULL;
    int sockfd;

    while ((c = getopt(argc, argv, "A:")) != EOF) {
        switch (c) {
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
    printf("honk %d.%d\n", (int)ntohs(remote_addr->s_net),
           (int)remote_addr->s_node);
    /* Construct remote sockaddr */
    struct sockaddr_at sa_remote = { 0 };
#ifdef BSD4_4
    sa_remote.sat_len = sizeof(struct sockaddr_at);
#endif /* BSD4_4 */
    sa_remote.sat_family = AF_APPLETALK;
    /* RIS socket is always 1 */
    sa_remote.sat_port = 1;
    memcpy(&sa_remote.sat_addr, remote_addr, sizeof(struct at_addr));
    do_rtmp_rdr(sockfd, &sa_remote, true, 2);
}

/* Set up a DDP socket ready for sending RTMP packets.  The remote_addr is a pointer
   to a pointer so that if it's null we can fill it in with the address the kernel
   provides, as we do in libatalk/nbp/nbp_lkup.c for example. */
int setup_ddp_socket(struct at_addr *local_addr, struct at_addr **remote_addr)
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

/* Send an RTMP Route Data Request (RDR) and wait for a reply. An RTMP RDR solicits
   RTMP data packets - generally, these are sent out broadcast on a timer, but one can
   specifically ask for them to be sent to a non-standard socket by means of an RDR.
   An RDR can also specify whether or not for the router to do split horizon
   processing (or whether to return all routes). */
void do_rtmp_rdr(int sockfd, struct sockaddr_at *sa_remote, bool get_all,
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
                            sizeof(struct sockaddr_at));

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
            ret = netddp_recvfrom(sockfd, buf, sizeof(buf), 0, NULL, NULL);

            if (ret < 0) {
                perror("netddp_recvfrom");
                exit(1);
            }

            printf("%d bytes\n", ret);
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
