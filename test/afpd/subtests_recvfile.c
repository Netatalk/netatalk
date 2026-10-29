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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/dsi.h>
#include <atalk/globals.h>
#include <atalk/netatalk_conf.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afpfunc_helpers.h"
#include "auth.h"
#include "directory.h"
#include "faultinject.h"
#include "fork.h"
#include "subtests_recvfile.h"
#include "test.h"
#include "test_capabilities.h"
#include "volume.h"

#define RF_A_LEN (1024 * 1024)
#define RF_B_LEN (256 * 1024)
/* fits a loopback window with nobody reading */
#define RF_C_LEN (12 * 1024)
/* a splice size well below a write, so the write takes many rounds */
#define RF_ROUND (64 * 1024)
/* splice() calls a counting arm fails none of, short of a runaway loop */
#define RF_RUNAWAY 100000

/*! A DSI session on one end of a socket pair and a writer on the other */
struct rf {
    AFPObj obj;
    struct vol *vol;
    DSI dsi;
    size_t rbuflen;
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
    return dsi_test_frame(p, DSIFUNC_CMD, reqid, 0, 2, AFP_GETSRVPARAM);
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
    /* a writer blocked in write() fails with EPIPE once its reader is gone */
    close(rf->dsi.socket);
    rf->dsi.socket = -1;

    if (rf->writing) {
        pthread_join(rf->writer, NULL);
    }

