/* 
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999-2000 Adrian Sun. 
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#ifdef TRU64
#include <sys/mbuf.h>
#include <net/route.h>
#endif /* TRU64 */
#include <net/if.h>
#include <errno.h>

#ifdef __svr4__
#include <sys/sockio.h>
#endif

#include <atalk/util.h>

/* allocation size for interface list. */
#define IFACE_NUM 5

/* we leave all of the ioctl's to the application */
static int addname(char **list, int *i, int *length, const char *name) 

{
    /* if we've run out of room, allocate some more. just return
     * the present list if we can't. */
     if (*i >= *length) {
      char **new = realloc(list, sizeof(char **)*(*length + IFACE_NUM));
       
      if (!new) /* just break if we can't allocate anything */
	return -1;
      *length += IFACE_NUM;
    }
     
    if ((list[*i] = strdup(name)) == NULL)
      return -1;

    (*i)++;
    list[*i] = NULL; /* zero out the next entry */
    return 0;
}


static int getifaces(const int sockfd, char **list, int *length)
{
#ifdef HAVE_IFNAMEINDEX
      struct if_nameindex *ifstart, *ifs;
      int i = 0;
  
      if (!list || *length < 1) 
	return 0;

      ifs = ifstart = if_nameindex();
      while (ifs && ifs->if_name) {
	/* just bail if there's a problem */
	if (addname(list, &i, length, ifs->if_name) < 0)
	  break;
	ifs++;
      }

      if_freenameindex(ifstart);
      return i;

#else
    struct ifconf	ifc;
    struct ifreq	ifrs[ 64 ], *ifr, *nextifr;
    int			ifrsize, i = 0;

    if (!list || *length < 1)
      return 0;

    memset( &ifc, 0, sizeof( struct ifconf ));
    ifc.ifc_len = sizeof( ifrs );
    memset( ifrs, 0, sizeof( ifrs ));
    ifc.ifc_buf = (caddr_t)ifrs;
    if ( ioctl( sockfd, SIOCGIFCONF, &ifc ) < 0 ) {
	return 0;
    }

    for ( ifr = ifc.ifc_req; ifc.ifc_len >= sizeof( struct ifreq );
	    ifc.ifc_len -= ifrsize, ifr = nextifr ) {
#ifdef BSD4_4
 	ifrsize = sizeof(ifr->ifr_name) +
	  (ifr->ifr_addr.sa_len > sizeof(struct sockaddr)
	   ? ifr->ifr_addr.sa_len : sizeof(struct sockaddr));
#else /* !BSD4_4 */
	ifrsize = sizeof( struct ifreq );
#endif /* BSD4_4 */
	nextifr = (struct ifreq *)((caddr_t)ifr + ifrsize );

	/* just bail if there's a problem */
	if (addname(list, &i, length, ifr->ifr_name) < 0)
	  break;
    }
    return i;
#endif
}


/*
 * Get interfaces from the kernel. we keep an extra null entry to signify
 * the end of the interface list. 
 */
char **getifacelist()
{
  char **list = (char **) malloc(sizeof(char **)*(IFACE_NUM + 1));
  char **new;
  int length = IFACE_NUM, i, fd;

  if (!list)
    return NULL;
      
  if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    return NULL;

  if ((i = getifaces(fd, list, &length)) == 0) {
    free(list);
    close(fd);
    return NULL;
  }
  close(fd);

  if ((i < length) && 
      (new = (char **) realloc(list, sizeof(char **)*(i + 1))))
    return new;

  return list;
}


/* go through and free the interface list */
void freeifacelist(char **ifacelist)
{
  char *value, **list = ifacelist;

  if (!ifacelist)
    return;

  while ((value = *list++)) {
    free(value);
  }

  free(ifacelist);
}
