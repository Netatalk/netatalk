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
 * @brief Listing, conversion and movecwd() walk tests against the afpd_test
 *        volume
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/cnid.h>
#include <atalk/compat.h>
#include <atalk/directory.h>
#include <atalk/globals.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "faultinject.h"
#include "file.h"
#include "subtests_enumerate.h"
#include "test.h"
#include "test_capabilities.h"
#include "virtual_icon.h"

/* 120 files list in three batches of 40; their saved names outgrow the
 * listing buffer's 2048-byte start */
#define ENUM_FILES 120
#define ENUM_BATCH 40
#define ENUM_FILE  "listing-entry-%03d"
/* more directories than afpd keeps listings for */
#define ENUM_DIRS  9

static const char *const dir_names[ENUM_DIRS] = {
    "A", "B", "C", "D", "E", "F", "G", "H", "I"
};
static const int dir_files[ENUM_DIRS] = {
    ENUM_FILES, ENUM_FILES, 1, 1, 1, 1, 1, 1, 1
};

/*!
 * @brief A directory at the volume root holding nfiles files
 *
 * Built through the requests a client sends; afp_createdir() leaves curdir
 * in it.
 */
static int make_dir(AFPObj *obj, uint16_t vid, const char *name, int nfiles,
                    cnid_t *did)
{
    char path[32];

    if (createdir(obj, vid, DIRDID_ROOT, name) != AFP_OK) {
        return -1;
    }

    *did = curdir->d_did;

    for (int i = 0; i < nfiles; i++) {
        snprintf(path, sizeof(path), ENUM_FILE, i);

        if (createfile(obj, vid, *did, path) != AFP_OK) {
            return -1;
        }
    }

    return 0;
}

/*!
 * @brief Delete make_dir()'s files and directory; nothing for a did of 0
 */
static void remove_dir(AFPObj *obj, uint16_t vid, const char *name,
                       int nfiles, cnid_t did)
{
    char path[32];

    if (did == 0) {
        return;
    }

    for (int i = 0; i < nfiles; i++) {
        snprintf(path, sizeof(path), ENUM_FILE, i);
        delete (obj, vid, did, path);
    }

    delete (obj, vid, DIRDID_ROOT, name);
}

/*!
 * @brief The ENUM_DIRS directories; did[] holds 0 for one not created
 */
static int enum_setup(AFPObj *obj, uint16_t vid, cnid_t did[ENUM_DIRS])
{
    for (int i = 0; i < ENUM_DIRS; i++) {
        if (make_dir(obj, vid, dir_names[i], dir_files[i], &did[i]) != 0) {
            return -1;
        }
    }

    return 0;
}

/*!
 * @brief Remove what enum_setup() created
 */
static void enum_teardown(AFPObj *obj, uint16_t vid, cnid_t did[ENUM_DIRS])
{
    for (int i = 0; i < ENUM_DIRS; i++) {
        remove_dir(obj, vid, dir_names[i], dir_files[i], did[i]);
    }
}

/*!
 * @brief One batch of ENUM_BATCH file ids from sindex; the AFP result
 */
static int enum_batch(AFPObj *obj, uint16_t vid, cnid_t did, uint32_t sindex)
{
    return enumerate_range(obj, vid, did, 1 << FILPBIT_FNUM, 0, sindex,
                           ENUM_BATCH);
}

/*!
 * @brief Listing reads, continuation rereads and names read are counted
 */
int utest_enumerate_rereads_counted(AFPObj *obj, uint16_t vid)
{
    struct enumerate_stats before;
    struct enumerate_stats after;
    cnid_t did[ENUM_DIRS] = { 0 };
    int ret = -1;

    if (enum_setup(obj, vid, did) != 0) {
        goto out;
    }

    enumerate_stats_get(&before);

    /* every directory's first batch, then A's second, which finds A's
     * listing evicted */
    for (int i = 0; i < ENUM_DIRS; i++) {
        if (enum_batch(obj, vid, did[i], 1) != AFP_OK) {
            goto out;
        }
    }

    if (enum_batch(obj, vid, did[0], 1 + ENUM_BATCH) != AFP_OK) {
        goto out;
    }

    enumerate_stats_get(&after);

    /* the names of A and B, one in each of the seven others, A's again */
    if (after.reads - before.reads != ENUM_DIRS + 1
            || after.rereads - before.rereads != 1
            || after.names - before.names != 3 * ENUM_FILES + ENUM_DIRS - 2) {
        goto out;
    }

    ret = 0;
out:
    enum_teardown(obj, vid, did);
    return ret;
}

/*!
 * @brief A name longer than 255 bytes stays out of its folder's listing
 */
int utest_enumerate_long_name_skipped(AFPObj *obj, uint16_t vid)
{
    /* 86 hiragana letters: 258 bytes of UTF-8, 86 UTF-16 units */
    char name[3 * 86 + 1];
    char path[MAXPATHLEN + 1];
    cnid_t did = 0;
    int fd;
    int ret = -1;

    for (int i = 0; i < 86; i++) {
        memcpy(name + 3 * i, "\xe3\x81\x82", 3);
    }

    name[3 * 86] = '\0';

    if (make_dir(obj, vid, dir_names[0], 1, &did) != 0) {
        goto out;
    }

    snprintf(path, sizeof(path), "%s/%s", cfrombstr(curdir->d_fullpath), name);

    /* most local filesystems cap a name at 255 bytes */
    if ((fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600)) < 0) {
        ret = errno == ENAMETOOLONG ? TEST_SKIP : -1;
        goto out;
    }

    close(fd);

    /* the folder lists its one other file, then ends, and counts one
     * offspring */
    if (enum_batch(obj, vid, did, 1) == AFP_OK
            && enum_batch(obj, vid, did, 2) == AFPERR_NOOBJ
            && curdir->d_offcnt == 1) {
        ret = 0;
    }

    unlink(path);
out:
    remove_dir(obj, vid, dir_names[0], 1, did);
    return ret;
}

/*!
 * @brief The three batches of n 120-file listings, the listings taking
 *        turns; 0 when each batch returns AFP_OK
 */
static int enum_list(AFPObj *obj, uint16_t vid, const cnid_t *did, int n)
{
    for (uint32_t sindex = 1; sindex <= 1 + 2 * ENUM_BATCH;
            sindex += ENUM_BATCH) {
        for (int i = 0; i < n; i++) {
            if (enum_batch(obj, vid, did[i], sindex) != AFP_OK) {
                return -1;
            }
        }
    }

    return 0;
}

/*!
 * @brief A 120-file listing to its end, then one batch past its last file;
 *        0 when they return AFP_OK and then AFPERR_NOOBJ
 */
static int enum_through(AFPObj *obj, uint16_t vid, cnid_t did)
{
    if (enum_list(obj, vid, &did, 1) != 0
            || enum_batch(obj, vid, did, 1 + 3 * ENUM_BATCH) != AFPERR_NOOBJ) {
        return -1;
    }

    return 0;
}

/*!
 * @brief A listing attempts a file's conversion only while its folder has
 *        a .AppleDouble, and a cached folder's once until its ctime changes
 */
int utest_enumerate_conversion_attempts(AFPObj *obj, struct vol *vol)
{
    char adpath[MAXPATHLEN + 1];
    char folder[MAXPATHLEN + 1];
    uint16_t vid = vol->v_vid;
    int noconv = vol->v_flags & AFPVOL_NOV2TOEACONV;
    const int freq = obj->options.dircache_validation_freq;
    cnid_t did = 0;
    int ret = -1;

    /* ad_convert() converts adouble:ea volumes only; a volume on a
     * filesystem without user xattrs is adouble:v2 */
    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)
            || vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    snprintf(adpath, sizeof(adpath), "%s/%s/.AppleDouble", vol->v_path,
             dir_names[0]);
    snprintf(folder, sizeof(folder), "%s/%s/folder-0", vol->v_path,
             dir_names[0]);

    if (make_dir(obj, vid, dir_names[0], ENUM_FILES, &did) != 0
            || createdir(obj, vid, did, "folder-0") != AFP_OK
            || createdir(obj, vid, did, "folder-1") != AFP_OK) {
        goto out;
    }

    vol->v_flags &= ~AFPVOL_NOV2TOEACONV;
    fault_inject_reset();
    /* count every unlink(), failing none */
    fi.unlink.armed = 1;
    fi.unlink.fail_after = -1;

    /* the interposer must see this file's own call, else the test skips */
    if (unlink(adpath) == 0 || fi.unlink.calls != 1) {
        ret = TEST_SKIP;
        goto out;
    }

    /* no .AppleDouble: each cached folder converts once, one unlink() of
     * its absent .Parent header, and no file is attempted */
    fi.unlink.calls = 0;

    if (enum_through(obj, vid, did) != 0 || fi.unlink.calls != 2) {
        goto out;
    }

    fi.unlink.calls = 0;

    if (enum_through(obj, vid, did) != 0 || fi.unlink.calls != 0) {
        goto out;
    }

    /* with a .AppleDouble every file is attempted, one unlink() of its
     * absent header apiece, and the .AppleDouble itself, which an
     * adouble:ea listing shows as a folder; the listing's end removes the
     * empty directory, so the next listing attempts no file */
    fi.unlink.calls = 0;

    if (mkdir(adpath, 0700) != 0 || enum_through(obj, vid, did) != 0
            || fi.unlink.calls != ENUM_FILES + 1) {
        goto out;
    }

    fi.unlink.calls = 0;

    if (enum_through(obj, vid, did) != 0 || fi.unlink.calls != 0) {
        goto out;
    }

    /* a folder whose ctime changed is attempted once more, here where
     * every dircache lookup checks the entry's stat */
    obj->options.dircache_validation_freq = 1;
    fi.unlink.calls = 0;

    if (next_second(folder) == 0 && enum_through(obj, vid, did) == 0
            && fi.unlink.calls == 1) {
        ret = 0;
    }

out:
    obj->options.dircache_validation_freq = freq;
    fault_inject_reset();
    vol->v_flags |= noconv;
    rmdir(adpath);

    if (did != 0) {
        delete (obj, vid, did, "folder-0");
        delete (obj, vid, did, "folder-1");
    }

    remove_dir(obj, vid, dir_names[0], ENUM_FILES, did);
    return ret;
}

/*!
 * @brief A cached file whose hex-encoded name a listing converts keeps its
 *        CNID, and a batch that comes back to a converted entry lists it
 */
