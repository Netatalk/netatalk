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
#include <atalk/directory.h>
#include <atalk/globals.h>
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

    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0
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
     * no descriptor. */
    close(fd);
    low = rl;
    low.rlim_cur = (rlim_t)fd;
    vol->v_flags |= AFPVOL_FOLLOWSYM;
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
 * @brief Eight listings stay saved: two in interleaved batches read their
 *        directories once each, and six more leave the first one saved
 */
int utest_enumerate_interleaved_reads_once(AFPObj *obj, uint16_t vid)
{
    struct enumerate_stats before;
    struct enumerate_stats mid;
    struct enumerate_stats after;
    cnid_t did[ENUM_DIRS] = { 0 };
    int ret = -1;

    if (enum_setup(obj, vid, did) != 0) {
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
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 2, &did) != 0
            || createdir(obj, vid, did, "folder-0") != AFP_OK
            || createdir(obj, vid, did, "folder-1") != AFP_OK) {
        goto out;
    }

    /* two entries of each kind: the batch at index 3 ends the listing and
     * drops it, so index 2 asked again reads the folder anew and walks over
     * its first name, a file or a folder */
    for (int i = 0; i < 2; i++) {
        const uint16_t *k = kinds[i];

        if (enum_at(obj, vid, did, k, 1) != AFP_OK
                || enum_at(obj, vid, did, k, 2) != AFP_OK
                || enum_at(obj, vid, did, k, 3) != AFPERR_NOOBJ
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
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 1, &did) != 0
            || createdir(obj, vid, did, "folder-0") != AFP_OK) {
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
 * @brief A name deleted after a batch returned it still holds its place
 *        in the listing's numbering
 */
int utest_enumerate_deleted_name_keeps_index(AFPObj *obj, uint16_t vid)
{
    char first[MAXPATHLEN + 1];
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0) {
        goto out;
    }

    listed_name(cfrombstr(curdir->d_fullpath), 0, first);

    /* one file per batch, then the first deleted and index 3 asked again:
     * the walk counts the deleted name, as the batch at index 1 did */
    if (first[0] != '\0'
            && enum_one(obj, vid, did, 1) == AFP_OK
            && enum_one(obj, vid, did, 2) == AFP_OK
            && enum_one(obj, vid, did, 3) == AFP_OK
            && delete (obj, vid, did, first) == AFP_OK
            && enum_one(obj, vid, did, 3) == AFP_OK) {
        ret = 0;
    }

out:
    remove_dir(obj, vid, dir_names[0], 3, did);
    return ret;
}

/*!
 * @brief A name deleted before a batch reached it leaves the numbering, for
 *        a batch asked again for an earlier index too, and the folder's
 *        offspring count keeps the three files left
 */
int utest_enumerate_failed_stat_leaves_index(AFPObj *obj, uint16_t vid)
{
    char second[MAXPATHLEN + 1];
    struct dir *folder;
    cnid_t did = 0;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 4, &did) != 0) {
        goto out;
    }

    folder = curdir;
    listed_name(cfrombstr(folder->d_fullpath), 1, second);

    /* FPDelete takes the name off the count, which FPGetFileDirParms then
     * serves, or recounts where the deletion moved the folder's ctime */
    if (second[0] == '\0' || enum_one(obj, vid, did, 1) != AFP_OK
            || delete (obj, vid, did, second) != AFP_OK
            || getfiledirparms_bitmap(obj, vid, DIRDID_ROOT, dir_names[0], 0,
                                      1 << DIRPBIT_OFFCNT) != AFP_OK) {
        goto out;
    }

    /* the batch at index 2, whose stat of the deleted name fails, and the
     * walk of index 3 asked again skip the name, and the count stays 3 */
    if (enum_one(obj, vid, did, 2) == AFP_OK
            && enum_one(obj, vid, did, 3) == AFP_OK
            && enum_one(obj, vid, did, 3) == AFP_OK
            && enum_one(obj, vid, did, 4) == AFPERR_NOOBJ
            && getfiledirparms_bitmap(obj, vid, DIRDID_ROOT, dir_names[0], 0,
                                      1 << DIRPBIT_OFFCNT) == AFP_OK
            && folder->d_offcnt == 3) {
        ret = 0;
    }

out:
    remove_dir(obj, vid, dir_names[0], 4, did);
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
    /* the converting batch lists the other file alone, so none follows */
    ret = enum_batch(obj, vid, did, 1) == AFP_OK
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
