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
#include <limits.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/dsi.h>
#include <atalk/ea.h>
#include <atalk/globals.h>
#include <atalk/util.h>
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

/* the longest resource fork the cache tests write, and the length a rewrite
 * grows a fork to */
#define ICONS_MAXLEN 256
#define ICONS_LONGER 80
#define ICONS_QUANTUM 4096

/*! A DSI session on one end of a socketpair, served by a copy of the
 *  harness's session object */
struct icons_dsi {
    AFPObj obj;
    DSI    dsi;
    int    peer;
};

/*!
 * @brief Byte i of the resource fork a seed names
 *
 * @param[in] seed  the fork's content
 * @param[in] i     the byte's offset
 * @returns the byte
 */
static char icons_byte(unsigned int seed, size_t i)
{
    return (char)(seed * 31 + i * 7 + 1);
}

/*!
 * @brief Whether bytes are those of a seed's fork from an offset
 *
 * @param[in] buf     the bytes
 * @param[in] seed    the fork's content
 * @param[in] offset  the first byte's offset in the fork
 * @param[in] len     their count
 * @returns true when they are
 */
static bool icons_is(const char *buf, unsigned int seed, size_t offset,
                     size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (buf[i] != icons_byte(seed, offset + i)) {
            return false;
        }
    }

    return true;
}

/*!
 * @brief obj->exit of the cache tests' session: the session stays
 *
 * @param[in] code  unused
 */
static void icons_exit(int code _U_)
{
    /* icons_dsi_close() ends the session */
}

/*!
 * @brief Serve a copy of obj on one end of a socketpair, as afp_over_dsi()
 *        does, its reads through the request buffer
 *
 * @param[out] s    the session
 * @param[in]  obj  the harness's session object
 * @returns 0, or -1 with nothing left open
 */
static int icons_dsi_open(struct icons_dsi *s, const AFPObj *obj)
{
    if (dsi_test_open(&s->dsi, ICONS_QUANTUM, &s->peer) != 0) {
        return -1;
    }

    s->obj = *obj;
    s->obj.dsi = &s->dsi;
    s->obj.proto = AFPPROTO_DSI;
    s->obj.options.flags |= OPTION_NOSENDFILE;
    s->obj.exit = icons_exit;
    return 0;
}

/*!
 * @brief End a session icons_dsi_open() served
 *
 * @param[in,out] s  the session
 */
static void icons_dsi_close(struct icons_dsi *s)
{
    close(s->peer);
    dsi_test_cleanup(&s->dsi);
}

/*!
 * @brief FPReadExt of count bytes at offset of a fork, as a client sends it
 *
 * @param[in,out] s       the session
 * @param[in]     refnum  the fork
 * @param[in]     offset  the first byte
 * @param[in]     count   the bytes asked
 * @param[out]    out     the reply's bytes, room for count
 * @param[out]    got     their count
 * @returns the AFP result, or -1 when the request or its reply went astray
 */
static int icons_read(struct icons_dsi *s, uint16_t refnum, uint64_t offset,
                      uint32_t count, char *out, uint32_t *got)
{
    uint8_t frame[DSI_BLOCKSIZ + 20];
    char block[DSI_BLOCKSIZ];
    const uint64_t offset_be = hton64(offset);
    const uint64_t count_be = hton64((uint64_t)count);
    size_t rbuflen = DSI_DATASIZ;
    uint32_t code;
    uint32_t len;
    ssize_t n;
    int ret;
    *got = 0;
    dsi_test_frame(frame, DSIFUNC_CMD, 1, 0, 20, AFP_READ_EXT);
    memcpy(frame + DSI_BLOCKSIZ + 2, &refnum, sizeof(refnum));
    memcpy(frame + DSI_BLOCKSIZ + 4, &offset_be, sizeof(offset_be));
    memcpy(frame + DSI_BLOCKSIZ + 12, &count_be, sizeof(count_be));

    if (write(s->peer, frame, sizeof(frame)) != (ssize_t)sizeof(frame)
            || dsi_stream_receive(&s->dsi) != DSIFUNC_CMD) {
        return -1;
    }

    ret = afp_read_ext(&s->obj, (char *)s->dsi.commands, s->dsi.cmdlen,
                       (char *)s->dsi.data, &rbuflen);

    /* a read without a reply of its own leaves the reply to the dispatcher */
    if (!(s->dsi.flags & DSI_NOREPLY)) {
        return ret;
    }

    s->dsi.flags &= ~DSI_NOREPLY;

    if (readt(s->peer, block, sizeof(block), 1, 1) != (ssize_t)sizeof(block)) {
        return -1;
    }

    memcpy(&code, block + 4, sizeof(code));
    memcpy(&len, block + 8, sizeof(len));
    n = (ssize_t)ntohl(len);

    if (n > (ssize_t)count || readt(s->peer, out, (size_t)n, 1, 1) != n) {
        return -1;
    }

    *got = (uint32_t)n;
    return (int)ntohl(code);
}

/*!
 * @brief Give name in the volume root a resource fork of len bytes of a
 *        seed's fork through a write fork, whose close stores the length in
 *        the file's dircache entry
 *
 * @returns 0, or -1 when a step failed
 */
static int icons_write_rfork(AFPObj *obj, const struct vol *vol,
                             const char *name, unsigned int seed, size_t len)
{
    char buf[ICONS_MAXLEN];
    uint16_t w = 0;
    int ok;

    if (len > sizeof(buf)
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD | OPENACC_WR, &w) != AFP_OK) {
        return -1;
    }

    for (size_t i = 0; i < len; i++) {
        buf[i] = icons_byte(seed, i);
    }

    /* only a length that changes marks the fork changed */
    ok = setforkparams(obj, w, 0) == AFP_OK
         && setforkparams(obj, w, (uint32_t)len) == AFP_OK
         && ad_write(of_find(w)->of_ad, ADEID_RFORK, 0, 0, buf, len)
         == (ssize_t)len;
    closefork(obj, w);
    return ok ? 0 : -1;
}

/*!
 * @brief Give name in the volume root a resource fork of ICONS_RLEN bytes of
 *        seed 0, and have a read through a fork, closed again, fill the
 *        file's buffer in the resource fork cache
 *
 * @param[in,out] s     the session
 * @param[in]     vol   the volume
 * @param[in]     name  the file
 * @returns the file's dircache entry, or NULL when a step failed
 */
static struct dir *icons_fill(struct icons_dsi *s, const struct vol *vol,
                              char *name)
{
    char out[ICONS_MAXLEN];
    struct dir *entry;
    uint32_t got;
    uint16_t r = 0;
    int rc;

    if (icons_write_rfork(&s->obj, vol, name, 0, ICONS_RLEN) != 0
            || openfork(&s->obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) != AFP_OK) {
        return NULL;
    }

    rc = icons_read(s, r, 0, ICONS_RLEN, out, &got);
    closefork(&s->obj, r);
    entry = dircache_search_by_name(vol, vol->v_root, name, strlen(name));
    return rc == AFP_OK && got == ICONS_RLEN && entry != NULL
           && entry->dcache_rfork_buf != NULL ? entry : NULL;
}