int utest_enumerate_dehex_keeps_cnid(AFPObj *obj, struct vol *vol)
{
    /* names netatalk 2 hex-encoded, which the conversion renames */
    static char hexed[2][6] = { "x:2ey", "z:2ew" };
    static char plain[2][4] = { "x.y", "z.w" };
    char path[MAXPATHLEN + 1];
    uint16_t vid = vol->v_vid;
    int noconv = vol->v_flags & AFPVOL_NOV2TOEACONV;
    struct dir *folder = NULL;
    const struct dir *entry;
    cnid_t did = 0;
    cnid_t id;
    int fd;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || make_dir(obj, vid, dir_names[0], 0, &did) != 0) {
        ret = vol->v_adouble != AD_VERSION_EA ? TEST_SKIP : -1;
        goto out;
    }

    folder = curdir;

    for (int i = 0; i < 2; i++) {
        snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath),
                 hexed[i]);

        if ((fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600)) < 0) {
            goto out;
        }

        close(fd);
    }

    /* a listing with conversion off caches the files and gives them CNIDs */
    if (enum_batch(obj, vid, did, 1) != AFP_OK
            || (entry = dircache_search_by_name(vol, folder, hexed[0], 5))
            == NULL) {
        goto out;
    }

    id = entry->d_did;
    vol->v_flags &= ~AFPVOL_NOV2TOEACONV;

    /* the converting listing, then its second entry asked for again */
    if (enum_batch(obj, vid, did, 1) == AFP_OK
            && cnid_get(vol->v_cdb, did, plain[0], 3) == id
            && enum_batch(obj, vid, did, 2) == AFP_OK) {
        ret = 0;
    }

out:
    vol->v_flags |= noconv;

    for (int i = 0; folder != NULL && i < 2; i++) {
        delete (obj, vid, did, plain[i]);
        snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath),
                 hexed[i]);
        unlink(path);
    }

    remove_dir(obj, vid, dir_names[0], 0, did);
    return ret;
}

/*!
 * @brief A cached file whose hex-encoded name decodes to one no rename can
 *        take keeps its name and its CNID, and its folder keeps its own
 */
int utest_enumerate_unrenamed_keeps_cnid(AFPObj *obj, struct vol *vol)
{
    /* ":2e" decodes to the folder itself and "x:2ey" to the folder x.y */
    static char hexed[2][6] = { ":2e", "x:2ey" };
    char path[MAXPATHLEN + 1];
    uint16_t vid = vol->v_vid;
    int noconv = vol->v_flags & AFPVOL_NOV2TOEACONV;
    struct dir *folder = NULL;
    const struct dir *entry;
    cnid_t did = 0;
    cnid_t id[2] = { CNID_INVALID, CNID_INVALID };
    int fd;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || make_dir(obj, vid, dir_names[0], 0, &did) != 0) {
        ret = vol->v_adouble != AD_VERSION_EA ? TEST_SKIP : -1;
        goto out;
    }

    folder = curdir;
    snprintf(path, sizeof(path), "%s/x.y", cfrombstr(folder->d_fullpath));

    if (mkdir(path, 0700) != 0) {
        goto out;
    }

    for (int i = 0; i < 2; i++) {
        snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath),
                 hexed[i]);

        if ((fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600)) < 0) {
            goto out;
        }

        close(fd);
    }

    /* a listing with conversion off caches the files and gives them CNIDs */
    if (enum_batch(obj, vid, did, 1) != AFP_OK) {
        goto out;
    }

    for (int i = 0; i < 2; i++) {
        if ((entry = dircache_search_by_name(vol, folder, hexed[i],
                                             strlen(hexed[i]))) == NULL) {
            goto out;
        }

        id[i] = entry->d_did;
    }

    vol->v_flags &= ~AFPVOL_NOV2TOEACONV;
    snprintf(path, sizeof(path), "%s", dir_names[0]);

    if (enum_batch(obj, vid, did, 1) == AFP_OK
            && cnid_get(vol->v_cdb, did, hexed[0], 3) == id[0]
            && cnid_get(vol->v_cdb, did, hexed[1], 5) == id[1]
            && cnid_get(vol->v_cdb, DIRDID_ROOT, path, 1) == did) {
        ret = 0;
    }

out:
    vol->v_flags |= noconv;

    for (int i = 0; folder != NULL && i < 2; i++) {
        snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath),
                 hexed[i]);
        unlink(path);

        if (id[i] != CNID_INVALID) {
            cnid_delete(vol->v_cdb, id[i]);
        }
    }

    if (folder != NULL) {
        delete (obj, vid, did, "x.y");
    }

    remove_dir(obj, vid, dir_names[0], 0, did);
    return ret;
}

/*!
 * @brief A cached folder whose decoded name a file holds is renamed by a
 *        later listing once the name is free
 */
int utest_enumerate_folder_rename_retried(AFPObj *obj, struct vol *vol)
{
    /* the folder's hex-encoded name, and the decoded one a file holds */
    static char hexed[] = "w:2ev";
    static char plain[] = "w.v";
    uint16_t vid = vol->v_vid;
    int noconv = vol->v_flags & AFPVOL_NOV2TOEACONV;
    const struct dir *entry;
    struct stat st;
    cnid_t did = 0;
    cnid_t id = CNID_INVALID;
    int dfd = -1;
    int fd;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || make_dir(obj, vid, dir_names[0], 0, &did) != 0) {
        ret = vol->v_adouble != AD_VERSION_EA ? TEST_SKIP : -1;
        goto out;
    }

    if ((dfd = open(cfrombstr(curdir->d_fullpath), O_RDONLY | O_DIRECTORY)) < 0
            || mkdirat(dfd, hexed, 0700) != 0
            || (fd = openat(dfd, plain, O_WRONLY | O_CREAT | O_EXCL,
                            0600)) < 0) {
        goto out;
    }

    close(fd);

    /* a listing with conversion off caches the folder */
    if (enumerate_range(obj, vid, did, 1 << FILPBIT_FNUM, 1 << DIRPBIT_DID, 1,
                        ENUM_BATCH) != AFP_OK
            || (entry = dircache_search_by_name(vol, curdir, hexed,
                                                strlen(hexed))) == NULL) {
        goto out;
    }

    id = entry->d_did;
    vol->v_flags &= ~AFPVOL_NOV2TOEACONV;

    /* the file holds w.v through the first converting listing; the one
     * after it goes renames the folder */
    if (enumerate_range(obj, vid, did, 1 << FILPBIT_FNUM, 1 << DIRPBIT_DID, 1,
                        ENUM_BATCH) == AFP_OK
            && unlinkat(dfd, plain, 0) == 0
            && enumerate_range(obj, vid, did, 1 << FILPBIT_FNUM,
                               1 << DIRPBIT_DID, 1, ENUM_BATCH) == AFP_OK
            && fstatat(dfd, plain, &st, 0) == 0 && S_ISDIR(st.st_mode)) {
        ret = 0;
    }

out:
    vol->v_flags |= noconv;

    if (dfd >= 0) {
        unlinkat(dfd, plain, 0);
        unlinkat(dfd, plain, AT_REMOVEDIR);
        unlinkat(dfd, hexed, AT_REMOVEDIR);
        close(dfd);
    }

    if (id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, id);
    }

    remove_dir(obj, vid, dir_names[0], 0, did);
    return ret;
}

/*!
 * @brief One file id from sindex; the AFP result
 */
static int enum_one(AFPObj *obj, uint16_t vid, cnid_t did, uint32_t sindex)
{
    return enumerate_range(obj, vid, did, 1 << FILPBIT_FNUM, 0, sindex, 1);
}

/*!
 * @brief Indexes 2 to 4 of a three-file listing, one per batch: 0 when 2
 *        and 3 return AFP_OK and 4 returns AFPERR_NOOBJ
 */
static int enum_rest(AFPObj *obj, uint16_t vid, cnid_t did)
{
    if (enum_one(obj, vid, did, 2) != AFP_OK
            || enum_one(obj, vid, did, 3) != AFP_OK
            || enum_one(obj, vid, did, 4) != AFPERR_NOOBJ) {
        return -1;
    }

    return 0;
}

/*!
 * @brief A batch that fails part-way leaves the next one at its index
 */
int utest_enumerate_failed_batch_keeps_index(AFPObj *obj, uint16_t vid)
{
    cnid_t did = 0;
    int ret = -1;

    /* a settled folder keeps its listing from batch to batch */
    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0
            || settle(cfrombstr(curdir->d_fullpath)) != 0
            || enum_one(obj, vid, did, 1) != AFP_OK) {
        goto out;
    }

    /* getfilparams() refuses file bitmap bit 12 on the batch's first file */
    if (enumerate_range(obj, vid, did, 1 << 12, 0, 2, 1) != AFPERR_BITMAP
            || enum_rest(obj, vid, did) != 0) {
        goto out;
    }

    ret = 0;
out:
    remove_dir(obj, vid, dir_names[0], 3, did);
    return ret;
}

/*!
 * @brief A listing whose read fails is read again by its next batch
 */
int utest_enumerate_failed_read_rereads(AFPObj *obj, struct vol *vol)
{
    uint16_t vid = vol->v_vid;
    int follow = vol->v_flags & AFPVOL_FOLLOWSYM;
    struct enumerate_stats before;
    struct enumerate_stats after;
    struct rlimit rl;
    struct rlimit low;
    cnid_t did = 0;
    int fd;
    int rc;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0
            || enum_one(obj, vid, did, 1) != AFP_OK
            || getrlimit(RLIMIT_NOFILE, &rl) != 0
            || (fd = open("/dev/null", O_RDONLY)) < 0) {
        goto out;
    }

    /* fd is the lowest free descriptor: with the soft limit there, the
     * restarted listing's opendir() fails, for root as well. A volume that
     * follows symlinks enters the directory with chdir() alone, which takes
     * no descriptor. A change of the folder's times has the restarted
     * listing read the folder. */
    close(fd);
    low = rl;
    low.rlim_cur = (rlim_t)fd;
    vol->v_flags |= AFPVOL_FOLLOWSYM;

    if (utimensat(AT_FDCWD, cfrombstr(curdir->d_fullpath), NULL, 0) != 0) {
        goto out;
    }

    enumerate_stats_get(&before);

    if (setrlimit(RLIMIT_NOFILE, &low) != 0) {
        goto out;
    }

    rc = enum_one(obj, vid, did, 1);

    if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
        goto out;
    }

    enumerate_stats_get(&after);

    /* the request reached the read, which failed */
    if (rc != AFPERR_MISC || after.reads - before.reads != 1
            || enum_rest(obj, vid, did) != 0) {
        goto out;
    }

    ret = 0;
out:
    vol->v_flags = (vol->v_flags & ~AFPVOL_FOLLOWSYM) | follow;
    remove_dir(obj, vid, dir_names[0], 3, did);
    return ret;
}

