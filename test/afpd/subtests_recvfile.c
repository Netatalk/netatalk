/*
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
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/dsi.h>
#include <atalk/globals.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afpfunc_helpers.h"
#include "directory.h"
#include "faultinject.h"
#include "fork.h"
#include "subtests_recvfile.h"
#include "test.h"
#include "test_capabilities.h"

#define RF_CMD   (DSI_BLOCKSIZ + 2)
#define RF_A_LEN (1024 * 1024)
#define RF_B_LEN (256 * 1024)
/* fits a loopback window with nobody reading */
#define RF_C_LEN (12 * 1024)
/* a countdown no write reaches: it counts calls and injects nothing */
#define RF_NEVER 1000

/*! A DSI session on one end of a socket pair and a writer on the other */
struct rf {
    AFPObj obj;
    struct vol *vol;
    DSI dsi;
    int peer;
    pthread_t writer;
    bool writing;
    uint8_t *buf;
    const uint8_t *out;
    size_t outlen;
    size_t oob_at;
    uint16_t refnum[2];
    char name[2][8];
};

/*!
 * @brief Pack an FPWriteExt for len bytes of fill at offset into p
 *
 * @returns the frame length
 */
static size_t rf_write_frame(uint8_t *p, uint16_t reqid, uint16_t refnum,
                             uint64_t offset, uint32_t len, uint8_t fill)
{
    uint64_t offset_be = hton64(offset);
    uint64_t len_be = hton64((uint64_t)len);
    dsi_test_header(p, DSIFUNC_WRITE, reqid, DSI_WROFF_FPWRITEEXT,
                    DSI_WROFF_FPWRITEEXT + len);
    p += DSI_BLOCKSIZ;
    p[0] = AFP_WRITE_EXT;
    p[1] = 0;
    /* refnum travels raw, as afp_openfork() returns it */
    memcpy(p + 2, &refnum, sizeof(refnum));
    memcpy(p + 4, &offset_be, sizeof(offset_be));
    memcpy(p + 12, &len_be, sizeof(len_be));
    memset(p + DSI_WROFF_FPWRITEEXT, fill, len);
    return DSI_FRAME_OVERHEAD_EXT + len;
}

/*!
 * @brief Pack a DSICommand carrying FPGetSrvrParms into p
 *
 * @returns the frame length
 */
static size_t rf_cmd_frame(uint8_t *p, uint16_t reqid)
{
    dsi_test_header(p, DSIFUNC_CMD, reqid, 0, 2);
    p[DSI_BLOCKSIZ] = AFP_GETSRVPARAM;
    p[DSI_BLOCKSIZ + 1] = 0;
    return RF_CMD;
}

/*!
 * @brief Writer thread: send out to the peer, one urgent byte at oob_at when
 *        set, then end the stream
 */
static void *rf_writer(void *arg)
{
    struct rf *rf = arg;
    size_t off = 0;

    while (off < rf->outlen) {
        size_t len = rf->outlen - off;
        ssize_t n;

        if (rf->oob_at > off) {
            len = rf->oob_at - off;
        } else if (rf->oob_at > 0 && rf->oob_at == off) {
            /* outside the payload the header announced: recv() skips it */
            rf->oob_at = 0;

            if (send(rf->peer, "!", 1, MSG_OOB) != 1) {
                break;
            }
        }

        n = write(rf->peer, rf->out + off, len);

        if (n == -1 && errno == EINTR) {
            continue;
        }

        if (n <= 0) {
            break;
        }

        off += (size_t)n;
    }

    shutdown(rf->peer, SHUT_WR);
    return NULL;
}

/*! @brief Close the forks, delete their files and release the session */
static void rf_teardown(struct rf *rf)
{
    fault_inject_reset();
    /* a writer blocked in write() fails with EPIPE once its reader is gone */
    close(rf->dsi.socket);
    rf->dsi.socket = -1;

    if (rf->writing) {
        pthread_join(rf->writer, NULL);
    }

    close(rf->peer);

    for (int i = 0; i < 2; i++) {
        if (rf->refnum[i] != 0) {
            closefork(&rf->obj, rf->refnum[i]);
        }

        if (rf->name[i][0] != '\0') {
            delete (&rf->obj, rf->vol->v_vid, DIRDID_ROOT, rf->name[i]);
        }
    }

    dir_free_invalid_q();
    free(rf->buf);
    dsi_test_cleanup(&rf->dsi);
}

