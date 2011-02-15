/*
 * $Id: dsi_tcp.c,v 1.25 2009-12-08 22:34:37 didg Exp $
 *
 * Copyright (c) 1997, 1998 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 *
 * this provides both proto_open() and proto_close() to account for
 * protocol specific initialization and shutdown procedures. all the
 * read/write stuff is done in dsi_stream.c.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#define USE_TCP_NODELAY

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <errno.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */

#include <sys/ioctl.h>
#ifdef TRU64
#include <sys/mbuf.h>
#include <net/route.h>
#endif /* TRU64 */
#include <net/if.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <signal.h>
#include <atalk/logger.h>

#ifdef __svr4__
#include <sys/sockio.h>
#endif /* __svr4__ */

#ifdef TCPWRAP
#include <tcpd.h>
int allow_severity = log_info;
int deny_severity = log_warning;
#endif /* TCPWRAP */

#include <atalk/dsi.h>
#include <atalk/compat.h>
#include <atalk/util.h>
#include <netatalk/endian.h>
#include "dsi_private.h"

#define min(a,b)  ((a) < (b) ? (a) : (b))

#ifndef DSI_TCPMAXPEND
#define DSI_TCPMAXPEND      20       /* max # of pending connections */
#endif /* DSI_TCPMAXPEND */

#ifndef DSI_TCPTIMEOUT
#define DSI_TCPTIMEOUT      120     /* timeout in seconds for connections */
#endif /* ! DSI_TCPTIMEOUT */


/* FIXME/SOCKLEN_T: socklen_t is a unix98 feature. */
#ifndef SOCKLEN_T
#define SOCKLEN_T unsigned int
#endif /* ! SOCKLEN_T */

static void dsi_tcp_close(DSI *dsi)
{
    if (dsi->socket == -1)
        return;

    close(dsi->socket);
    dsi->socket = -1;
}

/* alarm handler for tcp_open */
static void timeout_handler(int sig _U_)
{
    LOG(log_error, logtype_dsi, "dsi_tcp_open: connection timed out");
    exit(EXITERR_CLNT);
}

