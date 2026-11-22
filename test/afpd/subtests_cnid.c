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
 * @brief Component tests of the CNID databases, their name searches and volume identity
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <sqlite3.h>

#include <atalk/cnid.h>
#include <atalk/cnid_sqlite_private.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/sl_names.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afpfunc_helpers.h"
#include "desktop.h"
#include "faultinject.h"
#include "subtests_cnid.h"
#include "subtests_conf.h"
#include "test.h"
#include "test_capabilities.h"
#include "volume.h"

/*
 * A CNID path is predictable and its directory is intentionally writable by
 * local users. Verify that neither the main SQLite path nor a companion path
 * can redirect the backend's permission change to another file.
 */
int utest_cnid_sqlite_symlinks_rejected(void)
{
    char base[MAXPATHLEN];
    char main_dir[MAXPATHLEN];
    char aux_dir[MAXPATHLEN];
    char sentinel[MAXPATHLEN];
    char main_link[MAXPATHLEN];
    char aux_db[MAXPATHLEN];
    char aux_wal[MAXPATHLEN];
    char aux_shm[MAXPATHLEN];
    char aux_journal[MAXPATHLEN];
    char uuid_main[] = "11111111-2222-3333-4444-555555555555";
    char uuid_aux[] = "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE";
    const char sentinel_data[] = "netatalk-sentinel";
    struct _cnid_db *cdb = NULL;
    struct stat st;
    struct vol vol;
    sqlite3 *seed = NULL;
    int fd = -1;
    int result = 1;
    int len;
    len = snprintf(base, sizeof(base),
                   "%s/netatalk-cnid-symlink-XXXXXX", tmpdir());

    if (len <= 0 || (size_t)len >= sizeof(base) || mkdtemp(base) == NULL) {
        return TEST_SKIP;
    }

    if (snprintf(main_dir, sizeof(main_dir), "%s/main", base)
            >= sizeof(main_dir)
            || snprintf(aux_dir, sizeof(aux_dir), "%s/aux", base)
            >= sizeof(aux_dir)
            || snprintf(sentinel, sizeof(sentinel), "%s/sentinel", base)
            >= sizeof(sentinel)
            || snprintf(main_link, sizeof(main_link), "%s/main.sqlite", main_dir)
            >= sizeof(main_link)
            || snprintf(aux_db, sizeof(aux_db), "%s/aux.sqlite", aux_dir)
            >= sizeof(aux_db)
            || snprintf(aux_wal, sizeof(aux_wal), "%s-wal", aux_db)
            >= sizeof(aux_wal)
            || snprintf(aux_shm, sizeof(aux_shm), "%s-shm", aux_db)
            >= sizeof(aux_shm)
            || snprintf(aux_journal, sizeof(aux_journal), "%s-journal", aux_db)
            >= sizeof(aux_journal)) {
        result = 2;
        goto cleanup;
    }

    if (mkdir(main_dir, 0700) != 0 || mkdir(aux_dir, 0700) != 0) {
        result = 3;
        goto cleanup;
    }

    fd = open(sentinel, O_RDWR | O_CREAT | O_EXCL, 0600);

    if (fd < 0
            || write(fd, sentinel_data, sizeof(sentinel_data) - 1)
            != sizeof(sentinel_data) - 1
            || fchmod(fd, 0600) != 0) {
        result = 4;
        goto cleanup;
    }

    if (symlink(sentinel, main_link) != 0) {
        result = 5;
        goto cleanup;
    }

    memset(&vol, 0, sizeof(vol));
    vol.v_dbpath = main_dir;
    vol.v_localname = "main";
    vol.v_path = main_dir;
    vol.v_uuid = uuid_main;
    cdb = cnid_open(&vol, "sqlite", 0);

    if (cdb != NULL) {
        cnid_close(cdb);
        cdb = NULL;
        result = 6;
        goto cleanup;
    }

    if (fstat(fd, &st) != 0 || (st.st_mode & 07777) != 0600
            || st.st_size != sizeof(sentinel_data) - 1) {
        result = 7;
        goto cleanup;
    }

    if (sqlite3_open_v2(aux_db, &seed,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                        NULL) != SQLITE_OK) {
        result = 8;
        goto cleanup;
    }

    if (sqlite3_close(seed) != SQLITE_OK) {
        result = 8;
        goto cleanup;
    }

    seed = NULL;

    if (chmod(aux_db, 0666) != 0 || symlink(sentinel, aux_wal) != 0) { //NOSONAR: granting permissions to 'others' is intentional
        result = 9;
        goto cleanup;
    }

    memset(&vol, 0, sizeof(vol));
    vol.v_dbpath = aux_dir;
    vol.v_localname = "aux";
    vol.v_path = aux_dir;
    vol.v_uuid = uuid_aux;
    cdb = cnid_open(&vol, "sqlite", 0);

    if (cdb != NULL) {
        cnid_close(cdb);
        cdb = NULL;
        result = 10;
        goto cleanup;
    }

    if (fstat(fd, &st) != 0 || (st.st_mode & 07777) != 0600
            || st.st_size != sizeof(sentinel_data) - 1) {
        result = 11;
        goto cleanup;
    }

    if (unlink(aux_wal) != 0 || symlink(sentinel, aux_shm) != 0) {
        result = 12;
        goto cleanup;
    }

    cdb = cnid_open(&vol, "sqlite", 0);

    if (cdb != NULL) {
        cnid_close(cdb);
        cdb = NULL;
        result = 13;
        goto cleanup;
    }

    if (fstat(fd, &st) != 0 || (st.st_mode & 07777) != 0600
            || st.st_size != sizeof(sentinel_data) - 1) {
        result = 14;
        goto cleanup;
    }

    result = 0;
cleanup:

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    if (seed != NULL) {
        sqlite3_close(seed);
    }

    if (fd >= 0) {
        close(fd);
    }

    unlink(main_link);
    unlink(aux_wal);
    unlink(aux_shm);
    unlink(aux_journal);
    unlink(aux_db);
    unlink(sentinel);
    rmdir(main_dir);
    rmdir(aux_dir);
    rmdir(base);
    return result;
}

/* Scratch volume for cnid_open() tests: a mkdtemp directory serving as share
 * and, unless cnid_scratch_separate_db() moves it, as database directory */
struct cnid_scratch {
    char base[MAXPATHLEN];
    char dbdir[MAXPATHLEN];
    char dbfile[MAXPATHLEN];
    char uuid[37];
    struct vol vol;
};

/*!
 * @brief Create the scratch volume; -1 when the directory cannot be made
 */
static int cnid_scratch_setup(struct cnid_scratch *s, const char *tag,
                              char *localname, const char *uuid)
{
    char made[MAXPATHLEN];
    int len = snprintf(made, sizeof(made), "%s/netatalk-cnid-%s-XXXXXX",
                       tmpdir(), tag);

    if (len <= 0 || (size_t)len >= sizeof(made) || mkdtemp(made) == NULL) {
        return -1;
    }

    /* canonical like a loaded volume's path, which the backend compares
     * with its resolved database directory */
    if (realpath(made, s->base) == NULL) {
        rmdir(made);
        return -1;
    }

    s->dbdir[0] = '\0';
    snprintf(s->dbfile, sizeof(s->dbfile), "%s/%s.sqlite", s->base, localname);
    snprintf(s->uuid, sizeof(s->uuid), "%s", uuid);
    memset(&s->vol, 0, sizeof(s->vol));
    s->vol.v_dbpath = s->base;
    s->vol.v_localname = localname;
    s->vol.v_path = s->base;
    s->vol.v_uuid = s->uuid;
    return 0;
}

/*!
 * @brief Keep the scratch volume's database in a subdirectory of its share
 */
static int cnid_scratch_separate_db(struct cnid_scratch *s)
{
    snprintf(s->dbdir, sizeof(s->dbdir), "%s/db", s->base);

    if (mkdir(s->dbdir, 0700) != 0) {
        s->dbdir[0] = '\0';
        return -1;
    }

    snprintf(s->dbfile, sizeof(s->dbfile), "%s/%s.sqlite", s->dbdir,
             s->vol.v_localname);
    s->vol.v_dbpath = s->dbdir;
    return 0;
}

/*!
 * @brief Whether the scratch volume's sqlite database exists
 */
static bool cnid_scratch_db_exists(const struct cnid_scratch *s)
{
    struct stat st;
    return stat(s->dbfile, &st) == 0;
}

/*!
 * @brief Remove the scratch volume with its database and companions
 */
static void cnid_scratch_cleanup(const struct cnid_scratch *s)
{
    const char *suffixes[] = { "-wal", "-shm", "-journal" };
    char companion[MAXPATHLEN];

    for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
        snprintf(companion, sizeof(companion), "%s%s", s->dbfile, suffixes[i]);
        unlink(companion);
    }

    unlink(s->dbfile);

    if (s->dbdir[0] != '\0') {
        rmdir(s->dbdir);
    }

    rmdir(s->base);
}

/*!
 * @brief cnid_open() serves an unknown scheme through the sqlite backend
 */
