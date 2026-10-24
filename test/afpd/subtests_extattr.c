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

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/directory.h>
#include <atalk/ea.h>
#include <atalk/globals.h>
#include <atalk/volume.h>

#include "acls.h"
#include "ad_cache.h"
#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "faultinject.h"
#include "fork.h"
#include "subtests_extattr.h"
#include "test.h"
#include "test_capabilities.h"

#define EA_FILE "ea_cache_file"
#define EA_BARE "ea_cache_bare"
#define EA_DIR  "ea_cache_dir"
#define EA_BIG  "ea_cache_big"
#define EA_SAMBA "ea_cache_samba"
#define EA_ABSENT "ea_cache_absent"

/* one request's reply */
struct ea_reply {
    int    result;
    size_t len;
    char   buf[ATTRNAMEBUFSIZ + MAX_REPLY_EXTRA_BYTES];
};

/*!
 * @brief Delete the test files
 */
static void ea_teardown(AFPObj *obj, const struct vol *vol)
{
    fault_inject_reset();
    (void)delete (obj, vol->v_vid, DIRDID_ROOT, EA_FILE);
    (void)delete (obj, vol->v_vid, DIRDID_ROOT, EA_BARE);
}

/*!
 * @brief Create EA_FILE with EAs a (5 bytes), b (300) and c (1200) and
 *        EA_BARE with none; FPGetFileDirParms gives each a dircache entry
 *
 * @returns 0, or 1 when a step failed
 */
static int ea_setup(AFPObj *obj, const struct vol *vol)
{
    uint16_t vid = vol->v_vid;
    char path[MAXPATHLEN + 1];
    static char c[1200];
    char b[300];

    if (createfile(obj, vid, DIRDID_ROOT, EA_FILE) != AFP_OK
            || createfile(obj, vid, DIRDID_ROOT, EA_BARE) != AFP_OK
            || getfiledirparms(obj, vid, DIRDID_ROOT, EA_FILE) != AFP_OK
            || getfiledirparms(obj, vid, DIRDID_ROOT, EA_BARE) != AFP_OK) {
        ea_teardown(obj, vol);
        return 1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, EA_FILE);
    memset(b, 'b', sizeof(b));
    memset(c, 'c', sizeof(c));

    if (sys_setxattr(path, "a", "aaaaa", 5, 0) != 0
            || sys_setxattr(path, "b", b, sizeof(b), 0) != 0
            || sys_setxattr(path, "c", c, sizeof(c), 0) != 0) {
        ea_teardown(obj, vol);
        return 1;
    }

    return 0;
}

/*!
 * @brief FPGetExtAttr of file's attribute attr
 */
static void ea_get(AFPObj *obj, const struct vol *vol, const char *file,
                   const char *attr, uint32_t maxreply, struct ea_reply *r)
{
    r->result = getextattr(obj, vol->v_vid, DIRDID_ROOT, file, attr, maxreply,
                           r->buf, &r->len);
}

/*!
 * @brief FPListExtAttrs of file
 */
static void ea_list(AFPObj *obj, const struct vol *vol, const char *file,
                    uint32_t maxreply, struct ea_reply *r)
{
    r->result = listextattr(obj, vol->v_vid, DIRDID_ROOT, file, maxreply,
                            r->buf, &r->len);
}

/*!
 * @brief Same result, length and bytes?
 */
static bool ea_same_reply(const struct ea_reply *a, const struct ea_reply *b)
{
    return a->result == b->result && a->len == b->len
           && memcmp(a->buf, b->buf, a->len) == 0;
}

/*!
 * @brief The file's dircache entry under the volume root
 */
static struct dir *ea_entry(struct vol *vol, char *file)
{
    struct dir *root = dirlookup(vol, DIRDID_ROOT);
    return root ? dircache_search_by_name(vol, root, file, strlen(file))
           : NULL;
}

/*!
 * @brief A value larger than maxreply is refused with its size in the
 *        reply's length word
 */
int utest_ea_param_reports_size(AFPObj *obj, struct vol *vol)
{
    struct ea_reply r;
    uint32_t size;

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    ea_get(obj, vol, EA_FILE, "b", 100, &r);
    ea_teardown(obj, vol);

    if (r.result != AFPERR_PARAM || r.len != 2 + sizeof(size)) {
        return 2;
    }

    memcpy(&size, r.buf + 2, sizeof(size));
    return ntohl(size) == 300 ? 0 : 3;
}