/*!
 * @brief Create name in the volume root and fill its buffer in the cache, on
 *        a session of the cache tests
 *
 * @param[out] s     the session
 * @param[in]  obj   the harness's session object
 * @param[in]  vol   the volume
 * @param[in]  name  the file
 * @returns 0, or -1 with the file deleted and the session ended
 */
static int icons_cached_file(struct icons_dsi *s, const AFPObj *obj,
                             const struct vol *vol, char *name)
{
    if (icons_dsi_open(s, obj) != 0) {
        return -1;
    }

    if (createfile(&s->obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || icons_fill(s, vol, name) == NULL) {
        delete (&s->obj, vol->v_vid, DIRDID_ROOT, name);
        icons_dsi_close(s);
        return -1;
    }

    return 0;
}

/*!
 * @brief Close a fork when open, delete name and end the session
 *
 * @param[in,out] s       the session
 * @param[in]     vol     the volume
 * @param[in]     refnum  the fork, or 0
 * @param[in]     name    the file
 */
static void icons_done(struct icons_dsi *s, const struct vol *vol,
                       uint16_t refnum, const char *name)
{
    if (refnum) {
        closefork(&s->obj, refnum);
    }

    delete (&s->obj, vol->v_vid, DIRDID_ROOT, name);
    dir_free_invalid_q();
    icons_dsi_close(s);
}

/*!
 * @brief Whether a read-only resource fork open on a volume can take the
 *        dircache entry's buffer: the ea layout, with the fork no attribute
 *        of the file
 *
 * @param[in] vol  the volume
 * @returns true when it can
 */
static bool icons_opens_cached(const struct vol *vol)
{
#if defined(HAVE_EAFD) && defined(SOLARIS)
    (void)vol;
    return false;
#else
    return vol->v_adouble == AD_VERSION_EA;
#endif
}

/*!
 * @brief Whether a fork opened from the cache exactly where the volume lets
 *        a read-only resource fork open there
 *
 * @param[in] vol     the volume
 * @param[in] refnum  the fork
 * @returns true when it did
 */
static bool icons_cached_as_expected(const struct vol *vol, uint16_t refnum)
{
    return ((of_find(refnum)->of_flags & AFPFORK_CACHED) != 0)
           == icons_opens_cached(vol);
}

/*!
 * @brief Whether a descriptor's stat and a path's stat of one file agree on
 *        its ctime, as a buffer's stamp and a cached fork's read compare them
 *
 * An emulator may answer fstat() without the nanoseconds a path stat
 * carries; a buffer stamped through the descriptor then never serves a read.
 *
 * @param[in] vol  the volume, whose root holds the probe's file
 * @returns true when they agree
 */
static bool icons_stamps_agree(const struct vol *vol)
{
    char path[MAXPATHLEN + 1];
    struct stat by_fd;
    struct stat by_path;
    bool agree = false;
    int fd;
    snprintf(path, sizeof(path), "%s/icons_stamp_probe", vol->v_path);
    fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0600);

    if (fd < 0) {
        return false;
    }

    if (fstat(fd, &by_fd) == 0 && lstat(path, &by_path) == 0) {
        const struct timespec a = atalk_stat_ctime_timespec(&by_fd);
        const struct timespec b = atalk_stat_ctime_timespec(&by_path);
        agree = atalk_timespec_eq(&a, &b);
    }

    close(fd);
    unlink(path);
    return agree;
}

/*!
 * @brief The resource fork length of a reply whose bitmap asks it alone
 *
 * @param[in] head  the reply's head, a REPLY_* constant
 * @returns the length, or -1 for a reply without one
 */
static long long icons_reply_rlen(size_t head)
{
    size_t len;
    const char *params = reply_params(head, &len);
    uint32_t rlen;

    if (len != sizeof(rlen)) {
        return -1;
    }

    memcpy(&rlen, params, sizeof(rlen));
    return ntohl(rlen);
}

/*!
 * @brief A release on a fork opened from the cache that never locked
 *        answers AFPERR_NORANGE, as does one whose length reaches the
 *        lock band; a lock of a zero or negative length or at a negative
 *        offset that is not end-relative answers AFPERR_PARAM; none opens
 *        a file
 *
 * @returns 0, 1 when a reply differs or a request opened a file, -1 when
 *          the setup failed, TEST_SKIP without the fault injector
 */
int utest_cached_rfork_release_without_lock(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_crelease";
    struct icons_dsi s;
    uint16_t r = 0;
    int cached;
    int opens;
    int rc;
    int band;
    int bad[3];
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK) {
        goto out;
    }

    cached = icons_cached_as_expected(vol, r);
    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_after = -1;
    rc = bytelock_ext(obj, r, 0, true, 0, 8, NULL);
    band = bytelock_ext(obj, r, 0, true, 0, AD_FILELOCK_BASE, NULL);
    bad[0] = bytelock_ext(obj, r, 0, false, 0, 0, NULL);
    bad[1] = bytelock_ext(obj, r, 0, false, 0, (uint64_t) -2, NULL);
    bad[2] = bytelock_ext(obj, r, 0, false, (uint64_t) -5, 8, NULL);
    opens = fi.open_calls;
    fault_inject_reset();
    ret = cached && rc == AFPERR_NORANGE && band == AFPERR_NORANGE
          && bad[0] == AFPERR_PARAM && bad[1] == AFPERR_PARAM
          && bad[2] == AFPERR_PARAM && opens == 0 ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "cached release: cached %d, reply %d, across the band "
                        "%d, malformed locks %d %d %d, opens %d\n", cached, rc,
                band, bad[0], bad[1], bad[2], opens);
    }

out:
    icons_done(&s, vol, r, name);
    return ret;
}

/*!
 * @brief After the entry's buffer goes, a read of a fork opened from the
 *        cache returns the fork through the descriptor the fork then holds
 *
 * @returns 0, 1 when the read returned other bytes or the fork holds no
 *          descriptor, -1 when the setup failed
 */
int utest_cached_rfork_read_after_buffer_drop(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_cdrop";
    char out[ICONS_MAXLEN];
    struct icons_dsi s;
    struct dir *entry;
    uint32_t got = 0;
    uint16_t r = 0;
    int cached = 0;
    int held = 0;
    int rc = -1;
    int ret = -1;

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK
            || (entry = dircache_search_by_name(vol, vol->v_root, name,
                                                strlen(name))) == NULL) {
        goto out;
    }

    dir_modify(vol, entry, &(struct dir_modify_args) {
        .flags = DCMOD_AD_INV,
    });
    cached = icons_cached_as_expected(vol, r);
    rc = icons_read(&s, r, 0, ICONS_RLEN, out, &got);
    held = ad_rsrc_open(of_find(r)->of_ad);
    ret = cached && rc == AFP_OK && got == ICONS_RLEN
          && icons_is(out, 0, 0, got) && held ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "read past the buffer: cached %d, reply %d, %u bytes, "
                        "held %d\n", cached, rc, got, held);
    }

out:
    icons_done(&s, vol, r, name);
    return ret;
}

/*!
 * @brief A fork opened from the cache meets the deny modes: another
 *        process's deny-read band refuses the open, and while the fork is
 *        open its open-read band refuses another process's deny-read probe
 *
 * @returns 0, 1 when a deny mode was missed, -1 when the setup failed
 */
