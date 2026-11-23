/*
   Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>
   Copyright (c) 2026 Andy Lemin (andylemin)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

/*!
 * @file
 * @brief Netatalk utility functions: socket
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/util.h>

static char ipv4mapprefix[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff};

/*!
 * @brief set or unset non-blocking IO on a fd
 *
 * @param[in]     fd         File descriptor
 * @param[in]     cmd        0: disable non-blocking IO, i.e. block\n
 *                           <>0: enable non-blocking IO
 *
 * @returns   0 on success, -1 on failure
 */
int setnonblock(int fd, int cmd)
{
    int ofdflags;
    int fdflags;

    if ((fdflags = ofdflags = fcntl(fd, F_GETFL, 0)) == -1) {
        return -1;
    }

    if (cmd) {
        fdflags |= O_NONBLOCK;
    } else {
        fdflags &= ~O_NONBLOCK;
    }

    if (fdflags != ofdflags)
        if (fcntl(fd, F_SETFL, fdflags) == -1) {
            return -1;
        }

    return 0;
}

/*!
 * @brief Wait up to timeout seconds for socket to be ready for events
 *
 * A signal leaves the wait going for the rest of the timeout. The time
 * running out is logged at level, a poll error at log_error.
 *
 * @param[in] socket   socket
 * @param[in] events   POLLIN or POLLOUT
 * @param[in] timeout  seconds to wait, not negative: 0 to wait without a
 *                     limit; past INT_MAX milliseconds the wait takes INT_MAX
 * @param[in] level    log level of the time running out
 *
 * @returns 1 when poll() reports the socket, 0 with errno EAGAIN when the
 *          time runs out, or -1 with the errno of poll
 */
static int socket_wait(int socket, short events, int timeout,
                       enum loglevels level)
{
    struct pollfd pfd = { .fd = socket, .events = events };
    int ms = !timeout ? -1
             : timeout > INT_MAX / 1000 ? INT_MAX : timeout * 1000;
    long long end = monotonic_ms() + ms;
    int ret;
    int err;

    while ((ret = poll(&pfd, 1, ms)) == -1 && errno == EINTR) {
        if (timeout) {
            long long left = end - monotonic_ms();
            ms = left > 0 ? (int)left : 0;
        }
    }

    if (ret == 0) {
        LOG(level, logtype_afpd, "poll timeout %d s", timeout);
        errno = EAGAIN;
    } else if (ret == -1) {
        err = errno;
        LOG(log_error, logtype_afpd, "poll: %s", strerror(err));
        errno = err;
    }

    return ret;
}

/*!
 * @brief Read length bytes from a socket, waiting for data with poll()
 *
 * @param[in] socket          socket, if in blocking mode, pass "setnonblocking" arg as 1
 * @param[in,out] data        buffer for the read data
 * @param[in] length          how many bytes to read
 * @param[in] setnonblocking  when non-zero this func will enable and disable non blocking
 *                            io mode for the socket
 * @param[in] timeout         seconds each wait for data may take, not
 *                            negative; 0 means no timeout
 *
 * @returns the bytes read, fewer than length when the peer closes the
 *          connection or a wait times out after some arrived; -1 on an error,
 *          or with errno EAGAIN when a wait times out before any arrived
 */
ssize_t readt(int socket, void *data, const size_t length, int setnonblocking,
              int timeout)
{
    size_t stored = 0;
    ssize_t len = 0;
    int ret;

    if (setnonblocking) {
        if (setnonblock(socket, 1) != 0) {
            return -1;
        }
    }

    while (stored < length) {
        len = recv(socket, (char *) data + stored, length - stored, 0);

        if (len == -1) {
            switch (errno) {
            case EINTR:
                continue;

            case EAGAIN:
                ret = socket_wait(socket, POLLIN, timeout, log_debug);

                if (ret == 1) {
                    continue;
                }

                if (ret == -1) {
                    stored = -1;
                }

                goto exit;
            } /* switch (errno) */

            LOG(log_error, logtype_afpd, "read: %s", strerror(errno));
            stored = -1;
            goto exit;
        } /* (len == -1) */
        else if (len > 0) {
            stored += len;
        } else {
            break;
        }
    } /* while (stored < length) */

exit:

    if (setnonblocking) {
        if (setnonblock(socket, 0) != 0) {
            return -1;
        }
    }

    if (len == -1 && stored == 0)
        /* a wait timed out before any byte moved */
    {
        return -1;
    }

    return stored;
}