/*!
 * @brief A read whose readdir() fails part-way fails, and the next batch
 *        reads the directory again
 */
int utest_enumerate_read_error_fails(AFPObj *obj, struct vol *vol)
{
    uint16_t vid = vol->v_vid;
    struct enumerate_stats before;
    struct enumerate_stats after;
    DIR *dp = NULL;
    cnid_t did = 0;
    int rc;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0
            || (dp = opendir(vol->v_path)) == NULL) {
        goto out;
    }

    fault_inject_reset();
    fi.readdir.armed = 1;
    fi.readdir.errnum = EIO;

    /* the interposer must see this file's own call, else the test skips */
    if (readdir(dp) != NULL || fi.readdir.calls != 1) {
        ret = TEST_SKIP;
        goto out;
    }

    /* the listing's read takes one name, then fails with an error that has
     * no AFP result of its own */
    fi.readdir.fail_after = 1;
    enumerate_stats_get(&before);
    rc = enum_one(obj, vid, did, 1);
    fault_inject_reset();
    enumerate_stats_get(&after);

    if (rc == AFPERR_NODIR && after.reads - before.reads == 1
            && enum_rest(obj, vid, did) == 0) {
        ret = 0;
    }

out:
    fault_inject_reset();

    if (dp != NULL) {
        closedir(dp);
    }

    remove_dir(obj, vid, dir_names[0], 3, did);
    return ret;
}

/*!
 * @brief Let the volume root's first n folders of dir_names settle
 *
 * A stat fixes the ctime a filesystem sets lazily; the folder made last
 * settles last.
 *
 * @returns 0, -1 when a stat fails or the last folder does not settle
 */
static int enum_settle(const struct vol *vol, int n)
{
    char path[MAXPATHLEN + 1] = "";
    struct stat st;

    for (int i = 0; i < n; i++) {
        snprintf(path, sizeof(path), "%s/%s", vol->v_path, dir_names[i]);

        if (stat(path, &st) != 0) {
            return -1;
        }
    }

    return settle(path);
}

/*!
 * @brief A listing of the volume root takes no slot: each request for the
 *        root reads it, and the eight folders' listings that fill the slots
 *        all serve again after it
 */
int utest_enumerate_root_kept_nowhere(AFPObj *obj, struct vol *vol)
{
    uint16_t vid = vol->v_vid;
    struct enumerate_stats before;
    struct enumerate_stats mid;
    struct enumerate_stats after;
    cnid_t did[ENUM_DIRS] = { 0 };
    int ret = -1;

    /* the root settles too, so a listing of it kept anywhere would serve */
    if (enum_setup(obj, vid, did) != 0
            || enum_settle(vol, ENUM_DIRS - 1) != 0
            || settle(vol->v_path) != 0) {
        goto out;
    }

    /* A to H, settled, fill the session's eight slots */
    for (int i = 0; i < ENUM_DIRS - 1; i++) {
        if (enum_batch(obj, vid, did[i], 1) != AFP_OK) {
            goto out;
        }
    }

    enumerate_stats_get(&before);

    if (enumerate_range(obj, vid, DIRDID_ROOT, 1 << FILPBIT_FNUM,
                        1 << DIRPBIT_DID, 1, ENUM_BATCH) != AFP_OK
            || enumerate_range(obj, vid, DIRDID_ROOT, 1 << FILPBIT_FNUM,
                               1 << DIRPBIT_DID, 1, ENUM_BATCH) != AFP_OK) {
        goto out;
    }

    enumerate_stats_get(&mid);

    for (int i = 0; i < ENUM_DIRS - 1; i++) {
        if (enum_batch(obj, vid, did[i], 1) != AFP_OK) {
            goto out;
        }
    }

    enumerate_stats_get(&after);
    ret = mid.reads - before.reads == 2 && after.reads == mid.reads ? 0 : -1;
out:
    enum_teardown(obj, vid, did);
    return ret;
}

/*!
 * @brief Eight listings stay saved: two in interleaved batches read their
 *        directories once each, and six more leave the first one saved
 */
int utest_enumerate_interleaved_reads_once(AFPObj *obj, struct vol *vol)
{
    uint16_t vid = vol->v_vid;
    struct enumerate_stats before;
    struct enumerate_stats mid;
    struct enumerate_stats after;
    cnid_t did[ENUM_DIRS] = { 0 };
    int ret = -1;

    /* a listing read within a granule of its folder's last change reads it
     * again at its next batch, so A and B settle first */
    if (enum_setup(obj, vid, did) != 0 || enum_settle(vol, 2) != 0) {
        goto out;
    }

    enumerate_stats_get(&before);

    /* A and B interleaved to their last batches: one read each */
    if (enum_list(obj, vid, did, 2) != 0) {
        goto out;
    }

    enumerate_stats_get(&mid);

    /* C to H make eight listings with A and B */
    for (int i = 2; i < ENUM_DIRS - 1; i++) {
        if (enum_batch(obj, vid, did[i], 1) != AFP_OK) {
            goto out;
        }
    }

    /* A's names are still saved: the batch past their end reads nothing */
    if (enum_batch(obj, vid, did[0], 1 + 3 * ENUM_BATCH) != AFPERR_NOOBJ) {
        goto out;
    }

    enumerate_stats_get(&after);

    if (mid.reads - before.reads != 2
            || after.reads - mid.reads != ENUM_DIRS - 3
            || after.rereads - before.rereads != 0) {
        goto out;
    }

    ret = 0;
out:
    enum_teardown(obj, vid, did);
    return ret;
}

/*!
 * @brief One entry from sindex, bitmap[] the file and folder bitmaps; the
 *        AFP result
 */
static int enum_at(AFPObj *obj, uint16_t vid, cnid_t did,
                   const uint16_t bitmap[2], uint32_t sindex)
{
    return enumerate_range(obj, vid, did, bitmap[0], bitmap[1], sindex, 1);
}

/*!
 * @brief A listing of one kind keeps its index when it is read again
 */
int utest_enumerate_one_kind_keeps_index(AFPObj *obj, uint16_t vid)
{
    /* the bitmaps of a files-only and of a folders-only listing */
    static const uint16_t kinds[2][2] = {
        { 1 << FILPBIT_FNUM, 0 },
        { 0, 1 << DIRPBIT_DID }
    };
    char folder[MAXPATHLEN + 1];
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 2, &did) != 0) {
        goto out;
    }

    snprintf(folder, sizeof(folder), "%s", cfrombstr(curdir->d_fullpath));

    if (createdir(obj, vid, did, "folder-0") != AFP_OK
            || createdir(obj, vid, did, "folder-1") != AFP_OK) {
        goto out;
    }

    /* two entries of each kind: once the batch at index 3 ends the listing,
     * a change of the folder's times has index 2 asked again read the
     * folder anew and walk over its first name, a file or a folder */
    for (int i = 0; i < 2; i++) {
        const uint16_t *k = kinds[i];

        if (enum_at(obj, vid, did, k, 1) != AFP_OK
                || enum_at(obj, vid, did, k, 2) != AFP_OK
                || enum_at(obj, vid, did, k, 3) != AFPERR_NOOBJ
                || utimensat(AT_FDCWD, folder, NULL, 0) != 0
                || enum_at(obj, vid, did, k, 2) != AFP_OK
                || enum_at(obj, vid, did, k, 3) != AFPERR_NOOBJ) {
            goto out;
        }
    }

    ret = 0;
out:

    if (did != 0) {
        delete (obj, vid, did, "folder-0");
        delete (obj, vid, did, "folder-1");
    }

    remove_dir(obj, vid, dir_names[0], 2, did);
    return ret;
}

/*!
 * @brief A batch of the other kind at the saved index counts its own kind
 *        from the listing's first name
 */
int utest_enumerate_kind_change_rewinds(AFPObj *obj, uint16_t vid)
{
    static const uint16_t files[2] = { 1 << FILPBIT_FNUM, 0 };
    static const uint16_t folders[2] = { 0, 1 << DIRPBIT_DID };
    char folder[MAXPATHLEN + 1];
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 1, &did) != 0) {
        goto out;
    }

    snprintf(folder, sizeof(folder), "%s", cfrombstr(curdir->d_fullpath));

    /* a settled folder keeps its listing from batch to batch */
    if (createdir(obj, vid, did, "folder-0") != AFP_OK
            || settle(folder) != 0) {
        goto out;
    }

    /* one file and one folder: each kind ends at index 2, which follows
     * the other kind's first batch */
    if (enum_at(obj, vid, did, files, 1) == AFP_OK
            && enum_at(obj, vid, did, folders, 2) == AFPERR_NOOBJ
            && enum_at(obj, vid, did, folders, 1) == AFP_OK
            && enum_at(obj, vid, did, files, 2) == AFPERR_NOOBJ) {
        ret = 0;
    }

out:

    if (did != 0) {
        delete (obj, vid, did, "folder-0");
    }

    remove_dir(obj, vid, dir_names[0], 1, did);
    return ret;
}

/*!
 * @brief Whether readdir() gives a name in a folder as a regular file
 */
static int readdir_gives_file(const char *path, const char *name)
{
    int file = 0;
#ifdef DT_REG
    const struct dirent *de;
    DIR *dp;

    if ((dp = opendir(path)) == NULL) {
        return 0;
    }

    while ((de = readdir(dp)) != NULL) {
        if (strcmp(de->d_name, name) == 0) {
            file = de->d_type == DT_REG;
            break;
        }
    }

    closedir(dp);
#else
    (void)path;
    (void)name;
#endif
    return file;
}

/*!
 * @brief A listing of files lists a name that readdir() gives as a file,
 *        whose cached entry is the folder's that held the name before;
 *        skipped where readdir() gives no kind
 */
int utest_enumerate_other_kind_entry_stats(AFPObj *obj, uint16_t vid)
{
    char folder[MAXPATHLEN + 1];
    char aside[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1] = "";
    char name[32];
    char moved[32];
    cnid_t did = 0;
    int ret = -1;
    snprintf(name, sizeof(name), ENUM_FILE, 3);
    snprintf(moved, sizeof(moved), "%s-aside", dir_names[0]);

    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0) {
        goto out;
    }

    snprintf(folder, sizeof(folder), "%s", cfrombstr(curdir->d_fullpath));
    snprintf(path, sizeof(path), "%s/%s", folder, name);
    snprintf(aside, sizeof(aside), "%s-aside", folder);

    /* a folder made through a request is cached, and a file takes its name
     * behind afpd once the folder moves out of the listing; the settled
     * folder keeps its listing for every batch */
    if (createdir(obj, vid, did, name) != AFP_OK || rename(path, aside) != 0
            || create_disk_file(path) != 0 || settle(folder) != 0) {
        goto out;
    }

    if (!readdir_gives_file(folder, name)) {
        ret = TEST_SKIP;
        goto out;
    }

    /* the four files take indexes 1 to 4 */
    if (enum_one(obj, vid, did, 1) == AFP_OK
            && enum_one(obj, vid, did, 2) == AFP_OK
            && enum_one(obj, vid, did, 3) == AFP_OK
            && enum_one(obj, vid, did, 4) == AFP_OK
            && enum_one(obj, vid, did, 5) == AFPERR_NOOBJ) {
        ret = 0;
    }

