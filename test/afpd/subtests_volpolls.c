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
 * @brief Component tests of the volume polls and the volume change marks
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_POSIX_ACLS
#include <sys/acl.h>
#endif /* HAVE_POSIX_ACLS */

#include <atalk/acl.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/directory.h>
#include <atalk/dsi.h>
#include <atalk/ea.h>
#include <atalk/globals.h>
#include <atalk/netatalk_conf.h>
#include <atalk/unicode.h>
#include <atalk/util.h>
#include <atalk/uuid.h>
#include <atalk/vfs.h>
#include <atalk/volume.h>

#ifdef WITH_SPOTLIGHT
#include <talloc.h>

#include <atalk/dalloc.h>
#include <atalk/spotlight.h>

#include "etc/spotlight/spotlight_private.h"
#endif /* WITH_SPOTLIGHT */

#include "acls.h"
#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "extattrs.h"
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
 * @brief The root's access and modification times read back to the
 *        nanosecond through the stat helpers, as the stat's own members
 *        hold them
 *
 * @returns 0, 1 where the build has no stat member check or a helper drops
 *          the fraction, or TEST_SKIP on a filesystem that keeps whole
 *          seconds
 */
int utest_volpoll_stamp_nanoseconds(const struct vol *vol)
{
#if !defined(HAVE_STRUCT_STAT_ST_MTIM) \
    && !defined(HAVE_STRUCT_STAT_ST_MTIMESPEC)
    return 1;
#else
    struct timespec keep[2];
    struct timespec half[2];
    struct stat st;
    long raw;
    int ret;

    if (stat(vol->v_path, &st) != 0) {
        return -1;
    }

    keep[0] = atalk_stat_atime_timespec(&st);
    keep[1] = atalk_stat_mtime_timespec(&st);
    half[0].tv_sec = st.st_atime;
    half[1].tv_sec = st.st_mtime;
    half[0].tv_nsec = half[1].tv_nsec = 500 * 1000 * 1000;

    if (utimensat(AT_FDCWD, vol->v_path, half, 0) != 0
            || stat(vol->v_path, &st) != 0) {
        ret = -1;
    } else {
#ifdef HAVE_STRUCT_STAT_ST_MTIM
        raw = st.st_mtim.tv_nsec;
#else
        raw = st.st_mtimespec.tv_nsec;
#endif

        if (raw == 0) {
            ret = TEST_SKIP;
        } else {
            ret = atalk_stat_mtime_timespec(&st).tv_nsec == raw
                  && atalk_stat_atime_timespec(&st).tv_nsec == raw ? 0 : 1;
        }
    }

    (void)utimensat(AT_FDCWD, vol->v_path, keep, 0);
    return ret;
#endif
}

/*!
 * @brief Where the stat carries a creation time, the build knows it
 *
 * @returns 0, 1 on FreeBSD, NetBSD and macOS without the member's macro,
 *          or TEST_SKIP elsewhere
 */
int utest_volpoll_birthtime_member(void)
{
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
#ifdef HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC
    return 0;
#else
    return 1;
#endif
#else
    return TEST_SKIP;
#endif
}

#if defined(WITH_SPOTLIGHT) && defined(HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC)
/*!
 * @brief Ask add_filemeta() for the creation date of a stat with a birth time
 *
 * @param[in] birth  the stat's birth time
 * @param[in] date   the date the answer must carry, or NULL for nil
 *
 * @returns 0, 1 for another answer, or -1 when the fixture failed
 */
static int spotlight_birth_answer(const struct timespec *birth,
                                  const struct timespec *date)
{
    TALLOC_CTX *tmp = talloc_new(NULL);
    sl_array_t *reqinfo = talloc_zero(tmp, sl_array_t);
    sl_array_t *fm_array = talloc_zero(tmp, sl_array_t);
    const sl_array_t *meta;
    const void *answer = NULL;
    const sl_time_t *t;
    struct stat st;
    char *attr;
    int ret = -1;
    memset(&st, 0, sizeof(st));

    if (reqinfo != NULL && fm_array != NULL
            && (attr = dalloc_strdup(reqinfo, "kMDItemContentCreationDate"))
            != NULL) {
        dalloc_add(reqinfo, attr, char *);
        st.st_birthtimespec = *birth;

        /* the reply holds one metadata array, which holds the one answer */
        if (add_filemeta(reqinfo, fm_array, "volpoll-file", &st)
                && talloc_array_length(fm_array->dd_talloc_array) == 1) {
            meta = fm_array->dd_talloc_array[0];
            answer = talloc_array_length(meta->dd_talloc_array) == 1
                     ? meta->dd_talloc_array[0] : NULL;
        }
    }

    if (answer != NULL && date == NULL) {
        ret = talloc_check_name(answer, "sl_nil_t") != NULL ? 0 : 1;
    } else if (answer != NULL) {
        t = talloc_check_name(answer, "sl_time_t");
        ret = t != NULL && t->tv_sec == date->tv_sec ? 0 : 1;
    }

    talloc_free(tmp);
    return ret;
}
#endif

/*!
 * @brief Spotlight answers nil for the creation date of a file whose
 *        filesystem keeps no birth time, and the birth time of one that does
 *
 * @returns 0, 1 for a wrong answer, -1 when the fixture failed, or
 *          TEST_SKIP without Spotlight or the stat's creation time member
 */
int utest_volpoll_spotlight_no_birthtime(void)
{
#if defined(WITH_SPOTLIGHT) && defined(HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC)
    /* what FreeBSD's stat holds for no birth time, NetBSD's and macOS's,
     * and a birth time */
    const struct timespec none_freebsd = { .tv_sec = -1 };
    const struct timespec none = { .tv_sec = 0 };
    const struct timespec born = { .tv_sec = 1000000000 };
    int ret = spotlight_birth_answer(&none_freebsd, NULL);

    if (ret == 0) {
        ret = spotlight_birth_answer(&none, NULL);
    }

    if (ret == 0) {
        ret = spotlight_birth_answer(&born, &born);
    }

    return ret;
#else
    return TEST_SKIP;
#endif
}

/*!
 * @brief A volume poll reports a writable volume read-write and leaves its
 *        root's mtime
 */
