/*
  Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>
  Copyright (c) 2026 Andy Lemin (andylemin)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/dsi.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/queue.h>
#include <atalk/unicode.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#ifdef WITH_SPOTLIGHT
#include <talloc.h>

#include <atalk/dalloc.h>
#include <atalk/spotlight.h>

#include "etc/spotlight/spotlight_private.h"
#endif

#include "afp_config.h"
#include "afpfunc_helpers.h"
#include "auth.h"
#include "desktop.h"
#include "dircache.h"
#include "directory.h"
#include "faultinject.h"
#include "file.h"
#include "filedir.h"
#include "fork.h"
#include "hash.h"
#include "peer_lock.h"
#include "subtests.h"
#include "subtests_cnid.h"
#include "subtests_conf.h"
#include "subtests_enumerate.h"
#include "subtests_lock.h"
#include "subtests_offcnt.h"
#include "subtests_pfd.h"
#include "subtests_recvfile.h"
#include "subtests_request.h"
#include "subtests_sendfile.h"
#include "subtests_veto.h"
#include "subtests_volpolls.h"
#ifndef NO_DDP
#include "subtests_atp.h"
#endif /* NO_DDP */
#include "switch.h"
#include "test.h"
#include "test_capabilities.h"
#include "volume.h"

unsigned char nologin = 0;
static AFPObj obj, aspobj;
/* args[2] is filled with an absolute path to test.conf at startup: opening a
 * second volume triggers a load_afp_conf_vols() config reload, and by then afpd has
 * fchdir()'d into a volume, so a relative "test.conf" would no longer resolve. */
static char  confpath[MAXPATHLEN + 1];
static char *args[] = {"test", "-F", confpath};
int test_output_tap = 0;
int test_case_num = 0;
FILE *test_report_stream = NULL;

static int utest_decompose_reserves_terminator(void)
{
    ucs2_t input[] = {0x01d5}; /* U with diaeresis and macron */
    ucs2_t exact_fit[] = {0xa5a5, 0xa5a5, 0xa5a5};
    ucs2_t sufficient[4] = {0, 0, 0, 0};
    ucs2_t surrogate_input[] = {0xd800, 0xdc00};
    ucs2_t surrogate_exact_fit[] = {0xa5a5, 0xa5a5};
    ucs2_t surrogate_sufficient[3] = {0, 0, 0};
    size_t outlen;
    int saved_errno = errno;
    int result = -1;
    outlen = sizeof(exact_fit);
    errno = 0;

    if (decompose_w(input, sizeof(input), exact_fit, &outlen)
            != (size_t) -1
            || errno != E2BIG
            || exact_fit[0] != 0xa5a5
            || exact_fit[1] != 0xa5a5
            || exact_fit[2] != 0xa5a5) {
        goto done;
    }

    outlen = sizeof(sufficient);

    if (decompose_w(input, sizeof(input), sufficient, &outlen) != 6
            || sufficient[0] != 0x0055
            || sufficient[1] != 0x0308
            || sufficient[2] != 0x0304
            || sufficient[3] != 0) {
        goto done;
    }

    outlen = sizeof(surrogate_exact_fit);
    errno = 0;

    if (decompose_w(surrogate_input, sizeof(surrogate_input),
                    surrogate_exact_fit, &outlen) != (size_t) -1
            || errno != E2BIG
            || surrogate_exact_fit[0] != 0xa5a5
            || surrogate_exact_fit[1] != 0xa5a5) {
        goto done;
    }

    outlen = sizeof(surrogate_sufficient);

    if (decompose_w(surrogate_input, sizeof(surrogate_input),
                    surrogate_sufficient, &outlen) != 4
            || surrogate_sufficient[0] != 0xd800
            || surrogate_sufficient[1] != 0xdc00
            || surrogate_sufficient[2] != 0) {
        goto done;
    }

    result = 0;
done:
    errno = saved_errno;
    return result;
}

struct guarded_adouble {
    struct adouble ad;
    uint8_t guard[AD_DATASZ_MAX];
};

static int utest_fork_range_rejects_wrapped_read(void)
{
    if (fork_range_within(1, (off_t)INT64_MAX, 1024)) {
        return 1;
    }

    if (!fork_range_within(1, 1023, 1024)) {
        return 2;
    }

    if (fork_range_within(1, 1024, 1024)
            || fork_range_within(1024, 1, 1024)
            || fork_range_within(-1, 1, 1024)
            || fork_range_within(0, -1, 1024)) {
        return 3;
    }

    return 0;
}

/*! A reply a session owes a request: its frame's command, id and code, and
 *  the len bytes of data at data it carries, at most 16 */
struct dsp_reply {
    uint8_t command;
    uint16_t reqid;
    int code;
    const void *data;
    uint32_t len;
};

/*!
 * @brief Tell whether the next reply on peer is the one in want
 */
static bool dsp_reply_is(int peer, const struct dsp_reply *want)
{
    uint8_t expect[DSI_BLOCKSIZ];
    uint8_t got[DSI_BLOCKSIZ + 16];
    const size_t len = DSI_BLOCKSIZ + want->len;
    dsi_test_header(expect, want->command, want->reqid, (uint32_t)want->code,
                    want->len);
    expect[0] = DSIFL_REPLY;
    return len <= sizeof(got)
           && readt(peer, got, len, 1, 10) == (ssize_t)len
           && memcmp(got, expect, sizeof(expect)) == 0
           && (want->len == 0
               || memcmp(got + DSI_BLOCKSIZ, want->data, want->len) == 0);
}

/*! @brief Tell whether the session ends the stream on peer with no more data */
static bool dsp_ends(int peer)
{
    char byte;
    return readt(peer, &byte, 1, 1, 10) == 0;
}

/*!
 * @brief Send len bytes of frames to a session process and tell whether it
 *        gives the n replies in want, in order, and then ends the stream
 *
 * @returns 0, -1, or TEST_SKIP without loopback
 */
static int dsp_serve(const uint8_t *frames, size_t len,
                     const struct dsp_reply *want, int n)
{
    int peer = -1;
    pid_t pid = dsi_test_serve(&obj, &peer);
    int ret = -1;
    int i = 0;

    if (pid == -1) {
        return dsi_test_no_loopback(errno) ? TEST_SKIP : -1;
    }

    if (write(peer, frames, len) == (ssize_t)len
            && shutdown(peer, SHUT_WR) == 0) {
        while (i < n && dsp_reply_is(peer, &want[i])) {
            i++;
        }

        if (i == n && dsp_ends(peer)) {
            ret = 0;
        }
    }

    return dsi_test_reap(pid, peer) == 0 ? ret : -1;
}

/*!
 * @brief A write command in a DSICommand frame, which carries no payload, is
 *        refused, with AFPERR_PARAM when a handler serves it and AFPERR_NOOP
 *        when none does, and the requests after it are served
 */
static int utest_dsi_write_command_refused_in_command_frame(void)
{
    static uint8_t frames[3 * (DSI_BLOCKSIZ + 20) + DSI_BLOCKSIZ + 1];
    static const uint8_t command[] = { AFP_WRITE, AFP_WRITE_EXT, AFP_ADDICON };
    static const struct dsp_reply want[] = {
        { DSIFUNC_CMD, 1, AFPERR_PARAM }, { DSIFUNC_CMD, 2, AFPERR_PARAM },
        { DSIFUNC_CMD, 3, AFPERR_NOOP }, { DSIFUNC_CMD, 4, AFPERR_NOOP },
    };
    AFPCmd addicon = postauth_switch[AFP_ADDICON];
    size_t len = 0;
    int ret;

    /* the three with a 20-byte block of zeros, FPAddIcon with no handler
     * in the session's table, then a command no handler serves */
    for (int i = 0; i < 3; i++) {
        len += dsi_test_frame(frames + len, DSIFUNC_CMD, (uint16_t)(i + 1), 0,
                              20, command[i]);
    }

    len += dsi_test_frame(frames + len, DSIFUNC_CMD, 4, 0, 1, 0xff);
    postauth_switch[AFP_ADDICON] = NULL;
    ret = dsp_serve(frames, len, want, 4);
    postauth_switch[AFP_ADDICON] = addicon;
    return ret;
}

/*!
 * @brief A DSIWrite frame whose command takes no payload, or that no handler
 *        serves, is refused and its payload drained, so the next request
 *        parses
 */
static int utest_dsi_write_frame_drained(void)
{
    static uint8_t frames[2 * (DSI_BLOCKSIZ + DSI_WROFF_FPWRITE + 8192)
                          + DSI_BLOCKSIZ + 1];
    static const struct dsp_reply want[] = {
        { DSIFUNC_WRITE, 1, AFPERR_NOOP }, { DSIFUNC_WRITE, 2, AFPERR_PARAM },
        { DSIFUNC_CMD, 3, AFPERR_NOOP },
    };
    size_t len;
    /* a command no handler serves, then FPGetSrvrParms, each with 8 KiB of
     * zeros, past the session's 8 KiB read-ahead, that a parse of the next
     * frame would take for a header */
    len = dsi_test_frame(frames, DSIFUNC_WRITE, 1, DSI_WROFF_FPWRITE,
                         DSI_WROFF_FPWRITE + 8192, 0xff);
    len += dsi_test_frame(frames + len, DSIFUNC_WRITE, 2, DSI_WROFF_FPWRITE,
                          DSI_WROFF_FPWRITE + 8192, AFP_GETSRVPARAM);
    len += dsi_test_frame(frames + len, DSIFUNC_CMD, 3, 0, 1, 0xff);
    return dsp_serve(frames, len, want, 3);
}

/*!
 * @brief A DSI command the session does not serve has its payload read with
 *        its header, so the request after it parses
 */
static int utest_dsi_spurious_command_drains_nothing(void)
{
    static uint8_t frames[DSI_BLOCKSIZ + 6 + DSI_BLOCKSIZ + 1];
    static const struct dsp_reply want[] = {
        { DSIFUNC_CMD, 2, AFPERR_NOOP },
    };
    size_t len;
    /* a DSIOpenSession inside the session with a 6-byte block of zeros,
     * then a command no handler serves */
    len = dsi_test_frame(frames, DSIFUNC_OPEN, 1, 0, 6, 0);
    len += dsi_test_frame(frames + len, DSIFUNC_CMD, 2, 0, 1, 0xff);
    return dsp_serve(frames, len, want, 1);
}

/*!
 * @brief A DSIWrite that a session with an expired password refuses has its
 *        payload drained, so the next request parses
 */
static int utest_dsi_write_refused_pwdexpired_drained(void)
{
    static uint8_t frames[DSI_BLOCKSIZ + DSI_WROFF_FPWRITE + 8192
                          + DSI_BLOCKSIZ + 1];
    static const struct dsp_reply want[] = {
        { DSIFUNC_WRITE, 1, AFPERR_PWDEXPR }, { DSIFUNC_CMD, 2, AFPERR_NOOP },
    };
    AFPCmd handler = postauth_switch[AFP_WRITE];
    size_t len;
    int ret;
    /* FPWrite with 8 KiB of zeros, past the session's read-ahead, refused
     * as after a login with an expired password, then a command no handler
     * serves */
    len = dsi_test_frame(frames, DSIFUNC_WRITE, 1, DSI_WROFF_FPWRITE,
                         DSI_WROFF_FPWRITE + 8192, AFP_WRITE);
    len += dsi_test_frame(frames + len, DSIFUNC_CMD, 2, 0, 1, 0xff);
    postauth_switch[AFP_WRITE] = afp_errpwdexpired;
    ret = dsp_serve(frames, len, want, 2);
    postauth_switch[AFP_WRITE] = handler;
    return ret;
}

/* an FPWrite's payload, and the part of its frame sent before a pause */
#define DSP_PAYLOAD (64 * 1024)
#define DSP_HEAD (DSI_BLOCKSIZ + DSI_WROFF_FPWRITE + 4096)

/*!
 * @brief A session process of uid with ipc_fd as dsi_test_serve_as() starts
 *        it, recvfile on as asked, that times a silent client out after one
 *        tickle of seconds, and in frame an FPWrite of DSP_PAYLOAD zeros to
 *        the data fork of the new file dsp_write
 *
 * @returns the session's pid, or -1 with errno set
 */
static pid_t dsp_write_session(struct vol *vol, uid_t uid, int ipc_fd,
                               int seconds, bool recvfile, uint8_t *frame,
                               int *peer)
{
    struct afp_options options = obj.options;
    uint32_t count = htonl(DSP_PAYLOAD);
    uint16_t refnum;
    pid_t pid;
    int err;

    if (createfile(&obj, vol->v_vid, DIRDID_ROOT, "dsp_write") != AFP_OK
            || openfork(&obj, vol->v_vid, DIRDID_ROOT, "dsp_write",
                        OPENFORK_DATA, OPENACC_RD | OPENACC_WR, &refnum)
            != AFP_OK) {
        errno = EIO;
        return -1;
    }

    dsi_test_frame(frame, DSIFUNC_WRITE, 1, DSI_WROFF_FPWRITE,
                   DSI_WROFF_FPWRITE + DSP_PAYLOAD, AFP_WRITE);
    /* refnum travels raw, as afp_openfork() returns it */
    memcpy(frame + DSI_BLOCKSIZ + 2, &refnum, sizeof(refnum));
    memcpy(frame + DSI_BLOCKSIZ + 8, &count, sizeof(count));
    obj.options.timeout = 1;
    obj.options.tickleval = seconds;

    if (recvfile) {
        obj.options.flags |= OPTION_RECVFILE;
    }

    pid = dsi_test_serve_as(&obj, uid, ipc_fd, 0, peer);
    err = errno;
    obj.options = options;
    /* the session holds its own copy of the fork */
    closefork(&obj, refnum);
    errno = err;
    return pid;
}

/*!
 * @brief Wait up to ten seconds until the session wrote len bytes to
 *        dsp_write, so it has read that much of the payload
 */
static bool dsp_written(const struct vol *vol, off_t len)
{
    struct timespec tick = { .tv_nsec = 10 * 1000 * 1000 };
    char path[MAXPATHLEN + 1];
    struct stat st;
    snprintf(path, sizeof(path), "%s/dsp_write", vol->v_path);

    for (int i = 0; i < 1000; i++) {
        if (stat(path, &st) == 0 && st.st_size >= len) {
            return true;
        }

        nanosleep(&tick, NULL);
    }

    return false;
}

/*!
 * @brief Tell whether the session ends the stream on peer within ten
 *        seconds, whatever data comes first
 */
static bool dsp_drains(int peer)
{
    struct pollfd pfd = { .fd = peer, .events = POLLIN };
    uint8_t buf[256];
    ssize_t n = -1;

    while (poll(&pfd, 1, 10000) == 1
            && (n = recv(peer, buf, sizeof(buf), 0)) > 0) {
        /* what comes before the end is not looked at */
    }

    return n == 0;
}

/* the uid of a logged-in session: the harness's own, or 1 under root */
#define DSP_USER (getuid() != 0 ? getuid() : 1)

/*!
 * @brief An FPWrite whose client falls silent part way through its payload
 *        ends the session without a reply once the silence lasts the
 *        session's timeout of a second, not before
 *
 * @returns 0, -1, or TEST_SKIP without loopback or, for recvfile, without a
 *          socket that splices
 */
static int dsp_stall(struct vol *vol, bool recvfile)
{
    static uint8_t frame[DSI_BLOCKSIZ + DSI_WROFF_FPWRITE + DSP_PAYLOAD];
    struct timespec t0, t1;
    int peer = -1;
    int ret = -1;
    pid_t pid;

    if (recvfile && !test_capability(TEST_CAP_SOCKET_SPLICE, vol)) {
        return TEST_SKIP;
    }

    if ((pid = dsp_write_session(vol, 0, -1, 1, recvfile, frame,
                                 &peer)) == -1) {
        ret = dsi_test_no_loopback(errno) ? TEST_SKIP : -1;
    } else {
        clock_gettime(CLOCK_MONOTONIC, &t0);

        if (write(peer, frame, DSP_HEAD) == DSP_HEAD && dsp_ends(peer)) {
            clock_gettime(CLOCK_MONOTONIC, &t1);
            ret = (t1.tv_sec - t0.tv_sec) * 1000
                  + (t1.tv_nsec - t0.tv_nsec) / 1000000 >= 900 ? 0 : -1;
        }

        if (dsi_test_reap(pid, peer) != 0) {
            ret = -1;
        }
    }

    delete (&obj, vol->v_vid, DIRDID_ROOT, "dsp_write");
    return ret;
}

/*! @brief The session times out a write whose client falls silent */
static int utest_dsi_stalled_write_disconnects(struct vol *vol)
{
    return dsp_stall(vol, false);
}

/*! @brief The session times out a recvfile write whose client falls silent */
static int utest_recvfile_stalled_write_disconnects(struct vol *vol)
{
    return dsp_stall(vol, true);
}

/*!
 * @brief A termination that arrives while an FPWrite's payload is still
 *        arriving lets the write finish and be answered, and the session then
 *        ends as a termination ends it
 *
 * The rest of the payload follows the signal after a pause, so the session
 * waits for it with the termination recorded.
 *
 * @returns 0, -1, or TEST_SKIP without loopback
 */
static int utest_dsi_term_lets_write_finish(struct vol *vol)
{
    static uint8_t frame[DSI_BLOCKSIZ + DSI_WROFF_FPWRITE + DSP_PAYLOAD];
    const size_t rest = sizeof(frame) - DSP_HEAD;
    struct timespec pause = { .tv_nsec = 200 * 1000 * 1000 };
    uint32_t written = htonl(DSP_PAYLOAD);
    const struct dsp_reply want = {
        DSIFUNC_WRITE, 1, AFP_OK, &written, sizeof(written)
    };
    int peer = -1;
    int ret = -1;
    pid_t pid;

    if ((pid = dsp_write_session(vol, 0, -1, 60, false, frame,
                                 &peer)) == -1) {
        ret = dsi_test_no_loopback(errno) ? TEST_SKIP : -1;
    } else {
        /* the signal once the session holds the first part */
        if (write(peer, frame, DSP_HEAD) == DSP_HEAD && dsp_written(vol, 4096)
                && kill(pid, SIGTERM) == 0 && nanosleep(&pause, NULL) == 0
                && write(peer, frame + DSP_HEAD, rest) == (ssize_t)rest
                && dsp_reply_is(peer, &want) && dsp_drains(peer)) {
            ret = 0;
        }

        if (dsi_test_reap(pid, peer) != 0) {
            ret = -1;
        }
    }

    delete (&obj, vol->v_vid, DIRDID_ROOT, "dsp_write");
    return ret;
}

/*!
 * @brief Hand session process pid a new connection as the master afpd hands
 *        over a reconnect: the request ID 7 of the client's
 *        FPDisconnectOldSession and the server end of a new loopback
 *        connection on ipc, then SIGURG; newpeer gets the client end
 *
 * @returns whether the session answered the hand-over on the new connection
 */
static bool dsp_hand_over(pid_t pid, int ipc, int *newpeer)
{
    static const struct dsp_reply handover = { DSIFUNC_CMD, 7, AFP_OK };
    const uint16_t id = htons(7);
    DSI fresh = { .socket = -1 };
    bool sent = false;

    if (dsi_test_connect_tcp(&fresh, newpeer) == 0) {
        sent = write(ipc, &id, sizeof(id)) == (ssize_t)sizeof(id)
               && send_fd(ipc, fresh.socket) == 0;
        /* the session holds its own copy */
        close(fresh.socket);
    }

    return sent && kill(pid, SIGURG) == 0 && dsp_reply_is(*newpeer, &handover);
}

/*!
 * @brief A logged-in session whose client fell silent part way through an
 *        FPWrite takes over the connection a reconnect hands it, answers the
 *        hand-over and serves the requests on it: at once during the stall,
 *        with a tick the stall left unserved when tick, or in the
 *        disconnected state once the stall timed out when timed_out
 *
 * The session times a silent client out at its first tick, so a tick of the
 * old connection's that the new one inherits ends it before its request; a
 * stall times out after a second when timed_out, a minute otherwise.
 *
 * @returns 0, -1, or TEST_SKIP without loopback or, for tick, in a build that
 *          handles no tick
 */
static int dsp_reconnect(struct vol *vol, bool tick, bool timed_out)
{
    static uint8_t frame[DSI_BLOCKSIZ + DSI_WROFF_FPWRITE + DSP_PAYLOAD];
    uint8_t next[DSI_BLOCKSIZ + 1];
    static const struct dsp_reply served = { DSIFUNC_CMD, 8, AFPERR_NOOP };
    int newpeer = -1;
    int peer = -1;
    int ipc[2];
    int ret = -1;
    pid_t pid = -1;
#ifdef DEBUGGING

    /* the build installs no handler for the tickle timer's signal */
    if (tick) {
        return TEST_SKIP;
    }

#endif

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, ipc) != 0) {
        return -1;
    }

    dsi_test_frame(next, DSIFUNC_CMD, 8, 0, 1, 0xff);

    if ((pid = dsp_write_session(vol, DSP_USER, ipc[1], timed_out ? 1 : 60,
                                 false, frame, &peer)) == -1) {
        ret = dsi_test_no_loopback(errno) ? TEST_SKIP : -1;
    } else if (write(peer, frame, DSP_HEAD) == DSP_HEAD
               && dsp_written(vol, 4096)
               && (!tick || kill(pid, SIGALRM) == 0)
               && (!timed_out || dsp_ends(peer))
               && dsp_hand_over(pid, ipc[0], &newpeer)
               && write(newpeer, next, sizeof(next)) == (ssize_t)sizeof(next)
               && dsp_reply_is(newpeer, &served) && dsp_ends(peer)) {
        ret = 0;
    }

    if (pid != -1) {
        kill(pid, SIGTERM);

        if (dsi_test_reap(pid, peer) != 0) {
            ret = -1;
        }
    }

    if (newpeer != -1) {
        close(newpeer);
    }

    close(ipc[0]);
    close(ipc[1]);
    delete (&obj, vol->v_vid, DIRDID_ROOT, "dsp_write");
    return ret;
}