/*!
 * @brief Serve a copy of obj over a socketpair with recvfile on and the
 *        default splice size, as afp_over_dsi() serves a session
 *
 * @returns 0, or -1
 */
static int rf_setup(struct rf *rf, AFPObj *obj, struct vol *vol)
{
    memset(rf, 0, sizeof(*rf));
    fault_inject_reset();

    if (dsi_test_open(&rf->dsi, RF_A_LEN, &rf->peer) != 0) {
        return -1;
    }

    rf->vol = vol;
    rf->obj = *obj;
    rf->obj.dsi = &rf->dsi;
    rf->obj.proto = AFPPROTO_DSI;
    rf->obj.options.flags |= OPTION_RECVFILE;
    rf->buf = malloc(2 * DSI_FRAME_OVERHEAD_EXT + RF_A_LEN + RF_B_LEN);

    if (rf->buf == NULL || setnonblock(rf->dsi.socket, 1) != 0) {
        rf_teardown(rf);
        return -1;
    }

    return 0;
}

/*!
 * @brief rf_setup() for a test that splices
 *
 * @returns 0, -1, or TEST_SKIP where the platform cannot splice from a socket
 *          or the fault interposer is inactive
 */
static int rf_splice_setup(struct rf *rf, AFPObj *obj, struct vol *vol)
{
    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)
            || !test_capability(TEST_CAP_SOCKET_SPLICE, vol)) {
        return TEST_SKIP;
    }

    return rf_setup(rf, obj, vol);
}

/*!
 * @brief rf_splice_setup() over a loopback TCP connection
 *
 * @returns 0, -1, or TEST_SKIP, also where no loopback is available (a
 *          sandbox without a network namespace of its own)
 */
static int rf_tcp_setup(struct rf *rf, AFPObj *obj, struct vol *vol)
{
    int ret;

    if ((ret = rf_splice_setup(rf, obj, vol)) != 0) {
        return ret;
    }

    if (dsi_test_connect_tcp(&rf->dsi, &rf->peer) != 0) {
        rf_teardown(rf);
        return TEST_SKIP;
    }

    return 0;
}

/*! @brief Capacity of the pipe that fd writes to, or -1 */
static int rf_pipe_size(int fd _U_)
{
#ifdef F_GETPIPE_SZ
    return fcntl(fd, F_GETPIPE_SZ);
#else
    return -1;
#endif
}

/*!
 * @brief Capacity of a new pipe grown to size, or as made for size 0
 *
 * @returns the capacity, or -1 when the kernel refuses size
 */
static int rf_pipe_capacity(int size _U_)
{
    int got = -1;
#ifdef F_SETPIPE_SZ
    int p[2];

    if (pipe(p) == 0) {
        if (size == 0 || fcntl(p[1], F_SETPIPE_SZ, size) != -1) {
            got = rf_pipe_size(p[1]);
        }

        close(p[0]);
        close(p[1]);
    }

#endif
    return got;
}

/*! @brief Start the writer on len bytes at p; it ends the stream after them */
static int rf_send(struct rf *rf, const uint8_t *p, size_t len)
{
    rf->out = p;
    rf->outlen = len;
    rf->writing = pthread_create(&rf->writer, NULL, rf_writer, rf) == 0;
    return rf->writing ? 0 : -1;
}

/*!
 * @brief Send the request head alone and receive it before the writer starts
 *        on the rest, so the payload arrives after the header read
 *
 * @returns 0, or -1
 */
static int rf_send_after_head(struct rf *rf, size_t len)
{
    if (write(rf->peer, rf->buf, DSI_FRAME_OVERHEAD_EXT)
            != DSI_FRAME_OVERHEAD_EXT
            || dsi_stream_receive(&rf->dsi) != DSIFUNC_WRITE) {
        return -1;
    }

    return rf_send(rf, rf->buf + DSI_FRAME_OVERHEAD_EXT,
                   len - DSI_FRAME_OVERHEAD_EXT);
}

/*! @brief Run the received DSIWrite as afp_over_dsi() does */
static int rf_dispatch(struct rf *rf)
{
    size_t rbuflen = DSI_DATASIZ;
    return afp_write_ext(&rf->obj, (char *)rf->dsi.commands, rf->dsi.cmdlen,
                         (char *)rf->dsi.data, &rbuflen);
}

/*!
 * @brief Receive the next request, which must be a DSIWrite, and run it
 *
 * @returns the AFP result, or -1 when no DSIWrite arrives
 */
