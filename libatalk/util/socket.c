/*
   Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

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
 * Netatalk utility functions
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include <atalk/logger.h>

static char ipv4mapprefix[] = {0,0,0,0,0,0,0,0,0,0,0xff,0xff};

/*!
 * @brief set or unset non-blocking IO on a fd
 *
 * @param     fd         (r) File descriptor
 * @param     cmd        (r) 0: disable non-blocking IO, ie block\n
 *                           <>0: enable non-blocking IO
 *
 * @returns   0 on success, -1 on failure
 */
int setnonblock(int fd, int cmd)
{
    int ofdflags;
    int fdflags;

    if ((fdflags = ofdflags = fcntl(fd, F_GETFL, 0)) == -1)
        return -1;

    if (cmd)
        fdflags |= O_NONBLOCK;
    else
        fdflags &= ~O_NONBLOCK;

    if (fdflags != ofdflags)
        if (fcntl(fd, F_SETFL, fdflags) == -1)
            return -1;

    return 0;
}

/*!
 * non-blocking drop-in replacement for read with timeout using select
 *
 * @param socket          (r)  socket, if in blocking mode, pass "setnonblocking" arg as 1
 * @param data            (rw) buffer for the read data
 * @param lenght          (r)  how many bytes to read
 * @param setnonblocking  (r)  when non-zero this func will enable and disable non blocking
 *                             io mode for the socket
 * @param timeout         (r)  number of seconds to try reading
 *
 * @returns number of bytes actually read or -1 on fatal error
 */
ssize_t readt(int socket, void *data, const size_t length, int setnonblocking, int timeout)
{
    size_t stored;
    ssize_t len;
    struct timeval now, end, tv;
    fd_set rfds;
    int ret;

    stored = 0;

    if (setnonblocking) {
        if (setnonblock(socket, 1) != 0)
            return -1;
    }

    /* Calculate end time */
    (void)gettimeofday(&now, NULL);
    end = now;
    end.tv_sec += timeout;

    while (stored < length) {
        len = read(socket, (char *) data + stored, length - stored);
        if (len == -1) {
            switch (errno) {
            case EINTR:
                continue;
            case EAGAIN:
                FD_ZERO(&rfds);
                FD_SET(socket, &rfds);
                tv.tv_usec = 0;
                tv.tv_sec  = timeout;
                        
                while ((ret = select(socket + 1, &rfds, NULL, NULL, &tv)) < 1) {
                    switch (ret) {
                    case 0:
                        LOG(log_warning, logtype_afpd, "select timeout %d s", timeout);
                        goto exit;

                    default: /* -1 */
                        if (errno == EINTR) {
                            (void)gettimeofday(&now, NULL);
                            if (now.tv_sec >= end.tv_sec && now.tv_usec >= end.tv_usec) {
                                LOG(log_warning, logtype_afpd, "select timeout %d s", timeout);
                                goto exit;
                            }
                            if (now.tv_usec > end.tv_usec) {
                                tv.tv_usec = 1000000 + end.tv_usec - now.tv_usec;
                                tv.tv_sec  = end.tv_sec - now.tv_sec - 1;
                            } else {
                                tv.tv_usec = end.tv_usec - now.tv_usec;
                                tv.tv_sec  = end.tv_sec - now.tv_sec;
                            }
                            FD_ZERO(&rfds);
                            FD_SET(socket, &rfds);
                            continue;
                        }
                        LOG(log_error, logtype_afpd, "select: %s", strerror(errno));
                        stored = -1;
                        goto exit;
                    }
                } /* while (select) */
                continue;
            } /* switch (errno) */
            LOG(log_error, logtype_afpd, "read: %s", strerror(errno));
            stored = -1;
            goto exit;
        } /* (len == -1) */
        else if (len > 0)
            stored += len;
        else
            break;
    } /* while (stored < length) */

exit:
    if (setnonblocking) {
        if (setnonblock(socket, 0) != 0)
            return -1;
    }

    if (len == -1 && stored == 0)
        /* last read or select got an error and we haven't got yet anything => return -1*/
        return -1;
    return stored;
}

/*!
 * @brief convert an IPv4 or IPv6 address to a static string using inet_ntop
 *
 * IPv6 mapped IPv4 addresses are returned as IPv4 addreses eg
 * ::ffff:10.0.0.0 is returned as "10.0.0.0".
 *
 * @param  sa        (r) pointer to an struct sockaddr
 *
 * @returns pointer to a static string cotaining the converted address as string.\n
 *          On error pointers to "0.0.0.0" or "::0" are returned.
 */
const char *getip_string(const struct sockaddr *sa)
{
    static char ip4[INET_ADDRSTRLEN];
    static char ip6[INET6_ADDRSTRLEN];

    switch (sa->sa_family) {

    case AF_INET: {
        const struct sockaddr_in *sai4 = (const struct sockaddr_in *)sa;
        if ((inet_ntop(AF_INET, &(sai4->sin_addr), ip4, INET_ADDRSTRLEN)) == NULL)
            return "0.0.0.0";
        return ip4;
    }
    case AF_INET6: {
        const struct sockaddr_in6 *sai6 = (const struct sockaddr_in6 *)sa;
        if ((inet_ntop(AF_INET6, &(sai6->sin6_addr), ip6, INET6_ADDRSTRLEN)) == NULL)
            return "::0";

        /* Deal with IPv6 mapped IPv4 addresses*/
        if ((memcmp(sai6->sin6_addr.s6_addr, ipv4mapprefix, sizeof(ipv4mapprefix))) == 0)
            return (strrchr(ip6, ':') + 1);
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
 * @param  sa        (r) pointer to an struct sockaddr
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
 * eg 0 <= mask <= 32 for IPv4 or 0<= mask <= 128 for IPv6. mask > 32 for
 * IPv4 is treated as mask = 32, mask > 128 is set to 128 for IPv6.
 *
 * @param  ai        (rw) pointer to an struct sockaddr
 * @parma  mask      (r) number of maskbits
 */
void apply_ip_mask(struct sockaddr *sa, uint32_t mask)
{

    switch (sa->sa_family) {
    case AF_INET: {
        if (mask >= 32)
            return;

        struct sockaddr_in *si = (struct sockaddr_in *)sa;
        uint32_t nmask = mask ? ~((1 << (32 - mask)) - 1) : 0;
        si->sin_addr.s_addr &= htonl(nmask);
        break;
    }
    case AF_INET6: {
        if (mask >= 128)
            return;

        int i, maskbytes, maskbits;
        struct sockaddr_in6 *si6 = (struct sockaddr_in6 *)sa;

        /* Deal with IPv6 mapped IPv4 addresses*/
        if ((memcmp(si6->sin6_addr.s6_addr, ipv4mapprefix, sizeof(ipv4mapprefix))) == 0) {
            mask += 96;
            if (mask >= 128)
                return;
        }

        maskbytes = (128 - mask) / 8; /* maskbytes really are those that will be 0'ed */
        maskbits = mask % 8;

        for (i = maskbytes - 1; i >= 0; i--)
            si6->sin6_addr.s6_addr[15 - i] = 0;
        if (maskbits)
            si6->sin6_addr.s6_addr[15 - maskbytes] &= ~((1 << (8 - maskbits)) - 1);
        break;
    }
    default:
        break;
    }
}

/*!
 * @brief compare IP addresses for equality
 *
 * @param  sa1       (r) pointer to an struct sockaddr
 * @param  sa2       (r) pointer to an struct sockaddr
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