/*!
 * @brief A size probe of a protected name is refused as its fetch is
 */
int utest_ea_protected_name_refused(AFPObj *obj, struct vol *vol)
{
    struct ea_reply probe;
    struct ea_reply fetch;

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    ea_get(obj, vol, EA_FILE, AD_EA_META, 0, &probe);
    ea_get(obj, vol, EA_FILE, AD_EA_META, 4096, &fetch);
    ea_teardown(obj, vol);

    if (probe.result != AFPERR_ACCESS || probe.len != 2) {
        return 2;
    }

    return fetch.result == AFPERR_ACCESS && fetch.len == 2 ? 0 : 3;
}

/*!
 * @brief With ea = samba a value of MAX_EA_SIZE bytes, stored with its
 *        trailing NUL, probes as the size its fetch delivers
 */
int utest_ea_samba_size_matches(AFPObj *obj, struct vol *vol)
{
    static char s[MAX_EA_SIZE + 1];
    struct ea_reply probe;
    struct ea_reply fetch;
    char path[MAXPATHLEN + 1];
    uint32_t size;
    int flags = vol->v_flags;
    int fd;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, EA_SAMBA);
    fd = open(path, O_WRONLY | O_CREAT, 0600);

    if (fd == -1) {
        return 1;
    }

    close(fd);
    memset(s, 's', MAX_EA_SIZE);

    if (sys_setxattr(path, "s", s, sizeof(s), 0) != 0) {
        unlink(path);
        return 2;
    }

    vol->v_flags |= AFPVOL_EA_SAMBA;
    ea_get(obj, vol, EA_SAMBA, "s", 0, &probe);
    ea_get(obj, vol, EA_SAMBA, "s", MAX_EA_SIZE + MAX_REPLY_EXTRA_BYTES,
           &fetch);
    vol->v_flags = flags;
    unlink(path);
    memcpy(&size, probe.buf + 2, sizeof(size));

    if (probe.result != AFP_OK || probe.len != 6
            || ntohl(size) != MAX_EA_SIZE) {
        return 3;
    }

    return fetch.result == AFP_OK && fetch.len == 6 + MAX_EA_SIZE ? 0 : 4;
}

/*!
 * @brief A name the file lacks is a missing attribute on both paths
 */
int utest_ea_missing_name_absent(AFPObj *obj, struct vol *vol)
{
    struct ea_reply probe;
    struct ea_reply fetch;
    uint32_t size;

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    ea_get(obj, vol, EA_FILE, "missing", 0, &probe);
    ea_get(obj, vol, EA_FILE, "missing", 4096, &fetch);
    ea_teardown(obj, vol);

    if (probe.result != AFPERR_NOITEM || fetch.result != AFPERR_NOITEM
            || probe.len != 6 || fetch.len != 6) {
        return 2;
    }

    memcpy(&size, fetch.buf + 2, sizeof(size));
    return size == 0 ? 0 : 3;
}

/*!
 * @brief FPGetExtAttr on both paths, FPSetExtAttr and FPRemoveExtAttr of a
 *        file that does not exist are kFPObjectNotFound, as FPListExtAttrs
 *        answers it
 */
int utest_ea_missing_object_noobj(AFPObj *obj, struct vol *vol)
{
    struct ea_reply probe;
    struct ea_reply fetch;
    ea_get(obj, vol, EA_ABSENT, "a", 0, &probe);
    ea_get(obj, vol, EA_ABSENT, "a", 4096, &fetch);

    if (probe.result != AFPERR_NOOBJ || probe.len != 0) {
        return 1;
    }

    if (fetch.result != AFPERR_NOOBJ || fetch.len != 0) {
        return 2;
    }

    if (setextattr(obj, vol->v_vid, DIRDID_ROOT, EA_ABSENT, "a", "x", 1)
            != AFPERR_NOOBJ) {
        return 3;
    }

    return remextattr(obj, vol->v_vid, DIRDID_ROOT, EA_ABSENT, "a")
           == AFPERR_NOOBJ ? 0 : 4;
}

/*!
 * @brief An attribute name that is empty or not UTF-8 is refused with
 *        kFPParamErr by all three EA requests
 */
