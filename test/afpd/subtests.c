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

/*!
 * @brief Plant a cache entry keyed (DIRDID_ROOT, uname)
 *
 * Fullpath points at the volume root so lookup validation stats successfully
 * and returns the entry.
 */
static struct dir *plant_root_entry(const struct vol *vol, const char *uname,
                                    cnid_t did, struct stat *st)
{
    bstring fullpath = bfromcstr(vol->v_path);
    struct dir *entry = dir_new(uname, uname, vol, DIRDID_ROOT, did,
                                fullpath, st);

    if (entry == NULL) {
        /* dir_new takes ownership only on success */
        bdestroy(fullpath);
        return NULL;
    }

    dircache_add(vol, entry);
    return entry;
}

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
    const struct dir *stray;
    const struct dir *ret;
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

    /* Plant a stray entry for (root, uname) */
    stray = plant_root_entry(vol, uname, htonl(30003), &st);

    if (stray == NULL) {
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

/*!
 * @brief A second entry may never be published under a key already in use
 *
 * A key may name only one entry. Adds over both keys — (vid, did) and
 * (vid, pdid, uname) — with curdir on the entry being retired, the case that
 * drives a recovery lookup back into dircache_add().
 */
int test007_add_over_existing_key_leaves_one(struct vol *vol)
{
    struct dir *first = NULL;
    struct dir *second = NULL;
    struct dir *saved_curdir = curdir;
    const char *uname = "t007_dup_key";
    const cnid_t did = htonl(30007);
    struct stat st;
    int ret = -1;

    if (stat(vol->v_path, &st) != 0) {
        return -1;
    }

    first = plant_root_entry(vol, uname, did, &st);

    if (first == NULL) {
        return -1;
    }

    /* Not looked up: validation would reject the root's inode under this CNID
     * and retire the entry before the duplicate is attempted */
    curdir = first;
    /* Collides on both keys */
    second = dir_new(uname, uname, vol, DIRDID_ROOT, did,
                     bfromcstr(vol->v_path), &st);

    if (second == NULL) {
        ret = 3;
        goto out;
    }

    if (dircache_add(vol, second) != 0) {
        ret = 4;
        goto out;
    }

    /* Retired, awaiting the deferred free */
    if (first->d_did != CNID_INVALID) {
        ret = 7;
        goto out;
    }

    /* Taken over by the replacement, not recovered via dirlookup() */
    if (curdir != second) {
        ret = 8;
        goto out;
    }

    ret = 0;
out:

    if (ret != 0) {
        fprintf(stderr,
                "test007: failed at step %d (first=%p did:%u, second=%p, curdir=%p)\n",
                ret, (void *)first, first ? ntohl(first->d_did) : 0u,
                (void *)second, (void *)curdir);
    }

    curdir = saved_curdir;

    if (second != NULL && second->d_did != CNID_INVALID) {
        dir_remove(vol, second, 0);
    }

    /* On the paths that never reached the duplicate add, the planted entry
     * is still cached and must not outlive the test */
    if (first != NULL && first->d_did != CNID_INVALID) {
        dir_remove(vol, first, 0);
    }

    dir_free_invalid_q();
    return ret;
}

/*!
 * @brief Ghost-trim selection never yields the pinned entry
 *
 * Throwaway queues suffice: only the choice of node is under test.
 *
 * @returns 0, else the number of the case that disagreed
 */
int dircache_test_ghost_trim_selection(void)
{
    struct dir lru = {0};
    struct dir mru = {0};
    q_t *q = queue_init();
    int ret = 0;

    if (q == NULL) {
        return -1;
    }

    /* 1: nothing queued, nothing to release */
    if (arc_ghost_trim_candidate(q, NULL) != NULL) {
        ret = 1;
        goto out;
    }

    /* 2: a lone unpinned ghost is the candidate */
    const qnode_t *lru_node = enqueue(q, &lru);

    if (lru_node == NULL) {
        ret = -1;
        goto out;
    }

    if (arc_ghost_trim_candidate(q, NULL) != lru_node) {
        ret = 2;
        goto out;
    }

    /* 3: a lone ghost mid-promotion must be left alone */
    if (arc_ghost_trim_candidate(q, &lru) != NULL) {
        ret = 3;
        goto out;
    }

    /* 4: pinned at LRU, so the next ghost along is released instead */
    const qnode_t *mru_node = enqueue(q, &mru);

    if (mru_node == NULL) {
        ret = -1;
        goto out;
    }

    if (arc_ghost_trim_candidate(q, &lru) != mru_node) {
        ret = 4;
        goto out;
    }

    /* 5: pinned away from LRU leaves the LRU the candidate */
    if (arc_ghost_trim_candidate(q, &mru) != lru_node) {
        ret = 5;
        goto out;
    }

out:

    while (dequeue(q) != NULL) {
        /* nodes only; the entries are on this frame */
    }

    free(q);
    return ret;
}

/*!
 * @brief Re-keying onto a name a stale entry still holds retires the holder
 *
 * dir_modify(DCMOD_PATH) re-inserts under the destination key; a stale cache
 * entry for that key must be expunged, or the key would name two entries.
 *
 * @returns 0, else the number of the step that disagreed
 */
int test008_reindex_over_stale_key_expunges(struct vol *vol)
{
    struct dir *root;
    struct dir *stale = NULL;
    struct dir *renamed = NULL;
    struct stat st;
    int ret = -1;

    if ((root = dirlookup(vol, DIRDID_ROOT)) == NULL) {
        return -1;
    }

    if (stat(vol->v_path, &st) != 0) {
        return -1;
    }

    /* The stale holder of the destination key */
    stale = plant_root_entry(vol, "t008_dst", htonl(30008), &st);

    if (stale == NULL) {
        return 1;
    }

    /* The entry being renamed onto that key */
    renamed = plant_root_entry(vol, "t008_src", htonl(30018), &st);

    if (renamed == NULL) {
        ret = 2;
        goto out;
    }

    if (dir_modify(vol, renamed, &(struct dir_modify_args) {
    .flags = DCMOD_PATH,
    .new_mname = "t008_dst",
    .new_uname = "t008_dst",
    .new_pdir_path = root->d_fullpath,
}) != 0) {
        ret = 3;
        goto out;
    }

    /* The stale holder was retired, awaiting the deferred free */
    if (stale->d_did != CNID_INVALID) {
        ret = 4;
        goto out;
    }

    /* The renamed entry owns the key */
    if (renamed->d_did != htonl(30018)) {
        ret = 5;
        goto out;
    }

    ret = 0;
out:

    if (ret != 0) {
        fprintf(stderr,
                "test008: failed at step %d (stale=%p did:%u, renamed=%p did:%u)\n",
                ret, (void *)stale, stale ? ntohl(stale->d_did) : 0u,
                (void *)renamed, renamed ? ntohl(renamed->d_did) : 0u);
    }

    if (renamed != NULL && renamed->d_did != CNID_INVALID) {
        dir_remove(vol, renamed, 0);
    }

    if (stale != NULL && stale->d_did != CNID_INVALID) {
        dir_remove(vol, stale, 0);
    }

    dir_free_invalid_q();
    return ret;
}

/*!
 * @brief curdir survives its entry being expunged by a file
 *
 * A file colliding on the (vid, did) key retires the cached directory — a
 * mid-session CNID reset can hand a file a directory's stale did. A file
 * cannot stand in as curdir, so curdir must land on the volume root, never
 * NULL.
 *
 * @returns 0, else the number of the step that disagreed
 */
int test009_file_add_over_curdir_did(struct vol *vol)
{
    struct dir *dir = NULL;
    struct dir *file = NULL;
    struct dir *saved_curdir = curdir;
    const cnid_t did = htonl(30009);
    struct stat st;
    int ret = -1;

    if (stat(vol->v_path, &st) != 0) {
        return -1;
    }

    dir = plant_root_entry(vol, "t009_dir", did, &st);

    if (dir == NULL) {
        return 1;
    }

    curdir = dir;
    /* Same did, different name: collides only on the (vid, did) key */
    file = dir_new("t009_file", "t009_file", vol, DIRDID_ROOT, did,
                   bfromcstr(vol->v_path), &st);

    if (file == NULL) {
        ret = 2;
        goto out;
    }

    file->d_flags |= DIRF_ISFILE;

    if (dircache_add(vol, file) != 0) {
        ret = 3;
        goto out;
    }

    /* The directory was retired... */
    if (dir->d_did != CNID_INVALID) {
        ret = 4;
        goto out;
    }

    /* ...and curdir fell back to the volume root, not NULL or the file */
    if (curdir != vol->v_root) {
        ret = 5;
        goto out;
    }

    ret = 0;
out:

    if (ret != 0) {
        fprintf(stderr,
                "test009: failed at step %d (dir=%p did:%u, file=%p, curdir=%p)\n",
                ret, (void *)dir, dir ? ntohl(dir->d_did) : 0u,
                (void *)file, (void *)curdir);
    }

    curdir = saved_curdir;

    if (file != NULL && file->d_did != CNID_INVALID) {
        dir_remove(vol, file, 0);
    }

    if (dir != NULL && dir->d_did != CNID_INVALID) {
        dir_remove(vol, dir, 0);
    }

    dir_free_invalid_q();
    return ret;
}

/*!
 * @brief A full dircache keeps accepting inserts
 *
 * A cached entry and its ghost both occupy a hash slot, so once the working
 * set exceeds the cache size the hash sits at its 2c capacity; every add past
 * that point must release a slot before it inserts, or hash_insert()'s
 * capacity assert aborts. Ghosts appear only where an eviction demotes a
 * frequently-used entry, so each add is promoted at once: the entry moves to
 * T2 and its eventual eviction lands in B2, filling the cache to 2c. The
 * margin beyond that drives inserts against a cache pinned at capacity.
 *
 * Promoted rather than looked up: the planted names do not exist on disk, so
 * a lookup that validated would stat through the parent and expunge.
 *
 * @param[in] vol         volume the entries are planted on
 * @param[in] cache_size  the size dircache_init() was called with
 *
 * @returns 0, else the number of the step that disagreed
 */
int test010_full_cache_accepts_adds(struct vol *vol,
                                    unsigned int cache_size)
{
    const uint32_t base = 40000;
    const uint32_t count = cache_size * 2 + DIRCACHE_FREE_QUANTUM;
    char uname[24];
    struct stat st;
    uint32_t at = 0;
    int ret = 0;

    if (stat(vol->v_path, &st) != 0) {
        return -1;
    }

    for (uint32_t i = 0; i < count; i++) {
        const cnid_t did = htonl(base + i);
        struct dir *entry;
        at = i;
        snprintf(uname, sizeof(uname), "t010_%06u", i);

        if ((entry = plant_root_entry(vol, uname, did, &st)) == NULL) {
            ret = 1;
            break;
        }

        /* Published under its CNID: a plain index probe, no validation */
        if (dircache_lookup_parent(vol, did) != entry) {
            ret = 2;
            break;
        }

        dircache_promote(entry);
    }

    if (ret != 0) {
        fprintf(stderr,
                "test010: failed at step %d on add %u of %u (cache_size=%u)\n",
                ret, at, count, cache_size);
    }

    /* The flood evicted everything else; leave later tests a cold cache
     * rather than one full of t010 entries. The volume root is never cached,
     * so parking curdir there keeps it clear of the purge. */
    curdir = vol->v_root;
    dircache_purge_vol(vol);
    dir_free_invalid_q();
    return ret;
}
