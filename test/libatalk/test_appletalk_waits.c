/*
 * Tests for the socket handling of the AppleTalk library's waits and NBP calls
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
 * @brief Tests for the socket handling of the AppleTalk library's waits and
 *        NBP calls
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <netatalk/at.h>
#include <netatalk/ddp.h>

#include <atalk/atp.h>
#include <atalk/compat.h>
#include <atalk/ddp.h>
#include <atalk/nbp.h>
#include <atalk/netddp.h>
#include <atalk/util.h>

struct atpbuf;
struct atpbuf *appletalk_waits_alloc_buf(void);
int appletalk_waits_free_buf(struct atpbuf *buf);
int appletalk_waits_open(struct sockaddr_at *addr, struct sockaddr_at *bridge);
int appletalk_waits_close(int s);
ssize_t appletalk_waits_sendto(int s, const void *msg, size_t len, int flags,
                               const struct sockaddr *to, socklen_t tolen);
int appletalk_waits_select(int nfds, fd_set *readfds, fd_set *writefds,
                           fd_set *exceptfds, struct timeval *timeout);
int appletalk_waits_recv_atp(ATP ah, struct sockaddr_at *fromaddr,
                             uint8_t *func, uint16_t tid, char *rbuf, int wait);
ssize_t appletalk_waits_recvfrom(int s, void *buf, size_t len, int flags,
                                 struct sockaddr *from, socklen_t *fromlen);

/*
 * Rename the calls only after the headers the included files use have been
 * parsed.  Applying these definitions as compiler arguments breaks fortified
 * libc headers that need to declare and wrap the original functions.
 */
#define atp_alloc_buf appletalk_waits_alloc_buf
#define atp_free_buf appletalk_waits_free_buf
#define atp_recv_atp appletalk_waits_recv_atp
#define netddp_close appletalk_waits_close
#define netddp_open appletalk_waits_open
#define netddp_recvfrom appletalk_waits_recvfrom
#define netddp_sendto appletalk_waits_sendto
#define select appletalk_waits_select
#include "../../libatalk/atp/atp_rsel.c"
#include "../../libatalk/nbp/nbp_lkup.c"
#include "../../libatalk/nbp/nbp_rgstr.c"
#include "../../libatalk/nbp/nbp_unrgstr.c"
#undef atp_alloc_buf
#undef atp_free_buf
#undef atp_recv_atp
#undef netddp_close
#undef netddp_open
#undef netddp_recvfrom
#undef netddp_sendto
#undef select

static int tests_run;
static int tests_failed;
/*! calls the doubles took */
static int allocs;
static int opens;
static int closes;
static int receives;
static int sends;
static int selects;
/*! the socket the open double gives */
static int open_fd = FD_SETSIZE;
/*! the first select() call, from 1, that the double reports ready, or 0 */
static int ready_from;
/*! the DDP type and NBP operation of the reply the receive double gives */
static uint8_t reply_type;
static uint8_t reply_op;

/*!
 * @brief atp_alloc_buf() of the waits: count the call
 *
 * @returns a buffer of the test's own
 */
struct atpbuf *appletalk_waits_alloc_buf(void)
{
    static struct atpbuf buf;
    allocs++;
    return &buf;
}

/*!
 * @brief atp_free_buf() of the waits: a buffer of the test's own needs nothing
 *
 * @param[in] buf  unused
 * @returns 0
 */
int appletalk_waits_free_buf(struct atpbuf *buf _U_)
{
    return 0;
}

/*!
 * @brief netddp_open() of the waits: open_fd, a socket at FD_SETSIZE as a
 *        process that holds that many descriptors gets, unless a case asks
 *        for a low one
 *
 * errno is left EINTR, as a successful call may leave it, so that no check
 * after the open passes on an errno set before it.
 *
 * @param[in] addr    unused
 * @param[in] bridge  unused
 * @returns open_fd
 */
int appletalk_waits_open(struct sockaddr_at *addr _U_,
                         struct sockaddr_at *bridge _U_)
{
    opens++;
    errno = EINTR;
    return open_fd;
}

/*!
 * @brief netddp_close() of the waits: count the closes of the socket the open
 *        double gives
 *
 * @param[in] s  socket to close
 * @returns 0
 */
int appletalk_waits_close(int s)
{
    closes += s == open_fd;
    return 0;
}

/*!
 * @brief netddp_sendto() of the waits: count the call, as if all went out
 *
 * @param[in] s      unused
 * @param[in] msg    unused
 * @param[in] len    bytes to send
 * @param[in] flags  unused
 * @param[in] to     unused
 * @param[in] tolen  unused
 * @returns len
 */
