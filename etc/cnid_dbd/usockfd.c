/*
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */


#include <atalk/logger.h>
#include <atalk/compat.h>
#include "usockfd.h"

#include <sys/select.h>

/* ---------------
 * create a tcp socket
 */
int tsockfd_create(char *host, char *port, int backlog)
{
    int sockfd, flag, ret;
    struct addrinfo hints, *servinfo, *p;

    /* Prepare hint for getaddrinfo */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((ret = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        LOG(log_error, logtype_default, "tsockfd_create: getaddrinfo: %s\n", gai_strerror(ret));
        return 0;
    }

    /* create a socket */
    /* loop through all the results and bind to the first we can */
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            LOG(log_info, logtype_default, "tsockfd_create: socket: %s", strerror(errno));
            continue;
        }

        /*
         * Set some socket options:
         * SO_REUSEADDR deals w/ quick close/opens
         * TCP_NODELAY diables Nagle
         */
#ifdef SO_REUSEADDR
        flag = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
#endif

#ifdef USE_TCP_NODELAY
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif
        flag = 1;
        setsockopt(sockfd, SOL_TCP, TCP_NODELAY, &flag, sizeof(flag));
#endif /* USE_TCP_NODELAY */
            
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            LOG(log_info, logtype_default, "tsockfd_create: bind: %s\n", strerror(errno));
            continue;
        }

        if (listen(sockfd, backlog) < 0) {
            close(sockfd);
            LOG(log_info, logtype_default, "tsockfd_create: listen: %s\n", strerror(errno));
            continue;
        }

        /* We got a socket */
        break;
    }

    if (p == NULL)  {
        LOG(log_error, logtype_default, "tsockfd_create: no suitable network config %s:%s", host, port);
        freeaddrinfo(servinfo);
        return -1;
    }

    freeaddrinfo(servinfo);
    return sockfd;
}

/* --------------------- */
int usockfd_check(int sockfd, const sigset_t *sigset)
{
    int fd;
    socklen_t size;
    fd_set readfds;
    int ret;
     
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);

    if ((ret = pselect(sockfd + 1, &readfds, NULL, NULL, NULL, sigset)) < 0) {
        if (errno == EINTR)
            return 0;
        LOG(log_error, logtype_cnid, "error in select: %s",
            strerror(errno));
        return -1;
    }

    if (ret) {
        size = 0;
        if ((fd = accept(sockfd, NULL, &size)) < 0) {
            if (errno == EINTR)
                return 0;
            LOG(log_error, logtype_cnid, "error in accept: %s", 
                strerror(errno));
            return -1;
        }
        return fd;
    } else
        return 0;
}