int utest_cnid_open_unknown_scheme_uses_sqlite(void)
{
    static char scheme[] = "dbd";
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    int result = 1;

    if (cnid_scratch_setup(&s, "scheme", "fallback",
                           "0F0F0F0F-1111-2222-3333-444444444444") != 0) {
        return TEST_SKIP;
    }

    cdb = cnid_open(&s.vol, scheme, 0);

    if (cdb != NULL) {
        cnid_close(cdb);
        result = cnid_scratch_db_exists(&s) ? 0 : 2;
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief cnid_open() refuses a backend this build was compiled without
 */
int utest_cnid_open_missing_backend_refused(void)
{
#ifdef CNID_BACKEND_MYSQL
    return TEST_SKIP;
#else
    static char scheme[] = "mysql";
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    int result = 0;

    if (cnid_scratch_setup(&s, "missing", "missing",
                           "0F0F0F0F-1111-2222-3333-666666666666") != 0) {
        return TEST_SKIP;
    }

    cdb = cnid_open(&s.vol, scheme, 0);

    if (cdb != NULL) {
        cnid_close(cdb);
        result = 1;
    }

    if (cnid_scratch_db_exists(&s)) {
        result = 2;
    }

    cnid_scratch_cleanup(&s);
    return result;
#endif
}

/*!
 * @brief A MySQL open that fails at the volume's folder gives up root
 *
 * The folder's parent is missing, so the open fails before it connects.
 *
 * @returns 0, 1 when the open succeeds, 2 when it leaves the euid at 0 or
 *          root does not come back, TEST_SKIP without the MySQL backend or
 *          root
 */
int utest_cnid_mysql_open_drops_root(void)
{
#ifndef CNID_BACKEND_MYSQL
    return TEST_SKIP;
#else
    static char scheme[] = "mysql";
    char localname[64];
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    uid_t euid;
    int result = 0;
    snprintf(localname, sizeof(localname), "netatalk-test-%ld/euid",
             (long)getpid());

    if (getuid() != 0
            || cnid_scratch_setup(&s, "euid", localname,
                                  "0F0F0F0F-1111-2222-3333-777777777777")
            != 0) {
        return TEST_SKIP;
    }

    if (seteuid(1) != 0) {
        cnid_scratch_cleanup(&s);
        return TEST_SKIP;
    }

    cdb = cnid_open(&s.vol, scheme, 0);
    euid = geteuid();

    /* a lost hand-back would run the rest of the suite as uid 1 */
    if (euid != 0 && seteuid(0) != 0) {
        result = 2;
    } else if (cdb != NULL) {
        cnid_close(cdb);
        result = 1;
    } else if (euid != 1) {
        result = 2;
    }

    cnid_scratch_cleanup(&s);
    return result;
#endif
}

/*!
 * @brief cnid_scheme_registered() knows the registered backends by name
 */
int utest_cnid_scheme_registered(void)
{
    if (!cnid_scheme_registered("sqlite")
            || !cnid_scheme_registered("SQLite")) {
        return 1;
    }

    if (cnid_scheme_registered("dbd") || cnid_scheme_registered("")) {
        return 2;
    }

#ifdef CNID_BACKEND_MYSQL
    return cnid_scheme_registered("mysql") ? 0 : 3;
#else
    return cnid_scheme_registered("mysql") ? 3 : 0;
#endif
}

/*!
 * @brief Open and close the scratch volume's database, then report whether
 *        that open logged @p needle
 */
static int cnid_open_logged(struct cnid_scratch *s, const char *logpath,
                            const char *needle)
{
    static char scheme[] = "sqlite";
    struct _cnid_db *cdb;
    conf_log_truncate(logpath);

    if ((cdb = cnid_open(&s->vol, scheme, 0)) == NULL) {
        return -1;
    }

    cnid_close(cdb);
    return conf_log_contains(logpath, needle);
}

/*!
 * @brief cnid_open() logs the creation of a new database once
 */
int utest_cnid_sqlite_creation_logged(void)
{
    struct cnid_scratch s;
    char logpath[64];
    char needle[MAXPATHLEN + 64];
    int first;
    int second = -2;

    if (cnid_scratch_setup(&s, "create", "createvol",
                           "0F0F0F0F-1111-2222-3333-999999999999") != 0) {
        return TEST_SKIP;
    }

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        cnid_scratch_cleanup(&s);
        return TEST_SKIP;
    }

    /* shared or private depends on who runs the test */
    snprintf(needle, sizeof(needle),
             " SQLite CNID database for volume '%s' in %s", s.base, s.dbfile);
    setuplog("default:info", logpath, true);

    if ((first = cnid_open_logged(&s, logpath, needle)) == 1) {
        second = cnid_open_logged(&s, logpath, needle);
    }

    setuplog("default:note", "/dev/stderr", true);
    unlink(logpath);
    cnid_scratch_cleanup(&s);

    if (first != 1 || second != 0) {
        fprintf(test_stream(),
                "# utest_cnid_sqlite_creation_logged: first open %d, "
                "second open %d\n", first, second);
        return 1;
    }

    return 0;
}

/* Names that tell the search syntax's wildcards from literal characters */
static const char *const cnid_syntax_names[] = {
    "50% off.pdf", "50 off.pdf", "my_file.doc", "myXfile.doc",
    "report draft final.txt", "IMG_1234.JPG", "\xc3\x89lan.txt",
    "My Caf\xc3\xa9.txt", "foo\\bar.txt", "food.txt",
};

/*!
 * @brief Count the CNIDs a search for a term answers
 *
 * @param[in] cdb   CNID database
 * @param[in] term  search term
 * @returns the count, or -1 for a failed search
 */
static int cnid_find_count(struct _cnid_db *cdb, const char *term)
{
    cnid_t results[CNID_FIND_MIN_RESULTS];
    bool more = false;
    return cnid_find(cdb, term, strlen(term), results, sizeof(results),
                     &more);
}

/*!
 * @brief Register names in the root of a scratch database, each with an
 *        inode of its own
 *
 * @param[in] cdb    CNID database
 * @param[in] names  the names
 * @param[in] n      number of names
 * @param[in] ino    inode of the first name
 * @returns 0, or -1 when an add fails
 */
static int cnid_add_names(struct _cnid_db *cdb, const char *const *names,
                          size_t n, ino_t ino)
{
    struct stat st;
    memset(&st, 0, sizeof(st));
    st.st_dev = 4242;

    for (size_t i = 0; i < n; i++) {
        st.st_ino = ino + i;

        if (cnid_add(cdb, &st, DIRDID_ROOT, names[i], strlen(names[i]),
                     CNID_INVALID) == CNID_INVALID) {
            return -1;
        }
    }

    return 0;
}

/*!
 * @brief Check what searches for the syntax names answer
 *
 * @param[in] cdb  CNID database holding cnid_syntax_names
 * @returns 0, or the number of the first failed check
 */
static int cnid_syntax_checks(struct _cnid_db *cdb)
{
    static const struct {
        const char *term;
        int want;
    } checks[] = {
        {"50%", 1},
        {"my_file", 1},
        {"report*final", 1},
        {"IMG_??34", 1},
        {"\xc3\xa9lan", 1},
        {"off.pdf", 2},
        {"foo\\", 1},
    };

    for (size_t i = 0; i < sizeof(checks) / sizeof(checks[0]); i++) {
        if (cnid_find_count(cdb, checks[i].term) != checks[i].want) {
            return (int)i + 1;
        }
    }

    return 0;
}

/*!
 * @brief A name search reads '*' and '?' as wildcards, '%', '_' and a final
 *        backslash as themselves, and letters without regard to case
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_find_syntax(void)
{
    static char scheme[] = "sqlite";
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    int result = 10;

    if (cnid_scratch_setup(&s, "syntax", "syntaxvol",
                           "0F0F0F0F-1111-2222-3333-BBBBBBBBBBBB") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) != NULL) {
        result = cnid_add_names(cdb, cnid_syntax_names,
                                sizeof(cnid_syntax_names)
                                / sizeof(cnid_syntax_names[0]), 100) != 0
                 ? 11 : cnid_syntax_checks(cdb);
        cnid_close(cdb);
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief Count the CNIDs a query answers into a buffer of
 *        CNID_FIND_MIN_RESULTS entries
 *
 * @param[in]  cdb    CNID database
 * @param[in]  query  the query string
 * @param[out] more   receives whether more matches exist
 * @returns the count, or -1 for a failed search
 */
static int cnid_query_count(struct _cnid_db *cdb, const char *query,
                            bool *more)
{
    cnid_t results[CNID_FIND_MIN_RESULTS];
    struct sl_name_query *q = sl_name_query_parse(query, 0, 8);
    int count = q == NULL ? -1
                : cnid_find_scoped(cdb, q, CNID_INVALID, results,
                                   sizeof(results), more);
    sl_name_query_free(q);
    return count;
}

/*!
 * @brief An exact-name search answers the whole name alone, without regard to
 *        case, past more names that contain it than its buffer holds
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_find_exact_name(void)
{
    static char scheme[] = "sqlite";
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    char name[32];
    const char *names[1] = { name };
    cnid_t results[CNID_FIND_MIN_RESULTS];
    bool more = true;
    int result = 0;

    if (cnid_scratch_setup(&s, "exact", "exactvol",
                           "0F0F0F0F-1111-2222-3333-ACACACACACAC") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL) {
        cnid_scratch_cleanup(&s);
        return 1;
    }

    for (int i = 0; i <= CNID_FIND_MIN_RESULTS && result == 0; i++) {
        snprintf(name, sizeof(name), "ce target %03d", i);
        result = cnid_add_names(cdb, names, 1, 4000 + (ino_t)i) == 0 ? 0 : 10;
    }

    snprintf(name, sizeof(name), "ce target");

    if (result == 0 && cnid_add_names(cdb, names, 1, 5000) != 0) {
        result = 11;
    } else if (result == 0
               && (cnid_find_exact(cdb, "Ce Target", 9, results,
                                   sizeof(results), &more) != 1 || more)) {
        result = 2;
    }

    cnid_close(cdb);
    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief A search fills its buffer with matches of the whole query, an &&
 *        and a != included, however many names one comparison of it takes
 *
 * The names that match come after twice a buffer of names that match one
 * side of the && alone.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_find_query_limit(void)
{
    static char scheme[] = "sqlite";
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    char name[32];
    const char *names[1] = { name };
    bool more = true;
    int result = 0;

    if (cnid_scratch_setup(&s, "limit", "limitvol",
                           "0F0F0F0F-1111-2222-3333-DDDDDDDDDDDD") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL) {
        cnid_scratch_cleanup(&s);
        return 1;
    }

    for (int i = 0; i < 2 * CNID_FIND_MIN_RESULTS && result == 0; i++) {
        snprintf(name, sizeof(name), "photo %03d.jpg", i);
        result = cnid_add_names(cdb, names, 1, 2000 + i) == 0 ? 0 : 10;
    }

    for (int i = 0; i < 3 && result == 0; i++) {
        snprintf(name, sizeof(name), "trip 2024 %d.jpg", i);
        result = cnid_add_names(cdb, names, 1, 3000 + i) == 0 ? 0 : 11;
    }

    if (result == 0
            && (cnid_query_count(cdb, "kMDItemFSName==\"*.jpg\"c"
                                 "&&kMDItemFSName==\"*2024*\"c", &more) != 3
                || more)) {
        result = 1;
    } else if (result == 0
               && (cnid_query_count(cdb, "kMDItemFSName!=\"photo*\"c", &more)
                   != 3 || more)) {
        result = 2;
    } else if (result == 0
               && (cnid_query_count(cdb, "kMDItemFSName==\"*.jpg\"c", &more)
                   != CNID_FIND_MIN_RESULTS || !more)) {
        result = 3;
    }

    cnid_close(cdb);
    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief Tell whether this SQLite library builds an FTS5 trigram table
 *
 * @returns true when it does
 */
static bool cnid_fts5_trigram(void)
{
    static const char sql[] = "CREATE VIRTUAL TABLE t USING "
                              "fts5(n, tokenize='trigram')";
    sqlite3 *probe = NULL;
    bool ok = sqlite3_open(":memory:", &probe) == SQLITE_OK
              && sqlite3_exec(probe, sql, NULL, NULL, NULL) == SQLITE_OK;
    sqlite3_close(probe);
    return ok;
}

/*!
 * @brief Count the CNIDs a value with its modifiers answers on the whole
 *        volume
 *
 * @param[in] cdb    CNID database
 * @param[in] value  the value
 * @param[in] flags  SL_NAMES_* flags
 * @returns the count, or -1 for a failed search
 */
static int cnid_value_count(struct _cnid_db *cdb, const char *value,
                            unsigned int flags)
{
    cnid_t results[CNID_FIND_MIN_RESULTS];
    bool more = false;
    struct sl_name_query *query = sl_name_query_value(value, strlen(value),
                                                      flags);
    int count = query == NULL ? -1
                : cnid_find_scoped(cdb, query, CNID_INVALID, results,
                                   sizeof(results), &more);
    sl_name_query_free(query);
    return count;
}

/*!
 * @brief Check the anchors, d and w on the syntax names
 *
 * @param[in] cdb  CNID database holding cnid_syntax_names
 * @returns 0, or the number of the first failed check
 */
static int cnid_value_checks(struct _cnid_db *cdb)
{
    if (cnid_value_count(cdb, "cafe*", SL_NAMES_CASE | SL_NAMES_DIACRITIC
                         | SL_NAMES_WORD) != 1) {
        return 1;
    }

    if (cnid_value_count(cdb, "*.JPG", 0) != 1) {
        return 2;
    }

    if (cnid_value_count(cdb, "IMG_1234.JPG", 0) != 1) {
        return 3;
    }

    return cnid_value_count(cdb, "IMG_1234", 0) == 0 ? 0 : 4;
}

/*!
 * @brief Steps of the table scan statement during one search
 *
 * @param[in] cdb   sqlite CNID database
 * @param[in] term  search term
 * @param[in] want  results the search must answer
 * @returns the steps, or -1 when the search answers another count
 */
static int cnid_scan_steps(struct _cnid_db *cdb, const char *term, int want)
{
    const CNID_sqlite_private *db = cdb->cnid_db_private;
    sqlite3_stmt_status(db->cnid_find_stmt, SQLITE_STMTSTATUS_FULLSCAN_STEP, 1);

    if (cnid_find_count(cdb, term) != want) {
        return -1;
    }

    return sqlite3_stmt_status(db->cnid_find_stmt,
                               SQLITE_STMTSTATUS_FULLSCAN_STEP, 0);
}

/*!
 * @brief Tell whether a statement reads the volume's table by Id alone, as
 *        its query plan says
 *
 * @param[in] cdb   sqlite CNID database
 * @param[in] stmt  a prepared statement of the backend
 * @returns true when no step of the plan scans the table, under its name or
 *          the statements' aliases of it
 */
static bool cnid_reads_by_id(struct _cnid_db *cdb, sqlite3_stmt *stmt)
{
    const CNID_sqlite_private *db = cdb->cnid_db_private;
    const char *uuid = db->cnid_sqlite_voluuid_str;
    char *sql = sqlite3_mprintf("EXPLAIN QUERY PLAN %s", sqlite3_sql(stmt));
    sqlite3_stmt *plan = NULL;
    bool by_id = sql != NULL
                 && sqlite3_prepare_v2(db->cnid_sqlite_con, sql, -1, &plan,
                                       NULL) == SQLITE_OK;

    while (by_id && sqlite3_step(plan) == SQLITE_ROW) {
        const char *step = (const char *)sqlite3_column_text(plan, 3);
        const char *name;
        size_t len;

        if (step == NULL || strncmp(step, "SCAN ", 5) != 0) {
            continue;
        }

        /* SQLite before 3.36 writes SCAN TABLE */
        name = step + 5 + (strncmp(step + 5, "TABLE ", 6) == 0 ? 6 : 0);
        len = strcspn(name, " ");
        by_id = !((len == 1 && (name[0] == 't' || name[0] == 'p'))
                  || (len == strlen(uuid) && strncmp(name, uuid, len) == 0));
    }

    sqlite3_finalize(plan);
    sqlite3_free(sql);
    return by_id;
}

/*!
 * @brief Tell whether every index statement the searches prepared reads the
 *        volume's table by Id alone, while the scan statement does not
 *
 * @param[in] cdb  sqlite CNID database
 * @returns true when they do, and one at least was prepared
 */
static bool cnid_index_reads_by_id(struct _cnid_db *cdb)
{
    const CNID_sqlite_private *db = cdb->cnid_db_private;
    int prepared = 0;

    for (int i = 0; i < CNID_FIND_MAX_SEEDS; i++) {
        sqlite3_stmt *const stmts[2] = {
            db->cnid_find_index_stmt[i], db->cnid_find_scoped_index_stmt[i]
        };

        for (int k = 0; k < 2; k++) {
            if (stmts[k] != NULL && !cnid_reads_by_id(cdb, stmts[k])) {
                return false;
            }

            prepared += stmts[k] != NULL;
        }
    }

    return prepared > 0 && db->cnid_find_stmt != NULL
           && !cnid_reads_by_id(cdb, db->cnid_find_stmt);
}

/*!
 * @brief Open a second connection to a scratch database that may write the
 *        name index's own tables and the schema, as the tests that break an
 *        index do
 *
 * A library that opens its connections in defensive mode refuses both.
 *
 * @param[in]  dbfile  the database
 * @param[out] peer    receives the connection, to sqlite3_close() either way
 * @returns SQLITE_OK, or the result of the open
 */
static int cnid_scratch_peer(const char *dbfile, sqlite3 **peer)
{
    int rc = sqlite3_open(dbfile, peer);
#ifdef SQLITE_DBCONFIG_DEFENSIVE

    if (rc == SQLITE_OK) {
        sqlite3_db_config(*peer, SQLITE_DBCONFIG_DEFENSIVE, 0, (int *)NULL);
    }

#endif
    return rc;
}

/*!
 * @brief Run one SQL text through a second connection to the scratch database
 *
 * @param[in] s    scratch volume
 * @param[in] fmt  SQL with %1$s for the table name
 * @returns the first column of the first row, -1 on an error, 0 without a row
 */
static int cnid_scratch_sql(const struct cnid_scratch *s, const char *fmt)
{
    sqlite3 *peer = NULL;
    sqlite3_stmt *stmt = NULL;
    char *uuid = uuid_strip_dashes(s->vol.v_uuid);
    char *sql = NULL;
    int ret = -1;

    if (uuid != NULL && asprintf(&sql, fmt, uuid) >= 0) { //NOSONAR: fmt is one of this file's SQL templates
        if (cnid_scratch_peer(s->dbfile, &peer) == SQLITE_OK
                && sqlite3_prepare_v2(peer, sql, -1, &stmt, NULL)
                == SQLITE_OK) {
            int rc = sqlite3_step(stmt);
            ret = rc == SQLITE_ROW ? sqlite3_column_int(stmt, 0)
                  : rc == SQLITE_DONE ? 0 : -1;
        }

        sqlite3_finalize(stmt);
        sqlite3_close(peer);
        free(sql);
    }

    free(uuid);
    return ret;
}

/* The volume's name index objects, and the changes its log holds */
#define CNID_INDEX_OBJECTS_SQL \
    "SELECT count(*) FROM sqlite_master WHERE name COLLATE NOCASE IN " \
    "('%1$s_names1', '%1$s_changes', '%1$s_added', '%1$s_renamed', " \
    "'%1$s_deleted')"
#define CNID_INDEX_LOGGED_SQL "SELECT count(*) FROM \"%1$s_changes\""
#define CNID_INDEX_TRIGGERS_SQL \
    "SELECT count(*) FROM sqlite_master WHERE type = 'trigger' " \
    "AND name LIKE '%1$s\\_%%' ESCAPE '\\'"

/*!
 * @brief Register numbered names in the root of a scratch database, each with
 *        an inode of its own
 *
 * @param[in] cdb    CNID database
 * @param[in] fmt    printf format of a name, taking its number
 * @param[in] first  number of the first name
 * @param[in] n      number of names
 * @param[in] ino    inode of the first name
 * @returns 0, or -1 when an add fails
 */
static int cnid_add_numbered(struct _cnid_db *cdb, const char *fmt,
                             int first, int n, ino_t ino)
{
    char name[48];
    const char *names[1] = { name };

    for (int i = 0; i < n; i++) {
        snprintf(name, sizeof(name), fmt, first + i); //NOSONAR: fmt is a name pattern of this file's tests

        if (cnid_add_names(cdb, names, 1, ino + (ino_t)i) != 0) {
            return -1;
        }
    }

    return 0;
}

/*!
 * @brief Register a folder in the root, a folder in it and a needle two
 *        levels down, and a folder in the root of 20 names and a folder with
 *        a needle
 *
 * @param[in]  cdb  CNID database
 * @param[out] box  receives the outer folder's CNID
 * @param[out] big  receives the CNID of the folder of 22 names
 * @returns 0, or -1 when an add fails
 */
static int cnid_add_box(struct _cnid_db *cdb, cnid_t *box, cnid_t *big)
{
    struct stat st;
    cnid_t inner;
    char name[16];
    memset(&st, 0, sizeof(st));
    st.st_dev = 4242;
    st.st_ino = 60;

    if ((*box = cnid_add(cdb, &st, DIRDID_ROOT, "box", 3,
                         CNID_INVALID)) == CNID_INVALID) {
        return -1;
    }

    st.st_ino = 61;

    if ((inner = cnid_add(cdb, &st, *box, "inner", 5, CNID_INVALID))
            == CNID_INVALID) {
        return -1;
    }

    st.st_ino = 62;

    if (cnid_add(cdb, &st, inner, "needle deep.txt", 15, CNID_INVALID)
            == CNID_INVALID) {
        return -1;
    }

    st.st_ino = 63;

    if ((*big = cnid_add(cdb, &st, DIRDID_ROOT, "big", 3,
                         CNID_INVALID)) == CNID_INVALID) {
        return -1;
    }

    for (int i = 0; i < 20; i++) {
        snprintf(name, sizeof(name), "big %02d.txt", i);
        st.st_ino = 64 + (ino_t)i;

        if (cnid_add(cdb, &st, *big, name, strlen(name), CNID_INVALID)
                == CNID_INVALID) {
            return -1;
        }
    }

    st.st_ino = 84;

    if ((inner = cnid_add(cdb, &st, *big, "sub", 3, CNID_INVALID))
            == CNID_INVALID) {
        return -1;
    }

    st.st_ino = 85;
    return cnid_add(cdb, &st, inner, "needle big.txt", 14, CNID_INVALID)
           == CNID_INVALID ? -1 : 0;
}

/* The query of the folder searches that cnid_add_box()'s needles answer */
#define CNID_NEEDLE_QUERY "kMDItemFSName==\"*needle*\"c"

/*!
 * @brief Search a folder, counting the runs of the scan of the folder's
 *        subtree
 *
 * @param[in]  cdb     sqlite CNID database
 * @param[in]  text    the query
 * @param[in]  folder  CNID of the folder
 * @param[out] scans   receives the runs of the scan statement
 * @returns the CNIDs, or -1 for a failed search
 */
static int cnid_scoped_count(struct _cnid_db *cdb, const char *text,
                             cnid_t folder, int *scans)
{
    const CNID_sqlite_private *db = cdb->cnid_db_private;
    struct sl_name_query *query = sl_name_query_parse(text, 0, 8);
    cnid_t results[CNID_FIND_MIN_RESULTS];
    bool more = false;
    int count = -1;

    if (query != NULL) {
        sqlite3_stmt_status(db->cnid_find_scoped_stmt, SQLITE_STMTSTATUS_RUN,
                            1);
        count = cnid_find_scoped(cdb, query, folder, results, sizeof(results),
                                 &more);
        *scans = sqlite3_stmt_status(db->cnid_find_scoped_stmt,
                                     SQLITE_STMTSTATUS_RUN, 0);
        sl_name_query_free(query);
    }

    return count;
}

/*!
 * @brief A search inside a folder answers the names under the folder alone,
 *        walking the name index's few candidates up their folders, of one
 *        seed and of two
 *
 * @param[in] cdb  sqlite CNID database holding cnid_add_box()'s folders and a
 *                 needle in the root
 * @param[in] box  CNID of the outer folder, 3 names
 * @param[in] big  CNID of the folder of 22 names
 * @returns 0, or the number of the first failed check
 */
static int cnid_scoped_checks(struct _cnid_db *cdb, cnid_t box, cnid_t big)
{
    const CNID_sqlite_private *db = cdb->cnid_db_private;
    int scans = -1;

    if (cnid_scoped_count(cdb, CNID_NEEDLE_QUERY, big, &scans) != 1
            || scans != 0) {
        return 1;
    }

    if (cnid_scoped_count(cdb, CNID_NEEDLE_QUERY, box, &scans) != 1
            || scans != 0) {
        return 2;
    }

    /* two seeds walk up through the folder statement of two arms */
    return cnid_scoped_count(cdb, CNID_NEEDLE_QUERY
                             "||kMDItemFSName==\"*big 1*\"c", big, &scans)
           == 11 && scans == 0 && db->cnid_find_scoped_index_stmt[1] != NULL
           ? 0 : 3;
}

/* 17500 needles in the root, more than CNID_SQLITE_WALK_MAX, which a second
 * connection writes in one statement each: the rows, their names in the name
 * index, and their Ids out of its log, as a move leaves them */
#define CNID_WALK_NEEDLES_SQL \
    "WITH RECURSIVE n(i) AS (SELECT 0 UNION ALL SELECT i + 1 FROM n " \
    "WHERE i < 17499) INSERT INTO \"%1$s\" (Name, Did, DevNo, InodeNo) " \
    "SELECT printf('needle %%05d.txt', i), 2, 4242, 100000 + i FROM n"
#define CNID_WALK_INDEXED_SQL \
    "INSERT INTO \"%1$s_names1\" (rowid, Name) SELECT Id, lower(Name) " \
    "FROM \"%1$s\" WHERE Name GLOB 'needle [0-9]*'"
#define CNID_WALK_MOVED_SQL \
    "DELETE FROM \"%1$s_changes\" WHERE Id IN " \
    "(SELECT Id FROM \"%1$s\" WHERE Name GLOB 'needle [0-9]*')"

/*!
 * @brief A search whose rows hold an Id outside the CNID range skips it and
 *        still reports the matches past its buffer
 *
 * The row of Id 0 comes first, so the statement's LIMIT counts it, and the
 * 101st valid match lies past the rows the statement returns.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_find_skipped_more(void)
{
    static char scheme[] = "sqlite";
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    cnid_t results[CNID_FIND_MIN_RESULTS];
    bool more = false;
    int result = 0;

    if (cnid_scratch_setup(&s, "skipped", "skippedvol",
                           "0F0F0F0F-1111-2222-3333-DEDEDEDEDEDE") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL) {
        cnid_scratch_cleanup(&s);
        return 1;
    }

    if (cnid_add_numbered(cdb, "skipped %03d.txt", 0, CNID_FIND_MIN_RESULTS + 1,
                          10000) != 0
            || cnid_scratch_sql(&s, "INSERT INTO \"%1$s\" (Id, Name, Did, "
                                "DevNo, InodeNo) VALUES (0, 'skipped bad.txt', "
                                "2, 4242, 9999)") != 0) {
        result = 10;
    } else if (cnid_find(cdb, "skipped", 7, results, sizeof(results), &more)
               != CNID_FIND_MIN_RESULTS) {
        result = 1;
    } else if (!more) {
        result = 2;
    }

    cnid_close(cdb);
    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief A volume of Latin-1 names: a search folds the case and the
 *        diacritics of their letters and matches no other letter, through
 *        the name index and without it
 *
 * The names move into the index's full-text table at the 1024th write, its
 * copy of each folded from Latin-1; the rename after them answers from the
 * log. An exact search folds case and diacritics, so été.txt and ete.txt
 * answer it alike, and àtà.txt does not.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_find_latin1(void)
{
    static char scheme[] = "sqlite";
    static char ete_latin[] = "\xe9t\xe9.txt";
    static char ata_latin[] = "\xe0t\xe0.txt";
    static char ete[] = "ete.txt";
    const char *names[] = { ete_latin, ata_latin, ete };
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    struct stat st;
    cnid_t results[CNID_FIND_MIN_RESULTS];
    cnid_t id;
    bool more = false;
    int result = 0;

    if (cnid_scratch_setup(&s, "latin", "latinvol",
                           "0F0F0F0F-1111-2222-3333-E1E1E1E1E1E1") != 0) {
        return TEST_SKIP;
    }

    if ((s.vol.v_volcharset = add_charset("ISO-8859-1")) == (charset_t) -1) {
        cnid_scratch_cleanup(&s);
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL) {
        cnid_scratch_cleanup(&s);
        return 1;
    }

    if (cnid_add_names(cdb, names, 3, 70) != 0
            || cnid_add_numbered(cdb, "fill %04d.txt", 0, 1100, 10000) != 0) {
        result = 10;
    } else if (cnid_find_exact(cdb, "\xc9T\xc9.TXT", 7, results,
                               sizeof(results), &more) != 2
               || results[0] != cnid_get(cdb, DIRDID_ROOT, ete_latin, 7)
               || results[1] != cnid_get(cdb, DIRDID_ROOT, ete, 7)) {
        result = 1;
    } else if (cnid_find(cdb, "\xc0T\xc0", 3, results, sizeof(results),
                         &more) != 1
               || results[0] != cnid_get(cdb, DIRDID_ROOT, ata_latin, 7)) {
        result = 2;
    }

    /* a rename the log holds, the full-text table its old name */
    memset(&st, 0, sizeof(st));
    st.st_dev = 4242;
    st.st_ino = 72;

    if (result == 0
            && ((id = cnid_get(cdb, DIRDID_ROOT, ete, 7)) == CNID_INVALID
                || cnid_update(cdb, id, &st, DIRDID_ROOT, "\xe9T\xc9 2.txt",
                               9) != 0)) {
        result = 11;
    } else if (result == 0
               && (cnid_find_exact(cdb, "\xc9t\xe9 2.TXT", 9, results,
                                   sizeof(results), &more) != 1
                   || results[0] != id)) {
        result = 3;
    }

    cnid_close(cdb);
    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief A search whose LIMIT the rows of Ids outside the CNID range take up
 *        runs again and fills its buffer, without more matches to report
 *
 * The rows of Ids -1 and 0 come first, and exactly a buffer of valid matches
 * follows them.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_find_skipped_fills(void)
{
    static char scheme[] = "sqlite";
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    cnid_t results[CNID_FIND_MIN_RESULTS];
    bool more = true;
    int result = 0;

    if (cnid_scratch_setup(&s, "fills", "fillsvol",
                           "0F0F0F0F-1111-2222-3333-DFDFDFDFDFDF") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL) {
        cnid_scratch_cleanup(&s);
        return 1;
    }

    if (cnid_add_numbered(cdb, "fills %03d.txt", 0, CNID_FIND_MIN_RESULTS,
                          10000) != 0
            || cnid_scratch_sql(&s, "INSERT INTO \"%1$s\" (Id, Name, Did, "
                                "DevNo, InodeNo) VALUES (-1, 'fills bad.txt', "
                                "2, 4242, 9998), (0, 'fills worse.txt', 2, "
                                "4242, 9999)") != 0) {
        result = 10;
    } else if (cnid_find(cdb, "fills", 5, results, sizeof(results), &more)
               != CNID_FIND_MIN_RESULTS) {
        result = 1;
    } else if (more) {
        result = 2;
    }

    cnid_close(cdb);
    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief A search inside a folder scans the folder when the name index holds
 *        more than CNID_SQLITE_WALK_MAX candidates, or its change log more
 *        Ids, and answers alike
 *
 * The needles wait in the log first, none of them in the full-text table,
 * then move into the table.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_find_walk_max(void)
{
    static char scheme[] = "sqlite";
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    cnid_t box = CNID_INVALID;
    cnid_t big = CNID_INVALID;
    int scans = -1;
    int result = 0;

    if (!cnid_fts5_trigram()) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "walkmax", "walkmaxvol",
                           "0F0F0F0F-1111-2222-3333-BCBCBCBCBCBC") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL) {
        cnid_scratch_cleanup(&s);
        return 1;
    }

    if (cnid_add_box(cdb, &box, &big) != 0
            || cnid_scratch_sql(&s, CNID_WALK_NEEDLES_SQL) != 0) {
        result = 10;
    } else if (cnid_scoped_count(cdb, CNID_NEEDLE_QUERY, box, &scans) != 1
               || scans == 0) {
        result = 3;
    } else if (cnid_scratch_sql(&s, CNID_WALK_INDEXED_SQL) != 0
               || cnid_scratch_sql(&s, CNID_WALK_MOVED_SQL) != 0) {
        result = 10;
    } else if (cnid_scoped_count(cdb, CNID_NEEDLE_QUERY, box, &scans) != 1
               || scans == 0) {
        result = 1;
    } else if (cnid_scoped_count(cdb, CNID_NEEDLE_QUERY, big, &scans) != 1
               || scans == 0) {
        result = 2;
    }

    cnid_close(cdb);
    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief A search on a new sqlite database reads the name index, one on an
 *        older database reads the table, and both answer alike
 *
 * The names before the 1024th write move into the index's full-text table,
 * and those after it answer from the log. A search inside either of
 * cnid_add_box()'s folders walks its needle up through the index, whose
 * statements read the table by Id alone. A rename and a delete answer at
 * once. With the index's objects dropped, the reopened database scans, and a
 * wipe gives the emptied table its index back.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_find_name_index(void)
{
    static char scheme[] = "sqlite";
    static char needle[] = "needle report.pdf";
    static char newname[] = "haystack report.pdf";
    static char gone[] = "fill 0042.txt";
    /* bytes that start no UTF-8 character, one before a continuation byte */
    static char latin[] = "R\xe9sum\xe9\x96 2.txt";
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    struct stat st;
    const char *names[1] = { needle };
    const char *latins[1] = { latin };
    cnid_t box = CNID_INVALID;
    cnid_t big = CNID_INVALID;
    cnid_t id;
    int logged;
    int result = 0;

    if (!cnid_fts5_trigram()) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "index", "indexvol",
                           "0F0F0F0F-1111-2222-3333-CCCCCCCCCCCC") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL) {
        cnid_scratch_cleanup(&s);
        return 1;
    }

    if (cnid_scratch_sql(&s, CNID_INDEX_OBJECTS_SQL) != 5) {
        result = 12;
    } else if (cnid_add_names(cdb, cnid_syntax_names,
                              sizeof(cnid_syntax_names)
                              / sizeof(cnid_syntax_names[0]), 100) != 0
               || cnid_add_names(cdb, names, 1, 50) != 0
               || cnid_add_names(cdb, latins, 1, 51) != 0
               || cnid_add_box(cdb, &box, &big) != 0
               || cnid_add_numbered(cdb, "fill %04d.txt", 0, 1100, 10000)
               != 0) {
        result = 11;
    }

    logged = cnid_scratch_sql(&s, CNID_INDEX_LOGGED_SQL);

    if (result == 0 && cnid_scan_steps(cdb, "needle", 3) != 0) {
        result = 2;
    } else if (result == 0 && (logged < 1 || logged >= 1100)) {
        result = 3;
    } else if (result == 0
               && cnid_scratch_sql(&s, "SELECT count(*) FROM \"%1$s_names1\" "
                                   "WHERE Name GLOB '*needle*'") != 3) {
        result = 13;
    } else if (result == 0 && (cnid_syntax_checks(cdb) != 0
                               || cnid_value_checks(cdb) != 0)) {
        result = 4;
    } else if (result == 0 && cnid_scoped_checks(cdb, box, big) != 0) {
        result = 15;
    } else if (result == 0 && cnid_scan_steps(cdb, "\xe9sum\xe9", 1) != 0) {
        result = 16;
    } else if (result == 0 && !cnid_index_reads_by_id(cdb)) {
        result = 17;
    }

    memset(&st, 0, sizeof(st));
    st.st_dev = 4242;
    st.st_ino = 50;

    if (result == 0
            && ((id = cnid_get(cdb, DIRDID_ROOT, needle,
                               strlen(needle))) == CNID_INVALID
                || cnid_update(cdb, id, &st, DIRDID_ROOT, newname,
                               strlen(newname)) != 0
                || cnid_find_count(cdb, "needle") != 2
                || cnid_find_count(cdb, "haystack") != 1)) {
        result = 5;
    }

    if (result == 0
            && ((id = cnid_get(cdb, DIRDID_ROOT, gone, strlen(gone)))
                == CNID_INVALID
                || cnid_delete(cdb, id) != 0
                || cnid_find_count(cdb, "fill 0042") != 0)) {
        result = 6;
    }

    cnid_close(cdb);
    cdb = NULL;

    if (result == 0
            && cnid_scratch_sql(&s, "DROP TRIGGER \"%1$s_added\"") != 0) {
        result = 7;
    }

    if (result == 0) {
        cnid_scratch_sql(&s, "DROP TRIGGER \"%1$s_renamed\"");
        cnid_scratch_sql(&s, "DROP TRIGGER \"%1$s_deleted\"");
        cnid_scratch_sql(&s, "DROP TABLE \"%1$s_changes\"");
        cnid_scratch_sql(&s, "DROP TABLE \"%1$s_names1\"");
        cdb = cnid_open(&s.vol, scheme, 0);
    }

    if (result == 0
            && (cdb == NULL || cnid_scan_steps(cdb, "haystack", 1) < 1100
                || cnid_syntax_checks(cdb) != 0
                || cnid_value_checks(cdb) != 0)) {
        result = 8;
    }

    if (result == 0
            && cnid_scratch_sql(&s, CNID_INDEX_OBJECTS_SQL) != 0) {
        result = 9;
    }

    if (result == 0
            && (cnid_wipe(cdb) != 0
                || cnid_scratch_sql(&s, CNID_INDEX_OBJECTS_SQL) != 5
                || cnid_add_names(cdb, names, 1, 50) != 0
                || cnid_add_numbered(cdb, "after %04d.txt", 0, 30, 30000) != 0
                || cnid_scan_steps(cdb, "needle", 1) != 0)) {
        result = 14;
    }

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief The change log moves into the name index once the writes of the
 *        connections that share it reach a batch, a bounded share at a time
 *
 * One connection writes fewer names than a batch, and the next one moves the
 * log when its own writes bring the log to a batch. A log longer than one
 * move takes moves over the next writes.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_name_index_moves(void)
{
    static char scheme[] = "sqlite";
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    int logged[4] = { -2, -2, -2, -2 };
    int result = 0;

    if (!cnid_fts5_trigram()) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "moves", "movesvol",
                           "0F0F0F0F-1111-2222-3333-EEEEEEEEEEEE") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
            || cnid_add_numbered(cdb, "first %04d.txt", 0, 1000, 20000) != 0) {
        result = 10;
    }

    if (cdb != NULL) {
        cnid_close(cdb);
        cdb = NULL;
    }

    logged[0] = cnid_scratch_sql(&s, CNID_INDEX_LOGGED_SQL);

    if (result == 0
            && ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
                || cnid_add_numbered(cdb, "second %04d.txt", 0, 24, 30000)
                != 0)) {
        result = 11;
    }

    logged[1] = cnid_scratch_sql(&s, CNID_INDEX_LOGGED_SQL);

    if (cdb != NULL) {
        cnid_close(cdb);
        cdb = NULL;
    }

    /* five times the table's Ids, as deleted rows log them */
    for (int i = 0; result == 0 && i < 5; i++) {
        static const char *const backlog[] = {
            "INSERT INTO \"%1$s_changes\" SELECT Id FROM \"%1$s\"",
            "INSERT INTO \"%1$s_changes\" SELECT Id + 100000 FROM \"%1$s\"",
            "INSERT INTO \"%1$s_changes\" SELECT Id + 200000 FROM \"%1$s\"",
            "INSERT INTO \"%1$s_changes\" SELECT Id + 300000 FROM \"%1$s\"",
            "INSERT INTO \"%1$s_changes\" SELECT Id + 400000 FROM \"%1$s\"",
        };

        if (cnid_scratch_sql(&s, backlog[i]) != 0) {
            result = 12;
        }
    }

    if (result == 0
            && ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
                || cnid_add_numbered(cdb, "third %04d.txt", 0, 1, 40000)
                != 0)) {
        result = 13;
    }

    logged[2] = cnid_scratch_sql(&s, CNID_INDEX_LOGGED_SQL);

    if (result == 0
            && cnid_add_numbered(cdb, "third %04d.txt", 1, 1, 40001) != 0) {
        result = 14;
    }

    logged[3] = cnid_scratch_sql(&s, CNID_INDEX_LOGGED_SQL);

    if (result == 0
            && (logged[0] != 1000 || logged[1] != 0
                || logged[2] != 5 * 1024 + 1 - 4096 || logged[3] != 0)) {
        result = 1;
    } else if (result == 0
               && (cnid_find_count(cdb, "first 0999") != 1
                   || cnid_find_count(cdb, "second 0023") != 1
                   || cnid_find_count(cdb, "third 0001") != 1)) {
        result = 2;
    }

    if (result != 0) {
        fprintf(test_stream(), "# utest_cnid_name_index_moves: result %d, "
                "logged %d %d %d %d\n", result, logged[0], logged[1],
                logged[2], logged[3]);
    }

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief Progress handler that interrupts every statement of a connection
 *
 * @param[in] arg  unused
 * @returns 1
 */
