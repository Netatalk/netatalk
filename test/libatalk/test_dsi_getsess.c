/*
 * Tests for dsi_getsession(): session starts that fail, and the status child
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
 * @brief Tests for dsi_getsession(): session starts that fail, and the
 *        status child
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <atalk/dsi.h>
#include <atalk/server_child.h>
#include <atalk/unix.h>

#include "high_fd.h"

#define FILL_MAX 64

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

/*! @brief Print the TAP line of a case that cannot run here */
static void skip(const char *name, const char *why)
{
    tests_run++;
    printf("ok %d - %s # SKIP %s\n", tests_run, name, why);
}

/*! @brief proto_open() stand-in whose accept() fails */
static pid_t open_accept_fails(DSI *dsi)
{
    dsi->socket = -1;
    errno = ECONNABORTED;
    return -1;
}

/*! @brief proto_open() stand-in whose accept() succeeds and fork() fails */
static pid_t open_fork_fails(DSI *dsi)
{
    dsi->socket = open("/dev/null", O_RDONLY);
    errno = EAGAIN;
    return -1;
}

/*!
 * @brief A failed session start leaves the descriptor count unchanged
 *
 * @param[in] proto_open  stand-in for dsi_tcp_open()
 *
 * @returns 1 when nothing leaked
 */
static int test_no_leak(pid_t (*proto_open)(DSI *))
{
    DSI dsi;
    server_child_t *children = server_child_alloc(10);
    afp_child_t *child = NULL;
    int before, after, rc;

    if (children == NULL) {
        return 0;
    }

    memset(&dsi, 0, sizeof(dsi));
    dsi.socket = -1;
    dsi.serversock = -1;
    dsi.proto_open = proto_open;
    before = count_open_fds(NULL);
    rc = dsi_getsession(&dsi, children, 30, &child);
    after = count_open_fds(NULL);
    server_child_free(children);
    return rc != 0 && child == NULL && after == before;
}

/*!
 * @brief With no descriptor free, a waiting client is refused as busy
 *
 * Takes every descriptor below a lowered limit while a client waits in the
 * listen backlog, then asks for a session: the spare descriptor lets the
 * master accept and refuse the client, which drains the backlog.  Only that
 * path fails with EMFILE.  Valgrind applies RLIMIT_NOFILE to open() but not
 * to socketpair(), so under it the start reaches the accept() stub instead.
 *
 * @returns 1 when the client got the busy reply and the backlog is empty,
 *          0 when it did not, -1 when the session start did not starve
 */
