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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
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
#define FIO_FILL    0xAA
#define RQ_LEN      3000
/* FPWrite flag: the offset counts from the fork's end */
#define RQ_END_FLAG 0x80

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
    /* fio_done() ends the session */
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
 * @brief Send a frame from the peer and receive it into the session: an
 *        FPWriteExt request block followed by len payload bytes when data
 *        is given, else an FPReadExt request block
 *
 * @returns 0, or -1 when the session did not accept the frame
 */
static int fio_send(uint8_t flag, uint16_t refnum, uint64_t offset,
                    uint32_t len, const char *data)
{
    uint8_t frame[DSI_FRAME_OVERHEAD_EXT];
    uint8_t *p = frame + DSI_BLOCKSIZ;
    uint8_t dsi_cmd = data ? DSIFUNC_WRITE : DSIFUNC_CMD;
    uint64_t offset_be = hton64(offset);
    uint64_t len_be = hton64((uint64_t)len);
    uint32_t dlen = data ? len : 0;
    dsi_test_header(frame, dsi_cmd, 1, data ? DSI_WROFF_FPWRITEEXT : 0,
                    DSI_WROFF_FPWRITEEXT + dlen);
    p[0] = data ? AFP_WRITE_EXT : AFP_READ_EXT;
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
    size_t rbuflen = DSI_DATASIZ;

    if (fio_send(end ? RQ_END_FLAG : 0, refnum, offset, len, data) != 0) {
        return -1;
    }

    return afp_write_ext(&fio.obj, (char *)fio.dsi.commands, fio.dsi.cmdlen,
                         (char *)fio.dsi.data, &rbuflen);
}

/*!
 * @brief FPReadExt of len bytes at offset; the reply's want data bytes
 *        land in out
 *
 * @returns the reply header's AFP result, afp_read_ext()'s result when it
 *          refused the read without a reply, or -1 when the frame was not
 *          accepted or the session is not left as read_fork()'s own reply
 *          leaves it: want bytes under a reply header, nothing after them,
 *          the send finished and DSI_NOREPLY set
 */