int utest_cached_rfork_meets_deny_read(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_cdeny";
    char path[MAXPATHLEN + 1];
    struct icons_dsi s;
    struct peer peer;
    uint16_t r = 0;
    int denied = -1;
    int cached = 0;
    int probed = -1;
    int freed = -1;
    int ret = -1;

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    if (peer_hold_lock(&peer, path, F_RDLCK, AD_FILELOCK_RSRC_DENY_RD, 1)
            != 0) {
        goto out;
    }

    denied = openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                      OPENACC_RD, &r);
    peer_release(&peer);

    if (denied == AFP_OK) {
        closefork(obj, r);
        r = 0;
    } else if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) == AFP_OK) {
        cached = icons_cached_as_expected(vol, r);
        probed = peer_try_lock(path, F_WRLCK, AD_FILELOCK_RSRC_OPEN_RD, 1);
        closefork(obj, r);
        r = 0;
        freed = peer_try_lock(path, F_WRLCK, AD_FILELOCK_RSRC_OPEN_RD, 1);
    }

    ret = denied == AFPERR_DENYCONF && cached && probed == 0 && freed == 1
          ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "cached deny modes: open %d, cached %d, probe %d, "
                        "after %d\n", denied, cached, probed, freed);
    }

out:
    icons_done(&s, vol, r, name);
    return ret;
}

/*!
 * @brief The attributes, Finder info and resource fork length a file's
 *        parameters report while a fork opened from the cache holds it
 *
 * @param[in]  obj    the session
 * @param[in]  vol    the volume
 * @param[in]  name   the file
 * @param[in]  finfo  the Finder info the file was given
 * @param[out] attr   the attributes, in network order
 * @returns 0 when the Finder info is finfo and the length ICONS_RLEN, 1 when
 *          either differs, -1 when the call failed
 */
static int icons_file_params(AFPObj *obj, const struct vol *vol,
                             const char *name, const char *finfo,
                             uint16_t *attr)
{
    const uint16_t bitmap = (1 << FILPBIT_ATTR) | (1 << FILPBIT_FINFO)
                            | (1 << FILPBIT_RFLEN);
    const char *params;
    size_t len;
    uint32_t rlen;

    if (getfiledirparms_bitmap(obj, vol->v_vid, DIRDID_ROOT, name, bitmap, 0)
            != AFP_OK
            || (params = reply_params(REPLY_FILEDIRPARMS, &len)) == NULL
            || len != sizeof(*attr) + ADEDLEN_FINDERI + sizeof(rlen)) {
        return -1;
    }

    memcpy(attr, params, sizeof(*attr));
    memcpy(&rlen, params + sizeof(*attr) + ADEDLEN_FINDERI, sizeof(rlen));
    return memcmp(params + sizeof(*attr), finfo, ADEDLEN_FINDERI) == 0
           && ntohl(rlen) == ICONS_RLEN ? 0 : 1;
}

/*!
 * @brief The parameters of a file a fork opened from the cache holds come
 *        from the disk's header: its invisible bit, its Finder info, the
 *        fork's length and its open-fork bit, before and after a lock, and
 *        a copy of the file carries the Finder info
 *
 * @returns 0, 1 when a parameter differs, -1 when the setup failed
 */
int utest_cached_rfork_file_params_keep_metadata(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_cparams";
    static char copy[] = "icons_cparams_copy";
    const uint16_t want = ATTRBIT_INVISIBLE | ATTRBIT_ROPEN;
    const uint16_t flags = htons(FINDERINFO_INVISIBLE
                                 | FINDERINFO_HASCUSTOMICON);
    char finfo[ADEDLEN_FINDERI] = { 'T', 'E', 'S', 'T', 'i', 'c', 'n', 's' };
    struct icons_dsi s;
    const char *params;
    size_t len;
    uint16_t r = 0;
    uint16_t attr[2] = { 0, 0 };
    int cached = 0;
    int before = -1;
    int after = -1;
    int copied = 0;
    int ret = -1;
    memcpy(finfo + FINDERINFO_FRFLAGOFF, &flags, sizeof(flags));

    if (icons_dsi_open(&s, obj) != 0) {
        return -1;
    }

    if (createfile(&s.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || setfileparms(&s.obj, vol->v_vid, DIRDID_ROOT, name,
                            1 << FILPBIT_FINFO, finfo, sizeof(finfo))
            != AFP_OK
            || icons_fill(&s, vol, name) == NULL
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) != AFP_OK) {
        goto out;
    }

    cached = icons_cached_as_expected(vol, r);
    before = icons_file_params(obj, vol, name, finfo, &attr[0]);

    if (bytelock_ext(obj, r, 0, false, 0, 1, NULL) == AFP_OK) {
        after = icons_file_params(obj, vol, name, finfo, &attr[1]);
    }

    copied = copyfileobj(obj, vol->v_vid, DIRDID_ROOT, name, DIRDID_ROOT,
                         copy) == AFP_OK
             && getfiledirparms_bitmap(obj, vol->v_vid, DIRDID_ROOT, copy,
                                       1 << FILPBIT_FINFO, 0) == AFP_OK
             && (params = reply_params(REPLY_FILEDIRPARMS, &len)) != NULL
             && len == sizeof(finfo) && memcmp(params, finfo, len) == 0;
    ret = cached && before == 0 && after == 0 && copied
          && (ntohs(attr[0]) & want) == want
          && (ntohs(attr[1]) & want) == want ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "cached file params: cached %d, %d then %d, attributes "
                        "%#x then %#x, copy %d\n", cached, before, after,
                ntohs(attr[0]), ntohs(attr[1]), copied);
    }

out:
    delete (&s.obj, vol->v_vid, DIRDID_ROOT, copy);
    icons_done(&s, vol, r, name);
    return ret;
}

/*!
 * @brief A fork opened from the cache closed beside a data fork of the file
 *        drops its open-fork bit, which no reference carried
 *
 * @returns 0, 1 when the file reports other bits, -1 when the setup failed
 */
int utest_cached_rfork_close_drops_its_open_bit(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_cbit";
    const int bits = ATTRBIT_DOPEN | ATTRBIT_ROPEN;
    struct icons_dsi s;
    uint16_t r = 0;
    uint16_t d = 0;
    int cached = 0;
    int attr = -1;
    int ret = -1;

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                        OPENACC_RD, &d) != AFP_OK) {
        goto out;
    }

    cached = icons_cached_as_expected(vol, r);
    closefork(obj, r);
    r = 0;
    attr = icons_file_attr(obj, vol, name);
    ret = cached && attr >= 0 && (attr & bits) == ATTRBIT_DOPEN ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "cached fork closed: cached %d, attributes %#x, "
                "expected %#x\n", cached, attr, ATTRBIT_DOPEN);
    }

out:

    if (d) {
        closefork(obj, d);
    }

    icons_done(&s, vol, r, name);
    return ret;
}

/*!
 * @brief The session's second open of a file a fork opened from the cache
 *        holds joins the adouble and brings the descriptor; the first fork
 *        reads from the buffer beside it and after its close
 *
 * @returns 0, 1 when the open did not join or a read missed the buffer, -1
 *          when the setup failed, TEST_SKIP where a descriptor's stat and a
 *          path's disagree on the ctime
 */