int utest_ea_bad_name_refused(AFPObj *obj, struct vol *vol)
{
    /* no UTF-8 sequence holds these bytes */
    const char *bad = "\xff\xfe";
    struct ea_reply r;

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    ea_get(obj, vol, EA_FILE, "", 4096, &r);

    if (r.result != AFPERR_PARAM) {
        ea_teardown(obj, vol);
        return 2;
    }

    ea_get(obj, vol, EA_FILE, bad, 4096, &r);

    if (r.result != AFPERR_PARAM) {
        ea_teardown(obj, vol);
        return 3;
    }

    if (setextattr(obj, vol->v_vid, DIRDID_ROOT, EA_FILE, bad, "x", 1)
            != AFPERR_PARAM) {
        ea_teardown(obj, vol);
        return 4;
    }

    r.result = remextattr(obj, vol->v_vid, DIRDID_ROOT, EA_FILE, bad);
    ea_teardown(obj, vol);
    return r.result == AFPERR_PARAM ? 0 : 5;
}

/*!
 * @brief Two lists of a file read its EAs once; a file without EAs caches
 *        its empty list too
 */
int utest_ea_names_cached(AFPObj *obj, struct vol *vol)
{
    struct ea_reply first;
    struct ea_reply second;
    int ret = 0;

    if (!test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    fault_inject_reset();
    fi.xattr_watch = 1;
    ea_list(obj, vol, EA_FILE, 4096, &first);
    ea_list(obj, vol, EA_FILE, 4096, &second);

    if (first.result != AFP_OK || fi.listxattr_calls != 1) {
        ret = 2;
    } else if (!ea_same_reply(&first, &second)) {
        ret = 3;
    }

    ea_list(obj, vol, EA_BARE, 4096, &first);
    ea_list(obj, vol, EA_BARE, 4096, &second);

    if (ret == 0 && (first.result != AFP_OK || first.len != 6
                     || fi.listxattr_calls != 2
                     || !ea_same_reply(&first, &second))) {
        ret = 4;
    }

    ea_teardown(obj, vol);
    return ret;
}

/*!
 * @brief Cached names follow the file's EA writes: one from outside afpd
 *        (seen through ctime), one set and one removal through afpd
 */
int utest_ea_names_follow_writes(AFPObj *obj, struct vol *vol)
{
    struct ea_reply r;
    char path[MAXPATHLEN + 1];
    size_t len;
    int ret = 0;

    if (!test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, EA_FILE);
    fault_inject_reset();
    fi.xattr_watch = 1;
    ea_list(obj, vol, EA_FILE, 4096, &r);
    ea_list(obj, vol, EA_FILE, 4096, &r);
    len = r.len;

    if (r.result != AFP_OK || fi.listxattr_calls != 1) {
        ret = 2;
    }

    if (ret == 0 && next_second(path) != 0) {
        ret = 9;
    }

    /* an outside writer: the new ctime drops the cached names */
    if (ret == 0 && sys_setxattr(path, "d", "dd", 2, 0) != 0) {
        ret = 8;
    }

    ea_list(obj, vol, EA_FILE, 4096, &r);

    if (ret == 0 && (fi.listxattr_calls != 2 || r.len != len + 2)) {
        ret = 3;
    }

    /* this process's own write drops them at once */
    if (ret == 0 && setextattr(obj, vol->v_vid, DIRDID_ROOT, EA_FILE, "e",
                               "ee", 2) != AFP_OK) {
        ret = 4;
    }

    ea_list(obj, vol, EA_FILE, 4096, &r);

    if (ret == 0 && (fi.listxattr_calls != 3 || r.len != len + 4)) {
        ret = 5;
    }

    if (ret == 0 && remextattr(obj, vol->v_vid, DIRDID_ROOT, EA_FILE, "e")
            != AFP_OK) {
        ret = 6;
    }

    ea_list(obj, vol, EA_FILE, 4096, &r);

    if (ret == 0 && (fi.listxattr_calls != 4 || r.len != len + 2)) {
        ret = 7;
    }

    ea_teardown(obj, vol);
    return ret;
}

/*!
 * @brief An outside write within the second of the cached stat drops the
 *        names where the filesystem keeps ctime nanoseconds
 */
int utest_ea_names_subsecond_write(AFPObj *obj, struct vol *vol)
{
    const struct timespec tick = { .tv_sec = 0, .tv_nsec = 2 * 1000 * 1000 };
    struct ea_reply r;
    struct stat before;
    struct stat after;
    char path[MAXPATHLEN + 1];
    size_t len;
    int ret = 0;

    if (!test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, EA_FILE);
    fault_inject_reset();
    fi.xattr_watch = 1;
    ea_list(obj, vol, EA_FILE, 4096, &r);
    len = r.len;

    if (r.result != AFP_OK || fi.listxattr_calls != 1
            || stat(path, &before) != 0) {
        ret = 2;
    }

    /* the kernel stamps files from a coarse clock, so a write on the tick
     * of the cached stat leaves the ctime as it was: write until it moves */
    for (int i = 0; ret == 0 && i < 100; i++) {
        if (sys_setxattr(path, "d", "dd", 2, 0) != 0
                || stat(path, &after) != 0) {
            ret = 2;
        } else if (after.st_ctime != before.st_ctime
                   || atalk_stat_ctime_timespec(&after).tv_nsec
                   != atalk_stat_ctime_timespec(&before).tv_nsec) {
            break;
        }

        nanosleep(&tick, NULL);
    }

    if (ret == 0 && (before.st_ctime != after.st_ctime
                     || atalk_stat_ctime_timespec(&before).tv_nsec
                     == atalk_stat_ctime_timespec(&after).tv_nsec)) {
        /* the second rolled over, or the filesystem keeps no nanoseconds */
        ea_teardown(obj, vol);
        return TEST_SKIP;
    }

    ea_list(obj, vol, EA_FILE, 4096, &r);

    if (ret == 0 && (fi.listxattr_calls != 2 || r.len != len + 2)) {
        ret = 3;
    }

    ea_teardown(obj, vol);
    return ret;
}

/*!
 * @brief The cache is a tier of the entry: an AD invalidation drops it,
 *        and a multi protocol volume never fills it
 */
int utest_ea_names_tier_lifecycle(AFPObj *obj, struct vol *vol)
{
    struct ea_reply r;
    struct dir *entry;
    int flags = vol->v_flags;
    int ret = 0;

    if (!test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    fault_inject_reset();
    fi.xattr_watch = 1;
    ea_list(obj, vol, EA_FILE, 4096, &r);
    ea_list(obj, vol, EA_FILE, 4096, &r);
    entry = ea_entry(vol, EA_FILE);

    if (entry == NULL || fi.listxattr_calls != 1) {
        ret = 2;
    } else {
        dir_modify(vol, entry, &(struct dir_modify_args) {
            .flags = DCMOD_AD_INV,
        });
    }

    ea_list(obj, vol, EA_FILE, 4096, &r);

    if (ret == 0 && fi.listxattr_calls != 2) {
        ret = 3;
    }

    vol->v_flags |= AFPVOL_MULTIPROTO;
    ea_list(obj, vol, EA_FILE, 4096, &r);
    ea_list(obj, vol, EA_FILE, 4096, &r);
    vol->v_flags = flags;

    if (ret == 0 && fi.listxattr_calls != 4) {
        ret = 4;
    }

    ea_teardown(obj, vol);
    return ret;
}

/*!
 * @brief A directory's cached names follow an outside write that FPAccess
 *        sees first: the rights cache records the new ctime through the
 *        entry, not around it
 */
int utest_ea_names_follow_access(AFPObj *obj, struct vol *vol)
{
#ifdef HAVE_ACLS
    struct ea_reply r;
    char path[MAXPATHLEN + 1];
    uint16_t vid = vol->v_vid;
    int checked;
    int ret = 0;

    if (!test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (createdir(obj, vid, DIRDID_ROOT, EA_DIR) != AFP_OK) {
        return 1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, EA_DIR);
    fault_inject_reset();
    fi.xattr_watch = 1;
    ea_list(obj, vol, EA_DIR, 4096, &r);
    ea_list(obj, vol, EA_DIR, 4096, &r);

    if (r.result != AFP_OK || fi.listxattr_calls != 1) {
        ret = 2;
    }

    if (ret == 0 && next_second(path) != 0) {
        ret = 5;
    }

    if (ret == 0 && sys_setxattr(path, "d", "dd", 2, 0) != 0) {
        ret = 3;
    }

    checked = checkaccess(obj, vid, DIRDID_ROOT, EA_DIR, DARWIN_ACE_READ_DATA);

    if (ret == 0 && checked != AFP_OK && checked != AFPERR_ACCESS) {
        /* the filesystem carries no ACLs: the rights are never cached */
        ret = TEST_SKIP;
    }

    ea_list(obj, vol, EA_DIR, 4096, &r);

    if (ret == 0 && (r.result != AFP_OK || fi.listxattr_calls != 2
                     || r.len != 6 + 2)) {
        ret = 4;
    }

    fault_inject_reset();
    (void)delete (obj, vid, DIRDID_ROOT, EA_DIR);
    return ret;
#else
    (void)obj;
    (void)vol;
    return TEST_SKIP;
#endif
}

/*!
 * @brief A refused EA write hints no sibling; a successful one hints the
 *        object
 */
int utest_ea_refused_write_quiet(AFPObj *obj, struct vol *vol)
{
    char hint[64];
    int fds[2];
    int ret = 0;

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    if (hints_begin(obj, fds) != 0) {
        ea_teardown(obj, vol);
        return 2;
    }

    /* a name the file lacks: refused, nothing changed, nobody told */
    if (remextattr(obj, vol->v_vid, DIRDID_ROOT, EA_FILE, "nx") == AFP_OK
            || read(fds[0], hint, sizeof(hint)) != -1 || errno != EAGAIN) {
        ret = 3;
    }

    if (ret == 0 && (setextattr(obj, vol->v_vid, DIRDID_ROOT, EA_FILE, "e",
                                "ee", 2) != AFP_OK
                     || read(fds[0], hint, sizeof(hint)) <= 0)) {
        ret = 4;
    }

    hints_end(obj, fds);
    ea_teardown(obj, vol);
    return ret;
}

/*!
 * @brief An EA write to a directory the user cannot enter drops that
 *        directory's names and leaves the parent's entry alone
 */
int utest_ea_write_closed_dir(AFPObj *obj, struct vol *vol)
{
    struct ea_reply r;
    char path[MAXPATHLEN + 1];
    uint16_t vid = vol->v_vid;
    ino_t root_ino = vol->v_root->dcache_ino;
    int ret = 0;

    if (geteuid() == 0) {
        return TEST_SKIP;
    }

    if (createdir(obj, vid, DIRDID_ROOT, EA_DIR) != AFP_OK) {
        return 1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, EA_DIR);

    if (chmod(path, 0600) != 0) {
        ret = 2;
    }

    ea_list(obj, vol, EA_DIR, 4096, &r);
    ea_list(obj, vol, EA_DIR, 4096, &r);

    if (ret == 0 && (r.result != AFP_OK || r.len != 6)) {
        ret = 3;
    }

    if (ret == 0 && setextattr(obj, vid, DIRDID_ROOT, EA_DIR, "a", "aa", 2)
            != AFP_OK) {
        ret = 4;
    }

    ea_list(obj, vol, EA_DIR, 4096, &r);

    if (ret == 0 && (r.result != AFP_OK || r.len != 6 + 2)) {
        ret = 5;
    }

    if (ret == 0 && vol->v_root->dcache_ino != root_ino) {
        ret = 6;
    }

    (void)chmod(path, 0700);
    (void)delete (obj, vid, DIRDID_ROOT, EA_DIR);
    return ret;
}

/*!
 * @brief A value read once serves the probe and the fetches after it, a
 *        too-small maxreply is refused from the cache with the size, and
 *        a second value is cached beside the first
 */
int utest_ea_values_cached(AFPObj *obj, struct vol *vol)
{
    struct ea_reply probe;
    struct ea_reply first;
    struct ea_reply second;
    struct ea_reply small;
    uint32_t size;
    int ret = 0;

    if (!test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    fault_inject_reset();
    fi.xattr_watch = 1;
    ea_get(obj, vol, EA_FILE, "b", 0, &probe);
    ea_get(obj, vol, EA_FILE, "b", 4096, &first);
    ea_get(obj, vol, EA_FILE, "b", 4096, &second);
    ea_get(obj, vol, EA_FILE, "b", 100, &small);

    if (probe.result != AFP_OK || first.result != AFP_OK
            || fi.getxattr_calls != 1) {
        ret = 2;
    } else if (!ea_same_reply(&first, &second) || first.len != 6 + 300) {
        ret = 3;
    } else if (small.result != AFPERR_PARAM || small.len != 6
               || probe.len != 6 || memcmp(probe.buf, small.buf, 6) != 0) {
        ret = 4;
    } else {
        memcpy(&size, small.buf + 2, sizeof(size));

        if (ntohl(size) != 300) {
            ret = 5;
        }
    }

    ea_get(obj, vol, EA_FILE, "c", 4096, &first);
    ea_get(obj, vol, EA_FILE, "c", 4096, &second);

    if (ret == 0 && (first.result != AFP_OK || fi.getxattr_calls != 2
                     || !ea_same_reply(&first, &second))) {
        ret = 6;
    }

    ea_teardown(obj, vol);
    return ret;
}

/*!
 * @brief Cached values go with an EA write through afpd, and with an
 *        outside write seen through ctime while the AppleDouble tier is
 *        unknown
 */
int utest_ea_values_follow_writes(AFPObj *obj, struct vol *vol)
{
    struct ea_reply r;
    struct dir *entry;
    char path[MAXPATHLEN + 1];
    char b[300];
    int ret = 0;

    if (!test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, EA_FILE);
    fault_inject_reset();
    fi.xattr_watch = 1;
    ea_get(obj, vol, EA_FILE, "b", 4096, &r);
    ea_get(obj, vol, EA_FILE, "b", 4096, &r);

    if (r.result != AFP_OK || fi.getxattr_calls != 1) {
        ret = 2;
    }

    if (ret == 0 && remextattr(obj, vol->v_vid, DIRDID_ROOT, EA_FILE, "a")
            != AFP_OK) {
        ret = 3;
    }

    ea_get(obj, vol, EA_FILE, "b", 4096, &r);

    if (ret == 0 && (r.result != AFP_OK || fi.getxattr_calls != 2)) {
        ret = 4;
    }

    /* the AppleDouble tier unknown, as after a sibling's hint: a value read
     * then and rewritten outside afpd is read again */
    entry = ea_entry(vol, EA_FILE);

    if (entry == NULL) {
        ret = 5;
    } else {
        dir_modify(vol, entry, &(struct dir_modify_args) {
            .flags = DCMOD_AD_INV,
        });
    }

    ea_get(obj, vol, EA_FILE, "b", 4096, &r);

    if (ret == 0 && next_second(path) != 0) {
        ret = 8;
    }

    memset(b, 'B', sizeof(b));

    if (ret == 0 && sys_setxattr(path, "b", b, sizeof(b), 0) != 0) {
        ret = 6;
    }

    ea_get(obj, vol, EA_FILE, "b", 4096, &r);

    if (ret == 0 && (r.result != AFP_OK || fi.getxattr_calls != 4
                     || r.len != 6 + sizeof(b)
                     || memcmp(r.buf + 6, b, sizeof(b)) != 0)) {
        ret = 7;
    }

    ea_teardown(obj, vol);
    return ret;
}

/*!
 * @brief Did the counters grow by exactly these amounts since the snapshot?
 */
static bool ea_counted(const struct ea_cache_stats *before,
                       unsigned long long hits, unsigned long long misses,
                       unsigned long long stored, unsigned long long oversize,
                       unsigned long long dropped)
{
    struct ea_cache_stats d = ea_cache_stat;
    return d.hits - before->hits == hits
           && d.misses - before->misses == misses
           && d.stored - before->stored == stored
           && d.oversize - before->oversize == oversize
           && d.dropped - before->dropped == dropped;
}

/*!
 * @brief The statistics count a names store and hit, two value stores
 *        and a hit, and a drop
 */
int utest_ea_cache_counted(AFPObj *obj, struct vol *vol)
{
    struct ea_cache_stats before;
    struct ea_reply r;
    int ret = 0;

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    before = ea_cache_stat;
    ea_list(obj, vol, EA_FILE, 4096, &r);
    ea_list(obj, vol, EA_FILE, 4096, &r);
    ea_get(obj, vol, EA_FILE, "b", 0, &r);
    ea_get(obj, vol, EA_FILE, "b", 4096, &r);
    ea_get(obj, vol, EA_FILE, "c", 4096, &r);

    if (r.result != AFP_OK
            || remextattr(obj, vol->v_vid, DIRDID_ROOT, EA_FILE, "a")
            != AFP_OK) {
        ret = 2;
    }

    /* names: miss and store, hit; b: miss and store, hit; c: miss and
     * store; the removal: one drop */
    if (ret == 0 && !ea_counted(&before, 2, 3, 3, 0, 1)) {
        ret = 3;
    }

    ea_teardown(obj, vol);
    return ret;
}

/*!
 * @brief Values fill an entry up to its cap; a value the entry has no
 *        room for is read on every request
 */
int utest_ea_values_cap(AFPObj *obj, struct vol *vol)
{
    static char d[600];
    struct ea_cache_stats before;
    struct ea_reply r;
    char path[MAXPATHLEN + 1];
    int ret = 0;

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, EA_FILE);
    memset(d, 'd', sizeof(d));

    if (sys_setxattr(path, "d", d, sizeof(d), 0) != 0) {
        ea_teardown(obj, vol);
        return 2;
    }

    before = ea_cache_stat;
    ea_get(obj, vol, EA_FILE, "b", 4096, &r);
    ea_get(obj, vol, EA_FILE, "c", 4096, &r);
    ea_get(obj, vol, EA_FILE, "d", 4096, &r);
    ea_get(obj, vol, EA_FILE, "d", 4096, &r);

    if (r.result != AFP_OK || r.len != 6 + sizeof(d)) {
        ret = 3;
    }

    ea_get(obj, vol, EA_FILE, "c", 4096, &r);

    /* b and c: miss and store; d: miss, no room, twice; c: hit */
    if (ret == 0 && !ea_counted(&before, 1, 4, 2, 2, 0)) {
        ret = 4;
    }

    ea_teardown(obj, vol);
    return ret;
}

/*!
 * @brief A name the list hides is afpd's own, written behind the entry:
 *        its value is read on every request
 */
int utest_ea_hidden_name_uncached(AFPObj *obj, struct vol *vol)
{
    struct ea_reply first;
    struct ea_reply second;
    char path[MAXPATHLEN + 1];
    int ret = 0;

    if (!test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, EA_FILE);

    if (sys_setxattr(path, AD_EA_RESO, "rsrc", 4, 0) != 0) {
        ret = 2;
    }

    fault_inject_reset();
    fi.xattr_watch = 1;
    ea_get(obj, vol, EA_FILE, AD_EA_RESO, 4096, &first);
    ea_get(obj, vol, EA_FILE, AD_EA_RESO, 4096, &second);

    if (ret == 0 && (first.result != AFP_OK || first.len != 6 + 4
                     || fi.getxattr_calls != 2
                     || !ea_same_reply(&first, &second))) {
        ret = 3;
    }

    ea_teardown(obj, vol);
    return ret;
}

/*!
 * @brief A directory whose search bit went after the lookup is addressed
 *        by name from its parent, where of_statdir() leaves the cwd
 */
int utest_ea_dir_lost_search_bit(AFPObj *obj, struct vol *vol)
{
    struct path p = { .m_name = "", .u_name = "." };
    char path[MAXPATHLEN + 1];
    char twin[MAXPATHLEN + 1];
    struct stat root;
    uint16_t vid = vol->v_vid;
    struct dir *dir;
    int flags = vol->v_flags;
    int ret = 0;

    if (geteuid() == 0) {
        return TEST_SKIP;
    }

    if (createdir(obj, vid, DIRDID_ROOT, EA_DIR) != AFP_OK
            || getfiledirparms(obj, vid, DIRDID_ROOT, EA_DIR) != AFP_OK) {
        return 1;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, EA_DIR);
    dir = dircache_search_by_name(vol, vol->v_root, EA_DIR, strlen(EA_DIR));

    if (dir == NULL || movecwd(vol, dir) != 0 || chmod(path, 0600) != 0) {
        ret = 2;
    }

    p.d_dir = dir;

    if (ret == 0 && (ea_cache_entry(vol, &p, 0, NULL) != dir
                     || strcmp(p.u_name, EA_DIR) != 0
                     || curdir != vol->v_root)) {
        ret = 3;
    }

    /* a volume the cache never serves: the list handler's own stat moved
     * the cwd the same way, and the name must follow it there too */
    vol->v_flags |= AFPVOL_MULTIPROTO;
    p = (struct path) {
        .m_name = "", .u_name = ".", .d_dir = dir
    };

    if (ret == 0 && (chmod(path, 0700) != 0 || movecwd(vol, dir) != 0
                     || chmod(path, 0600) != 0 || of_statdir(vol, &p) != 0
                     || ea_cache_entry(vol, &p, 0, NULL) != NULL
                     || strcmp(p.u_name, EA_DIR) != 0)) {
        ret = 6;
    }

    vol->v_flags = flags;
    (void)chmod(path, 0700);
    /* the root's fallback stats by name inside the root itself: a child
     * spelled like the root must not stand in for it */
    snprintf(twin, sizeof(twin), "%s/%s", vol->v_path,
             strrchr(vol->v_path, '/') + 1);
    p = (struct path) {
        .m_name = "", .u_name = ".", .d_dir = vol->v_root
    };

    if (ret == 0 && (stat(vol->v_path, &root) != 0
                     || mkdir(twin, 0700) != 0)) {
        ret = 4;
    } else if (ret == 0) {
        if (movecwd(vol, vol->v_root) != 0
                || chmod(vol->v_path, root.st_mode & 0600) != 0) {
            ret = 4;
        } else if (ea_cache_entry(vol, &p, 0, NULL) != NULL
                   || strcmp(p.u_name, ".") != 0) {
            ret = 5;
        }

        (void)chmod(vol->v_path, root.st_mode & 07777);
        (void)rmdir(twin);
    }

    (void)movecwd(vol, vol->v_root);
    (void)delete (obj, vid, DIRDID_ROOT, EA_DIR);
    return ret;
}

/*!
 * @brief A value over MAX_EA_SIZE, which an outside writer alone can
 *        store, probes as the cap and is refused with it
 */
int utest_ea_oversize_value_refused(AFPObj *obj, struct vol *vol)
{
    static char big[MAX_EA_SIZE + 1];
    struct ea_reply probe;
    struct ea_reply fetch;
    char path[MAXPATHLEN + 1];
    uint16_t vid = vol->v_vid;
    uint32_t psize;
    uint32_t fsize;
    int fd;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, EA_BIG);
    fd = open(path, O_WRONLY | O_CREAT, 0600);

    if (fd == -1) {
        return 1;
    }

    close(fd);
    memset(big, 'x', sizeof(big));

    if (sys_setxattr(path, "x", big, sizeof(big), 0) != 0
            || getfiledirparms(obj, vid, DIRDID_ROOT, EA_BIG) != AFP_OK) {
        (void)delete (obj, vid, DIRDID_ROOT, EA_BIG);
        return 2;
    }

    ea_get(obj, vol, EA_BIG, "x", 0, &probe);
    ea_get(obj, vol, EA_BIG, "x", 4096, &fetch);
    (void)delete (obj, vid, DIRDID_ROOT, EA_BIG);
    memcpy(&psize, probe.buf + 2, sizeof(psize));
    memcpy(&fsize, fetch.buf + 2, sizeof(fsize));

    if (probe.result != AFP_OK || probe.len != 6
            || ntohl(psize) != MAX_EA_SIZE) {
        return 3;
    }

    return fetch.result == AFPERR_PARAM && fetch.len == 6
           && ntohl(fsize) == MAX_EA_SIZE ? 0 : 4;
}

/*!
 * @brief After a list, a probe and a fetch of a name the file lacks are
 *        answered from the cached names without a read, for a file without
 *        attributes too; a name outside ASCII still reads and a protected
 *        name is still refused
 */
int utest_ea_absent_name_from_list(AFPObj *obj, struct vol *vol)
{
    struct ea_cache_stats before;
    struct ea_reply list;
    struct ea_reply probe;
    struct ea_reply fetch;
    int reads;
    int ret = 0;

    if (!test_capability(TEST_CAP_XATTR_WATCH, vol)) {
        return TEST_SKIP;
    }

    if (ea_setup(obj, vol) != 0) {
        return 1;
    }

    fault_inject_reset();
    fi.xattr_watch = 1;
    ea_list(obj, vol, EA_FILE, 4096, &list);
    reads = fi.getxattr_calls;
    before = ea_cache_stat;
    ea_get(obj, vol, EA_FILE, "missing", 0, &probe);
    ea_get(obj, vol, EA_FILE, "missing", 4096, &fetch);

    if (list.result != AFP_OK || probe.result != AFPERR_NOITEM
            || probe.len != 6 || !ea_same_reply(&probe, &fetch)) {
        ret = 2;
    } else if (fi.getxattr_calls != reads
               || ea_cache_stat.absent - before.absent != 2) {
        ret = 3;
    }

    ea_get(obj, vol, EA_FILE, "caf\xc3\xa9", 0, &probe);

    if (ret == 0 && (probe.result != AFPERR_NOITEM
                     || fi.getxattr_calls != reads + 1)) {
        ret = 4;
    }

    ea_get(obj, vol, EA_FILE, "org.netatalk.Metadata.x", 0, &probe);

    if (ret == 0 && probe.result != AFPERR_ACCESS) {
        ret = 6;
    }

    ea_list(obj, vol, EA_BARE, 4096, &list);
    reads = fi.getxattr_calls;
    ea_get(obj, vol, EA_BARE, "missing", 0, &probe);

    if (ret == 0 && (probe.result != AFPERR_NOITEM
                     || fi.getxattr_calls != reads)) {
        ret = 5;
    }

    ea_teardown(obj, vol);
    return ret;
}