static int fio_read(uint16_t refnum, uint64_t offset, uint32_t len,
                    uint32_t want, char *out)
{
    char block[DSI_BLOCKSIZ];
    char extra;
    size_t rbuflen = DSI_DATASIZ;
    uint32_t code;
    uint32_t dsi_len;
    int ret;

    if (fio_send(0, refnum, offset, len, NULL) != 0) {
        return -1;
    }

    ret = afp_read_ext(&fio.obj, (char *)fio.dsi.commands, fio.dsi.cmdlen,
                       (char *)fio.dsi.data, &rbuflen);

    /* a refused read sends no reply of its own */
    if (ret != AFP_OK && ret != AFPERR_EOF) {
        return ret;
    }

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
 * @brief The first index in [from, to) at which buf no longer holds
 *        FIO_FILL, or -1 when the range is untouched
 */
static long fio_touched(const uint8_t *buf, size_t from, size_t to)
{
    for (size_t i = from; i < to; i++) {
        if (buf[i] != FIO_FILL) {
            return (long)i;
        }
    }

    return -1;
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
 * @brief Another session's save over name: the file moves to aside with the
 *        metadata the volume's layout keeps beside it, and a new file whose
 *        resource fork holds RQ_LEN bytes of data takes the name
 *
 * @returns 0, 1 when the file moved but the replacement is incomplete (its
 *          metadata stayed, or the new file is missing or its resource fork
 *          was not written and closed), or -1 when nothing moved
 */
static int fio_replace(const struct vol *vol, const char *name,
                       const char *aside, const char *data)
{
    char from[MAXPATHLEN + 1];
    char to[MAXPATHLEN + 1];
    uint16_t w;
    int rc;
    snprintf(from, sizeof(from), "%s/%s", vol->v_path, name);
    snprintf(to, sizeof(to), "%s/%s", vol->v_path, aside);

    if (rename(from, to) != 0) {
        return -1;
    }

    if (vol->vfs->vfs_renamefile(vol, -1, from, to) != 0
            || createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK) {
        return 1;
    }

    rc = fio_write(w, 0, 0, data, RQ_LEN);
    return closefork(&fio.obj, w) == AFP_OK && rc == AFP_OK ? 0 : 1;
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

#ifdef WITH_FCE
/*!
 * @brief A copy of the session object with an FCE notify script, so that
 *        events pass the listener test
 */
static AFPObj fce_session(const AFPObj *obj)
{
    static char script[] = "/nonexistent";
    AFPObj fce = *obj;
    fce.options.fce_notify_script = script;
    return fce;
}
#endif /* WITH_FCE */

/*!
 * @brief The data fork's size is read only for an EOF-relative write and
 *        on a volume with a size limit, where only appended bytes count
 */
int utest_write_reads_size_only_when_used(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_dfork";
    /* a plain write, one relative to EOF, then a write across the end and a
     * rewrite in place on a size-limited volume */
    static const struct {
        int end;
        uint64_t offset;
        VolSpace limit;
    } step[4] = { { 0, 0, 0 }, { 1, 0, 0 }, { 0, 100, 1 }, { 0, 0, 1 } };
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
               && grown[2] == 36 && grown[3] == 0) {
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
 * @brief A sparse bundle that holds no band adds nothing to a size-limited
 *        volume's usage
 *
 * @returns 0, 1 when the scan reports usage, -1 when the fixture failed
 */
int utest_volume_scan_counts_no_band(AFPObj *obj, struct vol *vol)
{
    static const char plist[] =
        "<key>band-size</key>\n<integer>8388608</integer>\n";
    char bundle[MAXPATHLEN + 1];
    char bands[MAXPATHLEN + 1];
    char info[MAXPATHLEN + 1];
    VolSpace saved_limit = vol->v_limitsize;
    VolSpace used;
    FILE *f;
    int written;
    int ret = -1;
    int rc;
    snprintf(bundle, sizeof(bundle), "%s/rq_tm.sparsebundle", vol->v_path);
    snprintf(bands, sizeof(bands), "%s/bands", bundle);
    snprintf(info, sizeof(info), "%s/Info.plist", bundle);

    if (mkdir(bundle, 0700) != 0 || mkdir(bands, 0700) != 0
            || (f = fopen(info, "w")) == NULL) {
        goto out;
    }

    written = fputs(plist, f) != EOF;

    if (fclose(f) != 0 || !written) {
        goto out;
    }

    vol->v_limitsize = 1;
    vol->v_tm_cachetime = 0;
    rc = getvolparams(obj, vol->v_vid, 1 << VOLPBIT_XBFREE);
    used = vol->v_tm_used;
    vol->v_limitsize = saved_limit;
    vol->v_tm_cachetime = 0;

    if (rc == AFP_OK && used == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "no band: %d, used %ju\n", rc, (uintmax_t)used);
        ret = 1;
    }

out:
    unlink(info);
    rmdir(bands);
    rmdir(bundle);
    return ret;
}

/*!
 * @brief A size-limited volume's usage scan replaces the bytes counted as
 *        appended before it; the cached read after it adds only the bytes
 *        appended since
 *
 * @returns 0, 1 when a read disagrees, -1 never
 */
int utest_volume_scan_resets_appended(AFPObj *obj, struct vol *vol)
{
    VolSpace saved_limit = vol->v_limitsize;
    VolSpace used[2];
    int rc[2];
    vol->v_limitsize = 1;
    vol->v_appended = 1000;
    /* the first call scans the volume, the second reads the cached result */
    vol->v_tm_cachetime = 0;
    rc[0] = getvolparams(obj, vol->v_vid, 1 << VOLPBIT_XBFREE);
    used[0] = vol->v_tm_used;
    /* bytes appended after the scan */
    vol->v_appended += 500;
    rc[1] = getvolparams(obj, vol->v_vid, 1 << VOLPBIT_XBFREE);
    used[1] = vol->v_tm_used;
    vol->v_limitsize = saved_limit;
    vol->v_appended = 0;
    vol->v_tm_cachetime = 0;

    if (rc[0] == AFP_OK && rc[1] == AFP_OK && used[0] == 0 && used[1] == 500) {
        return 0;
    }

    fprintf(stderr, "volume scan: %d/%d, used %ju/%ju\n", rc[0], rc[1],
            (uintmax_t)used[0], (uintmax_t)used[1]);
    return 1;
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
    static const uint32_t len[2] = { RQ_LEN, 1000 };
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

    /* the first read fills the cache, the second is a hit that ends short
     * of the fork's end */
    for (int pass = 0; pass < 2; pass++) {
        long at;
        memset(fio.dsi.commands, FIO_FILL, FIO_QUANTUM);
        memset(fio.dsi.data, FIO_FILL, sizeof(fio.dsi.data));

        if (fio_read(r, off[pass], len[pass], len[pass], out) != AFP_OK
                || memcmp(out, rq_data + off[pass], len[pass]) != 0) {
            fprintf(stderr, "rfork hit: pass %d: reply\n", pass);
            ret = 1;
            goto out;
        }

        if ((at = fio_touched(fio.dsi.commands, DSI_WROFF_FPWRITEEXT,
                              FIO_QUANTUM)) >= 0) {
            fprintf(stderr, "rfork hit: pass %d: commands[%ld] written\n",
                    pass, at);
            ret = 1;
            goto out;
        }

        if ((at = fio_touched(fio.dsi.data, 0, sizeof(fio.dsi.data))) >= 0) {
            fprintf(stderr, "rfork hit: pass %d: data[%ld] written\n",
                    pass, at);
            ret = 1;
            goto out;
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
 * @brief A cached state of no resource fork, which the fork has since
 *        outgrown, is dropped, and the read is served from the fork
 *
 * @param[in] obj  session object, copied by the fixture
 * @param[in] vol  volume whose root takes the file
 * @returns 0, 1 when a read or the entry disagrees, -1 when the setup
 *          failed
 */
int utest_rfork_cache_stale_length_reads_fork(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rstale";
    /* cached with no fork, and with no AppleDouble data */
    static const off_t stale[2] = { AD_RLEN_NO_RFORK, AD_RLEN_NO_AD };
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
 * @brief A fork whose file another session moved aside is not served from
 *        the cache entry of the file that took its name
 */
int utest_rfork_cache_checks_identity(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rident";
    static char aside[] = "rq_rident.old";
    char other[RQ_LEN];
    char out[RQ_LEN];
    unsigned long long added;
    uint16_t r = 0;
    uint16_t n = 0;
    int moved = 0;
    int ret = -1;
    int rc;

    if (fio_open(obj) != 0) {
        return -1;
    }

    for (int i = 0; i < RQ_LEN; i++) {
        other[i] = (char)~rq_data[i];
    }

    if (fio_rfork(vol, name, &r) != 0) {
        goto out;
    }

    rc = fio_replace(vol, name, aside, other);
    moved = rc >= 0;

    if (rc != 0) {
        goto out;
    }

    added = rfork_stat_added;

    /* the new file's read fills the entry under the name */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &n) != AFP_OK
            || fio_read(n, 0, RQ_LEN, RQ_LEN, out) != AFP_OK
            || memcmp(out, other, RQ_LEN) != 0
            || rfork_stat_added != added + 1) {
        fprintf(stderr, "rfork identity: the new file\n");
        goto out;
    }

    if (fio_read(r, 0, RQ_LEN, RQ_LEN, out) == AFP_OK
            && memcmp(out, rq_data, RQ_LEN) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "rfork identity: the moved file's fork read %s\n",
                memcmp(out, other, RQ_LEN) == 0 ? "the new file's bytes"
                : "other bytes");
        ret = 1;
    }

out:

    if (n) {
        closefork(&fio.obj, n);
    }

    if (r) {
        closefork(&fio.obj, r);
    }

    if (moved) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, aside);
    }

    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief The parent buffers one refresh hint per sender and file in a
 *        batch: a sender's repeat drops, while another session's refresh
 *        of the file, the sender's refresh of another file or of the same
 *        id on another volume, and a refresh in the next batch stay
 *
 * @param[in] obj  session object, copied as the two senders
 * @param[in] vol  volume the hints name
 * @returns 0, 1 when a batch holds another count, -1 when the setup
 *          failed
 */
int utest_hint_relay_drops_repeats(AFPObj *obj, struct vol *vol)
{
    server_child_t *children = server_child_alloc(3);
    AFPObj sender = *obj;
    AFPObj other = *obj;
    int fds[2] = { -1, -1 };
    int queued = -1;
    int again = -1;
    int ret = -1;

    /* a hint that never comes ends the count short rather than hangs the
     * suite */
    if (children != NULL && socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0
            && setnonblock(fds[0], 1) == 0) {
        /* the relay skips a parent with no session but the sender */
        children->servch_count = 2;
        sender.ipc_fd = fds[1];
        other.ipc_fd = fds[1];
        other.pid = obj->pid + 1;
        ipc_send_cache_hint(&sender, vol->v_vid, htonl(100),
                            CACHE_HINT_REFRESH);
        ipc_send_cache_hint(&sender, vol->v_vid, htonl(100),
                            CACHE_HINT_REFRESH);
        ipc_send_cache_hint(&sender, vol->v_vid, htonl(101),
                            CACHE_HINT_REFRESH);
        /* the same id on another volume names another file */
        ipc_send_cache_hint(&sender, (uint16_t)(vol->v_vid + 1), htonl(100),
                            CACHE_HINT_REFRESH);
        ipc_send_cache_hint(&other, vol->v_vid, htonl(100),
                            CACHE_HINT_REFRESH);
        struct pollfd pfd = { .fd = fds[0], .events = POLLIN };
        int reads = 0;
        int flushed;

        /* ipc_server_read() also returns 0 for a socket with nothing to read */
        while (reads < 5 && poll(&pfd, 1, 0) == 1
                && ipc_server_read(children, fds[0]) == 0) {
            reads++;
        }

        /* no child is registered, so the flush only empties the buffer and
         * reports what the batch held */
        flushed = hint_flush_pending(children);

        if (reads == 5) {
            queued = flushed;
        }

        ipc_send_cache_hint(&sender, vol->v_vid, htonl(100),
                            CACHE_HINT_REFRESH);

        if (poll(&pfd, 1, 0) == 1 && ipc_server_read(children, fds[0]) == 0) {
            again = hint_flush_pending(children);
        }

        if (queued >= 0 && again >= 0) {
            ret = queued == 4 && again == 1 ? 0 : 1;
        }
    }

    if (ret == 1) {
        fprintf(stderr, "hint relay: %d hints buffered, %d after the flush\n",
                queued, again);
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
 * @returns 0 for nothing, 1 for one event hint for id on vid, -1 for
 *          anything else
 */
static int hint_take(int fd, uint16_t vid, cnid_t id, int event)
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
    return cmd == IPC_CACHE_HINT && hint.event == event
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
    struct fsize_cap cap;
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
                              strlen(name))) == CNID_INVALID) {
        goto out;
    }

    /* the session's IPC socket: what it sends its parent arrives at fds[0] */
    fio.obj.ipc_fd = fds[1];
    /* the read-ahead holds the write's first 512 bytes; the write lands up to
     * the 1024-byte file size limit and then fails */
    fio.dsi.end = fio.dsi.buffer + DSI_FRAME_OVERHEAD_EXT + 512;

    if (fsize_cap(&cap, 1024) == 0) {
        rc = fio_write(w, 0, 0, rq_data, RQ_LEN);
        fsize_uncap(&cap);
    }

    fio.dsi.end = fio.dsi.buffer + FIO_QUANTUM;

    if (rc == AFPERR_DFULL) {
        got[0] = hint_take(fds[0], vol->v_vid, id, CACHE_HINT_REFRESH);
    }

    if (fio_write(w, 0, 0, rq_data, 64) == AFP_OK) {
        got[1] = hint_take(fds[0], vol->v_vid, id, CACHE_HINT_REFRESH);
    }

    if (closefork(&fio.obj, w) == AFP_OK) {
        got[2] = hint_take(fds[0], vol->v_vid, id, CACHE_HINT_REFRESH);
    }

    w = 0;

    /* a length set with no write after it: the second open's first change */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD | OPENACC_WR, &w) == AFP_OK
            && setforkparams(&fio.obj, w, 32) == AFP_OK) {
        got[3] = hint_take(fds[0], vol->v_vid, id, CACHE_HINT_REFRESH);
    }

    if (w && closefork(&fio.obj, w) == AFP_OK) {
        got[4] = hint_take(fds[0], vol->v_vid, id, CACHE_HINT_REFRESH);
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
 *
 * @param[in] obj  session object, copied by the fixture
 * @param[in] vol  volume whose root takes the file
 * @returns 0, 1 when a fill count disagrees, -1 when the setup or a read
 *          failed
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
    size_t rbuflen = DSI_DATASIZ;
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
    ok = ok && fio_send(0, r, RQ_LEN, 64, NULL) == 0
         && afp_read_ext(&fio.obj, (char *)fio.dsi.commands, fio.dsi.cmdlen,
                         (char *)fio.dsi.data, &rbuflen) == AFPERR_EOF;
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
 * @brief fce_register() skips an event in a directory listed through a
 *        symlink, one reached through the symlink or whose path no longer
 *        resolves included, and in one listed with a trailing slash before
 *        it existed, and lets through one in a directory whose name only
 *        starts with the listed one's
 *
 * @param[in] obj  session object, copied with FCE configured
 * @param[in] vol  volume whose root takes the directories
 * @returns 0, 1 when an event was matched wrongly, -1 when the setup
 *          failed, or TEST_SKIP
 */
int utest_fce_ignore_directories(AFPObj *obj, struct vol *vol)
{
#ifndef WITH_FCE
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    char ignored[MAXPATHLEN + 1];
    char alias[MAXPATHLEN + 1];
    char gone[MAXPATHLEN + 1];
    char gonel[MAXPATHLEN + 1];
    char other[MAXPATHLEN + 1];
    char later[MAXPATHLEN + 1];
    char list[2 * MAXPATHLEN + 3];
    AFPObj fce = fce_session(obj);
    int probe;
    int skipped;
    int aliased;
    int unresolved;
    int unresolvedl;
    int slashed = -1;
    int kept;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    /* the volume's path is resolved when the volume is loaded */
    snprintf(ignored, sizeof(ignored), "%s/rq_fceign", vol->v_path);
    snprintf(alias, sizeof(alias), "%s/rq_fceignl", vol->v_path);
    snprintf(gone, sizeof(gone), "%s/gone", ignored);
    snprintf(gonel, sizeof(gonel), "%s/gone", alias);
    snprintf(other, sizeof(other), "%s/rq_fceignx", vol->v_path);
    snprintf(later, sizeof(later), "%s/rq_fceignp", vol->v_path);
    snprintf(list, sizeof(list), "%s,%s/", alias, later);

    if (mkdir(ignored, 0700) != 0) {
        return -1;
    }

    if (mkdir(other, 0700) != 0 || symlink("rq_fceign", alias) != 0) {
        rmdir(other);
        rmdir(ignored);
        return -1;
    }

    fault_inject_reset();
    fi.time_watch = 1;
    (void)time(NULL);
    probe = fi.time_calls;
    fce.options.fce_ign_directories = list;
    fce.options.fce_fmodwait = 5;
    fi.time_calls = 0;
    fce_register(&fce, FCE_FILE_MODIFY, ignored, NULL);
    skipped = fi.time_calls;
    fi.time_calls = 0;
    fce_register(&fce, FCE_FILE_MODIFY, alias, NULL);
    aliased = fi.time_calls;
    fi.time_calls = 0;
    fce_register(&fce, FCE_FILE_MODIFY, gone, NULL);
    unresolved = fi.time_calls;
    fi.time_calls = 0;
    fce_register(&fce, FCE_FILE_MODIFY, gonel, NULL);
    unresolvedl = fi.time_calls;

    /* the first registration read the list; this entry exists only now */
    if (mkdir(later, 0700) == 0) {
        fi.time_calls = 0;
        fce_register(&fce, FCE_FILE_MODIFY, later, NULL);
        slashed = fi.time_calls;
    }

    fi.time_calls = 0;
    fce_register(&fce, FCE_FILE_MODIFY, other, NULL);
    kept = fi.time_calls;
    fce.options.fce_fmodwait = -1;
    fce_pending_events(&fce);
    fault_inject_reset();
    unlink(alias);
    rmdir(later);
    rmdir(other);
    rmdir(ignored);

    /* positive control: the probe's own time() is counted */
    if (probe == 0) {
        return TEST_SKIP;
    }

    if (slashed < 0) {
        return -1;
    }

    if (skipped == 0 && aliased == 0 && unresolved == 0 && unresolvedl == 0
            && slashed == 0 && kept == 1) {
        return 0;
    }

    fprintf(stderr,
            "fce ignore: skipped %d, aliased %d, unresolved %d/%d, slashed "
            "%d, kept %d\n", skipped, aliased, unresolved, unresolvedl,
            slashed, kept);
    return 1;
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
    AFPObj fce = fce_session(obj);
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
 * @brief The first write to the virtual Icon fork takes the open-mode locks
 *        a read-write open of the real fork holds, and the session's next
 *        open of the file finds the fork
 */
int utest_virtual_icon_write_takes_open_lock(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    struct path p = { 0 };
    uint16_t w = 0;
    int locked = -1;
    int found = 0;
    int rc = -1;
    int ret;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &w) == AFP_OK) {
        rc = fio_write(w, 0, 0, rq_data, 1);
        /* as another session's open sees it */
        locked = peer_try_lock(path, F_WRLCK, AD_FILELOCK_RSRC_OPEN_WR, 1) == 0
                 && peer_try_lock(path, F_WRLCK, AD_FILELOCK_RSRC_OPEN_RD,
                                  1) == 0;
        /* the session's next open of the file shares the fork's adouble */
        p.u_name = path;
        found = of_findname(vol, &p) == of_find(w);
        closefork(&fio.obj, w);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(path);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, vol->ad_path(name, 0));
    unlink(path);

    if (rc == AFP_OK && locked == 1 && found) {
        ret = 0;
    } else {
        fprintf(stderr, "virtual icon: write %d, open locks %d, found %d\n",
                rc, locked, found);
        ret = 1;
    }

    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief A first write to the virtual Icon fork that another open's deny
 *        modes refuse replies AFPERR_LOCK and leaves the fork virtual,
 *        holding nothing on the file
 */
int utest_virtual_icon_refused_write_stays_virtual(AFPObj *obj,
                                                   struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    const struct ofork *of;
    struct peer denier;
    uint16_t w = 0;
    int held = -1;
    int read_free = -1;
    int rc = -1;
    int fd;
    int ret;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    /* the virtual fork opens first: a real Icon file is served as itself */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &w) == AFP_OK
            && (fd = open(path, O_CREAT | O_WRONLY, 0600)) >= 0) {
        close(fd);

        /* another session's open of the real fork, denying writers */
        if (peer_hold_lock(&denier, path, F_RDLCK, AD_FILELOCK_RSRC_DENY_WR,
                           1) == 0) {
            rc = fio_write(w, 0, 0, rq_data, 1);

            if ((of = of_find(w)) != NULL) {
                held = !(of->of_flags & AFPFORK_VIRTUAL)
                       || ad_data_fileno(of->of_ad) != -1
                       || ad_reso_fileno(of->of_ad) != -1
                       || ad_meta_fileno(of->of_ad) != -1;
            }

            read_free = peer_try_lock(path, F_WRLCK, AD_FILELOCK_RSRC_OPEN_RD,
                                      1);
            peer_release(&denier);
        }
    }

    if (w) {
        closefork(&fio.obj, w);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, vol->ad_path(name, 0));
    unlink(path);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    unlink(path);

    if (rc == AFPERR_LOCK && held == 0 && read_free == 1) {
        ret = 0;
    } else {
        fprintf(stderr, "virtual icon refused: write %d, held %d, read lock "
                        "free %d\n", rc, held, read_free);
        ret = 1;
    }

    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief A second virtual Icon fork's first write, after the session's first
 *        made the file real, leaves the first fork's open lock at its close
 */