/*!
 * @brief Write length bytes to a socket, waiting for room with poll()
 *
 * @param[in] socket          socket, if in blocking mode, pass "setnonblocking" arg as 1
 * @param[in] data            bytes to write
 * @param[in] length          how many bytes to write
 * @param[in] setnonblocking  when non-zero this func will enable and disable non blocking
 *                            io mode for the socket
 * @param[in] timeout         seconds each wait for room may take, not
 *                            negative; 0 means no timeout
 *
 * @returns the bytes written, fewer than length when a wait times out after
 *          some went out; -1 on an error, or with errno EAGAIN when a wait
 *          times out before any went out
 */
ssize_t writet(int socket, void *data, const size_t length, int setnonblocking,
               int timeout)
{
    size_t stored = 0;
    ssize_t len = 0;
    int ret;

    if (setnonblocking) {
        if (setnonblock(socket, 1) != 0) {
            return -1;
        }
    }

    while (stored < length) {
        len = write(socket, (char *) data + stored, length - stored);

        if (len == -1) {
            switch (errno) {
            case EINTR:
                continue;

            case EAGAIN:
                ret = socket_wait(socket, POLLOUT, timeout, log_warning);

                if (ret == 1) {
                    continue;
                }

                if (ret == -1) {
                    stored = -1;
                }

                goto exit;
            } /* switch (errno) */

            LOG(log_error, logtype_afpd, "write: %s", strerror(errno));
            stored = -1;
            goto exit;
        } /* (len == -1) */
        else if (len > 0) {
            stored += len;
        } else {
            break;
        }
    } /* while (stored < length) */

exit:

    if (setnonblocking) {
        if (setnonblock(socket, 0) != 0) {
            return -1;
        }
    }

    if (len == -1 && stored == 0)
        /* a wait timed out before any byte moved */
    {
        return -1;
    }

    return stored;
}

/*!
 * @brief Convert an IPv4 or IPv6 address to a static string with inet_ntop
 *
 * An IPv6-mapped IPv4 address comes back as the IPv4 address: `::ffff:10.0.0.0`
 * becomes "10.0.0.0".
 *
 * @param[in] sa  pointer to a struct sockaddr
 * @returns a static string with the address, or "0.0.0.0" or "::0" on error
 */
const char *getip_string(const struct sockaddr *sa)
{
    static char ip4[INET_ADDRSTRLEN];
    static char ip6[INET6_ADDRSTRLEN];

    switch (sa->sa_family) {
    case AF_INET: {
        const struct sockaddr_in *sai4 = (const struct sockaddr_in *)sa;

        if ((inet_ntop(AF_INET, &(sai4->sin_addr), ip4, INET_ADDRSTRLEN)) == NULL) {
            return "0.0.0.0";
        }

        return ip4;
    }

    case AF_INET6: {
        const struct sockaddr_in6 *sai6 = (const struct sockaddr_in6 *)sa;

        if ((inet_ntop(AF_INET6, &(sai6->sin6_addr), ip6, INET6_ADDRSTRLEN)) == NULL) {
            return "::0";
        }

        /* Deal with IPv6 mapped IPv4 addresses*/
        if ((memcmp(sai6->sin6_addr.s6_addr, ipv4mapprefix,
                    sizeof(ipv4mapprefix))) == 0) {
            return strrchr(ip6, ':') + 1;
        }

        return ip6;
    }

    default:
        return "getip_string ERROR";
    }

    /* We never get here */
}

/*!
 * @brief return port number from struct sockaddr
 *
 * @param[in]  sa        pointer to an struct sockaddr
 *
 * @returns port as unsigned int
 */
unsigned int getip_port(const struct sockaddr  *sa)
{
    if (sa->sa_family == AF_INET) { /* IPv4 */
        const struct sockaddr_in *sai4 = (const struct sockaddr_in *)sa;
        return ntohs(sai4->sin_port);
    } else {                       /* IPv6 */
        const struct sockaddr_in6 *sai6 = (const struct sockaddr_in6 *)sa;
        return ntohs(sai6->sin6_port);
    }

    /* We never get here */
}

/*!
 * @brief apply netmask to IP (v4 or v6)
 *
 * Modifies IP address in sa->sin[6]_addr-s[6]_addr. The caller is responsible
 * for passing a value for mask that is sensible to the passed address,
 * e.g. 0 <= mask <= 32 for IPv4 or 0<= mask <= 128 for IPv6. mask > 32 for
 * IPv4 is treated as mask = 32, mask > 128 is set to 128 for IPv6.
 *
 * @param[in,out] sa    pointer to an struct sockaddr
 * @param[in]     mask  number of maskbits
 */