int utest_cached_rfork_second_open_joins(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_cjoin";
    char out[ICONS_MAXLEN];
    struct icons_dsi s;
    unsigned long long hits[3] = { 0, 0, 0 };
    uint32_t got[2] = { 0, 0 };
    uint16_t r = 0;
    uint16_t r2 = 0;
    int joined = 0;
    int served[2] = { 0, 0 };
    int ret = -1;

    if (!icons_stamps_agree(vol)) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r2) != AFP_OK) {
        goto out;
    }

    joined = icons_cached_as_expected(vol, r)
             && of_find(r2)->of_ad == of_find(r)->of_ad
             && ad_rsrc_open(of_find(r)->of_ad);

    for (int i = 0; i < 2; i++) {
        hits[i] = rfork_stat_hits;
        served[i] = icons_read(&s, r, 0, ICONS_RLEN, out, &got[i]) == AFP_OK
                    && got[i] == ICONS_RLEN && icons_is(out, 0, 0, got[i]);

        if (i == 0) {
            closefork(obj, r2);
            r2 = 0;
        }
    }

    hits[2] = rfork_stat_hits;
    ret = joined && served[0] && served[1] && hits[1] == hits[0] + 1
          && hits[2] == hits[1] + 1 ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "cached join: joined %d, reads %d %d, hits %llu %llu\n",
                joined, served[0], served[1], hits[1] - hits[0],
                hits[2] - hits[1]);
    }

out:

    if (r2) {
        closefork(obj, r2);
    }

    icons_done(&s, vol, r, name);
    return ret;
}

/*!
 * @brief A fork opened from the cache whose file another process renamed,
 *        with the entry gone, refuses a read with AFPERR_MISC; FPGetForkParms
 *        and the new name's FPGetFileDirParms through the held fork report
 *        the length the open took, and the close succeeds
 *
 * @returns 0, 1 when a read answered otherwise or a length differs, -1 when
 *          the setup failed, TEST_SKIP where no fork opens from the cache
 */
int utest_cached_rfork_renamed_file_read_refused(AFPObj *obj, struct vol *vol)
{
    static char name[] = "icons_crename";
    static char moved[] = "icons_crename_moved";
    char path[MAXPATHLEN + 1];
    char newpath[MAXPATHLEN + 1];
    char out[ICONS_MAXLEN];
    struct icons_dsi s;
    long long fork_rlen = -1;
    long long file_rlen = -1;
    off_t held_rlen = -1;
    uint32_t got = 0;
    uint16_t r = 0;
    int rc = -1;
    int closed = -1;
    int ret = -1;

    if (!icons_opens_cached(vol)) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(newpath, sizeof(newpath), "%s/%s", vol->v_path, moved);

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK || rename(path, newpath) != 0) {
        goto out;
    }

    drop_entry(vol, DIRDID_ROOT, name);
    rc = icons_read(&s, r, 0, ICONS_RLEN, out, &got);

    if (getforkparams(obj, r, 1 << FILPBIT_RFLEN) == AFP_OK) {
        fork_rlen = icons_reply_rlen(REPLY_FORKPARMS);
    }

    held_rlen = of_find(r)->of_ad->ad_rlen;

    if (getfiledirparms_bitmap(obj, vol->v_vid, DIRDID_ROOT, moved,
                               1 << FILPBIT_RFLEN, 0) == AFP_OK) {
        file_rlen = icons_reply_rlen(REPLY_FILEDIRPARMS);
    }

    closed = closefork(obj, r);
    r = 0;
    ret = rc == AFPERR_MISC && got == 0 && fork_rlen == ICONS_RLEN
          && held_rlen == ICONS_RLEN && file_rlen == ICONS_RLEN
          && closed == AFP_OK ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "renamed file: read %d, %u bytes, lengths %lld %lld "
                        "%lld, close %d\n", rc, got, fork_rlen,
                (long long)held_rlen, file_rlen, closed);
    }

out:
    delete (&s.obj, vol->v_vid, DIRDID_ROOT, moved);
#ifndef HAVE_EAFD
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, vol->ad_path(name, 0));
    unlink(path);
#endif
    icons_done(&s, vol, r, name);
    return ret;
}

#ifndef HAVE_EAFD
/*!
 * @brief Whether the preload counts the calls icons_count() arms, the xattr
 *        reads apart, which only Linux counts
 *
 * @param[in] vol  the volume, for the probes
 * @returns true when it does
 */
static bool icons_counts(const struct vol *vol)
{
    return test_capability(TEST_CAP_FAULT_INJECT, vol)
           && test_capability(TEST_CAP_PREAD_WATCH, vol)
           && test_capability(TEST_CAP_FSTAT_WATCH, vol)
           && test_capability(TEST_CAP_CLOSE_WATCH, vol)
           && test_capability(TEST_CAP_STAT_WATCH, vol);
}

/*!
 * @brief Count the calls the next operation makes: open(), pread(), the
 *        xattr reads, every fstat(), close() and the stats of paths ending in
 *        suffix, with no sampled validation of a dircache entry among them
 *
 * @param[in,out] obj     the harness's session object, whose validation
 *                        frequency the dircache reads
 * @param[in]     suffix  the sidecar's name
 * @returns the validation frequency, for icons_uncount()
 */
static int icons_count(AFPObj *obj, const char *suffix)
{
    const int freq = obj->options.dircache_validation_freq;
    obj->options.dircache_validation_freq = INT_MAX;
    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_after = -1;
    fi.xattr_watch = 1;
    fi.fstat_watch = 1;
    fi.fstat_fd = -1;
    fi.close.armed = 1;
    fi.close.fail_after = -1;
    fi.stat_suffix = suffix;
    return freq;
}

/*!
 * @brief Stop counting, keep the counts and give the dircache its
 *        validation frequency back
 *
 * @param[in,out] obj   the harness's session object
 * @param[in]     freq  the frequency icons_count() returned
 * @param[out]    seen  the counts
 */
static void icons_uncount(AFPObj *obj, int freq, struct fault_inject *seen)
{
    *seen = fi;
    fault_inject_reset();
    obj->options.dircache_validation_freq = freq;
}

/*!
 * @brief Write the ._ file at path behind the server's back, in place: the
 *        header afpd writes, then len bytes of a seed's fork
 *
 * @returns 0, or -1 when len is too long or the write failed
 */
static int icons_put_sidecar(const char *path, unsigned int seed, size_t len)
{
    const uint32_t entry[] = {
        ADEID_RFORK, ADEDOFF_RFORK_OSX, (uint32_t)len,
        ADEID_FINDERI, ADEDOFF_FINDERI_OSX, ADEDLEN_FINDERI,
    };
    uint8_t sidecar[ADEDOFF_RFORK_OSX + ICONS_MAXLEN] = { 0 };

    if (len > ICONS_MAXLEN) {
        return -1;
    }

    pack_appledouble(sidecar, entry, 2);

    for (size_t i = 0; i < len; i++) {
        sidecar[ADEDOFF_RFORK_OSX + i] = (uint8_t)icons_byte(seed, i);
    }

    return write_disk_file(path, sidecar, ADEDOFF_RFORK_OSX + len);
}

