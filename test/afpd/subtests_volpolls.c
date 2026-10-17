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

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#ifdef HAVE_POSIX_ACLS
#include <sys/acl.h>
#endif /* HAVE_POSIX_ACLS */

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/directory.h>
#include <atalk/dsi.h>
#include <atalk/globals.h>
#include <atalk/netatalk_conf.h>
#include <atalk/unicode.h>
#include <atalk/util.h>
#include <atalk/uuid.h>
#include <atalk/volume.h>

#include "acls.h"
#include "afpfunc_helpers.h"
#include "directory.h"
#include "faultinject.h"
#include "fork.h"
#include "subtests_volpolls.h"
#include "test.h"
#include "test_capabilities.h"
#include "volume.h"

static char reply[DSI_DATASIZ];
static size_t replylen;

/*!
 * @brief Send FPGetVolParms for a volume, leaving the reply in reply[] and
 *        replylen
 *
 * @param[in] obj     session
 * @param[in] vol     volume
 * @param[in] bitmap  volume parameters to return
 *
 * @returns the AFP result
 */
static int getvolparms(AFPObj *obj, const struct vol *vol, uint16_t bitmap)
{
    char req[6] = { 0 };
    uint16_t nbitmap = htons(bitmap);
    memcpy(req + 2, &vol->v_vid, sizeof(vol->v_vid));
    memcpy(req + 4, &nbitmap, sizeof(nbitmap));
    replylen = sizeof(reply);
    return afp_getvolparams(obj, req, sizeof(req), reply, &replylen);
}

/*!
 * @brief Send FPGetVolParms for a volume's attributes
 *
 * @param[in]  obj   session
 * @param[in]  vol   volume
 * @param[out] attr  volume attributes
 *
 * @returns the AFP result
 */
static int getvolattr(AFPObj *obj, const struct vol *vol, uint16_t *attr)
{
    int ret = getvolparms(obj, vol, 1 << VOLPBIT_ATTR);
    memcpy(attr, reply + 2, sizeof(*attr));
    *attr = ntohs(*attr);
    return ret;
}

/*!
 * @brief The root's modification time reads back to the nanosecond
 */
int utest_volpoll_stamp_nanoseconds(const struct vol *vol)
{
    struct timespec keep[2] = { { .tv_nsec = UTIME_OMIT } };
    struct timespec half[2] = { { .tv_nsec = UTIME_OMIT } };
    struct stat st;
    int ret;

    if (stat(vol->v_path, &st) != 0) {
        return -1;
    }

    keep[1] = atalk_stat_mtime_timespec(&st);
    half[1].tv_sec = st.st_mtime;
    half[1].tv_nsec = 500 * 1000 * 1000;

    if (utimensat(AT_FDCWD, vol->v_path, half, 0) != 0
            || stat(vol->v_path, &st) != 0) {
        ret = -1;
    } else {
        ret = atalk_stat_mtime_timespec(&st).tv_nsec == half[1].tv_nsec ? 0 : 1;
    }

    (void)utimensat(AT_FDCWD, vol->v_path, keep, 0);
    return ret;
}

/*!
 * @brief A volume poll reports a writable volume read-write and leaves its
 *        root's mtime
 */
int utest_volpoll_probe_keeps_mtime(AFPObj *obj, struct vol *vol)
{
    time_t old = time(NULL) - 60;
    struct utimbuf times = { .actime = old, .modtime = old };
    struct timespec stamp[2] = { { .tv_nsec = UTIME_OMIT } };
    struct stat st;
    uint16_t attr;
    int ret;

    /* the first poll of a volume can create its AppleDouble header */
    if (getvolattr(obj, vol, &attr) != AFP_OK
            || stat(vol->v_path, &st) != 0) {
        return -1;
    }

    stamp[1] = atalk_stat_mtime_timespec(&st);

    if (utime(vol->v_path, &times) != 0
            || getvolattr(obj, vol, &attr) != AFP_OK
            || stat(vol->v_path, &st) != 0) {
        ret = -1;
    } else {
        ret = st.st_mtime == old && !(attr & VOLPBIT_ATTR_RO) ? 0 : 1;
    }

    /* the stamp the session's last mark may have left */
    (void)utimensat(AT_FDCWD, vol->v_path, stamp, 0);
    return ret;
}