out:

    /* a folder entry left on the file's name refuses the delete */
    if (did != 0) {
        delete (obj, vid, did, name);
        unlink(path);
        delete (obj, vid, DIRDID_ROOT, moved);
    }

    remove_dir(obj, vid, dir_names[0], 3, did);
    return ret;
}

/*!
 * @brief The name at a position of a folder's listing, which afpd reads in
 *        readdir() order
 *
 * @param[in]  path  the folder
 * @param[in]  n     the position among its listing-entry names, from 0
 * @param[out] name  the name, empty when the folder holds fewer
 */
static void listed_name(const char *path, int n, char name[MAXPATHLEN + 1])
{
    struct dirent *de;
    DIR *dp;
    *name = '\0';

    if ((dp = opendir(path)) == NULL) {
        return;
    }

    while ((de = readdir(dp)) != NULL) {
        if (strncmp(de->d_name, "listing-entry-", 14) == 0 && n-- == 0) {
            snprintf(name, MAXPATHLEN + 1, "%s", de->d_name);
            break;
        }
    }

    closedir(dp);
}

/*!
 * @brief A name deleted after a batch returned it leaves the numbering: the
 *        next batch reads the folder again
 */
int utest_enumerate_deleted_name_rereads(AFPObj *obj, uint16_t vid)
{
    char first[MAXPATHLEN + 1];
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0) {
        goto out;
    }

    listed_name(cfrombstr(curdir->d_fullpath), 0, first);

    /* one file per batch, then the first deleted: index 3 is past the two
     * names left, and index 2 lists the last of them */
    if (first[0] != '\0'
            && enum_one(obj, vid, did, 1) == AFP_OK
            && enum_one(obj, vid, did, 2) == AFP_OK
            && enum_one(obj, vid, did, 3) == AFP_OK
            && delete (obj, vid, did, first) == AFP_OK
            && enum_one(obj, vid, did, 3) == AFPERR_NOOBJ
            && enum_one(obj, vid, did, 2) == AFP_OK) {
        ret = 0;
    }

out:
    remove_dir(obj, vid, dir_names[0], 3, did);
    return ret;
}

/*!
 * @brief A listing of the volume root, read at every request, lists each
 *        name once when a name readdir() gives as a file fails its stat at
 *        every call, as on a stale mount, and has the root's offspring
 *        counted again; skipped without the stat interposer or where
 *        readdir() gives no kind
 */
int utest_enumerate_root_failed_stat_once(AFPObj *obj, struct vol *vol)
{
    char lnames[1][MACFILELEN + 1];
    char unames[1][UTF8FILELEN_EARLY + 1];
    char seen[64][MACFILELEN + 1];
    char path[MAXPATHLEN + 1];
    char mute[MAXPATHLEN + 1];
    char name[32];
    struct enumerate_stats before;
    struct enumerate_stats after;
    struct stat st;
    uint32_t n = 0;
    int ours = 0;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    for (int i = 0; i < 3; i++) {
        snprintf(path, sizeof(path), "%s/" ENUM_FILE, vol->v_path, 900 + i);

        if (create_disk_file(path) != 0) {
            goto out;
        }
    }

    /* the first of the three in readdir() order fails, two names after it */
    listed_name(vol->v_path, 0, mute);
    fault_inject_reset();
    fi.stat_name = mute;
    fi.stat_errno = EIO;

    /* the probe makes the call afpd makes for a listed name */
    if (!readdir_gives_file(vol->v_path, mute)
            || ostat(mute, &st, vol_syml_opt(vol)) == 0 || errno != EIO
            || fi.stat_calls != 1) {
        ret = TEST_SKIP;
        goto out;
    }

    enumerate_stats_get(&before);

    /* one name per batch to the end of the listing */
    while (n < 64 && enumerate_names_from(obj, vol->v_vid, DIRDID_ROOT,
                                          n + 1, lnames, unames, 1) == 1) {
        for (uint32_t i = 0; i < n; i++) {
            if (strcmp(seen[i], lnames[0]) == 0) {
                goto out;
            }
        }

        ours += strncmp(lnames[0], "listing-entry-9", 15) == 0;
        snprintf(seen[n++], sizeof(seen[0]), "%s", lnames[0]);
    }

    /* past the probe and the batch that left the name out, each later
     * request stats it again on its way to its index; no read of the root
     * counts as a continuation reread, and the name each request left out
     * has the root's offspring counted again */
    enumerate_stats_get(&after);
    ret = n < 64 && ours == 2 && fi.stat_calls > 2
          && after.rereads == before.rereads && vol->v_root->d_ctime == 0
          ? 0 : -1;
out:
    fault_inject_reset();

    for (int i = 0; i < 3; i++) {
        snprintf(name, sizeof(name), ENUM_FILE, 900 + i);
        delete (obj, vol->v_vid, DIRDID_ROOT, name);
    }

    return ret;
}

/*!
 * @brief A folder's listing that each batch reads again, as in the granule
 *        of the folder's last change, lists each name once when a name
 *        readdir() gives as a file fails its stat at every call; skipped
 *        without the stat or clock interposer or where readdir() gives no
 *        kind
 */
int utest_enumerate_reread_failed_stat_once(AFPObj *obj, struct vol *vol)
{
    char lnames[1][MACFILELEN + 1];
    char unames[1][UTF8FILELEN_EARLY + 1];
    char seen[3][MACFILELEN + 1];
    char folder[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    char mute[MAXPATHLEN + 1];
    struct enumerate_stats before;
    struct enumerate_stats after;
    struct timespec now;
    struct stat st;
    cnid_t did = 0;
    uint32_t n = 0;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (make_dir(obj, vol->v_vid, dir_names[0], 0, &did) != 0) {
        goto out;
    }

    snprintf(folder, sizeof(folder), "%s/%s", vol->v_path, dir_names[0]);

    for (int i = 0; i < 3; i++) {
        snprintf(path, sizeof(path), "%s/" ENUM_FILE, folder, i);

        if (create_disk_file(path) != 0) {
            goto out;
        }
    }

    /* the first of the three in readdir() order fails, and the clock stays
     * in the second of the folder's ctime, so every batch reads it again */
    listed_name(folder, 0, mute);

    if (stat(folder, &st) != 0) {
        goto out;
    }

    fault_inject_reset();
    fi.stat_name = mute;
    fi.stat_errno = EIO;
    fi.clock_fake = 1;
    fi.clock_sec = st.st_ctime;

    /* the probes make the calls afpd makes */
    if (!readdir_gives_file(folder, mute)
            || ostat(mute, &st, vol_syml_opt(vol)) == 0 || errno != EIO
            || clock_gettime(CLOCK_REALTIME, &now) != 0
            || now.tv_sec != fi.clock_sec) {
        ret = TEST_SKIP;
        goto out;
    }

    enumerate_stats_get(&before);

    /* one name per batch to the end of the listing */
    while (n < 3 && enumerate_names_from(obj, vol->v_vid, did, n + 1, lnames,
                                         unames, 1) == 1) {
        for (uint32_t i = 0; i < n; i++) {
            if (strcmp(seen[i], lnames[0]) == 0) {
                goto out;
            }
        }

        snprintf(seen[n++], sizeof(seen[0]), "%s", lnames[0]);
    }

    /* the batch past the end read the folder too */
    enumerate_stats_get(&after);
    ret = n == 2 && after.reads - before.reads == 3 ? 0 : -1;
out:
    fault_inject_reset();
    remove_dir(obj, vol->v_vid, dir_names[0], 3, did);
    return ret;
}

/* the name at the volume root a dangling symlink in folder A points to */
#define ENUM_TARGET "listing-target"

/* folder A, its three files and a dangling symlink after them, and the
 * follow flag the volume had before dangling_setup() set it */
struct dangling {
    cnid_t did;
    int follow;
    char folder[MAXPATHLEN + 1];
    char link[MAXPATHLEN + 1];
};

/*!
 * @brief Folder A with three files and a symlink to ENUM_TARGET, which the
 *        volume root does not hold, on a volume that now follows symlinks,
 *        so the link's stat fails
 *
 * @returns 0, -1 when the setup fails
 */
static int dangling_setup(AFPObj *obj, struct vol *vol, struct dangling *dl)
{
    memset(dl, 0, sizeof(*dl));
    dl->follow = vol->v_flags & AFPVOL_FOLLOWSYM;

    if (make_dir(obj, vol->v_vid, dir_names[0], 3, &dl->did) != 0) {
        return -1;
    }

    snprintf(dl->folder, sizeof(dl->folder), "%s",
             cfrombstr(curdir->d_fullpath));
    snprintf(dl->link, sizeof(dl->link), "%s/" ENUM_FILE, dl->folder, 3);
    vol->v_flags |= AFPVOL_FOLLOWSYM;
    return symlink("../" ENUM_TARGET, dl->link);
}

/*!
 * @brief Remove what dangling_setup() made and give the volume its follow
 *        flag back
 */
static void dangling_teardown(AFPObj *obj, struct vol *vol,
                              const struct dangling *dl)
{
    vol->v_flags = (vol->v_flags & ~AFPVOL_FOLLOWSYM) | dl->follow;
    unlink(dl->link);
    remove_dir(obj, vol->v_vid, dir_names[0], 3, dl->did);
}

/*!
 * @brief A name whose stat fails, a dangling symlink on a volume that
 *        follows symlinks, leaves the numbering, for a batch asked again for
 *        an earlier index too, and has the folder's offspring counted again
 */
int utest_enumerate_failed_stat_leaves_index(AFPObj *obj, struct vol *vol)
{
    uint16_t vid = vol->v_vid;
    struct dangling dl;
    int ret = -1;

    /* the folder settled, its listing serves every batch: the three files
     * take indexes 1 to 3 wherever readdir puts the link, index 3 asked
     * again too */
    if (dangling_setup(obj, vol, &dl) == 0 && settle(dl.folder) == 0
            && enum_one(obj, vid, dl.did, 1) == AFP_OK
            && enum_one(obj, vid, dl.did, 2) == AFP_OK
            && enum_one(obj, vid, dl.did, 3) == AFP_OK
            && enum_one(obj, vid, dl.did, 3) == AFP_OK
            && enum_one(obj, vid, dl.did, 4) == AFPERR_NOOBJ
            && curdir->d_did == dl.did && curdir->d_ctime == 0) {
        ret = 0;
    }

    dangling_teardown(obj, vol, &dl);
    return ret;
}

/*!
 * @brief A new pass of a listing that left a name out reads the folder
 *        again: a symlink whose target appeared is listed, although the
 *        folder kept its stamp
 */
int utest_enumerate_new_pass_restats(AFPObj *obj, struct vol *vol)
{
    uint16_t vid = vol->v_vid;
    struct enumerate_stats before;
    struct enumerate_stats after;
    struct dangling dl;
    char target[MAXPATHLEN + 1];
    char name[32];
    int ret = -1;
    snprintf(target, sizeof(target), "%s/" ENUM_TARGET, vol->v_path);
    snprintf(name, sizeof(name), ENUM_FILE, 3);

    /* the first pass lists the three files and leaves the link out; its
     * target then appears outside the folder */
    if (dangling_setup(obj, vol, &dl) != 0 || settle(dl.folder) != 0
            || enum_one(obj, vid, dl.did, 1) != AFP_OK
            || enum_one(obj, vid, dl.did, 4) != AFPERR_NOOBJ
            || create_disk_file(target) != 0) {
        goto out;
    }

    enumerate_stats_get(&before);

    if (enum_one(obj, vid, dl.did, 1) == AFP_OK
            && enum_one(obj, vid, dl.did, 4) == AFP_OK) {
        enumerate_stats_get(&after);
        ret = after.reads - before.reads == 1 ? 0 : -1;
    }

out:
    /* the link goes through afpd, with the row its listing made, while its
     * target still makes it a file */
    delete (obj, vid, dl.did, name);
    unlink(target);
    dangling_teardown(obj, vol, &dl);
    return ret;
}

/*!
 * @brief A settled folder's listing serves every pass: a pass begun again,
 *        an index past the end asked twice, a batch that finds the end, and
 *        the passes after them read nothing
 */
int utest_enumerate_listing_serves_passes(AFPObj *obj, uint16_t vid)
{
    struct enumerate_stats before;
    struct enumerate_stats after;
    char folder[MAXPATHLEN + 1];
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0) {
        goto out;
    }

    snprintf(folder, sizeof(folder), "%s", cfrombstr(curdir->d_fullpath));

    if (settle(folder) != 0 || enum_one(obj, vid, did, 1) != AFP_OK) {
        goto out;
    }

    enumerate_stats_get(&before);

    if (enum_one(obj, vid, did, 2) == AFP_OK
            && enum_one(obj, vid, did, 1) == AFP_OK
            && enum_one(obj, vid, did, 5) == AFPERR_NOOBJ
            && enum_one(obj, vid, did, 5) == AFPERR_NOOBJ
            && enum_one(obj, vid, did, 1) == AFP_OK
            && enum_one(obj, vid, did, 3) == AFP_OK
            && enum_one(obj, vid, did, 4) == AFPERR_NOOBJ
            && enum_one(obj, vid, did, 1) == AFP_OK) {
        enumerate_stats_get(&after);
        ret = after.reads == before.reads ? 0 : -1;
    }

out:
    remove_dir(obj, vid, dir_names[0], 3, did);
    return ret;
}