static struct itimerval itimer;
/* accept the socket and do a little sanity checking */
static int dsi_tcp_open(DSI *dsi)
{
    pid_t pid;
    SOCKLEN_T len;

    len = sizeof(dsi->client);
    dsi->socket = accept(dsi->serversock, (struct sockaddr *) &dsi->client, &len);

#ifdef TCPWRAP
    {
        struct request_info req;
        request_init(&req, RQ_DAEMON, dsi->program, RQ_FILE, dsi->socket, NULL);
        fromhost(&req);
        if (!hosts_access(&req)) {
            LOG(deny_severity, logtype_dsi, "refused connect from %s", eval_client(&req));
            close(dsi->socket);
            errno = ECONNREFUSED;
            dsi->socket = -1;
        }
    }
#endif /* TCPWRAP */

    if (dsi->socket < 0)
        return -1;

    getitimer(ITIMER_PROF, &itimer);
    if (0 == (pid = fork()) ) { /* child */
        static struct itimerval timer = {{0, 0}, {DSI_TCPTIMEOUT, 0}};
        struct sigaction newact, oldact;
        u_int8_t block[DSI_BLOCKSIZ];
        size_t stored;

        /* Immediateyl mark globally that we're a child now */
        parent_or_child = 1;

        /* reset signals */
        server_reset_signal();

#ifndef DEBUGGING
        /* install an alarm to deal with non-responsive connections */
        newact.sa_handler = timeout_handler;
        sigemptyset(&newact.sa_mask);
        newact.sa_flags = 0;
        sigemptyset(&oldact.sa_mask);
        oldact.sa_flags = 0;
        setitimer(ITIMER_PROF, &itimer, NULL);

        if ((sigaction(SIGALRM, &newact, &oldact) < 0) ||
            (setitimer(ITIMER_REAL, &timer, NULL) < 0)) {
            LOG(log_error, logtype_dsi, "dsi_tcp_open: %s", strerror(errno));
            exit(EXITERR_SYS);
        }
#endif

        /* read in commands. this is similar to dsi_receive except
         * for the fact that we do some sanity checking to prevent
         * delinquent connections from causing mischief. */

        /* read in the first two bytes */
        len = dsi_stream_read(dsi, block, 2);
        if (!len ) {
            /* connection already closed, don't log it (normal OSX 10.3 behaviour) */
            exit(EXITERR_CLNT);
        }
        if (len < 2 || (block[0] > DSIFL_MAX) || (block[1] > DSIFUNC_MAX)) {
            LOG(log_error, logtype_dsi, "dsi_tcp_open: invalid header");
            exit(EXITERR_CLNT);
        }

        /* read in the rest of the header */
        stored = 2;
        while (stored < DSI_BLOCKSIZ) {
            len = dsi_stream_read(dsi, block + stored, sizeof(block) - stored);
            if (len > 0)
                stored += len;
            else {
                LOG(log_error, logtype_dsi, "dsi_tcp_open: stream_read: %s", strerror(errno));
                exit(EXITERR_CLNT);
            }
        }

        dsi->header.dsi_flags = block[0];
        dsi->header.dsi_command = block[1];
        memcpy(&dsi->header.dsi_requestID, block + 2,
               sizeof(dsi->header.dsi_requestID));
        memcpy(&dsi->header.dsi_code, block + 4, sizeof(dsi->header.dsi_code));
        memcpy(&dsi->header.dsi_len, block + 8, sizeof(dsi->header.dsi_len));
        memcpy(&dsi->header.dsi_reserved, block + 12,
               sizeof(dsi->header.dsi_reserved));
        dsi->clientID = ntohs(dsi->header.dsi_requestID);

        /* make sure we don't over-write our buffers. */
        dsi->cmdlen = min(ntohl(dsi->header.dsi_len), DSI_CMDSIZ);

        stored = 0;
        while (stored < dsi->cmdlen) {
            len = dsi_stream_read(dsi, dsi->commands + stored, dsi->cmdlen - stored);
            if (len > 0)
                stored += len;
            else {
                LOG(log_error, logtype_dsi, "dsi_tcp_open: stream_read: %s", strerror(errno));
                exit(EXITERR_CLNT);
            }
        }

        /* stop timer and restore signal handler */
#ifndef DEBUGGING
        memset(&timer, 0, sizeof(timer));
        setitimer(ITIMER_REAL, &timer, NULL);
        sigaction(SIGALRM, &oldact, NULL);
#endif

        LOG(log_info, logtype_dsi, "AFP/TCP session from %s:%u",
            getip_string((struct sockaddr *)&dsi->client),
            getip_port((struct sockaddr *)&dsi->client));
    }

    /* send back our pid */
    return pid;
}

/* get it from the interface list */
#ifndef IFF_SLAVE
#define IFF_SLAVE 0
#endif

static void guess_interface(DSI *dsi, const char *hostname, const char *port)
{
    int fd;
    char **start, **list;
    struct ifreq ifr;
    struct sockaddr_in *sa = (struct sockaddr_in *)&dsi->server;

    start = list = getifacelist();
    if (!start)
        return;
        
    fd = socket(PF_INET, SOCK_STREAM, 0);

    while (list && *list) {
        strlcpy(ifr.ifr_name, *list, sizeof(ifr.ifr_name));
        list++;


        if (ioctl(dsi->serversock, SIOCGIFFLAGS, &ifr) < 0)
            continue;

        if (ifr.ifr_flags & (IFF_LOOPBACK | IFF_POINTOPOINT | IFF_SLAVE))
            continue;

        if (!(ifr.ifr_flags & (IFF_UP | IFF_RUNNING)) )
            continue;

        if (ioctl(fd, SIOCGIFADDR, &ifr) < 0)
            continue;

        memset(&dsi->server, 0, sizeof(struct sockaddr_storage));
        sa->sin_family = AF_INET;
        sa->sin_port = htons(atoi(port));
        sa->sin_addr = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr;

        LOG(log_info, logtype_dsi, "dsi_tcp: '%s:%s' on interface '%s' will be used instead.",
            getip_string((struct sockaddr *)&dsi->server), port, ifr.ifr_name);
        goto iflist_done;
    }
    LOG(log_info, logtype_dsi, "dsi_tcp (Chooser will not select afp/tcp) "
        "Check to make sure %s is in /etc/hosts and the correct domain is in "
        "/etc/resolv.conf: %s", hostname, strerror(errno));

iflist_done:
    close(fd);
    freeifacelist(start);
}


#ifndef AI_NUMERICSERV
#define AI_NUMERICSERV 0
#endif

