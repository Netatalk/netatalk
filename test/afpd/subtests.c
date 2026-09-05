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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/ea.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/queue.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afp_config.h"
#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "hash.h"
#include "subtests.h"
#include "test.h"
#include "volume.h"

int test001_add_x_dirs(const struct vol *vol, cnid_t start, cnid_t end)
{
    struct dir *dir;
    char dirname[20];

    while (start++ < end) {
        snprintf(dirname, sizeof(dirname), "dir%04u", start);
        dir = dir_new(dirname, dirname, vol, DIRDID_ROOT, htonl(start),
                      bfromcstr(vol->v_path), 0);

        if (dir == NULL) {
            return -1;
        }

        if (dircache_add(vol, dir) != 0) {
            return -1;
        }
    }

    return 0;
}

int test002_rem_x_dirs(const struct vol *vol, cnid_t start, cnid_t end)
{
    struct dir *dir;

    while (start++ < end) {
        if ((dir = dircache_search_by_did(vol, htonl(start))))
            if (dir_remove(vol, dir, 0) != 0) {
                return -1;
            }
    }

    return 0;
}

/* dir_add() error path vs a stray cache entry: the stray is routed through
 * dir_remove() (which queues it on invalid_dircache_entries for deferred
 * free), then a later step fails.  The exit block must NOT free the queued
 * entry a second time — dir_free_invalid_q() owns that free.  Detection is
 * by the sanitizer/valgrind leg: a double free aborts the run. */
int test003_dir_add_error_no_double_free(struct vol *vol)
{
    const struct dir *root;
    struct dir *stray, *ret;
    struct path path;
    struct stat st;
    struct _cnid_db *saved_cdb;
    static char uname[] = "straydir_t003";

    if ((root = dirlookup(vol, DIRDID_ROOT)) == NULL) {
        return -1;
    }

    if (stat(vol->v_path, &st) != 0) {
        return -1;
    }

    /* Plant a stray entry for (root, uname).  Fullpath points at the volume
     * root so the lookup validation stat succeeds and returns the entry. */
    stray = dir_new(uname, uname, vol, DIRDID_ROOT, htonl(30003),
                    bfromcstr(vol->v_path), &st);

    if (stray == NULL || dircache_add(vol, stray) != 0) {
        return -1;
    }

    /* Force the get_id() step to fail: with v_cdb == NULL it returns
     * CNID_INVALID, taking dir_add's err=1 exit path right after the stray
     * has been dir_remove()'d (queued). */
    saved_cdb = vol->v_cdb;
    vol->v_cdb = NULL;
    memset(&path, 0, sizeof(path));
    path.u_name = uname;
    ret = dir_add(vol, root, &path, (int)strlen(uname));
    vol->v_cdb = saved_cdb;

    if (ret != NULL) {
        /* error path not taken — test preconditions broken */
        return -1;
    }

    /* Drain the deferred-free queue (normally end-of-request).  If dir_add's
     * exit block freed the queued stray itself, this is a double free — the
     * sanitizer/valgrind leg aborts here. */
    dir_free_invalid_q();
    return 0;
}

/* getmetadata() must not treat an unfilled struct path stat as evidence of
 * external replacement.  getforkparams()'s LNAME-only bitmap reaches the
 * CNID block with path.st all-zeros (no stat-filling bit requested); the
 * ino-mismatch check would compare dcache_ino against st_ino == 0 and
 * expunge a perfectly valid dircache entry on every call. */
