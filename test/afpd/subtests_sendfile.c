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
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/dsi.h>
#include <atalk/globals.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "faultinject.h"
#include "fork.h"
#include "subtests_sendfile.h"
#include "switch.h"
#include "test.h"
#include "test_capabilities.h"
#include "virtual_icon.h"

#define SF_LEN   (1024 * 1024)
#define SF_SMALL (64 * 1024)
#define SF_HALF  (DSI_DATASIZ / 2)
#define SF_SHORT 64
#define SF_HUGE  (((uint64_t)1 << 31) + 4096)
#define SF_BUF   65536
#define SF_FRAME (DSI_BLOCKSIZ + 20)
#define SF_ODD   4097

/*! A DSI session, one fork, and a reader on the peer */
struct sf {
    AFPObj obj;
    struct vol *vol;
    DSI dsi;
    int peer;
    pthread_t reader;
    uint8_t *got;
    size_t cap;
    size_t gotlen;
    uint16_t refnum;
    const char *name;
    int exit_code;
    /* in_write as obj->exit() saw it: afpd's session ends there, the test's
     * goes on */
    int exit_in_write;
    bool eof;
};

/*! The session whose read path calls sf_exit() */
static struct sf *sf_session;

/*! The file the tests create, its name writable as dircache lookups take it */
static char sf_file[] = "sf_file";

/*!
 * @brief obj->exit() of the read path: records the code it ends with and
 *        in_write, which afp_dsi_die() reads before it writes an attention
 */
static void sf_exit(int code)
{
    sf_session->exit_code = code;
    sf_session->exit_in_write = sf_session->dsi.in_write;
}

/*! @brief Byte i of every fork the tests write */
static uint8_t sf_byte(size_t i)
{
    return (uint8_t)(i % 251);
}

/*!
 * @brief Reader thread: take up to cap bytes of the reply, then close the
 *        peer, which ends a reply longer than cap
 */
static void *sf_reader(void *arg)
{
    struct sf *sf = arg;

    while (sf->gotlen < sf->cap) {
        ssize_t n = read(sf->peer, sf->got + sf->gotlen, sf->cap - sf->gotlen);

        if (n == -1 && errno == EINTR) {
            continue;
        }

        if (n <= 0) {
            sf->eof = n == 0;
            break;
        }

        sf->gotlen += (size_t)n;
    }

    close(sf->peer);
    sf->peer = -1;
    return NULL;
}

/*! @brief Close the fork, delete its file and release the session */
static void sf_teardown(struct sf *sf)
{
    fault_inject_reset();

    if (sf->peer != -1) {
        close(sf->peer);
    }

    if (sf->refnum != 0) {
        closefork(&sf->obj, sf->refnum);
    }

    if (sf->name != NULL) {
        delete (&sf->obj, sf->vol->v_vid, DIRDID_ROOT, sf->name);
    }

    dir_free_invalid_q();
    free(sf->got);
    dsi_test_cleanup(&sf->dsi);
}

/*!
 * @brief Serve a copy of obj on one end of a socket pair with sendfile on
 *
 * @returns 0, or -1
 */
static int sf_pair_setup(struct sf *sf, AFPObj *obj, struct vol *vol)
{
    memset(sf, 0, sizeof(*sf));
    sf->peer = -1;
    sf->vol = vol;
    sf->obj = *obj;
    sf->obj.dsi = &sf->dsi;
    sf->obj.proto = AFPPROTO_DSI;
    sf->obj.exit = sf_exit;
    sf->obj.options.flags &= ~OPTION_NOSENDFILE;
    sf_session = sf;
    fault_inject_reset();

    if (dsi_test_open(&sf->dsi, DSI_DATASIZ, &sf->peer) != 0) {
        sf_teardown(sf);
        return -1;
    }

    return 0;
}

/*!
 * @brief sf_pair_setup() over loopback TCP with a small send buffer, so a
 *        reply takes many calls, for a test of the sendfile path; with fault
 *        set, for one that arms the fault interposer
 *
 * @returns 0, -1, or TEST_SKIP on a build without sendfile, in a sandbox
 *          without loopback, and with fault set where the interposer does not
 *          reach
 */