/*!
 * @brief Give the volume root, a file and its sidecar to a user, or back
 *        to their owner, through descriptors of their own: no fork of the
 *        file may be open, as a descriptor's close drops the process's locks
 *        on its file
 *
 * @returns 0, or -1 when a change failed
 */
static int icons_give(const struct vol *vol, const char *path,
                      const char *sidecar, uid_t uid, gid_t gid)
{
    const char *const paths[] = { vol->v_path, path, sidecar };
    const mode_t modes[] = { 0700, 0600, 0600 };
    int ret = 0;

    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        const int fd = open(paths[i], O_RDONLY);

        if (fd < 0 || fchown(fd, uid, gid) != 0 || fchmod(fd, modes[i]) != 0) {
            ret = -1;
        }

        if (fd >= 0) {
            close(fd);
        }
    }

    return ret;
}

/*!
 * @brief Count the open() calls of a read-only resource fork open of name,
 *        closed again
 *
 * @returns the count, or -1 when the open failed
 */
static int icons_open_calls(AFPObj *obj, const struct vol *vol,
                            const char *name)
{
    uint16_t r = 0;
    int opens;
    int rc;
    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_after = -1;
    rc = openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                  OPENACC_RD, &r);
    opens = fi.open_calls;
    fault_inject_reset();

    if (rc != AFP_OK) {
        return -1;
    }

    closefork(obj, r);
    return opens;
}

/*!
 * @brief Whether a descriptor a fork held is closed
 *
 * @param[in] fd  the descriptor's number, as the fork held it
 * @returns true when an fstat() of it fails with EBADF
 */
static bool icons_fd_closed(int fd)
{
    struct stat st;
    return fd >= 0 && fstat(fd, &st) == -1 && errno == EBADF;
}

/*!
 * @brief A fork opened from the cache beside a second fork, which brings
 *        the descriptor and closes after the fork's lock: the range stays
 *        locked, the release answers AFP_OK, and the close closes both
 *        descriptors
 *
 * @returns 0, 1 when the lock went with the second fork or a reply differs,
 *          -1 when an open failed
 */
static int icons_lock_outlives_sibling(AFPObj *obj, const struct vol *vol,
                                       const char *name, const char *sidecar)
{
    const struct ofork *of;
    uint16_t r = 0;
    uint16_t r2 = 0;
    int cached;
    int locked;
    int held;
    int kept;
    int released;
    int closed;
    int dfd;
    int rfd;

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r2) != AFP_OK) {
        if (r) {
            closefork(obj, r);
        }

        return -1;
    }

    cached = icons_cached_as_expected(vol, r);
    locked = bytelock_ext(obj, r, 0, false, 0, 8, NULL);
    of = of_find(r);
    held = ad_rsrc_open(of->of_ad);
    closefork(obj, r2);
    kept = peer_try_lock(sidecar, F_WRLCK, ADEDOFF_RFORK_OSX, 8);
    released = bytelock_ext(obj, r, 0, true, 0, 8, NULL);
    dfd = ad_data_fileno(of->of_ad);
    rfd = ad_reso_fileno(of->of_ad);
    closefork(obj, r);
    closed = icons_fd_closed(dfd) && icons_fd_closed(rfd);

    if (cached && locked == AFP_OK && held && kept == 0 && released == AFP_OK
            && closed) {
        return 0;
    }

    fprintf(stderr, "cached lock: cached %d, reply %d, descriptor %d, kept %d, "
                    "release %d, closed %d\n", cached, locked, held, kept,
            released, closed);
    return 1;
}

/*!
 * @brief A lock on a fork opened from the cache meets another process's lock
 *        on the range: AFPERR_LOCK
 *
 * @returns 0, 1 when the lock answered otherwise, -1 when the setup failed
 */
static int icons_lock_meets_peer(AFPObj *obj, const struct vol *vol,
                                 const char *name, const char *sidecar)
{
    struct peer peer;
    uint16_t r = 0;
    int cached;
    int rc;

    if (peer_hold_lock(&peer, sidecar, F_WRLCK, ADEDOFF_RFORK_OSX, 8) != 0) {
        return -1;
    }

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK) {
        peer_release(&peer);
        return -1;
    }

    cached = icons_cached_as_expected(vol, r);
    rc = bytelock_ext(obj, r, 0, false, 0, 8, NULL);
    closefork(obj, r);
    peer_release(&peer);

    if (!cached || rc != AFPERR_LOCK) {
        fprintf(stderr, "cached lock beside a peer's: cached %d, reply %d\n",
                cached, rc);
        return 1;
    }

    return 0;
}

/*!
 * @brief A lock on a fork opened from the cache, by a user whose write the
 *        sidecar's mode refuses: the second fork holds a read-only
 *        descriptor, which the lock joins
 *
 * @returns 0, 1 when the lock was refused, -1 when the setup failed,
 *          TEST_SKIP where root cannot take on another user
 */
static int icons_lock_joins_readonly(struct icons_dsi *s, AFPObj *obj,
                                     const struct vol *vol, char *name,
                                     const char *sidecar)
{
    const uid_t euid = geteuid();
    const uid_t owner = euid == 0 ? 65534 : euid;
    char path[MAXPATHLEN + 1];
    struct stat root;
    uint16_t r = 0;
    uint16_t r2 = 0;
    int user = 0;
    int cached = 0;
    int rc = -1;
    int ret = -1;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    if (stat(vol->v_path, &root) != 0) {
        return -1;
    }

    /* the user's files, then the buffer filled under their stat */
    if ((euid == 0 && icons_give(vol, path, sidecar, owner, getegid()) != 0)
            || icons_fill(s, vol, name) == NULL || chmod(sidecar, 0400) != 0) {
        goto out;
    }

    if (euid == 0 && seteuid(owner) != 0) {
        ret = TEST_SKIP;
        goto out;
    }

    user = euid == 0;

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) == AFP_OK
            && openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r2) == AFP_OK) {
        cached = icons_cached_as_expected(vol, r);
        rc = bytelock_ext(obj, r, 0, false, 0, 8, NULL);
    }

    if (r2) {
        closefork(obj, r2);
    }

    if (r) {
        closefork(obj, r);
    }

    if (user && seteuid(0) != 0) {
        goto out;
    }

    user = 0;
    ret = cached && rc == AFP_OK ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "cached lock under a read-only sidecar: cached %d, "
                        "reply %d\n", cached, rc);
    }

out:

    if (user && seteuid(0) != 0) {
        ret = -1;
    }

    if (euid == 0
            && icons_give(vol, path, sidecar, root.st_uid, root.st_gid) != 0) {
        ret = -1;
    }

    (void)chmod(sidecar, 0600);
    return ret;
}

/*! One rewrite of the sidecar behind the server's back, which a fork opened
 *  from the cache then reads */
struct icons_rewrite {
    unsigned int seed;  /*!< the new fork's content */
    size_t len;         /*!< its length */
    bool lock;          /*!< the fork takes its resource fork by a lock first */
    bool tick;          /*!< the sidecar's next second comes first */
    uint64_t offset;    /*!< the read's first byte */
    uint32_t count;     /*!< the read's length */
};