/*! @brief A reconnect takes over a session stalled in a write */
static int utest_dsi_reconnect_takes_over_stalled_write(struct vol *vol)
{
    return dsp_reconnect(vol, false, false);
}

/*! @brief A reconnect drops a tick that a stalled write left unserved */
static int utest_dsi_reconnect_drops_stalled_tick(struct vol *vol)
{
    return dsp_reconnect(vol, true, false);
}

/*! @brief A reconnect takes over a session whose stalled write timed out */
static int utest_dsi_reconnect_after_write_timeout(struct vol *vol)
{
    return dsp_reconnect(vol, false, true);
}

/*!
 * @brief Wait up to ten seconds for the first report of a session process
 *        to the master afpd on ipc, which it sends with its signal handlers
 *        set
 */
static bool dsp_running(int ipc)
{
    struct pollfd pfd = { .fd = ipc, .events = POLLIN };
    return poll(&pfd, 1, 10000) == 1;
}

#ifndef DEBUGGING
/*!
 * @brief Tell whether the session sends a request on peer within ten
 *        seconds, as a tick does for a client that sent nothing since the
 *        last: a tickle, or the attention of a volume that changed
 */
static bool dsp_ticked(int peer)
{
    uint8_t head[DSI_BLOCKSIZ];
    return readt(peer, head, sizeof(head), 1, 10) == (ssize_t)sizeof(head)
           && head[0] == DSIFL_REQUEST;
}
#endif

/*!
 * @brief A logged-in session the timeout disconnected while idle counts the
 *        new connection's silence afresh once a reconnect takes it over, so
 *        the next tick reaches the client rather than disconnecting it again
 *
 * The session times a client out at its second tick without a request, and
 * each SIGALRM the test sends is a tick, as the tickle timer's would be.
 *
 * @returns 0, -1, or TEST_SKIP without loopback or in a build that handles
 *          no tick
 */
static int utest_dsi_reconnect_counts_ticks_afresh(void)
{
#ifdef DEBUGGING
    /* the build installs no handler for the tickle timer's signal */
    return TEST_SKIP;
#else
    struct afp_options options = obj.options;
    int newpeer = -1;
    int peer = -1;
    int ipc[2];
    int ret = -1;
    pid_t pid;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, ipc) != 0) {
        return -1;
    }

    obj.options.timeout = 2;
    pid = dsi_test_serve_as(&obj, DSP_USER, ipc[1], 0, &peer);
    obj.options = options;

    if (pid == -1) {
        ret = dsi_test_no_loopback(errno) ? TEST_SKIP : -1;
    } else if (dsp_running(ipc[0])
               && kill(pid, SIGALRM) == 0 && dsp_ticked(peer)
               && kill(pid, SIGALRM) == 0 && dsp_drains(peer)
               && dsp_hand_over(pid, ipc[0], &newpeer)
               && kill(pid, SIGALRM) == 0 && dsp_ticked(newpeer)) {
        ret = 0;
    }

    if (pid != -1) {
        kill(pid, SIGTERM);

        if (dsi_test_reap(pid, peer) != 0) {
            ret = -1;
        }
    }

    if (newpeer != -1) {
        close(newpeer);
    }

    close(ipc[0]);
    close(ipc[1]);
    return ret;
#endif
}

/*!
 * @brief A logged-in session that a termination reaches while its client
 *        has sent half a request header ends once the request times out
 *        after a second
 *
 * The half header follows the session's first report, so the session waits
 * in its main loop when it arrives, and the signal follows it after a pause.
 * The client end stays open, so the bound alone can end the read.
 *
 * @returns 0, -1, or TEST_SKIP without loopback
 */
static int utest_dsi_term_served_after_stalled_header(void)
{
    static const uint8_t half[DSI_BLOCKSIZ / 2];
    struct timespec tick = { .tv_nsec = 300 * 1000 * 1000 };
    struct afp_options options = obj.options;
    int peer = -1;
    int ipc[2];
    int ret = -1;
    pid_t pid;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, ipc) != 0) {
        return -1;
    }

    obj.options.timeout = 1;
    obj.options.tickleval = 1;
    pid = dsi_test_serve_as(&obj, DSP_USER, ipc[1], 0, &peer);
    obj.options = options;

    if (pid == -1) {
        ret = dsi_test_no_loopback(errno) ? TEST_SKIP : -1;
    } else {
        if (dsp_running(ipc[0])
                && write(peer, half, sizeof(half)) == (ssize_t)sizeof(half)
                && nanosleep(&tick, NULL) == 0) {
            ret = 0;
        }

        kill(pid, SIGTERM);

        if (dsi_test_reap(pid, -1) != 0) {
            ret = -1;
        }

        close(peer);
    }

    close(ipc[0]);
    close(ipc[1]);
    return ret;
}

/*!
 * @brief An FPWrite whose client pauses between the parts of its payload,
 *        each pause shorter than the session's timeout but all of them
 *        longer, completes
 *
 * @returns 0, -1, or TEST_SKIP without loopback
 */
static int utest_dsi_slow_write_completes(struct vol *vol)
{
    static uint8_t frame[DSI_BLOCKSIZ + DSI_WROFF_FPWRITE + DSP_PAYLOAD];
    const size_t part = (sizeof(frame) - DSP_HEAD) / 5;
    struct timespec pause = { .tv_nsec = 500 * 1000 * 1000 };
    uint32_t written = htonl(DSP_PAYLOAD);
    const struct dsp_reply want = {
        DSIFUNC_WRITE, 1, AFP_OK, &written, sizeof(written)
    };
    size_t off = DSP_HEAD;
    int peer = -1;
    int ret = -1;
    pid_t pid;

    /* five half-second pauses against a timeout of one two-second tickle */
    if ((pid = dsp_write_session(vol, 0, -1, 2, false, frame, &peer)) == -1) {
        ret = dsi_test_no_loopback(errno) ? TEST_SKIP : -1;
    } else {
        if (write(peer, frame, DSP_HEAD) == DSP_HEAD) {
            while (off < sizeof(frame)
                    && nanosleep(&pause, NULL) == 0
                    && write(peer, frame + off, part) == (ssize_t)part) {
                off += part;
            }

            if (off == sizeof(frame) && shutdown(peer, SHUT_WR) == 0
                    && dsp_reply_is(peer, &want) && dsp_ends(peer)) {
                ret = 0;
            }
        }

        if (dsi_test_reap(pid, peer) != 0) {
            ret = -1;
        }
    }

    delete (&obj, vol->v_vid, DIRDID_ROOT, "dsp_write");
    return ret;
}

/*!
 * @brief A session whose signal pipe can no longer wake it ends at once
 *
 * A child of the harness sets the pipe up and starts the session, so the
 * sessions of later tests make their own pipe, as afpd's do; its verdict
 * comes down a pipe, as valgrind rewrites the exit status of a child that
 * holds the harness's memory.
 *
 * @returns 0, 1 when the session keeps the connection, -1, or TEST_SKIP
 *          without loopback
 */
static int utest_dsi_dead_signal_pipe_ends_session(void)
{
    int res[2];
    signed char verdict = -1;
    pid_t helper;

    if (pipe(res) != 0) {
        return -1;
    }

    fflush(NULL);

    if ((helper = fork()) == 0) {
        int peer = -1;
        int q[2];
        signed char r = -1;
        pid_t pid;
        close(res[0]);

        /* the session inherits a read end without a writer, which polls
         * POLLHUP */
        if (atalk_sigpipe_init() == 0 && pipe(q) == 0) {
            close(q[1]);
            dup2(q[0], atalk_sigpipe_readfd());
            close(q[0]);

            if ((pid = dsi_test_serve(&obj, &peer)) == -1) {
                r = dsi_test_no_loopback(errno) ? TEST_SKIP : -1;
            } else {
                r = dsp_drains(peer) ? 0 : 1;

                if (dsi_test_reap(pid, peer) != 0) {
                    r = -1;
                }
            }
        }

        _exit(write(res[1], &r, 1) == 1 ? 0 : 1);
    }

    close(res[1]);

    if (helper > 0) {
        if (read(res[0], &verdict, 1) != 1) {
            verdict = -1;
        }

        (void)dsi_test_reap(helper, -1);
    }

    close(res[0]);
    return verdict;
}

/*!
 * @brief A write that comes within the second a session's mark wrote the
 *        root reaches the root once that second has passed, with no
 *        further request
 *
 * @returns 0, 1 while the root keeps its stamp from before the writes, -1,
 *          or TEST_SKIP without loopback or for a root written before both
 *          writes were on disk, which leaves no due write to wait for
 */
static int utest_dsi_due_mark_written(struct vol *vol)
{
    static uint8_t frame[2][DSI_BLOCKSIZ + DSI_WROFF_FPWRITE + DSP_PAYLOAD];
    const uint32_t offset = htonl(DSP_PAYLOAD);
    struct timespec tick = { .tv_nsec = 10 * 1000 * 1000 };
    struct timespec marked = { 0 };
    struct timespec last;
    struct stat st;
    int peer = -1;
    int ret = -1;
    pid_t pid;

    /* the file's create marks a fresh second, and both writes fall in it */
    if (next_second(vol->v_path) != 0) {
        return -1;
    }

    if ((pid = dsp_write_session(vol, 0, -1, 10, false, frame[0],
                                 &peer)) == -1) {
        ret = dsi_test_no_loopback(errno) ? TEST_SKIP : -1;
    } else {
        /* the same write as the next request, at the next offset */
        memcpy(frame[1], frame[0], sizeof(frame[1]));
        dsi_test_header(frame[1], DSIFUNC_WRITE, 2, DSI_WROFF_FPWRITE,
                        DSI_WROFF_FPWRITE + DSP_PAYLOAD);
        memcpy(frame[1] + DSI_BLOCKSIZ + 4, &offset, sizeof(offset));

        if (stat(vol->v_path, &st) == 0) {
            /* the root's stamp before the writes */
            marked = atalk_stat_mtime_timespec(&st);

            if (write(peer, frame, sizeof(frame)) == (ssize_t)sizeof(frame)
                    && dsp_written(vol, 2 * DSP_PAYLOAD)
                    && stat(vol->v_path, &st) == 0) {
                last = atalk_stat_mtime_timespec(&st);
                /* a root written already: a write in a later second, or a
                 * due write made before this check */
                ret = last.tv_sec == marked.tv_sec
                      && last.tv_nsec == marked.tv_nsec ? 1 : TEST_SKIP;
            }
        }

        /* the session writes the root within three seconds */
        for (int i = 0; i < 300 && ret == 1; i++) {
            nanosleep(&tick, NULL);

            if (stat(vol->v_path, &st) == 0) {
                last = atalk_stat_mtime_timespec(&st);
                ret = last.tv_sec != marked.tv_sec
                      || last.tv_nsec != marked.tv_nsec ? 0 : 1;
            }
        }

        if (dsi_test_reap(pid, peer) != 0) {
            ret = -1;
        }
    }

    delete (&obj, vol->v_vid, DIRDID_ROOT, "dsp_write");
    return ret;
}

#ifndef HAVE_EAFD
#define LR_SIDECAR (ADEDOFF_FINDERI_OSX + ADEDLEN_FINDERI + 32 + 100)

/*!
 * @brief FPDelete of a file whose ._ sidecar is the first len bytes of one
 *        that records 100 bytes of resource fork at roff, behind a
 *        FinderInfo of filen bytes at 50
 *
 * @returns 0 when the file and its sidecar are gone, -1 when not
 */
