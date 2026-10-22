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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/compat.h>
#include <atalk/directory.h>
#include <atalk/ea.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/queue.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afp_config.h"
#include "afpfunc_helpers.h"
#include "desktop.h"
#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "filedir.h"
#include "fork.h"
#include "hash.h"
#include "mangle.h"
#include "pfd_cache.h"
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

/*!
 * @brief Retire a name's dircache entry, as another session's cache hint would
 *
 * @param[in] vol   volume
 * @param[in] did   the entry's folder
 * @param[in] name  the entry's unix name
 */
static void drop_entry(const struct vol *vol, cnid_t did, char *name)
{
    const struct dir *parent = dirlookup(vol, did);
    struct dir *entry;

    if (parent != NULL
            && (entry = dircache_search_by_name(vol, parent, name,
                                                strlen(name))) != NULL) {
        dir_remove(vol, entry, 0);
    }

    dir_free_invalid_q();
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
 * @brief A stat refresh that moves the ctime drops the cached access rights
 *
 * check_acl_access() serves d_rights_cache while dcache_ctime matches the
 * disk; a refresh that brings dcache_ctime forward must not carry rights
 * computed under the old stat.
 *
 * @param[in] vol  volume whose root holds the planted entry
 * @returns 0, else the number of the step that disagreed
 */
int test011_stat_refresh_drops_rights(struct vol *vol)
{
    struct dir *entry;
    struct stat st;
    struct dir_modify_args args = { .flags = DCMOD_STAT, .st = &st };
    int ret;

    if (stat(vol->v_path, &st) != 0) {
        return -1;
    }

    entry = plant_root_entry(vol, "t011_dir", htonl(30011), &st);

    if (entry == NULL) {
        return 1;
    }

    entry->d_rights_cache = 1;
    st.st_ctime++;

    if (dir_modify(vol, entry, &args) != 0) {
        ret = 2;
    } else if (entry->d_rights_cache != 0xffffffff) {
        ret = 3;
    } else {
        /* the same stat again: nothing moved, so cached rights stay */
        entry->d_rights_cache = 1;
        ret = dir_modify(vol, entry, &args) == 0
              && entry->d_rights_cache == 1 ? 0 : 4;
    }

    if (ret == 0) {
        /* a sibling's hint carries DCMOD_AD_INV: the rights go even when the
         * stat did not move */
        args.flags = DCMOD_STAT | DCMOD_AD_INV;
        entry->d_rights_cache = 1;
        ret = dir_modify(vol, entry, &args) == 0
              && entry->d_rights_cache == 0xffffffff ? 0 : 5;
    }

    dir_remove(vol, entry, 0);
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

/*!
 * @brief Whether the file at path contains needle on some line
 */
static bool file_contains(const char *path, const char *needle)
{
    char buf[4096];
    FILE *fp = fopen(path, "r");
    bool found = false;

    if (fp == NULL) {
        return false;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (strstr(buf, needle) != NULL) {
            found = true;
            break;
        }
    }

    fclose(fp);
    return found;
}

/*!
 * @brief log_close_all() and log_reopen() bring back a relative and a mkstemp log
 *
 * daemonize() chdir()s to / between the two calls, so the logger has to keep
 * absolute names: a relative "log file" stays in the launch directory and a
 * mkstemp() file is reopened under the name it got.
 *
 * @returns 0 when both files receive the line written after the reopen
 */
int utest_logger_reopen_keeps_files(void)
{
    char tmpdir[MAXPATHLEN];
    char rel_path[MAXPATHLEN] = "";
    char generated[MAXPATHLEN] = "";
    char tmpl[MAXPATHLEN];
    DIR *dir;
    struct dirent *ent;
    struct stat st;
    mode_t saved_umask;
    int cwd;
    int result = 0;

    if ((cwd = open(".", O_RDONLY)) < 0) {
        return 1;
    }

    /* the scratch directory lives under the working directory, the build
     * tree, not under a world-writable /tmp */
    if (getcwd(tmpdir, sizeof(tmpdir)) == NULL
            || strlcat(tmpdir, "/afpd_logger_XXXXXX", sizeof(tmpdir))
            >= sizeof(tmpdir) || mkdtemp(tmpdir) == NULL) {
        close(cwd);
        return 2;
    }

    if (chdir(tmpdir) != 0) {
        result = 3;
        goto out;
    }

    /* Even a permissive caller umask must not expose log contents to others. */
    saved_umask = umask(0); //NOSONAR: exercise log creation with no umask protection
    setuplog("default:note", "reopen.log", true);
    umask(saved_umask); //NOSONAR: restore the caller's original umask
    snprintf(rel_path, sizeof(rel_path), "%s/reopen.log", tmpdir);

    if (stat(rel_path, &st) != 0 || (st.st_mode & 0777) != 0640) {
        result = 9;
        goto out;
    }

    /* Reopening an administrator-restricted file must preserve its mode. */
    if (chmod(rel_path, 0600) != 0) {
        result = 10;
        goto out;
    }

    log_close_all();

    if (chdir("/") != 0) {
        result = 4;
        goto out;
    }

    log_reopen();
    LOG(log_note, logtype_default, "utest marker: relative log reopened");

    if (!file_contains(rel_path, "relative log reopened")) {
        result = 5;
    }

    if (stat(rel_path, &st) != 0 || (st.st_mode & 0777) != 0600) {
        result = result ? result : 11;
    }

    if (access("/reopen.log", F_OK) == 0) {
        unlink("/reopen.log");
        result = 6;
    }

    snprintf(tmpl, sizeof(tmpl), "%s/mkstemp.XXXXXX", tmpdir);
    setuplog("default:note", tmpl, true);
    log_close_all();
    log_reopen();
    LOG(log_note, logtype_default, "utest marker: mkstemp log reopened");

    if ((dir = opendir(tmpdir)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, "mkstemp.", 8) == 0) {
                snprintf(generated, sizeof(generated), "%s/%s", tmpdir,
                         ent->d_name);
            }
        }

        closedir(dir);
    }

    if (generated[0] == '\0' || !file_contains(generated, "mkstemp log reopened")) {
        result = result ? result : 7;
    }

    if (stat(generated, &st) != 0 || (st.st_mode & 0777) != 0600) {
        result = result ? result : 12;
    }

out:
    setuplog("default:note", "/dev/stderr", true);

    if (fchdir(cwd) != 0 && result == 0) {
        result = 8;
    }

    close(cwd);

    if (rel_path[0] != '\0') {
        unlink(rel_path);
    }

    if (generated[0] != '\0') {
        unlink(generated);
    }

    rmdir(tmpdir);
    return result;
}

/* PID files are public, but creating one must not relax subsequent creations,
 * including when daemon startup aborts because the file is stale or unwritable. */
int utest_lockfile_restores_umask(void)
{
#ifndef SOLARIS
    char tmpdir[] = "afpd_lockfile_XXXXXX";
    char pidfile[MAXPATHLEN];
    struct stat st;
    mode_t saved_umask;
    int result = 0;

    if (mkdtemp(tmpdir) == NULL) {
        return 1;
    }

    snprintf(pidfile, sizeof(pidfile), "%s/server.pid", tmpdir);
    /* A restrictive caller mask exposes any failure to restore it from 022. */
    saved_umask = umask(077); //NOSONAR: establish a restrictive mask for the restoration test

    if (create_lockfile("utest", pidfile) != 0
            || stat(pidfile, &st) != 0 || (st.st_mode & 0777) != 0644
            || umask(077) != 077) { //NOSONAR: check and reset the test's restrictive mask
        result = 2;
        goto out;
    }

    /* The PID is our own, so neither call will proceed to daemonize. */
    if (server_lock("utest", pidfile, 0) != -1 || umask(077) != 077 //NOSONAR: check and reset the test's restrictive mask
            || create_lockfile("utest", pidfile) != -1 || umask(077) != 077) { //NOSONAR: check and reset the test's restrictive mask
        result = 3;
        goto out;
    }

    if (unlink(pidfile) != 0 || rmdir(tmpdir) != 0) {
        result = 4;
        goto out;
    }

    /* The parent is now absent, so opening the PID file must fail. */
    if (create_lockfile("utest", pidfile) != -1 || umask(077) != 077 //NOSONAR: check and reset the test's restrictive mask
            || server_lock("utest", pidfile, 0) != -1 || umask(077) != 077) { //NOSONAR: check and reset the test's restrictive mask
        result = 5;
    }

out:
    umask(saved_umask); //NOSONAR: restore the caller's original umask
    unlink(pidfile);
    rmdir(tmpdir);
    return result;
#else
    return 0;
#endif
}

/*!
 * @brief A failed CNID retry in movecwd() logs the directory it tried
 *
 * The directory moves aside and a dangling symlink takes its name: the
 * cached path fails with ENOENT, and the CNID rebuild lstat()s the same
 * path, so the retry reaches its chdir and fails as well.
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes the directory
 * @returns 0, 1 when the log lacks the path, -1 when the setup failed
 */
int test012_movecwd_retry_names_directory(AFPObj *obj, struct vol *vol)
{
    static char dname[] = "t012_dir";
    char aside[32];
    char logpath[MAXPATHLEN + 1];
    char needle[MAXPATHLEN + 64];
    struct dir *saved_curdir = curdir;
    struct dir *dir;
    cnid_t did = CNID_INVALID;
    int cwd_fd = open(".", O_RDONLY);
    int vfd = open(vol->v_path, O_RDONLY | O_DIRECTORY);
    int created = 0;
    int linked = 0;
    int logged = 0;
    int ret = -1;
    int rc;
    /* this run's names: what another run left keeps its own */
    snprintf(aside, sizeof(aside), "t012_aside.%d", (int)getpid());
    /* absolute: movecwd() changes the working directory */
    snprintf(logpath, sizeof(logpath), "%s/t012_log.%d", vol->v_path,
             (int)getpid());

    if (cwd_fd < 0 || vfd < 0
            || createdir(obj, vol->v_vid, DIRDID_ROOT, dname) != AFP_OK) {
        goto out;
    }

    created = 1;
    /* afp_createdir() leaves curdir in the directory it made */
    dir = curdir;
    did = dir->d_did;

    if (movecwd(vol, vol->v_root) != 0) {
        goto out;
    }

    /* a move keeps whatever metadata the directory holds; a failed one
     * leaves the directory, and the symlink below fails */
    renameat(vfd, dname, vfd, aside);

    if (symlinkat("t012_nowhere", vfd, dname) != 0) {
        goto out;
    }

    linked = 1;
    setuplog("default:info", logpath, true);
    logged = 1;
    rc = movecwd(vol, dir);
    setuplog("default:note", "/dev/stderr", true);
    snprintf(needle, sizeof(needle), "\"%s/%s\" (did:%u) failed:", vol->v_path,
             dname, ntohl(did));

    if (rc == -1 && file_contains(logpath, needle)) {
        ret = 0;
    } else {
        fprintf(stderr, "t012: movecwd %d, log lacks: %s\n", rc, needle);
        ret = 1;
    }

out:

    if (logged) {
        unlink(logpath);
    }

    /* the directory back under its name, for FPDelete to remove it and
     * whatever metadata it holds */
    if (created) {
        if (linked) {
            unlinkat(vfd, dname, 0);
        }

        renameat(vfd, aside, vfd, dname);
        drop_entry(vol, DIRDID_ROOT, dname);
        delete (obj, vol->v_vid, DIRDID_ROOT, dname);
        cnid_delete(vol->v_cdb, did);
    }

    if (vfd >= 0) {
        close(vfd);
    }

    if (cwd_fd >= 0) {
        if (fchdir(cwd_fd) != 0 && ret == 0) {
            ret = -1;
        }

        close(cwd_fd);
    }

    curdir = saved_curdir;
    return ret;
}

