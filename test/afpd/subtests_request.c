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

/*!
 * @file
 * @brief Request path tests: fork writes and reads over a DSI socket pair,
 *        the resource fork cache, sibling hints, FCE filters and the Icon
 *        file's virtual and emptied states
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/cnid.h>
#include <atalk/dsi.h>
#include <atalk/ea.h>
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
#include "file.h"
#include "fork.h"
#include "peer_lock.h"
#include "subtests_conf.h"
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
 * @brief The first index in [from, to) at which buf differs from FIO_FILL,
 *        or -1 when the range is untouched
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
 * @brief FPOpenFork of name's fork for reading as a user who may not write
 *        the file: the open() injector refuses the data file's read-write
 *        open, so the fork holds a read-only data descriptor
 *
 * @returns the open's AFP result, or TEST_SKIP when the open was granted but
 *          the injector missed the data file's open; *refnum is set whenever
 *          the open was granted
 */
static int fio_open_readonly(const struct vol *vol, const char *name,
                             uint8_t fork, uint16_t *refnum)
{
    const struct ofork *of;
    int rc;
    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_after = 0;
    fi.open_errno = EACCES;
    fi.open_fail_flags = O_RDWR;
    rc = openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, fork, OPENACC_RD,
                  refnum);
    fault_inject_reset();

    /* positive control: the injector reached the data file's open */
    if (rc == AFP_OK && (of = of_find(*refnum)) != NULL
            && (of->of_ad->ad_data_fork.adf_flags & O_RDWR)) {
        return TEST_SKIP;
    }

    return rc;
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
 * @brief Another session's save over name while the session's resource fork
 *        of it is open: the fork, holding len bytes of seed, closes after
 *        fio_replace() moved its file to aside
 *
 * @returns 0 when the file that took the name reads rq_data from its
 *          resource fork, 1 when it does not, -1 when the setup failed
 */
static int fio_close_after_save(const struct vol *vol, char *name,
                                const char *aside, const char *seed,
                                uint32_t len)
{
    char out[RQ_LEN];
    uint16_t e = 0;
    uint16_t r = 0;
    int rc = -1;
    int ret = -1;

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &e) != AFP_OK) {
        return -1;
    }

    if (len == 0 || fio_write(e, 0, 0, seed, len) == AFP_OK) {
        rc = fio_replace(vol, name, aside, rq_data);
    }

    if (rc == 0 && closefork(&fio.obj, e) == AFP_OK) {
        e = 0;
        ret = openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                       OPENACC_RD, &r) == AFP_OK
              && fio_read(r, 0, RQ_LEN, RQ_LEN, out) == AFP_OK
              && memcmp(out, rq_data, RQ_LEN) == 0 ? 0 : 1;
    }

    if (e) {
        closefork(&fio.obj, e);
    }

    if (r) {
        closefork(&fio.obj, r);
    }

    if (rc >= 0) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, aside);
    }

    return ret;
}

/*!
 * @brief Set or clear an attribute bit of the file at path, such as
 *        DeleteInhibit
 *
 * @returns 0, or -1 when its metadata could not be written
 */
static int fio_set_attr(const struct vol *vol, const char *path,
                        uint16_t bit, int on)
{
    struct adouble ad;
    uint16_t attr;
    int ret = -1;
    ad_init(&ad, vol);

    if (ad_open(&ad, path, ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                0666) != 0) {
        return -1;
    }

    ad_getattr(&ad, &attr);
    attr = on ? attr | htons(bit) : attr & ~htons(bit);

    if (ad_setattr(&ad, attr) == 0 && ad_flush(&ad) == 0) {
        ret = 0;
    }

    ad_close(&ad, ADFLAGS_HF);
    return ret;
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
 * @returns 0, or 1 when a read disagrees
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
 * @brief A write to a file's resource fork through one hard link drops the
 *        cached resource fork of the session's fork under the other name
 *
 * The session's forks of the file share the adouble of its first open,
 * here a writer under the first name, so every name reads the bytes the
 * write left.
 *
 * @returns 0, 1 when the other name's read served the old bytes, -1 when
 *          the setup failed
 */
int utest_rfork_cache_drops_every_link_name(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rlinka";
    static char other[] = "rq_rlinkb";
    char from[MAXPATHLEN + 1];
    char to[MAXPATHLEN + 1];
    char data[RQ_LEN];
    char out[RQ_LEN];
    const struct dir *entry;
    unsigned long long added;
    uint16_t first = 0;
    uint16_t r = 0;
    uint16_t w = 0;
    int linked = 0;
    int rc;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(from, sizeof(from), "%s/%s", vol->v_path, name);
    snprintf(to, sizeof(to), "%s/%s", vol->v_path, other);

    for (int i = 0; i < RQ_LEN; i++) {
        data[i] = (char)~rq_data[i];
    }

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || link(from, to) != 0) {
        goto out;
    }

    linked = 1;

    /* the close of a written fork records its length in the entry */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &w) != AFP_OK) {
        goto out;
    }

    rc = fio_write(w, 0, 0, rq_data, RQ_LEN);
    closefork(&fio.obj, w);
    w = 0;

    if (rc != AFP_OK
            || (entry = dircache_search_by_name(vol, vol->v_root, name,
                                                strlen(name))) == NULL
            || entry->dcache_rlen != RQ_LEN) {
        goto out;
    }

    added = rfork_stat_added;

    /* the reader's first read fills the first name's entry */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &first) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                        OPENFORK_RSCS, OPENACC_RD, &r) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, other,
                        OPENFORK_RSCS, OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || fio_read(r, 0, RQ_LEN, RQ_LEN, out) != AFP_OK
            || rfork_stat_added != added + 1
            || fio_write(w, 0, 0, data, RQ_LEN) != AFP_OK) {
        goto out;
    }

    if (fio_read(r, 0, RQ_LEN, RQ_LEN, out) == AFP_OK
            && memcmp(out, data, RQ_LEN) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "rfork hard link: the other name served old bytes\n");
        ret = 1;
    }

out:

    if (w) {
        closefork(&fio.obj, w);
    }

    if (r) {
        closefork(&fio.obj, r);
    }

    if (first) {
        closefork(&fio.obj, first);
    }

    if (linked) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, other);
    }

    fio_done(vol, 0, name);
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
 * @brief A read of a symlink's resource fork fills no cache entry, as no
 *        lock shows another session's write open of a symlink
 *
 * @returns 0, 1 when the read filled the cache, -1 when the setup failed,
 *          TEST_SKIP where a symlink has no resource fork of its own
 */
int utest_rfork_cache_skips_symlink_fill(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char target[] = "rq_rcsymt";
    static char alias[] = "rq_rcsyml";
    char path[MAXPATHLEN + 1];
    char out[RQ_LEN];
    const struct dir *entry;
    unsigned long long lookups;
    unsigned long long added;
    uint16_t w = 0;
    uint16_t r = 0;
    int rc;
    int ret = -1;

    /* only the ea layout's ._ sidecar gives a symlink a resource fork */
    if (vol->v_adouble != AD_VERSION_EA
            || (vol->v_flags & AFPVOL_FOLLOWSYM)) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, alias);

    /* the link's resource fork takes a length, and its entry that length */
    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, target) != AFP_OK
            || symlink(target, path) != 0
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, alias, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK) {
        goto out;
    }

    rc = setforkparams(&fio.obj, w, RQ_LEN);
    closefork(&fio.obj, w);

    if (rc != AFP_OK
            || getfiledirparms(&fio.obj, vol->v_vid, DIRDID_ROOT,
                               alias) != AFP_OK
            || (entry = dircache_search_by_name(vol, vol->v_root, alias,
                                                strlen(alias))) == NULL
            || entry->dcache_rlen != RQ_LEN
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, alias, OPENFORK_RSCS,
                        OPENACC_RD, &r) != AFP_OK) {
        goto out;
    }

    lookups = rfork_stat_lookups;
    added = rfork_stat_added;

    /* positive control: the read looked the cache up */
    if (fio_read(r, 0, RQ_LEN, RQ_LEN, out) != AFP_OK
            || rfork_stat_lookups != lookups + 1) {
        goto out;
    }

    if (rfork_stat_added == added) {
        ret = 0;
    } else {
        fprintf(stderr, "rfork symlink: the read filled the cache\n");
        ret = 1;
    }

out:

    if (r) {
        closefork(&fio.obj, r);
    }

    delete (&fio.obj, vol->v_vid, DIRDID_ROOT, alias);
    fio_done(vol, 0, target);
    return ret;
#endif
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
 *        symlink, one reached through the symlink or whose path does not
 *        resolve included, and in one listed with a trailing slash before
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

    /* this entry is made after the first registration read the list */
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
 *        is held, and sends the event once its hold time has passed,
 *        without a second read
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

    if (idle == 0 && held == 1 && fired == 1 && after == 0) {
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
    int ret = -1;

    /* only the ea layout's ._ sidecar goes when its fork is emptied */
    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) == AFP_OK
            && (ret = fio_close_after_save(vol, name, aside, NULL, 0)) == 1) {
        fprintf(stderr, "emptied rfork: the new file's fork is gone\n");
    }

    fio_done(vol, 0, name);
    return ret;
#endif
}

/*!
 * @brief The close of an emptied resource fork keeps its ._ file on a
 *        read-only volume, and while another process has the fork open,
 *        through a read-only data descriptor too and with access none
 *
 * @returns 0, 1 when a close removed the ._ file, -1 when the setup
 *          failed, TEST_SKIP where no ._ file holds the fork or without the
 *          open() fault injector
 */
int utest_emptied_rfork_close_keeps_sidecar(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "rq_rkeep";
    const int saved_flags = vol->v_flags;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    struct peer peer;
    uint16_t w = 0;
    uint16_t r = 0;
    int held = 0;
    int ro = -1;
    int shared = -1;
    int none = -1;
    int rc;
    int ret = -1;

    /* only the ea layout's ._ sidecar goes when its fork is emptied */
    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    /* the open made the empty ._ file before the volume turned read-only */
    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK) {
        goto out;
    }

    vol->v_flags |= AFPVOL_RO;
    closefork(&fio.obj, w);
    w = 0;
    vol->v_flags = saved_flags;
    ro = access(adpath, F_OK) == 0;

    /* another session's write open of the resource fork, taken before the
     * injector is armed: the peer inherits it */
    if (ro == 1) {
        if (peer_hold_lock(&peer, path, F_RDLCK, AD_FILELOCK_RSRC_OPEN_WR,
                           1) != 0) {
            goto out;
        }

        held = 1;

        /* a user who cannot write the file gets a read-only data
         * descriptor */
        if ((rc = fio_open_readonly(vol, name, OPENFORK_RSCS, &r)) != AFP_OK) {
            ret = rc == TEST_SKIP ? TEST_SKIP : ret;
            goto out;
        }

        closefork(&fio.obj, r);
        r = 0;
        shared = access(adpath, F_OK) == 0;
        peer_release(&peer);
        held = 0;
    }

    /* another session's open of the resource fork with access none */
    if (shared == 1) {
        if (peer_hold_lock(&peer, path, F_RDLCK, AD_FILELOCK_RSRC_OPEN_NONE,
                           1) != 0) {
            goto out;
        }

        held = 1;

        if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                     OPENACC_RD, &r) != AFP_OK) {
            goto out;
        }

        closefork(&fio.obj, r);
        r = 0;
        none = access(adpath, F_OK) == 0;
    }

    if (ro == 1 && shared == 1 && none == 1) {
        ret = 0;
    } else {
        fprintf(stderr,
                "emptied rfork: ._ file kept on a read-only volume %d, under "
                "another open %d, under an access-none open %d\n", ro, shared,
                none);
        ret = 1;
    }

out:
    vol->v_flags = saved_flags;
    fault_inject_reset();

    if (held) {
        peer_release(&peer);
    }

    if (w) {
        closefork(&fio.obj, w);
    }

    unlink(adpath);
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
 *        entry, tells the other sessions and drops it from the root's
 *        cached item count; a close while this session or another process
 *        holds another fork of the file keeps it
 *
 * The first fork's write seeds a resource fork header with no resources and
 * counts the new file in the root. Its close finds the session's data fork
 * of the file open and keeps the file, as do read-only closes while another
 * process reads the data fork, denies writes to it or opens it with no
 * access; the read-only fork's close, which changes nothing, deletes it.
 *
 * @returns 0, 1 when a step disagrees, -1 when the setup failed
 */
