/*
 * $Id: inet_aton.c,v 1.3 2001-06-29 14:14:46 rufustfirefly Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int _inet_aton_dummy;

#if defined(ultrix) || (defined(sun) && defined(__svr4__))
#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned) 0xffffffff)
#endif /* ! INADDR_NONE */

int inet_aton(const char *name, struct in_addr *addr)
{
  if ((addr->s_addr = inet_addr(name)) == htonl(INADDR_NONE))
    return 0;

  return 1;
}
#endif /* ultrix || ( sun && __svr4__ ) */
