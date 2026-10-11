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

#include <stdint.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>

#include <atalk/afp.h>
#include <atalk/directory.h>
#include <atalk/globals.h>
#include <atalk/util.h>

#include "acls.h"
#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "subtests_offcnt.h"
#include "test.h"

/* a folder X of three files inside a folder P, both as dircache entries */
struct offcnt_fixture {
    struct dir *p;
    struct dir *x;
};

static const char *const x_files[] = { "f1", "f2", "f3" };
#define X_FILES (sizeof(x_files) / sizeof(x_files[0]))

#define OFFCNT_BITMAP (1 << DIRPBIT_OFFCNT)

static void fixture_teardown(AFPObj *obj, uint16_t vid,
                             const struct offcnt_fixture *fx)
{
    for (size_t i = 0; i < X_FILES; i++) {
        delete (obj, vid, fx->x->d_did, x_files[i]);
    }

    delete (obj, vid, fx->p->d_did, "offcnt_x");
    delete (obj, vid, DIRDID_ROOT, "offcnt_p");
}

/* a failed setup leaves nothing behind: the tests share these names */
static int fixture_setup(AFPObj *obj, uint16_t vid, struct offcnt_fixture *fx)
{
    /* afp_createdir() leaves curdir in the directory it made */
    if (createdir(obj, vid, DIRDID_ROOT, "offcnt_p") != AFP_OK) {
        return -1;
    }

    fx->p = curdir;

    if (createdir(obj, vid, fx->p->d_did, "offcnt_x") != AFP_OK) {
        delete (obj, vid, DIRDID_ROOT, "offcnt_p");
        return -1;
    }

    fx->x = curdir;

    for (size_t i = 0; i < X_FILES; i++) {
        if (createfile(obj, vid, fx->x->d_did, x_files[i]) != AFP_OK) {
            fixture_teardown(obj, vid, fx);
            return -1;
        }
    }

    /* a full validation interval ahead, so no refresh undoes a ctime shift */
    dircache_reset_validation_counter();
    return 0;
}

/* the counters' growth since a snapshot */
static struct offcnt_stats since(const struct offcnt_stats *before)
{
    struct offcnt_stats d;
    offcnt_stats_get(&d);
    d.requests -= before->requests;
    d.cached -= before->cached;
    d.first -= before->first;
    d.older -= before->older;
    d.confirmed -= before->confirmed;
    d.changed -= before->changed;
    d.failed -= before->failed;
    d.entries -= before->entries;
    return d;
}

/* FPGetFileDirParms on the folder itself: a fresh stat, no AD bits */
static int query_folder(AFPObj *obj, uint16_t vid, const struct dir *dir)
{
    return getfiledirparms_bitmap(obj, vid, dir->d_did, "", 0, OFFCNT_BITMAP);
}

/* FPEnumerate of the parent: the folder's stat comes from the dircache */
static int list_parent(AFPObj *obj, uint16_t vid, const struct dir *parent)
{
    return enumerate_range(obj, vid, parent->d_did, 0, OFFCNT_BITMAP, 1, 20);
}

/*!
 * @brief Each walk is attributed to one cause, and a current count is served
 *        without a walk.
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when every counter moved as expected, else -1
 */