/*!
 * @brief A volume poll reports a root that refuses writes with EROFS
 *        read-only, and one that refuses only the caller read-write
 */
int utest_volpoll_probe_reports_ro(AFPObj *obj, struct vol *vol)
{
    uint16_t rofs;
    uint16_t denied;
    int ret;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    fault_inject_reset();
    fi.access_armed = 1;
    fi.access_errno = EROFS;

    /* control: the interposer fails this build's access() calls */
    if (access(vol->v_path, W_OK) == 0 || errno != EROFS) {
        fault_inject_reset();
        return TEST_SKIP;
    }

    /* an armed failure fires once */
    fi.access_fail_after = 0;
    ret = getvolattr(obj, vol, &rofs);
    fi.access_fail_after = 0;
    fi.access_errno = EACCES;

    if (ret == AFP_OK) {
        ret = getvolattr(obj, vol, &denied);
    }

    fault_inject_reset();

    if (ret != AFP_OK) {
        return -1;
    }

    return (rofs & VOLPBIT_ATTR_RO) && !(denied & VOLPBIT_ATTR_RO) ? 0 : 1;
}

/*!
 * @brief A volume poll reports a read-only mount read-only for a caller
 *        whose access() is refused
 */
int utest_volpoll_probe_mount_ro(AFPObj *obj, struct vol *vol)
{
    struct statvfs sv;
    uint16_t attr;
    int ret;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    fault_inject_reset();
    fi.statvfs_rdonly = 1;

    /* control: the interposer marks this build's statvfs() answers */
    if (statvfs(vol->v_path, &sv) != 0 || !(sv.f_flag & ST_RDONLY)) {
        fault_inject_reset();
        return TEST_SKIP;
    }

    fi.access_armed = 1;
    fi.access_errno = EACCES;
    ret = getvolattr(obj, vol, &attr);
    fault_inject_reset();

    if (ret != AFP_OK) {
        return -1;
    }

    return attr & VOLPBIT_ATTR_RO ? 0 : 1;
}

/*!
 * @brief Volume changes within a second write the root's mtime once, and a
 *        date a client sets on the root does not displace the mark
 */
int utest_volpoll_mark_once_a_second(AFPObj *obj, struct vol *vol)
{
    const int changes = 10;
    const struct timespec tick = { .tv_sec = 0, .tv_nsec = 20 * 1000 * 1000 };
    struct stat st;
    time_t start;
    uint32_t afpdate;
    int marks;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    fault_inject_reset();
    fi.utime_watch = 1;
    /* control: the interposer sees this build's utime() calls */
    (void)utime(vol->v_path, NULL);

    if (fi.utime_calls != 1) {
        fault_inject_reset();
        return TEST_SKIP;
    }

    if (next_second(vol->v_path) != 0) {
        fault_inject_reset();
        return -1;
    }

    /* the coarse clock may trail the touch's stamp by a tick */
    nanosleep(&tick, NULL);
    start = time(NULL);
    fi.utime_calls = 0;

    for (int i = 0; i < changes; i++) {
        setvoltime(obj, vol);
    }

    marks = fi.utime_calls;
    fault_inject_reset();

    if (stat(vol->v_path, &st) != 0) {
        return -1;
    }

    /* stamps that do not follow afpd's clock keep every change writing */
    if (marks == changes && st.st_mtime != start) {
        return TEST_SKIP;
    }

    if (marks < 1 || marks == changes) {
        return 1;
    }

    /* a client date on the root within the marked second */
    afpdate = AD_DATE_FROM_UNIX(start - 86400);

    if (setdirparms(obj, vol->v_vid, DIRDID_ROOT, "", 1 << DIRPBIT_MDATE,
                    &afpdate, sizeof(afpdate)) != AFP_OK
            || stat(vol->v_path, &st) != 0) {
        return -1;
    }

    return st.st_mtime >= start ? 0 : 1;
}

static int attentions;

/*! @brief Count the attentions a poll sends, in place of the session's */
static int count_attention(void *handle _U_, AFPUserBytes flags _U_)
{
    attentions++;
    return 1;
}

/*!
 * @brief A root stamp another session left between two marks reaches this
 *        session's client through its next poll
 */