/*!
 * @brief movecwd() refuses a folder whose cached path goes through a
 *        symlink to another folder
 *
 * t013_a/t013_b is cached, then t013_a moves to t013_a2 and a symlink takes
 * its place, so the cached path of t013_b goes through the symlink.
 */
int test013_movecwd_refuses_symlinked_path(struct vol *vol)
{
    static char aname[] = "t013_a";
    static char a2name[] = "t013_a2";
    static char bname[] = "t013_b";
    static char bpath[] = "t013_a/t013_b";
    struct dir *saved_curdir = curdir;
    struct dir *b = NULL;
    struct stat st;
    cnid_t adid = CNID_INVALID;
    cnid_t bdid = CNID_INVALID;
    int cwd_fd = open(".", O_RDONLY);
    int vfd = open(vol->v_path, O_RDONLY | O_DIRECTORY);
    int ret = -1;

    if (cwd_fd < 0 || vfd < 0 || mkdirat(vfd, aname, 0700) != 0
            || mkdirat(vfd, bpath, 0700) != 0
            || fstatat(vfd, aname, &st, 0) != 0
            || (adid = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, aname,
                                strlen(aname), 0)) == CNID_INVALID
            || fstatat(vfd, bpath, &st, 0) != 0
            || (bdid = cnid_add(vol->v_cdb, &st, adid, bname,
                                strlen(bname), 0)) == CNID_INVALID
            || (b = dirlookup(vol, bdid)) == NULL
            || renameat(vfd, aname, vfd, a2name) != 0
            || symlinkat(a2name, vfd, aname) != 0) {
        goto out;
    }

    if (movecwd(vol, b) == -1 && afp_errno == AFPERR_BADTYPE) {
        ret = 0;
    }

out:

    /* the symlink, then the folders back in place, however far the setup
     * came */
    if (vfd >= 0) {
        if (fstatat(vfd, aname, &st, AT_SYMLINK_NOFOLLOW) == 0
                && S_ISLNK(st.st_mode)) {
            unlinkat(vfd, aname, 0);
        }

        renameat(vfd, a2name, vfd, aname);
        unlinkat(vfd, bpath, AT_REMOVEDIR);
        unlinkat(vfd, aname, AT_REMOVEDIR);
        close(vfd);
    }

    if (b != NULL) {
        dir_remove(vol, b, 0);
    }

    drop_entry(vol, DIRDID_ROOT, aname);

    if (bdid != CNID_INVALID) {
        cnid_delete(vol->v_cdb, bdid);
    }

    if (adid != CNID_INVALID) {
        cnid_delete(vol->v_cdb, adid);
    }

    if (cwd_fd >= 0) {
        if (fchdir(cwd_fd) != 0 && ret == 0) {
            ret = -1;
        }

        close(cwd_fd);
    }

    curdir = saved_curdir;
    return ret;
}

/*!
 * @brief movecwd() fails with ENOTDIR on a cached folder that became a file
 *        or a symlink to a file
 */
int test014_movecwd_folder_became_file(struct vol *vol)
{
    static char dname[] = "t014_dir";
    static char fname[] = "t014_dir_file";
    struct dir *saved_curdir = curdir;
    struct dir *dir = NULL;
    struct stat st;
    cnid_t did = CNID_INVALID;
    int cwd_fd = open(".", O_RDONLY);
    int vfd = open(vol->v_path, O_RDONLY | O_DIRECTORY);
    int fd = -1;
    int ret = -1;

    if (cwd_fd < 0 || vfd < 0 || mkdirat(vfd, dname, 0700) != 0
            || fstatat(vfd, dname, &st, 0) != 0
            || (did = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, dname,
                               strlen(dname), 0)) == CNID_INVALID
            || (dir = dirlookup(vol, did)) == NULL
            || unlinkat(vfd, dname, AT_REMOVEDIR) != 0
            || (fd = openat(vfd, fname, O_WRONLY | O_CREAT, 0600)) < 0
            || linkat(vfd, fname, vfd, dname, 0) != 0) {
        goto out;
    }

    /* the file in the folder's place, then a symlink to it */
    errno = 0;

    if (movecwd(vol, dir) != -1 || errno != ENOTDIR
            || unlinkat(vfd, dname, 0) != 0
            || symlinkat(fname, vfd, dname) != 0) {
        goto out;
    }

    errno = 0;

    if (movecwd(vol, dir) == -1 && errno == ENOTDIR) {
        ret = 0;
    }

out:

    if (fd >= 0) {
        close(fd);
    }

    if (vfd >= 0) {
        unlinkat(vfd, fname, 0);

        /* the symlink, or the folder when the swap did not happen */
        if (unlinkat(vfd, dname, 0) != 0) {
            unlinkat(vfd, dname, AT_REMOVEDIR);
        }

        close(vfd);
    }

    drop_entry(vol, DIRDID_ROOT, dname);

    if (did != CNID_INVALID) {
        cnid_delete(vol->v_cdb, did);
    }

    if (cwd_fd >= 0) {
        if (fchdir(cwd_fd) != 0 && ret == 0) {
            ret = -1;
        }

        close(cwd_fd);
    }

    curdir = saved_curdir;
    return ret;
}

/*!
 * @brief movecwd() refuses a folder whose path climbs out of the volume
 *        through ".."
 *
 * The folder is built by hand, so its path reaches movecwd() as it is.
 *
 * @param[in] vol  volume whose root's parent the path names
 * @returns 0, 1 when movecwd() entered the parent, -1 when the setup failed
 */
int test015_movecwd_refuses_dotdot(struct vol *vol)
{
    char cwd[MAXPATHLEN + 1];
    struct dir *saved_curdir = curdir;
    struct dir *up = NULL;
    struct stat st;
    int cwd_fd = open(".", O_RDONLY);
    int ret = -1;

    if (cwd_fd < 0 || stat(vol->v_path, &st) != 0
            || (up = dir_new("..", "..", vol, DIRDID_ROOT, DIRDID_ROOT,
                             bformat("%s/..", vol->v_path), &st)) == NULL) {
        goto out;
    }

    if (movecwd(vol, up) == -1 && afp_errno == AFPERR_BADTYPE
            && getcwd(cwd, sizeof(cwd)) != NULL
            && strcmp(cwd, vol->v_path) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "t015: movecwd entered %s/..\n", vol->v_path);
        ret = 1;
    }

out:

    if (up != NULL) {
        dir_free(up);
    }

    if (cwd_fd >= 0) {
        if (fchdir(cwd_fd) != 0 && ret == 0) {
            ret = -1;
        }

        close(cwd_fd);
    }

    curdir = saved_curdir;
    return ret;
}

/*!
 * @brief A request naming "." or ".." is refused before the name gets a
 *        dircache entry or a CNID row
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes the test's folder
 * @returns 0, 1 when a request took the name, -1 when the setup failed
 */
int test016_cname_refuses_dot_names(AFPObj *obj, struct vol *vol)
{
    static char dname[] = "t016_dir";
    struct dir *saved_curdir = curdir;
    cnid_t did;
    int ret = 1;

    if (createdir(obj, vol->v_vid, DIRDID_ROOT, dname) != AFP_OK) {
        return -1;
    }

    /* afp_createdir() leaves curdir in the directory it made; "." names
     * it, ".." its parent */
    did = curdir->d_did;

    if (getfiledirparms(obj, vol->v_vid, did, ".") == AFPERR_PARAM
            && getfiledirparms(obj, vol->v_vid, did, "..") == AFPERR_PARAM
            && cnid_get(vol->v_cdb, DIRDID_ROOT, dname, strlen(dname)) == did) {
        ret = 0;
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, dname);
    curdir = saved_curdir;
    return ret;
}

/*!
 * @brief On a filesystem that ignores case, a request is refused a vetoed
 *        folder and a reserved name in another case
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes the test's folder
 * @returns 0, 1 when a request took a name or afpd found the filesystem
 *          matching case, -1 when the setup failed, TEST_SKIP where the
 *          filesystem matches case
 */
int test017_cname_vetoes_any_case(AFPObj *obj, struct vol *vol)
{
    static char dname[] = "T017_Veto";
    static char lname[] = "t017_veto";
    static char veto[] = "T017_Veto/";
    char lpath[MAXPATHLEN + 1];
    char *saved_veto = vol->v_veto;
    struct dir *saved_curdir = curdir;
    struct stat st;
    int ret = 1;

    if (createdir(obj, vol->v_vid, DIRDID_ROOT, dname) != AFP_OK) {
        return -1;
    }

    /* it runs where the filesystem ignores case, which afpd must find */
    snprintf(lpath, sizeof(lpath), "%s/%s", vol->v_path, lname);

    if (stat(lpath, &st) != 0) {
        ret = TEST_SKIP;
    } else if (vol->v_ignorecase) {
        vol->v_veto = veto;

        if (getfiledirparms(obj, vol->v_vid, DIRDID_ROOT, lname) == AFPERR_PARAM
                && getfiledirparms(obj, vol->v_vid, DIRDID_ROOT,
                                   ".appledb") == AFPERR_PARAM) {
            ret = 0;
        }

        vol->v_veto = saved_veto;
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, dname);
    curdir = saved_curdir;
    return ret;
}

