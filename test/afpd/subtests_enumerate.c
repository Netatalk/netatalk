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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
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

/* 120 files list in three batches of 40; their names take 2280 bytes in a
 * saved listing, past the buffer's 2048-byte start */
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

    /* Linux and the BSDs cap a name at 255 bytes */
    if ((fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644)) < 0) {
        ret = errno == ENAMETOOLONG ? TEST_SKIP : -1;
        goto out;
    }

    close(fd);

    /* the folder lists its one other file, then ends */
    if (enum_batch(obj, vid, did, 1) == AFP_OK
            && enum_batch(obj, vid, did, 2) == AFPERR_NOOBJ) {
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
 *        a .AppleDouble, and a cached folder's once
 */
int utest_enumerate_conversion_attempts(AFPObj *obj, struct vol *vol)
{
    char adpath[MAXPATHLEN + 1];
    uint16_t vid = vol->v_vid;
    int noconv = vol->v_flags & AFPVOL_NOV2TOEACONV;
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

    if (make_dir(obj, vid, dir_names[0], ENUM_FILES, &did) != 0
            || createdir(obj, vid, did, "folder-0") != AFP_OK
            || createdir(obj, vid, did, "folder-1") != AFP_OK) {
        goto out;
    }

    vol->v_flags &= ~AFPVOL_NOV2TOEACONV;
    fault_inject_reset();
    fi.unlink_watch = 1;

    /* the interposer must see this file's own call, else the test skips */
    if (unlink(adpath) == 0 || fi.unlink_calls != 1) {
        ret = TEST_SKIP;
        goto out;
    }

    /* no .AppleDouble: each cached folder converts once, one unlink() of
     * its absent .Parent header, and no file is attempted */
    fi.unlink_calls = 0;

    if (enum_through(obj, vid, did) != 0 || fi.unlink_calls != 2) {
        goto out;
    }

    fi.unlink_calls = 0;

    if (enum_through(obj, vid, did) != 0 || fi.unlink_calls != 0) {
        goto out;
    }

    /* with a .AppleDouble every file is attempted, one unlink() of its
     * absent header apiece, and the .AppleDouble itself, which an
     * adouble:ea listing shows as a folder; the listing's end removes the
     * empty directory, so the next listing attempts no file */
    fi.unlink_calls = 0;

    if (mkdir(adpath, 0777) != 0 || enum_through(obj, vid, did) != 0
            || fi.unlink_calls != ENUM_FILES + 1) {
        goto out;
    }

    fi.unlink_calls = 0;

    if (enum_through(obj, vid, did) == 0 && fi.unlink_calls == 0) {
        ret = 0;
    }

out:
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
 *        CNID
 */
int utest_enumerate_dehex_keeps_cnid(AFPObj *obj, struct vol *vol)
{
    char hexed[MAXPATHLEN + 1];
    char plain[MAXPATHLEN + 1];
    uint16_t vid = vol->v_vid;
    int noconv = vol->v_flags & AFPVOL_NOV2TOEACONV;
    struct dir *folder;
    struct dir *entry;
    cnid_t did = 0;
    cnid_t id;
    int fd;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || make_dir(obj, vid, dir_names[0], 0, &did) != 0) {
        ret = vol->v_adouble != AD_VERSION_EA ? TEST_SKIP : -1;
        goto out;
    }

    /* x:2ey, a name netatalk 2 hex-encoded, which the conversion renames
     * x.y */
    folder = curdir;
    snprintf(hexed, sizeof(hexed), "%s/x:2ey", cfrombstr(folder->d_fullpath));
    snprintf(plain, sizeof(plain), "%s/x.y", cfrombstr(folder->d_fullpath));

    if ((fd = open(hexed, O_WRONLY | O_CREAT | O_EXCL, 0644)) < 0) {
        goto out;
    }

    close(fd);

    /* a listing with conversion off caches the file and gives it a CNID */
    if (enum_batch(obj, vid, did, 1) != AFP_OK
            || (entry = dircache_search_by_name(vol, folder, "x:2ey", 5))
            == NULL) {
        goto out;
    }

    id = entry->d_did;
    vol->v_flags &= ~AFPVOL_NOV2TOEACONV;

    if (enum_batch(obj, vid, did, 1) == AFP_OK
            && cnid_get(vol->v_cdb, did, "x.y", 3) == id) {
        ret = 0;
    }

out:
    vol->v_flags |= noconv;

    if (did != 0) {
        unlink(hexed);
        unlink(plain);
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
    setrlimit(RLIMIT_NOFILE, &rl);
    enumerate_stats_get(&after);

    /* the request reached the read, which failed */
    if (rc != AFPERR_NODIR || after.reads - before.reads != 1
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
 * @brief A name deleted after a batch returned it still holds its place
 *        in the listing's numbering
 */
int utest_enumerate_deleted_name_keeps_index(AFPObj *obj, uint16_t vid)
{
    char first[MAXPATHLEN + 1] = "";
    struct dirent *de;
    cnid_t did = 0;
    DIR *dp;
    int ret = -1;

    if (make_dir(obj, vid, dir_names[0], 3, &did) != 0
            || (dp = opendir(cfrombstr(curdir->d_fullpath))) == NULL) {
        goto out;
    }

    /* the listing's first name: afpd reads the folder in readdir() order */
    while ((de = readdir(dp)) != NULL) {
        if (strncmp(de->d_name, "listing-entry-", 14) == 0) {
            snprintf(first, sizeof(first), "%s", de->d_name);
            break;
        }
    }

    closedir(dp);

    /* one file per batch, then the first deleted and index 3 asked again:
     * the walk counts the deleted name, as the batch at index 1 did */
    if (*first != '\0'
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
    fi.getcwd_watch = 1;

    /* the interposer must see this file's own call, else the test skips */
    if (getcwd(buf, sizeof(buf)) == NULL || fi.getcwd_calls != 1) {
        ret = TEST_SKIP;
        goto out;
    }

    fi.getcwd_calls = 0;

    if (movecwd(vol, vol->v_root) == 0 && movecwd(vol, sub) == 0
            && fi.getcwd_calls == 0) {
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
    setrlimit(RLIMIT_NOFILE, &rl);

    if (rc) {
        ret = 0;
    }

out:
    remove_dir(obj, vol->v_vid, dir_names[0], 0, did);
    return ret;
#endif
}