int utest_volpoll_foreign_mark_announced(AFPObj *obj, struct vol *vol)
{
    const struct timespec tick = { .tv_sec = 0, .tv_nsec = 20 * 1000 * 1000 };
    int (*attention)(void *, AFPUserBytes) = obj->attention;
    int flags = obj->options.flags;
    struct utimbuf times;
    struct stat st;
    time_t now;
    int ret = -1;
    obj->options.flags |= OPTION_SERVERNOTIF;
    obj->attention = count_attention;
    attentions = 0;

    /* the poll reports one volume per call: the other volumes' changes first */
    while (pollvoltime(obj) == 1) {
        attentions = 0;
    }

    /* this session's own mark, then a fresh second for the next one */
    setvoltime(obj, vol);

    if (next_second(vol->v_path) == 0) {
        nanosleep(&tick, NULL);
        now = time(NULL);
        /* this session polled 100 s ago; another session marked 95 s ago */
        vol->v_mtime = now - 100;
        times.actime = times.modtime = now - 95;

        if (utime(vol->v_path, &times) == 0) {
            setvoltime(obj, vol);
            setvoltime(obj, vol);
            ret = pollvoltime(obj) == 1 && attentions == 1
                  && stat(vol->v_path, &st) == 0 && vol->v_mtime == st.st_mtime
                  ? 0 : 1;
        }
    }

    obj->attention = attention;
    obj->options.flags = flags;
    return ret;
}

/*!
 * @brief A root stamp another session left in a second this session marked
 *        too reaches this session's client through its next poll
 */
int utest_volpoll_foreign_mark_same_second(AFPObj *obj, struct vol *vol)
{
    const struct timespec tick = { .tv_sec = 0, .tv_nsec = 20 * 1000 * 1000 };
    const time_t deadline = time(NULL) + 5;
    int (*attention)(void *, AFPUserBytes) = obj->attention;
    int flags = obj->options.flags;
    struct timespec stamp[2] = { { .tv_nsec = UTIME_OMIT } };
    struct stat st;
    long own = 0;
    int ret = -1;
    obj->options.flags |= OPTION_SERVERNOTIF;
    obj->attention = count_attention;
    attentions = 0;

    /* the poll reports one volume per call: the other volumes' changes first */
    while (pollvoltime(obj) == 1) {
        attentions = 0;
    }

    /* this session's mark in a fresh second */
    if (next_second(vol->v_path) == 0) {
        nanosleep(&tick, NULL);
        setvoltime(obj, vol);
        ret = stat(vol->v_path, &st);
    }

    if (ret == 0) {
        stamp[1] = atalk_stat_mtime_timespec(&st);
        own = stamp[1].tv_nsec;
        /* another session's mark half a second away, in the same second */
        stamp[1].tv_nsec = (own + 500 * 1000 * 1000) % (1000 * 1000 * 1000);
        ret = utimensat(AT_FDCWD, vol->v_path, stamp, 0) == 0
              && stat(vol->v_path, &st) == 0 ? 0 : -1;
    }

    /* a filesystem that keeps whole seconds cannot tell the two apart */
    if (ret == 0 && atalk_stat_mtime_timespec(&st).tv_nsec == own) {
        ret = TEST_SKIP;
    }

    if (ret == 0) {
        while (time(NULL) <= stamp[1].tv_sec && time(NULL) < deadline) {
            nanosleep(&tick, NULL);
        }

        nanosleep(&tick, NULL);
        /* pollvoltime() reads a root once the session's date is 30 s old */
        vol->v_mtime -= 100;
        setvoltime(obj, vol);
        ret = pollvoltime(obj) == 1 && attentions == 1
              && stat(vol->v_path, &st) == 0 && vol->v_mtime == st.st_mtime
              ? 0 : 1;
    }

    obj->attention = attention;
    obj->options.flags = flags;
    return ret;
}

/*!
 * @brief A quota answer younger than ten seconds serves the next poll
 */
