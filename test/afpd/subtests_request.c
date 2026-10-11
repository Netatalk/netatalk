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
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/cnid.h>
#include <atalk/dsi.h>
#include <atalk/fce_api.h>
#include <atalk/globals.h>
#include <atalk/server_child.h>
#include <atalk/server_ipc.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afpfunc_helpers.h"
#include "desktop.h"
#include "dircache.h"
#include "directory.h"
#include "faultinject.h"
#include "fork.h"
#include "peer_lock.h"
#include "subtests_request.h"
#include "test.h"
#include "test_capabilities.h"
#include "volume.h"

#define FIO_QUANTUM 4096
#define RQ_LEN      3000

/*! A DSI session on one end of a socketpair, served by a copy of obj */
struct fio {
    AFPObj obj;
    DSI    dsi;
    int    peer;
};

static struct fio fio;
static char rq_data[RQ_LEN];

/*!
 * @brief obj->exit while the fixture is open: the session stays
 */
static void fio_exit(int code _U_)
{
}

/*!
 * @brief A FIO_QUANTUM session from dsi_test_open(), served by a copy of
 *        obj; an uncached read takes the buffered read_file() path
 *
 * @returns 0, or -1 with nothing left open
 */
static int fio_open(const AFPObj *obj)
{
    if (dsi_test_open(&fio.dsi, FIO_QUANTUM, &fio.peer) != 0) {
        return -1;
    }

    fio.obj = *obj;
    fio.obj.dsi = &fio.dsi;
    fio.obj.proto = AFPPROTO_DSI;
    fio.obj.options.flags |= OPTION_NOSENDFILE;
    fio.obj.exit = fio_exit;

    for (int i = 0; i < RQ_LEN; i++) {
        rq_data[i] = (char)(i * 7 + 1);
    }

    return 0;
}

/*!
 * @brief Send a frame carrying an FPReadExt or FPWriteExt request block
 *        from the peer, with len payload bytes when data is given, and
 *        receive it into the session
 *
 * @returns 0, or -1 when the session did not accept the frame
 */
static int fio_send(uint8_t dsi_cmd, uint8_t afp_cmd, uint8_t flag,
                    uint16_t refnum, uint64_t offset, uint32_t len,
                    const char *data)
{
    uint8_t frame[DSI_FRAME_OVERHEAD_EXT];
    uint8_t *p = frame + DSI_BLOCKSIZ;
    uint64_t offset_be = hton64(offset);
    uint64_t len_be = hton64((uint64_t)len);
    uint32_t dlen = data ? len : 0;
    dsi_test_header(frame, dsi_cmd, 1, data ? DSI_WROFF_FPWRITEEXT : 0,
                    DSI_WROFF_FPWRITEEXT + dlen);
    p[0] = afp_cmd;
    p[1] = flag;
    memcpy(p + 2, &refnum, sizeof(refnum));
    memcpy(p + 4, &offset_be, sizeof(offset_be));
    memcpy(p + 12, &len_be, sizeof(len_be));

    if (write(fio.peer, frame, sizeof(frame)) != (ssize_t)sizeof(frame)
            || (dlen && write(fio.peer, data, dlen) != (ssize_t)dlen)) {
        return -1;
    }

    return dsi_stream_receive(&fio.dsi) == dsi_cmd ? 0 : -1;
}

/*!
 * @brief FPWriteExt of len bytes of data at offset, or at EOF + offset with
 *        end set
 *
 * @returns the AFP result, or -1 when the frame was not accepted
 */
static int fio_write(uint16_t refnum, int end, uint64_t offset,
                     const char *data, uint32_t len)
{
    char rbuf[16];
    size_t rbuflen = 0;

    if (fio_send(DSIFUNC_WRITE, AFP_WRITE_EXT, end ? 0x80 : 0, refnum,
                 offset, len, data) != 0) {
        return -1;
    }

    return afp_write_ext(&fio.obj, (char *)fio.dsi.commands, fio.dsi.cmdlen,
                         rbuf, &rbuflen);
}

