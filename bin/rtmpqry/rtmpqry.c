#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/socket.h>
#include <netatalk/at.h>

#include <atalk/ddp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int setup_ddp_socket(struct at_addr *local_addr, struct at_addr **remote_addr);

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

    sockfd = setup_ddp_socket(&local_addr, &remote_addr);
    printf("honk %d.%d\n", (int)ntohs(remote_addr->s_net),
           (int)remote_addr->s_node);
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