static int rf_write(struct rf *rf)
{
    if (dsi_stream_receive(&rf->dsi) != DSIFUNC_WRITE) {
        return -1;
    }

    return rf_dispatch(rf);
}

/*! @brief Does the next request parse as the DSICommand with reqid? */
static bool rf_next_is(struct rf *rf, uint16_t reqid)
{
    return dsi_stream_receive(&rf->dsi) == DSIFUNC_CMD
           && rf->dsi.clientID == reqid;
}

/*!
 * @brief FPCreateFile rf_i, then FPOpenFork of its fork read-write into
 *        slot i
 *
 * @returns true on success
 */
static bool rf_open(struct rf *rf, int i, uint8_t fork)
{
    snprintf(rf->name[i], sizeof(rf->name[i]), "rf_%d", i);

    if (createfile(&rf->obj, rf->vol->v_vid, DIRDID_ROOT, rf->name[i])
            != AFP_OK) {
        return false;
    }

    return openfork(&rf->obj, rf->vol->v_vid, DIRDID_ROOT, rf->name[i], fork,
                    OPENACC_RD | OPENACC_WR, &rf->refnum[i]) == AFP_OK;
}

/*! @brief Does fork slot i hold exactly len bytes of fill? */
static bool rf_holds(struct rf *rf, int i, size_t len, uint8_t fill)
{
    static char buf[65536];
    const struct ofork *of = of_find(rf->refnum[i]);
    size_t off = 0;
    int eid;

    if (of == NULL) {
        return false;
    }

    eid = (of->of_flags & AFPFORK_DATA) ? ADEID_DFORK : ADEID_RFORK;

    if (ad_size(of->of_ad, eid) != (off_t)len) {
        return false;
    }

    while (off < len) {
        ssize_t n = ad_read(of->of_ad, eid, (off_t)off, buf,
                            MIN(sizeof(buf), len - off));

        if (n <= 0) {
            return false;
        }

        for (ssize_t j = 0; j < n; j++) {
            if ((uint8_t)buf[j] != fill) {
                return false;
            }
        }

        off += (size_t)n;
    }

    return true;
}

/* the splice() side a fault is armed on */
enum rf_side { RF_SOCK, RF_FILE };

/*!
 * @brief Arm splice() call fail_after + 1 on side with errnum; errnum 0
 *        arms a countdown no write reaches, which counts the calls
 */
static void rf_arm(enum rf_side side, int fail_after, int errnum)
{
    int after = errnum != 0 ? fail_after : RF_NEVER;
    int err = errnum != 0 ? errnum : EIO;

    if (side == RF_SOCK) {
        fi.splice_sock_armed = 1;
        fi.splice_sock_fail_after = after;
        fi.splice_sock_errno = err;
    } else {
        fi.splice_file_armed = 1;
        fi.splice_file_fail_after = after;
        fi.splice_file_errno = err;
    }
}

/*!
 * @brief Did the fault rf_arm() armed fire or, for errnum 0, did its
 *        countdown count calls without firing?
 */
static bool rf_fired(enum rf_side side, int errnum)
{
    int left = side == RF_SOCK ? fi.splice_sock_fail_after
               : fi.splice_file_fail_after;
    return errnum != 0 ? left == -1 : left < RF_NEVER && left != -1;
}

/*!
 * @brief 1 MiB of 0xA5 to a new fork, then a DSICommand or, with second,
 *        256 KiB of 0x5A to a new data fork; splice() call fail_after + 1 on
 *        side fails with errnum unless errnum is 0
 *
 * @returns 0 when the first write returns expect, the armed fault fired or,
 *          unarmed, splice() ran, the next request is served, and each
 *          AFP_OK write left its data in its fork; -1 when not; TEST_SKIP
 */