int utest_volpoll_quota_cached(AFPObj *obj, struct vol *vol)
{
#ifdef NO_QUOTA_SUPPORT
    return TEST_SKIP;
#else
    int flags = vol->v_flags;
    uint64_t cached = hton64((uint64_t)4096);
    time_t seeded = coarse_time() - 1;
    int ret = -1;
    vol->v_flags = (vol->v_flags & ~AFPVOL_GVSMASK) | AFPVOL_UQUOTA;
    vol->v_qfree = 4096;
    vol->v_qtime = seeded;

    if (getvolparms(obj, vol, 1 << VOLPBIT_XBFREE) != AFP_OK) {
        goto cleanup;
    }

    ret = 1;

    if ((vol->v_flags & AFPVOL_GVSMASK) != AFPVOL_UQUOTA
            || vol->v_qtime != seeded
            || memcmp(reply + 2, &cached, sizeof(cached)) != 0) {
        goto cleanup;
    }

    seeded = coarse_time() - 10;
    vol->v_qtime = seeded;

    if (getvolparms(obj, vol, 1 << VOLPBIT_XBFREE) != AFP_OK) {
        ret = -1;
        goto cleanup;
    }

    /* a fetch either switches the volume to statfs or restamps the answer */
    if ((vol->v_flags & AFPVOL_GVSMASK) != AFPVOL_UQUOTA
            || vol->v_qtime != seeded) {
        ret = 0;
    }

cleanup:
    vol->v_flags = flags;
    return ret;
#endif /* NO_QUOTA_SUPPORT */
}

/*!
 * @brief Checking an unchanged config does not look the user up
 */
int utest_volpoll_fast_path_no_passwd(AFPObj *obj)
{
    const uid_t unknown = 2147483646;
    uid_t uid = obj->uid;
    int ret;

    if (getpwuid(unknown) != NULL) {
        return TEST_SKIP;
    }

    /* load any change first, so the next load takes the fast path */
    if (load_afp_conf_vols(obj, LV_DEFAULT) != 0) {
        return -1;
    }

    obj->uid = unknown;
    ret = load_afp_conf_vols(obj, LV_DEFAULT);
    obj->uid = uid;
    return ret == 0 ? 0 : 1;
}

/*!
 * @brief Send FPGetSrvrParms, leaving the reply in reply[] and replylen
 *
 * @returns the AFP result
 */
static int getsrvrparms(AFPObj *obj)
{
    replylen = sizeof(reply);
    return afp_getsrvrparms(obj, NULL, 0, reply, &replylen);
}

/*!
 * @brief FPGetSrvrParms reads a root's ACL only when its mode bits leave
 *        out read or search
 */
int utest_volpoll_srvrparms_mode_first(AFPObj *obj, struct vol *vol)
{
#ifndef HAVE_POSIX_ACLS
    return TEST_SKIP;
#else
    const int mapped = OPTION_ACL2MACCESS | OPTION_ACL2MODE;
    uid_t uid = obj->uid;
    uid_t euid = obj->euid;
    struct stat st;
    acl_t acl;
    int denied;
    int ret;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)
            || !(obj->options.flags & mapped)) {
        return TEST_SKIP;
    }

    /* load any change first: a reload probes each root's ACL itself */
    if (load_afp_conf_vols(obj, LV_DEFAULT) != 0) {
        return -1;
    }

    fault_inject_reset();
    fi.acl_get_file_watch = 1;
    /* control: the interposer sees this build's acl_get_file() calls */
    acl = acl_get_file(vol->v_path, ACL_TYPE_ACCESS);

    if (acl) {
        acl_free(acl);
    }

    if (fi.acl_get_file_calls != 1) {
        fault_inject_reset();
        return TEST_SKIP;
    }

    if (stat(vol->v_path, &st) != 0 || chmod(vol->v_path, 0700) != 0) {
        fault_inject_reset();
        return -1;
    }

    /* a user neither root nor the owner, whom 0700 gives nothing, so the
     * ACL decides, and whom 0755 grants read and search */
    fi.acl_get_file_calls = 0;
    obj->uid = st.st_uid + 1;
    obj->euid = st.st_uid + 1;
    ret = getsrvrparms(obj);
    denied = fi.acl_get_file_calls;
    fi.acl_get_file_calls = 0;

    /* the other volumes' roots read alike in both polls */
    if (ret == AFP_OK && chmod(vol->v_path, 0755) == 0
            && getsrvrparms(obj) == AFP_OK) {
        ret = denied > fi.acl_get_file_calls ? 0 : 1;
    } else {
        ret = -1;
    }

    obj->uid = uid;
    obj->euid = euid;
    fault_inject_reset();
    (void)chmod(vol->v_path, st.st_mode & 07777);
    return ret;
#endif /* HAVE_POSIX_ACLS */
}

