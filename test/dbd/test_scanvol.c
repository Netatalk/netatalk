/*
 * Copyright (c) 2026 Andy Lemin (andylemin)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*!
 * @file
 * @brief Unit tests of the dbd volume scan under the afpd fault injector
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/compat.h>
#include <atalk/logger.h>
#include <atalk/unicode.h>
#include <atalk/util.h>
#include <atalk/vfs.h>
#include <atalk/volume.h>

#include "cmd_dbd.h"
#include "faultinject.h"

/* The scan reads this between directory entries, as the dbd signal handlers
 * set it */
volatile sig_atomic_t alarmed;

static int tests_run;
static int tests_failed;

/* Scratch volume: the share and the CNID database in two subdirectories of
 * one mkdtemp directory, so the scan never meets the database file */
struct dbd_scratch {
    char base[MAXPATHLEN];
    char share[MAXPATHLEN];
    char dbdir[MAXPATHLEN];
    char dbfile[MAXPATHLEN];
    char uuid[37];
    struct vol vol;
};

/*! @brief Print one TAP result line */
static void report(int passed, const char *name)
{
    tests_run++;
    printf("%s %d - %s\n", passed ? "ok" : "not ok", tests_run, name);

    if (!passed) {
        tests_failed++;
    }
}

/*! @brief Print the TAP line of a case that cannot run here */
static void skip(const char *name, const char *why)
{
    tests_run++;
    printf("ok %d - %s # SKIP %s\n", tests_run, name, why);
}

/*!
 * @brief Print a dbd log line as a TAP diagnostic
 *
 * The test build stands in for the dbd program, whose dbd_log prints to
 * stdout.
 */
void dbd_log(enum logtype lt, const char *fmt, ...)
{
    va_list args;

    if (lt != LOGSTD) {
        return;
    }

    printf("# ");
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

/*!
 * @brief Create the scratch volume with an empty SQLite CNID database
 *
 * @returns 0, or -1 when a directory or the database cannot be made
 */
static int scratch_setup(struct dbd_scratch *s)
{
    char made[MAXPATHLEN];
    int len = snprintf(made, sizeof(made), "%s/netatalk-dbd-XXXXXX",
                       tmpdir());
    memset(s, 0, sizeof(*s));

    if (len <= 0 || (size_t)len >= sizeof(made) || mkdtemp(made) == NULL) {
        return -1;
    }

    /* canonical like a loaded volume's path, which the backend compares
     * with its resolved database directory */
    if (realpath(made, s->base) == NULL) {
        rmdir(made);
        return -1;
    }

    snprintf(s->share, sizeof(s->share), "%s/share", s->base);
    snprintf(s->dbdir, sizeof(s->dbdir), "%s/db", s->base);

    if (mkdir(s->share, 0700) != 0 || mkdir(s->dbdir, 0700) != 0) {
        return -1;
    }

    snprintf(s->uuid, sizeof(s->uuid), "%s",
             "9B1D7E6A-5C3F-4A2B-8D0E-1F2A3B4C5D6E");
    s->vol.v_path = s->share;
    s->vol.v_dbpath = s->dbdir;
    s->vol.v_localname = "dbdtest";
    s->vol.v_uuid = s->uuid;
    s->vol.v_adouble = AD_VERSION2;
    s->vol.v_vfs_ea = AFPVOL_EA_AD;
    s->vol.v_volcharset = CH_UTF8;
    s->vol.v_maccharset = CH_MAC;
    s->vol.v_volcodepage = "UTF8";
    snprintf(s->dbfile, sizeof(s->dbfile), "%s/%s.sqlite", s->dbdir,
             s->vol.v_localname);
    initvol_vfs(&s->vol);
    s->vol.v_cdb = cnid_open(&s->vol, "sqlite", 0);
    return s->vol.v_cdb != NULL ? 0 : -1;
}

/*!
 * @brief Close the scratch volume and remove what the test and the scan made
 */
static void scratch_cleanup(struct dbd_scratch *s)
{
    const char *suffixes[] = { "-wal", "-shm", "-journal" };
    char path[MAXPATHLEN];

    if (s->vol.v_cdb != NULL) {
        cnid_close(s->vol.v_cdb);
        s->vol.v_cdb = NULL;
    }

    if (s->base[0] == '\0') {
        return;
    }

    for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
        snprintf(path, sizeof(path), "%s%s", s->dbfile, suffixes[i]);
        unlink(path);
    }

    unlink(s->dbfile);
    rmdir(s->dbdir);
    snprintf(path, sizeof(path), "%s/%s/sample", s->share, ADv2_DIRNAME);
    unlink(path);
    snprintf(path, sizeof(path), "%s/sample", s->share);
    unlink(path);
    snprintf(path, sizeof(path), "%s/%s/.Parent", s->share, ADv2_DIRNAME);
    unlink(path);
    snprintf(path, sizeof(path), "%s/%s", s->share, ADv2_DIRNAME);
    rmdir(path);
    rmdir(s->share);
    rmdir(s->base);
}