int utest_emptied_icon_close_deletes_file(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* another process's open of the data fork for reading, its open denying
     * writers and its open with no access */
    static const off_t bands[] = { AD_FILELOCK_OPEN_RD, AD_FILELOCK_DENY_WR,
                                   AD_FILELOCK_OPEN_NONE
                                 };
    const int legs = (int)(sizeof(bands) / sizeof(bands[0]));
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    const uint32_t count = vol->v_root->d_offcnt;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    struct peer peer;
    int fds[2] = { -1, -1 };
    uint16_t w = 0;
    uint16_t d = 0;
    uint16_t r = 0;
    cnid_t id = CNID_INVALID;
    int counted = -1;
    int kept = -1;
    int shared = 0;
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
    /* a count of one item, whatever count earlier requests left */
    vol->v_root->d_offcnt = 1;

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

    counted = vol->v_root->d_offcnt == 2;
    closefork(&fio.obj, w);
    w = 0;
    kept = access(path, F_OK) == 0;
    closefork(&fio.obj, d);
    d = 0;

    for (int i = 0; kept == 1 && shared == i && i < legs; i++) {
        int rc;

        if (peer_hold_lock(&peer, path, F_RDLCK, bands[i], 1) != 0) {
            goto out;
        }

        rc = openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                      OPENACC_RD, &r);

        if (rc == AFP_OK) {
            closefork(&fio.obj, r);
            r = 0;
        }

        shared += rc == AFP_OK && access(path, F_OK) == 0;
        peer_release(&peer);
    }

    /* the session's IPC socket: what it sends its parent arrives at fds[0] */
    fio.obj.ipc_fd = fds[1];

    if (kept == 1 && shared == legs
            && openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                        OPENFORK_RSCS, OPENACC_RD, &r) == AFP_OK) {
        closefork(&fio.obj, r);
        r = 0;
        gone = access(path, F_OK) != 0 && access(adpath, F_OK) != 0;
        counted = counted && vol->v_root->d_offcnt == 1;
        recorded = cnid_get(vol->v_cdb, DIRDID_ROOT, name,
                            strlen(name)) != CNID_INVALID;
        hinted = hint_take(fds[0], vol->v_vid, id, CACHE_HINT_DELETE);
        cached = dircache_search_by_name(vol, vol->v_root, name,
                                         strlen(name)) != NULL;
    }

    if (kept == 1 && shared == legs && gone == 1 && counted == 1
            && recorded == 0 && hinted == 1 && cached == 0) {
        ret = 0;
    } else {
        fprintf(stderr,
                "emptied icon: kept %d, under another open %d of %d, gone %d, "
                "counted %d, CNID %d, hinted %d, cached %d\n", kept, shared,
                legs, gone, counted, recorded, hinted, cached);
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

    vol->v_root->d_offcnt = count;
    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(path);
    unlink(adpath);
    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief The close of a root Icon file's emptied resource fork keeps the
 *        file on a read-only volume, while DeleteInhibit protects it, and
 *        while its data fork holds bytes
 *
 * The file gets a resource fork header with no resources while a data fork
 * of it is open, so that close deletes nothing.
 *
 * @returns 0, 1 when a step disagrees, -1 when the setup failed
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
    int inhibited = -1;
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

    if (ro == 1 && fio_set_attr(vol, path, ATTRBIT_NODELETE, 1) == 0) {
        if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                     OPENACC_RD, &r) == AFP_OK) {
            closefork(&fio.obj, r);
            r = 0;
            inhibited = access(path, F_OK) == 0;
        }

        /* the data fork's bytes alone keep the file below */
        if (fio_set_attr(vol, path, ATTRBIT_NODELETE, 0) != 0) {
            inhibited = -1;
        }
    }

    if (inhibited == 1
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

    if (ro == 1 && inhibited == 1 && data == 1) {
        ret = 0;
    } else {
        fprintf(stderr,
                "emptied icon: kept on a read-only volume %d, under "
                "DeleteInhibit %d, with data %d\n", ro, inhibited, data);
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
 *        keeps the symlink, on a volume that follows symlinks too
 *
 * @returns 0, 1 when a close removed the symlink, -1 when the setup failed,
 *          or TEST_SKIP
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
    const int saved_flags = vol->v_flags;
    const int saved_options = vol->v_ad_options;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    struct stat st;
    uint16_t w = 0;
    int kept[2] = { -1, -1 };
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

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, target) == AFP_OK) {
        /* the second close follows symlinks, as the option sets the volume
         * up */
        for (int i = 0; i < 2; i++) {
            if (i == 1) {
                vol->v_flags |= AFPVOL_FOLLOWSYM;
                vol->v_ad_options |= ADVOL_FOLLO_SYML;
            }

            unlink(path);

            /* the fork holds a resource map without resources */
            if (symlink(target, path) == 0
                    && openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                                OPENFORK_RSCS, OPENACC_RD | OPENACC_WR,
                                &w) == AFP_OK) {
                const int mapped = setforkparams(&fio.obj, w,
                                                 sizeof(icon)) == AFP_OK;
                closefork(&fio.obj, w);

                if (mapped) {
                    kept[i] = lstat(path, &st) == 0 && S_ISLNK(st.st_mode);
                }
            }
        }
    }

    vol->v_flags = saved_flags;
    vol->v_ad_options = saved_options;

    if (kept[0] == 1 && kept[1] == 1) {
        ret = 0;
    } else if (kept[0] >= 0 && kept[1] >= 0) {
        fprintf(stderr,
                "emptied icon: symlink kept %d, kept when followed %d\n",
                kept[0], kept[1]);
        ret = 1;
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
 * @brief Closing an emptied resource fork of the root Icon file that another
 *        session saved over leaves the Icon file that took its name
 *
 * @returns 0, 1 when the close removed the new file, -1 when the setup
 *          failed
 */
int utest_emptied_icon_close_spares_new_file(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    static char aside[] = "rq_iconold";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) == AFP_OK
            && (ret = fio_close_after_save(vol, name, aside,
                                           (const char *)icon,
                                           sizeof(icon))) == 1) {
        fprintf(stderr, "emptied icon: the new Icon file is gone\n");
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief Add an entry named eaname to the 'ea = ad' EA header of path with
 *        no EA file behind it
 *
 * @returns 0, or -1 when the header could not be written
 */
static int ea_header_lists(const struct vol *vol, const char *path,
                           const char *eaname)
{
    struct ea ea;
    struct ea_entry(*entries)[];
    int ret = -1;

    if (ea_open(vol, path, EA_CREATE | EA_RDWR, &ea) != 0) {
        return -1;
    }

    entries = realloc(ea.ea_entries,
                      sizeof(struct ea_entry) * (ea.ea_count + 1));

    if (entries != NULL) {
        ea.ea_entries = entries;
        (*ea.ea_entries)[ea.ea_count].ea_size = 1;
        (*ea.ea_entries)[ea.ea_count].ea_namelen = strlen(eaname);
        (*ea.ea_entries)[ea.ea_count].ea_name = strdup(eaname);

        if ((*ea.ea_entries)[ea.ea_count].ea_name != NULL) {
            ea.ea_count++;
            ret = 0;
        }
    }

    return ea_close(&ea) == 0 ? ret : -1;
}

/*!
 * @brief The 'ea = ad' EAs of a file: the removal of its EAs, which FPDelete
 *        and the emptied Icon file's close run, returns past an EA file it
 *        cannot remove, which the header keeps listing; a failed write of a
 *        new EA leaves no entry and no EA file, but an entry for an EA file
 *        it cannot remove, and the removal drops an entry no EA file backs
 *
 * An EA name with a slash names no EA file, a file size limit fails a
 * write after it made its EA file, and a directory under an EA file's name
 * fails both the write and the unlink.
 *
 * @returns 0, 1 when an entry or a result disagrees, -1 when the setup
 *          failed, TEST_SKIP on macOS's ea layout, which keeps no EA header
 *          file, or where the platform unlinks a directory
 */
int utest_ea_deletefile_returns_past_stuck_ea(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_eastuck";
    static char big[2048];
    char path[MAXPATHLEN + 1];
    char header[MAXPATHLEN + 1];
    char stuck[MAXPATHLEN + 1];
    char gone[MAXPATHLEN + 1];
    char inner[MAXPATHLEN + 1];
    struct fsize_cap cap;
    struct ea ea;
    int kept = -1;
    int listed = -1;
    int unlisted = -1;
    int dropped = -1;
    int fd;
    int rc;
    int ret = -1;
#ifdef __APPLE__

    /* the layout names the EA header under the file's named fork */
    if (vol->v_adouble == AD_VERSION_EA) {
        return TEST_SKIP;
    }

#endif

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(header, sizeof(header), "%s/%s::EA", vol->v_path,
             vol->ad_path(name, 0));
    snprintf(stuck, sizeof(stuck), "%s::rq_ea", header);
    snprintf(gone, sizeof(gone), "%s::rq_eb", header);
    snprintf(inner, sizeof(inner), "%s/x", stuck);

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK) {
        goto out;
    }

    /* an EA file the removal cannot unlink, a directory, stays listed after
     * one it removed */
    if (set_ea(vol, path, "rq_eb", "x", 1, 0, -1) != AFP_OK
            || set_ea(vol, path, "rq_ea", "x", 1, 0, -1) != AFP_OK
            || unlink(stuck) != 0 || mkdir(stuck, 0700) != 0
            || (fd = open(inner, O_CREAT | O_WRONLY, 0600)) < 0) {
        goto out;
    }

    close(fd);
    rc = ea_deletefile(vol, -1, path);

    if (access(stuck, F_OK) != 0) {
        ret = TEST_SKIP;
        goto out;
    }

    if (ea_open(vol, path, EA_RDONLY, &ea) == 0) {
        kept = rc == AFPERR_MISC && ea.ea_count == 1
               && strcmp((*ea.ea_entries)[0].ea_name, "rq_ea") == 0
               && access(gone, F_OK) != 0;
        ea_close(&ea);
    }

    /* a failed write of a new EA whose EA file cannot go, the directory
     * left under its name, keeps the entry that lists it */
    unlink(header);
    rc = set_ea(vol, path, "rq_ea", "x", 1, 0, -1);

    if (ea_open(vol, path, EA_RDONLY, &ea) == 0) {
        listed = rc == AFPERR_MISC && ea.ea_count == 1
                 && strcmp((*ea.ea_entries)[0].ea_name, "rq_ea") == 0;
        ea_close(&ea);
    } else {
        listed = 0;
    }

    unlink(inner);
    rmdir(stuck);
    unlink(header);
    /* a write that fails leaves no entry, and no EA file where it made one */
    rc = set_ea(vol, path, "rq/ea", "x", 1, 0, -1);
    unlisted = rc == AFPERR_MISC && access(header, F_OK) != 0;
    unlink(header);

    if (fsize_cap(&cap, 1024) != 0) {
        goto out;
    }

    rc = set_ea(vol, path, "rq_ea", big, sizeof(big), 0, -1);
    fsize_uncap(&cap);
    unlisted = unlisted && rc == AFPERR_MISC && access(stuck, F_OK) != 0
               && access(header, F_OK) != 0;
    unlink(stuck);
    unlink(header);

    if (ea_header_lists(vol, path, "rq/ea") == 0) {
        rc = ea_deletefile(vol, -1, path);
        dropped = rc == AFP_OK && access(header, F_OK) != 0;
    }

    if (kept == 1 && listed == 1 && unlisted == 1 && dropped == 1) {
        ret = 0;
    } else {
        fprintf(stderr,
                "EA removal: a stuck EA kept %d, a failed write's stuck EA "
                "listed %d, a failed write unlisted %d, an entry without a "
                "file dropped %d\n", kept, listed, unlisted, dropped);
        ret = 1;
    }

out:
    unlink(inner);
    rmdir(stuck);
    unlink(gone);
    unlink(header);
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief An 'ea = ad' EA set whose header write fails replies AFPERR_MISC
 *
 * @returns 0, 1 when the set crashed or replied otherwise, -1 when the setup
 *          failed, TEST_SKIP on macOS's ea layout, which keeps no EA header
 *          file
 */
int utest_set_ea_failed_close_replies_misc(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_eaclose";
    char path[MAXPATHLEN + 1];
    char header[MAXPATHLEN + 1];
    char eafile[MAXPATHLEN + 1];
    struct fsize_cap cap;
    int fds[2] = { -1, -1 };
    char reply = 0;
    pid_t pid;
    int ret = -1;
#ifdef __APPLE__

    /* the layout names the EA header under the file's named fork */
    if (vol->v_adouble == AD_VERSION_EA) {
        return TEST_SKIP;
    }

#endif

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(header, sizeof(header), "%s/%s::EA", vol->v_path,
             vol->ad_path(name, 0));
    snprintf(eafile, sizeof(eafile), "%s::rq_ea", header);

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || pipe(fds) != 0) {
        goto out;
    }

    /* the EA's byte fits a file size limit that its header does not; the set
     * runs in a child, whose crash spares the run, and its reply comes down
     * the pipe, as valgrind rewrites the exit status of a child that holds
     * the harness's memory */
    fflush(NULL);

    if ((pid = fork()) == 0) {
        char r = 's';

        if (fsize_cap(&cap, 12) == 0) {
            r = set_ea(vol, path, "rq_ea", "x", 1, 0, -1) == AFPERR_MISC
                ? 'y' : 'n';
        }

        _exit(write(fds[1], &r, 1) == 1 ? 0 : 1);
    }

    close(fds[1]);

    /* a crash, or a sanitizer's exit after one, sends no reply */
    if (pid > 0) {
        const ssize_t n = read(fds[0], &reply, 1);
        (void)waitpid(pid, NULL, 0);

        if (n == 1 && reply == 'y') {
            ret = 0;
        } else if (n != 1 || reply != 's') {
            fprintf(stderr, "EA set: a failed header write %s\n",
                    n == 1 ? "replied otherwise" : "crashed the set");
            ret = 1;
        }
    }

out:

    if (fds[0] >= 0) {
        close(fds[0]);
    }

    unlink(eafile);
    unlink(header);
    fio_done(vol, 0, name);
    return ret;
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
 * @brief FPGetForkParms of a virtual Icon fork returns the length of its
 *        own fork and attributes that show it open, and a request for the
 *        other fork's length fails with AFPERR_BITMAP, as does an FPOpenFork
 *        that asks for it
 *
 * @returns 0, 1 when a reply is another code, -1 when the setup failed
 */
int utest_virtual_icon_params_name_its_fork(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char req[6] = { 0 };
    char rep[16];
    size_t replen = sizeof(rep);
    const uint16_t bitmap = htons(1 << FILPBIT_ATTR);
    uint16_t attr = 0;
    uint16_t r = 0;
    uint16_t o = 0;
    int own = -1;
    int other = -1;
    int asked = -1;
    int ret;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) == AFP_OK) {
        own = getforkparams(&fio.obj, r, 1 << FILPBIT_RFLEN);
        other = getforkparams(&fio.obj, r, 1 << FILPBIT_DFLEN);
        /* the attributes follow the reply's bitmap */
        memcpy(req + 2, &r, sizeof(r));
        memcpy(req + 4, &bitmap, sizeof(bitmap));

        if (afp_getforkparams(&fio.obj, req, sizeof(req), rep,
                              &replen) == AFP_OK) {
            memcpy(&attr, rep + 2, sizeof(attr));
        }

        closefork(&fio.obj, r);
    }

    asked = openfork_bitmap(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                            OPENFORK_RSCS, OPENACC_RD, 1 << FILPBIT_DFLEN, &o);

    if (asked == AFP_OK) {
        closefork(&fio.obj, o);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;

    if (own == AFP_OK && other == AFPERR_BITMAP
            && (ntohs(attr) & ATTRBIT_ROPEN) && asked == AFPERR_BITMAP) {
        ret = 0;
    } else {
        fprintf(stderr,
                "virtual icon params: own fork %d, other fork %d, attributes "
                "0x%04x, open asking for the other fork %d\n", own, other,
                ntohs(attr), asked);
        ret = 1;
    }

    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief A first write whose create of the Icon file fails with EACCES
 *        replies AFPERR_ACCESS, also when the kernel answers the refusal
 *        with EEXIST and no file exists, and a length set whose create fails
 *        with another error AFPERR_MISC; on a read-only file system the
 *        write replies AFPERR_ACCESS, as FPWrite has no AFPERR_VLOCK, and the
 *        length set AFPERR_VLOCK
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
    uint16_t d = 0;
    int rc = -1;
    int set = -1;
    int rofs[2] = { -1, -1 };
    int exist = -1;
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
        fi.open_fail_after = 0;
        fi.open_errno = EROFS;
        rofs[0] = fio_write(w, 0, 0, rq_data, 1);
        fi.open_fail_after = 0;
        rofs[1] = setforkparams(&fio.obj, w, 32);
        fault_inject_reset();
        closefork(&fio.obj, w);
    }

    /* a data fork's reopen creates nothing, so a refusal answered with
     * EEXIST leaves no file to open */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD | OPENACC_WR, &d) == AFP_OK) {
        fault_inject_reset();
        fi.open_armed = 1;
        fi.open_fail_after = 0;
        fi.open_errno = EEXIST;
        fi.open_fail_flags = O_CREAT | O_EXCL;
        exist = fio_write(d, 0, 0, rq_data, 1);
        fault_inject_reset();
        closefork(&fio.obj, d);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;

    if (rc == AFPERR_ACCESS && set == AFPERR_MISC
            && rofs[0] == AFPERR_ACCESS && rofs[1] == AFPERR_VLOCK
            && exist == AFPERR_ACCESS) {
        ret = 0;
    } else {
        fprintf(stderr,
                "refused icon create: write %d, length set %d; on a "
                "read-only file system write %d, length set %d; refused "
                "with EEXIST %d\n", rc, set, rofs[0], rofs[1], exist);
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
 * @brief The close of the root Icon file's resource fork keeps a new file
 *        whose resource fork it did not change and that holds no resource
 *        map
 *
 * FPCreateFile's file reads no resource bytes: a read-only open's close and
 * a read-write open's close without a write keep it, while the close of a
 * fork that cut its resource fork to nothing removes it.
 *
 * @returns 0, 1 when a close disagrees, -1 when the setup failed
 */
int utest_emptied_icon_close_keeps_unwritten(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    uint16_t f = 0;
    int ro = -1;
    int rw = -1;
    int cut = -1;
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
                        OPENACC_RD, &f) != AFP_OK) {
        goto out;
    }

    closefork(&fio.obj, f);
    f = 0;
    ro = access(path, F_OK) == 0;

    if (ro == 1
            && openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &f) == AFP_OK) {
        closefork(&fio.obj, f);
        f = 0;
        rw = access(path, F_OK) == 0;
    }

    if (rw == 1
            && openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &f) == AFP_OK) {
        const int emptied = fio_write(f, 0, 0, rq_data, 32) == AFP_OK
                            && setforkparams(&fio.obj, f, 0) == AFP_OK;
        closefork(&fio.obj, f);
        f = 0;
        cut = emptied && access(path, F_OK) != 0;
    }

    if (ro == 1 && rw == 1 && cut == 1) {
        ret = 0;
    } else {
        fprintf(stderr,
                "unwritten icon: kept after a read %d, after a write open "
                "without a write %d, gone after a cut %d\n", ro, rw, cut);
        ret = 1;
    }