int utest_offcnt_stat_causes(AFPObj *obj, uint16_t vid)
{
    struct offcnt_fixture fx;
    struct offcnt_stats before;
    struct offcnt_stats d;
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    offcnt_stats_get(&before);

    /* never counted, then unchanged */
    if (query_folder(obj, vid, fx.x) != AFP_OK
            || query_folder(obj, vid, fx.x) != AFP_OK) {
        goto out;
    }

    d = since(&before);

    if (d.first != 1 || d.cached != 1 || d.entries != X_FILES) {
        goto out;
    }

    /* the parent's listing carries a dircache ctime behind the counted one */
    fx.x->dcache_ctime -= 5;

    if (list_parent(obj, vid, fx.p) != AFP_OK || since(&before).older != 1) {
        goto out;
    }

    /* that walk stored the listing's ctime, so the fresh stat differs from
     * it; the walk finds the count this session's create adjusted */
    if (createfile(obj, vid, fx.x->d_did, "new") != AFP_OK
            || query_folder(obj, vid, fx.x) != AFP_OK
            || since(&before).confirmed != 1) {
        goto out;
    }

    /* a count the folder has outgrown */
    fx.x->d_offcnt--;
    fx.x->d_ctime -= 5;

    if (query_folder(obj, vid, fx.x) != AFP_OK) {
        goto out;
    }

    d = since(&before);
    ret = d.changed == 1 && d.requests == 5 && d.failed == 0 ? 0 : -1;
out:
    delete (obj, vid, fx.x->d_did, "new");
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A hard create over an existing file leaves the adjusted count equal
 *        to what a walk computes.
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when the adjusted count holds the new file once and matches
 *          the walk, else -1
 */
int utest_offcnt_hard_create_existing(AFPObj *obj, uint16_t vid)
{
    struct offcnt_fixture fx;
    uint32_t adjusted;
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    /* a counted folder, then a new file and the same name created again */
    if (createfile(obj, vid, fx.x->d_did, "f") != AFP_OK
            || query_folder(obj, vid, fx.x) != AFP_OK
            || createfile(obj, vid, fx.x->d_did, "g") != AFP_OK
            || createfile(obj, vid, fx.x->d_did, "f") != AFP_OK) {
        goto out;
    }

    adjusted = fx.x->d_offcnt;
    /* force a walk: the adjusted count must match it */
    fx.x->d_ctime -= 5;

    if (query_folder(obj, vid, fx.x) != AFP_OK) {
        goto out;
    }

    ret = adjusted == X_FILES + 2 && adjusted == fx.x->d_offcnt ? 0 : -1;
out:
    delete (obj, vid, fx.x->d_did, "g");
    delete (obj, vid, fx.x->d_did, "f");
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A fresh stat of a folder brings its dircache ctime forward, so the
 *        parent's listing and the folder's own parameters share one walk.
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when three rounds cost one walk, else -1
 */
int utest_offcnt_fresh_stat_refreshes_cache(AFPObj *obj, uint16_t vid)
{
    struct offcnt_fixture fx;
    struct offcnt_stats before;
    struct offcnt_stats d;
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    if (query_folder(obj, vid, fx.x) != AFP_OK) {
        goto out;
    }

    /* a count taken while the dircache trailed the disk */
    fx.x->dcache_ctime -= 5;
    fx.x->d_ctime = fx.x->dcache_ctime;
    offcnt_stats_get(&before);

    for (int round = 0; round < 3; round++) {
        if (query_folder(obj, vid, fx.x) != AFP_OK
                || list_parent(obj, vid, fx.p) != AFP_OK) {
            goto out;
        }
    }

    d = since(&before);
    ret = d.requests == 6 && d.confirmed == 1 && d.cached == 5 ? 0 : -1;
out:
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPAccess on a folder whose dircache entry trails the disk brings
 *        the entry to the fresh stat before it caches the rights.
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when the entry holds the disk's ctime, no AppleDouble state
 *          and the rights; TEST_SKIP where no ACL rights can be read; else -1
 */
int utest_offcnt_access_refreshes_cache(AFPObj *obj, uint16_t vid)
{
#ifdef HAVE_ACLS
    struct offcnt_fixture fx;
    time_t fresh;
    int rc;
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    rc = checkaccess(obj, vid, fx.p->d_did, "offcnt_x", DARWIN_ACE_DELETE);

    if (rc == -1) {
        /* no ACL rights to read here: nothing reaches the cache */
        ret = TEST_SKIP;
        goto out;
    }

    if (rc != AFP_OK && rc != AFPERR_ACCESS) {
        goto out;
    }

    /* the entry falls behind the disk, with AppleDouble state and no rights */
    fresh = fx.x->dcache_ctime;
    fx.x->dcache_ctime = fresh - 5;
    fx.x->dcache_rlen = 0;
    fx.x->d_rights_cache = 0xffffffff;
    rc = checkaccess(obj, vid, fx.p->d_did, "offcnt_x", DARWIN_ACE_DELETE);

    if (rc == AFP_OK || rc == AFPERR_ACCESS) {
        ret = fx.x->dcache_ctime == fresh
              && fx.x->dcache_rlen == AD_RLEN_UNKNOWN
              && fx.x->d_rights_cache != 0xffffffff ? 0 : -1;
    }

out:
    fixture_teardown(obj, vid, &fx);
    return ret;
#else
    (void)obj;
    (void)vid;
    return TEST_SKIP;
#endif
}

/*!
 * @brief FPAccess answers a file's delete right from the parent folder's
 *        ACL, whatever the file's own permissions say.
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when a writable folder grants the right and a read-only folder
 *          refuses it; TEST_SKIP where no ACL rights can be read; else -1
 */
int utest_access_delete_right_follows_parent(AFPObj *obj, uint16_t vid)
{
#ifdef HAVE_ACLS
    struct offcnt_fixture fx;
    const char *folder;
    char file[MAXPATHLEN + 1];
    int granted;
    int refused;
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    folder = cfrombstr(fx.x->d_fullpath);
    snprintf(file, sizeof(file), "%s/%s", folder, x_files[0]);

    /* a read-only file in a folder anyone may write */
    if (chmod(folder, 0777) != 0 || chmod(file, 0444) != 0) {
        goto out;
    }

    granted = checkaccess(obj, vid, fx.x->d_did, x_files[0], DARWIN_ACE_DELETE);

    /* a writable file in a folder nobody may write */
    if (chmod(folder, 0555) != 0 || chmod(file, 0666) != 0) {
        goto out;
    }

    refused = checkaccess(obj, vid, fx.x->d_did, x_files[0], DARWIN_ACE_DELETE);

    if (granted == -1 || refused == -1) {
        /* no ACL rights to read here */
        ret = TEST_SKIP;
    } else {
        ret = granted == AFP_OK && refused == AFPERR_ACCESS ? 0 : -1;
    }

out:
    chmod(folder, 0755);
    fixture_teardown(obj, vid, &fx);
    return ret;
#else
    (void)obj;
    (void)vid;
    return TEST_SKIP;
#endif
}