int utest_virtual_icon_second_write_keeps_locks(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    uint16_t v1 = 0;
    uint16_t v2 = 0;
    int rc[2] = { -1, -1 };
    int held = -1;
    int ret;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &v1) == AFP_OK
            && openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                        OPENFORK_RSCS, OPENACC_RD | OPENACC_WR,
                        &v2) == AFP_OK) {
        rc[0] = fio_write(v1, 0, 0, rq_data, 1);
        rc[1] = fio_write(v2, 0, 1, rq_data, 1);
        closefork(&fio.obj, v2);
        v2 = 0;
        /* as another session's open sees it */
        held = peer_try_lock(path, F_WRLCK, AD_FILELOCK_RSRC_OPEN_WR, 1) == 0;
    }

    if (v1) {
        closefork(&fio.obj, v1);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(path);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, vol->ad_path(name, 0));
    unlink(path);

    if (rc[0] == AFP_OK && rc[1] == AFP_OK && held == 1) {
        ret = 0;
    } else {
        fprintf(stderr, "virtual icon second write: %d/%d, open lock %d\n",
                rc[0], rc[1], held);
        ret = 1;
    }

    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief A data fork open that another session's deny mode refuses leaves
 *        the session's open resource fork of the file whole and keeps no
 *        lock of its own
 */