/*!
 * @brief The entries a files-only listing of the volume root returns, one
 *        per batch
 */
static uint32_t root_files(AFPObj *obj, uint16_t vid)
{
    static const uint16_t files[2] = { 1 << FILPBIT_FNUM, 0 };
    uint32_t n = 0;

    while (enum_at(obj, vid, DIRDID_ROOT, files, n + 1) == AFP_OK) {
        n++;
    }

    return n;
}

/*!
 * @brief The virtual Icon ends a files-only listing of a volume root that
 *        holds a folder
 */
int utest_enumerate_icon_ends_files(AFPObj *obj, struct vol *vol)
{
    struct vi_lent lent;
    uint32_t plain;
    cnid_t did = 0;
    int ret = -1;

    if (vol->v_icon_rfork != NULL || real_icon_exists(vol)) {
        return TEST_SKIP;
    }

    if (make_dir(obj, vol->v_vid, dir_names[0], 0, &did) != 0) {
        goto out;
    }

    plain = root_files(obj, vol->v_vid);

    if (vi_lend(vol, &lent) == 0
            && root_files(obj, vol->v_vid) == plain + 1) {
        ret = 0;
    }

    vi_return(vol, &lent);
out:
    remove_dir(obj, vol->v_vid, dir_names[0], 0, did);
    return ret;
}

/*!
 * @brief The index of the virtual Icon in a listing of the volume root, one
 *        entry per batch
 *
 * @returns the index, 0 when the listing ends without the Icon
 */
static uint32_t icon_index(AFPObj *obj, uint16_t vid)
{
    char lnames[1][MACFILELEN + 1];
    char unames[1][UTF8FILELEN_EARLY + 1];
    uint32_t n = 1;

    while (enumerate_names_from(obj, vid, DIRDID_ROOT, n, lnames, unames, 1)
            == 1) {
        if (strcmp(lnames[0], VIRTUAL_ICON_NAME) == 0) {
            return n;
        }

        n++;
    }

    return 0;
}

/*!
 * @brief A real Icon file made at the volume root after its listing reached
 *        the virtual Icon replaces it: a listing of the root lists the real
 *        file, which a dircache entry then holds, and no second Icon
 */
int utest_enumerate_real_icon_replaces(AFPObj *obj, struct vol *vol)
{
    char lnames[1][MACFILELEN + 1];
    char unames[1][UTF8FILELEN_EARLY + 1];
    char path[MAXPATHLEN + 1];
    char icon[] = VIRTUAL_ICON_NAME;
    struct vi_lent lent;
    struct stat st;
    struct dir *entry;
    uint32_t icons = 0;
    uint32_t count = 0;
    uint32_t n;
    int ret = -1;

    if (vol->v_icon_rfork != NULL || real_icon_exists(vol)) {
        return TEST_SKIP;
    }

    if (vi_lend(vol, &lent) != 0) {
        vi_return(vol, &lent);
        return -1;
    }

    snprintf(path, sizeof(path), "%s/" VIRTUAL_ICON_NAME, vol->v_path);

    /* an entry an earlier test left under the name would stand in for the
     * real file's stat */
    if ((entry = dircache_search_by_name(vol, vol->v_root, icon,
                                         strlen(icon))) != NULL) {
        dir_remove(vol, entry, 0);
    }

    /* the root's n names, the virtual Icon last, then the real file in its
     * place among them */
    if ((n = icon_index(obj, vol->v_vid)) != 0
            && create_disk_file(path) == 0 && stat(path, &st) == 0) {
        while (enumerate_names_from(obj, vol->v_vid, DIRDID_ROOT, count + 1,
                                    lnames, unames, 1) == 1) {
            icons += strcmp(lnames[0], VIRTUAL_ICON_NAME) == 0;
            count++;
        }

        ret = count == n && icons == 1
              && (entry = dircache_search_by_name(vol, vol->v_root, icon,
                                                  strlen(icon))) != NULL
              && entry->dcache_ino == st.st_ino ? 0 : -1;
    }

    /* FPDelete takes the real file's entry and row with it */
    delete (obj, vol->v_vid, DIRDID_ROOT, icon);
    unlink(path);
    vi_return(vol, &lent);
    return ret;
}

/*!
 * @brief A file whose hex-encoded name decodes to a vetoed one leaves the
 *        listing that converts it
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes folder A
 * @returns 0, 1 when a batch listed the decoded name, -1 when the setup
 *          failed, TEST_SKIP on a volume without the conversion
 */
int utest_enumerate_dehex_vetoed_leaves(AFPObj *obj, struct vol *vol)
{
    /* "t:2ev" decodes to the vetoed "t.v" */
    static char names[3][6] = { "t:2ev", "other", "t.v" };
    static char veto[] = "t.v/";
    char lnames[3][MACFILELEN + 1];
    char unames[3][UTF8FILELEN_EARLY + 1];
    char path[MAXPATHLEN + 1];
    uint16_t vid = vol->v_vid;
    int noconv = vol->v_flags & AFPVOL_NOV2TOEACONV;
    char *saved_veto = vol->v_veto;
    const struct dir *folder = NULL;
    cnid_t did = 0;
    int fd;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || make_dir(obj, vid, dir_names[0], 0, &did) != 0) {
        ret = vol->v_adouble != AD_VERSION_EA ? TEST_SKIP : -1;
        goto out;
    }

    folder = curdir;

    for (int i = 0; i < 2; i++) {
        snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath),
                 names[i]);

        if ((fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600)) < 0) {
            goto out;
        }

        close(fd);
    }

    vol->v_veto = veto;
    vol->v_flags &= ~AFPVOL_NOV2TOEACONV;
    /* the converting batch lists the other file alone, and none follows */
    ret = enumerate_names(obj, vid, did, lnames, unames, 3) == 1
          && strcmp(unames[0], names[1]) == 0
          && enum_batch(obj, vid, did, 2) == AFPERR_NOOBJ ? 0 : 1;
out:
    vol->v_veto = saved_veto;
    vol->v_flags |= noconv;

    for (int i = 0; folder != NULL && i < 3; i++) {
        snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath),
                 names[i]);
        unlink(path);
    }

    remove_dir(obj, vid, dir_names[0], 0, did);
    return ret;
}

/*!
 * @brief A cached link whose decoded name fails its stat leaves the listing
 *        that renames it, and its CNID row keeps its old name
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes folder A
 * @returns 0, 1 when the link was listed or its row changed, -1 when the
 *          setup failed, TEST_SKIP on a volume without the conversion
 */
int utest_enumerate_dehex_gone_leaves(AFPObj *obj, struct vol *vol)
{
    /* "x:2ey" decodes to "x.y" */
    static char names[2][6] = { "x:2ey", "x.y" };
    char path[MAXPATHLEN + 1];
    uint16_t vid = vol->v_vid;
    int flags = vol->v_flags & (AFPVOL_NOV2TOEACONV | AFPVOL_FOLLOWSYM);
    const struct dir *folder = NULL;
    const struct dir *entry;
    cnid_t did = 0;
    cnid_t id;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || make_dir(obj, vid, dir_names[0], 0, &did) != 0) {
        ret = vol->v_adouble != AD_VERSION_EA ? TEST_SKIP : -1;
        goto out;
    }

    folder = curdir;
    snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath),
             names[0]);
    /* a dangling link, listed and cached as a file while the volume
     * neither converts nor follows links */
    vol->v_flags = (vol->v_flags & ~AFPVOL_FOLLOWSYM) | AFPVOL_NOV2TOEACONV;

    if (symlink("nowhere", path) != 0
            || enum_batch(obj, vid, did, 1) != AFP_OK
            || (entry = dircache_search_by_name(vol, folder, names[0], 5))
            == NULL) {
        goto out;
    }

    id = entry->d_did;
    vol->v_flags = (vol->v_flags & ~AFPVOL_NOV2TOEACONV) | AFPVOL_FOLLOWSYM;
    ret = enum_batch(obj, vid, did, 1) == AFPERR_NOOBJ
          && cnid_get(vol->v_cdb, did, names[0], 5) == id ? 0 : 1;
