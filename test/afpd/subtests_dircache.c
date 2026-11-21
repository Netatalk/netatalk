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
 * @brief Dircache tests against the afpd_test volume: the stat a request
 *        refreshes an entry with, and objects replaced or gone behind the
 *        cache
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/globals.h>
#include <atalk/server_ipc.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "acls.h"
#include "ad_cache.h"
#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "faultinject.h"
#include "fork.h"
#include "pfd_cache.h"
#include "subtests_dircache.h"
#include "test.h"
#include "test_capabilities.h"

#define DC_FOLDER "dc-folder"
#define DC_CHILD  "dc-child"
#define DC_LEAF   "dc-leaf"
#define DC_ASIDE  "dc-folder-aside"
#define DC_CHILD_ASIDE "aside"

/* the validation frequency the teardown restores, the test folder's CNID
 * and its child's, the folder's path and the path it moves aside to,
 * whether it moved there, and the child's path and the path it moves aside
 * to */
struct dc_fixture {
    int freq;
    cnid_t did;
    cnid_t child_id;
    char path[MAXPATHLEN + 1];
    char aside[MAXPATHLEN + 1];
    int moved;
    char child[MAXPATHLEN + 1];
    char child_aside[MAXPATHLEN + 1];
};

/*!
 * @brief The path of a name at the volume root
 */
static void root_path(const struct vol *vol, char *path, const char *name)
{
    snprintf(path, MAXPATHLEN + 1, "%s/%s", vol->v_path, name);
}

/*!
 * @brief The path of a name inside the test folder
 */
static void folder_path(const struct vol *vol, char *path, const char *name)
{
    snprintf(path, MAXPATHLEN + 1, "%s/" DC_FOLDER "/%s", vol->v_path, name);
}

/*!
 * @brief Start a test whose lookups no validation answers
 *
 * @param[in]  obj  the test session
 * @param[in]  vol  the open volume
 * @param[out] fx   the fixture
 */
static void dc_setup(AFPObj *obj, const struct vol *vol, struct dc_fixture *fx)
{
    memset(fx, 0, sizeof(*fx));
    fx->freq = obj->options.dircache_validation_freq;
    obj->options.dircache_validation_freq = INT_MAX;
    root_path(vol, fx->path, DC_FOLDER);
    root_path(vol, fx->aside, DC_ASIDE);
    folder_path(vol, fx->child, DC_CHILD);
    folder_path(vol, fx->child_aside, DC_CHILD_ASIDE);
}

/*!
 * @brief Remove the test folder, its child and the child moved aside
 *        through a client's requests, by path from the volume root,
 *        whatever the cache holds for them
 */
static void remove_pair(AFPObj *obj, uint16_t vid)
{
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_CHILD_ASIDE);
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_CHILD);
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER);
}

/*!
 * @brief Take an empty replacement folder away and move the folder at aside
 *        back to its path
 */
static void restore_folder(const char *path, const char *aside)
{
    rmdir(path);
    rename(aside, path);
}

/*!
 * @brief End a test: the moved folder comes back, and the test folder and
 *        its child are removed
 *
 * @param[in] obj  the test session
 * @param[in] vid  the volume's id
 * @param[in] fx   the fixture
 */
static void dc_teardown(AFPObj *obj, uint16_t vid, const struct dc_fixture *fx)
{
    obj->options.dircache_validation_freq = fx->freq;

    if (fx->moved) {
        restore_folder(fx->path, fx->aside);
    }

    remove_pair(obj, vid);
}

/*!
 * @brief The test folder at the volume root holding one child, a folder or a
 *        file, both made through the requests a client sends, so both are
 *        cached; the working directory is the volume root afterwards
 *
 * @param[in]     obj     the test session
 * @param[in]     vol     the open volume
 * @param[in]     vid     the volume's id
 * @param[in]     folder  1 for a folder child, 0 for a file
 * @param[in,out] fx      takes the folder's and the child's CNIDs
 * @returns the child's entry, or NULL when the setup fails
 */
static struct dir *cached_pair(AFPObj *obj, const struct vol *vol,
                               uint16_t vid, int folder,
                               struct dc_fixture *fx)
{
    char child[] = DC_CHILD;
    const struct dir *parent;
    struct dir *entry;
    int err;

    if (createdir(obj, vid, DIRDID_ROOT, DC_FOLDER) != AFP_OK) {
        return NULL;
    }

    fx->did = curdir->d_did;
    err = folder ? createdir(obj, vid, fx->did, child)
          : createfile(obj, vid, fx->did, child);

    if (err != AFP_OK || movecwd(vol, vol->v_root) != 0
            || (parent = dirlookup(vol, fx->did)) == NULL
            || (entry = dircache_search_by_name(vol, parent, child,
                                                strlen(child))) == NULL) {
        return NULL;
    }

    fx->child_id = entry->d_did;
    return entry;
}

/*!
 * @brief Put another file under path: one is made at aside and renamed over
 *        it
 *
 * @returns 0, -1 when a call fails
 */
static int replace_file(const char *path, const char *aside)
{
    return create_disk_file(aside) != 0 || rename(aside, path) != 0 ? -1 : 0;
}

/*!
 * @brief Put another folder under path: the folder moves to aside and a new
 *        one is made at its path; a folder that cannot be made moves back
 *
 * @returns 0, -1 when a call fails and the folder is where it was
 */
static int replace_folder(const char *path, const char *aside)
{
    if (rename(path, aside) != 0) {
        return -1;
    }

    if (mkdir(path, 0750) != 0) { // NOSONAR (the scenario under test)
        rename(aside, path);
        return -1;
    }

    return 0;
}

/*!
 * @brief The test folder replaced on disk by another folder of its name,
 *        its entry cached; with a child, a cached folder child and the
 *        working directory at the volume root, without one the working
 *        directory in the folder
 *
 * @param[in]     obj    the test session
 * @param[in]     vol    the open volume
 * @param[in]     vid    the volume's id
 * @param[in]     child  1 for a folder child, 0 for none
 * @param[in,out] fx     takes the CNIDs and the folder's move
 * @returns the folder's entry, or NULL when the setup fails
 */
static struct dir *replaced_folder(AFPObj *obj, const struct vol *vol,
                                   uint16_t vid, int child,
                                   struct dc_fixture *fx)
{
    if (child) {
        if (cached_pair(obj, vol, vid, 1, fx) == NULL) {
            return NULL;
        }
    } else if (createdir(obj, vid, DIRDID_ROOT, DC_FOLDER) != AFP_OK) {
        return NULL;
    } else {
        fx->did = curdir->d_did;
    }

    if (replace_folder(fx->path, fx->aside) != 0) {
        return NULL;
    }

    fx->moved = 1;
    return dirlookup(vol, fx->did);
}

/*!
 * @brief A path that names a folder and steps up to its parent leaves the
 *        parent's entry with the parent's own inode: the request stats the
 *        parent for itself
 */
int utest_dircache_step_up_keeps_entry(AFPObj *obj, struct vol *vol,
                                       uint16_t vid)
{
    struct dc_fixture fx;
    struct stat st;
    const struct dir *parent;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL) {
        goto out;
    }

    /* "/" is a NUL on the wire: the path names the child and steps up; the
     * attribute bit reads the AppleDouble metadata against the stat held */
    if (getfiledirparms_bitmap(obj, vid, fx.did, DC_CHILD "//", 0,
                               (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_DID))
            != AFP_OK
            || stat(fx.path, &st) != 0
            || (parent = dirlookup(vol, fx.did)) == NULL
            || parent->dcache_ino != st.st_ino) {
        goto out;
    }

    ret = 0;
out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPGetFileDirParms of a cached file that another file replaced
 *        answers for the new file under its own CNID, the sibling sessions
 *        hear of it, and the old CNID names nothing
 */
