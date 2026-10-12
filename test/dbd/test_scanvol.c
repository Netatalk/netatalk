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
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
