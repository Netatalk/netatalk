/*
 * Regression tests for the readt(), writet() and recv_fd() timeout helpers
 * and the poll set helpers
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
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
 * @brief Tests for the waits of readt(), writet() and recv_fd() and for the
 *        poll sets
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <atalk/util.h>

#include "high_fd.h"

ssize_t socket_helpers_recv(int socket, void *buffer, size_t length, int flags);
ssize_t socket_helpers_write(int fd, const void *buffer, size_t length);
int socket_helpers_poll(struct pollfd *fds, nfds_t nfds, int timeout);
long long socket_helpers_clock(void);

/*
 * Rename libc calls only after system headers have been parsed.  Applying
 * these definitions as compiler arguments breaks fortified libc headers that
 * need to declare and wrap the original functions.
 */
#define monotonic_ms socket_helpers_clock
#define poll socket_helpers_poll
#define recv socket_helpers_recv
#define write socket_helpers_write
#include "../../libatalk/util/socket.c"
#undef monotonic_ms
#undef poll
#undef recv
#undef write

static int tests_run;
static int tests_failed;
/*! calls the doubles fail with EAGAIN before they pass calls through */
static int eagain_calls;
/*! calls the poll() double fails with EINTR before it times out, or -1 while
 *  it passes calls through */
static int poll_eintr = -1;
/*! calls the poll() double took while poll_eintr was set */
static int poll_calls;
/*! timeout of the last of those calls */
static int poll_timeout;
/*! milliseconds the clock double moves per reading, 0 for the real clock */
static long long clock_step;
/*! the clock double's last reading */
static long long clock_now;

/*!
 * @brief recv() of the helpers: EAGAIN while eagain_calls lasts, as from a
 *        socket with no data, then the real recv()
 */
ssize_t socket_helpers_recv(int socket, void *buffer, size_t length, int flags)
{
    if (eagain_calls > 0) {
        eagain_calls--;
        errno = EAGAIN;
        return -1;
    }

    return recv(socket, buffer, length, flags);
}

/*!
 * @brief write() of the helpers: EAGAIN while eagain_calls lasts, as from a
 *        socket with no room, then the real write()
 */
ssize_t socket_helpers_write(int fd, const void *buffer, size_t length)
{
    if (eagain_calls > 0) {
        eagain_calls--;
        errno = EAGAIN;
        return -1;
    }

    return write(fd, buffer, length);
}

/*!
 * @brief poll() of the helpers: while poll_eintr is set, EINTR for that many
 *        calls and then a timeout, each call's timeout recorded; else the real
 *        poll()
 */
int socket_helpers_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    if (poll_eintr < 0) {
        return poll(fds, nfds, timeout);
    }

    poll_calls++;
    poll_timeout = timeout;

    if (poll_eintr > 0) {
        poll_eintr--;
        errno = EINTR;
        return -1;
    }

    return 0;
}

/*!
 * @brief monotonic_ms() of the helpers: while clock_step is set, a clock that
 *        moves that many milliseconds per reading; else the real clock
 */
long long socket_helpers_clock(void)
{
    if (clock_step == 0) {
        return monotonic_ms();
    }

    clock_now += clock_step;
    return clock_now;
}

/*! @brief Print one TAP result line */
static void report(int passed, const char *name)
{
    tests_run++;
    printf("%s %d - %s\n", passed ? "ok" : "not ok", tests_run, name);

    if (!passed) {
        tests_failed++;
    }
}

/*! @brief Print the TAP line of a case that cannot run here */
static void skip(const char *name, const char *why)
{
    tests_run++;
    printf("ok %d - %s # SKIP %s\n", tests_run, name, why);
}

/*!
 * @brief readt() on a descriptor above FD_SETSIZE waits for its data and
 *        reads it
 *
 * @returns 1, 0, or -1 when no descriptor that high can be had
 */
static int test_read_above_fd_setsize(void)
{
    struct rlimit saved;
    char byte = 0;
    int pair[2];
    int ok;

    if ((ok = high_fd_pair(pair, &saved)) != 1) {
        return ok == 0 ? -1 : 0;
    }

    /* the first recv() says EAGAIN with the byte queued, so readt() waits */
    eagain_calls = 1;
    ok = write(pair[0], "x", 1) == 1 && readt(pair[1], &byte, 1, 0, 1) == 1
         && byte == 'x';
    close(pair[0]);
    close(pair[1]);
    setrlimit(RLIMIT_NOFILE, &saved);
    return ok;
}

/*!
 * @brief writet() on a descriptor above FD_SETSIZE waits for room and writes
 *
 * @returns 1, 0, or -1 when no descriptor that high can be had
 */