static int rf_write_test(AFPObj *obj, struct vol *vol, uint8_t fork,
                         enum rf_side side, int fail_after, int errnum,
                         int expect, bool second)
{
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, fork)
            && (!second || rf_open(&rf, 1, OPENFORK_DATA))) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_A_LEN, 0xA5);

        if (second) {
            len += rf_write_frame(rf.buf + len, 2, rf.refnum[1], 0, RF_B_LEN,
                                  0x5A);
        } else {
            len += rf_cmd_frame(rf.buf + len, 2);
        }

        rf_arm(side, fail_after, errnum);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == expect
                && rf_fired(side, errnum)
                && (expect != AFP_OK || rf_holds(&rf, 0, RF_A_LEN, 0xA5))) {
            if (!second) {
                ret = rf_next_is(&rf, 2) ? 0 : -1;
            } else {
                fault_inject_reset();
                rf_arm(RF_FILE, 0, 0);

                if (rf_write(&rf) == AFP_OK && rf_fired(RF_FILE, 0)
                        && rf_holds(&rf, 1, RF_B_LEN, 0x5A)) {
                    ret = 0;
                }
            }
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*! @brief A recvfile write lands intact and the next request parses */
int utest_recvfile_write_lands_intact(AFPObj *obj, struct vol *vol)
{
    return rf_write_test(obj, vol, OPENFORK_DATA, RF_FILE, 0, 0, AFP_OK,
                         false);
}

/*! @brief A disk-full recvfile write drains its request */
int utest_recvfile_dfull_drains_request(AFPObj *obj, struct vol *vol)
{
    return rf_write_test(obj, vol, OPENFORK_DATA, RF_FILE, 1, ENOSPC,
                         AFPERR_DFULL, false);
}

/*! @brief A recvfile write that fails otherwise drains its request */
int utest_recvfile_error_drains_request(AFPObj *obj, struct vol *vol)
{
    return rf_write_test(obj, vol, OPENFORK_DATA, RF_FILE, 1, EIO,
                         AFPERR_PARAM, false);
}

/*! @brief A failed recvfile write leaves nothing behind for the next one */
int utest_recvfile_error_leaves_no_residue(AFPObj *obj, struct vol *vol)
{
    return rf_write_test(obj, vol, OPENFORK_DATA, RF_FILE, 1, EIO,
                         AFPERR_PARAM, true);
}

/*!
 * @brief A client that disconnects mid-payload ends the write
 *
 * The header announces 1 MiB and 256 KiB arrive before EOF; a socket-side
 * countdown no finite receive reaches bounds the splice() calls at EOF.
 */
int utest_recvfile_peer_close_ends_write(AFPObj *obj, struct vol *vol)
{
    struct rf rf;
    int ret;

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_A_LEN, 0xA5);
        rf_arm(RF_SOCK, 0, 0);

        if (rf_send(&rf, rf.buf, DSI_FRAME_OVERHEAD_EXT + RF_B_LEN) == 0
                && rf_write(&rf) == AFPERR_PARAM && rf_fired(RF_SOCK, 0)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief A payload the stream ends before completing fails the request on
 *        the userspace path as well
 *
 * The header announces 1 MiB and 256 KiB arrive before EOF.
 */
int utest_write_fork_eof_fails_request(AFPObj *obj, struct vol *vol)
{
    struct rf rf;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;
    rf.obj.options.flags &= ~OPTION_RECVFILE;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_A_LEN, 0xA5);

        if (rf_send(&rf, rf.buf, DSI_FRAME_OVERHEAD_EXT + RF_B_LEN) == 0
                && rf_write(&rf) == AFPERR_PARAM) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief A write with a TCP urgent byte inside its payload finishes through
 *        recv(), which steps over the byte, and the session stays in step
 */
int utest_recvfile_urgent_byte_finishes_write(AFPObj *obj, struct vol *vol)
{
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_tcp_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_A_LEN, 0xA5);
        len += rf_cmd_frame(rf.buf + len, 2);
        rf.oob_at = DSI_FRAME_OVERHEAD_EXT + RF_B_LEN;
        rf_arm(RF_SOCK, 0, 0);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK
                && rf_fired(RF_SOCK, 0) && rf_holds(&rf, 0, RF_A_LEN, 0xA5)
                && rf_next_is(&rf, 2)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief A write whose urgent byte and FIN are in before the receive
 *        finishes through recv() as well
 */
int utest_recvfile_urgent_byte_at_eof(AFPObj *obj, struct vol *vol)
{
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_tcp_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_C_LEN, 0xA5);
        len += rf_cmd_frame(rf.buf + len, 2);
        rf.oob_at = DSI_FRAME_OVERHEAD_EXT + RF_C_LEN / 2;
        rf.out = rf.buf;
        rf.outlen = len;
        rf_writer(&rf);
        rf_arm(RF_SOCK, 0, 0);

        if (rf_write(&rf) == AFP_OK && rf_fired(RF_SOCK, 0)
                && rf_holds(&rf, 0, RF_C_LEN, 0xA5) && rf_next_is(&rf, 2)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief A recvfile write to a symlink is refused through the userspace
 *        path, which drains the request
 */
int utest_recvfile_symlink_refused(AFPObj *obj, struct vol *vol)
{
    char path[MAXPATHLEN + 1];
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;
    snprintf(rf.name[0], sizeof(rf.name[0]), "rf_0");
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, rf.name[0]);

    if (symlink("rf_target", path) == 0
            && openfork(&rf.obj, vol->v_vid, DIRDID_ROOT, rf.name[0],
                        OPENFORK_DATA, OPENACC_RD | OPENACC_WR,
                        &rf.refnum[0]) == AFP_OK
            && ad_data_fileno(of_find(rf.refnum[0])->of_ad) == AD_SYMLINK) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_B_LEN, 0x5A);
        len += rf_cmd_frame(rf.buf + len, 2);

        if (rf_send_after_head(&rf, len) == 0
                && rf_dispatch(&rf) == AFPERR_ACCESS && rf_next_is(&rf, 2)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*! @brief A recvfile write extends the resource fork length */
int utest_recvfile_rfork_length(AFPObj *obj, struct vol *vol)
{
    return rf_write_test(obj, vol, OPENFORK_RSCS, RF_FILE, 0, 0, AFP_OK,
                         false);
}

/*!
 * @brief A failed resource fork write sets the fork length to the bytes that
 *        landed, and marks the fork modified when some did
 */
int utest_recvfile_rfork_failure_keeps_length(AFPObj *obj, struct vol *vol)
{
    const struct ofork *of;
    struct stat st;
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_RSCS)
            && (of = of_find(rf.refnum[0])) != NULL) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], RF_A_LEN, RF_B_LEN,
                             0x5A);
        len += rf_write_frame(rf.buf + len, 2, rf.refnum[0], 0, RF_A_LEN,
                              0xA5);
        rf_arm(RF_FILE, 0, ENOSPC);

        if (rf_send_after_head(&rf, len) == 0
                && rf_dispatch(&rf) == AFPERR_DFULL
                && rf_fired(RF_FILE, ENOSPC)
                && ad_size(of->of_ad, ADEID_RFORK) == 0
                && !(of->of_flags & AFPFORK_MODIFIED)) {
            fault_inject_reset();
            rf_arm(RF_FILE, 1, ENOSPC);

            if (rf_write(&rf) == AFPERR_DFULL && rf_fired(RF_FILE, ENOSPC)
                    && (of->of_flags & AFPFORK_MODIFIED)
                    && fstat(ad_reso_fileno(of->of_ad), &st) == 0
                    && ad_size(of->of_ad, ADEID_RFORK)
                    == st.st_size - ADEDOFF_RFORK_OSX) {
                ret = 0;
            }
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief A recvfile write on a socket above FD_SETSIZE waits for its data
 */
int utest_recvfile_high_fd_waits(AFPObj *obj, struct vol *vol)
{
    struct rlimit saved;
    struct rlimit raised;
    struct rf rf;
    size_t len;
    int hi;
    int ret;

    if (getrlimit(RLIMIT_NOFILE, &saved) != 0) {
        return -1;
    }

    if (saved.rlim_max < FD_SETSIZE + 16) {
        return TEST_SKIP;
    }

    raised = saved;

    if (raised.rlim_cur < FD_SETSIZE + 16) {
        raised.rlim_cur = FD_SETSIZE + 16;
    }

    if (setrlimit(RLIMIT_NOFILE, &raised) != 0) {
        return -1;
    }

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        setrlimit(RLIMIT_NOFILE, &saved);
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)
            && (hi = fcntl(rf.dsi.socket, F_DUPFD, FD_SETSIZE + 8)) != -1) {
        close(rf.dsi.socket);
        rf.dsi.socket = hi;
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_A_LEN, 0xA5);
        rf_arm(RF_SOCK, 0, EAGAIN);

        /* the head is buffered before the receive, so readt() never waits */
        if (rf_send_after_head(&rf, len) == 0 && rf_dispatch(&rf) == AFP_OK
                && rf_fired(RF_SOCK, EAGAIN)
                && rf_holds(&rf, 0, RF_A_LEN, 0xA5)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    setrlimit(RLIMIT_NOFILE, &saved);
    return ret;
}

