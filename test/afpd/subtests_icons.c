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
 * @brief Icon path tests: the resource fork open and close of a file a
 *        client reads, and the metadata a folder's new dircache entry keeps
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/ea.h>
#include <atalk/globals.h>
#include <atalk/volume.h>

#include "ad_cache.h"
#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "faultinject.h"
#include "fork.h"
#include "peer_lock.h"
#include "subtests_icons.h"
#include "test.h"
#include "test_capabilities.h"
#include "virtual_icon.h"

/* the resource fork length the fixture gives a file */
#define ICONS_RLEN 64

/*!
 * @brief Create name in the volume root with a resource fork of ICONS_RLEN
 *        bytes, closed again
 *
 * @returns 0, or -1 with the file deleted
 */
static int icons_make_file(AFPObj *obj, const struct vol *vol,
                           const char *name)
{
    uint16_t w = 0;

    if (createfile(obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK) {
        return -1;
    }

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &w) != AFP_OK
            || setforkparams(obj, w, ICONS_RLEN) != AFP_OK) {
        if (w) {
            closefork(obj, w);
        }

        delete (obj, vol->v_vid, DIRDID_ROOT, name);
        return -1;
    }

    closefork(obj, w);
    return 0;
}

/*!
 * @brief The close of a resource fork that nothing changed, the file's only
 *        open fork, re-reads the fork's own header and not the metadata: a
 *        fork opened read-only, then one opened for writing and left
 *        unwritten
 *
 * @returns 0, 1 when a close skipped the header or read the metadata, -1
 *          when the setup failed, TEST_SKIP without the counting preload or
 *          on a volume served as version 2, whose metadata file is the
 *          header
 */
int utest_rfork_unchanged_close_skips_metadata(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_rclose";
    static const uint16_t modes[] = { OPENACC_RD, OPENACC_RD | OPENACC_WR };
    uint16_t r = 0;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
        int preads;
        int reads;

        if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                     modes[i], &r) != AFP_OK) {
            ret = -1;
            goto out;
        }

        fault_inject_reset();
        fi.xattr_watch = 1;
        closefork(obj, r);
        r = 0;
        preads = fi.pread_calls;
        reads = fi.getxattr_calls;
        fault_inject_reset();

        if (preads != 1 || reads != 0) {
            fprintf(stderr, "rfork close: mode %d, pread %d, getxattr %d\n",
                    modes[i], preads, reads);
            ret = 1;
            goto out;
        }

        ret = 0;
    }

out:

    if (r) {
        closefork(obj, r);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

#ifndef HAVE_EAFD
/* the sidecar of either layout: a header, FinderInfo, a fork header */
#define ICONS_SIDECAR_LEN (ADEDOFF_RFORK_OSX + ADEDLEN_FINDERI \
                           + RFORK_HEADER_SIZE)

/*!
 * @brief Write the ._ file at path, ICONS_SIDECAR_LEN bytes long: a
 *        FinderInfo of 32 + wide bytes at 50, the macOS layout when wide is
 *        not 0, then a resource fork whose header records one byte of
 *        resources
 *
 * @returns 0, or -1 when wide is wider than FinderInfo or the write failed
 */
static int icons_write_sidecar(const char *path, uint32_t wide)
{
    const uint32_t entry[] = {
        ADEID_RFORK, ADEDOFF_RFORK_OSX + wide,
        ADEDLEN_FINDERI - wide + RFORK_HEADER_SIZE,
        ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI + wide,
    };
    uint8_t sidecar[ICONS_SIDECAR_LEN] = { 0 };
    const uint32_t one = htonl(1);

    if (wide > ADEDLEN_FINDERI) {
        return -1;
    }

    pack_appledouble(sidecar, entry, 2);
    memcpy(sidecar + ADEDOFF_RFORK_OSX + wide + RFORK_DATA_LEN_OFF, &one,
           sizeof(one));
    return write_disk_file(path, sidecar, sizeof(sidecar));
}
#endif /* HAVE_EAFD */

/*!
 * @brief The close of the root Icon file's read-only resource fork reads the
 *        fork through the header its re-read converts: the file stays when
 *        another process rewrote its ._ file in the macOS layout, at the same
 *        length, while the fork was open
 *
 * @returns 0, 1 when the close removed the file or its ._ file, -1 when the
 *          setup failed, TEST_SKIP where the sidecar is no file of its own or
 *          on a volume served as version 2
 */