/*!
 * @brief veto_file() matches a name in either ASCII case when asked, and
 *        byte for byte otherwise
 *
 * @returns 0, or the number of the check that failed
 */
int utest_veto_file_any_case(void)
{
    if (veto_file("abc/T017_Veto/", "t017_veto", 1) != 1) {
        return 1;
    }

    if (veto_file("abc/T017_Veto/", "t017_veto", 0) != 0) {
        return 2;
    }

    if (veto_file("abc/T017_Veto/", "T017_Veto", 0) != 1) {
        return 3;
    }

    return 0;
}

/*!
 * @brief Listings and requests leave out or refuse a vetoed or reserved
 *        name in another case only with the volume's case folding on
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes the test's folder
 * @returns 0, 1 when a listing or a request got the wrong answer, -1 when
 *          the setup failed
 */
int test018_listing_vetoes_any_case(AFPObj *obj, struct vol *vol)
{
    static char dname[] = "t018_dir";
    static char fname[] = "T018_Secret";
    static char veto[] = "t018_secret/";
    char *saved_veto = vol->v_veto;
    int saved_nocase = vol->v_ignorecase;
    struct dir *saved_curdir = curdir;
    cnid_t did;
    int ret = -1;

    if (createdir(obj, vol->v_vid, DIRDID_ROOT, dname) != AFP_OK) {
        return -1;
    }

    /* afp_createdir() leaves curdir in the directory it made */
    did = curdir->d_did;

    if (createfile(obj, vol->v_vid, did, fname) == AFP_OK) {
        vol->v_veto = veto;
        vol->v_ignorecase = 0;
        /* byte for byte the entry and .AppleDB match nothing here */
        ret = enumerate_range(obj, vol->v_vid, did, 1 << FILPBIT_FNUM, 0, 1, 1)
              == AFP_OK
              && getfiledirparms(obj, vol->v_vid, did, fname) == AFP_OK
              && getfiledirparms(obj, vol->v_vid, did, ".appledb")
              == AFPERR_NOOBJ ? 0 : 1;
        vol->v_ignorecase = 1;

        /* folded, both are refused and the listing is empty */
        if (ret == 0 && (enumerate_range(obj, vol->v_vid, did,
                                         1 << FILPBIT_FNUM, 0, 1, 1)
                         != AFPERR_NOOBJ
                         || getfiledirparms(obj, vol->v_vid, did, fname)
                         != AFPERR_PARAM
                         || getfiledirparms(obj, vol->v_vid, did, ".appledb")
                         != AFPERR_PARAM)) {
            ret = 1;
        }

        vol->v_veto = saved_veto;
        vol->v_ignorecase = saved_nocase;
        delete (obj, vol->v_vid, did, fname);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, dname);
    curdir = saved_curdir;
    return ret;
}

/*!
 * @brief Whether readdir() lists one of the probe test's folders before its
 *        file
 *
 * @param[in] dir    the test's folder
 * @param[in] fname  the file's name
 * @returns true when a T019_Sub folder comes first
 */
static bool probe_folder_first(const char *dir, const char *fname)
{
    const struct dirent *de;
    bool first = false;
    DIR *dp;

    if ((dp = opendir(dir)) == NULL) {
        return false;
    }

    while ((de = readdir(dp)) != NULL && strcmp(de->d_name, fname) != 0) {
        first = first || strncmp(de->d_name, "T019_Sub", 8) == 0;
    }

    closedir(dp);
    return first;
}

/*!
 * @brief path_probe_case() passes over folders and answers for a file as a
 *        lookup of its name in another case does, as path_ignores_case()
 *        does
 *
 * @param[in] vol  volume whose root takes the test's folder
 * @returns 0; 1 or 2 when the empty folder or the folder of folders gave an
 *          answer other than undecided; 3 or 4 when the probe or
 *          path_ignores_case() disagreed with the lookup; -1 when the setup
 *          failed
 */
int utest_path_probe_case_agrees(struct vol *vol)
{
    static char fname[] = "T019_Name";
    static char lname[] = "t019_name";
    char dir[MAXPATHLEN + 1];
    char sname[16];
    struct stat st;
    struct stat lst;
    int ignores;
    int subs = 0;
    int dfd;
    int fd;
    int ret = -1;
    snprintf(dir, sizeof(dir), "%s/t019_probe", vol->v_path);

    if (mkdir(dir, 0700) != 0) {
        return -1;
    }

    if ((dfd = open(dir, O_RDONLY | O_DIRECTORY)) < 0) {
        rmdir(dir);
        return -1;
    }

    /* an empty folder or a folder alone gives the probe no name */
    if (path_probe_case(dir) != -1) {
        ret = 1;
        goto cleanup;
    }

    if (mkdirat(dfd, "T019_Sub0", 0700) != 0) {
        goto cleanup;
    }

    subs = 1;

    if (path_probe_case(dir) != -1) {
        ret = 2;
        goto cleanup;
    }

    if ((fd = openat(dfd, fname, O_WRONLY | O_CREAT | O_EXCL, 0600)) < 0) {
        goto cleanup;
    }

    close(fd);

    /* a folder listed before the file makes the probe pass one over */
    while (subs < 16 && !probe_folder_first(dir, fname)) {
        snprintf(sname, sizeof(sname), "T019_Sub%d", subs);

        if (mkdirat(dfd, sname, 0700) != 0) {
            break;
        }

        subs++;
    }

    /* a filesystem that ignores case finds the file in another case */
    ignores = fstatat(dfd, fname, &st, 0) == 0
              && fstatat(dfd, lname, &lst, 0) == 0
              && st.st_ino == lst.st_ino;

    if (path_probe_case(dir) != ignores) {
        ret = 3;
    } else if (path_ignores_case(dir) != ignores) {
        ret = 4;
    } else {
        ret = 0;
    }

    unlinkat(dfd, fname, 0);
cleanup:

    while (subs-- > 0) {
        snprintf(sname, sizeof(sname), "T019_Sub%d", subs);
        unlinkat(dfd, sname, AT_REMOVEDIR);
    }

    close(dfd);
    rmdir(dir);
    return ret;
}

/*!
 * @brief path_probe_case() takes no file with a second link for its probe,
 *        and tells two files apart
 *
 * @param[in] vol  volume whose root takes the test's folder
 * @returns 0, 1 when the probe took the linked file or answered 1, -1 when
 *          the setup failed, TEST_SKIP where the filesystem refuses the
 *          second name
 */
int utest_path_probe_case_skips_links(struct vol *vol)
{
    static char fname[] = "Ab";
    static char lname[] = "ab";
    char dir[MAXPATHLEN + 1];
    int dfd;
    int fd;
    int ret = -1;
    snprintf(dir, sizeof(dir), "%s/t019_links", vol->v_path);

    if (mkdir(dir, 0700) != 0) {
        return -1;
    }

    if ((dfd = open(dir, O_RDONLY | O_DIRECTORY)) < 0) {
        rmdir(dir);
        return -1;
    }

    if ((fd = openat(dfd, fname, O_WRONLY | O_CREAT | O_EXCL, 0600)) >= 0) {
        close(fd);

        /* a filesystem that ignores case holds the second name already */
        if (linkat(dfd, fname, dfd, lname, 0) != 0) {
            ret = errno == EEXIST ? TEST_SKIP : -1;
        } else {
            ret = path_probe_case(dir) == -1 ? 0 : 1;
            unlinkat(dfd, lname, 0);
        }

        /* "ab" a file of its own: the swapped name finds another object */
        if (ret == 0) {
            if ((fd = openat(dfd, lname, O_WRONLY | O_CREAT | O_EXCL,
                             0600)) < 0) {
                ret = -1;
            } else {
                close(fd);
                ret = path_probe_case(dir) == 0 ? 0 : 1;
                unlinkat(dfd, lname, 0);
            }
        }

        unlinkat(dfd, fname, 0);
    }

    close(dfd);
    rmdir(dir);
    return ret;
}

/*!
 * @brief A request under DID 1 for the volume's name reaches the volume
 *        though a veto entry matches that name
 *
 * @param[in] obj  session object
 * @param[in] vol  the volume, whose veto list the test sets
 * @returns 0, 1 when the request was refused
 */
int test019_cname_volume_vetoed(AFPObj *obj, struct vol *vol)
{
    char veto[MAXPATHLEN + 2];
    char *saved_veto = vol->v_veto;
    struct dir *saved_curdir = curdir;
    int ret;
    snprintf(veto, sizeof(veto), "%s/", cfrombstr(vol->v_root->d_m_name));
    vol->v_veto = veto;
    ret = getfiledirparms(obj, vol->v_vid, DIRDID_ROOT_PARENT,
                          cfrombstr(vol->v_root->d_m_name)) == AFP_OK ? 0 : 1;
    vol->v_veto = saved_veto;
    curdir = saved_curdir;
    return ret;
}

/*!
 * @brief veto_file() takes the list's end as its last entry's slash, and
 *        reads nothing after it
 *
 * @returns 0, or the number of the check that failed
 */
int utest_veto_file_list_end(void)
{
    /* a list whose last entry lacks its slash, a name after its end */
    static const char list[] = "abc\0zzz/";

    if (veto_file(list, "zzz", 0) != 0) {
        return 1;
    }

    if (veto_file(list, "ZZZ", 1) != 0) {
        return 2;
    }

    if (veto_file(list, "abc", 0) != 1) {
        return 3;
    }

    if (veto_file(list, "ABC", 1) != 1) {
        return 4;
    }

    if (veto_file(list, "abcd", 0) != 0) {
        return 5;
    }

    if (veto_file(list, "ab", 0) != 0) {
        return 6;
    }

    return 0;
}

static char *(*cnid_real_resolve)(struct _cnid_db *, cnid_t *, void *,
                                  size_t);
static cnid_t (*cnid_real_add)(struct _cnid_db *, const struct stat *, cnid_t,
                               const char *, size_t, cnid_t);
static int cnid_resolves;
static int cnid_adds;
/* the folder whose registrations count, CNID_INVALID for every folder */
static cnid_t cnid_add_did;