ssize_t appletalk_waits_sendto(int s _U_, const void *msg _U_, size_t len,
                               int flags _U_, const struct sockaddr *to _U_,
                               socklen_t tolen _U_)
{
    sends++;
    return (ssize_t)len;
}

/*!
 * @brief select() of the waits: count the call, and report the socket ready
 *        from call ready_from on, failing the calls before it
 *
 * @param[in] nfds       unused
 * @param[in] readfds    unused
 * @param[in] writefds   unused
 * @param[in] exceptfds  unused
 * @param[in] timeout    unused
 * @returns 1 from call ready_from on, else -1 with errno EINVAL
 */
int appletalk_waits_select(int nfds _U_, fd_set *readfds _U_,
                           fd_set *writefds _U_, fd_set *exceptfds _U_,
                           struct timeval *timeout _U_)
{
    selects++;

    if (ready_from > 0 && selects >= ready_from) {
        return 1;
    }

    errno = EINVAL;
    return -1;
}

/*!
 * @brief netddp_recvfrom() of the waits: a reply of reply_type that carries
 *        the NBP operation reply_op
 *
 * @param[in]  s        unused
 * @param[out] buf      receives the reply
 * @param[in]  len      unused, as the reply's three bytes fit every buffer
 * @param[in]  flags    unused
 * @param[in]  from     unused
 * @param[in]  fromlen  unused
 * @returns the reply's length
 */
ssize_t appletalk_waits_recvfrom(int s _U_, void *buf, size_t len _U_,
                                 int flags _U_, struct sockaddr *from _U_,
                                 socklen_t *fromlen _U_)
{
    struct nbphdr nh;
    uint8_t *data = buf;
    memset(&nh, 0, sizeof(nh));
    nh.nh_op = reply_op;
    data[0] = reply_type;
    memcpy(data + 1, &nh, SZ_NBPHDR);
    return 1 + SZ_NBPHDR;
}

/*!
 * @brief atp_recv_atp() of the waits: count the call, with no packet to give
 *
 * @param[in] ah        unused
 * @param[in] fromaddr  unused
 * @param[in] func      unused
 * @param[in] tid       unused
 * @param[in] rbuf      unused
 * @param[in] wait      unused
 * @returns -1 with errno EAGAIN
 */