int utest_icon_readonly_close_converts_first(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = VIRTUAL_ICON_NAME;
    /* a resource fork header with no resources, the volume's built-in icon */
    static unsigned char icon[RFORK_HEADER_SIZE];
    unsigned char *saved = vol->v_icon_rfork;
    const size_t saved_len = vol->v_icon_rfork_len;
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    uint16_t r = 0;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    if (createfile(obj, vol->v_vid, DIRDID_ROOT, name) == AFP_OK
            && icons_write_sidecar(adpath, 0) == 0
            && openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) == AFP_OK
            && icons_write_sidecar(adpath, ADEDLEN_FINDERI) == 0) {
        int file_kept;
        int sidecar_kept;
        /* a volume that serves a built-in icon runs the close's removal */
        vol->v_icon_rfork = icon;
        vol->v_icon_rfork_len = sizeof(icon);
        closefork(obj, r);
        r = 0;
        file_kept = access(path, F_OK) == 0;
        sidecar_kept = access(adpath, F_OK) == 0;
        ret = file_kept && sidecar_kept ? 0 : 1;

        if (ret != 0) {
            fprintf(stderr, "root Icon close: file kept %d, sidecar kept %d\n",
                    file_kept, sidecar_kept);
        }
    }

    if (r) {
        closefork(obj, r);
    }

    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    unlink(adpath);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief The close of a read-only resource fork whose ._ file another
 *        process grew under it reads the header afresh, so the emptied
 *        sidecar's unlink leaves the grown file alone
 *
 * @returns 0, 1 when the close removed the grown ._ file, -1 when the setup
 *          failed, TEST_SKIP where the sidecar is no file of its own or on a
 *          volume served as version 2
 */
int utest_rfork_readonly_close_sees_growth(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_rgrow";
    /* a header alone: an empty resource fork */
    const uint32_t entry[] = {
        ADEID_RFORK, ADEDOFF_RFORK_OSX, 0,
        ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI,
    };
    /* the header, then the sixteen bytes of fork another process adds */
    uint8_t grown[ADEDOFF_RFORK_OSX + RFORK_HEADER_SIZE] = { 0 };
    char adpath[MAXPATHLEN + 1];
    struct stat st;
    off_t size;
    uint16_t r = 0;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));
    pack_appledouble(grown, entry, 2);

    if (createfile(obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || write_disk_file(adpath, grown, ADEDOFF_RFORK_OSX) != 0
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) != AFP_OK
            || write_disk_file(adpath, grown, sizeof(grown)) != 0) {
        goto out;
    }

    closefork(obj, r);
    r = 0;
    size = stat(adpath, &st) == 0 ? st.st_size : -1;
    ret = size == (off_t)sizeof(grown) ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "rfork growth: sidecar size %lld\n", (long long)size);
    }

out:

    if (r) {
        closefork(obj, r);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief The close of a resource fork whose adouble the session's data fork
 *        shares reads the header afresh, so the data fork serves the Finder
 *        info another process set under the open
 *
 * @returns 0, 1 when the shared header kept the Finder info of the open, -1
 *          when the setup failed, TEST_SKIP on a volume whose metadata is no
 *          extended attribute
 */
int utest_rfork_close_refreshes_shared(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_rshared";
    const uint16_t flags = htons(FINDERINFO_HASCUSTOMICON);
    char path[MAXPATHLEN + 1];
    struct adouble other;
    const char *finfo;
    uint16_t served = 0;
    uint16_t d = 0;
    uint16_t r = 0;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    ad_init(&other, vol);

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    /* the data fork holds the adouble, the resource fork joins it, and an
     * adouble of its own, another process's, sets the custom icon bit */
    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD, &d) != AFP_OK
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) != AFP_OK
            || ad_open(&other, path, ADFLAGS_HF | ADFLAGS_RDWR, 0) != 0) {
        goto out;
    }

    memcpy((char *)ad_entry(&other, ADEID_FINDERI) + FINDERINFO_FRFLAGOFF,
           &flags, sizeof(flags));
    ad_flush(&other);
    ad_close(&other, ADFLAGS_HF);
    closefork(obj, r);
    r = 0;
    finfo = ad_entry(of_find(d)->of_ad, ADEID_FINDERI);

    if (finfo != NULL) {
        memcpy(&served, finfo + FINDERINFO_FRFLAGOFF, sizeof(served));
    }

    ret = finfo != NULL && served == flags ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "shared close: Finder flags %04x, expected %04x\n",
                ntohs(served), ntohs(flags));
    }

