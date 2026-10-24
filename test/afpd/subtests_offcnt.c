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
 * @brief Folder offspring count, FPAccess and FPResolveID tests
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/globals.h>
#include <atalk/netatalk_conf.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "acls.h"
#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "subtests_offcnt.h"
#include "test.h"

/* the open volume and folder X of three files inside folder P on it; the
 * DIDs outlive an entry a test replaces; freq is the validation frequency
 * the teardown restores */
struct offcnt_fixture {
    struct vol *vol;
    struct dir *x;
    cnid_t p_did;
    cnid_t x_did;
    int freq;
};

/* writable: cnid_get() takes a char * name */
static char x_files[][3] = { "f1", "f2", "f3" };
#define X_FILES (sizeof(x_files) / sizeof(x_files[0]))

#define OFFCNT_BITMAP (1 << DIRPBIT_OFFCNT)

/*!
 * @brief Delete the fixture's files and folders and restore the validation
 *        frequency
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @param[in] fx   the fixture
 */
static void fixture_teardown(AFPObj *obj, uint16_t vid,
                             const struct offcnt_fixture *fx)
{
    for (size_t i = 0; i < X_FILES; i++) {
        delete (obj, vid, fx->x_did, x_files[i]);
    }

    delete (obj, vid, fx->p_did, "offcnt_x");
    delete (obj, vid, DIRDID_ROOT, "offcnt_p");
    dir_free_invalid_q();
    obj->options.dircache_validation_freq = fx->freq;
}

/*!
 * @brief Create folder P, folder X inside it and X's files, and pin the
 *        validation frequency a full interval ahead
 *
 * A failed setup leaves nothing behind: the tests share these names.
 *
 * @param[in]  obj  the test session
 * @param[in]  vid  the open test volume
 * @param[out] fx   the fixture
 * @returns 0, or -1 when a create failed
 */
static int fixture_setup(AFPObj *obj, uint16_t vid, struct offcnt_fixture *fx)
{
    fx->vol = getvolbyvid(vid);
    fx->freq = obj->options.dircache_validation_freq;

    /* afp_createdir() leaves curdir in the directory it made */
    if (createdir(obj, vid, DIRDID_ROOT, "offcnt_p") != AFP_OK) {
        return -1;
    }

    fx->p_did = curdir->d_did;

    if (createdir(obj, vid, fx->p_did, "offcnt_x") != AFP_OK) {
        delete (obj, vid, DIRDID_ROOT, "offcnt_p");
        return -1;
    }

    fx->x = curdir;
    fx->x_did = curdir->d_did;

    for (size_t i = 0; i < X_FILES; i++) {
        if (createfile(obj, vid, fx->x_did, x_files[i]) != AFP_OK) {
            fixture_teardown(obj, vid, fx);
            return -1;
        }
    }

    /* a full validation interval ahead, so no refresh undoes a ctime shift */
    obj->options.dircache_validation_freq = DEFAULT_DIRCACHE_VALIDATION_FREQ;
    dircache_reset_validation_counter();
    return 0;
}

/*!
 * @brief The counters' growth since a snapshot
 *
 * @param[in] before  the snapshot
 * @returns the difference, counter by counter
 */
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

/*!
 * @brief FPGetFileDirParms for a folder's count: a fresh stat, no AD bits
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @param[in] did  the folder
 * @returns the AFP result
 */
static int query_folder(AFPObj *obj, uint16_t vid, cnid_t did)
{
    return getfiledirparms_bitmap(obj, vid, did, "", 0, OFFCNT_BITMAP);
}

/*!
 * @brief FPEnumerate of a parent for its folders' counts: each folder's stat
 *        comes from the dircache
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @param[in] did  the parent
 * @returns the AFP result
 */
static int list_parent(AFPObj *obj, uint16_t vid, cnid_t did)
{
    return enumerate_range(obj, vid, did, 0, OFFCNT_BITMAP, 1, 20);
}

/*!
 * @brief FPEnumerate of the folder itself: the listing stores its count
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @param[in] did  the folder
 * @returns the AFP result
 */
