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
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
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
#include "subtests_sendfile.h"
#include "test.h"
#include "test_capabilities.h"

#define SF_LEN   (1024 * 1024)
#define SF_SMALL (64 * 1024)
#define SF_SHORT 64
#define SF_BUF   16384

/*! A DSI session on loopback TCP, one fork, and a reader on the peer */
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
    int exit_in_write;
};

/*! The session whose read path calls sf_exit() */
static struct sf *sf_session;

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
 * @brief Serve a copy of obj over loopback TCP with sendfile on and a small
 *        send buffer, so a reply takes many calls
 *
 * @returns 0, -1, or TEST_SKIP in a sandbox without loopback
 */
static int sf_setup(struct sf *sf, AFPObj *obj, struct vol *vol)
{
    int sndbuf = SF_BUF;
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

    /* no loopback: a sandbox without a network namespace of its own */
    if (dsi_test_connect_tcp(&sf->dsi, &sf->peer) != 0) {
        sf_teardown(sf);
        return TEST_SKIP;
    }

    if (setsockopt(sf->dsi.socket, SOL_SOCKET, SO_SNDBUF, &sndbuf,
                   sizeof(sndbuf)) != 0) {
        sf_teardown(sf);
        return -1;
    }

    return 0;
}

/*! @brief sf_setup() for a test of the sendfile path; TEST_SKIP without it */
static int sf_sendfile_setup(struct sf *sf, AFPObj *obj, struct vol *vol)
{
#ifndef WITH_SENDFILE
    return TEST_SKIP;
#else
    return sf_setup(sf, obj, vol);
#endif
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
    sf->name = "sf_file";

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
 * @brief Send FPReadExt for count bytes at offset as a client does, and run
 *        it as afp_over_dsi() does while the reader takes up to cap bytes of
 *        the reply; the stream ends after it
 *
 * @returns the AFP result, or -1 when the request goes astray
 */
static int sf_read(struct sf *sf, uint64_t offset, uint64_t count, size_t cap)
{
    uint8_t frame[DSI_BLOCKSIZ + 20];
    uint8_t *p = frame + DSI_BLOCKSIZ;
    uint64_t offset_be = hton64(offset);
    uint64_t count_be = hton64(count);
    size_t rbuflen = DSI_DATASIZ;
    int ret;
    dsi_test_header(frame, DSIFUNC_CMD, 1, 0, 20);
    p[0] = AFP_READ_EXT;
    p[1] = 0;
    /* refnum travels raw, as afp_openfork() returns it */
    memcpy(p + 2, &sf->refnum, sizeof(sf->refnum));
    memcpy(p + 4, &offset_be, sizeof(offset_be));
    memcpy(p + 12, &count_be, sizeof(count_be));

    if (write(sf->peer, frame, sizeof(frame)) != sizeof(frame)
            || dsi_stream_receive(&sf->dsi) != DSIFUNC_CMD
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

/*! @brief Is everything after the header exactly len bytes of the fork? */
static bool sf_payload_is(const struct sf *sf, size_t len)
{
    if (sf->gotlen != DSI_BLOCKSIZ + len) {
        return false;
    }

    for (size_t i = 0; i < len; i++) {
        if (sf->got[DSI_BLOCKSIZ + i] != sf_byte(i)) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief A reply sent with sendfile through a small send buffer arrives
 *        whole and in step
 */
int utest_sendfile_read_lands_intact(AFPObj *obj, struct vol *vol)
{
    struct sf sf;
    int ret;

    if ((ret = sf_sendfile_setup(&sf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (sf_open(&sf, OPENFORK_DATA, SF_LEN) != NULL
            && sf_read(&sf, 0, SF_LEN, DSI_BLOCKSIZ + SF_LEN + 1) == AFP_OK
            && sf_header_is(&sf, AFP_OK, SF_LEN)
            && sf_payload_is(&sf, SF_LEN)
            && sf.exit_code == 0
            && sf.dsi.in_write == 0) {
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

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if ((ret = sf_sendfile_setup(&sf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (sf_open(&sf, OPENFORK_DATA, SF_SMALL) != NULL) {
        fi.send_armed = 1;
        fi.send_fail_after = 0;
        fi.send_errno = ECONNRESET;
        sf_read(&sf, 0, SF_SMALL, DSI_BLOCKSIZ + SF_SMALL + 1);

        if (fi.send_fail_after == -1
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

/*! @brief proto_close of a session the test closes itself */
static void sf_proto_close(DSI *dsi)
{
    close(dsi->socket);
    dsi->socket = -1;
}

/*!
 * @brief dsi_close() on a session with a reply under way closes the socket
 *        without its CloseSession frame
 */
int utest_dsi_close_sends_nothing_mid_reply(AFPObj *obj _U_,
                                            struct vol *vol _U_)
{
    DSI *dsi;
    int fds[2];
    char byte;
    int ret = -1;

    if ((dsi = calloc(1, sizeof(*dsi))) == NULL) {
        return -1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        free(dsi);
        return -1;
    }

    dsi->socket = fds[1];
    dsi->proto_close = sf_proto_close;
    dsi->in_write = 1;
    /* frees dsi */
    dsi_close(dsi);

    if (recv(fds[0], &byte, 1, 0) == 0) {
        ret = 0;
    }

    close(fds[0]);
    return ret;
}

/*!
 * @brief A resource fork shorter than its recorded length sends what it has
 *        and ends the session, the header having promised the rest
 */
int utest_sendfile_short_fork_ends_session(AFPObj *obj, struct vol *vol)
{
    struct adouble *ad;
    struct sf sf;
    int ret;

    if ((ret = sf_sendfile_setup(&sf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if ((ad = sf_open(&sf, OPENFORK_RSCS, SF_SMALL)) != NULL) {
        /* a length left stale by another writer's shrink */
        ad->ad_rlen = SF_SMALL + SF_SHORT;
        sf_read(&sf, 0, SF_SMALL + SF_SHORT,
                DSI_BLOCKSIZ + SF_SMALL + SF_SHORT + 1);
        ad->ad_rlen = SF_SMALL;

        if (sf_header_is(&sf, AFP_OK, SF_SMALL + SF_SHORT)
                && sf_payload_is(&sf, SF_SMALL)
                && sf.exit_code == EXITERR_CLNT
                && sf.exit_in_write == 1
                && sf.dsi.in_write == 0) {
            ret = 0;
        }
    }

    sf_teardown(&sf);
    return ret;
}

/*!
 * @brief Both reply entry points refuse a length the DSI header cannot
 *        carry before any byte goes out
 */
int utest_dsi_reply_refuses_unframed_length(AFPObj *obj, struct vol *vol)
{
    struct adouble *ad;
    struct sf sf;
    char byte;
    int ret;

    if (sizeof(size_t) < 8) {
        return TEST_SKIP;
    }

    if ((ret = sf_setup(&sf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if ((ad = sf_open(&sf, OPENFORK_DATA, 0)) != NULL) {
        size_t unframed = (size_t)UINT32_MAX + 1;
        bool refused = true;
#ifdef WITH_SENDFILE
        errno = 0;
        refused = dsi_stream_read_file(&sf.dsi, ad_data_fileno(ad), 0,
                                       unframed, AFP_OK) == -1
                  && errno == EMSGSIZE && sf.dsi.in_write == 1;
        dsi_readdone(&sf.dsi);
#endif
        errno = 0;

        if (refused
                && dsi_readinit(&sf.dsi, &byte, 0, unframed, AFP_OK) == -1
                && errno == EMSGSIZE && sf.dsi.in_write == 1
                && recv(sf.peer, &byte, 1, MSG_DONTWAIT) == -1
                && errno == EAGAIN) {
            ret = 0;
        }

        dsi_readdone(&sf.dsi);
    }

    sf_teardown(&sf);
    return ret;
}

/*!
 * @brief An FPReadExt of 8 GiB on a fork of 4 GiB and 4 KiB is cut to the
 *        largest reply DSI frames, which ends short of the fork's end
 */
int utest_sendfile_read_ext_fits_dsi_length(AFPObj *obj, struct vol *vol)
{
    struct adouble *ad;
    struct sf sf;
    int ret;

    if ((ret = sf_sendfile_setup(&sf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if ((ad = sf_open(&sf, OPENFORK_RSCS, SF_SMALL)) != NULL) {
        /* a recorded length the file lacks: the reader takes the header */
        ad->ad_rlen = ((off_t)1 << 32) + 4096;

        if (sf_read(&sf, 0, (uint64_t)1 << 33, DSI_BLOCKSIZ) == AFP_OK
                && sf_header_is(&sf, AFP_OK, UINT32_MAX)) {
            ret = 0;
        }

        ad->ad_rlen = SF_SMALL;
    }

    sf_teardown(&sf);
    return ret;
}

/*!
 * @brief A file the kernel will not send from goes out through the request
 *        buffer after one refused call, whole and in step
 */
int utest_sendfile_refused_file_sent_from_buffer(AFPObj *obj, struct vol *vol)
{
#if !defined(SENDFILE_FLAVOR_LINUX) && !defined(SENDFILE_FLAVOR_FREEBSD)
    /* the fault interposer covers these two sendfile() calls */
    return TEST_SKIP;
#else
    struct sf sf;
    int ret;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if ((ret = sf_sendfile_setup(&sf, obj, vol)) != 0) {
        return ret;
    }

    ret = -1;

    if (sf_open(&sf, OPENFORK_DATA, SF_LEN) != NULL) {
        fi.sendfile_armed = 1;
        fi.sendfile_fail_after = 0;
        fi.sendfile_errno = EINVAL;

        if (sf_read(&sf, 0, SF_LEN, DSI_BLOCKSIZ + SF_LEN + 1) == AFP_OK
                && fi.sendfile_calls == 1
                && sf_header_is(&sf, AFP_OK, SF_LEN)
                && sf_payload_is(&sf, SF_LEN)
                && sf.exit_code == 0
                && sf.dsi.in_write == 0) {
            ret = 0;
        }
    }

    sf_teardown(&sf);
    return ret;
#endif
}