void apply_ip_mask(struct sockaddr *sa, int mask)
{
    switch (sa->sa_family) {
    case AF_INET: {
        if (mask >= 32) {
            return;
        }

        struct sockaddr_in *si = (struct sockaddr_in *)sa;

        uint32_t nmask = mask ? ~((1 << (32 - mask)) - 1) : 0;
        si->sin_addr.s_addr &= htonl(nmask);
        break;
    }

    case AF_INET6: {
        if (mask >= 128) {
            return;
        }

        int i, maskbytes, maskbits;
        struct sockaddr_in6 *si6 = (struct sockaddr_in6 *)sa;

        /* Deal with IPv6 mapped IPv4 addresses*/
        if ((memcmp(si6->sin6_addr.s6_addr, ipv4mapprefix,
                    sizeof(ipv4mapprefix))) == 0) {
            mask += 96;

            if (mask >= 128) {
                return;
            }
        }

        maskbytes = (128 - mask) / 8; /* maskbytes really are those that will be 0'ed */
        maskbits = mask % 8;

        for (i = maskbytes - 1; i >= 0; i--) {
            si6->sin6_addr.s6_addr[15 - i] = 0;
        }

        if (maskbits) {
            si6->sin6_addr.s6_addr[15 - maskbytes] &= ~((1 << (8 - maskbits)) - 1);
        }

        break;
    }

    default:
        break;
    }
}

/*!
 * @brief compare IP addresses for equality
 *
 * @param[in]  sa1       pointer to an struct sockaddr
 * @param[in]  sa2       pointer to an struct sockaddr
 *
 * @returns Addresses are converted to strings and compared with strcmp and
 *          the result of strcmp is returned.
 *
 * @note IPv6 mapped IPv4 addresses are treated as IPv4 addresses.
 */
int compare_ip(const struct sockaddr *sa1, const struct sockaddr *sa2)
{
    int ret;
    char *ip1;
    const char *ip2;
    ip1 = strdup(getip_string(sa1));
    ip2 = getip_string(sa2);
    ret = strcmp(ip1, ip2);
    free(ip1);
    return ret;
}

/*!
 * @brief Tokenize IP(4/6) addresses with an optional port
 * into address and port
 *
 * @param[in] ipurl     IP URL string
 * @param[out] address  IP address
 * @param[out] port     IP port
 *
 * @returns 0 on success, -1 on failure
 *
 * Tokenize IPv4, IPv4:port, IPv6, [IPv6] or [IPv6:port] URL into address and
 * port and return two allocated strings with the address and the port.
 *
 * If the function returns 0, then address point to a newly allocated
 * valid address string, port may either be NULL or point to a newly
 * allocated port number.
 *
 * If the function returns -1, then the contents of address and port are
 * undefined.
 */
int tokenize_ip_port(const char *ipurl, char **address, char **port)
{
    EC_INIT;
    char *p = NULL;
    char *s;
    AFP_ASSERT(ipurl && address && port);
    EC_NULL(p = strdup(ipurl));

    /* Either ipv4, ipv4:port, ipv6, [ipv6] or [ipv6]:port */

    if (!strchr(p, ':')) {
        /* IPv4 address without port */
        *address = p;
        p = NULL;  /* prevent free() */
        *port = NULL;
        EC_EXIT_STATUS(0);
    }

    /* Either ipv4:port, ipv6, [ipv6] or [ipv6]:port */

    if (strchr(p, '.')) {
        /* ipv4:port */
        *address = p;
        p = strchr(p, ':');
        *p = '\0';
        EC_NULL(*port = strdup(p + 1));
        p = NULL; /* prevent free() */
        EC_EXIT_STATUS(0);
    }

    /* Either ipv6, [ipv6] or [ipv6]:port */

    if (p[0] != '[') {
        /* ipv6 */
        *address = p;
        p = NULL;  /* prevent free() */
        *port = NULL;
        EC_EXIT_STATUS(0);
    }

    /* [ipv6] or [ipv6]:port */
    EC_NULL(*address = strdup(p + 1));

    if ((s = strchr(*address, ']')) == NULL) {
        LOG(log_error, logtype_dsi, "tokenize_ip_port: malformed ipv6 address %s\n",
            ipurl);
        EC_FAIL;
    }

    *s = '\0';
    /* address now points to the ipv6 address without [] */

    if (s[1] == ':') {
        /* [ipv6]:port */
        EC_NULL(*port = strdup(s + 2));
    } else {
        /* [ipv6] */
        *port = NULL;
    }

EC_CLEANUP:

    if (p) {
        free(p);
    }

    EC_EXIT;
}