static int lr_delete(struct vol *vol, uint32_t roff, uint32_t filen,
                     size_t len)
{
    const uint32_t entry[] = {
        ADEID_RFORK, roff, 100,
        ADEID_FINDERI, ADEDOFF_FINDERI_OSX, filen,
    };
    uint8_t sidecar[LR_SIDECAR] = { 0 };
    const char *name = "lowrfork";
    char data[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    uint32_t be;
    struct stat st;
    int ret = -1;
    int fd;
    /* magic, version, 16 bytes of filler, two entries */
    be = htonl(AD_MAGIC);
    memcpy(sidecar, &be, sizeof(be));
    be = htonl(AD_VERSION2);
    memcpy(sidecar + 4, &be, sizeof(be));
    sidecar[25] = 2;

    for (size_t i = 0; i < sizeof(entry) / sizeof(entry[0]); i++) {
        be = htonl(entry[i]);
        memcpy(sidecar + AD_HEADER_LEN + 4 * i, &be, sizeof(be));
    }

    snprintf(data, sizeof(data), "%s/%s", vol->v_path, name);
    snprintf(path, sizeof(path), "%s/._%s", vol->v_path, name);

    if (createfile(&obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK) {
        return -1;
    }

    if ((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600)) != -1) {
        if (write(fd, sidecar, len) == (ssize_t)len
                && delete (&obj, vol->v_vid, DIRDID_ROOT, name) == AFP_OK
                && stat(path, &st) == -1 && stat(data, &st) == -1) {
            ret = 0;
        }

        close(fd);
    }

    unlink(path);
    unlink(data);
    return ret;
}
#endif

/*!
 * @brief FPDelete of a file whose ._ sidecar no reader or writer opens as it
 *        is deletes the file and the sidecar: a resource fork recorded below
 *        offset 82, a FinderInfo that needs converting, an empty sidecar
 */
static int utest_delete_file_whatever_sidecar_holds(struct vol *vol)
{
#ifdef HAVE_EAFD
    /* the resource fork is a native stream, which no ._ file holds */
    return TEST_SKIP;
#else

    /* without EAs the volume keeps AppleDouble v2, with no ._ file */
    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    /* the fork at 70, inside the canonical FinderInfo at 50; at 50, inside
     * a 64-byte FinderInfo; at 114, right after it; then no header at all */
    return lr_delete(vol, ADEDOFF_FINDERI_OSX + 20, ADEDLEN_FINDERI,
                     LR_SIDECAR) == 0
           && lr_delete(vol, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI + 32,
                        LR_SIDECAR) == 0
           && lr_delete(vol, ADEDOFF_FINDERI_OSX + ADEDLEN_FINDERI + 32,
                        ADEDLEN_FINDERI + 32, LR_SIDECAR) == 0
           && lr_delete(vol, 0, 0, 0) == 0 ? 0 : -1;
#endif
}

/*!
 * @brief Closing an empty resource fork after a path request in another
 *        folder deletes the fork's own ._ sidecar, and a same-named file's
 *        in that folder keeps its resource fork
 */
static int utest_closefork_empty_sidecar_own_folder(struct vol *vol)
{
#ifdef HAVE_EAFD
    /* the resource fork is a native stream, which no ._ file holds */
    return TEST_SKIP;
#else
    const uint16_t rw = OPENACC_RD | OPENACC_WR;
    char own[MAXPATHLEN + 1];
    char other[MAXPATHLEN + 1];
    struct stat before;
    struct stat st;
    cnid_t xdid = 0;
    cnid_t ydid = 0;
    uint16_t xfork;
    uint16_t yfork;
    int ret = -1;

    /* without EAs the volume keeps AppleDouble v2, with no ._ file */
    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    snprintf(own, sizeof(own), "%s/sidecar_x/._doc", vol->v_path);
    snprintf(other, sizeof(other), "%s/sidecar_y/._doc", vol->v_path);

    /* afp_createdir() leaves curdir in the directory it made */
    if (createdir(&obj, vol->v_vid, DIRDID_ROOT, "sidecar_x") == AFP_OK) {
        xdid = curdir->d_did;
    }

    if (createdir(&obj, vol->v_vid, DIRDID_ROOT, "sidecar_y") == AFP_OK) {
        ydid = curdir->d_did;
    }

    /* doc in y with a resource fork of 100 bytes */
    if (xdid != 0 && ydid != 0
            && createfile(&obj, vol->v_vid, xdid, "doc") == AFP_OK
            && createfile(&obj, vol->v_vid, ydid, "doc") == AFP_OK
            && openfork(&obj, vol->v_vid, ydid, "doc", OPENFORK_RSCS, rw,
                        &yfork) == AFP_OK) {
        ret = setforkparams(&obj, yfork, 100) == AFP_OK ? 0 : -1;
        closefork(&obj, yfork);
    }

    /* doc's empty resource fork in x, open across a request in y */
    if (ret == 0 && stat(other, &before) == 0
            && openfork(&obj, vol->v_vid, xdid, "doc", OPENFORK_RSCS, rw,
                        &xfork) == AFP_OK) {
        ret = getfiledirparms(&obj, vol->v_vid, ydid, "doc") == AFP_OK
              && closefork(&obj, xfork) == AFP_OK ? 0 : -1;

        if (ret == 0) {
            ret = stat(other, &st) == 0 && st.st_size == before.st_size
                  && stat(own, &st) == -1 ? 0 : 1;
        }
    } else {
        ret = -1;
    }

    delete (&obj, vol->v_vid, xdid, "doc");
    delete (&obj, vol->v_vid, ydid, "doc");
    delete (&obj, vol->v_vid, DIRDID_ROOT, "sidecar_x");
    delete (&obj, vol->v_vid, DIRDID_ROOT, "sidecar_y");
    return ret;
#endif
}

/*!
 * @brief An FPWrite over AppleTalk for count bytes whose data the fault
 *        interposer's asp_wrtcont() answers, failing with errnum or, for
 *        errnum 0, handing back the whole reply buffer
 *
 * @returns 0 when the reply is AFPERR_MISC for the failure and AFPERR_PARAM
 *          for the longer data, strict locking took and cleared the count's
 *          range and the fork is still empty, -1 when not, TEST_SKIP without
 *          AppleTalk, a shared libatalk or the preload
 */
static int asp_write_refused(struct vol *vol, uint32_t count, int errnum)
{
#if defined(NO_DDP) || !defined(AFPDTEST_SHARED_LIBATALK)
    return TEST_SKIP;
#else
    const struct ofork *of;
    char rbuf[DSI_BLOCKSIZ];
    uint8_t req[12] = { 0 };
    uint32_t count_be = htonl(count);
    size_t rbuflen = sizeof(rbuf);
    uint16_t refnum = 0;
    AFPObj o = obj;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (createfile(&obj, vol->v_vid, DIRDID_ROOT, "asp_write") != AFP_OK) {
        return -1;
    }

    if (openfork(&obj, vol->v_vid, DIRDID_ROOT, "asp_write", OPENFORK_DATA,
                 OPENACC_RD | OPENACC_WR, &refnum) != AFP_OK) {
        delete (&obj, vol->v_vid, DIRDID_ROOT, "asp_write");
        return -1;
    }

    /* FPWrite: command, flag, fork, offset 0, count */
    req[0] = AFP_WRITE;
    memcpy(req + 2, &refnum, sizeof(refnum));
    memcpy(req + 8, &count_be, sizeof(count_be));
    o.proto = AFPPROTO_ASP;
    o.dsi = NULL;
    o.options.flags |= OPTION_STRICT_LOCKING;
    fault_inject_reset();
    fi.fcntl_watch = 1;
    fi.asp_wrtcont.armed = errnum != 0;
    fi.asp_wrtcont.errnum = errnum;
    fi.asp_wrtcont_fill = errnum == 0;

    if (afp_write(&o, (char *)req, sizeof(req), rbuf, &rbuflen)
            == (errnum != 0 ? AFPERR_MISC : AFPERR_PARAM)
            && fi.setlk_calls == 2 && fi.lock_last_type == F_UNLCK
            && fi.lock_last_start == 0 && fi.lock_last_len == count
            && (of = of_find(refnum)) != NULL
            && ad_size(of->of_ad, ADEID_DFORK) == 0) {
        ret = 0;
    }

    fault_inject_reset();
    closefork(&obj, refnum);
    delete (&obj, vol->v_vid, DIRDID_ROOT, "asp_write");
    return ret;
#endif
}

/*!
 * @brief An FPWrite over AppleTalk whose data fails to arrive clears the
 *        write lock strict locking took for its range
 */
static int utest_write_fork_asp_failure_unlocks(struct vol *vol)
{
    return asp_write_refused(vol, 100, EIO);
}

/*!
 * @brief An FPWrite over AppleTalk whose data is longer than its ReqCount is
 *        refused before any byte is written
 */
static int utest_write_fork_asp_over_count_refused(struct vol *vol)
{
    return asp_write_refused(vol, 1, 0);
}

#define GI_REPLY 4624
#define GI_ICON  8192
#define GI_SMALL 64
#define GI_PART  1024
/* more than the reply buffer holds and less than the icon */
#define GI_OVER  6000
#define GI_REQ   16
#define GI_INFO  10
#define GI_ENTRY 12

/*! @brief Pack an icon file entry's header: tag, file type, icon type, pad,
 *         the size of the bits after it */
static void gi_entry(uint8_t *entry, uint8_t itype, uint16_t size)
{
    uint16_t size_be = htons(size);
    memset(entry, 0, GI_ENTRY);
    memcpy(entry + 4, "ICN#", 4);
    entry[8] = itype;
    memcpy(entry + 10, &size_be, sizeof(size_be));
}

/*! @brief Pack an FPGetIcon request: command, pad, volume, creator, file
 *         type, icon type, pad, size */
static void gi_req(uint8_t *req, const struct vol *vol, const uint8_t *creator,
                   uint8_t itype, uint16_t size)
{
    uint16_t size_be = htons(size);
    memset(req, 0, GI_REQ);
    req[0] = AFP_GETICON;
    memcpy(req + 2, &vol->v_vid, sizeof(vol->v_vid));
    memcpy(req + 4, creator, 4);
    memcpy(req + 8, "ICN#", 4);
    req[12] = itype;
    memcpy(req + 14, &size_be, sizeof(size_be));
}

/*! @brief Pack an FPGetIconInfo request: command, pad, volume, creator,
 *         index */
static void gi_info(uint8_t *info, const struct vol *vol,
                    const uint8_t *creator, uint16_t index)
{
    uint16_t index_be = htons(index);
    memset(info, 0, GI_INFO);
    info[0] = AFP_GTICNINFO;
    memcpy(info + 2, &vol->v_vid, sizeof(vol->v_vid));
    memcpy(info + 4, creator, 4);
    memcpy(info + 8, &index_be, sizeof(index_be));
}

/*!
 * @brief Does FPGetIconInfo on the session o with the request in info return
 *        the entry? The reply length goes in as the dispatcher sets it
 */
static int gi_info_is(AFPObj *o, uint8_t *info, const uint8_t *entry)
{
    char rbuf[GI_REPLY];
    size_t rbuflen = GI_REPLY;
    return afp_geticoninfo(o, (char *)info, GI_INFO, rbuf, &rbuflen)
           == AFP_OK
           && rbuflen == GI_ENTRY && memcmp(rbuf, entry, GI_ENTRY) == 0;
}

/*!
 * @brief Create the creator's icon file, empty, in the volume's desktop
 *        database, where iconopen() looks for it
 *
 * @returns its descriptor, or -1; *path names it
 */
static int gi_icon_file(const struct vol *vol, uint8_t *creator, char **path)
{
    char dir[MAXPATHLEN + 1];
    snprintf(dir, sizeof(dir), "%s/" APPLEDESKTOP, vol->v_dbpath);
    (void)mkdir(dir, 0700);
    *path = dtfile(vol, creator, ".icon");
    snprintf(dir, sizeof(dir), "%s", *path);
    *strrchr(dir, '/') = '\0';
    (void)mkdir(dir, 0700);
    return open(*path, O_RDWR | O_CREAT | O_TRUNC, 0600);
}

/*! @brief Append an icon to the icon file: its entry header, then size bytes
 *         of fill */
static int gi_store(int fd, uint8_t itype, uint16_t size, int fill)
{
    static char bits[GI_ICON];
    uint8_t entry[GI_ENTRY];
    gi_entry(entry, itype, size);
    memset(bits, fill, size);
    return write(fd, entry, sizeof(entry)) == (ssize_t)sizeof(entry)
           && write(fd, bits, size) == (ssize_t)size ? 0 : -1;
}

/*!
 * @brief FPGetIcon whose read is larger than the reply buffer fails and
 *        writes nothing past the buffer, sized as an ASP session's, and
 *        FPGetIconInfo then returns the second entry, as after FPGetIcon of
 *        part of the icon
 */
static int utest_geticon_within_reply_buffer(struct vol *vol)
{
    static char rbuf[GI_REPLY + GI_ICON];
    static const char zero[GI_ICON];
    char bits[GI_PART];
    uint8_t creator[4] = { 'S', 'F', 'g', 'i' };
    uint8_t entry2[GI_ENTRY];
    uint8_t req[GI_REQ];
    uint8_t part[GI_REQ];
    uint8_t info2[GI_INFO];
    size_t rbuflen = GI_REPLY;
    char *path;
    int fd;
    int ret = -1;

    if ((fd = gi_icon_file(vol, creator, &path)) == -1) {
        return -1;
    }

    gi_entry(entry2, 2, GI_SMALL);
    gi_req(req, vol, creator, 1, GI_OVER);
    gi_req(part, vol, creator, 1, GI_PART);
    gi_info(info2, vol, creator, 2);
    memset(bits, 0xa5, sizeof(bits));
    memset(rbuf, 0, sizeof(rbuf));

    if (gi_store(fd, 1, GI_ICON, 0xa5) == 0
            && gi_store(fd, 2, GI_SMALL, 0x5a) == 0
            && afp_geticon(&obj, (char *)req, sizeof(req), rbuf,
                           &rbuflen) == AFPERR_PARAM
            && rbuflen == 0
            && memcmp(rbuf + GI_REPLY, zero, sizeof(zero)) == 0
            && gi_info_is(&obj, info2, entry2)) {
        rbuflen = GI_REPLY;

        if (afp_geticon(&obj, (char *)part, sizeof(part), rbuf,
                        &rbuflen) == AFP_OK
                && rbuflen == GI_PART
                && memcmp(rbuf, bits, GI_PART) == 0
                && gi_info_is(&obj, info2, entry2)) {
            ret = 0;
        }
    }

    close(fd);
    unlink(path);
    return ret;
}

/*!
 * @brief FPGetIconInfo returns the entry each index names, for the same index
 *        asked again as well, and AFPERR_NOITEM past the last whole entry,
 *        a header whose bits the file cuts short being none, which FPGetIcon
 *        does not find either
 */
static int utest_geticoninfo_walks_by_index(struct vol *vol)
{
    uint8_t creator[4] = { 'S', 'F', 'g', 'w' };
    uint8_t entry[GI_ENTRY];
    uint8_t entry2[GI_ENTRY];
    uint8_t cut[GI_ENTRY];
    uint8_t info1[GI_INFO];
    uint8_t info2[GI_INFO];
    uint8_t info3[GI_INFO];
    uint8_t req3[GI_REQ];
    char rbuf[GI_REPLY];
    size_t rbuflen = GI_REPLY;
    size_t iconlen = GI_REPLY;
    char *path;
    int fd;
    int ret = -1;

    if ((fd = gi_icon_file(vol, creator, &path)) == -1) {
        return -1;
    }

    gi_entry(entry, 1, GI_ICON);
    gi_entry(entry2, 2, GI_SMALL);
    gi_entry(cut, 3, GI_SMALL);
    gi_info(info1, vol, creator, 1);
    gi_info(info2, vol, creator, 2);
    gi_info(info3, vol, creator, 3);
    gi_req(req3, vol, creator, 3, GI_SMALL);

    if (gi_store(fd, 1, GI_ICON, 0xa5) == 0
            && gi_store(fd, 2, GI_SMALL, 0x5a) == 0
            && write(fd, cut, sizeof(cut)) == (ssize_t)sizeof(cut)
            && gi_info_is(&obj, info1, entry)
            && gi_info_is(&obj, info1, entry)
            && gi_info_is(&obj, info2, entry2)
            && gi_info_is(&obj, info1, entry)
            && afp_geticoninfo(&obj, (char *)info3, GI_INFO, rbuf, &rbuflen)
            == AFPERR_NOITEM
            && afp_geticon(&obj, (char *)req3, GI_REQ, rbuf, &iconlen)
            == AFPERR_NOITEM) {
        ret = 0;
    }

    close(fd);
    unlink(path);
    return ret;
}

/* the fault an icon call meets in the icon file: its open fails, its first
 * or second read or lseek() fails, or its reads stop at GI_SMALL bytes or
 * short of a header */
enum gi_fault {
    GI_OPEN_FAILS,
    GI_READ1_FAILS,
    GI_READ2_FAILS,
    GI_READ_SHORT,
    GI_HEADER_SHORT,
    GI_SEEK1_FAILS,
    GI_SEEK2_FAILS
};

/*! @brief Arm the fault interposer for the fault and nothing else */
static void gi_arm(enum gi_fault fault)
{
    fault_inject_reset();

    switch (fault) {
    case GI_OPEN_FAILS:
        fi.open_armed = 1;
        fi.open_errno = EACCES;
        break;

    case GI_READ1_FAILS:
        fi.read.armed = 1;
        fi.read.errnum = EIO;
        break;

    case GI_READ2_FAILS:
        fi.read.armed = 1;
        fi.read.fail_after = 1;
        fi.read.errnum = EIO;
        break;

    case GI_READ_SHORT:
        fi.read_max = GI_SMALL;
        break;

    case GI_HEADER_SHORT:
        fi.read_max = GI_ENTRY - 4;
        break;

    case GI_SEEK1_FAILS:
        fi.lseek.armed = 1;
        fi.lseek.errnum = EIO;
        break;

    case GI_SEEK2_FAILS:
        fi.lseek.armed = 1;
        fi.lseek.fail_after = 1;
        fi.lseek.errnum = EIO;
        break;
    }
}

/*!
 * @brief FPGetIcon with the request in req under the fault
 *
 * @returns its answer when the reply is empty, 1 otherwise
 */
static int gi_fault(uint8_t *req, enum gi_fault fault)
{
    /* a read the bound misses lands past the reply buffer, not on the stack */
    static char rbuf[GI_REPLY + GI_ICON];
    size_t rbuflen = GI_REPLY;
    int rc;
    gi_arm(fault);
    rc = afp_geticon(&obj, (char *)req, GI_REQ, rbuf, &rbuflen);
    fault_inject_reset();
    return rbuflen == 0 ? rc : 1;
}

/*!
 * @brief FPGetIconInfo with the request in info under the fault
 *
 * @returns its answer when the reply is empty, 1 otherwise
 */
static int gi_info_fault(uint8_t *info, enum gi_fault fault)
{
    char rbuf[GI_REPLY];
    size_t rbuflen = GI_REPLY;
    int rc;
    gi_arm(fault);
    rc = afp_geticoninfo(&obj, (char *)info, GI_INFO, rbuf, &rbuflen);
    fault_inject_reset();
    return rbuflen == 0 ? rc : 1;
}

/*!
 * @brief FPGetIcon and FPGetIconInfo whose open, read or seek of the icon
 *        file fails, or whose header read stops short of the file's end,
 *        answer AFPERR_MISC, and FPGetIconInfo then returns the entry its
 *        index names
 */
static int utest_icon_failed_io_replies_misc(struct vol *vol)
{
    uint8_t creator[4] = { 'S', 'F', 'g', 'r' };
    uint8_t entry2[GI_ENTRY];
    uint8_t part[GI_REQ];
    uint8_t part2[GI_REQ];
    uint8_t info1[GI_INFO];
    uint8_t info2[GI_INFO];
    char *path;
    int fd;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if ((fd = gi_icon_file(vol, creator, &path)) == -1) {
        return -1;
    }

    gi_entry(entry2, 2, GI_SMALL);
    gi_req(part, vol, creator, 1, GI_PART);
    gi_req(part2, vol, creator, 2, GI_SMALL);
    gi_info(info1, vol, creator, 1);
    gi_info(info2, vol, creator, 2);

    /* the entry FPGetIconInfo finds after each failure, then the answer each
     * failure gives; the failed opens follow FPGetIconInfo's failed read,
     * which closes the file */
    if (gi_store(fd, 1, GI_ICON, 0xa5) == 0
            && gi_store(fd, 2, GI_SMALL, 0x5a) == 0
            && gi_fault(part, GI_READ2_FAILS) < 0
            && gi_info_is(&obj, info2, entry2)
            && gi_fault(part, GI_READ_SHORT) < 0
            && gi_info_is(&obj, info2, entry2)
            && gi_fault(part2, GI_SEEK2_FAILS) < 0
            && gi_info_is(&obj, info2, entry2)
            && gi_info_fault(info2, GI_READ2_FAILS) < 0
            && gi_info_is(&obj, info2, entry2)
            && gi_fault(part, GI_SEEK1_FAILS) == AFPERR_MISC
            && gi_fault(part2, GI_READ2_FAILS) == AFPERR_MISC
            && gi_fault(part2, GI_SEEK2_FAILS) == AFPERR_MISC
            && gi_fault(part, GI_READ2_FAILS) == AFPERR_MISC
            && gi_fault(part, GI_READ_SHORT) == AFPERR_MISC
            && gi_info_fault(info2, GI_READ2_FAILS) == AFPERR_MISC
            && gi_fault(part, GI_HEADER_SHORT) == AFPERR_MISC
            && gi_info_fault(info1, GI_HEADER_SHORT) == AFPERR_MISC
            && gi_fault(part, GI_OPEN_FAILS) == AFPERR_MISC
            && gi_info_fault(info1, GI_OPEN_FAILS) == AFPERR_MISC
            && gi_info_fault(info1, GI_READ1_FAILS) == AFPERR_MISC) {
        ret = 0;
    }

    close(fd);
    unlink(path);
    return ret;
}

#define AI_QUANTUM 2048
#define AI_ICON    1024
#define AI_SENT    512
#define AI_STORED  64
#define AI_REQ     20
#define AI_FRAME   (DSI_BLOCKSIZ + AI_REQ + AI_ICON)
#define AI_CUT     (DSI_BLOCKSIZ + AI_REQ + AI_SENT)

/*! An FPAddIcon session over a socket pair, on an icon file that holds one
 *  stored icon */
struct ai {
    AFPObj o;
    DSI dsi;
    const struct vol *vol;
    uint8_t *creator;         /*!< the icon file's */
    int peer;                 /*!< the client's end */
    char *path;               /*!< the icon file */
    uint8_t entry[GI_ENTRY];  /*!< the stored icon's header */
    uint8_t info[GI_INFO];    /*!< FPGetIconInfo of the stored icon */
};

/*!
 * @brief Store one icon, type 1 of AI_STORED bytes, in the creator's icon
 *        file and open a session on it whose quantum is AI_QUANTUM
 *
 * @returns 0, or -1 with nothing left open
 */
static int ai_setup(struct ai *ai, const struct vol *vol, uint8_t *creator)
{
    int fd;
    ai->vol = vol;
    ai->creator = creator;

    if ((fd = gi_icon_file(vol, creator, &ai->path)) == -1) {
        return -1;
    }

    if (gi_store(fd, 1, AI_STORED, 0x5a) != 0) {
        close(fd);
        unlink(ai->path);
        return -1;
    }

    close(fd);

    if (dsi_test_open(&ai->dsi, AI_QUANTUM, &ai->peer) != 0) {
        unlink(ai->path);
        return -1;
    }

    ai->o = obj;
    ai->o.dsi = &ai->dsi;
    ai->o.proto = AFPPROTO_DSI;
    gi_entry(ai->entry, 1, AI_STORED);
    gi_info(ai->info, vol, creator, 1);
    return 0;
}

/*! @brief End the session and remove the icon file */
static void ai_teardown(struct ai *ai)
{
    close(ai->peer);
    dsi_test_cleanup(&ai->dsi);
    unlink(ai->path);
}

/*!
 * @brief Pack a DSIWrite of FPAddIcon to the session's icon file for an icon
 *        of type itype and size bytes whose payload is the request and len
 *        bytes of bits: command, pad, volume, creator, file type, icon type,
 *        pad, tag, size
 */
static void ai_frame(const struct ai *ai, uint8_t *frame, uint16_t id,
                     uint8_t itype, uint16_t size, uint32_t len)
{
    uint8_t *req = frame + DSI_BLOCKSIZ;
    uint16_t size_be = htons(size);
    dsi_test_frame(frame, DSIFUNC_WRITE, id, AI_REQ, AI_REQ + len,
                   AFP_ADDICON);
    memcpy(req + 2, &ai->vol->v_vid, sizeof(ai->vol->v_vid));
    memcpy(req + 4, ai->creator, 4);
    memcpy(req + 8, "ICN#", 4);
    req[12] = itype;
    memcpy(req + 18, &size_be, sizeof(size_be));

    /* each byte of the bits tells its place, for a reply read back */
    for (uint32_t i = 0; i < len; i++) {
        req[AI_REQ + i] = (uint8_t)(i * 7);
    }
}

/*! @brief Receive the next DSIWrite and run it as FPAddIcon, as the
 *         dispatcher does */
static int ai_dispatch(struct ai *ai)
{
    size_t rbuflen = DSI_DATASIZ;

    if (dsi_stream_receive(&ai->dsi) != DSIFUNC_WRITE) {
        return -1;
    }

    return afp_addicon(&ai->o, (char *)ai->dsi.commands, ai->dsi.cmdlen,
                       (char *)ai->dsi.data, &rbuflen);
}

/*! @brief One FPAddIcon under a file size limit of cap bytes, where the
 *         write of the icon stops short or fails as on a volume filling up */
static int ai_dispatch_capped(struct ai *ai, off_t cap)
{
    struct fsize_cap limit;
    int rc;

    if (fsize_cap(&limit, (rlim_t)cap) != 0) {
        return -1;
    }

    rc = ai_dispatch(ai);
    return fsize_uncap(&limit) == 0 ? rc : -1;
}

/*! @brief Is the session's icon file size bytes long? */
static int ai_size_is(const struct ai *ai, off_t size)
{
    struct stat st;
    return stat(ai->path, &st) == 0 && st.st_size == size;
}

/*! @brief The icon file holds the stored icon alone, and FPGetIconInfo
 *         reads it */
static int ai_stored_alone(struct ai *ai)
{
    return ai_size_is(ai, GI_ENTRY + AI_STORED)
           && gi_info_is(&ai->o, ai->info, ai->entry);
}

/*! @brief Set the tag of an FPAddIcon request ai_frame() packed */
static void ai_tag(uint8_t *frame, const uint8_t *tag)
{
    memcpy(frame + DSI_BLOCKSIZ + 14, tag, 4);
}

/*!
 * @brief The stored icon's entry, at its length, carries the tag and holds a
 *        run of the bits ai_frame() packs from their start, the stored bits
 *        after them, as a replacement whose write failed part way leaves it
 */
static int ai_replaced_part_way(struct ai *ai, const uint8_t *tag)
{
    uint8_t entry[GI_ENTRY];
    uint8_t req[GI_REQ];
    size_t rbuflen = DSI_DATASIZ;
    uint16_t i = 0;
    gi_entry(entry, 1, AI_STORED);
    memcpy(entry, tag, 4);
    gi_req(req, ai->vol, ai->creator, 1, AI_STORED);

    if (!ai_size_is(ai, GI_ENTRY + AI_STORED)
            || !gi_info_is(&ai->o, ai->info, entry)
            || afp_geticon(&ai->o, (char *)req, GI_REQ, (char *)ai->dsi.data,
                           &rbuflen) != AFP_OK || rbuflen != AI_STORED) {
        return 0;
    }

    while (i < AI_STORED && ai->dsi.data[i] == (uint8_t)(i * 7)) {
        i++;
    }

    while (i < AI_STORED && ai->dsi.data[i] == 0x5a) {
        i++;
    }

    return i == AI_STORED;
}

/*!
 * @brief FPAddIcon of a new icon under the fault replies AFPERR_MISC and
 *        stores no new icon; tag ends the creator
 *
 * @returns 0, or -1
 */
static int ai_fault_misc(struct vol *vol, enum gi_fault fault, uint8_t tag)
{
    static uint8_t frame[AI_FRAME];
    uint8_t creator[4] = { 'S', 'F', 'a', tag };
    struct ai ai;
    int rc = -1;
    int ret = -1;

    if (ai_setup(&ai, vol, creator) != 0) {
        return -1;
    }

    ai_frame(&ai, frame, 1, 2, AI_ICON, AI_ICON);
    gi_arm(fault);

    if (write(ai.peer, frame, sizeof(frame)) == (ssize_t)sizeof(frame)) {
        rc = ai_dispatch(&ai);
    }

    fault_inject_reset();

    if (rc == AFPERR_MISC && ai_stored_alone(&ai)) {
        ret = 0;
    }

    ai_teardown(&ai);
    return ret;
}

/*!
 * @brief FPAddIcon whose icon file cannot be opened, whose scan meets a
 *        failed read() of it, a read() short of the stored icon's header or
 *        a failed lseek() over the stored icon, or whose seek to the new
 *        entry's offset fails, replies AFPERR_MISC and stores no new icon
 */
static int utest_addicon_failed_io_replies_misc(struct vol *vol)
{
    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    return ai_fault_misc(vol, GI_OPEN_FAILS, 'o') == 0
           && ai_fault_misc(vol, GI_READ1_FAILS, 'r') == 0
           && ai_fault_misc(vol, GI_HEADER_SHORT, 'h') == 0
           && ai_fault_misc(vol, GI_SEEK1_FAILS, 'k') == 0
           && ai_fault_misc(vol, GI_SEEK2_FAILS, 'p') == 0 ? 0 : -1;
}

/*!
 * @brief Receive and run the next FPAddIcon with its first lock attempt
 *        failing with errnum, the interposer's lock record kept
 */
static int ai_dispatch_lock_refused(struct ai *ai, int errnum)
{
    int rc;
    fi.fcntl_armed = 1;
    fi.fcntl_fail_after = 0;
    fi.fcntl_errno = errnum;
    rc = ai_dispatch(ai);
    fi.fcntl_armed = 0;
    return rc;
}

/*!
 * @brief ai_dispatch_lock_refused() with the scan's first read() armed to
 *        fail
 */
static int ai_dispatch_lock_fails(struct ai *ai, int errnum)
{
    int rc;
    fi.read.armed = 1;
    fi.read.fail_after = 0;
    fi.read.errnum = EIO;
    rc = ai_dispatch_lock_refused(ai, errnum);
    fi.read.armed = 0;
    return rc;
}

/*!
 * @brief FPAddIcon takes a write lock on the whole icon file before its scan,
 *        tries again while another process holds it, writes without one on a
 *        file system that takes none, gives up on any other error, and takes
 *        none for a request refused before the file opens
 */
static int utest_addicon_locks_icon_file(struct vol *vol)
{
    static uint8_t full[AI_FRAME];
    static uint8_t frame[DSI_BLOCKSIZ + AI_REQ + AI_QUANTUM];
    static uint8_t again[AI_FRAME];
    uint8_t creator[4] = { 'S', 'F', 'a', 'l' };
    struct ai ai;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (ai_setup(&ai, vol, creator) != 0) {
        return -1;
    }

    /* a request the server stores, one whose payload is not the icon's
     * size, three stored without a lock, one whose lock fails before its scan
     * reads anything, and one whose scan, reached on the second lock
     * attempt, fails its read */
    ai_frame(&ai, full, 1, 2, AI_ICON, AI_ICON);
    ai_frame(&ai, frame, 2, 2, AI_ICON, AI_QUANTUM);
    ai_frame(&ai, again, 3, 2, AI_ICON, AI_ICON);
    fault_inject_reset();
    fi.fcntl_watch = 1;

    if (write(ai.peer, full, sizeof(full)) == (ssize_t)sizeof(full)
            && write(ai.peer, frame, sizeof(frame)) == (ssize_t)sizeof(frame)
            && ai_dispatch(&ai) == AFP_OK
            && fi.lock_last_cmd == F_SETLK && fi.lock_last_type == F_WRLCK
            && fi.lock_last_start == 0 && fi.lock_last_len == 0
            && ai_dispatch(&ai) == AFPERR_PARAM && fi.setlk_calls == 1
            && write(ai.peer, again, sizeof(again)) == (ssize_t)sizeof(again)
            && ai_dispatch_lock_refused(&ai, ENOLCK) == AFP_OK
            && fi.setlk_calls == 2
            && write(ai.peer, again, sizeof(again)) == (ssize_t)sizeof(again)
            && ai_dispatch_lock_refused(&ai, EINVAL) == AFP_OK
            && fi.setlk_calls == 3
            && write(ai.peer, again, sizeof(again)) == (ssize_t)sizeof(again)
            && ai_dispatch_lock_refused(&ai, EOPNOTSUPP) == AFP_OK
            && fi.setlk_calls == 4
            && write(ai.peer, again, sizeof(again)) == (ssize_t)sizeof(again)
            && ai_dispatch_lock_fails(&ai, EIO) == AFPERR_MISC
            && fi.setlk_calls == 5 && fi.read.calls == 0
            && write(ai.peer, again, sizeof(again)) == (ssize_t)sizeof(again)
            && ai_dispatch_lock_fails(&ai, EAGAIN) == AFPERR_MISC
            && fi.setlk_calls == 7 && fi.read.calls == 1) {
        ret = 0;
    }

    fault_inject_reset();
    ai_teardown(&ai);
    return ret;
}

/*!
 * @brief FPAddIcon gives up on an icon file another process keeps locked,
 *        replying AFPERR_MISC and storing nothing
 */
static int utest_addicon_lock_wait_bounded(struct vol *vol)
{
    static uint8_t frame[AI_FRAME];
    uint8_t creator[4] = { 'S', 'F', 'a', 'w' };
    struct peer peer;
    struct ai ai;
    int ret = -1;

    if (ai_setup(&ai, vol, creator) != 0) {
        return -1;
    }

    ai_frame(&ai, frame, 1, 2, AI_ICON, AI_ICON);

    if (peer_hold_lock(&peer, ai.path, F_RDLCK, 0, 0) == 0) {
        if (write(ai.peer, frame, sizeof(frame)) == (ssize_t)sizeof(frame)
                && ai_dispatch(&ai) == AFPERR_MISC
                && ai_stored_alone(&ai)) {
            ret = 0;
        }

        peer_release(&peer);
    }

    ai_teardown(&ai);
    return ret;
}

#if !defined(NO_DDP) && defined(AFPDTEST_SHARED_LIBATALK)
/*!
 * @brief One FPAddIcon over AppleTalk of the request in frame under the
 *        fault, the icon handed over whole
 */
static int ai_asp_dispatch(struct ai *ai, uint8_t *frame, enum gi_fault fault)
{
    char rbuf[AI_ICON];
    size_t rbuflen = sizeof(rbuf);
    int rc;
    gi_arm(fault);
    fi.asp_wrtcont_fill = 1;
    rc = afp_addicon(&ai->o, (char *)frame + DSI_BLOCKSIZ, AI_REQ, rbuf,
                     &rbuflen);
    fault_inject_reset();
    return rc;
}
#endif

/*!
 * @brief FPAddIcon over AppleTalk whose scan fails, or whose icon file cannot
 *        be opened, replies AFPERR_MISC and stores no icon
 */
static int utest_addicon_asp_failure_reported(struct vol *vol)
{
#if defined(NO_DDP) || !defined(AFPDTEST_SHARED_LIBATALK)
    return TEST_SKIP;
#else
    static uint8_t frame[AI_FRAME];
    uint8_t creator[4] = { 'S', 'F', 'a', 'a' };
    struct ai ai;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (ai_setup(&ai, vol, creator) != 0) {
        return -1;
    }

    ai.o.proto = AFPPROTO_ASP;
    ai.o.dsi = NULL;
    ai_frame(&ai, frame, 1, 2, AI_ICON, 0);

    if (ai_asp_dispatch(&ai, frame, GI_READ1_FAILS) == AFPERR_MISC
            && ai_asp_dispatch(&ai, frame, GI_OPEN_FAILS) == AFPERR_MISC
            && ai_stored_alone(&ai)) {
        ret = 0;
    }

    ai_teardown(&ai);
    return ret;
#endif
}

/*!
 * @brief FPAddIcon whose payload differs from the announced icon size, whose
 *        write of the icon fails at a file size limit, for a new entry or
 *        over the stored icon, or whose client ends the stream before the
 *        icon is complete, fails the request and adds no entry; the stored
 *        icon's entry stays readable at its length
 */
static int utest_addicon_failed_request_adds_no_entry(struct vol *vol)
{
    static uint8_t frame[DSI_BLOCKSIZ + AI_REQ + AI_QUANTUM];
    static uint8_t full[AI_FRAME];
    static uint8_t over[DSI_BLOCKSIZ + AI_REQ + AI_STORED];
    static uint8_t cut[AI_FRAME];
    static const uint8_t tag[4] = { 0, 0, 0, 7 };
    uint8_t creator[4] = { 'S', 'F', 'a', 'i' };
    /* room for the stored icon, the new entry's header and half its bits */
    off_t cap = GI_ENTRY + AI_STORED + GI_ENTRY + AI_SENT;
    /* room for half the stored icon's bits */
    off_t inside = GI_ENTRY + AI_STORED / 2;
    struct ai ai;
    int ret = -1;

    if (ai_setup(&ai, vol, creator) != 0) {
        return -1;
    }

    /* the first frame is longer than the session's read-ahead buffer */
    ai_frame(&ai, frame, 1, 2, AI_ICON, AI_QUANTUM);
    ai_frame(&ai, full, 2, 2, AI_ICON, AI_ICON);
    ai_frame(&ai, over, 3, 1, AI_STORED, AI_STORED);
    ai_tag(over, tag);
    ai_frame(&ai, cut, 4, 2, AI_ICON, AI_ICON);

    if (write(ai.peer, frame, sizeof(frame)) == (ssize_t)sizeof(frame)
            && write(ai.peer, full, sizeof(full)) == (ssize_t)sizeof(full)
            && write(ai.peer, over, sizeof(over)) == (ssize_t)sizeof(over)
            && write(ai.peer, cut, AI_CUT) == AI_CUT
            && shutdown(ai.peer, SHUT_WR) == 0
            && ai_dispatch(&ai) == AFPERR_PARAM
            && ai_stored_alone(&ai)
            && ai_dispatch_capped(&ai, cap) == AFPERR_MISC
            && ai_stored_alone(&ai)
            && ai_dispatch_capped(&ai, inside) == AFPERR_MISC
            && ai_replaced_part_way(&ai, tag)
            && ai_dispatch(&ai) == AFPERR_PARAM
            && ai_replaced_part_way(&ai, tag)) {
        ret = 0;
    }

    ai_teardown(&ai);
    return ret;
}

/*!
 * @brief Does FPGetIcon of the session's icon of type itype return size
 *        bytes, each the one ai_frame() put at its place?
 */
static int ai_icon_is(struct ai *ai, uint8_t itype, uint16_t size)
{
    uint8_t req[GI_REQ];
    size_t rbuflen = DSI_DATASIZ;
    gi_req(req, ai->vol, ai->creator, itype, size);

    if (afp_geticon(&ai->o, (char *)req, GI_REQ, (char *)ai->dsi.data,
                    &rbuflen) != AFP_OK || rbuflen != size) {
        return 0;
    }

    for (uint16_t i = 0; i < size; i++) {
        if (ai->dsi.data[i] != (uint8_t)(i * 7)) {
            return 0;
        }
    }

    return 1;
}

/*!
 * @brief FPAddIcon stores an icon whose payload comes in two pieces whole,
 *        replaces the stored icon of its type under another tag, header and
 *        bits, and refuses that icon at another size with AFPERR_ITYPE
 */
static int utest_addicon_stores_icon_by_type(struct vol *vol)
{
    static uint8_t two[DSI_BLOCKSIZ + AI_REQ + AI_QUANTUM];
    static uint8_t same[DSI_BLOCKSIZ + AI_REQ + AI_STORED];
    static uint8_t other[DSI_BLOCKSIZ + AI_REQ + 2 * AI_STORED];
    static const uint8_t tag[4] = { 0, 0, 0, 7 };
    static const uint8_t tag2[4] = { 0, 0, 0, 9 };
    uint8_t creator[4] = { 'S', 'F', 'a', 's' };
    /* the stored icon, then the new one's header and bits */
    off_t size = GI_ENTRY + AI_STORED + GI_ENTRY + AI_QUANTUM;
    struct ai ai;
    int ret = -1;

    if (ai_setup(&ai, vol, creator) != 0) {
        return -1;
    }

    ai_frame(&ai, two, 1, 2, AI_QUANTUM, AI_QUANTUM);
    ai_frame(&ai, same, 2, 1, AI_STORED, AI_STORED);
    ai_tag(same, tag);
    ai_frame(&ai, other, 3, 1, 2 * AI_STORED, 2 * AI_STORED);
    ai_tag(other, tag2);
    /* the stored icon's header as the second request leaves it */
    memcpy(ai.entry, tag, sizeof(tag));

    if (write(ai.peer, two, sizeof(two)) == (ssize_t)sizeof(two)
            && write(ai.peer, same, sizeof(same)) == (ssize_t)sizeof(same)
            && write(ai.peer, other, sizeof(other)) == (ssize_t)sizeof(other)
            && ai_dispatch(&ai) == AFP_OK
            && ai_icon_is(&ai, 2, AI_QUANTUM)
            && ai_dispatch(&ai) == AFP_OK && ai_size_is(&ai, size)
            && gi_info_is(&ai.o, ai.info, ai.entry)
            && ai_icon_is(&ai, 1, AI_STORED)
            && ai_dispatch(&ai) == AFPERR_ITYPE && ai_size_is(&ai, size)) {
        ret = 0;
    }

    ai_teardown(&ai);
    return ret;
}

/*! @brief Append len bytes of tail to the icon file, after the stored icon */
static int ai_append(const struct ai *ai, const uint8_t *tail, size_t len)
{
    ssize_t n = -1;
    int fd;

    if ((fd = open(ai->path, O_WRONLY | O_APPEND)) != -1) {
        n = write(fd, tail, len);
        close(fd);
    }

    return n == (ssize_t)len;
}

/*!
 * @brief FPAddIcon of an icon of type 2 and the stored icon's size lands
 *        right after the stored icon, in place of what follows it, and
 *        FPGetIconInfo and FPGetIcon read it there
 */
static int ai_add_after_stored(struct ai *ai)
{
    static uint8_t frame[DSI_BLOCKSIZ + AI_REQ + AI_STORED];
    uint8_t entry2[GI_ENTRY];
    uint8_t info2[GI_INFO];
    gi_entry(entry2, 2, AI_STORED);
    gi_info(info2, ai->vol, ai->creator, 2);
    ai_frame(ai, frame, 1, 2, AI_STORED, AI_STORED);
    return write(ai->peer, frame, sizeof(frame)) == (ssize_t)sizeof(frame)
           && ai_dispatch(ai) == AFP_OK
           && ai_size_is(ai, 2 * (GI_ENTRY + AI_STORED))
           && gi_info_is(&ai->o, info2, entry2)
           && ai_icon_is(ai, 2, AI_STORED);
}

/*!
 * @brief FPAddIcon on an icon file that ends in a torn header stores the
 *        icon after the last whole entry, over the torn bytes, where FPGetIcon
 *        finds no icon before it
 */
static int utest_addicon_overwrites_torn_header(struct vol *vol)
{
    uint8_t creator[4] = { 'S', 'F', 'a', 't' };
    uint8_t entry3[GI_ENTRY];
    uint8_t req[GI_REQ];
    size_t rbuflen = DSI_DATASIZ;
    struct ai ai;
    int ret = -1;

    if (ai_setup(&ai, vol, creator) != 0) {
        return -1;
    }

    gi_entry(entry3, 3, AI_STORED);
    gi_req(req, vol, creator, 3, AI_STORED);

    /* an icon's header, the stored one's size, cut before its size */
    if (ai_append(&ai, entry3, GI_ENTRY - 2)
            && afp_geticon(&ai.o, (char *)req, GI_REQ, (char *)ai.dsi.data,
                           &rbuflen) == AFPERR_NOITEM
            && ai_add_after_stored(&ai)) {
        ret = 0;
    }

    ai_teardown(&ai);
    return ret;
}

/*!
 * @brief FPAddIcon on an icon file that ends in an entry whose bits stop
 *        short of its size stores the icon after the last whole entry and
 *        cuts off what is left of the short one
 */
static int utest_addicon_overwrites_short_entry(struct vol *vol)
{
    uint8_t creator[4] = { 'S', 'F', 'a', 'u' };
    uint8_t cut[GI_ENTRY + AI_SENT];
    struct ai ai;
    int ret = -1;

    if (ai_setup(&ai, vol, creator) != 0) {
        return -1;
    }

    /* an icon of AI_ICON bytes whose bits stop after AI_SENT, which run past
     * the entry that takes its place */
    gi_entry(cut, 3, AI_ICON);
    memset(cut + GI_ENTRY, 0x5a, AI_SENT);

    if (ai_append(&ai, cut, sizeof(cut))
            && ai_add_after_stored(&ai)) {
        ret = 0;
    }

    ai_teardown(&ai);
    return ret;
}

/*!
 * @brief FPGetIconInfo of the null creator walks its icon file as any other
 *        creator's: AFPERR_NOITEM without one, its stored entry with one,
 *        and AFPERR_NOITEM for index 0
 */
static int utest_geticoninfo_null_creator_walks_file(struct vol *vol)
{
    uint8_t creator[4] = { 0, 0, 0, 0 };
    uint8_t entry[GI_ENTRY];
    uint8_t info0[GI_INFO];
    uint8_t info1[GI_INFO];
    char rbuf[GI_REPLY];
    size_t rbuflen = GI_REPLY;
    char *path;
    int fd;
    int ret = -1;
    gi_entry(entry, 1, GI_SMALL);
    gi_info(info0, vol, creator, 0);
    gi_info(info1, vol, creator, 1);

    if (afp_geticoninfo(&obj, (char *)info1, GI_INFO, rbuf, &rbuflen)
            != AFPERR_NOITEM
            || (fd = gi_icon_file(vol, creator, &path)) == -1) {
        return -1;
    }

    rbuflen = GI_REPLY;

    if (gi_store(fd, 1, GI_SMALL, 0x5a) == 0
            && gi_info_is(&obj, info1, entry)
            && afp_geticoninfo(&obj, (char *)info0, GI_INFO, rbuf, &rbuflen)
            == AFPERR_NOITEM) {
        ret = 0;
    }

    close(fd);
    unlink(path);
    return ret;
}

/* three of copy_fork()'s 8 KiB pieces and part of a fourth */
#define CF_LEN (3 * 8192 + 100)

/*! @brief copy_fork() of the data fork, each write() capped at max bytes and
 *         the first zero of them writing nothing */
static int cf_copy(struct ofork *dst, struct ofork *src, int max, int zero)
{
    int rc;
    fault_inject_reset();
    fi.write_max = max;
    fi.write_zero = zero;
    rc = copy_fork(ADEID_DFORK, dst->of_ad, src->of_ad, NULL, 0);
    fault_inject_reset();
    return rc;
}

/*! @brief copy_fork() whose writes stop short copies the data fork whole,
 *         and one whose write writes nothing fails with EIO */
static int utest_copy_fork_writes_whole_or_fails(struct vol *vol)
{
    static char data[CF_LEN];
    static char back[CF_LEN];
    struct ofork *src;
    struct ofork *dst;
    uint16_t srefnum = 0;
    uint16_t drefnum = 0;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    /* no two of copy_fork()'s pieces alike */
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (char)(i * 7 + i / 8192);
    }

    /* each write takes 1000 of the 8192 bytes copy_fork() hands it, then a
     * write that writes nothing ends the copy */
    if (createfile(&obj, vol->v_vid, DIRDID_ROOT, "cf_src") == AFP_OK
            && createfile(&obj, vol->v_vid, DIRDID_ROOT, "cf_dst") == AFP_OK
            && openfork(&obj, vol->v_vid, DIRDID_ROOT, "cf_src", OPENFORK_DATA,
                        OPENACC_RD | OPENACC_WR, &srefnum) == AFP_OK
            && openfork(&obj, vol->v_vid, DIRDID_ROOT, "cf_dst", OPENFORK_DATA,
                        OPENACC_RD | OPENACC_WR, &drefnum) == AFP_OK
            && (src = of_find(srefnum)) != NULL
            && (dst = of_find(drefnum)) != NULL
            && ad_write(src->of_ad, ADEID_DFORK, 0, 0, data, sizeof(data))
            == (ssize_t)sizeof(data)
            && cf_copy(dst, src, 1000, 0) == 0
            && ad_read(dst->of_ad, ADEID_DFORK, 0, back, sizeof(back))
            == (ssize_t)sizeof(back)
            && memcmp(back, data, sizeof(data)) == 0
            && cf_copy(dst, src, 0, 1) == -1 && errno == EIO) {
        ret = 0;
    }

    if (srefnum != 0) {
        closefork(&obj, srefnum);
    }

    if (drefnum != 0) {
        closefork(&obj, drefnum);
    }

    delete (&obj, vol->v_vid, DIRDID_ROOT, "cf_src");
    delete (&obj, vol->v_vid, DIRDID_ROOT, "cf_dst");
    return ret;
}