/*!
 * @brief cnid_resolve() of the watched backend, counted
 *
 * @param[in]     cdb     the watched database
 * @param[in,out] id      the CNID to resolve, receives its parent's
 * @param[out]    buffer  receives the name
 * @param[in]     len     size of buffer
 * @returns what the backend's cnid_resolve() returns
 */
static char *cnid_count_resolve(struct _cnid_db *cdb, cnid_t *id,
                                void *buffer, size_t len)
{
    cnid_resolves++;
    return cnid_real_resolve(cdb, id, buffer, len);
}

/*!
 * @brief cnid_add() of the watched backend, counted in cnid_add_did's folder
 *
 * @param[in] cdb   the watched database
 * @param[in] st    the object's stat
 * @param[in] did   its parent's DID
 * @param[in] name  its unix name
 * @param[in] len   length of name
 * @param[in] hint  the CNID its metadata holds, or CNID_INVALID
 * @returns what the backend's cnid_add() returns
 */
static cnid_t cnid_count_add(struct _cnid_db *cdb, const struct stat *st,
                             cnid_t did, const char *name, size_t len,
                             cnid_t hint)
{
    if (cnid_add_did == CNID_INVALID || did == cnid_add_did) {
        cnid_adds++;
    }

    return cnid_real_add(cdb, st, did, name, len, hint);
}

/*!
 * @brief Count the backend's resolve and add calls until cnid_unwatch()
 *
 * @param[in,out] cdb  database whose cnid_resolve() and cnid_add() are wrapped
 */
static void cnid_watch(struct _cnid_db *cdb)
{
    cnid_real_resolve = cdb->cnid_resolve;
    cnid_real_add = cdb->cnid_add;
    cdb->cnid_resolve = cnid_count_resolve;
    cdb->cnid_add = cnid_count_add;
    cnid_resolves = 0;
    cnid_adds = 0;
}

/*!
 * @brief Give the watched backend its own cnid_resolve() and cnid_add()
 *
 * @param[in,out] cdb  the watched database
 */
static void cnid_unwatch(struct _cnid_db *cdb)
{
    cdb->cnid_resolve = cnid_real_resolve;
    cdb->cnid_add = cnid_real_add;
}

/*!
 * @brief Put files on disk in a folder of the volume, behind the server's back
 *
 * @param[in] vol     volume
 * @param[in] folder  the folder's unix name under the root
 * @param[in] names   the files' unix names
 * @param[in] count   how many names
 * @returns 0, -1 when a file cannot be made
 */
static int disk_files(const struct vol *vol, const char *folder,
                      char *const names[], size_t count)
{
    char path[MAXPATHLEN + 1];

    for (size_t i = 0; i < count; i++) {
        snprintf(path, sizeof(path), "%s/%s/%s", vol->v_path, folder,
                 names[i]);

        if (create_disk_file(path) != 0) {
            return -1;
        }
    }

    return 0;
}

/*!
 * @brief FPCreateDir of a folder in the root, whose DID curdir then holds
 *
 * @param[in] obj   the test session
 * @param[in] vid   the open volume
 * @param[in] name  the folder's name
 * @returns the folder's DID, CNID_INVALID when the create fails
 */
static cnid_t make_folder(AFPObj *obj, uint16_t vid, const char *name)
{
    if (createdir(obj, vid, DIRDID_ROOT, name) != 0) {
        return CNID_INVALID;
    }

    return curdir->d_did;
}

/*!
 * @brief The long name an AFP 2 session lists for a name it cannot hold whole
 *
 * mangle() keeps the first MACFILELEN - strlen("#ID") - strlen(ext) bytes of
 * the Mac name, then "#ID" in uppercase hex and the extension.
 *
 * @param[out] lname  receives the name, MACFILELEN + 1 bytes
 * @param[in]  mac    the Mac name, or the part of it the charset holds
 * @param[in]  ext    the extension kept, "" for none
 * @param[in]  id     the object's CNID
 */
static void cut_lname(char *lname, const char *mac, const char *ext, cnid_t id)
{
    /* a CNID has at most eight hex digits */
    char suffix[sizeof("#FFFFFFFF")];
    int k = snprintf(suffix, sizeof(suffix), "#%X", ntohl(id));
    snprintf(lname, MACFILELEN + 1, "%.*s%s%s",
             (int)(MACFILELEN - k - strlen(ext)), mac, suffix, ext);
}

/*!
 * @brief FPEnumerate lists each entry under its own names and resolves no id
 *
 * The mangled-looking name carries the sibling's CNID, so demangling it
 * reaches the sibling, through the CNID database while the sibling has no
 * entry. The folder's name and the fourth, whose U+00A9 is one Mac byte and
 * two UTF-8 ones, overflow a long name, and the fifth, 100 decomposed
 * accents, overflows both, so all three are mangled. Files put on disk have
 * no entry, so the listing converts their names afresh. Asked for alone, the
 * mangled-looking file's long name costs only its path's one resolve.
 *
 * @returns 0, 1 when a name or the lookup count is wrong or the setup fails
 */
int utest_enumerate_lname_is_the_files_own(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    /* a spare record shows a name listed twice */
    enum { LISTED = 5, ROOM = LISTED + 1 };
    char sibling[] = "sibling.txt";
    char folder[] = "a folder whose name no long name holds";
    char longname[] = "a file whose name no long name holds \xc2\xa9.txt";
    char hugename[2 * 100 + sizeof(".txt")];
    char mangled[32] = "";
    char *names[LISTED] = { sibling, folder, mangled, longname, hugename };
    /* the names in the Mac charset, U+00A9 as 0xA9 and U+00E9 as 0x8E, and
     * which of them a long name cannot hold whole */
    char maclong[] = "a file whose name no long name holds \xa9";
    char machuge[100 + 1];
    const char *macs[LISTED] = { sibling, folder, mangled, maclong, machuge };
    const char *exts[LISTED] = { "", "", "", ".txt", ".txt" };
    const bool cut[LISTED] = { false, true, false, true, true };
    char lnames[ROOM][MACFILELEN + 1];
    char unames[ROOM][UTF8FILELEN_EARLY + 1];
    char want_l[MACFILELEN + 1];
    char want_u[UTF8FILELEN_EARLY + 1];
    cnid_t did;
    cnid_t id;
    int count;
    int err;
    int own = 0;
    int result = 1;

    for (int i = 0; i < 100; i++) {
        memcpy(hugename + 2 * i, "\xc3\xa9", 2);
    }

    memcpy(hugename + 200, ".txt", sizeof(".txt"));
    memset(machuge, 0x8e, 100);
    machuge[100] = '\0';

    if ((did = make_folder(obj, vid, "ln")) == CNID_INVALID) {
        return 1;
    }

    if (createfile(obj, vid, did, sibling) != 0
            || createdir(obj, vid, did, folder) != 0) {
        goto out;
    }

    id = cnid_get(vol->v_cdb, did, sibling, strlen(sibling));

    if (id == CNID_INVALID) {
        goto out;
    }

    /* demangling must reach the CNID database, as for a file the session
     * has not met */
    drop_entry(vol, did, sibling);
    snprintf(mangled, sizeof(mangled), "dm #%X.txt", ntohl(id));

    /* past the sibling and the folder, which AFP requests made */
    if (disk_files(vol, "ln", names + 2, LISTED - 2) != 0) {
        goto out;
    }

    cnid_watch(vol->v_cdb);
    count = enumerate_names(obj, vid, did, lnames, unames, ROOM);
    cnid_unwatch(vol->v_cdb);

    for (size_t i = 0; i < LISTED; i++) {
        int found = 0;
        id = cnid_get(vol->v_cdb, did, names[i], strlen(names[i]));

        if (id == CNID_INVALID) {
            goto out;
        }

        if (cut[i]) {
            cut_lname(want_l, macs[i], exts[i], id);
        } else {
            snprintf(want_l, sizeof(want_l), "%s", macs[i]);
        }

        snprintf(want_u, sizeof(want_u), "%s", utompath(vol, names[i], id, 1));

        for (int r = 0; r < count; r++) {
            found += (strcmp(lnames[r], want_l) == 0
                      && strcmp(unames[r], want_u) == 0);
        }

        own += found == 1;
    }

    if (count != LISTED || own != LISTED || cnid_resolves != 0) {
        fprintf(stderr, "enumerate: %d names, %d own, %d lookups\n", count,
                own, cnid_resolves);
        goto out;
    }

    /* the listing may have cached the sibling again */
    drop_entry(vol, did, sibling);
    cnid_watch(vol->v_cdb);
    err = getfiledirparms_lname(obj, vid, did, mangled, want_l);
    cnid_unwatch(vol->v_cdb);

    if (err != AFP_OK || cnid_resolves != 1) {
        fprintf(stderr, "enumerate: '%s' alone: error %d, %d lookups\n",
                mangled, err, cnid_resolves);
        goto out;
    }

    result = 0;
out:

    for (size_t i = 0; i < LISTED; i++) {
        if (*names[i] != '\0') {
            delete (obj, vid, did, names[i]);
        }
    }

    delete (obj, vid, DIRDID_ROOT, "ln");
    return result;
}

/*!
 * @brief The volume root's long name is the volume's Mac name
 *
 * A UTF-8 session gets the Mac name FPGetVolParms sends, which creatvol()
 * cuts to 27 characters plus '#' and the volume id; an AFP 2 session gets the
 * root entry's Mac name, which a lookup in DID 1 matches. The test sets a Mac
 * name that neither the UTF-8 volume name nor the root entry holds, as a
 * volume Mac charset other than the server's can.
 *
 * @returns 0, 1 when a request fails or a name differs
 */
int utest_root_lname_is_the_volume_name(AFPObj *obj, struct vol *vol,
                                        uint16_t vid)
{
    ucs2_t macname[] = { 'M', 'a', 'c', ' ', 'n', 'a', 'm', 'e', 0 };
    ucs2_t *saved = vol->v_macname;
    int version = obj->afp_version;
    char lname[MACFILELEN + 1] = "";
    char afp2[MACFILELEN + 1] = "";
    int err = getfiledirparms_lname(obj, vid, DIRDID_ROOT, "", lname);
    int afp2_err;

    if (err != AFP_OK || strcmp(lname, "afpd_test") != 0) {
        fprintf(stderr, "root: error %d, long name '%s'\n", err, lname);
        return 1;
    }

    vol->v_macname = macname;
    err = getfiledirparms_lname(obj, vid, DIRDID_ROOT, "", lname);
    obj->afp_version = 21;
    afp2_err = getfiledirparms_lname(obj, vid, DIRDID_ROOT, "", afp2);

    if (afp2_err == AFP_OK) {
        afp2_err = getfiledirparms(obj, vid, DIRDID_ROOT_PARENT, afp2);
    }

    obj->afp_version = version;
    vol->v_macname = saved;

    if (err != AFP_OK || strcmp(lname, "Mac name") != 0
            || afp2_err != AFP_OK) {
        fprintf(stderr,
                "root: Mac name: error %d, '%s'; AFP 2.1: error %d, '%s'\n",
                err, lname, afp2_err, afp2);
        return 1;
    }

    return 0;
}