static int test_refuses_when_starved(void)
{
    struct sockaddr_in addr;
    socklen_t alen = sizeof(addr);
    struct rlimit saved;
    struct rlimit low;
    int fills[FILL_MAX];
    int nfill = 0;
    DSI dsi;
    afp_child_t *child = NULL;
    uint8_t reply[DSI_BLOCKSIZ];
    uint32_t code;
    struct pollfd pfd;
    int got = 0;
    int starved;
    int drained;
    int rc;
    int csock = -1;
    server_child_t *children = NULL;
    int lsock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    /* what cannot be set up here is the host's doing, not a defect */
    if (lsock < 0 || bind(lsock, (struct sockaddr *)&addr, sizeof(addr)) != 0
            || listen(lsock, 4) != 0
            || getsockname(lsock, (struct sockaddr *)&addr, &alen) != 0) {
        goto cannot;
    }

    csock = socket(AF_INET, SOCK_STREAM, 0);

    if (csock < 0
            || connect(csock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        goto cannot;
    }

    children = server_child_alloc(10);

    if (children == NULL || getrlimit(RLIMIT_NOFILE, &saved) != 0) {
        goto cannot;
    }

    memset(&dsi, 0, sizeof(dsi));
    dsi.socket = -1;
    dsi.serversock = lsock;
    dsi.proto_open = open_accept_fails;
    dsi_reserve_spare_fd();
    low = saved;
    low.rlim_cur = (rlim_t)count_open_fds(NULL) + 4;

    if (low.rlim_cur >= saved.rlim_cur || setrlimit(RLIMIT_NOFILE, &low) != 0) {
        goto cannot;
    }

    while (nfill < FILL_MAX) {
        int fd = open("/dev/null", O_RDONLY);

        if (fd < 0) {
            break;
        }

        fills[nfill++] = fd;
    }

    rc = dsi_getsession(&dsi, children, 30, &child);
    starved = rc != 0 && errno == EMFILE;

    while (nfill > 0) {
        close(fills[--nfill]);
    }

    if (setrlimit(RLIMIT_NOFILE, &saved) != 0) {
        return 0;
    }

    pfd.fd = csock;
    pfd.events = POLLIN;
    pfd.revents = 0;

    if (starved && poll(&pfd, 1, 2000) == 1
            && recv(csock, reply, sizeof(reply), MSG_WAITALL)
            == (ssize_t)sizeof(reply)) {
        memcpy(&code, reply + 4, sizeof(code));
        got = (ntohl(code) == DSIERR_SERVBUSY);
    }

    pfd.fd = lsock;
    pfd.events = POLLIN;
    pfd.revents = 0;
    drained = (poll(&pfd, 1, 0) == 0);
    close(csock);
    close(lsock);
    server_child_free(children);

    if (!starved) {
        return -1;
    }

    return child == NULL && got && drained;
cannot:

    if (csock >= 0) {
        close(csock);
    }

    if (lsock >= 0) {
        close(lsock);
    }

    if (children != NULL) {
        server_child_free(children);
    }

    return -1;
}

/*! the session end of the status test's socket pair, which open_status()
 *  hands over as an accept() would */
static int status_sock = -1;
/*! the client end, which the session process of an accept() does not hold */
static int status_client = -1;

/*! @brief proto_open() stand-in that accepts a DSIGetStatus and forks */
static pid_t open_status(DSI *dsi)
{
    pid_t pid;
    dsi->socket = status_sock;
    dsi->header.dsi_command = DSIFUNC_STAT;

    /* children must not inherit buffered TAP output */
    if (fflush(stdout) == EOF) {
        return -1;
    }

    if ((pid = fork()) == 0) {
        close(status_client);
    }

    return pid;
}

/*! @brief proto_close() stand-in: close the session socket */
static void close_status(DSI *dsi)
{
    close(dsi->socket);
    dsi->socket = -1;
}

/*!
 * @brief The status child of a socket above FD_SETSIZE keeps the socket after
 *        its reply until the client closes its end
 *
 * The child waits for the close for OpenTransport 1.1.2, up to 120 seconds.
 *
 * @returns 1, 0, or -1 when no descriptor that high can be had
 */
static int test_status_child_waits(void)
{
    struct timespec half = { .tv_nsec = 500 * 1000 * 1000 };
    struct timespec tick = { .tv_nsec = 10 * 1000 * 1000 };
    static AFPObj obj;
    static uint8_t commands[DSI_BLOCKSIZ];
    uint8_t reply[DSI_BLOCKSIZ];
    server_child_t *children;
    afp_child_t *child = NULL;
    struct rlimit saved;
    DSI *dsi;
    pid_t pid = -1;
    pid_t w;
    int waited = 0;
    int ended = 0;
    int pair[2];
    int status;
    int rc;

    if ((rc = high_fd_pair(pair, &saved)) != 1) {
        return rc == 0 ? -1 : 0;
    }

    children = server_child_alloc(10);
    dsi = calloc(1, sizeof(*dsi));
    status_sock = pair[1];
    status_client = pair[0];

    if (children == NULL || dsi == NULL) {
        close(pair[1]);
    } else {
        dsi->socket = -1;
        dsi->serversock = -1;
        dsi->AFPobj = &obj;
        dsi->commands = commands;
        dsi->proto_open = open_status;
        dsi->proto_close = close_status;

        /* the master's side closes its copy of the socket */
        if (dsi_getsession(dsi, children, 30, &child) == 0 && child != NULL) {
            pid = child->afpch_pid;
        }
    }

    if (pid != -1 && recv(pair[0], reply, sizeof(reply), MSG_WAITALL)
            == (ssize_t)sizeof(reply) && nanosleep(&half, NULL) == 0) {
        /* half a second after its reply the child still waits */
        w = waitpid(pid, &status, WNOHANG);
        waited = w == 0;
        pid = w == 0 ? pid : -1;
    }

    close(pair[0]);

    /* the client's close ends the wait */
    for (int i = 0; i < 500 && pid != -1; i++) {
        if ((w = waitpid(pid, &status, WNOHANG)) == 0) {
            nanosleep(&tick, NULL);
            continue;
        }

        ended = w == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0;
        pid = -1;
    }

    if (pid != -1) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
    }

    setrlimit(RLIMIT_NOFILE, &saved);

    if (children != NULL) {
        server_child_free(children);
    }

    free(dsi);
    return waited && ended;
}

/*! @brief Run the tests and print their TAP report */
int main(void)
{
    const char *starved =
        "with no descriptor free, a waiting client gets a busy reply";
    const char *status_wait =
        "a status child above FD_SETSIZE waits for the client to close";
    int result;
    printf("TAP version 13\n");
    report(test_no_leak(open_accept_fails),
           "a failed accept() leaks no descriptor");
    report(test_no_leak(open_fork_fails),
           "a failed fork() after accept() leaks no descriptor");
    result = test_refuses_when_starved();

    if (result < 0) {
        skip(starved, "a starved session start cannot be set up here");
    } else {
        report(result, starved);
    }

    result = test_status_child_waits();

    if (result < 0) {
        skip(status_wait, "the descriptor limit allows none above FD_SETSIZE");
    } else {
        report(result, status_wait);
    }

    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