/*!
 * @brief A file that cannot take splice() is finished through the userspace
 *        path, and the next write to another file splices again
 */
int utest_recvfile_unsplicable_fs_falls_back(AFPObj *obj, struct vol *vol)
{
    return rf_write_test(obj, vol, OPENFORK_DATA, RF_FILE, 0, EINVAL, AFP_OK,
                         true);
}

/*!
 * @brief A write the kernel refuses for its offset alone fails without
 *        marking the file as one that cannot splice
 */
int utest_recvfile_bad_offset_keeps_splice(AFPObj *obj, struct vol *vol)
{
    const struct ofork *of;
    uint64_t one = hton64(UINT64_C(1));
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)
            && (of = of_find(rf.refnum[0])) != NULL) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], INT64_MAX - 1, RF_C_LEN,
                             0xA5);
        /* a count of one keeps offset + count in range for write_fork() */
        memcpy(rf.buf + DSI_BLOCKSIZ + 12, &one, sizeof(one));

        if (rf_send_after_head(&rf, len) == 0 && rf_dispatch(&rf) != AFP_OK
                && !of->of_ad->ad_nosplice) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief A socket splice() that fails before reading anything leaves the
 *        write to the userspace path, and the next write splices again
 */
int utest_recvfile_no_socket_splice_falls_back(AFPObj *obj, struct vol *vol)
{
    return rf_write_test(obj, vol, OPENFORK_DATA, RF_SOCK, 0, ENOSYS, AFP_OK,
                         true);
}