out:

    if (r) {
        closefork(obj, r);
    }

    if (d) {
        closefork(obj, d);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

/*!
 * @brief A read-only open of a resource fork measures its sidecar once, by
 *        the descriptor it opened, keeps that length and stats the
 *        sidecar's path never; beside a held data fork, a write open that
 *        creates the sidecar records an empty fork, and a read-only open
 *        that finds no sidecar measures the fork afresh
 *
 * @returns 0, 1 when an open failed, stat'ed the sidecar's path or kept
 *          another length, -1 when the setup failed, TEST_SKIP without the
 *          counting preload, where the sidecar is no file of its own or on a
 *          volume served as version 2
 */
int utest_rfork_open_measures_sidecar_once(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_ropen";
    char suffix[MAXPATHLEN + 1];
    char sidecar[MAXPATHLEN + 1];
    uint16_t d = 0;
    uint16_t r = 0;
    off_t kept = -1;
    off_t made = -1;
    off_t fresh = -1;
    int calls;
    int rc;
    int ret;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_STAT_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    /* the sidecar's name, as the volume spells it beside the file; the
     * preload counts the open's stat() and lstat() calls on it */
    snprintf(suffix, sizeof(suffix), "%s", vol->ad_path(name, 0));
    snprintf(sidecar, sizeof(sidecar), "%s/%s", vol->v_path, suffix);
    fault_inject_reset();
    fi.stat_suffix = suffix;
    rc = openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                  OPENACC_RD, &r);
    calls = fi.stat_suffix_calls;
    fault_inject_reset();

    if (r) {
        kept = of_find(r)->of_ad->ad_rlen;
        closefork(obj, r);
        r = 0;
    }

    /* the data fork's open measures the sidecar; another process removes
     * it; the resource fork's open finds none */
    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD, &d) == AFP_OK && unlink(sidecar) == 0
            && openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) == AFP_OK) {
        fresh = of_find(r)->of_ad->ad_rlen;
        closefork(obj, r);
        r = 0;
        closefork(obj, d);
        d = 0;
    }

    /* the fork filled again and held through the data fork, the sidecar
     * removed by another process, a write open creating it anew */
    if (fresh == 0
            && openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &r) == AFP_OK) {
        const int filled = setforkparams(obj, r, ICONS_RLEN);
        closefork(obj, r);
        r = 0;

        if (filled == AFP_OK
                && openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                            OPENACC_RD, &d) == AFP_OK
                && unlink(sidecar) == 0
                && openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                            OPENACC_RD | OPENACC_WR, &r) == AFP_OK) {
            made = of_find(r)->of_ad->ad_rlen;
        }
    }

    if (rc == AFP_OK && calls == 0 && kept == ICONS_RLEN && fresh == 0
            && made == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "rfork open: result %d, path stats %d, length %lld, "
                        "no sidecar %lld, created %lld\n", rc, calls,
                (long long)kept, (long long)fresh, (long long)made);
        ret = 1;
    }

    if (r) {
        closefork(obj, r);
    }

    if (d) {
        closefork(obj, d);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief A second open of a resource fork the session holds takes the fork's
 *        length from the descriptor it joins, not from the sidecar's path
 *
 * @returns 0, 1 when the join reported the path's length, -1 when the setup
 *          failed, TEST_SKIP where the sidecar is no file of its own or on a
 *          volume served as version 2
 */
int utest_rfork_join_measures_held_descriptor(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_rjoin";
    char adpath[MAXPATHLEN + 1];
    uint16_t r1 = 0;
    uint16_t r2 = 0;
    off_t rlen = -1;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    /* another process removes the sidecar under the open fork, whose
     * descriptor keeps the fork; the second open joins that descriptor */
    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r1) != AFP_OK
            || unlink(adpath) != 0
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r2) != AFP_OK) {
        goto out;
    }

    rlen = of_find(r2)->of_ad->ad_rlen;
    ret = rlen == ICONS_RLEN ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "rfork join: length %lld, expected %d\n",
                (long long)rlen, ICONS_RLEN);
    }

