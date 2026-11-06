/*
 * Regression tests for self-pipe draining and EOF on socket-backed pipes.
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
#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atalk/dsi.h>
#include <atalk/util.h>

static int tests_run;
static int tests_failed;

/*! @brief Print one TAP result line */
static void report(int passed, const char *name)
{
    tests_run++;
    printf("%s %d - %s\n", passed ? "ok" : "not ok", tests_run, name);

    if (!passed) {
        tests_failed++;
    }
}

/*!
 * @brief Drain an EOF through the self-pipe descriptor, with optional queued
 *        bytes, using either a native pipe or a socket pair
 *
 * Calls the drain directly so the test checks EOF even on platforms whose
 * poll() also reports POLLHUP for sockets. NetBSD's PIPE_SOCKETPAIR kernels
 * report socket EOF as POLLIN alone.
 *
 * @returns 1 when EOF is reported as EPIPE, else 0
 */
static int test_eof(int socket_backed, int queued)
{
    char bytes[256] = {0};
    int pair[2];
    int ok = 0;

    if ((socket_backed ? socketpair(AF_UNIX, SOCK_STREAM, 0, pair)
            : pipe(pair)) != 0) {
        return 0;
    }

    if (setnonblock(pair[0], 1) == 0
            && (!queued || write(pair[1], bytes, sizeof(bytes))
                == (ssize_t)sizeof(bytes))
            && dup2(pair[0], atalk_sigpipe_readfd()) != -1) {
        close(pair[1]);
        pair[1] = -1;
        errno = 0;
        ok = atalk_sigpipe_drain() == -1 && errno == EPIPE;
    }

    close(pair[0]);

    if (pair[1] != -1) {
        close(pair[1]);
    }

    return ok;
}

/*! @brief Stand-in for the session's fatal error cleanup callback */
static void session_exit(int status)
{
    _exit(status == EXITERR_SYS ? 0 : 1);
}

/*! @brief An idle DSI wait terminates on signal-pipe EOF or an invalid fd */
static int test_wait_failure(int socket_backed, int invalid)
{
    int status;
    pid_t child;

    /* Children must not inherit buffered TAP output. */
    if (fflush(stdout) == EOF) {
        return 0;
    }

    child = fork();

    if (child < 0) {
        return 0;
    }

    if (child == 0) {
        AFPObj obj = {0};
        DSI dsi = {0};
        int transport[2];
        int pair[2];
        alarm(3);
        obj.exit = session_exit;
        obj.options.timeout = 1;
        obj.options.tickleval = 1;
        dsi.AFPobj = &obj;

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, transport) != 0) {
            _exit(1);
        }

        dsi.socket = transport[0];

        if (invalid) {
            close(atalk_sigpipe_readfd());
        } else {
            if ((socket_backed ? socketpair(AF_UNIX, SOCK_STREAM, 0, pair)
                    : pipe(pair)) != 0
                    || setnonblock(pair[0], 1) != 0
                    || dup2(pair[0], atalk_sigpipe_readfd()) == -1) {
                _exit(1);
            }

            close(pair[0]);
            close(pair[1]);
        }

        dsi_wait_input(&dsi);
        _exit(1);
    }

    while (waitpid(child, &status, 0) == -1) {
        if (errno != EINTR) {
            return 0;
        }
    }

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

int main(void)
{
    printf("TAP version 13\n");

    if (atalk_sigpipe_init() != 0) {
        printf("Bail out! cannot create signal pipe\n");
        return 1;
    }

    report(atalk_sigpipe_drain() == 0,
           "an empty live signal pipe drains successfully");

    for (int i = 0; i < 256; i++) {
        atalk_sigpipe_notify();
    }

    report(atalk_sigpipe_drain() == 0,
           "queued notifications drain without treating EAGAIN as failure");
    report(test_eof(0, 0), "native pipe EOF is reported");
    report(test_eof(1, 0), "socket-backed pipe EOF is reported");
    report(test_eof(1, 1), "socket-backed pipe EOF after queued bytes is reported");
    report(test_wait_failure(0, 0),
           "DSI wait terminates the session on native pipe EOF");
    report(test_wait_failure(1, 0),
           "DSI wait terminates the session on socket-backed pipe EOF");
    report(test_wait_failure(0, 1),
           "DSI wait terminates the session on an invalid signal fd");
    close(atalk_sigpipe_readfd());
    errno = 0;
    report(atalk_sigpipe_drain() == -1 && errno == EBADF,
           "a signal pipe read error is reported with its errno");
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