/*!
 * @brief 256 KiB to a new data fork with splice size set to size, and
 *        F_SETPIPE_SZ above limit refused when limit is set
 *
 * @returns 0 when the write splices and leaves the session's pipe holding
 *          at least min_size bytes and no more than limit; -1 when not;
 *          TEST_SKIP
 */
static int rf_pipe_test(AFPObj *obj, struct vol *vol, int size, int limit,
                        int min_size)
{
    struct rf rf;
    size_t len;
    int got;
    int ret;

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    /* a pipe this process cannot grow to min_size is the documented
     * fallback, and a new pipe that holds size leaves nothing to halve */
    if (rf_pipe_capacity(min_size) < min_size
            || (limit > 0 && rf_pipe_capacity(0) >= size)) {
        rf_teardown(&rf);
        return TEST_SKIP;
    }

    ret = -1;
    rf.dsi.splice_size = size;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_B_LEN, 0xA5);
        rf_arm(RF_SOCK, 0, 0);
        fi.pipe_size_limit = limit;

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK
                && rf_fired(RF_SOCK, 0)
                && (got = rf_pipe_size(rf.dsi.splice_pipe[1])) >= min_size
                && (limit == 0 || got <= limit)
                && rf_holds(&rf, 0, RF_B_LEN, 0xA5)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*! @brief The recvfile pipe holds the size splice size asks for */
int utest_recvfile_pipe_takes_splice_size(AFPObj *obj, struct vol *vol)
{
    return rf_pipe_test(obj, vol, RF_B_LEN, 0, RF_B_LEN);
}

/*! @brief A splice size below a new pipe's capacity leaves the pipe alone */
int utest_recvfile_pipe_never_shrinks(AFPObj *obj, struct vol *vol)
{
    return rf_pipe_test(obj, vol, SPLICE_SIZE_MIN, 0, 2 * SPLICE_SIZE_MIN);
}

/*! @brief A refused splice size halves down to the largest size granted */
int utest_recvfile_pipe_steps_down(AFPObj *obj, struct vol *vol)
{
    return rf_pipe_test(obj, vol, RF_A_LEN, 2 * RF_B_LEN, 2 * RF_B_LEN);
}

/*!
 * @brief A fork write the filesystem cuts short fails the request as disk
 *        full
 *
 * The request carries 1 MiB through the userspace path under a 256 KiB file
 * size limit.
 */
int utest_write_fork_short_write_fails_request(AFPObj *obj, struct vol *vol)
{
    struct rlimit saved;
    struct rlimit limit;
    void (*xfsz)(int);
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;
    rf.obj.options.flags &= ~OPTION_RECVFILE;
    xfsz = signal(SIGXFSZ, SIG_IGN);

    if (getrlimit(RLIMIT_FSIZE, &saved) == 0
            && rf_open(&rf, 0, OPENFORK_DATA)) {
        limit = saved;
        limit.rlim_cur = RF_B_LEN;
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_A_LEN, 0xA5);
        len += rf_cmd_frame(rf.buf + len, 2);

        if (setrlimit(RLIMIT_FSIZE, &limit) == 0
                && rf_send(&rf, rf.buf, len) == 0
                && rf_write(&rf) == AFPERR_DFULL && rf_next_is(&rf, 2)) {
            ret = 0;
        }

        setrlimit(RLIMIT_FSIZE, &saved);
    }

    signal(SIGXFSZ, xfsz);
    rf_teardown(&rf);
    return ret;
}