out:

    if (f) {
        closefork(&fio.obj, f);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(path);
    unlink(adpath);
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief The close of the root Icon file's resource fork keeps the file while
 *        the adouble reads another name's resource fork: a hard link's forks
 *        share the adouble, whose resource fork is the first opener's
 *
 * @returns 0, 1 when the Icon file lost its resource fork, -1 when the setup
 *          failed, TEST_SKIP where the resource fork belongs to the file
 */
int utest_emptied_icon_close_keeps_link_icon(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    static char other[] = "rq_iconlink";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char linkpath[MAXPATHLEN + 1];
    char out[RQ_LEN];
    uint16_t w = 0;
    uint16_t l = 0;
    uint16_t r = 0;
    int linked = 0;
    int rc = -1;
    int ret = -1;
#ifdef HAVE_EAFD

    /* a resource fork in an EA is the file's under every name */
    if (vol->v_adouble == AD_VERSION_EA) {
        return TEST_SKIP;
    }

#endif

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(linkpath, sizeof(linkpath), "%s/%s", vol->v_path, other);

    /* the Icon file's resource map holds resources */
    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || fio_write(w, 0, 0, rq_data, RQ_LEN) != AFP_OK
            || closefork(&fio.obj, w) != AFP_OK) {
        goto out;
    }

    w = 0;

    if (link(path, linkpath) != 0) {
        goto out;
    }

    linked = 1;

    /* the link's own resource fork holds a map without resources, and the
     * Icon fork's open shares the link fork's adouble */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, other, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &l) != AFP_OK
            || fio_write(l, 0, 0, (const char *)icon, sizeof(icon)) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) != AFP_OK
            || closefork(&fio.obj, l) != AFP_OK) {
        goto out;
    }

    l = 0;
    closefork(&fio.obj, r);
    r = 0;

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) == AFP_OK) {
        rc = fio_read(r, 0, RQ_LEN, RQ_LEN, out);
    }

    if (rc == AFP_OK && memcmp(out, rq_data, RQ_LEN) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "icon link: the Icon file's resource fork read %d\n",
                rc);
        ret = 1;
    }

out:

    if (w) {
        closefork(&fio.obj, w);
    }

    if (l) {
        closefork(&fio.obj, l);
    }

    if (linked) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, other);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief The close of the root Icon file's emptied resource fork keeps the
 *        file under another process's byte lock on it, where the fork's
 *        change still reaches the siblings, and under a DeleteInhibit set
 *        after the fork opened, where the unchanged file gives them no hint;
 *        the removal from a cached item count of 0 drops the count
 *
 * @returns 0, 1 when a step disagrees, -1 when the setup failed
 */
int utest_emptied_icon_close_keeps_claimed(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    const uint32_t count = vol->v_root->d_offcnt;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    struct peer peer;
    struct stat st;
    int fds[2] = { -1, -1 };
    cnid_t id = CNID_INVALID;
    uint16_t w = 0;
    uint16_t d = 0;
    uint16_t r = 0;
    int locked = -1;
    int inhibited = -1;
    int zero = -1;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    /* the session's open data fork keeps the emptied file */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0
            || setnonblock(fds[0], 1) != 0
            || createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || (id = cnid_get(vol->v_cdb, DIRDID_ROOT, name,
                              strlen(name))) == CNID_INVALID
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

    /* another process's lock on the data fork's first byte */
    if (access(path, F_OK) != 0
            || peer_hold_lock(&peer, path, F_RDLCK, 0, 1) != 0) {
        goto out;
    }

    /* the fork writes the map again: its refused removal tells the siblings
     * of the change */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &r) == AFP_OK) {
        const int wrote = fio_write(r, 0, 0, (const char *)icon,
                                    sizeof(icon)) == AFP_OK;
        fio.obj.ipc_fd = fds[1];
        closefork(&fio.obj, r);
        fio.obj.ipc_fd = -1;
        r = 0;
        locked = wrote && access(path, F_OK) == 0
                 && hint_take(fds[0], vol->v_vid, id, CACHE_HINT_REFRESH) == 1;
    }

    peer_release(&peer);

    /* the fork opens on a file without metadata on adouble:ea, and reads the
     * header in core on adouble:v2, where the AppleDouble file holds it
     * beside the resource fork */
    if (locked == 1 && vol->v_adouble == AD_VERSION_EA
            && sys_lremovexattr(path, AD_EA_META) != 0) {
        inhibited = -1;
    } else if (locked == 1
               && openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                           OPENFORK_RSCS, OPENACC_RD, &r) == AFP_OK) {
        const int set = fio_set_attr(vol, path, ATTRBIT_NODELETE, 1) == 0;
        /* the session's IPC socket: what it sends its parent arrives at
         * fds[0] */
        fio.obj.ipc_fd = fds[1];
        closefork(&fio.obj, r);
        fio.obj.ipc_fd = -1;
        r = 0;
        inhibited = set ? access(path, F_OK) == 0
                    && hint_take(fds[0], vol->v_vid, CNID_INVALID,
                                 CACHE_HINT_REFRESH) == 0 : -1;

        if (fio_set_attr(vol, path, ATTRBIT_NODELETE, 0) != 0) {
            inhibited = -1;
        }
    }

    /* a count of no items, current by the root's ctime */
    if (inhibited == 1 && stat(vol->v_path, &st) == 0) {
        vol->v_root->d_offcnt = 0;
        vol->v_root->d_ctime = st.st_ctime;

        if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                     OPENACC_RD, &r) == AFP_OK) {
            closefork(&fio.obj, r);
            r = 0;
            zero = access(path, F_OK) != 0 && vol->v_root->d_offcnt == 0
                   && vol->v_root->d_ctime == 0;
        }

        vol->v_root->d_offcnt = count;
        vol->v_root->d_ctime = 0;
    }

    if (locked == 1 && inhibited == 1 && zero == 1) {
        ret = 0;
    } else {
        fprintf(stderr,
                "claimed icon: kept under a byte lock with a hint %d, under "
                "a later DeleteInhibit with no hint %d, dropped a count of 0 "
                "%d\n", locked, inhibited, zero);
        ret = 1;
    }

out:

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
 * @brief The close of a symlink's emptied resource fork keeps the link's ._
 *        file, as no probe can see who else holds it
 *
 * @returns 0, 1 when the close removed the ._ file, -1 when the setup failed,
 *          TEST_SKIP where no ._ file holds the fork or the volume follows
 *          the link
 */
int utest_symlink_emptied_rfork_keeps_sidecar(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char target[] = "rq_slkt";
    static char alias[] = "rq_slkl";
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    const struct ofork *of;
    uint16_t w = 0;
    int kept = -1;
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
    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(alias, 0));

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, target) != AFP_OK
            || symlink(target, path) != 0
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, alias, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || (of = of_find(w)) == NULL) {
        goto out;
    }

    if (ad_data_fileno(of->of_ad) != AD_SYMLINK) {
        ret = TEST_SKIP;
        goto out;
    }

    closefork(&fio.obj, w);
    w = 0;
    kept = access(adpath, F_OK) == 0;

    if (kept == 1) {
        ret = 0;
    } else {
        fprintf(stderr, "symlink rfork: the link's ._ file kept %d\n", kept);
        ret = 1;
    }