/*!
 * @brief A volume name the Mac charset cannot encode fails the request
 */
int utest_volpoll_name_unencodable(AFPObj *obj, struct vol *vol)
{
    static ucs2_t name[] = { 0x4E2D, 0 };
    ucs2_t *macname = vol->v_macname;
    int ret;
    vol->v_macname = name;
    ret = getvolparms(obj, vol, 1 << VOLPBIT_NAME);
    vol->v_macname = macname;
    return ret == AFPERR_MISC ? 0 : 1;
}

/*!
 * @brief FPGetSrvrParms stops listing volumes at its reply cap
 */
int utest_volpoll_srvrparms_cap(AFPObj *obj)
{
    static ucs2_t name[AFPVOL_U8MNAMELEN + 1];
    static struct vol fake[20];
    const size_t fakes = sizeof(fake) / sizeof(fake[0]);
    time_t volfile_mtime = obj->options.volfile.mtime;
    struct vol *last;
    int ret;

    for (size_t i = 0; i < AFPVOL_U8MNAMELEN; i++) {
        name[i] = 'v';
    }

    for (size_t i = 0; i < fakes; i++) {
        fake[i].v_flags = AFPVOL_NOSTAT;
        fake[i].v_u8mname = name;
        fake[i].v_next = i + 1 < fakes ? &fake[i + 1] : NULL;
    }

    last = getvolumes();

    if (last == NULL) {
        return -1;
    }

    while (last->v_next) {
        last = last->v_next;
    }

    /* the config reads as unchanged while the fakes are listed: the
     * request's reload would free the volumes the config lacks */
    obj->options.volfile.mtime = time(NULL) + 86400;
    last->v_next = fake;
    ret = getsrvrparms(obj);
    last->v_next = NULL;
    obj->options.volfile.mtime = volfile_mtime;
    /* full: one more fake's entry would pass the 4600-byte cap */
    return ret == AFP_OK && replylen <= 4600
           && replylen + 3 + AFPVOL_U8MNAMELEN > 4600 ? 0 : 1;
}

/*!
 * @brief FPGetExtAttr, FPSetExtAttr and FPRemoveExtAttr fail an EA name
 *        that is not UTF-8
 */
int utest_volpoll_ea_name_not_utf8(AFPObj *obj, struct vol *vol)
{
    /* no UTF-8 sequence holds these bytes */
    const char *bad = "\xff\xfe";
    size_t len = sizeof(reply);

    if (getextattr(obj, vol->v_vid, DIRDID_ROOT, "", bad, 0, reply, &len)
            != AFPERR_PARAM) {
        return 1;
    }

    if (setextattr(obj, vol->v_vid, DIRDID_ROOT, "", bad, "", 0)
            != AFPERR_PARAM) {
        return 1;
    }

    return remextattr(obj, vol->v_vid, DIRDID_ROOT, "", bad) == AFPERR_PARAM
           ? 0 : 1;
}

/*!
 * @brief FPOpenVol asking for the volume id, with the name's raw bytes
 *
 * @returns the AFP result
 */
static int openvol_raw(AFPObj *obj, const char *name, uint8_t namelen)
{
    char req[2 + 2 + 1 + 255 + 1] = { 0 };
    uint16_t bitmap = htons(1 << VOLPBIT_VID);
    size_t len = sizeof(reply);
    memcpy(req + 2, &bitmap, sizeof(bitmap));
    req[4] = namelen;
    memcpy(req + 5, name, namelen);
    return afp_openvol(obj, req, sizeof(req), reply, &len);
}

/*!
 * @brief FPOpenVol fails a volume name that is not UTF-8
 */
int utest_volpoll_volname_not_utf8(AFPObj *obj)
{
    /* two bytes no UTF-8 sequence holds */
    return openvol_raw(obj, "\xff\xfe", 2) == AFPERR_PARAM ? 0 : 1;
}

/*!
 * @brief FPOpenVol converts the name into the start of its buffer, whatever
 *        the buffer holds
 */