/* this needs to accept passed in addresses */
int dsi_tcp_init(DSI *dsi, const char *hostname, const char *address,
                 const char *port, const int proxy)
{
    int                ret;
    int                flag;
    struct addrinfo    hints, *servinfo, *p;

    dsi->protocol = DSI_TCPIP;

    /* Prepare hint for getaddrinfo */
    memset(&hints, 0, sizeof hints);
#if !defined(FREEBSD)
    hints.ai_family = AF_UNSPEC;
#endif
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_NUMERICSERV;

    if ( ! address) {
        hints.ai_flags |= AI_PASSIVE;
#if defined(FREEBSD)
        hints.ai_family = AF_INET6;
#endif
    } else {
        hints.ai_flags |= AI_NUMERICHOST;
#if defined(FREEBSD)
        hints.ai_family = AF_UNSPEC;
#endif
    }
    if ((ret = getaddrinfo(address ? address : NULL, port ? port : "548", &hints, &servinfo)) != 0) {
        LOG(log_error, logtype_dsi, "dsi_tcp_init: getaddrinfo: %s\n", gai_strerror(ret));
        return 0;
    }

    /* create a socket */
    if (proxy)
        dsi->serversock = -1;
    else {
        /* loop through all the results and bind to the first we can */
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((dsi->serversock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
                LOG(log_info, logtype_dsi, "dsi_tcp_init: socket: %s", strerror(errno));
                continue;
            }

            /*
             * Set some socket options:
             * SO_REUSEADDR deals w/ quick close/opens
             * TCP_NODELAY diables Nagle
             */
#ifdef SO_REUSEADDR
            flag = 1;
            setsockopt(dsi->serversock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
#endif
#if defined(FREEBSD) && defined(IPV6_BINDV6ONLY)
            int on = 0;
            setsockopt(dsi->serversock, IPPROTO_IPV6, IPV6_BINDV6ONLY, (char *)&on, sizeof (on));
#endif

#ifdef USE_TCP_NODELAY
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif
            flag = 1;
            setsockopt(dsi->serversock, SOL_TCP, TCP_NODELAY, &flag, sizeof(flag));
#endif /* USE_TCP_NODELAY */
            
            if (bind(dsi->serversock, p->ai_addr, p->ai_addrlen) == -1) {
                close(dsi->serversock);
                LOG(log_info, logtype_dsi, "dsi_tcp_init: bind: %s\n", strerror(errno));
                continue;
            }

            if (listen(dsi->serversock, DSI_TCPMAXPEND) < 0) {
                close(dsi->serversock);
                LOG(log_info, logtype_dsi, "dsi_tcp_init: listen: %s\n", strerror(errno));
                continue;
            }
            
            break;
        }

        if (p == NULL)  {
            LOG(log_error, logtype_dsi, "dsi_tcp_init: no suitable network config for TCP socket");
            freeaddrinfo(servinfo);
            return 0;
        }

        /* Copy struct sockaddr to struct sockaddr_storage */
        memcpy(&dsi->server, p->ai_addr, p->ai_addrlen);
        freeaddrinfo(servinfo);
    } /* if (proxy) */

    /* Point protocol specific functions to tcp versions */
    dsi->proto_open = dsi_tcp_open;
    dsi->proto_close = dsi_tcp_close;

    /* get real address for GetStatus. */

    if (address) {
        /* address is a parameter, use it 'as is' */
        return 1;
    }

    /* Prepare hint for getaddrinfo */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((ret = getaddrinfo(hostname, port ? port : "548", &hints, &servinfo)) != 0) {
        LOG(log_info, logtype_dsi, "dsi_tcp_init: getaddrinfo '%s': %s\n", hostname, gai_strerror(ret));
        goto interfaces;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            if ( (ipv4->sin_addr.s_addr & htonl(0x7f000000)) != htonl(0x7f000000) )
                break;
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            unsigned char ipv6loopb[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
            if ((memcmp(ipv6->sin6_addr.s6_addr, ipv6loopb, 16)) != 0)
                break;
        }
    }

    if (p) {
        /* Store found address in dsi->server */
        memcpy(&dsi->server, p->ai_addr, p->ai_addrlen);
        freeaddrinfo(servinfo);
        return 1;
    }
    LOG(log_info, logtype_dsi, "dsi_tcp: hostname '%s' resolves to loopback address", hostname);
    freeaddrinfo(servinfo);

interfaces:
    guess_interface(dsi, hostname, port ? port : "548");
    return 1;
}