out:

    if (w) {
        closefork(&fio.obj, w);
    }

    delete (&fio.obj, vol->v_vid, DIRDID_ROOT, alias);
    unlink(adpath);
    fio_done(vol, 0, target);
    return ret;
#endif
}

/*!
 * @brief A first write whose seed of the created Icon file cannot create its
 *        AppleDouble file replies AFPERR_ACCESS, leaves the fork virtual and
 *        removes the file
 *
 * @returns 0, 1 when the write made the file real, -1 when the setup failed,
 *          TEST_SKIP without the open() fault injector or where the seed
 *          creates no file through open()
 */
int utest_icon_seed_refused_stays_virtual(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    const struct ofork *of;
    uint16_t d = 0;
    int fired = 0;
    int virt = -1;
    int gone = -1;
    int rc = -1;
    int ret;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
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

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD | OPENACC_WR, &d) == AFP_OK) {
        /* the exclusive create passes, the seed's create of the AppleDouble
         * file fails */
        fault_inject_reset();
        fi.open_armed = 1;
        fi.open_fail_after = 1;
        fi.open_errno = EACCES;
        fi.open_fail_flags = O_CREAT;
        rc = fio_write(d, 0, 0, rq_data, 1);
        fired = fi.open_fail_after == -1;
        fault_inject_reset();
        virt = (of = of_find(d)) != NULL && (of->of_flags & AFPFORK_VIRTUAL);
        gone = access(path, F_OK) != 0;
        closefork(&fio.obj, d);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;

    if (rc != -1 && !fired) {
        ret = TEST_SKIP;
    } else if (rc == AFPERR_ACCESS && virt == 1 && gone == 1) {
        ret = 0;
    } else {
        fprintf(stderr, "refused icon seed: write %d, virtual %d, gone %d\n",
                rc, virt, gone);
        ret = 1;
    }

    unlink(adpath);
    unlink(path);
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief A first write whose open-mode locks fail for another reason than a
 *        deny mode replies AFPERR_MISC, logs the failure at error level and
 *        removes the file it created
 *
 * @returns 0, 1 when the reply, the log or the file disagrees, -1 when the
 *          setup failed, TEST_SKIP without the fault injector
 */
int utest_icon_lock_error_logged(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    char logf[MAXPATHLEN + 1];
    uint16_t d = 0;
    int found = -1;
    int gone = -1;
    int rc = -1;
    int ret;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
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
    snprintf(logf, sizeof(logf), "%s/rq_locklog.log", vol->v_path);

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD | OPENACC_WR, &d) == AFP_OK) {
        setuplog("default:error", logf, true);
        /* the first fcntl() is the open-mode lock probe */
        fault_inject_reset();
        fi.fcntl_armed = 1;
        fi.fcntl_fail_after = 0;
        fi.fcntl_errno = ENOLCK;
        rc = fio_write(d, 0, 0, rq_data, 1);
        fault_inject_reset();
        setuplog("default:note", "/dev/stderr", true);
        gone = access(path, F_OK) != 0;
        closefork(&fio.obj, d);
        found = conf_log_contains(logf, "open-mode locks refused");
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;

    if (rc == AFPERR_MISC && found == 1 && gone == 1) {
        ret = 0;
    } else if (found == -1) {
        ret = -1;
    } else {
        fprintf(stderr, "icon lock error: write %d, logged %d, gone %d\n", rc,
                found, gone);
        ret = 1;
    }

    unlink(logf);
    unlink(adpath);
    unlink(path);
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief A first write whose open-mode locks fail after another process
 *        renamed its file onto the Icon file's name keeps that file: the
 *        cleanup removes only the file the write created
 *
 * @returns 0, 1 when the cleanup deleted the other file, -1 when the setup
 *          failed, TEST_SKIP without the fault injector
 */
int utest_icon_cleanup_spares_swapped_file(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    static char swap[] = "rq_iconswap";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    char swappath[MAXPATHLEN + 1];
    size_t rbuflen = DSI_DATASIZ;
    const struct ofork *of;
    struct stat st;
    ino_t swapped;
    uint16_t v = 0;
    int fired;
    int fd;
    int rc;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
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
    snprintf(swappath, sizeof(swappath), "%s/%s", vol->v_path, swap);

    /* a write-only fork's locks take one probe before the first lock */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_WR, &v) != AFP_OK
            || (fd = open(swappath, O_CREAT | O_WRONLY, 0600)) < 0) {
        goto out;
    }

    swapped = fstat(fd, &st) == 0 ? st.st_ino : 0;
    close(fd);

    if (swapped == 0 || fio_send(0, v, 0, 1, rq_data) != 0) {
        goto out;
    }

    /* another process's rename onto the name lands at the first lock, which
     * fails */
    fault_inject_reset();
    fi.rename_from = swappath;
    fi.rename_to = path;
    fi.rename_armed = 1;
    fi.fcntl_armed = 1;
    fi.fcntl_fail_after = 1;
    fi.fcntl_errno = ENOLCK;
    rc = afp_write_ext(&fio.obj, (char *)fio.dsi.commands, fio.dsi.cmdlen,
                       (char *)fio.dsi.data, &rbuflen);
    fired = !fi.rename_armed && fi.fcntl_fail_after == -1;
    fault_inject_reset();

    if (!fired) {
        goto out;
    }

    if (rc == AFPERR_MISC && (of = of_find(v)) != NULL
            && (of->of_flags & AFPFORK_VIRTUAL)
            && stat(path, &st) == 0 && st.st_ino == swapped) {
        ret = 0;
    } else {
        fprintf(stderr, "swapped icon create: write %d, the file that took "
                        "the name gone %d\n", rc, stat(path, &st) != 0);
        ret = 1;
    }

out:
    fault_inject_reset();

    if (v) {
        closefork(&fio.obj, v);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(adpath);
    unlink(path);
    unlink(swappath);
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief A virtual Icon fork's first write onto an Icon file that is real
 *        and write-inhibited replies AFPERR_ACCESS and leaves the fork
 *        virtual, as a write open of the file is refused; on a volume that
 *        ignores WriteInhibit the write lands
 *
 * @returns 0, 1 when a write disagrees, -1 when the setup failed
 */
int utest_virtual_icon_write_inhibited_stays_virtual(AFPObj *obj,
                                                     struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    const uint32_t saved_ignattr = vol->v_ignattr;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    const struct ofork *of;
    uint16_t w = 0;
    uint16_t x = 0;
    int locked = -1;
    int virt = -1;
    int ignored = -1;
    int rc = -1;
    int fd;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    /* the virtual fork opens first, then the file turns real and
     * write-inhibited */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || (fd = open(path, O_CREAT | O_WRONLY, 0600)) < 0) {
        goto out;
    }

    close(fd);

    if (fio_set_attr(vol, path, ATTRBIT_NOWRITE, 1) != 0) {
        goto out;
    }

    rc = openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                  OPENACC_RD | OPENACC_WR, &x);
    locked = rc == AFPERR_OLOCK;

    if (rc == AFP_OK) {
        closefork(&fio.obj, x);
    }

    rc = fio_write(w, 0, 0, rq_data, 1);
    virt = (of = of_find(w)) != NULL && (of->of_flags & AFPFORK_VIRTUAL);

    if (virt == 1) {
        vol->v_ignattr |= ATTRBIT_NOWRITE;
        ignored = fio_write(w, 0, 0, rq_data, 1) == AFP_OK
                  && (of = of_find(w)) != NULL
                  && !(of->of_flags & AFPFORK_VIRTUAL);
        vol->v_ignattr = saved_ignattr;
    }

    if (locked == 1 && rc == AFPERR_ACCESS && virt == 1 && ignored == 1) {
        ret = 0;
    } else {
        fprintf(stderr,
                "write-inhibited icon: write open refused %d, write %d, "
                "virtual %d, landed where ignored %d\n", locked, rc, virt,
                ignored);
        ret = 1;
    }

out:

    if (w) {
        closefork(&fio.obj, w);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(adpath);
    unlink(path);
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief A virtual Icon fork's first write whose resource fork open fails on
 *        the adouble the session's data fork lends leaves the lender's
 *        resource fork length
 *
 * @returns 0, 1 when the lender's length changed, -1 when the setup failed,
 *          TEST_SKIP without the fault injector or where no ._ file holds
 *          the resource fork
 */
int utest_icon_refused_borrow_keeps_length(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    const struct ofork *of;
    size_t rbuflen = DSI_DATASIZ;
    uint16_t v = 0;
    uint16_t x = 0;
    uint16_t l = 0;
    off_t rlen = -1;
    int rc = -1;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
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

    /* a virtual fork, then a real Icon file with resources, whose data fork
     * the session opens */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &v) != AFP_OK
            || createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &x) != AFP_OK
            || fio_write(x, 0, 0, rq_data, RQ_LEN) != AFP_OK
            || closefork(&fio.obj, x) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                        OPENACC_RD, &l) != AFP_OK
            || (of = of_find(l)) == NULL
            || of->of_ad->ad_rlen != RQ_LEN
            || fio_send(0, v, 0, 1, rq_data) != 0) {
        goto out;
    }

    /* the borrowed resource fork open's header read fails */
    fault_inject_reset();
    fi.read.armed = 1;
    fi.read.fail_after = 0;
    fi.read.errnum = EIO;
    rc = afp_write_ext(&fio.obj, (char *)fio.dsi.commands, fio.dsi.cmdlen,
                       (char *)fio.dsi.data, &rbuflen);
    fault_inject_reset();
    rlen = (of = of_find(l)) != NULL ? of->of_ad->ad_rlen : -1;

    if (rc == AFPERR_MISC && rlen == RQ_LEN) {
        ret = 0;
    } else {
        fprintf(stderr, "refused borrow: write %d, lender's length %jd\n", rc,
                (intmax_t)rlen);
        ret = 1;
    }

out:
    fault_inject_reset();

    if (v) {
        closefork(&fio.obj, v);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(adpath);
    fio_done(vol, l, name);
    return ret;
#endif
}

/*!
 * @brief Count the delete and refresh hints for id on vid waiting at the
 *        parent's end of the session's IPC socket
 */
static void hints_count(int fd, uint16_t vid, cnid_t id, int *deleted,
                        int *refreshed)
{
    char msg[IPC_HEADERLEN + sizeof(struct ipc_cache_hint_payload)];
    struct ipc_cache_hint_payload hint;
    uint16_t cmd;

    while (read(fd, msg, sizeof(msg)) == (ssize_t)sizeof(msg)) {
        memcpy(&cmd, msg, sizeof(cmd));
        memcpy(&hint, msg + IPC_HEADERLEN, sizeof(hint));

        if (cmd == IPC_CACHE_HINT && hint.vid == vid && hint.cnid == id) {
            *deleted += hint.event == CACHE_HINT_DELETE;
            *refreshed += hint.event == CACHE_HINT_REFRESH;
        }
    }
}

/*!
 * @brief An FPDelete of a file whose fork the session modified and holds
 *        tells the sibling sessions of the deletion, and the close of the
 *        fork after it sends no refresh for the gone file; a modified fork
 *        under another name of a hard link still sends its refresh
 *
 * Where the names keep metadata of their own, adouble:v2, the other name's
 * fork has no id to send.
 *
 * @returns 0, 1 when a delete hint is missing or a refresh disagrees, -1
 *          when the setup failed
 */