int utest_refused_open_keeps_sibling_fork(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_refused";
    char path[MAXPATHLEN + 1];
    char out[RQ_LEN];
    struct peer denier;
    uint16_t r = 0;
    uint16_t d = 0;
    int read_free;
    int whole;
    int rc;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    /* another session's open of the data fork, denying writers */
    if (fio_rfork(vol, name, &r) != 0
            || peer_hold_lock(&denier, path, F_RDLCK, AD_FILELOCK_DENY_WR,
                              1) != 0) {
        goto out;
    }

    rc = openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                  OPENACC_RD | OPENACC_WR, &d);
    peer_release(&denier);
    read_free = peer_try_lock(path, F_WRLCK, AD_FILELOCK_OPEN_RD, 1);
    whole = fio_read(r, 0, RQ_LEN, RQ_LEN, out) == AFP_OK
            && memcmp(out, rq_data, RQ_LEN) == 0;

    if (rc == AFPERR_DENYCONF && read_free == 1 && whole) {
        ret = 0;
    } else {
        fprintf(stderr, "refused open: %d, read lock free %d, fork %s\n", rc,
                read_free, whole ? "whole" : "cut");
        ret = 1;
    }

out:

    if (d) {
        closefork(&fio.obj, d);
    }

    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief Closing a read-only resource fork opened before the fork existed
 *        leaves the writer that created it working
 */