/*!
 * @brief FPReadExt of len bytes at offset; the reply's want data bytes
 *        land in out
 *
 * @returns the reply header's AFP result, what the client sees, or -1 when
 *          the frame was not accepted or the session is not as a reply
 *          read_fork() sends itself leaves it: want bytes under a reply
 *          header and nothing after them, the send finished and
 *          DSI_NOREPLY set
 */
static int fio_read(uint16_t refnum, uint64_t offset, uint32_t len,
                    uint32_t want, char *out)
{
    char block[DSI_BLOCKSIZ];
    char rbuf[16];
    char extra;
    size_t rbuflen = 0;
    uint32_t code;
    uint32_t dsi_len;

    if (fio_send(DSIFUNC_CMD, AFP_READ_EXT, 0, refnum, offset, len,
                 NULL) != 0) {
        return -1;
    }

    afp_read_ext(&fio.obj, (char *)fio.dsi.commands, fio.dsi.cmdlen, rbuf,
                 &rbuflen);

    if (readt(fio.peer, block, sizeof(block), 1, 1) != (ssize_t)sizeof(block)
            || readt(fio.peer, out, want, 1, 1) != (ssize_t)want
            || recv(fio.peer, &extra, 1, MSG_DONTWAIT) != -1
            || fio.dsi.in_write != 0 || !(fio.dsi.flags & DSI_NOREPLY)) {
        return -1;
    }

    /* the dispatcher clears it after a command that sent its own reply */
    fio.dsi.flags &= ~DSI_NOREPLY;
    memcpy(&code, block + 4, sizeof(code));
    memcpy(&dsi_len, block + 8, sizeof(dsi_len));

    if (block[0] != DSIFL_REPLY || ntohl(dsi_len) != want) {
        return -1;
    }

    return (int)ntohl(code);
}

/*!
 * @brief A new file with its data fork open read-write in *refnum
 *
 * @returns 0, or -1
 */
static int fio_dfork(const struct vol *vol, const char *name,
                     uint16_t *refnum)
{
    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK) {
        return -1;
    }

    return openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                    OPENACC_RD | OPENACC_WR, refnum) == AFP_OK ? 0 : -1;
}

/*!
 * @brief A new file whose resource fork holds rq_data, with the fork's
 *        length in the file's dircache entry and the fork open read-only
 *        in *refnum
 *
 * @returns 0, or -1
 */
static int fio_rfork(const struct vol *vol, char *name, uint16_t *refnum)
{
    const struct dir *entry;
    char *uname;
    uint16_t w;
    int rc;

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK) {
        return -1;
    }

    rc = fio_write(w, 0, 0, rq_data, RQ_LEN);

    /* the dircache's name index holds the name on disk */
    if (closefork(&fio.obj, w) != AFP_OK || rc != AFP_OK
            || (uname = mtoupath(vol, name, DIRDID_ROOT,
                                 utf8_encoding(&fio.obj))) == NULL
            || (entry = dircache_search_by_name(vol, vol->v_root, uname,
                                                strlen(uname))) == NULL
            || entry->dcache_rlen != RQ_LEN) {
        return -1;
    }

    return openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                    OPENACC_RD, refnum) == AFP_OK ? 0 : -1;
}

/*!
 * @brief Close refnum when open, delete name and release the session
 */
static void fio_done(const struct vol *vol, uint16_t refnum, const char *name)
{
    if (refnum) {
        closefork(&fio.obj, refnum);
    }

    delete (&fio.obj, vol->v_vid, DIRDID_ROOT, name);
    dir_free_invalid_q();
    dsi_test_cleanup(&fio.dsi);
    close(fio.peer);
}

/*!
 * @brief The data fork's size is read only for an EOF-relative write and
 *        on a volume with a size limit, where only appended bytes count
 */