int utest_delete_sweep_sends_no_refresh(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_dsweep";
    static char other[] = "rq_dsweepb";
    char from[MAXPATHLEN + 1];
    char to[MAXPATHLEN + 1];
    int fds[2] = { -1, -1 };
    int deleted[2] = { 0, 0 };
    int refreshed[2] = { 0, 0 };
    const int ea = vol->v_adouble == AD_VERSION_EA;
    uint16_t w = 0;
    cnid_t id = CNID_INVALID;
    int linked = 0;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(from, sizeof(from), "%s/%s", vol->v_path, name);
    snprintf(to, sizeof(to), "%s/%s", vol->v_path, other);

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0
            || setnonblock(fds[0], 1) != 0
            || fio_dfork(vol, name, &w) != 0
            || (id = cnid_get(vol->v_cdb, DIRDID_ROOT, name,
                              strlen(name))) == CNID_INVALID) {
        goto out;
    }

    fio.obj.ipc_fd = fds[1];

    /* the fork's first change tells the siblings at once */
    if (fio_write(w, 0, 0, rq_data, 64) != AFP_OK
            || hint_take(fds[0], vol->v_vid, id, CACHE_HINT_REFRESH) != 1
            || delete (&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK) {
        goto out;
    }

    /* the delete closed the fork */
    w = 0;
    hints_count(fds[0], vol->v_vid, id, &deleted[0], &refreshed[0]);

    /* the same under the other name of a hard link, which the delete
     * leaves; the create's own hints go nowhere */
    if (ea) {
        fio.obj.ipc_fd = -1;

        if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
                || (id = cnid_get(vol->v_cdb, DIRDID_ROOT, name,
                                  strlen(name))) == CNID_INVALID
                || link(from, to) != 0) {
            goto out;
        }

        linked = 1;

        if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, other, OPENFORK_DATA,
                     OPENACC_RD | OPENACC_WR, &w) != AFP_OK) {
            goto out;
        }

        fio.obj.ipc_fd = fds[1];

        if (fio_write(w, 0, 0, rq_data, 64) != AFP_OK
                || hint_take(fds[0], vol->v_vid, id, CACHE_HINT_REFRESH) != 1
                || delete (&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK) {
            goto out;
        }

        w = 0;
        hints_count(fds[0], vol->v_vid, id, &deleted[1], &refreshed[1]);
    }

    if (deleted[0] == 1 && refreshed[0] == 0
            && (!ea || (deleted[1] == 1 && refreshed[1] == 1))) {
        ret = 0;
    } else {
        fprintf(stderr,
                "delete sweep: %d delete and %d refresh hints, under a hard "
                "link's other name %d and %d\n", deleted[0], refreshed[0],
                deleted[1], refreshed[1]);
        ret = 1;
    }

out:
    fio.obj.ipc_fd = -1;

    if (fds[0] >= 0) {
        close(fds[0]);
        close(fds[1]);
    }

    if (linked) {
        if (w) {
            closefork(&fio.obj, w);
            w = 0;
        }

        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, other);
    }

    fio_done(vol, w, name);
    return ret;
}

/*!
 * @brief The close of a root Icon file's emptied resource fork through a
 *        read-only data descriptor keeps the file while another process
 *        reads it
 *
 * @returns 0, 1 when the close deleted the file, -1 when the setup failed,
 *          TEST_SKIP without the open() fault injector
 */
int utest_emptied_icon_close_sees_peer_read_only(AFPObj *obj,
                                                 struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    struct peer reader;
    uint16_t w = 0;
    uint16_t d = 0;
    uint16_t r = 0;
    int held = 0;
    int rc;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
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

    /* the open data fork keeps the emptied file at the writer's close */
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

    /* another process reads the data fork, its lock taken while the
     * injector, which the peer's process inherits, is disarmed */
    if (peer_hold_lock(&reader, path, F_RDLCK, AD_FILELOCK_OPEN_RD, 1) != 0) {
        goto out;
    }

    held = 1;

    /* a user who cannot write the file gets a read-only data descriptor */
    if ((rc = fio_open_readonly(vol, name, OPENFORK_RSCS, &r)) != AFP_OK) {
        ret = rc == TEST_SKIP ? TEST_SKIP : ret;
        goto out;
    }

    closefork(&fio.obj, r);
    r = 0;

    if (access(path, F_OK) == 0) {
        ret = 0;
    } else {
        fprintf(stderr,
                "emptied icon: deleted under another process's read, closed "
                "through a read-only descriptor\n");
        ret = 1;
    }

out:
    fault_inject_reset();

    if (held) {
        peer_release(&reader);
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
 * @brief A first write to the virtual Icon fork replies AFPERR_ACCESS and
 *        leaves the fork virtual, holding nothing, while the session's open
 *        fork of the file holds a read-only data descriptor
 *
 * @returns 0, 1 when the write is not refused, -1 when the setup failed,
 *          TEST_SKIP without the open() fault injector
 */
int utest_icon_write_refuses_readonly_lender(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    const struct ofork *of;
    uint16_t v = 0;
    uint16_t l = 0;
    int held = -1;
    int rc = -1;
    int fd;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
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

    /* the virtual fork opens first: a real Icon file is served as itself */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &v) != AFP_OK
            || (fd = open(path, O_CREAT | O_WRONLY, 0600)) < 0) {
        goto out;
    }

    close(fd);

    /* a user who cannot write the file gets a read-only data descriptor */
    if ((rc = fio_open_readonly(vol, name, OPENFORK_DATA, &l)) != AFP_OK) {
        ret = rc == TEST_SKIP ? TEST_SKIP : ret;
        goto out;
    }

    rc = fio_write(v, 0, 0, rq_data, 1);

    if ((of = of_find(v)) != NULL) {
        held = !(of->of_flags & AFPFORK_VIRTUAL)
               || ad_data_fileno(of->of_ad) != -1
               || ad_reso_fileno(of->of_ad) != -1
               || ad_meta_fileno(of->of_ad) != -1;
    }

    if (rc == AFPERR_ACCESS && held == 0) {
        ret = 0;
    } else {
        fprintf(stderr,
                "virtual icon over a read-only open: write %d, held %d\n",
                rc, held);
        ret = 1;
    }

out:
    fault_inject_reset();

    if (l) {
        closefork(&fio.obj, l);
    }

    if (v) {
        closefork(&fio.obj, v);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(adpath);
    unlink(path);
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief A first write to the virtual Icon fork replies AFPERR_ACCESS and
 *        leaves the fork virtual while a symlink holds the name, also when
 *        the session holds a fork of the link, which keeps its emulated
 *        descriptor
 *
 * @returns 0, 1 when a write was not refused or the link's fork lost its
 *          descriptor, -1 when the setup failed, TEST_SKIP on a volume that
 *          follows symlinks
 */
int utest_icon_write_refuses_symlink(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    static char target[] = "rq_icontgt";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    const struct ofork *of;
    cnid_t id;
    uint16_t v = 0;
    uint16_t l = 0;
    int refused[2] = { 0, 0 };
    int held = -1;
    int ret = -1;

    if (vol->v_flags & AFPVOL_FOLLOWSYM) {
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

    /* the virtual fork opens first, then a symlink takes the name */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &v) != AFP_OK
            || symlink(target, path) != 0) {
        goto out;
    }

    for (int i = 0; i < 2; i++) {
        /* the second write comes while the session holds a fork of the
         * link */
        if (i == 1 && openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                               OPENFORK_DATA, OPENACC_RD, &l) != AFP_OK) {
            goto out;
        }

        refused[i] = fio_write(v, 0, 0, rq_data, 1) == AFPERR_ACCESS
                     && (of = of_find(v)) != NULL
                     && (of->of_flags & AFPFORK_VIRTUAL);
    }

    closefork(&fio.obj, v);
    v = 0;
    held = (of = of_find(l)) != NULL
           && ad_data_fileno(of->of_ad) == AD_SYMLINK;

    if (refused[0] && refused[1] && held == 1) {
        ret = 0;
    } else {
        fprintf(stderr, "virtual icon over a symlink: refused %d/%d, the "
                        "link's fork kept its descriptor %d\n", refused[0],
                refused[1], held);
        ret = 1;
    }

out:

    if (l) {
        closefork(&fio.obj, l);
    }

    if (v) {
        closefork(&fio.obj, v);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;

    /* FPDelete refuses a symlink where it cannot read the link's lock
     * state, so the link goes by unlink(2) */
    if ((id = cnid_get(vol->v_cdb, DIRDID_ROOT, name,
                       strlen(name))) != CNID_INVALID) {
        cnid_delete(vol->v_cdb, id);
    }

    unlink(adpath);
    unlink(path);
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief The session's last fork of the root Icon file deletes the file when
 *        another fork of the session cut its resource fork short and closed
 *        first
 *
 * @returns 0, 1 when a close disagrees, -1 when the setup failed
 */
int utest_emptied_icon_close_after_sibling_cut(AFPObj *obj, struct vol *vol)
{
    static char name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    uint16_t x = 0;
    uint16_t r = 0;
    uint16_t w = 0;
    int kept = -1;
    int gone = -1;
    int rc;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    /* an Icon file whose resource fork holds resources */
    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &x) != AFP_OK) {
        goto out;
    }

    rc = fio_write(x, 0, 0, rq_data, RQ_LEN);
    closefork(&fio.obj, x);
    x = 0;

    /* a reader, then a writer that cuts the fork and closes first */
    if (rc != AFP_OK || access(path, F_OK) != 0
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || setforkparams(&fio.obj, w, 0) != AFP_OK) {
        goto out;
    }

    closefork(&fio.obj, w);
    w = 0;
    kept = access(path, F_OK) == 0;
    closefork(&fio.obj, r);
    r = 0;
    gone = access(path, F_OK) != 0;

    if (kept == 1 && gone == 1) {
        ret = 0;
    } else {
        fprintf(stderr,
                "sibling's cut: kept at the writer's close %d, gone at the "
                "reader's %d\n", kept, gone);
        ret = 1;
    }

out:

    if (w) {
        closefork(&fio.obj, w);
    }

    if (r) {
        closefork(&fio.obj, r);
    }

    if (x) {
        closefork(&fio.obj, x);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(adpath);
    unlink(path);
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief A read through a hard link's second name fills no cache entry of
 *        that name from the first name's resource fork, which the session's
 *        forks of the file share
 *
 * Each name of a hard link has an AppleDouble file of its own.
 *
 * @returns 0, 1 when the second name served the first name's resource fork,
 *          -1 when the setup failed, TEST_SKIP where the resource fork
 *          belongs to the file
 */
int utest_rfork_cache_fills_by_own_link(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rfilla";
    static char other[] = "rq_rfillb";
    char from[MAXPATHLEN + 1];
    char to[MAXPATHLEN + 1];
    char data[RQ_LEN];
    char out[RQ_LEN];
    uint16_t a = 0;
    uint16_t b = 0;
    int linked = 0;
    int rc;
    int ret = -1;
#ifdef HAVE_EAFD

    /* a resource fork in an EA is the file's under every name */
    if (vol->v_adouble == AD_VERSION_EA) {
        return TEST_SKIP;
    }

#endif

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(from, sizeof(from), "%s/%s", vol->v_path, name);
    snprintf(to, sizeof(to), "%s/%s", vol->v_path, other);

    for (int i = 0; i < RQ_LEN; i++) {
        data[i] = (char)~rq_data[i];
    }

    /* each name's resource fork holds bytes of its own, of one length */
    if (fio_rfork(vol, name, &a) != 0) {
        goto out;
    }

    closefork(&fio.obj, a);
    a = 0;

    if (link(from, to) != 0) {
        goto out;
    }

    linked = 1;

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, other, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &b) != AFP_OK) {
        goto out;
    }

    rc = fio_write(b, 0, 0, data, RQ_LEN);
    closefork(&fio.obj, b);
    b = 0;

    /* the second name's reader shares the first name's adouble */
    if (rc != AFP_OK
            || getfiledirparms(&fio.obj, vol->v_vid, DIRDID_ROOT,
                               other) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &a) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, other,
                        OPENFORK_RSCS, OPENACC_RD, &b) != AFP_OK
            || fio_read(b, 0, RQ_LEN, RQ_LEN, out) != AFP_OK) {
        goto out;
    }

    closefork(&fio.obj, b);
    b = 0;
    closefork(&fio.obj, a);
    a = 0;

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, other, OPENFORK_RSCS,
                 OPENACC_RD, &b) != AFP_OK) {
        goto out;
    }

    if (fio_read(b, 0, RQ_LEN, RQ_LEN, out) == AFP_OK
            && memcmp(out, data, RQ_LEN) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "rfork hard link: the second name served the first "
                        "name's resource fork\n");
        ret = 1;
    }

out:

    if (b) {
        closefork(&fio.obj, b);
    }

    if (a) {
        closefork(&fio.obj, a);
    }

    if (linked) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, other);
    }

    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief FPReadExt, through the sendfile arm, of a hard link's resource fork
 *        opened read-only under the name with no AppleDouble file, while the
 *        session holds the other name's data fork
 *
 * @returns the read's reply, or -1 when the setup failed
 */
static int rfork_read_without_sidecar(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rnoside";
    static char other[] = "rq_rnosideb";
    char from[MAXPATHLEN + 1];
    char to[MAXPATHLEN + 1];
    size_t rbuflen = DSI_DATASIZ;
    uint16_t a = 0;
    uint16_t b = 0;
    int linked = 0;
    int rc = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(from, sizeof(from), "%s/%s", vol->v_path, name);
    snprintf(to, sizeof(to), "%s/%s", vol->v_path, other);

    /* the first name's AppleDouble file holds a resource fork, the second
     * name has none */
    if (fio_rfork(vol, name, &a) != 0) {
        goto out;
    }

    closefork(&fio.obj, a);
    a = 0;

    if (link(from, to) != 0) {
        goto out;
    }

    linked = 1;

    /* the second name's read open takes no resource fork, and the first
     * name's open shares its adouble and opens or measures its own
     * AppleDouble file in it */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, other, OPENFORK_RSCS,
                 OPENACC_RD, &b) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                        OPENACC_RD, &a) != AFP_OK
            || fio_send(0, b, 0, 64, NULL) != 0) {
        goto out;
    }

    fio.obj.options.flags &= ~OPTION_NOSENDFILE;
    rc = afp_read_ext(&fio.obj, (char *)fio.dsi.commands, fio.dsi.cmdlen,
                      (char *)fio.dsi.data, &rbuflen);
    fio.obj.options.flags |= OPTION_NOSENDFILE;