/*!
 * @brief Run one rewrite on a buffer filled anew and a fork opened from the
 *        cache
 *
 * @param[in,out] s     the session
 * @param[in]     vol   the volume
 * @param[in]     name  the file
 * @param[in]     c     the rewrite
 * @returns 0, 1 when the read returned other bytes, -1 when the setup failed
 */
static int icons_rewrite_case(struct icons_dsi *s, const struct vol *vol,
                              char *name, const struct icons_rewrite *c)
{
    char sidecar[MAXPATHLEN + 1];
    char out[ICONS_MAXLEN];
    uint32_t got = 0;
    uint16_t r = 0;
    int cached;
    int rc = -1;
    snprintf(sidecar, sizeof(sidecar), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    if (icons_fill(s, vol, name) == NULL
            || openfork(&s->obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) != AFP_OK) {
        return -1;
    }

    cached = icons_cached_as_expected(vol, r);

    if ((!c->lock || bytelock_ext(&s->obj, r, 0, false, 0, 1, NULL) == AFP_OK)
            && (!c->tick || next_second(sidecar) == 0)
            && icons_put_sidecar(sidecar, c->seed, c->len) == 0) {
        rc = icons_read(s, r, c->offset, c->count, out, &got);
    }

    closefork(&s->obj, r);

    if (cached && rc == AFP_OK && got == c->count
            && icons_is(out, c->seed, (size_t)c->offset, got)) {
        return 0;
    }

    fprintf(stderr, "rewrite %u to %zu bytes: cached %d, reply %d, %u bytes\n",
            c->seed, c->len, cached, rc, got);
    return 1;
}
#endif /* HAVE_EAFD */

/*!
 * @brief A read-only open of a resource fork the cache holds opens the data
 *        file alone: one open(), no pread(), the metadata EA's read, the
 *        reply's fstat() and no stat of the sidecar; its reply is the one an
 *        open of the fork the entry no longer holds gives
 *
 * @returns 0, 1 when the open made other calls or replied otherwise, -1 when
 *          the setup failed, TEST_SKIP without the counting preload, where
 *          the sidecar is no file of its own or on a volume served as
 *          version 2
 */
int utest_cached_rfork_open_opens_data_file_only(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_copen";
    const uint16_t bitmap = (1 << FILPBIT_ATTR) | (1 << FILPBIT_MDATE)
                            | (1 << FILPBIT_FINFO) | (1 << FILPBIT_FNUM)
                            | (1 << FILPBIT_RFLEN);
    char cached[128];
    char suffix[MAXPATHLEN + 1];
    struct fault_inject seen;
    struct icons_dsi s;
    struct dir *entry;
    const char *params;
    size_t cached_len = 0;
    size_t len = 0;
    uint16_t r = 0;
    int same = 0;
    int freq;
    int rc;
    int ret = -1;

    if (!icons_opens_cached(vol) || !icons_counts(vol)
            || !test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    snprintf(suffix, sizeof(suffix), "%s", vol->ad_path(name, 0));
    freq = icons_count(obj, suffix);
    rc = openfork_bitmap(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                         OPENACC_RD, bitmap, &r);
    icons_uncount(obj, freq, &seen);
    params = reply_params(REPLY_OPENFORK, &cached_len);

    if (rc != AFP_OK || cached_len > sizeof(cached)) {
        goto out;
    }

    memcpy(cached, params, cached_len);
    closefork(obj, r);
    r = 0;

    /* an entry without the buffer: the open takes the sidecar */
    if ((entry = dircache_search_by_name(vol, vol->v_root, name,
                                         strlen(name))) == NULL) {
        goto out;
    }

    dir_modify(vol, entry, &(struct dir_modify_args) {
        .flags = DCMOD_AD_INV,
    });

    if (openfork_bitmap(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, bitmap, &r) == AFP_OK) {
        params = reply_params(REPLY_OPENFORK, &len);
        same = cached_len > 0 && len == cached_len
               && memcmp(params, cached, len) == 0;
    }

    ret = seen.open_calls == 1 && seen.pread_calls == 0
          && seen.getxattr_calls == 1 && seen.fstat_calls == 1
          && seen.stat_suffix_calls == 0 && same ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "cached open: open %d, pread %d, getxattr %d, "
                        "fstat %d, sidecar stats %d, same reply %d\n",
                seen.open_calls, seen.pread_calls, seen.getxattr_calls,
                seen.fstat_calls, seen.stat_suffix_calls, same);
    }

out:
    icons_done(&s, vol, r, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief A read of a fork opened from the cache stats the sidecar once,
 *        opens and reads no file, and returns the fork from the buffer
 *
 * @returns 0, 1 when the read made other calls or returned other bytes, -1
 *          when the setup failed, TEST_SKIP without the counting preload,
 *          where the sidecar is no file of its own, on a volume served as
 *          version 2 or where a descriptor's stat and a path's disagree on
 *          the ctime
 */
int utest_cached_rfork_read_stats_sidecar_once(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_cread";
    char suffix[MAXPATHLEN + 1];
    char out[ICONS_MAXLEN];
    struct fault_inject seen;
    struct icons_dsi s;
    uint32_t got = 0;
    uint16_t r = 0;
    int freq;
    int rc;
    int ret = -1;

    if (!icons_opens_cached(vol) || !icons_counts(vol)
            || !icons_stamps_agree(vol)) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    snprintf(suffix, sizeof(suffix), "%s", vol->ad_path(name, 0));

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK) {
        goto out;
    }

    freq = icons_count(obj, suffix);
    rc = icons_read(&s, r, 0, ICONS_RLEN, out, &got);
    icons_uncount(obj, freq, &seen);
    ret = rc == AFP_OK && got == ICONS_RLEN && icons_is(out, 0, 0, got)
          && seen.stat_suffix_calls == 1 && seen.fstat_calls == 0
          && seen.open_calls == 0 && seen.pread_calls == 0 ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "cached read: reply %d, %u bytes, sidecar stats %d, "
                        "fstat %d, open %d, pread %d\n", rc, got,
                seen.stat_suffix_calls, seen.fstat_calls, seen.open_calls,
                seen.pread_calls);
    }

out:
    icons_done(&s, vol, r, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief The close of a fork opened from the cache closes one descriptor and
 *        reads no header
 *
 * @returns 0, 1 when the close made other calls, -1 when the setup failed,
 *          TEST_SKIP without the counting preload, where the sidecar is no
 *          file of its own or on a volume served as version 2
 */
int utest_cached_rfork_close_closes_one_descriptor(AFPObj *obj,
                                                   struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_cclose";
    char suffix[MAXPATHLEN + 1];
    struct fault_inject seen;
    struct icons_dsi s;
    uint16_t r = 0;
    int freq;
    int rc;
    int ret = -1;

    if (!icons_opens_cached(vol) || !icons_counts(vol)) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    snprintf(suffix, sizeof(suffix), "%s", vol->ad_path(name, 0));

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK) {
        goto out;
    }

    freq = icons_count(obj, suffix);
    rc = closefork(obj, r);
    icons_uncount(obj, freq, &seen);
    r = 0;
    ret = rc == AFP_OK && seen.close.calls == 1 && seen.pread_calls == 0
          && seen.open_calls == 0 && seen.fstat_calls == 0
          && seen.stat_suffix_calls == 0 ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "cached close: reply %d, close %d, pread %d, open %d, "
                        "fstat %d, sidecar stats %d\n", rc, seen.close.calls,
                seen.pread_calls, seen.open_calls, seen.fstat_calls,
                seen.stat_suffix_calls);
    }