/*!
 * @brief A repair whose new .AppleDouble cannot be opened again fails
 *
 * On a directory without .AppleDouble the scan opens that name twice with
 * O_DIRECTORY. The first open finds it missing, and the second reopens it
 * after ad_open created it. The injector fails the second open with EACCES.
 * The created .Parent proves that the failure hit the reopen, and the scan
 * has to report an error.
 */
static void test_new_addir_reopen_failure(void)
{
    const char *name = "a repair that cannot reopen its new .AppleDouble fails";
    struct dbd_scratch s;
    char path[MAXPATHLEN];
    char origcwd[MAXPATHLEN];
    struct stat st;
    int ret;
    int fired;
    int calls;
    int created;

    if (scratch_setup(&s) < 0 || getcwd(origcwd, sizeof(origcwd)) == NULL) {
        printf("Bail out! cannot create the scratch volume: %s\n",
               strerror(errno));
        scratch_cleanup(&s);
        exit(1);
    }

    snprintf(path, sizeof(path), "%s/probe", s.base);

    if (!faultinject_open_works(path)) {
        scratch_cleanup(&s);
        skip(name, "fault injection inactive");
        return;
    }

    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_flags = O_DIRECTORY;
    fi.open_fail_after = 1;
    fi.open_errno = EACCES;
    ret = cmd_dbd_scanvol(&s.vol, 0);
    fired = (fi.open_fail_after == -1);
    calls = fi.open_calls;
    fault_inject_reset();
    /* the scan moved into the share */
    (void)chdir(origcwd);
    snprintf(path, sizeof(path), "%s/%s/.Parent", s.share, ADv2_DIRNAME);
    created = (stat(path, &st) == 0);
    scratch_cleanup(&s);

    if (!fired) {
        printf("# the injected open() never fired, %d directory opens seen\n",
               calls);
    } else if (!created) {
        printf("# .AppleDouble/.Parent is missing: the failure hit the first "
               "open, not the reopen\n");
    } else if (ret != -1) {
        printf("# cmd_dbd_scanvol returned %d after the failed reopen\n", ret);
    }

    report(fired && created && ret == -1, name);
}

/*! @brief Create a data file and valid v2 metadata with a known CNID hint */
static int scratch_metadata(struct dbd_scratch *s)
{
    struct adouble ad;
    struct stat st;
    char path[MAXPATHLEN];
    /* SQLite returns a string; ad_setid copies the full fixed-width field. */
    char stamp[ADEDLEN_PRIVSYN] = {0};
    int fd, ret;
    ad_init(&ad, &s->vol);

    if (ad_open(&ad, s->share, ADFLAGS_HF | ADFLAGS_DIR | ADFLAGS_RDWR
                | ADFLAGS_CREATE, 0700) != 0) {
        return -1;
    }

    ad_close(&ad, ADFLAGS_HF);
    snprintf(path, sizeof(path), "%s/sample", s->share);
    fd = open(path, O_CREAT | O_RDWR | O_EXCL, 0600);

    if (fd < 0) {
        return -1;
    }

    ret = fstat(fd, &st);
    close(fd);

    if (ret != 0 || cnid_getstamp(s->vol.v_cdb, stamp, sizeof(stamp)) != 0) {
        return -1;
    }

    ad_init(&ad, &s->vol);

    if (ad_open(&ad, path, ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                0600) != 0) {
        return -1;
    }

    ret = ad_setid(&ad, st.st_dev, st.st_ino, htonl(1000), htonl(2), stamp);

    if (ret == 1) {
        ret = ad_flush(&ad);
    } else {
        errno = EINVAL;
        ret = -1;
    }

    ad_close(&ad, ADFLAGS_HF);
    return ret;
}

/*! @brief Read the metadata bytes and stat for before/after comparisons */
static ssize_t metadata_snapshot(const char *path, char *buf, struct stat *st)
{
    int fd = open(path, O_RDONLY);
    ssize_t len = -1;

    if (fd >= 0) {
        if (fstat(fd, st) == 0) {
            len = read(fd, buf, AD_DATASZ2);
        }

        close(fd);
    }

    return len;
}