_Static_assert(offsetof(struct guarded_adouble,
                        guard) == sizeof(struct adouble),
               "guard must immediately follow struct adouble");

static int utest_ad_copy_header_valid_finderinfo(void)
{
    struct adouble src;
    struct adouble dst;
    char *src_finderinfo;
    const char *dst_finderinfo;
    ad_init_old(&src, AD_VERSION2, 0);
    ad_init_old(&dst, AD_VERSION2, 0);

    if (ad_init_offsets(&src) != 0 || ad_init_offsets(&dst) != 0) {
        return 1;
    }

    src_finderinfo = ad_entry(&src, ADEID_FINDERI);

    if (src_finderinfo == NULL) {
        return 2;
    }

    memset(src_finderinfo, 'F', ADEDLEN_FINDERI);

    if (ad_copy_header(&dst, &src) != 0) {
        return 3;
    }

    dst_finderinfo = ad_entry(&dst, ADEID_FINDERI);

    if (dst_finderinfo == NULL
            || ad_getentrylen(&dst, ADEID_FINDERI) != ADEDLEN_FINDERI
            || memcmp(dst_finderinfo, src_finderinfo, ADEDLEN_FINDERI) != 0) {
        return 4;
    }

    return 0;
}

static int utest_ad_copy_header_bounds_finderinfo(void)
{
    static const uint8_t guard_byte = 0xa5;
    struct adouble src;
    struct guarded_adouble dst;
    const uint32_t finderi_len = AD_DATASZ2 - 1;
    int copy_ret;
    ad_init_old(&src, AD_VERSION2, 0);
    ad_init_old(&dst.ad, AD_VERSION2, 0);

    if (ad_init_offsets(&src) != 0 || ad_init_offsets(&dst.ad) != 0) {
        return 1;
    }

    memset(src.ad_data, 'A', src.valid_data_len);
    ad_setentryoff(&src, ADEID_FINDERI, 1);
    ad_setentrylen(&src, ADEID_FINDERI, finderi_len);
    memset(dst.guard, guard_byte, sizeof(dst.guard));

    /* Both operands pass the same ad_entry() checks used by
     * ad_copy_header(): source against 740 bytes, destination against its old
     * 32-byte length. */
    if (ad_entry(&src, ADEID_FINDERI) == NULL
            || ad_entry(&dst.ad, ADEID_FINDERI) == NULL) {
        return 2;
    }

    copy_ret = ad_copy_header(&dst.ad, &src);

    for (size_t i = 0; i < sizeof(dst.guard); i++) {
        if (dst.guard[i] != guard_byte) {
            return 3;
        }
    }

    /* An oversized entry must be rejected without committing its length. */
    if (copy_ret != -1) {
        return 4;
    }

    if (ad_getentrylen(&dst.ad, ADEID_FINDERI) != ADEDLEN_FINDERI) {
        return 5;
    }

    return 0;
}

/*!
 * @brief A rejected entry leaves every earlier entry of the destination intact
 *
 * ADEID_NAME precedes ADEID_FINDERI, so the name is what a per-entry copy
 * would already have committed when the oversized FinderInfo is refused.
 */
static int utest_ad_copy_header_rejects_before_writing(void)
{
    struct adouble src;
    struct adouble dst;
    const char *dst_name;
    ad_init_old(&src, AD_VERSION2, 0);
    ad_init_old(&dst, AD_VERSION2, 0);

    if (ad_init_offsets(&src) != 0 || ad_init_offsets(&dst) != 0) {
        return 1;
    }

    ad_setentrylen(&src, ADEID_NAME, 4);
    memcpy(ad_entry(&src, ADEID_NAME), "NAME", 4);
    memset(ad_entry(&src, ADEID_FINDERI), 'F', ADEDLEN_FINDERI);
    ad_setentryoff(&src, ADEID_FINDERI, 1);
    ad_setentrylen(&src, ADEID_FINDERI, AD_DATASZ2 - 1);

    if (ad_copy_header(&dst, &src) != -1) {
        return 2;
    }

    dst_name = dst.ad_data + ad_getentryoff(&dst, ADEID_NAME);

    if (ad_getentrylen(&dst, ADEID_NAME) != 0
            || memcmp(dst_name, "\0\0\0\0", 4) != 0) {
        return 3;
    }

    return 0;
}