/*!
 * @brief A Mac name the server's Mac charset cannot hold fails FPGetVolParms
 *
 * FPGetVolParms has no name to send and fails; the root's long name falls
 * back to its directory's. creatvol() never builds such a name, so the test
 * sets one with U+0416, which MAC_ROMAN, the test volume's charset, lacks.
 *
 * @returns 0, 1 when FPGetVolParms answers or the root's long name is not its
 *          directory's
 */
int utest_root_lname_beyond_the_mac_charset(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    ucs2_t macname[] = { 0x0416, 'x', 0 };
    ucs2_t *saved = vol->v_macname;
    char lname[MACFILELEN + 1] = "";
    int volerr;
    int err;
    vol->v_macname = macname;
    volerr = getvolparams(obj, vid, 1 << VOLPBIT_NAME);
    err = getfiledirparms_lname(obj, vid, DIRDID_ROOT, "", lname);
    vol->v_macname = saved;

    if (volerr != AFPERR_MISC || err != AFP_OK
            || strcmp(lname, utompath(vol, cfrombstr(vol->v_root->d_u_name),
                                      DIRDID_ROOT, 0)) != 0) {
        fprintf(stderr, "root: FPGetVolParms %d, long name error %d '%s'\n",
                volerr, err, lname);
        return 1;
    }

    return 0;
}

/*!
 * @brief An AFP 2.1 listing holds each name's Mac charset form
 *
 * The forms are written out: U+00E9 and U+00A9 as Mac Roman 0x8E and 0xA9,
 * a name too long for a long name cut around "#ID", and a name with no Mac
 * byte as "???#ID" plus its extension. Files put on disk have no entry.
 *
 * @returns 0, 1 when a name is missing or the setup fails
 */
int utest_enumerate_afp2_lists_the_mac_forms(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    enum { LISTED = 4, ROOM = LISTED + 1 };
    char accent[] = "caf\xc3\xa9.txt";
    char sign[] = "copyright \xc2\xa9.txt";
    char ascii[] = "a long ascii name for an afp two session.txt";
    char kanji[] = "\xe5\x86\x99\xe7\x9c\x9f.jpg";
    char *names[LISTED] = { accent, sign, ascii, kanji };
    const char *macs[LISTED] = {
        "caf\x8e.txt",
        "copyright \xa9.txt",
        ascii,
        "???",
    };
    const char *exts[LISTED] = { "", "", ".txt", ".jpg" };
    const bool cut[LISTED] = { false, false, true, true };
    char lnames[ROOM][MACFILELEN + 1];
    char want[MACFILELEN + 1];
    int version = obj->afp_version;
    cnid_t did;
    int count;
    int own = 0;
    int result = 1;

    if ((did = make_folder(obj, vid, "la")) == CNID_INVALID) {
        return 1;
    }

    if (disk_files(vol, "la", names, LISTED) != 0) {
        goto out;
    }

    obj->afp_version = 21;
    count = enumerate_lnames(obj, vid, did, lnames, ROOM);
    obj->afp_version = version;

    for (size_t i = 0; i < LISTED; i++) {
        int found = 0;

        if (cut[i]) {
            cnid_t id = cnid_get(vol->v_cdb, did, names[i], strlen(names[i]));

            if (id == CNID_INVALID) {
                goto out;
            }

            cut_lname(want, macs[i], exts[i], id);
        } else {
            snprintf(want, sizeof(want), "%s", macs[i]);
        }

        for (int r = 0; r < count; r++) {
            found += strcmp(lnames[r], want) == 0;
        }

        own += found == 1;
    }

    if (count != LISTED || own != LISTED) {
        fprintf(stderr, "enumerate: AFP 2.1: %d names, %d in their Mac forms\n",
                count, own);
        goto out;
    }

    result = 0;
out:

    for (size_t i = 0; i < LISTED; i++) {
        delete (obj, vid, did, names[i]);
    }

    delete (obj, vid, DIRDID_ROOT, "la");
    return result;
}

/*!
 * @brief A decomposed unix name lists under a mangled long name
 *
 * A writer outside AFP can leave "café" decomposed on a UTF8 volume, where
 * netatalk writes it precomposed. The combining accent has no Mac charset
 * byte, so the long name is cut to "cafe#ID.txt" in both sessions, and
 * FPGetFileDirParms by it reaches the file in both. The UTF-8 name is the
 * name as it lies.
 *
 * @returns 0, 1 when a name or a lookup is wrong or the setup fails
 */
int utest_enumerate_decomposed_name_is_mangled(AFPObj *obj, struct vol *vol,
                                               uint16_t vid)
{
    char name[] = "cafe\xcc\x81.txt";
    char *names[] = { name };
    char path[MAXPATHLEN + 1];
    char lnames[2][MACFILELEN + 1] = { "" };
    char unames[2][UTF8FILELEN_EARLY + 1] = { "" };
    char afp2[2][MACFILELEN + 1] = { "" };
    char want[MACFILELEN + 1];
    int version = obj->afp_version;
    cnid_t did;
    cnid_t id = CNID_INVALID;
    int count;
    int afp2_count;
    int err;
    int afp2_err;
    int result = 1;

    if ((did = make_folder(obj, vid, "dn")) == CNID_INVALID) {
        return 1;
    }

    snprintf(path, sizeof(path), "%s/dn/%s", vol->v_path, name);

    if (disk_files(vol, "dn", names, 1) != 0) {
        goto out;
    }

    count = enumerate_names(obj, vid, did, lnames, unames, 2);
    id = cnid_get(vol->v_cdb, did, name, strlen(name));

    if (id == CNID_INVALID) {
        fprintf(stderr, "enumerate: decomposed: %d names, '%s', no CNID\n",
                count, lnames[0]);
        goto out;
    }

    cut_lname(want, "cafe", ".txt", id);
    err = getfiledirparms(obj, vid, did, want);
    /* an entry holds the Mac name of the session that cached it, and a
     * process serves one session */
    drop_entry(vol, did, name);
    obj->afp_version = 21;
    afp2_count = enumerate_lnames(obj, vid, did, afp2, 2);
    afp2_err = getfiledirparms(obj, vid, did, want);
    obj->afp_version = version;

    if (count != 1 || strcmp(lnames[0], want) != 0
            || strcmp(unames[0], name) != 0 || err != AFP_OK
            || afp2_count != 1 || strcmp(afp2[0], want) != 0
            || afp2_err != AFP_OK) {
        fprintf(stderr,
                "enumerate: decomposed: %d names, '%s' and '%s', error %d; "
                "AFP 2.1: %d names, '%s', error %d; want '%s'\n",
                count, lnames[0], unames[0], err, afp2_count, afp2[0],
                afp2_err, want);
        goto out;
    }

    result = 0;
out:

    if (id != CNID_INVALID) {
        delete (obj, vid, did, want);
    } else {
        unlink(path);
    }

    delete (obj, vid, DIRDID_ROOT, "dn");
    return result;
}

/*!
 * @brief The long name an AFP 2 session lists for a file
 *
 * @param[in]  vol      volume
 * @param[in]  did      the file's folder
 * @param[in]  name     the file's unix name
 * @param[out] mangled  receives the long name, MACFILELEN + 1 bytes
 * @returns the file's CNID, CNID_INVALID when the file has none
 */
static cnid_t long_name(const struct vol *vol, cnid_t did, char *name,
                        char *mangled)
{
    cnid_t id = cnid_get(vol->v_cdb, did, name, strlen(name));

    if (id != CNID_INVALID) {
        snprintf(mangled, MACFILELEN + 1, "%s", utompath(vol, name, id, 0));
    }

    return id;
}

/*!
 * @brief Create a file through AFP and give its AFP 2 long name
 *
 * @param[in]  obj      the test session
 * @param[in]  vol      volume
 * @param[in]  vid      the open volume
 * @param[in]  did      the file's folder
 * @param[in]  name     the file's name
 * @param[out] mangled  receives the long name, MACFILELEN + 1 bytes
 * @returns the file's CNID, CNID_INVALID when the create or its lookup fails
 */
static cnid_t mangled_file(AFPObj *obj, const struct vol *vol, uint16_t vid,
                           cnid_t did, char *name, char *mangled)
{
    if (createfile(obj, vid, did, name) != 0) {
        return CNID_INVALID;
    }

    return long_name(vol, did, name, mangled);
}

/*!
 * @brief One FPGetFileDirParms of a name in a folder, counting CNID resolves
 *
 * @param[in] obj   the test session
 * @param[in] vol   volume
 * @param[in] vid   the open volume
 * @param[in] did   the folder
 * @param[in] name  the name asked for
 * @returns the request's result; cnid_resolves holds the count
 */
static int watched_getfiledirparms(AFPObj *obj, const struct vol *vol,
                                   uint16_t vid, cnid_t did, const char *name)
{
    int err;
    cnid_watch(vol->v_cdb);
    err = getfiledirparms(obj, vid, did, name);
    cnid_unwatch(vol->v_cdb);
    return err;
}

/*!
 * @brief CNID resolves made by one FPGetFileDirParms of a name at the root
 *
 * @param[in] obj   the test session
 * @param[in] vol   volume
 * @param[in] vid   the open volume
 * @param[in] name  the name asked for, which no file holds
 * @returns the count, -1 when the request fails other than with
 *          AFPERR_NOOBJ
 */
static int demangle_resolve_count(AFPObj *obj, const struct vol *vol,
                                  uint16_t vid, const char *name)
{
    int err = watched_getfiledirparms(obj, vol, vid, DIRDID_ROOT, name);

    if (err != AFPERR_NOOBJ) {
        fprintf(stderr, "demangle: '%s': error %d\n", name, err);
        return -1;
    }

    return cnid_resolves;
}

