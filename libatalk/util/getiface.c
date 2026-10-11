/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 1999-2000 Adrian Sun.
 * All Rights Reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#ifdef __svr4__
#include <sys/sockio.h>
#endif

#include <sys/types.h>

#include <atalk/util.h>

/* allocation size for interface list. */
#define IFACE_NUM 5

/*!
 * @brief Append an interface name to a NULL-terminated list
 *
 * The list needs room for the name and the NULL after it.
 *
 * @param[in,out] list  name list
 * @param[in,out] i     number of names in list, incremented on success
 * @param[in]     name  interface name
 * @returns 0, or -1 when strdup fails
 */
static int addname(char **list, int *i, const char *name)
{
    /* if we've run out of room, allocate some more. just return
     * the present list if we can't. */
    if ((list[*i] = strdup(name)) == NULL) {
        return -1;
    }

    (*i)++;
    list[*i] = NULL; /* zero out the next entry */
    return 0;
}


/*!
 * @brief Build a NULL-terminated list of the interface names of the system
 *
 * The call reads the names from if_nameindex, or from SIOCGIFCONF on sockfd
 * where if_nameindex is missing. A failed strdup ends the list early.
 *
 * @param[in]  sockfd  socket for SIOCGIFCONF, unused with if_nameindex
 * @param[out] list    receives the list, and with a return of 0 it holds no
 *                     names and needs only free
 * @returns the number of names, 0 without names or after a failure
 */
static int getifaces(const int sockfd, char ***list)
{
#ifdef HAVE_IFNAMEINDEX
    struct if_nameindex *ifstart, *ifs;
    int i = 0, count = 0;
    char **new;
    ifs = ifstart = if_nameindex();

    if (!ifstart) {
        return 0;
    }

    while (ifs->if_name) {
        count++;
        ifs++;
    }

    ifs = ifstart;
    new = (char **) malloc((count + 1) * sizeof(char *));

    if (!new) {
        if_freenameindex(ifstart);
        return 0;
    }

    while (ifs->if_name) {
        /* just bail if there's a problem */
        if (addname(new, &i, ifs->if_name) < 0) {
            break;
        }

        ifs++;
    }

    if_freenameindex(ifstart);
    *list = new;
    return i;
#else
    struct ifconf	ifc;
    struct ifreq	ifrs[64], ifrbuf;
    int			ifrsize, i = 0;
    char *nextifrp;
    char **new;

    if (!list) {
        return 0;
    }

    memset(&ifc, 0, sizeof(struct ifconf));
    ifc.ifc_len = sizeof(ifrs);
    memset(ifrs, 0, sizeof(ifrs));
    ifc.ifc_buf = (caddr_t)ifrs;

    if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
        return 0;
    }

    new = (char **) malloc((ifc.ifc_len / sizeof(struct ifreq) + 1) * sizeof(
                               char *));

    for (char *ifrp = ifc.ifc_buf; ifc.ifc_len >= (int) sizeof(struct ifreq);
            ifc.ifc_len -= ifrsize, ifrp = nextifrp) {
        memcpy(&ifrbuf, ifrp, sizeof(ifrbuf));
#ifdef BSD4_4
        ifrsize = sizeof(ifrbuf.ifr_name) +
                  (ifrbuf.ifr_addr.sa_len > sizeof(struct sockaddr)
                   ? ifrbuf.ifr_addr.sa_len : sizeof(struct sockaddr));
#else /* !BSD4_4 */
        ifrsize = sizeof(struct ifreq);
#endif /* BSD4_4 */
        nextifrp = ifrp + ifrsize;

        /* just bail if there's a problem */
        if (addname(new, &i, ifrbuf.ifr_name) < 0) {
            break;
        }
    }

    *list = new;
    return i;
#endif
}


/*!
 * @brief Return the names of the network interfaces of the system
 *
 * The list ends with a NULL entry, and freeifacelist releases it.
 *
 * @returns the list, or NULL without a socket or without any names
 */
char **getifacelist(void)
{
    char **list = NULL; /* FIXME */
    int  i, fd;

    if ((fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        return NULL;
    }

    if ((i = getifaces(fd, &list)) == 0) {
        free(list);
        close(fd);
        return NULL;
    }

    close(fd);
    return list;
}


/*!
 * @brief Free a list from getifacelist and its names
 *
 * @param[in] ifacelist  list, or NULL
 */
void freeifacelist(char **ifacelist)
{
    char *value, **list = ifacelist;

    if (!ifacelist) {
        return;
    }

    while ((value = *list++)) {
        free(value);
    }

    free(ifacelist);
}