    /* the writer's write() calls read the fault settings until it ends */
    fault_inject_reset();
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
        int err = errno;
        rf_teardown(rf);
        return dsi_test_no_loopback(err) ? TEST_SKIP : -1;
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
 * @brief Send the first head bytes of the request alone and receive them
 *        before the writer starts on the rest, so the rest arrives after the
 *        header read
 *
 * @returns 0, or -1
 */
static int rf_send_split(struct rf *rf, size_t head, size_t len)
{
    if (write(rf->peer, rf->buf, head) != (ssize_t)head
            || dsi_stream_receive(&rf->dsi) != DSIFUNC_WRITE) {
        return -1;
    }

    return rf_send(rf, rf->buf + head, len - head);
}

/*! @brief rf_send_split() at the request head, before any payload */
static int rf_send_after_head(struct rf *rf, size_t len)
{
    return rf_send_split(rf, DSI_FRAME_OVERHEAD_EXT, len);
}

/*!
 * @brief Run the received DSIWrite as afp_over_dsi() does, by its command,
 *        the reply's length left in rbuflen
 */
static int rf_dispatch(struct rf *rf)
{
    AFPCmd handler = afp_write_ext;
    rf->rbuflen = DSI_DATASIZ;

    if (rf->dsi.commands[0] == AFP_WRITE) {
        handler = afp_write;
    }

    return handler(&rf->obj, (char *)rf->dsi.commands, rf->dsi.cmdlen,
                   (char *)rf->dsi.data, &rf->rbuflen);
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

    /* ad_size() reads an open resource fork's length from the file, so the
     * length the writes keep in ad_rlen is checked beside it */
    if (ad_size(of->of_ad, eid) != (off_t)len
            || (eid == ADEID_RFORK && of->of_ad->ad_rlen != (off_t)len)) {
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

/* the call a fault is armed on: a splice() side or pipe2() */
enum rf_side { RF_SOCK, RF_FILE, RF_PIPE2 };

/*! @brief The fault interposer's state for the call side names */
static struct fi_call *rf_call(enum rf_side side)
{
    return side == RF_SOCK ? &fi.splice_sock
           : side == RF_FILE ? &fi.splice_file : &fi.pipe2;
}

/*!
 * @brief Arm call fail_after + 1 of side with errnum, or with errnum 0
 *        count its calls, failing none short of RF_RUNAWAY
 */
static void rf_arm(enum rf_side side, int fail_after, int errnum)
{
    struct fi_call *s = rf_call(side);
    s->armed = 1;
    s->fail_after = errnum != 0 ? fail_after : RF_RUNAWAY;
    s->errnum = errnum != 0 ? errnum : EIO;
    s->calls = 0;
}

/*!
 * @brief Tell whether the fault rf_arm() armed fired or, for errnum 0, side
 *        saw a call and stayed short of RF_RUNAWAY
 */
static bool rf_fired(enum rf_side side, int errnum)
{
    const struct fi_call *s = rf_call(side);
    return errnum != 0 ? s->fail_after == -1
           : s->calls > 0 && s->fail_after != -1;
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
    /* a fault on a later call meets a write still under way */
    rf.dsi.splice_size = RF_ROUND;

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
                         AFPERR_MISC, false);
}

/*! @brief A failed recvfile write leaves nothing behind for the next one */
int utest_recvfile_error_leaves_no_residue(AFPObj *obj, struct vol *vol)
{
    return rf_write_test(obj, vol, OPENFORK_DATA, RF_FILE, 1, EIO,
                         AFPERR_MISC, true);
}

/*!
 * @brief A recvfile write the filesystem refuses as read-only replies access
 *        denied and drains its request
 */
int utest_recvfile_readonly_replies_access(AFPObj *obj, struct vol *vol)
{
    return rf_write_test(obj, vol, OPENFORK_DATA, RF_FILE, 0, EROFS,
                         AFPERR_ACCESS, false);
}

/*!
 * @brief A write whose header announces 1 MiB and whose stream ends after
 *        256 KiB fails, through splice() with recvfile and through recv()
 *        without it
 *
 * @returns 0, -1, or TEST_SKIP
 */
static int rf_eof_test(AFPObj *obj, struct vol *vol, bool recvfile)
{
    struct rf rf;
    int ret = recvfile ? rf_splice_setup(&rf, obj, vol)
              : rf_setup(&rf, obj, vol);

    if (ret != 0) {
        return ret;
    }

    ret = -1;

    if (!recvfile) {
        rf.obj.options.flags &= ~OPTION_RECVFILE;
    }

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_A_LEN, 0xA5);
        rf_arm(RF_SOCK, 0, 0);

        if (rf_send(&rf, rf.buf, DSI_FRAME_OVERHEAD_EXT + RF_B_LEN) == 0
                && rf_write(&rf) == AFPERR_MISC
                && rf_fired(RF_SOCK, 0) == recvfile) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*! @brief A client that disconnects mid-payload ends the recvfile write */
int utest_recvfile_peer_close_ends_write(AFPObj *obj, struct vol *vol)
{
    return rf_eof_test(obj, vol, true);
}

/*!
 * @brief A payload the stream ends before completing fails the request on
 *        the userspace path as well
 */
int utest_write_fork_eof_fails_request(AFPObj *obj, struct vol *vol)
{
    return rf_eof_test(obj, vol, false);
}

/*!
 * @brief A side armed to count its splice() calls fails none of them, past a
 *        thousand
 */
int utest_recvfile_counting_arm_fails_nothing(AFPObj *obj, struct vol *vol)
{
    uint8_t *frames = malloc(4 * (DSI_FRAME_OVERHEAD_EXT + RF_A_LEN));
    struct rf rf;
    size_t len = 0;
    int ret;

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        free(frames);
        return ret;
    }

    ret = -1;
    /* rounds of 4 KiB: four 1 MiB writes take over a thousand socket
     * splice() calls */
    rf.dsi.splice_size = SPLICE_SIZE_MIN;

    if (frames != NULL && rf_open(&rf, 0, OPENFORK_DATA)) {
        for (int i = 0; i < 4; i++) {
            len += rf_write_frame(frames + len, (uint16_t)(i + 1),
                                  rf.refnum[0], (uint64_t)i * RF_A_LEN,
                                  RF_A_LEN, 0xA5);
        }

        rf_arm(RF_SOCK, 0, 0);

        if (rf_send(&rf, frames, len) == 0 && rf_write(&rf) == AFP_OK
                && rf_write(&rf) == AFP_OK && rf_write(&rf) == AFP_OK
                && rf_write(&rf) == AFP_OK && fi.splice_sock.calls > 1000
                && rf_holds(&rf, 0, 4 * (size_t)RF_A_LEN, 0xA5)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    free(frames);
    return ret;
}

/*!
 * @brief A payload that lands marks the volume
 */
int utest_write_fork_marks_volume(AFPObj *obj, struct vol *vol)
{
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;
    rf.obj.options.flags &= ~OPTION_RECVFILE;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_C_LEN, 0xA5);
        vol->v_root_marked = 0;

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK) {
            ret = vol->v_root_marked != 0 ? 0 : 1;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief Closing a resource fork a write reached marks the volume as the
 *        close stamps the fork's modification date
 */
int utest_closefork_rsrc_marks_volume(AFPObj *obj, struct vol *vol)
{
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;
    rf.obj.options.flags &= ~OPTION_RECVFILE;

    if (rf_open(&rf, 0, OPENFORK_RSCS)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_C_LEN, 0xA5);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK) {
            vol->v_root_marked = 0;

            if (closefork(&rf.obj, rf.refnum[0]) == AFP_OK) {
                rf.refnum[0] = 0;
                ret = vol->v_root_marked != 0 ? 0 : 1;
            }
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief FPFlush marks the volume as it stamps the modification date of a
 *        resource fork a write reached, and not before the write
 */
int utest_flush_rsrc_marks_volume(AFPObj *obj, struct vol *vol)
{
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;
    rf.obj.options.flags &= ~OPTION_RECVFILE;

    if (rf_open(&rf, 0, OPENFORK_RSCS)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_C_LEN, 0xA5);
        /* a flush of the unwritten fork stamps nothing */
        vol->v_root_marked = 0;

        if (flushvol(&rf.obj, vol->v_vid) != AFP_OK
                || vol->v_root_marked != 0) {
            ret = 1;
        } else if (rf_send(&rf, rf.buf, len) == 0
                   && rf_write(&rf) == AFP_OK) {
            vol->v_root_marked = 0;

            if (flushvol(&rf.obj, vol->v_vid) == AFP_OK) {
                ret = vol->v_root_marked != 0 ? 0 : 1;
            }
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief A resource fork whose header write fails stays dirty, so the next
 *        flush stamps it and marks the volume
 */
int utest_flush_rsrc_failed_kept(AFPObj *obj, struct vol *vol)
{
    struct ofork *ofork;
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;
    rf.obj.options.flags &= ~OPTION_RECVFILE;

    if (rf_open(&rf, 0, OPENFORK_RSCS)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_C_LEN, 0xA5);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK
                && (ofork = of_find(rf.refnum[0])) != NULL) {
            vol->v_root_marked = 0;
            fi.pwrite.armed = 1;
            fi.pwrite.errnum = EIO;
            ret = flushfork(ofork);
            fi.pwrite.armed = 0;

            /* no pwrite() reached the interposer: the header write takes
             * none here, or the preload misses libatalk */
            if (fi.pwrite.calls == 0) {
                ret = TEST_SKIP;
            } else if (ret == 0 || vol->v_root_marked != 0) {
                ret = 1;
            } else {
                ret = flushfork(ofork) == 0 && vol->v_root_marked != 0 ? 0 : 1;
            }
        }
    }

    rf_teardown(&rf);
    return ret;
}

static int rf_attentions;

/*! @brief Count the attentions a sweep's marks send */
static int rf_count_attention(void *handle _U_, AFPUserBytes flags _U_)
{
    rf_attentions++;
    return 1;
}

/*!
 * @brief A volume close whose sweep closes a written resource fork writes
 *        the root as it stamps the fork's modification date, and sends a
 *        client from AFP 2.2 to 3.1 no free-space attention
 */
int utest_closevol_rsrc_marks_volume(AFPObj *obj, struct vol *vol)
{
    int (*const attention)(void *, AFPUserBytes) = obj->attention;
    const int version = obj->afp_version;
    const int flags = obj->options.flags;
    const int proto = obj->proto;
    struct timespec old[2] = { { 0 } };
    struct stat st;
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;
    rf.obj.options.flags &= ~OPTION_RECVFILE;

    if (rf_open(&rf, 0, OPENFORK_RSCS)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_C_LEN, 0xA5);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK) {
            /* the root a minute back, with the write's mark spent */
            old[0].tv_sec = old[1].tv_sec = time(NULL) - 60;
            vol->v_root_marked = 0;
            vol->v_root_due = 0;
            ret = utimensat(AT_FDCWD, vol->v_path, old, 0) == 0 ? 0 : -1;
        }
    }

    if (ret == 0) {
        /* an AFP 3.1 client with server notifications */
        rf_attentions = 0;
        obj->attention = rf_count_attention;
        obj->afp_version = 31;
        obj->options.flags |= OPTION_SERVERNOTIF;
        obj->proto = AFPPROTO_ASP;
        /* the sweep closes the fork; a volume close clears the current
         * directory first */
        curdir = NULL;
        closevol(obj, vol);
        rf.refnum[0] = 0;
        obj->attention = attention;
        obj->afp_version = version;
        obj->options.flags = flags;
        obj->proto = proto;
        ret = stat(vol->v_path, &st) == 0 && st.st_mtime > old[1].tv_sec
              && rf_attentions == 0 ? 0 : 1;

        /* the volume goes back open for the tests after this one */
        if (openvol(obj, vol->v_localname) == 0) {
            ret = -1;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief A logout whose sweep closes a written resource fork sends a client
 *        from AFP 2.2 to 3.1 no free-space attention
 */
int utest_logout_rsrc_quiet(AFPObj *obj, struct vol *vol)
{
    int (*const attention)(void *, AFPUserBytes) = obj->attention;
    const int version = obj->afp_version;
    const int flags = obj->options.flags;
    const int proto = obj->proto;
    static char names[16][256];
    size_t len;
    size_t n = 0;
    size_t rlen = 0;
    struct rf rf;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;
    rf.obj.options.flags &= ~OPTION_RECVFILE;

    if (rf_open(&rf, 0, OPENFORK_RSCS)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_C_LEN, 0xA5);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK) {
            ret = 0;
        }
    }

    if (ret == 0) {
        for (const struct vol *v = getvolumes(); v != NULL && n < 16;
                v = v->v_next) {
            if (v->v_flags & AFPVOL_OPEN) {
                snprintf(names[n++], sizeof(names[0]), "%s", v->v_localname);
            }
        }

        /* an AFP 3.1 client with server notifications, the write's mark
         * spent */
        rf_attentions = 0;
        obj->attention = rf_count_attention;
        obj->afp_version = 31;
        obj->options.flags |= OPTION_SERVERNOTIF;
        obj->proto = AFPPROTO_ASP;
        vol->v_root_marked = 0;
        vol->v_root_due = 0;
        curdir = NULL;
        (void)afp_logout(obj, NULL, 0, NULL, &rlen);
        rf.refnum[0] = 0;
        obj->attention = attention;
        obj->afp_version = version;
        obj->options.flags = flags;
        obj->proto = proto;
        ret = rf_attentions == 0 ? 0 : 1;

        /* the volumes go back open for the tests after this one */
        for (size_t i = 0; i < n; i++) {
            if (openvol(obj, names[i]) == 0) {
                ret = -1;
            }
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
    /* the second write's first round lands before its fault */
    rf.dsi.splice_size = RF_ROUND;

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
                && of->of_ad->ad_rlen == 0
                && !(of->of_flags & AFPFORK_MODIFIED)) {
            fault_inject_reset();
            rf_arm(RF_FILE, 1, ENOSPC);

            if (rf_write(&rf) == AFPERR_DFULL && rf_fired(RF_FILE, ENOSPC)
                    && (of->of_flags & AFPFORK_MODIFIED)
                    && fstat(ad_reso_fileno(of->of_ad), &st) == 0
                    && of->of_ad->ad_rlen == st.st_size - ADEDOFF_RFORK_OSX) {
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

        /* the head is buffered before the receive, so the wait the test
         * reaches is the recvfile loop's */
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
 * @brief A recvfile write whose socket holds an urgent byte past a gap in
 *        the stream waits for its payload, not for the urgent byte
 */
int utest_recvfile_urgent_byte_ahead_waits(AFPObj *obj, struct vol *vol)
{
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_A_LEN, 0xA5);
        /* the first socket splice finds the queue empty */
        rf_arm(RF_SOCK, 0, EAGAIN);
        fi.poll_urgent = 1;

        if (rf_send_after_head(&rf, len) == 0 && rf_dispatch(&rf) == AFP_OK
                && rf_fired(RF_SOCK, EAGAIN)
                && fi.poll_waits > 0 && fi.poll_urgent_wakeups == 0
                && rf_holds(&rf, 0, RF_A_LEN, 0xA5)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
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
 * @brief A write whose copy fails after the file refused splice() fails
 *        without marking the file as one that cannot splice
 *
 * The file-side splice() is refused with EINVAL and the copy that takes
 * over meets a file size limit of 0.
 */
int utest_recvfile_failed_copy_keeps_splice(AFPObj *obj, struct vol *vol)
{
    const struct ofork *of;
    struct fsize_cap cap;
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)
            && (of = of_find(rf.refnum[0])) != NULL) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_C_LEN, 0xA5);
        rf_arm(RF_FILE, 0, EINVAL);

        if (rf_send_after_head(&rf, len) == 0 && fsize_cap(&cap, 0) == 0) {
            ret = rf_dispatch(&rf) == AFPERR_DFULL
                  && rf_fired(RF_FILE, EINVAL) && !of->of_ad->ad_nosplice
                  ? 0 : -1;
            fsize_uncap(&cap);
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief An FPWrite whose payload is longer than its ReqCount is refused
 *        before anything is written, and the next request parses
 *
 * Half the 8 KiB payload arrives with the header, in the read-ahead, and
 * half after it, so neither part alone exceeds the ReqCount of 4 KiB.
 */
int utest_write_fork_payload_over_count_refused(AFPObj *obj, struct vol *vol)
{
    uint64_t half = hton64(UINT64_C(4096));
    size_t head = DSI_FRAME_OVERHEAD_EXT + 4096;
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, 8192, 0xA5);
        memcpy(rf.buf + DSI_BLOCKSIZ + 12, &half, sizeof(half));
        len += rf_cmd_frame(rf.buf + len, 2);

        if (rf_send_split(&rf, head, len) == 0
                && rf_dispatch(&rf) == AFPERR_PARAM
                && rf_holds(&rf, 0, 0, 0xA5) && rf_next_is(&rf, 2)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief An FPWrite with a ReqCount of 0 replies AFP_OK and its offset,
 *        the payload drained unwritten, and the next request parses
 *
 * The 16 KiB payload arrives after the header read, so the drain reads all
 * of it through the reply buffer.
 */
int utest_write_fork_zero_count_replies_offset(AFPObj *obj, struct vol *vol)
{
    uint64_t zero = 0;
    uint64_t off_be = hton64(UINT64_C(4096));
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 4096, 16384, 0xA5);
        memcpy(rf.buf + DSI_BLOCKSIZ + 12, &zero, sizeof(zero));
        len += rf_cmd_frame(rf.buf + len, 2);

        if (rf_send_after_head(&rf, len) == 0 && rf_dispatch(&rf) == AFP_OK
                && rf.rbuflen == sizeof(off_be)
                && memcmp(rf.dsi.data, &off_be, sizeof(off_be)) == 0
                && rf_holds(&rf, 0, 0, 0xA5) && rf_next_is(&rf, 2)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief A 32-bit FPWrite from the end of a fork whose range passes 2 GiB is
 *        refused with AFPERR_DFULL, its payload drained, the fork as it was
 *
 * The end flag turns an offset of INT32_MAX on a 2-byte fork into one past
 * 2 GiB, whose sum with a count of INT32_MAX has bit 31 clear.
 */
int utest_write_fork_end_past_2gib_refused(AFPObj *obj, struct vol *vol)
{
    uint32_t max_be = htonl(INT32_MAX);
    struct rf rf;
    uint8_t *p;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, 2, 0xA5);
        p = rf.buf + len;
        len += dsi_test_frame(p, DSIFUNC_WRITE, 2, DSI_WROFF_FPWRITE,
                              DSI_WROFF_FPWRITE + 16, AFP_WRITE);
        /* the end flag, the fork, then offset and count */
        p[DSI_BLOCKSIZ + 1] = 0x80;
        memcpy(p + DSI_BLOCKSIZ + 2, &rf.refnum[0], sizeof(rf.refnum[0]));
        memcpy(p + DSI_BLOCKSIZ + 4, &max_be, sizeof(max_be));
        memcpy(p + DSI_BLOCKSIZ + 8, &max_be, sizeof(max_be));
        memset(p + DSI_BLOCKSIZ + DSI_WROFF_FPWRITE, 0xA5, 16);
        len += rf_cmd_frame(rf.buf + len, 3);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK
                && rf_write(&rf) == AFPERR_DFULL
                && rf_holds(&rf, 0, 2, 0xA5) && rf_next_is(&rf, 3)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief An FPWriteExt to a data fork whose range passes off_t is refused
 *        with AFPERR_DFULL, its payload drained, the fork as it was
 *
 * An offset of INT64_MAX - 15 and a count of 64 give a sum past INT64_MAX,
 * a signed overflow that a compiler may assume never happens.
 */
int utest_write_fork_dfork_past_off_t_refused(AFPObj *obj, struct vol *vol)
{
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], INT64_MAX - 15, 64, 0xA5);
        len += rf_cmd_frame(rf.buf + len, 2);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFPERR_DFULL
                && rf_holds(&rf, 0, 0, 0xA5) && rf_next_is(&rf, 2)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief An FPWriteExt to a resource fork whose range passes off_t at the
 *        fork's offset in its file is refused with AFPERR_DFULL, as one to a
 *        data fork is
 */
int utest_write_fork_rfork_past_off_t_refused(AFPObj *obj, struct vol *vol)
{
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_RSCS)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], INT64_MAX - 10, 10, 0xA5);
        len += rf_cmd_frame(rf.buf + len, 2);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFPERR_DFULL
                && rf_holds(&rf, 0, 0, 0xA5) && rf_next_is(&rf, 2)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief Append 50 bytes of 0x5A to the 100-byte resource fork of fork slot
 *        i through an adouble of its own, as another session's afpd writes
 *
 * @returns 0, or -1
 */
static int rf_append_elsewhere(struct rf *rf, int i)
{
    const int flags = ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF | ADFLAGS_RDWR;
    char path[MAXPATHLEN + 1];
    char bytes[50];
    struct adouble ad;
    int ret = -1;
    snprintf(path, sizeof(path), "%s/%s", rf->vol->v_path, rf->name[i]);
    memset(bytes, 0x5A, sizeof(bytes));
    ad_init(&ad, rf->vol);

    if (ad_open(&ad, path, flags, 0) == 0) {
        if (ad_write(&ad, ADEID_RFORK, 100, 0, bytes, sizeof(bytes))
                == (ssize_t)sizeof(bytes) && ad_flush(&ad) == 0) {
            ret = 0;
        }

        ad_close(&ad, flags);
    }

    return ret;
}

/*!
 * @brief An FPWriteExt from the end of a resource fork lands after the bytes
 *        another session appended since this one last wrote the fork
 */
int utest_write_fork_rfork_end_after_append(AFPObj *obj, struct vol *vol)
{
    const struct ofork *of;
    char got[60];
    struct rf rf;
    size_t first;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_RSCS)
            && (of = of_find(rf.refnum[0])) != NULL) {
        first = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, 100, 0xA5);
        len = first + rf_write_frame(rf.buf + first, 2, rf.refnum[0], 0, 10,
                                     0xC3);
        /* the second write counts its offset from the end of the fork */
        rf.buf[first + DSI_BLOCKSIZ + 1] = 0x80;

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK
                && rf_append_elsewhere(&rf, 0) == 0 && rf_write(&rf) == AFP_OK
                && ad_size(of->of_ad, ADEID_RFORK) == 160
                && ad_read(of->of_ad, ADEID_RFORK, 100, got, sizeof(got))
                == (ssize_t)sizeof(got)) {
            ret = 0;

            for (size_t j = 0; j < sizeof(got); j++) {
                if ((uint8_t)got[j] != (j < 50 ? 0x5A : 0xC3)) {
                    ret = -1;
                }
            }
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief An FPByteRangeLockExt from the end of a resource fork starts after
 *        the bytes another session appended, where a write from the end lands
 */
int utest_byte_lock_rfork_end_after_append(AFPObj *obj, struct vol *vol)
{
    uint64_t start = 0;
    struct rf rf;
    size_t len;
    int rc;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_RSCS)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, 100, 0xA5);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK
                && rf_append_elsewhere(&rf, 0) == 0) {
            /* a lock of 10 bytes from the fork's end lands after the
             * appended bytes, at 150 */
            rc = bytelock_ext(&rf.obj, rf.refnum[0], 1, 0, 0, 10, &start);

            if (rc == AFP_OK && start == 150) {
                ret = 0;
            }
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief An FPWriteExt framed with a 32-bit FPWrite's 12-byte block, too
 *        short for its ReqCount, is refused with its payload drained
 *
 * The count the block lacks is the one the 64-byte write before it left in
 * the command buffer, so a write that took it would pass the payload bound.
 */
int utest_write_fork_short_block_refused(AFPObj *obj, struct vol *vol)
{
    struct rf rf;
    uint8_t *p;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, 64, 0xA5);
        p = rf.buf + len;
        len += rf_write_frame(p, 2, rf.refnum[0], 64, 16, 0xA5);
        /* the ReqCount past the 12-byte block arrives as payload */
        dsi_test_header(p, DSIFUNC_WRITE, 2, DSI_WROFF_FPWRITE,
                        DSI_WROFF_FPWRITEEXT + 16);
        len += rf_cmd_frame(rf.buf + len, 3);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK
                && rf_write(&rf) == AFPERR_PARAM
                && rf_holds(&rf, 0, 64, 0xA5) && rf_next_is(&rf, 3)) {
            ret = 0;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief 1 MiB of 0xA5 to a new data fork while splice() on side fails
 *        with errnum before reading anything, then 256 KiB more, of 0xA5 to
 *        the same fork at 1 MiB with same_fork, else of 0x5A to a second
 *        data fork
 *
 * @returns 0 when the first write replies AFP_OK with the fault fired, a
 *          socket's refusal closing the session's pipe, the second write
 *          replies AFP_OK with no splice() on either side, and each fork
 *          holds its writes' bytes; -1 when not; TEST_SKIP
 */
static int rf_decline_test(AFPObj *obj, struct vol *vol, enum rf_side side,
                           int errnum, bool same_fork)
{
    int fork = same_fork ? 0 : 1;
    uint64_t off = same_fork ? RF_A_LEN : 0;
    size_t held = same_fork ? RF_A_LEN + RF_B_LEN : RF_B_LEN;
    uint8_t fill = same_fork ? 0xA5 : 0x5A;
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)
            && (same_fork || rf_open(&rf, 1, OPENFORK_DATA))) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_A_LEN, 0xA5);
        len += rf_write_frame(rf.buf + len, 2, rf.refnum[fork], off, RF_B_LEN,
                              fill);
        rf_arm(side, 0, errnum);

        if (rf_send(&rf, rf.buf, len) == 0 && rf_write(&rf) == AFP_OK
                && rf_fired(side, errnum)
                && rf_holds(&rf, 0, RF_A_LEN, 0xA5)
                && (side != RF_SOCK || rf.dsi.splice_pipe[0] == -1)) {
            rf_arm(RF_SOCK, 0, 0);
            rf_arm(RF_FILE, 0, 0);

            if (rf_write(&rf) == AFP_OK && !rf_fired(RF_SOCK, 0)
                    && !rf_fired(RF_FILE, 0)
                    && rf_holds(&rf, fork, held, fill)) {
                ret = 0;
            }
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief A socket splice() that fails before reading anything leaves the
 *        write to the userspace path, and the session's later writes too,
 *        with no splice() tried and the session's pipe closed
 */
int utest_recvfile_no_socket_splice_falls_back(AFPObj *obj, struct vol *vol)
{
    return rf_decline_test(obj, vol, RF_SOCK, ENOSYS, false);
}

/*!
 * @brief 256 KiB twice to one new data fork, the first write with the pipe
 *        knobs set and the second once they have cleared
 *
 * @returns 0 when the first write lands through recv() with no pipe kept,
 *          the second opens a pipe, splices from the socket and lands after
 *          it, and each tried one pipe2(), -1 when not, TEST_SKIP without
 *          splice
 */
static int rf_pipe_fault_test(AFPObj *obj, struct vol *vol, int budget_spent,
                              int pipe2_errno)
{
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_splice_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_B_LEN, 0xA5);
        len += rf_write_frame(rf.buf + len, 2, rf.refnum[0], RF_B_LEN,
                              RF_B_LEN, 0xA5);
        fi.pipe_budget_spent = budget_spent;
        rf_arm(RF_PIPE2, 0, pipe2_errno);

        if (rf_send(&rf, rf.buf, len) == 0
                && rf_write(&rf) == AFP_OK && rf_holds(&rf, 0, RF_B_LEN, 0xA5)
                && rf.dsi.splice_pipe[0] == -1) {
            /* the budget comes back as other sessions close their pipes */
            fi.pipe_budget_spent = 0;
            rf_arm(RF_SOCK, 0, 0);
            ret = rf_write(&rf) == AFP_OK
                  && rf_holds(&rf, 0, 2 * RF_B_LEN, 0xA5)
                  && fi.pipe2.calls == 2 && rf.dsi.splice_pipe[0] != -1
                  && rf_fired(RF_SOCK, 0) ? 0 : -1;
        }
    }

    rf_teardown(&rf);
    return ret;
}

/*!
 * @brief Over the pipe budget a write tries one pipe and goes through
 *        recv(), and the next write splices once a pipe fits
 */
int utest_recvfile_spent_budget_retries(AFPObj *obj, struct vol *vol)
{
    return rf_pipe_fault_test(obj, vol, 1, 0);
}

/*!
 * @brief A pipe2() that fails leaves that write to recv(), and the next
 *        write of the session opens a pipe and splices
 */
int utest_recvfile_pipe2_failure_retries(AFPObj *obj, struct vol *vol)
{
    return rf_pipe_fault_test(obj, vol, 0, EMFILE);
}

/*!
 * @brief A file whose filesystem refused splice() is not spliced again: its
 *        next write goes through recv() with no splice() on either side
 */
int utest_recvfile_unsplicable_file_not_spliced(AFPObj *obj, struct vol *vol)
{
    return rf_decline_test(obj, vol, RF_FILE, EINVAL, true);
}

/*!
 * @brief A connection error on the first socket splice() fails the write
 *        and drains its request, rather than leaving it to recv()
 */
int utest_recvfile_socket_error_fails_request(AFPObj *obj, struct vol *vol)
{
    return rf_write_test(obj, vol, OPENFORK_DATA, RF_SOCK, 0, ETIMEDOUT,
                         AFPERR_MISC, false);
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
    struct fsize_cap cap;
    struct rf rf;
    size_t len;
    int ret;

    if ((ret = rf_setup(&rf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;
    rf.obj.options.flags &= ~OPTION_RECVFILE;

    if (rf_open(&rf, 0, OPENFORK_DATA)) {
        len = rf_write_frame(rf.buf, 1, rf.refnum[0], 0, RF_A_LEN, 0xA5);
        len += rf_cmd_frame(rf.buf + len, 2);

        if (fsize_cap(&cap, RF_B_LEN) == 0) {
            if (rf_send(&rf, rf.buf, len) == 0
                    && rf_write(&rf) == AFPERR_DFULL && rf_next_is(&rf, 2)) {
                ret = 0;
            }

            fsize_uncap(&cap);
        }
    }

    rf_teardown(&rf);
    return ret;
}