int utest_reader_close_keeps_writer(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rdfirst";
    char path[MAXPATHLEN + 1];
    char out[RQ_LEN];
    uint16_t rd = 0;
    uint16_t w = 0;
    int fd;
    int rc;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    /* a file made outside AFP, with no resource fork yet */
    if ((fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0600)) < 0) {
        goto out;
    }

    close(fd);

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &rd) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || closefork(&fio.obj, rd) != AFP_OK) {
        goto out;
    }

    rd = 0;
    rc = fio_write(w, 0, 0, rq_data, RQ_LEN);

    if (rc == AFP_OK && fio_read(w, 0, RQ_LEN, RQ_LEN, out) == AFP_OK
            && memcmp(out, rq_data, RQ_LEN) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "reader close: write %d\n", rc);
        ret = 1;
    }

out:

    if (rd) {
        closefork(&fio.obj, rd);
    }

    fio_done(vol, w, name);
    return ret;
}

/*!
 * @brief Closing an emptied resource fork whose file another session moved
 *        aside leaves the resource fork of the file that took its name
 */
int utest_emptied_rfork_close_spares_new_file(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "rq_rempty";
    static char aside[] = "rq_rempty.old";
    char out[RQ_LEN];
    uint16_t e = 0;
    uint16_t r = 0;
    int moved = 0;
    int ret = -1;
    int rc;

    /* only the ea layout's ._ sidecar goes when its fork is emptied */
    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &e) != AFP_OK) {
        goto out;
    }

    rc = fio_replace(vol, name, aside, rq_data);
    moved = rc >= 0;

    if (rc != 0 || closefork(&fio.obj, e) != AFP_OK) {
        goto out;
    }

    e = 0;

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) == AFP_OK
            && fio_read(r, 0, RQ_LEN, RQ_LEN, out) == AFP_OK
            && memcmp(out, rq_data, RQ_LEN) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "emptied rfork: the new file's fork is gone\n");
        ret = 1;
    }

out:

    if (e) {
        closefork(&fio.obj, e);
    }

    if (moved) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, aside);
    }

    fio_done(vol, r, name);
    return ret;
#endif
}

/*!
 * @brief Setting the length of a symlink's resource fork and closing the
 *        fork keeps the CNIDs of the link and of the file it points to
 */
int utest_symlink_rfork_close_keeps_cnids(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char target[] = "rq_symt";
    static char alias[] = "rq_syml";
    char path[MAXPATHLEN + 1];
    cnid_t was[2] = { CNID_INVALID, CNID_INVALID };
    cnid_t now[2];
    uint16_t w = 0;
    int rc;
    int ret = -1;

    /* only the ea layout's ._ sidecar gives a symlink a writable resource
     * fork */
    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, alias);

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, target) != AFP_OK
            || symlink(target, path) != 0
            || getfiledirparms(&fio.obj, vol->v_vid, DIRDID_ROOT,
                               alias) != AFP_OK
            || (was[0] = cnid_get(vol->v_cdb, DIRDID_ROOT, target,
                                  strlen(target))) == CNID_INVALID
            || (was[1] = cnid_get(vol->v_cdb, DIRDID_ROOT, alias,
                                  strlen(alias))) == CNID_INVALID
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, alias, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK) {
        goto out;
    }

    rc = setforkparams(&fio.obj, w, 32);
    closefork(&fio.obj, w);
    now[0] = cnid_get(vol->v_cdb, DIRDID_ROOT, target, strlen(target));
    now[1] = cnid_get(vol->v_cdb, DIRDID_ROOT, alias, strlen(alias));

    if (rc == AFP_OK && now[0] == was[0] && now[1] == was[1]) {
        ret = 0;
    } else {
        fprintf(stderr, "symlink rfork: length %d, CNIDs %u/%u, were %u/%u\n",
                rc, ntohl(now[0]), ntohl(now[1]), ntohl(was[0]),
                ntohl(was[1]));
        ret = 1;
    }

out:
    delete (&fio.obj, vol->v_vid, DIRDID_ROOT, alias);
    fio_done(vol, 0, target);
    return ret;
#endif
}

/*!
 * @brief A close that leaves the root Icon file with an empty resource fork
 *        deletes it with its AppleDouble file, its CNID and its dircache
 *        entry, and tells the other sessions; a close while the session
 *        holds another fork of the file keeps it
 *
 * The first fork's write seeds a resource fork header with no resources.
 * Its close finds the session's data fork of the file open and keeps the
 * file; the read-only fork's close, which changes nothing, deletes it.
 *
 * @returns 0, 1 when a step disagrees, -1 when the setup failed
 */