/*!
 * @brief An unresolvable mangled-looking name costs one CNID lookup
 *
 * A plain name, or one with more hex digits than a CNID holds, costs none.
 * 0xFFFFF0 parses as mangled but names no object. "dm #FFFFFFF0.text" puts
 * the '#' as far from the end as a mangled name can, eight digits and a
 * five-byte extension; nine digits would wrap to 0xFFFFF0 in 32 bits.
 *
 * @returns 0, 1 when a lookup count is wrong or a request fails
 */
int utest_demangle_unknown_id_resolves_once(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    int plain = demangle_resolve_count(obj, vol, vid, "dm plain.txt");
    int hashed = demangle_resolve_count(obj, vol, vid, "dm #FFFFF0.txt");
    int eight = demangle_resolve_count(obj, vol, vid, "dm #FFFFFFF0.text");
    int wrapped = demangle_resolve_count(obj, vol, vid, "dm #100FFFFF0.txt");

    if (plain < 0 || hashed < 0 || eight < 0 || wrapped < 0) {
        return 1;
    }

    if (plain != 0 || hashed - plain != 1 || eight - plain != 1
            || wrapped != plain) {
        fprintf(stderr,
                "demangle: %d resolves, %d eight, %d wrapped, %d plain\n",
                hashed, eight, wrapped, plain);
        return 1;
    }

    return 0;
}

/*!
 * @brief A cached file answers its mangled name until its entry goes stale
 *
 * A 31-byte name is the form demangle_checks() matches by prefix. Sent at
 * the root, which holds a same-named file, it must find nothing, and an entry
 * whose ctime differs from the disk's costs one CNID lookup. Another
 * session's rename reaches the disk and the row at once but this session's
 * entry only with the cache hint, so the test renames on disk and in the row
 * and puts a new file at the old name, which the stale entry would reach.
 *
 * @returns 0, 1 when a lookup count or a result is wrong or the setup fails
 */
int utest_demangle_cached_file_checks_its_entry(AFPObj *obj,
                                                struct vol *vol,
                                                uint16_t vid)
{
    char name[] = "a cached file with a long name.txt";
    char moved[] = "moved.txt";
    char mangled[MACFILELEN + 1];
    char from[MAXPATHLEN + 1];
    char to[MAXPATHLEN + 1];
    char at_root[MAXPATHLEN + 1];
    const char *answer;
    struct stat st;
    const struct dir *parent;
    struct dir *entry;
    cnid_t did;
    cnid_t id;
    cnid_t fileid;
    int err;
    int result = 1;

    if ((did = make_folder(obj, vid, "dc")) == CNID_INVALID) {
        return 1;
    }

    snprintf(from, sizeof(from), "%s/dc/%s", vol->v_path, name);
    snprintf(to, sizeof(to), "%s/dc/%s", vol->v_path, moved);
    snprintf(at_root, sizeof(at_root), "%s/%s", vol->v_path, name);
    id = mangled_file(obj, vol, vid, did, name, mangled);
    parent = dirlookup(vol, did);

    /* the create cached the file with the stat it took before writing the
     * metadata, which the dircache's own validation would refresh */
    if (id == CNID_INVALID || parent == NULL
            || (entry = dircache_search_by_name(vol, parent, name,
                                                strlen(name))) == NULL
            || pfd_ostat(vol, entry, &st, vol_syml_opt(vol)) != 0) {
        goto out;
    }

    dir_stat_refresh(vol, entry, &st);
    err = watched_getfiledirparms(obj, vol, vid, did, mangled);

    if (err != AFP_OK || cnid_resolves != 0) {
        fprintf(stderr, "demangle: cached: error %d, %d resolves\n", err,
                cnid_resolves);
        goto out;
    }

    /* a caller that renames the object frees the entry's name */
    answer = demangle_osx(vol, mangled, did, &fileid);
    entry = dircache_search_by_name(vol, parent, name, strlen(name));

    if (entry == NULL || strcmp(answer, name) != 0
            || answer == cfrombstr(entry->d_u_name)) {
        fprintf(stderr, "demangle: cached: the answer is the entry's\n");
        goto out;
    }

    if (create_disk_file(at_root) != 0) {
        goto out;
    }

    err = getfiledirparms(obj, vid, DIRDID_ROOT, mangled);

    if (err != AFPERR_NOOBJ) {
        fprintf(stderr, "demangle: root: error %d\n", err);
        goto out;
    }

    if ((entry = dircache_search_by_name(vol, parent, name,
                                         strlen(name))) == NULL) {
        goto out;
    }

    entry->dcache_ctime--;
    err = watched_getfiledirparms(obj, vol, vid, did, mangled);

    if (err != AFP_OK || cnid_resolves != 1) {
        fprintf(stderr, "demangle: ctime: error %d, %d resolves\n", err,
                cnid_resolves);
        goto out;
    }

    /* the rename keeps the inode the row records */
    if (rename(from, to) != 0
            || cnid_update(vol->v_cdb, id, &st, did, moved, strlen(moved)) != 0
            || create_disk_file(from) != 0) {
        goto out;
    }

    err = watched_getfiledirparms(obj, vol, vid, did, mangled);

    if (err != AFPERR_NOOBJ || cnid_resolves != 1) {
        fprintf(stderr, "demangle: stale: error %d, %d resolves\n", err,
                cnid_resolves);
        goto out;
    }

    result = 0;
out:
    /* the cache hint that would retire the stale entry never comes */
    drop_entry(vol, did, name);
    delete (obj, vid, did, moved);
    delete (obj, vid, did, name);
    delete (obj, vid, DIRDID_ROOT, "dc");
    delete (obj, vid, DIRDID_ROOT, name);
    return result;
}

/*!
 * @brief A cached folder answers its mangled name whatever its entry's ctime
 *
 * A folder's ctime moves with each change inside it, so its entry is
 * trusted on its inode alone.
 *
 * @returns 0, 1 when the request fails, makes a lookup or the setup fails
 */
int utest_demangle_cached_folder_ignores_ctime(AFPObj *obj, struct vol *vol,
                                               uint16_t vid)
{
    char name[] = "a cached folder with a long name";
    char mangled[MACFILELEN + 1];
    struct dir *entry;
    cnid_t did;
    int err;
    int result = 1;

    if ((did = make_folder(obj, vid, name)) == CNID_INVALID) {
        return 1;
    }

    if ((entry = dirlookup(vol, did)) == NULL) {
        goto out;
    }

    entry->dcache_ctime--;
    snprintf(mangled, sizeof(mangled), "%s",
             utompath(vol, name, entry->d_did, 0));
    err = watched_getfiledirparms(obj, vol, vid, DIRDID_ROOT, mangled);

    if (err != AFP_OK || cnid_resolves != 0) {
        fprintf(stderr, "demangle: folder: error %d, %d resolves\n", err,
                cnid_resolves);
        goto out;
    }

    result = 0;
out:
    delete (obj, vid, DIRDID_ROOT, name);
    return result;
}

/*!
 * @brief A mangled name whose kept prefix holds a '#' demangles to its file
 *
 * mangle() keeps the start of the name before "#ID", so the CNID follows
 * the last '#' of the 31-byte name.
 *
 * @returns 0, 1 when the name misses the file or the setup fails
 */
int utest_demangle_hash_in_prefix(AFPObj *obj, struct vol *vol, uint16_t vid)
{
    char name[] = "Song #2 (Remastered Version 2011).mp3";
    char mangled[MACFILELEN + 1];
    cnid_t did;
    cnid_t id;
    int err;
    int result = 1;

    if ((did = make_folder(obj, vid, "dh")) == CNID_INVALID) {
        return 1;
    }

    id = mangled_file(obj, vol, vid, did, name, mangled);

    if (id == CNID_INVALID) {
        goto out;
    }

    if ((err = getfiledirparms(obj, vid, did, mangled)) != AFP_OK) {
        fprintf(stderr, "demangle: '%s': error %d\n", mangled, err);
        goto out;
    }

    result = 0;
out:
    delete (obj, vid, did, name);
    delete (obj, vid, DIRDID_ROOT, "dh");
    return result;
}

/*!
 * @brief A mangled name whose extension holds an id demangles to its file
 *
 * mangle() keeps the name's extension after "#ID", so the 31-byte name ends
 * in ".#11", which reads as CNID 17 too. The name goes back as AFP 3, through
 * demangle_checks(), and as AFP 2.1, through the exact check.
 *
 * @returns 0, 1 when the name misses the file or the setup fails
 */
int utest_demangle_hash_in_extension(AFPObj *obj, struct vol *vol,
                                     uint16_t vid)
{
    char name[] = "Quarterly results backup copy.#11";
    char mangled[MACFILELEN + 1] = "";
    int version = obj->afp_version;
    int err = AFPERR_PARAM;
    int afp2_err = AFPERR_PARAM;
    int failed;
    cnid_t did;
    size_t len;

    if ((did = make_folder(obj, vid, "dx")) == CNID_INVALID) {
        return 1;
    }

    if (mangled_file(obj, vol, vid, did, name, mangled) != CNID_INVALID
            && (len = strlen(mangled)) > 4
            && strcmp(mangled + len - 4, ".#11") == 0) {
        err = getfiledirparms(obj, vid, did, mangled);
        obj->afp_version = 21;
        afp2_err = getfiledirparms(obj, vid, did, mangled);
        obj->afp_version = version;
    }

    failed = err != AFP_OK || afp2_err != AFP_OK;

    if (failed) {
        fprintf(stderr, "demangle: '%s': error %d, as AFP 2.1 %d\n", mangled,
                err, afp2_err);
    }

    delete (obj, vid, did, name);
    delete (obj, vid, DIRDID_ROOT, "dx");
    return failed;
}

/*!
 * @brief An uncached object's mangled name reaches nothing in another folder
 *
 * The CNID resolves to the object's own folder, the only one its mangled name
 * names it in; a file of the same name in another folder stays out of reach.
 * The name goes as AFP 2.1, whose exact check would accept it anywhere.
 *
 * @returns 0, 1 when the name reaches the other folder's file or the setup
 *          fails
 */