/*!
 * @brief Allocate an empty poll set with room for max descriptors
 *
 * @param[in] max  capacity of the poll and data arrays
 * @returns the set, or NULL when an allocation fails
 */
struct asev *asev_init(int max)
{
    struct asev *asev = calloc(1, sizeof(struct asev));

    if (asev == NULL) {
        return NULL;
    }

    /* Initialize with space for all possibly active fds */
    asev->fdset = calloc(max, sizeof(struct pollfd));
    asev->data = calloc(max, sizeof(struct asev_data));

    if (asev->fdset == NULL || asev->data == NULL) {
        asev_free(asev);
        return NULL;
    }

    asev->max = max;
    asev->used = 0;
    return asev;
}

/*!
 * @brief Free a poll set
 *
 * @param[in] sev  poll set, or NULL
 */
void asev_free(struct asev *sev)
{
    if (sev) {
        free(sev->fdset);
        free(sev->data);
        free(sev);
    }
}

/*!
 * @brief Replace a poll set by one with room for max descriptors beyond the
 *        entries it keeps, in their order
 *
 * @param[in] sev  poll set to replace and free, or NULL
 * @param[in] max  room beyond the kept entries
 * @returns the new set, or NULL with sev unchanged
 */
struct asev *asev_renew(struct asev *sev, int max)
{
    int used = sev ? sev->used : 0;
    struct asev *renewed = asev_init(used + max);

    if (renewed == NULL || sev == NULL) {
        return renewed;
    }

    memcpy(renewed->fdset, sev->fdset, used * sizeof(*sev->fdset));
    memcpy(renewed->data, sev->data, used * sizeof(*sev->data));
    renewed->used = used;
    asev_free(sev);
    return renewed;
}

/*!
 * @brief Append a descriptor to a poll set, watched for POLLIN
 *
 * A second array holds the type, the private data, and the protocol of each
 * entry, at the same index as its pollfd.
 *
 * @param[in,out] asev      poll set
 * @param[in]     fd        descriptor
 * @param[in]     fdtype    type that the poll loop dispatches on
 * @param[in]     private   data of the entry, such as a child or a DSI
 * @param[in]     protocol  AFPPROTO_DSI or AFPPROTO_ASP for a listener, 0
 *                          otherwise
 * @returns true, or false for a NULL or full set
 */
bool asev_add_fd(struct asev *asev,
                 int fd,
                 enum asev_fdtype fdtype,
                 void *private,
                 int protocol)
{
    if (asev == NULL) {
        return false;
    }

    if (!(asev->used < asev->max)) {
        return false;
    }

    asev->fdset[asev->used].fd = fd;
    asev->fdset[asev->used].events = POLLIN;
    asev->data[asev->used].fdtype = fdtype;
    asev->data[asev->used].private = private;
    asev->data[asev->used].protocol = protocol;
    asev->used++;
    return true;
}

/*!
 * @brief Remove a descriptor from a poll set, keeping the order of the others
 *
 * The entries after it move down by one.
 *
 * @param[in,out] asev  poll set
 * @param[in]     fd    descriptor
 * @returns true when the set held fd, false otherwise
 */
bool asev_del_fd(struct asev *asev, int fd)
{
    int i;
    int numafter;

    if (asev == NULL) {
        return false;
    }

    if (asev->used == 0) {
        LOG(log_error, logtype_cnid, "asev_del_fd: empty");
        return false;
    }

    for (i = 0; i < asev->used; i++) {
        /*
         * Scan the array for a matching fd
         */
        if (asev->fdset[i].fd == fd) {
            /*
             * found fd
             */
            if ((i + 1) == asev->used) {
                /*
                 * it's the last (or only) array element, simply null it
                 */
                asev->fdset[i].fd = -1;
                asev->data[i].fdtype = 0;
                asev->data[i].private = NULL;
                asev->data[i].protocol = 0;
            } else {
                /*
                 * Move down by one all subsequent elements
                 */
                numafter = asev->used - (i + 1);
                memmove(&asev->fdset[i], &asev->fdset[i + 1],
                        numafter * sizeof(struct pollfd));
                memmove(&asev->data[i], &asev->data[i + 1],
                        numafter * sizeof(struct asev_data));
            }

            asev->used--;
            return true;
        }
    }

    return false;
}

