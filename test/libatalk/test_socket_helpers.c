/*
 * Regression tests for the readt() and writet() timeout helpers.
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atalk/util.h>

ssize_t socket_helpers_recv(int socket, void *buffer, size_t length, int flags);
ssize_t socket_helpers_write(int fd, const void *buffer, size_t length);
int socket_helpers_select(int nfds, fd_set *readfds, fd_set *writefds,
                          fd_set *exceptfds, struct timeval *timeout);

/*
 * Rename libc calls only after system headers have been parsed.  Applying
 * these definitions as compiler arguments breaks fortified libc headers that
 * need to declare and wrap the original functions.
 */
#define recv socket_helpers_recv
#define select socket_helpers_select
#define write socket_helpers_write
#include "../../libatalk/util/socket.c"
#undef recv
#undef select
#undef write

static int tests_run;
static int tests_failed;
static int select_calls;

/*
 * These test doubles make the regression deterministic: the helpers enter
 * their EAGAIN paths with a descriptor at FD_SETSIZE without requiring the
 * process to allocate that many descriptors.
 */
ssize_t socket_helpers_recv(int socket, void *buffer, size_t length, int flags)
{
    (void)socket;
    (void)buffer;
    (void)length;
    (void)flags;
    errno = EAGAIN;
    return -1;
}

ssize_t socket_helpers_write(int fd, const void *buffer, size_t length)
{
    (void)fd;
    (void)buffer;
    (void)length;
    errno = EAGAIN;
    return -1;
}

int socket_helpers_select(int nfds, fd_set *readfds, fd_set *writefds,
                          fd_set *exceptfds, struct timeval *timeout)
{
    (void)nfds;
    (void)readfds;
    (void)writefds;
    (void)exceptfds;
    (void)timeout;
    select_calls++;
    errno = EINVAL;
    return -1;
}

static void report(int passed, const char *name)
{
    tests_run++;
    printf("%s %d - %s\n", passed ? "ok" : "not ok", tests_run, name);

    if (!passed) {
        tests_failed++;
    }
}

static int test_high_fd_read_fails_before_select(void)
{
    char byte;
    ssize_t received;
    select_calls = 0;
    errno = 0;
    received = readt(FD_SETSIZE, &byte, 1, 0, 1);
    return received == -1 && errno == EINVAL && select_calls == 0;
}

static int test_high_fd_write_fails_before_select(void)
{
    char byte = 'x';
    ssize_t written;
    select_calls = 0;
    errno = 0;
    written = writet(FD_SETSIZE, &byte, 1, 0, 1);
    return written == -1 && errno == EINVAL && select_calls == 0;
}

int main(void)
{
    printf("TAP version 13\n");
    report(test_high_fd_read_fails_before_select(),
           "readt rejects fd >= FD_SETSIZE before select");
    report(test_high_fd_write_fails_before_select(),
           "writet rejects fd >= FD_SETSIZE before select");
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
