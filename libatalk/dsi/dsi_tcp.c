/*
 * $Id: dsi_tcp.c,v 1.11 2007-02-17 03:25:17 didg Exp $
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
static void timeout_handler()
{
  LOG(log_error, logtype_default, "dsi_tcp_open: connection timed out");
  exit(EXITERR_CLNT);
}

#ifdef ATACC
#define fork aTaC_fork
#endif

static struct itimerval itimer;
/* accept the socket and do a little sanity checking */
static int dsi_tcp_open(DSI *dsi)
{
  pid_t pid;
  SOCKLEN_T len;

  len = sizeof(dsi->client);
  dsi->socket = accept(dsi->serversock, (struct sockaddr *) &dsi->client,
		       &len);

#ifdef TCPWRAP
  {
    struct request_info req;
    request_init(&req, RQ_DAEMON, dsi->program, RQ_FILE, dsi->socket, NULL);
    fromhost(&req);
    if (!hosts_access(&req)) {
      LOG(deny_severity, logtype_default, "refused connect from %s", eval_client(&req));
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
    
    /* reset signals */
    server_reset_signal();

    /* install an alarm to deal with non-responsive connections */
    newact.sa_handler = timeout_handler;
    sigemptyset(&newact.sa_mask);
    newact.sa_flags = 0;
    sigemptyset(&oldact.sa_mask);
    oldact.sa_flags = 0;
    setitimer(ITIMER_PROF, &itimer, NULL);

    if ((sigaction(SIGALRM, &newact, &oldact) < 0) ||
        (setitimer(ITIMER_REAL, &timer, NULL) < 0)) {
	LOG(log_error, logtype_default, "dsi_tcp_open: %s", strerror(errno));
	exit(EXITERR_SYS);
    }
    
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
      LOG(log_error, logtype_default, "dsi_tcp_open: invalid header");
      exit(EXITERR_CLNT);
    }      
    
    /* read in the rest of the header */
    stored = 2;
    while (stored < DSI_BLOCKSIZ) {
      len = dsi_stream_read(dsi, block + stored, sizeof(block) - stored);
      if (len > 0)
	stored += len;
      else {
	LOG(log_error, logtype_default, "dsi_tcp_open: stream_read: %s", strerror(errno));
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
	LOG(log_error, logtype_default, "dsi_tcp_open: stream_read: %s", strerror(errno));
	exit(EXITERR_CLNT);
      }
    }
    
    /* stop timer and restore signal handler */
    memset(&timer, 0, sizeof(timer));
    setitimer(ITIMER_REAL, &timer, NULL);
    sigaction(SIGALRM, &oldact, NULL);

    LOG(log_info, logtype_default,"ASIP session:%u(%d) from %s:%u(%d)", 
	   ntohs(dsi->server.sin_port), dsi->serversock, 
	   inet_ntoa(dsi->client.sin_addr), ntohs(dsi->client.sin_port),
	   dsi->socket);
  }
  
  /* send back our pid */
  return pid;
}

/* this needs to accept passed in addresses */
int dsi_tcp_init(DSI *dsi, const char *hostname, const char *address,
		 const u_int16_t ipport, const int proxy)
{
  struct servent     *service;
  struct hostent     *host;
  int                port;

  dsi->protocol = DSI_TCPIP;

  /* create a socket */
  if (proxy)
    dsi->serversock = -1;
  else if ((dsi->serversock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    return 0;
      
  /* find port */
  if (ipport)
    port = htons(ipport);
  else if ((service = getservbyname("afpovertcp", "tcp")))
    port = service->s_port;
  else
    port = htons(DSI_AFPOVERTCP_PORT);

  /* find address */
  if (!address) 
    dsi->server.sin_addr.s_addr = htonl(INADDR_ANY);
  else if (inet_aton(address, &dsi->server.sin_addr) == 0) {
    LOG(log_info, logtype_default, "dsi_tcp: invalid address (%s)", address);
    return 0;
  }

  dsi->server.sin_family = AF_INET;
  dsi->server.sin_port = port;

  if (!proxy) {
    /* this deals w/ quick close/opens */    
#ifdef SO_REUSEADDR
    port = 1;
    setsockopt(dsi->serversock, SOL_SOCKET, SO_REUSEADDR, &port, sizeof(port));
#endif

#ifdef USE_TCP_NODELAY 

#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif 

    port = 1;
    setsockopt(dsi->serversock, SOL_TCP, TCP_NODELAY, &port, sizeof(port));
#endif /* USE_TCP_NODELAY */

    /* now, bind the socket and set it up for listening */
    if ((bind(dsi->serversock, (struct sockaddr *) &dsi->server, 
	      sizeof(dsi->server)) < 0) || 
	(listen(dsi->serversock, DSI_TCPMAXPEND) < 0)) {
      close(dsi->serversock);
      return 0;
    }
  }

  /* Point protocol specific functions to tcp versions */
  dsi->proto_open = dsi_tcp_open;
  dsi->proto_close = dsi_tcp_close;

  /* get real address for GetStatus. we'll go through the list of 
   * interfaces if necessary. */

  if (address) {
      /* address is a parameter, use it 'as is' */
      return 1;
  }
  
  if (!(host = gethostbyname(hostname)) ) { /* we can't resolve the name */

      LOG(log_info, logtype_default, "dsi_tcp: cannot resolve hostname '%s'", hostname);
      if (proxy) {
         /* give up we have nothing to advertise */
         return 0;
      }
  }
  else {
      if (((struct in_addr *) host->h_addr)->s_addr !=  htonl(0x7F000001)) { /* FIXME ugly check */
          dsi->server.sin_addr.s_addr = ((struct in_addr *) host->h_addr)->s_addr;
          return 1;
      }
      LOG(log_info, logtype_default, "dsi_tcp: hostname '%s' resolves to loopback address", hostname);
  }
  {
      char **start, **list;
      struct ifreq ifr;

      /* get it from the interface list */
      start = list = getifacelist();
      while (list && *list) {
          strlcpy(ifr.ifr_name, *list, sizeof(ifr.ifr_name));
	  list++;

#ifndef IFF_SLAVE
#define IFF_SLAVE 0
#endif

	  if (ioctl(dsi->serversock, SIOCGIFFLAGS, &ifr) < 0)
	    continue;

	  if (ifr.ifr_flags & (IFF_LOOPBACK | IFF_POINTOPOINT | IFF_SLAVE))
	    continue;

	  if (!(ifr.ifr_flags & (IFF_UP | IFF_RUNNING)) )
	    continue;

	  if (ioctl(dsi->serversock, SIOCGIFADDR, &ifr) < 0)
	    continue;
	
	  dsi->server.sin_addr.s_addr = 
	    ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;
	  LOG(log_info, logtype_default, "dsi_tcp: '%s' on interface '%s' will be used instead.",
	       inet_ntoa(dsi->server.sin_addr), ifr.ifr_name);
	  goto iflist_done;
      }
      LOG(log_info, logtype_default, "dsi_tcp (Chooser will not select afp/tcp) \
Check to make sure %s is in /etc/hosts and the correct domain is in \
/etc/resolv.conf: %s", hostname, strerror(errno));

iflist_done:
      if (start)
          freeifacelist(start);
  }

  return 1;

}

