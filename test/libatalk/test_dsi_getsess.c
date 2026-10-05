/*
 * Tests for dsi_getsession() when a session cannot start
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atalk/dsi.h>
#include <atalk/server_child.h>
#include <atalk/unix.h>

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

int main(void)
{
    const char *starved =
        "with no descriptor free, a waiting client gets a busy reply";
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

    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
