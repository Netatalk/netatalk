/*
 * Tests for the wait of a DSI reply that the socket cannot take yet
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
 * @brief Tests for the wait of a DSI reply that the socket cannot take yet
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <atalk/dsi.h>
#include <atalk/util.h>

#include "high_fd.h"

/* the session's send buffer, far smaller than the reply */
#define SEND_BUF (16 * 1024)
#define REPLY_LEN (1024 * 1024)
/* the exit status of a session process that cannot set the session up */
#define SETUP_FAILED 2

static int tests_run;
static int tests_failed;
static uint8_t reply[REPLY_LEN];
static char readbuf[8192];
static AFPObj obj;
static DSI dsi;
/*! set as afpd's SIGURG handler sets the flag stop_wait points to */
static volatile sig_atomic_t handed_over;
/*! bytes the client sends during the reply, which the session must buffer */
static size_t input_len;

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
 * @brief Read the monotonic clock
 *
 * @returns milliseconds since an arbitrary start
 */
static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/*!
 * @brief proto_close() of the session: close its socket
 *
 * @param[in,out] d  DSI session
 */
static void session_close(DSI *d)
{
    close(d->socket);
    d->socket = -1;
}

/*!
 * @brief SIGUSR1 handler that records a reconnect's hand-over, as afpd's
 *        SIGURG handler does
 *
 * @param[in] sig  unused
 */
static void hand_over(int sig _U_)
{
    handed_over = 1;
    atalk_sigpipe_notify();
}

/*!
 * @brief Start a session process that writes the reply on sock as afpd does,
 *        on a non-blocking socket, with a bound of seconds when it is above 0
 *
 * The process exits with 0 after a whole write that left the input_len bytes
 * the client sent in its read-ahead buffer, with ETIMEDOUT or ECONNABORTED
 * after a write that failed with it and left the session disconnected, with
 * SETUP_FAILED when it cannot set the session up, and with 1 otherwise.
 * SIGALRM ends it after 10 seconds.
 *
 * @param[in] sock     session end of a socket pair
 * @param[in] seconds  the session's bound, or 0 for a session without options
 * @returns the pid, or -1
 */
static pid_t session_start(int sock, int seconds)
{
    int sndbuf = SEND_BUF;
    ssize_t n;
    pid_t pid;

    /* Children must not inherit buffered TAP output. */
    if (fflush(stdout) == EOF) {
        return -1;
    }

    if ((pid = fork()) != 0) {
        return pid;
    }

    alarm(10);
    dsi.socket = sock;
    dsi.splice_pipe[0] = dsi.splice_pipe[1] = -1;
    dsi.buffer = dsi.start = dsi.eof = readbuf;
    dsi.end = readbuf + sizeof(readbuf);
    dsi.proto_close = session_close;
    dsi.stop_wait = &handed_over;

    if (seconds > 0) {
        obj.options.timeout = 1;
        obj.options.tickleval = seconds;
        dsi.AFPobj = &obj;
    }

    if (atalk_sigpipe_init() != 0 || setnonblock(sock, 1) != 0
            || setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &sndbuf,
                          sizeof(sndbuf)) != 0) {
        _exit(SETUP_FAILED);
    }

    n = dsi_stream_write(&dsi, reply, REPLY_LEN, 0);

    if (n == REPLY_LEN) {
        _exit((size_t)(dsi.eof - dsi.start) == input_len ? 0 : 1);
    }

    _exit(n == -1 && (errno == ETIMEDOUT || errno == ECONNABORTED)
          && (dsi.flags & DSI_DISCONNECTED) && dsi.socket == -1 ? errno : 1);
}

/*!
 * @brief Reap the session process
 *
 * @param[in] pid  session process
 * @returns its exit status, or -1 when a signal ended it
 */
