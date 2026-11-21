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
 * @brief CNID layer tests: opens, rows, conditional writes and folder tables
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
#include <atalk/globals.h>
#include <atalk/logger.h>
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

static size_t fake_find_namelen;

/*!
 * @brief Backend cnid_find that records the name length handed to it
 */
static int fake_cnid_find(struct _cnid_db *cdb _U_, const char *name _U_,
                          size_t namelen, cnid_t scope_did _U_,
                          void *buffer _U_, size_t buflen _U_,
                          bool *more_available _U_)
{
    fake_find_namelen = namelen;
    return 0;
}

/*!
 * @brief cnid_find() accepts a name of MAXPATHLEN bytes and rejects one more
 *
 * The wrapper's bound is the path-buffer limit; a longer name, or an empty
 * one, is a parameter error that never reaches the backend.
 */
int utest_cnid_find_name_bound(void)
{
    struct _cnid_db cdb = { 0 };
    char name[MAXPATHLEN + 1];
    cnid_t results[CNID_FIND_MIN_RESULTS];
    bool more = false;
    cdb.cnid_find = fake_cnid_find;
    memset(name, 'a', MAXPATHLEN);
    name[MAXPATHLEN] = '\0';
    fake_find_namelen = 0;
    errno = 0;

    if (cnid_find(&cdb, name, MAXPATHLEN, results, sizeof(results), &more) != 0
            || fake_find_namelen != MAXPATHLEN) {
        return 1;
    }

    fake_find_namelen = 0;
    errno = 0;

    if (cnid_find(&cdb, name, MAXPATHLEN + 1, results, sizeof(results),
                  &more) != -1 || fake_find_namelen != 0) {
        return 2;
    }

    if (CNID_ERRNO() != CNID_ERR_PARAM) {
        return 3;
    }

    errno = 0;

    if (cnid_find(&cdb, name, 0, results, sizeof(results), &more) != -1
            || fake_find_namelen != 0) {
        return 4;
    }

    if (CNID_ERRNO() != CNID_ERR_PARAM) {
        return 5;
    }

    return 0;
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