int utest_emptied_icon_close_deletes_file(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    int fds[2] = { -1, -1 };
    uint16_t w = 0;
    uint16_t d = 0;
    uint16_t r = 0;
    cnid_t id = CNID_INVALID;
    int kept = -1;
    int gone = -1;
    int recorded = -1;
    int hinted = -1;
    int cached = -1;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0
            || setnonblock(fds[0], 1) != 0
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || fio_write(w, 0, 0, rq_data, 1) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                        OPENACC_RD, &d) != AFP_OK
            || getfiledirparms(&fio.obj, vol->v_vid, DIRDID_ROOT,
                               name) != AFP_OK
            || (id = cnid_get(vol->v_cdb, DIRDID_ROOT, name,
                              strlen(name))) == CNID_INVALID) {
        goto out;
    }

    closefork(&fio.obj, w);
    w = 0;
    kept = access(path, F_OK) == 0;
    closefork(&fio.obj, d);
    d = 0;
    /* the session's IPC socket: what it sends its parent arrives at fds[0] */
    fio.obj.ipc_fd = fds[1];

    if (kept && openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                         OPENFORK_RSCS, OPENACC_RD, &r) == AFP_OK) {
        closefork(&fio.obj, r);
        r = 0;
        gone = access(path, F_OK) != 0;
#ifndef HAVE_EAFD
        gone = gone && access(adpath, F_OK) != 0;
#endif
        recorded = cnid_get(vol->v_cdb, DIRDID_ROOT, name,
                            strlen(name)) != CNID_INVALID;
        hinted = hint_take(fds[0], vol->v_vid, id, CACHE_HINT_DELETE);
        cached = dircache_search_by_name(vol, vol->v_root, name,
                                         strlen(name)) != NULL;
    }

    if (kept == 1 && gone == 1 && recorded == 0 && hinted == 1
            && cached == 0) {
        ret = 0;
    } else {
        fprintf(stderr,
                "emptied icon: kept %d, gone %d, CNID %d, hinted %d, cached "
                "%d\n", kept, gone, recorded, hinted, cached);
        ret = 1;
    }

out:
    fio.obj.ipc_fd = -1;

    if (fds[0] >= 0) {
        close(fds[0]);
        close(fds[1]);
    }

    if (w) {
        closefork(&fio.obj, w);
    }

    if (d) {
        closefork(&fio.obj, d);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(path);
    unlink(adpath);
    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief The close of a root Icon file's emptied resource fork keeps the
 *        file while its data fork holds bytes, and on a read-only volume
 *
 * The file gets a resource fork header with no resources while a data fork
 * of it is open, so that close deletes nothing.
 *
 * @returns 0, 1 when a close deleted the file, -1 when the setup failed
 */
int utest_emptied_icon_close_keeps_data(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    const int saved_flags = vol->v_flags;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    uint16_t w = 0;
    uint16_t d = 0;
    uint16_t r = 0;
    int ro = -1;
    int data = -1;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || fio_write(w, 0, 0, (const char *)icon, sizeof(icon)) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                        OPENACC_RD, &d) != AFP_OK) {
        goto out;
    }

    closefork(&fio.obj, w);
    w = 0;
    closefork(&fio.obj, d);
    d = 0;
    vol->v_flags |= AFPVOL_RO;

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) == AFP_OK) {
        closefork(&fio.obj, r);
        r = 0;
        ro = access(path, F_OK) == 0;
    }

    vol->v_flags = saved_flags;

    if (ro == 1
            && openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                        OPENACC_RD | OPENACC_WR, &d) == AFP_OK) {
        const int wrote = fio_write(d, 0, 0, rq_data, 1) == AFP_OK;
        closefork(&fio.obj, d);
        d = 0;

        if (wrote && openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                              OPENFORK_RSCS, OPENACC_RD, &r) == AFP_OK) {
            closefork(&fio.obj, r);
            r = 0;
            data = access(path, F_OK) == 0;
        }
    }

    if (ro == 1 && data == 1) {
        ret = 0;
    } else {
        fprintf(stderr,
                "emptied icon: kept on a read-only volume %d, with data %d\n",
                ro, data);
        ret = 1;
    }

out:
    vol->v_flags = saved_flags;

    if (w) {
        closefork(&fio.obj, w);
    }

    if (d) {
        closefork(&fio.obj, d);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(path);
    unlink(adpath);
    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief The close of an emptied resource fork of a root symlink named Icon
 *        keeps the symlink
 *
 * @returns 0, 1 when the close removed the symlink, -1 when the setup
 *          failed, or TEST_SKIP
 */
int utest_emptied_icon_close_keeps_symlink(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "Icon\r";
    static char target[] = "rq_icont";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    struct stat st;
    uint16_t w = 0;
    int ret = -1;

    /* only the ea layout's ._ sidecar gives a symlink a writable resource
     * fork */
    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, target) == AFP_OK
            && symlink(target, path) == 0
            && openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) == AFP_OK) {
        closefork(&fio.obj, w);
        ret = lstat(path, &st) == 0 && S_ISLNK(st.st_mode) ? 0 : 1;
    }

    if (ret == 1) {
        fprintf(stderr, "emptied icon: the close removed the symlink\n");
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(path);
    unlink(adpath);
    fio_done(vol, 0, target);
    return ret;
#endif
}

/*!
 * @brief A virtual Icon fork opened without read access refuses a read
 *
 * @returns 0, 1 when the read is served, -1 when the setup failed
 */