int test004_getmetadata_nostat_keeps_cache(struct vol *vol)
{
    struct dir *root, *entry;
    struct path p;
    struct stat st;
    char fpath[MAXPATHLEN + 1];
    char buf[4096];
    size_t buflen = 0;
    static char fname[] = "t004_file";
    int ret = -1;

    if ((root = dirlookup(vol, DIRDID_ROOT)) == NULL) {
        return -1;
    }

    snprintf(fpath, sizeof(fpath), "%s/%s", vol->v_path, fname);
    FILE *fh = fopen(fpath, "w");

    if (fh == NULL) {
        return -1;
    }

    fputs("t004", fh);
    fclose(fh);

    if (stat(fpath, &st) != 0) {
        unlink(fpath);
        return -1;
    }

    entry = dir_new(fname, fname, vol, DIRDID_ROOT, htonl(30004),
                    bfromcstr(fpath), &st);

    if (entry == NULL || dircache_add(vol, entry) != 0) {
        unlink(fpath);
        return -1;
    }

    /* Mirror getforkparams()'s LNAME-only state: zeroed path, no id, and
     * the stat block never ran (st all-zeros, st_valid 0). */
    memset(&p, 0, sizeof(p));
    p.u_name = fname;
    p.m_name = fname;

    if (getmetadata(AFPobj, vol, 1 << FILPBIT_LNAME, &p, root,
                    buf, &buflen, NULL) != AFP_OK) {
        goto out;
    }

    /* The cached entry must survive: an unfilled stat is not evidence of
     * replacement (a zero-ino compare would expunge it here). */
    if (entry->d_did == CNID_INVALID) {
        /* stderr, not test_stream(): this file is also linked into the
         * fuzz target, which does not carry test.c's globals */
        fprintf(stderr,
                "t004: valid dircache entry expunged on unfilled stat\n");
        goto out;
    }

    ret = 0;
out:
    /* On failure a replacement entry may have been re-added under a
     * different pointer — remove whatever the cache now holds. */
    {
        struct dir *now = dircache_search_by_name(vol, root, fname,
                                                  strlen(fname));

        if (now) {
            dir_remove(vol, now, 0);
        }
    }

    if (entry->d_did != CNID_INVALID) {
        dir_remove(vol, entry, 0);
    }

    dir_free_invalid_q();
    unlink(fpath);
    return ret;
}

/* An open fork legitimately outlives its path (renamed/unlinked by another
 * session).  getforkparams provides the object's stat from the held fd
 * (st_fd); with the path dead, getmetadata must answer identity by READ
 * (adouble id or, only while the name is unoccupied, the CNID DB record),
 * never mint the dead path into the dircache, and never rebind the CNID
 * DB record.  Exercised in the production data-fork-only shape (fd-derived
 * stat, adp == NULL) across two legs:
 *   unlink  — name unoccupied: the registered record answers, unchanged;
 *   replace — another object now occupies the dead name: its identity
 *             must never be returned for the held fork (refusal is the
 *             only honest answer without an adouble header), and its
 *             record must not be rebound. */