int utest_write_reads_size_only_when_used(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_dfork";
    /* a plain write, one relative to EOF, then an append and a rewrite in
     * place on a size-limited volume */
    static const struct {
        int end;
        uint64_t offset;
        VolSpace limit;
    } step[4] = { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 128, 1 }, { 0, 0, 1 } };
    struct stat st;
    VolSpace saved_appended = vol->v_appended;
    VolSpace saved_limit = vol->v_limitsize;
    VolSpace appended;
    VolSpace grown[4] = { 0, 0, 0, 0 };
    int calls[4] = { -1, -1, -1, -1 };
    uint16_t w = 0;
    int probe;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (fio_dfork(vol, name, &w) != 0) {
        goto out;
    }

    fault_inject_reset();
    fi.fstat_fd = ad_data_fileno(of_find(w)->of_ad);
    fi.fstat_watch = 1;
    /* positive control: the interposer sees an fstat() of the watched fd */
    (void)fstat(fi.fstat_fd, &st);
    probe = fi.fstat_calls;

    for (int i = 0; i < 4; i++) {
        fi.fstat_calls = 0;
        appended = vol->v_appended;
        vol->v_limitsize = step[i].limit;

        if (fio_write(w, step[i].end, step[i].offset, rq_data, 64) == AFP_OK) {
            calls[i] = fi.fstat_calls;
            grown[i] = vol->v_appended - appended;
        }
    }

    vol->v_limitsize = saved_limit;
    vol->v_appended = saved_appended;
    fault_inject_reset();

    if (probe == 0) {
        ret = TEST_SKIP;
    } else if (calls[0] == 0 && calls[1] == 1 && calls[2] == 1
               && calls[3] == 1 && grown[0] == 0 && grown[1] == 0
               && grown[2] == 64 && grown[3] == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "write: fstat %d/%d/%d/%d, appended %ju/%ju/%ju/%ju\n",
                calls[0], calls[1], calls[2], calls[3], (uintmax_t)grown[0],
                (uintmax_t)grown[1], (uintmax_t)grown[2], (uintmax_t)grown[3]);
        ret = 1;
    }

out:
    fio_done(vol, w, name);
    return ret;
}

/*!
 * @brief A size-limited volume's usage scan replaces the bytes counted as
 *        appended before it, which the next cached read does not add again
 */
int utest_volume_scan_resets_appended(AFPObj *obj, struct vol *vol)
{
    char req[6] = { 0 };
    char rbuf[64];
    size_t rbuflen;
    uint16_t bitmap = htons(1 << VOLPBIT_XBFREE);
    VolSpace saved_limit = vol->v_limitsize;
    VolSpace used[2] = { 0, 0 };
    int rc[2];
    memcpy(req + 2, &vol->v_vid, sizeof(vol->v_vid));
    memcpy(req + 4, &bitmap, sizeof(bitmap));
    vol->v_limitsize = 1;
    vol->v_appended = 1000;
    /* the first call scans the volume, the second reads the cached result */
    vol->v_tm_cachetime = 0;

    for (int i = 0; i < 2; i++) {
        rbuflen = sizeof(rbuf);
        rc[i] = afp_getvolparams(obj, req, sizeof(req), rbuf, &rbuflen);
        used[i] = vol->v_tm_used;
    }

    vol->v_limitsize = saved_limit;
    vol->v_appended = 0;
    vol->v_tm_cachetime = 0;

    if (rc[0] == AFP_OK && rc[1] == AFP_OK && used[0] == 0 && used[1] == 0) {
        return 0;
    }

    fprintf(stderr, "volume scan: %d/%d, used %ju/%ju\n", rc[0], rc[1],
            (uintmax_t)used[0], (uintmax_t)used[1]);
    return 1;
}

/*!
 * @brief A write whose end passes the largest 64-bit offset fails as a
 *        full disk and leaves the fork as it was
 */
int utest_write_end_past_limit_is_disk_full(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_dend";
    uint16_t w = 0;
    int ret = -1;
    int rc;

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (fio_dfork(vol, name, &w) != 0) {
        goto out;
    }

    rc = fio_write(w, 0, (uint64_t)INT64_MAX - 15, rq_data, 64);

    if (rc == AFPERR_DFULL && ad_size(of_find(w)->of_ad, ADEID_DFORK) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "write end: result %d\n", rc);
        ret = 1;
    }