int utest_dircache_replaced_file_by_name(AFPObj *obj, struct vol *vol,
                                         uint16_t vid)
{
    const uint16_t bitmap = (1 << FILPBIT_ATTR) | (1 << FILPBIT_FNUM);
    struct dc_fixture fx;
    char child[] = DC_CHILD;
    struct stat st;
    const struct dir *parent;
    const struct dir *entry;
    cnid_t old;
    int fds[2];
    int gone;
    int rc;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the first request loads the file's AppleDouble state into its entry,
     * which the request after the replacement compares with its stat */
    if (cached_pair(obj, vol, vid, 0, &fx) == NULL
            || getfiledirparms_bitmap(obj, vid, fx.did, child, bitmap, 0)
            != AFP_OK
            || (parent = dirlookup(vol, fx.did)) == NULL
            || replace_file(fx.child, fx.child_aside) != 0
            || stat(fx.child, &st) != 0
            || hints_begin(obj, fds) != 0) {
        goto out;
    }

    old = fx.child_id;
    rc = getfiledirparms_bitmap(obj, vid, fx.did, child, bitmap, 0);
    gone = hint_came(fds[0], CACHE_HINT_DELETE, old);
    hints_end(obj, fds);

    if (rc == AFP_OK && gone
            && (entry = dircache_search_by_name(vol, parent, child,
                                                strlen(child))) != NULL
            && entry->d_did != old && entry->dcache_ino == st.st_ino
            && resolveid(obj, vid, old) == AFPERR_NOID) {
        ret = 0;
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A working folder replaced under its name takes its own CNID at the
 *        refresh and its old children leave the cache, at once here, as the
 *        harness runs no idle worker; the old DID names nothing
 */
int utest_dircache_rekeyed_folder_children(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    struct dc_fixture fx;
    struct stat st;
    struct dir *parent;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if ((parent = replaced_folder(obj, vol, vid, 1, &fx)) == NULL
            || stat(fx.path, &st) != 0 || movecwd(vol, parent) != 0) {
        goto out;
    }

    dir_stat_refresh(vol, parent, &st);

    if (parent->d_did == fx.did || parent->d_did == CNID_INVALID
            || dircache_search_by_did(vol, fx.child_id) != NULL
            || dirlookup(vol, fx.did) != NULL || afp_errno != AFPERR_NOOBJ) {
        goto out;
    }

    ret = 0;
out:
    /* illumos and Solaris refuse to remove the working directory */
    movecwd(vol, vol->v_root);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief Validation of a replaced working folder re-keys it in place: the
 *        lookup of its old DID answers kFPObjectNotFound, and the entry stays
 *        the working directory under the folder's own CNID
 */
int utest_dircache_replaced_curdir_rekeyed(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    struct dc_fixture fx;
    char name[] = DC_FOLDER;
    struct stat st;
    const struct dir *entry;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if ((entry = replaced_folder(obj, vol, vid, 0, &fx)) == NULL
            || entry != curdir || stat(fx.path, &st) != 0) {
        goto out;
    }

    /* the lookup validates the entry */
    obj->options.dircache_validation_freq = 1;

    if (dirlookup(vol, fx.did) != NULL || afp_errno != AFPERR_NOOBJ
            || curdir != entry || curdir->d_did == fx.did
            || curdir->dcache_ino != st.st_ino
            || dircache_search_by_name(vol, vol->v_root, name, strlen(name))
            != curdir) {
        goto out;
    }

    ret = 0;
out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A request that names a replaced folder by name lets the cached
 *        entry go with its children and answers under the folder's own
 *        CNID; the old DID names nothing
 */
int utest_dircache_replaced_folder_by_name(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    struct dc_fixture fx;
    char name[] = DC_FOLDER;
    struct stat st;
    const struct dir *entry;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (replaced_folder(obj, vol, vid, 1, &fx) == NULL
            || stat(fx.path, &st) != 0) {
        goto out;
    }

    if (getfiledirparms(obj, vid, DIRDID_ROOT, name) != AFP_OK
            || (entry = dircache_search_by_name(vol, vol->v_root, name,
                                                strlen(name))) == NULL
            || entry->d_did == fx.did || entry->dcache_ino != st.st_ino
            || dircache_search_by_did(vol, fx.child_id) != NULL
            || dirlookup(vol, fx.did) != NULL || afp_errno != AFPERR_NOOBJ) {
        goto out;
    }

    ret = 0;
out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A strict lookup of a folder that a file with a row of its own
 *        replaced lets the folder's entry go, where the entry would take the
 *        file's CNID as a folder's
 */
int utest_dircache_folder_became_file(AFPObj *obj, struct vol *vol,
                                      uint16_t vid)
{
    struct dc_fixture fx;
    char child[] = DC_CHILD;
    struct stat st;
    const struct dir *parent;
    const struct dir *entry;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the folder child moves aside, and a file takes its name with the row
     * another session's FPCreateFile adds */
    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || (parent = dirlookup(vol, fx.did)) == NULL
            || rename(fx.child, fx.child_aside) != 0
            || create_disk_file(fx.child) != 0 || stat(fx.child, &st) != 0
            || cnid_add(vol->v_cdb, &st, fx.did, child, strlen(child),
                        CNID_INVALID) == CNID_INVALID) {
        goto out;
    }

    if (dirlookup_strict(vol, fx.child_id) == NULL
            && ((entry = dircache_search_by_name(vol, parent, child,
                                                 strlen(child))) == NULL
                || (entry->d_flags & DIRF_ISFILE))) {
        ret = 0;
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A working folder that a file replaced leaves the cache with its
 *        row: the working folder goes to the folder's parent, and the
 *        folder's CNID names no object
 */
int utest_dircache_working_folder_became_file(AFPObj *obj, struct vol *vol,
                                              uint16_t vid)
{
    struct dc_fixture fx;
    struct dir *child;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the folder child is the working folder when a file takes its name */
    if ((child = cached_pair(obj, vol, vid, 1, &fx)) == NULL
            || movecwd(vol, child) != 0
            || rename(fx.child, fx.child_aside) != 0
            || create_disk_file(fx.child) != 0) {
        goto out;
    }

    /* the lookup by the folder's CNID validates its entry */
    obj->options.dircache_validation_freq = 1;

    if (dirlookup(vol, fx.child_id) == NULL && curdir->d_did == fx.did
            && !(curdir->d_flags & DIRF_ISFILE)) {
        ret = 0;
    }

out:
    unlink(fx.child);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A folder whose name a file with a row of its own took leaves the
 *        cache with its row: the folder's CNID names no object, and the
 *        file keeps its own row
 */
int utest_dircache_rowed_file_takes_folder_name(AFPObj *obj, struct vol *vol,
                                                uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    static char file[] = "dc-file";
    struct dc_fixture fx;
    char from[MAXPATHLEN + 1];
    struct stat st;
    cnid_t row = CNID_INVALID;
    cnid_t gone;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    folder_path(vol, from, file);

    /* the folder moves aside, and a file with its row moves onto its name */
    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || create_disk_file(from) != 0 || stat(from, &st) != 0
            || (row = cnid_add(vol->v_cdb, &st, fx.did, file, strlen(file),
                               CNID_INVALID)) == CNID_INVALID
            || rename(fx.child, fx.child_aside) != 0
            || rename(from, fx.child) != 0) { // NOSONAR (the scenario under test)
        goto out;
    }

    /* the lookup by the folder's CNID validates its entry */
    obj->options.dircache_validation_freq = 1;
    gone = fx.child_id;

    /* the validation, not dirlookup()'s check, deletes the folder's row */
    if (dircache_search_by_did(vol, fx.child_id) == NULL
            && cnid_resolve(vol->v_cdb, &gone, buf, sizeof(buf)) == NULL
            && CNID_ERRNO() == CNID_ERR_NOTFOUND
            && dirlookup(vol, fx.child_id) == NULL
            && cnid_resolve(vol->v_cdb, &row, buf, sizeof(buf)) != NULL) {
        ret = 0;
    }

out:
    unlink(from);
    unlink(fx.child);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A cached folder that another session renamed keeps its row when a
 *        file takes its old name: the folder's CNID still finds it
 */
int utest_dircache_renamed_folder_keeps_row(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    static char moved[] = "dc-moved";
    struct dc_fixture fx;
    char to[MAXPATHLEN + 1];
    struct stat st;
    const struct dir *found;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    folder_path(vol, to, moved);

    /* another session's FPMoveAndRename moves the folder and its row, and a
     * file takes the old name before the session's hint arrives */
    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || rename(fx.child, to) != 0 || stat(to, &st) != 0
            || cnid_update(vol->v_cdb, fx.child_id, &st, fx.did, moved,
                           strlen(moved)) != 0
            || create_disk_file(fx.child) != 0) {
        goto out;
    }

    /* the lookup by the folder's CNID validates its entry at the old path */
    obj->options.dircache_validation_freq = 1;
    found = dirlookup(vol, fx.child_id);

    if (found != NULL && !(found->d_flags & DIRF_ISFILE)
            && strcmp(cfrombstr(found->d_fullpath), to) == 0) {
        ret = 0;
    }

out:
    unlink(fx.child);
    delete (obj, vid, fx.did, moved);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A cnid_get() that fails as a backend that is gone fails
 */
static cnid_t gone_get(struct _cnid_db *cdb _U_, cnid_t did _U_,
                       const char *name _U_, size_t len _U_)
{
    errno = CNID_ERR_DB;
    return CNID_INVALID;
}

/*!
 * @brief A folder whose name a file took keeps its row when the database
 *        cannot answer the lookup: the folder's CNID still names the path
 */
int utest_dircache_unanswered_kind_keeps_row(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    struct dc_fixture fx;
    cnid_t (*get)(struct _cnid_db *, cnid_t, const char *, size_t) =
        vol->v_cdb->cnid_get;
    char child[] = DC_CHILD;
    struct stat st;
    struct dir *entry;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the folder child moves aside, and a file takes its name */
    if ((entry = cached_pair(obj, vol, vid, 1, &fx)) == NULL
            || rename(fx.child, fx.child_aside) != 0
            || create_disk_file(fx.child) != 0 || stat(fx.child, &st) != 0) {
        goto out;
    }

    vol->v_cdb->cnid_get = gone_get;
    dir_stat_refresh(vol, entry, &st);
    vol->v_cdb->cnid_get = get;

    /* the kind check let the entry go */
    if (dircache_search_by_did(vol, fx.child_id) == NULL
            && cnid_get(vol->v_cdb, fx.did, child, strlen(child))
            == fx.child_id) {
        ret = 0;
    }

out:
    unlink(fx.child);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A folder's entry that a file's stat at its inode lets go drops its
 *        EA cache, so a request still holding it reads none of the folder's
 *        names for the file, and leaves the row, which the database takes
 *        for the file's at that inode
 */
int utest_dircache_kind_change_drops_ea(AFPObj *obj, struct vol *vol,
                                        uint16_t vid)
{
    static const char names[] = "user.dc";
    char child[] = DC_CHILD;
    struct dc_fixture fx;
    struct dir *entry;
    struct stat st;
    size_t len;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the folder child holds EA names, and a stat shows a file at its inode
     * a second later, as where the filesystem keeps whole seconds and
     * reuses inodes */
    if ((entry = cached_pair(obj, vol, vid, 1, &fx)) == NULL
            || stat(fx.child, &st) != 0) {
        goto out;
    }

    ea_cache_store_names(entry, names, sizeof(names));
    st.st_mode = S_IFREG | 0640;
    st.st_ctime++;

    if (dir_stat_refresh(vol, entry, &st) == AFPERR_NOOBJ
            && ea_cache_names(entry, &len) == NULL
            && cnid_get(vol->v_cdb, fx.did, child, strlen(child))
            == fx.child_id) {
        ret = 0;
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief An entry already removed takes no stat: the refresh answers that it
 *        holds none and sends no hint
 */
int utest_dircache_removed_entry_takes_nothing(AFPObj *obj, struct vol *vol,
                                               uint16_t vid)
{
    struct dc_fixture fx;
    struct dir *entry;
    struct stat st;
    int fds[2];
    int hinted;
    int rc;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the folder child leaves the cache, and a stat of its path shows
     * another ctime */
    if ((entry = cached_pair(obj, vol, vid, 1, &fx)) == NULL
            || stat(fx.child, &st) != 0 || hints_begin(obj, fds) != 0) {
        goto out;
    }

    dir_remove(vol, entry, 0);
    st.st_ctime++;
    rc = dir_stat_refresh(vol, entry, &st);
    hinted = hint_came(fds[0], CACHE_HINT_DELETE_CHILDREN, CNID_INVALID);
    hints_end(obj, fds);
    ret = rc == AFPERR_NOOBJ && !hinted ? 0 : -1;
out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief The working folder's entry goes while its CNID's row leads to a
 *        file: the session stays in a folder, whether the entry goes alone,
 *        below its parent, or with its parent's row leading to a file too
 */
int utest_dircache_curdir_never_file(AFPObj *obj, struct vol *vol,
                                     uint16_t vid)
{
    static char file[] = "dc-file";
    static char top[] = "dc-top";
    char child[] = DC_CHILD;
    char parent[] = DC_FOLDER;
    struct dc_fixture fx;
    char path[MAXPATHLEN + 1];
    char tpath[MAXPATHLEN + 1];
    struct stat fst;
    struct stat cst;
    struct stat pst;
    struct stat tst;
    struct dir *entry;
    struct dir *folder;
    int moved = 0;
    int pmoved = 0;
    int alone = 0;
    int below = 0;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    folder_path(vol, path, file);
    root_path(vol, tpath, top);

    if ((entry = cached_pair(obj, vol, vid, 1, &fx)) == NULL
            || stat(cfrombstr(entry->d_fullpath), &cst) != 0
            || create_disk_file(path) != 0 || stat(path, &fst) != 0
            || movecwd(vol, entry) != 0) {
        goto out;
    }

    /* the child's row names a file, as another session's change left it */
    moved = cnid_update(vol->v_cdb, fx.child_id, &fst, fx.did, file,
                        strlen(file)) == 0;

    if (!moved) {
        goto out;
    }

    dir_remove(vol, entry, 0);
    alone = curdir->d_did == fx.did && !(curdir->d_flags & DIRF_ISFILE);

    /* and again through the children's scan of its parent */
    if (cnid_update(vol->v_cdb, fx.child_id, &cst, fx.did, child,
                    strlen(child)) != 0
            || (entry = dirlookup(vol, fx.child_id)) == NULL
            || movecwd(vol, entry) != 0
            || cnid_update(vol->v_cdb, fx.child_id, &fst, fx.did, file,
                           strlen(file)) != 0
            || (folder = dirlookup(vol, fx.did)) == NULL) {
        goto out;
    }

    dir_remove_tree(vol, folder, 0);
    below = !(curdir->d_flags & DIRF_ISFILE);

    /* and through the fallback to the parent, whose own row names a file
     * and whose entry has left the cache */
    if (cnid_update(vol->v_cdb, fx.child_id, &cst, fx.did, child,
                    strlen(child)) != 0
            || (entry = dirlookup(vol, fx.child_id)) == NULL
            || movecwd(vol, entry) != 0 || stat(fx.path, &pst) != 0
            || create_disk_file(tpath) != 0 || stat(tpath, &tst) != 0
            || cnid_update(vol->v_cdb, fx.child_id, &fst, fx.did, file,
                           strlen(file)) != 0) {
        goto out;
    }

    pmoved = cnid_update(vol->v_cdb, fx.did, &tst, DIRDID_ROOT, top,
                         strlen(top)) == 0;

    if (!pmoved || (folder = dircache_lookup_did(vol, fx.did)) == NULL) {
        goto out;
    }

    dir_remove(vol, folder, 0);
    dir_remove(vol, entry, 0);

    if (alone && below && !(curdir->d_flags & DIRF_ISFILE)) {
        ret = 0;
    }

out:

    if (pmoved) {
        cnid_update(vol->v_cdb, fx.did, &pst, DIRDID_ROOT, parent,
                    strlen(parent));
    }

    if (moved) {
        cnid_update(vol->v_cdb, fx.child_id, &cst, fx.did, child,
                    strlen(child));
    }

    unlink(tpath);
    unlink(path);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A request that names a cached file whose name a folder with a row
 *        of its own now holds lets the file's entry go, where the entry
 *        would take the folder's CNID as a file's, tells the sibling
 *        sessions, and answers for the folder
 */
int utest_dircache_file_became_folder(AFPObj *obj, struct vol *vol,
                                      uint16_t vid)
{
    struct dc_fixture fx;
    char child[] = DC_CHILD;
    struct stat st;
    const struct dir *parent;
    const struct dir *entry;
    int fds[2];
    int gone;
    int rc;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the file child moves aside, and a folder takes its name with the row
     * another session's FPCreateDir adds */
    if (cached_pair(obj, vol, vid, 0, &fx) == NULL
            || (parent = dirlookup(vol, fx.did)) == NULL
            || rename(fx.child, fx.child_aside) != 0
            || mkdir(fx.child, 0750) != 0 // NOSONAR (the scenario under test)
            || stat(fx.child, &st) != 0
            || cnid_add(vol->v_cdb, &st, fx.did, child, strlen(child),
                        CNID_INVALID) == CNID_INVALID
            || hints_begin(obj, fds) != 0) {
        goto out;
    }

    rc = getfiledirparms(obj, vid, fx.did, child);
    gone = hint_came(fds[0], CACHE_HINT_DELETE, fx.child_id);
    hints_end(obj, fds);

    if (rc == AFP_OK && gone
            && (entry = dircache_search_by_name(vol, parent, child,
                                                strlen(child))) != NULL
            && !(entry->d_flags & DIRF_ISFILE)
            && entry->d_did != fx.child_id) {
        ret = 0;
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPGetFileDirParms of a file under a cached folder's name lets the
 *        folder's entry go with its children
 */
int utest_dircache_file_takes_folder_name(AFPObj *obj, struct vol *vol,
                                          uint16_t vid)
{
    struct dc_fixture fx;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the folder moves aside with its child, and a file takes its name */
    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || rename(fx.path, fx.aside) != 0) {
        goto out;
    }

    fx.moved = 1;

    /* a long name alone reads no metadata, so the file's identity check
     * meets the folder's entry */
    if (create_disk_file(fx.path) == 0
            && getfiledirparms_bitmap(obj, vid, DIRDID_ROOT, DC_FOLDER,
                                      1 << FILPBIT_LNAME, 0) == AFP_OK
            && dircache_lookup_did(vol, fx.did) == NULL
            && dircache_lookup_did(vol, fx.child_id) == NULL) {
        ret = 0;
    }

out:
    unlink(fx.path);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPGetFileDirParms of a working folder replaced under its name
 *        answers kFPObjectNotFound, not the new folder's parameters under
 *        the old DID
 */
int utest_dircache_replaced_folder_params_fail(AFPObj *obj, struct vol *vol,
                                               uint16_t vid)
{
    struct dc_fixture fx;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* an empty path names the folder itself */
    if (replaced_folder(obj, vol, vid, 0, &fx) != NULL
            && getfiledirparms(obj, vid, fx.did, "") == AFPERR_NOOBJ) {
        ret = 0;
    }

    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPAccess on a working folder replaced under its name answers
 *        kFPObjectNotFound, not the new folder's rights under the old DID;
 *        skipped where the request reaches no readable ACL rights
 */
int utest_dircache_replaced_access_fails(AFPObj *obj, struct vol *vol,
                                         uint16_t vid)
{
    struct dc_fixture fx;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the rights check answers -1 where no ACL rights can be read */
    if (checkaccess(obj, vid, DIRDID_ROOT, "", DARWIN_ACE_LIST_DIRECTORY)
            == -1) {
        ret = TEST_SKIP;
    } else if (replaced_folder(obj, vol, vid, 0, &fx) != NULL
               && checkaccess(obj, vid, fx.did, "", DARWIN_ACE_LIST_DIRECTORY)
               == AFPERR_NOOBJ) {
        ret = 0;
    }

    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPCreateFile under the DID of a working folder replaced under its
 *        name answers kFPObjectNotFound: the strict lookup brings the entry
 *        to the folder at the path, which takes its own CNID
 */
int utest_dircache_replaced_folder_create_fails(AFPObj *obj, struct vol *vol,
                                                uint16_t vid)
{
    struct dc_fixture fx;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (replaced_folder(obj, vol, vid, 0, &fx) != NULL
            && createfile(obj, vid, fx.did, DC_CHILD) == AFPERR_NOOBJ) {
        ret = 0;
    }

    /* the file a server that answers the request makes */
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_CHILD);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A folder gone behind the cache takes its children with it when
 *        validation expunges it
 */
int utest_dircache_gone_folder_children(AFPObj *obj, struct vol *vol,
                                        uint16_t vid)
{
    struct dc_fixture fx;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || rename(fx.path, fx.aside) != 0) {
        goto out;
    }

    fx.moved = 1;
    /* the lookup of the gone folder validates it; the child's does not */
    obj->options.dircache_validation_freq = 1;

    if (dirlookup(vol, fx.did) != NULL) {
        goto out;
    }

    obj->options.dircache_validation_freq = INT_MAX;

    if (dircache_search_by_did(vol, fx.child_id) == NULL) {
        ret = 0;
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A sibling's refresh hint for a folder gone behind the cache takes
 *        the folder's children with it
 */
int utest_dircache_hint_gone_folder_children(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    struct dc_fixture fx;
    int fds[2];
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || rename(fx.path, fx.aside) != 0) {
        goto out;
    }

    fx.moved = 1;

    /* the hint reaches the session as the parent afpd relays it */
    if (hints_begin(obj, fds) != 0) {
        goto out;
    }

    ipc_send_cache_hint(obj, vol->v_vid, fx.did, CACHE_HINT_REFRESH);
    obj->hint_fd = fds[0];
    process_cache_hints(obj);
    obj->hint_fd = -1;
    hints_end(obj, fds);

    if (dircache_lookup_did(vol, fx.did) == NULL
            && dircache_lookup_did(vol, fx.child_id) == NULL) {
        ret = 0;
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A listing of a folder replaced under its name, with a file of the
 *        same name, answers kFPDirNotFound, the folder taking its own CNID
 *        at the listing's stat; the old folder's file leaves the cache, and
 *        the folder, found by name under its own CNID, lists the new file
 *        from its own stat
 */
int utest_dircache_replaced_folder_listing(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    struct dc_fixture fx;
    char name[] = DC_FOLDER;
    char child[] = DC_CHILD;
    char inner[MAXPATHLEN + 1] = "";
    struct stat st;
    const struct dir *entry;
    const struct dir *file;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    folder_path(vol, inner, child);

    if (cached_pair(obj, vol, vid, 0, &fx) == NULL
            || replace_folder(fx.path, fx.aside) != 0) {
        goto out;
    }

    fx.moved = 1;

    /* another folder of the same name, with a file of the same name */
    if (create_disk_file(inner) != 0 || stat(inner, &st) != 0) {
        goto out;
    }

    /* the listing's stat gives the folder its own CNID and the batch answers
     * kFPDirNotFound; the old DID names nothing */
    if (enumerate_range(obj, vid, fx.did, 1 << FILPBIT_FNUM, 0, 1, 40)
            != AFPERR_NODIR
            || dirlookup(vol, fx.did) != NULL || afp_errno != AFPERR_NOOBJ) {
        goto out;
    }

    /* by name the folder answers under its own CNID, the old folder's file
     * left the cache with it, and the folder's listing serves the new file
     * from its own stat */
    if (getfiledirparms(obj, vid, DIRDID_ROOT, name) != AFP_OK
            || (entry = dircache_search_by_name(vol, vol->v_root, name,
                                                strlen(name))) == NULL
            || entry->d_did == fx.did
            || dircache_lookup_did(vol, fx.child_id) != NULL
            || enumerate_range(obj, vid, entry->d_did, 1 << FILPBIT_FNUM, 0,
                               1, 40) != AFP_OK
            || (file = dircache_search_by_name(vol, entry, child,
                                               strlen(child))) == NULL
            || file->dcache_ino != st.st_ino) {
        goto out;
    }

    ret = 0;
out:
    unlink(inner);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A folder two levels below a renamed folder, whose path the rename
 *        left stale, with other folders at the old paths: a stat of the
 *        object at its path leaves its stat fields and the date it serves,
 *        and a strict lookup finds it where the rename took it, leaves the
 *        other folders' rows alone and sends no refresh hint
 */
int utest_dircache_stale_child_path_kept(AFPObj *obj, struct vol *vol,
                                         uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    struct dc_fixture fx;
    char upath[] = DC_ASIDE;
    const struct dir_modify_args rename_args = {
        .flags = DCMOD_PATH,
        .new_uname = upath,
        .new_pdir_path = vol->v_root->d_fullpath
    };
    struct stat later;
    const struct dir_modify_args stat_args = {
        .flags = DCMOD_STAT,
        .st = &later
    };
    struct timespec times[2] = {
        { .tv_nsec = UTIME_OMIT }, { .tv_nsec = 0 }
    };
    char moved[MAXPATHLEN + 1];
    char made[MAXPATHLEN + 1];
    char mdate[4];
    struct stat st;
    struct dir *folder;
    struct dir *entry;
    const struct dir *leaf;
    cnid_t leaf_id;
    cnid_t other;
    ino_t ino;
    int fds[2];
    int refreshed;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    snprintf(moved, sizeof(moved), "%s/" DC_CHILD "/" DC_LEAF, fx.aside);
    snprintf(made, sizeof(made), "%s/" DC_CHILD "/" DC_LEAF, fx.path);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || createdir(obj, vid, fx.child_id, DC_LEAF) != AFP_OK) {
        goto out;
    }

    leaf_id = curdir->d_did;
    ino = curdir->dcache_ino;

    if (movecwd(vol, vol->v_root) != 0
            || (folder = dirlookup(vol, fx.did)) == NULL
            || stat(fx.path, &st) != 0) {
        goto out;
    }

    /* the folder moves aside, and another folder takes its path */
    if (replace_folder(fx.path, fx.aside) != 0) {
        goto out;
    }

    fx.moved = 1;

    /* its row and entry follow as FPMoveAndRename moves them, before the
     * removal of its children ran, and other folders with rows take the
     * child's and the leaf's old paths */
    if (cnid_update(vol->v_cdb, fx.did, &st, DIRDID_ROOT, upath,
                    strlen(upath)) != 0
            || dir_modify(vol, folder, &rename_args) != 0
            || createdir(obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_CHILD)
            != AFP_OK
            || createdir(obj, vid, DIRDID_ROOT,
                         DC_FOLDER "/" DC_CHILD "/" DC_LEAF) != AFP_OK) {
        goto out;
    }

    other = curdir->d_did;

    if (movecwd(vol, vol->v_root) != 0
            || (entry = dircache_lookup_did(vol, leaf_id)) == NULL
            || stat(made, &later) != 0) {
        goto out;
    }

    /* the entry takes nothing of the stat: it keeps its CNID, its inode
     * and its stat fields, and with a known AppleDouble state the date it
     * serves, which the stat's later mtime would move */
    later.st_mtime += 3600;
    entry->dcache_rlen = AD_RLEN_NO_RFORK;
    memcpy(mdate, entry->dcache_filedatesi + AD_DATE_MODIFY, sizeof(mdate));

    if (dir_modify(vol, entry, &stat_args) != 0 || entry->d_did != leaf_id
            || entry->dcache_ino != ino
            || entry->dcache_mtime == later.st_mtime
            || memcmp(mdate, entry->dcache_filedatesi + AD_DATE_MODIFY,
                      sizeof(mdate)) != 0) {
        goto out;
    }

    /* nor does a sampled validation, which stats the object there */
    times[1].tv_sec = later.st_mtime;
    obj->options.dircache_validation_freq = 1;
    leaf = utimensat(AT_FDCWD, made, times, 0) == 0
           ? dircache_search_by_did(vol, leaf_id) : NULL;
    obj->options.dircache_validation_freq = INT_MAX;

    if (leaf != entry || entry->dcache_mtime == later.st_mtime
            || hints_begin(obj, fds) != 0) {
        goto out;
    }

    leaf = dirlookup_strict(vol, leaf_id);
    refreshed = hint_came(fds[0], CACHE_HINT_REFRESH, CNID_INVALID);
    hints_end(obj, fds);

    if (leaf != NULL && leaf->dcache_ino == ino
            && strcmp(cfrombstr(leaf->d_fullpath), moved) == 0
            && cnid_resolve(vol->v_cdb, &other, buf, sizeof(buf)) != NULL
            && !refreshed) {
        ret = 0;
    }

out:
    /* the moved folders go where they went, the others with the teardown */
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_CHILD "/" DC_LEAF);
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_CHILD);
    delete (obj, vid, DIRDID_ROOT, DC_ASIDE "/" DC_CHILD "/" DC_LEAF);
    delete (obj, vid, DIRDID_ROOT, DC_ASIDE "/" DC_CHILD);
    delete (obj, vid, DIRDID_ROOT, DC_ASIDE);
    fx.moved = 0;
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief The test folder and its cached child folder moved aside with the
 *        folder's row, as another session's FPMoveAndRename moves them,
 *        with another folder and child at the old paths; this session's
 *        entries keep the old paths
 *
 * @returns 0, -1 when the setup fails
 */
static int moved_pair(AFPObj *obj, const struct vol *vol, uint16_t vid,
                      struct dc_fixture *fx)
{
    char upath[] = DC_ASIDE;
    char made[MAXPATHLEN + 1];
    struct stat st;
    snprintf(made, sizeof(made), "%s/" DC_CHILD, fx->path);

    if (cached_pair(obj, vol, vid, 1, fx) == NULL || stat(fx->path, &st) != 0
            || replace_folder(fx->path, fx->aside) != 0) {
        return -1;
    }

    fx->moved = 1;
    return cnid_update(vol->v_cdb, fx->did, &st, DIRDID_ROOT, upath,
                       strlen(upath)) == 0
           && mkdir(made, 0750) == 0 ? 0 : -1; // NOSONAR (the scenario under test)
}

/*!
 * @brief Whether a strict lookup finds moved_pair()'s child where the
 *        rename took it, its row kept
 */
static int moved_child_found(const struct vol *vol,
                             const struct dc_fixture *fx)
{
    static char buf[CNID_MAX_PATH_LEN];
    char moved[MAXPATHLEN + 1];
    const struct dir *child;
    cnid_t id = fx->child_id;
    snprintf(moved, sizeof(moved), "%s/" DC_CHILD, fx->aside);
    return (child = dirlookup_strict(vol, fx->child_id)) != NULL
           && strcmp(cfrombstr(child->d_fullpath), moved) == 0
           && cnid_resolve(vol->v_cdb, &id, buf, sizeof(buf)) != NULL;
}

/*!
 * @brief End a moved_pair() test: the other child goes, and the moved pair
 *        goes where it went
 */
static void moved_pair_end(AFPObj *obj, uint16_t vid, struct dc_fixture *fx)
{
    char made[MAXPATHLEN + 1];
    snprintf(made, sizeof(made), "%s/" DC_CHILD, fx->path);
    rmdir(made);
    delete (obj, vid, DIRDID_ROOT, DC_ASIDE "/" DC_CHILD);
    delete (obj, vid, DIRDID_ROOT, DC_ASIDE);
    fx->moved = 0;
    dc_teardown(obj, vid, fx);
}

/*!
 * @brief A strict lookup of a child whose folder another session renamed,
 *        with another folder and child at the old paths, finds the child
 *        where the rename took it and keeps its row
 */
int utest_dircache_moved_parent_keeps_rows(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    struct dc_fixture fx;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (moved_pair(obj, vol, vid, &fx) == 0 && moved_child_found(vol, &fx)) {
        ret = 0;
    }

    moved_pair_end(obj, vid, &fx);
    return ret;
}

/*!
 * @brief The same with the folder's entry out of the cache, as a ghost's
 *        is to the lookup: the child's path cannot be vouched for, and its
 *        row stays
 */
int utest_dircache_uncached_parent_keeps_rows(AFPObj *obj, struct vol *vol,
                                              uint16_t vid)
{
    struct dc_fixture fx;
    struct dir *folder;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (moved_pair(obj, vol, vid, &fx) == 0
            && (folder = dircache_lookup_did(vol, fx.did)) != NULL) {
        dir_remove(vol, folder, 0);

        if (moved_child_found(vol, &fx)) {
            ret = 0;
        }
    }

    moved_pair_end(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A catalog search by name through the CNID database leaves out the
 *        child whose cached path holds the other child, and answers
 */
int utest_dircache_search_skips_moved_child(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    struct dc_fixture fx;
    /* the UTF-8 name's offset past the record's fixed part, then an
     * encoding hint, the name's length and its bytes */
    unsigned char spec[12 + sizeof(DC_CHILD) - 1] = { 0, 6 };
    char lname[MACFILELEN + 1];
    const int searchdb = vol->v_flags & AFPVOL_SEARCHDB;
    uint32_t nrecs = 1;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    spec[11] = sizeof(DC_CHILD) - 1;
    memcpy(spec + 12, DC_CHILD, sizeof(DC_CHILD) - 1);
    vol->v_flags |= AFPVOL_SEARCHDB;

    if (moved_pair(obj, vol, vid, &fx) == 0
            && catsearch_ext(obj, vid, 1 << DIRPBIT_LNAME,
                             1U << FILPBIT_PDINFO, spec, spec, sizeof(spec),
                             &nrecs, lname) == AFPERR_EOF
            && nrecs == 0) {
        ret = 0;
    }

    vol->v_flags = (vol->v_flags & ~AFPVOL_SEARCHDB) | searchdb;
    moved_pair_end(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPResolveID of a file in the child whose cached path holds the
 *        other child answers kFPIDNotFound, and gives the other child's
 *        files no rows under the child's CNID
 */
int utest_dircache_resolve_moved_child_fails(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    static char file[] = "dc-file";
    static char other[] = "dc-other";
    struct dc_fixture fx;
    char path[MAXPATHLEN + 1];
    char made[MAXPATHLEN + 1];
    struct stat st;
    cnid_t id;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    snprintf(path, sizeof(path), "%s/" DC_CHILD "/%s", fx.aside, file);
    snprintf(made, sizeof(made), "%s/" DC_CHILD "/%s", fx.path, other);

    /* the moved child holds a file with a row, the other child a file */
    if (moved_pair(obj, vol, vid, &fx) == 0 && create_disk_file(path) == 0
            && stat(path, &st) == 0
            && (id = cnid_add(vol->v_cdb, &st, fx.child_id, file,
                              strlen(file), CNID_INVALID)) != CNID_INVALID
            && create_disk_file(made) == 0
            && resolveid(obj, vid, id) == AFPERR_NOID
            && cnid_get(vol->v_cdb, fx.child_id, other, strlen(other))
            == CNID_INVALID) {
        ret = 0;
    }

    /* the other child goes first, so the moved child's stale path heals;
     * illumos and Solaris refuse to remove the working directory, where
     * FPResolveID left the session */
    unlink(made);
    snprintf(made, sizeof(made), "%s/" DC_CHILD, fx.path);
    movecwd(vol, vol->v_root);
    rmdir(made);
    snprintf(path, sizeof(path), DC_ASIDE "/" DC_CHILD "/%s", file);
    delete (obj, vid, DIRDID_ROOT, path);
    moved_pair_end(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A request by the folder's CNID and its child's name keeps both
 *        rows where the child's entry kept the path the session's rename
 *        left stale, another session moved the child away, and a folder
 *        with a row of its own took the child's name: the request's stat of
 *        that folder stands for neither entry
 */
int utest_dircache_healed_path_keeps_rows(AFPObj *obj, struct vol *vol,
                                          uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    static char moved[] = "dc-moved";
    struct dc_fixture fx;
    char upath[] = DC_ASIDE;
    char child[] = DC_CHILD;
    const struct dir_modify_args rename_args = {
        .flags = DCMOD_PATH,
        .new_uname = upath,
        .new_pdir_path = vol->v_root->d_fullpath
    };
    char made[MAXPATHLEN + 1];
    char to[MAXPATHLEN + 1];
    const char *name;
    struct stat st;
    struct dir *folder;
    cnid_t other = CNID_INVALID;
    cnid_t id;
    int rc;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    snprintf(made, sizeof(made), "%s/" DC_CHILD, fx.aside);
    root_path(vol, to, moved);

    /* the session renames the folder before the removal of its children
     * ran, so the child's entry keeps the old path */
    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || (folder = dirlookup(vol, fx.did)) == NULL
            || stat(fx.path, &st) != 0
            || rename(fx.path, fx.aside) != 0 // NOSONAR (the scenario under test)
            || cnid_update(vol->v_cdb, fx.did, &st, DIRDID_ROOT, upath,
                           strlen(upath)) != 0
            || dir_modify(vol, folder, &rename_args) != 0) {
        goto out;
    }

    /* another session moves the child to the volume root with its row,
     * and makes a folder with a row of its own at the child's name */
    if (rename(made, to) != 0 || stat(to, &st) != 0
            || cnid_update(vol->v_cdb, fx.child_id, &st, DIRDID_ROOT, moved,
                           strlen(moved)) != 0
            || mkdir(made, 0750) != 0 // NOSONAR (the scenario under test)
            || stat(made, &st) != 0
            || (other = cnid_add(vol->v_cdb, &st, fx.did, child,
                                 strlen(child), CNID_INVALID))
            == CNID_INVALID) {
        goto out;
    }

    /* the stale path fails, movecwd() finds the child where it went, and
     * the request answers */
    rc = getfiledirparms(obj, vid, fx.did, DC_CHILD);
    id = fx.child_id;

    if (rc == AFP_OK
            && (name = cnid_resolve(vol->v_cdb, &id, buf, sizeof(buf))) != NULL
            && strcmp(name, moved) == 0) {
        id = other;
        name = cnid_resolve(vol->v_cdb, &id, buf, sizeof(buf));
        ret = name != NULL && strcmp(name, child) == 0 ? 0 : -1;
    }

out:
    /* the folders go where they are, the others with the teardown */
    delete (obj, vid, DIRDID_ROOT, DC_ASIDE "/" DC_CHILD);
    delete (obj, vid, DIRDID_ROOT, DC_ASIDE);
    delete (obj, vid, DIRDID_ROOT, moved);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A request by the folder's CNID and its child's name, whose cached
 *        child kept the path the session's rename left stale while another
 *        folder took that path, enters the child where the rename took it;
 *        the child's entry takes that path, and the child's descriptor
 *        opened at the old path goes
 */
int utest_dircache_stale_name_path_repaired(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    struct dc_fixture fx;
    char upath[] = DC_ASIDE;
    const struct dir_modify_args rename_args = {
        .flags = DCMOD_PATH,
        .new_uname = upath,
        .new_pdir_path = vol->v_root->d_fullpath
    };
    char real[MAXPATHLEN + 1];
    char real_leaf[MAXPATHLEN + 1];
    struct stat st;
    struct stat lst;
    struct stat seen;
    struct stat cwd;
    struct dir *folder;
    struct dir *entry;
    struct dir *leaf;
    cnid_t leaf_id;
    int renamed = 0;
    int rc;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    snprintf(real, sizeof(real), "%s/" DC_CHILD, fx.aside);
    snprintf(real_leaf, sizeof(real_leaf), "%s/" DC_LEAF, real);

    if ((entry = cached_pair(obj, vol, vid, 1, &fx)) == NULL
            || createdir(obj, vid, fx.child_id, DC_LEAF) != AFP_OK) {
        goto out;
    }

    leaf_id = curdir->d_did;

    if (movecwd(vol, vol->v_root) != 0
            || (folder = dirlookup(vol, fx.did)) == NULL
            || stat(fx.path, &st) != 0
            || rename(fx.path, fx.aside) != 0) { // NOSONAR (the scenario under test)
        goto out;
    }

    renamed = 1;

    /* the session renames the folder before the removal of its children
     * ran, and other folders take the old paths; a stat of the folder inside
     * the child opens the child's descriptor at the old path, where the other
     * child holds no such folder */
    if (cnid_update(vol->v_cdb, fx.did, &st, DIRDID_ROOT, upath,
                    strlen(upath)) != 0
            || dir_modify(vol, folder, &rename_args) != 0
            || mkdir(fx.path, 0750) != 0 // NOSONAR (the scenario under test)
            || mkdir(fx.child, 0750) != 0 // NOSONAR (the scenario under test)
            || stat(real, &st) != 0 || stat(real_leaf, &lst) != 0
            || (leaf = dircache_lookup_did(vol, leaf_id)) == NULL
            || pfd_ostat(vol, leaf, &seen, vol_syml_opt(vol)) == 0) {
        goto out;
    }

    rc = getfiledirparms(obj, vid, fx.did, DC_CHILD);

    if (rc == AFP_OK && stat(".", &cwd) == 0 && cwd.st_ino == st.st_ino
            && strcmp(cfrombstr(entry->d_fullpath), real) == 0
            && (leaf = dircache_lookup_did(vol, leaf_id)) != NULL
            && pfd_ostat(vol, leaf, &seen, vol_syml_opt(vol)) == 0
            && seen.st_ino == lst.st_ino) {
        ret = 0;
    }

out:
    /* the other folders go first, then the renamed folders where they went */
    movecwd(vol, vol->v_root);

    if (renamed) {
        rmdir(fx.child);
        rmdir(fx.path);
    } else {
        delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_CHILD "/" DC_LEAF);
    }

    delete (obj, vid, DIRDID_ROOT, DC_ASIDE "/" DC_CHILD "/" DC_LEAF);
    delete (obj, vid, DIRDID_ROOT, DC_ASIDE "/" DC_CHILD);
    delete (obj, vid, DIRDID_ROOT, DC_ASIDE);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A catalog search through the CNID database leaves out a folder
 *        whose cached entry names another place than the folder's row,
 *        where another folder sits, and the row of the folder now at the
 *        entry's place stays
 */
int utest_dircache_search_keeps_other_row(AFPObj *obj, struct vol *vol,
                                          uint16_t vid)
{
    static char moved[] = "dc-new";
    static char kept[] = "dc-kept";
    struct dc_fixture fx;
    char folder[] = DC_FOLDER;
    /* the UTF-8 name's offset past the record's fixed part, then an
     * encoding hint, the name's length and its bytes */
    unsigned char spec[12 + sizeof(moved) - 1] = { 0, 6 };
    char lname[MACFILELEN + 1];
    char to[MAXPATHLEN + 1];
    char aside[MAXPATHLEN + 1];
    const int searchdb = vol->v_flags & AFPVOL_SEARCHDB;
    uint32_t nrecs = 1;
    struct stat st;
    cnid_t other = CNID_INVALID;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    root_path(vol, to, moved);
    root_path(vol, aside, kept);
    spec[11] = sizeof(moved) - 1;
    memcpy(spec + 12, moved, sizeof(moved) - 1);
    vol->v_flags |= AFPVOL_SEARCHDB;

    if (createdir(obj, vid, DIRDID_ROOT, DC_FOLDER) != AFP_OK) {
        goto out;
    }

    fx.did = curdir->d_did;

    /* another session renames the cached folder with its row and makes a
     * folder with a row at the old name; a process then moves the renamed
     * folder aside for another one */
    if (movecwd(vol, vol->v_root) != 0
            || rename(fx.path, to) != 0 || stat(to, &st) != 0
            || cnid_update(vol->v_cdb, fx.did, &st, DIRDID_ROOT, moved,
                           strlen(moved)) != 0
            || mkdir(fx.path, 0750) != 0 // NOSONAR (the scenario under test)
            || stat(fx.path, &st) != 0
            || (other = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, folder,
                                 strlen(folder), CNID_INVALID))
            == CNID_INVALID
            || rename(to, aside) != 0 // NOSONAR (the scenario under test)
            || mkdir(to, 0750) != 0) { // NOSONAR (the scenario under test)
        goto out;
    }

    /* the search by the new name finds the renamed folder's row, whose
     * entry still names the old place */
    if (catsearch_ext(obj, vid, 1 << DIRPBIT_LNAME, 1U << FILPBIT_PDINFO,
                      spec, spec, sizeof(spec), &nrecs, lname) == AFPERR_EOF
            && nrecs == 0
            && cnid_get(vol->v_cdb, DIRDID_ROOT, folder, strlen(folder))
            == other) {
        ret = 0;
    }

out:
    vol->v_flags = (vol->v_flags & ~AFPVOL_SEARCHDB) | searchdb;
    delete (obj, vid, DIRDID_ROOT, kept);
    delete (obj, vid, DIRDID_ROOT, moved);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A catalog search through the CNID database finds a folder that
 *        another session renamed with its row, while the folder's cached
 *        entry still names the old place
 */
int utest_dircache_search_finds_moved_folder(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    static char moved[] = "dc-new";
    struct dc_fixture fx;
    /* the UTF-8 name's offset past the record's fixed part, then an
     * encoding hint, the name's length and its bytes */
    unsigned char spec[12 + sizeof(moved) - 1] = { 0, 6 };
    char lname[MACFILELEN + 1];
    char to[MAXPATHLEN + 1];
    const int searchdb = vol->v_flags & AFPVOL_SEARCHDB;
    uint32_t nrecs = 0;
    struct stat st;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    root_path(vol, to, moved);
    spec[11] = sizeof(moved) - 1;
    memcpy(spec + 12, moved, sizeof(moved) - 1);
    vol->v_flags |= AFPVOL_SEARCHDB;

    if (createdir(obj, vid, DIRDID_ROOT, DC_FOLDER) != AFP_OK) {
        goto out;
    }

    fx.did = curdir->d_did;

    if (movecwd(vol, vol->v_root) == 0
            && rename(fx.path, to) == 0 && stat(to, &st) == 0
            && cnid_update(vol->v_cdb, fx.did, &st, DIRDID_ROOT, moved,
                           strlen(moved)) == 0
            && catsearch_ext(obj, vid, 1 << DIRPBIT_LNAME,
                             1U << FILPBIT_PDINFO, spec, spec, sizeof(spec),
                             &nrecs, lname) == AFPERR_EOF
            && nrecs == 1) {
        ret = 0;
    }

out:
    vol->v_flags = (vol->v_flags & ~AFPVOL_SEARCHDB) | searchdb;
    delete (obj, vid, DIRDID_ROOT, moved);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A stat at a path the session's rename left stale is not taken, nor
 *        is the AppleDouble metadata that comes with it
 */
int utest_dircache_stale_path_takes_no_metadata(AFPObj *obj, struct vol *vol,
                                                uint16_t vid)
{
    struct dc_fixture fx;
    char upath[] = DC_ASIDE;
    const struct dir_modify_args rename_args = {
        .flags = DCMOD_PATH,
        .new_uname = upath,
        .new_pdir_path = vol->v_root->d_fullpath
    };
    struct adouble ad;
    struct stat st;
    struct stat other;
    const struct dir_modify_args stat_args = {
        .flags = DCMOD_STAT | DCMOD_AD,
        .st = &other,
        .adp = &ad
    };
    struct dir *folder;
    struct dir *entry;
    ino_t ino;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the session renames the folder before the removal of its children
     * ran, and a stat shows another object for the child */
    if ((entry = cached_pair(obj, vol, vid, 1, &fx)) == NULL
            || (folder = dirlookup(vol, fx.did)) == NULL
            || stat(fx.path, &st) != 0 || stat(vol->v_path, &other) != 0
            || rename(fx.path, fx.aside) != 0 // NOSONAR (the scenario under test)
            || cnid_update(vol->v_cdb, fx.did, &st, DIRDID_ROOT, upath,
                           strlen(upath)) != 0
            || dir_modify(vol, folder, &rename_args) != 0) {
        goto out;
    }

    ino = entry->dcache_ino;
    ad_init(&ad, vol);
    ad.ad_rlen = 4321;
    entry->dcache_rlen = AD_RLEN_NO_RFORK;

    if (dir_modify(vol, entry, &stat_args) == 0 && entry->dcache_ino == ino
            && entry->dcache_rlen == AD_RLEN_NO_RFORK) {
        ret = 0;
    }

out:
    /* the renamed pair goes where it went */
    delete (obj, vid, DIRDID_ROOT, DC_ASIDE "/" DC_CHILD);
    delete (obj, vid, DIRDID_ROOT, DC_ASIDE);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A replaced folder's entry that the request does not hold takes the
 *        CNID of the row its replacement already has
 */
int utest_dircache_replaced_folder_takes_row(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    struct dc_fixture fx;
    char child[] = DC_CHILD;
    struct stat st;
    struct dir *entry;
    cnid_t row = CNID_INVALID;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the folder child moves aside, and another folder with a row of its
     * own takes its name */
    if ((entry = cached_pair(obj, vol, vid, 1, &fx)) == NULL
            || replace_folder(fx.child, fx.child_aside) != 0
            || stat(fx.child, &st) != 0
            || (row = cnid_add(vol->v_cdb, &st, fx.did, child, strlen(child),
                               CNID_INVALID)) == CNID_INVALID) {
        goto out;
    }

    dir_stat_refresh(vol, entry, &st);

    if (entry->d_did == row && entry->dcache_ino == st.st_ino) {
        ret = 0;
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A folder re-keyed to the object at its path counts that object's
 *        offspring afresh, whatever count its old object held
 */
int utest_dircache_rekeyed_folder_recounts(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    struct dc_fixture fx;
    char name[] = DC_FOLDER;
    struct stat st;
    struct dir *entry;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if ((entry = replaced_folder(obj, vol, vid, 0, &fx)) == NULL
            || stat(fx.path, &st) != 0) {
        goto out;
    }

    /* the old object's count, stamped in the second the replacement holds */
    entry->d_offcnt = 5;
    entry->d_ctime = st.st_ctime;
    entry->d_flags |= DIRF_CNID;

    if (dir_stat_refresh(vol, entry, &st) == 0
            || (entry->d_flags & DIRF_CNID)
            || getfiledirparms_bitmap(obj, vid, DIRDID_ROOT, name, 0,
                                      1 << DIRPBIT_OFFCNT) != AFP_OK
            || entry->d_offcnt != 0) {
        goto out;
    }

    ret = 0;
out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A strict lookup of a folder whose own row names the object now at
 *        its path keeps the folder's CNID and lets its old children go
 */
int utest_dircache_inplace_refresh_children(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    struct dc_fixture fx;
    char name[] = DC_FOLDER;
    struct stat st;
    const struct dir *entry;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the row as a session that gave the replacement the folder's CNID
     * leaves it */
    if (replaced_folder(obj, vol, vid, 1, &fx) == NULL
            || stat(fx.path, &st) != 0
            || cnid_update(vol->v_cdb, fx.did, &st, DIRDID_ROOT, name,
                           strlen(name)) != 0) {
        goto out;
    }

    if ((entry = dirlookup_strict(vol, fx.did)) != NULL
            && entry->d_did == fx.did && entry->dcache_ino == st.st_ino
            && dircache_search_by_did(vol, fx.child_id) == NULL) {
        ret = 0;
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A cnid_add() that fails as a busy backend's does
 */
static cnid_t busy_add(struct _cnid_db *cdb _U_, const struct stat *st _U_,
                       cnid_t did _U_, const char *name _U_, size_t len _U_,
                       cnid_t hint _U_)
{
    errno = CNID_ERR_BUSY;
    return CNID_INVALID;
}

/*!
 * @brief FPGetFileDirParms of a working folder replaced under its name
 *        answers the retryable AFPERR_MISC when a busy database makes no row
 *        for the new folder, and the entry keeps its DID for the next refresh
 */
int utest_dircache_held_add_fails(AFPObj *obj, struct vol *vol, uint16_t vid)
{
    struct dc_fixture fx;
    cnid_t (*add)(struct _cnid_db *, const struct stat *, cnid_t,
                  const char *, size_t, cnid_t) = vol->v_cdb->cnid_add;
    const struct dir *entry;
    int rc;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if ((entry = replaced_folder(obj, vol, vid, 0, &fx)) == NULL) {
        goto out;
    }

    vol->v_cdb->cnid_add = busy_add;
    rc = getfiledirparms(obj, vid, fx.did, "");
    vol->v_cdb->cnid_add = add;

    if (rc == AFPERR_MISC && entry->d_did == fx.did) {
        ret = 0;
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A request's refresh that re-keys a replaced folder tells the
 *        sibling sessions under the folder's old CNID
 */
int utest_dircache_refresh_hints_siblings(AFPObj *obj, struct vol *vol,
                                          uint16_t vid)
{
    struct dc_fixture fx;
    struct stat st;
    struct dir *entry;
    int fds[2];
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if ((entry = replaced_folder(obj, vol, vid, 0, &fx)) == NULL
            || stat(fx.path, &st) != 0 || hints_begin(obj, fds) != 0) {
        goto out;
    }

    dir_stat_refresh(vol, entry, &st);

    /* the entry takes the folder's own CNID, which an in-place refresh
     * sending the same hint would not give it */
    if (entry->d_did != fx.did && entry->d_did != CNID_INVALID
            && hint_came(fds[0], CACHE_HINT_REFRESH, fx.did)) {
        ret = 0;
    }

    hints_end(obj, fds);
out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A working folder replaced under a parent whose row is gone gets no
 *        row under that parent
 */
int utest_dircache_orphan_parent_no_row(AFPObj *obj, struct vol *vol,
                                        uint16_t vid)
{
    struct dc_fixture fx;
    char child[] = DC_CHILD;
    struct stat st;
    struct dir *entry;
    int moved = 0;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the parent's row goes, as a lookup that finds another folder at the
     * parent's path removes it */
    if ((entry = cached_pair(obj, vol, vid, 1, &fx)) == NULL
            || movecwd(vol, entry) != 0
            || cnid_delete(vol->v_cdb, fx.did) != 0
            || replace_folder(fx.child, fx.child_aside) != 0) {
        goto out;
    }

    moved = 1;

    if (stat(fx.child, &st) == 0 && dir_stat_refresh(vol, entry, &st) != 0
            && cnid_get(vol->v_cdb, fx.did, child, strlen(child))
            == CNID_INVALID) {
        ret = 0;
    }

out:

    if (moved) {
        restore_folder(fx.child, fx.child_aside);
    }

    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPMoveAndRename of a folder named by its DID whose parent DID
 *        names no folder answers kFPObjectNotFound
 */
int utest_dircache_orphan_move_fails(AFPObj *obj, struct vol *vol,
                                     uint16_t vid)
{
    struct dc_fixture fx;
    struct dir *folder;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the parent's entry and row go, as another session's lookups can leave
     * the child's DID */
    if (cached_pair(obj, vol, vid, 1, &fx) != NULL
            && (folder = dircache_lookup_did(vol, fx.did)) != NULL
            && cnid_delete(vol->v_cdb, fx.did) == 0) {
        dir_remove(vol, folder, 0);

        if (moveobj(obj, vid, fx.child_id, "", DIRDID_ROOT, "", "dc-moved")
                == AFPERR_NOOBJ) {
            ret = 0;
        }
    }

    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief Create the test folder through FPCreateDir and drop its entry
 *
 * @param[in]     obj  the test session
 * @param[in]     vol  the open volume
 * @param[in]     vid  the volume's id
 * @param[in,out] fx   takes the folder's CNID
 * @returns 0, -1 when the setup fails
 */
static int uncached_folder(AFPObj *obj, const struct vol *vol, uint16_t vid,
                           struct dc_fixture *fx)
{
    char name[] = DC_FOLDER;

    if (createdir(obj, vid, DIRDID_ROOT, DC_FOLDER) != AFP_OK) {
        return -1;
    }

    fx->did = curdir->d_did;

    if (movecwd(vol, vol->v_root) != 0) {
        return -1;
    }

    drop_entry(vol, DIRDID_ROOT, name);
    return 0;
}

/*!
 * @brief Arm the shim to fail every stat() and lstat() of path with EACCES
 *
 * @param[in] vol   the volume of path
 * @param[in] path  the path to deny
 * @returns 1 when the call afpd makes for path fails so, 0 otherwise
 */
static int deny_stat(const struct vol *vol, const char *path)
{
    struct stat st;
    fault_inject_reset();
    fi.stat_name = path;
    fi.stat_errno = EACCES;
    return ostat(path, &st, vol_syml_opt(vol)) != 0 && errno == EACCES
           && fi.stat_calls == 1;
}

/*!
 * @brief An uncached folder a folder replaced answers kFPObjectNotFound by DID
 */
int utest_dircache_replaced_folder_by_did(AFPObj *obj, struct vol *vol,
                                          uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    char name[] = DC_FOLDER;
    struct dc_fixture fx;
    cnid_t gone;
    int err;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (uncached_folder(obj, vol, vid, &fx) != 0
            || getfiledirparms(obj, vid, fx.did, "") != AFP_OK
            || movecwd(vol, vol->v_root) != 0) {
        goto out;
    }

    drop_entry(vol, DIRDID_ROOT, name);

    if (replace_folder(fx.path, fx.aside) != 0) {
        goto out;
    }

    fx.moved = 1;
    err = getfiledirparms(obj, vid, fx.did, "");
    gone = fx.did;

    if (err == AFPERR_NOOBJ && dircache_lookup_did(vol, fx.did) == NULL
            && cnid_resolve(vol->v_cdb, &gone, buf, sizeof(buf)) == NULL
            && CNID_ERRNO() == CNID_ERR_NOTFOUND) {
        ret = 0;
    } else {
        fprintf(stderr, "replaced folder by its DID: %d\n", err);
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A replaced file answers kFPObjectNotFound by CNID and loses its row
 */
int utest_dircache_replaced_file_by_cnid(AFPObj *obj, struct vol *vol,
                                         uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    struct dc_fixture fx;
    cnid_t gone;
    int err;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 0, &fx) == NULL
            || getfiledirparms(obj, vid, fx.child_id, "") != AFP_OK
            || replace_file(fx.child, fx.child_aside) != 0) {
        goto out;
    }

    err = getfiledirparms(obj, vid, fx.child_id, "");
    gone = fx.child_id;

    if (err == AFPERR_NOOBJ
            && cnid_resolve(vol->v_cdb, &gone, buf, sizeof(buf)) == NULL
            && CNID_ERRNO() == CNID_ERR_NOTFOUND) {
        ret = 0;
    } else {
        fprintf(stderr, "replaced file by its CNID: %d\n", err);
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief An uncached folder whose stat is denied answers kFPAccessDenied by DID
 */
int utest_dircache_stat_denied_by_did(AFPObj *obj, struct vol *vol,
                                      uint16_t vid)
{
    struct dc_fixture fx;
    int err;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);

    if (uncached_folder(obj, vol, vid, &fx) != 0) {
        goto out;
    }

    if (!deny_stat(vol, fx.path)) {
        ret = TEST_SKIP;
        goto out;
    }

    if ((err = getfiledirparms(obj, vid, fx.did, "")) == AFPERR_ACCESS) {
        ret = 0;
    } else {
        fprintf(stderr, "denied stat by the DID: %d\n", err);
    }

out:
    fault_inject_reset();
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief ID requests in an uncached denied folder answer kFPAccessDenied
 */
int utest_dircache_ids_under_denied_folder(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    char folder[] = DC_FOLDER;
    struct dc_fixture fx;
    cnid_t kept;
    int resolved = -1;
    int deleted = -1;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 0, &fx) == NULL) {
        goto out;
    }

    drop_entry(vol, DIRDID_ROOT, folder);

    if (!deny_stat(vol, fx.path)) {
        ret = TEST_SKIP;
        goto out;
    }

    resolved = resolveid(obj, vid, fx.child_id);
    deleted = deleteid(obj, vid, fx.child_id);
    fault_inject_reset();
    kept = fx.child_id;

    if (resolved == AFPERR_ACCESS && deleted == AFPERR_ACCESS
            && cnid_resolve(vol->v_cdb, &kept, buf, sizeof(buf)) != NULL) {
        ret = 0;
    } else {
        fprintf(stderr, "IDs under a denied folder: resolve %d, delete %d\n",
                resolved, deleted);
    }

out:
    fault_inject_reset();
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief Move the child and its row aside, as another session's FPMoveAndRename
 *
 * @param[in] vol  the open volume
 * @param[in] fx   the fixture, its child a cached folder
 * @returns 0, -1 when a call fails
 */
static int move_child_with_row(const struct vol *vol,
                               const struct dc_fixture *fx)
{
    char aside[] = DC_CHILD_ASIDE;
    struct stat st;

    if (rename(fx->child, fx->child_aside) != 0
            || stat(fx->child_aside, &st) != 0
            || cnid_update(vol->v_cdb, fx->child_id, &st, fx->did, aside,
                           strlen(aside)) != 0) {
        return -1;
    }

    return 0;
}

/*!
 * @brief A cached folder moved with its row to a denied name: kFPAccessDenied
 */
int utest_dircache_moved_folder_stat_denied(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    struct dc_fixture fx;
    int err = AFP_OK;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || move_child_with_row(vol, &fx) != 0) {
        goto out;
    }

    if (!deny_stat(vol, fx.child_aside)) {
        ret = TEST_SKIP;
        goto out;
    }

    if ((err = getfiledirparms(obj, vid, fx.child_id, "")) == AFPERR_ACCESS) {
        ret = 0;
    } else {
        fprintf(stderr, "moved folder, stat denied: %d\n", err);
    }

out:
    fault_inject_reset();
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A cached folder a file replaced answers kFPObjectNotFound by its DID
 */
int utest_dircache_folder_became_file_by_did(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    struct dc_fixture fx;
    int err = AFP_OK;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || rename(fx.child, fx.child_aside) != 0
            || create_disk_file(fx.child) != 0) {
        goto out;
    }

    if ((err = getfiledirparms(obj, vid, fx.child_id, "")) == AFPERR_NOOBJ) {
        ret = 0;
    } else {
        fprintf(stderr, "cached folder a file replaced: %d\n", err);
    }

out:
    unlink(fx.child);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A cached folder moved with its row answers by DID, a file at its name
 */
int utest_dircache_moved_folder_file_at_name(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    struct dc_fixture fx;
    const char *head;
    const char *params;
    size_t hlen;
    size_t len;
    cnid_t did = CNID_INVALID;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || move_child_with_row(vol, &fx) != 0
            || create_disk_file(fx.child) != 0) {
        goto out;
    }

    err = getfiledirparms(obj, vid, fx.child_id, "");
    head = reply_params(2 * sizeof(uint16_t), &hlen);
    params = reply_params(REPLY_FILEDIRPARMS, &len);

    if (err == AFP_OK && hlen > 0 && (head[0] & FILDIRBIT_ISDIR)
            && len >= sizeof(did)) {
        memcpy(&did, params, sizeof(did));
    }

    if (did == fx.child_id) {
        ret = 0;
    } else {
        fprintf(stderr, "moved folder, a file at its name: %d, DID %u\n", err,
                ntohl(did));
    }

out:
    unlink(fx.child);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief One ID request in a cached folder moved to a name whose stat is denied
 *
 * @param[in]  obj   the test session
 * @param[in]  vol   the open volume
 * @param[in]  vid   the volume's id
 * @param[in]  call  resolveid() or deleteid()
 * @param[out] kept  1 when the file keeps its ID afterwards
 * @returns the request's AFP result, TEST_SKIP when the stat cannot be
 *          denied, -1 when the setup fails
 */
static int id_under_moved_folder(AFPObj *obj, struct vol *vol, uint16_t vid,
                                 int (*call)(AFPObj *, uint16_t, cnid_t),
                                 int *kept)
{
    static char buf[CNID_MAX_PATH_LEN];
    char leaf[] = DC_LEAF;
    struct dc_fixture fx;
    cnid_t id = CNID_INVALID;
    cnid_t left;
    int err = -1;
    dc_setup(obj, vol, &fx);
    *kept = 0;

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || createfile(obj, vid, fx.child_id, leaf) != AFP_OK
            || (id = cnid_get(vol->v_cdb, fx.child_id, leaf, strlen(leaf)))
            == CNID_INVALID
            || movecwd(vol, vol->v_root) != 0
            || move_child_with_row(vol, &fx) != 0) {
        goto out;
    }

    if (!deny_stat(vol, fx.child_aside)) {
        err = TEST_SKIP;
        goto out;
    }

    err = call(obj, vid, id);
    fault_inject_reset();
    left = id;
    *kept = cnid_resolve(vol->v_cdb, &left, buf, sizeof(buf)) != NULL;
out:
    fault_inject_reset();
    /* the file is in the child under either name, as far as the setup got */
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_CHILD "/" DC_LEAF);
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_CHILD_ASIDE "/" DC_LEAF);
    dc_teardown(obj, vid, &fx);
    return err;
}

/*!
 * @brief ID requests in a cached, moved and denied folder: kFPAccessDenied
 */
int utest_dircache_ids_in_moved_denied_folder(AFPObj *obj, struct vol *vol,
                                              uint16_t vid)
{
    int kept[2];
    int resolved;
    int deleted;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    resolved = id_under_moved_folder(obj, vol, vid, resolveid, &kept[0]);
    deleted = id_under_moved_folder(obj, vol, vid, deleteid, &kept[1]);

    if (resolved == TEST_SKIP || deleted == TEST_SKIP) {
        return TEST_SKIP;
    }

    if (resolved == AFPERR_ACCESS && deleted == AFPERR_ACCESS && kept[0]
            && kept[1]) {
        return 0;
    }

    fprintf(stderr, "IDs under a moved denied folder: resolve %d, delete %d, "
                    "kept %d %d\n", resolved, deleted, kept[0], kept[1]);
    return -1;
}

/*!
 * @brief A cached folder whose uncached parent was replaced: kFPObjectNotFound
 */
int utest_dircache_child_of_replaced_folder(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    char folder[] = DC_FOLDER;
    struct dc_fixture fx;
    cnid_t gone;
    bool kept;
    int err = AFP_OK;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (replaced_folder(obj, vol, vid, 1, &fx) == NULL
            || movecwd(vol, vol->v_root) != 0) {
        goto out;
    }

    drop_entry(vol, DIRDID_ROOT, folder);
    err = getfiledirparms(obj, vid, fx.child_id, "");
    gone = fx.did;
    kept = cnid_resolve(vol->v_cdb, &gone, buf, sizeof(buf)) != NULL
           || CNID_ERRNO() != CNID_ERR_NOTFOUND;

    if (err == AFPERR_NOOBJ && !kept) {
        ret = 0;
    } else {
        fprintf(stderr, "child of a replaced folder: %d, the folder's row %s\n",
                err, kept ? "kept or unread" : "gone");
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

static char *(*rewrite_real_resolve)(struct _cnid_db *, cnid_t *, void *,
                                     size_t, uint64_t *, uint64_t *);
static const struct dc_fixture *rewrite_fx;
static int rewrite_done;

/*!
 * @brief Resolve, then rewrite the child's row as a symlink conversion does
 *
 * @param[in]     cdb     the database
 * @param[in,out] id      the CNID to resolve, receives its parent's
 * @param[out]    buffer  receives the name
 * @param[in]     len     size of buffer
 * @param[out]    dev     receives the row's device
 * @param[out]    ino     receives the row's inode
 * @returns what the backend's cnid_resolve() returns
 */
static char *resolve_then_rewrite(struct _cnid_db *cdb, cnid_t *id,
                                  void *buffer, size_t len, uint64_t *dev,
                                  uint64_t *ino)
{
    char child[] = DC_CHILD;
    const cnid_t asked = *id;
    char *name = rewrite_real_resolve(cdb, id, buffer, len, dev, ino);
    struct stat st;

    if (name != NULL && asked == rewrite_fx->child_id
            && lstat(rewrite_fx->child, &st) == 0) {
        cdb->cnid_resolve = rewrite_real_resolve;
        rewrite_done = cnid_update(cdb, rewrite_fx->child_id, &st,
                                   rewrite_fx->did, child, strlen(child)) == 0;
    }

    return name;
}

/*!
 * @brief A row rewritten for a symlink between resolve and stat answers by CNID
 */
int utest_dircache_row_rewritten_after_resolve(AFPObj *obj, struct vol *vol,
                                               uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    struct dc_fixture fx;
    struct stat lst;
    cnid_t kept;
    uint64_t dev = 0;
    uint64_t ino = 0;
    int err = -1;
    int ret = -1;

    if (vol->v_flags & AFPVOL_FOLLOWSYM) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 0, &fx) == NULL
            || symlink(DC_LEAF, fx.child_aside) != 0
            || rename(fx.child_aside, fx.child) != 0) {
        goto out;
    }

    rewrite_fx = &fx;
    rewrite_done = 0;
    rewrite_real_resolve = vol->v_cdb->cnid_resolve;
    vol->v_cdb->cnid_resolve = resolve_then_rewrite;
    err = getfiledirparms(obj, vid, fx.child_id, "");
    vol->v_cdb->cnid_resolve = rewrite_real_resolve;
    kept = fx.child_id;

    if (rewrite_done && err == AFP_OK
            && cnid_resolve_row(vol->v_cdb, &kept, buf, sizeof(buf), &dev,
                                &ino) != NULL
            && lstat(fx.child, &lst) == 0
            && cnid_row_holds(vol->v_cdb, dev, ino, &lst)) {
        ret = 0;
    } else {
        fprintf(stderr, "row rewritten after the resolve: %d, the rewrite "
                        "%s\n", err, rewrite_done ? "ran" : "never ran");
    }

out:

    /* FPDelete refuses a symlink whose lock state it cannot read */
    if (fx.child_id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, fx.child_id);
    }

    unlink(fx.child);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief Replace the child by a file with a row of its own and check the rows
 *
 * @param[in]     obj  the test session
 * @param[in]     vol  the open volume
 * @param[in]     vid  the volume's id
 * @param[in,out] fx   the fixture, set up
 * @param[out]    own  receives the replacement's CNID, whose row the caller
 *                     deletes: no file stays at the name the row records
 * @returns 0, 1 when the request or the rows answer otherwise, -1 when the
 *          setup failed
 */
static int replaced_by_rowed_file(AFPObj *obj, const struct vol *vol,
                                  uint16_t vid, struct dc_fixture *fx,
                                  cnid_t *own)
{
    static char buf[CNID_MAX_PATH_LEN];
    char aside[] = DC_CHILD_ASIDE;
    char child[] = DC_CHILD;
    const struct dir *parent;
    const struct dir *entry;
    cnid_t probe;
    cnid_t gone;
    int err;

    if (cached_pair(obj, vol, vid, 0, fx) == NULL
            || getfiledirparms(obj, vid, fx->child_id, "") != AFP_OK
            || createfile(obj, vid, fx->did, aside) != AFP_OK
            || movecwd(vol, vol->v_root) != 0
            || (parent = dirlookup(vol, fx->did)) == NULL
            || (entry = dircache_search_by_name(vol, parent, aside,
                                                strlen(aside))) == NULL) {
        return -1;
    }

    *own = entry->d_did;

    if (rename(fx->child_aside, fx->child) != 0) {
        return -1;
    }

    err = getfiledirparms(obj, vid, fx->child_id, "");
    probe = *own;
    gone = fx->child_id;

    if (err != AFPERR_NOOBJ
            || cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf)) == NULL
            || cnid_resolve(vol->v_cdb, &gone, buf, sizeof(buf)) != NULL
            || CNID_ERRNO() != CNID_ERR_NOTFOUND
            || cnid_get(vol->v_cdb, fx->did, child, strlen(child))
            == fx->child_id) {
        fprintf(stderr, "replaced by a file with a row: %d\n", err);
        return 1;
    }

    return 0;
}

/*!
 * @brief A file replaced by one with a row: the replacement keeps its row
 */
int utest_dircache_replacement_keeps_its_row(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    struct dc_fixture fx;
    cnid_t own = CNID_INVALID;
    int ret;
    dc_setup(obj, vol, &fx);
    ret = replaced_by_rowed_file(obj, vol, vid, &fx, &own);
    dc_teardown(obj, vid, &fx);

    if (own != CNID_INVALID) {
        cnid_delete(vol->v_cdb, own);
    }

    return ret;
}

/*!
 * @brief On ea=sys a replacement keeps its own CNID by its name
 */
int utest_dircache_replacement_keeps_its_cnid(AFPObj *obj, struct vol *vol,
                                              uint16_t vid)
{
    char child[] = DC_CHILD;
    struct dc_fixture fx;
    const char *params;
    size_t len = 0;
    cnid_t own = CNID_INVALID;
    cnid_t now = CNID_INVALID;
    int err = -1;
    int ret;
    dc_setup(obj, vol, &fx);

    if ((ret = replaced_by_rowed_file(obj, vol, vid, &fx, &own)) == 0) {
        err = getfiledirparms(obj, vid, fx.did, child);
        params = reply_params(REPLY_FILEDIRPARMS, &len);

        if (err == AFP_OK && len >= sizeof(now)) {
            memcpy(&now, params, sizeof(now));
        }

        if (now != own) {
            fprintf(stderr, "the replacement by its name: %d, CNID %u of "
                            "%u\n", err, ntohl(now), ntohl(own));
            ret = 1;
        }
    }

    dc_teardown(obj, vid, &fx);

    if (own != CNID_INVALID) {
        cnid_delete(vol->v_cdb, own);
    }

    return ret;
}

/* the stand-ins' fixture, the stat another session writes into the refused
 * row, whether a stand-in ran, and the backend's conditional delete */
static const struct dc_fixture *race_fx;
static struct stat race_st;
static int race_fired;
static int (*race_real_delete_row)(struct _cnid_db *, cnid_t, cnid_t,
                                   const char *, size_t, uint64_t, uint64_t);

/*!
 * @brief Rewrite the row as another session would, then run the real delete
 *
 * @returns what the backend's conditional delete returns
 */
static int rewrite_then_delete_row(struct _cnid_db *cdb, cnid_t id,
                                   cnid_t did, const char *name, size_t len,
                                   uint64_t dev, uint64_t ino)
{
    char child[] = DC_CHILD;
    cdb->cnid_delete_row = race_real_delete_row;
    race_fired = 1;
    (void)cnid_update(cdb, race_fx->child_id, &race_st, race_fx->did, child,
                      strlen(child));
    return race_real_delete_row(cdb, id, did, name, len, dev, ino);
}

/*!
 * @brief A row rewritten as the refusal deletes it stays and answers by CNID
 */
int utest_dircache_row_rewritten_at_retirement(AFPObj *obj, struct vol *vol,
                                               uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    struct dc_fixture fx;
    cnid_t kept;
    uint64_t dev = 0;
    uint64_t ino = 0;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 0, &fx) == NULL
            || getfiledirparms(obj, vid, fx.child_id, "") != AFP_OK
            || replace_file(fx.child, fx.child_aside) != 0
            || lstat(fx.child, &race_st) != 0) {
        goto out;
    }

    race_fx = &fx;
    race_fired = 0;
    race_real_delete_row = vol->v_cdb->cnid_delete_row;
    vol->v_cdb->cnid_delete_row = rewrite_then_delete_row;
    err = getfiledirparms(obj, vid, fx.child_id, "");
    vol->v_cdb->cnid_delete_row = race_real_delete_row;
    kept = fx.child_id;

    if (race_fired && err == AFP_OK
            && cnid_resolve_row(vol->v_cdb, &kept, buf, sizeof(buf), &dev,
                                &ino) != NULL
            && cnid_row_holds(vol->v_cdb, dev, ino, &race_st)) {
        ret = 0;
    } else {
        fprintf(stderr, "row rewritten as the refusal deletes: %d, the "
                        "delete %s\n", err, race_fired ? "ran" : "never ran");
    }

out:
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief Move the row to the file's new name, then run the real delete
 *
 * @returns what the backend's conditional delete returns
 */
static int move_then_delete_row(struct _cnid_db *cdb, cnid_t id, cnid_t did,
                                const char *name, size_t len, uint64_t dev,
                                uint64_t ino)
{
    char aside[] = DC_CHILD_ASIDE;
    cdb->cnid_delete_row = race_real_delete_row;
    race_fired = 1;
    (void)cnid_update(cdb, race_fx->child_id, &race_st, race_fx->did, aside,
                      strlen(aside));
    return race_real_delete_row(cdb, id, did, name, len, dev, ino);
}

/*!
 * @brief A row moved as the refusal deletes it stays at its new name
 */
int utest_dircache_row_moved_at_retirement(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    char aside[] = DC_CHILD_ASIDE;
    struct dc_fixture fx;
    const char *name = NULL;
    cnid_t kept;
    uint64_t dev = 0;
    uint64_t ino = 0;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 0, &fx) == NULL
            || getfiledirparms(obj, vid, fx.child_id, "") != AFP_OK
            || rename(fx.child, fx.child_aside) != 0
            || lstat(fx.child_aside, &race_st) != 0
            || create_disk_file(fx.child) != 0) {
        goto out;
    }

    race_fx = &fx;
    race_fired = 0;
    race_real_delete_row = vol->v_cdb->cnid_delete_row;
    vol->v_cdb->cnid_delete_row = move_then_delete_row;
    err = getfiledirparms(obj, vid, fx.child_id, "");
    vol->v_cdb->cnid_delete_row = race_real_delete_row;
    kept = fx.child_id;
    name = cnid_resolve_row(vol->v_cdb, &kept, buf, sizeof(buf), &dev, &ino);

    if (race_fired && err == AFPERR_NOOBJ && name != NULL
            && strcmp(name, aside) == 0
            && cnid_row_holds(vol->v_cdb, dev, ino, &race_st)) {
        ret = 0;
    } else {
        fprintf(stderr, "row moved as the refusal deletes: %d, the row %s, "
                        "the delete %s\n", err, name ? name : "gone",
                race_fired ? "ran" : "never ran");
    }

out:
    unlink(fx.child);
    dc_teardown(obj, vid, &fx);
    return ret;
}

static char *(*race_real_resolve)(struct _cnid_db *, cnid_t *, void *, size_t,
                                  uint64_t *, uint64_t *);

static int resolve_busy_fired;

/*!
 * @brief Stand in for the backend's resolve, once: it cannot answer
 *
 * @returns NULL with errno CNID_ERR_BUSY
 */
static char *resolve_busy_once(struct _cnid_db *cdb, cnid_t *id _U_,
                               void *buffer _U_, size_t len _U_,
                               uint64_t *dev _U_, uint64_t *ino _U_)
{
    cdb->cnid_resolve = race_real_resolve;
    resolve_busy_fired = 1;
    errno = CNID_ERR_BUSY;
    return NULL;
}

static int race_resolves_left;
static int resolve_busy_later_fired;

/*!
 * @brief Pass race_resolves_left resolves to the backend, then fail one busy
 *
 * @returns what the backend's resolve returns, then NULL with errno
 *          CNID_ERR_BUSY
 */
static char *resolve_busy_later(struct _cnid_db *cdb, cnid_t *id,
                                void *buffer, size_t len, uint64_t *dev,
                                uint64_t *ino)
{
    if (race_resolves_left-- > 0) {
        return race_real_resolve(cdb, id, buffer, len, dev, ino);
    }

    cdb->cnid_resolve = race_real_resolve;
    resolve_busy_later_fired = 1;
    errno = CNID_ERR_BUSY;
    return NULL;
}

/*!
 * @brief One ID request in a moved cached folder whose self-heal resolve fails
 *
 * @param[in]  obj   the test session
 * @param[in]  vol   the open volume
 * @param[in]  vid   the volume's id
 * @param[in]  call  resolveid() or deleteid()
 * @param[out] kept  1 when the self-heal resolve failed and the file keeps
 *                   its ID afterwards
 * @returns the request's AFP result, -1 when the setup fails
 */
static int id_under_moved_folder_busy(AFPObj *obj, struct vol *vol,
                                      uint16_t vid,
                                      int (*call)(AFPObj *, uint16_t, cnid_t),
                                      int *kept)
{
    static char buf[CNID_MAX_PATH_LEN];
    char leaf[] = DC_LEAF;
    char *(*real_resolve)(struct _cnid_db *, cnid_t *, void *, size_t,
                          uint64_t *, uint64_t *);
    struct dc_fixture fx;
    cnid_t id = CNID_INVALID;
    cnid_t left;
    int err = -1;
    dc_setup(obj, vol, &fx);
    real_resolve = vol->v_cdb->cnid_resolve;
    *kept = 0;

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || createfile(obj, vid, fx.child_id, leaf) != AFP_OK
            || (id = cnid_get(vol->v_cdb, fx.child_id, leaf, strlen(leaf)))
            == CNID_INVALID
            || movecwd(vol, vol->v_root) != 0
            || move_child_with_row(vol, &fx) != 0
            || dircache_lookup_did(vol, fx.child_id) == NULL) {
        goto out;
    }

    /* the file's resolve passes and the self-heal's fails: the folder's
     * cached entry takes no resolve */
    race_real_resolve = real_resolve;
    race_resolves_left = 1;
    resolve_busy_later_fired = 0;
    vol->v_cdb->cnid_resolve = resolve_busy_later;
    err = call(obj, vid, id);
    vol->v_cdb->cnid_resolve = real_resolve;
    left = id;
    *kept = resolve_busy_later_fired
            && cnid_resolve(vol->v_cdb, &left, buf, sizeof(buf)) != NULL;
out:
    vol->v_cdb->cnid_resolve = real_resolve;
    /* the file is in the child under either name, as far as the setup got */
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_CHILD "/" DC_LEAF);
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_CHILD_ASIDE "/" DC_LEAF);
    dc_teardown(obj, vid, &fx);
    return err;
}

/*!
 * @brief ID requests in a moved cached folder whose lookup fails: kFPMiscErr
 */
int utest_dircache_moved_folder_ids_backend_failure(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid)
{
    int kept[2];
    const int resolved = id_under_moved_folder_busy(obj, vol, vid, resolveid,
                                                    &kept[0]);
    const int deleted = id_under_moved_folder_busy(obj, vol, vid, deleteid,
                                                   &kept[1]);

    if (resolved == AFPERR_MISC && deleted == AFPERR_MISC && kept[0]
            && kept[1]) {
        return 0;
    }

    fprintf(stderr, "IDs in a moved folder the backend cannot look up: "
                    "resolve %d, delete %d, kept %d %d\n", resolved, deleted,
            kept[0], kept[1]);
    return -1;
}

static int delete_row_busy_calls;

/*!
 * @brief Stand in for the backend's conditional delete: it cannot answer
 *
 * @returns -1 with errno CNID_ERR_BUSY
 */
static int delete_row_busy(struct _cnid_db *cdb _U_, cnid_t id _U_,
                           cnid_t did _U_, const char *name _U_,
                           size_t len _U_, uint64_t dev _U_, uint64_t ino _U_)
{
    delete_row_busy_calls++;
    errno = CNID_ERR_BUSY;
    return -1;
}

/*!
 * @brief Delete nothing, as for a changed row, and fail the next resolve busy
 *
 * @returns 0
 */
static int delete_row_changed(struct _cnid_db *cdb, cnid_t id _U_,
                              cnid_t did _U_, const char *name _U_,
                              size_t len _U_, uint64_t dev _U_,
                              uint64_t ino _U_)
{
    race_fired = 1;
    race_real_resolve = cdb->cnid_resolve;
    cdb->cnid_resolve = resolve_busy_once;
    return 0;
}

/*!
 * @brief A refusal the backend fails answers kFPMiscErr and keeps the row
 */
int utest_dircache_retirement_backend_failure(AFPObj *obj, struct vol *vol,
                                              uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    struct dc_fixture fx;
    int (*real_delete_row)(struct _cnid_db *, cnid_t, cnid_t, const char *,
                           size_t, uint64_t, uint64_t);
    char *(*real_resolve)(struct _cnid_db *, cnid_t *, void *, size_t,
                          uint64_t *, uint64_t *);
    cnid_t kept;
    int failed = -1;
    int changed = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    real_delete_row = vol->v_cdb->cnid_delete_row;
    real_resolve = vol->v_cdb->cnid_resolve;

    if (cached_pair(obj, vol, vid, 0, &fx) == NULL
            || getfiledirparms(obj, vid, fx.child_id, "") != AFP_OK
            || replace_file(fx.child, fx.child_aside) != 0) {
        goto out;
    }

    vol->v_cdb->cnid_delete_row = delete_row_busy;
    delete_row_busy_calls = 0;
    failed = getfiledirparms(obj, vid, fx.child_id, "");
    race_fired = 0;
    resolve_busy_fired = 0;
    vol->v_cdb->cnid_delete_row = delete_row_changed;
    changed = getfiledirparms(obj, vid, fx.child_id, "");
    vol->v_cdb->cnid_delete_row = real_delete_row;
    vol->v_cdb->cnid_resolve = real_resolve;
    kept = fx.child_id;

    if (failed == AFPERR_MISC && delete_row_busy_calls > 0 && race_fired
            && resolve_busy_fired && changed == AFPERR_MISC
            && cnid_resolve(vol->v_cdb, &kept, buf, sizeof(buf)) != NULL) {
        ret = 0;
    } else {
        fprintf(stderr, "retirement the backend cannot answer: the delete "
                        "%d after %d calls, the second resolve %d%s%s\n",
                failed, delete_row_busy_calls, changed,
                race_fired ? "" : ", the delete never ran",
                resolve_busy_fired ? "" : ", no resolve after the delete");
    }

out:
    vol->v_cdb->cnid_delete_row = real_delete_row;
    vol->v_cdb->cnid_resolve = real_resolve;
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief An uncached folder a followed link replaced answers kFPObjectNotFound
 */
int utest_dircache_followed_link_replacement(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    char child[] = DC_CHILD;
    const int flags = vol->v_flags;
    struct dc_fixture fx;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || createdir(obj, vid, fx.did, DC_LEAF) != AFP_OK
            || movecwd(vol, vol->v_root) != 0
            || rename(fx.child, fx.child_aside) != 0
            || symlink(DC_LEAF, fx.child) != 0) {
        goto out;
    }

    drop_entry(vol, fx.did, child);
    vol->v_flags |= AFPVOL_FOLLOWSYM;
    err = getfiledirparms(obj, vid, fx.child_id, "");
    vol->v_flags = flags;

    if (err == AFPERR_NOOBJ) {
        ret = 0;
    } else {
        fprintf(stderr, "a folder a followed link replaced: %d\n", err);
    }

out:
    vol->v_flags = flags;
    movecwd(vol, vol->v_root);
    unlink(fx.child);
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_LEAF);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A row rewritten for a followed link as the refusal deletes it stays
 */
int utest_dircache_link_row_rewritten_at_retirement(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    char child[] = DC_CHILD;
    const int flags = vol->v_flags;
    struct dc_fixture fx;
    cnid_t kept;
    uint64_t dev = 0;
    uint64_t ino = 0;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || createdir(obj, vid, fx.did, DC_LEAF) != AFP_OK
            || movecwd(vol, vol->v_root) != 0
            || rename(fx.child, fx.child_aside) != 0
            || symlink(DC_LEAF, fx.child) != 0
            || lstat(fx.child, &race_st) != 0) {
        goto out;
    }

    drop_entry(vol, fx.did, child);
    race_fx = &fx;
    race_fired = 0;
    race_real_delete_row = vol->v_cdb->cnid_delete_row;
    vol->v_cdb->cnid_delete_row = rewrite_then_delete_row;
    vol->v_flags |= AFPVOL_FOLLOWSYM;
    err = getfiledirparms(obj, vid, fx.child_id, "");
    vol->v_flags = flags;
    vol->v_cdb->cnid_delete_row = race_real_delete_row;
    kept = fx.child_id;

    if (race_fired && err == AFP_OK
            && cnid_resolve_row(vol->v_cdb, &kept, buf, sizeof(buf), &dev,
                                &ino) != NULL
            && cnid_row_holds(vol->v_cdb, dev, ino, &race_st)) {
        ret = 0;
    } else {
        fprintf(stderr, "link row rewritten as the refusal deletes: %d, the "
                        "delete %s\n", err, race_fired ? "ran" : "never ran");
    }

out:
    vol->v_flags = flags;
    movecwd(vol, vol->v_root);
    unlink(fx.child);

    /* the row the stand-in gave the link */
    if (fx.child_id != CNID_INVALID) {
        drop_entry(vol, fx.did, child);
        cnid_delete(vol->v_cdb, fx.child_id);
    }

    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_LEAF);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A followed link's rewritten row answers kFPMiscErr if lstat() fails
 */
int utest_dircache_rewritten_link_unreadable(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    char child[] = DC_CHILD;
    const int flags = vol->v_flags;
    struct dc_fixture fx;
    struct stat st;
    struct stat lst;
    cnid_t kept;
    uint64_t dev = 0;
    uint64_t ino = 0;
    int probe[3];
    int calls = 0;
    int err = -1;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || createdir(obj, vid, fx.did, DC_LEAF) != AFP_OK
            || movecwd(vol, vol->v_root) != 0
            || rename(fx.child, fx.child_aside) != 0
            || symlink(DC_LEAF, fx.child) != 0
            || lstat(fx.child, &race_st) != 0) {
        goto out;
    }

    /* the shim lets the lookup's stat() of the link and the refusal's
     * lstat() pass, and fails the lstat() after the rewrite */
    fault_inject_reset();
    fi.stat_name = fx.child;
    fi.stat_errno = EIO;
    fi.stat_pass = 2;
    probe[0] = stat(fx.child, &st);
    probe[1] = lstat(fx.child, &lst);
    probe[2] = lstat(fx.child, &lst);

    if (probe[0] != 0 || probe[1] != 0 || probe[2] == 0 || errno != EIO
            || fi.stat_calls != 3) {
        ret = TEST_SKIP;
        goto out;
    }

    fi.stat_calls = 0;
    drop_entry(vol, fx.did, child);
    race_fx = &fx;
    race_fired = 0;
    race_real_delete_row = vol->v_cdb->cnid_delete_row;
    vol->v_cdb->cnid_delete_row = rewrite_then_delete_row;
    vol->v_flags |= AFPVOL_FOLLOWSYM;
    err = getfiledirparms(obj, vid, fx.child_id, "");
    vol->v_flags = flags;
    vol->v_cdb->cnid_delete_row = race_real_delete_row;
    calls = fi.stat_calls;
    fault_inject_reset();
    kept = fx.child_id;

    if (race_fired && calls >= 3 && err == AFPERR_MISC
            && cnid_resolve_row(vol->v_cdb, &kept, buf, sizeof(buf), &dev,
                                &ino) != NULL
            && cnid_row_holds(vol->v_cdb, dev, ino, &race_st)) {
        ret = 0;
    } else {
        fprintf(stderr, "rewritten link whose lstat() fails: %d after %d "
                        "stats, the delete %s\n", err, calls,
                race_fired ? "ran" : "never ran");
    }

out:
    fault_inject_reset();
    vol->v_flags = flags;
    movecwd(vol, vol->v_root);
    unlink(fx.child);

    /* the row the stand-in gave the link */
    if (fx.child_id != CNID_INVALID) {
        drop_entry(vol, fx.did, child);
        cnid_delete(vol->v_cdb, fx.child_id);
    }

    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_LEAF);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A followed link with its own row answers kFPMiscErr if lstat() fails
 */
int utest_dircache_followed_link_unreadable(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    char link[] = DC_CHILD;
    const int flags = vol->v_flags;
    struct dc_fixture fx;
    struct stat st;
    struct stat lst;
    cnid_t id = CNID_INVALID;
    cnid_t kept;
    bool row = false;
    int calls = 0;
    int err = -1;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);

    if (createdir(obj, vid, DIRDID_ROOT, DC_FOLDER) != AFP_OK) {
        goto out;
    }

    fx.did = curdir->d_did;

    if (createdir(obj, vid, fx.did, DC_LEAF) != AFP_OK
            || symlink(DC_LEAF, fx.child) != 0 || lstat(fx.child, &lst) != 0
            || (id = cnid_add(vol->v_cdb, &lst, fx.did, link, strlen(link),
                              CNID_INVALID)) == CNID_INVALID
            || movecwd(vol, vol->v_root) != 0) {
        goto out;
    }

    /* the shim must fail the link's lstat() after a stat() passes */
    fault_inject_reset();
    fi.stat_name = fx.child;
    fi.stat_errno = EIO;
    fi.stat_pass = 1;

    if (stat(fx.child, &st) != 0 || lstat(fx.child, &lst) == 0
            || errno != EIO || fi.stat_calls != 2) {
        ret = TEST_SKIP;
        goto out;
    }

    fi.stat_calls = 0;
    vol->v_flags |= AFPVOL_FOLLOWSYM;
    err = getfiledirparms(obj, vid, id, "");
    vol->v_flags = flags;
    calls = fi.stat_calls;
    fault_inject_reset();
    kept = id;
    row = cnid_resolve(vol->v_cdb, &kept, buf, sizeof(buf)) != NULL;

    if (err == AFPERR_MISC && row && calls >= 2) {
        ret = 0;
    } else {
        fprintf(stderr, "followed link whose lstat() fails: %d after %d "
                        "stats, its row %s\n", err, calls,
                row ? "kept" : "gone");
    }

out:
    fault_inject_reset();
    vol->v_flags = flags;
    movecwd(vol, vol->v_root);
    unlink(fx.child);

    if (id != CNID_INVALID) {
        drop_entry(vol, fx.did, link);
        cnid_delete(vol->v_cdb, id);
    }

    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_LEAF);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief An uncached folder whose stat fails ENOTDIR answers kFPObjectNotFound
 */
int utest_dircache_path_crosses_file_by_did(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    struct dc_fixture fx;
    struct stat st;
    int err = -1;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);

    if (uncached_folder(obj, vol, vid, &fx) != 0) {
        goto out;
    }

    /* the call afpd makes for the folder's path must fail so */
    fault_inject_reset();
    fi.stat_name = fx.path;
    fi.stat_errno = ENOTDIR;

    if (ostat(fx.path, &st, vol_syml_opt(vol)) == 0 || errno != ENOTDIR
            || fi.stat_calls != 1) {
        ret = TEST_SKIP;
        goto out;
    }

    err = getfiledirparms(obj, vid, fx.did, "");

    if (err == AFPERR_NOOBJ && fi.stat_calls > 1) {
        ret = 0;
    } else {
        fprintf(stderr, "a folder whose stat answers ENOTDIR: %d after %d "
                        "stats\n", err, fi.stat_calls);
    }

out:
    fault_inject_reset();
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPEnumerate of a cached folder that vanished answers kFPDirNotFound
 */
int utest_dircache_enumerate_vanished_folder(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    struct dc_fixture fx;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || rename(fx.child, fx.child_aside) != 0) {
        goto out;
    }

    err = enumerate_range(obj, vid, fx.child_id, 1 << FILPBIT_FNUM, 0, 1, 40);

    if (err == AFPERR_NODIR) {
        ret = 0;
    } else {
        fprintf(stderr, "FPEnumerate of a cached folder that vanished: %d\n",
                err);
    }

out:

    if (fx.child_id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, fx.child_id);
    }

    dc_teardown(obj, vid, &fx);
    return ret;
}

#define DC_WALK_A     "dc-walk-a"
#define DC_WALK_B     "dc-walk-b"
#define DC_WALK_LEAF  "dc-walk-leaf"
#define DC_WALK_MOVED "dc-walk-moved"

/* a volume-walking catalog search paused at its first match: the two
 * folders' CNIDs and paths, the one still to enter, and the position */
struct dc_walk {
    cnid_t did[2];
    char path[2][MAXPATHLEN + 1];
    int pending;
    unsigned char pos[16];
};

static const char *const dc_walk_name[2] = { DC_WALK_A, DC_WALK_B };

/*!
 * @brief Run the walk on to its next match or its end, past the clock's pauses
 *
 * @returns the AFP result, AFP_OK with the match's parent in pdid
 */
static int dc_walk_on(AFPObj *obj, uint16_t vid, struct dc_walk *w,
                      uint32_t *nrecs, cnid_t *pdid)
{
    int err = AFP_OK;
    *nrecs = 0;

    for (int i = 0; i < 256 && *nrecs == 0 && err == AFP_OK; i++) {
        err = catsearch_from(obj, vid, 1, w->pos, DC_WALK_LEAF, nrecs, pdid);
    }

    return err;
}

/*!
 * @brief Two folders with a same-named file each, and a walk paused at one
 *
 * The other folder waits on the walk's stack, its entry cached.
 *
 * @param[in]     obj  the test session
 * @param[in]     vol  the open volume, its searches through the CNID
 *                     database off
 * @param[in]     vid  the volume's id
 * @param[in,out] fx   takes the test folder's CNID
 * @param[out]    w    the walk
 * @returns 0, -1 when the setup failed
 */
static int dc_walk_paused(AFPObj *obj, const struct vol *vol, uint16_t vid,
                          struct dc_fixture *fx, struct dc_walk *w)
{
    uint32_t nrecs = 0;
    cnid_t pdid = CNID_INVALID;
    int err;
    memset(w, 0, sizeof(*w));

    if (createdir(obj, vid, DIRDID_ROOT, DC_FOLDER) != AFP_OK) {
        return -1;
    }

    fx->did = curdir->d_did;

    for (int i = 0; i < 2; i++) {
        folder_path(vol, w->path[i], dc_walk_name[i]);

        if (createdir(obj, vid, fx->did, dc_walk_name[i]) != AFP_OK) {
            return -1;
        }

        w->did[i] = curdir->d_did;

        if (createfile(obj, vid, w->did[i], DC_WALK_LEAF) != AFP_OK) {
            return -1;
        }
    }

    if (movecwd(vol, vol->v_root) != 0) {
        return -1;
    }

    err = dc_walk_on(obj, vid, w, &nrecs, &pdid);

    if (err != AFP_OK || nrecs != 1
            || (pdid != w->did[0] && pdid != w->did[1])) {
        fprintf(stderr, "a walking search's first match: %d, %u found\n",
                err, nrecs);
        return -1;
    }

    w->pending = pdid == w->did[0] ? 1 : 0;
    return 0;
}

/*!
 * @brief Remove the walk's files and folders, moved or replaced ones included
 */
static void dc_walk_done(AFPObj *obj, uint16_t vid)
{
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_WALK_A "/" DC_WALK_LEAF);
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_WALK_B "/" DC_WALK_LEAF);
    delete (obj, vid, DIRDID_ROOT,
            DC_FOLDER "/" DC_WALK_MOVED "/" DC_WALK_LEAF);
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_WALK_A);
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_WALK_B);
    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_WALK_MOVED);
}

/*!
 * @brief Move a walk folder and its row, as another session's FPMoveAndRename
 *
 * @param[in] vol    the open volume
 * @param[in] fx     the fixture
 * @param[in] w      the walk
 * @param[in] which  the folder: w->pending, which the walk has still to
 *                   enter, or the other, which it paused in
 * @param[in] mpath  the folder's new path
 * @returns 0, -1 when a call fails
 */
static int dc_walk_move(const struct vol *vol, const struct dc_fixture *fx,
                        const struct dc_walk *w, int which, const char *mpath)
{
    char moved[] = DC_WALK_MOVED;
    struct stat st;

    if (rename(w->path[which], mpath) != 0 || stat(mpath, &st) != 0
            || cnid_update(vol->v_cdb, w->did[which], &st, fx->did, moved,
                           strlen(moved)) != 0) {
        return -1;
    }

    return 0;
}

/*!
 * @brief A paused walk finds a folder's file under its DID once its row moved
 */
int utest_dircache_walk_after_self_heal(AFPObj *obj, struct vol *vol,
                                        uint16_t vid)
{
    char mpath[MAXPATHLEN + 1];
    const int searchdb = vol->v_flags & AFPVOL_SEARCHDB;
    struct dc_fixture fx;
    struct dc_walk w;
    uint32_t nrecs = 0;
    cnid_t pdid = CNID_INVALID;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    folder_path(vol, mpath, DC_WALK_MOVED);
    vol->v_flags &= ~AFPVOL_SEARCHDB;

    if (dc_walk_paused(obj, vol, vid, &fx, &w) != 0
            || dc_walk_move(vol, &fx, &w, w.pending, mpath) != 0
            || create_disk_file(w.path[w.pending]) != 0) {
        goto out;
    }

    err = dc_walk_on(obj, vid, &w, &nrecs, &pdid);

    if (err == AFP_OK && nrecs == 1 && pdid == w.did[w.pending]) {
        ret = 0;
    } else {
        fprintf(stderr, "a walking search after a self-heal: %d, %u found "
                        "under %u, the folder %u\n", err, nrecs, ntohl(pdid),
                ntohl(w.did[w.pending]));
    }

out:
    vol->v_flags = (vol->v_flags & ~AFPVOL_SEARCHDB) | searchdb;
    movecwd(vol, vol->v_root);
    dc_walk_done(obj, vid);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A paused walk skips a cached folder moved to a denied place and ends
 */
int utest_dircache_walk_skips_denied_folder(AFPObj *obj, struct vol *vol,
                                            uint16_t vid)
{
    char mpath[MAXPATHLEN + 1];
    const int searchdb = vol->v_flags & AFPVOL_SEARCHDB;
    struct dc_fixture fx;
    struct dc_walk w;
    uint32_t nrecs = 0;
    cnid_t pdid = CNID_INVALID;
    int calls = 0;
    int err = -1;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);
    folder_path(vol, mpath, DC_WALK_MOVED);
    vol->v_flags &= ~AFPVOL_SEARCHDB;

    if (dc_walk_paused(obj, vol, vid, &fx, &w) != 0
            || dc_walk_move(vol, &fx, &w, w.pending, mpath) != 0
            || create_disk_file(w.path[w.pending]) != 0) {
        goto out;
    }

    if (!deny_stat(vol, mpath)) {
        ret = TEST_SKIP;
        goto out;
    }

    err = dc_walk_on(obj, vid, &w, &nrecs, &pdid);
    calls = fi.stat_calls;
    fault_inject_reset();

    /* the self-heal's lookup met the denied stat */
    if (calls > 1 && err == AFPERR_EOF && nrecs == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "a walking search past a denied folder: %d, %u "
                        "found, %d stats denied\n", err, nrecs, calls - 1);
    }

out:
    fault_inject_reset();
    vol->v_flags = (vol->v_flags & ~AFPVOL_SEARCHDB) | searchdb;
    movecwd(vol, vol->v_root);
    dc_walk_done(obj, vid);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A paused walk whose self-heal the backend cannot answer: kFPMiscErr
 */
int utest_dircache_walk_backend_failure(AFPObj *obj, struct vol *vol,
                                        uint16_t vid)
{
    char mpath[MAXPATHLEN + 1];
    const int searchdb = vol->v_flags & AFPVOL_SEARCHDB;
    char *(*real_resolve)(struct _cnid_db *, cnid_t *, void *, size_t,
                          uint64_t *, uint64_t *);
    struct dc_fixture fx;
    struct dc_walk w;
    uint32_t nrecs = 0;
    cnid_t pdid = CNID_INVALID;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    folder_path(vol, mpath, DC_WALK_MOVED);
    real_resolve = vol->v_cdb->cnid_resolve;
    vol->v_flags &= ~AFPVOL_SEARCHDB;

    /* with the entry cached and nothing at the old name, the first resolve,
     * the busy one, is the self-heal's */
    if (dc_walk_paused(obj, vol, vid, &fx, &w) != 0
            || dc_walk_move(vol, &fx, &w, w.pending, mpath) != 0
            || dircache_lookup_did(vol, w.did[w.pending]) == NULL) {
        goto out;
    }

    race_real_resolve = real_resolve;
    resolve_busy_fired = 0;
    vol->v_cdb->cnid_resolve = resolve_busy_once;
    err = dc_walk_on(obj, vid, &w, &nrecs, &pdid);
    vol->v_cdb->cnid_resolve = real_resolve;

    if (err == AFPERR_MISC && resolve_busy_fired) {
        ret = 0;
    } else {
        fprintf(stderr, "a walking search whose self-heal the backend "
                        "cannot answer: %d, %u found, the stand-in %s\n", err,
                nrecs, resolve_busy_fired ? "ran" : "never ran");
    }

out:
    vol->v_cdb->cnid_resolve = real_resolve;
    vol->v_flags = (vol->v_flags & ~AFPVOL_SEARCHDB) | searchdb;
    movecwd(vol, vol->v_root);
    dc_walk_done(obj, vid);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A walk skips the moved, denied folder it paused in and lists the other
 */
int utest_dircache_walk_paused_folder_denied(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    char mpath[MAXPATHLEN + 1];
    const int searchdb = vol->v_flags & AFPVOL_SEARCHDB;
    struct dc_fixture fx;
    struct dc_walk w;
    uint32_t nrecs = 0;
    cnid_t pdid = CNID_INVALID;
    int calls = 0;
    int err = -1;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);
    folder_path(vol, mpath, DC_WALK_MOVED);
    vol->v_flags &= ~AFPVOL_SEARCHDB;

    /* the folder the walk paused in moves, its listing left open */
    if (dc_walk_paused(obj, vol, vid, &fx, &w) != 0
            || dc_walk_move(vol, &fx, &w, 1 - w.pending, mpath) != 0) {
        goto out;
    }

    if (!deny_stat(vol, mpath)) {
        ret = TEST_SKIP;
        goto out;
    }

    err = dc_walk_on(obj, vid, &w, &nrecs, &pdid);
    calls = fi.stat_calls;
    fault_inject_reset();

    /* the self-heal's lookup met the denied stat */
    if (calls > 1 && err == AFP_OK && nrecs == 1
            && pdid == w.did[w.pending]) {
        ret = 0;
    } else {
        fprintf(stderr, "a walking search past the folder it paused in: %d, "
                        "%u found under %u, the other folder %u, %d stats "
                        "denied\n", err, nrecs, ntohl(pdid),
                ntohl(w.did[w.pending]), calls - 1);
    }

out:
    fault_inject_reset();
    vol->v_flags = (vol->v_flags & ~AFPVOL_SEARCHDB) | searchdb;
    movecwd(vol, vol->v_root);
    dc_walk_done(obj, vid);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A file in the cached child folder, which a file then replaces on disk
 *
 * @param[in]     obj  the test session
 * @param[in]     vol  the open volume
 * @param[in]     vid  the volume's id
 * @param[in,out] fx   the fixture, set up
 * @returns the file's CNID, or CNID_INVALID when the setup failed
 */
static cnid_t file_in_folder_a_file_took(AFPObj *obj, const struct vol *vol,
                                         uint16_t vid, struct dc_fixture *fx)
{
    char leaf[] = DC_LEAF;
    const struct dir *parent;
    const struct dir *entry;
    cnid_t id;

    if (cached_pair(obj, vol, vid, 1, fx) == NULL
            || createfile(obj, vid, fx->child_id, leaf) != AFP_OK
            || movecwd(vol, vol->v_root) != 0
            || (parent = dirlookup(vol, fx->child_id)) == NULL
            || (entry = dircache_search_by_name(vol, parent, leaf,
                                                strlen(leaf))) == NULL) {
        return CNID_INVALID;
    }

    id = entry->d_did;

    if (rename(fx->child, fx->child_aside) != 0
            || create_disk_file(fx->child) != 0) {
        return CNID_INVALID;
    }

    return id;
}

/*!
 * @brief Remove file_in_folder_a_file_took()'s objects and rows, wherever left
 */
static void file_in_folder_a_file_took_done(const struct vol *vol,
                                            const struct dc_fixture *fx,
                                            cnid_t id)
{
    char name[] = DC_LEAF;
    char leaf[MAXPATHLEN + 1];
    char unmoved[MAXPATHLEN + 1];
    snprintf(leaf, sizeof(leaf), "%s/" DC_LEAF, fx->child_aside);
    snprintf(unmoved, sizeof(unmoved), "%s/" DC_LEAF, fx->child);

    if (id == CNID_INVALID && fx->child_id != CNID_INVALID) {
        id = cnid_get(vol->v_cdb, fx->child_id, name, strlen(name));
    }

    unlink(unmoved);
    unlink(fx->child);
    rmdir(fx->child);
    unlink(leaf);
    rmdir(fx->child_aside);

    if (id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, id);
    }

    cnid_delete(vol->v_cdb, fx->child_id);
}

/*!
 * @brief FPResolveID in a cached folder a file replaced answers kFPIDNotFound
 */
int utest_dircache_resolveid_folder_a_file_took(AFPObj *obj, struct vol *vol,
                                                uint16_t vid)
{
    struct dc_fixture fx;
    cnid_t id;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if ((id = file_in_folder_a_file_took(obj, vol, vid, &fx))
            != CNID_INVALID) {
        if ((err = resolveid(obj, vid, id)) == AFPERR_NOID) {
            ret = 0;
        } else {
            fprintf(stderr, "FPResolveID in a folder a file took: %d\n", err);
        }
    }

    movecwd(vol, vol->v_root);
    file_in_folder_a_file_took_done(vol, &fx, id);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPDeleteID in a cached folder a file replaced deletes the ID
 */
int utest_dircache_deleteid_folder_a_file_took(AFPObj *obj, struct vol *vol,
                                               uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    struct dc_fixture fx;
    cnid_t id;
    cnid_t probe;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if ((id = file_in_folder_a_file_took(obj, vol, vid, &fx))
            != CNID_INVALID) {
        err = deleteid(obj, vid, id);
        probe = id;

        if (err == AFPERR_NOOBJ
                && cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf)) == NULL
                && CNID_ERRNO() == CNID_ERR_NOTFOUND) {
            ret = 0;
        } else {
            fprintf(stderr, "FPDeleteID in a folder a file took: %d\n", err);
        }
    }

    movecwd(vol, vol->v_root);
    file_in_folder_a_file_took_done(vol, &fx, id);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPResolveID answers kFPAccessDenied if the chdir's getcwd() is denied
 */
int utest_dircache_resolveid_getcwd_denied(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    char leaf[] = DC_LEAF;
    struct dc_fixture fx;
    const struct dir *parent;
    const struct dir *entry;
    int calls = 0;
    int err = -1;
    int ret = -1;

    /* a volume that walks enters a folder without a getcwd() */
    if (vol->v_rootfd >= 0 || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || createfile(obj, vid, fx.child_id, leaf) != AFP_OK
            || movecwd(vol, vol->v_root) != 0
            || (parent = dirlookup(vol, fx.child_id)) == NULL
            || (entry = dircache_search_by_name(vol, parent, leaf,
                                                strlen(leaf))) == NULL) {
        goto out;
    }

    /* every getcwd() fails, so that an earlier call that tolerates a
     * failure cannot take the only one */
    fault_inject_reset();
    fi.getcwd.armed = 1;
    fi.getcwd.fail_after = 0;
    fi.getcwd.fail_more = INT_MAX;
    fi.getcwd.errnum = EACCES;
    err = resolveid(obj, vid, entry->d_did);
    calls = fi.getcwd.calls;
    fault_inject_reset();

    /* the interposer saw no call, as where libc renames getcwd() */
    if (calls == 0) {
        ret = TEST_SKIP;
    } else if (err == AFPERR_ACCESS) {
        ret = 0;
    } else {
        fprintf(stderr, "FPResolveID with getcwd() denied: %d\n", err);
    }

out:
    fault_inject_reset();
    movecwd(vol, vol->v_root);
    delete (obj, vid, fx.child_id, leaf);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/* the backend's find, and the calls find_counted() passed on */
static int (*search_real_find)(struct _cnid_db *, const char *, size_t,
                               cnid_t, void *, size_t, bool *);
static int search_find_calls;

/*!
 * @brief Count a CNID-database search, then run the backend's
 *
 * @returns what the backend's find returns
 */
static int find_counted(struct _cnid_db *cdb, const char *name,
                        size_t namelen, cnid_t scope_did, void *buffer,
                        size_t buflen, bool *more_available)
{
    search_find_calls++;
    return search_real_find(cdb, name, namelen, scope_did, buffer, buflen,
                            more_available);
}

/*!
 * @brief A CNID-database search finds a file once its cached folder's row moved
 */
int utest_dircache_search_after_self_heal(AFPObj *obj, struct vol *vol,
                                          uint16_t vid)
{
    char leaf[] = DC_LEAF;
    unsigned char pos[16] = { 0 };
    const int searchdb = vol->v_flags & AFPVOL_SEARCHDB;
    struct dc_fixture fx;
    cnid_t pdid[2] = { CNID_INVALID, CNID_INVALID };
    uint32_t nrecs = 0;
    int moved = 0;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    vol->v_flags |= AFPVOL_SEARCHDB;

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || createfile(obj, vid, fx.child_id, leaf) != AFP_OK
            || movecwd(vol, vol->v_root) != 0
            || (moved = move_child_with_row(vol, &fx)) != 0
            || create_disk_file(fx.child) != 0) {
        goto out;
    }

    search_real_find = vol->v_cdb->cnid_find;
    search_find_calls = 0;
    vol->v_cdb->cnid_find = find_counted;
    err = catsearch_from(obj, vid, 2, pos, DC_LEAF, &nrecs, pdid);
    vol->v_cdb->cnid_find = search_real_find;

    if ((err == AFP_OK || err == AFPERR_EOF) && nrecs == 1
            && pdid[0] == fx.child_id && search_find_calls > 0) {
        ret = 0;
    } else {
        fprintf(stderr, "a catalog search after a self-heal: %d, %u found, "
                        "the first in %u, %d database searches\n", err,
                nrecs, ntohl(pdid[0]), search_find_calls);
    }

out:
    vol->v_flags = (vol->v_flags & ~AFPVOL_SEARCHDB) | searchdb;

    /* a move whose stat or row update failed leaves the folder aside under
     * a row that still names its old place, where the delete looks */
    if (moved != 0) {
        (void)rename(fx.child_aside, fx.child);
    }

    movecwd(vol, vol->v_root);
    delete (obj, vid, fx.child_id, leaf);
    unlink(fx.child);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief FPGetForkParms of a link names its folder once the folder's row moved
 */
int utest_dircache_forkparms_after_self_heal(AFPObj *obj, struct vol *vol,
                                             uint16_t vid)
{
    static char link[] = "dc-link";
    char lpath[MAXPATHLEN + 1];
    struct dc_fixture fx;
    const char *params;
    size_t len = 0;
    const struct dir *healed;
    uint16_t refnum = 0;
    cnid_t pdid = CNID_INVALID;
    cnid_t fnum = CNID_INVALID;
    cnid_t lid;
    int err = -1;
    int ret = -1;

    if (vol->v_flags & AFPVOL_FOLLOWSYM) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);
    snprintf(lpath, sizeof(lpath), "%s/%s", fx.child, link);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || symlink(DC_LEAF, lpath) != 0
            || openfork(obj, vid, fx.child_id, link, OPENFORK_DATA, OPENACC_RD,
                        &refnum) != AFP_OK
            || movecwd(vol, vol->v_root) != 0
            || move_child_with_row(vol, &fx) != 0
            || create_disk_file(fx.child) != 0) {
        goto out;
    }

    /* the file number needs a stat, which enters the folder first */
    err = getforkparams(obj, refnum,
                        (1 << FILPBIT_PDID) | (1 << FILPBIT_FNUM));
    params = reply_params(sizeof(uint16_t), &len);

    if (err == AFP_OK && len >= sizeof(pdid) + sizeof(fnum)) {
        memcpy(&pdid, params, sizeof(pdid));
        memcpy(&fnum, params + sizeof(pdid), sizeof(fnum));
    }

    healed = dirlookup(vol, fx.child_id);

    if (pdid == fx.child_id && fnum != CNID_INVALID
            && fnum == cnid_get(vol->v_cdb, fx.child_id, link, strlen(link))
            && healed != NULL
            && strcmp(cfrombstr(healed->d_u_name), DC_CHILD_ASIDE) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "fork parameters after a self-heal: %d, parent %u "
                        "of %u, file %u\n", err, ntohl(pdid),
                ntohl(fx.child_id), ntohl(fnum));
    }

out:

    if (refnum != 0) {
        closefork(obj, refnum);
    }

    movecwd(vol, vol->v_root);
    /* the file number gave the link a row */
    lid = cnid_get(vol->v_cdb, fx.child_id, link, strlen(link));

    if (lid != CNID_INVALID) {
        cnid_delete(vol->v_cdb, lid);
    }

    /* the link is in the child under either name, as far as the setup got */
    snprintf(lpath, sizeof(lpath), "%s/%s", fx.child, link);
    unlink(lpath);
    unlink(fx.child);
    snprintf(lpath, sizeof(lpath), "%s/%s", fx.child_aside, link);
    unlink(lpath);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A followed folder link whose row records the link answers by its DID
 */
int utest_dircache_followed_link_row(AFPObj *obj, struct vol *vol,
                                     uint16_t vid)
{
    char link[] = DC_CHILD;
    const int flags = vol->v_flags;
    struct dc_fixture fx;
    struct stat lst;
    cnid_t id = CNID_INVALID;
    int err = -1;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (createdir(obj, vid, DIRDID_ROOT, DC_FOLDER) != AFP_OK) {
        goto out;
    }

    fx.did = curdir->d_did;

    if (createdir(obj, vid, fx.did, DC_LEAF) != AFP_OK
            || symlink(DC_LEAF, fx.child) != 0 || lstat(fx.child, &lst) != 0
            || (id = cnid_add(vol->v_cdb, &lst, fx.did, link, strlen(link),
                              CNID_INVALID)) == CNID_INVALID
            || movecwd(vol, vol->v_root) != 0) {
        goto out;
    }

    vol->v_flags |= AFPVOL_FOLLOWSYM;
    err = getfiledirparms(obj, vid, id, "");
    vol->v_flags = flags;

    if (err == AFP_OK) {
        ret = 0;
    } else {
        fprintf(stderr, "followed link, its own row: %d\n", err);
    }

out:
    vol->v_flags = flags;
    movecwd(vol, vol->v_root);
    unlink(fx.child);

    if (id != CNID_INVALID) {
        drop_entry(vol, fx.did, link);
        cnid_delete(vol->v_cdb, id);
    }

    delete (obj, vid, DIRDID_ROOT, DC_FOLDER "/" DC_LEAF);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief movecwd() refuses a file entry, also one whose name a folder took
 */
int utest_dircache_file_entry_not_entered(AFPObj *obj, struct vol *vol,
                                          uint16_t vid)
{
    char child[] = DC_CHILD;
    struct dc_fixture fx;
    const struct dir *before;
    struct dir *entry;
    int rc = 0;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    if (cached_pair(obj, vol, vid, 0, &fx) == NULL
            || (entry = dirlookup(vol, fx.child_id)) == NULL
            || !(entry->d_flags & DIRF_ISFILE) || unlink(fx.child) != 0) {
        goto out;
    }

    if (mkdir(fx.child, 0750) != 0) { // NOSONAR (the scenario under test)
        goto out;
    }

    before = curdir;
    rc = movecwd(vol, entry);

    if (rc == -1 && afp_errno == AFPERR_NOOBJ && curdir == before) {
        ret = 0;
    } else {
        fprintf(stderr, "file entry, a folder at its name: movecwd %d, "
                        "afp_errno %d\n", rc, afp_errno);
    }

out:
    movecwd(vol, vol->v_root);
    drop_entry(vol, fx.did, child);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief cname() refuses a file entry whose uncached folder was replaced
 */
int utest_dircache_file_of_replaced_folder(AFPObj *obj, struct vol *vol,
                                           uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    char folder[] = DC_FOLDER;
    cnid_t gone;
    struct dc_fixture fx;
    struct dir *entry = NULL;
    const struct path *named;
    int ret = -1;
    dc_setup(obj, vol, &fx);

    /* the lookup by the CNID builds the file's entry */
    if (cached_pair(obj, vol, vid, 0, &fx) == NULL
            || (entry = dirlookup(vol, fx.child_id)) == NULL
            || movecwd(vol, vol->v_root) != 0) {
        goto out;
    }

    drop_entry(vol, DIRDID_ROOT, folder);

    if (replace_folder(fx.path, fx.aside) != 0) {
        goto out;
    }

    fx.moved = 1;
    named = cname(vol, entry, cnamewrap(""));
    gone = fx.did;

    if (named == NULL && afp_errno == AFPERR_NOOBJ
            && dircache_lookup_did(vol, fx.did) == NULL
            && cnid_resolve(vol->v_cdb, &gone, buf, sizeof(buf)) == NULL
            && CNID_ERRNO() == CNID_ERR_NOTFOUND) {
        ret = 0;
    } else {
        fprintf(stderr, "file of a replaced folder: %s, afp_errno %d\n",
                named == NULL ? "no path" : "a path", afp_errno);
    }

out:
    movecwd(vol, vol->v_root);

    if (entry != NULL && entry->d_did != CNID_INVALID) {
        dir_remove(vol, entry, 0);
    }

    dir_free_invalid_q();
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief A folder symlink by its CNID answers as the link, as it does by name
 */
int utest_dircache_folder_link_by_cnid(AFPObj *obj, struct vol *vol,
                                       uint16_t vid)
{
    char name[] = DC_LEAF;
    char path[MAXPATHLEN + 1];
    struct dc_fixture fx;
    struct stat st;
    const char *head;
    const char *params;
    size_t hlen = 0;
    size_t len = 0;
    cnid_t id = CNID_INVALID;
    cnid_t fnum = CNID_INVALID;
    int err;
    int ret = -1;

    if (vol->v_flags & AFPVOL_FOLLOWSYM) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);
    folder_path(vol, path, DC_LEAF);

    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || symlink(DC_CHILD, path) != 0 || lstat(path, &st) != 0
            || (id = cnid_add(vol->v_cdb, &st, fx.did, name, strlen(name),
                              CNID_INVALID)) == CNID_INVALID
            || getfiledirparms(obj, vid, fx.did, name) != AFP_OK) {
        goto out;
    }

    err = getfiledirparms(obj, vid, id, "");
    head = reply_params(2 * sizeof(uint16_t), &hlen);
    params = reply_params(REPLY_FILEDIRPARMS, &len);

    if (err == AFP_OK && len >= sizeof(fnum)) {
        memcpy(&fnum, params, sizeof(fnum));
    }

    /* a file's reply, whose first parameter is the file's number */
    if (err == AFP_OK && hlen > 0 && !(head[0] & FILDIRBIT_ISDIR)
            && fnum == id) {
        ret = 0;
    } else {
        fprintf(stderr, "folder link by its CNID: %d\n", err);
    }

out:

    if (id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, id);
    }

    unlink(path);
    dc_teardown(obj, vid, &fx);
    return ret;
}

/*!
 * @brief An uncached folder on another device than its row: kFPObjectNotFound
 */
int utest_dircache_other_device_by_did(AFPObj *obj, struct vol *vol,
                                       uint16_t vid)
{
    static char buf[CNID_MAX_PATH_LEN];
    struct dc_fixture fx;
    struct stat st;
    cnid_t gone;
    int err = AFP_OK;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)
            || (vol->v_cdb->cnid_db_flags & CNID_FLAG_NODEV)) {
        return TEST_SKIP;
    }

    dc_setup(obj, vol, &fx);

    if (uncached_folder(obj, vol, vid, &fx) != 0 || stat(fx.path, &st) != 0) {
        goto out;
    }

    fault_inject_reset();
    fi.stat_dev.path = fx.path;
    fi.stat_dev.dev = st.st_dev + 1;

    if (ostat(fx.path, &st, vol_syml_opt(vol)) != 0
            || st.st_dev != fi.stat_dev.dev) {
        ret = TEST_SKIP;
        goto out;
    }

    err = getfiledirparms(obj, vid, fx.did, "");
    fault_inject_reset();
    gone = fx.did;

    if (err == AFPERR_NOOBJ
            && cnid_resolve(vol->v_cdb, &gone, buf, sizeof(buf)) == NULL
            && CNID_ERRNO() == CNID_ERR_NOTFOUND) {
        ret = 0;
    } else {
        fprintf(stderr, "another device by the DID: %d\n", err);
    }

out:
    fault_inject_reset();
    dc_teardown(obj, vid, &fx);
    return ret;
}