static int list_folder(AFPObj *obj, uint16_t vid, cnid_t did)
{
    return enumerate_range(obj, vid, did, 1 << FILPBIT_FNUM, 0, 1, 20);
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
    if (query_folder(obj, vid, fx.x_did) != AFP_OK
            || query_folder(obj, vid, fx.x_did) != AFP_OK) {
        goto out;
    }

    d = since(&before);

    if (d.first != 1 || d.cached != 1 || d.entries != X_FILES) {
        goto out;
    }

    /* the parent's listing carries a dircache ctime behind the counted one */
    fx.x->dcache_ctime -= 5;

    if (list_parent(obj, vid, fx.p_did) != AFP_OK || since(&before).older != 1) {
        goto out;
    }

    /* that walk stored the listing's ctime, so the fresh stat differs from
     * it; the walk finds the count this session's create adjusted */
    if (createfile(obj, vid, fx.x_did, "new") != AFP_OK
            || query_folder(obj, vid, fx.x_did) != AFP_OK
            || since(&before).confirmed != 1) {
        goto out;
    }

    /* a count the folder has outgrown */
    fx.x->d_offcnt--;
    fx.x->d_ctime -= 5;

    if (query_folder(obj, vid, fx.x_did) != AFP_OK) {
        goto out;
    }

    d = since(&before);
    ret = d.changed == 1 && d.requests == 5 && d.failed == 0 ? 0 : -1;
out:
    delete (obj, vid, fx.x_did, "new");
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief The adjusted count is the expected one, and a forced walk agrees
 *
 * @param[in]     obj       the test session
 * @param[in]     vid       the open test volume
 * @param[in,out] x         the folder's dircache entry, whose count ctime
 *                          moves for the forced walk
 * @param[in]     expected  the count the folder should hold
 * @returns 0 when the entry holds expected before and after the walk, else -1
 */
static int walk_confirms(AFPObj *obj, uint16_t vid, struct dir *x,
                         uint32_t expected)
{
    if (x->d_offcnt != expected) {
        return -1;
    }

    x->d_ctime -= 5;
    return query_folder(obj, vid, x->d_did) == AFP_OK
           && x->d_offcnt == expected ? 0 : -1;
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
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    /* a counted folder, then the same name created again */
    if (createfile(obj, vid, fx.x_did, "f") != AFP_OK
            || query_folder(obj, vid, fx.x_did) != AFP_OK
            || createfile(obj, vid, fx.x_did, "f") != AFP_OK) {
        goto out;
    }

    ret = walk_confirms(obj, vid, fx.x, X_FILES + 1);
out:
    delete (obj, vid, fx.x_did, "f");
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A hard create through a dangling symlink, on a volume that follows
 *        symlinks, leaves the folder's count where a walk puts it: unchanged
 *        when the target lands elsewhere, one more when the link points into
 *        the folder itself.
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when the count after each create equals the walk's, else -1
 */
int utest_offcnt_hard_create_through_link(AFPObj *obj, uint16_t vid)
{
    struct offcnt_fixture fx;
    char link_path[MAXPATHLEN + 1];
    char link2_path[MAXPATHLEN + 1];
    int flags;
    int ad_options;
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    snprintf(link_path, sizeof(link_path), "%s/link",
             cfrombstr(fx.x->d_fullpath));
    snprintf(link2_path, sizeof(link2_path), "%s/link2",
             cfrombstr(fx.x->d_fullpath));
    /* the volume follows symlinks, as the option sets it up */
    flags = fx.vol->v_flags;
    ad_options = fx.vol->v_ad_options;
    fx.vol->v_flags |= AFPVOL_FOLLOWSYM;
    fx.vol->v_ad_options |= ADVOL_FOLLO_SYML;

    /* a counted folder holding a link into P that points at nothing */
    if (symlink("../target", link_path) != 0
            || query_folder(obj, vid, fx.x_did) != AFP_OK
            || createfile(obj, vid, fx.x_did, "link") != AFP_OK
            || walk_confirms(obj, vid, fx.x, X_FILES + 1) != 0) {
        goto out;
    }

    /* a link into the folder itself: the create adds the target to X */
    if (symlink("target2", link2_path) != 0
            || query_folder(obj, vid, fx.x_did) != AFP_OK) {
        goto out;
    }

    /* the count and the entry hold a ctime behind the create's, and the
     * parent's listing judges X by its dircache ctime */
    fx.x->d_ctime -= 5;
    fx.x->dcache_ctime -= 5;

    if (createfile(obj, vid, fx.x_did, "link2") != AFP_OK
            || list_parent(obj, vid, fx.p_did) != AFP_OK) {
        goto out;
    }

    ret = fx.x->d_offcnt == X_FILES + 3 ? 0 : -1;
out:
    fx.vol->v_flags = flags;
    fx.vol->v_ad_options = ad_options;
    /* unlink(), not FPDelete, which Solaris and illumos refuse for a link
     * whose target is gone */
    unlink(link2_path);
    unlink(link_path);
    delete (obj, vid, fx.x_did, "target2");
    delete (obj, vid, fx.p_did, "target");
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A create over an existing symlink, which a volume with `ea = sys`
 *        that does not follow symlinks takes as the opened file, leaves the
 *        folder's count where a walk puts it.
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open `ea = sys` test volume
 * @returns 0 when the count after the create equals the walk's, else -1
 */
int utest_offcnt_hard_create_over_symlink(AFPObj *obj, uint16_t vid)
{
    struct offcnt_fixture fx;
    char link_path[MAXPATHLEN + 1];
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    snprintf(link_path, sizeof(link_path), "%s/l", cfrombstr(fx.x->d_fullpath));

    /* a counted folder holding a link to one of its files; the create takes
     * the link as the file it opens */
    if (symlink(x_files[0], link_path) != 0
            || query_folder(obj, vid, fx.x_did) != AFP_OK
            || createfile(obj, vid, fx.x_did, "l") != AFP_OK) {
        goto out;
    }

    ret = walk_confirms(obj, vid, fx.x, X_FILES + 1);
out:
    unlink(link_path);
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A create over an existing symlink on a volume with `.AppleDouble`
 *        sidecars fails and leaves the link and the folder's count in place.
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when the link survives the create and the count equals the
 *          walk's, else -1
 */
int utest_offcnt_create_over_symlink_keeps_link(AFPObj *obj, uint16_t vid)
{
    struct offcnt_fixture fx;
    char link_path[MAXPATHLEN + 1];
    struct stat st;
    int adouble;
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    snprintf(link_path, sizeof(link_path), "%s/l", cfrombstr(fx.x->d_fullpath));
    adouble = fx.vol->v_adouble;
    fx.vol->v_adouble = AD_VERSION2;

    /* a sidecar volume cannot open metadata for a link: the create fails
     * and must not take the link with it */
    if (symlink(x_files[0], link_path) != 0
            || query_folder(obj, vid, fx.x_did) != AFP_OK
            || createfile(obj, vid, fx.x_did, "l") != AFPERR_ACCESS
            || lstat(link_path, &st) != 0) {
        goto out;
    }

    ret = walk_confirms(obj, vid, fx.x, X_FILES + 1);
out:
    fx.vol->v_adouble = adouble;
    unlink(link_path);
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A folder's own listing stores its count from a fresh stat and brings
 *        the entry to it, so the parent's listing serves the count without a
 *        walk.
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when a listing of the parent costs no walk, else -1
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

    /* the entry trails the disk when the folder's own listing stores its
     * count from a fresh stat */
    fx.x->dcache_ctime -= 5;

    if (list_folder(obj, vid, fx.x_did) != AFP_OK) {
        goto out;
    }

    offcnt_stats_get(&before);

    if (list_parent(obj, vid, fx.p_did) != AFP_OK) {
        goto out;
    }

    d = since(&before);
    ret = d.requests == 1 && d.cached == 1 ? 0 : -1;
out:
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPAccess on a folder whose dircache entry trails the disk brings
 *        the entry to the fresh stat before it caches the rights, and on an
 *        entry keyed to another inode bypasses the cached rights and brings
 *        the entry back to the object the CNID database names, by DID or by
 *        name
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when the entry holds the disk's ctime, no AppleDouble state
 *          and the rights, and the other-inode entry takes the inode back;
 *          TEST_SKIP where no ACL rights can be read; else -1
 */
int utest_offcnt_access_refreshes_cache(AFPObj *obj, uint16_t vid)
{
    struct offcnt_fixture fx;
    time_t fresh;
    int rc;
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    rc = checkaccess(obj, vid, fx.p_did, "offcnt_x", DARWIN_ACE_DELETE);

    if (rc == -1) {
        /* no ACL rights to read here, which afp_access() forwards as -1:
         * nothing reaches the cache */
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
    rc = checkaccess(obj, vid, fx.p_did, "offcnt_x", DARWIN_ACE_DELETE);
    ret = (rc == AFP_OK || rc == AFPERR_ACCESS)
          && fx.x->dcache_ctime == fresh
          && fx.x->dcache_rlen == AD_RLEN_UNKNOWN
          && fx.x->d_rights_cache != 0xffffffff ? 0 : -1;

    if (ret == 0) {
        ino_t ino = fx.x->dcache_ino;
        uid_t session_uid = obj->uid;
        uid_t session_euid = obj->euid;
        /* an entry keyed to another inode, named by its DID: the rights are
         * bypassed and its row gives the inode back; the fixture owner asks */
        fx.x->dcache_ino = ino + 1;
        fx.x->d_rights_cache = 0;
        obj->uid = getuid();
        obj->euid = obj->uid;
        rc = checkaccess(obj, vid, fx.x->d_did, "", DARWIN_ACE_LIST_DIRECTORY);
        obj->uid = session_uid;
        obj->euid = session_euid;
        ret = rc == AFP_OK && fx.x->dcache_ino == ino ? 0 : -1;

        /* named by its parent's DID and its name, the same */
        if (ret == 0) {
            fx.x->dcache_ino = ino + 1;
            rc = checkaccess(obj, vid, fx.p_did, "offcnt_x", DARWIN_ACE_DELETE);
            ret = (rc == AFP_OK || rc == AFPERR_ACCESS)
                  && fx.x->dcache_ino == ino ? 0 : -1;
        }
    }

out:
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPGetFileDirParms with a fresh stat brings the entry to that stat
 *        before the AppleDouble lookup, so the metadata the lookup reads
 *        stays cached through the count's store.
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when the entry holds the disk's ctime and a known metadata
 *          state after the request, else -1
 */
int utest_offcnt_params_keep_metadata(AFPObj *obj, uint16_t vid)
{
    const uint16_t finfo_count = (1 << DIRPBIT_FINFO) | OFFCNT_BITMAP;
    struct offcnt_fixture fx;
    time_t fresh;
    int rc;
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    if (query_folder(obj, vid, fx.x_did) != AFP_OK) {
        goto out;
    }

    /* the entry and its count trail the disk, with no metadata state */
    fresh = fx.x->dcache_ctime;
    fx.x->dcache_ctime = fresh - 5;
    fx.x->d_ctime = fresh - 5;
    fx.x->dcache_rlen = AD_RLEN_UNKNOWN;
    rc = getfiledirparms_bitmap(obj, vid, fx.x_did, "", 0, finfo_count);

    if (rc != AFP_OK) {
        goto out;
    }

    ret = fx.x->dcache_ctime == fresh
          && fx.x->dcache_rlen != AD_RLEN_UNKNOWN ? 0 : -1;
out:
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPAccess answers a file's delete right as unlink() would: from the
 *        parent folder's ACL, the sticky bit and the user the server acts as,
 *        judging the folder itself where its entry names it through a link
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when every leg answers as unlink() would; TEST_SKIP where no
 *          ACL rights can be read; else -1
 */
int utest_access_delete_right_follows_parent(AFPObj *obj, uint16_t vid)
{
    struct offcnt_fixture fx;
    bstring folder_path;
    bstring link_path = NULL;
    const char *folder;
    char file[MAXPATHLEN + 1];
    char link[MAXPATHLEN + 1];
    gid_t group;
    gid_t *session_groups = obj->groups;
    int session_ngroups = obj->ngroups;
    uid_t session_uid = obj->uid;
    uid_t session_euid = obj->euid;
    int flags;
    int granted;
    int refused;
    int sticky;
    int linked;
    int forced;
    int admin;
    int writer;
    int shared;
#ifdef HAVE_NFSV4_ACLS
    const int writer_expected = AFP_OK;
#else
    const int writer_expected = AFPERR_ACCESS;
#endif
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    folder_path = fx.x->d_fullpath;
    folder = cfrombstr(folder_path);
    snprintf(file, sizeof(file), "%s/%s", folder, x_files[0]);
    snprintf(link, sizeof(link), "%s/l",
             cfrombstr(dirlookup(fx.vol, fx.p_did)->d_fullpath));
    flags = fx.vol->v_flags;
    /* the rights of the owner of what the fixture made, whichever user runs
     * the tests */
    obj->uid = getuid();
    obj->euid = obj->uid;

    /* a read-only file in a folder its owner may write */
    if (chmod(folder, 0700) != 0 || chmod(file, 0400) != 0) {
        goto out;
    }

    granted = checkaccess(obj, vid, fx.x_did, x_files[0], DARWIN_ACE_DELETE);

    /* a writable file in a folder its owner may not write */
    if (chmod(folder, 0500) != 0 || chmod(file, 0600) != 0) {
        goto out;
    }

    refused = checkaccess(obj, vid, fx.x_did, x_files[0], DARWIN_ACE_DELETE);
    /* a stranger in the folder's group, which may write the folder: the
     * sticky bit refuses as unlink() would, its absence grants; NFSv4 ACLs
     * (Solaris, FreeBSD) also pass a writer of the file */
    group = fx.x->dcache_gid;
    obj->groups = &group;
    obj->ngroups = 1;
    obj->uid = getuid() + 1;
    obj->euid = obj->uid;

    if (chmod(folder, 01770) != 0 || chmod(file, 0640) != 0) {
        goto out;
    }

    sticky = checkaccess(obj, vid, fx.x_did, x_files[0], DARWIN_ACE_DELETE);

    /* the entry names the folder through a link, as a volume that follows
     * symlinks leaves it: the rule judges the folder, not the link */
    if (symlink(folder, link) != 0 || (link_path = bfromcstr(link)) == NULL) {
        goto out;
    }

    fx.vol->v_flags |= AFPVOL_FOLLOWSYM;
    fx.x->d_fullpath = link_path;
    linked = checkaccess(obj, vid, fx.x_did, x_files[0], DARWIN_ACE_DELETE);
    fx.x->d_fullpath = folder_path;
    fx.vol->v_flags = flags;
    /* the server acts as the forced user, who owns what the session made,
     * or as root for an admin group member: the kernel lets both delete */
    obj->euid = getuid();
    forced = checkaccess(obj, vid, fx.x_did, x_files[0], DARWIN_ACE_DELETE);
    obj->euid = 0;
    admin = checkaccess(obj, vid, fx.x_did, x_files[0], DARWIN_ACE_DELETE);
    obj->euid = obj->uid;

    if (chmod(file, 0660) != 0) {
        goto out;
    }

    writer = checkaccess(obj, vid, fx.x_did, x_files[0], DARWIN_ACE_DELETE);

    if (chmod(folder, 0770) != 0) {
        goto out;
    }

    shared = checkaccess(obj, vid, fx.x_did, x_files[0], DARWIN_ACE_DELETE);

    if (granted == -1 && refused == -1) {
        /* no ACL rights to read here: afp_access() forwards the -1 */
        ret = TEST_SKIP;
    } else {
        ret = granted == AFP_OK && refused == AFPERR_ACCESS
              && sticky == AFPERR_ACCESS && linked == AFPERR_ACCESS
              && forced == AFP_OK && admin == AFP_OK
              && writer == writer_expected && shared == AFP_OK ? 0 : -1;
    }

out:
    obj->uid = session_uid;
    obj->euid = session_euid;
    obj->groups = session_groups;
    obj->ngroups = session_ngroups;
    chmod(folder, 0700);
    unlink(link);
    bdestroy(link_path);
    fixture_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPResolveID re-enumerates the folder it moved into: after
 *        movecwd() healed a stale entry, the folder's other file IDs keep
 *        their parent and the live entry carries the walk's flag.
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open test volume
 * @returns 0 when the sibling's ID still resolves under the folder and the
 *          live entry is flagged, else -1
 */
int utest_resolveid_healed_folder_keeps_ids(AFPObj *obj, uint16_t vid)
{
    struct offcnt_fixture fx;
    const struct dir *live;
    cnid_t gone;
    cnid_t sibling;
    char path[MAXPATHLEN + 1];
    int rc;
    int ret = -1;

    if (fixture_setup(obj, vid, &fx) != 0) {
        return -1;
    }

    gone = cnid_get(fx.vol->v_cdb, fx.x_did, x_files[0], strlen(x_files[0]));
    sibling = cnid_get(fx.vol->v_cdb, fx.x_did, x_files[1], strlen(x_files[1]));
    snprintf(path, sizeof(path), "%s/%s", cfrombstr(fx.x->d_fullpath),
             x_files[0]);

    /* the file goes away on disk and the folder's cached path turns stale,
     * so the resolve heals the entry before it re-enumerates */
    if (gone == CNID_INVALID || sibling == CNID_INVALID || unlink(path) != 0
            || bcatcstr(fx.x->d_fullpath, "-stale") != BSTR_OK) {
        goto out;
    }

    rc = resolveid(obj, vid, gone);
    live = dirlookup(fx.vol, fx.x_did);
    ret = rc == AFPERR_NOID && live != NULL
          && strstr(cfrombstr(live->d_fullpath), "-stale") == NULL
          && (live->d_flags & DIRF_CNID)
          && cnid_get(fx.vol->v_cdb, fx.x_did, x_files[1],
                      strlen(x_files[1])) == sibling ? 0 : -1;
out:
    fixture_teardown(obj, vid, &fx);
    return ret;
}