out:
    fio_done(vol, w, name);
    return ret;
}

/*!
 * @brief A cache hit is sent from the cache buffer: the reply is the
 *        fork's bytes from the read's offset, and neither the command
 *        buffer past the request nor the reply buffer is written
 */
int utest_rfork_cache_hit_sends_cached_bytes(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rfork";
    static const uint64_t off[2] = { 0, 1000 };
    char out[RQ_LEN];
    unsigned long long added = rfork_stat_added;
    unsigned long long hits = rfork_stat_hits;
    uint16_t r = 0;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (fio_rfork(vol, name, &r) != 0) {
        goto out;
    }

    /* the first read fills the cache, the second is a hit at an offset */
    for (int pass = 0; pass < 2; pass++) {
        memset(fio.dsi.commands, 0xAA, FIO_QUANTUM);
        memset(fio.dsi.data, 0xAA, sizeof(fio.dsi.data));

        if (fio_read(r, off[pass], RQ_LEN - off[pass], RQ_LEN - off[pass],
                     out) != AFP_OK
                || memcmp(out, rq_data + off[pass], RQ_LEN - off[pass]) != 0) {
            fprintf(stderr, "rfork hit: pass %d: reply\n", pass);
            goto out;
        }

        for (int i = DSI_WROFF_FPWRITEEXT; i < FIO_QUANTUM; i++) {
            if (fio.dsi.commands[i] != 0xAA) {
                fprintf(stderr, "rfork hit: pass %d: commands[%d] written\n",
                        pass, i);
                ret = 1;
                goto out;
            }
        }

        for (size_t i = 0; i < sizeof(fio.dsi.data); i++) {
            if (fio.dsi.data[i] != 0xAA) {
                fprintf(stderr, "rfork hit: pass %d: data[%zu] written\n",
                        pass, i);
                ret = 1;
                goto out;
            }
        }
    }

    if (rfork_stat_added == added + 1 && rfork_stat_hits == hits + 2) {
        ret = 0;
    } else {
        fprintf(stderr, "rfork hit: added +%llu, hits +%llu\n",
                rfork_stat_added - added, rfork_stat_hits - hits);
        ret = 1;
    }

out:
    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief A cached resource fork length the fork has outgrown, from no
 *        fork or from a shorter one, is dropped, and the read is served
 *        from the fork
 */
int utest_rfork_cache_stale_length_reads_fork(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rstale";
    /* cached before another session wrote the fork, and before it grew */
    static const off_t stale[2] = { AD_RLEN_NO_RFORK, 1000 };
    char out[RQ_LEN];
    struct dir *entry;
    unsigned long long misses;
    uint16_t r = 0;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (fio_rfork(vol, name, &r) != 0
            || (entry = dircache_search_by_name(vol, vol->v_root, name,
                                                strlen(name))) == NULL) {
        goto out;
    }

    for (int i = 0; i < 2; i++) {
        entry->dcache_rlen = stale[i];
        misses = rfork_stat_misses;

        if (fio_read(r, 0, RQ_LEN, RQ_LEN, out) != AFP_OK
                || memcmp(out, rq_data, RQ_LEN) != 0) {
            fprintf(stderr, "rfork stale %lld: reply\n", (long long)stale[i]);
            ret = 1;
            goto out;
        }

        if (entry->dcache_rlen != AD_RLEN_UNKNOWN
                || rfork_stat_misses != misses + 1) {
            fprintf(stderr, "rfork stale %lld: rlen %lld, misses +%llu\n",
                    (long long)stale[i], (long long)entry->dcache_rlen,
                    rfork_stat_misses - misses);
            ret = 1;
            goto out;
        }
    }

    ret = 0;
out:
    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief A resource fork whose name the client sends decomposed is served
 *        from the cache, and the session's own write drops what it cached
 */
int utest_rfork_cache_finds_name_on_disk(AFPObj *obj, struct vol *vol)
{
    /* "rq_café" with the accent as a combining character, as AFP 3 sends it */
    static char name[] = "rq_cafe\xcc\x81";
    char out[RQ_LEN];
    unsigned long long added = rfork_stat_added;
    unsigned long long hits = rfork_stat_hits;
    uint16_t r = 0;
    uint16_t w = 0;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (fio_rfork(vol, name, &r) != 0) {
        goto out;
    }

    for (int pass = 0; pass < 2; pass++) {
        if (fio_read(r, 0, RQ_LEN, RQ_LEN, out) != AFP_OK
                || memcmp(out, rq_data, RQ_LEN) != 0) {
            fprintf(stderr, "rfork name: pass %d: reply\n", pass);
            goto out;
        }
    }

    /* the session's own write of other bytes, through a second fork */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || fio_write(w, 0, 0, rq_data + 1000, 1000) != AFP_OK
            || fio_read(r, 0, 1000, 1000, out) != AFP_OK) {
        fprintf(stderr, "rfork name: write and read back\n");
        goto out;
    }

    if (rfork_stat_added == added + 1 && rfork_stat_hits == hits + 2
            && memcmp(out, rq_data + 1000, 1000) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "rfork name: added +%llu, hits +%llu, %s bytes\n",
                rfork_stat_added - added, rfork_stat_hits - hits,
                memcmp(out, rq_data + 1000, 1000) == 0 ? "new" : "old");
        ret = 1;
    }

out:

    if (w) {
        closefork(&fio.obj, w);
    }

    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief The parent buffers one refresh hint per sender and file in a
 *        batch, and a hint for another file beside it
 */
int utest_hint_relay_drops_repeats(AFPObj *obj, struct vol *vol)
{
    server_child_t *children = server_child_alloc(3);
    AFPObj sender = *obj;
    int fds[2] = { -1, -1 };
    int queued = -1;
    int ret = -1;

    if (children != NULL && socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
        /* the relay skips a parent with no session but the sender */
        children->servch_count = 2;
        sender.ipc_fd = fds[1];
        ipc_send_cache_hint(&sender, vol->v_vid, htonl(100),
                            CACHE_HINT_REFRESH);
        ipc_send_cache_hint(&sender, vol->v_vid, htonl(100),
                            CACHE_HINT_REFRESH);
        ipc_send_cache_hint(&sender, vol->v_vid, htonl(101),
                            CACHE_HINT_REFRESH);

        if (ipc_server_read(children, fds[0]) == 0
                && ipc_server_read(children, fds[0]) == 0
                && ipc_server_read(children, fds[0]) == 0) {
            queued = hint_buf_count();
        }

        /* no child is registered, so the flush only empties the buffer */
        hint_flush_pending(children);
        ret = queued == 2 ? 0 : 1;
    }

    if (ret == 1) {
        fprintf(stderr, "hint relay: %d hints buffered\n", queued);
    }

    if (fds[0] >= 0) {
        close(fds[0]);
        close(fds[1]);
    }

    if (children != NULL) {
        server_child_free(children);
    }

    return ret;
}

/*!
 * @brief What waits at the parent's end of the session's IPC socket
 *
 * @returns 0 for nothing, 1 for one REFRESH hint for id on vid, -1 for
 *          anything else
 */
static int hint_take(int fd, uint16_t vid, cnid_t id)
{
    char msg[2 * (IPC_HEADERLEN + sizeof(struct ipc_cache_hint_payload))];
    struct ipc_cache_hint_payload hint;
    uint16_t cmd;
    ssize_t n = read(fd, msg, sizeof(msg));

    if (n < 0) {
        return errno == EAGAIN || errno == EWOULDBLOCK ? 0 : -1;
    }

    if (n != (ssize_t)(IPC_HEADERLEN + sizeof(hint))) {
        return -1;
    }

    memcpy(&cmd, msg, sizeof(cmd));
    memcpy(&hint, msg + IPC_HEADERLEN, sizeof(hint));
    return cmd == IPC_CACHE_HINT && hint.event == CACHE_HINT_REFRESH
           && hint.vid == vid && hint.cnid == id ? 1 : -1;
}

/*!
 * @brief A fork's first change, a write that fails part way included, tells
 *        sibling sessions at once and its close tells them again; later
 *        writes wait for the close
 */
int utest_fork_first_change_hints_siblings(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_dhint";
    struct rlimit saved;
    struct rlimit limit;
    void (*xfsz)(int);
    int fds[2] = { -1, -1 };
    int got[5] = { -1, -1, -1, -1, -1 };
    uint16_t w = 0;
    cnid_t id = CNID_INVALID;
    int ret = -1;
    int rc = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0
            || setnonblock(fds[0], 1) != 0
            || fio_dfork(vol, name, &w) != 0
            || (id = cnid_get(vol->v_cdb, DIRDID_ROOT, name,
                              strlen(name))) == CNID_INVALID
            || getrlimit(RLIMIT_FSIZE, &saved) != 0) {
        goto out;
    }

    /* the session's IPC socket: what it sends its parent arrives at fds[0] */
    fio.obj.ipc_fd = fds[1];
    /* the file size limit cuts the write short after its first 1024 bytes */
    limit = saved;
    limit.rlim_cur = 1024;
    xfsz = signal(SIGXFSZ, SIG_IGN);

    if (setrlimit(RLIMIT_FSIZE, &limit) == 0) {
        rc = fio_write(w, 0, 0, rq_data, RQ_LEN);
        setrlimit(RLIMIT_FSIZE, &saved);
    }

    signal(SIGXFSZ, xfsz);

    if (rc == AFPERR_DFULL) {
        got[0] = hint_take(fds[0], vol->v_vid, id);
    }

    if (fio_write(w, 0, 0, rq_data, 64) == AFP_OK) {
        got[1] = hint_take(fds[0], vol->v_vid, id);
    }

    if (closefork(&fio.obj, w) == AFP_OK) {
        got[2] = hint_take(fds[0], vol->v_vid, id);
    }

    w = 0;

    /* a length set with no write after it: the second open's first change */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD | OPENACC_WR, &w) == AFP_OK
            && setforkparams(&fio.obj, w, 32) == AFP_OK) {
        got[3] = hint_take(fds[0], vol->v_vid, id);
    }

    if (w && closefork(&fio.obj, w) == AFP_OK) {
        got[4] = hint_take(fds[0], vol->v_vid, id);
    }

    w = 0;

    if (got[0] == 1 && got[1] == 0 && got[2] == 1 && got[3] == 1
            && got[4] == 1) {
        ret = 0;
    } else {
        fprintf(stderr, "fork hints: %d/%d/%d/%d/%d\n", got[0], got[1],
                got[2], got[3], got[4]);
        ret = 1;
    }