out:
    icons_done(&s, vol, r, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief A byte-range lock on a fork opened from the cache takes the fork's
 *        own resource fork reference: a second fork that brought the
 *        descriptor closes without the lock, another process's lock on the
 *        range refuses it, and a read-only descriptor joins
 *
 * @returns 0, 1 when a lock was lost or refused, -1 when the setup failed,
 *          TEST_SKIP where the sidecar is no file of its own or on a volume
 *          served as version 2
 */
int utest_cached_rfork_lock_takes_own_reference(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_clock";
    char sidecar[MAXPATHLEN + 1];
    struct icons_dsi s;
    int ret;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    snprintf(sidecar, sizeof(sidecar), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    if ((ret = icons_lock_outlives_sibling(obj, vol, name, sidecar)) == 0
            && (ret = icons_lock_meets_peer(obj, vol, name, sidecar)) == 0) {
        ret = icons_lock_joins_readonly(&s, obj, vol, name, sidecar);
    }

    icons_done(&s, vol, 0, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief A read of a fork opened from the cache returns the sidecar another
 *        process rewrote: a longer fork, the same length a second later, the
 *        same on a fork that took its resource fork by a lock, and a read
 *        past the old length of a longer fork
 *
 * @returns 0, 1 when a read returned the old fork, -1 when the setup failed,
 *          TEST_SKIP where the sidecar is no file of its own or on a volume
 *          served as version 2
 */
int utest_cached_rfork_read_sees_rewrite(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_crewrite";
    static const struct icons_rewrite cases[] = {
        { 1, ICONS_LONGER, false, false, 0, ICONS_LONGER },
        { 2, ICONS_RLEN, false, true, 0, ICONS_RLEN },
        { 3, ICONS_RLEN, true, true, 0, ICONS_RLEN },
        {
            4, ICONS_LONGER, false, false, ICONS_RLEN,
            ICONS_LONGER - ICONS_RLEN
        },
    };
    struct icons_dsi s;
    int ret = 0;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]) && ret == 0; i++) {
        ret = icons_rewrite_case(&s, vol, name, &cases[i]);
    }

    icons_done(&s, vol, 0, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief A fork opened from the cache whose sidecar another process removed
 *        reads nothing, refuses a lock and reports an empty fork to
 *        FPGetForkParms and to a listing, holding no resource fork; its
 *        close leaves nothing open
 *
 * @returns 0, 1 when a reply differs or a descriptor stays, -1 when the
 *          setup failed, TEST_SKIP where the sidecar is no file of its own
 *          or no fork opens from the cache
 */
int utest_cached_rfork_missing_sidecar_reads_eof(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_cmissing";
    char sidecar[MAXPATHLEN + 1];
    char out[ICONS_MAXLEN];
    struct icons_dsi s;
    long long rlen = -1;
    long long listed = -1;
    uint32_t got[2] = { 1, 1 };
    uint16_t r = 0;
    int rc[3] = { -1, -1, -1 };
    int unheld = 0;
    int closed = 0;
    int dfd;
    int ret = -1;

    if (!icons_opens_cached(vol)) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    snprintf(sidecar, sizeof(sidecar), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK || unlink(sidecar) != 0) {
        goto out;
    }

    rc[0] = icons_read(&s, r, 0, ICONS_RLEN, out, &got[0]);
    rc[1] = icons_read(&s, r, 0, ICONS_RLEN, out, &got[1]);
    rc[2] = bytelock_ext(obj, r, 0, false, 0, 1, NULL);

    if (getforkparams(obj, r, 1 << FILPBIT_RFLEN) == AFP_OK) {
        rlen = icons_reply_rlen(REPLY_FORKPARMS);
    }

    if (getfiledirparms_bitmap(obj, vol->v_vid, DIRDID_ROOT, name,
                               1 << FILPBIT_RFLEN, 0) == AFP_OK) {
        listed = icons_reply_rlen(REPLY_FILEDIRPARMS);
    }

    unheld = !(of_find(r)->of_adflags & ADFLAGS_RF);
    dfd = ad_data_fileno(of_find(r)->of_ad);
    closed = closefork(obj, r) == AFP_OK && icons_fd_closed(dfd);
    r = 0;
    ret = rc[0] == AFPERR_EOF && rc[1] == AFPERR_EOF && got[0] == 0
          && got[1] == 0 && rc[2] == AFPERR_PARAM && rlen == 0 && listed == 0
          && unheld && closed ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "no sidecar: reads %d %d, bytes %u %u, lock %d, "
                        "length %lld, listed %lld, unheld %d, closed %d\n",
                rc[0], rc[1], got[0], got[1], rc[2], rlen, listed, unheld,
                closed);
    }

out:
    icons_done(&s, vol, r, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief A fork opened from the cache whose sidecar another process cut to
 *        its header reads an empty fork through the descriptor it then
 *        holds, and locks through it
 *
 * @returns 0, 1 when a reply differs, -1 when the setup failed, TEST_SKIP
 *          where the sidecar is no file of its own or on a volume served as
 *          version 2
 */
int utest_cached_rfork_short_sidecar_reads_eof(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_cshort";
    char sidecar[MAXPATHLEN + 1];
    char out[ICONS_MAXLEN];
    struct icons_dsi s;
    uint32_t got = 1;
    uint16_t r = 0;
    int cached = 0;
    int rc = -1;
    int held = 0;
    int locked = -1;
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    snprintf(sidecar, sizeof(sidecar), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK
            || truncate(sidecar, ADEDOFF_RFORK_OSX) != 0) {
        goto out;
    }

    cached = icons_cached_as_expected(vol, r);
    rc = icons_read(&s, r, 0, ICONS_RLEN, out, &got);
    held = ad_rsrc_open(of_find(r)->of_ad);
    locked = bytelock_ext(obj, r, 0, false, 0, 1, NULL);
    ret = cached && rc == AFPERR_EOF && got == 0 && held && locked == AFP_OK
          ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "emptied sidecar: cached %d, read %d, %u bytes, held "
                        "%d, lock %d\n", cached, rc, got, held, locked);
    }

out:
    icons_done(&s, vol, r, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief A fork opened from the cache whose sidecar another process filled
 *        with bytes that are no header refuses the read, and the entry drops
 *        the buffer
 *
 * @returns 0, 1 when the read went through or the buffer stayed, -1 when the
 *          setup failed, TEST_SKIP where the sidecar is no file of its own
 *          or no fork opens from the cache
 */
int utest_cached_rfork_bad_sidecar_read_refused(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_cbad";
    static const char garbage[200] = { 'x' };
    char sidecar[MAXPATHLEN + 1];
    char out[ICONS_MAXLEN];
    struct icons_dsi s;
    const struct dir *entry;
    uint32_t got = 0;
    uint16_t r = 0;
    int rc = -1;
    int dropped = 0;
    int ret = -1;

    if (!icons_opens_cached(vol)) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    snprintf(sidecar, sizeof(sidecar), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                 OPENACC_RD, &r) != AFP_OK
            || write_disk_file(sidecar, garbage, sizeof(garbage)) != 0) {
        goto out;
    }

    rc = icons_read(&s, r, 0, ICONS_RLEN, out, &got);
    entry = dircache_search_by_name(vol, vol->v_root, name, strlen(name));
    dropped = entry != NULL && entry->dcache_rfork_buf == NULL;
    ret = rc == AFPERR_PARAM && dropped ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "bad sidecar: read %d, buffer gone %d\n", rc,
                dropped);
    }

out:
    icons_done(&s, vol, r, name);
    unlink(sidecar);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief A fork opened from the cache whose sidecar's mode another process
 *        cleared refuses a read by a user the mode refuses with
 *        AFPERR_ACCESS and that user's lock with AFPERR_MISC, and the entry
 *        drops the buffer the sidecar no longer matches
 *
 * @returns 0, 1 when a reply differs or the buffer stayed, -1 when the
 *          setup failed,
 *          TEST_SKIP where the sidecar is no file of its own, no fork opens
 *          from the cache or root cannot take on another user
 */
int utest_cached_rfork_unreadable_sidecar_read_refused(AFPObj *obj,
                                                       struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_cmode";
    const uid_t euid = geteuid();
    const uid_t owner = euid == 0 ? 65534 : euid;
    char path[MAXPATHLEN + 1];
    char sidecar[MAXPATHLEN + 1];
    char out[ICONS_MAXLEN];
    struct icons_dsi s;
    struct stat root;
    const struct dir *entry;
    uint32_t got = 0;
    uint16_t r = 0;
    int user = 0;
    int dropped = 0;
    int rc = -1;
    int locked = -1;
    int ret = -1;

    if (!icons_opens_cached(vol) || stat(vol->v_path, &root) != 0) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(sidecar, sizeof(sidecar), "%s/%s", vol->v_path,
             vol->ad_path(name, 0));

    /* the user's files, then the buffer filled under their stat; the mode
     * changes a second after the fill, so the stamp differs */
    if ((euid == 0 && (icons_give(vol, path, sidecar, owner, getegid()) != 0
                       || icons_fill(&s, vol, name) == NULL))
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS,
                        OPENACC_RD, &r) != AFP_OK
            || next_second(sidecar) != 0 || chmod(sidecar, 0) != 0) {
        goto out;
    }

    if (euid == 0 && seteuid(owner) != 0) {
        ret = TEST_SKIP;
        goto out;
    }

    user = euid == 0;
    rc = icons_read(&s, r, 0, ICONS_RLEN, out, &got);
    locked = bytelock_ext(obj, r, 0, false, 0, 1, NULL);

    if (user && seteuid(0) != 0) {
        goto out;
    }

    user = 0;
    entry = dircache_search_by_name(vol, vol->v_root, name, strlen(name));
    dropped = entry != NULL && entry->dcache_rfork_buf == NULL;
    ret = rc == AFPERR_ACCESS && locked == AFPERR_MISC && dropped ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "unreadable sidecar: read %d, %u bytes, lock %d, "
                        "buffer gone %d\n", rc, got, locked, dropped);
    }