int utest_demangle_uncached_id_keeps_its_folder(AFPObj *obj, struct vol *vol,
                                                uint16_t vid)
{
    char name[] = "a name an AFP 2 session must mangle.txt";
    char mangled[MACFILELEN + 1] = "";
    int version = obj->afp_version;
    int err = AFPERR_PARAM;
    int failed;
    cnid_t one;
    cnid_t two;

    if ((one = make_folder(obj, vid, "du")) == CNID_INVALID) {
        return 1;
    }

    two = make_folder(obj, vid, "dv");

    if (two != CNID_INVALID
            && mangled_file(obj, vol, vid, one, name, mangled) != CNID_INVALID
            && createfile(obj, vid, two, name) == 0) {
        drop_entry(vol, one, name);
        obj->afp_version = 21;
        err = getfiledirparms(obj, vid, two, mangled);
        obj->afp_version = version;
    }

    failed = err != AFPERR_NOOBJ;

    if (failed) {
        fprintf(stderr, "demangle: '%s' in another folder: error %d\n",
                mangled, err);
    }

    delete (obj, vid, one, name);
    delete (obj, vid, two, name);
    delete (obj, vid, DIRDID_ROOT, "dv");
    delete (obj, vid, DIRDID_ROOT, "du");
    return failed;
}

/*!
 * @brief Delete a file named with a sibling's CNID and check the sibling
 *
 * The file is prefix, MANGLE_CHAR, the sibling's CNID and ext, and is put on
 * disk in the folder "dp" beside the sibling.
 *
 * @param[in] obj      the test session
 * @param[in] vol      volume
 * @param[in] vid      the open volume
 * @param[in] did      the folder "dp"
 * @param[in] sibling  the sibling's name, created here
 * @param[in] prefix   the file's name before MANGLE_CHAR
 * @param[in] ext      the file's name after the CNID
 * @returns 0, 1 when the sibling goes, the file stays or the setup fails
 */
static int delete_beside(AFPObj *obj, const struct vol *vol, uint16_t vid,
                         cnid_t did, char *sibling, const char *prefix,
                         const char *ext)
{
    char literal[32] = "";
    char path[MAXPATHLEN + 1];
    struct stat st;
    cnid_t id;
    int result = 1;

    if (createfile(obj, vid, did, sibling) != 0) {
        return 1;
    }

    id = cnid_get(vol->v_cdb, did, sibling, strlen(sibling));
    snprintf(literal, sizeof(literal), "%s#%X%s", prefix, ntohl(id), ext);
    snprintf(path, sizeof(path), "%s/dp/%s", vol->v_path, literal);

    if (id == CNID_INVALID || create_disk_file(path) != 0) {
        goto out;
    }

    if (delete (obj, vid, did, literal) != AFP_OK || stat(path, &st) == 0) {
        fprintf(stderr, "demangle: '%s' stays\n", literal);
        goto out;
    }

    snprintf(path, sizeof(path), "%s/dp/%s", vol->v_path, sibling);

    if (stat(path, &st) != 0) {
        fprintf(stderr, "demangle: the sibling of '%s' went\n", literal);
        goto out;
    }

    result = 0;
out:
    delete (obj, vid, did, literal);
    delete (obj, vid, did, sibling);
    return result;
}

/*!
 * @brief A literal name takes no unconvertible sibling's place
 *
 * Each file carries the CNID of a sibling whose name the Mac Roman charset
 * cannot hold, after a prefix that is not the sibling's mangled one: none,
 * "?", "??", or one that runs past the sibling's convertible start into
 * other text.
 *
 * @returns 0, 1 when a sibling goes, a file stays or the setup fails
 */
int utest_demangle_literal_takes_no_sibling(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    /* U+5199 U+771F, which the Mac Roman charset cannot hold */
    char photo[] = "\xe5\x86\x99\xe7\x9c\x9f.jpg";
    char track[] = "Track \xe5\x86\x99.mp3";
    cnid_t did;
    int failed = 0;

    if ((did = make_folder(obj, vid, "dp")) == CNID_INVALID) {
        return 1;
    }

    failed += delete_beside(obj, vol, vid, did, photo, "", ".jpg");
    failed += delete_beside(obj, vol, vid, did, photo, "?", ".jpg");
    failed += delete_beside(obj, vol, vid, did, photo, "??", ".jpg");
    failed += delete_beside(obj, vol, vid, did, track, "Track Two", ".mp3");
    delete (obj, vid, DIRDID_ROOT, "dp");
    return failed != 0;
}

/*!
 * @brief A variant demangles on the Mac OS X pass, not on the exact one
 *
 * "Track " U+5199 ".mp3" lists to an AFP 2 session as "Track #ID.mp3". The
 * variant Mac OS X may send, with U+5199's UTF-8 bytes before the '#',
 * demangles through demangle_osx(), cname()'s AFP 3 pass, and stays literal
 * through demangle(), mtoupath()'s exact pass.
 *
 * @returns 0, 1 when a name demangles wrongly or the setup fails
 */
int utest_demangle_variant_needs_afp3(AFPObj *obj, struct vol *vol,
                                      uint16_t vid)
{
    char name[] = "Track \xe5\x86\x99.mp3";
    char mangled[MACFILELEN + 1];
    char variant[MACFILELEN + 1];
    cnid_t did;
    cnid_t id;
    cnid_t fileid;
    int version = obj->afp_version;
    int taken;
    int exact;
    int literal;
    int result = 1;

    if ((did = make_folder(obj, vid, "d2")) == CNID_INVALID) {
        return 1;
    }

    id = mangled_file(obj, vol, vid, did, name, mangled);

    if (id == CNID_INVALID) {
        goto out;
    }

    snprintf(variant, sizeof(variant), "Track \xe5\x86\x99#%X.mp3", ntohl(id));
    taken = strcmp(demangle_osx(vol, variant, did, &fileid), name) == 0;
    obj->afp_version = 21;
    exact = strcmp(demangle(vol, mangled, did), name) == 0;
    literal = demangle(vol, variant, did) == variant;
    obj->afp_version = version;

    if (!taken || !exact || !literal) {
        fprintf(stderr, "demangle: '%s' taken %d; AFP 2: '%s' %d, '%s' %d\n",
                variant, taken, mangled, exact, variant, literal);
        goto out;
    }

    result = 0;
out:
    delete (obj, vid, did, name);
    delete (obj, vid, DIRDID_ROOT, "d2");
    return result;
}

/*!
 * @brief AFP 2.1 search for files changed in 2001, CNID calls counted
 *
 * @param[in]  obj    the test session
 * @param[in]  vol    volume
 * @param[in]  vid    the open volume
 * @param[out] nrecs  receives the match count
 * @param[out] lname  receives the match's long name, MACFILELEN + 1 bytes
 * @returns the search's result
 */
static int afp2_date_search(AFPObj *obj, const struct vol *vol, uint16_t vid,
                            uint32_t *nrecs, char *lname)
{
    uint32_t from = AD_DATE_FROM_UNIX(978307200);
    uint32_t to = AD_DATE_FROM_UNIX(1009843200);
    int version = obj->afp_version;
    int ret;
    obj->afp_version = 21;
    cnid_watch(vol->v_cdb);
    ret = catsearch_ext(obj, vid, 1U << FILPBIT_MDATE, &from, &to,
                        sizeof(from), nrecs, lname);
    cnid_unwatch(vol->v_cdb);
    obj->afp_version = version;
    return ret;
}

/*!
 * @brief A search by date converts and registers no file it does not return
 *
 * Two files have 44-character names an AFP 2 session must mangle, which costs
 * a registration for their ids; the match, dated 2001-06-01, needs none.
 * Files put on disk are uncached and unregistered. The search runs as AFP
 * 2.1, since a UTF-8 session mangles only names over 255 bytes or with
 * unconvertible bytes.
 *
 * @returns 0, 1 when the reply is wrong, a file is registered or the setup
 *          fails
 */
int utest_catsearch_converts_matches_only(AFPObj *obj, struct vol *vol,
                                          uint16_t vid)
{
    char *names[] = {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.txt",
        "cs match.txt",
        "cccccccccccccccccccccccccccccccccccccccc.txt",
    };
    const struct timespec june2001[2] = { { 991353600, 0 }, { 991353600, 0 } };
    char path[MAXPATHLEN + 1];
    char lname[MACFILELEN + 1];
    uint32_t nrecs = 0;
    cnid_t did;
    int registered = 0;
    int ret;
    int result = 1;

    if ((did = make_folder(obj, vid, "cs")) == CNID_INVALID) {
        return 1;
    }

    snprintf(path, sizeof(path), "%s/cs/%s", vol->v_path, names[1]);

    if (disk_files(vol, "cs", names, sizeof(names) / sizeof(names[0])) != 0
            || utimensat(AT_FDCWD, path, june2001, 0) != 0) {
        goto out;
    }

    ret = afp2_date_search(obj, vol, vid, &nrecs, lname);

    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        registered += (cnid_get(vol->v_cdb, did, names[i], strlen(names[i]))
                       != CNID_INVALID);
    }

    if (ret != AFPERR_EOF || nrecs != 1 || strcmp(lname, names[1]) != 0
            || registered != 0) {
        fprintf(stderr,
                "catsearch: result %d, %u matches, '%s', %d registered\n",
                ret, nrecs, lname, registered);
        goto out;
    }

    result = 0;
out:

    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        delete (obj, vid, did, names[i]);
    }

    delete (obj, vid, DIRDID_ROOT, "cs");
    return result;
}

/*!
 * @brief A search by long name matches a name with an accented letter
 *
 * The criterion arrives in the Mac charset, and a candidate's name is UTF-8
 * in an AFP 3 session and Mac Roman in an AFP 2 one; Mac Roman 0x8E is
 * U+00E9. The folder also holds "cafe.txt", which the search must pass over.
 *
 * @returns 0, 1 when a search misses the file or the setup fails
 */
int utest_catsearch_lname_matches_accents(AFPObj *obj, struct vol *vol,
                                          uint16_t vid)
{
    static const char want[] = "caf\x8e.txt";
    char *names[] = { "caf\xc3\xa9.txt", "cafe.txt" };
    unsigned char spec[3 + sizeof(want) - 1] = { 0, 2, sizeof(want) - 1 };
    const int versions[] = { obj->afp_version, 21 };
    char lname[MACFILELEN + 1];
    uint32_t nrecs = 0;
    cnid_t did;
    int ret;
    int result = 1;
    memcpy(spec + 3, want, sizeof(want) - 1);

    if ((did = make_folder(obj, vid, "cl")) == CNID_INVALID) {
        return 1;
    }

    if (disk_files(vol, "cl", names, sizeof(names) / sizeof(names[0])) != 0) {
        goto out;
    }

    for (size_t v = 0; v < sizeof(versions) / sizeof(versions[0]); v++) {
        obj->afp_version = versions[v];
        ret = catsearch_ext(obj, vid, 1U << FILPBIT_LNAME, spec, spec,
                            sizeof(spec), &nrecs, lname);
        obj->afp_version = versions[0];

        if (ret != AFPERR_EOF || nrecs != 1 || strcmp(lname, want) != 0) {
            fprintf(stderr,
                    "catsearch: long name as AFP %d: result %d, %u matches\n",
                    versions[v], ret, nrecs);
            goto out;
        }
    }

    result = 0;
out:

    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        delete (obj, vid, did, names[i]);
    }

    delete (obj, vid, DIRDID_ROOT, "cl");
    return result;
}