static int cnid_interrupt(void *arg _U_)
{
    return 1;
}

/*!
 * @brief SQL function netatalk_fold() that fails as a busy database does, so
 *        only a move of the change log fails
 *
 * @param[in,out] ctx   receives SQLITE_BUSY
 * @param[in]     argc  unused
 * @param[in]     argv  unused
 */
static void cnid_fold_busy(sqlite3_context *ctx, int argc _U_,
                           sqlite3_value **argv _U_)
{
    sqlite3_result_error_code(ctx, SQLITE_BUSY);
}

/*!
 * @brief Part of a name index on an empty table, as an older build's cleanup
 *        of a volume leaves the index's tables, gives way to a whole index
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_name_index_partial(void)
{
    static char scheme[] = "sqlite";
    static char needle[] = "partial needle.txt";
    struct cnid_scratch s;
    struct _cnid_db *cdb = NULL;
    const char *names[1] = { needle };
    int result = 0;

    if (!cnid_fts5_trigram()) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "partial", "partialvol",
                           "0F0F0F0F-1111-2222-3333-CECECECECECE") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL) {
        result = 10;
    } else {
        cnid_close(cdb);
        cdb = NULL;
    }

    /* the table goes with its triggers, and the index's tables stay */
    if (result == 0 && (cnid_scratch_sql(&s, "DROP TABLE \"%1$s\"") != 0
                        || cnid_scratch_sql(&s, CNID_INDEX_OBJECTS_SQL) != 2)) {
        result = 11;
    }

    if (result == 0 && ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
                        || cnid_scratch_sql(&s, CNID_INDEX_OBJECTS_SQL) != 5
                        || cnid_add_names(cdb, names, 1, 50) != 0
                        || cnid_find_count(cdb, "needle") != 1)) {
        result = 1;
    }

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief The name index outlasts a failure that says nothing about it, and an
 *        empty table's open keeps its index without a word
 *
 * An interrupted search fails, and the next one still reads the index. A
 * move that fails keeps the index and the changes it would move, which the
 * searches through the index still answer. The second open of an empty
 * table logs no missing index and keeps the index it finds, the change its
 * log holds included.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_name_index_kept(void)
{
    static char scheme[] = "sqlite";
    static char needle[] = "kept needle.txt";
    struct cnid_scratch s;
    struct _cnid_db *cdb = NULL;
    const char *names[1] = { needle };
    char logpath[64];
    int quiet = -2;
    int result = 0;

    if (!cnid_fts5_trigram()) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "kept", "keptvol",
                           "0F0F0F0F-1111-2222-3333-FFFFFFFFFFFF") != 0) {
        return TEST_SKIP;
    }

    if (conf_mklog(logpath, sizeof(logpath)) != 0) {
        cnid_scratch_cleanup(&s);
        return TEST_SKIP;
    }

    setuplog("default:info", logpath, true);

    if (cnid_open_logged(&s, logpath, "has no name index") == 0
            && cnid_scratch_sql(&s, "INSERT INTO \"%1$s_changes\" "
                                "VALUES (999)") == 0) {
        quiet = cnid_open_logged(&s, logpath, "has no name index");
    }

    setuplog("default:note", "/dev/stderr", true);
    unlink(logpath);

    if (quiet != 0) {
        result = 1;
    } else if (cnid_scratch_sql(&s, CNID_INDEX_LOGGED_SQL) != 1) {
        result = 5;
    } else if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
               || cnid_add_names(cdb, names, 1, 50) != 0
               || cnid_add_numbered(cdb, "fill %04d.txt", 0, 1100, 10000)
               != 0) {
        result = 10;
    } else {
        const CNID_sqlite_private *db = cdb->cnid_db_private;
        sqlite3_progress_handler(db->cnid_sqlite_con, 1, cnid_interrupt, NULL);

        if (cnid_find_count(cdb, "needle") != -1) {
            result = 2;
        }

        sqlite3_progress_handler(db->cnid_sqlite_con, 0, NULL, NULL);

        if (result == 0 && cnid_scan_steps(cdb, "needle", 1) != 0) {
            result = 3;
        }

        if (result == 0
                && (sqlite3_create_function(db->cnid_sqlite_con,
                                            "netatalk_fold", 1, SQLITE_UTF8,
                                            NULL, cnid_fold_busy, NULL, NULL)
                    != SQLITE_OK
                    || cnid_add_numbered(cdb, "more %04d.txt", 0, 1100, 30000)
                    != 0)) {
            result = 6;
        }

        if (result == 0
                && (db->cnid_find_index_stmt[0] == NULL
                    || cnid_scratch_sql(&s, CNID_INDEX_LOGGED_SQL) < 1100
                    || cnid_scan_steps(cdb, "more 1099", 1) != 0)) {
            result = 7;
        }
    }

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/* The volume's full-text table, 1 or 0 */
#define CNID_INDEX_NAMES_SQL \
    "SELECT count(*) FROM sqlite_master " \
    "WHERE name = '%1$s_names1' COLLATE NOCASE"

/*!
 * @brief A name index that fails to create leaves the table without one, and
 *        the open and a wipe go on with searches that read the table
 *
 * An index of the database named as the index's change log makes the
 * creation fail, which rolls back to its savepoint and leaves no full-text
 * table behind.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_name_index_refused(void)
{
    static char scheme[] = "sqlite";
    static char needle[] = "refused needle.txt";
    struct cnid_scratch s;
    struct _cnid_db *cdb = NULL;
    const char *names[1] = { needle };
    int result = 0;

    if (!cnid_fts5_trigram()) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "refused", "refusedvol",
                           "0F0F0F0F-1111-2222-3333-ABABABABABAB") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL) {
        result = 10;
    } else {
        cnid_close(cdb);
        cdb = NULL;
    }

    if (result == 0
            && (cnid_scratch_sql(&s, "DROP TRIGGER \"%1$s_added\"") != 0
                || cnid_scratch_sql(&s, "DROP TRIGGER \"%1$s_renamed\"") != 0
                || cnid_scratch_sql(&s, "DROP TRIGGER \"%1$s_deleted\"") != 0
                || cnid_scratch_sql(&s, "DROP TABLE \"%1$s_changes\"") != 0
                || cnid_scratch_sql(&s, "DROP TABLE \"%1$s_names1\"") != 0
                || cnid_scratch_sql(&s, "CREATE INDEX \"%1$s_changes\" "
                                    "ON \"%1$s\" (Did)") != 0)) {
        result = 11;
    }

    if (result == 0 && ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
                        || cnid_scratch_sql(&s, CNID_INDEX_NAMES_SQL) != 0)) {
        result = 1;
    } else if (result == 0
               && (cnid_add_numbered(cdb, "fill %04d.txt", 0, 20, 10000) != 0
                   || cnid_add_names(cdb, names, 1, 50) != 0
                   || cnid_scan_steps(cdb, "needle", 1) < 10)) {
        result = 2;
    } else if (result == 0
               && (cnid_wipe(cdb) != 0
                   || cnid_scratch_sql(&s, CNID_INDEX_NAMES_SQL) != 0)) {
        result = 3;
    }

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief A name index a connection cannot use stays, and its searches scan
 *
 * An incomplete index shows at the open of its table, a broken one at the
 * search that finds it so. A wipe gives the emptied table a whole index
 * again.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_name_index_unused(void)
{
    static char scheme[] = "sqlite";
    static char needle[] = "unused needle.txt";
    struct cnid_scratch s;
    struct _cnid_db *cdb = NULL;
    const char *names[1] = { needle };
    int result = 0;

    if (!cnid_fts5_trigram()) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "unused", "unusedvol",
                           "0F0F0F0F-1111-2222-3333-CDCDCDCDCDCD") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
            || cnid_add_names(cdb, names, 1, 50) != 0) {
        result = 10;
    }

    if (cdb != NULL) {
        cnid_close(cdb);
        cdb = NULL;
    }

    if (result == 0
            && cnid_scratch_sql(&s, "DROP TABLE \"%1$s_names1\"") != 0) {
        result = 11;
    }

    if (result == 0 && ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
                        || cnid_scratch_sql(&s, CNID_INDEX_OBJECTS_SQL) != 4
                        || cnid_find_count(cdb, "needle") != 1)) {
        result = 1;
    } else if (result == 0
               && (cnid_wipe(cdb) != 0
                   || cnid_scratch_sql(&s, CNID_INDEX_OBJECTS_SQL) != 5
                   || cnid_add_names(cdb, names, 1, 50) != 0
                   || cnid_add_numbered(cdb, "fill %04d.txt", 0, 1100, 10000)
                   != 0)) {
        result = 12;
    } else if (result == 0
               && cnid_scratch_sql(&s, "DELETE FROM \"%1$s_names1_data\"")
               != 0) {
        result = 13;
    } else if (result == 0) {
        const CNID_sqlite_private *db = cdb->cnid_db_private;

        if (cnid_find_count(cdb, "needle") != 1
                || db->cnid_find_index_stmt[0] != NULL
                || cnid_scratch_sql(&s, CNID_INDEX_TRIGGERS_SQL) != 3) {
            result = 2;
        }
    }

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief A two-seed prepare that fails on a broken index ends its use and scans
 *
 * A lookup after another connection drops the log reads the new schema, so
 * the statement of two seeds fails at its prepare rather than at its first
 * step.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_name_index_prepare_broken(void)
{
    static char scheme[] = "sqlite";
    static char volname[] = "brokenvol";
    static char needle[] = "broken needle.txt";
    struct cnid_scratch s;
    struct _cnid_db *cdb = NULL;
    const CNID_sqlite_private *db = NULL;
    const char *names[1] = { needle };
    bool more = false;
    int result = 0;

    if (!cnid_fts5_trigram()) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "broken", volname,
                           "0F0F0F0F-1111-2222-3333-BABABABABABA") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
            || cnid_add_names(cdb, names, 1, 60) != 0
            || (db = cdb->cnid_db_private)->cnid_find_index_stmt[0] == NULL
            || cnid_scratch_sql(&s, "DROP TABLE \"%1$s_changes\"") != 0
            || cnid_get(cdb, DIRDID_ROOT, needle, strlen(needle))
            == CNID_INVALID) {
        result = 10;
    } else if (cnid_query_count(cdb, "kMDItemFSName==\"*broken*\"c"
                                "||kMDItemFSName==\"*needle*\"c", &more) != 1
               || db->cnid_find_index_stmt[0] != NULL) {
        result = 1;
    }

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief A move of the logged changes that finds the name index broken ends
 *        the connection's use of it, and its searches read the table
 *
 * Another connection empties the index's data; the writes then reach a move
 * before any search reads the index.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_name_index_move_broken(void)
{
    static char scheme[] = "sqlite";
    static char needle[] = "moved needle.txt";
    struct cnid_scratch s;
    struct _cnid_db *cdb = NULL;
    const CNID_sqlite_private *db = NULL;
    const char *names[1] = { needle };
    int result = 0;

    if (!cnid_fts5_trigram()) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "movebroken", "movebrokenvol",
                           "0F0F0F0F-1111-2222-3333-ABABABABABAB") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
            || cnid_add_names(cdb, names, 1, 70) != 0
            || (db = cdb->cnid_db_private)->cnid_find_index_stmt[0] == NULL
            || cnid_scratch_sql(&s, "DELETE FROM \"%1$s_names1_data\"") != 0
            || cnid_add_numbered(cdb, "moved %04d.txt", 0, 1100, 20000) != 0) {
        result = 10;
    } else if (db->cnid_find_index_stmt[0] != NULL) {
        result = 1;
    } else if (cnid_find_count(cdb, "needle") != 1
               || cnid_scratch_sql(&s, CNID_INDEX_TRIGGERS_SQL) != 3) {
        result = 2;
    }

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/* The full-text rows of the name "a long needle name.txt", 1 once a move
 * takes it into the index */
#define CNID_LONG_NEEDLE_SQL \
    "SELECT count(*) FROM \"%1$s_names1\" WHERE rowid IN " \
    "(SELECT Id FROM \"%1$s\" WHERE Name = 'a long needle name.txt')"