int utest_virtual_icon_write_only_open_refuses_read(AFPObj *obj,
                                                    struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char out[16];
    uint16_t w = 0;
    int rc = -1;
    int ret;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_WR, &w) == AFP_OK) {
        rc = fio_read(w, 0, sizeof(out), sizeof(out), out);
        closefork(&fio.obj, w);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;

    if (rc == AFPERR_ACCESS) {
        ret = 0;
    } else {
        fprintf(stderr, "write-only virtual icon: read %d\n", rc);
        ret = 1;
    }

    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief A first write whose create of the Icon file fails with EACCES
 *        replies AFPERR_ACCESS, and a length set whose create fails with
 *        another error AFPERR_MISC
 *
 * @returns 0, 1 when a reply is another code, -1 when the setup failed,
 *          TEST_SKIP without the open() fault injector
 */
int utest_icon_create_refused_replies_access(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    uint16_t w = 0;
    int rc = -1;
    int set = -1;
    int ret;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &w) == AFP_OK) {
        /* only the exclusive create fails, for root too */
        fault_inject_reset();
        fi.open_armed = 1;
        fi.open_fail_after = 0;
        fi.open_errno = EACCES;
        fi.open_fail_flags = O_CREAT | O_EXCL;
        rc = fio_write(w, 0, 0, rq_data, 1);
        /* the fork stayed virtual; a failed create is no bad refnum */
        fi.open_fail_after = 0;
        fi.open_errno = EIO;
        set = setforkparams(&fio.obj, w, 32);
        fault_inject_reset();
        closefork(&fio.obj, w);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;

    if (rc == AFPERR_ACCESS && set == AFPERR_MISC) {
        ret = 0;
    } else {
        fprintf(stderr, "refused icon create: write %d, length set %d\n", rc,
                set);
        ret = 1;
    }

    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief The close that unlinks an emptied ._ file leaves another name's
 *        ._ file alone: a hard link's forks share the adouble, whose ._ file
 *        is the first opener's
 *
 * @returns 0, 1 when the other name lost its resource fork, -1 when the
 *          setup failed, TEST_SKIP where no ._ file holds the fork
 */
int utest_emptied_rfork_close_keeps_hard_link(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "rq_hla";
    static char other[] = "rq_hlb";
    char path[MAXPATHLEN + 1];
    char linkpath[MAXPATHLEN + 1];
    char out[RQ_LEN];
    uint16_t a = 0;
    uint16_t b = 0;
    int linked = 0;
    int rc = -1;
    int ret = -1;

    /* only the ea layout's ._ sidecar goes when its fork is emptied */
    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(linkpath, sizeof(linkpath), "%s/%s", vol->v_path, other);

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || link(path, linkpath) != 0) {
        goto out;
    }

    linked = 1;

    /* the other name's resource fork holds data */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, other, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &b) != AFP_OK
            || fio_write(b, 0, 0, rq_data, RQ_LEN) != AFP_OK
            || closefork(&fio.obj, b) != AFP_OK) {
        goto out;
    }

    b = 0;

    /* this name's open makes an empty ._ file, which the other name's fork
     * then shares; the last close finds it empty */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &a) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, other, OPENFORK_RSCS,
                        OPENACC_RD, &b) != AFP_OK
            || closefork(&fio.obj, a) != AFP_OK) {
        goto out;
    }

    a = 0;

    if (closefork(&fio.obj, b) != AFP_OK) {
        goto out;
    }

    b = 0;

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, other, OPENFORK_RSCS,
                 OPENACC_RD, &b) == AFP_OK) {
        rc = fio_read(b, 0, RQ_LEN, RQ_LEN, out);
    }

    if (rc == AFP_OK && memcmp(out, rq_data, RQ_LEN) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "hard link rfork: read %d\n", rc);
        ret = 1;
    }

out:

    if (a) {
        closefork(&fio.obj, a);
    }

    if (b) {
        closefork(&fio.obj, b);
    }

    if (linked) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, other);
    }

    fio_done(vol, 0, name);
    return ret;
#endif
}

/*!
 * @brief FPRename takes the session's open forks of the file to its new
 *        name: a write through a fork opened under the old name reaches the
 *        reads of one opened under the new, and the open's deny mode holds
 */
int utest_rename_moves_open_forks(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rnold";
    static char newname[] = "rq_rnnew";
    char path[MAXPATHLEN + 1];
    char out[RQ_LEN];
    uint16_t r = 0;
    uint16_t w = 0;
    uint16_t n = 0;
    int fresh;
    int denied;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, newname);

    if (fio_rfork(vol, name, &r) != 0
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR | OPENACC_DWR, &w) != AFP_OK
            || renameobj(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                         newname) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, newname,
                        OPENFORK_RSCS, OPENACC_RD, &n) != AFP_OK
            || fio_read(n, 0, RQ_LEN, RQ_LEN, out) != AFP_OK
            || fio_write(w, 0, 0, rq_data + 1000, 1000) != AFP_OK
            || fio_read(n, 0, 1000, 1000, out) != AFP_OK) {
        fprintf(stderr, "rename: setup\n");
        goto out;
    }

    fresh = memcmp(out, rq_data + 1000, 1000) == 0;
    /* as another session's open sees it */
    denied = peer_try_lock(path, F_WRLCK, AD_FILELOCK_RSRC_DENY_WR, 1) == 0;

    if (fresh && denied) {
        ret = 0;
    } else {
        fprintf(stderr, "rename: %s bytes, deny lock %s\n",
                fresh ? "new" : "old", denied ? "held" : "free");
        ret = 1;
    }

out:

    if (n) {
        closefork(&fio.obj, n);
    }

    if (w) {
        closefork(&fio.obj, w);
    }

    if (r) {
        closefork(&fio.obj, r);
    }

    delete (&fio.obj, vol->v_vid, DIRDID_ROOT, name);
    fio_done(vol, 0, newname);
    return ret;
}

/*!
 * @brief A size-limited volume counts the bytes a write appended: a payload
 *        shorter than its ReqCount counts its own length, a write that fails
 *        part way the bytes that landed, and a length set the bytes it grows
 *        the fork by
 */