out:

    if (b) {
        closefork(&fio.obj, b);
    }

    if (a) {
        closefork(&fio.obj, a);
    }

    if (linked) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, other);
    }

    fio_done(vol, 0, name);
    return rc;
}

/*!
 * @brief A read-only resource fork open of a hard link's name that has no
 *        AppleDouble file reads EOF, whatever the other name's metadata open
 *        put into the adouble the session's forks share: a length on the ea
 *        layout, the descriptor itself on adouble:v2
 *
 * @returns 0, 1 when a read did not reply AFPERR_EOF, -1 when the setup
 *          failed
 */
int utest_rfork_without_sidecar_reads_eof(AFPObj *obj, struct vol *vol)
{
    const int saved_adouble = vol->v_adouble;
    char adpath[MAXPATHLEN + 1];
    int ea = AFPERR_EOF;
    int v2;
#ifndef HAVE_EAFD

    /* the ea layout gives each name a ._ file of its own */
    if (saved_adouble == AD_VERSION_EA) {
        ea = rfork_read_without_sidecar(obj, vol);
    }

#endif
    vol->v_adouble = AD_VERSION2;
    initvol_vfs(vol);
    v2 = rfork_read_without_sidecar(obj, vol);
    vol->v_adouble = saved_adouble;
    initvol_vfs(vol);
    snprintf(adpath, sizeof(adpath), "%s/.AppleDouble", vol->v_path);
    (void)rmdir(adpath);

    if (ea == -1 || v2 == -1) {
        return -1;
    }

    if (ea == AFPERR_EOF && v2 == AFPERR_EOF) {
        return 0;
    }

    fprintf(stderr, "rfork without an AppleDouble file: the reads replied "
                    "%d on adouble:ea, %d on adouble:v2\n", ea, v2);
    return 1;
}

/*!
 * @brief The first change of a fork whose name another session's save gave
 *        to a new file sends no refresh hint for the new file, where neither
 *        the dircache nor the fork's metadata names the fork's own file
 *
 * @returns 0, 1 when the first change hinted the new file, -1 when the setup
 *          failed
 */
int utest_first_change_hint_spares_new_file(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_fchint";
    static char aside[] = "rq_fchint.old";
    struct ofork *of;
    struct dir *entry;
    struct stat st;
    int fds[2] = { -1, -1 };
    cnid_t id = CNID_INVALID;
    cnid_t newid = CNID_INVALID;
    uint16_t w = 0;
    int hinted;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    if (fio_dfork(vol, name, &w) != 0 || (of = of_find(w)) == NULL
            || fstat(ad_data_fileno(of->of_ad), &st) != 0
            || (id = cnid_get(vol->v_cdb, DIRDID_ROOT, name,
                              strlen(name))) == CNID_INVALID) {
        goto out;
    }

    /* the session holds no entry for the name, and the fork's metadata
     * records another file's inode */
    if ((entry = dircache_search_by_name(vol, vol->v_root, name,
                                         strlen(name))) != NULL) {
        (void)dir_remove(vol, entry, 0);
    }

    (void)ad_setid(of->of_ad, st.st_dev, st.st_ino + 1, id, DIRDID_ROOT,
                   vol->v_stamp);

    /* another session's save gives the name to a new file */
    if (fio_replace(vol, name, aside, rq_data) != 0
            || (newid = cnid_get(vol->v_cdb, DIRDID_ROOT, name,
                                 strlen(name))) == CNID_INVALID
            || socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0
            || setnonblock(fds[0], 1) != 0) {
        goto out;
    }

    /* the session's IPC socket: what it sends its parent arrives at fds[0] */
    fio.obj.ipc_fd = fds[1];

    if (fio_write(w, 0, 0, rq_data, 64) != AFP_OK) {
        goto out;
    }

    hinted = hint_take(fds[0], vol->v_vid, newid, CACHE_HINT_REFRESH);

    if (hinted == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "first change: a refresh hint for the new file %d\n",
                hinted);
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

    delete (&fio.obj, vol->v_vid, DIRDID_ROOT, aside);
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief The close of a fork written under a hard link's second name stores
 *        no length of the first name's resource fork, which the session's
 *        forks of the file share, in the second name's cache entry
 *
 * Each name of a hard link has an AppleDouble file of its own.
 *
 * @returns 0, 1 when the second name's entry took the first name's length,
 *          -1 when the setup failed, TEST_SKIP where the resource fork
 *          belongs to the file
 */
int utest_rfork_close_refreshes_by_own_link(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rclosa";
    static char other[] = "rq_rclosb";
    char from[MAXPATHLEN + 1];
    char to[MAXPATHLEN + 1];
    const struct dir *entry;
    uint16_t a = 0;
    uint16_t b = 0;
    int linked = 0;
    int rc;
    int ret = -1;
#ifdef HAVE_EAFD

    /* a resource fork in an EA is the file's under every name */
    if (vol->v_adouble == AD_VERSION_EA) {
        return TEST_SKIP;
    }

#endif

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(from, sizeof(from), "%s/%s", vol->v_path, name);
    snprintf(to, sizeof(to), "%s/%s", vol->v_path, other);

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || link(from, to) != 0) {
        goto out;
    }

    linked = 1;

    /* the second name has an entry and no resource fork of its own, and
     * its writer shares the adouble of the first name's */
    if (getfiledirparms(&fio.obj, vol->v_vid, DIRDID_ROOT, other) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &a) != AFP_OK
            || fio_write(a, 0, 0, rq_data, RQ_LEN) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, other,
                        OPENFORK_RSCS, OPENACC_RD | OPENACC_WR, &b) != AFP_OK) {
        goto out;
    }

    rc = fio_write(b, 0, 0, rq_data, 64);
    closefork(&fio.obj, b);
    b = 0;

    if (rc != AFP_OK
            || (entry = dircache_search_by_name(vol, vol->v_root, other,
                                                strlen(other))) == NULL) {
        goto out;
    }

    if (entry->dcache_rlen != RQ_LEN) {
        ret = 0;
    } else {
        fprintf(stderr, "rfork hard link: the second name's entry took the "
                        "first name's length\n");
        ret = 1;
    }

out:

    if (b) {
        closefork(&fio.obj, b);
    }

    if (a) {
        closefork(&fio.obj, a);
    }

    if (linked) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, other);
    }

    fio_done(vol, 0, name);
    return ret;
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
 * @brief A rename of one name of a hard link leaves the session's forks of
 *        the file under the other name, so a write through them drops that
 *        name's cached resource fork
 *
 * @returns 0, 1 when the forks took the new name or the old bytes were
 *          served, -1 when the setup failed
 */
int utest_rename_keeps_link_forks_name(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rnlka";
    static char other[] = "rq_rnlkb";
    static char renamed[] = "rq_rnlkc";
    const char *second = other;
    char from[MAXPATHLEN + 1];
    char to[MAXPATHLEN + 1];
    char data[RQ_LEN];
    char out[RQ_LEN];
    const struct dir *entry;
    const struct ofork *of;
    uint16_t r = 0;
    uint16_t w = 0;
    uint16_t r2 = 0;
    int linked = 0;
    int kept = -1;
    int fresh = -1;
    int rc;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(from, sizeof(from), "%s/%s", vol->v_path, name);
    snprintf(to, sizeof(to), "%s/%s", vol->v_path, other);

    for (int i = 0; i < RQ_LEN; i++) {
        data[i] = (char)~rq_data[i];
    }

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || link(from, to) != 0) {
        goto out;
    }

    linked = 1;

    /* the close of a written fork records its length in the entry */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &w) != AFP_OK) {
        goto out;
    }

    rc = fio_write(w, 0, 0, rq_data, RQ_LEN);
    closefork(&fio.obj, w);
    w = 0;

    /* a read fills the entry, and a writer joins the reader's adouble */
    if (rc != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                        OPENFORK_RSCS, OPENACC_RD, &r) != AFP_OK
            || fio_read(r, 0, RQ_LEN, RQ_LEN, out) != AFP_OK
            || (entry = dircache_search_by_name(vol, vol->v_root, name,
                                                strlen(name))) == NULL
            || entry->dcache_rfork_buf == NULL
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                        OPENFORK_RSCS, OPENACC_RD | OPENACC_WR, &w) != AFP_OK) {
        goto out;
    }

    /* the other name, which no fork is open under, moves */
    if (renameobj(&fio.obj, vol->v_vid, DIRDID_ROOT, other,
                  renamed) != AFP_OK) {
        goto out;
    }

    second = renamed;
    kept = (of = of_find(w)) != NULL && strcmp(of->of_uname, name) == 0;

    if (fio_write(w, 0, 0, data, RQ_LEN) != AFP_OK) {
        goto out;
    }

    fresh = openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                     OPENACC_RD, &r2) == AFP_OK
            && fio_read(r2, 0, RQ_LEN, RQ_LEN, out) == AFP_OK
            && memcmp(out, data, RQ_LEN) == 0;

    if (kept == 1 && fresh == 1) {
        ret = 0;
    } else {
        fprintf(stderr,
                "rename of a hard link's other name: the forks kept their "
                "name %d, a reader got the new bytes %d\n", kept, fresh);
        ret = 1;
    }

out:

    if (r2) {
        closefork(&fio.obj, r2);
    }

    if (w) {
        closefork(&fio.obj, w);
    }

    if (r) {
        closefork(&fio.obj, r);
    }

    if (linked) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, second);
    }

    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief FPRename of a file whose name another session changed while the
 *        session held a fork of it moves the fork to the new name
 *
 * @returns 0, 1 when the fork kept the dead name, -1 when the setup failed
 */
int utest_rename_takes_stale_fork_name(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_stalea";
    static char other[] = "rq_staleb";
    static char renamed[] = "rq_stalec";
    char from[MAXPATHLEN + 1];
    char to[MAXPATHLEN + 1];
    const struct ofork *of;
    uint16_t w = 0;
    int moved = -1;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(from, sizeof(from), "%s/%s", vol->v_path, name);
    snprintf(to, sizeof(to), "%s/%s", vol->v_path, other);

    /* another session's rename, which the session's fork does not see */
    if (fio_dfork(vol, name, &w) != 0 || rename(from, to) != 0
            || vol->vfs->vfs_renamefile(vol, -1, from, to) != 0) {
        goto out;
    }

    if (renameobj(&fio.obj, vol->v_vid, DIRDID_ROOT, other,
                  renamed) == AFP_OK && (of = of_find(w)) != NULL) {
        moved = strcmp(of->of_uname, renamed) == 0
                && strcmp(of_name(of), renamed) == 0;
    }

    if (moved == 1) {
        ret = 0;
    } else if (moved == 0) {
        fprintf(stderr, "stale fork name: the rename left the fork on its "
                        "dead name\n");
        ret = 1;
    }

out:

    if (w) {
        closefork(&fio.obj, w);
    }

    delete (&fio.obj, vol->v_vid, DIRDID_ROOT, name);
    delete (&fio.obj, vol->v_vid, DIRDID_ROOT, other);
    fio_done(vol, 0, renamed);
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
 * @brief An FPDelete that another session's deny mode refuses leaves the
 *        session's open fork of a file without metadata its open-mode lock
 *
 * @returns 0, 1 when the lock was dropped, -1 when the setup failed
 */
int utest_refused_delete_keeps_open_lock(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_dlk";
    char path[MAXPATHLEN + 1];
    struct peer denier;
    uint16_t r = 0;
    int fd;
    int rc;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    /* a file made outside afpd has no metadata */
    if ((fd = open(path, O_CREAT | O_WRONLY, 0600)) < 0) {
        goto out;
    }

    close(fd);

    /* the session's read open shows on its band, and another session
     * denies writers */
    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD, &r) != AFP_OK
            || peer_try_lock(path, F_WRLCK, AD_FILELOCK_OPEN_RD, 1) != 0
            || peer_hold_lock(&denier, path, F_RDLCK, AD_FILELOCK_DENY_WR,
                              1) != 0) {
        goto out;
    }

    rc = delete (&fio.obj, vol->v_vid, DIRDID_ROOT, name);
    peer_release(&denier);

    if (rc != AFPERR_BUSY || access(path, F_OK) != 0) {
        goto out;
    }

    if (peer_try_lock(path, F_WRLCK, AD_FILELOCK_OPEN_RD, 1) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "refused delete dropped the open's band lock\n");
        ret = 1;
    }