/*!
 * @brief A seed whose pattern passes the connection's limit for a GLOB
 *        pattern reads the table, and the connection keeps its index
 *
 * The 1100 names after the needle reach a move, which takes the needle into
 * the full-text table: a search through the index would hand it to SQLite's
 * check of each candidate against the pattern, which fails past the limit.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_name_index_long_pattern(void)
{
    static char scheme[] = "sqlite";
    static char needle[] = "a long needle name.txt";
    struct cnid_scratch s;
    struct _cnid_db *cdb = NULL;
    const CNID_sqlite_private *db = NULL;
    const char *names[1] = { needle };
    int result = 0;

    if (!cnid_fts5_trigram()) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "longglob", "longglobvol",
                           "0F0F0F0F-1111-2222-3333-EFEFEFEFEFEF") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
            || cnid_add_names(cdb, names, 1, 80) != 0
            || cnid_add_numbered(cdb, "long %04d.txt", 0, 1100, 30000) != 0
            || (db = cdb->cnid_db_private)->cnid_find_index_stmt[0] == NULL
            || cnid_scratch_sql(&s, CNID_LONG_NEEDLE_SQL) != 1) {
        result = 10;
    } else {
        /* "*long needle name.txt*" holds 22 bytes */
        sqlite3_limit(db->cnid_sqlite_con, SQLITE_LIMIT_LIKE_PATTERN_LENGTH,
                      20);

        if (cnid_find_count(cdb, "long needle name.txt") != 1
                || db->cnid_find_index_stmt[0] == NULL) {
            result = 1;
        }
    }

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/* A stale volume's row and the objects of the schema its UUID ?1 names */
#define CNID_STALE_LEFT_SQL \
    "SELECT (SELECT count(*) FROM volumes WHERE VolUUID = ?1) " \
    "+ (SELECT count(*) FROM sqlite_master WHERE name LIKE ?1 || '%')"

/*!
 * @brief The open's cleanup of a stale volume keeps the volume's row while
 *        its full-text table stays, as a library without FTS5 leaves it, and
 *        removes the row once the table is gone
 *
 * The old volume's full-text table names a module the library lacks, so no
 * drop removes it; the other objects of the old volume go at once.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_stale_names_kept(void)
{
    static char scheme[] = "sqlite";
    static char volname[] = "stalevol";
    static const char old_uuid[] = "0F0F0F0F-1111-2222-3333-CACACACACACA";
    struct cnid_scratch s;
    struct _cnid_db *cdb = NULL;
    sqlite3 *peer = NULL;
    sqlite3_stmt *stmt = NULL;
    char *old = uuid_strip_dashes(old_uuid);
    char *sql = NULL;
    int result = 0;

    if (old == NULL || !cnid_fts5_trigram()) {
        free(old);
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "stale", volname, old_uuid) != 0) {
        free(old);
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL) {
        result = 10;
    } else {
        cnid_close(cdb);
        cdb = NULL;
    }

    if (result == 0
            && asprintf(&sql, "DROP TABLE \"%s_names1\";"
                        "PRAGMA writable_schema = ON;"
                        "INSERT INTO sqlite_master VALUES ('table', "
                        "'%s_names1', '%s_names1', 0, 'CREATE VIRTUAL "
                        "TABLE \"%s_names1\" USING nosuchmodule()');"
                        "PRAGMA writable_schema = OFF;", old, old, old, old)
            < 0) {
        sql = NULL;
        result = 11;
    }

    if (result == 0
            && (cnid_scratch_peer(s.dbfile, &peer) != SQLITE_OK
                || sqlite3_exec(peer, sql, NULL, NULL, NULL) != SQLITE_OK)) {
        result = 11;
    }

    sqlite3_close(peer);
    peer = NULL;

    /* the volume comes back under another UUID, so the old one is stale */
    if (result == 0) {
        snprintf(s.uuid, sizeof(s.uuid), "%s",
                 "0F0F0F0F-1111-2222-3333-CACACACACACB");

        if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL) {
            result = 12;
        }
    }

    if (result == 0
            && (cnid_scratch_peer(s.dbfile, &peer) != SQLITE_OK
                || sqlite3_prepare_v2(peer, CNID_STALE_LEFT_SQL, -1, &stmt,
                                      NULL) != SQLITE_OK
                || sqlite3_bind_text(stmt, 1, old, -1, SQLITE_STATIC)
                != SQLITE_OK
                || sqlite3_step(stmt) != SQLITE_ROW
                || sqlite3_column_int(stmt, 0) != 2)) {
        result = 1;
    }

    if (cdb != NULL) {
        cnid_close(cdb);
        cdb = NULL;
    }

    /* without its full-text table the next open removes the stale volume,
     * its row the last */
    free(sql);
    sql = NULL;

    if (result == 0
            && asprintf(&sql, "PRAGMA writable_schema = ON;"
                        "DELETE FROM sqlite_master WHERE name = "
                        "'%s_names1';PRAGMA writable_schema = OFF;", old)
            < 0) {
        sql = NULL;
        result = 13;
    } else if (result == 0
               && (sqlite3_exec(peer, sql, NULL, NULL, NULL) != SQLITE_OK
                   || (cdb = cnid_open(&s.vol, scheme, 0)) == NULL)) {
        result = 13;
    } else if (result == 0
               && (sqlite3_reset(stmt) != SQLITE_OK
                   || sqlite3_step(stmt) != SQLITE_ROW
                   || sqlite3_column_int(stmt, 0) != 0)) {
        result = 2;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(peer);

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    free(sql);
    free(old);
    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief A search by two seeds through the name index takes each match once,
 *        the renamed names its change log holds included, up to its limit
 *
 * Names that both comparisons match come through both seeds' arms, and
 * renames log names whose Ids come before the indexed ones; their old names
 * match neither seed, so the log's arm alone takes them. The UNION of the
 * arms keeps one of each, the first matches in Id order, and the table is
 * not scanned.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_find_seeds_once(void)
{
    static char scheme[] = "sqlite";
    struct cnid_scratch s;
    struct _cnid_db *cdb = NULL;
    const CNID_sqlite_private *db = NULL;
    struct sl_name_query *q = NULL;
    cnid_t results[200];
    struct stat st;
    char name[32];
    bool more = false;
    int count = -1;
    int result = 0;

    if (!cnid_fts5_trigram()) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&s, "seeds", "seedsvol",
                           "0F0F0F0F-1111-2222-3333-EFEFEFEFEFEF") != 0) {
        return TEST_SKIP;
    }

    if ((cdb = cnid_open(&s.vol, scheme, 0)) == NULL
            || cnid_add_numbered(cdb, "alpha beta %02d.txt", 0, 30, 19000)
            != 0
            || cnid_add_numbered(cdb, "gamma %03d.txt", 0, 60, 21000) != 0
            || cnid_add_numbered(cdb, "beta %03d.txt", 0, 250, 20000) != 0
            || cnid_add_numbered(cdb, "fill %04d.txt", 0, 1100, 10000) != 0) {
        result = 10;
    }

    memset(&st, 0, sizeof(st));
    st.st_dev = 4242;

    for (int i = 0; result == 0 && i < 60; i++) {
        cnid_t id;
        snprintf(name, sizeof(name), "gamma %03d.txt", i);
        id = cnid_get(cdb, DIRDID_ROOT, name, strlen(name));
        snprintf(name, sizeof(name), "beta again %03d.txt", i);
        st.st_ino = 21000 + (ino_t)i;

        if (id == CNID_INVALID
                || cnid_update(cdb, id, &st, DIRDID_ROOT, name,
                               strlen(name)) != 0) {
            result = 11;
        }
    }

    if (result == 0
            && (q = sl_name_query_parse("kMDItemFSName==\"*alpha*\"c"
                                        "||kMDItemFSName==\"*beta*\"c", 0, 8))
            == NULL) {
        result = 12;
    }

    if (result == 0) {
        db = cdb->cnid_db_private;
        sqlite3_stmt_status(db->cnid_find_stmt, SQLITE_STMTSTATUS_RUN, 1);
        count = cnid_find_scoped(cdb, q, CNID_INVALID, results,
                                 sizeof(results), &more);
    }

    for (int i = 1; result == 0 && i < count; i++) {
        if (ntohl(results[i]) <= ntohl(results[i - 1])) {
            result = 2;
        }
    }

    if (result == 0 && (count != 200 || !more)) {
        result = 1;
    }

    if (result == 0
            && (sqlite3_stmt_status(db->cnid_find_stmt, SQLITE_STMTSTATUS_RUN,
                                    0) != 0
                || db->cnid_find_index_stmt[1] == NULL)) {
        result = 4;
    }

    /* the 31st match is the first renamed name, which only the log's arm
     * takes */
    if (result == 0
            && results[30] != cnid_get(cdb, DIRDID_ROOT,
                                       "beta again 000.txt", 18)) {
        result = 3;
    }

    sl_name_query_free(q);

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief Whether opening the scratch volume reports @p orphan as a database
 *        left in the share root
 */
static int cnid_orphan_logged(struct cnid_scratch *s, const char *logpath,
                              const char *orphan)
{
    char needle[MAXPATHLEN + 64];
    snprintf(needle, sizeof(needle),
             "CNID database %s in the share root is not used", orphan);
    return cnid_open_logged(s, logpath, needle);
}

/*!
 * @brief cnid_open() reports a sqlite database left in the share root
 *
 * A regular file with the SQLite header is reported; a file without it, a
 * symbolic link, and the live database of a volume whose database directory
 * is its share root are not.
 */
int utest_cnid_sqlite_orphan_reported(void)
{
    static const char header[] = "SQLite format 3";
    struct cnid_scratch s;
    struct cnid_scratch symvol;
    struct cnid_scratch live;
    char logpath[64];
    char orphan[MAXPATHLEN];
    char linked[MAXPATHLEN];
    char target[MAXPATHLEN];
    int got[4] = { -2, -2, -2, -2 };
    int result = 0;

    if (cnid_scratch_setup(&s, "orphan", "orphanvol",
                           "0F0F0F0F-1111-2222-3333-777777777777") != 0) {
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&symvol, "link", "linkvol",
                           "0F0F0F0F-1111-2222-3333-AAAAAAAAAAAA") != 0) {
        cnid_scratch_cleanup(&s);
        return TEST_SKIP;
    }

    if (cnid_scratch_setup(&live, "live", "livevol",
                           "0F0F0F0F-1111-2222-3333-888888888888") != 0) {
        cnid_scratch_cleanup(&symvol);
        cnid_scratch_cleanup(&s);
        return TEST_SKIP;
    }

    if (cnid_scratch_separate_db(&s) != 0
            || cnid_scratch_separate_db(&symvol) != 0
            || conf_mklog(logpath, sizeof(logpath)) != 0) {
        cnid_scratch_cleanup(&live);
        cnid_scratch_cleanup(&symvol);
        cnid_scratch_cleanup(&s);
        return TEST_SKIP;
    }

    snprintf(orphan, sizeof(orphan), "%s/orphanvol.sqlite", s.base);
    snprintf(linked, sizeof(linked), "%s/linkvol.sqlite", symvol.base);
    snprintf(target, sizeof(target), "%s/target.sqlite", symvol.base);
    setuplog("default:note", logpath, true);

    if (write_disk_file(orphan, header, sizeof(header)) == 0) {
        got[0] = cnid_orphan_logged(&s, logpath, orphan);
    }

    if (write_disk_file(orphan, "plain text\n", 11) == 0) {
        got[1] = cnid_orphan_logged(&s, logpath, orphan);
    }

    if (write_disk_file(target, header, sizeof(header)) == 0
            && symlink(target, linked) == 0) {
        got[2] = cnid_orphan_logged(&symvol, logpath, linked);
    }

    /* the second open finds the database the first one created */
    if (cnid_orphan_logged(&live, logpath, live.dbfile) >= 0) {
        got[3] = cnid_orphan_logged(&live, logpath, live.dbfile);
    }

    setuplog("default:note", "/dev/stderr", true);

    if (got[0] != 1 || got[1] != 0 || got[2] != 0 || got[3] != 0) {
        fprintf(test_stream(),
                "# utest_cnid_sqlite_orphan_reported: header %d, plain %d, "
                "symlink %d, live %d\n", got[0], got[1], got[2], got[3]);
        result = 1;
    }

    unlink(orphan);
    unlink(linked);
    unlink(target);
    unlink(logpath);
    cnid_scratch_cleanup(&live);
    cnid_scratch_cleanup(&symvol);
    cnid_scratch_cleanup(&s);
    return result;
}

static int fake_open_calls;

/*!
 * @brief Backend open that records the call and opens nothing
 */
static struct _cnid_db *fake_cnid_open(struct cnid_open_args *args _U_)
{
    fake_open_calls++;
    return NULL;
}

/*!
 * @brief cnid_open() matches the scheme name without regard to case
 */
int utest_cnid_open_scheme_case_insensitive(void)
{
    static struct _cnid_module fake = {
        .name = "UTestScheme",
        .cnid_open = fake_cnid_open,
    };
    static char scheme[] = "utestscheme";
    struct cnid_scratch s;
    struct _cnid_db *cdb;
    int result;

    if (cnid_scratch_setup(&s, "case", "casevol",
                           "0F0F0F0F-1111-2222-3333-555555555555") != 0) {
        return TEST_SKIP;
    }

    cnid_register(&fake);
    fake_open_calls = 0;
    cdb = cnid_open(&s.vol, scheme, 0);

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    result = fake_open_calls == 1 ? 0 : 1;
    /* a fallback to sqlite, the failing case, leaves a database to remove */
    cnid_scratch_cleanup(&s);
    return result;
}

/*!
 * @brief cnid_volume_tag() names the CNID table, not the session's volume slot
 *
 * v_vid is a counter each child assigns in its own volume-load order, so the
 * same vid names different volumes in different children. The tag is derived
 * from the volume UUID, which every child that opens the volume agrees on.
 */
int utest_cnid_volume_tag_identity(void)
{
    struct vol vol_a;
    struct vol vol_b;
    char uuid_a[] = "8A1B2C3D-4E5F-6071-8293-A4B5C6D7E8F9";
    char uuid_same[] = "8A1B2C3D-4E5F-6071-8293-A4B5C6D7E8F9";
    char uuid_b[] = "8A1B2C3D-4E5F-6071-8293-A4B5C6D7E8FA";
    memset(&vol_a, 0, sizeof(vol_a));
    memset(&vol_b, 0, sizeof(vol_b));

    /* A volume with no UUID cannot be named, and must say so rather than
     * returning a tag that could collide with a real one */
    if (cnid_volume_tag(NULL) != 0) {
        return 1;
    }

    vol_a.v_uuid = NULL;

    if (cnid_volume_tag(&vol_a) != 0) {
        return 2;
    }

    /* Same UUID in two separate volume structs — the cross-process case, where
     * the sender and receiver have different v_vid but the same table */
    vol_a.v_uuid = uuid_a;
    vol_b.v_uuid = uuid_same;
    vol_a.v_vid = 1;
    vol_b.v_vid = 7;

    if (cnid_volume_tag(&vol_a) != cnid_volume_tag(&vol_b)) {
        return 3;
    }

    /* v_vid must not contribute: it is exactly the field that is unreliable */
    vol_b.v_vid = 99;

    if (cnid_volume_tag(&vol_a) != cnid_volume_tag(&vol_b)) {
        return 4;
    }

    /* A one-character UUID difference must not alias */
    vol_b.v_uuid = uuid_b;

    if (cnid_volume_tag(&vol_a) == cnid_volume_tag(&vol_b)) {
        return 5;
    }

    /* 0 is reserved for "no tag", so a real volume never yields it */
    if (cnid_volume_tag(&vol_a) == 0 || cnid_volume_tag(&vol_b) == 0) {
        return 6;
    }

    return 0;
}

/*!
 * @brief Second connection to the volume's CNID database
 *
 * The view a peer session, or any other writer of the world-writable db file,
 * has. The sqlite backend keeps that file at v_dbpath/v_localname.sqlite, mode
 * 0666. NULL when the volume is not on the sqlite backend.
 */
static sqlite3 *cnid_peer_open(struct vol *vol)
{
    sqlite3 *peer = NULL;
    char dbfile[MAXPATHLEN];
    struct stat st;

    if (vol->v_cdb == NULL || vol->v_dbpath == NULL
            || vol->v_cnidscheme == NULL
            || strcmp(vol->v_cnidscheme, "sqlite") != 0) {
        return NULL;
    }

    snprintf(dbfile, sizeof(dbfile), "%s/%s.sqlite", vol->v_dbpath,
             vol->v_localname);

    if (stat(dbfile, &st) != 0) {
        return NULL;
    }

    if (sqlite3_open(dbfile, &peer) != SQLITE_OK) {
        if (peer) {
            sqlite3_close(peer);
        }

        return NULL;
    }

    return peer;
}

/*!
 * @brief Read the volume's AUTOINCREMENT high-water mark
 *
 * sqlite keeps the mark in sqlite_sequence keyed by table name, and the sqlite
 * backend names each per-volume table after the volume UUID with the dashes
 * stripped.
 *
 * @returns the mark, or -1 when it cannot be read -- in which case a test must
 *          not plant a row that raises it, having no way to put it back
 */
static long long cnid_peer_get_seq(sqlite3 *peer, const char *uuid)
{
    sqlite3_stmt *stmt = NULL;
    long long seq = -1;
    char *sql = NULL;

    if (asprintf(&sql, "SELECT seq FROM sqlite_sequence WHERE name = '%s'",
                 uuid) == -1) {
        return -1;
    }

    if (sqlite3_prepare_v2(peer, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            seq = sqlite3_column_int64(stmt, 0);
        }

        sqlite3_finalize(stmt);
    }

    free(sql);
    return seq;
}

/*!
 * @brief Restore the AUTOINCREMENT high-water mark a probe row raised
 *
 * An explicit rowid above 2^32 lifts sqlite_sequence past the CNID range, and
 * from there every add on the volume mints out-of-range rowids that fail the
 * rest of the suite. A negative @p seq means the mark was never read, so no
 * caller should have planted such a row; the mismatch is reported rather than
 * passed over in silence.
 */
static void cnid_peer_set_seq(sqlite3 *peer, const char *uuid, long long seq)
{
    char *sql = NULL;

    if (seq < 0) {
        fprintf(stderr,
                "# cnid_peer_set_seq: no saved sequence for '%s', cannot restore\n",
                uuid);
        return;
    }

    if (asprintf(&sql, "UPDATE sqlite_sequence SET seq = %lld WHERE name = '%s'",
                 seq, uuid) != -1) {
        sqlite3_exec(peer, sql, NULL, NULL, NULL);
        free(sql);
    }
}

/*!
 * @brief Read the volume's Depleted mark
 *
 * Set when the CNID space is exhausted; while it stands, the backend discards
 * every AppleDouble CNID hint offered to it.
 *
 * @returns the mark, or -1 when it cannot be read
 */