int appletalk_waits_recv_atp(ATP ah _U_, struct sockaddr_at *fromaddr _U_,
                             uint8_t *func _U_, uint16_t tid _U_,
                             char *rbuf _U_, int wait _U_)
{
    receives++;
    errno = EAGAIN;
    return -1;
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

/*! @brief Zero the call counts of the doubles and errno */
static void calls_reset(void)
{
    allocs = opens = closes = receives = sends = selects = 0;
    ready_from = 0;
    errno = 0;
}

/*!
 * @brief atp_rsel() refuses a socket at FD_SETSIZE while a request waits for
 *        its response, before it takes a buffer or selects
 *
 * @returns 1, else 0
 */
static int test_atp_rsel_refuses_high_socket(void)
{
    struct atp_handle ah;
    struct sockaddr_at faddr;
    memset(&ah, 0, sizeof(ah));
    memset(&faddr, 0, sizeof(faddr));
    /* a request sent just now, with its one response to come */
    ah.atph_socket = FD_SETSIZE;
    ah.atph_reqtries = 1;
    ah.atph_reqto = 2;
    ah.atph_rrespcount = 1;
    ah.atph_rbitmap = 1;

    if (gettimeofday(&ah.atph_reqtv, NULL) != 0) {
        return 0;
    }

    calls_reset();
    return atp_rsel(&ah, &faddr, ATP_TRESP) == -1 && errno == EINVAL
           && allocs == 0 && selects == 0;
}

/*!
 * @brief atp_rsel() reads a request on a socket at FD_SETSIZE, while a request
 *        of its own waits for its response: a read that waits for no
 *        response selects nothing, so the check lets it through
 *
 * @returns 1, else 0
 */
static int test_atp_rsel_reads_request_on_high_socket(void)
{
    struct atp_handle ah;
    struct sockaddr_at faddr;
    memset(&ah, 0, sizeof(ah));
    memset(&faddr, 0, sizeof(faddr));
    /* a request of its own sent just now, with its one response to come */
    ah.atph_socket = FD_SETSIZE;
    ah.atph_reqtries = 1;
    ah.atph_reqto = 2;
    ah.atph_rrespcount = 1;
    ah.atph_rbitmap = 1;

    if (gettimeofday(&ah.atph_reqtv, NULL) != 0) {
        return 0;
    }

    calls_reset();
    return atp_rsel(&ah, &faddr, ATP_TREQ) == -1 && errno == EAGAIN
           && allocs == 1 && receives == 1 && selects == 0;
}

/*!
 * @brief nbp_lookup() refuses a socket at FD_SETSIZE before it sends its
 *        request, and closes it
 *
 * @returns 1, else 0
 */
static int test_nbp_lookup_refuses_high_socket(void)
{
    struct nbpnve nn;
    calls_reset();
    return nbp_lookup("a", "b", "*", &nn, 1, NULL) == -1 && errno == EINVAL
           && opens == 1 && closes == 1 && sends == 0 && selects == 0;
}

/*!
 * @brief nbp_rgstr() refuses a socket at FD_SETSIZE before it sends its
 *        request, and closes it, after its lookup does the same
 *
 * @returns 1, else 0
 */
static int test_nbp_rgstr_refuses_high_socket(void)
{
    struct sockaddr_at sat;
    memset(&sat, 0, sizeof(sat));
    calls_reset();
    return nbp_rgstr(&sat, "a", "b", "*") == -1 && errno == EINVAL
           && opens == 2 && closes == 2 && sends == 0 && selects == 0;
}

/*!
 * @brief nbp_unrgstr() sends its request on a socket at FD_SETSIZE, so the
 *        name goes, refuses the wait for the reply, and closes the socket
 *
 * @returns 1, else 0
 */
static int test_nbp_unrgstr_sends_without_wait(void)
{
    calls_reset();
    return nbp_unrgstr("a", "b", "*", NULL) == -1 && errno == EINVAL
           && opens == 1 && closes == 1 && sends == 1 && selects == 0;
}

/*!
 * @brief Fill name with one byte more than an NBP name part holds
 *
 * @param[out] name  buffer of NBPSTRLEN + 2 bytes
 */
static void long_name(char *name)
{
    memset(name, 'a', NBPSTRLEN + 1);
    name[NBPSTRLEN + 1] = '\0';
}

/*!
 * @brief nbp_lookup() refuses a name whose object, type or zone is too long
 *        with ENAMETOOLONG before it sends, and closes its socket
 *
 * @returns 1, else 0
 */
static int test_nbp_lookup_long_name_closes(void)
{
    char part[NBPSTRLEN + 2];
    const char *names[3][3] = {
        { part, "b", "*" }, { "a", part, "*" }, { "a", "b", part }
    };
    struct nbpnve nn;
    int ok = 1;
    long_name(part);
    /* a socket below FD_SETSIZE, which the descriptor check lets through */
    open_fd = 3;

    for (int i = 0; i < 3 && ok; i++) {
        const char **n = names[i];
        calls_reset();
        ok = nbp_lookup(n[0], n[1], n[2], &nn, 1, NULL) == -1
             && errno == ENAMETOOLONG && opens == 1 && closes == 1
             && sends == 0 && selects == 0;
    }

    open_fd = FD_SETSIZE;
    return ok;
}

/*!
 * @brief nbp_rgstr() refuses a name whose object, type or zone is too long
 *        with ENAMETOOLONG before it sends, and closes every socket it
 *        opened, its lookup's and its own
 *
 * @returns 1, else 0
 */
static int test_nbp_rgstr_long_name_closes(void)
{
    char part[NBPSTRLEN + 2];
    const char *names[3][3] = {
        { part, "b", "*" }, { "a", part, "*" }, { "a", "b", part }
    };
    struct sockaddr_at sat;
    int ok = 1;
    long_name(part);
    memset(&sat, 0, sizeof(sat));
    open_fd = 3;

    for (int i = 0; i < 3 && ok; i++) {
        const char **n = names[i];
        calls_reset();
        ok = nbp_rgstr(&sat, n[0], n[1], n[2]) == -1
             && errno == ENAMETOOLONG && opens == 2 && closes == 2
             && sends == 0 && selects == 0;
    }

    open_fd = FD_SETSIZE;
    return ok;
}

/*!
 * @brief nbp_unrgstr() refuses a name whose object, type or zone is too long
 *        with ENAMETOOLONG before it sends, and closes its socket
 *
 * @returns 1, else 0
 */
static int test_nbp_unrgstr_long_name_closes(void)
{
    char part[NBPSTRLEN + 2];
    const char *names[3][3] = {
        { part, "b", "*" }, { "a", part, "*" }, { "a", "b", part }
    };
    int ok = 1;
    long_name(part);
    open_fd = 3;

    for (int i = 0; i < 3 && ok; i++) {
        const char **n = names[i];
        calls_reset();
        ok = nbp_unrgstr(n[0], n[1], n[2], NULL) == -1
             && errno == ENAMETOOLONG && opens == 1 && closes == 1
             && sends == 0 && selects == 0;
    }

    open_fd = FD_SETSIZE;
    return ok;
}

/*!
 * @brief Register a name on a socket below FD_SETSIZE whose answer from
 *        atalkd is a reply of type with the NBP operation op; the lookup
 *        before it finds nothing, as its select() fails
 *
 * @returns what nbp_rgstr() returns, or -2 unless it sent its request and
 *          closed both sockets it opened
 */
static int rgstr_reply(uint8_t type, uint8_t op)
{
    struct sockaddr_at sat;
    int ret;
    memset(&sat, 0, sizeof(sat));
    calls_reset();
    /* the lookup's select() is the first, nbp_rgstr()'s own the second */
    ready_from = 2;
    reply_type = type;
    reply_op = op;
    open_fd = 3;
    ret = nbp_rgstr(&sat, "a", "b", "*");
    open_fd = FD_SETSIZE;
    return opens == 2 && closes == 2 && sends == 2 ? ret : -2;
}

/*!
 * @brief Remove a name on a socket below FD_SETSIZE whose answer from atalkd
 *        is a reply of type with the NBP operation op
 *
 * @returns what nbp_unrgstr() returns, or -2 unless it sent its request and
 *          closed its socket
 */
static int unrgstr_reply(uint8_t type, uint8_t op)
{
    int ret;
    calls_reset();
    ready_from = 1;
    reply_type = type;
    reply_op = op;
    open_fd = 3;
    ret = nbp_unrgstr("a", "b", "*", NULL);
    open_fd = FD_SETSIZE;
    return opens == 1 && closes == 1 && sends == 1 ? ret : -2;
}

/*!
 * @brief nbp_rgstr() succeeds on NBPOP_OK, and says why another answer fails
 *        it: EADDRNOTAVAIL for atalkd's refusal, EPROTO for a reply that is
 *        not NBP
 *
 * @returns 1, else 0
 */
static int test_nbp_rgstr_reply_errors(void)
{
    if (rgstr_reply(DDPTYPE_NBP, NBPOP_OK) != 0) {
        return 0;
    }

    if (rgstr_reply(DDPTYPE_NBP, NBPOP_ERROR) != -1 || errno != EADDRNOTAVAIL) {
        return 0;
    }

    return rgstr_reply(DDPTYPE_ATP, NBPOP_OK) == -1 && errno == EPROTO;
}

/*!
 * @brief nbp_unrgstr() succeeds on NBPOP_OK, and says why another answer
 *        fails it: EADDRNOTAVAIL for atalkd's refusal, EPROTO for a reply
 *        that is not NBP
 *
 * @returns 1, else 0
 */
static int test_nbp_unrgstr_reply_errors(void)
{
    if (unrgstr_reply(DDPTYPE_NBP, NBPOP_OK) != 0) {
        return 0;
    }

    if (unrgstr_reply(DDPTYPE_NBP, NBPOP_ERROR) != -1
            || errno != EADDRNOTAVAIL) {
        return 0;
    }

    return unrgstr_reply(DDPTYPE_ATP, NBPOP_OK) == -1 && errno == EPROTO;
}

/*! @brief Run the tests and print their TAP report */
int main(void)
{
    printf("TAP version 13\n");
    report(test_atp_rsel_refuses_high_socket(),
           "atp_rsel refuses a socket at FD_SETSIZE before its select");
    report(test_atp_rsel_reads_request_on_high_socket(),
           "atp_rsel reads a request on a socket at FD_SETSIZE");
    report(test_nbp_lookup_refuses_high_socket(),
           "nbp_lookup refuses a socket at FD_SETSIZE before it sends");
    report(test_nbp_rgstr_refuses_high_socket(),
           "nbp_rgstr refuses a socket at FD_SETSIZE before it sends");
    report(test_nbp_unrgstr_sends_without_wait(),
           "nbp_unrgstr sends on a socket at FD_SETSIZE but does not wait");
    report(test_nbp_lookup_long_name_closes(),
           "nbp_lookup closes its socket for a name part that is too long");
    report(test_nbp_rgstr_long_name_closes(),
           "nbp_rgstr closes its sockets for a name part that is too long");
    report(test_nbp_unrgstr_long_name_closes(),
           "nbp_unrgstr closes its socket for a name part that is too long");
    report(test_nbp_rgstr_reply_errors(),
           "nbp_rgstr says why atalkd's answer fails it");
    report(test_nbp_unrgstr_reply_errors(),
           "nbp_unrgstr says why atalkd's answer fails it");
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
