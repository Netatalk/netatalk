#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/param.h>

#include <atalk/fce_api.h>
#include <atalk/util.h>

#define MAXBUFLEN 1024

static char *fce_ev_names[] = {
    "",
    "FCE_FILE_MODIFY",
    "FCE_FILE_DELETE",
    "FCE_DIR_DELETE",
    "FCE_FILE_CREATE",
    "FCE_DIR_CREATE",
    "FCE_FILE_MOVE",
    "FCE_DIR_MOVE",
    "FCE_LOGIN",
    "FCE_LOGOUT"
};

static int unpack_fce_packet(unsigned char *buf, struct fce_packet *packet)
{
    unsigned char *p = buf;
    uint16_t uint16;
    uint32_t uint32;
    uint64_t uint64;

    memcpy(&packet->fcep_magic[0], p, sizeof(packet->fcep_magic));
    p += sizeof(packet->fcep_magic);

    packet->fcep_version = *p++;

    if (packet->fcep_version > 1)
        packet->fcep_options = *p++;

    packet->fcep_event = *p++;

    if (packet->fcep_version > 1)
        /* padding */
        p++;

    if (packet->fcep_version > 1)
        /* reserved */
        p += 8;

    memcpy(&packet->fcep_event_id, p, sizeof(packet->fcep_event_id));
    p += sizeof(packet->fcep_event_id);
    packet->fcep_event_id = ntohl(packet->fcep_event_id);

    if (packet->fcep_options & FCE_EV_INFO_PID) {
        memcpy(&packet->fcep_pid, p, sizeof(packet->fcep_pid));
        packet->fcep_pid = hton64(packet->fcep_pid);
        p += sizeof(packet->fcep_pid);
    }

    if (packet->fcep_options & FCE_EV_INFO_USER) {
        memcpy(&packet->fcep_userlen, p, sizeof(packet->fcep_userlen));
        packet->fcep_userlen = ntohs(packet->fcep_userlen);
        p += sizeof(packet->fcep_userlen);

        memcpy(&packet->fcep_user[0], p, packet->fcep_userlen);
        packet->fcep_user[packet->fcep_userlen] = 0; /* 0 terminate strings */
        p += packet->fcep_userlen;
    }

    /* path */
    memcpy(&packet->fcep_pathlen1, p, sizeof(packet->fcep_pathlen1));
    p += sizeof(packet->fcep_pathlen1);
    packet->fcep_pathlen1 = ntohs(packet->fcep_pathlen1);

    memcpy(&packet->fcep_path1[0], p, packet->fcep_pathlen1);
    packet->fcep_path1[packet->fcep_pathlen1] = 0; /* 0 terminate strings */
    p += packet->fcep_pathlen1;

    if (packet->fcep_options & FCE_EV_INFO_SRCPATH) {
        memcpy(&packet->fcep_pathlen2, p, sizeof(packet->fcep_pathlen2));
        p += sizeof(packet->fcep_pathlen2);
        packet->fcep_pathlen2 = ntohs(packet->fcep_pathlen2);
        memcpy(&packet->fcep_path2[0], p, packet->fcep_pathlen2);
        packet->fcep_path2[packet->fcep_pathlen2] = 0; /* 0 terminate strings */
        p += packet->fcep_pathlen2;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int sockfd, rv, c;
    struct addrinfo hints, *servinfo, *p;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    char *host = "localhost";

    while ((c = getopt(argc, argv, "h:")) != -1) {
        switch(c) {
        case 'h':
            host = strdup(optarg);
            break;
        }
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(host, FCE_DEFAULT_PORT_STRING, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("listener: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;

    struct fce_packet packet;
    while (1) {
        if ((numbytes = recvfrom(sockfd,
                                 buf,
                                 MAXBUFLEN - 1,
                                 0,
                                 (struct sockaddr *)&their_addr,
                                 &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        unpack_fce_packet((unsigned char *)buf, &packet);

        if (memcmp(packet.fcep_magic, FCE_PACKET_MAGIC, sizeof(packet.fcep_magic)) == 0) {

            switch (packet.fcep_event) {
            case FCE_CONN_START:
                printf("FCE Start\n");
                break;

            case FCE_CONN_BROKEN:
                printf("Broken FCE connection\n");
                break;

            default:
                printf("ID: %" PRIu32 ", Event: %s", packet.fcep_event_id, fce_ev_names[packet.fcep_event]);
                if (packet.fcep_options & FCE_EV_INFO_PID)
                    printf(", pid: %" PRId64, packet.fcep_pid);
                if (packet.fcep_options & FCE_EV_INFO_USER)
                    printf(", user: %s", packet.fcep_user);

                if (packet.fcep_options & FCE_EV_INFO_SRCPATH)
                    printf(", source: %s", packet.fcep_path2);

                printf(", Path: %s\n", packet.fcep_path1);
                break;
            }
        }
    }

    close(sockfd);
    return 0;
}