static int sf_sendfile_setup(struct sf *sf, AFPObj *obj, struct vol *vol,
                             bool fault)
{
#ifndef WITH_SENDFILE
    return TEST_SKIP;
#else
    int sndbuf = SF_BUF;
    int err;

    if (fault && !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (sf_pair_setup(sf, obj, vol) != 0) {
        return -1;
    }

    if (dsi_test_connect_tcp(&sf->dsi, &sf->peer) != 0) {
        err = errno;
        sf_teardown(sf);
        return dsi_test_no_loopback(err) ? TEST_SKIP : -1;
    }

    if (setsockopt(sf->dsi.socket, SOL_SOCKET, SO_SNDBUF, &sndbuf,
                   sizeof(sndbuf)) != 0) {
        sf_teardown(sf);
        return -1;
    }

    return 0;
#endif
}

/*!
 * @brief sf_pair_setup() for a read through the request buffer, as under
 *        use sendfile = no
 *
 * @returns 0, or -1
 */
static int sf_buffered_setup(struct sf *sf, AFPObj *obj, struct vol *vol)
{
    if (sf_pair_setup(sf, obj, vol) != 0) {
        return -1;
    }

    sf->obj.options.flags |= OPTION_NOSENDFILE;
    return 0;
}

/*!
 * @brief FPCreateFile sf_file, FPOpenFork of its fork read-write, and len
 *        bytes of sf_byte() written into the fork
 *
 * @returns the fork's adouble, or NULL
 */
static struct adouble *sf_open(struct sf *sf, uint8_t fork, size_t len)
{
    static char buf[65536];
    int eid = fork == OPENFORK_DATA ? ADEID_DFORK : ADEID_RFORK;
    struct ofork *of;
    sf->name = sf_file;

    if (createfile(&sf->obj, sf->vol->v_vid, DIRDID_ROOT, sf->name) != AFP_OK
            || openfork(&sf->obj, sf->vol->v_vid, DIRDID_ROOT, sf->name, fork,
                        OPENACC_RD | OPENACC_WR, &sf->refnum) != AFP_OK
            || (of = of_find(sf->refnum)) == NULL) {
        return NULL;
    }

    for (size_t off = 0; off < len; off += sizeof(buf)) {
        size_t n = MIN(sizeof(buf), len - off);

        for (size_t i = 0; i < n; i++) {
            buf[i] = (char)sf_byte(off + i);
        }

        if (ad_write(of->of_ad, eid, (off_t)off, 0, buf, n) != (ssize_t)n) {
            return NULL;
        }
    }

    return of->of_ad;
}

/*!
 * @brief Close the fork sf_open() opened, have FPGetFileDirParms put the file
 *        in the dircache with rlen as its resource fork's length, and open
 *        that fork read-only, the open the rfork cache serves
 *
 * @returns the file's dircache entry, or NULL
 */
static struct dir *sf_reopen_cached(struct sf *sf, off_t rlen)
{
    struct dir *entry;

    if (closefork(&sf->obj, sf->refnum) != AFP_OK) {
        return NULL;
    }

    sf->refnum = 0;

    if (getfiledirparms(&sf->obj, sf->vol->v_vid, DIRDID_ROOT, sf_file)
            != AFP_OK
            || (entry = dircache_search_by_name(sf->vol,
                                                dirlookup(sf->vol, DIRDID_ROOT),
                                                sf_file,
                                                sizeof(sf_file) - 1)) == NULL
            || openfork(&sf->obj, sf->vol->v_vid, DIRDID_ROOT, sf_file,
                        OPENFORK_RSCS, OPENACC_RD, &sf->refnum) != AFP_OK) {
        return NULL;
    }

    entry->dcache_rlen = rlen;
    return entry;
}

/*!
 * @brief Have the next read cut the resource fork of ad to len bytes after it
 *        sizes the fork, as another process's truncate would: strict locking
 *        takes the read's lock between the two, and the fault interposer
 *        truncates at that lock
 */
static void sf_shrink_at_lock(struct sf *sf, const struct adouble *ad,
                              off_t len)
{
    sf->obj.options.flags |= OPTION_STRICT_LOCKING;
    fi.shrink_fd = ad_reso_fileno(ad);
    fi.shrink_to = ad_getentryoff(ad, ADEID_RFORK) + len;
    fi.shrink_armed = 1;
}

/*!
 * @brief Make the data fork of ad len bytes long, the bytes past what it
 *        holds a hole, as a long fork reads without the test writing it
 *
 * @returns 0, TEST_SKIP when the file system will not hold that length, as a
 *          tmpfs that counts the pages of a hole does, or -1
 */
static int sf_grow(const struct adouble *ad, off_t len)
{
    int err;

    if (ftruncate(ad_data_fileno(ad), len) == 0) {
        return 0;
    }

    err = errno;
    fprintf(stderr, "# sf_grow: a fork of %jd bytes: %s\n", (intmax_t)len,
            strerror(err));
    return err == ENOSPC || err == EFBIG ? TEST_SKIP : -1;
}

/*! @brief Pack request 1, an FPReadExt for count bytes at offset, into frame */
static void sf_frame(const struct sf *sf, uint8_t *frame, uint64_t offset,
                     uint64_t count)
{
    uint8_t *p = frame + DSI_BLOCKSIZ;
    uint64_t offset_be = hton64(offset);
    uint64_t count_be = hton64(count);
    dsi_test_frame(frame, DSIFUNC_CMD, 1, 0, 20, AFP_READ_EXT);
    /* refnum travels raw, as afp_openfork() returns it */
    memcpy(p + 2, &sf->refnum, sizeof(sf->refnum));
    memcpy(p + 4, &offset_be, sizeof(offset_be));
    memcpy(p + 12, &count_be, sizeof(count_be));
}

/*!
 * @brief Send FPReadExt for count bytes at offset as a client does, and
 *        receive it on the session
 *
 * @returns 0, or -1 when the request goes astray
 */
static int sf_request(struct sf *sf, uint64_t offset, uint64_t count)
{
    uint8_t frame[SF_FRAME];
    sf_frame(sf, frame, offset, count);

    if (write(sf->peer, frame, sizeof(frame)) != sizeof(frame)
            || dsi_stream_receive(&sf->dsi) != DSIFUNC_CMD) {
        return -1;
    }

    return 0;
}

/*!
 * @brief Send FPReadExt for count bytes at offset as a client does, and run
 *        it as afp_over_dsi() does while the reader takes up to cap bytes of
 *        the reply; the stream ends after it
 *
 * @returns the AFP result, or -1 when the request goes astray
 */
static int sf_read(struct sf *sf, uint64_t offset, uint64_t count, size_t cap)
{
    size_t rbuflen = DSI_DATASIZ;
    int ret;

    if (sf_request(sf, offset, count) != 0
            || (sf->got = malloc(cap)) == NULL) {
        return -1;
    }

    sf->cap = cap;

    if (pthread_create(&sf->reader, NULL, sf_reader, sf) != 0) {
        return -1;
    }

    ret = afp_read_ext(&sf->obj, (char *)sf->dsi.commands, sf->dsi.cmdlen,
                       (char *)sf->dsi.data, &rbuflen);
    shutdown(sf->dsi.socket, SHUT_WR);
    pthread_join(sf->reader, NULL);
    return ret;
}

/*! @brief Does the reply open with request 1's header carrying code and len? */
static bool sf_header_is(const struct sf *sf, int code, uint32_t len)
{
    uint8_t want[DSI_BLOCKSIZ];
    dsi_test_header(want, DSIFUNC_CMD, 1, (uint32_t)code, len);
    want[0] = DSIFL_REPLY;
    return sf->gotlen >= DSI_BLOCKSIZ
           && memcmp(sf->got, want, sizeof(want)) == 0;
}

/*!
 * @brief Is everything after the header exactly len bytes of the fork from
 *        offset start?
 */
static bool sf_payload_from(const struct sf *sf, size_t start, size_t len)
{
    if (sf->gotlen != DSI_BLOCKSIZ + len) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        if (sf->got[DSI_BLOCKSIZ + i] != sf_byte(start + i)) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief Did the session send len bytes of the fork from start whole, under
 *        an AFP_OK header, as its only reply, and go on with no reply under
 *        way?
 */
static bool sf_reply_whole(const struct sf *sf, size_t start, size_t len)
{
    return sf_header_is(sf, AFP_OK, (uint32_t)len)
           && sf_payload_from(sf, start, len)
           && (sf->dsi.flags & DSI_NOREPLY)
           && sf->exit_code == 0 && sf->dsi.in_write == 0;
}

/*!
 * @brief A reply sent with sendfile through a small send buffer arrives
 *        whole and in step
 */
int utest_sendfile_read_lands_intact(AFPObj *obj, struct vol *vol)
{
    struct sf sf;
    int ret;

    if ((ret = sf_sendfile_setup(&sf, obj, vol, false)) != 0) {
        return ret;
    }

    ret = -1;

    if (sf_open(&sf, OPENFORK_DATA, SF_LEN) != NULL
            && sf_read(&sf, 0, SF_LEN, DSI_BLOCKSIZ + SF_LEN + 1) == AFP_OK
            && sf_reply_whole(&sf, 0, SF_LEN)) {
        ret = 0;
    }

    sf_teardown(&sf);
    return ret;
}

/*!
 * @brief A reply header that fails to send ends the session with nothing
 *        sent: no file data goes out without it
 */
int utest_sendfile_header_failure_sends_nothing(AFPObj *obj, struct vol *vol)
{
#ifndef SENDFILE_FLAVOR_LINUX
    /* elsewhere the header travels in the sendfile call itself */
    return TEST_SKIP;
#else
    struct sf sf;
    int ret;

    if ((ret = sf_sendfile_setup(&sf, obj, vol, true)) != 0) {
        return ret;
    }

    ret = -1;

    if (sf_open(&sf, OPENFORK_DATA, SF_SMALL) != NULL) {
        fi.send.armed = 1;
        fi.send.fail_after = 0;
        fi.send.errnum = ECONNRESET;
        sf_read(&sf, 0, SF_SMALL, DSI_BLOCKSIZ + SF_SMALL + 1);

        if (fi.send.fail_after == -1
                && sf.gotlen == 0
                && sf.exit_code == EXITERR_CLNT
                && sf.exit_in_write == 1) {
            ret = 0;
        }
    }

    sf_teardown(&sf);
    return ret;
#endif
}

/*!
 * @brief A reply header the socket takes a few bytes at a time goes out whole
 *        before the file data
 */
int utest_sendfile_partial_header_lands_intact(AFPObj *obj, struct vol *vol)
{
#ifndef SENDFILE_FLAVOR_LINUX
    /* elsewhere the header travels in the sendfile call itself */
    return TEST_SKIP;
#else
    struct sf sf;
    int ret;

    if ((ret = sf_sendfile_setup(&sf, obj, vol, true)) != 0) {
        return ret;
    }

    ret = -1;

    if (sf_open(&sf, OPENFORK_DATA, SF_SMALL) != NULL) {
        fi.send_max = 5;

        if (sf_read(&sf, 0, SF_SMALL, DSI_BLOCKSIZ + SF_SMALL + 1) == AFP_OK
                && sf_reply_whole(&sf, 0, SF_SMALL)) {
            ret = 0;
        }
    }

    sf_teardown(&sf);
    return ret;
#endif
}

/*!
 * @brief dsi_close() on a session over a socket pair whose in_write is
 *        in_write; got takes what the peer reads before the end of the stream
 *
 * @returns the bytes got takes, or -1
 */
static ssize_t sf_close(int in_write, uint8_t *got, size_t len)
{
    DSI *dsi;
    int fds[2];
    ssize_t n;

    if ((dsi = calloc(1, sizeof(*dsi))) == NULL) {
        return -1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        free(dsi);
        return -1;
    }

    dsi->socket = fds[1];
    dsi->proto_close = dsi_test_close_socket;
    dsi->in_write = in_write;
    /* frees dsi */
    dsi_close(dsi);
    n = recv(fds[0], got, len, 0);
    close(fds[0]);
    return n;
}

/*!
 * @brief dsi_close() sends its CloseSession frame on a session with no reply
 *        under way, and closes one with a reply under way without it
 */
int utest_dsi_close_frame_unless_mid_reply(AFPObj *obj _U_,
                                           struct vol *vol _U_)
{
    uint8_t want[DSI_BLOCKSIZ];
    uint8_t got[DSI_BLOCKSIZ + 1];
    dsi_test_header(want, DSIFUNC_CLOSE, 0, 0, 0);
    return sf_close(0, got, sizeof(got)) == DSI_BLOCKSIZ
           && memcmp(got, want, sizeof(want)) == 0
           && sf_close(1, got, sizeof(got)) == 0 ? 0 : -1;
}

/*!
 * @brief A resource fork another process cuts short after the read sized it
 *        sends what it has and ends the session with in_write raised, the
 *        header having promised the rest: through sendfile or through the
 *        request buffer
 *
 * @returns 0, -1, or TEST_SKIP
 */
static int sf_short_fork_test(AFPObj *obj, struct vol *vol, bool sendfile)
{
    const struct adouble *ad;
    struct sf sf;
    int ret;

    if (!sendfile && !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    ret = sendfile ? sf_sendfile_setup(&sf, obj, vol, true)
          : sf_buffered_setup(&sf, obj, vol);

    if (ret != 0) {
        return ret;
    }

    ret = -1;

    if ((ad = sf_open(&sf, OPENFORK_RSCS, SF_SMALL + SF_SHORT)) != NULL) {
        sf_shrink_at_lock(&sf, ad, SF_SMALL);
        /* the end of the file as illumos reports it, which no copy through
         * the request buffer, made with pread(), may take for a refusal */
        fi.sendfile_eof_einval = 1;
        fi.pread_calls = 0;
        sf_read(&sf, 0, SF_SMALL + SF_SHORT,
                DSI_BLOCKSIZ + SF_SMALL + SF_SHORT + 1);

        if (sf_header_is(&sf, AFP_OK, SF_SMALL + SF_SHORT)
                && sf_payload_from(&sf, 0, SF_SMALL)
                && sf.exit_code == EXITERR_CLNT
                && sf.exit_in_write == 1
                && (!sendfile || fi.pread_calls == 0)) {
            ret = 0;
        }
    }

    sf_teardown(&sf);
    return ret;
}

/*!
 * @brief A resource fork another process cuts short after the read sized it
 *        sends what it has and ends the session, the header having promised
 *        the rest
 */
int utest_sendfile_short_fork_ends_session(AFPObj *obj, struct vol *vol)
{
    return sf_short_fork_test(obj, vol, true);
}

/*!
 * @brief A buffered reply the resource fork cuts short ends the session as
 *        the sendfile reply does, with no attention behind it
 */
int utest_buffered_short_fork_ends_session(AFPObj *obj, struct vol *vol)
{
    return sf_short_fork_test(obj, vol, false);
}

/*! @brief A reply of 2 GiB or more on a 32-bit build keeps the session */
int utest_sendfile_large_reply_keeps_session(AFPObj *obj, struct vol *vol)
{
#ifndef SENDFILE_FLAVOR_LINUX
    /* the fault interposer fakes the Linux call alone */
    return TEST_SKIP;
#else
    const struct adouble *ad;
    struct sf sf;
    int ret;

    /* a 64-bit size_t carries the count whole */
    if (sizeof(size_t) > 4) {
        return TEST_SKIP;
    }

    if ((ret = sf_sendfile_setup(&sf, obj, vol, true)) != 0) {
        return ret;
    }

    ret = -1;

    if ((ad = sf_open(&sf, OPENFORK_DATA, SF_SMALL)) != NULL) {
        ret = sf_grow(ad, (off_t)SF_HUGE);
    }

    if (ret == 0) {
        /* a reply the interposer reports sent and moves none of: the reader
         * takes the header */
        fi.sendfile_fake = 1;

        if (sf_read(&sf, 0, SF_HUGE, DSI_BLOCKSIZ) != AFP_OK
                || !sf_header_is(&sf, AFP_OK, (uint32_t)SF_HUGE)
                || sf.exit_code != 0) {
            ret = -1;
        }

        fi.sendfile_fake = 0;
    }

    sf_teardown(&sf);
    return ret;
#endif
}

/*!
 * @brief An FPReadExt of 8 GiB on a fork of rlen bytes gets the largest reply
 *        DSI frames, with the code want: through sendfile or through the
 *        request buffer
 *
 * @returns 0, -1, or TEST_SKIP
 */
static int sf_read_ext_test(AFPObj *obj, struct vol *vol, bool sendfile,
                            off_t rlen, int want)
{
    const struct adouble *ad;
    struct sf sf;
    int ret = sendfile ? sf_sendfile_setup(&sf, obj, vol, false)
              : sf_buffered_setup(&sf, obj, vol);

    if (ret != 0) {
        return ret;
    }

    ret = -1;

    if ((ad = sf_open(&sf, OPENFORK_DATA, SF_SMALL)) != NULL) {
        ret = sf_grow(ad, rlen);
    }

    /* the reader takes the header */
    if (ret == 0) {
        int rc = sf_read(&sf, 0, (uint64_t)1 << 33, DSI_BLOCKSIZ);
        bool header = sf_header_is(&sf, want, UINT32_MAX);

        if (rc != want || !header) {
            fprintf(stderr, "# sf_read_ext_test: a fork of %jd bytes: "
                    "answer %d, %d due, header %s\n", (intmax_t)rlen, rc,
                    want, header ? "right" : "wrong");
            ret = -1;
        }
    }

    sf_teardown(&sf);
    return ret;
}

/*!
 * @brief FPReadExt of 8 GiB on a fork longer than DSI frames, then on one
 *        UINT32_MAX bytes long, through sendfile or the request buffer: the
 *        first reply stops short with AFP_OK, the second ends at the fork's
 *        end with AFPERR_EOF
 */
static int sf_read_ext_cases(AFPObj *obj, struct vol *vol, bool sendfile)
{
    int ret = sf_read_ext_test(obj, vol, sendfile, ((off_t)1 << 32) + 4096,
                               AFP_OK);
    return ret != 0 ? ret
           : sf_read_ext_test(obj, vol, sendfile, UINT32_MAX, AFPERR_EOF);
}

/*! @brief An FPReadExt of 8 GiB through sendfile gets the largest DSI reply */
int utest_sendfile_read_ext_fits_dsi_length(AFPObj *obj, struct vol *vol)
{
    return sf_read_ext_cases(obj, vol, true);
}

/*!
 * @brief An FPReadExt of 8 GiB through the request buffer gets the largest
 *        DSI reply
 */
int utest_buffered_read_ext_fits_dsi_length(AFPObj *obj, struct vol *vol)
{
    return sf_read_ext_cases(obj, vol, false);
}

/*!
 * @brief A file the kernel will not send from goes out through the request
 *        buffer after one refused call, from the offset asked for, whole and
 *        in step
 */
int utest_sendfile_refused_file_sent_from_buffer(AFPObj *obj, struct vol *vol)
{
#if !defined(SENDFILE_FLAVOR_LINUX) && !defined(SENDFILE_FLAVOR_FREEBSD)
    /* the fault interposer covers these two sendfile() calls */
    return TEST_SKIP;
#else
    struct sf sf;
    int ret;

    if ((ret = sf_sendfile_setup(&sf, obj, vol, true)) != 0) {
        return ret;
    }

    ret = -1;

    if (sf_open(&sf, OPENFORK_DATA, SF_LEN) != NULL) {
        fi.sendfile.armed = 1;
        fi.sendfile.fail_after = 0;
        fi.sendfile.errnum = EINVAL;

        if (sf_read(&sf, SF_ODD, SF_LEN - SF_ODD,
                    DSI_BLOCKSIZ + SF_LEN - SF_ODD + 1) == AFP_OK
                && fi.sendfile.calls == 1
                && sf_reply_whole(&sf, SF_ODD, SF_LEN - SF_ODD)) {
            ret = 0;
        }
    }

    sf_teardown(&sf);
    return ret;
#endif
}

/*!
 * @brief A virtual icon fork holds the one reference to its adouble, which
 *        closing the fork releases
 */
int utest_virtual_icon_fork_frees_adouble(AFPObj *obj, struct vol *vol)
{
    const struct ofork *of;
    struct vi_lent lent;
    uint16_t refnum = 0;
    int ret = -1;

    if (vi_lend(vol, &lent) == 0
            && openfork(obj, vol->v_vid, DIRDID_ROOT, VIRTUAL_ICON_NAME,
                        OPENFORK_RSCS, OPENACC_RD, &refnum) == AFP_OK
            && (of = of_find(refnum)) != NULL
            && of->of_ad->ad_refcount == 1) {
        ret = 0;
    }

    if (refnum != 0) {
        closefork(obj, refnum);
    }

    vi_return(vol, &lent);
    return ret;
}

/*!
 * @brief A virtual icon fork's read over DSI is answered from the reply
 *        buffer, with nothing streamed and no reply under way
 */
int utest_virtual_icon_read_from_reply_buffer(AFPObj *obj, struct vol *vol)
{
    size_t rbuflen = DSI_DATASIZ;
    struct vi_lent lent;
    struct sf sf;
    int ret = -1;

    if (sf_pair_setup(&sf, obj, vol) != 0) {
        return -1;
    }

    if (vi_lend(vol, &lent) == 0
            && openfork(&sf.obj, vol->v_vid, DIRDID_ROOT, VIRTUAL_ICON_NAME,
                        OPENFORK_RSCS, OPENACC_RD, &sf.refnum) == AFP_OK
            && sf_request(&sf, 0, SF_SHORT) == 0
            && afp_read_ext(&sf.obj, (char *)sf.dsi.commands, sf.dsi.cmdlen,
                            (char *)sf.dsi.data, &rbuflen) == AFP_OK
            && rbuflen == SF_SHORT
            && memcmp(sf.dsi.data, vol->v_icon_rfork, SF_SHORT) == 0
            && sf.dsi.in_write == 0
            && !(sf.dsi.flags & DSI_NOREPLY)
            && sf.exit_code == 0) {
        ret = 0;
    }

    sf_teardown(&sf);
    vi_return(vol, &lent);
    return ret;
}

/*!
 * @brief A session that a failed primary reconnect left serving ends when a
 *        read reply fails: the data the file had, then the end of the stream
 */
int utest_sendfile_reconnect_session_ends(AFPObj *obj, struct vol *vol)
{
    struct timeval wait = { .tv_sec = 10 };
    uint8_t frame[SF_FRAME + DSI_BLOCKSIZ + 1];
    const struct adouble *ad;
    struct sf sf;
    pid_t pid = -1;
    int peer = -1;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (sf_pair_setup(&sf, obj, vol) != 0) {
        return -1;
    }

    sf.cap = DSI_BLOCKSIZ + SF_SMALL + DSI_BLOCKSIZ;

    if ((ad = sf_open(&sf, OPENFORK_RSCS, SF_SMALL + SF_SHORT)) != NULL
            && (sf.got = malloc(sf.cap)) != NULL) {
        /* the session inherits the cut */
        sf_shrink_at_lock(&sf, ad, SF_SMALL);
        pid = dsi_test_serve_as(&sf.obj, 0, -1, DSI_RECONINPROG, &peer);

        if (pid == -1 && dsi_test_no_loopback(errno)) {
            ret = TEST_SKIP;
        }

        fi.shrink_armed = 0;
    }

    if (pid > 0) {
        /* the read, then a request only a session that goes on answers */
        sf_frame(&sf, frame, 0, SF_SMALL + SF_SHORT);
        dsi_test_frame(frame + SF_FRAME, DSIFUNC_CMD, 2, 0, 1, 0xff);
        close(sf.peer);
        sf.peer = peer;

        if (setsockopt(sf.peer, SOL_SOCKET, SO_RCVTIMEO, &wait,
                       sizeof(wait)) == 0
                && write(sf.peer, frame, sizeof(frame)) == sizeof(frame)) {
            sf_reader(&sf);
        }

        if (dsi_test_reap(pid, sf.peer) == 0
                && sf.eof
                && sf_header_is(&sf, AFP_OK, SF_SMALL + SF_SHORT)
                && sf_payload_from(&sf, 0, SF_SMALL)) {
            ret = 0;
        }

        sf.peer = -1;
    }

    sf_teardown(&sf);
    return ret;
}

/* a command number afpd's handler table leaves unused */
#define SF_SPARE_COMMAND 0xfe

/*!
 * @brief A handler that returns with its streamed reply broken off, as no
 *        handler of afpd's does: the reply under way, nothing sent
 */
static int sf_broken_reply(AFPObj *obj, char *ibuf _U_, size_t ibuflen _U_,
                           char *rbuf _U_, size_t *rbuflen)
{
    *rbuflen = 0;
    obj->dsi->flags |= DSI_NOREPLY;
    obj->dsi->in_write++;
    return AFP_OK;
}

/*!
 * @brief A handler that returns with its streamed reply broken off ends the
 *        session: the next request gets no answer, and no attention or
 *        CloseSession frame follows the reply
 */
int utest_dsi_broken_reply_ends_session(AFPObj *obj, struct vol *vol _U_)
{
    struct timeval wait = { .tv_sec = 10 };
    uint8_t frame[3 * (DSI_BLOCKSIZ + 1)];
    uint8_t got[DSI_BLOCKSIZ];
    int peer = -1;
    int ret = -1;
    pid_t pid;
    /* the session process copies the table with the handler in it */
    postauth_switch[SF_SPARE_COMMAND] = sf_broken_reply;
    pid = dsi_test_serve(obj, &peer);
    postauth_switch[SF_SPARE_COMMAND] = NULL;

    if (pid == -1) {
        return dsi_test_no_loopback(errno) ? TEST_SKIP : -1;
    }

    /* a request the session answers, the broken reply, then a request only a
     * session that goes on answers */
    dsi_test_frame(frame, DSIFUNC_CMD, 1, 0, 1, 0xff);
    dsi_test_frame(frame + DSI_BLOCKSIZ + 1, DSIFUNC_CMD, 2, 0, 1,
                   SF_SPARE_COMMAND);
    dsi_test_frame(frame + 2 * (DSI_BLOCKSIZ + 1), DSIFUNC_CMD, 3, 0, 1, 0xff);

    if (setsockopt(peer, SOL_SOCKET, SO_RCVTIMEO, &wait, sizeof(wait)) == 0
            && write(peer, frame, sizeof(frame)) == sizeof(frame)
            && readt(peer, got, DSI_BLOCKSIZ, 1, 10) == DSI_BLOCKSIZ
            && read(peer, got, sizeof(got)) == 0) {
        ret = 0;
    }

    return dsi_test_reap(pid, peer) == 0 ? ret : -1;
}

/*!
 * @brief A session that a failed primary reconnect left serving ends at
 *        SIGTERM as any other: its shutdown attention, then its CloseSession
 *        frame
 */
int utest_reconnect_sigterm_sends_attention(AFPObj *obj, struct vol *vol _U_)
{
    uint8_t frame[DSI_BLOCKSIZ + 1];
    uint8_t got[2 * DSI_BLOCKSIZ + 2];
    uint16_t shutdown_be = htons(AFPATTN_SHUTDOWN);
    int peer = -1;
    pid_t pid = dsi_test_serve_as(obj, 0, -1, DSI_RECONINPROG, &peer);
    int ret = -1;

    if (pid == -1) {
        return dsi_test_no_loopback(errno) ? TEST_SKIP : -1;
    }

    /* a request only a running session answers, its handlers set */
    dsi_test_frame(frame, DSIFUNC_CMD, 1, 0, 1, 0xff);

    if (write(peer, frame, sizeof(frame)) == sizeof(frame)
            && readt(peer, got, DSI_BLOCKSIZ, 1, 10) == DSI_BLOCKSIZ
            && kill(pid, SIGTERM) == 0
            && readt(peer, got, sizeof(got), 1, 10) == sizeof(got)
            && got[0] == DSIFL_REQUEST && got[1] == DSIFUNC_ATTN
            && memcmp(got + DSI_BLOCKSIZ, &shutdown_be,
                      sizeof(shutdown_be)) == 0
            && got[DSI_BLOCKSIZ + 2] == DSIFL_REQUEST
            && got[DSI_BLOCKSIZ + 3] == DSIFUNC_CLOSE) {
        ret = 0;
    }

    return dsi_test_reap(pid, peer) == 0 ? ret : -1;
}

/*!
 * @brief Append len bytes of sf_byte() at off to the resource fork of
 *        sf_file through an adouble of its own, as another session's afpd
 *        writes
 *
 * @returns 0, or -1
 */
static int sf_append_elsewhere(const struct sf *sf, off_t off, size_t len)
{
    const int flags = ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF | ADFLAGS_RDWR;
    static char bytes[SF_SMALL];
    char path[MAXPATHLEN + 1];
    struct adouble ad;
    int ret = -1;
    snprintf(path, sizeof(path), "%s/%s", sf->vol->v_path, sf_file);

    for (size_t i = 0; i < len; i++) {
        bytes[i] = (char)sf_byte((size_t)off + i);
    }

    ad_init(&ad, sf->vol);

    if (ad_open(&ad, path, flags, 0) == 0) {
        if (ad_write(&ad, ADEID_RFORK, off, 0, bytes, len) == (ssize_t)len
                && ad_flush(&ad) == 0) {
            ret = 0;
        }

        ad_close(&ad, flags);
    }

    return ret;
}

/*!
 * @brief A resource fork another session grows while this one holds it open
 *        read-only, the dircache keeping the length it had, is read whole at
 *        its new length
 */
int utest_rfork_cache_held_open_append_read_whole(AFPObj *obj,
                                                  struct vol *vol)
{
    struct sf sf;
    int ret = -1;

    if (sf_buffered_setup(&sf, obj, vol) != 0) {
        return -1;
    }

    /* the write arrives with no cache hint, its writer's fork still open */
    if (sf_open(&sf, OPENFORK_RSCS, SF_SHORT) != NULL
            && sf_reopen_cached(&sf, SF_SHORT) != NULL
            && sf_append_elsewhere(&sf, SF_SHORT, SF_SMALL - SF_SHORT) == 0
            && sf_read(&sf, 0, SF_SMALL, DSI_BLOCKSIZ + SF_SMALL + 1) == AFP_OK
            && sf_reply_whole(&sf, 0, SF_SMALL)) {
        ret = 0;
    }

    sf_teardown(&sf);
    return ret;
}

/*!
 * @brief A resource fork the dircache records at a length shorter than the
 *        file's, as another session's write leaves it, is read whole at the
 *        file's length
 */
int utest_rfork_cache_stale_length_read_whole(AFPObj *obj, struct vol *vol)
{
    struct sf sf;
    int ret = -1;

    if (sf_buffered_setup(&sf, obj, vol) != 0) {
        return -1;
    }

    /* the length recorded before another session grew the fork */
    if (sf_open(&sf, OPENFORK_RSCS, SF_SMALL) != NULL
            && sf_reopen_cached(&sf, SF_SHORT) != NULL
            && sf_read(&sf, 0, SF_SMALL, DSI_BLOCKSIZ + SF_SMALL + 1) == AFP_OK
            && sf_reply_whole(&sf, 0, SF_SMALL)) {
        ret = 0;
    }

    sf_teardown(&sf);
    return ret;
}

/*!
 * @brief Read the whole fork with a session quantum of half the request
 *        buffer: does the reply arrive whole, and the buffer past the quantum
 *        keep its fill?
 */
static bool sf_read_within_quantum(struct sf *sf)
{
    uint8_t *past = sf->dsi.commands + SF_HALF;
    sf->dsi.server_quantum = SF_HALF;
    memset(past, 0xee, DSI_DATASIZ - SF_HALF);

    if (sf_read(sf, 0, SF_SMALL, DSI_BLOCKSIZ + SF_SMALL + 1) != AFP_OK
            || !sf_reply_whole(sf, 0, SF_SMALL)) {
        return false;
    }

    for (size_t i = 0; i < DSI_DATASIZ - SF_HALF; i++) {
        if (past[i] != 0xee) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief A buffered read stages its pieces inside the session's quantum of
 *        the request buffer
 */
int utest_buffered_read_stays_in_quantum(AFPObj *obj, struct vol *vol)
{
    struct sf sf;
    int ret = -1;

    if (sf_buffered_setup(&sf, obj, vol) != 0) {
        return -1;
    }

    if (sf_open(&sf, OPENFORK_DATA, SF_SMALL) != NULL
            && sf_read_within_quantum(&sf)) {
        ret = 0;
    }

    sf_teardown(&sf);
    return ret;
}

/*!
 * @brief A read the rfork cache serves stages its pieces inside the session's
 *        quantum of the request buffer
 */
int utest_rfork_cache_read_stays_in_quantum(AFPObj *obj, struct vol *vol)
{
    const struct dir *entry;
    struct sf sf;
    int ret = -1;

    if (sf_buffered_setup(&sf, obj, vol) != 0) {
        return -1;
    }

    if (sf_open(&sf, OPENFORK_RSCS, SF_SMALL) != NULL
            && (entry = sf_reopen_cached(&sf, SF_SMALL)) != NULL
            && sf_read_within_quantum(&sf)
            && entry->dcache_rfork_buf != NULL) {
        ret = 0;
    }

    sf_teardown(&sf);
    return ret;
}

/*!
 * @brief A buffered read whose first piece another process's cut shortens
 *        replies with the bytes the fork holds and AFPERR_EOF, and the
 *        session goes on
 */
int utest_buffered_short_first_piece_replies(AFPObj *obj, struct vol *vol)
{
    const struct adouble *ad;
    struct sf sf;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (sf_buffered_setup(&sf, obj, vol) != 0) {
        return -1;
    }

    /* within a quantum */
    if ((ad = sf_open(&sf, OPENFORK_RSCS, SF_HALF + SF_SHORT)) != NULL) {
        sf_shrink_at_lock(&sf, ad, SF_HALF);

        if (sf_read(&sf, 0, SF_HALF + SF_SHORT,
                    DSI_BLOCKSIZ + SF_HALF + SF_SHORT + 1) == AFPERR_EOF
                && sf_header_is(&sf, AFPERR_EOF, SF_HALF)
                && sf_payload_from(&sf, 0, SF_HALF)
                && (sf.dsi.flags & DSI_NOREPLY)
                && sf.exit_code == 0 && sf.dsi.in_write == 0) {
            ret = 0;
        }
    }

    sf_teardown(&sf);
    return ret;
}

/*!
 * @brief A buffered FPRead whose read of the fork fails answers AFPERR_MISC,
 *        with nothing sent
 */
int utest_buffered_read_error_replies_misc(AFPObj *obj, struct vol *vol)
{
    size_t rbuflen = DSI_DATASIZ;
    struct sf sf;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (sf_buffered_setup(&sf, obj, vol) != 0) {
        return -1;
    }

    if (sf_open(&sf, OPENFORK_DATA, SF_SMALL) != NULL
            && sf_request(&sf, 0, SF_SHORT) == 0) {
        fi.read.armed = 1;
        fi.read.fail_after = 0;
        fi.read.errnum = EIO;

        if (afp_read_ext(&sf.obj, (char *)sf.dsi.commands, sf.dsi.cmdlen,
                         (char *)sf.dsi.data, &rbuflen) == AFPERR_MISC
                && rbuflen == 0 && fi.read.calls == 1
                && !(sf.dsi.flags & DSI_NOREPLY)
                && sf.dsi.in_write == 0 && sf.exit_code == 0) {
            ret = 0;
        }
    }

    sf_teardown(&sf);
    return ret;
}

#ifndef NO_DDP
/*!
 * @brief Run an AppleTalk FPRead for count bytes at offset of the session's
 *        fork, its reply into the request buffer
 *
 * @returns the AFP result, with the reply length in *rbuflen
 */
static int sf_asp_read(struct sf *sf, uint32_t offset, uint32_t count,
                       size_t *rbuflen)
{
    uint8_t req[12] = { AFP_READ };
    uint32_t offset_be = htonl(offset);
    uint32_t count_be = htonl(count);
    int ret;
    /* refnum travels raw, as afp_openfork() returns it */
    memcpy(req + 2, &sf->refnum, sizeof(sf->refnum));
    memcpy(req + 4, &offset_be, sizeof(offset_be));
    memcpy(req + 8, &count_be, sizeof(count_be));
    sf->obj.proto = AFPPROTO_ASP;
    ret = afp_read(&sf->obj, (char *)req, sizeof(req), (char *)sf->dsi.data,
                   rbuflen);
    sf->obj.proto = AFPPROTO_DSI;
    return ret;
}
#endif

/*!
 * @brief An AppleTalk FPRead under strict locking clears the range it
 *        locked, its whole ReqCount however little the reply buffer holds,
 *        and that range alone
 */
int utest_asp_read_unlocks_its_range(AFPObj *obj, struct vol *vol)
{
#ifdef NO_DDP
    return TEST_SKIP;
#else
    size_t rbuflen = SF_SHORT;
    struct sf sf;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (sf_pair_setup(&sf, obj, vol) != 0) {
        return -1;
    }

    if (sf_open(&sf, OPENFORK_DATA, SF_SMALL) != NULL) {
        sf.obj.options.flags |= OPTION_STRICT_LOCKING;
        fault_inject_reset();
        fi.fcntl_watch = 1;

        if (sf_asp_read(&sf, SF_ODD, SF_SMALL, &rbuflen) == AFP_OK
                && rbuflen == SF_SHORT && fi.setlk_calls == 2
                && fi.lock_last_type == F_UNLCK
                && fi.lock_last_start == SF_ODD
                && fi.lock_last_len == SF_SMALL) {
            ret = 0;
        }

        sf.obj.options.flags &= ~OPTION_STRICT_LOCKING;
    }

    sf_teardown(&sf);
    return ret;
#endif
}

/*!
 * @brief FPRead of a symlink's data fork from an offset returns the link
 *        target from that offset, and over AppleTalk nothing past its end
 */
int utest_symlink_read_from_offset(AFPObj *obj, struct vol *vol)
{
    static const char target[] = "verylonglinkname";
    const size_t off = 4;
    const size_t len = sizeof(target) - 1 - off;
    char path[MAXPATHLEN + 1];
    struct sf sf;
    int ret = -1;

    if (sf_pair_setup(&sf, obj, vol) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, sf_file);

    if (symlink(target, path) == 0) {
        if (openfork(&sf.obj, vol->v_vid, DIRDID_ROOT, sf_file, OPENFORK_DATA,
                     OPENACC_RD, &sf.refnum) == AFP_OK
                && sf_read(&sf, off, len, DSI_BLOCKSIZ + len + 1) == AFP_OK
                && sf_header_is(&sf, AFP_OK, (uint32_t)len)
                && sf.gotlen == DSI_BLOCKSIZ + len
                && memcmp(sf.got + DSI_BLOCKSIZ, target + off, len) == 0
                && sf.exit_code == 0) {
            ret = 0;
        }

#ifndef NO_DDP
        size_t rbuflen = SF_SHORT;

        /* AppleTalk cuts no count at the fork's end */
        if (ret == 0 && (sf_asp_read(&sf, sizeof(target), SF_SHORT, &rbuflen)
                         != AFPERR_EOF || rbuflen != 0)) {
            ret = -1;
        }

#endif
    }

    sf_teardown(&sf);
    /* unlink(), not FPDelete, which Solaris and illumos refuse for the link */
    (void)unlink(path);
    return ret;
}
