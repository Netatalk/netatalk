/*
 * Regression tests for NBP lookup timeouts
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>

#include <atalk/ddp.h>
#include <atalk/netddp.h>
#include <atalk/nbp.h>

#include "nbp_conf.h"

enum { LOOKUP_FD = 3 };

/* Two 200 ms waits, the second crossing a second boundary. */
static const struct timeval lookup_times[] = {
    {1800000000, 100000}, {1800000000, 300000},
    {1800000000, 900000}, {1800000001, 100000},
    {1800000001, 100000}, {1800000003, 100000}, {1800000005, 100000},
};
static const struct timeval expected_timeouts[] = {
    {2, 0}, {1, 800000}, {1, 600000}, {2, 0}, {2, 0},
};

static size_t time_index, select_calls, replies_read;
static bool select_updates_timeout;

static int lookup_gettimeofday(struct timeval *tv, void *zone)
{
    (void)zone;

    if (time_index >= sizeof(lookup_times) / sizeof(lookup_times[0])) {
        errno = EINVAL;
        return -1;
    }

    *tv = lookup_times[time_index++];
    return 0;
}

static int lookup_select(int nfds, fd_set *readfds, fd_set *writefds,
                         fd_set *exceptfds, struct timeval *timeout)
{
    (void)writefds;
    (void)exceptfds;

    if (nfds != LOOKUP_FD + 1 || !FD_ISSET(LOOKUP_FD, readfds) ||
            select_calls >= sizeof(expected_timeouts) / sizeof(expected_timeouts[0])) {
        errno = EINVAL;
        return -1;
    }

    struct timeval expected = expected_timeouts[select_calls++];

    if (timeout->tv_sec != expected.tv_sec
            || timeout->tv_usec != expected.tv_usec) {
        fprintf(stderr, "Wait %zu got %ld s %ld us, expected %ld s %ld us\n",
                select_calls, (long)timeout->tv_sec, (long)timeout->tv_usec,
                (long)expected.tv_sec, (long)expected.tv_usec);
        errno = EINVAL;
        return -1;
    }

    if (select_calls <= 2) {
        if (select_updates_timeout) {
            *timeout = expected_timeouts[select_calls];
        }

        return 1;
    }

    FD_ZERO(readfds);
    return 0;
}

static ssize_t lookup_recvfrom(int socket, void *buf, size_t len, int flags,
                               struct sockaddr *from, socklen_t *fromlen)
{
    struct nbphdr header = { .nh_op = NBPOP_LKUPREPLY, .nh_cnt = 1, .nh_id = nbp_id };
    unsigned char tuple[] = { 0, 70, 1, 125, 0, 1, 'A', 1, 'T', 1, 'Z' };
    unsigned char *reply = buf;
    (void)flags;
    (void)from;
    (void)fromlen;

    if (socket != LOOKUP_FD || len < 1 + SZ_NBPHDR + sizeof(tuple)) {
        errno = EINVAL;
        return -1;
    }

    tuple[SZ_NBPTUPLE + 1] += replies_read++;
    *reply++ = DDPTYPE_NBP;
    memcpy(reply, &header, SZ_NBPHDR);
    memcpy(reply + SZ_NBPHDR, tuple, sizeof(tuple));
    return 1 + SZ_NBPHDR + sizeof(tuple);
}

/* Substitute the clock and socket operations after their headers. */
#define gettimeofday lookup_gettimeofday
#define select lookup_select
#define netddp_open(addr, bridge) LOOKUP_FD
#define netddp_sendto(s, msg, len, flags, to, tolen) ((ssize_t)(len))
#define netddp_recvfrom lookup_recvfrom
#define netddp_close(s) ((void)(s))
#include "../../libatalk/nbp/nbp_lkup.c"
#undef gettimeofday
#undef select
#undef netddp_open
#undef netddp_sendto
#undef netddp_recvfrom
#undef netddp_close

static int test_lookup_timeout(bool updates_timeout)
{
    struct nbpnve results[3] = { 0 };
    time_index = select_calls = replies_read = 0;
    select_updates_timeout = updates_timeout;
    nbp_port = 2;
    int count = nbp_do_lookup_op(NULL, NULL, NULL, results, 3, NULL, NULL,
                                 NBPOP_BRRQ);

    if (count != 2 || replies_read != 2 ||
            select_calls != sizeof(expected_timeouts) / sizeof(expected_timeouts[0]) ||
            results[0].nn_obj[0] != 'A' || results[1].nn_obj[0] != 'B') {
        fprintf(stderr, "Lookup failed with a %s select timeout (returned %d)\n",
                updates_timeout ? "mutable" : "preserved", count);
        return 1;
    }

    return 0;
}

int main(void)
{
    int status = test_lookup_timeout(false);
    status |= test_lookup_timeout(true);
    return status;
}
