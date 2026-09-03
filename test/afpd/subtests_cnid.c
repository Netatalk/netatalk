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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef CNID_BACKEND_SQLITE
#include <sqlite3.h>
#endif

#include <atalk/cnid.h>
#include <atalk/cnid_bdb_private.h>
#include <atalk/globals.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "subtests_cnid.h"
#include "test.h"
#include "volume.h"

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

#ifdef CNID_BACKEND_SQLITE
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
#endif /* CNID_BACKEND_SQLITE */

/*!
 * @brief Every CNID_INVALID from the wrapper carries its own errno
 *
 * Callers branch on the CNID_ERR_* values to choose between a permanent reply
 * and a retryable one, and get_id() (etc/afpd/file.c) ends the session on
 * CNID_ERR_DB, so a rejection sets errno itself rather than leaving an earlier
 * operation's value in place. The poison value here, EPERM, is what a failed
 * chown or seteuid leaves behind and is also CNID_DBD_RES_NOTFOUND's value.
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
#ifndef CNID_BACKEND_SQLITE
    (void) vol;
    return TEST_SKIP;
#else
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
#endif /* CNID_BACKEND_SQLITE */
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
#ifndef CNID_BACKEND_SQLITE
    (void) vol;
    return TEST_SKIP;
#else
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
#endif /* CNID_BACKEND_SQLITE */
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
 * The dbd wire constant CNID_DBD_RES_NOTFOUND equals EPERM, and callers that
 * switch on raw errno map EPERM to AFPERR_ACCESS (movecwd() in
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

static char fake_resolve_dotdot[] = "..";

/*!
 * @brief Backend cnid_resolve that answers "..", the corrupt name to reject
 */
static char *fake_cnid_resolve(struct _cnid_db *cdb _U_, cnid_t *id,
                               void *buffer _U_, size_t len _U_)
{
    *id = htonl(2);
    return fake_resolve_dotdot;
}

/*!
 * @brief The wrapper's ".." rejection fails like a backend failure
 *
 * cnid_resolve() (libatalk/cnid/cnid.c) refuses a ".." name from the backend
 * as a whole failure: NULL return, *id overwritten with CNID_INVALID in place
 * of the parent DID the backend wrote, and errno set to CNID_ERR_CORRUPT.
 */
int utest_cnid_resolve_dotdot_rejected(void)
{
    struct _cnid_db cdb = { 0 };
    char buf[64];
    cnid_t id = htonl(1234);
    cdb.cnid_resolve = fake_cnid_resolve;
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
#ifndef CNID_BACKEND_SQLITE
    (void) vol;
    return TEST_SKIP;
#else
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
#endif /* CNID_BACKEND_SQLITE */
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
#ifndef CNID_BACKEND_SQLITE
    (void) vol;
    return TEST_SKIP;
#else
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
#endif /* CNID_BACKEND_SQLITE */
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
#ifndef CNID_BACKEND_SQLITE
    (void) vol;
    return TEST_SKIP;
#else
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
#endif /* CNID_BACKEND_SQLITE */
}