int test005_getmetadata_open_fork_outlives_path(struct vol *vol)
{
    struct dir *root;
    struct adouble ad;
    struct path p;
    struct stat st, st2;
    char fpath[MAXPATHLEN + 1];
    char buf[4096];
    size_t buflen = 0;
    static char fname[] = "t005_file";
    cnid_t live_id = CNID_INVALID, replacement_id = CNID_INVALID;
    cnid_t got;
    int ret = -1;
    int replacement_fd = -1;
    int rc;

    if ((root = dirlookup(vol, DIRDID_ROOT)) == NULL) {
        return -1;
    }

    snprintf(fpath, sizeof(fpath), "%s/%s", vol->v_path, fname);
    ad_init(&ad, vol);

    if (ad_open(&ad, fpath, ADFLAGS_DF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                0600) != 0) {
        return -1;
    }

    /* Register the object's CNID while the path is live, as any real
     * access would have */
    if (fstat(ad_data_fileno(&ad), &st) != 0
            || (live_id = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, fname,
                                   strlen(fname), 0)) == CNID_INVALID) {
        goto out;
    }

    /* getforkparams shape for a data-fork-only fork: st filled by fstat
     * on the held fd (st_fd), adp NULL (no metadata open) */
    memset(&p, 0, sizeof(p));
    p.u_name = fname;
    p.m_name = fname;
    p.st = st;
    p.st_valid = 1;
    p.st_fd = 1;

    /* --- Leg 1: unlink — the other session removes the file --- */
    if (unlink(fpath) != 0) {
        goto out;
    }

    rc = getmetadata(AFPobj, vol, 1 << FILPBIT_FNUM, &p, root,
                     buf, &buflen, NULL);

    if (rc != AFP_OK) {
        fprintf(stderr,
                "t005: held fork failed getmetadata after unlink (rc %d)\n",
                rc);
        goto out;
    }

    /* The dead path must not have been cached. Probe the request-scoped
     * memo, not the by-name index — that lookup validates (stat) and
     * would expunge the dead entry, masking the mint. */
    if (p.d_cached != NULL) {
        fprintf(stderr, "t005: dead path minted into the dircache\n");
        goto out;
    }

    if (cnid_get(vol->v_cdb, DIRDID_ROOT, fname, strlen(fname)) != live_id) {
        fprintf(stderr, "t005: CNID DB record rebound after unlink leg\n");
        goto out;
    }

    /* --- Leg 2: replace — a different object now occupies the name --- */
    /* Path-based on purpose: this test IS the racing second session — it
     * recreates the just-unlinked name, exactly the replacement race the
     * held fork's identity answer must survive. O_EXCL guarantees a fresh
     * object. */
    replacement_fd = open(fpath, O_WRONLY | O_CREAT | O_EXCL, 0600); // NOSONAR

    if (replacement_fd < 0 || fstat(replacement_fd, &st2) != 0) {
        goto out;
    }

    /* The replacement registers itself, taking over the (did, name)
     * record — the renamer/creator side of a real race does this */
    if ((replacement_id = cnid_add(vol->v_cdb, &st2, DIRDID_ROOT, fname,
                                   strlen(fname), 0)) == CNID_INVALID) {
        goto out;
    }

    memset(&p, 0, sizeof(p));
    p.u_name = fname;
    p.m_name = fname;
    p.st = st;
    p.st_valid = 1;
    p.st_fd = 1;
    rc = getmetadata(AFPobj, vol, 1 << FILPBIT_FNUM, &p, root,
                     buf, &buflen, NULL);

    if (rc == AFP_OK) {
        memcpy(&got, buf, sizeof(got));

        if (got == replacement_id) {
            fprintf(stderr,
                    "t005: held fork answered with the replacement's CNID\n");
            goto out;
        }
    }

    /* Either refusal (NOOBJ) or a non-replacement id is acceptable; the
     * replacement's record must be untouched either way */
    if (cnid_get(vol->v_cdb, DIRDID_ROOT, fname,
                 strlen(fname)) != replacement_id) {
        fprintf(stderr, "t005: replacement's CNID record rebound\n");
        goto out;
    }

    ret = 0;
out:

    if (replacement_fd >= 0) {
        close(replacement_fd);
    }

    ad_close(&ad, ADFLAGS_DF);
    unlink(fpath);
    {
        /* Reruns and later tests start clean */
        struct dir *now = dircache_search_by_name(vol, root, fname,
                                                  strlen(fname));

        if (now) {
            dir_remove(vol, now, 0);
        }
    }

    if (live_id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, live_id);
    }

    if (replacement_id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, replacement_id);
    }

    dir_free_invalid_q();
    return ret;
}

/* ea=sys can hold a resource fork with no metadata EA (Samba vfs_fruit,
 * macOS-native writers). Such a file must mint AD_RLEN_RFORK_ONLY, not
 * AD_RLEN_NO_AD, and its RFLEN must come from the on-disk fork. */