int utest_volpoll_volname_from_start(AFPObj *obj)
{
    char name[AFPVOL_U8MNAMELEN];
    memset(name, 'v', sizeof(name));
    /* a '+' just before the terminator; newtmp follows oldtmp in memory */
    memset(obj->oldtmp, 'a', sizeof(obj->oldtmp) - 1);
    obj->oldtmp[sizeof(obj->oldtmp) - 2] = '+';
    obj->oldtmp[sizeof(obj->oldtmp) - 1] = '\0';
    memset(obj->newtmp, 'n', 8);
    (void)openvol_raw(obj, name, sizeof(name));
    return memcmp(obj->newtmp, "nnnnnnnn", 8) == 0 ? 0 : 1;
}

/*!
 * @brief Judge a mark test's request by its result and the volume's mark
 *
 * @returns -1 if the request failed, 0 if it marked the volume, else 1
 */
static int marked(const struct vol *vol, int ret)
{
    if (ret != AFP_OK) {
        return -1;
    }

    return vol->v_root_marked != 0 ? 0 : 1;
}

/*!
 * @brief A resource fork opened and closed unwritten leaves the volume
 *        unmarked
 */
int utest_volpoll_unwritten_unmarked(AFPObj *obj, struct vol *vol)
{
    const char *name = "volpoll-unwritten";
    const uint16_t rw = OPENACC_RD | OPENACC_WR;
    uint16_t refnum;
    int ret = -1;

    if (createfile(obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK) {
        return -1;
    }

    vol->v_root_marked = 0;

    if (openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_RSCS, rw,
                 &refnum) == AFP_OK && closefork(obj, refnum) == AFP_OK) {
        ret = vol->v_root_marked != 0 ? 1 : 0;
    }

    (void)delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

/*!
 * @brief FPSetForkParms marks the volume changed
 */
int utest_volpoll_mark_setforkparams(AFPObj *obj, struct vol *vol)
{
    const char *name = "volpoll-setforkparams";
    uint16_t refnum;
    int ret;

    if (createfile(obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                        OPENACC_RD | OPENACC_WR, &refnum) != AFP_OK) {
        return -1;
    }

    vol->v_root_marked = 0;
    ret = marked(vol, setforkparams(obj, refnum, 1));
    (void)closefork(obj, refnum);
    (void)delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

/*!
 * @brief FPExchangeFiles marks the volume changed
 */
int utest_volpoll_mark_exchangefiles(AFPObj *obj, struct vol *vol)
{
    const char *a = "volpoll-a";
    const char *b = "volpoll-b";
    int ret;

    if (createfile(obj, vol->v_vid, DIRDID_ROOT, a) != AFP_OK
            || createfile(obj, vol->v_vid, DIRDID_ROOT, b) != AFP_OK) {
        return -1;
    }

    vol->v_root_marked = 0;
    ret = marked(vol, exchangefiles(obj, vol->v_vid, DIRDID_ROOT, DIRDID_ROOT,
                                    a, b));
    (void)delete (obj, vol->v_vid, DIRDID_ROOT, a);
    (void)delete (obj, vol->v_vid, DIRDID_ROOT, b);
    return ret;
}

/*!
 * @brief FPSetExtAttr marks the volume changed
 */
int utest_volpoll_mark_setextattr(AFPObj *obj, struct vol *vol)
{
    int ret;
    vol->v_root_marked = 0;
    ret = marked(vol, setextattr(obj, vol->v_vid, DIRDID_ROOT, "", "volpoll",
                                 "1", 1));
    (void)remextattr(obj, vol->v_vid, DIRDID_ROOT, "", "volpoll");
    return ret;
}

/*!
 * @brief FPRemoveExtAttr marks the volume changed
 */
int utest_volpoll_mark_remextattr(AFPObj *obj, struct vol *vol)
{
    if (setextattr(obj, vol->v_vid, DIRDID_ROOT, "", "volpoll", "1", 1)
            != AFP_OK) {
        return -1;
    }

    vol->v_root_marked = 0;
    return marked(vol, remextattr(obj, vol->v_vid, DIRDID_ROOT, "", "volpoll"));
}

/*!
 * @brief Send FPSetVolParms setting a volume's backup date to now
 *
 * @returns the AFP result
 */
static int setbackupdate(AFPObj *obj, const struct vol *vol)
{
    char req[10] = { 0 };
    uint16_t bitmap = htons(1 << VOLPBIT_BDATE);
    uint32_t date = AD_DATE_FROM_UNIX(time(NULL));
    size_t len = sizeof(reply);
    memcpy(req + 2, &vol->v_vid, sizeof(vol->v_vid));
    memcpy(req + 4, &bitmap, sizeof(bitmap));
    memcpy(req + 6, &date, sizeof(date));
    return afp_setvolparams(obj, req, sizeof(req), reply, &len);
}

/*!
 * @brief FPSetVolParms marks the volume changed
 */
int utest_volpoll_mark_setvolparms(AFPObj *obj, struct vol *vol)
{
    vol->v_root_marked = 0;
    return marked(vol, setbackupdate(obj, vol));
}

/*!
 * @brief FPSetVolParms fails, and marks nothing, when the backup date does
 *        not reach the root's metadata
 */
int utest_volpoll_setvolparms_unwritten(AFPObj *obj, struct vol *vol)
{
    int ret;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    fault_inject_reset();
    /* the write of a directory's metadata changes into the directory */
    fi.fchdir_armed = 1;
    fi.fchdir_errno = EIO;
    vol->v_root_marked = 0;
    ret = setbackupdate(obj, vol);
    fault_inject_reset();
    return ret == AFPERR_MISC && vol->v_root_marked == 0 ? 0 : 1;
}

/*!
 * @brief FPSetACL marks the volume changed
 */
int utest_volpoll_mark_setacl(AFPObj *obj, struct vol *vol)
{
#ifndef HAVE_ACLS
    return TEST_SKIP;
#else

    /* creatvol() clears the flag where the root's ACL cannot be read */
    if (!(vol->v_flags & AFPVOL_ACLS)) {
        return TEST_SKIP;
    }

    vol->v_root_marked = 0;
    return marked(vol, setacl(obj, vol->v_vid, DIRDID_ROOT, "",
                              kFileSec_REMOVEACL, NULL, 0));
#endif /* HAVE_ACLS */
}

/*!
 * @brief FPSetACL sets the ACL of a request that carries the group UUID
 *        and no owner UUID
 */
int utest_volpoll_setacl_group_uuid(AFPObj *obj, struct vol *vol)
{
#ifndef HAVE_ACLS
    return TEST_SKIP;
#else
    /* the group UUID, then an ACL header with no ACEs */
    unsigned char body[UUID_BINSIZE + sizeof(darwin_acl_header_t)] = { 0 };
    struct stat st;
    int ret;

    /* creatvol() clears the flag where the root's ACL cannot be read */
    if (!(vol->v_flags & AFPVOL_ACLS)) {
        return TEST_SKIP;
    }

    if (setacl(obj, vol->v_vid, DIRDID_ROOT, "", kFileSec_REMOVEACL, NULL, 0)
            != AFP_OK || stat(vol->v_path, &st) != 0) {
        return -1;
    }

    localuuid_from_id(body, UUID_GROUP, getegid());
    ret = setacl(obj, vol->v_vid, DIRDID_ROOT, "",
                 kFileSec_GRPUUID | kFileSec_ACL, body, sizeof(body));
    (void)setacl(obj, vol->v_vid, DIRDID_ROOT, "", kFileSec_REMOVEACL, NULL, 0);
    /* the ACL's base entries replaced the root's mode bits */
    (void)chmod(vol->v_path, st.st_mode & 07777);
    return ret == AFP_OK ? 0 : 1;
#endif /* HAVE_ACLS */
}

/*!
 * @brief FPSetExtAttr, FPRemoveExtAttr and FPSetACL refuse a read-only
 *        volume with kFPVolLocked
 */
int utest_volpoll_readonly_refused(AFPObj *obj, struct vol *vol)
{
    int flags = vol->v_flags;
    int ret = 1;
    vol->v_flags |= AFPVOL_RO;

    if (setextattr(obj, vol->v_vid, DIRDID_ROOT, "", "volpoll", "1", 1)
            == AFPERR_VLOCK
            && remextattr(obj, vol->v_vid, DIRDID_ROOT, "", "volpoll")
            == AFPERR_VLOCK) {
        ret = 0;
    }

#ifdef HAVE_ACLS

    if (ret == 0 && setacl(obj, vol->v_vid, DIRDID_ROOT, "",
                           kFileSec_REMOVEACL, NULL, 0) != AFPERR_VLOCK) {
        ret = 1;
    }

#endif /* HAVE_ACLS */
    vol->v_flags = flags;
    return ret;
}
