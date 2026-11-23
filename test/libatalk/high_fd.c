/*
 * Socket pairs above FD_SETSIZE for the tests of waits that take them
 *
 * Copyright (c) 2026 Andy Lemin (andylemin)
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

/*!
 * @file
 * @brief Socket pairs above FD_SETSIZE for the tests of waits that take them
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "high_fd.h"

/*!
 * @brief Make a stream socket pair whose second end is above FD_SETSIZE,
 *        raising the soft descriptor limit for it
 *
 * @param[out] pair   the low end, then the high end
 * @param[out] saved  the limits before the call, for the caller to restore
 * @returns 1; 0 when the limits allow no descriptor that high, or -1 when the
 *          socket pair fails, both with nothing open and the limits restored
 */
int high_fd_pair(int pair[2], struct rlimit *saved)
{
    struct rlimit raised;
    int hi;

    if (getrlimit(RLIMIT_NOFILE, saved) != 0
            || saved->rlim_max < FD_SETSIZE + 16) {
        return 0;
    }

    raised = *saved;

    if (raised.rlim_cur < FD_SETSIZE + 16) {
        raised.rlim_cur = FD_SETSIZE + 16;
    }

    if (setrlimit(RLIMIT_NOFILE, &raised) != 0) {
        return 0;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        setrlimit(RLIMIT_NOFILE, saved);
        return -1;
    }

    if ((hi = fcntl(pair[1], F_DUPFD, FD_SETSIZE + 8)) == -1) {
        close(pair[0]);
        close(pair[1]);
        setrlimit(RLIMIT_NOFILE, saved);
        return 0;
    }

    close(pair[1]);
    pair[1] = hi;
    return 1;
}