static int cnid_peer_get_depleted(sqlite3 *peer, const char *uuid)
{
    sqlite3_stmt *stmt = NULL;
    int depleted = -1;
    char *sql = NULL;

    if (asprintf(&sql, "SELECT Depleted FROM volumes WHERE VolUUID = '%s'",
                 uuid) == -1) {
        return -1;
    }

    if (sqlite3_prepare_v2(peer, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            depleted = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
    }

    free(sql);
    return depleted;
}

/*!
 * @brief Every CNID_INVALID from the wrapper carries its own errno
 *
 * Callers branch on the CNID_ERR_* values to choose between a permanent reply
 * and a retryable one, and get_id() (etc/afpd/file.c) ends the session on
 * CNID_ERR_DB, so a rejection sets errno itself rather than leaving an earlier
 * operation's value in place. The poison value here, EPERM, is what a failed
 * chown or seteuid leaves behind.
 */
int utest_cnid_wrapper_sets_errno(void)
{
    cnid_t id;
    errno = EPERM;
    /* len == 0 is rejected before cdb is dereferenced, so NULL is safe here */
    id = cnid_add(NULL, NULL, htonl(2), "name", 0, CNID_INVALID);

    if (id != CNID_INVALID) {
        return 1;
    }

    if (CNID_ERRNO() != CNID_ERR_PARAM) {
        return 2;
    }

    return 0;
}

static cnid_t fake_get_result;

/*!
 * @brief Backend cnid_get that answers with fake_get_result, whatever is asked
 */
static cnid_t fake_cnid_get(struct _cnid_db *cdb _U_, cnid_t did _U_,
                            const char *name _U_, size_t len _U_)
{
    return fake_get_result;
}

/*!
 * @brief valide() compares CNIDs in the right byte order
 *
 * Backends return CNIDs in network byte order, and valide()
 * (libatalk/cnid/cnid.c) screens them against the host-order CNID_START of 17.
 * On a little-endian host 16 valid ids byte-swap to below that bound, the
 * lowest being host id 0x01000000, and the reserved ids 1-16 swap to above it.
 */
int utest_cnid_valide_byteorder(void)
{
    struct _cnid_db cdb = { 0 };
    char name[] = "name";
    cnid_t id;
    cdb.cnid_get = fake_cnid_get;
    /* Host id 0x01000000 is valid; its network representation on
     * little-endian is 1, below CNID_START */
    fake_get_result = htonl(0x01000000u);
    errno = 0;
    id = cnid_get(&cdb, htonl(2), name, 4);

    if (id != fake_get_result) {
        return 1;
    }

    /* Host id 3 is reserved and must be rejected whatever the host order */
    fake_get_result = htonl(3);
    errno = 0;
    id = cnid_get(&cdb, htonl(2), name, 4);

    if (id != CNID_INVALID) {
        return 2;
    }

    if (CNID_ERRNO() != CNID_ERR_CORRUPT) {
        return 3;
    }

    return 0;
}

/*!
 * @brief A contended CNID database classifies as BUSY, not as a dead backend
 *
 * A peer connection holding the write lock is ordinary contention between two
 * sessions on one volume. afpd ends the session on CNID_ERR_DB (get_id() in
 * etc/afpd/file.c), so contention must not report as one. The blocked add
 * waits out the backend's busy timeout, so this test stalls for that long.
 */
int utest_cnid_add_busy_not_fatal(struct vol *vol)
{
    sqlite3 *peer;
    char probe[MAXPATHLEN];
    const char *name;
    struct stat st;
    cnid_t id;
    int fd;
    int result = 0;

    /* NULL when the volume is not on the sqlite backend: a leftover .sqlite
     * file beside a differently-backed volume must not be locked instead */
    if ((peer = cnid_peer_open(vol)) == NULL) {
        return TEST_SKIP;
    }

    snprintf(probe, sizeof(probe), "%s/cnid_busy_XXXXXX", vol->v_path);

    if ((fd = mkstemp(probe)) < 0) {
        sqlite3_close(peer);
        return 1;
    }

    if (fstat(fd, &st) != 0) {
        result = 2;
        goto exit;
    }

    name = strrchr(probe, '/') + 1;

    /* Take the writer lock the way a sibling session's transaction would */
    if (sqlite3_exec(peer, "BEGIN EXCLUSIVE", NULL, NULL, NULL) != SQLITE_OK) {
        result = 4;
        goto exit;
    }

    errno = 0;
    id = cnid_add(vol->v_cdb, &st, htonl(2), name, strlen(name), CNID_INVALID);

    if (id != CNID_INVALID) {
        /* The row this unexpected success minted must not outlive the test */
        cnid_delete(vol->v_cdb, id);
        result = 5;
        goto exit;
    }

    if (CNID_ERRNO() != CNID_ERR_BUSY) {
        result = 6;
        goto exit;
    }

    sqlite3_exec(peer, "ROLLBACK", NULL, NULL, NULL);
    sqlite3_close(peer);
    peer = NULL;
    /* The lock is gone: the very same add must now succeed */
    id = cnid_add(vol->v_cdb, &st, htonl(2), name, strlen(name), CNID_INVALID);

    if (id == CNID_INVALID) {
        result = 7;
        goto exit;
    }

    cnid_delete(vol->v_cdb, id);
exit:

    if (peer) {
        sqlite3_exec(peer, "ROLLBACK", NULL, NULL, NULL);
        sqlite3_close(peer);
    }

    close(fd);
    unlink(probe);
    return result;
}

/*!
 * @brief A UUID differing only in case reuses the live table, never drops it
 *
 * SQLite table names are case-insensitive, but the stale-volume scan compares
 * VolUUID bytewise. A second session under a case-flipped spelling of the same
 * UUID must therefore find the same table, and the CNIDs in it must survive.
 */
int utest_cnid_uuid_case_keeps_table(struct vol *vol)
{
    struct vol peer_vol;
    struct _cnid_db *peer_db;
    char *flipped = NULL;
    char probe[MAXPATHLEN];
    char resolved[MAXPATHLEN];
    const char *name;
    struct stat st;
    cnid_t id;
    cnid_t did;
    int fd;
    int result = 0;

    if (vol->v_cdb == NULL || vol->v_uuid == NULL
            || vol->v_cnidscheme == NULL
            || strcmp(vol->v_cnidscheme, "sqlite") != 0) {
        return TEST_SKIP;
    }

    if ((flipped = strdup(vol->v_uuid)) == NULL) {
        return 1;
    }

    for (char *c = flipped; *c != '\0'; c++) {
        unsigned char ch = *c;
        *c = (char)(isupper(ch) ? tolower(ch) : toupper(ch));
    }

    /* An all-digit UUID has nothing to flip */
    if (strcmp(flipped, vol->v_uuid) == 0) {
        free(flipped);
        return TEST_SKIP;
    }

    snprintf(probe, sizeof(probe), "%s/cnid_uuidcase_XXXXXX", vol->v_path);

    if ((fd = mkstemp(probe)) < 0) {
        free(flipped);
        return 2;
    }

    if (fstat(fd, &st) != 0) {
        result = 3;
        goto close_probe;
    }

    name = strrchr(probe, '/') + 1;
    id = cnid_add(vol->v_cdb, &st, htonl(2), name, strlen(name), CNID_INVALID);

    if (id == CNID_INVALID) {
        result = 4;
        goto close_probe;
    }

    peer_vol = *vol;
    peer_vol.v_uuid = flipped;
    peer_vol.v_cdb = NULL;
    peer_db = cnid_open(&peer_vol, vol->v_cnidscheme, 0);

    if (peer_db == NULL) {
        result = 5;
        goto delete_id;
    }

    cnid_close(peer_db);
    did = id;

    if (cnid_resolve(vol->v_cdb, &did, resolved, sizeof(resolved)) == NULL
            || strcmp(resolved, name) != 0) {
        result = 6;
    }

delete_id:
    cnid_delete(vol->v_cdb, id);
close_probe:
    close(fd);
    unlink(probe);
    free(flipped);
    return result;
}

/*!
 * @brief Exhausting the CNID space reports CNID_ERR_RESET, not a recycled id
 *
 * The 32-bit ceiling empties the table, so every CNID any session holds now
 * names nothing or, as the table refills, another file. Returning a fresh id
 * as though nothing happened is what the classification exists to prevent.
 */
int utest_cnid_add_depletion_resets(struct vol *vol)
{
    sqlite3 *peer;
    char *uuid = NULL;
    char probe[MAXPATHLEN];
    const char *name;
    struct stat st;
    long long saved_seq;
    cnid_t id;
    int fd;
    int result = 0;
    int depleted = 0;

    if ((peer = cnid_peer_open(vol)) == NULL) {
        return TEST_SKIP;
    }

    if ((uuid = uuid_strip_dashes(vol->v_uuid)) == NULL) {
        sqlite3_close(peer);
        return 1;
    }

    if ((saved_seq = cnid_peer_get_seq(peer, uuid)) < 0) {
        /* Without the mark there is no way to put the sequence back */
        result = TEST_SKIP;
        goto exit;
    }

    snprintf(probe, sizeof(probe), "%s/cnid_depleted_XXXXXX", vol->v_path);

    if ((fd = mkstemp(probe)) < 0) {
        result = 1;
        goto exit;
    }

    if (fstat(fd, &st) != 0) {
        result = 2;
        goto close_probe;
    }

    name = strrchr(probe, '/') + 1;
    /* The next AUTOINCREMENT rowid is the last CNID the space can hold */
    cnid_peer_set_seq(peer, uuid, (long long) UINT32_MAX - 1);
    errno = 0;
    /* Past here the add may have emptied the table and marked the volume,
     * whatever it returns, so the cleanup below has to undo that */
    depleted = 1;
    id = cnid_add(vol->v_cdb, &st, htonl(2), name, strlen(name), CNID_INVALID);

    if (id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, id);
        result = 3;
        goto close_probe;
    }

    if (CNID_ERRNO() != CNID_ERR_RESET) {
        result = 4;
    }

close_probe:
    close(fd);
    unlink(probe);
exit:

    /* Reaching the ceiling marked the volume depleted, on disk and latched in
     * this connection — left set, every later test and run would have its
     * AppleDouble CNID hints discarded. cnid_wipe() is the path that clears
     * both; it reseeds the sequence, so the saved mark goes back after it. */
    if (depleted && cnid_wipe(vol->v_cdb) != 0) {
        fprintf(stderr, "# utest_cnid_add_depletion_resets: cnid_wipe failed, "
                "volume '%s' left marked depleted\n", vol->v_localname);

        if (result == 0) {
            result = 5;
        }
    }

    cnid_peer_set_seq(peer, uuid, saved_seq);

    /* An unreadable mark says nothing about the cleanup, so only a mark still
     * standing fails */
    if (depleted && cnid_peer_get_depleted(peer, uuid) > 0) {
        fprintf(stderr, "# utest_cnid_add_depletion_resets: volume '%s' still "
                "marked depleted after cleanup\n", vol->v_localname);

        if (result == 0) {
            result = 6;
        }
    }

    free(uuid);
    sqlite3_close(peer);
    return result;
}

/*!
 * @brief The CNID error codes stay distinct and out of the errno range
 *
 * The codes travel in errno and are compared against it, so each must be
 * unique and clear of every system errno value. Their handling diverges
 * sharply: get_id() (etc/afpd/file.c) ends the session on CNID_ERR_DB and
 * fails just the one operation on the rest.
 */
int utest_cnid_error_codes_distinct(void)
{
    static const int codes[] = {
        CNID_ERR_PARAM, CNID_ERR_PATH, CNID_ERR_DB, CNID_ERR_MAX,
        CNID_ERR_CLOSE, CNID_ERR_RESET, CNID_ERR_BUSY, CNID_ERR_CORRUPT,
        CNID_ERR_NOTFOUND,
    };
    const int count = (int)(sizeof(codes) / sizeof(codes[0]));

    for (int i = 0; i < count; i++) {
        /* Above INT_MAX, well clear of any system errno, so a stale errno
         * cannot read as a CNID condition */
        if ((unsigned int)codes[i] < 0x80000000u) {
            return 1;
        }

        for (int j = i + 1; j < count; j++) {
            if (codes[i] == codes[j]) {
                return 2;
            }
        }
    }

    return 0;
}

/*!
 * @brief A not-found answer classifies out of the syscall errno range
 *
 * Callers that switch on raw errno map EPERM to AFPERR_ACCESS (movecwd() in
 * etc/afpd/directory.c), so an absent row answers CNID_ERR_NOTFOUND. The CNID
 * probed here is high in the range and has no row.
 */
int utest_cnid_resolve_notfound_errno(struct vol *vol)
{
    char buf[MAXPATHLEN];
    cnid_t id = htonl(0x00fffff0u);

    if (vol->v_cdb == NULL || vol->v_cnidscheme == NULL
            || strcmp(vol->v_cnidscheme, "sqlite") != 0) {
        return TEST_SKIP;
    }

    errno = 0;

    if (cnid_resolve(vol->v_cdb, &id, buf, sizeof(buf)) != NULL) {
        return 1;
    }

    if (CNID_ERRNO() != CNID_ERR_NOTFOUND) {
        return 2;
    }

    return 0;
}

static char fake_resolve_dot[] = ".";
static char fake_resolve_dotdot[] = "..";
static char *fake_resolve_name;

/*!
 * @brief Backend cnid_resolve that answers fake_resolve_name, a corrupt name
 *        to reject
 */
static char *fake_cnid_resolve(struct _cnid_db *cdb _U_, cnid_t *id,
                               void *buffer _U_, size_t len _U_,
                               uint64_t *dev _U_, uint64_t *ino _U_)
{
    *id = htonl(2);
    return fake_resolve_name;
}

/*!
 * @brief The wrapper's "." and ".." rejection fails like a backend failure
 *
 * cnid_resolve() (libatalk/cnid/cnid.c) refuses either name from the backend
 * as a whole failure: NULL return, *id overwritten with CNID_INVALID in place
 * of the parent DID the backend wrote, and errno set to CNID_ERR_CORRUPT.
 */
int utest_cnid_resolve_dot_names_rejected(void)
{
    char *const names[] = { fake_resolve_dot, fake_resolve_dotdot };
    struct _cnid_db cdb = { 0 };
    char buf[64];
    cdb.cnid_resolve = fake_cnid_resolve;

    for (int i = 0; i < 2; i++) {
        cnid_t id = htonl(1234);
        fake_resolve_name = names[i];
        errno = 0;

        if (cnid_resolve(&cdb, &id, buf, sizeof(buf)) != NULL) {
            return 1;
        }

        if (id != CNID_INVALID) {
            return 2;
        }

        if (CNID_ERRNO() != CNID_ERR_CORRUPT) {
            return 3;
        }
    }

    return 0;
}

/* The searches fake_seeds_find() saw, and what it answers */
static struct {
    int found;              /*!< matches the search has */
    bool fails;             /*!< whether the search fails */
    int runs;               /*!< searches that ran */
    int nseeds;             /*!< seeds the last search took */
    size_t literals;        /*!< literal bytes of its first seed */
    bool narrows;           /*!< what fake_cnid_find_narrows() answers */
    cnid_t scope_did;       /*!< the folder fake_cnid_find_narrows() saw */
} fake_seeds;

/*!
 * @brief Backend cnid_find that answers as fake_seeds says, and notes the
 *        seeds it takes and the literal bytes of the first
 *
 * @param[in]  cdb             unused
 * @param[in]  query           unused
 * @param[in]  seeds           the seeds, or NULL
 * @param[in]  nseeds          number of seeds
 * @param[in]  scope_did       unused
 * @param[out] buffer          receives CNIDs 17 onwards
 * @param[in]  buflen          size of buffer
 * @param[out] more_available  receives whether more matches exist
 * @returns the CNIDs written, or -1 when the search fails
 */
static int fake_seeds_find(struct _cnid_db *cdb _U_,
                           const struct sl_name_query *query _U_,
                           const struct sl_name_pattern *const *seeds,
                           int nseeds, cnid_t scope_did _U_, void *buffer,
                           size_t buflen, bool *more_available)
{
    cnid_t *cnids = buffer;
    int room = (int)(buflen / sizeof(cnid_t));
    int n = fake_seeds.found < room ? fake_seeds.found : room;
    fake_seeds.runs++;
    fake_seeds.nseeds = seeds != NULL ? nseeds : 0;
    fake_seeds.literals = fake_seeds.nseeds > 0
                          ? sl_name_pattern_literals(seeds[0]) : 0;

    if (fake_seeds.fails) {
        return -1;
    }

    for (int i = 0; i < n; i++) {
        cnids[i] = htonl((uint32_t)(17 + i));
    }

    *more_available = fake_seeds.found > n;
    return n;
}

/*!
 * @brief Backend cnid_find_narrows that answers as fake_seeds says, and notes
 *        the search's folder
 *
 * @param[in] cdb        unused
 * @param[in] seed       unused
 * @param[in] scope_did  the search's folder
 * @returns fake_seeds.narrows
 */
static bool fake_cnid_find_narrows(struct _cnid_db *cdb _U_,
                                   const struct sl_name_pattern *seed _U_,
                                   cnid_t scope_did)
{
    fake_seeds.scope_did = scope_did;
    return fake_seeds.narrows;
}

/*!
 * @brief cnid_find() accepts a name of MAXPATHLEN bytes and rejects one more
 *
 * The wrapper's bound is the path-buffer limit; a longer name, or an empty
 * one, is a parameter error that never reaches the backend.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_find_name_bound(void)
{
    struct _cnid_db cdb = { 0 };
    char name[MAXPATHLEN + 1];
    cnid_t results[CNID_FIND_MIN_RESULTS];
    bool more = false;
    cdb.cnid_find = fake_seeds_find;
    cdb.cnid_find_narrows = fake_cnid_find_narrows;
    memset(&fake_seeds, 0, sizeof(fake_seeds));
    fake_seeds.narrows = true;
    memset(name, 'a', MAXPATHLEN);
    name[MAXPATHLEN] = '\0';
    errno = 0;

    if (cnid_find(&cdb, name, MAXPATHLEN, results, sizeof(results), &more) != 0
            || fake_seeds.literals != MAXPATHLEN) {
        return 1;
    }

    errno = 0;

    if (cnid_find(&cdb, name, MAXPATHLEN + 1, results, sizeof(results),
                  &more) != -1 || fake_seeds.runs != 1) {
        return 2;
    }

    if (CNID_ERRNO() != CNID_ERR_PARAM) {
        return 3;
    }

    errno = 0;

    if (cnid_find(&cdb, name, 0, results, sizeof(results), &more) != -1
            || fake_seeds.runs != 1) {
        return 4;
    }

    if (CNID_ERRNO() != CNID_ERR_PARAM) {
        return 5;
    }

    return 0;
}

/*!
 * @brief A query's seeds reach the backend in one search
 *
 * None go when the backend does not narrow by them, the backend's test sees
 * the search's folder, and a failed search leaves more_available false.
 *
 * @returns 0, or the number of the first failed check
 */
int utest_cnid_find_seeds_together(void)
{
    static const char either[] = "kMDItemFSName==\"*alpha*\""
                                 "||kMDItemFSName==\"*beta*\"";
    struct _cnid_db cdb = { 0 };
    cnid_t results[CNID_FIND_MIN_RESULTS];
    struct sl_name_query *q = sl_name_query_parse(either, 0, 8);
    bool more = true;
    int result = 0;
    cdb.cnid_find = fake_seeds_find;
    cdb.cnid_find_narrows = fake_cnid_find_narrows;

    if (q == NULL) {
        return 1;
    }

    memset(&fake_seeds, 0, sizeof(fake_seeds));
    fake_seeds.found = CNID_FIND_MIN_RESULTS + 1;
    fake_seeds.narrows = true;

    if (cnid_find_scoped(&cdb, q, CNID_INVALID, results, sizeof(results),
                         &more) != CNID_FIND_MIN_RESULTS || !more
            || fake_seeds.runs != 1 || fake_seeds.nseeds != 2) {
        result = 2;
    }

    memset(&fake_seeds, 0, sizeof(fake_seeds));
    fake_seeds.found = 3;

    if (result == 0 && (cnid_find_scoped(&cdb, q, CNID_INVALID, results,
                                         sizeof(results), &more) != 3 || more
                        || fake_seeds.runs != 1 || fake_seeds.nseeds != 0)) {
        result = 3;
    }

    memset(&fake_seeds, 0, sizeof(fake_seeds));
    fake_seeds.narrows = true;

    if (result == 0 && (cnid_find_scoped(&cdb, q, htonl(17), results,
                                         sizeof(results), &more) != 0
                        || fake_seeds.scope_did != htonl(17))) {
        result = 5;
    }

    memset(&fake_seeds, 0, sizeof(fake_seeds));
    fake_seeds.narrows = true;
    fake_seeds.fails = true;
    more = true;

    if (result == 0 && (cnid_find_scoped(&cdb, q, CNID_INVALID, results,
                                         sizeof(results), &more) != -1
                        || more)) {
        result = 4;
    }

    sl_name_query_free(q);
    return result;
}

/*!
 * @brief 64-bit rows that do not fit a CNID classify as corrupt, not truncate
 *
 * A narrowed rowid names an unrelated live file, to hand out or to delete
 * through, so an out-of-range Id or Did fails as CNID_ERR_CORRUPT. The db file
 * is mode 0666, so such a row is one plain INSERT away.
 */
int utest_cnid_corrupt_row_classified(struct vol *vol)
{
    sqlite3 *peer;
    char *uuid = NULL;
    char *sql = NULL;
    char buf[MAXPATHLEN];
    char name_id[] = "cnid_corrupt_id";
    cnid_t id;
    int result = 0;

    if ((peer = cnid_peer_open(vol)) == NULL) {
        return TEST_SKIP;
    }

    if ((uuid = uuid_strip_dashes(vol->v_uuid)) == NULL) {
        sqlite3_close(peer);
        return 1;
    }

    long long saved_seq = cnid_peer_get_seq(peer, uuid);

    /* The row below raises the high-water mark past the CNID range, so it is
     * only planted when the mark can be restored afterwards */
    if (saved_seq < 0) {
        free(uuid);
        sqlite3_close(peer);
        return TEST_SKIP;
    }

    /* Id 2^32 + 17, which narrows to CNID 17, the first valid id */
    if (asprintf(&sql,
                 "INSERT INTO \"%s\" (Id, Name, Did, DevNo, InodeNo) "
                 "VALUES (4294967313, 'cnid_corrupt_id', 2, 424241, 4242424241)",
                 uuid) == -1) {
        /* asprintf leaves the pointer indeterminate on failure, so it must not
         * reach the free() at the exit label */
        sql = NULL;
        result = 2;
        goto exit;
    }

    if (sqlite3_exec(peer, sql, NULL, NULL, NULL) != SQLITE_OK) {
        result = 2;
        goto exit;
    }

    free(sql);
    sql = NULL;
    errno = 0;
    id = cnid_get(vol->v_cdb, htonl(2), name_id, strlen(name_id));

    if (id != CNID_INVALID) {
        result = 3;
        goto exit;
    }

    if (CNID_ERRNO() != CNID_ERR_CORRUPT) {
        result = 4;
        goto exit;
    }

    /* Did too large for a CNID on the resolve read */
    if (asprintf(&sql,
                 "INSERT INTO \"%s\" (Id, Name, Did, DevNo, InodeNo) "
                 "VALUES (16777001, 'cnid_corrupt_did', 4294967300, 424242, 4242424242)",
                 uuid) == -1) {
        sql = NULL;
        result = 5;
        goto exit;
    }

    if (sqlite3_exec(peer, sql, NULL, NULL, NULL) != SQLITE_OK) {
        result = 5;
        goto exit;
    }

    free(sql);
    sql = NULL;
    id = htonl(16777001);
    errno = 0;

    if (cnid_resolve(vol->v_cdb, &id, buf, sizeof(buf)) != NULL) {
        result = 6;
        goto exit;
    }

    if (CNID_ERRNO() != CNID_ERR_CORRUPT) {
        result = 7;
        goto exit;
    }

exit:

    if (sql) {
        free(sql);
    }

    if (asprintf(&sql, "DELETE FROM \"%s\" WHERE Name LIKE 'cnid_corrupt_%%'",
                 uuid) != -1) {
        sqlite3_exec(peer, sql, NULL, NULL, NULL);
        free(sql);
    }

    cnid_peer_set_seq(peer, uuid, saved_seq);
    free(uuid);
    sqlite3_close(peer);
    return result;
}