/*! @brief Compare metadata content and modification times, ignoring read atime */
static int metadata_unchanged(const char *path, const char *before,
                              ssize_t before_len, const struct stat *before_st)
{
    char after[AD_DATASZ2];
    struct stat after_st;
    struct timespec before_mtime, after_mtime;
    ssize_t after_len = metadata_snapshot(path, after, &after_st);

    if (before_len < 0 || after_len != before_len) {
        return 0;
    }

    before_mtime = atalk_stat_mtime_timespec(before_st);
    after_mtime = atalk_stat_mtime_timespec(&after_st);
    return before_st->st_size == after_st.st_size
           && before_st->st_ctime == after_st.st_ctime
           && before_mtime.tv_sec == after_mtime.tv_sec
           && before_mtime.tv_nsec == after_mtime.tv_nsec
           && memcmp(before, after, (size_t)before_len) == 0;
}

/*! @brief Strict reads use O_RDONLY, while ordinary v2 opens retain O_RDWR */
static void test_strict_metadata(void)
{
    const int conflicting[] = {
        ADFLAGS_RDWR, ADFLAGS_CREATE, ADFLAGS_TRUNC, ADFLAGS_EXCL,
        ADFLAGS_SETSHRMD, ADFLAGS_CHECK_OF, ADFLAGS_DF, ADFLAGS_RF,
    };
    struct dbd_scratch s;
    struct adouble ad;
    struct stat st;
    char path[MAXPATHLEN];
    int passed = 1;

    if (scratch_setup(&s) < 0 || scratch_metadata(&s) != 0) {
        printf("Bail out! cannot create metadata: %s\n", strerror(errno));
        scratch_cleanup(&s);
        exit(1);
    }

    snprintf(path, sizeof(path), "%s/sample", s.share);
    ad_init(&ad, &s.vol);

    if (ad_open(&ad, path, ADFLAGS_HF | ADFLAGS_RDONLY_STRICT) != 0) {
        passed = 0;
    } else {
        passed = (fcntl(ad_meta_fileno(&ad), F_GETFL) & O_ACCMODE) == O_RDONLY;
        /* A strict handle cannot be reused for a write upgrade. */
        passed &= ad_open(&ad, path, ADFLAGS_HF | ADFLAGS_RDWR) == -1
                  && errno == EINVAL;
        ad_close(&ad, ADFLAGS_HF);
    }

    ad_init(&ad, &s.vol);

    if (ad_open(&ad, path, ADFLAGS_HF | ADFLAGS_RDONLY) != 0) {
        passed = 0;
    } else {
        passed &= (fcntl(ad_meta_fileno(&ad), F_GETFL) & O_ACCMODE) == O_RDWR;
        /* A strict request must not inherit an existing writable handle. */
        passed &= ad_open(&ad, path, ADFLAGS_HF | ADFLAGS_RDONLY_STRICT) == -1
                  && errno == EINVAL;
        ad_close(&ad, ADFLAGS_HF);
    }

    for (size_t i = 0; i < sizeof(conflicting) / sizeof(conflicting[0]); i++) {
        ad_init(&ad, &s.vol);
        passed &= ad_open(&ad, path, ADFLAGS_HF | ADFLAGS_RDONLY_STRICT
                          | conflicting[i]) == -1 && errno == EINVAL;
        passed &= ad_meta_fileno(&ad) == -1 && ad.ad_meta_refcount == 0;
    }

    report(passed,
           "strict metadata opens are read-only and preserve ordinary v2 promotion");
    snprintf(path, sizeof(path), "%s/%s/sample", s.share, ADv2_DIRNAME);
    passed = truncate(path, 0) == 0;
    snprintf(path, sizeof(path), "%s/sample", s.share);
    ad_init(&ad, &s.vol);

    if (ad_open(&ad, path, ADFLAGS_HF | ADFLAGS_RDONLY_STRICT) == 0) {
        passed = 0;
        ad_close(&ad, ADFLAGS_HF);
    } else {
        passed &= errno == EIO && ad_meta_fileno(&ad) == -1
                  && ad.ad_meta_refcount == 0 && ad.ad_mdp->adf_refcount == 0;
    }

    snprintf(path, sizeof(path), "%s/%s/sample", s.share, ADv2_DIRNAME);
    passed &= stat(path, &st) == 0 && st.st_size == 0;
    report(passed,
           "strict reads reject empty v2 metadata and close the failed open");
    scratch_cleanup(&s);
}