int test006_rflen_rfork_without_metadata(struct vol *vol)
{
    static const char rdata[] = "t006-resource-fork-payload";
    struct dir *root, *entry = NULL;
    struct adouble ad;
    struct path p;
    struct stat st;
    char fpath[MAXPATHLEN + 1], sidecar[MAXPATHLEN + 1];
    char buf[4096];
    size_t buflen = 0;
    static char fname[] = "t006_file";
    uint32_t rlen_wire;
    int ret = -1;
    int rc;
    int cwd_fd = -1;

    if ((root = dirlookup(vol, DIRDID_ROOT)) == NULL) {
        return -1;
    }

    snprintf(fpath, sizeof(fpath), "%s/%s", vol->v_path, fname);
    snprintf(sidecar, sizeof(sidecar), "%s/._%s", vol->v_path, fname);

    /* getmetadata resolves u_name relative to cwd — afpd is always
     * fchdir'd into curdir; mirror that for this volume's root */
    if ((cwd_fd = open(".", O_RDONLY)) < 0 || chdir(vol->v_path) != 0) {
        if (cwd_fd >= 0) {
            close(cwd_fd);
        }

        return -1;
    }

    ad_init(&ad, vol);

    if (ad_open(&ad, fpath,
                ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF | ADFLAGS_RDWR |
                ADFLAGS_CREATE, 0600) != 0) {
        goto out;
    }

    if (ad_write(&ad, ADEID_RFORK, 0, 0, rdata,
                 sizeof(rdata)) != (ssize_t)sizeof(rdata)) {
        ad_close(&ad, ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF);
        goto out;
    }

    ad_flush(&ad);
    ad_close(&ad, ADFLAGS_DF | ADFLAGS_HF | ADFLAGS_RF);

    /* Strip ONLY the metadata, as a foreign writer never creates it;
     * the resource fork stays. Metadata not in an EA (no xattr support,
     * or a v2 sidecar volume where metadata and rfork share the file):
     * the mixed state cannot exist there — skip. */
    if (sys_lremovexattr(fpath, AD_EA_META) != 0) {
        ret = TEST_SKIP;
        goto out;
    }

    if (stat(fpath, &st) != 0) {
        goto out;
    }

    bstring fullpath = bfromcstr(fpath);

    if (fullpath == NULL) {
        goto out;
    }

    /* dir_new takes ownership of fullpath only on success */
    entry = dir_new(fname, fname, vol, DIRDID_ROOT, htonl(30006),
                    fullpath, &st);

    if (entry == NULL) {
        bdestroy(fullpath);
        goto out;
    }

    if (dircache_add(vol, entry) != 0) {
        dir_free(entry);
        entry = NULL;
        goto out;
    }

    /* One FPGetFileParms for RFLEN: the metadata read inside fails ENOENT
     * (mint point), and the reply must still carry the on-disk fork size */
    memset(&p, 0, sizeof(p));
    p.u_name = fname;
    p.m_name = fname;
    p.st = st;
    p.st_valid = 1;
    rc = getfilparams(AFPobj, vol, 1 << FILPBIT_RFLEN, &p, root,
                      buf, &buflen, 0);

    if (rc != AFP_OK || buflen < sizeof(rlen_wire)) {
        fprintf(stderr, "t006: getfilparams rc %d buflen %zu\n", rc, buflen);
        goto out;
    }

    memcpy(&rlen_wire, buf, sizeof(rlen_wire));

    if (ntohl(rlen_wire) != sizeof(rdata)) {
        fprintf(stderr,
                "t006: RFLEN %u, want %zu (rfork masked by negative AD cache)\n",
                ntohl(rlen_wire), sizeof(rdata));
        goto out;
    }

    /* NO_AD here would pin RFLEN at 0; UNKNOWN would re-read the
     * metadata from disk on every access */
    if (entry->dcache_rlen != AD_RLEN_RFORK_ONLY) {
        fprintf(stderr, "t006: dcache_rlen %lld, want AD_RLEN_RFORK_ONLY\n",
                (long long)entry->dcache_rlen);
        goto out;
    }

    ret = 0;
out:

    if (entry) {
        dir_remove(vol, entry, 0);
    }

    dir_free_invalid_q();
    unlink(fpath);
    unlink(sidecar);

    if (fchdir(cwd_fd) != 0 && ret == 0) {
        ret = -1;
    }

    close(cwd_fd);
    return ret;
}