out:
    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief An FPDelete that another session's deny mode refuses leaves the
 *        length of the session's open resource fork of the file, which the
 *        delete's metadata read measures by the name
 *
 * @returns 0, 1 when the fork's length changed, -1 when the setup failed,
 *          TEST_SKIP where no EA holds the metadata or no ._ file the
 *          resource fork
 */
int utest_refused_delete_keeps_rfork_length(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "rq_dlen";
    static char other[] = "rq_dlen2";
    char path[MAXPATHLEN + 1];
    char otherpath[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    char ea[AD_DATASZ_EA];
    const struct ofork *of;
    struct peer denier;
    uint16_t r = 0;
    int held = 0;
    int fd;
    int rc;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(otherpath, sizeof(otherpath), "%s/%s", vol->v_path, other);
    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    /* a file made outside afpd has no metadata, and the session's read open
     * of its resource fork finds no length */
    if ((fd = open(path, O_CREAT | O_WRONLY, 0600)) < 0) {
        goto out;
    }

    close(fd);

    if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK
            || (of = of_find(r)) == NULL || of->of_ad->ad_rlen != 0
            || createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, other) != AFP_OK
            || sys_lgetxattr(otherpath, AD_EA_META, ea,
                             sizeof(ea)) != AD_DATASZ_EA) {
        goto out;
    }

    /* another session gives the file metadata and a resource fork, and
     * denies writers */
    if (sys_lsetxattr(path, AD_EA_META, ea, sizeof(ea), 0) != 0
            || (fd = open(adpath, O_CREAT | O_WRONLY, 0600)) < 0) {
        goto out;
    }

    rc = ftruncate(fd, ADEDOFF_RFORK_OSX + RQ_LEN);
    close(fd);

    if (rc != 0 || peer_hold_lock(&denier, path, F_RDLCK, AD_FILELOCK_DENY_WR,
                                  1) != 0) {
        goto out;
    }

    held = 1;

    if (delete (&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFPERR_BUSY) {
        goto out;
    }

    if ((of = of_find(r)) != NULL && of->of_ad->ad_rlen == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "refused delete: the open resource fork took the "
                        "name's length\n");
        ret = 1;
    }

out:

    if (held) {
        peer_release(&denier);
    }

    unlink(adpath);
    delete (&fio.obj, vol->v_vid, DIRDID_ROOT, other);
    fio_done(vol, r, name);
    return ret;
#endif
}

/*!
 * @brief On a volume that does not follow symlinks, a rename of a symlink
 *        keeps the file IDs of the link and of the file it points to, also
 *        when the rename of a link the cache holds is refused; a dangling
 *        symlink renames, and a rename onto its name replies AFPERR_EXIST
 *
 * @returns 0, 1 when a rename disagrees, -1 when the setup failed, TEST_SKIP
 *          on a volume that follows symlinks
 */
int utest_rename_symlink_keeps_cnids(AFPObj *obj, struct vol *vol)
{
    static char target[] = "rq_rnst";
    static char file[] = "rq_rnsf";
    static char alias[] = "rq_rnsl";
    static char alias2[] = "rq_rnsl2";
    static char dangling[] = "rq_rnsd";
    static char dangling2[] = "rq_rnsd2";
    char *const links[] = { alias, alias2, dangling, dangling2 };
    char path[MAXPATHLEN + 1];
    struct stat st;
    cnid_t was_t = CNID_INVALID;
    cnid_t was_l = CNID_INVALID;
    int moved = -1;
    int refused = -1;
    int dang = -1;
    int onto = -1;
    int ret = -1;

    if (vol->v_flags & AFPVOL_FOLLOWSYM) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, alias);

    /* a link with an ID of its own and no dircache entry */
    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, target) != AFP_OK
            || createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, file) != AFP_OK
            || symlink(target, path) != 0 || lstat(path, &st) != 0
            || (was_l = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, alias,
                                 strlen(alias), 0)) == CNID_INVALID
            || (was_t = cnid_get(vol->v_cdb, DIRDID_ROOT, target,
                                 strlen(target))) == CNID_INVALID) {
        goto out;
    }

    moved = renameobj(&fio.obj, vol->v_vid, DIRDID_ROOT, alias,
                      alias2) == AFP_OK
            && cnid_get(vol->v_cdb, DIRDID_ROOT, target,
                        strlen(target)) == was_t
            && cnid_get(vol->v_cdb, DIRDID_ROOT, alias2,
                        strlen(alias2)) == was_l;
    /* the link's entry, then its rename onto the file's name */
    refused = getfiledirparms(&fio.obj, vol->v_vid, DIRDID_ROOT,
                              alias2) == AFP_OK
              && renameobj(&fio.obj, vol->v_vid, DIRDID_ROOT, alias2,
                           file) == AFPERR_EXIST
              && cnid_get(vol->v_cdb, DIRDID_ROOT, target,
                          strlen(target)) == was_t
              && cnid_get(vol->v_cdb, DIRDID_ROOT, alias2,
                          strlen(alias2)) == was_l;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, dangling);

    if (symlink("rq_nowhere", path) != 0
            || getfiledirparms(&fio.obj, vol->v_vid, DIRDID_ROOT,
                               dangling) != AFP_OK) {
        goto out;
    }

    dang = renameobj(&fio.obj, vol->v_vid, DIRDID_ROOT, dangling,
                     dangling2) == AFP_OK;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, dangling2);
    onto = renameobj(&fio.obj, vol->v_vid, DIRDID_ROOT, file,
                     dangling2) == AFPERR_EXIST
           && lstat(path, &st) == 0 && S_ISLNK(st.st_mode);

    if (moved == 1 && refused == 1 && dang == 1 && onto == 1) {
        ret = 0;
    } else {
        fprintf(stderr,
                "symlink rename: IDs kept %d, a cached link's refused rename "
                "kept them %d, a dangling link renamed %d, its name taken "
                "%d\n", moved, refused, dang, onto);
        ret = 1;
    }

out:

    /* FPDelete refuses a symlink where it cannot read the link's lock
     * state, so the links go by unlink(2) */
    for (size_t i = 0; i < sizeof(links) / sizeof(links[0]); i++) {
        cnid_t id = cnid_get(vol->v_cdb, DIRDID_ROOT, links[i],
                             strlen(links[i]));

        if (id != CNID_INVALID) {
            cnid_delete(vol->v_cdb, id);
        }

        snprintf(path, sizeof(path), "%s/%s", vol->v_path, links[i]);
        unlink(path);
    }

    delete (&fio.obj, vol->v_vid, DIRDID_ROOT, file);
    fio_done(vol, 0, target);
    return ret;
}

/*!
 * @brief The close of the root Icon file's emptied resource fork keeps a file
 *        that another process renames onto the name while the close runs,
 *        and keeps no entry of the file that went
 *
 * @returns 0, 1 when the close deleted the file that took the name or its
 *          entry kept the old file, -1 when the setup failed, TEST_SKIP
 *          without the fault injector
 */
int utest_emptied_icon_close_spares_swapped_file(AFPObj *obj,
                                                 struct vol *vol)
{
    static char name[] = "Icon\r";
    static char swap[] = "rq_iconswap";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    const int saved_freq = obj->options.dircache_validation_freq;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    char swappath[MAXPATHLEN + 1];
    const struct dir *entry;
    struct stat st;
    ino_t swapped;
    uint16_t w = 0;
    int fired;
    int kept;
    int fresh;
    int fd;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
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
    snprintf(swappath, sizeof(swappath), "%s/%s", vol->v_path, swap);

    /* the fork's write leaves a resource map without resources, and the
     * session has an entry for the name */
    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || fio_write(w, 0, 0, (const char *)icon, sizeof(icon)) != AFP_OK
            || getfiledirparms(&fio.obj, vol->v_vid, DIRDID_ROOT,
                               name) != AFP_OK
            || dircache_search_by_name(vol, vol->v_root, name,
                                       strlen(name)) == NULL
            || (fd = open(swappath, O_CREAT | O_WRONLY, 0600)) < 0) {
        goto out;
    }

    close(fd);

    if (stat(swappath, &st) != 0) {
        goto out;
    }

    swapped = st.st_ino;
    /* the close's first lock call, the release of the fork's open-mode
     * locks, comes after its check of the name and before the removal */
    fault_inject_reset();
    fi.fcntl_watch = 1;
    fi.rename_from = swappath;
    fi.rename_to = path;
    fi.rename_armed = 1;
    closefork(&fio.obj, w);
    w = 0;
    /* positive control: the rename landed, and the close went on to probe
     * the file's locks for the removal */
    fired = !fi.rename_armed && access(swappath, F_OK) != 0
            && fi.getlk_calls > 0;
    fault_inject_reset();

    if (!fired) {
        goto out;
    }

    /* the entry, read where no periodic validation refreshes it, holds no
     * stat of the file that went */
    obj->options.dircache_validation_freq = INT_MAX;
    entry = dircache_search_by_name(vol, vol->v_root, name, strlen(name));
    obj->options.dircache_validation_freq = saved_freq;
    fresh = entry == NULL || entry->dcache_ino == swapped;
    kept = stat(path, &st) == 0 && st.st_ino == swapped;

    if (kept && fresh) {
        ret = 0;
    } else {
        fprintf(stderr, "swapped icon: kept the file that took the name %d, "
                        "no entry of the one that went %d\n", kept, fresh);
        ret = 1;
    }

out:
    fault_inject_reset();

    if (w) {
        closefork(&fio.obj, w);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    unlink(swappath);
    unlink(adpath);
    unlink(path);
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief FPRename of a symlink whose data fork the session holds open keeps
 *        the fork reading the link, on a volume that does not follow
 *        symlinks
 *
 * @returns 0, 1 when the rename spent the fork's emulated descriptor, -1 when
 *          the setup failed, TEST_SKIP on a volume that follows symlinks
 */
int utest_rename_keeps_symlink_fork(AFPObj *obj, struct vol *vol)
{
    static char target[] = "rq_rnhst";
    static char alias[] = "rq_rnhsl";
    static char alias2[] = "rq_rnhsl2";
    char *const links[] = { alias, alias2 };
    char path[MAXPATHLEN + 1];
    char out[sizeof(target) - 1];
    const struct ofork *of;
    uint16_t r = 0;
    int ret = -1;

    if (vol->v_flags & AFPVOL_FOLLOWSYM) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, alias);

    if (symlink(target, path) != 0
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, alias,
                        OPENFORK_DATA, OPENACC_RD, &r) != AFP_OK
            || renameobj(&fio.obj, vol->v_vid, DIRDID_ROOT, alias,
                         alias2) != AFP_OK) {
        goto out;
    }

    /* the fork reads the link's target name through its emulated
     * descriptor */
    if ((of = of_find(r)) != NULL
            && ad_data_fileno(of->of_ad) == AD_SYMLINK
            && fio_read(r, 0, sizeof(out), sizeof(out), out) == AFP_OK
            && memcmp(out, target, sizeof(out)) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "renamed symlink: the open fork lost the link\n");
        ret = 1;
    }

out:

    if (r) {
        closefork(&fio.obj, r);
    }

    /* FPDelete refuses a symlink where it cannot read the link's lock
     * state, so the links go by unlink(2) */
    for (size_t i = 0; i < sizeof(links) / sizeof(links[0]); i++) {
        cnid_t id = cnid_get(vol->v_cdb, DIRDID_ROOT, links[i],
                             strlen(links[i]));

        if (id != CNID_INVALID) {
            cnid_delete(vol->v_cdb, id);
        }

        snprintf(path, sizeof(path), "%s/%s", vol->v_path, links[i]);
        unlink(path);
    }

    fio_done(vol, 0, alias2);
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
 * @brief A rename onto the virtual Icon file's name in the volume root fails
 *        with AFPERR_EXIST, as onto a real file
 *
 * @returns 0, 1 when the rename is not refused, -1 when the setup failed
 */