/*! @brief Both scan modes preserve writable empty and read-only valid sidecars */
static void test_scan_metadata(int empty, dbd_flags_t flags, const char *name)
{
    struct dbd_scratch s;
    char path[MAXPATHLEN], origcwd[MAXPATHLEN], before[AD_DATASZ2];
    struct stat before_st;
    struct timespec times[2] = {{946684800, 0}, {946684800, 0}};
    ssize_t before_len;
    cnid_t id;
    int ret, passed;

    if (scratch_setup(&s) < 0 || scratch_metadata(&s) != 0
            || getcwd(origcwd, sizeof(origcwd)) == NULL) {
        printf("Bail out! cannot create metadata: %s\n", strerror(errno));
        scratch_cleanup(&s);
        exit(1);
    }

    snprintf(path, sizeof(path), "%s/%s/sample", s.share, ADv2_DIRNAME);

    if ((empty && truncate(path, 0) != 0)
            || utimensat(AT_FDCWD, path, times, 0) != 0
            || chmod(path, empty ? 0600 : 0444) != 0) {
        printf("Bail out! cannot prepare metadata: %s\n", strerror(errno));
        scratch_cleanup(&s);
        exit(1);
    }

    before_len = metadata_snapshot(path, before, &before_st);
    ret = cmd_dbd_scanvol(&s.vol, DBD_FLAGS_SCAN | flags);
    passed = chdir(origcwd) == 0 && ret == 0
             && metadata_unchanged(path, before, before_len, &before_st);
    id = cnid_get(s.vol.v_cdb, htonl(2), "sample", strlen("sample"));

    if (!empty) {
        /* Prove the entry was processed, including its hint when -i is absent. */
        passed &= id != CNID_INVALID
                  && ((flags & DBD_FLAGS_STRIP_AD) || id == htonl(1000));
    }

    scratch_cleanup(&s);
    report(passed, name);
}

/*! @brief Ordinary maintenance repairs empty headers and mismatched CNID hints */
static void test_metadata_repair(int empty)
{
    struct dbd_scratch s;
    struct adouble ad;
    struct stat st = {0};
    char path[MAXPATHLEN], origcwd[MAXPATHLEN], stamp[ADEDLEN_PRIVSYN];
    cnid_t id;
    int passed;

    if (scratch_setup(&s) < 0 || scratch_metadata(&s) != 0
            || getcwd(origcwd, sizeof(origcwd)) == NULL) {
        printf("Bail out! cannot create metadata: %s\n", strerror(errno));
        scratch_cleanup(&s);
        exit(1);
    }

    snprintf(path, sizeof(path), "%s/sample", s.share);
    passed = stat(path, &st) == 0;
    /* The database takes precedence over the fixture's stale hint of 1000. */
    id = cnid_add(s.vol.v_cdb, &st, htonl(2), "sample", strlen("sample"),
                  htonl(2000));
    passed &= id == htonl(2000);
    snprintf(path, sizeof(path), "%s/%s/sample", s.share, ADv2_DIRNAME);
    passed &= !empty || truncate(path, 0) == 0;
    passed &= cmd_dbd_scanvol(&s.vol, 0) == 0;
    passed &= chdir(origcwd) == 0;
    passed &= stat(path, &st) == 0 && st.st_size > 0;
    snprintf(path, sizeof(path), "%s/sample", s.share);
    passed &= stat(path, &st) == 0;
    passed &= cnid_get(s.vol.v_cdb, htonl(2), "sample", strlen("sample")) == id;
    cnid_getstamp(s.vol.v_cdb, stamp, sizeof(stamp));
    ad_init(&ad, &s.vol);

    if (ad_open(&ad, path, ADFLAGS_HF | ADFLAGS_RDONLY_STRICT) != 0) {
        passed = 0;
    } else {
        passed &= ad_getid(&ad, st.st_dev, st.st_ino, htonl(2), stamp) == id;
        ad_close(&ad, ADFLAGS_HF);
    }

    scratch_cleanup(&s);
    report(passed, empty ? "ordinary maintenance repairs empty v2 metadata"
           : "ordinary maintenance replaces a stale hint with the database CNID");
}

/*!
 * @brief Run the dbd scan tests and print their TAP results
 *
 * The charsets and the CNID backends register as they do in dbd, where
 * afp.conf supplies the charset names, and libatalk errors go to stderr.
 *
 * @returns 0 when every test passes or skips, or 1
 */
int main(void)
{
    printf("TAP version 13\n");
    set_processname("dbdtest");
    setuplog("default:error", "/dev/stderr", false);

    if (set_charset_name(CH_UNIX, "UTF8") != 0
            || set_charset_name(CH_MAC, "MAC_ROMAN") != 0) {
        printf("Bail out! cannot initialize built-in charsets\n");
        return 1;
    }

    cnid_init();
    test_new_addir_reopen_failure();
    test_strict_metadata();
    test_scan_metadata(1, 0, "-s leaves empty v2 metadata unchanged");
    test_scan_metadata(1, DBD_FLAGS_STRIP_AD,
                       "-s -i leaves empty v2 metadata unchanged");
    test_scan_metadata(0, 0,
                       "-s reads a CNID hint from read-only v2 metadata without changing it");
    test_scan_metadata(0, DBD_FLAGS_STRIP_AD,
                       "-s -i leaves valid read-only v2 metadata unchanged");
    test_metadata_repair(1);
    test_metadata_repair(0);
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
