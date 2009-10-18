/*
 * $Id: usockfd.c,v 1.4 2009-10-18 19:02:43 didg Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
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
#include "usockfd.h"

#include <sys/select.h>

int usockfd_create(char *usock_fn, mode_t mode, int backlog)
{
    int sockfd;
    struct sockaddr_un addr;


    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        LOG(log_error, logtype_cnid, "error in socket call: %s",
            strerror(errno));
        return -1;
    }
     
    if (unlink(usock_fn) < 0 && errno != ENOENT) {
        LOG(log_error, logtype_cnid, "error unlinking unix socket file %s: %s",
            usock_fn, strerror(errno));
        return -1;
    }
    memset((char *) &addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, usock_fn, sizeof(addr.sun_path) - 1);
    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) < 0) {
        LOG(log_error, logtype_cnid, "error binding to socket for %s: %s",
            usock_fn, strerror(errno));
        return -1;
    }

    if (listen(sockfd, backlog) < 0) {
        LOG(log_error, logtype_cnid, "error in listen for %s: %s",
            usock_fn, strerror(errno));
        return -1;
    }

    if (chmod(usock_fn, mode) < 0) {
        LOG(log_error, logtype_cnid, "error changing permissions for %s: %s",
            usock_fn, strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/* ---------------
   create a tcp socket (should share dsi stuff)
*/
int tsockfd_create(char *host, u_int16_t ipport, int backlog)
{
    int sockfd;
    struct sockaddr_in server;
    struct hostent     *hp;  
    int                port;
    
    hp=gethostbyname(host);
    if (!hp) {
        unsigned long int addr=inet_addr(host);
        if (addr!= (unsigned)-1)
            hp=gethostbyaddr((char*)addr,sizeof(addr),AF_INET);
 
        if (!hp) {
            LOG(log_error, logtype_cnid, "gethostbyaddr %s: %s", host, strerror(errno));
            return -1;
        }
    }
    memcpy((char*)&server.sin_addr,(char*)hp->h_addr,sizeof(server.sin_addr));    

    if ((sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        LOG(log_error, logtype_cnid, "error in socket call: %s", strerror(errno));
        return -1;
    }
     
    port = htons(ipport);
    
    server.sin_family = AF_INET;
    server.sin_port = port;

#ifdef SO_REUSEADDR
    port = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &port, sizeof(port));
#endif /* SO_REUSEADDR */

#ifdef USE_TCP_NODELAY 
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif /* ! SOL_TCP */
    port = 1;
    setsockopt(sockfd, SOL_TCP, TCP_NODELAY, &port, sizeof(port));
#endif /* USE_TCP_NODELAY */

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        LOG(log_error, logtype_cnid, "error binding to socket for %s: %s",
            host, strerror(errno));
        return -1;
    }

    if (listen(sockfd, backlog) < 0) {
        LOG(log_error, logtype_cnid, "error in listen for %s: %s",
            host, strerror(errno));
        return -1;
    }

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
