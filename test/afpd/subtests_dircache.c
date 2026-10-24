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
#include <atalk/volume.h>

#include "acls.h"
#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "subtests_dircache.h"
#include "test.h"

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
 * @brief A folder replaced under its name takes its own CNID at the refresh
 *        and its old children leave the cache, at once here, as the harness
 *        runs no idle worker; the old DID names nothing
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
            || stat(fx.path, &st) != 0) {
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
 *        cache with its row: the folder's CNID names no object
 */
int utest_dircache_rowed_file_takes_folder_name(AFPObj *obj, struct vol *vol,
                                                uint16_t vid)
{
    static char file[] = "dc-file";
    struct dc_fixture fx;
    char from[MAXPATHLEN + 1];
    struct stat st;
    int ret = -1;
    dc_setup(obj, vol, &fx);
    folder_path(vol, from, file);

    /* the folder moves aside, and a file with its row moves onto its name:
     * the lookup at the name matches the folder's row and the file's */
    if (cached_pair(obj, vol, vid, 1, &fx) == NULL
            || create_disk_file(from) != 0 || stat(from, &st) != 0
            || cnid_add(vol->v_cdb, &st, fx.did, file, strlen(file),
                        CNID_INVALID) == CNID_INVALID
            || rename(fx.child, fx.child_aside) != 0
            || rename(from, fx.child) != 0) { // NOSONAR (the scenario under test)
        goto out;
    }

    /* the lookup by the folder's CNID validates its entry */
    obj->options.dircache_validation_freq = 1;

    if (dirlookup(vol, fx.child_id) == NULL) {
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
 * @brief A cnid_lookup() that fails as a backend that is gone fails
 */
static cnid_t gone_lookup(struct _cnid_db *cdb _U_,
                          const struct stat *st _U_, cnid_t did _U_,
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
    cnid_t (*lookup)(struct _cnid_db *, const struct stat *, cnid_t,
                     const char *, size_t) = vol->v_cdb->cnid_lookup;
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

    vol->v_cdb->cnid_lookup = gone_lookup;
    dir_stat_refresh(vol, entry, &st);
    vol->v_cdb->cnid_lookup = lookup;

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
 *        at the listing's stat; the folder found by name lists the new file
 *        from its own stat, under a CNID of its own
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

    /* by name the folder answers under its own CNID, and its listing serves
     * the new file from its own stat */
    if (getfiledirparms(obj, vid, DIRDID_ROOT, name) != AFP_OK
            || (entry = dircache_search_by_name(vol, vol->v_root, name,
                                                strlen(name))) == NULL
            || entry->d_did == fx.did
            || enumerate_range(obj, vid, entry->d_did, 1 << FILPBIT_FNUM, 0,
                               1, 40) != AFP_OK
            || (file = dircache_search_by_name(vol, entry, child,
                                               strlen(child))) == NULL
            || file->dcache_ino != st.st_ino || file->d_did == fx.child_id) {
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
 *        object at its path leaves its stat fields, and a strict lookup
 *        finds it where the rename took it, leaves the other folders' rows
 *        alone and sends no refresh hint
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

    /* an entry with a known AppleDouble state that took this stat would
     * serve its later mtime */
    later.st_mtime += 3600;
    entry->dcache_rlen = AD_RLEN_NO_RFORK;
    memcpy(mdate, entry->dcache_filedatesi + AD_DATE_MODIFY, sizeof(mdate));

    if (dir_modify(vol, entry, &stat_args) != 0
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
 *        answers kFPObjectNotFound when no row can be made for the new
 *        folder, and the entry keeps its DID for the next refresh
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

    if (rc == AFPERR_NOOBJ && entry->d_did == fx.did) {
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

    if (hint_came(fds[0], CACHE_HINT_REFRESH, fx.did)) {
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