out:
    vol->v_flags = (vol->v_flags & ~(AFPVOL_NOV2TOEACONV | AFPVOL_FOLLOWSYM))
                   | flags;

    for (int i = 0; folder != NULL && i < 2; i++) {
        snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath),
                 names[i]);
        unlink(path);
    }

    remove_dir(obj, vid, dir_names[0], 0, did);
    return ret;
}

/*!
 * @brief movecwd() enters a walking volume's root, and a folder below it,
 *        without getcwd()
 */
int utest_movecwd_skips_getcwd(AFPObj *obj, struct vol *vol)
{
    char buf[MAXPATHLEN + 1];
    struct dir *sub;
    cnid_t did = 0;
    int ret = -1;

    /* a volume walks when afp_openvol() opened its root */
    if (vol->v_rootfd < 0 || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (make_dir(obj, vol->v_vid, dir_names[0], 0, &did) != 0) {
        goto out;
    }

    sub = curdir;
    fault_inject_reset();
    /* count every getcwd(), failing none */
    fi.getcwd.armed = 1;
    fi.getcwd.fail_after = -1;

    /* the interposer must see this file's own call, else the test skips */
    if (getcwd(buf, sizeof(buf)) == NULL || fi.getcwd.calls != 1) {
        ret = TEST_SKIP;
        goto out;
    }

    fi.getcwd.calls = 0;

    if (movecwd(vol, vol->v_root) == 0 && movecwd(vol, sub) == 0
            && fi.getcwd.calls == 0) {
        ret = 0;
    }

out:
    fault_inject_reset();
    remove_dir(obj, vol->v_vid, dir_names[0], 0, did);
    return ret;
}

/*!
 * @brief movecwd() into a folder below a walking volume's root succeeds
 *        with no descriptor free
 */
int utest_movecwd_at_descriptor_limit(AFPObj *obj, struct vol *vol)
{
#ifdef __APPLE__
    /* macOS's getcwd() takes a descriptor */
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    struct rlimit rl;
    struct rlimit low;
    struct dir *sub;
    cnid_t did = 0;
    int fd;
    int rc;
    int ret = -1;

    if (vol->v_rootfd < 0) {
        return TEST_SKIP;
    }

    if (make_dir(obj, vol->v_vid, dir_names[0], 0, &did) != 0
            || getrlimit(RLIMIT_NOFILE, &rl) != 0
            || (fd = open("/dev/null", O_RDONLY)) < 0) {
        goto out;
    }

    sub = curdir;
    /* fd is the lowest free descriptor: with the soft limit there, the walk
     * cannot open the folder and falls back to chdir() and getcwd() */
    close(fd);
    low = rl;
    low.rlim_cur = (rlim_t)fd;

    if (setrlimit(RLIMIT_NOFILE, &low) != 0) {
        goto out;
    }

    rc = movecwd(vol, vol->v_root) == 0 && movecwd(vol, sub) == 0;

    if (setrlimit(RLIMIT_NOFILE, &rl) == 0 && rc) {
        ret = 0;
    }

out:
    remove_dir(obj, vol->v_vid, dir_names[0], 0, did);
    return ret;
#endif
}

/*!
 * @brief On macOS, illumos and Solaris every volume that refuses symlinks
 *        walks, so afp_openvol() holds the test volume's root
 *
 * @returns 0, 1 without the root, or TEST_SKIP where a volume may not walk
 */
int utest_movecwd_volume_walks(const struct vol *vol)
{
    /* the walk needs O_SEARCH, which macOS SDKs before 13 lack */
#if (defined(__APPLE__) || defined(__sun)) && defined(O_SEARCH)
    return vol->v_rootfd >= 0 ? 0 : 1;
#else
    (void)vol;
    return TEST_SKIP;
#endif
}

/*!
 * @brief movecwd() refuses a folder below a volume root made unsearchable
 *        after the volume opened
 *
 * @returns 0, -1 when the change succeeded or the setup failed, TEST_SKIP
 *          for root where no other user can be taken on
 */
int utest_movecwd_rechecks_root(AFPObj *obj, struct vol *vol)
{
    struct stat st;
    struct dir *sub;
    uid_t euid = geteuid();
    cnid_t did = 0;
    int rc = 0;
    int ret = -1;

    if (make_dir(obj, vol->v_vid, dir_names[0], 0, &did) != 0
            || stat(vol->v_path, &st) != 0) {
        goto out;
    }

    sub = curdir;

    if (chmod(vol->v_path, 0) != 0) {
        goto out;
    }

    /* no permission bit refuses root, so root changes directory as uid 1 */
    if (euid == 0 && seteuid(1) != 0) {
        ret = TEST_SKIP;
    } else {
        errno = 0;
        rc = movecwd(vol, sub) == -1 && errno == EACCES;

        if (euid == 0 && seteuid(0) != 0) {
            rc = 0;
        }
    }

    chmod(vol->v_path, st.st_mode & 07777);

    if (rc) {
        ret = 0;
    }

out:
    remove_dir(obj, vol->v_vid, dir_names[0], 0, did);
    return ret;
}

/*!
 * @brief movecwd() enters a cached folder whose name on disk changed case,
 *        on a walking volume whose filesystem ignores case
 *
 * The walk opens the cached name; the getcwd() compare sees the name on
 * disk and refuses it.
 *
 * @returns 0, -1 when the change failed or the setup failed, TEST_SKIP where
 *          the volume does not walk or tells case apart
 */
int utest_movecwd_walk_ignores_case(AFPObj *obj, struct vol *vol)
{
    char path[MAXPATHLEN + 1];
    char upper[MAXPATHLEN + 1];
    struct stat st;
    struct dir *sub;
    cnid_t did = 0;
    int ret = -1;

    if (vol->v_rootfd < 0) {
        return TEST_SKIP;
    }

    if (make_dir(obj, vol->v_vid, "case-folder", 0, &did) != 0) {
        goto out;
    }

    sub = curdir;
    snprintf(path, sizeof(path), "%s/case-folder", vol->v_path);
    snprintf(upper, sizeof(upper), "%s/CASE-FOLDER", vol->v_path);

    if (rename(path, upper) != 0) {
        goto out;
    }

    /* where the filesystem tells case apart the cached name is gone */
    if (stat(path, &st) != 0) {
        ret = TEST_SKIP;
    } else if (movecwd(vol, sub) == 0) {
        ret = 0;
    }

    rename(upper, path);
out:
    remove_dir(obj, vol->v_vid, "case-folder", 0, did);
    return ret;
}

/*!
 * @brief movecwd() enters the volume root by its path once the held root's
 *        descriptor stops naming a folder
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes folder A
 * @returns 0, -1 when the change failed or the setup failed, TEST_SKIP where
 *          the volume does not walk
 */
int utest_movecwd_stale_root(AFPObj *obj, struct vol *vol)
{
    char path[MAXPATHLEN + 1];
    cnid_t did = 0;
    int held = -1;
    int fd;
    int ret = -1;

    if (vol->v_rootfd < 0) {
        return TEST_SKIP;
    }

    /* the folder's file leaves curdir in the folder, away from the root */
    if (make_dir(obj, vol->v_vid, dir_names[0], 1, &did) != 0
            || (held = dup(vol->v_rootfd)) < 0) {
        goto out;
    }

    snprintf(path, sizeof(path), "%s/%s/" ENUM_FILE, vol->v_path,
             dir_names[0], 0);

    /* the held root's descriptor names a file now, as a reclaimed vnode
     * stops naming a folder */
    if ((fd = open(path, O_RDONLY)) >= 0) {
        if (dup2(fd, vol->v_rootfd) >= 0 && movecwd(vol, vol->v_root) == 0) {
            ret = 0;
        }

        close(fd);
    }

    dup2(held, vol->v_rootfd);
out:

    if (held >= 0) {
        close(held);
    }

    remove_dir(obj, vol->v_vid, dir_names[0], 1, did);
    return ret;
}

static cnid_t (*backend_cnid_add)(struct _cnid_db *cdb, const struct stat *st,
                                  cnid_t did, const char *name, size_t len,
                                  cnid_t hint);
static int backend_adds;

/*!
 * @brief The backend's cnid_add, counted
 *
 * @param[in] cdb   CNID database
 * @param[in] st    stat of the object
 * @param[in] did   CNID of the parent directory
 * @param[in] name  name on disk
 * @param[in] len   length of name
 * @param[in] hint  CNID that the metadata of the object carries
 * @returns what the backend returns
 */
static cnid_t counted_cnid_add(struct _cnid_db *cdb, const struct stat *st,
                               cnid_t did, const char *name, size_t len,
                               cnid_t hint)
{
    backend_adds++;
    return backend_cnid_add(cdb, st, did, name, len, hint);
}

/*!
 * @brief Drop the dircache entries of folder A's files, keeping their CNIDs,
 *        then wait until time() reads two seconds past the precise clock,
 *        which a file's ctime can lead time() by: a folder table answers
 *        only for objects changed before the second preceding its load
 *
 * @param[in]  vol     the volume
 * @param[in]  folder  folder A
 * @param[out] ids     the files' CNIDs, by file number
 * @returns 0, or -1 when a file was not cached
 */
static int drop_files(const struct vol *vol, const struct dir *folder,
                      cnid_t ids[ENUM_FILES])
{
    char name[32];
    struct dir *entry;
    struct timespec tick = { .tv_nsec = 10 * 1000 * 1000 };
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    for (int i = 0; i < ENUM_FILES; i++) {
        snprintf(name, sizeof(name), ENUM_FILE, i);

        if ((entry = dircache_search_by_name(vol, folder, name,
                                             strlen(name))) == NULL) {
            return -1;
        }

        ids[i] = entry->d_did;
        dir_remove(vol, entry, 0);
    }

    dir_free_invalid_q();

    while (time(NULL) < now.tv_sec + 2) {
        nanosleep(&tick, NULL);
    }

    return 0;
}

/*!
 * @brief The CNID a listing cached for a file of folder A
 *
 * @param[in] vol     the volume
 * @param[in] folder  folder A
 * @param[in] name    the file's name
 * @returns the cached entry's CNID, or CNID_INVALID
 */
static cnid_t cached_id(const struct vol *vol, const struct dir *folder,
                        char *name)
{
    const struct dir *entry = dircache_search_by_name(vol, folder, name,
                                                      strlen(name));
    return entry ? entry->d_did : CNID_INVALID;
}

/*!
 * @brief The backend's cnid_add calls of folder A's batches from sindex on
 *
 * @param[in] obj     session object
 * @param[in] vol     the volume
 * @param[in] did     folder A
 * @param[in] sindex  the first batch's index: 1, 41 or 81
 * @returns the count, or -1 when a batch failed
 */
static int backend_adds_from(AFPObj *obj, struct vol *vol, cnid_t did,
                             uint32_t sindex)
{
    int listed = 1;
    backend_cnid_add = vol->v_cdb->cnid_add;
    vol->v_cdb->cnid_add = counted_cnid_add;
    backend_adds = 0;

    for (; listed && sindex <= 1 + 2 * ENUM_BATCH; sindex += ENUM_BATCH) {
        listed = enum_batch(obj, vol->v_vid, did, sindex) == AFP_OK;
    }

    vol->v_cdb->cnid_add = backend_cnid_add;
    return listed ? backend_adds : -1;
}

/*!
 * @brief A listing of names the dircache dropped asks the backend for
 *        CNID_DIRTAB_MISSES of them, the rest coming from one folder
 *        query, and each name keeps its CNID
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes folder A
 * @returns 0, 1 when the backend saw another count or a name changed CNID,
 *          -1 when the setup failed
 */
int utest_enumerate_cnid_one_query(AFPObj *obj, struct vol *vol)
{
    char name[32];
    cnid_t ids[ENUM_FILES];
    uint16_t vid = vol->v_vid;
    const struct dir *folder;
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], ENUM_FILES, &did) != 0) {
        goto out;
    }

    folder = curdir;

    if (drop_files(vol, folder, ids) != 0) {
        goto out;
    }

    ret = backend_adds_from(obj, vol, did, 1) == CNID_DIRTAB_MISSES ? 0 : 1;

    for (int i = 0; ret == 0 && i < ENUM_FILES; i++) {
        snprintf(name, sizeof(name), ENUM_FILE, i);

        if (cached_id(vol, folder, name) != ids[i]) {
            ret = 1;
        }
    }