out:

    if (r2) {
        closefork(obj, r2);
    }

    if (r1) {
        closefork(obj, r1);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief Retire a folder's dircache entry and fetch it anew through one AFP
 *        call, the folder's own parameters or a listing of its parent, each
 *        asking the DID alone
 *
 * @param[in]  obj         the test session
 * @param[in]  vol         the volume
 * @param[in]  pdir        the parent folder's entry
 * @param[in]  pdid        the parent folder's DID
 * @param[in]  name        the folder's unix name
 * @param[in]  by_listing  fetch through a listing of the parent, else through
 *                         the folder's own parameters
 * @param[out] rlen        the new entry's resource fork state
 * @returns 0, or -1 when a step failed: the move to the root, the retired
 *          entry still found, the call, or no new entry
 */
static int icons_refetch(AFPObj *obj, struct vol *vol, const struct dir *pdir,
                         cnid_t pdid, char *name, int by_listing, off_t *rlen)
{
    const struct dir *entry;
    int rc;

    /* the working directory leaves the folder before the drop: dir_remove()
     * rebuilds the entry of the working directory */
    if (movecwd(vol, vol->v_root) != 0) {
        return -1;
    }

    drop_entry(vol, pdid, name);

    if (dircache_search_by_name(vol, pdir, name, strlen(name)) != NULL) {
        return -1;
    }

    rc = by_listing
         ? enumerate_range(obj, vol->v_vid, pdid, 0, 1 << DIRPBIT_DID, 1, 20)
         : getfiledirparms_bitmap(obj, vol->v_vid, pdid, name, 0,
                                  1 << DIRPBIT_DID);

    if (rc != AFP_OK
            || (entry = dircache_search_by_name(vol, pdir, name,
                                                strlen(name))) == NULL) {
        return -1;
    }

    *rlen = entry->dcache_rlen;
    return 0;
}

/*!
 * @brief A listing's new folder entries keep the metadata the id lookup
 *        read, so the folder parameters read nothing more: one metadata read
 *        for a folder with Finder info, whose entry then holds that info,
 *        one failed read for a folder without, whose entry then holds the
 *        mark; a path walk, a listing that asks no metadata and a folder
 *        whose metadata does not parse leave a new entry's metadata unknown
 *
 * @returns 0, 1 when a folder was read twice, the cache missed or an entry
 *          holds other state, -1 when the setup failed, TEST_SKIP without
 *          the counting preload or on a volume whose metadata is no extended
 *          attribute
 */
int utest_folder_entry_keeps_metadata_read(AFPObj *obj, struct vol *vol)
{
    static char parent[] = "icons_parent";
    static char flagged[] = "icons_flagged";
    static char bare[] = "icons_bare";
    static char broken[] = "icons_broken";
    char finfo[ADEDLEN_FINDERI] = { 0 };
    char path[MAXPATHLEN + 1];
    const struct dir *pdir;
    const struct dir *entry;
    unsigned long long misses;
    uint16_t flags = htons(FINDERINFO_HASCUSTOMICON);
    cnid_t pdid = 0;
    off_t walked_rlen = 0;
    off_t unread_rlen = 0;
    int reads;
    int served;
    int walked;
    int unread;
    int unparsed;
    int rc;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (createdir(obj, vol->v_vid, DIRDID_ROOT, parent) != AFP_OK
            || (pdir = dircache_search_by_name(vol, vol->v_root, parent,
                                               strlen(parent))) == NULL) {
        goto out;
    }

    pdid = pdir->d_did;
    snprintf(path, sizeof(path), "%s/%s/%s", vol->v_path, parent, bare);
    memcpy(finfo + FINDERINFO_FRFLAGOFF, &flags, sizeof(flags));

    /* the working directory leaves the flagged folder before its entry
     * goes: dir_remove() rebuilds the entry of the working directory */
    if (createdir(obj, vol->v_vid, pdid, flagged) != AFP_OK
            || setdirparms(obj, vol->v_vid, pdid, flagged, 1 << DIRPBIT_FINFO,
                           finfo, sizeof(finfo)) != AFP_OK
            || mkdir(path, S_IRWXU) != 0
            || movecwd(vol, vol->v_root) != 0) {
        goto out;
    }

    drop_entry(vol, pdid, flagged);

    if (dircache_search_by_name(vol, pdir, flagged, strlen(flagged)) != NULL) {
        goto out;
    }

    fault_inject_reset();
    fi.xattr_watch = 1;
    misses = ad_cache_misses;
    rc = enumerate_range(obj, vol->v_vid, pdid, 0,
                         (1 << DIRPBIT_DID) | (1 << DIRPBIT_FINFO), 1, 20);
    reads = fi.getxattr_calls;
    misses = ad_cache_misses - misses;
    fault_inject_reset();
    /* the entries hold what the listing read: the Finder info set on the
     * one folder, the mark of the folder without metadata */
    served = (entry = dircache_search_by_name(vol, pdir, flagged,
                                              strlen(flagged))) != NULL
             && memcmp(entry->dcache_finderinfo, finfo, sizeof(finfo)) == 0
             && (entry = dircache_search_by_name(vol, pdir, bare,
                                                 strlen(bare))) != NULL
             && entry->dcache_rlen == AD_RLEN_NO_AD;
    /* the folder's entry again, from a lookup that asks its DID alone, and
     * from a listing that asks its DID alone */
    walked = icons_refetch(obj, vol, pdir, pdid, bare, 0, &walked_rlen) == 0
             && walked_rlen == AD_RLEN_UNKNOWN;
    unread = icons_refetch(obj, vol, pdir, pdid, bare, 1, &unread_rlen) == 0
             && unread_rlen == AD_RLEN_UNKNOWN;
    /* a folder whose metadata does not parse is no folder without any */
    snprintf(path, sizeof(path), "%s/%s/%s", vol->v_path, parent, broken);
    unparsed = mkdir(path, S_IRWXU) == 0
               && sys_lsetxattr(path, AD_EA_META, "x", 1, 0) == 0
               && enumerate_range(obj, vol->v_vid, pdid, 0,
                                  (1 << DIRPBIT_DID) | (1 << DIRPBIT_FINFO), 1,
                                  20) == AFP_OK
               && (entry = dircache_search_by_name(vol, pdir, broken,
                                                   strlen(broken))) != NULL
               && entry->dcache_rlen == AD_RLEN_UNKNOWN;

    if (rc == AFP_OK && reads == 2 && misses == 0 && served && walked
            && unread && unparsed) {
        ret = 0;
    } else {
        fprintf(stderr,
                "folder entries: result %d, metadata reads %d, misses %llu, "
                "served %d, walked %d (state %lld), unread %d (state %lld), "
                "unparsed %d\n", rc, reads, misses, served, walked,
                (long long)walked_rlen, unread, (long long)unread_rlen,
                unparsed);
        ret = 1;
    }

out:
    delete (obj, vol->v_vid, pdid, broken);
    delete (obj, vol->v_vid, pdid, bare);
    delete (obj, vol->v_vid, pdid, flagged);
    delete (obj, vol->v_vid, DIRDID_ROOT, parent);
    return ret;
}

/*!
 * @brief A catalog search that meets a folder for the first time keeps the
 *        metadata its id lookup read for the reply's Finder info: one
 *        metadata read for the folder, whose entry then holds that info
 *
 * @returns 0, 1 when the search read the folder's metadata twice, missed it
 *          or left its entry without the info, -1 when the setup failed,
 *          TEST_SKIP without the counting preload or on a volume whose
 *          metadata is no extended attribute
 */
int utest_catsearch_entry_keeps_metadata_read(AFPObj *obj, struct vol *vol)
{
    static char parent[] = "icons_cparent";
    static char flagged[] = "icons_cflagged";
    /* the long name criterion: the Pascal string's offset past the record's
     * fixed part, then the name's length and its bytes */
    unsigned char spec[3 + sizeof(flagged) - 1] = { 0, 2, sizeof(flagged) - 1 };
    const uint16_t dbitmap = (1 << DIRPBIT_DID) | (1 << DIRPBIT_FINFO);
    char finfo[ADEDLEN_FINDERI] = { 0 };
    char lname[MACFILELEN + 1];
    const struct dir *pdir;
    const struct dir *entry;
    uint16_t flags = htons(FINDERINFO_HASCUSTOMICON);
    uint32_t nrecs = 1;
    cnid_t pdid = 0;
    int reads;
    int rc;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    memcpy(spec + 3, flagged, sizeof(flagged) - 1);
    memcpy(finfo + FINDERINFO_FRFLAGOFF, &flags, sizeof(flags));

    if (createdir(obj, vol->v_vid, DIRDID_ROOT, parent) != AFP_OK
            || (pdir = dircache_search_by_name(vol, vol->v_root, parent,
                                               strlen(parent))) == NULL) {
        goto out;
    }

    pdid = pdir->d_did;

    /* a first search warms every other entry's metadata, so the second
     * reads for the one folder whose entry went; the working directory
     * leaves the folder before its entry goes */
    if (createdir(obj, vol->v_vid, pdid, flagged) != AFP_OK
            || setdirparms(obj, vol->v_vid, pdid, flagged, 1 << DIRPBIT_FINFO,
                           finfo, sizeof(finfo)) != AFP_OK
            || catsearch_ext(obj, vol->v_vid, dbitmap, 1U << FILPBIT_LNAME,
                             spec, spec, sizeof(spec), &nrecs,
                             lname) != AFPERR_EOF
            || nrecs != 1 || movecwd(vol, vol->v_root) != 0) {
        goto out;
    }

    drop_entry(vol, pdid, flagged);

    if (dircache_search_by_name(vol, pdir, flagged, strlen(flagged)) != NULL) {
        goto out;
    }

    nrecs = 1;
    fault_inject_reset();
    fi.xattr_watch = 1;
    rc = catsearch_ext(obj, vol->v_vid, dbitmap, 1U << FILPBIT_LNAME, spec,
                       spec, sizeof(spec), &nrecs, lname);
    reads = fi.getxattr_calls;
    fault_inject_reset();
    entry = dircache_search_by_name(vol, pdir, flagged, strlen(flagged));

    if (rc == AFPERR_EOF && nrecs == 1 && reads == 1 && entry != NULL
            && memcmp(entry->dcache_finderinfo, finfo, sizeof(finfo)) == 0) {
        ret = 0;
    } else {
        fprintf(stderr,
                "catsearch: result %d, records %u, metadata reads %d, "
                "entry %d\n", rc, nrecs, reads, entry != NULL);
        ret = 1;
    }

out:
    delete (obj, vol->v_vid, pdid, flagged);
    delete (obj, vol->v_vid, DIRDID_ROOT, parent);
    return ret;
}

/*!
 * @brief An open of a resource fork without a deny mode probes the deny
 *        band of its access alone; a deny request probes the open band it
 *        consults too
 *
 * @returns 0, 1 when an open probed another number of bands, -1 when the
 *          setup failed, TEST_SKIP without the counting preload
 */
int utest_rfork_open_probes_bands_for_deny_only(AFPObj *obj,
                                                struct vol *vol)
{
    static char name[] = "icons_rprobe";
    static const struct {
        uint16_t access;
        int probes;
    } want[] = {
        { OPENACC_RD, 1 },
        { OPENACC_RD | OPENACC_DRD, 2 },
        { OPENACC_WR, 1 },
        { OPENACC_WR | OPENACC_DWR, 2 },
    };
    uint16_t r = 0;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    for (size_t i = 0; i < sizeof(want) / sizeof(want[0]); i++) {
        int probes;
        int rc;
        fault_inject_reset();
        fi.fcntl_watch = 1;
        rc = openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                      want[i].access, &r);
        probes = fi.getlk_calls;
        fault_inject_reset();

        if (rc != AFP_OK) {
            ret = -1;
            goto out;
        }

        closefork(obj, r);
        r = 0;

        if (probes != want[i].probes) {
            fprintf(stderr, "rfork open: access %#x, F_GETLK %d, expected %d\n",
                    want[i].access, probes, want[i].probes);
            ret = 1;
            goto out;
        }

        ret = 0;
    }

out:

    if (r) {
        closefork(obj, r);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

/*!
 * @brief A read open of a file whose mode bits grant the user read reads no
 *        ACL under an ACL mapping
 *
 * @returns 0, 1 when the open read the ACL, -1 when the setup failed,
 *          TEST_SKIP without POSIX ACL support, without the counting preload
 *          or on a volume served as version 2
 */
int utest_rfork_open_skips_acl_read_mode_grants(AFPObj *obj, struct vol *vol)
{
#if !defined(HAVE_ACLS) || !defined(HAVE_POSIX_ACLS)
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_racl";
    const uint32_t saved = obj->options.flags;
    uint16_t r = 0;
    int reads;
    int rc;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    /* the mapping that reads the ACL at an open */
    obj->options.flags |= OPTION_ACL2MACCESS;
    fault_inject_reset();
    fi.xattr_watch = 1;
    rc = openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                  OPENACC_RD, &r);
    reads = fi.getxattr_calls;
    fault_inject_reset();
    obj->options.flags = saved;

    if (rc != AFP_OK) {
        goto out;
    }

    /* the metadata EA's read alone */
    ret = reads == 1 ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "rfork open: getxattr %d, expected 1\n", reads);
    }

