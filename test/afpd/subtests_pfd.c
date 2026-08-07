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

/*
 * pfd_ostat() vs plain ostat(d_fullpath) equivalence harness.
 *
 * Quiescent equivalence: no external mutation between the two calls — return
 * value, errno, and every consumed struct stat field must be identical
 * across the matrix (existing/ENOENT, root-parented and deep-parented,
 * slot absent/present, ghost-parented must fall back).
 *
 * Divergence window: external mutation raced between the calls —
 * assert the permitted-outcome set (one of two truthful world views,
 * never a third state) and the probe bound: a rename-away+recreate is
 * detected within PFD_REGROUND_INTERVAL uses, and after the probe refill
 * the slot serves WITHOUT re-opening while the dircache entry still lags
 * (pins the three-way sync-check; a naive compare re-opens every hit).
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "pfd_cache.h"
#include "subtests_pfd.h"
#include "test.h"
#include "volume.h"

/* Compare the stat fields the dircache validation sites consume */
static int st_fields_equal(const struct stat *a, const struct stat *b)
{
    return a->st_dev == b->st_dev && a->st_ino == b->st_ino
           && a->st_ctime == b->st_ctime && a->st_mode == b->st_mode
           && a->st_mtime == b->st_mtime && a->st_uid == b->st_uid
           && a->st_gid == b->st_gid && a->st_size == b->st_size;
}

/* One equivalence point: run both forms, demand identical rc/errno and,
 * on success, identical consumed fields. */
static int equiv_check(const struct vol *vol, struct dir *entry,
                       const char *label)
{
    struct stat st_pfd, st_plain;
    int rc_pfd, rc_plain, errno_pfd, errno_plain;
    memset(&st_pfd, 0xAA, sizeof(st_pfd));
    memset(&st_plain, 0x55, sizeof(st_plain));
    errno = 0;
    rc_pfd = pfd_ostat(vol, entry, &st_pfd, vol_syml_opt(vol));
    errno_pfd = errno;
    errno = 0;
    rc_plain = ostat(cfrombstr(entry->d_fullpath), &st_plain,
                     vol_syml_opt(vol));
    errno_plain = errno;

    if (rc_pfd != rc_plain) {
        fprintf(test_stream(), "# pfd-equiv %s: rc %d != %d\n", label, rc_pfd,
                rc_plain);
        return -1;
    }

    if (rc_pfd != 0 && errno_pfd != errno_plain) {
        fprintf(test_stream(), "# pfd-equiv %s: errno %d != %d\n", label, errno_pfd,
                errno_plain);
        return -1;
    }

    if (rc_pfd == 0 && !st_fields_equal(&st_pfd, &st_plain)) {
        fprintf(test_stream(), "# pfd-equiv %s: stat fields differ\n", label);
        return -1;
    }

    return 0;
}

/* Build a real directory + child file under the volume root, with live
 * dircache entries for both, mirroring what dirlookup/dir_add produce.
 * On failure nothing stays cached (the caller cannot clean up what it
 * never received). */
static struct dir *make_tree(struct vol *vol, const char *dname,
                             const char *fname, cnid_t ddid, cnid_t fdid,
                             struct dir **file_out)
{
    char pbuf[MAXPATHLEN + 1];
    struct stat st;
    struct dir *d, *f;
    bstring dpath, fpath;
    snprintf(pbuf, sizeof(pbuf), "%s/%s", vol->v_path, dname);

    if (mkdir(pbuf, 0700) != 0 && errno != EEXIST) {
        return NULL;
    }

    if (stat(pbuf, &st) != 0) {
        return NULL;
    }

    dpath = bfromcstr(pbuf);
    d = dir_new(dname, dname, vol, DIRDID_ROOT, ddid, dpath, &st);

    if (d == NULL) {
        bdestroy(dpath);
        return NULL;
    }

    if (dircache_add(vol, d) != 0) {
        dir_free(d);
        return NULL;
    }