out:
    remove_dir(obj, vid, dir_names[0], ENUM_FILES, did);
    return ret;
}

/*!
 * @brief A file whose row holds another inode takes the backend past its
 *        folder's loaded rows
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes folder A
 * @returns 0, 1 when the backend saw another count, -1 when the setup failed
 */
int utest_enumerate_cnid_other_inode(AFPObj *obj, struct vol *vol)
{
    char last[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    cnid_t ids[ENUM_FILES];
    uint16_t vid = vol->v_vid;
    const struct dir *folder;
    struct stat st;
    cnid_t did = 0;
    cnid_t id;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], ENUM_FILES, &did) != 0) {
        goto out;
    }

    folder = curdir;
    listed_name(cfrombstr(folder->d_fullpath), ENUM_FILES - 1, last);
    snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath), last);

    if (last[0] == '\0' || lstat(path, &st) != 0
            || (id = cnid_get(vol->v_cdb, did, last, strlen(last)))
            == CNID_INVALID) {
        goto out;
    }

    /* an inode no file has */
    st.st_ino = ~st.st_ino;

    if (cnid_update(vol->v_cdb, id, &st, did, last, strlen(last)) != 0
            || drop_files(vol, folder, ids) != 0
            || enum_batch(obj, vid, did, 1) != AFP_OK) {
        goto out;
    }

    ret = backend_adds_from(obj, vol, did, 1 + ENUM_BATCH) == 1 ? 0 : 1;
out:
    remove_dir(obj, vid, dir_names[0], ENUM_FILES, did);
    return ret;
}

/*!
 * @brief A file changed since its folder's rows loaded takes the backend
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes folder A
 * @returns 0, 1 when the backend saw another count, -1 when the setup failed
 */
int utest_enumerate_cnid_changed_file(AFPObj *obj, struct vol *vol)
{
    char last[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    cnid_t ids[ENUM_FILES];
    uint16_t vid = vol->v_vid;
    const struct dir *folder;
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], ENUM_FILES, &did) != 0) {
        goto out;
    }

    folder = curdir;
    listed_name(cfrombstr(folder->d_fullpath), ENUM_FILES - 1, last);
    snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath), last);

    if (last[0] == '\0' || drop_files(vol, folder, ids) != 0
            || enum_batch(obj, vid, did, 1) != AFP_OK
            || chmod(path, 0600) != 0) {
        goto out;
    }

    ret = backend_adds_from(obj, vol, did, 1 + ENUM_BATCH) == 1 ? 0 : 1;
out:
    remove_dir(obj, vid, dir_names[0], ENUM_FILES, did);
    return ret;
}

/*!
 * @brief A file whose metadata names another CNID than its row takes the
 *        backend past its folder's loaded rows
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes folder A
 * @returns 0, 1 when the backend saw another count, -1 when the setup failed,
 *          TEST_SKIP where the volume keeps no metadata
 */
int utest_enumerate_cnid_other_metadata(AFPObj *obj, struct vol *vol)
{
    char last[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    cnid_t ids[ENUM_FILES];
    uint16_t vid = vol->v_vid;
    const struct dir *folder;
    struct adouble ad;
    struct stat st;
    cnid_t did = 0;
    cnid_t id;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], ENUM_FILES, &did) != 0) {
        goto out;
    }

    folder = curdir;
    listed_name(cfrombstr(folder->d_fullpath), ENUM_FILES - 1, last);
    snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath), last);

    if (last[0] == '\0' || lstat(path, &st) != 0
            || (id = cnid_get(vol->v_cdb, did, last, strlen(last)))
            == CNID_INVALID) {
        goto out;
    }

    ad_init(&ad, vol);

    if (ad_open(&ad, path, ADFLAGS_HF | ADFLAGS_RDWR) != 0) {
        ret = TEST_SKIP;
        goto out;
    }

    ad_setid(&ad, st.st_dev, st.st_ino, htonl(ntohl(id) + 1), did,
             vol->v_stamp);
    ad_flush(&ad);
    ad_close(&ad, ADFLAGS_HF);

    if (drop_files(vol, folder, ids) != 0
            || enum_batch(obj, vid, did, 1) != AFP_OK) {
        goto out;
    }

    ret = backend_adds_from(obj, vol, did, 1 + ENUM_BATCH) == 1 ? 0 : 1;
out:
    remove_dir(obj, vid, dir_names[0], ENUM_FILES, did);
    return ret;
}

/*!
 * @brief A CNID write in the session sends a loaded folder's names to the
 *        backend
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes folder A
 * @returns 0, 1 when the backend saw another count, -1 when the setup failed
 */
int utest_enumerate_cnid_write_unloads(AFPObj *obj, struct vol *vol)
{
    char name[32];
    char path[MAXPATHLEN + 1];
    cnid_t ids[ENUM_FILES];
    uint16_t vid = vol->v_vid;
    const struct dir *folder;
    struct stat st;
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], ENUM_FILES, &did) != 0) {
        goto out;
    }

    folder = curdir;
    snprintf(name, sizeof(name), ENUM_FILE, 0);
    snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath), name);

    /* file 0's row written again as it stands */
    if (lstat(path, &st) != 0 || drop_files(vol, folder, ids) != 0
            || enum_batch(obj, vid, did, 1) != AFP_OK
            || cnid_update(vol->v_cdb, ids[0], &st, did, name,
                           strlen(name)) != 0) {
        goto out;
    }

    ret = backend_adds_from(obj, vol, did, 1 + ENUM_BATCH) == 2 * ENUM_BATCH
          ? 0 : 1;
out:
    remove_dir(obj, vid, dir_names[0], ENUM_FILES, did);
    return ret;
}

/*!
 * @brief A file whose row another process deleted after its folder's rows
 *        loaded gets a new CNID from a lookup outside the listing
 *
 * The backend's own delete stands in for the other process, leaving the
 * table loaded; the lookup is the one FPCreateID makes.
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes folder A
 * @returns 0, 1 when the lookup gave the deleted CNID, -1 when the setup
 *          failed
 */
int utest_enumerate_cnid_outside_listing(AFPObj *obj, struct vol *vol)
{
    char last[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    cnid_t ids[ENUM_FILES];
    uint16_t vid = vol->v_vid;
    const struct dir *folder;
    struct stat st;
    cnid_t did = 0;
    cnid_t id;
    cnid_t fresh;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], ENUM_FILES, &did) != 0) {
        goto out;
    }

    folder = curdir;
    listed_name(cfrombstr(folder->d_fullpath), ENUM_FILES - 1, last);
    snprintf(path, sizeof(path), "%s/%s", cfrombstr(folder->d_fullpath), last);

    if (last[0] == '\0' || lstat(path, &st) != 0
            || (id = cnid_get(vol->v_cdb, did, last, strlen(last)))
            == CNID_INVALID
            || drop_files(vol, folder, ids) != 0
            || enum_batch(obj, vid, did, 1) != AFP_OK
            || vol->v_cdb->cnid_delete(vol->v_cdb, id) != 0) {
        goto out;
    }

    fresh = cnid_add(vol->v_cdb, &st, did, last, strlen(last), CNID_INVALID);
    ret = fresh != CNID_INVALID && fresh != id ? 0 : 1;
out:
    remove_dir(obj, vid, dir_names[0], ENUM_FILES, did);
    return ret;
}

/*!
 * @brief A listing read again after its slot went takes its CNIDs from the
 *        backend name by name, not from a reload of the folder's rows
 *
 * @param[in] obj  session object
 * @param[in] vol  volume whose root takes folders A to I
 * @returns 0, 1 when the backend saw another count, -1 when the setup failed
 */
int utest_enumerate_cnid_reread_unarmed(AFPObj *obj, struct vol *vol)
{
    cnid_t did[ENUM_DIRS] = { 0 };
    cnid_t ids[ENUM_FILES];
    uint16_t vid = vol->v_vid;
    const struct dir *folder;
    int ret = -1;

    if (enum_setup(obj, vid, did) != 0
            || (folder = dirlookup(vol, did[0])) == NULL
            || drop_files(vol, folder, ids) != 0
            || enum_batch(obj, vid, did[0], 1) != AFP_OK) {
        goto out;
    }

    /* eight other listings take every slot, A's too */
    for (int i = 1; i < ENUM_DIRS; i++) {
        if (enum_batch(obj, vid, did[i], 1) != AFP_OK) {
            goto out;
        }
    }

    ret = backend_adds_from(obj, vol, did[0], 1 + ENUM_BATCH)
          == 2 * ENUM_BATCH ? 0 : 1;
out:
    enum_teardown(obj, vid, did);
    return ret;
}