/*!
 * @brief A search match whose long name needs its CNID takes it from its entry
 *
 * The entry serves while its inode is the file's. A file renamed over the
 * name, which the entry learns of only from the cache hint, costs one
 * registration, and the search's metadata read refreshes the entry, so
 * FPDelete of the match succeeds. The search runs as AFP 2.1, which must
 * mangle the 40-character name. Periodic dircache validation is off, as its
 * refresh would depend on the tests run before; only the test folder's
 * registrations count.
 *
 * @returns 0, 1 when a long name, a registration count or the delete is
 *          wrong or the setup fails
 */
int utest_catsearch_match_takes_its_entrys_id(AFPObj *obj, struct vol *vol,
                                              uint16_t vid)
{
    char name[] = "a match whose long name needs its id.txt";
    const struct timespec june2001[2] = { { 991353600, 0 }, { 991353600, 0 } };
    char path[MAXPATHLEN + 1];
    char other[MAXPATHLEN + 1];
    char want[MACFILELEN + 1];
    char lname[MACFILELEN + 1];
    uint32_t nrecs = 0;
    cnid_t did;
    cnid_t id;
    int freq = obj->options.dircache_validation_freq;
    int ret;
    int result = 1;

    if ((did = make_folder(obj, vid, "cm")) == CNID_INVALID) {
        return 1;
    }

    obj->options.dircache_validation_freq = INT_MAX;
    cnid_add_did = did;
    snprintf(path, sizeof(path), "%s/cm/%s", vol->v_path, name);

    if (mangled_file(obj, vol, vid, did, name, want) == CNID_INVALID
            || utimensat(AT_FDCWD, path, june2001, 0) != 0) {
        goto out;
    }

    ret = afp2_date_search(obj, vol, vid, &nrecs, lname);

    if (ret != AFPERR_EOF || nrecs != 1 || strcmp(lname, want) != 0
            || cnid_adds != 0) {
        fprintf(stderr,
                "catsearch: cached: result %d, %u matches, '%s', %d added\n",
                ret, nrecs, lname, cnid_adds);
        goto out;
    }

    /* another file takes the name, as another session's rename leaves it
     * until the cache hint arrives; made first, it gets its own inode */
    snprintf(other, sizeof(other), "%s/cm/other.txt", vol->v_path);

    if (create_disk_file(other) != 0
            || utimensat(AT_FDCWD, other, june2001, 0) != 0
            || rename(other, path) != 0) {
        goto out;
    }

    ret = afp2_date_search(obj, vol, vid, &nrecs, lname);
    id = long_name(vol, did, name, want);

    if (ret != AFPERR_EOF || nrecs != 1 || id == CNID_INVALID
            || strcmp(lname, want) != 0 || cnid_adds != 1) {
        fprintf(stderr,
                "catsearch: stale: result %d, %u matches, '%s', %d added\n",
                ret, nrecs, lname, cnid_adds);
        goto out;
    }

    /* the search's metadata read refreshed the entry of the old inode */
    if ((ret = delete (obj, vid, did, name)) != AFP_OK) {
        fprintf(stderr, "catsearch: stale: delete %d\n", ret);
        goto out;
    }

    result = 0;
out:
    obj->options.dircache_validation_freq = freq;
    cnid_add_did = CNID_INVALID;
    /* FPDelete refuses a name whose entry holds another inode */
    drop_entry(vol, did, name);
    delete (obj, vid, did, name);
    delete (obj, vid, did, "other.txt");
    delete (obj, vid, DIRDID_ROOT, "cm");
    return result;
}

/*!
 * @brief A file FPCreateFile makes through a path registers in its folder
 *
 * cname() moves into the path's last folder, which holds the file, so the
 * file's CNID row and dircache entry belong there. A file of the same name
 * in the folder the request names keeps its CNID. Periodic dircache
 * validation is off, since a lookup that validates rebuilds an entry's path
 * from its folder's and would hide a wrong one.
 *
 * @returns 0, 1 when the new file registers outside its folder, the root's
 *          file loses its CNID or the setup fails
 */
int utest_createfile_registers_in_its_folder(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    char name[] = "made through a path.txt";
    char want[MAXPATHLEN + 1];
    size_t len = strlen(name);
    int freq = obj->options.dircache_validation_freq;
    const struct dir *folder;
    const struct dir *entry;
    cnid_t root_id;
    cnid_t did;
    int ret;
    int result = 1;

    if (createfile(obj, vid, DIRDID_ROOT, name) != 0) {
        return 1;
    }

    root_id = cnid_get(vol->v_cdb, DIRDID_ROOT, name, len);

    if (root_id == CNID_INVALID
            || (did = make_folder(obj, vid, "pc")) == CNID_INVALID) {
        delete (obj, vid, DIRDID_ROOT, name);
        return 1;
    }

    snprintf(want, sizeof(want), "%s/pc/%s", vol->v_path, name);
    obj->options.dircache_validation_freq = INT_MAX;

    if ((ret = createfile(obj, vid, DIRDID_ROOT,
                          "pc/made through a path.txt")) != 0) {
        fprintf(stderr, "create: error %d\n", ret);
    } else if (cnid_get(vol->v_cdb, DIRDID_ROOT, name, len) != root_id) {
        fprintf(stderr, "create: the root's '%s' lost its CNID\n", name);
    } else if (cnid_get(vol->v_cdb, did, name, len) == CNID_INVALID) {
        fprintf(stderr, "create: '%s' has no CNID in its folder\n", name);
    } else if ((folder = dirlookup(vol, did)) == NULL
               || (entry = dircache_search_by_name(vol, folder, name,
                                                   len)) == NULL
               || strcmp(cfrombstr(entry->d_fullpath), want) != 0) {
        fprintf(stderr, "create: '%s' has no entry at '%s'\n", name, want);
    } else {
        result = 0;
    }

    obj->options.dircache_validation_freq = freq;
    delete (obj, vid, did, name);
    delete (obj, vid, DIRDID_ROOT, "pc");
    delete (obj, vid, DIRDID_ROOT, name);
    return result;
}

/*!
 * @brief A file FPOpenFork meets through a path registers in its folder
 *
 * The file lies on disk without metadata, so the read-write open creates the
 * metadata and registers the file's CNID, which belongs under the path's last
 * folder. The open asks for no fork parameters, since getforkparams() would
 * register the file again under the fork's folder.
 *
 * @returns 0, 1 when the row sits outside the folder or the setup fails
 */
int utest_openfork_registers_in_its_folder(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    char name[] = "opened through a path.txt";
    char *names[] = { name };
    uint16_t refnum;
    cnid_t did;
    int ret;
    int result = 1;

    if ((did = make_folder(obj, vid, "po")) == CNID_INVALID) {
        return 1;
    }

    if (disk_files(vol, "po", names, 1) != 0) {
        fprintf(stderr, "openfork: no file on disk\n");
    } else if ((ret = openfork(obj, vid, DIRDID_ROOT,
                               "po/opened through a path.txt", OPENFORK_DATA,
                               OPENACC_RD | OPENACC_WR, &refnum)) != AFP_OK) {
        fprintf(stderr, "openfork: error %d\n", ret);
    } else {
        cnid_t id = cnid_get(vol->v_cdb, did, name, strlen(name));
        closefork(obj, refnum);

        if (id != CNID_INVALID) {
            result = 0;
        } else {
            fprintf(stderr, "openfork: '%s' has no CNID in its folder\n",
                    name);
        }
    }

    delete (obj, vid, did, name);
    delete (obj, vid, DIRDID_ROOT, "po");
    return result;
}

/*!
 * @brief FPCreateID through a path registers the file in its folder
 *
 * The file lies on disk without a CNID. The first FPCreateID registers it
 * under the path's last folder and the second finds that row. The third
 * names the file by its CNID with an empty path, which cname() resolves
 * without a stat, and finds the row too. The last two answer AFPERR_EXISTID
 * with the first one's CNID.
 *
 * @returns 0, 1 when a reply or the row disagrees or the setup fails
 */
int utest_createid_registers_in_its_folder(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    char name[] = "identified through a path.txt";
    char path[] = "pi/identified through a path.txt";
    char *names[] = { name };
    size_t len = strlen(name);
    cnid_t made = CNID_INVALID;
    cnid_t found = CNID_INVALID;
    cnid_t did;
    int ret;
    int result = 1;

    if ((did = make_folder(obj, vid, "pi")) == CNID_INVALID) {
        return 1;
    }

    if (disk_files(vol, "pi", names, 1) != 0) {
        fprintf(stderr, "createid: no file on disk\n");
    } else if ((ret = createid(obj, vid, DIRDID_ROOT, path, &made)) != AFP_OK
               || cnid_get(vol->v_cdb, did, name, len) != made) {
        fprintf(stderr, "createid: first: error %d, CNID %u\n", ret,
                ntohl(made));
    } else if ((ret = createid(obj, vid, DIRDID_ROOT, path,
                               &found)) != AFPERR_EXISTID
               || found != made
               || cnid_get(vol->v_cdb, did, name, len) != made) {
        fprintf(stderr, "createid: second: error %d, CNID %u for %u\n", ret,
                ntohl(found), ntohl(made));
    } else if ((ret = createid(obj, vid, made, "", &found)) != AFPERR_EXISTID
               || found != made
               || cnid_get(vol->v_cdb, did, name, len) != made) {
        fprintf(stderr, "createid: by CNID: error %d, CNID %u for %u\n", ret,
                ntohl(found), ntohl(made));
    } else {
        result = 0;
    }

    delete (obj, vid, did, name);
    delete (obj, vid, DIRDID_ROOT, "pi");
    return result;
}