out:

    if (r) {
        closefork(obj, r);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
#endif /* HAVE_ACLS && HAVE_POSIX_ACLS */
}

/*!
 * @brief A data fork that shares its adouble with a resource fork keeps the
 *        resource fork's length after that fork closes
 *
 * @returns 0, 1 when the length went with the close, -1 when the setup
 *          failed, TEST_SKIP on a volume served as version 2
 */
int utest_shared_rfork_length_survives_rsrc_close(AFPObj *obj,
                                                  struct vol *vol)
{
    static char name[] = "icons_rkeep";
    uint16_t d = 0;
    uint16_t r = 0;
    off_t rlen = -1;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD, &d) != AFP_OK
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) != AFP_OK) {
        goto out;
    }

    closefork(obj, r);
    r = 0;
    rlen = of_find(d)->of_ad->ad_rlen;
    ret = rlen == ICONS_RLEN ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "shared length: %lld after the resource fork's close\n",
                (long long)rlen);
    }

out:

    if (r) {
        closefork(obj, r);
    }

    if (d) {
        closefork(obj, d);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

/*!
 * @brief A data fork that holds an adouble keeps the resource fork's length
 *        when a resource fork open on the same adouble fails
 *
 * @returns 0, 1 when the failed open zeroed the length, -1 when the setup
 *          failed, TEST_SKIP where the sidecar is no file of its own,
 *          without the fault injector or on a volume served as version 2
 */
int utest_shared_rfork_length_survives_failed_open(AFPObj *obj,
                                                   struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_rfail";
    uint16_t d = 0;
    uint16_t r = 0;
    off_t rlen = -1;
    int fired;
    int rc;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD, &d) != AFP_OK) {
        goto out;
    }

    /* the next open() is the sidecar's, and fails with an error the open
     * does not retry */
    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_after = 0;
    fi.open_errno = EIO;
    rc = openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                  OPENACC_RD, &r);
    fired = fi.open_fail_after == -1;
    fault_inject_reset();

    if (!fired || rc == AFP_OK) {
        goto out;
    }

    rlen = of_find(d)->of_ad->ad_rlen;
    ret = rlen == ICONS_RLEN ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr,
                "shared length: %lld after a failed resource fork open\n",
                (long long)rlen);
    }