int utest_rename_onto_virtual_icon_refused(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_toicon";
    static char icon_name[] = "Icon\r";
    /* a resource fork header with no resources */
    static unsigned char icon[16];
    unsigned char *saved = vol->v_icon_rfork;
    size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    int rc = -1;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, icon_name);

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) == AFP_OK) {
        rc = renameobj(&fio.obj, vol->v_vid, DIRDID_ROOT, name, icon_name);

        if (rc == AFPERR_EXIST && access(path, F_OK) != 0) {
            ret = 0;
        } else {
            fprintf(stderr, "rename onto the virtual icon: %d\n", rc);
            ret = 1;
        }
    }

    if (rc == AFP_OK) {
        delete (&fio.obj, vol->v_vid, DIRDID_ROOT, icon_name);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief An open of the virtual Icon file for writing replies AFPERR_ACCESS
 *        in a volume root the user cannot write, and an open for reading
 *        succeeds there and in a root the user cannot read
 *
 * @returns 0, 1 when a reply is another code, -1 when the setup failed
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
    int rd[2] = { -1, -1 };
    int wr;
    int ret;

    if (stat(vol->v_path, &st) != 0 || fio_open(obj) != 0) {
        return -1;
    }

    /* the AFP rights of the root's owner, a user other than root, follow
     * its owner bits */
    fio.obj.uid = st.st_uid;
    fio.obj.euid = st.st_uid ? st.st_uid : 1;

    if (chmod(vol->v_path, st.st_mode & 0555) != 0) {
        fio_done(vol, 0, name);
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    wr = openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                  OPENACC_RD | OPENACC_WR, &w);
    rd[0] = openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                     OPENACC_RD, &r);

    if (wr == AFP_OK) {
        closefork(&fio.obj, w);
    }

    if (rd[0] == AFP_OK) {
        closefork(&fio.obj, r);
    }

    if (chmod(vol->v_path, st.st_mode & 0333) == 0) {
        rd[1] = openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name,
                         OPENFORK_RSCS, OPENACC_RD, &r);

        if (rd[1] == AFP_OK) {
            closefork(&fio.obj, r);
        }
    }

    chmod(vol->v_path, st.st_mode & 07777);
    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;

    if (wr == AFPERR_ACCESS && rd[0] == AFP_OK && rd[1] == AFP_OK) {
        ret = 0;
    } else {
        fprintf(stderr,
                "virtual icon: write open in a root not writable %d, read "
                "open %d, read open in a root not readable %d\n", wr, rd[0],
                rd[1]);
        ret = 1;
    }

    fio_done(vol, 0, name);
    return ret;
}

/*!
 * @brief The close of a written resource fork survives a malformed metadata
 *        EA that a writer outside afpd left on the file, and logs the file's
 *        name
 *
 * @returns 0, 1 when the close crashed or its log names no file, -1 when the
 *          setup failed, TEST_SKIP where no EA holds the metadata
 */
int utest_rfork_close_survives_bad_metadata(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_badmeta";
    char path[MAXPATHLEN + 1];
    char logpath[MAXPATHLEN + 1];
    uint16_t w = 0;
    int fds[2] = { -1, -1 };
    char done = 0;
    pid_t pid;
    int named = -1;
    int fd;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    snprintf(logpath, sizeof(logpath), "%s/netatalk-rqlog-XXXXXX", tmpdir());

    if ((fd = mkstemp(logpath)) < 0) {
        return -1;
    }

    close(fd);

    if (fio_open(obj) != 0) {
        unlink(logpath);
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || fio_write(w, 0, 0, rq_data, 1) != AFP_OK) {
        goto out;
    }

    if (sys_lsetxattr(path, AD_EA_META, "x", 1, 0) != 0) {
        ret = TEST_SKIP;
        goto out;
    }

    if (pipe(fds) != 0) {
        goto out;
    }

    /* the close runs in a child, whose crash spares the run, and which says
     * so down the pipe once the close returns, as valgrind rewrites the exit
     * status of a child that holds the harness's memory; its log level has
     * the header read's error lines formatted */
    fflush(NULL);

    if ((pid = fork()) == 0) {
        setuplog("default:note", logpath, true);
        (void)closefork(&fio.obj, w);
        _exit(write(fds[1], "c", 1) == 1 ? 0 : 1);
    }

    close(fds[1]);

    /* a crash, or a sanitizer's exit after one, sends nothing */
    if (pid > 0) {
        const int returned = read(fds[0], &done, 1) == 1;
        (void)waitpid(pid, NULL, 0);
        named = conf_log_contains(logpath, name);

        if (returned && named == 1) {
            ret = 0;
        } else {
            fprintf(stderr, "bad metadata EA: the close %s, its log names "
                            "the file %d\n", returned ? "returned" : "died",
                    named);
            ret = 1;
        }
    }

    (void)sys_lremovexattr(path, AD_EA_META);
out:

    if (fds[0] >= 0) {
        close(fds[0]);
    }

    unlink(logpath);
    fio_done(vol, w, name);
    return ret;
}

/*!
 * @brief The close of a written resource fork keeps the file's metadata when
 *        a writer outside afpd left a malformed metadata EA on it
 *
 * @returns 0, 1 when the close stored the malformed EA's bytes, -1 when the
 *          setup failed, TEST_SKIP where no EA holds the metadata
 */
int utest_rfork_close_keeps_metadata_past_bad_ea(AFPObj *obj,
                                                 struct vol *vol)
{
    static char name[] = "rq_badmeta2";
    /* the entries' bytes follow the header and its table */
    const size_t table = AD_HEADER_LEN + ADEID_NUM_EA * AD_ENTRY_LEN;
    const uint16_t entries = htons(ADEID_NUM_EA + 1);
    char path[MAXPATHLEN + 1];
    char ea[AD_DATASZ_EA];
    const unsigned char *finfo;
    struct adouble ad;
    uint16_t w = 0;
    int kept[2] = { 0, 0 };
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK) {
        goto out;
    }

    for (int i = 0; i < 2; i++) {
        /* a written fork flushes its header at the close */
        if (openfork(&fio.obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                     OPENACC_RD | OPENACC_WR, &w) != AFP_OK
                || fio_write(w, 0, 0, rq_data, 1) != AFP_OK
                || sys_lgetxattr(path, AD_EA_META, ea,
                                 sizeof(ea)) != AD_DATASZ_EA) {
            goto out;
        }

        /* an entry count the layout never writes, then a first entry with
         * no id, over entries of FIO_FILL */
        if (i == 0) {
            memcpy(ea + AD_HEADER_LEN - ADEDLEN_NENTRIES, &entries,
                   sizeof(entries));
        } else {
            memset(ea + AD_HEADER_LEN, 0, sizeof(uint32_t));
        }

        memset(ea + table, FIO_FILL, sizeof(ea) - table);

        if (sys_lsetxattr(path, AD_EA_META, ea, sizeof(ea), 0) != 0) {
            goto out;
        }

        closefork(&fio.obj, w);
        w = 0;
        ad_init(&ad, vol);

        if (ad_metadata(path, 0, &ad) == 0) {
            finfo = ad_entry(&ad, ADEID_FINDERI);
            kept[i] = finfo != NULL && finfo[0] != FIO_FILL;
            ad_close(&ad, ADFLAGS_HF);
        }
    }

    if (kept[0] && kept[1]) {
        ret = 0;
    } else {
        fprintf(stderr, "bad metadata EA: kept past a bad entry count %d, a "
                        "bad entry %d\n", kept[0], kept[1]);
        ret = 1;
    }

out:
    fio_done(vol, w, name);
    return ret;
}

/*!
 * @brief A user who may only read a file is refused an open that another
 *        session's deny mode excludes
 *
 * @returns 0, 1 when the open was granted, -1 when the setup failed,
 *          TEST_SKIP without the open() fault injector
 */
int utest_readonly_open_meets_deny_mode(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_rodeny";
    char path[MAXPATHLEN + 1];
    struct peer peer;
    uint16_t r = 0;
    int held = 0;
    int rc;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    if (createfile(&fio.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK) {
        goto out;
    }

    /* a user who cannot write the file gets a read-only data descriptor */
    if ((rc = fio_open_readonly(vol, name, OPENFORK_DATA, &r)) != AFP_OK) {
        ret = rc == TEST_SKIP ? TEST_SKIP : ret;
        goto out;
    }

    closefork(&fio.obj, r);
    r = 0;

    /* another session's open denying readers, taken while the injector,
     * which the peer's process inherits, is disarmed */
    if (peer_hold_lock(&peer, path, F_RDLCK, AD_FILELOCK_DENY_RD, 1) != 0) {
        goto out;
    }

    held = 1;
    rc = fio_open_readonly(vol, name, OPENFORK_DATA, &r);

    if (rc == AFPERR_DENYCONF) {
        ret = 0;
    } else {
        fprintf(stderr, "read-only open under a deny-read: %d\n", rc);
        ret = 1;
    }

out:
    fault_inject_reset();

    if (held) {
        peer_release(&peer);
    }

    fio_done(vol, r, name);
    return ret;
}

/*!
 * @brief A delete whose unlink the file's folder refuses keeps the file's
 *        AppleDouble file, which an adouble:v2 folder keeps in a folder of its
 *        own that the user may write
 *
 * @returns 0, 1 when the refused delete removed the AppleDouble file, -1 when
 *          the setup failed
 */
int utest_refused_delete_keeps_appledouble(AFPObj *obj, struct vol *vol)
{
    static char sub[] = "rq_keepad";
    static char name[] = "rq_file";
    const int saved_adouble = vol->v_adouble;
    const uid_t euid = geteuid();
    /* root unlinks anywhere, so as root the delete runs as another user, who
     * owns the folders */
    const uid_t owner = euid == 0 ? 65534 : euid;
    char dir[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    char adname[MAXPATHLEN + 1];
    struct adouble ad;
    struct stat st;
    int dfd;
    int adfd = -1;
    int made = 0;
    int user = 0;
    int rc = -1;
    int kept = -1;
    int ret = -1;
    (void)obj;
    snprintf(dir, sizeof(dir), "%s/%s", vol->v_path, sub);
    snprintf(path, sizeof(path), "%s/%s", dir, name);

    if (mkdir(dir, 0700) != 0) {
        return -1;
    }

    if ((dfd = open(dir, O_RDONLY | O_DIRECTORY)) < 0) {
        rmdir(dir);
        return -1;
    }

    vol->v_adouble = AD_VERSION2;
    initvol_vfs(vol);
    snprintf(adname, sizeof(adname), "%s", vol->ad_path(name, 0));
    ad_init(&ad, vol);

    if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RDWR
                | ADFLAGS_CREATE, 0666) != 0) {
        goto out;
    }

    made = ad_flush(&ad) == 0;
    ad_close(&ad, ADFLAGS_DF | ADFLAGS_HF);

    /* the folder refuses the file's unlink, its .AppleDouble folder that of
     * the AppleDouble file does not; the delete goes through the folder's
     * descriptor, so its user needs no way through the volume root */
    if (!made || fstatat(dfd, adname, &st, AT_SYMLINK_NOFOLLOW) != 0
            || (adfd = openat(dfd, ".AppleDouble", O_RDONLY | O_DIRECTORY)) < 0
            || (euid == 0 && (fchown(adfd, owner, getegid()) != 0
                              || fchown(dfd, owner, getegid()) != 0))
            || fchmod(adfd, 0700) != 0 || fchmod(dfd, 0500) != 0
            || (euid == 0 && seteuid(owner) != 0)) {
        goto out;
    }

    user = euid == 0;
    rc = deletefile(vol, dfd, name, 0, NULL);

    if (user && seteuid(0) != 0) {
        goto out;
    }

    user = 0;
    kept = fstatat(dfd, adname, &st, AT_SYMLINK_NOFOLLOW) == 0;

    if (rc == AFPERR_ACCESS && kept == 1) {
        ret = 0;
    } else {
        fprintf(stderr, "refused delete: reply %d, AppleDouble file kept %d\n",
                rc, kept);
        ret = 1;
    }

out:

    if (user && seteuid(0) != 0) {
        ret = -1;
    }

    if (adfd >= 0) {
        close(adfd);
    }

    fchmod(dfd, 0700);
    unlinkat(dfd, adname, 0);
    unlinkat(dfd, name, 0);
    unlinkat(dfd, ".AppleDouble", AT_REMOVEDIR);
    close(dfd);
    rmdir(dir);
    vol->v_adouble = saved_adouble;
    initvol_vfs(vol);
    return ret;
}

/*!
 * @brief FPDelete of a file the root's cached item count never held, one made
 *        outside afpd, drops the count rather than wrap it
 *
 * @returns 0, 1 when the count wrapped or stayed in force, -1 when the setup
 *          failed
 */
int utest_delete_uncounted_drops_count(AFPObj *obj, struct vol *vol)
{
    static char name[] = "rq_uncounted";
    const uint32_t count = vol->v_root->d_offcnt;
    const time_t stamp = vol->v_root->d_ctime;
    char path[MAXPATHLEN + 1];
    struct stat st;
    int fd;
    int rc;
    int ret = -1;

    if (fio_open(obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    if ((fd = open(path, O_CREAT | O_WRONLY, 0600)) < 0) {
        goto out;
    }

    close(fd);

    if (stat(vol->v_path, &st) != 0) {
        goto out;
    }

    /* a count of no items, current by the root's ctime */
    vol->v_root->d_offcnt = 0;
    vol->v_root->d_ctime = st.st_ctime;
    rc = delete (&fio.obj, vol->v_vid, DIRDID_ROOT, name);

    if (rc == AFP_OK && vol->v_root->d_offcnt == 0
            && vol->v_root->d_ctime == 0) {
        ret = 0;
    } else {
        fprintf(stderr,
                "uncounted delete: reply %d, count %u, still in force %d\n", rc,
                vol->v_root->d_offcnt, vol->v_root->d_ctime != 0);
        ret = 1;
    }

out:
    vol->v_root->d_offcnt = count;
    vol->v_root->d_ctime = stamp;
    fio_done(vol, 0, name);
    return ret;
}