/*!
 * @brief A search never returns a CNID narrowed out of a 64-bit rowid
 *
 * cnid_find() feeds FPCatSearch, and a client opens what the search reports, so
 * a narrowed Id hands it an unrelated live file. The out-of-range row is left
 * out of the results.
 */
int utest_cnid_find_no_truncated_id(struct vol *vol)
{
    sqlite3 *peer;
    char *uuid = NULL;
    char *sql = NULL;
    char name[] = "cnid_find_trunc";
    cnid_t results[CNID_FIND_MIN_RESULTS];
    bool more = false;
    int count;
    int result = 0;

    if ((peer = cnid_peer_open(vol)) == NULL) {
        return TEST_SKIP;
    }

    if ((uuid = uuid_strip_dashes(vol->v_uuid)) == NULL) {
        sqlite3_close(peer);
        return 1;
    }

    long long saved_seq = cnid_peer_get_seq(peer, uuid);

    if (saved_seq < 0) {
        free(uuid);
        sqlite3_close(peer);
        return TEST_SKIP;
    }

    /* Id 2^32 + 4242 narrows to 4242, the id the search must not report for
     * this name */
    if (asprintf(&sql,
                 "INSERT INTO \"%s\" (Id, Name, Did, DevNo, InodeNo) "
                 "VALUES (4294971538, 'cnid_find_trunc', 2, 434341, 4343434341)",
                 uuid) == -1) {
        sql = NULL;
        result = 2;
        goto exit;
    }

    if (sqlite3_exec(peer, sql, NULL, NULL, NULL) != SQLITE_OK) {
        result = 2;
        goto exit;
    }

    free(sql);
    sql = NULL;
    errno = 0;
    count = cnid_find(vol->v_cdb, name, strlen(name), results, sizeof(results),
                      &more);

    if (count < 0) {
        result = 3;
        goto exit;
    }

    for (int i = 0; i < count; i++) {
        if (ntohl(results[i]) == 4242) {
            result = 4;
            goto exit;
        }
    }

exit:

    if (sql) {
        free(sql);
    }

    if (asprintf(&sql, "DELETE FROM \"%s\" WHERE Name = 'cnid_find_trunc'",
                 uuid) != -1) {
        sqlite3_exec(peer, sql, NULL, NULL, NULL);
        free(sql);
    }

    cnid_peer_set_seq(peer, uuid, saved_seq);
    free(uuid);
    sqlite3_close(peer);
    return result;
}

/*!
 * @brief Duplicate-row cleanup never deletes through a truncated rowid
 *
 * Two rows can legitimately satisfy lookup's disjuncts, (Name, Did) and
 * (DevNo, InodeNo), and the surplus one is deleted. A surplus Id that does not
 * fit a CNID narrows onto an unrelated live row, so it is reported as
 * corruption and both rows are left in place.
 */
int utest_cnid_dup_row_no_truncated_delete(struct vol *vol)
{
    sqlite3 *peer;
    char *uuid = NULL;
    char *sql = NULL;
    char name_a[] = "cnid_dup_a";
    struct stat st;
    cnid_t id;
    int result = 0;

    if ((peer = cnid_peer_open(vol)) == NULL) {
        return TEST_SKIP;
    }

    if (vol->v_cdb->cnid_db_flags & CNID_FLAG_NODEV) {
        sqlite3_close(peer);
        return TEST_SKIP;
    }

    if ((uuid = uuid_strip_dashes(vol->v_uuid)) == NULL) {
        sqlite3_close(peer);
        return 1;
    }

    long long saved_seq = cnid_peer_get_seq(peer, uuid);

    /* As above: row B's explicit rowid lifts the high-water mark out of the
     * CNID range, so it is only planted when the mark can be restored */
    if (saved_seq < 0) {
        free(uuid);
        sqlite3_close(peer);
        return TEST_SKIP;
    }

    /* A matches lookup by (Name, Did); B matches by (DevNo, InodeNo) with an
     * Id of 2^32 + 99999926, which narrows to C's Id; C matches nothing and is
     * the live bystander a narrowed DELETE reaches */
    if (asprintf(&sql,
                 "INSERT INTO \"%s\" (Id, Name, Did, DevNo, InodeNo) VALUES "
                 "(99999903, 'cnid_dup_a', 2, 515151, 616161), "
                 "(4394967222, 'cnid_dup_b', 3, 717171, 818181), "
                 "(99999926, 'cnid_dup_c', 5, 919191, 101010)",
                 uuid) == -1) {
        sql = NULL;
        result = 2;
        goto exit;
    }

    if (sqlite3_exec(peer, sql, NULL, NULL, NULL) != SQLITE_OK) {
        result = 2;
        goto exit;
    }

    free(sql);
    sql = NULL;
    memset(&st, 0, sizeof(st));
    st.st_dev = 717171;
    st.st_ino = 818181;
    errno = 0;
    id = cnid_lookup(vol->v_cdb, &st, htonl(2), name_a, strlen(name_a));

    if (id != CNID_INVALID) {
        result = 3;
        goto exit;
    }

    /* Which row the cursor returned first is unspecified: the out-of-range
     * surplus classifies as corrupt, a deletable surplus as not-found */
    if (CNID_ERRNO() != CNID_ERR_CORRUPT
            && CNID_ERRNO() != CNID_ERR_NOTFOUND) {
        result = 4;
        goto exit;
    }

    /* The bystander and the out-of-range row itself must both survive */
    sqlite3_stmt *check = NULL;

    if (asprintf(&sql,
                 "SELECT COUNT(*) FROM \"%s\" WHERE Id IN (99999926, 4394967222)",
                 uuid) == -1) {
        sql = NULL;
        result = 6;
        goto exit;
    }

    if (sqlite3_prepare_v2(peer, sql, -1, &check, NULL) != SQLITE_OK) {
        result = 6;
        goto exit;
    }

    if (sqlite3_step(check) != SQLITE_ROW
            || sqlite3_column_int(check, 0) != 2) {
        result = 7;
    }

    sqlite3_finalize(check);
exit:

    if (sql) {
        free(sql);
    }

    if (asprintf(&sql, "DELETE FROM \"%s\" WHERE Name LIKE 'cnid_dup_%%'",
                 uuid) != -1) {
        sqlite3_exec(peer, sql, NULL, NULL, NULL);
        free(sql);
    }

    cnid_peer_set_seq(peer, uuid, saved_seq);
    free(uuid);
    sqlite3_close(peer);
    return result;
}

/*!
 * @brief cnid_lookup() drops by CNID a surplus row with an out-of-range parent
 *
 * @param[in] vol  the harness's open volume
 *
 * @returns 0 when the lookup answers CNID_ERR_NOTFOUND and the surplus row is
 *          gone, a step number otherwise, or TEST_SKIP without a second
 *          SQLite connection, on a backend that records no device, or where
 *          the volume's CNID sequence cannot be read
 */
int utest_cnid_dup_row_parent_out_of_range(struct vol *vol)
{
    sqlite3 *peer;
    sqlite3_stmt *check = NULL;
    char *uuid = NULL;
    char *sql = NULL;
    char name_a[] = "cnid_dup_pa";
    struct stat st;
    cnid_t id;
    bool planted = false;
    int result = 0;

    if ((peer = cnid_peer_open(vol)) == NULL) {
        return TEST_SKIP;
    }

    if (vol->v_cdb->cnid_db_flags & CNID_FLAG_NODEV) {
        sqlite3_close(peer);
        return TEST_SKIP;
    }

    if ((uuid = uuid_strip_dashes(vol->v_uuid)) == NULL) {
        sqlite3_close(peer);
        return 1;
    }

    long long saved_seq = cnid_peer_get_seq(peer, uuid);

    if (saved_seq < 0) {
        free(uuid);
        sqlite3_close(peer);
        return TEST_SKIP;
    }

    /* A matches the lookup by name and parent, B by device and inode with a
     * parent of 2^32 + 3 */
    if (asprintf(&sql,
                 "INSERT INTO \"%s\" (Id, Name, Did, DevNo, InodeNo) VALUES "
                 "(99999905, 'cnid_dup_pa', 2, 525252, 626262), "
                 "(99999906, 'cnid_dup_pb', 4294967299, 727272, 828282)",
                 uuid) == -1) {
        sql = NULL;
        result = 2;
        goto exit;
    }

    if (sqlite3_exec(peer, sql, NULL, NULL, NULL) != SQLITE_OK) {
        result = 2;
        goto exit;
    }

    planted = true;
    free(sql);
    sql = NULL;

    /* A comes first: SQLite reads OR terms in order; table scans go by Id */
    if (asprintf(&sql,
                 "SELECT Id,Did,Name,DevNo,InodeNo FROM \"%s\" "
                 "WHERE (Name = 'cnid_dup_pa' AND Did = 2) "
                 "OR (DevNo = 727272 AND InodeNo = 828282)",
                 uuid) == -1) {
        sql = NULL;
        result = 6;
        goto exit;
    }

    if (sqlite3_prepare_v2(peer, sql, -1, &check, NULL) != SQLITE_OK) {
        result = 6;
        goto exit;
    }

    if (sqlite3_step(check) != SQLITE_ROW
            || sqlite3_column_int64(check, 0) != 99999905) {
        sqlite3_finalize(check);
        result = 6;
        goto exit;
    }

    sqlite3_finalize(check);
    free(sql);
    sql = NULL;
    memset(&st, 0, sizeof(st));
    st.st_dev = 727272;
    st.st_ino = 828282;
    errno = 0;
    id = cnid_lookup(vol->v_cdb, &st, htonl(2), name_a, strlen(name_a));

    if (id != CNID_INVALID || CNID_ERRNO() != CNID_ERR_NOTFOUND) {
        result = 3;
        goto exit;
    }

    if (asprintf(&sql, "SELECT COUNT(*) FROM \"%s\" WHERE Id = 99999906",
                 uuid) == -1) {
        sql = NULL;
        result = 4;
        goto exit;
    }

    if (sqlite3_prepare_v2(peer, sql, -1, &check, NULL) != SQLITE_OK) {
        result = 4;
        goto exit;
    }

    if (sqlite3_step(check) != SQLITE_ROW
            || sqlite3_column_int(check, 0) != 0) {
        result = 5;
    }

    sqlite3_finalize(check);
exit:

    if (sql) {
        free(sql);
    }

    if (planted && asprintf(&sql, "DELETE FROM \"%s\" WHERE Id IN "
                                  "(99999905, 99999906)", uuid) != -1) {
        sqlite3_exec(peer, sql, NULL, NULL, NULL);
        free(sql);
    }

    cnid_peer_set_seq(peer, uuid, saved_seq);
    free(uuid);
    sqlite3_close(peer);
    return result;
}

/*!
 * @brief The process's supplementary groups
 *
 * @param[out] ngroups  their number
 *
 * @returns the list, which the caller frees, or NULL
 */
static gid_t *cnid_groups(int *ngroups)
{
    int n = getgroups(0, NULL);
    gid_t *groups = n < 0 ? NULL : malloc((size_t)(n + 1) * sizeof(gid_t));

    if (groups == NULL || (*ngroups = getgroups(n, groups)) < 0) {
        free(groups);
        return NULL;
    }

    return groups;
}

/*!
 * @brief Tell whether the process's supplementary groups are a list taken
 *        before
 *
 * @param[in] groups   the list
 * @param[in] ngroups  its number
 *
 * @returns true for the same groups in the same order
 */
static bool cnid_groups_are(const gid_t *groups, int ngroups)
{
    int n;
    gid_t *now = cnid_groups(&n);
    bool same = now != NULL && n == ngroups
                && memcmp(now, groups, (size_t)n * sizeof(gid_t)) == 0;
    free(now);
    return same;
}

/*!
 * @brief cnid_delete_row() deletes only while parent, name, dev and ino match
 *
 * @param[in] vol  the harness's open volume
 *
 * @returns 0 when calls that each change one of the four keep the row and
 *          answer 0, the call with the row's own removes it and answers 1,
 *          and one more answers 0
 */
int utest_cnid_delete_row_conditional(struct vol *vol)
{
    static char name[] = "cnid_delete_row";
    static char other[] = "cnid_delete_rox";
    char path[MAXPATHLEN + 1];
    char buf[CNID_MAX_PATH_LEN];
    struct stat st;
    /* a parent the row does not record */
    const cnid_t elsewhere = htonl(ntohl(DIRDID_ROOT) + 1);
    const size_t len = strlen(name);
    cnid_t id = CNID_INVALID;
    cnid_t probe;
    uint64_t dev;
    uint64_t ino;
    int kept[4] = { -1, -1, -1, -1 };
    bool row_kept = false;
    int gone = -1;
    int again = -1;
    int ret = -1;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);

    if (create_disk_file(path) != 0 || stat(path, &st) != 0
            || (id = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, name, strlen(name),
                              CNID_INVALID)) == CNID_INVALID) {
        goto out;
    }

    dev = cnid_row_dev(vol->v_cdb, &st);
    ino = (uint64_t)st.st_ino;
    kept[0] = cnid_delete_row(vol->v_cdb, id, DIRDID_ROOT, name, len, dev,
                              ino + 1);
    kept[1] = cnid_delete_row(vol->v_cdb, id, DIRDID_ROOT, name, len, dev + 1,
                              ino);
    kept[2] = cnid_delete_row(vol->v_cdb, id, elsewhere, name, len, dev, ino);
    kept[3] = cnid_delete_row(vol->v_cdb, id, DIRDID_ROOT, other, len, dev,
                              ino);
    probe = id;
    row_kept = cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf)) != NULL;
    gone = cnid_delete_row(vol->v_cdb, id, DIRDID_ROOT, name, len, dev, ino);
    again = cnid_delete_row(vol->v_cdb, id, DIRDID_ROOT, name, len, dev, ino);

    if (kept[0] == 0 && kept[1] == 0 && kept[2] == 0 && kept[3] == 0
            && row_kept && gone == 1 && again == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "cnid_delete_row: another inode %d, device %d, "
                        "parent %d, name %d, the row %s, its own %d, once "
                        "more %d\n", kept[0], kept[1], kept[2], kept[3],
                row_kept ? "kept" : "gone", gone, again);
    }

out:

    if (id != CNID_INVALID && gone != 1) {
        cnid_delete(vol->v_cdb, id);
    }

    unlink(path);
    return ret;
}

/*!
 * @brief cnid_update_row() rewrites only while parent, name, dev and ino match
 *
 * @param[in] vol  the harness's open volume
 *
 * @returns 0 when calls that each change one of the four keep the row and
 *          the other row and answer 0, the call with the row's own answers 1,
 *          the row then records the stat and the other row is gone, and one
 *          more answers 0
 */
int utest_cnid_update_row_conditional(struct vol *vol)
{
    static char name[] = "cnid_update_row";
    static char other[] = "cnid_update_rox";
    static char held[] = "cnid_update_held";
    char path[MAXPATHLEN + 1];
    char hpath[MAXPATHLEN + 1];
    char buf[CNID_MAX_PATH_LEN];
    struct stat st;
    struct stat hst;
    /* a parent the row does not record */
    const cnid_t elsewhere = htonl(ntohl(DIRDID_ROOT) + 1);
    const size_t len = strlen(name);
    cnid_t id = CNID_INVALID;
    cnid_t hid = CNID_INVALID;
    cnid_t probe;
    uint64_t dev;
    uint64_t ino;
    uint64_t now_dev = 0;
    uint64_t now_ino = 0;
    int kept[4] = { -1, -1, -1, -1 };
    bool other_kept = false;
    bool holds = false;
    int changed = -1;
    int again = -1;
    int ret = -1;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(hpath, sizeof(hpath), "%s/%s", vol->v_path, held);

    /* a file's row, and another's with the identity the first row takes */
    if (create_disk_file(path) != 0 || stat(path, &st) != 0
            || (id = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, name, len,
                              CNID_INVALID)) == CNID_INVALID
            || create_disk_file(hpath) != 0 || stat(hpath, &hst) != 0
            || (hid = cnid_add(vol->v_cdb, &hst, DIRDID_ROOT, held,
                               strlen(held), CNID_INVALID)) == CNID_INVALID) {
        goto out;
    }

    dev = cnid_row_dev(vol->v_cdb, &st);
    ino = (uint64_t)st.st_ino;
    kept[0] = cnid_update_row(vol->v_cdb, id, DIRDID_ROOT, name, len, dev,
                              ino + 1, &hst, NULL, NULL);
    kept[1] = cnid_update_row(vol->v_cdb, id, DIRDID_ROOT, name, len, dev + 1,
                              ino, &hst, NULL, NULL);
    kept[2] = cnid_update_row(vol->v_cdb, id, elsewhere, name, len, dev, ino,
                              &hst, NULL, NULL);
    kept[3] = cnid_update_row(vol->v_cdb, id, DIRDID_ROOT, other, len, dev,
                              ino, &hst, NULL, NULL);
    probe = hid;
    other_kept = cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf)) != NULL;
    changed = cnid_update_row(vol->v_cdb, id, DIRDID_ROOT, name, len, dev, ino,
                              &hst, NULL, NULL);
    again = cnid_update_row(vol->v_cdb, id, DIRDID_ROOT, name, len, dev, ino,
                            &hst, NULL, NULL);
    probe = id;
    holds = cnid_resolve_row(vol->v_cdb, &probe, buf, sizeof(buf), &now_dev,
                             &now_ino) != NULL
            && cnid_row_holds(vol->v_cdb, now_dev, now_ino, &hst);
    probe = hid;

    if (kept[0] == 0 && kept[1] == 0 && kept[2] == 0 && kept[3] == 0
            && other_kept && changed == 1 && again == 0 && holds
            && cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf)) == NULL
            && CNID_ERRNO() == CNID_ERR_NOTFOUND) {
        ret = 0;
    } else {
        fprintf(stderr, "cnid_update_row: another inode %d, device %d, "
                        "parent %d, name %d, the other row %s, its own %d, "
                        "once more %d, the row %s\n",
                kept[0], kept[1], kept[2], kept[3],
                other_kept ? "kept" : "gone", changed, again,
                holds ? "moved" : "unmoved");
    }

out:

    if (id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, id);
    }

    if (hid != CNID_INVALID) {
        cnid_delete(vol->v_cdb, hid);
    }

    unlink(path);
    unlink(hpath);
    return ret;
}

/* the statement run_hook_sql() runs, whether the hook ran, and whether the
 * statement did */
static const char *hook_sql;
static int hook_fired;
static int hook_ran;

/*!
 * @brief Run hook_sql on the backend's connection ahead of its statement
 */
static void run_hook_sql(void *con)
{
    hook_fired = 1;
    hook_ran = fi.sql.exec(con, hook_sql) == 0;
}

/*!
 * @brief cnid_update_row() of a row moved before its hold keeps the other row
 *
 * @param[in] vol  the harness's open volume
 *
 * @returns 0 when the call answers 0, the other row stays and the row is
 *          where the other session moved it, 1 otherwise, -1 when the
 *          fixture failed, or TEST_SKIP without the fault-injection shim
 */
int utest_cnid_update_row_moved_keeps_other(struct vol *vol)
{
    static char name[] = "cnid_update_rbk";
    static char held[] = "cnid_update_rbh";
    static char moved[] = "cnid_update_rbm";
    const bool mysql = vol->v_cnidscheme != NULL
                       && strcasecmp(vol->v_cnidscheme, "mysql") == 0;
    char path[MAXPATHLEN + 1];
    char hpath[MAXPATHLEN + 1];
    const char *quote = mysql ? "`" : "\"";
    char buf[CNID_MAX_PATH_LEN];
    char *uuid = NULL;
    char *sql = NULL;
    const char *now = NULL;
    struct stat st;
    struct stat hst;
    cnid_t id = CNID_INVALID;
    cnid_t hid = CNID_INVALID;
    cnid_t probe;
    bool held_kept = false;
    int changed = -1;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(hpath, sizeof(hpath), "%s/%s", vol->v_path, held);
    fault_inject_reset();

    /* the row, and another file's row with the identity the row is to take */
    if (create_disk_file(path) != 0 || stat(path, &st) != 0
            || (id = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, name,
                              strlen(name), CNID_INVALID)) == CNID_INVALID
            || create_disk_file(hpath) != 0 || stat(hpath, &hst) != 0
            || (hid = cnid_add(vol->v_cdb, &hst, DIRDID_ROOT, held,
                               strlen(held), CNID_INVALID)) == CNID_INVALID
            || (uuid = uuid_strip_dashes(vol->v_uuid)) == NULL
            || asprintf(&sql,
                        "UPDATE %s%s%s SET Name = '%s' WHERE Id = %" PRIu32,
                        quote, uuid, quote, moved, ntohl(id)) == -1) {
        sql = NULL;
        goto out;
    }

    /* the move lands ahead of SQLite's BEGIN and of InnoDB's row lock, which
     * the check's FOR UPDATE takes */
    hook_sql = sql;
    hook_fired = 0;
    hook_ran = 0;
    fi.sql.hook = run_hook_sql;
    fi.sql.prefix = mysql ? "SELECT Id FROM" : "BEGIN";
    changed = cnid_update_row(vol->v_cdb, id, DIRDID_ROOT, name, strlen(name),
                              cnid_row_dev(vol->v_cdb, &st),
                              (uint64_t)st.st_ino, &hst, NULL, NULL);
    fault_inject_reset();
    probe = hid;
    held_kept = cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf)) != NULL;
    probe = id;
    now = cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf));

    if (hook_ran && changed == 0 && held_kept && now != NULL
            && strcmp(now, moved) == 0) {
        ret = 0;
    } else {
        fprintf(stderr, "cnid_update_row, its row moved before the hold: %d, "
                        "the move %s, the other row %s, the row at %s\n",
                changed,
                !hook_fired ? "never ran" : hook_ran ? "ran" : "failed",
                held_kept ? "kept" : "gone", now != NULL ? now : "no name");
        ret = 1;
    }