out:
    fio.obj.ipc_fd = -1;

    if (fds[0] >= 0) {
        close(fds[0]);
        close(fds[1]);
    }

    fio_done(vol, w, name);
    return ret;
}

/*!
 * @brief The cache is not filled while another session has the fork open
 *        for writing, and is once that session has let go
 */
int utest_rfork_cache_skips_fill_while_written(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rwopen";
    char path[MAXPATHLEN + 1];
    char out[RQ_LEN];
    struct peer writer;
    unsigned long long base = rfork_stat_added;
    unsigned long long added[2] = { 0, 0 };
    uint16_t r = 0;
    int ret = -1;
    int ok;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    /* another session's open for writing: its lock in the share-mode band */
    if (fio_rfork(vol, name, &r) != 0
            || peer_hold_lock(&writer, path, F_RDLCK,
                              AD_FILELOCK_RSRC_OPEN_WR, 1) != 0) {
        goto out;
    }

    ok = fio_read(r, 0, RQ_LEN, RQ_LEN, out) == AFP_OK
         && memcmp(out, rq_data, RQ_LEN) == 0;
    added[0] = rfork_stat_added - base;
    peer_release(&writer);
    ok = ok && fio_read(r, 0, RQ_LEN, RQ_LEN, out) == AFP_OK
         && memcmp(out, rq_data, RQ_LEN) == 0;
    added[1] = rfork_stat_added - base;

    if (!ok) {
        fprintf(stderr, "rfork writer: reply\n");
    } else if (added[0] == 0 && added[1] == 1) {
        ret = 0;
    } else {
        fprintf(stderr, "rfork writer: added %llu/%llu\n", added[0],
                added[1]);
        ret = 1;
    }

out:
    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief The rfork cache is looked up for a read inside the fork, on a
 *        multi-protocol volume only when its operator set the budget
 */
int utest_rfork_cache_lookup_only_inside_fork(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rlook";
    /* volume flag, operator-set budget */
    static const struct {
        int multi;
        int optin;
    } step[3] = {
        { 0, 0 },
        { AFPVOL_MULTIPROTO, 1 },
        { AFPVOL_MULTIPROTO, 0 },
    };
    char out[RQ_LEN];
    char rbuf[16];
    size_t rbuflen = 0;
    int saved_flags = vol->v_flags;
    unsigned long long got[4] = { 9, 9, 9, 9 };
    unsigned long long lookups;
    uint16_t r = 0;
    int ok = 1;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (fio_rfork(vol, name, &r) != 0) {
        goto out;
    }

    for (int i = 0; i < 3; i++) {
        vol->v_flags = saved_flags | step[i].multi;
        fio.obj.options.dircache_rfork_budget_explicit = step[i].optin;
        lookups = rfork_stat_lookups;
        ok = ok && fio_read(r, 0, RQ_LEN, RQ_LEN, out) == AFP_OK
             && memcmp(out, rq_data, RQ_LEN) == 0;
        got[i] = rfork_stat_lookups - lookups;
    }

    /* a read at the fork's end replies AFPERR_EOF before any lookup */
    vol->v_flags = saved_flags;
    lookups = rfork_stat_lookups;
    ok = ok && fio_send(DSIFUNC_CMD, AFP_READ_EXT, 0, r, RQ_LEN, 64, NULL) == 0
         && afp_read_ext(&fio.obj, (char *)fio.dsi.commands, fio.dsi.cmdlen,
                         rbuf, &rbuflen) == AFPERR_EOF;
    got[3] = rfork_stat_lookups - lookups;

    if (!ok) {
        fprintf(stderr, "rfork lookup: reply\n");
    } else if (got[0] == 1 && got[1] == 1 && got[2] == 0 && got[3] == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "rfork lookup: %llu/%llu/%llu/%llu\n", got[0], got[1],
                got[2], got[3]);
        ret = 1;
    }

out:
    vol->v_flags = saved_flags;
    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief fce_register() keeps nothing of the path it resolves for the
 *        ignore list, for an event it ignores and for one it sends
 *
 * The valgrind run of the suite reports any block the calls leave.
 */
int utest_fce_register_keeps_no_path(AFPObj *obj, struct vol *vol)
{
#ifndef WITH_FCE
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char script[] = "/nonexistent";
    char ignored[MAXPATHLEN + 1];
    char *root = realpath_safe(vol->v_path);
    AFPObj fce = *obj;

    if (root == NULL) {
        return -1;
    }

    snprintf(ignored, sizeof(ignored), "%s/rq_fceign", root);

    if (mkdir(ignored, 0700) != 0) {
        free(root);
        return -1;
    }

    fce.fce_notify_script = script;
    fce.fce_ign_directories = ignored;
    fce.options.fce_fmodwait = 0;
    /* below the ignored directory: resolved, matched, not sent */
    fce_register(&fce, FCE_FILE_MODIFY, ignored, NULL);
    /* outside it: resolved, not matched, sent */
    fce_register(&fce, FCE_FILE_MODIFY, root, NULL);
    rmdir(ignored);
    free(root);
    return 0;
#endif
}

/*!
 * @brief The request epilogue reads the clock only while a close event
 *        is held, and sends the event once its hold time has passed
 */
int utest_fce_clock_read_only_when_held(AFPObj *obj, struct vol *vol)
{
#ifndef WITH_FCE
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char script[] = "/nonexistent";
    AFPObj fce = *obj;
    int probe;
    int idle;
    int held;
    int fired;
    int after;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    fault_inject_reset();
    fi.time_watch = 1;
    (void)time(NULL);
    probe = fi.time_calls;
    fi.time_calls = 0;
    fce.fce_notify_script = script;
    fce_pending_events(&fce);
    idle = fi.time_calls;
    /* a hold time: the modify event is held, not sent */
    fce.options.fce_fmodwait = 5;
    fce_register(&fce, FCE_FILE_MODIFY, "/held", NULL);
    fi.time_calls = 0;
    fce_pending_events(&fce);
    held = fi.time_calls;
    /* a hold time already passed: the epilogue sends the event */
    fce.options.fce_fmodwait = -1;
    fi.time_calls = 0;
    fce_pending_events(&fce);
    fired = fi.time_calls;
    fi.time_calls = 0;
    fce_pending_events(&fce);
    after = fi.time_calls;
    fault_inject_reset();

    /* positive control: the probe's own time() is counted */
    if (probe == 0) {
        return TEST_SKIP;
    }

    if (idle == 0 && held == 1 && fired >= 2 && after == 0) {
        return 0;
    }

    fprintf(stderr, "fce clock: idle %d, held %d, fired %d, after %d\n",
            idle, held, fired, after);
    return 1;
#endif
}

/*!
 * @brief Every reply piece the buffered path stages ends within the
 *        command buffer's quantum, the first and the loop's alike
 */
int utest_read_pieces_fit_command_buffer(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_dbig";
    char out[RQ_LEN];
    uint16_t w = 0;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (fio_dfork(vol, name, &w) != 0
            || fio_write(w, 0, 0, rq_data, RQ_LEN) != AFP_OK) {
        goto out;
    }

    /* three pieces of at most 1024 bytes; past the quantum stays 0xAA */
    fio.dsi.server_quantum = 1024;
    memset(fio.dsi.commands, 0xAA, FIO_QUANTUM);

    if (fio_read(w, 0, RQ_LEN, RQ_LEN, out) != AFP_OK
            || memcmp(out, rq_data, RQ_LEN) != 0) {
        fprintf(stderr, "read pieces: reply\n");
        goto out;
    }

    ret = 0;

    for (int i = 1024; i < FIO_QUANTUM; i++) {
        if (fio.dsi.commands[i] != 0xAA) {
            fprintf(stderr, "read pieces: commands[%d] written\n", i);
            ret = 1;
            break;
        }
    }

out:
    fio_done(vol, w, name);
    return ret;
}

/*!
 * @brief A buffered read within one quantum of a fork shorter than its
 *        ad_size() replies with the bytes read and AFPERR_EOF
 */
int utest_read_short_fork_sends_bytes_read(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rshort";
    char out[RQ_LEN];
    uint16_t r = 0;
    int ret = -1;
    int rc;

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (fio_rfork(vol, name, &r) != 0) {
        goto out;
    }

    /* a length left stale by another writer's shrink; no cache, so the
     * read goes to the fork */
    of_find(r)->of_ad->ad_rlen = RQ_LEN + 100;
    fio.obj.options.dircache_rfork_budget = 0;
    rc = fio_read(r, 0, RQ_LEN + 100, RQ_LEN, out);
    of_find(r)->of_ad->ad_rlen = RQ_LEN;

    if (rc == AFPERR_EOF && memcmp(out, rq_data, RQ_LEN) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "short read: result %d\n", rc);
        ret = 1;
    }

out:
    fio_done(vol, r, name);
    return ret;
}