static int session_end(pid_t pid)
{
    int status;

    while (waitpid(pid, &status, 0) == -1) {
        if (errno != EINTR) {
            return -1;
        }
    }

    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/*!
 * @brief Wait up to five seconds for the session to send to client, which it
 *        does just before its wait for room starts
 *
 * @param[in] client  client end
 * @returns the time of the first data on the monotonic clock in ms, or -1
 */
static long long session_sends(int client)
{
    struct pollfd pfd = { .fd = client, .events = POLLIN };
    return poll(&pfd, 1, 5000) == 1 ? now_ms() : -1;
}

/*!
 * @brief A reply on a socket above FD_SETSIZE goes out whole as the client
 *        reads it
 *
 * @returns 1, 0, or -1 when no descriptor that high can be had
 */
static int test_reply_above_fd_setsize(void)
{
    struct timespec delay = { .tv_nsec = 200 * 1000 * 1000 };
    struct rlimit saved;
    char buf[65536];
    size_t got = 0;
    ssize_t n;
    pid_t pid;
    int pair[2];
    int rc;

    if ((rc = high_fd_pair(pair, &saved)) != 1) {
        return rc == 0 ? -1 : 0;
    }

    pid = session_start(pair[1], 0);
    close(pair[1]);
    /* the session fills the socket and waits before the client reads */
    nanosleep(&delay, NULL);

    while ((n = read(pair[0], buf, sizeof(buf))) > 0) {
        got += (size_t)n;
    }

    close(pair[0]);
    setrlimit(RLIMIT_NOFILE, &saved);
    return pid != -1 && session_end(pid) == 0 && got == REPLY_LEN;
}

/*!
 * @brief A reply the client stops reading ends at the session timeout of a
 *        second, with the session disconnected
 *
 * @returns 1, else 0
 */
static int test_reply_times_out(void)
{
    long long start = -1;
    long long elapsed;
    pid_t pid;
    int pair[2];
    int status = -1;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return 0;
    }

    pid = session_start(pair[1], 1);
    close(pair[1]);

    if (pid != -1) {
        start = session_sends(pair[0]);
        status = session_end(pid);
    }

    elapsed = now_ms() - start;
    close(pair[0]);
    /* the bound of a second, with two more for the scheduler */
    return status == ETIMEDOUT && start != -1 && elapsed >= 900
           && elapsed <= 3000;
}

/*!
 * @brief Run a session on a reply its client does not read, with a bound of a
 *        minute, which outlasts the test, and 200 ms in hand the session over
 *        as afpd's SIGURG does, or shut down the client's sending side
 *
 * @param[in] hand_over  non-zero for the hand-over, 0 for the shutdown
 * @returns the session's exit status, or -1
 */
static int stalled_reply_end(int hand_over)
{
    struct timespec delay = { .tv_nsec = 200 * 1000 * 1000 };
    pid_t pid;
    int pair[2];
    int status = -1;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return -1;
    }

    pid = session_start(pair[1], 60);
    close(pair[1]);

    if (pid != -1) {
        nanosleep(&delay, NULL);

        if (hand_over) {
            kill(pid, SIGUSR1);
        } else {
            shutdown(pair[0], SHUT_WR);
        }

        status = session_end(pid);
    }

    close(pair[0]);
    return status;
}

/*!
 * @brief A reconnect's hand-over ends the wait of a reply the client stopped
 *        reading, with the session disconnected
 *
 * @returns 1, else 0
 */
static int test_hand_over_ends_reply(void)
{
    return stalled_reply_end(1) == ECONNABORTED;
}

/*!
 * @brief Run a session with a bound of two seconds on sock, whose client
 *        takes part of the reply one second into the session's wait and then
 *        nothing
 *
 * @param[in] client  client end, closed on return
 * @param[in] sock    session end, closed on return
 * @param[in] all     non-zero for a client that takes all the socket holds,
 *                    up to half the reply, 0 for one that takes up to 20000
 *                    bytes in one read
 * @returns 1 when the client took data and the reply ended at the session
 *          timeout a bound after the take, else 0 with what it saw as a TAP
 *          comment
 */