out:
    fault_inject_reset();
    free(sql);
    free(uuid);

    if (id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, id);
    }

    if (hid != CNID_INVALID) {
        cnid_delete(vol->v_cdb, hid);
    }

    unlink(path);
    unlink(hpath);
    return ret;
}

/* the handle and the other row's CNID that replace_reads_other_row() reads,
 * and whether it found the row */
static struct _cnid_db *replace_cdb;
static cnid_t replace_other;
static int replace_read;

/*!
 * @brief A replace that reads another row through the update's own handle
 *
 * @returns 0
 */
static int replace_reads_other_row(void *arg _U_)
{
    char buf[CNID_MAX_PATH_LEN];
    cnid_t probe = replace_other;
    replace_read = cnid_resolve(replace_cdb, &probe, buf, sizeof(buf)) != NULL;
    return 0;
}

/*!
 * @brief cnid_update_row() whose replace reads another row changes only its own
 *
 * @param[in] vol  the harness's open volume
 *
 * @returns 0 when the call answers 1, the row records the stat and the other
 *          row its own file, 1 otherwise, or -1 when the fixture failed
 */
int utest_cnid_update_row_replace_calls(struct vol *vol)
{
    static char name[] = "cnid_update_rpl";
    static char other[] = "cnid_update_rpo";
    static char repl[] = "cnid_update_rpr";
    char path[MAXPATHLEN + 1];
    char opath[MAXPATHLEN + 1];
    char rpath[MAXPATHLEN + 1];
    char buf[CNID_MAX_PATH_LEN];
    struct stat st;
    struct stat ost;
    struct stat rst;
    cnid_t id = CNID_INVALID;
    cnid_t oid = CNID_INVALID;
    cnid_t probe;
    uint64_t now_dev = 0;
    uint64_t now_ino = 0;
    bool holds = false;
    bool other_kept = false;
    int changed = -1;
    int ret = -1;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(opath, sizeof(opath), "%s/%s", vol->v_path, other);
    snprintf(rpath, sizeof(rpath), "%s/%s", vol->v_path, repl);

    /* the row, another file's row, and the stat the row is to take */
    if (create_disk_file(path) != 0 || stat(path, &st) != 0
            || (id = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, name,
                              strlen(name), CNID_INVALID)) == CNID_INVALID
            || create_disk_file(opath) != 0 || stat(opath, &ost) != 0
            || (oid = cnid_add(vol->v_cdb, &ost, DIRDID_ROOT, other,
                               strlen(other), CNID_INVALID)) == CNID_INVALID
            || create_disk_file(rpath) != 0 || stat(rpath, &rst) != 0) {
        goto out;
    }

    replace_cdb = vol->v_cdb;
    replace_other = oid;
    replace_read = 0;
    changed = cnid_update_row(vol->v_cdb, id, DIRDID_ROOT, name, strlen(name),
                              cnid_row_dev(vol->v_cdb, &st),
                              (uint64_t)st.st_ino, &rst,
                              replace_reads_other_row, NULL);
    probe = id;
    holds = cnid_resolve_row(vol->v_cdb, &probe, buf, sizeof(buf), &now_dev,
                             &now_ino) != NULL
            && cnid_row_holds(vol->v_cdb, now_dev, now_ino, &rst);
    probe = oid;
    other_kept = cnid_resolve_row(vol->v_cdb, &probe, buf, sizeof(buf),
                                  &now_dev, &now_ino) != NULL
                 && cnid_row_holds(vol->v_cdb, now_dev, now_ino, &ost);

    if (changed == 1 && replace_read && holds && other_kept) {
        ret = 0;
    } else {
        fprintf(stderr, "cnid_update_row whose replace read another row: %d, "
                        "the read %s, the row %s, the other row %s\n", changed,
                replace_read ? "found it" : "failed",
                holds ? "changed" : "unchanged",
                other_kept ? "kept" : "changed");
        ret = 1;
    }

out:

    if (id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, id);
    }

    if (oid != CNID_INVALID) {
        cnid_delete(vol->v_cdb, oid);
    }

    unlink(path);
    unlink(opath);
    unlink(rpath);
    return ret;
}

/*!
 * @brief Run a statement on a volume's MySQL connection ahead of a stamp read
 *
 * @param[in] vol  the harness's open volume
 * @param[in] sql  the statement
 * @returns 1 when the statement ran, 0 when it failed, or -1 when the shim
 *          reached no stamp read
 */
static int reconnect_step(struct vol *vol, const char *sql)
{
    char stamp[sizeof(vol->v_stamp)];
    fault_inject_reset();
    hook_sql = sql;
    hook_fired = 0;
    hook_ran = 0;
    fi.sql.hook = run_hook_sql;
    fi.sql.prefix = "SELECT Stamp";
    (void)cnid_getstamp(vol->v_cdb, stamp, sizeof(stamp));
    fault_inject_reset();
    return hook_fired ? hook_ran : -1;
}

/*!
 * @brief cnid_update_row() on a connection the server dropped reconnects
 *
 * A session variable set before the drop tells the new session from the
 * old one.
 *
 * @param[in] vol  the harness's open volume
 *
 * @returns 0 when the call answers 1, the row records the stat and the
 *          connection is a new session, 1 otherwise, -1 when the fixture
 *          failed, or TEST_SKIP on a backend other than MySQL or without the
 *          fault-injection shim
 */
int utest_cnid_update_row_after_reconnect(struct vol *vol)
{
    static char name[] = "cnid_update_rcn";
    static char other[] = "cnid_update_rco";
    const bool mysql = vol->v_cnidscheme != NULL
                       && strcasecmp(vol->v_cnidscheme, "mysql") == 0;
    char path[MAXPATHLEN + 1];
    char opath[MAXPATHLEN + 1];
    char buf[CNID_MAX_PATH_LEN];
    struct stat st;
    struct stat ost;
    cnid_t id = CNID_INVALID;
    cnid_t probe;
    uint64_t now_dev = 0;
    uint64_t now_ino = 0;
    bool holds = false;
    int dropping = -1;
    int renewed = -1;
    int changed = -1;
    int ret = -1;

    if (!mysql || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(opath, sizeof(opath), "%s/%s", vol->v_path, other);

    if (create_disk_file(path) != 0 || stat(path, &st) != 0
            || (id = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, name,
                              strlen(name), CNID_INVALID)) == CNID_INVALID
            || create_disk_file(opath) != 0 || stat(opath, &ost) != 0) {
        goto out;
    }

    /* the server drops the connection once it idles for a second */
    if ((dropping = reconnect_step(vol, "SET SESSION wait_timeout = 1, "
                                        "@fi_drop = 1")) != 1) {
        goto out;
    }

    sleep(2);
    changed = cnid_update_row(vol->v_cdb, id, DIRDID_ROOT, name, strlen(name),
                              cnid_row_dev(vol->v_cdb, &st),
                              (uint64_t)st.st_ino, &ost, NULL, NULL);
    /* the subquery has a second row while the old session's variable
     * lives, and SET reports that error, which MariaDB's DO drops */
    renewed = reconnect_step(vol, "SET @fi_seen = (SELECT 1 UNION ALL SELECT "
                                  "2 FROM DUAL WHERE @fi_drop IS NOT NULL)");
    probe = id;
    holds = cnid_resolve_row(vol->v_cdb, &probe, buf, sizeof(buf), &now_dev,
                             &now_ino) != NULL
            && cnid_row_holds(vol->v_cdb, now_dev, now_ino, &ost);

    if (changed == 1 && holds && renewed == 1) {
        ret = 0;
    } else {
        fprintf(stderr, "cnid_update_row after a dropped connection: %d, the "
                        "row %s, the session %s\n", changed,
                holds ? "changed" : "unchanged",
                renewed == 1 ? "new" : renewed == 0 ? "the old one"
                : "unread");
        ret = 1;
    }

out:

    /* a session the server kept keeps its timeout */
    if (dropping == 1 && renewed != 1) {
        (void)reconnect_step(vol, "SET SESSION wait_timeout = DEFAULT");
    }

    if (id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, id);
    }

    unlink(path);
    unlink(opath);
    return ret;
}

/*!
 * @brief A MySQL open that fails after its connect closes the connection
 *
 * @param[in] vol  the harness's open volume
 *
 * @returns 0 when the open answers NULL and its session has ended, 1
 *          otherwise, or TEST_SKIP on a backend other than MySQL or without
 *          the fault-injection shim
 */
int utest_cnid_mysql_failed_open_closes(struct vol *vol)
{
    const bool mysql = vol->v_cnidscheme != NULL
                       && strcasecmp(vol->v_cnidscheme, "mysql") == 0;
    const struct timespec tick = { .tv_nsec = 100 * 1000 * 1000 };
    struct _cnid_db *cdb;
    int locked;
    int same = 0;
    int ret;

    if (!mysql || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    /* the open's session takes a named lock, which only its end releases */
    fault_inject_reset();
    hook_sql = "DO GET_LOCK('fi_failed_open', 0)";
    hook_fired = 0;
    hook_ran = 0;
    fi.sql.hook = run_hook_sql;
    fi.sql.prefix = "SELECT @@character_set_results";
    fi.sql.fail = 1;
    cdb = cnid_open(vol, "mysql", 0);
    locked = hook_ran;
    fault_inject_reset();

    /* the SET fails while the lock is held, as its subquery has two rows */
    for (int i = 0; locked && i < 20 && !same; i++) {
        same = reconnect_step(vol, "SET @fi_same = (SELECT 1 UNION ALL "
                                   "SELECT 2 FROM DUAL WHERE IS_USED_LOCK("
                                   "'fi_failed_open') IS NOT NULL)") == 1;

        if (!same) {
            nanosleep(&tick, NULL);
        }
    }

    if (cdb == NULL && same) {
        ret = 0;
    } else {
        fprintf(stderr, "cnid_open of MySQL failing after its connect: %s, "
                        "its session %s\n",
                cdb != NULL ? "opened" : "failed",
                !locked ? "untraced" : same ? "ended" : "still open");
        ret = 1;
    }

    if (cdb != NULL) {
        cnid_close(cdb);
    }

    return ret;
}

/* the volume whose connection replace_drops_connection() lets the server
 * drop, the row its CNID call reads, and whether the drop was armed */
static struct vol *replace_vol;
static cnid_t replace_id;
static int replace_dropping;

/*!
 * @brief A replace whose CNID call meets a connection the server dropped
 *
 * @returns 0
 */
static int replace_drops_connection(void *arg _U_)
{
    char buf[CNID_MAX_PATH_LEN];
    cnid_t probe = replace_id;
    replace_dropping = reconnect_step(replace_vol,
                                      "SET SESSION wait_timeout = 1, "
                                      "@fi_drop = 1");
    sleep(2);
    (void)cnid_resolve(replace_vol->v_cdb, &probe, buf, sizeof(buf));
    return 0;
}

/*!
 * @brief cnid_update_row() whose connection drops in the hold answers -1
 *
 * @param[in] vol  the harness's open volume
 *
 * @returns 0 when the call answers -1 and the row still records the file, 1
 *          otherwise, -1 when the fixture failed, or TEST_SKIP on a backend
 *          other than MySQL or without the fault-injection shim
 */
int utest_cnid_update_row_dropped_in_hold(struct vol *vol)
{
    static char name[] = "cnid_update_rrc";
    static char other[] = "cnid_update_rro";
    const bool mysql = vol->v_cnidscheme != NULL
                       && strcasecmp(vol->v_cnidscheme, "mysql") == 0;
    char path[MAXPATHLEN + 1];
    char opath[MAXPATHLEN + 1];
    char buf[CNID_MAX_PATH_LEN];
    struct stat st;
    struct stat ost;
    cnid_t id = CNID_INVALID;
    cnid_t probe;
    uint64_t now_dev = 0;
    uint64_t now_ino = 0;
    bool kept = false;
    int renewed = -1;
    int changed = -2;
    int ret = -1;

    if (!mysql || !test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(opath, sizeof(opath), "%s/%s", vol->v_path, other);

    if (create_disk_file(path) != 0 || stat(path, &st) != 0
            || (id = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, name,
                              strlen(name), CNID_INVALID)) == CNID_INVALID
            || create_disk_file(opath) != 0 || stat(opath, &ost) != 0) {
        goto out;
    }

    replace_vol = vol;
    replace_id = id;
    replace_dropping = -1;
    changed = cnid_update_row(vol->v_cdb, id, DIRDID_ROOT, name, strlen(name),
                              cnid_row_dev(vol->v_cdb, &st),
                              (uint64_t)st.st_ino, &ost,
                              replace_drops_connection, NULL);
    probe = id;
    kept = cnid_resolve_row(vol->v_cdb, &probe, buf, sizeof(buf), &now_dev,
                            &now_ino) != NULL
           && cnid_row_holds(vol->v_cdb, now_dev, now_ino, &st);
    /* the subquery has a second row while the dropped session lives */
    renewed = reconnect_step(vol, "SET @fi_seen = (SELECT 1 UNION ALL SELECT "
                                  "2 FROM DUAL WHERE @fi_drop IS NOT NULL)");

    if (replace_dropping != 1) {
        fprintf(stderr, "cnid_update_row, its connection dropped in the "
                        "hold: the drop not armed\n");
    } else if (changed == -1 && kept && renewed == 1) {
        ret = 0;
    } else {
        fprintf(stderr, "cnid_update_row, its connection dropped in the "
                        "hold: %d, the row %s, the session %s\n", changed,
                kept ? "kept" : "changed or unread",
                renewed == 1 ? "new" : renewed == 0 ? "the old one"
                : "unread");
        ret = 1;
    }

out:
    /* a session the server kept keeps its timeout otherwise */
    (void)reconnect_step(vol, "SET SESSION wait_timeout = DEFAULT");

    if (id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, id);
    }

    unlink(path);
    unlink(opath);
    return ret;
}

/*!
 * @brief cnid_add() keeps the CNID of a stale row rewritten before the delete
 *
 * @param[in] vol  the harness's open volume
 *
 * @returns 0 when the add answers the row's CNID, 1 when it answers another
 *          or the other session's write did not run, -1 when the fixture
 *          failed, or TEST_SKIP without the fault-injection shim
 */
int utest_cnid_lookup_row_changed(struct vol *vol)
{
    static char name[] = "cnid_lookup_chg";
    static char other[] = "cnid_lookup_chr";
    const bool mysql = vol->v_cnidscheme != NULL
                       && strcasecmp(vol->v_cnidscheme, "mysql") == 0;
    const char *quote = mysql ? "`" : "\"";
    char path[MAXPATHLEN + 1];
    char opath[MAXPATHLEN + 1];
    char *uuid = NULL;
    char *sql = NULL;
    struct stat st;
    struct stat ost;
    cnid_t id = CNID_INVALID;
    cnid_t oid = CNID_INVALID;
    cnid_t got = CNID_INVALID;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(opath, sizeof(opath), "%s/%s", vol->v_path, other);
    fault_inject_reset();

    /* another file takes the file's name, its own row deleted; the rewrite
     * sets the inode alone, as the files share a device whose number SQLite
     * would keep as a float from SQL text past 2^63 */
    if (create_disk_file_stat(path, &st) != 0
            || (id = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, name,
                              strlen(name), CNID_INVALID)) == CNID_INVALID
            || create_disk_file_stat(opath, &ost) != 0
            || (oid = cnid_add(vol->v_cdb, &ost, DIRDID_ROOT, other,
                               strlen(other), CNID_INVALID)) == CNID_INVALID
            || cnid_delete(vol->v_cdb, oid) != 0
            || rename(opath, path) != 0
            || (uuid = uuid_strip_dashes(vol->v_uuid)) == NULL
            || asprintf(&sql,
                        "UPDATE %s%s%s SET InodeNo = %" PRIu64
                        " WHERE Id = %" PRIu32, quote, uuid, quote,
                        (uint64_t)ost.st_ino, ntohl(id)) == -1) {
        sql = NULL;
        goto out;
    }

    hook_sql = sql;
    hook_fired = 0;
    hook_ran = 0;
    fi.sql.hook = run_hook_sql;
    fi.sql.prefix = "DELETE FROM";
    got = cnid_add(vol->v_cdb, &ost, DIRDID_ROOT, name, strlen(name),
                   CNID_INVALID);
    fault_inject_reset();

    if (hook_ran && got == id) {
        ret = 0;
    } else {
        fprintf(stderr, "cnid_add, its stale row rewritten before the "
                        "lookup's delete: %u for the row's %u, the rewrite "
                        "%s\n", ntohl(got), ntohl(id),
                !hook_fired ? "never ran" : hook_ran ? "ran"
                : "failed");
        ret = 1;
    }

out:
    fault_inject_reset();
    free(sql);
    sql = NULL;
    free(uuid);

    if (got != CNID_INVALID && got != id) {
        cnid_delete(vol->v_cdb, got);
    }

    /* the other file's row, where the fixture stopped before its delete */
    if (oid != CNID_INVALID && oid != got) {
        cnid_delete(vol->v_cdb, oid);
    }

    if (id != CNID_INVALID) {
        cnid_delete(vol->v_cdb, id);
    }

    unlink(path);
    unlink(opath);
    return ret;
}

/*!
 * @brief cnid_lookup() keeps stale rows changed before its delete
 *
 * @param[in] vol  the harness's open volume
 *
 * @returns 0 when both lookups answer CNID_ERR_NOTFOUND and every row they
 *          read stays, 1 otherwise, -1 when the fixture failed, or TEST_SKIP
 *          without the fault-injection shim
 */
int utest_cnid_lookup_rows_changed(struct vol *vol)
{
    static char moved[] = "cnid_lookup_mvd";
    static char was[] = "cnid_lookup_was";
    static char name[] = "cnid_lookup_two";
    static char other[] = "cnid_lookup_oth";
    const bool mysql = vol->v_cnidscheme != NULL
                       && strcasecmp(vol->v_cnidscheme, "mysql") == 0;
    const char *quote = mysql ? "`" : "\"";
    char mpath[MAXPATHLEN + 1];
    char npath[MAXPATHLEN + 1];
    char opath[MAXPATHLEN + 1];
    char buf[CNID_MAX_PATH_LEN];
    char *uuid = NULL;
    char *sql = NULL;
    struct stat mst;
    struct stat nst;
    struct stat ost;
    cnid_t rid = CNID_INVALID;
    cnid_t aid = CNID_INVALID;
    cnid_t bid = CNID_INVALID;
    cnid_t probe;
    cnid_t got_moved = CNID_INVALID;
    cnid_t got_two = CNID_INVALID;
    unsigned int moved_errno = 0;
    unsigned int two_errno = 0;
    bool moved_kept = false;
    bool both_kept = false;
    int moved_write = -1;
    int two_write = -1;
    int ret = -1;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    snprintf(mpath, sizeof(mpath), "%s/%s", vol->v_path, moved);
    snprintf(npath, sizeof(npath), "%s/%s", vol->v_path, name);
    snprintf(opath, sizeof(opath), "%s/%s", vol->v_path, other);
    fault_inject_reset();

    /* a file whose row records the name it had before it moved */
    if (create_disk_file(mpath) != 0 || stat(mpath, &mst) != 0
            || (rid = cnid_add(vol->v_cdb, &mst, DIRDID_ROOT, was,
                               strlen(was), CNID_INVALID)) == CNID_INVALID
            || (uuid = uuid_strip_dashes(vol->v_uuid)) == NULL
            || asprintf(&sql,
                        "UPDATE %s%s%s SET Name = '%s' WHERE Id = %" PRIu32,
                        quote, uuid, quote, moved, ntohl(rid)) == -1) {
        sql = NULL;
        goto out;
    }

    hook_sql = sql;
    hook_fired = 0;
    hook_ran = 0;
    fi.sql.hook = run_hook_sql;
    fi.sql.prefix = "DELETE FROM";
    got_moved = cnid_lookup(vol->v_cdb, &mst, DIRDID_ROOT, moved,
                            strlen(moved));
    moved_errno = CNID_ERRNO();
    fault_inject_reset();
    moved_write = hook_fired ? hook_ran : -1;
    probe = rid;
    moved_kept = cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf)) != NULL;
    free(sql);
    sql = NULL;

    /* one row meets the name, another the inode of the file at the name */
    if (create_disk_file(npath) != 0 || stat(npath, &nst) != 0
            || (aid = cnid_add(vol->v_cdb, &nst, DIRDID_ROOT, name,
                               strlen(name), CNID_INVALID)) == CNID_INVALID
            || create_disk_file(opath) != 0 || stat(opath, &ost) != 0
            || (bid = cnid_add(vol->v_cdb, &ost, DIRDID_ROOT, other,
                               strlen(other), CNID_INVALID)) == CNID_INVALID
            || asprintf(&sql,
                        "UPDATE %s%s%s SET InodeNo = InodeNo + 1000000 "
                        "WHERE Id = %" PRIu32 " OR Id = %" PRIu32, quote,
                        uuid, quote, ntohl(aid), ntohl(bid)) == -1) {
        sql = NULL;
        goto out;
    }

    hook_sql = sql;
    hook_fired = 0;
    hook_ran = 0;
    fi.sql.hook = run_hook_sql;
    fi.sql.prefix = "DELETE FROM";
    got_two = cnid_lookup(vol->v_cdb, &ost, DIRDID_ROOT, name, strlen(name));
    two_errno = CNID_ERRNO();
    fault_inject_reset();
    two_write = hook_fired ? hook_ran : -1;
    probe = aid;
    both_kept = cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf)) != NULL;
    probe = bid;
    both_kept = both_kept
                && cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf)) != NULL;

    if (moved_write == 1 && got_moved == CNID_INVALID
            && moved_errno == CNID_ERR_NOTFOUND && moved_kept
            && two_write == 1 && got_two == CNID_INVALID
            && two_errno == CNID_ERR_NOTFOUND && both_kept) {
        ret = 0;
    } else {
        fprintf(stderr, "cnid_lookup, its rows changed before its delete: "
                        "the moved file's write %s, its lookup %u errno %#x, "
                        "its row %s, the two rows' write %s, their lookup %u "
                        "errno %#x, the rows %s\n",
                moved_write < 0 ? "never ran" : moved_write ? "ran" : "failed",
                ntohl(got_moved), moved_errno, moved_kept ? "kept" : "gone",
                two_write < 0 ? "never ran" : two_write ? "ran" : "failed",
                ntohl(got_two), two_errno,
                both_kept ? "kept" : "not both kept");
        ret = 1;
    }