#ifndef CMSG_SPACE
/*! Length of the space taken up by a padded control message of length len */
#define CMSG_SPACE(len) (__CMSG_ALIGN(sizeof(struct cmsghdr)) + __CMSG_ALIGN(len))
#endif

/*!
 * @brief Receive a descriptor on a PF_UNIX socket
 *
 * @param[in] fd           PF_UNIX socket to receive on
 * @param[in] nonblocking  0 for a blocking fd; 1 for a non-blocking one, on
 *                         which the call waits up to 2 seconds for the message
 * @returns the descriptor, or -1 on error
 */
int recv_fd(int fd, int nonblocking)
{
    int ret;
    struct msghdr msgh;
    struct iovec iov[1];
    struct cmsghdr *cmsgp = NULL;
    char buf[CMSG_SPACE(sizeof(int))];
    char dbuf[80];
    memset(&msgh, 0, sizeof(msgh));
    memset(buf, 0, sizeof(buf));
    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    msgh.msg_iov = iov;
    msgh.msg_iovlen = 1;
    iov[0].iov_base = dbuf;
    iov[0].iov_len = sizeof(dbuf);
    msgh.msg_control = buf;
    msgh.msg_controllen = sizeof(buf);

    if (nonblocking) {
        if (socket_wait(fd, POLLIN, 2, log_debug) != 1) {
            return -1;
        }

        ret = recvmsg(fd, &msgh, 0);
    } else {
        do  {
            ret = recvmsg(fd, &msgh, 0);
        } while (ret == -1 && errno == EINTR);
    }

    if (ret == -1) {
        return -1;
    }

    for (cmsgp = CMSG_FIRSTHDR(&msgh); cmsgp != NULL;
            cmsgp = CMSG_NXTHDR(&msgh, cmsgp)) {
        if (cmsgp->cmsg_level == SOL_SOCKET && cmsgp->cmsg_type == SCM_RIGHTS) {
            return *(int *) CMSG_DATA(cmsgp);
        }
    }

    if (ret == sizeof(int)) {
        /* Rcvd errno */
        errno = *(int *)dbuf;
    } else {
        /* Default errno */
        errno = ENOENT;
    }

    return -1;
}

/*!
 * @brief Pass a descriptor over a Unix socket with SCM_RIGHTS
 *
 * The descriptor needs data to travel with, so the message carries an int of
 * 0. The call repeats sendmsg after EINTR.
 *
 * @param[in] socket  PF_UNIX socket
 * @param[in] fd      descriptor to pass
 * @returns 0, or -1 when malloc or sendmsg fails
 */
int send_fd(int socket, int fd)
{
    int ret;
    struct msghdr msgh;
    struct iovec iov[1];
    struct cmsghdr *cmsgp = NULL;
    char *buf;
    size_t size;
    int er = 0;
    size = CMSG_SPACE(sizeof(fd));
    buf = malloc(size);

    if (!buf) {
        LOG(log_error, logtype_cnid, "error in sendmsg: %s", strerror(errno));
        return -1;
    }

    memset(&msgh, 0, sizeof(msgh));
    memset(buf, 0, size);
    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    msgh.msg_iov = iov;
    msgh.msg_iovlen = 1;
    iov[0].iov_base = &er;
    iov[0].iov_len = sizeof(er);
    msgh.msg_control = buf;
    msgh.msg_controllen = size;
    cmsgp = CMSG_FIRSTHDR(&msgh);
    cmsgp->cmsg_level = SOL_SOCKET;
    cmsgp->cmsg_type = SCM_RIGHTS;
    cmsgp->cmsg_len = CMSG_LEN(sizeof(fd));
    *((int *)CMSG_DATA(cmsgp)) = fd;
    msgh.msg_controllen = cmsgp->cmsg_len;

    do  {
        ret = sendmsg(socket, &msgh, 0);
    } while (ret == -1 && errno == EINTR);

    if (ret == -1) {
        LOG(log_error, logtype_cnid, "error in sendmsg: %s", strerror(errno));
        free(buf);
        return -1;
    }

    free(buf);
    return 0;
}