static int slow_client(int client, int sock, int all)
{
    struct timespec delay = { .tv_sec = 1 };
    char buf[20000];
    long long taken = -1;
    long long after = -1;
    size_t took = 0;
    ssize_t n;
    pid_t pid;
    int status = -1;
    pid = session_start(sock, 2);
    close(sock);

    if (pid != -1) {
        if (session_sends(client) != -1) {
            nanosleep(&delay, NULL);

            /* half the reply at most, so that a session refilling the
             * socket during the drain cannot finish the reply */
            if (!all || setnonblock(client, 1) == 0) {
                do {
                    n = read(client, buf, sizeof(buf));
                    took += n > 0 ? (size_t)n : 0;
                } while (all && n > 0 && took < REPLY_LEN / 2);
            }

            taken = now_ms();
        }

        status = session_end(pid);
    }

    if (taken != -1) {
        after = now_ms() - taken;
    }

    close(client);

    /* the take starts the bound of two seconds again; two seconds more
     * cover the scheduler */
    if (took == 0 || status != ETIMEDOUT || after < 1800 || after > 4000) {
        printf("# the client took %zu bytes; the session ended with %d %lld "
               "ms after the take\n", took, status, after);
        return 0;
    }

    return 1;
}

/*!
 * @brief A client that takes part of a reply within each bound keeps it going
 *        past the first bound
 *
 * The client takes 20000 bytes one second into a bound of two seconds, then
 * stops. On Linux that frees one of the two buffers the socket pair queues
 * for a send buffer of SEND_BUF, too little for poll() to report room.
 * illumos runs a socket pair over STREAMS, whose flow control one read may
 * not lift, so there, and on Solaris, the client takes all the pair holds.
 *
 * @returns 1, else 0
 */
static int test_slow_client_keeps_reply(void)
{
    int pair[2];

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return 0;
    }

#ifdef __sun
    return slow_client(pair[0], pair[1], 1);
#else
    return slow_client(pair[0], pair[1], 0);
#endif
}

#ifndef __sun
/*!
 * @brief Connect a loopback TCP pair whose client end receives into a buffer
 *        of rcvbuf bytes, set before it connects so its window stays small
 *
 * @param[out] pair    client end, then session end
 * @param[in]  rcvbuf  receive buffer of the client end
 * @returns 0, or -1 with neither end open
 */