out:

    if (r) {
        closefork(obj, r);
    }

    if (d) {
        closefork(obj, d);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief The close of a resource fork a length set emptied removes the
 *        emptied ._ file although the file's metadata EA does not parse
 *
 * @returns 0, 1 when the ._ file stayed, -1 when the setup failed, TEST_SKIP
 *          where the sidecar is no file of its own, where no EA holds the
 *          metadata or on a volume served as version 2
 */
int utest_emptied_sidecar_goes_under_bad_metadata(AFPObj *obj,
                                                  struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_rbadmeta";
    char path[MAXPATHLEN + 1];
    char adpath[MAXPATHLEN + 1];
    uint16_t r = 0;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD | OPENACC_WR, &r) != AFP_OK
            || setforkparams(obj, r, 0) != AFP_OK) {
        goto out;
    }

    /* the metadata EA goes bad under the open fork; the close's re-read of
     * it fails, and the emptied ._ file goes all the same */
    if (sys_lsetxattr(path, AD_EA_META, "x", 1, 0) != 0) {
        ret = TEST_SKIP;
        goto out;
    }

    closefork(obj, r);
    r = 0;
    ret = access(adpath, F_OK) != 0 ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "emptied close under bad metadata: ._ file kept\n");
    }

out:

    if (r) {
        closefork(obj, r);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    unlink(adpath);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief A data fork that holds an adouble keeps the resource fork's length
 *        when a resource fork open finds a sidecar whose header does not
 *        parse
 *
 * @returns 0, 1 when the failed open took the sidecar's size for the length,
 *          -1 when the setup failed, TEST_SKIP where the sidecar is no file
 *          of its own or on a volume served as version 2
 */
int utest_shared_rfork_length_survives_bad_sidecar(AFPObj *obj,
                                                   struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_rgarbage";
    static const char garbage[200] = { 'x' };
    char adpath[MAXPATHLEN + 1];
    uint16_t d = 0;
    uint16_t r = 0;
    off_t rlen = -1;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    snprintf(adpath, sizeof(adpath), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    /* another process replaces the sidecar with one no header read parses;
     * the open of it fails */
    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD, &d) != AFP_OK
            || write_disk_file(adpath, garbage, sizeof(garbage)) != 0
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) == AFP_OK) {
        goto out;
    }

    rlen = of_find(d)->of_ad->ad_rlen;
    ret = rlen == ICONS_RLEN ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr,
                "shared length: %lld after an open of a sidecar whose header "
                "does not parse\n", (long long)rlen);
    }