out:

    if (user && seteuid(0) != 0) {
        ret = -1;
    }

    if (r) {
        closefork(obj, r);
        r = 0;
    }

    (void)chmod(sidecar, 0600);

    if (euid == 0
            && icons_give(vol, path, sidecar, root.st_uid, root.st_gid) != 0) {
        ret = -1;
    }

    icons_done(&s, vol, r, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief A file with two names opens its resource fork with its sidecar
 *        under each, its buffer in the cache filled through a fork since
 *        closed
 *
 * @returns 0, 1 when an open opened fewer files, -1 when the setup failed,
 *          TEST_SKIP without the fault injector, where the sidecar is no
 *          file of its own or on a volume served as version 2
 */
int utest_hard_link_takes_base_open(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_clink";
    static char other[] = "icons_clink_other";
    char path[MAXPATHLEN + 1];
    char link_path[MAXPATHLEN + 1];
    struct icons_dsi s;
    int opens[2] = { -1, -1 };
    int ret = -1;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (icons_dsi_open(&s, obj) != 0) {
        return -1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(link_path, sizeof(link_path), "%s/%s", vol->v_path, other);

    /* the link first: the entry's stat then counts both names */
    if (createfile(&s.obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || link(path, link_path) != 0
            || icons_fill(&s, vol, name) == NULL) {
        goto out;
    }

    opens[0] = icons_open_calls(obj, vol, name);
    opens[1] = icons_open_calls(obj, vol, other);
    ret = opens[0] == 2 && opens[1] == 2 ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "hard link: opens %d and %d, expected 2\n", opens[0],
                opens[1]);
    }

out:
    delete (&s.obj, vol->v_vid, DIRDID_ROOT, other);
    icons_done(&s, vol, 0, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief The root Icon file of a volume serving a built-in icon opens its
 *        resource fork with its sidecar, its buffer in the cache filled
 *        through a fork since closed
 *
 * @returns 0, 1 when the open opened fewer files, -1 when the setup failed,
 *          TEST_SKIP without the fault injector, where the sidecar is no
 *          file of its own or on a volume served as version 2
 */
int utest_root_icon_takes_base_open(AFPObj *obj, struct vol *vol)
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
    struct icons_dsi s;
    int opens;
    int ret;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    vol->v_icon_rfork = icon;
    vol->v_icon_rfork_len = sizeof(icon);
    opens = icons_open_calls(obj, vol, name);
    vol->v_icon_rfork = saved;
    vol->v_icon_rfork_len = saved_len;
    ret = opens == 2 ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "root Icon: opens %d, expected 2\n", opens);
    }

    icons_done(&s, vol, 0, name);
    return ret;
#endif /* HAVE_EAFD */
}

/*!
 * @brief Under strict locking a read-only open of a resource fork the cache
 *        holds opens its sidecar
 *
 * @returns 0, 1 when the open opened fewer files, -1 when the setup failed,
 *          TEST_SKIP without the fault injector, where the sidecar is no
 *          file of its own or on a volume served as version 2
 */
int utest_cached_rfork_strict_locking_takes_base_open(AFPObj *obj,
                                                      struct vol *vol)
{
#ifdef HAVE_EAFD
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#else
    static char name[] = "icons_cstrict";
    const uint32_t saved = obj->options.flags;
    struct icons_dsi s;
    int opens;
    int ret;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (icons_cached_file(&s, obj, vol, name) != 0) {
        return -1;
    }

    obj->options.flags |= OPTION_STRICT_LOCKING;
    opens = icons_open_calls(obj, vol, name);
    obj->options.flags = saved;
    ret = opens == 2 ? 0 : 1;

    if (ret != 0) {
        fprintf(stderr, "strict locking: opens %d, expected 2\n", opens);
    }

    icons_done(&s, vol, 0, name);
    return ret;
#endif /* HAVE_EAFD */
}