    snprintf(pbuf, sizeof(pbuf), "%s/%s/%s", vol->v_path, dname, fname);
    int tfd = open(pbuf, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (tfd < 0) {
        goto fail_dir;
    }

    (void)!write(tfd, "eq", 2);
    close(tfd);

    if (stat(pbuf, &st) != 0) {
        goto fail_dir;
    }

    fpath = bfromcstr(pbuf);
    f = dir_new(fname, fname, vol, ddid, fdid, fpath, &st);

    if (f == NULL) {
        bdestroy(fpath);
        goto fail_dir;
    }

    if (dircache_add(vol, f) != 0) {
        dir_free(f);
        goto fail_dir;
    }

    *file_out = f;
    return d;
fail_dir:
    dir_remove(vol, d, 0);
    dir_free_invalid_q();
    return NULL;
}

static void unmake_tree(struct vol *vol, const char *dname, const char *fname)
{
    char pbuf[MAXPATHLEN + 1];
    snprintf(pbuf, sizeof(pbuf), "%s/%s/%s", vol->v_path, dname, fname);
    unlink(pbuf);
    snprintf(pbuf, sizeof(pbuf), "%s/%s", vol->v_path, dname);
    rmdir(pbuf);
}

/* Quiescent equivalence matrix */
int test_pfd_ostat_equivalence(struct vol *vol)
{
    struct dir *d = NULL, *f = NULL, *rootkid = NULL;
    struct stat st;
    char pbuf[MAXPATHLEN + 1];
    int ret = -1;

    /* deep-parented file entry (parent in dircache) */
    if ((d = make_tree(vol, "pfd_eq_dir", "pfd_eq_file", htonl(50001),
                       htonl(50002), &f)) == NULL) {
        goto out;
    }

    /* 1: existing file, deep parent — twice (slot absent, then present) */
    if (equiv_check(vol, f, "deep-existing/slot-miss") != 0
            || equiv_check(vol, f, "deep-existing/slot-hit") != 0) {
        goto out;
    }

    /* 2: existing dir entry whose parent is the volume root */
    snprintf(pbuf, sizeof(pbuf), "%s/pfd_eq_dir", vol->v_path);

    if (stat(pbuf, &st) != 0) {
        goto out;
    }

    rootkid = d;

    if (equiv_check(vol, rootkid, "root-parented") != 0) {
        goto out;
    }

    /* 3: ENOENT — delete the file on disk, entries still cached */
    snprintf(pbuf, sizeof(pbuf), "%s/pfd_eq_dir/pfd_eq_file", vol->v_path);
    unlink(pbuf);

    if (equiv_check(vol, f, "enoent") != 0) {
        goto out;
    }

    /* 4: ghost-parented entry must fall back to the identical full-path
     * form: force the parent to read as a ghost, compare, restore */
    d->d_flags |= DIRF_ARC_GHOST;

    if (equiv_check(vol, f, "ghost-parent-fallback") != 0) {
        d->d_flags &= ~DIRF_ARC_GHOST;
        goto out;
    }

    d->d_flags &= ~DIRF_ARC_GHOST;
    ret = 0;
out:

    if (f) {
        dir_remove(vol, f, 0);
    }

    if (d) {
        dir_remove(vol, d, 0);
    }

    dir_free_invalid_q();
    unmake_tree(vol, "pfd_eq_dir", "pfd_eq_file");
    return ret;
}

/* Internal rename: parent entry re-pathed in place (dir_modify DCMOD_PATH,
 * same inode), children deferred — the window where a child's d_fullpath is
 * stale while its parent's is already correct. Validation through the pfd
 * path must not serve the dead path: it must repair d_fullpath from the
 * parent (same object, so no expunge needed). Plain ostat self-healed here
 * by ENOENT+expunge; serving the stale string is the one forbidden outcome. */
int test_pfd_rename_repairs_child_path(struct vol *vol)
{
    struct dir *d = NULL, *f = NULL;
    struct stat st;
    char oldp[MAXPATHLEN + 1], newp[MAXPATHLEN + 1], expect[MAXPATHLEN + 1];
    int ret = -1;
    snprintf(oldp, sizeof(oldp), "%s/pfd_rp_dir", vol->v_path);
    snprintf(newp, sizeof(newp), "%s/pfd_rp_new", vol->v_path);
    snprintf(expect, sizeof(expect), "%s/pfd_rp_new/pfd_rp_file", vol->v_path);

    if ((d = make_tree(vol, "pfd_rp_dir", "pfd_rp_file", htonl(50021),
                       htonl(50022), &f)) == NULL) {
        goto out;
    }

    /* Warm the slot so the parent fd predates the rename */
    if (pfd_ostat(vol, f, &st, vol_syml_opt(vol)) != 0) {
        goto out;
    }

    /* The internal-rename shape: disk renamed, parent entry re-pathed in
     * place (same did, same inode), child entry untouched (deferred). */
    if (rename(oldp, newp) != 0) {
        goto out;
    }

    {
        bstring rootpath = bfromcstr(vol->v_path);

        if (dir_modify(vol, d, &(struct dir_modify_args) {
        .flags = DCMOD_PATH,
        .new_pdid = DIRDID_ROOT,
        .new_mname = "pfd_rp_new",
        .new_uname = "pfd_rp_new",
        .new_pdir_path = rootpath
    }) != 0) {
            bdestroy(rootpath);
            goto out;
        }
        bdestroy(rootpath);
    }

    /* Validate the child: the relative stat succeeds (parent fd + basename
     * both still true), so the entry survives — but its d_fullpath must
     * come out repaired to the parent's new path, never the dead one. */
    if (pfd_ostat(vol, f, &st, vol_syml_opt(vol)) != 0) {
        goto out;
    }

    if (f->d_fullpath == NULL
            || strcmp(cfrombstr(f->d_fullpath), expect) != 0) {
        fprintf(test_stream(),
                "# pfd-repair: child path \"%s\", want \"%s\"\n",
                f->d_fullpath ? cfrombstr(f->d_fullpath) : "(null)", expect);
        goto out;
    }

    ret = 0;
out:

    if (f) {
        dir_remove(vol, f, 0);
    }

    if (d) {
        dir_remove(vol, d, 0);
    }

    dir_free_invalid_q();
    unlink(expect);
    rmdir(newp);
    unmake_tree(vol, "pfd_rp_dir", "pfd_rp_file");
    return ret;
}

/* Volume close must retire every slot keyed on the volume: the O_PATH fds
 * otherwise outlive the close (pinning directories against unmount) and,
 * because struct vol and v_vid persist across logout/re-login, a stale
 * slot would satisfy (vid, did) matches for a directory replaced while
 * the volume was closed. The test drives the documented logout/re-login
 * cycle (closevol, assert, reopen) and leaves the volume as it found it —
 * the harness keeps ownership of the final close. */
int test_pfd_vol_close_purges_slots(AFPObj *obj, struct vol *vol)
{
    struct dir *d = NULL, *f = NULL;
    struct stat st;
    struct pfd_stats before, after;
    int ret = -1;
    int closed = 0;

    if ((d = make_tree(vol, "pfd_cv_dir", "pfd_cv_file", htonl(50031),
                       htonl(50032), &f)) == NULL) {
        goto out;
    }

    /* Warm a slot keyed on (v_vid, d->d_did) */
    if (pfd_ostat(vol, f, &st, vol_syml_opt(vol)) != 0) {
        goto out;
    }

    /* Close with the entries still cached — the dircache is not flushed
     * by a volume close, so the slot must be retired by closevol itself,
     * not by an entry-removal side effect. Mirror the real close paths
     * (afp_closevol, close_all_vol), which clear curdir first. */
    curdir = NULL;
    pfd_stats_get(&before);
    closevol(obj, vol);
    closed = 1;
    pfd_stats_get(&after);

    if (after.purges <= before.purges) {
        fprintf(test_stream(),
                "# pfd-volclose: slot survived closevol (purges %llu -> %llu)\n",
                before.purges, after.purges);
        goto out;
    }

    ret = 0;
out:

    if (f) {
        dir_remove(vol, f, 0);
    }

    if (d) {
        dir_remove(vol, d, 0);
    }

    dir_free_invalid_q();
    unmake_tree(vol, "pfd_cv_dir", "pfd_cv_file");

    /* Re-login: same struct vol and v_vid, fresh v_root/CNID handle */
    if (closed && openvol(obj, "afpd_test") == 0) {
        fprintf(test_stream(), "# pfd-volclose: reopen after close failed\n");
        ret = -1;
    }

    return ret;
}

/* Divergence window: probe bound + no-thrash */
int test_pfd_probe_rename_detection(struct vol *vol)
{
    struct dir *d = NULL, *f = NULL;
    struct stat st;
    struct pfd_stats before, after;
    char oldp[MAXPATHLEN + 1], movedp[MAXPATHLEN + 1], childp[MAXPATHLEN + 1];
    int ret = -1;
    int rc;
    /* Cleanup consumes these; early gotos must not unlink garbage paths */
    snprintf(oldp, sizeof(oldp), "%s/pfd_mv_dir", vol->v_path);
    snprintf(movedp, sizeof(movedp), "%s/pfd_mv_away", vol->v_path);
    snprintf(childp, sizeof(childp), "%s/pfd_mv_dir/pfd_mv_file", vol->v_path);

    if ((d = make_tree(vol, "pfd_mv_dir", "pfd_mv_file", htonl(50011),
                       htonl(50012), &f)) == NULL) {
        goto out;
    }

    /* Warm the slot */
    if (pfd_ostat(vol, f, &st, vol_syml_opt(vol)) != 0) {
        goto out;
    }

    /* External rename-away + recreate: the one real staleness case */

    /* Path-based on purpose: this test IS the external mutator — it
     * simulates a foreign process renaming the directory away and
     * recreating the path, exactly the race the probe must detect.
     * Nothing here trusts the pre-race resolution. */
    if (rename(oldp, movedp) != 0
            || mkdir(oldp, 0700) != 0) { // NOSONAR (TOCTOU is the scenario under test)
        goto out;
    }

    int nfd = open(childp, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (nfd < 0) {
        goto out;
    }

    (void)!write(nfd, "new-world", 9);
    /* Permitted-outcome set: every result until the probe fires must be a
     * truthful answer from ONE of the two coherent worlds — the old object
     * (old ino via the held fd) or the new path (new ino) — never a third
     * state. Drive PFD_REGROUND_INTERVAL uses; the probe must detect the
     * rename within the bound. */
    struct stat old_st, new_st;

    /* Identity via the creating fd (no path re-resolution race) */
    if (fstat(nfd, &new_st) != 0) {
        close(nfd);
        goto out;
    }

    close(nfd);
    old_st = st; /* old-world identity captured by the warmup call */
    pfd_stats_get(&before);
    int detected = 0;

    for (int i = 0; i < PFD_REGROUND_INTERVAL + 2; i++) {
        rc = pfd_ostat(vol, f, &st, vol_syml_opt(vol));

        if (rc == 0 && st.st_ino == new_st.st_ino) {
            detected = 1; /* new world reached */
            break;
        }

        if (rc == 0 && st.st_ino != old_st.st_ino) {
            fprintf(test_stream(), "# pfd-probe: third-state ino %llu\n",
                    (unsigned long long)st.st_ino);
            goto out; /* neither old nor new world: forbidden */
        }

        /* rc != 0 (ENOENT through a refilled fd while only the old world
         * had the child) is also truthful; keep driving */
    }

    pfd_stats_get(&after);

    if (!detected) {
        fprintf(test_stream(), "# pfd-probe: probe did not converge within %d uses\n",
                PFD_REGROUND_INTERVAL + 2);
        goto out;
    }

    if (after.probe_refreshes == before.probe_refreshes) {
        fprintf(test_stream(), "# pfd-probe: convergence without a probe refresh?\n");
        goto out;
    }

    /* No-thrash: the slot now holds the new object while f's PARENT
     * dircache entry (d) still records the old ino. Further hits must
     * serve without re-opening: opens and sync_refreshes stay flat. */
    pfd_stats_get(&before);

    for (int i = 0; i < 16; i++) {
        (void)pfd_ostat(vol, f, &st, vol_syml_opt(vol));
    }

    pfd_stats_get(&after);

    if (after.opens != before.opens
            || after.sync_refreshes != before.sync_refreshes) {
        fprintf(test_stream(),
                "# pfd-probe thrash: opens %llu->%llu sync_refreshes %llu->%llu\n",
                before.opens, after.opens, before.sync_refreshes,
                after.sync_refreshes);
        goto out;
    }

    ret = 0;
out:

    if (f) {
        dir_remove(vol, f, 0);
    }

    if (d) {
        dir_remove(vol, d, 0);
    }

    dir_free_invalid_q();
    unlink(childp);
    snprintf(childp, sizeof(childp), "%s/pfd_mv_away/pfd_mv_file", vol->v_path);
    unlink(childp);
    unmake_tree(vol, "pfd_mv_dir", "pfd_mv_file");
    snprintf(oldp, sizeof(oldp), "%s/pfd_mv_away", vol->v_path);
    rmdir(oldp);
    return ret;
}