static int test_write_above_fd_setsize(void)
{
    struct rlimit saved;
    char byte = 'x';
    char got = 0;
    int pair[2];
    int ok;

    if ((ok = high_fd_pair(pair, &saved)) != 1) {
        return ok == 0 ? -1 : 0;
    }

    /* the first write() says EAGAIN with room to write, so writet() waits */
    eagain_calls = 1;
    ok = writet(pair[1], &byte, 1, 0, 1) == 1 && read(pair[0], &got, 1) == 1
         && got == 'x';
    close(pair[0]);
    close(pair[1]);
    setrlimit(RLIMIT_NOFILE, &saved);
    return ok;
}

/*!
 * @brief writet() waits for room to write, not for data to read
 *
 * @returns 1 when the write goes through, else 0
 */
static int test_write_waits_for_room(void)
{
    char byte = 'x';
    char got = 0;
    int pair[2];
    int ok;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return 0;
    }

    /* the socket has room and nothing to read */
    eagain_calls = 1;
    ok = writet(pair[0], &byte, 1, 0, 1) == 1 && read(pair[1], &got, 1) == 1
         && got == 'x';
    close(pair[0]);
    close(pair[1]);
    return ok;
}

/*!
 * @brief readt() gives up after its timeout of a second when no data comes
 *
 * @returns 1, else 0
 */
static int test_read_times_out(void)
{
    char byte;
    long long start;
    int pair[2];
    int ok;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return 0;
    }

    eagain_calls = 0;
    start = monotonic_ms();
    ok = readt(pair[0], &byte, 1, 1, 1) == -1 && errno == EAGAIN
         && monotonic_ms() - start >= 900;
    close(pair[0]);
    close(pair[1]);
    return ok;
}

/*!
 * @brief writet() gives up after its timeout of a second when no room comes
 *
 * @returns 1, else 0
 */
static int test_write_times_out(void)
{
    char block[4096] = {0};
    long long start;
    int pair[2];
    int full;
    int ok;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return 0;
    }

    if (setnonblock(pair[0], 1) != 0) {
        close(pair[0]);
        close(pair[1]);
        return 0;
    }

    while (write(pair[0], block, sizeof(block)) > 0) {
        /* fill the socket until it takes no more */
    }

    full = errno == EAGAIN;
    eagain_calls = 0;
    start = monotonic_ms();
    ok = full && writet(pair[0], block, 1, 0, 1) == -1
         && monotonic_ms() - start >= 900;
    close(pair[0]);
    close(pair[1]);
    return ok;
}

/*!
 * @brief A signal leaves readt()'s wait the rest of its timeout, read from the
 *        clock once
 *
 * The clock moves 600 ms per reading: the wait starts at 600 with its end at
 * 1600, and the reading after the signal gives 1200, so 400 ms remain. A
 * second reading there would give 1800, past the end.
 *
 * @returns 1, else 0
 */
static int test_wait_after_signal(void)
{
    char byte;
    int pair[2];
    int ok;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return 0;
    }

    eagain_calls = 1;
    poll_eintr = 1;
    poll_calls = 0;
    clock_now = 0;
    clock_step = 600;
    ok = readt(pair[0], &byte, 1, 0, 1) == -1 && errno == EAGAIN
         && poll_calls == 2 && poll_timeout == 400;
    poll_eintr = -1;
    clock_step = 0;
    close(pair[0]);
    close(pair[1]);
    return ok;
}

/*!
 * @brief A signal past readt()'s deadline leaves its wait no time
 *
 * The clock moves 1500 ms per reading: the wait starts at 1500 with its end
 * at 2500, and the reading after the signal gives 3000, past the end, so the
 * second poll gets 0 ms.
 *
 * @returns 1, else 0
 */
static int test_wait_past_deadline(void)
{
    char byte;
    int pair[2];
    int ok;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return 0;
    }

    eagain_calls = 1;
    poll_eintr = 1;
    poll_calls = 0;
    clock_now = 0;
    clock_step = 1500;
    ok = readt(pair[0], &byte, 1, 0, 1) == -1 && errno == EAGAIN
         && poll_calls == 2 && poll_timeout == 0;
    poll_eintr = -1;
    clock_step = 0;
    close(pair[0]);
    close(pair[1]);
    return ok;
}

/*!
 * @brief readt()'s wait takes INT_MAX milliseconds for a timeout of INT_MAX
 *        seconds, which in milliseconds is past them
 *
 * @returns 1, else 0
 */
static int test_wait_clamps_timeout(void)
{
    char byte;
    int pair[2];
    int ok;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return 0;
    }

    /* a byte to read, so that a wait the poll double does not take returns
     * at once */
    if (write(pair[1], "x", 1) != 1) {
        close(pair[0]);
        close(pair[1]);
        return 0;
    }

    eagain_calls = 1;
    poll_eintr = 0;
    poll_calls = 0;
    ok = readt(pair[0], &byte, 1, 0, INT_MAX) == -1 && errno == EAGAIN
         && poll_calls == 1 && poll_timeout == INT_MAX;
    poll_eintr = -1;
    close(pair[0]);
    close(pair[1]);
    return ok;
}

