/*
 * $Id: inet_aton.c,v 1.4 2003-02-17 01:51:08 srittau Exp $
 */

#include "config.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef sun
#if defined(sun) && defined(__svr4__)
#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned) 0xffffffff)
#endif /* ! INADDR_NONE */

int inet_aton(const char *name, struct in_addr *addr)
{
  if ((addr->s_addr = inet_addr(name)) == htonl(INADDR_NONE))
    return 0;

  return 1;
}
#endif /* sun && __svr4__ */