out:

    if (r) {
        closefork(obj, r);
    }

    if (d) {
        closefork(obj, d);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    unlink(adpath);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief The close of a read-only resource fork on a read-only volume reads
 *        neither the header nor the metadata: nothing at that close consumes
 *        them
 *
 * @returns 0, 1 when the close read either, -1 when the setup failed,
 *          TEST_SKIP without the counting preload or on a volume served as
 *          version 2
 */
int utest_readonly_volume_close_reads_nothing(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_rrovol";
    const int saved = vol->v_flags;
    uint16_t r = 0;
    int preads = -1;
    int reads = -1;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK) {
        goto out;
    }

    /* the volume turns read-only under the open fork: no unlink or removal
     * can follow the close */
    vol->v_flags |= AFPVOL_RO;
    fault_inject_reset();
    fi.xattr_watch = 1;
    closefork(obj, r);
    r = 0;
    preads = fi.pread_calls;
    reads = fi.getxattr_calls;
    fault_inject_reset();
    vol->v_flags = saved;
    ret = preads == 0 && reads == 0 ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "read-only volume close: pread %d, getxattr %d\n",
                preads, reads);
    }

out:

    if (r) {
        closefork(obj, r);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

/*!
 * @brief The attribute word FPGetFileDirParms reports for name in the
 *        volume root, from the last reply
 *
 * @returns the word in host order, or -1 when the call failed
 */
static int icons_file_attr(AFPObj *obj, const struct vol *vol,
                           const char *name)
{
    const char *params;
    size_t len;
    uint16_t attr;

    if (getfiledirparms_bitmap(obj, vol->v_vid, DIRDID_ROOT, name,
                               1 << FILPBIT_ATTR, 0) != AFP_OK) {
        return -1;
    }

    params = reply_params(REPLY_FILEDIRPARMS, &len);

    if (len < sizeof(attr)) {
        return -1;
    }

    memcpy(&attr, params, sizeof(attr));
    return ntohs(attr);
}

/*!
 * @brief A fork closed while another fork of the file stays open drops its
 *        open-fork attribute bit at once: a resource fork opened, a data
 *        fork opened beside it and the resource fork closed, the file
 *        reports the data fork open alone
 *
 * @returns 0, 1 when the file reports other bits, -1 when the setup failed
 */
int utest_closed_fork_drops_its_open_bit(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_odrop";
    const int bits = ATTRBIT_DOPEN | ATTRBIT_ROPEN;
    uint16_t r = 0;
    uint16_t d = 0;
    int attr = -1;
    int ret = -1;

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                        OPENACC_RD, &d) != AFP_OK) {
        goto out;
    }

    closefork(obj, r);
    r = 0;
    attr = icons_file_attr(obj, vol, name);
    ret = attr >= 0 && (attr & bits) == ATTRBIT_DOPEN ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "closed fork: attributes %#x, expected %#x\n", attr,
                ATTRBIT_DOPEN);
    }