/*!
 * @brief A signal leaves recv_fd()'s wait the rest of its 2 seconds
 *
 * The clock moves 600 ms per reading: the wait starts at 600 with its end at
 * 2600, and the reading after the signal gives 1200, so 1400 ms remain.
 *
 * @returns 1, else 0
 */
static int test_recv_fd_wait_after_signal(void)
{
    int pair[2];
    int ok;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return 0;
    }

    poll_eintr = 1;
    poll_calls = 0;
    clock_now = 0;
    clock_step = 600;
    ok = recv_fd(pair[1], 1) == -1 && poll_calls == 2 && poll_timeout == 1400;
    poll_eintr = -1;
    clock_step = 0;
    close(pair[0]);
    close(pair[1]);
    return ok;
}

/*!
 * @brief recv_fd() on a non-blocking socket waits for a descriptor that comes
 *        after its wait began, and takes it
 *
 * @returns 1, else 0
 */
static int test_recv_fd_waits(void)
{
    struct timespec delay = { .tv_nsec = 200 * 1000 * 1000 };
    int pair[2];
    int status;
    int fd;
    pid_t pid;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return 0;
    }

    /* children must not inherit buffered TAP output */
    if (setnonblock(pair[1], 1) != 0 || fflush(stdout) == EOF
            || (pid = fork()) == -1) {
        close(pair[0]);
        close(pair[1]);
        return 0;
    }

    if (pid == 0) {
        nanosleep(&delay, NULL);
        _exit(send_fd(pair[0], pair[0]) == 0 ? 0 : 1);
    }

    fd = recv_fd(pair[1], 1);
    close(pair[0]);
    close(pair[1]);

    if (fd != -1) {
        close(fd);
    }

    return waitpid(pid, &status, 0) == pid && WIFEXITED(status)
           && WEXITSTATUS(status) == 0 && fd != -1;
}

/*!
 * @brief A renewed poll set keeps its entries, in order, with room for the
 *        descriptors asked for and no more
 *
 * @returns 1 when the renewed set checks out, else 0
 */
static int test_asev_renew_keeps_entries(void)
{
    int owner;
    struct asev *sev = asev_init(2);
    struct asev *renewed;
    int ok;

    if (sev == NULL || !asev_add_fd(sev, 7, IPC_FD, &owner, 0)
            || !asev_add_fd(sev, 9, STATS_FD, NULL, 0)
            || (renewed = asev_renew(sev, 1)) == NULL) {
        asev_free(sev);
        return 0;
    }

    ok = renewed->used == 2 && renewed->fdset[0].fd == 7
         && renewed->fdset[0].events == POLLIN
         && renewed->data[0].fdtype == IPC_FD
         && renewed->data[0].private == &owner
         && renewed->fdset[1].fd == 9 && renewed->data[1].fdtype == STATS_FD
         && asev_add_fd(renewed, 11, LISTEN_FD, NULL, 0)
         && !asev_add_fd(renewed, 13, LISTEN_FD, NULL, 0);
    asev_free(renewed);
    return ok;
}

/*! @brief Run the tests and print their TAP report */
int main(void)
{
    const char *read_high =
        "readt on a descriptor above FD_SETSIZE waits for data";
    const char *write_high =
        "writet on a descriptor above FD_SETSIZE waits for room";
    const char *why = "the descriptor limit allows none above FD_SETSIZE";
    int result;
    printf("TAP version 13\n");
    result = test_read_above_fd_setsize();

    if (result < 0) {
        skip(read_high, why);
    } else {
        report(result, read_high);
    }

    result = test_write_above_fd_setsize();

    if (result < 0) {
        skip(write_high, why);
    } else {
        report(result, write_high);
    }

    report(test_write_waits_for_room(),
           "writet waits for room to write, not for data to read");
    report(test_read_times_out(), "readt gives up after its timeout");
    report(test_write_times_out(), "writet gives up after its timeout");
    report(test_wait_after_signal(),
           "a signal leaves readt's wait the rest of its timeout");
    report(test_wait_past_deadline(),
           "a signal past readt's deadline leaves its wait no time");
    report(test_wait_clamps_timeout(),
           "readt's wait takes INT_MAX ms for a timeout past them");
    report(test_recv_fd_wait_after_signal(),
           "a signal leaves recv_fd's wait the rest of its 2 seconds");
    report(test_recv_fd_waits(),
           "recv_fd waits for a descriptor that comes after its wait began");
    report(test_asev_renew_keeps_entries(),
           "asev_renew keeps the entries with room for more");
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