int utest_write_counts_bytes_appended(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_dcount";
    struct fsize_cap cap;
    struct stat st;
    VolSpace saved_appended = vol->v_appended;
    VolSpace saved_limit = vol->v_limitsize;
    VolSpace counted[3] = { 0, 0, 0 };
    uint64_t reqcount_be = hton64((uint64_t)RQ_LEN);
    size_t rbuflen = DSI_DATASIZ;
    off_t size = -1;
    uint16_t w = 0;
    int rc[3] = { -1, -1, -1 };
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (fio_dfork(vol, name, &w) != 0) {
        goto out;
    }

    vol->v_limitsize = 1;
    vol->v_appended = 0;

    /* 64 bytes under a ReqCount of RQ_LEN */
    if (fio_send(0, w, 0, 64, rq_data) == 0) {
        memcpy(fio.dsi.commands + 12, &reqcount_be, sizeof(reqcount_be));
        rc[0] = afp_write_ext(&fio.obj, (char *)fio.dsi.commands,
                              fio.dsi.cmdlen, (char *)fio.dsi.data, &rbuflen);
    }

    counted[0] = vol->v_appended;
    vol->v_appended = 0;
    /* the write lands up to the 1024-byte file size limit, past the
     * read-ahead's 512 bytes, and then fails */
    fio.dsi.end = fio.dsi.buffer + DSI_FRAME_OVERHEAD_EXT + 512;

    if (fsize_cap(&cap, 1024) == 0) {
        rc[1] = fio_write(w, 1, 0, rq_data, RQ_LEN);
        fsize_uncap(&cap);
    }

    fio.dsi.end = fio.dsi.buffer + FIO_QUANTUM;
    counted[1] = vol->v_appended;

    if (fstat(ad_data_fileno(of_find(w)->of_ad), &st) == 0) {
        size = st.st_size;
    }

    vol->v_appended = 0;
    rc[2] = setforkparams(&fio.obj, w, 4096);
    counted[2] = vol->v_appended;
    vol->v_limitsize = saved_limit;
    vol->v_appended = saved_appended;

    if (rc[0] == AFP_OK && counted[0] == 64 && rc[1] == AFPERR_DFULL
            && size > 64 && counted[1] == (VolSpace)(size - 64)
            && rc[2] == AFP_OK && counted[2] == (VolSpace)(4096 - size)) {
        ret = 0;
    } else {
        fprintf(stderr, "appended: %d/%d/%d, counted %ju/%ju/%ju, size %jd\n",
                rc[0], rc[1], rc[2], (uintmax_t)counted[0],
                (uintmax_t)counted[1], (uintmax_t)counted[2], (intmax_t)size);
        ret = 1;
    }

out:
    fio_done(vol, w, name);
    return ret;
}

/*!
 * @brief The fork lookup by a name relative to a directory follows a symlink
 *        as the volume does, so a path through a link finds the fork of the
 *        file it opens
 *
 * @returns 0, 1 when the lookup disagrees with the volume's symlink policy,
 *          -1 when the setup failed
 */
int utest_findnameat_follows_as_volume(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_fol";
    static char alias[] = "rq_foll";
    const int saved_flags = vol->v_flags;
    char path[MAXPATHLEN + 1];
    struct path p = { .u_name = alias };
    const struct ofork *found[2] = { NULL, NULL };
    int dirfd = -1;
    uint16_t w = 0;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, alias);

    if (fio_dfork(vol, name, &w) != 0 || symlink(name, path) != 0
            || (dirfd = open(vol->v_path, O_RDONLY | O_DIRECTORY)) < 0) {
        goto out;
    }

    vol->v_flags |= AFPVOL_FOLLOWSYM;
    found[0] = of_findnameat(vol, dirfd, &p);
    vol->v_flags &= ~AFPVOL_FOLLOWSYM;
    p.st_valid = 0;
    found[1] = of_findnameat(vol, dirfd, &p);

    if (found[0] == of_find(w) && found[1] == NULL) {
        ret = 0;
    } else {
        fprintf(stderr, "findnameat: follow %s, no follow %s\n",
                found[0] == of_find(w) ? "found" : "missed",
                found[1] ? "found" : "missed");
        ret = 1;
    }

out:
    vol->v_flags = saved_flags;

    if (dirfd >= 0) {
        close(dirfd);
    }

    unlink(path);
    fio_done(vol, w, name);
    return ret;
}

/*!
 * @brief A rename refused after its metadata read leaves an open fork's
 *        resource fork length as the fork holds it, whatever length the
 *        dircache entry carries
 *
 * @returns 0, 1 when the fork took the cached length, -1 when the setup
 *          failed
 */
int utest_rename_keeps_open_fork_length(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rnlen";
    static char other[] = "rq_rnlen2";
    struct dir *entry;
    uint16_t r = 0;
    int created = 0;
    int rc;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (fio_rfork(vol, name, &r) != 0
            || (entry = dircache_search_by_name(vol, vol->v_root, name,
                                                strlen(name))) == NULL
            || createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, other) != AFP_OK) {
        goto out;
    }

    created = 1;
    /* a stale length, as after another session's write */
    entry->dcache_rlen = 5;
    rc = renameobj(&fio.obj, vol->v_vid, DIRDID_ROOT, name, other);

    if (rc == AFPERR_EXIST && of_find(r)->of_ad->ad_rlen == RQ_LEN) {
        ret = 0;
    } else {
        fprintf(stderr, "rename kept length: %d, length %lld\n", rc,
                (long long)of_find(r)->of_ad->ad_rlen);
        ret = 1;
    }

out:

    if (created) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, other);
    }

    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief An open of the virtual Icon file for writing replies AFPERR_ACCESS
 *        in a volume root the user cannot write, and an open for reading
 *        succeeds
 *
 * @returns 0, 1 when a reply is another code, -1 when the setup failed,
 *          TEST_SKIP for root, whom no mode refuses
 */
int utest_virtual_icon_open_checks_root_access(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    struct stat st;
    uint16_t r = 0;
    uint16_t w = 0;
    int rd;
    int wr;
    int ret;

    if (geteuid() == 0) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    /* the AFP rights are those of the user a login would have set */
    fio.obj.uid = getuid();
    fio.obj.euid = geteuid();

    if (stat(vol->v_path, &st) != 0
            || chmod(vol->v_path, st.st_mode & 0555) != 0) {
        fio_done(vol, 0, name);
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    wr = openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                  OPENACC_RD | OPENACC_WR, &w);
    rd = openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                  OPENACC_RD, &r);

    if (wr == AFP_OK) {
        closefork(&fio.obj, w);
    }

    if (rd == AFP_OK) {
        closefork(&fio.obj, r);
    }

    chmod(vol->v_path, st.st_mode & 07777);
    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;

    if (wr == AFPERR_ACCESS && rd == AFP_OK) {
        ret = 0;
    } else {
        fprintf(stderr,
                "virtual icon, root not writable: write open %d, read open "
                "%d\n", wr, rd);
        ret = 1;
    }

    fio_done(vol, 0, name);
    return ret;
}