int utest_volpoll_probe_keeps_mtime(AFPObj *obj, struct vol *vol)
{
    const time_t old = time(NULL) - 60;
    const struct timespec times[2] = { { .tv_sec = old }, { .tv_sec = old } };
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

    if (utimensat(AT_FDCWD, vol->v_path, times, 0) != 0
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
 * @brief Volume changes within a second write the root's mtime once, and
 *        the write moves it forward
 */
int utest_volpoll_mark_once_a_second(AFPObj *obj, struct vol *vol)
{
    const int changes = 10;
    struct timespec old[2] = { { 0 } };
    struct stat st;
    time_t start;
    int marks;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    fault_inject_reset();
    fi.utime_watch = 1;
    /* control: the interposer sees this build's utimensat() calls */
    (void)utimensat(AT_FDCWD, vol->v_path, NULL, 0);

    if (fi.utime_calls != 1) {
        fault_inject_reset();
        return TEST_SKIP;
    }

    if (next_second(vol->v_path) != 0) {
        fault_inject_reset();
        return -1;
    }

    /* a root a minute behind, which only a mark can bring forward */
    start = time(NULL);
    old[0].tv_sec = old[1].tv_sec = start - 60;

    if (utimensat(AT_FDCWD, vol->v_path, old, 0) != 0) {
        fault_inject_reset();
        return -1;
    }

    vol->v_root_marked = 0;
    fi.utime_calls = 0;

    for (int i = 0; i < changes; i++) {
        setvoltime(obj, vol);
    }

    marks = fi.utime_calls;
    fault_inject_reset();
    /* the old stamp reads as another session's */
    vol->v_root_foreign = 0;

    if (stat(vol->v_path, &st) != 0) {
        return -1;
    }

    /* a second that turns during the calls costs one more write */
    return marks >= 1 && marks <= 2 && st.st_mtime >= start ? 0 : 1;
}

/*!
 * @brief A date a client sets on the volume root does not displace the
 *        root's mark
 */
int utest_volpoll_root_date_kept(AFPObj *obj, struct vol *vol)
{
    struct stat st;
    time_t stamped;
    uint32_t afpdate;

    if (next_second(vol->v_path) != 0 || stat(vol->v_path, &st) != 0) {
        return -1;
    }

    stamped = st.st_mtime;
    /* a closed gate, so no mark writes the root after the client's date */
    vol->v_root_marked = time(NULL);
    /* a client date a day back */
    afpdate = AD_DATE_FROM_UNIX(stamped - 86400);

    if (setdirparms(obj, vol->v_vid, DIRDID_ROOT, "", 1 << DIRPBIT_MDATE,
                    &afpdate, sizeof(afpdate)) != AFP_OK
            || stat(vol->v_path, &st) != 0) {
        return -1;
    }

    return st.st_mtime >= stamped ? 0 : 1;
}

static int attentions;

/*! @brief Count the attentions a poll sends, in place of the session's */
static int count_attention(void *handle _U_, AFPUserBytes flags _U_)
{
    attentions++;
    return 1;
}

/*! @brief What attentions_begin() saves of the session */
struct attention_count {
    int (*attention)(void *, AFPUserBytes);
    void *handle;
    int flags;
    int proto;
};

/*!
 * @brief Count the attentions the session sends as a DSI session with
 *        server notifications, once the other volumes' pending changes are
 *        polled away
 *
 * @param[in,out] obj    session, its handler, handle, flags and protocol
 *                       saved in saved
 * @param[out]    saved  what attentions_end() restores
 */
static void attentions_begin(AFPObj *obj, struct attention_count *saved)
{
    saved->attention = obj->attention;
    saved->handle = obj->handle;
    saved->flags = obj->options.flags;
    saved->proto = obj->proto;
    obj->options.flags |= OPTION_SERVERNOTIF;
    obj->proto = AFPPROTO_DSI;
    obj->attention = count_attention;
    attentions = 0;

    /* the poll reports one volume per call */
    while (pollvoltime(obj) == 1) {
        attentions = 0;
    }
}

/*! @brief Give the session what attentions_begin() changed back */
static void attentions_end(AFPObj *obj, const struct attention_count *saved)
{
    obj->attention = saved->attention;
    obj->handle = saved->handle;
    obj->options.flags = saved->flags;
    obj->proto = saved->proto;
}

/*!
 * @brief The free-space attention a mark sends goes to clients from AFP 2.2
 *        to 3.1
 */
int utest_volpoll_attention_versions(AFPObj *obj, struct vol *vol)
{
    const int versions[] = { 21, 22, 32 };
    const int version = obj->afp_version;
    struct attention_count saved;
    int ret = 0;
    attentions_begin(obj, &saved);

    for (size_t i = 0; i < sizeof(versions) / sizeof(versions[0]); i++) {
        obj->afp_version = versions[i];
        vol->v_root_marked = 0;
        attentions = 0;
        setvoltime(obj, vol);

        if (attentions != (versions[i] == 22)) {
            ret = 1;
        }
    }

    obj->afp_version = version;
    attentions_end(obj, &saved);
    return ret;
}

/*!
 * @brief A root stamp another session left between two marks reaches this
 *        session's client through its next poll
 */
int utest_volpoll_foreign_mark_announced(AFPObj *obj, struct vol *vol)
{
    struct attention_count saved;
    struct timespec times[2] = { { 0 } };
    struct stat st;
    time_t now;
    int ret = -1;
    attentions_begin(obj, &saved);
    /* this session's own mark records the root's stamp; its compare can read
     * an older stamp of the session's own as another session's */
    vol->v_root_marked = 0;
    setvoltime(obj, vol);
    vol->v_root_foreign = 0;
    now = time(NULL);
    /* this session polled 100 s ago; another session marked 95 s ago */
    vol->v_mtime = now - 100;
    times[0].tv_sec = times[1].tv_sec = now - 95;

    /* two marks in a second the gate has not seen */
    if (utimensat(AT_FDCWD, vol->v_path, times, 0) == 0) {
        vol->v_root_marked = 0;
        setvoltime(obj, vol);
        setvoltime(obj, vol);
        ret = pollvoltime(obj) == 1 && attentions == 1
              && stat(vol->v_path, &st) == 0 && vol->v_mtime == st.st_mtime
              ? 0 : 1;
    }

    attentions_end(obj, &saved);
    return ret;
}

/*!
 * @brief A stamp the session's poll announced is not announced again after
 *        the session's next mark
 */
int utest_volpoll_announced_mark_known(AFPObj *obj, struct vol *vol)
{
    struct attention_count saved;
    struct timespec times[2] = { { 0 } };
    time_t now;
    int ret = -1;
    attentions_begin(obj, &saved);
    /* this session's mark, then another session's, which the poll announces */
    vol->v_root_marked = 0;
    setvoltime(obj, vol);
    vol->v_root_foreign = 0;
    now = time(NULL);
    vol->v_mtime = now - 100;
    times[0].tv_sec = times[1].tv_sec = now - 95;

    if (utimensat(AT_FDCWD, vol->v_path, times, 0) == 0
            && pollvoltime(obj) == 1 && attentions == 1) {
        /* the next mark writes over a stamp the client was told of */
        attentions = 0;
        vol->v_root_marked = 0;
        setvoltime(obj, vol);
        ret = pollvoltime(obj) == 0 && attentions == 0 ? 0 : 1;
    }

    attentions_end(obj, &saved);
    return ret;
}

/*! @brief Refuse an attention, as dsi_attention() does on a full socket */
static int refuse_attention(void *handle _U_, AFPUserBytes flags _U_)
{
    attentions++;
    return -1;
}

/*!
 * @brief Another session's stamp whose attention the socket refuses goes
 *        out at the session's next poll
 */
int utest_volpoll_refused_attention_kept(AFPObj *obj, struct vol *vol)
{
    struct attention_count saved;
    struct timespec times[2] = { { 0 } };
    time_t now;
    int ret = -1;
    attentions_begin(obj, &saved);
    /* this session's mark, then another session's 95 s ago */
    vol->v_root_marked = 0;
    setvoltime(obj, vol);
    vol->v_root_foreign = 0;
    now = time(NULL);
    vol->v_mtime = now - 100;
    times[0].tv_sec = times[1].tv_sec = now - 95;

    if (utimensat(AT_FDCWD, vol->v_path, times, 0) == 0) {
        obj->attention = refuse_attention;
        (void)pollvoltime(obj);
        obj->attention = count_attention;
        ret = pollvoltime(obj) == 1 && attentions == 2 ? 0 : 1;
    }

    attentions_end(obj, &saved);
    return ret;
}

/*!
 * @brief A session's own mark keeps its date at the stamp the mark left, so
 *        its poll stays quiet, and a mark that cannot write the root leaves
 *        the date where the root has it
 */
int utest_volpoll_own_mark_quiet(AFPObj *obj, struct vol *vol)
{
    const struct timespec tick = { .tv_sec = 0, .tv_nsec = 20 * 1000 * 1000 };
    struct attention_count saved;
    struct stat st;
    time_t marked;
    int ret = 1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    /* control: the interposer sees this build's root writes */
    fault_inject_reset();
    fi.utime_watch = 1;
    (void)utimensat(AT_FDCWD, vol->v_path, NULL, 0);

    if (fi.utime_calls != 1) {
        fault_inject_reset();
        return TEST_SKIP;
    }

    fault_inject_reset();
    /* the session's poll takes what the root holds */
    vol->v_mtime = time(NULL) - 100;
    attentions_begin(obj, &saved);
    /* two marks of the session's own, in two seconds, and nothing else;
     * the second stamps the root 300 ms behind the clock */
    setvoltime(obj, vol);
    marked = time(NULL);

    while (time(NULL) <= marked) {
        nanosleep(&tick, NULL);
    }

    nanosleep(&tick, NULL);
    fi.utime_lag_ms = 300;
    setvoltime(obj, vol);
    fault_inject_reset();

    if (stat(vol->v_path, &st) == 0 && vol->v_mtime == st.st_mtime) {
        ret = 0;
    }

    /* a mark in a later second whose write fails */
    if (ret == 0) {
        marked = time(NULL);

        while (time(NULL) <= marked) {
            nanosleep(&tick, NULL);
        }

        nanosleep(&tick, NULL);
        fault_inject_reset();
        fi.utime.armed = 1;
        fi.utime.errnum = EACCES;
        setvoltime(obj, vol);

        if (fi.utime.calls == 0 || stat(vol->v_path, &st) != 0
                || vol->v_mtime != st.st_mtime) {
            ret = 1;
        }

        fault_inject_reset();
    }

    attentions_end(obj, &saved);
    return ret;
}

/*!
 * @brief A root stamp another session left earlier in the second of this
 *        session's next mark reaches this session's client through its poll
 */
int utest_volpoll_foreign_mark_this_second(AFPObj *obj, struct vol *vol)
{
    struct attention_count saved;
    int ret = -1;
    /* the session's poll takes what the root holds */
    vol->v_mtime = time(NULL) - 100;
    attentions_begin(obj, &saved);
    /* this session's own mark */
    setvoltime(obj, vol);

    /* another session's mark opens a new second; this session polled 100 s
     * ago */
    if (next_second(vol->v_path) == 0) {
        vol->v_mtime = time(NULL) - 100;
        setvoltime(obj, vol);
        ret = pollvoltime(obj) == 1 && attentions == 1 ? 0 : 1;
    }

    attentions_end(obj, &saved);
    return ret;
}

/*!
 * @brief A root stamp another session left in the second of the stamp this
 *        session's poll took reaches this session's client through its next
 *        poll
 */
int utest_volpoll_foreign_mark_seen_second(AFPObj *obj, struct vol *vol)
{
    struct attention_count saved;
    struct timespec times[2] = { { 0 } };
    struct stat st;
    int ret = -1;
    /* the session's poll takes what the root holds */
    vol->v_mtime = time(NULL) - 100;
    attentions_begin(obj, &saved);

    /* a fresh second: the poll writes a change an earlier mark left due */
    if (next_second(vol->v_path) == 0) {
        vol->v_mtime = time(NULL) - 100;
        (void)pollvoltime(obj);
        attentions = 0;
        /* a change 100 s ago, which the poll announces */
        times[0].tv_sec = times[1].tv_sec = time(NULL) - 100;
        times[0].tv_nsec = times[1].tv_nsec = 200 * 1000 * 1000;
        vol->v_mtime = times[1].tv_sec - 1;

        if (utimensat(AT_FDCWD, vol->v_path, times, 0) == 0
                && stat(vol->v_path, &st) == 0) {
            /* a filesystem that keeps whole seconds cannot hold the two
             * stamps apart */
            if (atalk_stat_mtime_timespec(&st).tv_nsec == 0) {
                ret = TEST_SKIP;
            } else if (pollvoltime(obj) == 1 && attentions == 1) {
                /* another session's change later in that second */
                attentions = 0;
                times[0].tv_nsec = times[1].tv_nsec = 600 * 1000 * 1000;
                ret = utimensat(AT_FDCWD, vol->v_path, times, 0) == 0
                      && pollvoltime(obj) == 1 && attentions == 1 ? 0 : 1;
            }
        }
    }

    attentions_end(obj, &saved);
    return ret;
}

/*!
 * @brief Files this session creates in the volume root are its own changes,
 *        so its poll stays quiet after them
 */
int utest_volpoll_root_entry_own(AFPObj *obj, struct vol *vol)
{
    const struct timespec tick = { .tv_sec = 0, .tv_nsec = 20 * 1000 * 1000 };
    const char *first = "volpoll-entry-1";
    const char *second = "volpoll-entry-2";
    struct attention_count saved;
    struct timespec stamp;
    struct stat st;
    time_t polled;
    time_t marked;
    int ret = -1;
    /* the session's poll takes what the root holds */
    vol->v_mtime = time(NULL) - 100;
    attentions_begin(obj, &saved);
    /* in a fresh second the first create writes the root, and the second
     * falls within the written second */
    polled = time(NULL);

    while (time(NULL) <= polled) {
        nanosleep(&tick, NULL);
    }

    nanosleep(&tick, NULL);

    if (createfile(obj, vol->v_vid, DIRDID_ROOT, first) == AFP_OK
            && stat(vol->v_path, &st) == 0) {
        stamp = atalk_stat_mtime_timespec(&st);
        marked = vol->v_root_marked;
        nanosleep(&tick, NULL);

        if (createfile(obj, vol->v_vid, DIRDID_ROOT, second) == AFP_OK
                && stat(vol->v_path, &st) == 0) {
            const struct timespec later = atalk_stat_mtime_timespec(&st);
            const int alike = later.tv_sec == stamp.tv_sec
                              && later.tv_nsec == stamp.tv_nsec;

            /* creates that stamp the root alike, or that a second boundary
             * splits, leave the written second's stamp untested */
            if (alike || vol->v_root_marked != marked) {
                ret = TEST_SKIP;
            } else {
                /* pollvoltime() reads a root once the session's date is
                 * 30 s old */
                vol->v_mtime -= 100;
                ret = pollvoltime(obj) == 0 && attentions == 0 ? 0 : 1;
            }
        }
    }

    delete (obj, vol->v_vid, DIRDID_ROOT, first);
    delete (obj, vol->v_vid, DIRDID_ROOT, second);
    attentions_end(obj, &saved);
    return ret;
}

/*!
 * @brief A hard create over a file of the volume root adds no entry, so a
 *        stamp another session left before it reaches the client
 */
int utest_volpoll_hard_create_compares(AFPObj *obj, struct vol *vol)
{
    const char *name = "volpoll-hard-create";
    struct attention_count saved;
    struct timespec times[2] = { { 0 } };
    time_t now;
    int ret = -1;

    if (createfile(obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK) {
        return -1;
    }

    attentions_begin(obj, &saved);
    /* this session's mark, then another session's 95 s ago */
    vol->v_root_marked = 0;
    setvoltime(obj, vol);
    vol->v_root_foreign = 0;
    now = time(NULL);
    vol->v_mtime = now - 100;
    times[0].tv_sec = times[1].tv_sec = now - 95;

    /* the helper's create is a hard one, here over the existing file */
    if (utimensat(AT_FDCWD, vol->v_path, times, 0) == 0) {
        vol->v_root_marked = 0;

        if (createfile(obj, vol->v_vid, DIRDID_ROOT, name) == AFP_OK) {
            ret = pollvoltime(obj) == 1 && attentions == 1 ? 0 : 1;
        }
    }

    attentions_end(obj, &saved);
    delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

/*!
 * @brief A create through a dangling link in the volume root, on a volume
 *        that follows symlinks, adds no entry of the root's when the file
 *        lands in a subfolder, so another session's stamp reaches the client
 */
int utest_volpoll_link_create_compares(AFPObj *obj, struct vol *vol)
{
    static char folder[] = "volpoll-link-dir";
    const char *name = "volpoll-link";
    const int flags = vol->v_flags;
    const int ad_options = vol->v_ad_options;
    struct attention_count saved;
    struct timespec times[2] = { { 0 } };
    char link_path[MAXPATHLEN + 1];
    char target[MAXPATHLEN + 1];
    const struct dir *dir;
    time_t now;
    int ret = -1;
    snprintf(link_path, sizeof(link_path), "%s/%s", vol->v_path, name);
    snprintf(target, sizeof(target), "%s/%s/target", vol->v_path, folder);

    /* a folder of the root, and a root link to a name in it that is free */
    if (createdir(obj, vol->v_vid, DIRDID_ROOT, folder) != AFP_OK) {
        return -1;
    }

    if (symlink("volpoll-link-dir/target", link_path) != 0) {
        (void)delete (obj, vol->v_vid, DIRDID_ROOT, folder);
        return -1;
    }

    /* the volume follows symlinks, as the option sets it up */
    vol->v_flags |= AFPVOL_FOLLOWSYM;
    vol->v_ad_options |= ADVOL_FOLLO_SYML;
    attentions_begin(obj, &saved);
    /* this session's mark, then another session's 95 s ago */
    vol->v_root_marked = 0;
    setvoltime(obj, vol);
    vol->v_root_foreign = 0;
    now = time(NULL);
    vol->v_mtime = now - 100;
    times[0].tv_sec = times[1].tv_sec = now - 95;

    if (utimensat(AT_FDCWD, vol->v_path, times, 0) == 0) {
        vol->v_root_marked = 0;

        if (createfile(obj, vol->v_vid, DIRDID_ROOT, name) == AFP_OK) {
            ret = pollvoltime(obj) == 1 && attentions == 1 ? 0 : 1;
        }
    }

    attentions_end(obj, &saved);
    vol->v_flags = flags;
    vol->v_ad_options = ad_options;
    /* unlink(), not FPDelete, which Solaris and illumos refuse for a link
     * whose target is gone */
    unlink(link_path);
    /* FPCreateDir cached the folder; its delete takes the target's sidecar */
    dir = dircache_search_by_name(vol, vol->v_root, folder, strlen(folder));

    if (dir == NULL
            || delete (obj, vol->v_vid, dir->d_did, "target") != AFP_OK) {
        unlink(target);
    }

    (void)delete (obj, vol->v_vid, DIRDID_ROOT, folder);
    return ret;
}

/*!
 * @brief Create through a dangling link whose target is a free name of the
 *        volume root, after a root stamp the session has seen
 *
 * @param[in,out] obj   session
 * @param[in,out] vol   volume
 * @param[in]     link  the link's path below the root
 * @param[in]     text  the link's contents, which reach
 *                      volpoll-root-link-target in the root
 *
 * @returns 0 when the creator's poll stays quiet, 1 when it announces the
 *          create, or -1 when the fixture failed
 */
static int link_root_create(AFPObj *obj, struct vol *vol, const char *link,
                            const char *text)
{
    const char *target = "volpoll-root-link-target";
    const int flags = vol->v_flags;
    const int ad_options = vol->v_ad_options;
    struct attention_count saved;
    struct timespec times[2] = { { 0 } };
    char link_path[MAXPATHLEN + 1];
    struct stat st;
    time_t now;
    int ret = -1;
    snprintf(link_path, sizeof(link_path), "%s/%s", vol->v_path, link);

    if (symlink(text, link_path) != 0) {
        return -1;
    }

    /* the volume follows symlinks, as the option sets it up */
    vol->v_flags |= AFPVOL_FOLLOWSYM;
    vol->v_ad_options |= ADVOL_FOLLO_SYML;
    attentions_begin(obj, &saved);
    /* a root stamp 95 s old that the session has seen */
    now = time(NULL);
    times[0].tv_sec = times[1].tv_sec = now - 95;

    if (utimensat(AT_FDCWD, vol->v_path, times, 0) == 0
            && stat(vol->v_path, &st) == 0) {
        vol->v_root_stamp = atalk_stat_mtime_timespec(&st);
        vol->v_root_foreign = 0;
        vol->v_root_marked = 0;

        if (createfile(obj, vol->v_vid, DIRDID_ROOT, link) == AFP_OK) {
            /* pollvoltime() reads a root once the session's date is 30 s
             * old */
            vol->v_mtime = now - 100;
            ret = pollvoltime(obj) == 0 && attentions == 0 ? 0 : 1;
        }
    }

    attentions_end(obj, &saved);
    vol->v_flags = flags;
    vol->v_ad_options = ad_options;
    unlink(link_path);

    if (delete (obj, vol->v_vid, DIRDID_ROOT, target) != AFP_OK) {
        snprintf(link_path, sizeof(link_path), "%s/%s", vol->v_path, target);
        unlink(link_path);
    }

    return ret;
}

/*!
 * @brief A create through a dangling link in the volume root whose target
 *        is a free name of the root adds a root entry, so the creator's
 *        poll stays quiet
 */
int utest_volpoll_link_root_create_own(AFPObj *obj, struct vol *vol)
{
    return link_root_create(obj, vol, "volpoll-root-link",
                            "volpoll-root-link-target");
}

/*!
 * @brief A create through a dangling link in a folder of the root whose
 *        target is a free name of the root adds a root entry too
 */
int utest_volpoll_sublink_root_create_own(AFPObj *obj, struct vol *vol)
{
    const char *folder = "volpoll-sublink-dir";
    int ret;

    if (createdir(obj, vol->v_vid, DIRDID_ROOT, folder) != AFP_OK) {
        return -1;
    }

    ret = link_root_create(obj, vol, "volpoll-sublink-dir/volpoll-sublink",
                           "../volpoll-root-link-target");
    (void)delete (obj, vol->v_vid, DIRDID_ROOT, folder);
    return ret;
}

/*!
 * @brief On a filesystem whose stamps trail afpd's clock, a marked second's
 *        change reaches the root once the second has passed
 */
int utest_volpoll_mark_trailing_written(AFPObj *obj, struct vol *vol)
{
    const struct timespec tick = { .tv_sec = 0, .tv_nsec = 20 * 1000 * 1000 };
    int flags = obj->options.flags;
    struct stat st;
    time_t second = 0;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    /* the poll only writes the root here */
    obj->options.flags &= ~OPTION_SERVERNOTIF;

    if (next_second(vol->v_path) == 0) {
        second = time(NULL);
        /* stamps 300 ms behind the clock; two marks at the second's start */
        fault_inject_reset();
        fi.utime_lag_ms = 300;
        setvoltime(obj, vol);
        setvoltime(obj, vol);
        ret = stat(vol->v_path, &st);
    }

    /* control: the interposer stamped the root behind the clock */
    if (ret == 0 && st.st_mtime >= second) {
        ret = TEST_SKIP;
    }

    /* one mark early in the next second, then a poll in the one after */
    if (ret == 0) {
        while (time(NULL) <= second) {
            nanosleep(&tick, NULL);
        }

        nanosleep(&tick, NULL);
        setvoltime(obj, vol);

        while (time(NULL) <= second + 1) {
            nanosleep(&tick, NULL);
        }

        nanosleep(&tick, NULL);
        (void)pollvoltime(obj);
        ret = stat(vol->v_path, &st) == 0 && st.st_mtime >= second + 1 ? 0 : 1;
    }

    fault_inject_reset();
    obj->options.flags = flags;
    return ret;
}

/*!
 * @brief A root stamp another session left in a second this session marked
 *        too reaches this session's client through its next poll
 */
int utest_volpoll_foreign_mark_same_second(AFPObj *obj, struct vol *vol)
{
    struct attention_count saved;
    struct timespec stamp[2] = { { .tv_nsec = UTIME_OMIT } };
    struct stat st;
    int ret = -1;
    attentions_begin(obj, &saved);
    /* this session's own mark records the root's stamp */
    vol->v_root_marked = 0;
    setvoltime(obj, vol);
    vol->v_root_foreign = 0;

    /* five seconds back: the stamp this session saw, and another session's
     * mark half a second away from it, in the same second */
    if (stat(vol->v_path, &st) == 0) {
        vol->v_root_stamp = atalk_stat_mtime_timespec(&st);
        vol->v_root_stamp.tv_sec -= 5;
        stamp[1] = vol->v_root_stamp;
        stamp[1].tv_nsec = (stamp[1].tv_nsec + 500 * 1000 * 1000)
                           % (1000 * 1000 * 1000);
        ret = utimensat(AT_FDCWD, vol->v_path, stamp, 0) == 0
              && stat(vol->v_path, &st) == 0 ? 0 : -1;
    }

    /* a filesystem that keeps whole seconds cannot tell the two apart */
    if (ret == 0 && atalk_stat_mtime_timespec(&st).tv_nsec
            == vol->v_root_stamp.tv_nsec) {
        ret = TEST_SKIP;
    }

    if (ret == 0) {
        /* pollvoltime() reads a root once the session's date is 30 s old */
        vol->v_mtime = time(NULL) - 100;
        vol->v_root_marked = 0;
        setvoltime(obj, vol);
        ret = pollvoltime(obj) == 1 && attentions == 1
              && stat(vol->v_path, &st) == 0 && vol->v_mtime == st.st_mtime
              ? 0 : 1;
    }

    attentions_end(obj, &saved);
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
    uint64_t cached = hton64(UINT64_C(4096));
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
 * @brief Tell whether the last FPGetSrvrParms reply lists a volume
 *
 * The reply holds the server time, a volume count, then a flags byte, a
 * length byte and the name of each volume.
 */
static int srvrparms_lists(const struct vol *vol)
{
    const size_t len = strlen(vol->v_localname);
    size_t off = 5;

    for (int i = 0; i < (unsigned char)reply[4] && off + 2 <= replylen; i++) {
        const size_t nlen = (unsigned char)reply[off + 1];

        if (nlen == len && off + 2 + len <= replylen
                && memcmp(reply + off + 2, vol->v_localname, len) == 0) {
            return 1;
        }

        off += 2 + nlen;
    }

    return 0;
}

/*!
 * @brief FPGetSrvrParms reads a root's ACL only when its mode bits leave
 *        out read or search, and lists the volume as the rights answer
 */
int utest_volpoll_srvrparms_mode_first(AFPObj *obj, struct vol *vol)
{
#ifndef HAVE_POSIX_ACLS
    return TEST_SKIP;
#else
    const int mapped = OPTION_ACL2MACCESS | OPTION_ACL2MODE;
    const time_t mtime = obj->options.volfile.mtime;
    uid_t uid = obj->uid;
    uid_t euid = obj->euid;
    gid_t *groups = obj->groups;
    int ngroups = obj->ngroups;
    gid_t other;
    struct stat st;
    char text[64];
    acl_t acl;
    acl_t own;
    int denied;
    int ret;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)
            || !(obj->options.flags & mapped)) {
        return TEST_SKIP;
    }

    /* the capability probe reads the root's ACL before the count starts */
    (void)test_capability(TEST_CAP_ACL_SET, vol);
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

    /* a member of the volume root's group, not its owner, whom 0700 leaves
     * to the ACL; a future volfile mtime keeps the volumes from reloading */
    fi.acl_get_file_calls = 0;
    obj->options.volfile.mtime = time(NULL) + 86400;
    obj->uid = st.st_uid + 1;
    obj->euid = st.st_uid + 1;
    obj->groups = &st.st_gid;
    obj->ngroups = 1;
    ret = getsrvrparms(obj) == AFP_OK && !srvrparms_lists(vol) ? 0 : 1;
    denied = fi.acl_get_file_calls;
    fi.acl_get_file_calls = 0;

    /* the other volumes' roots read alike in both polls */
    if (ret == 0 && (chmod(vol->v_path, 0750) != 0
                     || getsrvrparms(obj) != AFP_OK)) {
        ret = -1;
    } else if (ret == 0) {
        ret = denied > fi.acl_get_file_calls && srvrparms_lists(vol) ? 0 : 1;
    }

    /* outside the root's group, which 0750 gives nothing, with an ACL entry
     * that grants the user read and search: the ACL lists the volume */
    if (ret == 0 && test_capability(TEST_CAP_ACL_SET, vol)) {
        other = st.st_gid + 1;
        obj->groups = &other;
        own = acl_get_file(vol->v_path, ACL_TYPE_ACCESS);
        snprintf(text, sizeof(text), "u::rwx,g::---,o::---,u:%u:r-x,m::r-x",
                 (unsigned int)obj->uid);
        acl = acl_from_text(text);

        if (own == NULL || acl == NULL
                || acl_set_file(vol->v_path, ACL_TYPE_ACCESS, acl) != 0
                || getsrvrparms(obj) != AFP_OK) {
            ret = -1;
        } else {
            ret = srvrparms_lists(vol) ? 0 : 1;
        }

        if (acl) {
            acl_free(acl);
        }

        if (own) {
            (void)acl_set_file(vol->v_path, ACL_TYPE_ACCESS, own);
            acl_free(own);
        }
    }

    obj->options.volfile.mtime = mtime;
    obj->uid = uid;
    obj->euid = euid;
    obj->groups = groups;
    obj->ngroups = ngroups;
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
    int before;
    int after;
    int ret;
    /* the lowest free descriptor: an exit that leaves the root's metadata
     * open moves it up */
    before = open("/dev/null", O_RDONLY);
    close(before);
    vol->v_macname = name;
    ret = getvolparms(obj, vol, 1 << VOLPBIT_NAME);
    vol->v_macname = macname;
    after = open("/dev/null", O_RDONLY);
    close(after);
    return ret == AFPERR_MISC && after == before ? 0 : 1;
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
 * @brief FPOpenVol asking for the parameters in bitmap, with the name's
 *        raw bytes
 *
 * @returns the AFP result
 */
static int openvol_raw(AFPObj *obj, uint16_t bitmap, const char *name,
                       uint8_t namelen)
{
    char req[2 + 2 + 1 + 255 + 1] = { 0 };
    size_t len = sizeof(reply);
    bitmap = htons(bitmap);
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
    return openvol_raw(obj, 1 << VOLPBIT_VID, "\xff\xfe", 2) == AFPERR_PARAM
           ? 0 : 1;
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
    (void)openvol_raw(obj, 1 << VOLPBIT_VID, name, sizeof(name));
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
 * @brief FPSetForkParms marks the volume changed, and a request refused
 *        before the truncate marks nothing
 */
int utest_volpoll_mark_setforkparams(AFPObj *obj, struct vol *vol)
{
    const char *name = "volpoll-setforkparams";
    const uint16_t rflen = htons(1 << FILPBIT_RFLEN);
    char req[10] = { 0 };
    size_t len = sizeof(reply);
    uint16_t refnum;
    int ret;

    if (createfile(obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                        OPENACC_RD | OPENACC_WR, &refnum) != AFP_OK) {
        return -1;
    }

    /* the resource fork's length for a data fork fails the bitmap */
    memcpy(req + 2, &refnum, sizeof(refnum));
    memcpy(req + 4, &rflen, sizeof(rflen));
    vol->v_root_marked = 0;
    ret = afp_setforkparams(obj, req, sizeof(req), reply, &len)
          == AFPERR_BITMAP && vol->v_root_marked == 0 ? 0 : 1;

    if (ret == 0) {
        ret = marked(vol, setforkparams(obj, refnum, 1));
    }

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

    /* a request that changes nothing marks nothing */
    vol->v_root_marked = 0;
    ret = exchangefiles(obj, vol->v_vid, DIRDID_ROOT, DIRDID_ROOT, a, a)
          == AFPERR_SAMEOBJ && vol->v_root_marked == 0 ? 0 : 1;

    if (ret == 0) {
        ret = marked(vol, exchangefiles(obj, vol->v_vid, DIRDID_ROOT,
                                        DIRDID_ROOT, a, b));
    }

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

    /* a removal that fails marks nothing */
    vol->v_root_marked = 0;

    if (remextattr(obj, vol->v_vid, DIRDID_ROOT, "", "volpoll-absent")
            == AFP_OK || vol->v_root_marked != 0) {
        (void)remextattr(obj, vol->v_vid, DIRDID_ROOT, "", "volpoll");
        return 1;
    }

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
    if (!(vol->v_flags & AFPVOL_ACLS)
            || !test_capability(TEST_CAP_ACL_SET, vol)) {
        return TEST_SKIP;
    }

    vol->v_root_marked = 0;
    return marked(vol, setacl(obj, vol->v_vid, DIRDID_ROOT, "",
                              kFileSec_REMOVEACL, NULL, 0));
#endif /* HAVE_ACLS */
}

/*!
 * @brief FPSetACL sets the ACL that follows a lone group UUID, marking the
 *        volume, and the one that follows a lone owner UUID
 */
int utest_volpoll_setacl_group_uuid(AFPObj *obj, struct vol *vol)
{
#ifndef HAVE_ACLS
    return TEST_SKIP;
#else
    /* one UUID, an ACL header with no ACEs, then where a parse that skipped
     * two UUIDs would read its header, a count no request can carry */
    unsigned char body[2 * UUID_BINSIZE + sizeof(darwin_acl_header_t)];
    struct stat st;
    int ret;

    /* creatvol() clears the flag where the root's ACL cannot be read */
    if (!(vol->v_flags & AFPVOL_ACLS)
            || !test_capability(TEST_CAP_ACL_SET, vol)) {
        return TEST_SKIP;
    }

    if (setacl(obj, vol->v_vid, DIRDID_ROOT, "", kFileSec_REMOVEACL, NULL, 0)
            != AFP_OK || stat(vol->v_path, &st) != 0) {
        return -1;
    }

    memset(body, 0, sizeof(body));
    memset(body + UUID_BINSIZE + sizeof(darwin_acl_header_t), 0xff,
           UUID_BINSIZE);
    localuuid_from_id(body, UUID_GROUP, getegid());
    vol->v_root_marked = 0;
    ret = setacl(obj, vol->v_vid, DIRDID_ROOT, "",
                 kFileSec_GRPUUID | kFileSec_ACL, body, sizeof(body)) == AFP_OK
          && vol->v_root_marked != 0 ? 0 : 1;

    /* the owner UUID alone: the ACL follows it, not a second UUID */
    if (ret == 0) {
        localuuid_from_id(body, UUID_USER, geteuid());
        ret = setacl(obj, vol->v_vid, DIRDID_ROOT, "",
                     kFileSec_UUID | kFileSec_ACL, body, sizeof(body)) == AFP_OK
              ? 0 : 1;
    }

    (void)setacl(obj, vol->v_vid, DIRDID_ROOT, "", kFileSec_REMOVEACL, NULL, 0);
    /* the ACL's base entries replaced the root's mode bits */
    (void)chmod(vol->v_path, st.st_mode & 07777);
    return ret;
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

    /* a build without ACLs has no FPSetACL to refuse */
    if (ret == 0) {
        int rc = setacl(obj, vol->v_vid, DIRDID_ROOT, "", kFileSec_REMOVEACL,
                        NULL, 0);
        ret = rc == AFPERR_VLOCK || rc == -1 ? 0 : 1;
    }

    vol->v_flags = flags;
    return ret;
}

/*!
 * @brief A volume open that fails leaves the session's current directory
 *        as it was, with no pointer to the root the open freed
 */
int utest_volpoll_openvol_failed_keeps_curdir(AFPObj *obj, struct vol *vol)
{
    /* an undefined bit fails the parameter reply, after the root exists */
    const uint16_t bitmap = (1 << VOLPBIT_VID) | (1 << 15);
    int ret;
    /* a volume close clears the current directory first */
    curdir = NULL;
    closevol(obj, vol);
    ret = openvol_raw(obj, bitmap, "afpd_test", 9) == AFPERR_BITMAP
          && curdir == NULL ? 0 : 1;

    /* the volume goes back open for the tests after this one */
    if (openvol(obj, "afpd_test") == 0) {
        ret = -1;
    }

    return ret;
}

/*!
 * @brief A volume that holds no quota answer fetches one, whatever the
 *        clock reads
 */
int utest_volpoll_quota_unfetched(AFPObj *obj, struct vol *vol)
{
#ifdef NO_QUOTA_SUPPORT
    return TEST_SKIP;
#else
    int flags = vol->v_flags;
    int ret;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    /* a clock five seconds past the epoch, and a volume not fetched */
    vol->v_flags = (vol->v_flags & ~AFPVOL_GVSMASK) | AFPVOL_UQUOTA;
    vol->v_qtime = 0;
    fault_inject_reset();
    fi.clock_fake = 1;
    fi.clock_sec = 5;
    ret = getvolparms(obj, vol, 1 << VOLPBIT_XBFREE);

    /* control: the poll read its clock through the interposer */
    if (fi.clock_calls == 0) {
        ret = TEST_SKIP;
    } else if (ret != AFP_OK) {
        ret = -1;
    } else {
        /* a fetch either switches the volume to statfs or stamps it */
        ret = (vol->v_flags & AFPVOL_GVSMASK) != AFPVOL_UQUOTA
              || vol->v_qtime != 0 ? 0 : 1;
    }

    fault_inject_reset();
    vol->v_flags = flags;
    vol->v_qtime = 0;
    return ret;
#endif /* NO_QUOTA_SUPPORT */
}

/*!
 * @brief A fork's length change sends a client before AFP 3.2 no
 *        free-space attention, through its own mark or a due write, while
 *        a change of another kind does
 */
int utest_volpoll_write_attention_quiet(AFPObj *obj, struct vol *vol)
{
    const char *name = "volpoll-write-attention";
    const int version = obj->afp_version;
    struct attention_count saved;
    uint16_t refnum;
    int ret = -1;

    if (createfile(obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK
            || openfork(obj, vol->v_vid, DIRDID_ROOT, name, OPENFORK_DATA,
                        OPENACC_RD | OPENACC_WR, &refnum) != AFP_OK) {
        return -1;
    }

    /* a root write that covers the create, before the count starts */
    vol->v_root_marked = 0;
    setvoltime(obj, vol);
    attentions_begin(obj, &saved);
    obj->afp_version = 31;
    /* one change that writes the root, one left due and written once its
     * second has passed */
    vol->v_root_marked = 0;

    if (setforkparams(obj, refnum, 1) == AFP_OK
            && setforkparams(obj, refnum, 2) == AFP_OK) {
        vol->v_root_marked = coarse_time() - 1;
        (void)flushvoltime(obj);
        ret = attentions == 0 ? 0 : 1;
    }

    if (ret == 0) {
        vol->v_root_marked = 0;
        setvoltime(obj, vol);
        ret = attentions == 1 ? 0 : 1;
    }

    obj->afp_version = version;
    attentions_end(obj, &saved);
    (void)closefork(obj, refnum);
    (void)delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

/*!
 * @brief FPSetVolParms answers the error the backup date's flush met:
 *        kFPAccessDenied for a user who cannot write the root's metadata,
 *        kFPVolLocked for a read-only filesystem
 */
int utest_volpoll_setvolparms_flush_error(AFPObj *obj, struct vol *vol)
{
    const int errs[] = { EACCES, EPERM, EROFS };
    const int want[] = { AFPERR_ACCESS, AFPERR_ACCESS, AFPERR_VLOCK };
    int ret = 0;

    if (vol->v_adouble != AD_VERSION_EA
            || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    for (size_t i = 0; i < sizeof(errs) / sizeof(errs[0]) && ret == 0; i++) {
        fault_inject_reset();
        /* the write of a directory's metadata changes into the directory */
        fi.fchdir_armed = 1;
        fi.fchdir_errno = errs[i];
        ret = setbackupdate(obj, vol) == want[i] ? 0 : 1;
    }

    fault_inject_reset();
    return ret;
}

/*!
 * @brief An FPExchangeFiles that fails before its renames leaves the
 *        destination folder's directory cache entry in place
 */
int utest_volpoll_exchange_failure_cached(AFPObj *obj, struct vol *vol)
{
    static char folder[] = "volpoll-xfolder";
    const char *file = "volpoll-xfile";
    const struct dir *dir;
    int ret = -1;

    if (createdir(obj, vol->v_vid, DIRDID_ROOT, folder) != AFP_OK
            || createfile(obj, vol->v_vid, DIRDID_ROOT, file) != AFP_OK) {
        (void)delete (obj, vol->v_vid, DIRDID_ROOT, folder);
        return -1;
    }

    /* FPCreateDir caches the folder it creates */
    dir = dircache_search_by_name(vol, vol->v_root, folder, strlen(folder));

    /* the destination file does not exist */
    if (dir != NULL) {
        ret = exchangefiles(obj, vol->v_vid, DIRDID_ROOT, dir->d_did, file,
                            "volpoll-missing") != AFP_OK
              && dircache_search_by_did(vol, dir->d_did) == dir ? 0 : 1;
    }

    (void)delete (obj, vol->v_vid, DIRDID_ROOT, file);
    (void)delete (obj, vol->v_vid, DIRDID_ROOT, folder);
    return ret;
}

/*!
 * @brief A volume's Mac name converts in the global Mac charset it came
 *        from, whatever the volume's own mac charset holds
 */
int utest_volpoll_name_global_charset(AFPObj *obj, struct vol *vol)
{
    static ucs2_t name[] = { 0x00E9, 0 };
    ucs2_t *macname = vol->v_macname;
    const charset_t maccharset = vol->v_maccharset;
    const charset_t cyrillic = add_charset("MAC_CYRILLIC");
    int ret;

    if (cyrillic == (charset_t) -1) {
        return -1;
    }

    /* Mac Cyrillic has no U+00E9; the global Mac Roman has, as 0x8e */
    vol->v_macname = name;
    vol->v_maccharset = cyrillic;
    ret = getvolparms(obj, vol, 1 << VOLPBIT_NAME);
    vol->v_macname = macname;
    vol->v_maccharset = maccharset;
    /* the bitmap, the name's offset, then its length byte and the name */
    return ret == AFP_OK && replylen == 6 && reply[4] == 1
           && (unsigned char)reply[5] == 0x8e ? 0 : 1;
}

/*!
 * @brief FPRemoveExtAttr on the volume root with a name field that claims
 *        namelen bytes and holds the len bytes at name, any byte among them
 *
 * @returns the AFP result
 */
static int remextattr_raw(AFPObj *obj, const struct vol *vol,
                          uint16_t namelen, const char *name, size_t len)
{
    static char req[20 + 255];
    const uint32_t hint = htonl(kTextEncodingUTF8);
    const uint16_t field = htons(namelen);
    const cnid_t did = DIRDID_ROOT;
    /* the name field's length word starts at an even address */
    const size_t at = 17 + ((unsigned long)(req + 17) & 1);
    size_t rlen = sizeof(reply);
    memset(req, 0, sizeof(req));
    memcpy(req + 2, &vol->v_vid, sizeof(vol->v_vid));
    memcpy(req + 4, &did, sizeof(did));
    /* bitmap 0, then the folder itself as an empty UTF-8 name */
    req[10] = 3;
    memcpy(req + 11, &hint, sizeof(hint));
    memcpy(req + at, &field, sizeof(field));
    memcpy(req + at + 2, name, len);
    return afp_remextattr(obj, req, at + 2 + len, reply, &rlen);
}

/*!
 * @brief FPSetExtAttr of "abc" on the volume root whose value field claims
 *        size bytes, the request ending tail bytes after the name
 *
 * @returns the AFP result
 */
static int setextattr_raw(AFPObj *obj, const struct vol *vol, uint32_t size,
                          size_t tail)
{
    static char req[48];
    const uint32_t hint = htonl(kTextEncodingUTF8);
    const uint16_t namelen = htons(3);
    const uint32_t field = htonl(size);
    const cnid_t did = DIRDID_ROOT;
    /* the name field's length word starts at an even address */
    const size_t at = 25 + ((unsigned long)(req + 25) & 1);
    size_t rlen = sizeof(reply);
    memset(req, 0, sizeof(req));
    memcpy(req + 2, &vol->v_vid, sizeof(vol->v_vid));
    memcpy(req + 4, &did, sizeof(did));
    /* bitmap and offset 0, then the folder itself as an empty UTF-8 name */
    req[18] = 3;
    memcpy(req + 19, &hint, sizeof(hint));
    memcpy(req + at, &namelen, sizeof(namelen));
    memcpy(req + at + 2, "abc", 3);
    memcpy(req + at + 5, &field, sizeof(field));
    return afp_setextattr(obj, req, at + 5 + tail, reply, &rlen);
}

/*!
 * @brief FPListExtAttrs of the volume root; the names follow the bitmap and
 *        the length in reply
 *
 * @returns the AFP result
 */
static int listextattr_root(AFPObj *obj, const struct vol *vol)
{
    static char req[27];
    const uint32_t hint = htonl(kTextEncodingUTF8);
    const uint32_t maxreply = htonl(sizeof(reply) - 6);
    const cnid_t did = DIRDID_ROOT;
    memset(req, 0, sizeof(req));
    memcpy(req + 2, &vol->v_vid, sizeof(vol->v_vid));
    memcpy(req + 4, &did, sizeof(did));
    /* bitmap, request count and start index 0, the reply size, then the
     * folder itself as an empty UTF-8 name */
    memcpy(req + 16, &maxreply, sizeof(maxreply));
    req[20] = 3;
    memcpy(req + 21, &hint, sizeof(hint));
    replylen = sizeof(reply);
    return afp_listextattr(obj, req, sizeof(req), reply, &replylen);
}

/*! @brief Tell whether the last FPListExtAttrs reply lists an EA name */
static int listed_ea(const char *name)
{
    uint32_t len;
    memcpy(&len, reply + 2, sizeof(len));
    len = ntohl(len);

    for (size_t off = 0; off < len && 6 + off < replylen;
            off += strlen(reply + 6 + off) + 1) {
        if (strcmp(reply + 6 + off, name) == 0) {
            return 1;
        }
    }

    return 0;
}

/*!
 * @brief The EA requests refuse a name over 255 bytes, one longer than the
 *        request and one that holds a NUL byte
 */
int utest_volpoll_ea_name_field(AFPObj *obj, struct vol *vol)
{
    char decomposed[3 * 100 + 1];
    int ret;

    if (vol->v_vfs_ea == AFPVOL_EA_NONE) {
        return TEST_SKIP;
    }

    /* 100 decomposed U+00E9: 300 bytes on the wire, 200 once composed */
    for (int i = 0; i < 100; i++) {
        memcpy(decomposed + 3 * i, "e\xcc\x81", 3);
    }

    decomposed[300] = '\0';

    if (setextattr(obj, vol->v_vid, DIRDID_ROOT, "", "abc", "1", 1)
            != AFP_OK) {
        return -1;
    }

    ret = getextattr(obj, vol->v_vid, DIRDID_ROOT, "", decomposed, 0, reply,
                     &replylen) == AFPERR_PARAM
          /* a NUL alone, and one inside a name that starts as "abc" */
          && remextattr_raw(obj, vol, 1, "", 1) == AFPERR_PARAM
          && remextattr_raw(obj, vol, 7, "abc\0def", 7) == AFPERR_PARAM
          /* a length past the request's end */
          && remextattr_raw(obj, vol, 10, "abc", 3) == AFPERR_PARAM ? 0 : 1;

    /* "abc" outlives them */
    if (remextattr(obj, vol->v_vid, DIRDID_ROOT, "", "abc") != AFP_OK) {
        ret = 1;
    }

    return ret;
}

/*!
 * @brief FPSetExtAttr refuses a value field the request does not hold
 */
int utest_volpoll_ea_value_bound(AFPObj *obj, struct vol *vol)
{
    int ret;

    if (vol->v_vfs_ea == AFPVOL_EA_NONE) {
        return TEST_SKIP;
    }

    /* control: a request that holds its 9-byte value */
    if (setextattr_raw(obj, vol, 9, 4 + 9) != AFP_OK) {
        return -1;
    }

    /* a length field cut short, and 9 bytes claimed with 1 sent */
    ret = setextattr_raw(obj, vol, 9, 2) == AFPERR_PARAM
          && setextattr_raw(obj, vol, 9, 4 + 1) == AFPERR_PARAM ? 0 : 1;
    (void)remextattr(obj, vol->v_vid, DIRDID_ROOT, "", "abc");
    return ret;
}

/*!
 * @brief On a volume without EA support the EA requests fail, as no EA
 *        module runs, and mark nothing
 */
int utest_volpoll_ea_none_refused(AFPObj *obj, struct vol *vol)
{
    if (vol->v_vfs_ea != AFPVOL_EA_NONE) {
        return TEST_SKIP;
    }

    vol->v_root_marked = 0;
    return setextattr(obj, vol->v_vid, DIRDID_ROOT, "", "volpoll", "1", 1)
           == AFPERR_MISC
           && remextattr(obj, vol->v_vid, DIRDID_ROOT, "", "volpoll")
           == AFPERR_MISC
           && getextattr(obj, vol->v_vid, DIRDID_ROOT, "", "volpoll", 0,
                         reply, &replylen) == AFPERR_MISC
           && vol->v_root_marked == 0 ? 0 : 1;
}

/*!
 * @brief An EA name that takes all 255 bytes as the Mac lists it lists from
 *        AppleDouble EA files
 */
int utest_volpoll_ea_ad_long_listed(AFPObj *obj, struct vol *vol)
{
    char name[3 * 85 + 1];
    int ret = -1;

    if (vol->v_vfs_ea != AFPVOL_EA_SYS) {
        return TEST_SKIP;
    }

#ifdef __APPLE__

    /* the layout names the EA header under the file's named fork */
    if (vol->v_adouble == AD_VERSION_EA) {
        return TEST_SKIP;
    }

#endif

    /* 85 decomposed U+00E9: 255 bytes on the wire, 170 once composed */
    for (int i = 0; i < 85; i++) {
        memcpy(name + 3 * i, "e\xcc\x81", 3);
    }

    name[255] = '\0';
    vol->v_vfs_ea = AFPVOL_EA_AD;
    initvol_vfs(vol);

    if (setextattr(obj, vol->v_vid, DIRDID_ROOT, "", name, "1", 1)
            == AFP_OK) {
        ret = listextattr_root(obj, vol) == AFP_OK && listed_ea(name) ? 0 : 1;
        (void)remextattr(obj, vol->v_vid, DIRDID_ROOT, "", name);
    }

    vol->v_vfs_ea = AFPVOL_EA_SYS;
    initvol_vfs(vol);
    return ret;
}

/*!
 * @brief An EA name set on a volume whose charset is not the unix charset
 *        lists as it was set
 */
int utest_volpoll_ea_name_listed(AFPObj *obj, struct vol *vol)
{
    /* U+00E9 decomposed, as a Mac sends it */
    const char *name = "volpoll-e\xcc\x81";
    /* the same name in Mac Roman */
    const char *macname = "volpoll-\x8e";
    const charset_t volcharset = vol->v_volcharset;
    int ret = -1;

    if (vol->v_vfs_ea == AFPVOL_EA_NONE) {
        return TEST_SKIP;
    }

    /* control: the filesystem takes a Mac Roman name; macOS takes UTF-8 */
    if (sys_setxattr(vol->v_path, macname, "1", 1, 0) != 0) {
        return TEST_SKIP;
    }

    (void)sys_removexattr(vol->v_path, macname);
    /* Mac Roman holds U+00E9, in another byte than UTF-8 */
    vol->v_volcharset = CH_MAC;

    if (setextattr(obj, vol->v_vid, DIRDID_ROOT, "", name, "1", 1)
            == AFP_OK) {
        ret = listextattr_root(obj, vol) == AFP_OK && listed_ea(name) ? 0 : 1;
        (void)remextattr(obj, vol->v_vid, DIRDID_ROOT, "", name);
    }

    vol->v_volcharset = volcharset;
    return ret;
}

/*!
 * @brief FPOpenVol refuses a volume name that holds a NUL byte, which ends
 *        the name the lookup compares but not the one it converts
 */
int utest_volpoll_volname_nul(AFPObj *obj)
{
    /* the test volume's name, a NUL, and one byte more */
    static const char name[] = "afpd_test\0x";
    return openvol_raw(obj, 1 << VOLPBIT_VID, name, sizeof(name) - 1)
           == AFPERR_PARAM ? 0 : 1;
}

/*!
 * @brief An FPSetDirParms that makes a root folder invisible and then fails
 *        on a later bit marks the volume for the root's date
 */
int utest_volpoll_setdirparms_partial_marks(AFPObj *obj, struct vol *vol)
{
    const char *name = "volpoll-partial";
    const uint16_t attr = htons(ATTRBIT_SETCLR | ATTRBIT_INVISIBLE);
    /* the attribute word, then a parent folder id, which no request sets */
    char params[6] = { 0 };
    int ret;

    if (createdir(obj, vol->v_vid, DIRDID_ROOT, name) != AFP_OK) {
        return -1;
    }

    memcpy(params, &attr, sizeof(attr));
    vol->v_root_marked = 0;
    ret = setdirparms(obj, vol->v_vid, DIRDID_ROOT, name,
                      (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_PDID), params,
                      sizeof(params)) == AFPERR_BITMAP
          && vol->v_root_marked != 0 ? 0 : 1;
    (void)delete (obj, vol->v_vid, DIRDID_ROOT, name);
    return ret;
}

/*!
 * @brief The free-space attention a mark sends drops a limited quota answer,
 *        so the poll it asks for fetches one, and keeps an unlimited one
 */
int utest_volpoll_attention_quota_fresh(AFPObj *obj, struct vol *vol)
{
#ifdef NO_QUOTA_SUPPORT
    return TEST_SKIP;
#else
    const int version = obj->afp_version;
    const int flags = vol->v_flags;
    const VolSpace qtotal = vol->v_qtotal;
    struct attention_count saved;
    int ret;
    attentions_begin(obj, &saved);
    obj->afp_version = 31;
    /* a limited answer just fetched, then a change that writes the root */
    vol->v_flags = (vol->v_flags & ~AFPVOL_GVSMASK) | AFPVOL_UQUOTA;
    vol->v_qtotal = 1024 * 1024;
    vol->v_qtime = coarse_time();
    vol->v_root_marked = 0;
    setvoltime(obj, vol);
    ret = attentions == 1 && vol->v_qtime == 0 ? 0 : 1;
    /* an unlimited answer outlives the attention */
    vol->v_qtotal = ~(VolSpace)0;
    vol->v_qtime = coarse_time();
    vol->v_root_marked = 0;
    setvoltime(obj, vol);

    if (attentions != 2 || vol->v_qtime == 0) {
        ret = 1;
    }

    obj->afp_version = version;
    attentions_end(obj, &saved);
    vol->v_flags = flags;
    vol->v_qtotal = qtotal;
    vol->v_qtime = 0;
    return ret;
#endif /* NO_QUOTA_SUPPORT */
}

/*!
 * @brief The free-space attention drops the limited quota answer of every
 *        open volume on the marked volume's filesystem
 */
int utest_volpoll_attention_sibling_quota(AFPObj *obj, struct vol *vol)
{
#ifdef NO_QUOTA_SUPPORT
    return TEST_SKIP;
#else
    const int version = obj->afp_version;
    struct attention_count saved;
    struct vol *sibling = getvolumes();
    VolSpace qtotal;
    int ret;

    while (sibling != NULL && (sibling == vol
                               || !(sibling->v_flags & AFPVOL_OPEN)
                               || sibling->v_dev != vol->v_dev)) {
        sibling = sibling->v_next;
    }

    /* no other open volume shares the filesystem */
    if (sibling == NULL) {
        return TEST_SKIP;
    }

    qtotal = sibling->v_qtotal;
    attentions_begin(obj, &saved);
    obj->afp_version = 31;
    /* a limited answer on the sibling, then a change that writes the root */
    sibling->v_qtotal = 1024 * 1024;
    sibling->v_qtime = coarse_time();
    vol->v_root_marked = 0;
    setvoltime(obj, vol);
    ret = attentions == 1 && sibling->v_qtime == 0 ? 0 : 1;
    obj->afp_version = version;
    attentions_end(obj, &saved);
    sibling->v_qtotal = qtotal;
    sibling->v_qtime = 0;
    return ret;
#endif /* NO_QUOTA_SUPPORT */
}