static int utest_ad_copy_header_skips_dfork(void)
{
    static const uint8_t guard_byte = 0xa5;
    struct adouble src;
    struct guarded_adouble dst;
    int copy_ret;
    ad_init_old(&src, AD_VERSION2, 0);
    ad_init_old(&dst.ad, AD_VERSION2, 0);

    if (ad_init_offsets(&src) != 0 || ad_init_offsets(&dst.ad) != 0) {
        return 1;
    }

    /* Each entry fits by itself, but copying the source length at the
     * destination offset would write 739 bytes beyond valid_data_len. */
    memset(src.ad_data, 'D', src.valid_data_len);
    ad_setentryoff(&src, ADEID_DFORK, 1);
    ad_setentrylen(&src, ADEID_DFORK, AD_DATASZ2 - 1);
    ad_setentryoff(&dst.ad, ADEID_DFORK, AD_DATASZ2 - 1);
    ad_setentrylen(&dst.ad, ADEID_DFORK, 1);
    memset(dst.guard, guard_byte, sizeof(dst.guard));
    copy_ret = ad_copy_header(&dst.ad, &src);

    if (copy_ret != 0) {
        return 2;
    }

    for (size_t i = 0; i < sizeof(dst.guard); i++) {
        if (dst.guard[i] != guard_byte) {
            return 3;
        }
    }

    /* Fork contents and their entry metadata are not header metadata. */
    if (ad_getentryoff(&dst.ad, ADEID_DFORK) != AD_DATASZ2 - 1
            || ad_getentrylen(&dst.ad, ADEID_DFORK) != 1) {
        return 4;
    }

    return 0;
}

#ifdef WITH_SPOTLIGHT
static uint64_t spotlight_test_get_le64(const char *buf, size_t offset)
{
    const uint8_t *p = (const uint8_t *)buf + offset;
    return (uint64_t)p[0]
           | ((uint64_t)p[1] << 8)
           | ((uint64_t)p[2] << 16)
           | ((uint64_t)p[3] << 24)
           | ((uint64_t)p[4] << 32)
           | ((uint64_t)p[5] << 40)
           | ((uint64_t)p[6] << 48)
           | ((uint64_t)p[7] << 56);
}

static int utest_spotlight_rejects_empty_long_cnids(void)
{
    char rpcbuf[DSI_DATASIZ - 64];
    TALLOC_CTX *tmp = talloc_new(NULL);
    DALLOC_CTX *packed = talloc_zero(tmp, DALLOC_CTX);
    DALLOC_CTX *unpacked = talloc_zero(tmp, DALLOC_CTX);
    sl_cnids_t *cnids = talloc_zero(packed, sl_cnids_t);
    const uint16_t ca_unkn1 = 0x0fec;
    const uint32_t ca_context = 0x5a17c0de;
    uint64_t cnid = 0x1122334455667788;
    uint64_t cnid_header;
    int rpclen;
    int result = 0;

    if (tmp == NULL || packed == NULL || unpacked == NULL || cnids == NULL) {
        goto cleanup;
    }

    cnids->ca_cnids = talloc_zero(cnids, DALLOC_CTX);

    if (cnids->ca_cnids == NULL) {
        goto cleanup;
    }

    cnids->ca_unkn1 = ca_unkn1;
    cnids->ca_context = ca_context;
    dalloc_add_copy(cnids->ca_cnids, &cnid, uint64_t);
    dalloc_add(packed, cnids, sl_cnids_t);
    rpclen = sl_pack(packed, rpcbuf);

    if (rpclen < 0) {
        goto cleanup;
    }

    cnid_header = ((uint64_t)ca_context << 32)
                  | ((uint64_t)ca_unkn1 << 16) | 1;

    for (size_t offset = 16;
            offset + sizeof(uint64_t) <= (size_t)rpclen;
            offset += sizeof(uint64_t)) {
        if (spotlight_test_get_le64(rpcbuf, offset) != cnid_header) {
            continue;
        }

        rpcbuf[offset] = 0;
        rpcbuf[offset + 1] = 0;
        result = sl_unpack_len(unpacked, rpcbuf, (size_t)rpclen);
        break;
    }

cleanup:
    talloc_free(tmp);
    return result == -1 ? 0 : -1;
}

static int utest_spotlight_cnid_array_types(void)
{
    TALLOC_CTX *tmp = talloc_new(NULL);
    slq_t *slq = talloc_zero(tmp, slq_t);
    sl_array_t *items = talloc_zero(tmp, sl_array_t);
    sl_bool_t bool_item = true;
    uint64_t cnid1 = 9;
    uint64_t cnid2 = 3;
    int result = -1;

    if (tmp == NULL || slq == NULL || items == NULL) {
        goto cleanup;
    }

    dalloc_add_copy(items, &bool_item, sl_bool_t);

    if (sl_createCNIDArray(slq, items) != -1
            || slq->slq_cnids != NULL
            || slq->slq_cnids_num != 0) {
        goto cleanup;
    }

    talloc_free(items);
    items = talloc_zero(tmp, sl_array_t);

    if (items == NULL) {
        goto cleanup;
    }

    dalloc_add_copy(items, &cnid1, uint64_t);
    dalloc_add_copy(items, &cnid2, uint64_t);

    if (sl_createCNIDArray(slq, items) != 0
            || slq->slq_cnids == NULL
            || slq->slq_cnids_num != 2
            || slq->slq_cnids[0] != cnid2
            || slq->slq_cnids[1] != cnid1) {
        goto cleanup;
    }

    result = 0;
cleanup:
    talloc_free(tmp);
    return result;
}
#endif

/*! @brief Receive one request with this header and payload on a new session */
static int dsi_test_receive(uint8_t command, uint32_t code_or_doff,
                            uint32_t len, const uint8_t *payload,
                            size_t payload_len, uint32_t quantum, DSI *dsi)
{
    uint8_t block[DSI_BLOCKSIZ];
    int peer;
    ssize_t written;

    if (dsi_test_open(dsi, quantum, &peer) != 0) {
        return -1;
    }

    dsi_test_header(block, command, 0x1234, code_or_doff, len);
    written = write(peer, block, sizeof(block));

    if (written != sizeof(block)) {
        close(peer);
        return -1;
    }

    if (payload_len > 0) {
        written = write(peer, payload, payload_len);

        if (written != (ssize_t)payload_len) {
            close(peer);
            return -1;
        }
    }

    close(peer);
    return dsi_stream_receive(dsi);
}

/* dsi_len = quantum + doff is spec-legal: the quantum bounds write data,
 * not the whole frame.  Only the header drives accept/reject, so the
 * harness sends just doff payload bytes; data is consumed by dsi_write. */
static int utest_dsi_receive_accepts_full_quantum_write(uint32_t doff)
{
    DSI dsi;
    uint8_t cmd[DSI_WROFF_FPWRITEEXT];
    int ret;
    memset(cmd, 0, sizeof(cmd));
    ret = dsi_test_receive(DSIFUNC_WRITE, doff, 4096 + doff, cmd, doff,
                           4096, &dsi);
    dsi_test_cleanup(&dsi);
    return ret == DSIFUNC_WRITE ? 0 : -1;
}

/* One byte of data beyond the quantum is rejected. */
static int utest_dsi_receive_rejects_overquantum_write(uint32_t doff)
{
    DSI dsi;
    uint8_t cmd[DSI_WROFF_FPWRITEEXT];
    int ret;
    memset(cmd, 0, sizeof(cmd));
    ret = dsi_test_receive(DSIFUNC_WRITE, doff, 4096 + doff + 1, cmd, doff,
                           4096, &dsi);
    dsi_test_cleanup(&dsi);
    return ret == 0 ? 0 : -1;
}

/* doff outside {12, 20} is malformed; doff 0 maps to 12 (ASC 3.7). */
static int utest_dsi_receive_validates_doff(void)
{
    DSI dsi;
    uint8_t cmd[DSI_WROFF_FPWRITEEXT + 1];
    int ret;
    memset(cmd, 0, sizeof(cmd));
    ret = dsi_test_receive(DSIFUNC_WRITE, 13, 64, cmd, 13, 4096, &dsi);
    dsi_test_cleanup(&dsi);

    if (ret != 0) {
        return -1;
    }

    ret = dsi_test_receive(DSIFUNC_WRITE, 21, 64, cmd, 21, 4096, &dsi);
    dsi_test_cleanup(&dsi);

    if (ret != 0) {
        return -1;
    }

    ret = dsi_test_receive(DSIFUNC_WRITE, 0, DSI_WROFF_FPWRITE + 8, cmd,
                           DSI_WROFF_FPWRITE + 8, 4096, &dsi);
    dsi_test_cleanup(&dsi);
    return ret == DSIFUNC_WRITE ? 0 : -1;
}

/* Alignment search: grows to the smallest page multiple at or above the
 * configured value meeting the fill threshold; growth bounded by 256
 * KiB and DSI_SERVQUANT_MAX; never returns less than configured. */
static int utest_dsi_align_quantum(void)
{
    static const uint32_t mss_cases[] = { 1448, 1460, 8948, 8960, 65483, 536 };
    static const uint32_t quanta[] = { 0x100000, 0x400000, 0x100ABC };

    for (size_t q = 0; q < sizeof(quanta) / sizeof(quanta[0]); q++) {
        for (size_t m = 0; m < sizeof(mss_cases) / sizeof(mss_cases[0]); m++) {
            uint32_t mss = mss_cases[m];
            uint32_t configured = quanta[q];
            uint32_t d = dsi_align_quantum(configured, mss);

            if (d < configured || d > configured + DSI_QUANTUM_GROWTH_MAX
                    || d % 4096 != 0 || d > DSI_SERVQUANT_MAX) {
                return -1;
            }
        }
    }

    /* No usable MSS: page-align up only. */
    if (dsi_align_quantum(0x100000, 0) != 0x100000) {
        return -1;
    }

    if (dsi_align_quantum(0x100ABC, 0) != 0x101000) {
        return -1;
    }

    /* MSS shaping not worthwhile (4 * mss >= quantum): page-align up. */
    if (dsi_align_quantum(32768, 8960) != 32768) {
        return -1;
    }

    /* Fill search: 36864 + 36 fills poorly at mss 8960; first hit is
     * 53248 (53284 % 8960 = 8484 >= 7840). */
    if (dsi_align_quantum(33000, 8960) != 53248) {
        return -1;
    }

    /* Fill search: first hit above 32768 at mss 1448 is 49152
     * (49188 % 1448 = 1404 >= 1267). */
    if (dsi_align_quantum(32001, 1448) != 49152) {
        return -1;
    }

    /* Values above the maximum cap to it; page-up must not wrap. */
    if (dsi_align_quantum(0xFFFFFF00, 1448) != DSI_SERVQUANT_MAX) {
        return -1;
    }

    if (dsi_align_quantum(DSI_SERVQUANT_MAX, 0) != DSI_SERVQUANT_MAX) {
        return -1;
    }

    return 0;
}

/* Read-ahead product clamp: the limit always wins, floored at 1. */
static int utest_readbuf_clamp(void)
{
    if (netatalk_readbuf_clamp(32, 0x10000000, NETATALK_READBUF_LIMIT) != 4) {
        return -1;
    }

    if (netatalk_readbuf_clamp(1024, 0x10000000, UINT64_MAX) != 1024) {
        return -1;
    }

    if (netatalk_readbuf_clamp(32, 0x10000000, 0x10000000) != 1) {
        return -1;
    }

    return 0;
}

/* Buffered write payload must be handed off, not copied out.  eof - start
 * == 64 holds because the peer closes before dsi_stream_receive, so one
 * recv coalesces header and payload. */
static int utest_dsi_writeinit_no_duplicate_copy(void)
{
    DSI dsi;
    uint8_t frame[DSI_WROFF_FPWRITE + 64];
    char *wbuf = NULL;
    size_t i;
    size_t cc;
    int ret;
    memset(frame, 0, DSI_WROFF_FPWRITE);

    for (i = 0; i < 64; i++) {
        frame[DSI_WROFF_FPWRITE + i] = (uint8_t)i;
    }

    ret = dsi_test_receive(DSIFUNC_WRITE, DSI_WROFF_FPWRITE,
                           DSI_WROFF_FPWRITE + 64, frame, sizeof(frame),
                           4096, &dsi);

    if (ret != DSIFUNC_WRITE || dsi.eof - dsi.start != 64) {
        dsi_test_cleanup(&dsi);
        return -1;
    }

    memset(dsi.commands, 0xAA, 4096);
    cc = dsi_writeinit(&dsi, &wbuf);

    if (cc != 64 || dsi.datasize != 0) {
        dsi_test_cleanup(&dsi);
        return -1;
    }

    for (i = 0; i < 64; i++) {
        if (dsi.commands[i] != 0xAA) {
            dsi_test_cleanup(&dsi);
            return -1;
        }
    }

    if (wbuf == NULL || wbuf < dsi.buffer || wbuf + cc > dsi.end
            || wbuf[0] != 0 || (uint8_t)wbuf[63] != 63) {
        dsi_test_cleanup(&dsi);
        return -1;
    }

    dsi_test_cleanup(&dsi);
    return 0;
}

/* Non-write commands: dsi_len itself is bounded by the quantum. */
static int utest_dsi_receive_rejects_overquantum_cmd(void)
{
    DSI dsi;
    uint8_t payload[1] = { AFP_LOGOUT };
    int ret;
    ret = dsi_test_receive(DSIFUNC_CMD, 0, 4097, payload, 1, 4096, &dsi);
    dsi_test_cleanup(&dsi);
    return ret == 0 ? 0 : -1;
}

/* A command frame with no payload at all: the dispatcher reads commands[0] to
 * pick the AFP call, so it would take it from whatever the previous request
 * left behind. Every other length check here is an upper bound. */
static int utest_dsi_receive_rejects_zero_length_cmd(void)
{
    DSI dsi;
    uint8_t payload[1] = { AFP_LOGOUT };
    int ret;
    ret = dsi_test_receive(DSIFUNC_CMD, 0, 0, payload, 0, 4096, &dsi);
    dsi_test_cleanup(&dsi);
    return ret == 0 ? 0 : -1;
}

static void dsi_test_noop_close(DSI *dsi)
{
    (void)dsi;
}

/*!
 * @brief dsi_disconnect() on dsi, a session never logged in, then on one
 *        logged in without a logout hook, then on one without a session
 *
 * @returns 0, or the number of the check that failed
 */
static int dsi_disconnect_checks(DSI *dsi, AFPObj *sess)
{
    int stdin_open = fcntl(STDIN_FILENO, F_GETFD) != -1;
    dsi->proto_close = dsi_test_noop_close;
    dsi->AFPobj = sess;

    if (dsi_disconnect(dsi) != -1 || !(dsi->flags & DSI_DISCONNECTED)) {
        return 1;
    }

    sess->uid = 1000;
    sess->logout = NULL;
    dsi->flags = 0;

    if (dsi_disconnect(dsi) != 0 || !(dsi->flags & DSI_DISCONNECTED)) {
        return 2;
    }

    dsi->AFPobj = NULL;

    if (dsi_disconnect(dsi) != -1) {
        return 3;
    }

    /* the session had no recvfile pipe to close */
    if (stdin_open && fcntl(STDIN_FILENO, F_GETFD) == -1) {
        return 4;
    }

    return 0;
}

/* dsi_disconnect() enters the disconnected state for a session that logged
 * in and refuses one that did not. login() refuses uid 0 and sets obj->uid
 * only on success, so that is the mark; the UAM's logout hook is not, since
 * the SRP, passwd, randnum and guest UAMs register none. */
static int utest_dsi_disconnect_needs_login(void)
{
    DSI dsi;
    AFPObj sess;
    int peer;
    int ret;

    if (dsi_test_open(&dsi, DSI_DATASIZ, &peer) != 0) {
        return 5;
    }

    close(peer);
    memset(&sess, 0, sizeof(sess));
    ret = dsi_disconnect_checks(&dsi, &sess);
    dsi_test_cleanup(&dsi);
    return ret;
}

static int utest_dsi_receive_valid_cmd(void)
{
    uint8_t payload[2] = {AFP_LOGIN, 0};
    DSI dsi;
    int ret;
    ret = dsi_test_receive(DSIFUNC_CMD, 0, sizeof(payload), payload,
                           sizeof(payload), 16, &dsi);

    if (ret != DSIFUNC_CMD || dsi.cmdlen != sizeof(payload)
            || memcmp(dsi.commands, payload, sizeof(payload)) != 0) {
        dsi_test_cleanup(&dsi);
        return -1;
    }

    dsi_test_cleanup(&dsi);
    return 0;
}

static int utest_dsi_receive_rejects_oversized_payload(void)
{
    DSI dsi;
    int ret;
    ret = dsi_test_receive(DSIFUNC_CMD, 0, 17, NULL, 0, 16, &dsi);

    if (ret != 0) {
        dsi_test_cleanup(&dsi);
        return -1;
    }

    dsi_test_cleanup(&dsi);
    return 0;
}

static int utest_dsi_receive_rejects_write_offset_past_payload(void)
{
    uint8_t payload[4] = {AFP_WRITE, 0, 0, 0};
    DSI dsi;
    int ret;
    ret = dsi_test_receive(DSIFUNC_WRITE, 8, sizeof(payload), payload,
                           sizeof(payload), 16, &dsi);

    if (ret != 0) {
        dsi_test_cleanup(&dsi);
        return -1;
    }

    dsi_test_cleanup(&dsi);
    return 0;
}