static int tcp_pair(int pair[2], int rcvbuf)
{
    struct sockaddr_in addr = { .sin_family = AF_INET };
    socklen_t alen = sizeof(addr);
    int lsock;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    pair[0] = pair[1] = -1;

    if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return -1;
    }

    if (bind(lsock, (struct sockaddr *)&addr, sizeof(addr)) == 0
            && listen(lsock, 1) == 0
            && getsockname(lsock, (struct sockaddr *)&addr, &alen) == 0
            && (pair[0] = socket(AF_INET, SOCK_STREAM, 0)) != -1
            && setsockopt(pair[0], SOL_SOCKET, SO_RCVBUF, &rcvbuf,
                          sizeof(rcvbuf)) == 0
            && connect(pair[0], (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        pair[1] = accept(lsock, NULL, NULL);
    }

    close(lsock);

    if (pair[1] == -1) {
        if (pair[0] != -1) {
            close(pair[0]);
        }

        return -1;
    }

    return 0;
}

/*!
 * @brief A TCP client that takes part of a reply within each bound keeps it
 *        going past the first bound
 *
 * The client's receive buffer of 4096 bytes holds little of the reply, so
 * what it takes one second in lets the session send too little to free the
 * third of its send buffer at which Linux reports room; only the fall of the
 * socket's TIOCOUTQ count starts the bound again. Elsewhere poll() reporting
 * room starts it. macOS and DragonFly BSD give the client end more than 4096
 * bytes, and one read of 20000 of them frees room that the session does not
 * see within the bound, so outside Linux the client takes all its end holds.
 *
 * @returns 1, 0, or -1 without a loopback connection
 */
static int test_slow_tcp_client_keeps_reply(void)
{
    int pair[2];

    if (tcp_pair(pair, 4096) != 0) {
        return -1;
    }

#ifdef __linux__
    return slow_client(pair[0], pair[1], 0);
#else
    return slow_client(pair[0], pair[1], 1);
#endif
}
#endif

/*!
 * @brief A client that shuts down its sending side ends the wait of a reply it
 *        does not read: the write fails without a timeout or a hand-over
 *
 * @returns 1, else 0
 */
static int test_hangup_ends_reply(void)
{
    return stalled_reply_end(0) == 1;
}

/*!
 * @brief Input the client sends while a reply waits for room lands in the
 *        read-ahead buffer, and the reply still goes out whole
 *
 * The client sends once the reply's first bytes reach it, and reads the reply
 * only once the session has taken the input: the session's end of the pair,
 * which the test holds too, then has nothing to read.
 *
 * @returns 1, else 0
 */
static int test_input_during_reply(void)
{
    struct timespec tick = { .tv_nsec = 10 * 1000 * 1000 };
    struct pollfd pfd;
    char buf[65536];
    size_t got = 0;
    ssize_t sent = -1;
    ssize_t n;
    pid_t pid;
    int pair[2];
    int taken = 0;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) != 0) {
        return 0;
    }

    memset(buf, 'i', sizeof(buf));
    input_len = 100;
    pid = session_start(pair[1], 60);
    input_len = 0;

    if (pid != -1 && session_sends(pair[0]) != -1) {
        sent = write(pair[0], buf, 100);
        pfd.fd = pair[1];
        pfd.events = POLLIN;

        /* five seconds for the session's wait to read the input */
        for (int i = 0; i < 500 && !taken; i++) {
            taken = poll(&pfd, 1, 0) == 0;

            if (!taken) {
                nanosleep(&tick, NULL);
            }
        }
    }

    close(pair[1]);

    while ((n = read(pair[0], buf, sizeof(buf))) > 0) {
        got += (size_t)n;
    }

    close(pair[0]);
    return pid != -1 && sent == 100 && taken && session_end(pid) == 0
           && got == REPLY_LEN;
}

/*! @brief Run the tests and print their TAP report */
int main(void)
{
    const char *high =
        "a reply above FD_SETSIZE goes out whole as the client reads it";
    const char *tcp =
        "a TCP client that takes part of a reply in each bound keeps it going";
    struct sigaction sa;
    int result;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = hand_over;
    sigemptyset(&sa.sa_mask);
    printf("TAP version 13\n");

    if (sigaction(SIGUSR1, &sa, NULL) != 0) {
        printf("Bail out! cannot handle SIGUSR1\n");
        return 1;
    }

    result = test_reply_above_fd_setsize();

    if (result < 0) {
        skip(high, "the descriptor limit allows none above FD_SETSIZE");
    } else {
        report(result, high);
    }

    report(test_reply_times_out(),
           "a reply the client stops reading ends at the session timeout");
    report(test_slow_client_keeps_reply(),
           "a client that takes part of a reply in each bound keeps it going");
#ifndef __sun
    result = test_slow_tcp_client_keeps_reply();

    if (result < 0) {
        skip(tcp, "no loopback TCP connection");
    } else {
        report(result, tcp);
    }

#else
    skip(tcp, "a part taken may not lift the flow control of illumos");
#endif
    report(test_hand_over_ends_reply(),
           "a reconnect's hand-over ends the wait of a stalled reply");
    report(test_hangup_ends_reply(),
           "a client that shuts down its sending side ends the reply");
    report(test_input_during_reply(),
           "input sent during a reply's wait lands in the read-ahead buffer");
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