out:
    fault_inject_reset();
    free(sql);
    sql = NULL;
    free(uuid);

    if (rid != CNID_INVALID) {
        cnid_delete(vol->v_cdb, rid);
    }

    if (aid != CNID_INVALID) {
        cnid_delete(vol->v_cdb, aid);
    }

    if (bid != CNID_INVALID) {
        cnid_delete(vol->v_cdb, bid);
    }

    unlink(mpath);
    unlink(npath);
    unlink(opath);
    return ret;
}

/*!
 * @brief Conditional row writes keep a row renamed by case alone
 *
 * @param[in] vol  the harness's open volume
 *
 * @returns 0 when both calls with the old name answer 0 and keep the renamed
 *          row, and the delete by the resolved name answers 1, 1 otherwise,
 *          or -1 when the fixture failed
 */
int utest_cnid_row_case_renamed(struct vol *vol)
{
    static char name[] = "cnid_row_Case";
    static char lower[] = "cnid_row_case";
    static char held[] = "cnid_row_held";
    static char accented[] = "cnid_row_caf\xc3\xa9";
    char path[MAXPATHLEN + 1];
    char hpath[MAXPATHLEN + 1];
    char apath[MAXPATHLEN + 1];
    char buf[CNID_MAX_PATH_LEN];
    struct stat st;
    struct stat hst;
    struct stat ast;
    const size_t len = strlen(name);
    const char *now = NULL;
    cnid_t id = CNID_INVALID;
    cnid_t aid = CNID_INVALID;
    cnid_t probe;
    uint64_t dev;
    uint64_t ino;
    bool kept = false;
    int gone = -1;
    int changed = -1;
    int accented_gone = -1;
    int ret = -1;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, name);
    snprintf(hpath, sizeof(hpath), "%s/%s", vol->v_path, held);
    snprintf(apath, sizeof(apath), "%s/%s", vol->v_path, accented);

    /* the row at its new name, another file's stat for the update, and a
     * row whose name is not ASCII */
    if (create_disk_file(path) != 0 || stat(path, &st) != 0
            || (id = cnid_add(vol->v_cdb, &st, DIRDID_ROOT, name, len,
                              CNID_INVALID)) == CNID_INVALID
            || cnid_update(vol->v_cdb, id, &st, DIRDID_ROOT, lower,
                           strlen(lower)) != 0
            || create_disk_file(hpath) != 0 || stat(hpath, &hst) != 0
            || create_disk_file(apath) != 0 || stat(apath, &ast) != 0
            || (aid = cnid_add(vol->v_cdb, &ast, DIRDID_ROOT, accented,
                               strlen(accented), CNID_INVALID))
            == CNID_INVALID) {
        goto out;
    }

    dev = cnid_row_dev(vol->v_cdb, &st);
    ino = (uint64_t)st.st_ino;
    gone = cnid_delete_row(vol->v_cdb, id, DIRDID_ROOT, name, len, dev, ino);
    changed = cnid_update_row(vol->v_cdb, id, DIRDID_ROOT, name, len, dev, ino,
                              &hst, NULL, NULL);
    probe = id;
    now = cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf));
    kept = now != NULL && strcmp(now, lower) == 0;
    probe = aid;

    if ((now = cnid_resolve(vol->v_cdb, &probe, buf, sizeof(buf))) != NULL) {
        accented_gone = cnid_delete_row(vol->v_cdb, aid, DIRDID_ROOT, now,
                                        strlen(now),
                                        cnid_row_dev(vol->v_cdb, &ast),
                                        (uint64_t)ast.st_ino);
    }

    if (gone == 0 && changed == 0 && kept && accented_gone == 1) {
        ret = 0;
    } else {
        fprintf(stderr, "cnid_delete_row and cnid_update_row by the old case: "
                        "%d and %d, the row %s, the non-ASCII row's delete "
                        "%d\n", gone, changed, kept ? "kept" : "changed",
                accented_gone);
        ret = 1;
    }

out:

    if (id != CNID_INVALID && gone != 1) {
        cnid_delete(vol->v_cdb, id);
    }

    if (aid != CNID_INVALID && accented_gone != 1) {
        cnid_delete(vol->v_cdb, aid);
    }

    unlink(path);
    unlink(hpath);
    unlink(apath);
    return ret;
}

/*!
 * @brief A private volume's CNID state is its owner's and owner-only; a
 *        shared open widens it
 *
 * The test runs as root, so cnid_open() takes the owner's ids for the open:
 * the sqlite backend, as the owner, creates the directory 0700 and the
 * database 0600, and gives the same directory the shared modes (01777, 0666)
 * when a shared volume opens it. The owner is nobody, since root can't own a
 * private volume. An owner without an account fails the open before the
 * backend runs.
 *
 * @param[in] vol  the harness's open sqlite volume
 *
 * @returns 0 when both opens leave the owner and modes their rule gives them,
 *          the private one hands the ids back, and the open for an owner
 *          without an account leaves no state
 */
int utest_cnid_sqlite_owner_only(struct vol *vol)
{
    struct vol priv_vol;
    struct _cnid_db *db;
    struct stat st;
    const struct passwd *owner;
    uid_t owner_uid;
    gid_t egid;
    gid_t *groups;
    int ngroups;
    char dbdir[MAXPATHLEN];
    char dbfile[MAXPATHLEN];
    char aux[MAXPATHLEN];
    int result = 0;

    if (vol->v_cdb == NULL || vol->v_dbpath == NULL
            || vol->v_cnidscheme == NULL
            || strcmp(vol->v_cnidscheme, "sqlite") != 0 || getuid() != 0
            || (owner = getpwnam("nobody")) == NULL
            || (groups = cnid_groups(&ngroups)) == NULL) {
        return TEST_SKIP;
    }

    snprintf(dbdir, sizeof(dbdir), "%s.owneronly", vol->v_dbpath);
    snprintf(dbfile, sizeof(dbfile), "%s/%s.sqlite", dbdir, vol->v_localname);
    /* the record is the C library's, which the open looks up again */
    owner_uid = owner->pw_uid;
    priv_vol = *vol;
    priv_vol.v_dbpath = dbdir;
    priv_vol.v_cdb = NULL;
    priv_vol.v_flags |= AFPVOL_PRIVATE;
    priv_vol.v_private_uid = owner_uid;
    egid = getegid();

    if ((db = cnid_open(&priv_vol, vol->v_cnidscheme, 0)) == NULL) {
        result = 1;
    } else {
        cnid_close(db);

        /* the open hands euid 0, the egid and the groups back */
        if (geteuid() != 0 || getegid() != egid
                || !cnid_groups_are(groups, ngroups)) {
            result = 9;
        } else if (stat(dbdir, &st) != 0 || st.st_uid != owner_uid
                   || (st.st_mode & 07777) != 0700) {
            result = 2;
        } else if (stat(dbfile, &st) != 0 || st.st_uid != owner_uid
                   || (st.st_mode & 0777) != 0600) {
            result = 3;
        }
    }

    priv_vol.v_flags &= ~AFPVOL_PRIVATE;

    if (result == 0) {
        if ((db = cnid_open(&priv_vol, vol->v_cnidscheme, 0)) == NULL) {
            result = 4;
        } else {
            cnid_close(db);

            if (stat(dbdir, &st) != 0 || (st.st_mode & 07777) != 01777) {
                result = 5;
            } else if (stat(dbfile, &st) != 0 || (st.st_mode & 0777) != 0666) {
                result = 6;
            }
        }
    }

    unlink(dbfile);
    snprintf(aux, sizeof(aux), "%s-wal", dbfile);
    unlink(aux);
    snprintf(aux, sizeof(aux), "%s-shm", dbfile);
    unlink(aux);
    rmdir(dbdir);

    /* an owner without an account fails the open before the backend creates
     * anything as root */
    if (result == 0) {
        uid_t ghost = owner_uid + 1;

        while (getpwuid(ghost) != NULL) {
            ghost++;
        }

        priv_vol.v_flags |= AFPVOL_PRIVATE;
        priv_vol.v_private_uid = ghost;

        if ((db = cnid_open(&priv_vol, vol->v_cnidscheme, 0)) != NULL) {
            cnid_close(db);
            result = 7;
        } else if (stat(dbdir, &st) == 0) {
            result = 8;
        }

        rmdir(dbdir);
    }

    free(groups);
    return result;
}

/*!
 * @brief Give a fresh directory, and a db directory made inside it, to an
 *        account
 *
 * Both are reached through descriptors, so no path is resolved twice.
 *
 * @param[in] root  the fresh directory
 * @param[in] uid   the account
 * @param[in] gid   the account's group
 *
 * @returns true when both directories are the account's
 */
static bool desktop_scratch_owned(const char *root, uid_t uid, gid_t gid)
{
    int rootfd = open(root, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    int dbfd = -1;
    bool owned = false;

    if (rootfd >= 0 && mkdirat(rootfd, "db", 0700) == 0
            && (dbfd = openat(rootfd, "db", O_RDONLY | O_DIRECTORY
                              | O_NOFOLLOW | O_CLOEXEC)) >= 0) {
        owned = fchown(dbfd, uid, gid) == 0 && fchown(rootfd, uid, gid) == 0;
    }

    if (dbfd >= 0) {
        close(dbfd);
    }

    if (rootfd >= 0) {
        close(rootfd);
    }

    return owned;
}

/*!
 * @brief FPOpenDT on a private volume creates the desktop folder as the
 *        owner and hands root's ids back
 *
 * The test runs as root, as an admin-group session does. For the call, the
 * harness's volume is private to nobody, with its root and CNID directory in
 * a directory nobody owns beside the harness's CNID directory.
 *
 * @param[in] obj  the harness's session
 * @param[in] vol  the harness's open volume
 *
 * @returns 0, 1 when FPOpenDT fails, 2 when euid 0, the egid or the groups do
 *          not come back, 3 when the folder is not the owner's
 */
int utest_cnid_desktop_folder_as_owner(AFPObj *obj, struct vol *vol)
{
    char root[MAXPATHLEN];
    char dbdir[MAXPATHLEN];
    char dtdir[MAXPATHLEN];
    /* the request: the command, a pad byte and the volume id */
    char ibuf[4] = { 0 };
    char rbuf[sizeof(uint16_t)];
    size_t rbuflen = 0;
    char *const path = vol->v_path;
    char *const dbpath = vol->v_dbpath;
    const int flags = vol->v_flags;
    const uid_t private_uid = vol->v_private_uid;
    const gid_t egid = getegid();
    const struct passwd *owner;
    uid_t owner_uid;
    gid_t owner_gid;
    gid_t *groups;
    int ngroups;
    struct stat st;
    int result = 0;

    if (getuid() != 0 || (owner = getpwnam("nobody")) == NULL) {
        return TEST_SKIP;
    }

    /* the record is the C library's, which the owner switch looks up again */
    owner_uid = owner->pw_uid;
    owner_gid = owner->pw_gid;
    snprintf(root, sizeof(root), "%s.desktop-XXXXXX", vol->v_dbpath);

    if (mkdtemp(root) == NULL) {
        return TEST_SKIP;
    }

    snprintf(dbdir, sizeof(dbdir), "%s/db", root);
    snprintf(dtdir, sizeof(dtdir), "%s/.AppleDesktop", dbdir);

    if (!desktop_scratch_owned(root, owner_uid, owner_gid)
            || (groups = cnid_groups(&ngroups)) == NULL) {
        rmdir(dbdir);
        rmdir(root);
        return TEST_SKIP;
    }

    vol->v_path = root;
    vol->v_dbpath = dbdir;
    vol->v_flags |= AFPVOL_PRIVATE;
    vol->v_private_uid = owner_uid;
    memcpy(ibuf + 2, &vol->v_vid, sizeof(vol->v_vid));

    if (afp_opendt(obj, ibuf, sizeof(ibuf), rbuf, &rbuflen) != AFP_OK) {
        result = 1;
    } else if (geteuid() != 0 || getegid() != egid
               || !cnid_groups_are(groups, ngroups)) {
        result = 2;
    } else if (stat(dtdir, &st) != 0 || st.st_uid != owner_uid) {
        result = 3;
    }

    /* a lost hand-back would run the rest of the suite as the owner */
    if ((geteuid() != 0 && seteuid(0) != 0) || setegid(egid) != 0
            || setgroups(ngroups, groups) != 0) {
        result = 2;
    }

    vol->v_path = path;
    vol->v_dbpath = dbpath;
    vol->v_flags = flags;
    vol->v_private_uid = private_uid;
    free(groups);
    rmdir(dtdir);
    rmdir(dbdir);
    rmdir(root);
    return result;
}

static int fake_adds;

/*!
 * @brief Backend cnid_add that counts its calls and answers one CNID
 */
static cnid_t fake_cnid_add(struct _cnid_db *cdb _U_,
                            const struct stat *st _U_, cnid_t did _U_,
                            const char *name _U_, size_t len _U_,
                            cnid_t hint _U_)
{
    fake_adds++;
    return htonl(1000);
}

/*!
 * @brief Backend cnid_dirlist whose rows are one above 32 bits, one
 *        without its name and a sound one
 */
static int fake_cnid_dirlist(struct _cnid_db *cdb _U_, cnid_t did _U_,
                             size_t max _U_, cnid_dirlist_fn fn, void *ctx)
{
    return fn(ctx, 0x100000064ULL, "big", 3, 1, 10) != 0
           || fn(ctx, 101, NULL, 4, 1, 11) != 0
           || fn(ctx, 102, "sound", 5, 1, 12) != 0 ? -1 : 0;
}

/*!
 * @brief cnid_close() of the stand-in backend, which holds nothing
 */
static void fake_cnid_close(struct _cnid_db *cdb _U_)
{
    /* the backend opened nothing */
}

/*!
 * @brief A folder table leaves the rows a lookup refuses to the backend
 *        and answers a sound one inside its listing's batch alone
 *
 * @returns 0, 1 when the row above 32 bits was answered, 2 when the sound
 *          row was not, 3 when the table answered outside the batch
 */
int utest_cnid_dirtab_rows(void)
{
    struct _cnid_db cdb = { 0 };
    struct stat st = { .st_dev = 1 };
    char miss[] = "miss";
    char big[] = "big";
    char sound[] = "sound";
    const cnid_t did = htonl(100);
    int ret = 0;
    cdb.cnid_add = fake_cnid_add;
    cdb.cnid_dirlist = fake_cnid_dirlist;
    cdb.cnid_close = fake_cnid_close;
    cnid_dir_arm(&cdb, did);
    cnid_dir_serve(&cdb, did);

    /* the misses before the load, the last of them loading the rows */
    for (int i = 0; i <= CNID_DIRTAB_MISSES; i++) {
        cnid_add(&cdb, &st, did, miss, 4, CNID_INVALID);
    }

    fake_adds = 0;
    st.st_ino = 10;
    cnid_add(&cdb, &st, did, big, 3, CNID_INVALID);

    if (fake_adds != 1) {
        ret = 1;
    }

    st.st_ino = 12;

    if (ret == 0
            && cnid_add(&cdb, &st, did, sound, 5, CNID_INVALID) != htonl(102)) {
        ret = 2;
    }

    cnid_dir_serve(&cdb, CNID_INVALID);
    fake_adds = 0;
    cnid_add(&cdb, &st, did, sound, 5, CNID_INVALID);

    if (ret == 0 && fake_adds != 1) {
        ret = 3;
    }

    cnid_close(&cdb);
    return ret;
}

/* names whose hashes share an index slot; row i holds CNID and inode
 * 200 + i */
static char collide_names[CNID_DIRTAB_PROBES + 1][16];
static int collide_rows;

/*!
 * @brief The FNV-1a hash a folder table keeps of a name
 */
static uint64_t name_hash(const char *name, size_t len)
{
    uint64_t h = 14695981039346656037ULL;

    for (size_t i = 0; i < len; i++) {
        h = (h ^ (unsigned char)name[i]) * 1099511628211ULL;
    }

    return h;
}

/*!
 * @brief Backend cnid_dirlist whose collide_rows rows' names share an
 *        index slot
 */
static int collide_dirlist(struct _cnid_db *cdb _U_, cnid_t did _U_,
                           size_t max _U_, cnid_dirlist_fn fn, void *ctx)
{
    for (int i = 0; i < collide_rows; i++) {
        if (fn(ctx, 200 + i, collide_names[i], strlen(collide_names[i]), 1,
                200 + i) != 0) {
            return -1;
        }
    }

    return 0;
}

/*!
 * @brief What cnid_add() answers for one of rows colliding rows once their
 *        table loads, with fake_adds counting the backend's lookups
 */
static cnid_t collide_answer(int rows, int row)
{
    struct _cnid_db cdb = { 0 };
    struct stat st = { .st_dev = 1, .st_ino = 200 + row };
    char miss[] = "miss";
    const char *name = collide_names[row];
    const cnid_t did = htonl(100);
    cnid_t id;
    cdb.cnid_add = fake_cnid_add;
    cdb.cnid_dirlist = collide_dirlist;
    cdb.cnid_close = fake_cnid_close;
    collide_rows = rows;
    cnid_dir_arm(&cdb, did);
    cnid_dir_serve(&cdb, did);

    for (int i = 0; i <= CNID_DIRTAB_MISSES; i++) {
        cnid_add(&cdb, &st, did, miss, 4, CNID_INVALID);
    }

    fake_adds = 0;
    id = cnid_add(&cdb, &st, did, name, strlen(name), CNID_INVALID);
    cnid_close(&cdb);
    return id;
}

/*!
 * @brief A folder table answers a row CNID_DIRTAB_PROBES - 1 slots past
 *        its own, and turns off where a row would sit farther
 *
 * @returns 0, 1 when the table missed its farthest row, 2 when it loaded
 *          rows past the limit
 */
int utest_cnid_dirtab_collisions(void)
{
    char name[16];
    uint64_t slot = name_hash("c0", 2) & 0x3ff;
    int n = 0;

    /* names whose hashes share their low 10 bits share a slot in any
     * table of up to 512 rows */
    for (unsigned int i = 0; n <= CNID_DIRTAB_PROBES; i++) {
        int len = snprintf(name, sizeof(name), "c%u", i);

        if ((name_hash(name, (size_t)len) & 0x3ff) == slot) {
            memcpy(collide_names[n++], name, (size_t)len + 1);
        }
    }

    /* the last of CNID_DIRTAB_PROBES rows sits CNID_DIRTAB_PROBES - 1 slots
     * past its hash slot */
    if (collide_answer(CNID_DIRTAB_PROBES, CNID_DIRTAB_PROBES - 1)
            != htonl(200 + CNID_DIRTAB_PROBES - 1) || fake_adds != 0) {
        return 1;
    }

    /* one more turns the table off, so even the first row asks the
     * backend */
    if (collide_answer(CNID_DIRTAB_PROBES + 1, 0) != htonl(1000)
            || fake_adds != 1) {
        return 2;
    }

    return 0;
}