int main(int argc, char *argv[])
{
    int reti;
    uint16_t vid;
    uint16_t vidsys;
    struct vol *vol;
    struct vol *volsys;
    struct dir *retdir;
    struct path *path;
    int test_report_fd;
    signal(SIGPIPE, SIG_IGN);

    /* Resolve test.conf to an absolute path before any chdir (see args[] note). */
    if (realpath("test.conf", confpath) == NULL) {
        /* fall back to the relative name; single-volume use still works */
        snprintf(confpath, sizeof(confpath), "test.conf");
    }

    if (argc == 2 && strcmp(argv[1], "--tap") == 0) {
        test_output_tap = 1;
    }

    test_report_fd = dup(STDOUT_FILENO);

    if (test_report_fd != -1) {
        test_report_stream = fdopen(test_report_fd, "w");

        if (test_report_stream == NULL) {
            close(test_report_fd);
        }
    }

    if (test_report_stream == NULL) {
        if (test_output_tap) {
            printf("Bail out! failed to initialize TAP output\n");
            fflush(stdout);
            exit(1);
        }

        test_report_stream = stdout;
    }

    if (test_output_tap) {
        setvbuf(test_report_stream, NULL, _IONBF, 0);
    }

    /* initialize */
    test_section("Initializing", "============");
    /* Log to stderr, not /dev/tty: under a non-interactive runner (CI, meson
     * test, docker run) there is no controlling terminal, so /dev/tty writes
     * fail with ENXIO and every LOG() is silently dropped.  stderr is captured. */
    TEST(setuplog("default:note", "/dev/stderr", true),
         "init logging to stderr");
    TEST_int(utest_delete_veto_symlink_stays_in_volume(), 0,
             "delete veto files unlinks directory symlinks without following them");
    TEST_int(utest_delete_veto_recursive(), 0,
             "delete veto files removes matching trees and preserves ordinary files");
    TEST_int_or_skip(utest_delete_veto_dirfd_failure(), 0,
                     "delete veto files closes its directory stream when dirfd fails");
    TEST_int(utest_delete_veto_any_case(), 0,
             "delete veto files matches in either case when asked");
    TEST_int(utest_logger_reopen_keeps_files(), 0,
             "logger: log_close_all/log_reopen preserve files and private permissions");
    TEST_int(utest_lockfile_restores_umask(), 0,
             "PID files: creation and startup errors restore the caller's umask");
    TEST_int(utest_decompose_reserves_terminator(), 0,
             "decompose_w reserves space for its UTF-16 terminator");
    TEST_int(utest_fork_range_rejects_wrapped_read(), 0,
             "fork read bounds reject signed offset-plus-count overflow");
    TEST_int(utest_ad_copy_header_valid_finderinfo(), 0,
             "ad_copy_header copies a valid FinderInfo entry");
    TEST_int(utest_ad_copy_header_bounds_finderinfo(), 0,
             "ad_copy_header rejects oversized FinderInfo without overwriting destination");
    TEST_int(utest_ad_copy_header_rejects_before_writing(), 0,
             "ad_copy_header leaves the destination untouched when an entry is rejected");
    TEST_int(utest_ad_copy_header_skips_dfork(), 0,
             "ad_copy_header does not copy data-fork entries");
    /* DSI unit tests: frame acceptance, write handoff, quantum shaping. */
    TEST_int(utest_dsi_receive_accepts_full_quantum_write(DSI_WROFF_FPWRITE), 0,
             "dsi_stream_receive accepts full-quantum FPWrite in one record");
    TEST_int(utest_dsi_receive_accepts_full_quantum_write(DSI_WROFF_FPWRITEEXT), 0,
             "dsi_stream_receive accepts full-quantum FPWriteExt in one record");
    TEST_int(utest_dsi_receive_rejects_overquantum_write(DSI_WROFF_FPWRITE), 0,
             "dsi_stream_receive rejects over-quantum FPWrite data");
    TEST_int(utest_dsi_receive_rejects_overquantum_write(DSI_WROFF_FPWRITEEXT), 0,
             "dsi_stream_receive rejects over-quantum FPWriteExt data");
    TEST_int(utest_dsi_receive_rejects_overquantum_cmd(), 0,
             "dsi_stream_receive keeps strict bound for non-write commands");
    TEST_int(utest_dsi_receive_validates_doff(), 0,
             "dsi_stream_receive validates write data offset");
    TEST_int(utest_dsi_writeinit_no_duplicate_copy(), 0,
             "dsi_writeinit hands buffered payload off without copying");
    TEST_int(utest_dsi_align_quantum(), 0,
             "dsi_align_quantum terminates within floors and page-aligns");
    TEST_int(utest_readbuf_clamp(), 0,
             "netatalk_readbuf_clamp bounds the read-ahead product");
    TEST_int(utest_dsi_receive_valid_cmd(), 0,
             "DSI receive accepts valid command frame");
    TEST_int(utest_dsi_receive_rejects_oversized_payload(), 0,
             "DSI receive rejects payload larger than server quantum");
    TEST_int(utest_dsi_receive_rejects_write_offset_past_payload(), 0,
             "DSI receive rejects DSIWrite offset beyond payload");
#ifdef WITH_SPOTLIGHT
    TEST_int(utest_spotlight_rejects_empty_long_cnids(), 0,
             "Spotlight rejects long-form empty CNID array");
    TEST_int(utest_spotlight_cnid_array_types(), 0,
             "Spotlight accepts only uint64_t query item arrays");
#endif
    TEST_int(utest_dsi_receive_rejects_zero_length_cmd(), 0,
             "DSI receive rejects a command frame with no AFP function byte");
    TEST_int(utest_dsi_disconnect_needs_login(), 0,
             "dsi_disconnect enters the disconnected state for a login whose UAM has no logout hook");
#ifndef NO_DDP
    TEST_int(utest_atp_queue_push_evicts_oldest(), 0,
             "ATP queue: full queue evicts the oldest, not the arrival");
    TEST_int(utest_atp_queue_push_keeps_newest(), 0,
             "ATP queue: the packet just received is never the victim");
#endif /* NO_DDP */
    /* Config-resolution tests: self-contained fixtures with their own
     * AFPObj + temp configs.  They MUST run before the harness's own
     * config/volume init below, which leaves process state live for the
     * lock/fork tests. */
    TEST_int_or_skip(utest_conf_parse_bool(), 0,
                     "conf_parse_bool: strict boolean spellings, invalid values warn");
    TEST_int_or_skip(utest_conf_singleuser_state_paths(), 0,
                     "single-user: signature and volume uuid files live under vol dbpath, created 0700");
    TEST_int_or_skip(utest_conf_singleuser_dbpath_volume_root(), 0,
                     "single-user: a [Global] vol dbpath at a volume's path is refused");
    TEST_int_or_skip(utest_conf_session_loads_after_login(), 0,
                     "session: a load before login builds no volume list, a "
                     "load after it does");
    TEST_int_or_skip(utest_conf_private_volume_state(), 0,
                     "private volume: the owner, sqlite and cnid search "
                     "inside it, a section option warned, a volume-section "
                     "$u refused");
    TEST_int_or_skip(utest_conf_unchanged_conf_read_once(), 0,
                     "loader: an unchanged afp.conf is read once for an empty "
                     "volume list, a changed one again");
    TEST_int_or_skip(utest_conf_open_volume_reload(), 0,
                     "loader: a held volume is rebuilt after a failed read, "
                     "and a dropped one swept once closed");
    TEST_int_or_skip(utest_conf_reload_follows_sections(), 0,
                     "loader: a reload follows the volume names and paths of "
                     "afp.conf");
    TEST_int_or_skip(utest_conf_permission_options_require_unix_priv(), 0,
                     "volume permission options are ignored when unix priv is disabled");
    TEST_int_or_skip(utest_conf_cnid_server_listen_ignored(), 0,
                     "config: cnid server and cnid listen are ignored");
    TEST_int_or_skip(utest_conf_vol_dbnest_ignored(), 0,
                     "config: vol dbnest is ignored");
    TEST_int_or_skip(utest_conf_removed_options_appletalk_parse(), 0,
                     "config: the AppleTalk parse repeats no removed-option warning");
    TEST_int_or_skip(utest_conf_ea_fallback(), 0,
                     "ea resolves volume -> preset -> [Global] -> auto-detect");
    TEST_int_or_skip(utest_conf_strict_locking_keys(), 0,
                     "strict locking key, afp read locks alias, precedence, explicitness");
    TEST_int_or_skip(utest_conf_srp_verifier_path_keys(), 0,
                     "SRP verifier path key, deprecated passwd-file alias, precedence");
    TEST_int_or_skip(utest_conf_spotlight_results_limit_keys(), 0,
                     "spotlight results limit key, sparql results limit alias, precedence");
    TEST_int_or_skip(utest_conf_multiproto_defaults(), 0,
                     "multi protocol = yes applies the coherency defaults with notes");
    TEST_int_or_skip(utest_conf_multiproto_explicit_wins(), 0,
                     "multi protocol never overrides explicit settings, warns instead");
    TEST_int_or_skip(utest_conf_no_multiproto_regression(), 0,
                     "no multi protocol: strict locking off, freq honoured");
    TEST_int_or_skip(utest_conf_stock_defaults(), 0,
                     "stock config gets the single-accessor posture (freq 100)");
    TEST_int_or_skip(utest_conf_dircache_validation_freq_range(), 0,
                     "dircache validation freq is bounded at parse, unusable values not explicit");
    TEST_int_or_skip(utest_conf_splice_size_bounds(), 0,
                     "splice size: 4096 to 8 MiB, power of two, 1 MiB default");
    TEST_int_or_skip(utest_conf_multiproto_reverts_unusable_freq(), 0,
                     "multi protocol still defaults an unusable explicit freq to 1");
    TEST_int_or_skip(utest_conf_ea_samba_no_defaults(), 0,
                     "ea = samba selects the storage format only, no coherency defaults");
    TEST_int_or_skip(utest_conf_multiproto_ea_recommendation(), 0,
                     "multi protocol volume without ea = samba logs the recommendation");
    TEST_int_or_skip(utest_conf_samba_requires_ea(), 0,
                     "ea = samba volume without filesystem EAs fails to load");
    TEST_int_or_skip(utest_conf_samba_ea_failure_keeps_vid(), 0,
                     "volume refused for missing EAs returns its volume id");
    TEST_int_or_skip(utest_conf_multiproto_defaults_not_leaked_on_failed_volume(),
                     0,
                     "failed multi protocol volume leaves process defaults untouched");
    TEST_int_or_skip(utest_conf_multiproto_reverts_compiled_defaults(), 0,
                     "multi protocol reverts performance-oriented compiled defaults");
    TEST_int_or_skip(utest_conf_load_afp_conf_vols_locked(), 0,
                     "config loader fails closed under lock contention, state-neutral");
    TEST_int(utest_cnid_wrapper_sets_errno(), 0,
             "CNID wrapper rejection sets its own errno, not a stale one");
    TEST_int(utest_cnid_error_codes_distinct(), 0,
             "CNID error codes are distinct and clear of the errno range");
    TEST_int(utest_cnid_volume_tag_identity(), 0,
             "volume tag names the CNID table, independent of v_vid");
    TEST_int(utest_cnid_valide_byteorder(), 0,
             "CNID wrapper validates returned ids in host byte order");
    TEST_int(utest_cnid_resolve_dot_names_rejected(), 0,
             "CNID wrapper refuses '.' and '..' names as corrupt");
    TEST_int(utest_cnid_dirtab_rows(), 0,
             "CNID folder table: refused rows go to the backend");
    TEST_int(utest_cnid_dirtab_collisions(), 0,
             "CNID folder table: names that collide turn it off");
    TEST_int(utest_cnid_find_name_bound(), 0,
             "CNID wrapper accepts a MAXPATHLEN name, rejects one byte more");
    TEST_int_or_skip(utest_conf_dircache_resolve_size(), 0,
                     "dircache_resolve_size: default, minimum, round-up, clamp");
    TEST_int_or_skip(utest_conf_listen_skips_failed_address(), 0,
                     "afp listen: an address that cannot start leaves the "
                     "others and no wildcard");
    {
        AFPObj singleuser_obj = { 0 };
        char *singleuser_args[] = { "afpd", "-u" };
        TEST(afp_options_parse_cmdline(&singleuser_obj, 2, singleuser_args),
             "parse afpd single-user command-line option");
        TEST_expr(reti = 0,
                  (singleuser_obj.cmdlineflags & OPTION_SINGLEUSER) != 0,
                  "afpd single-user command-line option sets its flag");
        singleuser_obj.options.flags = OPTION_SINGLEUSER;
        TEST_expr(reti = 0, singleuser_admits_uid(&singleuser_obj, getuid()),
                  "single-user login admits the uid that started the server");
        TEST_expr(reti = 0, !singleuser_admits_uid(&singleuser_obj, getuid() + 1),
                  "single-user login refuses every other uid");
        singleuser_obj.options.flags = 0;
        TEST_expr(reti = 0, singleuser_admits_uid(&singleuser_obj, getuid() + 1),
                  "a root service's login admits other uids");
    }
    TEST(afp_options_parse_cmdline(&obj, 3, &args[0]),
         "parse afpd command-line options");
    TEST_int(afp_config_parse(&obj, NULL), 0,
             "parse afp.conf into AFPObj config");
    TEST_int(configinit(&obj, &aspobj), 0,
             "initialize server config state");
    TEST(cnid_init(), "initialize CNID subsystem");
    TEST_int_or_skip(utest_cnid_sqlite_symlinks_rejected(), 0,
                     "sqlite CNID rejects main, WAL, and SHM symlink attacks");
    TEST_int_or_skip(utest_cnid_open_unknown_scheme_uses_sqlite(), 0,
                     "cnid_open: an unknown scheme opens the sqlite backend");
    TEST_int_or_skip(utest_cnid_open_scheme_case_insensitive(), 0,
                     "cnid_open: scheme names match without regard to case");
    TEST_int_or_skip(utest_cnid_open_missing_backend_refused(), 0,
                     "cnid_open: a backend this build lacks is refused");
    TEST_int(utest_cnid_scheme_registered(), 0,
             "cnid_scheme_registered: names the registered backends");
    TEST_int_or_skip(utest_cnid_sqlite_orphan_reported(), 0,
                     "cnid_open: a sqlite database left in the share root is reported");
    TEST_int_or_skip(utest_cnid_sqlite_creation_logged(), 0,
                     "cnid_open: creating a new sqlite database is logged once");
    TEST(load_afp_conf_vols(&obj, LV_ALL), "load all volumes from config");
    obj.afp_version = 34;
    /* No IPC channel or dircache hint pipe in test harness */
    obj.ipc_fd = -1;
    obj.hint_fd = -1;
    /* Set global AFPobj — normally done by afp_over_dsi() which tests bypass.
     * Before dircache_init() so the cache runs in the configured (default:
     * ARC) mode, as in production. */
    AFPobj = &obj;
    /* Both cache modes ship, so the suite runs once under each; the LRU leg
     * is the only cover for index_queue and dircache_evict() */
    const char *dcmode = getenv("AFPDTEST_DIRCACHE_MODE");

    if (dcmode != NULL && strcmp(dcmode, "lru") == 0) {
        obj.options.dircache_mode = 0;
    }

    TEST_int(dircache_init(8192), 0, "initialize dircache (8192 entries)");
    test_section(obj.options.dircache_mode ? "dircache mode: ARC"
                 : "dircache mode: LRU", "");
    TEST_int(dircache_test_ghost_trim_selection(), 0,
             "ARC ghost trim never releases the ghost being promoted");

    if (!test_output_tap) {
        fprintf(test_stream(), "\n");
    }

    /* now run tests */
    test_section("Running tests", "=============");
    TEST_expr(vid = openvol(&obj, "afpd_test"), vid != 0,
              "open the afpd_test volume");
    TEST_expr(vol = getvolbyvid(vid), vol != NULL,
              "resolve volume by volume id");
    /* Second volume on the ea=sys backend (metadata in EAs, not a ._ sidecar), so
     * the backend-specific tests can run both legs in-process — but only where the
     * filesystem actually supports user xattrs.  openvol() does not write-probe, so
     * gate on the capability (a real xattr round-trip) to skip honestly elsewhere. */
    volsys = NULL;

    if (test_capability(TEST_CAP_EA_SYS, vol)) {
        TEST_expr(vidsys = openvol(&obj, "afpd_test_sys"), vidsys != 0,
                  "open the afpd_test_sys (ea=sys) volume");
        TEST_expr(volsys = getvolbyvid(vidsys), volsys != NULL,
                  "resolve ea=sys volume by volume id");
    }

    TEST_int_or_skip(utest_cnid_add_busy_not_fatal(vol), 0,
                     "cnid_add: contended backend classifies BUSY, not session-fatal");
    TEST_int_or_skip(utest_cnid_resolve_notfound_errno(vol), 0,
                     "cnid_resolve: not-found classifies out of the syscall errno range");
    TEST_int_or_skip(utest_cnid_corrupt_row_classified(vol), 0,
                     "cnid_get/resolve: 64-bit rows classify corrupt, never truncate");
    TEST_int_or_skip(utest_cnid_find_no_truncated_id(vol), 0,
                     "cnid_find: search results never carry a truncated id");
    TEST_int_or_skip(utest_cnid_dup_row_no_truncated_delete(vol), 0,
                     "cnid_lookup: duplicate cleanup never deletes through a truncated id");
    TEST_int_or_skip(utest_cnid_uuid_case_keeps_table(vol), 0,
                     "cnid_open: a case-flipped volume UUID reuses the live table");
    TEST_int_or_skip(utest_cnid_sqlite_owner_only(vol), 0,
                     "cnid_open: a private volume's state is 0700/0600 and a "
                     "shared open widens it");
    TEST_int_or_skip(utest_cnid_desktop_folder_as_owner(&obj, vol), 0,
                     "FPOpenDT: a private volume's desktop folder is the "
                     "owner's, and root's ids come back");
    /* Last of the CNID group: recovering from the reset empties the volume's
     * table, so anything expecting a CNID minted earlier must run before it */
    TEST_int_or_skip(utest_cnid_add_depletion_resets(vol), 0,
                     "cnid_add: exhausting the CNID space reports CNID_ERR_RESET");
    /* test directory.c stuff */
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT_PARENT), retdir != NULL,
              "dirlookup: root parent directory");
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT), retdir != NULL,
              "dirlookup: root directory");
    TEST_expr(path = cname(vol, retdir, cnamewrap("Network Trash Folder")),
              path != NULL,
              "cname: resolve 'Network Trash Folder'");
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT), retdir != NULL,
              "dirlookup: root directory again");
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT_PARENT, "afpd_test"), 0,
             "getfiledirparms: volume root");
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, ""), 0,
             "getfiledirparms: root dir, empty name");
    TEST_expr(reti = createdir(&obj, vid, DIRDID_ROOT, "dir1"),
              reti == 0 || reti == AFPERR_EXIST,
              "createdir: 'dir1' (created or already exists)");
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, "dir1"), 0,
             "getfiledirparms: 'dir1'");
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, "\000\000"), 0,
             "getfiledirparms: null-name edge case");
    TEST_int(createfile(&obj, vid, DIRDID_ROOT, "dir1/file1"), 0,
             "createfile: 'dir1/file1'");
    TEST_int(delete (&obj, vid, DIRDID_ROOT, "dir1/file1"), 0,
             "delete: 'dir1/file1'");
    TEST_int(delete (&obj, vid, DIRDID_ROOT, "dir1"), 0,
             "delete: 'dir1' directory");
    TEST_int(createfile(&obj, vid, DIRDID_ROOT, "file1"), 0,
             "createfile: 'file1' at root");
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, "file1"), 0,
             "getfiledirparms: 'file1'");
    TEST_int(delete (&obj, vid, DIRDID_ROOT, "file1"), 0,
             "delete: 'file1'");
    /* test enumerate.c stuff */
    TEST_int(enumerate(&obj, vid, DIRDID_ROOT), 0,
             "enumerate: root directory contents");
    /* dircache lifetime: dir_add error path must not double-free a stray
     * entry already queued on invalid_dircache_entries (sanitizer leg
     * aborts on the second free) */
    TEST_int(test003_dir_add_error_no_double_free(vol), 0,
             "dir_add error path: queued stray entry not freed twice");
    /* getmetadata must not expunge a valid entry when the caller's stat
     * was never filled (LNAME-only FPGetForkParms shape) */
    TEST_int(test004_getmetadata_nostat_keeps_cache(vol), 0,
             "getmetadata: unfilled stat is not external-replacement evidence");
    /* an open fork outlives its path: identity comes from the held fd,
     * and the dead path must not be minted into the dircache */
    TEST_int(test005_getmetadata_open_fork_outlives_path(vol), 0,
             "getmetadata: open fork identified via held fd when path is gone");
    /* ea=sys can hold a resource fork with no metadata EA (foreign
     * writers); the negative AD cache must not mask its length */
    TEST_int_or_skip(volsys ? test006_rflen_rfork_without_metadata(volsys)
                     : TEST_SKIP, 0,
                     "getmetadata: rfork without metadata EA reports real RFLEN");
    TEST_int(test007_add_over_existing_key_leaves_one(vol), 0,
             "dircache: adding over an existing key leaves exactly one entry");
    TEST_int(test008_reindex_over_stale_key_expunges(vol), 0,
             "dircache: re-keying onto a held name retires the stale holder");
    TEST_int(test009_file_add_over_curdir_did(vol), 0,
             "dircache: a file expunging curdir hands curdir to the volume root");
    TEST_int(test010_full_cache_accepts_adds(vol, 8192), 0,
             "dircache: a full cache keeps accepting inserts");
    TEST_int(test011_stat_refresh_drops_rights(vol), 0,
             "dircache: a stat refresh with a new ctime drops cached rights");
    TEST_int(test012_movecwd_retry_names_directory(&obj, vol), 0,
             "movecwd: a failed CNID retry logs the directory it tried");
    TEST_int(test013_movecwd_refuses_symlinked_path(vol), 0,
             "movecwd: a path through a symlink to a folder is refused");
    TEST_int(test014_movecwd_folder_became_file(vol), 0,
             "movecwd: a file or a symlink to one fails with ENOTDIR");
    TEST_int(test015_movecwd_refuses_dotdot(vol), 0,
             "movecwd: a .. path component is refused");
    TEST_int(test016_cname_refuses_dot_names(&obj, vol), 0,
             "cname: a request naming . or .. is refused");
    TEST_int_or_skip(test017_cname_vetoes_any_case(&obj, vol), 0,
                     "cname: a vetoed or reserved name is refused in any case");
    TEST_int(utest_veto_file_any_case(), 0,
             "veto files: a name matches in either case only when asked");
    TEST_int(utest_veto_file_list_end(), 0,
             "veto files: the list's end closes its last entry, none past it");
    TEST_int(test018_listing_vetoes_any_case(&obj, vol), 0,
             "veto files: listings and requests fold case only when asked");
    TEST_int(utest_path_probe_case_agrees(vol), 0,
             "path_probe_case: folders are passed over, a file agrees");
    TEST_int_or_skip(utest_path_probe_case_skips_links(vol), 0,
                     "path_probe_case: a link or another file is no match");
    TEST_int(test019_cname_volume_vetoed(&obj, vol), 0,
             "cname: a veto entry for the volume's name keeps the volume");
    /* pfd cache: strict ostat equivalence (quiescent), and the
     * divergence window — probe bound + three-way sync-check no-thrash */
    TEST_int(test_pfd_ostat_equivalence(vol), 0,
             "pfd_ostat: byte-identical to ostat across the quiescent matrix");
    TEST_int(test_pfd_rename_repairs_child_path(vol), 0,
             "pfd_ostat: stale child d_fullpath repaired from re-pathed parent");
    TEST_int(test_pfd_probe_rename_detection(vol), 0,
             "pfd_ostat: rename-away caught by probe bound; no refill thrash");
    /* fork/adouble fault-injection unit tests (TEST_int_or_skip: tests that
     * cannot run on this platform/build return TEST_SKIP rather than failing) */
    TEST_int_or_skip(utest_faultinject_selftest(vol), 0,
                     "fault-injection: LD_PRELOAD interposer is active");
    TEST_int_or_skip(utest_openfork_no_fd_leak(vol), 0,
                     "afp_openfork() error path closes the leaked fork");
    TEST_int_or_skip(utest_shared_adouble_refcount_balance(vol), 0,
                     "shared adouble: ad_close() refcount balance, no early fd close");
    TEST_int_or_skip(utest_adclose_underflow_aborts(vol), 0,
                     "ad_close() hard-fails (abort) on adf_refcount underflow");
    TEST_int_or_skip(utest_ro_retry_strips_destructive_flags(vol), 0,
                     "read-only downgrade retry strips O_TRUNC (no truncate)");
    TEST_int_or_skip(utest_ad2openflags_accmode(vol), 0,
                     "ad2openflags: SETSHRMD|RDONLY promoted to O_RDWR, plain RDONLY stays RO");
    /* delete-conflict GET / of_get_locks / ad_testlock_range unit tests */
    TEST_int_or_skip(utest_testlock_range_clamp(vol), 0,
                     "ad_testlock_range: data-zone probe is clamped, never sweeps the band");
    TEST_int_or_skip(utest_testlock_whole(vol), 0,
                     "ad_testlock_whole: one unclamped F_GETLK spanning content + band");
    TEST_int_or_skip(utest_testlock_range_no_self_report(vol), 0,
                     "ad_testlock_range: F_GETLK does not report our own band entry");
    TEST_int_or_skip(utest_testlock_range_wrlck_sees_rdlck(vol), 0,
                     "ad_testlock_range: F_WRLCK probe sees a peer's F_RDLCK band entry");
    TEST_int_or_skip(utest_of_get_locks_contract(vol), 0,
                     "of_get_locks: positional band bitmap, independent df/rf, tri-valued");
    TEST_int_or_skip(utest_of_get_locks_fastpath(vol), 0,
                     "of_get_locks: >=2-dimension whole-fd fast path, per-bit on a hit");
    TEST_int_or_skip(utest_of_get_locks_failclosed(vol), 0,
                     "of_get_locks: fail-closed/NOENT/absent-rfork (v2 backend)");
    /* same fail-closed + NORF error-handling contract on the ea=sys backend */
    TEST_int_or_skip(volsys ? utest_of_get_locks_failclosed(volsys) : TEST_SKIP, 0,
                     "of_get_locks: fail-closed/NOENT/absent-rfork (ea=sys backend)");
    TEST_int_or_skip(utest_deletefile_quirk_hazard(vol), 0,
                     "POSIX close-drops-locks hazard is real here (negative control)");
    TEST_int_or_skip(utest_deletefile_quirk(vol), 0,
                     "deletefile conflict GET through a held fd preserves the held lock");
    TEST_int_or_skip(utest_deletefile_nodelete(vol), 0,
                     "deletefile honours NODELETE (v2 backend)");
    /* ea=sys leg: only where the filesystem supports user xattrs (else volsys is
     * NULL and the test skips, like any other unmet capability). */
    TEST_int_or_skip(volsys ? utest_deletefile_nodelete(volsys) : TEST_SKIP, 0,
                     "deletefile honours NODELETE (ea=sys backend)");
    /* release-path correctness: per-fork unlock, coalesced-union release, unlock-fail log */
    TEST_int_or_skip(utest_shared_rlock(vol), 0,
                     "shared read lock: kernel lock survives first release, drops on last");
    TEST_int_or_skip(utest_overlap_strand(vol), 0,
                     "overlapping read locks: union unlock leaves no stranded remainder");
    TEST_int_or_skip(utest_freelock_unlck_fail_logs(vol), 0,
                     "adf_freelock: a failed F_UNLCK is logged and the entry still dropped");
    TEST_int_or_skip(utest_fork_setmode_fdeny(vol), 0,
                     "fork_setmode_deny: each access mode maps to the correct deny bits");
    TEST_int(utest_of_alloc_fifo(vol), 0,
             "of_alloc: refnum==slot, never 0, of_find round-trip, FIFO reuse window");
    TEST_int(utest_of_breakdown(vol), 0,
             "of_breakdown: live forks counted by fork and access mode");
    TEST_int_or_skip(utest_recvfile_write_lands_intact(&obj, vol), 0,
                     "recvfile: a write lands intact, the next request parses");
    TEST_int_or_skip(utest_recvfile_dfull_drains_request(&obj, vol), 0,
                     "recvfile: a disk-full write drains its request");
    TEST_int_or_skip(utest_recvfile_error_drains_request(&obj, vol), 0,
                     "recvfile: a failed write drains its request");
    TEST_int_or_skip(utest_recvfile_error_leaves_no_residue(&obj, vol), 0,
                     "recvfile: a failed write leaves nothing for the next");
    TEST_int_or_skip(utest_recvfile_readonly_replies_access(&obj, vol), 0,
                     "recvfile: a read-only refusal replies access denied");
    TEST_int_or_skip(utest_recvfile_peer_close_ends_write(&obj, vol), 0,
                     "recvfile: a client closing mid-write ends the write");
    TEST_int_or_skip(utest_write_fork_eof_fails_request(&obj, vol), 0,
                     "write_fork: a payload cut by EOF fails the request");
    TEST_int_or_skip(utest_recvfile_counting_arm_fails_nothing(&obj, vol), 0,
                     "recvfile: a counting fault arm fails no splice() call");
    TEST_int_or_skip(utest_recvfile_urgent_byte_finishes_write(&obj, vol), 0,
                     "recvfile: a write past an urgent byte finishes in step");
    TEST_int_or_skip(utest_recvfile_urgent_byte_at_eof(&obj, vol), 0,
                     "recvfile: an urgent byte before EOF finishes the write");
    TEST_int_or_skip(utest_recvfile_symlink_refused(&obj, vol), 0,
                     "recvfile: a write to a symlink is refused and drained");
    TEST_int_or_skip(utest_recvfile_rfork_length(&obj, vol), 0,
                     "recvfile: a resource fork write extends its length");
    TEST_int_or_skip(utest_recvfile_rfork_failure_keeps_length(&obj, vol), 0,
                     "recvfile: a failed resource fork write keeps its length");
    TEST_int_or_skip(utest_recvfile_high_fd_waits(&obj, vol), 0,
                     "recvfile: a socket above FD_SETSIZE waits for its data");
    TEST_int_or_skip(utest_recvfile_urgent_byte_ahead_waits(&obj, vol), 0,
                     "recvfile: an urgent byte past a gap leaves the write "
                     "waiting for data");
    TEST_int_or_skip(utest_recvfile_unsplicable_fs_falls_back(&obj, vol), 0,
                     "recvfile: unsplicable files take the userspace path");
    TEST_int_or_skip(utest_recvfile_failed_copy_keeps_splice(&obj, vol), 0,
                     "recvfile: a failed copy leaves the file splicing");
    TEST_int_or_skip(utest_recvfile_socket_error_fails_request(&obj, vol), 0,
                     "recvfile: a socket error on the first splice fails the "
                     "write");
    TEST_int_or_skip(utest_recvfile_no_socket_splice_falls_back(&obj, vol), 0,
                     "recvfile: a refused socket splice falls back");
    TEST_int_or_skip(utest_recvfile_spent_budget_retries(&obj, vol), 0,
                     "recvfile: a spent pipe budget is tried again next write");
    TEST_int_or_skip(utest_recvfile_pipe2_failure_retries(&obj, vol), 0,
                     "recvfile: a write after a failed pipe2() splices");
    TEST_int_or_skip(utest_recvfile_unsplicable_file_not_spliced(&obj, vol), 0,
                     "recvfile: an unsplicable file is not spliced again");
    TEST_int_or_skip(utest_recvfile_pipe_takes_splice_size(&obj, vol), 0,
                     "recvfile: the pipe takes the size splice size asks for");
    TEST_int_or_skip(utest_recvfile_pipe_never_shrinks(&obj, vol), 0,
                     "recvfile: a small splice size leaves the pipe alone");
    TEST_int_or_skip(utest_recvfile_pipe_steps_down(&obj, vol), 0,
                     "recvfile: a refused pipe size is halved until granted");
    TEST_int_or_skip(utest_write_fork_short_write_fails_request(&obj, vol), 0,
                     "write_fork: a short fork write fails the request");
    TEST_int_or_skip(utest_write_fork_payload_over_count_refused(&obj, vol), 0,
                     "write_fork: a payload past its ReqCount is refused");
    TEST_int_or_skip(utest_write_fork_asp_over_count_refused(vol), 0,
                     "write_fork: AppleTalk data past its ReqCount is refused");
    TEST_int_or_skip(utest_write_fork_zero_count_replies_offset(&obj, vol), 0,
                     "write_fork: a ReqCount of 0 replies its offset");
    TEST_int_or_skip(utest_write_fork_end_past_2gib_refused(&obj, vol), 0,
                     "write_fork: a 32-bit end write past 2 GiB is refused");
    TEST_int_or_skip(utest_write_fork_dfork_past_off_t_refused(&obj, vol), 0,
                     "write_fork: a data fork write past off_t is refused");
    TEST_int_or_skip(utest_write_fork_rfork_past_off_t_refused(&obj, vol), 0,
                     "write_fork: a resource fork write past off_t is refused");
    TEST_int_or_skip(utest_write_fork_rfork_end_after_append(&obj, vol), 0,
                     "write_fork: a resource fork end write follows an append");
    TEST_int_or_skip(utest_byte_lock_rfork_end_after_append(&obj, vol), 0,
                     "FPByteRangeLock: a resource fork end lock follows an append");
    TEST_int_or_skip(utest_write_fork_short_block_refused(&obj, vol), 0,
                     "write_fork: a block too short for its ReqCount is refused");
    TEST_int_or_skip(utest_dsi_write_command_refused_in_command_frame(), 0,
                     "DSI: a write command in a DSICommand is refused");
    TEST_int_or_skip(utest_dsi_write_frame_drained(), 0,
                     "DSI: a DSIWrite no write command takes is drained");
    TEST_int_or_skip(utest_dsi_spurious_command_drains_nothing(), 0,
                     "DSI: a spurious command leaves the next request whole");
    TEST_int_or_skip(utest_dsi_write_refused_pwdexpired_drained(), 0,
                     "DSI: an expired password's refusal drains a DSIWrite");
    TEST_int_or_skip(utest_dsi_stalled_write_disconnects(vol), 0,
                     "DSI: a write whose client falls silent times out");
    TEST_int_or_skip(utest_recvfile_stalled_write_disconnects(vol), 0,
                     "recvfile: a write whose client falls silent times out");
    TEST_int_or_skip(utest_dsi_term_lets_write_finish(vol), 0,
                     "DSI: a termination lets a write still arriving finish");
    TEST_int_or_skip(utest_dsi_reconnect_takes_over_stalled_write(vol), 0,
                     "DSI: a reconnect takes over a session stalled in a write");
    TEST_int_or_skip(utest_dsi_reconnect_drops_stalled_tick(vol), 0,
                     "DSI: a reconnect drops a tick its stalled write left");
    TEST_int_or_skip(utest_dsi_reconnect_after_write_timeout(vol), 0,
                     "DSI: a reconnect takes over a session whose write timed out");
    TEST_int_or_skip(utest_dsi_reconnect_counts_ticks_afresh(), 0,
                     "DSI: a reconnect counts the new connection's silence afresh");
    TEST_int_or_skip(utest_dsi_term_served_after_stalled_header(), 0,
                     "DSI: a termination is served once a stalled header times out");
    TEST_int_or_skip(utest_dsi_slow_write_completes(vol), 0,
                     "DSI: a write with pauses shorter than the timeout completes");
    TEST_int_or_skip(utest_dsi_dead_signal_pipe_ends_session(), 0,
                     "DSI: a session whose signal pipe fails ends");
    TEST_int_or_skip(utest_delete_file_whatever_sidecar_holds(vol), 0,
                     "FPDelete: a file is deleted whatever its sidecar holds");
    TEST_int_or_skip(utest_closefork_empty_sidecar_own_folder(vol), 0,
                     "FPCloseFork: the fork's own empty sidecar is deleted");
    TEST_int_or_skip(utest_write_fork_asp_failure_unlocks(vol), 0,
                     "write_fork: a failed AppleTalk write clears its lock");
    TEST_int_or_skip(utest_sendfile_read_lands_intact(&obj, vol), 0,
                     "sendfile: a reply through a small buffer lands intact");
    TEST_int_or_skip(utest_sendfile_header_failure_sends_nothing(&obj, vol), 0,
                     "sendfile: no file data follows a failed header send");
    TEST_int_or_skip(utest_sendfile_partial_header_lands_intact(&obj, vol), 0,
                     "sendfile: a header sent in pieces lands whole");
    TEST_int(utest_dsi_close_frame_unless_mid_reply(&obj, vol), 0,
             "dsi_close: CloseSession goes out unless a reply is under way");
    TEST_int_or_skip(utest_sendfile_short_fork_ends_session(&obj, vol), 0,
                     "sendfile: a fork short of its length ends the session");
    TEST_int_or_skip(utest_buffered_short_fork_ends_session(&obj, vol), 0,
                     "FPRead: a buffered reply the fork cuts short ends the "
                     "session");
    TEST_int_or_skip(utest_sendfile_large_reply_keeps_session(&obj, vol), 0,
                     "sendfile: a 32-bit reply of 2 GiB keeps the session");
    TEST_int_or_skip(utest_sendfile_read_ext_fits_dsi_length(&obj, vol), 0,
                     "FPReadExt: an 8 GiB read gets the largest DSI reply");
    TEST_int_or_skip(utest_buffered_read_ext_fits_dsi_length(&obj, vol), 0,
                     "FPReadExt: an 8 GiB buffered read gets the largest "
                     "DSI reply");
    TEST_int_or_skip(utest_sendfile_refused_file_sent_from_buffer(&obj, vol), 0,
                     "sendfile: a file sendfile refuses goes through a buffer");
    TEST_int(utest_geticon_within_reply_buffer(vol), 0,
             "FPGetIcon: a read larger than the reply buffer is refused");
    TEST_int(utest_geticoninfo_walks_by_index(vol), 0,
             "FPGetIconInfo: each index finds its entry, in any order");
    TEST_int_or_skip(utest_icon_failed_io_replies_misc(vol), 0,
                     "FPGetIcon, FPGetIconInfo: a failed open, read or seek "
                     "answers AFPERR_MISC");
    TEST_int_or_skip(utest_addicon_failed_io_replies_misc(vol), 0,
                     "FPAddIcon: a failed icon file open, read or seek "
                     "replies AFPERR_MISC");
    TEST_int_or_skip(utest_addicon_locks_icon_file(vol), 0,
                     "FPAddIcon: a request locks the icon file it writes");
    TEST_int(utest_addicon_lock_wait_bounded(vol), 0,
             "FPAddIcon: a lock another process keeps is given up on");
    TEST_int_or_skip(utest_addicon_asp_failure_reported(vol), 0,
                     "FPAddIcon: AppleTalk reports a failed scan or open");
    TEST_int(utest_addicon_failed_request_adds_no_entry(vol), 0,
             "FPAddIcon: a request that fails adds no entry");
    TEST_int(utest_addicon_stores_icon_by_type(vol), 0,
             "FPAddIcon: an icon is stored whole and replaced by its type");
    TEST_int(utest_addicon_overwrites_torn_header(vol), 0,
             "FPAddIcon: an icon goes over a torn header, not after it");
    TEST_int(utest_addicon_overwrites_short_entry(vol), 0,
             "FPAddIcon: an icon goes over an entry short of its bits");
    TEST_int(utest_geticoninfo_null_creator_walks_file(vol), 0,
             "FPGetIconInfo: the null creator walks its icon file");
    TEST_int(utest_virtual_icon_fork_frees_adouble(&obj, vol), 0,
             "virtual icon: closing the fork frees its adouble");
    TEST_int(utest_virtual_icon_read_from_reply_buffer(&obj, vol), 0,
             "virtual icon: a read is answered from the reply buffer");
    TEST_int_or_skip(utest_sendfile_reconnect_session_ends(&obj, vol), 0,
                     "FPRead: a failed read ends a session left by a "
                     "failed reconnect");
    TEST_int_or_skip(utest_dsi_broken_reply_ends_session(&obj, vol), 0,
                     "DSI: a handler's broken reply ends the session");
    TEST_int_or_skip(utest_reconnect_sigterm_sends_attention(&obj, vol), 0,
                     "afp_dsi_die: a session left by a failed reconnect gets "
                     "the shutdown notice");
    TEST_int_or_skip(utest_copy_fork_writes_whole_or_fails(vol), 0,
                     "copy_fork: writes that stop short still copy the fork, "
                     "a write of nothing fails it");
    TEST_int(utest_rfork_cache_held_open_append_read_whole(&obj, vol), 0,
             "rfork cache: a fork grown while held open reads whole");
    TEST_int(utest_rfork_cache_stale_length_read_whole(&obj, vol), 0,
             "rfork cache: a fork longer than its recorded length reads "
             "whole");
    TEST_int(utest_buffered_read_stays_in_quantum(&obj, vol), 0,
             "FPRead: buffered reply pieces stay inside the quantum");
    TEST_int(utest_rfork_cache_read_stays_in_quantum(&obj, vol), 0,
             "rfork cache: reply pieces stay inside the quantum");
    TEST_int_or_skip(utest_buffered_short_first_piece_replies(&obj, vol), 0,
                     "FPRead: a short first buffered piece is the reply");
    TEST_int_or_skip(utest_buffered_read_error_replies_misc(&obj, vol), 0,
                     "FPRead: a failed read of the fork answers AFPERR_MISC");
    TEST_int_or_skip(utest_asp_read_unlocks_its_range(&obj, vol), 0,
                     "FPRead: AppleTalk clears the range it locked");
    TEST_int(utest_symlink_read_from_offset(&obj, vol), 0,
             "FPRead: a symlink reads from the offset asked for");
    TEST_int(utest_offcnt_stat_causes(&obj, vid), 0,
             "offspring count: each walk is attributed to one cause");
    TEST_int(utest_offcnt_hard_create_existing(&obj, vid), 0,
             "offspring count: hard create over an existing file adds nothing");
    TEST_int_or_skip(volsys
                     ? utest_offcnt_hard_create_over_symlink(&obj, vidsys)
                     : TEST_SKIP, 0,
                     "offspring count: a create over an existing symlink adds "
                     "nothing");
    TEST_int(utest_offcnt_create_over_symlink_keeps_link(&obj, vid), 0,
             "offspring count: a create over a symlink keeps the link on a "
             "sidecar volume");
    TEST_int(utest_offcnt_hard_create_through_link(&obj, vid), 0,
             "offspring count: a create through a dangling link counts the "
             "entry it adds");
    TEST_int_or_skip(utest_offcnt_access_refreshes_cache(&obj, vid), 0,
                     "offspring count: FPAccess refreshes the cached stat");
    TEST_int_or_skip(utest_access_delete_right_follows_parent(&obj, vid), 0,
                     "FPAccess: a file's delete right follows the parent's ACL");
    TEST_int(utest_resolveid_healed_folder_keeps_ids(&obj, vid), 0,
             "FPResolveID: a healed folder keeps its file IDs");
    TEST_int(utest_offcnt_params_keep_metadata(&obj, vid), 0,
             "offspring count: FPGetFileDirParms keeps the metadata it reads");
    TEST_int(utest_offcnt_fresh_stat_refreshes_cache(&obj, vid), 0,
             "offspring count: a fresh stat refreshes the cached ctime");
    TEST_int_or_skip(utest_write_reads_size_only_when_used(&obj, vol), 0,
                     "FPWrite: fork size read only when a write uses it");
    TEST_int(utest_volume_scan_counts_no_band(&obj, vol), 0,
             "vol size limit: a bundle without bands adds no usage");
    TEST_int(utest_volume_scan_resets_appended(&obj, vol), 0,
             "vol size limit: a usage scan replaces the appended count");
    TEST_int(utest_rfork_cache_hit_sends_cached_bytes(&obj, vol), 0,
             "rfork cache: hits are sent without a copy");
    TEST_int(utest_rfork_cache_stale_length_reads_fork(&obj, vol), 0,
             "rfork cache: a stale length is dropped, not served");
    TEST_int(utest_rfork_cache_finds_name_on_disk(&obj, vol), 0,
             "rfork cache: a fork is found by its name on disk");
    TEST_int(utest_rfork_cache_checks_identity(&obj, vol), 0,
             "rfork cache: an entry serves only the file it describes");
    TEST_int(utest_hint_relay_drops_repeats(&obj, vol), 0,
             "hint relay: a sender's repeated refresh is buffered once");
    TEST_int(utest_fork_first_change_hints_siblings(&obj, vol), 0,
             "fork: the first change and the close hint sibling sessions");
    TEST_int(utest_rfork_cache_drops_every_link_name(&obj, vol), 0,
             "rfork cache: a write through one hard link drops the other "
             "name's cached fork");
    TEST_int(utest_rfork_cache_skips_fill_while_written(&obj, vol), 0,
             "rfork cache: no fill while another session writes the fork");
    TEST_int_or_skip(utest_rfork_cache_skips_symlink_fill(&obj, vol), 0,
                     "rfork cache: no fill from a symlink's resource fork");
    TEST_int(utest_rfork_cache_lookup_only_inside_fork(&obj, vol), 0,
             "rfork cache: looked up only for a read inside the fork");
    TEST_int_or_skip(utest_fce_ignore_directories(&obj, vol), 0,
                     "FCE: ignore directories match at a name boundary");
    TEST_int_or_skip(utest_fce_clock_read_only_when_held(&obj, vol), 0,
                     "FCE: clock read only while a close event is held");
    TEST_int(utest_virtual_icon_write_takes_open_lock(&obj, vol), 0,
             "virtual Icon: its first write takes the open lock");
    TEST_int(utest_virtual_icon_refused_write_stays_virtual(&obj, vol), 0,
             "virtual Icon: a refused first write leaves the fork virtual");
    TEST_int(utest_virtual_icon_second_write_keeps_locks(&obj, vol), 0,
             "virtual Icon: a second fork's first write keeps the first's "
             "locks");
    TEST_int(utest_refused_open_keeps_sibling_fork(&obj, vol), 0,
             "FPOpenFork: a refused open leaves the file's open fork whole");
    TEST_int(utest_reader_close_keeps_writer(&obj, vol), 0,
             "FPCloseFork: a reader's close leaves the fork's writer working");
    TEST_int_or_skip(utest_emptied_rfork_close_spares_new_file(&obj, vol), 0,
                     "FPCloseFork: an emptied fork's close spares the file "
                     "that took its name");
    TEST_int_or_skip(utest_emptied_rfork_close_keeps_sidecar(&obj, vol), 0,
                     "FPCloseFork: an emptied fork's ._ file stays on a "
                     "read-only volume and under another open of the fork");
    TEST_int_or_skip(utest_symlink_rfork_close_keeps_cnids(&obj, vol), 0,
                     "FPCloseFork: a symlink's resource fork keeps the CNIDs");
    TEST_int(utest_emptied_icon_close_deletes_file(&obj, vol), 0,
             "fork close: an emptied Icon file goes with its AppleDouble file, "
             "CNID, entry, count and a hint, not under an open fork");
    TEST_int(utest_emptied_icon_close_keeps_data(&obj, vol), 0,
             "fork close: an emptied Icon file stays on a read-only volume, "
             "under DeleteInhibit and while its data fork holds bytes");
    TEST_int_or_skip(utest_emptied_icon_close_keeps_symlink(&obj, vol), 0,
                     "fork close: an emptied Icon symlink stays, followed or "
                     "not");
    TEST_int(utest_emptied_icon_close_spares_new_file(&obj, vol), 0,
             "fork close: an emptied Icon fork's close spares the Icon file "
             "that took its name");
    TEST_int_or_skip(utest_ea_deletefile_returns_past_stuck_ea(&obj, vol), 0,
                     "ea = ad: a failed write of a new EA leaves no entry "
                     "but for an EA file it cannot remove, and a file's EA "
                     "removal drops an entry with no EA file and returns past "
                     "one it cannot remove");
    TEST_int_or_skip(utest_set_ea_failed_close_replies_misc(&obj, vol), 0,
                     "ea = ad: a set whose header write fails replies "
                     "AFPERR_MISC");
    TEST_int(utest_virtual_icon_write_only_open_refuses_read(&obj, vol), 0,
             "virtual icon: a write-only open refuses a read");
    TEST_int(utest_virtual_icon_params_name_its_fork(&obj, vol), 0,
             "virtual icon: fork parameters name the open fork's length only "
             "and show it open");
    TEST_int_or_skip(utest_icon_create_refused_replies_access(&obj, vol), 0,
                     "virtual icon: a refused create replies AFPERR_ACCESS, "
                     "a length set's on a read-only file system "
                     "AFPERR_VLOCK, another failed one AFPERR_MISC");
    TEST_int_or_skip(utest_emptied_rfork_close_keeps_hard_link(&obj, vol), 0,
                     "fork close: the emptied ._ file's unlink spares a hard "
                     "link's");
    TEST_int(utest_emptied_icon_close_keeps_unwritten(&obj, vol), 0,
             "fork close: a new Icon file stays unless its fork changed "
             "the resource fork");
    TEST_int_or_skip(utest_emptied_icon_close_keeps_link_icon(&obj, vol), 0,
                     "fork close: an Icon file keeps its resource fork when "
                     "its adouble reads a hard link's empty one");
    TEST_int(utest_emptied_icon_close_keeps_claimed(&obj, vol), 0,
             "fork close: an emptied Icon file stays under a byte lock and "
             "DeleteInhibit, and its removal drops a count of 0");
    TEST_int_or_skip(utest_symlink_emptied_rfork_keeps_sidecar(&obj, vol), 0,
                     "fork close: a symlink's emptied ._ file stays");
    TEST_int_or_skip(utest_icon_seed_refused_stays_virtual(&obj, vol), 0,
                     "virtual Icon: a refused seed leaves the fork virtual "
                     "and no file");
    TEST_int_or_skip(utest_icon_lock_error_logged(&obj, vol), 0,
                     "virtual Icon: a failed open-mode lock is logged as an "
                     "error");
    TEST_int_or_skip(utest_icon_cleanup_spares_swapped_file(&obj, vol), 0,
                     "virtual Icon: a failed first write keeps a file another "
                     "process renamed onto the name");
    TEST_int(utest_virtual_icon_write_inhibited_stays_virtual(&obj, vol), 0,
             "virtual Icon: a write onto a write-inhibited file is refused "
             "unless the volume ignores WriteInhibit");
    TEST_int_or_skip(utest_icon_refused_borrow_keeps_length(&obj, vol), 0,
                     "virtual Icon: a refused borrowed open keeps the "
                     "lender's length");
    TEST_int(utest_delete_sweep_sends_no_refresh(&obj, vol), 0,
             "FPDelete: the swept fork's close sends no refresh");
    TEST_int_or_skip(utest_emptied_icon_close_sees_peer_read_only(&obj, vol),
                     0, "fork close: an emptied Icon file stays under another "
                     "process's read, closed through a read-only descriptor");
    TEST_int_or_skip(utest_icon_write_refuses_readonly_lender(&obj, vol), 0,
                     "virtual Icon: a first write over the session's "
                     "read-only open of the file replies AFPERR_ACCESS");
    TEST_int_or_skip(utest_icon_write_refuses_symlink(&obj, vol), 0,
                     "virtual Icon: a first write onto a symlink under the "
                     "name replies AFPERR_ACCESS");
    TEST_int(utest_emptied_icon_close_after_sibling_cut(&obj, vol), 0,
             "fork close: the last fork deletes an Icon file whose resource "
             "fork another fork cut short");
    TEST_int_or_skip(utest_rfork_cache_fills_by_own_link(&obj, vol), 0,
                     "rfork cache: a hard link's name caches its own "
                     "resource fork");
    TEST_int(utest_rfork_without_sidecar_reads_eof(&obj, vol), 0,
             "FPRead: a resource fork opened without its AppleDouble file "
             "reads EOF");
    TEST_int(utest_first_change_hint_spares_new_file(&obj, vol), 0,
             "fork hints: a first change hints no file that took the name");
    TEST_int_or_skip(utest_rfork_close_refreshes_by_own_link(&obj, vol), 0,
                     "rfork cache: a hard link's close caches no length of "
                     "another name's resource fork");
    TEST_int(utest_rename_moves_open_forks(&obj, vol), 0,
             "FPRename: the file's open forks follow it to its new name");
    TEST_int(utest_rename_keeps_link_forks_name(&obj, vol), 0,
             "FPRename: a hard link's other name keeps its forks");
    TEST_int(utest_rename_takes_stale_fork_name(&obj, vol), 0,
             "FPRename: a fork whose name another session changed follows");
    TEST_int(utest_write_counts_bytes_appended(&obj, vol), 0,
             "vol size limit: a write or a length set counts the bytes it "
             "added");
    TEST_int(utest_findnameat_follows_as_volume(&obj, vol), 0,
             "fork lookup by name: follows symlinks as the volume does");
    TEST_int(utest_refused_delete_keeps_open_lock(&obj, vol), 0,
             "FPDelete: a refused delete keeps the open's band lock");
    TEST_int_or_skip(utest_refused_delete_keeps_rfork_length(&obj, vol), 0,
                     "FPDelete: a refused delete keeps the open resource "
                     "fork's length");
    TEST_int_or_skip(utest_rename_symlink_keeps_cnids(&obj, vol), 0,
                     "FPRename: a symlink renames as a name of its own");
    TEST_int_or_skip(utest_emptied_icon_close_spares_swapped_file(&obj, vol),
                     0, "fork close: the emptied Icon file's removal spares a "
                     "file renamed onto the name");
    TEST_int_or_skip(utest_rename_keeps_symlink_fork(&obj, vol), 0,
                     "FPRename: a symlink's open fork keeps reading the link");
    TEST_int(utest_rename_keeps_open_fork_length(&obj, vol), 0,
             "rename: a refused rename leaves the open fork's length");
    TEST_int(utest_rename_onto_virtual_icon_refused(&obj, vol), 0,
             "rename: the virtual Icon file's name is taken");
    TEST_int(utest_virtual_icon_open_checks_root_access(&obj, vol), 0,
             "virtual icon: a write open needs a writable volume root, a "
             "read open no readable one");
    TEST_int_or_skip(utest_rfork_close_survives_bad_metadata(&obj, vol), 0,
                     "fork close: a malformed metadata EA does not crash");
    TEST_int_or_skip(utest_rfork_close_keeps_metadata_past_bad_ea(&obj, vol),
                     0, "fork close: a malformed metadata EA leaves the "
                     "fork's metadata in core");
    TEST_int_or_skip(utest_readonly_open_meets_deny_mode(&obj, vol), 0,
                     "FPOpenFork: a read-only descriptor sees a deny mode");
    TEST_int(utest_refused_delete_keeps_appledouble(&obj, vol), 0,
             "delete: a refused unlink keeps the AppleDouble file");
    TEST_int(utest_delete_uncounted_drops_count(&obj, vol), 0,
             "delete: a file the cached item count never held drops the count");
    TEST_int_or_skip(utest_volpoll_stamp_nanoseconds(vol), 0,
                     "stat helpers: the root's times keep their nanoseconds");
    TEST_int_or_skip(utest_volpoll_birthtime_member(), 0,
                     "config.h: the stat's creation time member is known");
    TEST_int_or_skip(utest_volpoll_spotlight_no_birthtime(), 0,
                     "spotlight: a creation date comes only with a birth time");
    TEST_int(utest_volpoll_probe_keeps_mtime(&obj, vol), 0,
             "FPGetVolParms: the read-only probe leaves the root's mtime");
    TEST_int_or_skip(utest_volpoll_probe_reports_ro(&obj, vol), 0,
                     "FPGetVolParms: EROFS is read only, EACCES is not");
    TEST_int_or_skip(utest_volpoll_probe_mount_ro(&obj, vol), 0,
                     "FPGetVolParms: the mount flag reports read only");
    TEST_int_or_skip(utest_volpoll_mark_once_a_second(&obj, vol), 0,
                     "setvoltime: the root's mtime is written once a second");
    TEST_int(utest_volpoll_root_date_kept(&obj, vol), 0,
             "FPSetDirParms: a client's date leaves the root's mark");
    TEST_int(utest_volpoll_attention_versions(&obj, vol), 0,
             "setvoltime: the free-space attention goes to AFP 2.2 to 3.1");
    TEST_int(utest_volpoll_foreign_mark_announced(&obj, vol), 0,
             "setvoltime: another session's mark reaches the next poll");
    TEST_int(utest_volpoll_announced_mark_known(&obj, vol), 0,
             "setvoltime: an announced mark is not announced again");
    TEST_int(utest_volpoll_refused_attention_kept(&obj, vol), 0,
             "pollvoltime: a refused attention goes out at the next poll");
    TEST_int_or_skip(utest_volpoll_own_mark_quiet(&obj, vol), 0,
                     "setvoltime: a session's own mark leaves its poll quiet");
    TEST_int(utest_volpoll_foreign_mark_this_second(&obj, vol), 0,
             "setvoltime: a mark earlier in the second reaches the poll");
    TEST_int_or_skip(utest_volpoll_foreign_mark_seen_second(&obj, vol), 0,
                     "pollvoltime: a stamp in the second it took is announced");
    TEST_int_or_skip(utest_volpoll_root_entry_own(&obj, vol), 0,
                     "setvoltime: root creates leave the creator's poll quiet");
    TEST_int(utest_volpoll_hard_create_compares(&obj, vol), 0,
             "setvoltime: a hard create over a root file compares the stamp");
    TEST_int(utest_volpoll_link_create_compares(&obj, vol), 0,
             "setvoltime: a create through a root link compares the stamp");
    TEST_int(utest_volpoll_link_root_create_own(&obj, vol), 0,
             "setvoltime: a root link's create into the root is the creator's");
    TEST_int(utest_volpoll_sublink_root_create_own(&obj, vol), 0,
             "setvoltime: a subfolder link's root create is the creator's");
    TEST_int_or_skip(utest_volpoll_mark_trailing_written(&obj, vol), 0,
                     "setvoltime: a trailing stamp's second gets its write");
    TEST_int_or_skip(utest_volpoll_foreign_mark_same_second(&obj, vol), 0,
                     "setvoltime: a same-second mark reaches the next poll");
    TEST_int_or_skip(utest_volpoll_quota_cached(&obj, vol), 0,
                     "getvolspace: a quota answer is reused for ten seconds");
    TEST_int_or_skip(utest_volpoll_fast_path_no_passwd(&obj), 0,
                     "load_afp_conf_vols: the fast path skips getpwuid_r");
    TEST_int_or_skip(utest_volpoll_srvrparms_mode_first(&obj, vol), 0,
                     "FPGetSrvrParms: granting mode bits skip the ACL read");
    TEST_int(utest_volpoll_name_unencodable(&obj, vol), 0,
             "FPGetVolParms: an unencodable volume name fails cleanly");
    TEST_int(utest_volpoll_srvrparms_cap(&obj), 0,
             "FPGetSrvrParms: the volume list stops at the reply cap");
    TEST_int(utest_volpoll_ea_name_not_utf8(&obj, vol), 0,
             "EA requests: a name that is not UTF-8 fails with kFPParamErr");
    TEST_int(utest_volpoll_volname_not_utf8(&obj), 0,
             "FPOpenVol: a volume name that is not UTF-8 fails");
    TEST_int(utest_volpoll_volname_from_start(&obj), 0,
             "FPOpenVol: the name converts into the start of its buffer");
    TEST_int(utest_volpoll_unwritten_unmarked(&obj, vol), 0,
             "FPOpenFork: an unwritten open and close mark nothing");
    TEST_int(utest_volpoll_mark_setforkparams(&obj, vol), 0,
             "FPSetForkParms: a length change marks the volume");
    TEST_int(utest_volpoll_mark_exchangefiles(&obj, vol), 0,
             "FPExchangeFiles: an exchange marks the volume");
    TEST_int_or_skip(volsys ? utest_volpoll_mark_setextattr(&obj, volsys)
                     : TEST_SKIP, 0,
                     "FPSetExtAttr: setting an EA marks the volume");
    TEST_int_or_skip(volsys ? utest_volpoll_mark_remextattr(&obj, volsys)
                     : TEST_SKIP, 0,
                     "FPRemoveExtAttr: removing an EA marks the volume");
    TEST_int(utest_volpoll_mark_setvolparms(&obj, vol), 0,
             "FPSetVolParms: setting the backup date marks the volume");
    TEST_int_or_skip(utest_volpoll_setvolparms_unwritten(&obj, vol), 0,
                     "FPSetVolParms: a backup date not written fails");
    TEST_int_or_skip(utest_volpoll_mark_setacl(&obj, vol), 0,
                     "FPSetACL: removing an ACL marks the volume");
    TEST_int_or_skip(utest_volpoll_setacl_group_uuid(&obj, vol), 0,
                     "FPSetACL: the ACL follows a lone group or owner UUID");
    TEST_int(utest_volpoll_readonly_refused(&obj, vol), 0,
             "read only: EA and ACL changes fail with kFPVolLocked");
    TEST_int_or_skip(utest_write_fork_marks_volume(&obj, vol), 0,
                     "FPWrite: a payload that lands marks the volume");
    TEST_int_or_skip(utest_closefork_rsrc_marks_volume(&obj, vol), 0,
                     "FPCloseFork: a written rfork's close marks the volume");
    TEST_int_or_skip(utest_flush_rsrc_marks_volume(&obj, vol), 0,
                     "FPFlush: a written rfork's flush marks the volume");
    TEST_int_or_skip(utest_flush_rsrc_failed_kept(&obj, vol), 0,
                     "FPFlush: an rfork whose header write failed stays dirty");
    TEST_int_or_skip(utest_closevol_rsrc_marks_volume(&obj, vol), 0,
                     "closevol: a written rfork's sweep writes quietly");
    TEST_int_or_skip(utest_logout_rsrc_quiet(&obj, vol), 0,
                     "FPLogout: a written rfork's sweep sends no attention");
    TEST_int_or_skip(utest_dsi_due_mark_written(vol), 0,
                     "DSI: a write within a marked second reaches the root");
    /* closes vol, fails an open of it, then hands it back open */
    TEST_int(utest_volpoll_openvol_failed_keeps_curdir(&obj, vol), 0,
             "FPOpenVol: a failed open leaves the current directory");
    TEST_int_or_skip(utest_volpoll_quota_unfetched(&obj, vol), 0,
                     "getvolspace: a volume with no quota answer fetches one");
    TEST_int(utest_volpoll_write_attention_quiet(&obj, vol), 0,
             "setvoltime: a fork's change sends no free-space attention");
    TEST_int_or_skip(utest_volpoll_setvolparms_flush_error(&obj, vol), 0,
                     "FPSetVolParms: a failed flush answers its own error");
    TEST_int(utest_volpoll_exchange_failure_cached(&obj, vol), 0,
             "FPExchangeFiles: an early failure keeps the folder cached");
    TEST_int(utest_volpoll_name_global_charset(&obj, vol), 0,
             "FPGetVolParms: the Mac name converts in the global charset");
    TEST_int_or_skip(volsys ? utest_volpoll_ea_name_field(&obj, volsys)
                     : TEST_SKIP, 0,
                     "EA requests: a long, short or NUL-holding name fails");
    TEST_int_or_skip(volsys ? utest_volpoll_ea_value_bound(&obj, volsys)
                     : TEST_SKIP, 0,
                     "FPSetExtAttr: a value the request lacks fails");
    TEST_int_or_skip(utest_volpoll_ea_none_refused(&obj, vol), 0,
                     "EA requests: a volume without EA support refuses them");
    TEST_int_or_skip(volsys ? utest_volpoll_ea_ad_long_listed(&obj, volsys)
                     : TEST_SKIP, 0,
                     "FPListExtAttrs: a 255-byte name lists from EA files");
    TEST_int_or_skip(volsys ? utest_volpoll_ea_name_listed(&obj, volsys)
                     : TEST_SKIP, 0,
                     "EA requests: a name lists as it was set");
    TEST_int(utest_volpoll_volname_nul(&obj), 0,
             "FPOpenVol: a name that holds a NUL byte fails");
    TEST_int(utest_volpoll_setdirparms_partial_marks(&obj, vol), 0,
             "FPSetDirParms: a partial change marks for the root's date");
    TEST_int_or_skip(utest_volpoll_attention_quota_fresh(&obj, vol), 0,
                     "setvoltime: an attention drops a limited quota answer");
    TEST_int_or_skip(utest_volpoll_attention_sibling_quota(&obj, vol), 0,
                     "setvoltime: an attention drops a sibling's quota answer");
    TEST_int(utest_enumerate_rereads_counted(&obj, vid), 0,
             "enumerate: listing reads and continuation rereads are counted");
    TEST_int_or_skip(utest_enumerate_conversion_attempts(&obj, vol), 0,
                     "enumerate: conversion is attempted only where needed");
    TEST_int_or_skip(utest_enumerate_dehex_keeps_cnid(&obj, vol), 0,
                     "enumerate: a dehexed name keeps its CNID");
    TEST_int_or_skip(utest_enumerate_folder_rename_retried(&obj, vol), 0,
                     "enumerate: a folder's failed rename is tried again");
    TEST_int_or_skip(utest_enumerate_unrenamed_keeps_cnid(&obj, vol), 0,
                     "enumerate: a name dehexing cannot take keeps its CNID");
    TEST_int(utest_enumerate_failed_batch_keeps_index(&obj, vid), 0,
             "enumerate: a failed batch leaves the next at its index");
    TEST_int(utest_enumerate_failed_read_rereads(&obj, vol), 0,
             "enumerate: a listing whose read failed is read again");
    TEST_int_or_skip(utest_enumerate_read_error_fails(&obj, vol), 0,
                     "enumerate: a read that fails part-way fails");
    TEST_int(utest_enumerate_interleaved_reads_once(&obj, vid), 0,
             "enumerate: interleaved listings read directories once");
    TEST_int(utest_enumerate_one_kind_keeps_index(&obj, vid), 0,
             "enumerate: a one-kind listing read again keeps its index");
    TEST_int(utest_enumerate_kind_change_rewinds(&obj, vid), 0,
             "enumerate: a batch of the other kind rewinds");
    TEST_int(utest_enumerate_deleted_name_keeps_index(&obj, vid), 0,
             "enumerate: a deleted name keeps its place in the numbering");
    TEST_int(utest_enumerate_failed_stat_leaves_index(&obj, vid), 0,
             "enumerate: a name whose stat failed leaves the numbering");
    TEST_int_or_skip(utest_enumerate_dehex_vetoed_leaves(&obj, vol), 0,
                     "enumerate: a name decoded to a vetoed one leaves");
    TEST_int_or_skip(utest_enumerate_dehex_gone_leaves(&obj, vol), 0,
                     "enumerate: a decoded name that fails its stat leaves");
    TEST_int_or_skip(utest_enumerate_icon_ends_files(&obj, vol), 0,
                     "enumerate: the virtual Icon ends a files-only listing");
    TEST_int(utest_enumerate_cnid_one_query(&obj, vol), 0,
             "enumerate: one folder query answers a listing's CNID lookups");
    TEST_int(utest_enumerate_cnid_other_inode(&obj, vol), 0,
             "enumerate: a row holding another inode asks the backend");
    TEST_int(utest_enumerate_cnid_changed_file(&obj, vol), 0,
             "enumerate: a file changed since the load asks the backend");
    TEST_int_or_skip(utest_enumerate_cnid_other_metadata(&obj, vol), 0,
                     "enumerate: a differing metadata CNID asks the backend");
    TEST_int(utest_enumerate_cnid_write_unloads(&obj, vol), 0,
             "enumerate: a CNID write turns the folder tables off");
    TEST_int(utest_enumerate_cnid_outside_listing(&obj, vol), 0,
             "enumerate: a lookup outside the listing asks the backend");
    TEST_int(utest_enumerate_cnid_reread_unarmed(&obj, vol), 0,
             "enumerate: a listing read again past its slot loads no table");
    TEST_int_or_skip(utest_movecwd_skips_getcwd(&obj, vol), 0,
                     "movecwd: a walking volume skips getcwd()");
    TEST_int_or_skip(utest_movecwd_at_descriptor_limit(&obj, vol), 0,
                     "movecwd: the walk falls back at the descriptor limit");
    TEST_int_or_skip(utest_movecwd_volume_walks(vol), 0,
                     "movecwd: macOS, illumos and Solaris hold the root");
    TEST_int_or_skip(utest_movecwd_rechecks_root(&obj, vol), 0,
                     "movecwd: an unsearchable volume root refuses below it");
    TEST_int_or_skip(utest_movecwd_walk_ignores_case(&obj, vol), 0,
                     "movecwd: the walk enters a folder whose case changed");
    TEST_int_or_skip(utest_movecwd_stale_root(&obj, vol), 0,
                     "movecwd: a stale held root is entered by its path");
    TEST_int_or_skip(utest_enumerate_long_name_skipped(&obj, vid), 0,
                     "enumerate: names over 255 bytes are left out");
    /* drives a full closevol/openvol cycle on vol and hands it back open */
    TEST_int(test_pfd_vol_close_purges_slots(&obj, vol), 0,
             "closevol: pfd slots keyed on the volume are retired");
    /* cleanup — vol may be closed if the cycle's reopen failed (closevol
     * is not idempotent: dir_free(v_root) has no NULL guard) */
    curdir = NULL;

    if (vol->v_flags & AFPVOL_OPEN) {
        closevol(&obj, vol);
    }

    if (volsys) {
        closevol(&obj, volsys);
    }

    unload_volumes(&obj);
    test_plan(test_case_num);
}