/*!
 * @brief A batch after its folder changed reads the folder again and goes on
 *        at its index in the new read, which holds a name made since and not
 *        one removed since
 */
int utest_enumerate_changed_folder_rereads(AFPObj *obj, uint16_t vid)
{
    struct enumerate_stats before;
    struct enumerate_stats after;
    char lnames[3][MACFILELEN + 1];
    char unames[3][UTF8FILELEN_EARLY + 1];
    char want[2][MAXPATHLEN + 1];
    char folder[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0) {
        goto out;
    }

    snprintf(folder, sizeof(folder), "%s", cfrombstr(curdir->d_fullpath));

    /* a read past the folder's change window keeps its stamp */
    if (settle(folder) != 0
            || enumerate_names(obj, vid, did, lnames, unames, 3) != 3) {
        goto out;
    }

    /* the second goes and a fourth comes */
    snprintf(path, sizeof(path), "%s/%s", folder, unames[1]);

    if (unlink(path) != 0) {
        goto out;
    }

    snprintf(path, sizeof(path), "%s/" ENUM_FILE, folder, 3);

    if (create_disk_file(path) != 0) {
        goto out;
    }

    /* the new read's names at indexes 2 and 3 */
    listed_name(folder, 1, want[0]);
    listed_name(folder, 2, want[1]);
    enumerate_stats_get(&before);

    if (enumerate_names_from(obj, vid, did, 2, lnames, unames, 3) == 2
            && strcmp(unames[0], want[0]) == 0
            && strcmp(unames[1], want[1]) == 0) {
        enumerate_stats_get(&after);
        ret = after.reads - before.reads == 1
              && after.changed - before.changed == 1 ? 0 : -1;
    }

out:
    remove_dir(obj, vid, dir_names[0], 4, did);
    return ret;
}

/*!
 * @brief A folder's last change can hide another for two seconds, or for
 *        two ticks where the ctime keeps nanoseconds, and up to two seconds
 *        ahead of the clock; a date stored far ahead or long before is old
 */
int utest_enumerate_ctime_window(void)
{
    /* every value fits a 32-bit time_t */
    const struct timespec now = { .tv_sec = 1700000000, .tv_nsec = 500000000 };
    const struct timespec recent[] = {
        { .tv_sec = 1700000000, .tv_nsec = 490 * 1000 * 1000 },
        { .tv_sec = 1699999999, .tv_nsec = 0 },
        { .tv_sec = 1700000001, .tv_nsec = 0 },
        { .tv_sec = 1700000002, .tv_nsec = 500 * 1000 * 1000 }
    };
    const struct timespec old[] = {
        { .tv_sec = 1700000000, .tv_nsec = 470 * 1000 * 1000 },
        { .tv_sec = 1699999998, .tv_nsec = 0 },
        { .tv_sec = 1700000002, .tv_nsec = 510 * 1000 * 1000 },
        { .tv_sec = 1800000000, .tv_nsec = 0 },
        { .tv_sec = -1500000000, .tv_nsec = 0 }
    };
    /* the two ticks' edge, 19 ms in and 21 ms out, and across a second */
    const struct timespec past = { .tv_sec = 1700000001, .tv_nsec = 5000000 };
    const struct timespec edge_in[] = {
        { .tv_sec = 1700000000, .tv_nsec = 481 * 1000 * 1000 },
        { .tv_sec = 1700000000, .tv_nsec = 986 * 1000 * 1000 }
    };
    const struct timespec edge_out[] = {
        { .tv_sec = 1700000000, .tv_nsec = 479 * 1000 * 1000 },
        { .tv_sec = 1700000000, .tv_nsec = 984 * 1000 * 1000 }
    };

    for (size_t i = 0; i < sizeof(recent) / sizeof(recent[0]); i++) {
        if (!enumerate_ctime_recent(&recent[i], &now)) {
            return -1;
        }
    }

    for (size_t i = 0; i < sizeof(old) / sizeof(old[0]); i++) {
        if (enumerate_ctime_recent(&old[i], &now)) {
            return -1;
        }
    }

    if (!enumerate_ctime_recent(&edge_in[0], &now)
            || !enumerate_ctime_recent(&edge_in[1], &past)
            || enumerate_ctime_recent(&edge_out[0], &now)
            || enumerate_ctime_recent(&edge_out[1], &past)) {
        return -1;
    }

    return 0;
}

/*!
 * @brief The nanoseconds from one instant to a later one
 */
static long long ns_between(const struct timespec *from,
                            const struct timespec *to)
{
    return (long long)(to->tv_sec - from->tv_sec) * 1000000000LL
           + (to->tv_nsec - from->tv_nsec);
}

/*!
 * @brief A listing read within a tick of its folder's last change is read
 *        again by its next batch: a change in that tick can leave the ctime
 *        as it is; skipped where no try reads the folder within 10 ms of the
 *        change
 */
int utest_enumerate_recent_read_rechecked(AFPObj *obj, uint16_t vid)
{
    struct enumerate_stats before;
    struct enumerate_stats after;
    struct timespec made;
    struct timespec read;
    char path[MAXPATHLEN + 1];
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0) {
        goto out;
    }

    snprintf(path, sizeof(path), "%s/scratch", cfrombstr(curdir->d_fullpath));
    ret = TEST_SKIP;

    /* a name made and removed puts the folder's last change just before the
     * read; a try whose read comes later than that proves nothing */
    for (int i = 0; ret == TEST_SKIP && i < 10; i++) {
        clock_gettime(CLOCK_REALTIME, &made);

        if (create_disk_file(path) != 0 || unlink(path) != 0
                || enum_one(obj, vid, did, 1) != AFP_OK
                || clock_gettime(CLOCK_REALTIME, &read) != 0) {
            ret = -1;
            break;
        }

        enumerate_stats_get(&before);

        if (enum_one(obj, vid, did, 2) != AFP_OK) {
            ret = -1;
            break;
        }

        enumerate_stats_get(&after);

        if (ns_between(&made, &read) < 10000000LL) {
            ret = after.reads - before.reads == 1
                  && after.changed - before.changed == 1 ? 0 : -1;
        }
    }

out:
    remove_dir(obj, vid, dir_names[0], 3, did);
    return ret;
}

/*!
 * @brief A listing read three ticks after its folder's last change serves
 *        its next batch where the filesystem keeps the ctime's nanoseconds;
 *        skipped where it keeps whole seconds
 */
int utest_enumerate_ticked_read_serves(AFPObj *obj, uint16_t vid)
{
    const struct timespec tick = { .tv_sec = 0, .tv_nsec = 10 * 1000 * 1000 };
    struct enumerate_stats before;
    struct enumerate_stats after;
    struct timespec made;
    struct timespec now;
    char folder[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    struct stat st;
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0) {
        goto out;
    }

    snprintf(folder, sizeof(folder), "%s", cfrombstr(curdir->d_fullpath));
    snprintf(path, sizeof(path), "%s/scratch", folder);

    /* the clock read after the change bounds the change's ctime */
    if (create_disk_file(path) != 0 || unlink(path) != 0
            || stat(folder, &st) != 0
            || clock_gettime(CLOCK_REALTIME, &made) != 0) {
        goto out;
    }

    if (atalk_stat_ctime_timespec(&st).tv_nsec == 0) {
        ret = TEST_SKIP;
        goto out;
    }

    do {
        nanosleep(&tick, NULL);
        clock_gettime(CLOCK_REALTIME, &now);
    } while (ns_between(&made, &now) < 30000000LL);

    enumerate_stats_get(&before);

    if (enum_one(obj, vid, did, 1) == AFP_OK
            && enum_one(obj, vid, did, 2) == AFP_OK) {
        enumerate_stats_get(&after);
        ret = after.reads - before.reads == 1 ? 0 : -1;
    }

out:
    remove_dir(obj, vid, dir_names[0], 3, did);
    return ret;
}

/* the time, in seconds, that slow_resolve() moves the fake clock to, and
 * the backend's resolve that it wraps */
static time_t resolved_sec;
static char *(*backend_resolve)(struct _cnid_db *cdb, cnid_t *id,
                                void *buffer, size_t len);

/*!
 * @brief A cnid_resolve() that takes seconds: the fake clock moves on to
 *        resolved_sec before the backend answers
 */
static char *slow_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer,
                          size_t len)
{
    fi.clock_sec = resolved_sec;
    return backend_resolve(cdb, id, buffer, len);
}

/*!
 * @brief A request whose lookup of its folder takes seconds judges the
 *        listing's stamp by the clock at the read: a read in the second of
 *        the folder's last change has the next batch read the folder again;
 *        skipped without the clock interposer
 */
int utest_enumerate_window_at_read(AFPObj *obj, struct vol *vol)
{
    struct enumerate_stats before;
    struct enumerate_stats after;
    struct timespec now;
    struct dir *entry;
    struct stat st;
    cnid_t did = 0;
    int rc;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (make_dir(obj, vol->v_vid, dir_names[0], 3, &did) != 0
            || stat(cfrombstr(curdir->d_fullpath), &st) != 0
            || movecwd(vol, vol->v_root) != 0
            || (entry = dircache_lookup_did(vol, did)) == NULL) {
        goto out;
    }

    /* the request begins three seconds before the folder's last change, and
     * its lookup by the folder's DID, the entry gone, resolves until that
     * change's second */
    dir_remove(vol, entry, 0);
    fault_inject_reset();
    fi.clock_fake = 1;
    fi.clock_sec = st.st_ctime - 3;

    /* the probe reads the clock as afpd does */
    if (clock_gettime(CLOCK_REALTIME, &now) != 0
            || now.tv_sec != fi.clock_sec) {
        ret = TEST_SKIP;
        goto out;
    }

    resolved_sec = st.st_ctime;
    backend_resolve = vol->v_cdb->cnid_resolve;
    vol->v_cdb->cnid_resolve = slow_resolve;
    enumerate_stats_get(&before);
    rc = enum_one(obj, vol->v_vid, did, 1);
    vol->v_cdb->cnid_resolve = backend_resolve;

    if (rc == AFP_OK && enum_one(obj, vol->v_vid, did, 2) == AFP_OK) {
        enumerate_stats_get(&after);
        ret = after.reads - before.reads == 2 ? 0 : -1;
    }

out:
    fault_inject_reset();
    remove_dir(obj, vol->v_vid, dir_names[0], 3, did);
    return ret;
}