out:

    if (r) {
        closefork(obj, r);
    }

    if (d) {
        closefork(obj, d);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

/*!
 * @brief A file with its resource fork open alone reports that fork's
 *        open-fork bit, and with a data fork open beside it both
 *
 * @returns 0, 1 when the file reports other bits, -1 when the setup failed
 */
int utest_open_forks_report_both(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_oboth";
    const int bits = ATTRBIT_DOPEN | ATTRBIT_ROPEN;
    uint16_t r = 0;
    uint16_t d = 0;
    int alone = -1;
    int paired = -1;
    int ret = -1;

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK) {
        goto out;
    }

    alone = icons_file_attr(obj, vol, name);

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                 OPENACC_RD, &d) != AFP_OK) {
        goto out;
    }

    paired = icons_file_attr(obj, vol, name);
    ret = alone >= 0 && (alone & ATTRBIT_ROPEN) && paired >= 0
          && (paired & bits) == bits ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "open forks: attributes %#x alone, %#x paired, "
                "expected %#x and %#x\n", alone, paired, ATTRBIT_ROPEN, bits);
    }

out:

    if (r) {
        closefork(obj, r);
    }

    if (d) {
        closefork(obj, d);
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

/*!
 * @brief A data fork another process holds open shows in the attributes
 *        FPGetFileDirParms reads from disk: the probe of the open bands
 *        finds the peer's lock
 *
 * @returns 0, 1 when the data fork's bit is missing, -1 when the setup
 *          failed
 */
int utest_peer_data_fork_shows_in_attributes(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_opeer";
    char path[MAXPATHLEN + 1];
    struct peer peer;
    int attr = -1;
    int ret = -1;

    if (icons_make_file(obj, vol, name) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    if (peer_hold_lock(&peer, path, F_RDLCK, AD_FILELOCK_OPEN_RD, 1) != 0) {
        goto out;
    }

    /* without its entry the file's parameters read the metadata from disk
     * and probe the bands */
    drop_entry(vol, DIRDID_ROOT, name);
    attr = icons_file_attr(obj, vol, name);
    peer_release(&peer);
    ret = attr >= 0 && (attr & ATTRBIT_DOPEN) ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "peer's data fork: attributes %#x\n", attr);
    }

out:
    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}
