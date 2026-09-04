/*
 * CNID database implementation using SQLite embedded database engine.
 * Built on proof-of-concept code written by Christopher Kobayashi in 2022
 * for the netatalk-classic project, in turn based on the cnid_mysql.c
 * implementation by Ralph Boehme.
 *
 * Copyright (C) 2013 Ralph Boehme
 * Copyright (C) 2024-2026 Daniel Markstedt
 * All Rights Reserved.  See COPYING.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <time.h>

#include <bstrlib.h>
#include <sqlite3.h>

#include <atalk/adouble.h>
#include <atalk/cnid_bdb_private.h>
#include <atalk/cnid_sqlite_private.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/volume.h>

/* Total time SQLite waits for a contended lock before returning SQLITE_BUSY.
 * The busy handler polls internally and returns as soon as the lock frees. */
#define CNID_SQLITE_BUSY_TIMEOUT    3000

/* Bound on the lookup-then-insert cycle, which does not converge when a row
 * satisfies the insert's unique keys but no lookup index reaches it */
#define CNID_SQLITE_ADD_ATTEMPTS    16

/* Attempts at the table-emptiness probe during open. Each step already waits
 * out CNID_SQLITE_BUSY_TIMEOUT, so this only covers sustained contention. */
#define CNID_SQLITE_EMPTY_PROBE_TRIES   3

/* Ceiling for a CNID hint. A hint binds as an explicit rowid, which raises the
 * AUTOINCREMENT high-water mark, so the headroom below the 32-bit ceiling
 * keeps a hint out of corrupt AppleDouble/EA metadata from reaching it. */
#define CNID_SQLITE_MAX_HINT    (UINT32_MAX - 65536)

static void cnid_sqlite_set_errno(int sqlite_return);

/*!
 * @brief Prepare one per-volume statement, replacing any previous handle
 *
 * @param[in,out] db      backend private data
 * @param[out] stmt       statement handle to finalize and re-prepare
 * @param[in] tag         statement name, for the debug log only
 * @param[in] sql_fmt     SQL with one %s for the volume's table name, or none
 */
static int init_prepared_stmt_one(CNID_sqlite_private *db,
                                  sqlite3_stmt **stmt,
                                  const char *tag, const char *sql_fmt)
{
    EC_INIT;
    char *sql = NULL;

    if (*stmt) {
        sqlite3_finalize(*stmt);
        *stmt = NULL;
    }

    EC_NEG1(asprintf(&sql, sql_fmt, db->cnid_sqlite_voluuid_str));
    LOG(log_maxdebug, logtype_cnid, "init_prepared_stmt_%s: SQL: %s", tag, sql);
    int prepare_return = sqlite3_prepare_v2(db->cnid_sqlite_con, sql,
                                            (int) strlen(sql), stmt, NULL);

    /* Preparing loads the schema, so a corrupt or read-only database
     * surfaces here, not at step time; classify it like any other failure */
    if (prepare_return != SQLITE_OK) {
        LOG(log_error, logtype_cnid, "init_prepared_stmt_%s: prepare failed: %s",
            tag, sqlite3_errmsg(db->cnid_sqlite_con));
        cnid_sqlite_set_errno(prepare_return);
        EC_FAIL;
    }

EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

/*!
 * @brief (Re-)prepare every per-volume statement, at open and after a wipe
 *
 * A failure leaves the handles past that point NULL; cnid_sqlite_stmt_ready()
 * rejects those.
 */
static int init_prepared_stmt(CNID_sqlite_private * db)
{
    EC_INIT;
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_lookup_stmt, "lookup",
                                   "SELECT Id,Did,Name,DevNo,InodeNo FROM \"%s\" "
                                   "WHERE (Name = ? AND Did = ?) OR (DevNo = ? AND InodeNo = ?)"));
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_add_stmt, "add",
                                   "INSERT INTO \"%s\" (Name, Did, DevNo, InodeNo) VALUES(?, ?, ?, ?)"));
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_put_stmt, "put",
                                   "INSERT INTO \"%s\" (Id, Name, Did, DevNo, InodeNo) VALUES(?, ?, ?, ?, ?)"));
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_get_stmt, "get",
                                   "SELECT Id FROM \"%s\" WHERE Name = ? AND Did = ?"));
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_resolve_stmt, "resolve",
                                   "SELECT Did, Name FROM \"%s\" WHERE Id = ?"));
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_delete_stmt, "delete",
                                   "DELETE FROM \"%s\" WHERE Id = ?"));
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_getstamp_stmt, "getstamp",
                                   "SELECT Stamp FROM volumes WHERE VolPath = ?"));
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_find_stmt, "find",
                                   "SELECT Id FROM \"%s\" WHERE Name LIKE ? ORDER BY Id LIMIT ?"));
    /* Subtree-scoped variant: the recursive CTE enumerates the scope
     * directory's transitive contents via the (Did, Name) unique index, and
     * the CROSS JOIN pins the scope set as the outer loop so cost is
     * proportional to the subtree, not the volume. Recursive CTEs are a hard
     * requirement (SQLite >= 3.8.3). The table name appears twice, hence the
     * positional %1$s. */
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_find_scoped_stmt, "find_scoped",
                                   "WITH RECURSIVE scope(Id) AS ("
                                   "  VALUES(?)"
                                   "  UNION"
                                   "  SELECT t.Id FROM \"%1$s\" t JOIN scope s ON t.Did = s.Id"
                                   ") "
                                   "SELECT t.Id FROM scope s CROSS JOIN \"%1$s\" t "
                                   "ON t.Did = s.Id "
                                   "WHERE t.Name LIKE ? ORDER BY t.Id LIMIT ?"));
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_update_stmt, "update",
                                   "UPDATE \"%s\" SET Name = ?, Did = ?, DevNo = ?, InodeNo = ? WHERE Id = ?"));
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_del_didname_stmt, "del_didname",
                                   "DELETE FROM \"%s\" WHERE Did = ? AND Name = ?"));
    EC_ZERO(init_prepared_stmt_one(db, &db->cnid_del_devino_stmt, "del_devino",
                                   "DELETE FROM \"%s\" WHERE DevNo = ? AND InodeNo = ?"));
    EC_EXIT;
EC_CLEANUP:
    EC_EXIT;
}

/*!
 * @brief Finalize every prepared statement and clear the handles
 */
static void close_prepared_stmt(CNID_sqlite_private * db)
{
    sqlite3_finalize(db->cnid_lookup_stmt);
    sqlite3_finalize(db->cnid_add_stmt);
    sqlite3_finalize(db->cnid_put_stmt);
    sqlite3_finalize(db->cnid_get_stmt);
    sqlite3_finalize(db->cnid_resolve_stmt);
    sqlite3_finalize(db->cnid_delete_stmt);
    sqlite3_finalize(db->cnid_getstamp_stmt);
    sqlite3_finalize(db->cnid_find_stmt);
    sqlite3_finalize(db->cnid_find_scoped_stmt);
    sqlite3_finalize(db->cnid_update_stmt);
    sqlite3_finalize(db->cnid_del_didname_stmt);
    sqlite3_finalize(db->cnid_del_devino_stmt);
    db->cnid_lookup_stmt = NULL;
    db->cnid_add_stmt = NULL;
    db->cnid_put_stmt = NULL;
    db->cnid_get_stmt = NULL;
    db->cnid_resolve_stmt = NULL;
    db->cnid_delete_stmt = NULL;
    db->cnid_getstamp_stmt = NULL;
    db->cnid_find_stmt = NULL;
    db->cnid_find_scoped_stmt = NULL;
    db->cnid_update_stmt = NULL;
    db->cnid_del_didname_stmt = NULL;
    db->cnid_del_devino_stmt = NULL;
}

/*!
 * @brief Whether a CNID hint from AppleDouble/EA metadata is safe to bind
 *
 * Rejects the reserved range below CNID_START and anything above
 * CNID_SQLITE_MAX_HINT, which as an explicit rowid would raise the
 * AUTOINCREMENT high-water mark towards the depletion reset.
 */
static bool cnid_sqlite_hint_usable(cnid_t hint)
{
    uint32_t id = ntohl(hint);
    return id >= CNID_START && id <= CNID_SQLITE_MAX_HINT;
}

/*!
 * @brief Whether a rowid read from the database still fits a CNID
 *
 * The Id column is a 64-bit sqlite integer while a CNID is 32 bits. Narrowing
 * one that does not fit would produce the id of an unrelated live row, which
 * the caller would then hand to a client, or delete.
 */
static bool cnid_sqlite_rowid_ok(uint64_t rowid)
{
    return rowid != 0 && rowid <= (uint64_t) UINT32_MAX;
}

/*!
 * @brief Whether a volume UUID is safe to interpolate as a table name
 *
 * A table name cannot be a bound parameter, so it is built into the SQL text.
 * uuid_strip_dashes() yields exactly 32 hex digits; the UUIDs read back out of
 * the volumes table are only as trustworthy as the world-writable database
 * file, so anything else is refused rather than quoted and hoped for.
 */
static bool cnid_sqlite_uuid_usable(const char *uuid)
{
    size_t i;

    if (uuid == NULL) {
        return false;
    }

    for (i = 0; uuid[i] != '\0'; i++) {
        if (!isxdigit((unsigned char) uuid[i])) {
            return false;
        }
    }

    return i == 32;
}

/*!
 * @brief Map a sqlite3 result code onto the CNID error contract in errno
 *
 * CNID_ERR_BUSY covers what clears on its own: contention, disk full, I/O
 * error, out of memory, and SQLITE_PROTOCOL, which is WAL's bounded locking
 * handshake giving up. SQLITE_LOCKED is not in that set — it reports a
 * same-connection or shared-cache conflict the busy handler never waits on, so
 * the state survives a retry.
 *
 * CNID_ERR_DB, which get_id() in etc/afpd/file.c answers by ending the session,
 * means the backend itself is unreachable — for the dbd backend, cnid_metad
 * being gone. A local database file has no such state: the connection outlives
 * whatever one statement returns, so an unrecognised code fails the single
 * operation as CNID_ERR_CORRUPT.
 */
static void cnid_sqlite_set_errno(int sqlite_return)
{
    /* The READONLY family mixes a permanently read-only database with recovery
     * states that clear on their own, so it is read before the 0xff mask */
    switch (sqlite_return) {
    case SQLITE_READONLY_RECOVERY:
    case SQLITE_READONLY_ROLLBACK:
    case SQLITE_READONLY_DBMOVED:
        errno = CNID_ERR_BUSY;
        return;

    case SQLITE_READONLY:
        /* afp_deleteid() in etc/afpd/file.c maps EROFS to AFPERR_VLOCK */
        errno = EROFS;
        return;

    default:
        /* Not a READONLY code: classified by primary result below */
        break;
    }

    switch (sqlite_return & 0xff) {
    case SQLITE_BUSY:
    case SQLITE_PROTOCOL:
    case SQLITE_FULL:
    case SQLITE_IOERR:
    case SQLITE_NOMEM:
        errno = CNID_ERR_BUSY;
        break;

    default:
        errno = CNID_ERR_CORRUPT;
        break;
    }
}

/*!
 * @brief Run one SQL statement, classifying a failure into errno
 *
 * @returns 0 on success, -1 with errno set by cnid_sqlite_set_errno()
 */
static int cnid_sqlite_execute(sqlite3 *con, const char *sql)
{
    int rv;
    LOG(log_maxdebug, logtype_cnid, "SQL: %s", sql);
    rv = sqlite3_exec(con, sql, NULL, NULL, NULL);

    if (rv != SQLITE_OK) {
        LOG(log_error, logtype_cnid,
            "sqlite query \"%s\", error: %s", sql, sqlite3_errmsg(con));
        cnid_sqlite_set_errno(rv);
        return -1;
    }

    return 0;
}

/*!
 * @brief Delete one row identified by a UUID, through a bound parameter
 *
 * The UUID is data here, not an identifier, so it never enters the SQL text:
 * the stale entries this removes are read back out of the world-writable
 * database file, where a value chosen to break out of a quoted string would
 * otherwise run as SQL under the become_root() the cleanup holds.
 *
 * @returns 0 on success, -1 with errno set by cnid_sqlite_set_errno()
 */
static int cnid_sqlite_delete_by_uuid(sqlite3 *con, const char *sql,
                                      const char *uuid)
{
    sqlite3_stmt *stmt = NULL;
    int rv;
    LOG(log_maxdebug, logtype_cnid, "SQL: %s", sql);

    if ((rv = sqlite3_prepare_v2(con, sql, -1, &stmt, NULL)) != SQLITE_OK) {
        LOG(log_error, logtype_cnid, "sqlite prepare \"%s\", error: %s", sql,
            sqlite3_errmsg(con));
        cnid_sqlite_set_errno(rv);
        return -1;
    }

    if ((rv = sqlite3_bind_text(stmt, 1, uuid, -1, SQLITE_STATIC)) == SQLITE_OK) {
        rv = sqlite3_step(stmt);
    }

    sqlite3_finalize(stmt);

    if (rv != SQLITE_DONE) {
        LOG(log_error, logtype_cnid, "sqlite query \"%s\", error: %s", sql,
            sqlite3_errmsg(con));
        cnid_sqlite_set_errno(rv);
        return -1;
    }

    return 0;
}

/*!
 * @brief Remove a stale volume's row from the volumes table
 */
static int cnid_sqlite_delete_volumes_row(sqlite3 *con, const char *uuid)
{
    return cnid_sqlite_delete_by_uuid(con,
                                      "DELETE FROM volumes WHERE VolUUID = ?", uuid);
}

/*!
 * @brief Remove a stale volume's AUTOINCREMENT high-water mark
 */
static int cnid_sqlite_delete_sequence_row(sqlite3 *con, const char *uuid)
{
    return cnid_sqlite_delete_by_uuid(con,
                                      "DELETE FROM sqlite_sequence WHERE name = ?", uuid);
}

/*!
 * @brief Reset a prepared statement without disturbing the classification
 *
 * sqlite3_reset() enters the VFS and can leave errno set by a speculative
 * syscall. It runs in every cleanup path after errno has been classified, and
 * that classification is the caller's only signal for what failed.
 */
static void cnid_sqlite_stmt_reset(sqlite3_stmt *stmt)
{
    int saved_errno = errno;
    sqlite3_reset(stmt);
    errno = saved_errno;
}

/*!
 * @brief Whether a prepared statement is available to bind and step
 *
 * The handles are NULL when cnid_sqlite_wipe()'s re-preparation stopped partway.
 * sqlite3_bind_*() tolerates a NULL statement; sqlite3_step() on one is
 * undefined, so every entry point checks the handles it uses.
 */
static bool cnid_sqlite_stmt_ready(sqlite3_stmt *stmt, const char *caller)
{
    if (stmt == NULL) {
        LOG(log_error, logtype_cnid,
            "%s: prepared statement is unavailable; the CNID database for this "
            "volume has to be reopened", caller);
        errno = CNID_ERR_CLOSE;
        return false;
    }

    return true;
}

/*!
 * @brief Open a transaction and mark it owned by the calling function
 *
 * An exec'd COMMIT can fail and error-jump to cleanup, leaving the connection
 * inside a transaction. Ownership is tracked so a cleanup handler can only
 * abandon a transaction its own function started, never a caller's.
 */
static int cnid_sqlite_begin(sqlite3 *con, int *owned)
{
    if (cnid_sqlite_execute(con, "BEGIN") < 0) {
        return -1;
    }

    *owned = 1;
    return 0;
}

/*!
 * @brief Commit the owned transaction, releasing ownership only on success
 */
static int cnid_sqlite_commit(sqlite3 *con, int *owned)
{
    /* Ownership must survive a failed COMMIT: SQLITE_BUSY leaves the
     * transaction open, and the cleanup rollback is the only thing that can
     * close it. Releasing ownership first would strand the write lock for the
     * life of the connection. */
    if (cnid_sqlite_execute(con, "COMMIT") < 0) {
        return -1;
    }

    *owned = 0;
    return 0;
}

/*!
 * @brief Abandon an owned transaction without disturbing the classification
 *
 * Runs from cleanup paths after errno has been classified; the ROLLBACK
 * enters the VFS and could leave errno set by a speculative syscall.
 */
static void cnid_sqlite_rollback(sqlite3 *con, int *owned)
{
    if (!*owned) {
        return;
    }

    *owned = 0;

    if (con && !sqlite3_get_autocommit(con)) {
        int saved_errno = errno;
        sqlite3_exec(con, "ROLLBACK", NULL, NULL, NULL);
        errno = saved_errno;
    }
}

int cnid_sqlite_delete(struct _cnid_db *cdb, const cnid_t id)
{
    EC_INIT;
    CNID_sqlite_private *db = NULL;
    int sqlite_return;

    if (!cdb || !(db = cdb->cnid_db_private) || !id) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_delete: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_delete(id: %" PRIu32 "): BEGIN", ntohl(id));

    if (!cnid_sqlite_stmt_ready(db->cnid_delete_stmt, "cnid_sqlite_delete")) {
        EC_FAIL;
    }

    sqlite3_reset(db->cnid_delete_stmt);
    sqlite3_clear_bindings(db->cnid_delete_stmt);
    sqlite3_bind_int64(db->cnid_delete_stmt, 1, ntohl(id));
    sqlite_return = sqlite3_step(db->cnid_delete_stmt);

    if (sqlite_return != SQLITE_DONE) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_delete: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        cnid_sqlite_set_errno(sqlite_return);
        EC_FAIL;
    }

EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_delete(id: %" PRIu32 "): END", ntohl(id));

    if (db) {
        cnid_sqlite_stmt_reset(db->cnid_delete_stmt);
    }

    EC_EXIT;
}

void cnid_sqlite_close(struct _cnid_db *cdb)
{
    CNID_sqlite_private *db;

    if (!cdb) {
        LOG(log_error, logtype_cnid,
            "cnid_close called with NULL argument !");
        return;
    }

    db = cdb->cnid_db_private;

    if (db != NULL) {
        LOG(log_debug, logtype_cnid,
            "closing database connection for volume '%s'",
            cdb->cnid_db_vol->v_localname);
        free(db->cnid_sqlite_voluuid_str);
        close_prepared_stmt(db);

        if (db->cnid_sqlite_con) {
            /* Fold the WAL back into the database so the next session starts
             * with a small one. TRUNCATE invokes the busy handler, which with
             * readers in other sessions waits the whole budget and truncates
             * nothing, so the timeout is dropped first. A skipped checkpoint is
             * retried at the next close. */
            sqlite3_busy_timeout(db->cnid_sqlite_con, 0);
            sqlite3_wal_checkpoint_v2(db->cnid_sqlite_con, NULL,
                                      SQLITE_CHECKPOINT_TRUNCATE, NULL, NULL);
            sqlite3_close(db->cnid_sqlite_con);
        }

        free(db);
    }

    free(cdb);
    return;
}

int cnid_sqlite_update(struct _cnid_db *cdb,
                       cnid_t id,
                       const struct stat *st,
                       cnid_t did, const char *name, size_t len)
{
    EC_INIT;
    CNID_sqlite_private *db = NULL;
    uint64_t dev = 0;
    uint64_t ino;
    int sqlite_return;
    int owned = 0;

    if (!cdb || !(db = cdb->cnid_db_private) || !id || !st || !name) {
        if (!cdb) {
            LOG(log_error, logtype_cnid,
                "cnid_update: Parameter error: cdb is NULL");
        } else if (!db) {
            LOG(log_error, logtype_cnid,
                "cnid_update: Parameter error: db is NULL");
        } else if (!id) {
            LOG(log_error, logtype_cnid, "cnid_update: Parameter error: id is NULL");
        } else if (!st) {
            LOG(log_error, logtype_cnid,
                "cnid_update: Parameter error: st is NULL");
        } else {
            LOG(log_error, logtype_cnid,
                "cnid_update: Parameter error: name is NULL");
        }

        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_update(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): BEGIN",
        ntohl(id), ntohl(did), name);

    if (!cnid_sqlite_stmt_ready(db->cnid_update_stmt, "cnid_sqlite_update")
            || !cnid_sqlite_stmt_ready(db->cnid_put_stmt, "cnid_sqlite_update")
            || !cnid_sqlite_stmt_ready(db->cnid_del_didname_stmt,
                                       "cnid_sqlite_update")
            || !cnid_sqlite_stmt_ready(db->cnid_del_devino_stmt,
                                       "cnid_sqlite_update")) {
        EC_FAIL;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid,
            "cnid_update: Path name is too long");
        errno = CNID_ERR_PATH;
        EC_FAIL;
    }

    if (!(cdb->cnid_db_flags & CNID_FLAG_NODEV)) {
        dev = st->st_dev;
    }

    ino = st->st_ino;

    for (int attempt = 0; attempt < 2; attempt++) {
        if (attempt > 0) {
            /* Clearing the colliding rows and re-applying the update has to
             * be atomic: a failure in between would leave another file
             * without its CNID row */
            EC_NEG1(cnid_sqlite_begin(db->cnid_sqlite_con, &owned));
            sqlite3_reset(db->cnid_del_didname_stmt);
            sqlite3_clear_bindings(db->cnid_del_didname_stmt);
            sqlite3_bind_int64(db->cnid_del_didname_stmt, 1, ntohl(did));
            sqlite3_bind_text(db->cnid_del_didname_stmt, 2, name, (int)len,
                              SQLITE_STATIC);
            sqlite_return = sqlite3_step(db->cnid_del_didname_stmt);

            if (sqlite_return != SQLITE_DONE) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_update: clearing (Did, Name) failed: %s",
                    sqlite3_errmsg(db->cnid_sqlite_con));
                sqlite3_reset(db->cnid_del_didname_stmt);
                cnid_sqlite_set_errno(sqlite_return);
                EC_FAIL;
            }

            sqlite3_reset(db->cnid_del_didname_stmt);
            sqlite3_reset(db->cnid_del_devino_stmt);
            sqlite3_clear_bindings(db->cnid_del_devino_stmt);
            sqlite3_bind_int64(db->cnid_del_devino_stmt, 1, dev);
            sqlite3_bind_int64(db->cnid_del_devino_stmt, 2, ino);
            sqlite_return = sqlite3_step(db->cnid_del_devino_stmt);

            if (sqlite_return != SQLITE_DONE) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_update: clearing (DevNo, InodeNo) failed: %s",
                    sqlite3_errmsg(db->cnid_sqlite_con));
                sqlite3_reset(db->cnid_del_devino_stmt);
                cnid_sqlite_set_errno(sqlite_return);
                EC_FAIL;
            }

            sqlite3_reset(db->cnid_del_devino_stmt);
        }

        sqlite3_reset(db->cnid_update_stmt);
        sqlite3_clear_bindings(db->cnid_update_stmt);
        sqlite3_bind_text(db->cnid_update_stmt, 1, name, (int)len,
                          SQLITE_STATIC);
        sqlite3_bind_int64(db->cnid_update_stmt, 2, ntohl(did));
        sqlite3_bind_int64(db->cnid_update_stmt, 3, dev);
        sqlite3_bind_int64(db->cnid_update_stmt, 4, ino);
        sqlite3_bind_int64(db->cnid_update_stmt, 5, ntohl(id));
        sqlite_return = sqlite3_step(db->cnid_update_stmt);
        sqlite3_reset(db->cnid_update_stmt);

        if (sqlite_return == SQLITE_DONE) {
            if (sqlite3_changes(db->cnid_sqlite_con) != 0) {
                break;
            }

            /* The UPDATE matched nothing, so no row holds this Id: reinsert */
            sqlite3_reset(db->cnid_put_stmt);
            sqlite3_clear_bindings(db->cnid_put_stmt);
            sqlite3_bind_int64(db->cnid_put_stmt, 1, ntohl(id));
            sqlite3_bind_text(db->cnid_put_stmt, 2, name, (int)len,
                              SQLITE_STATIC);
            sqlite3_bind_int64(db->cnid_put_stmt, 3, ntohl(did));
            sqlite3_bind_int64(db->cnid_put_stmt, 4, dev);
            sqlite3_bind_int64(db->cnid_put_stmt, 5, ino);
            sqlite_return = sqlite3_step(db->cnid_put_stmt);
            sqlite3_reset(db->cnid_put_stmt);

            if (sqlite_return == SQLITE_DONE) {
                break;
            }
        }

        /* Only a unique-key collision is worth a second attempt, and only
         * once: the next pass clears the colliding rows first */
        if ((sqlite_return & 0xff) != SQLITE_CONSTRAINT || attempt > 0) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_update: sqlite query error: %s",
                sqlite3_errmsg(db->cnid_sqlite_con));
            cnid_sqlite_set_errno(sqlite_return);
            EC_FAIL;
        }
    }

    if (owned) {
        EC_NEG1(cnid_sqlite_commit(db->cnid_sqlite_con, &owned));
    }

EC_CLEANUP:

    if (db) {
        LOG(log_maxdebug, logtype_cnid,
            "cnid_sqlite_update(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): END",
            ntohl(id), ntohl(did), name);
        cnid_sqlite_stmt_reset(db->cnid_put_stmt);
        cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
    }

    EC_EXIT;
}

cnid_t cnid_sqlite_lookup(struct _cnid_db *cdb, const struct stat *st,
                          cnid_t did, const char *name, size_t len)
{
    EC_INIT;
    CNID_sqlite_private *db = NULL;
    cnid_t id = CNID_INVALID;
    uint64_t dev = 0;
    uint64_t ino;
    cnid_t hint = CNID_INVALID;
    const unsigned char *retname;
    cnid_t retid;
    uint64_t retdid;
    int sqlite_return;
    char lookup_result_name[MAXPATHLEN];
    uint64_t lookup_result_id;
    uint64_t lookup_result_did;
    uint64_t lookup_result_dev;
    uint64_t lookup_result_ino;

    if (!cdb || !st || !name || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    if (!cnid_sqlite_stmt_ready(db->cnid_lookup_stmt, "cnid_sqlite_lookup")) {
        EC_FAIL;
    }

    ino = st->st_ino;
    hint = db->cnid_sqlite_hint;

    if (!(cdb->cnid_db_flags & CNID_FLAG_NODEV)) {
        dev = st->st_dev;
    }

    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_lookup(did: %" PRIu32 ", name: \"%s\", hint: %" PRIu32 "): BEGIN",
        ntohl(did), name, ntohl(hint));

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: Path name is too long");
        errno = CNID_ERR_PATH;
        EC_FAIL;
    }

    if (ntohl(did) < 2) {
        LOG(log_warning, logtype_cnid,
            "cnid_sqlite_lookup: not looking up illegal did: %" PRIu32, ntohl(did));
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    sqlite3_reset(db->cnid_lookup_stmt);
    sqlite3_clear_bindings(db->cnid_lookup_stmt);
    sqlite3_bind_text(db->cnid_lookup_stmt, 1, name, (int)len, SQLITE_STATIC);
    sqlite3_bind_int64(db->cnid_lookup_stmt, 2, ntohl(did));
    sqlite3_bind_int64(db->cnid_lookup_stmt, 3, dev);
    sqlite3_bind_int64(db->cnid_lookup_stmt, 4, ino);
    sqlite_return = sqlite3_step(db->cnid_lookup_stmt);

    if (sqlite_return == SQLITE_DONE) {
        /* Not found (no rows) */
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup: name: '%s', did: %u is not in the CNID database",
            name, ntohl(did));
        errno = CNID_ERR_NOTFOUND;
        EC_FAIL;
    } else if (sqlite_return != SQLITE_ROW) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: sqlite query error (1): %s - %i",
            sqlite3_errmsg(db->cnid_sqlite_con), sqlite_return);
        cnid_sqlite_set_errno(sqlite_return);
        EC_FAIL;
    }

    /* got at least one row, store result in lookup_result_X */
    lookup_result_id = sqlite3_column_int64(db->cnid_lookup_stmt, 0);
    lookup_result_did = sqlite3_column_int64(db->cnid_lookup_stmt, 1);
    retname = sqlite3_column_text(db->cnid_lookup_stmt, 2);

    if (retname) {
        strlcpy(lookup_result_name, (const char *)retname, MAXPATHLEN);
    } else {
        lookup_result_name[0] = '\0';
    }

    lookup_result_dev = sqlite3_column_int64(db->cnid_lookup_stmt, 3);
    lookup_result_ino = sqlite3_column_int64(db->cnid_lookup_stmt, 4);
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_lookup: id=%" PRIu64 ", did=%" PRIu64 ", name=%s, dev=%" PRIu64
        ", ino=%" PRIu64,
        lookup_result_id, lookup_result_did, lookup_result_name,
        lookup_result_dev, lookup_result_ino);
    /* Check for additional rows; collect them first, delete once the
     * statement is reset. Deleting mid-SELECT mutates the table under the
     * active cursor, whose remaining row order is then unspecified. The two
     * UNIQUE indexes bound the query to one row per disjunct, so the buffer
     * only has to hold the surplus one; a larger count is logged and cleaned
     * up over successive lookups. */
    int row_count = 1;
    int delete_errno = 0;
    uint64_t extra_ids[2];
    int extra_count = 0;

    while ((sqlite_return = sqlite3_step(db->cnid_lookup_stmt)) == SQLITE_ROW) {
        if (extra_count < (int)(sizeof(extra_ids) / sizeof(extra_ids[0]))) {
            extra_ids[extra_count++] =
                sqlite3_column_int64(db->cnid_lookup_stmt, 0);
        }

        row_count++;
    }

    if (sqlite_return != SQLITE_DONE) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: sqlite query error (step): %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        cnid_sqlite_set_errno(sqlite_return);
        EC_FAIL;
    }

    sqlite3_reset(db->cnid_lookup_stmt);

    for (int i = 0; i < extra_count; i++) {
        /* An out-of-range rowid is unreachable through this statement, whose
         * bound id is 32-bit: it is reported and left in place */
        if (!cnid_sqlite_rowid_ok(extra_ids[i])) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_lookup: out of range duplicate id %" PRIu64
                " for did: %u, name: '%s', not deleted",
                extra_ids[i], ntohl(did), name);
            delete_errno = CNID_ERR_CORRUPT;
            continue;
        }

        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup: multiple matches for did: %u, name: '%s', deleting extra id: %"
            PRIu64,
            ntohl(did), name, extra_ids[i]);

        if (cnid_sqlite_delete(cdb, htonl((cnid_t)extra_ids[i]))) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_lookup: sqlite query error (delete extra): %s",
                sqlite3_errmsg(db->cnid_sqlite_con));
            delete_errno = errno;
        }
    }

    /* row_count, not extra_count: extra_ids saturates at its size */
    if (row_count > 1) {
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup: deleted %d duplicate rows for did: %u, name: '%s'",
            row_count - 1, ntohl(did), name);

        if (delete_errno != 0) {
            /* Latched at the failure: a later successful delete enters the VFS,
             * which can overwrite errno */
            errno = delete_errno;
            EC_FAIL;
        }

        errno = CNID_ERR_NOTFOUND;
        EC_FAIL;
    }

    /* Did is range-checked with Id: a truncated Did can equal the queried did,
     * passing the comparison below on a row cnid_sqlite_resolve() calls corrupt */
    if (!cnid_sqlite_rowid_ok(lookup_result_id)
            || !cnid_sqlite_rowid_ok(lookup_result_did)) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: Invalid/corrupt row: id=%" PRIu64 ", did=%" PRIu64
            ", name='%s', dev=%" PRIu64 ", ino=%" PRIu64,
            lookup_result_id, lookup_result_did, lookup_result_name,
            lookup_result_dev, lookup_result_ino);
        errno = CNID_ERR_CORRUPT;
        EC_FAIL;
    }

    retid = htonl((uint32_t)lookup_result_id);
    retdid = htonl((uint32_t)lookup_result_did);

    if (retdid != did || STRCMP(lookup_result_name, !=, name)) {
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup(CNID %" PRIu32 ", hint %" PRIu32 ", DID: %" PRIu32
            ", name: \"%s\"): server side mv oder reused inode",
            retdid, ntohl(hint), ntohl(did), name);

        if (hint != retid) {
            if (cnid_sqlite_delete(cdb, retid) != 0) {
                /* errno holds the delete's classification and is passed through */
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_lookup: sqlite query error (hint delete): %s",
                    sqlite3_errmsg(db->cnid_sqlite_con));
                EC_FAIL;
            }

            errno = CNID_ERR_NOTFOUND;
            EC_FAIL;
        }

        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup: server side mv, got hint, updating");

        if (cnid_sqlite_update(cdb, retid, st, did, name, len) != 0) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_lookup: sqlite query error (update): %s",
                sqlite3_errmsg(db->cnid_sqlite_con));
            EC_FAIL;
        }

        id = retid;
    } else if (lookup_result_dev != dev || lookup_result_ino != ino) {
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup(DID:%u, name: \"%s\"): changed dev/ino",
            ntohl(did), name);

        if (cnid_sqlite_delete(cdb, retid) != 0) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_lookup: sqlite query error (delete): %s",
                sqlite3_errmsg(db->cnid_sqlite_con));
            EC_FAIL;
        }

        errno = CNID_ERR_NOTFOUND;
        EC_FAIL;
    } else {
        /* everything is good */
        id = retid;
    }

EC_CLEANUP:

    if (db) {
        LOG(log_maxdebug, logtype_cnid,
            "cnid_sqlite_lookup(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): END",
            ntohl(id), ntohl(did), name);
        cnid_sqlite_stmt_reset(db->cnid_lookup_stmt);
    }

    if (ret != 0) {
        id = CNID_INVALID;
    }

    return id;
}

cnid_t cnid_sqlite_add(struct _cnid_db *cdb,
                       const struct stat *st,
                       cnid_t did,
                       const char *name, size_t len, cnid_t hint)
{
    EC_INIT;
    CNID_sqlite_private *db = NULL;
    cnid_t id = CNID_INVALID;
    uint64_t lastid;
    uint64_t dev = 0;
    uint64_t ino;
    int sqlite_return;
    int owned = 0;
    int attempts = 0;
    uint64_t stmt_param_id;
    uint64_t stmt_param_did;
    uint64_t stmt_param_dev;
    uint64_t stmt_param_ino;
    char *sql = NULL;

    if (!cdb || !(db = cdb->cnid_db_private) || !st || !name) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_add: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_add(did: %" PRIu32 ", name: \"%s\", hint: %" PRIu32 "): BEGIN",
        ntohl(did), name, ntohl(hint));

    if (!cnid_sqlite_stmt_ready(db->cnid_add_stmt, "cnid_sqlite_add")
            || !cnid_sqlite_stmt_ready(db->cnid_put_stmt, "cnid_sqlite_add")) {
        EC_FAIL;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_add: Path name is too long");
        errno = CNID_ERR_PATH;
        EC_FAIL;
    }

    if (!(cdb->cnid_db_flags & CNID_FLAG_NODEV)) {
        dev = st->st_dev;
    }

    ino = st->st_ino;

    if (hint != CNID_INVALID && !cnid_sqlite_hint_usable(hint)) {
        LOG(log_warning, logtype_cnid,
            "cnid_sqlite_add: ignoring out-of-range CNID hint %" PRIu32
            " for name: \"%s\"", ntohl(hint), name);
        hint = CNID_INVALID;
    }

    db->cnid_sqlite_hint = hint;

    do {
        id = cnid_sqlite_lookup(cdb, st, did, name, len);
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_add: lookup returned id=%" PRIu32 ", errno=%d", ntohl(id), errno);

        if (id == CNID_INVALID) {
            /* Only a "not in the database" answer is resolvable by inserting.
             * Any other classification passes to the caller unchanged. */
            if (errno != CNID_ERR_NOTFOUND) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_add: lookup failed for did: %" PRIu32
                    ", name: \"%s\", errno=%d", ntohl(did), name, errno);
                EC_FAIL;
            }

            stmt_param_did = ntohl(did);
            stmt_param_dev = dev;
            stmt_param_ino = ino;
            LOG(log_debug, logtype_cnid,
                "cnid_sqlite_add: binding name='%s', did=%" PRIu64 ", dev=%" PRIu64 ", ino=%"
                PRIu64,
                name, stmt_param_did, stmt_param_dev, stmt_param_ino);

            if (name[0] == '\0' ||
                    stmt_param_did == 0 ||
                    stmt_param_ino == 0) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_add: Refusing to insert invalid/empty entry");
                errno = CNID_ERR_PARAM;
                EC_FAIL;
            }

            EC_NEG1(cnid_sqlite_begin(db->cnid_sqlite_con, &owned));

            /*
             * If the CNID set has previously overflowed
             * (CNID_SQLITE_FLAG_DEPLETED flag) ignore the CNID "hint"
             * read from AppleDouble or Extended Attributes.
             */
            if (!db->cnid_sqlite_hint
                    || (db->cnid_sqlite_flags & CNID_SQLITE_FLAG_DEPLETED)) {
                LOG(log_debug, logtype_cnid,
                    "cnid_sqlite_add: not using CNID hint, CNID set is depleted or hint not set");
                sqlite3_reset(db->cnid_add_stmt);
                sqlite3_clear_bindings(db->cnid_add_stmt);
                sqlite3_bind_text(db->cnid_add_stmt, 1, name, (int)len,
                                  SQLITE_STATIC);
                sqlite3_bind_int64(db->cnid_add_stmt, 2, stmt_param_did);
                sqlite3_bind_int64(db->cnid_add_stmt, 3, stmt_param_dev);
                sqlite3_bind_int64(db->cnid_add_stmt, 4, stmt_param_ino);
                sqlite_return = sqlite3_step(db->cnid_add_stmt);

                if ((sqlite_return & 0xff) == SQLITE_BUSY) {
                    LOG(log_warning, logtype_cnid,
                        "cnid_sqlite_add: database still locked after %d ms",
                        CNID_SQLITE_BUSY_TIMEOUT);
                }
            } else {
                stmt_param_id = ntohl(db->cnid_sqlite_hint);
                sqlite3_reset(db->cnid_put_stmt);
                sqlite3_clear_bindings(db->cnid_put_stmt);
                sqlite3_bind_int64(db->cnid_put_stmt, 1, stmt_param_id);
                sqlite3_bind_text(db->cnid_put_stmt, 2, name, (int)len,
                                  SQLITE_STATIC);
                sqlite3_bind_int64(db->cnid_put_stmt, 3, stmt_param_did);
                sqlite3_bind_int64(db->cnid_put_stmt, 4, stmt_param_dev);
                sqlite3_bind_int64(db->cnid_put_stmt, 5, stmt_param_ino);
                sqlite_return = sqlite3_step(db->cnid_put_stmt);

                if ((sqlite_return & 0xff) == SQLITE_BUSY) {
                    LOG(log_warning, logtype_cnid,
                        "cnid_sqlite_add: database still locked after %d ms",
                        CNID_SQLITE_BUSY_TIMEOUT);
                }
            }

            if (sqlite_return == SQLITE_DONE) {
                EC_NEG1(cnid_sqlite_commit(db->cnid_sqlite_con, &owned));
            } else {
                cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);

                if ((sqlite_return & 0xff) == SQLITE_CONSTRAINT) {
                    LOG(log_debug, logtype_cnid,
                        "cnid_sqlite_add: Unique constraint violation for combinations of: (name=%s, did=%"
                        PRIu32 ") or (dev=%" PRIu64 ", ino=%" PRIu64 ")", name, ntohl(did),
                        dev, ino);
                    db->cnid_sqlite_hint = CNID_INVALID;
                    continue;
                } else {
                    LOG(log_error, logtype_cnid,
                        "cnid_sqlite_add: sqlite query error: %s",
                        sqlite3_errmsg(db->cnid_sqlite_con));
                    cnid_sqlite_set_errno(sqlite_return);
                    EC_FAIL;
                }
            }

            lastid = sqlite3_last_insert_rowid(db->cnid_sqlite_con);

            if ((uint32_t) lastid > UINT32_MAX) {
                /* CNID set is depleted, restart from scratch */
                LOG(log_warning, logtype_cnid,
                    "cnid_sqlite_add: CNID set is depleted, emptying the CNID "
                    "table for volume '%s'; entries are rebuilt on access",
                    cdb->cnid_db_vol->v_localname);
                EC_NEG1(cnid_sqlite_begin(db->cnid_sqlite_con, &owned));
                EC_NEG1(asprintf(&sql, "UPDATE volumes SET Depleted = 1 WHERE VolUUID = '%s'",
                                 db->cnid_sqlite_voluuid_str));

                if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
                    cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
                    EC_FAIL;
                }

                free(sql);
                sql = NULL;
                EC_NEG1(asprintf(&sql, "DELETE FROM \"%s\"", db->cnid_sqlite_voluuid_str));

                if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
                    cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
                    EC_FAIL;
                }

                free(sql);
                sql = NULL;
                EC_NEG1(asprintf(&sql,
                                 "UPDATE sqlite_sequence SET seq = 16 WHERE name = '%s';",
                                 db->cnid_sqlite_voluuid_str));

                if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
                    cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
                    EC_FAIL;
                }

                free(sql);
                sql = NULL;
                EC_NEG1(asprintf(&sql, "INSERT INTO sqlite_sequence (name,seq) SELECT '%s', "
                                       "16 WHERE NOT EXISTS "
                                       "(SELECT changes() AS change "
                                       "FROM sqlite_sequence WHERE change <> 0);",
                                 db->cnid_sqlite_voluuid_str));

                if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
                    cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
                    EC_FAIL;
                }

                free(sql);
                sql = NULL;
                EC_NEG1(cnid_sqlite_commit(db->cnid_sqlite_con, &owned));
                db->cnid_sqlite_flags |= CNID_SQLITE_FLAG_DEPLETED;
                continue;
            }

            /* Finally assign our result */
            id = htonl((uint32_t) lastid);

            if (id == 0) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_add: Invalid CNID 0 returned after insert!");
                errno = CNID_ERR_CORRUPT;
                EC_FAIL;
            }
        }
    } while (id == CNID_INVALID && ++attempts < CNID_SQLITE_ADD_ATTEMPTS);

    if (id == CNID_INVALID) {
        /* Every attempt ended in a unique-key collision that the following
         * lookup then failed to find: a row owns (Did, Name) or
         * (DevNo, InodeNo) but is not reachable through either index. */
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_add: giving up after %d attempts for did: %" PRIu32
            ", name: \"%s\": colliding row is not reachable by lookup",
            attempts, ntohl(did), name);
        errno = CNID_ERR_CORRUPT;
        EC_FAIL;
    }

EC_CLEANUP:

    if (db) {
        LOG(log_maxdebug, logtype_cnid,
            "cnid_sqlite_add(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\", hint: %"
            PRIu32 "): END",
            ntohl(id), ntohl(did), name, ntohl(hint));
        cnid_sqlite_stmt_reset(db->cnid_add_stmt);
        cnid_sqlite_stmt_reset(db->cnid_put_stmt);
        cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
    }

    if (sql) {
        free(sql);
    }

    return id;
}

cnid_t cnid_sqlite_get(struct _cnid_db *cdb, cnid_t did, const char *name,
                       size_t len)
{
    EC_INIT;
    CNID_sqlite_private *db = NULL;
    cnid_t id = CNID_INVALID;
    int sqlite_return;

    if (!cdb || !(db = cdb->cnid_db_private) || !name) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_get: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_get(did: %" PRIu32 ", name: \"%s\"): BEGIN",
        ntohl(did), name);

    if (!cnid_sqlite_stmt_ready(db->cnid_get_stmt, "cnid_sqlite_get")) {
        EC_FAIL;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_get: name is too long");
        errno = CNID_ERR_PATH;
        return CNID_INVALID;
    }

    sqlite3_reset(db->cnid_get_stmt);
    sqlite3_clear_bindings(db->cnid_get_stmt);
    sqlite_return = sqlite3_bind_text(db->cnid_get_stmt, 1, name, (int) len,
                                      SQLITE_STATIC);

    if (sqlite_return == SQLITE_OK) {
        sqlite_return = sqlite3_bind_int64(db->cnid_get_stmt, 2, ntohl(did));
    }

    if (sqlite_return != SQLITE_OK) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_get: bind failed: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        cnid_sqlite_set_errno(sqlite_return);
        EC_FAIL;
    }

    sqlite_return = sqlite3_step(db->cnid_get_stmt);

    if (sqlite_return == SQLITE_DONE) {
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_get: name: '%s', did: %u is not in the CNID database",
            name, ntohl(did));
        errno = CNID_ERR_NOTFOUND;
    } else if (sqlite_return != SQLITE_ROW) {
        /* Classified so the caller can tell a missing entry from a contended
         * database */
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_get: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        cnid_sqlite_set_errno(sqlite_return);
        EC_FAIL;
    } else {
        uint64_t retid = sqlite3_column_int64(db->cnid_get_stmt, 0);

        if (!cnid_sqlite_rowid_ok(retid)) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_get: out of range id %" PRIu64
                " for did: %u, name: '%s'", retid, ntohl(did), name);
            errno = CNID_ERR_CORRUPT;
            EC_FAIL;
        }

        id = htonl((uint32_t) retid);
    }

EC_CLEANUP:

    if (db) {
        LOG(log_maxdebug, logtype_cnid,
            "cnid_sqlite_get(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): END",
            ntohl(id), ntohl(did), name);
        cnid_sqlite_stmt_reset(db->cnid_get_stmt);
    }

    return id;
}

char *cnid_sqlite_resolve(struct _cnid_db *cdb, cnid_t * id, void *buffer,
                          size_t len)
{
    EC_INIT;
    CNID_sqlite_private *db = NULL;

    /* id is both an input and an output: the failure paths below write it */
    if (!cdb || !(db = cdb->cnid_db_private) || !id || !buffer || len == 0) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_resolve: Parameter error");
        errno = CNID_ERR_PARAM;

        if (id) {
            *id = CNID_INVALID;
        }

        EC_FAIL;
    }

    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_resolve(id: %" PRIu32 "): BEGIN",
        ntohl(*id));

    if (!cnid_sqlite_stmt_ready(db->cnid_resolve_stmt, "cnid_sqlite_resolve")) {
        *id = CNID_INVALID;
        EC_FAIL;
    }

    sqlite3_reset(db->cnid_resolve_stmt);
    sqlite3_clear_bindings(db->cnid_resolve_stmt);
    int resolve_return = sqlite3_bind_int64(db->cnid_resolve_stmt, 1,
                                            ntohl(*id));

    if (resolve_return != SQLITE_OK) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_resolve: bind failed for id: %" PRIu32 ": %s",
            ntohl(*id), sqlite3_errmsg(db->cnid_sqlite_con));
        cnid_sqlite_set_errno(resolve_return);
        EC_FAIL;
    }

    resolve_return = sqlite3_step(db->cnid_resolve_stmt);

    if (resolve_return != SQLITE_ROW) {
        /* Only SQLITE_DONE is a missing id. afp_resolveid() in etc/afpd/file.c
         * answers that with the permanent AFPERR_NOID. */
        if (resolve_return == SQLITE_DONE) {
            LOG(log_debug, logtype_cnid,
                "cnid_sqlite_resolve: No result found for id: %" PRIu32, ntohl(*id));
            errno = CNID_ERR_NOTFOUND;
        } else {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_resolve: sqlite query error for id: %" PRIu32 ": %s",
                ntohl(*id), sqlite3_errmsg(db->cnid_sqlite_con));
            cnid_sqlite_set_errno(resolve_return);
        }

        *id = CNID_INVALID;
        EC_FAIL;
    }

    uint64_t resolve_result_did = sqlite3_column_int64(db->cnid_resolve_stmt, 0);
    const unsigned char *resolve_result_name =
        sqlite3_column_text(db->cnid_resolve_stmt, 1);

    /* Name can be NULL on a corrupt page or under memory pressure, and
     * strlcpy on it would fault afpd inside dirlookup */
    if (!cnid_sqlite_rowid_ok(resolve_result_did)
            || resolve_result_name == NULL) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_resolve: invalid/corrupt row for id: %" PRIu32
            ": did=%" PRIu64 ", name=%s",
            ntohl(*id), resolve_result_did,
            resolve_result_name ? (const char *) resolve_result_name : "(null)");
        errno = CNID_ERR_CORRUPT;
        EC_FAIL;
    }

    *id = htonl((uint32_t) resolve_result_did);
    strlcpy(buffer, (const char *) resolve_result_name, len);
    ((char *)buffer)[len - 1] = '\0';
    LOG(log_debug, logtype_cnid,
        "cnid_sqlite_resolve: resolved did: %u, name: \"%s\"",
        ntohl(*id), buffer);
EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_resolve(): END");

    if (db) {
        cnid_sqlite_stmt_reset(db->cnid_resolve_stmt);
    }

    if (ret != 0) {
        if (id) {
            *id = CNID_INVALID;
        }

        return NULL;
    }

    return buffer;
}

/*!
 * Caller passes buffer where we will store the db stamp
 */
int cnid_sqlite_getstamp(struct _cnid_db *cdb, void *buffer,
                         const size_t len)
{
    EC_INIT;
    CNID_sqlite_private *db = NULL;
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_getstamp(): BEGIN");

    /* CNID_INVALID is 0, which this int-returning function's callers read as
     * success, so parameter failures have to return -1 like every other path */
    if (!cdb || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_getstamp: Parameter error");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    if (!buffer) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_getstamp: bad buffer");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    if (!cnid_sqlite_stmt_ready(db->cnid_getstamp_stmt, "cnid_sqlite_getstamp")) {
        return -1;
    }

    sqlite3_reset(db->cnid_getstamp_stmt);
    sqlite3_clear_bindings(db->cnid_getstamp_stmt);
    int stamp_return = sqlite3_bind_text(db->cnid_getstamp_stmt, 1,
                                         cdb->cnid_db_vol->v_path,
                                         (int) strlen(cdb->cnid_db_vol->v_path),
                                         SQLITE_STATIC);

    if (stamp_return != SQLITE_OK) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_getstamp: bind failed for volume \"%s\": %s",
            cdb->cnid_db_vol->v_path, sqlite3_errmsg(db->cnid_sqlite_con));
        cnid_sqlite_set_errno(stamp_return);
        EC_FAIL;
    }

    stamp_return = sqlite3_step(db->cnid_getstamp_stmt);

    /* A missing volumes row is missing data, not a failing backend:
     * classifying SQLITE_DONE would end the session over a row that a
     * rebuild recreates */
    if (stamp_return == SQLITE_DONE) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_getstamp: no DB stamp for volume \"%s\"",
            cdb->cnid_db_vol->v_path);
        errno = CNID_ERR_CORRUPT;
        EC_FAIL;
    }

    if (stamp_return != SQLITE_ROW) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_getstamp: Can't get DB stamp for volume \"%s\": %s",
            cdb->cnid_db_vol->v_path, sqlite3_errmsg(db->cnid_sqlite_con));
        cnid_sqlite_set_errno(stamp_return);
        EC_FAIL;
    }

    /* A row with no Stamp is unusable data, not a failing backend: classifying
     * SQLITE_ROW would fall through to the session-fatal CNID_ERR_DB */
    if (sqlite3_column_text(db->cnid_getstamp_stmt, 0) == NULL) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_getstamp: NULL DB stamp for volume \"%s\"",
            cdb->cnid_db_vol->v_path);
        errno = CNID_ERR_CORRUPT;
        EC_FAIL;
    }

    LOG(log_debug, logtype_cnid,
        "cnid_sqlite_getstamp: Got DB stamp for volume \"%s\"",
        cdb->cnid_db_vol->v_path);
    strlcpy(buffer, (const char *)sqlite3_column_text(db->cnid_getstamp_stmt, 0),
            len);
EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_getstamp(): END");

    if (db) {
        cnid_sqlite_stmt_reset(db->cnid_getstamp_stmt);
    }

    EC_EXIT;
}

/*!
 * @brief Backend implementation of cnid_find() for the sqlite backend
 *
 * Parameters are pre-validated by the libatalk/cnid/cnid.c wrapper, so
 * cdb, name, namelen and buflen are all sane on entry. Detects truncation
 * by binding LIMIT max_results+1 and observing whether the extra row was
 * produced; reports it via @p more_available.
 *
 * @p namelen is unused: the SQLite backend builds the LIKE pattern via
 * asprintf("%%%s%%", name), which already requires a NUL-terminated
 * @p name. The parameter is kept to satisfy the cnid_db function-pointer
 * signature shared with the dbd / mysql backends.
 *
 * @p scope_did selects the statement: CNID_INVALID searches the whole
 * volume, anything else only the subtree rooted at that directory.
 */
int cnid_sqlite_find(struct _cnid_db *cdb, const char *name, size_t namelen _U_,
                     cnid_t scope_did,
                     void *buffer, size_t buflen, bool *more_available)
{
    EC_INIT;
    CNID_sqlite_private *db = NULL;
    sqlite3_stmt *stmt = NULL;
    int count = 0;
    char *namelike = NULL;
    cnid_t *cnids = (cnid_t *)buffer;
    unsigned long max_results = buflen / sizeof(cnid_t);
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_find(\"%s\"): BEGIN", name);

    if (!cdb || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_find: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    stmt = scope_did != CNID_INVALID ? db->cnid_find_scoped_stmt
           : db->cnid_find_stmt;

    if (!cnid_sqlite_stmt_ready(stmt, "cnid_sqlite_find")) {
        EC_FAIL;
    }

    /* Parameters pre-validated by the libatalk/cnid/cnid.c wrapper:
     *   cdb, name non-NULL; namelen in [1, MAXPATHLEN-sizeof(uint32_t)];
     *   buflen >= CNID_FIND_MIN_BUFLEN.
     * Re-checking here would risk emitting a different errno value
     * (CNID_ERR_PATH) than the wrapper (CNID_ERR_PARAM) for the same
     * input, contradicting the uniform-behaviour-across-backends goal. */

    if (more_available) {
        *more_available = false;
    }

    /* Construct the LIKE pattern, escaping any special characters */
    EC_NEG1(asprintf(&namelike, "%%%s%%", name));
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    int find_return;

    if (scope_did != CNID_INVALID) {
        /* The table stores host-order integers; the API traffics in network
         * order */
        find_return = sqlite3_bind_int64(stmt, 1,
                                         (sqlite3_int64) ntohl(scope_did));

        if (find_return == SQLITE_OK) {
            find_return = sqlite3_bind_text(stmt, 2, namelike,
                                            (int) strlen(namelike),
                                            SQLITE_STATIC);
        }

        if (find_return == SQLITE_OK) {
            /* LIMIT max_results + 1: peek for an extra row to detect "more
             * results exist" without a second query. */
            find_return = sqlite3_bind_int64(stmt, 3,
                                             (sqlite3_int64)(max_results + 1));
        }
    } else {
        find_return = sqlite3_bind_text(stmt, 1, namelike,
                                        (int) strlen(namelike), SQLITE_STATIC);

        if (find_return == SQLITE_OK) {
            /* LIMIT max_results + 1: peek for an extra row to detect "more
             * results exist" without a second query. */
            find_return = sqlite3_bind_int64(stmt, 2,
                                             (sqlite3_int64)(max_results + 1));
        }
    }

    if (find_return != SQLITE_OK) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_find: bind failed: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        cnid_sqlite_set_errno(find_return);
        EC_FAIL;
    }

    /* Fetch up to max_results + 1 rows. The (max_results + 1)-th row, if
     * it exists, is the "more available" sentinel and is not stored in
     * cnids[]. */
    /* The step return is the only reliable source here: sqlite3_errcode() is
     * sticky on the connection, so a SQLITE_CONSTRAINT that cnid_sqlite_add()
     * handled and moved on from would make this successful search look failed. */
    while ((find_return = sqlite3_step(stmt)) == SQLITE_ROW
            && count <= (int)max_results) {
        uint64_t rowid = sqlite3_column_int64(stmt, 0);

        /* A corrupt row is skipped, not fatal: the remaining matches are sound */
        if (!cnid_sqlite_rowid_ok(rowid)) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_find: out of range id %" PRIu64
                " for name like '%s', skipped", rowid, name);
            continue;
        }

        if (count < (int)max_results) {
            cnids[count] = htonl((uint32_t) rowid);
        }

        count++;
    }

    if (count > (int)max_results) {
        if (more_available) {
            *more_available = true;
        }

        count = (int)max_results;
    }

    if (find_return != SQLITE_ROW && find_return != SQLITE_DONE) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_find: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        cnid_sqlite_set_errno(find_return);
        EC_FAIL;
    }

    LOG(log_debug, logtype_cnid, "cnid_sqlite_find: got %d matches", count);
EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_find(): END");

    if (db && stmt) {
        cnid_sqlite_stmt_reset(stmt);
    }

    if (namelike) {
        free(namelike);
    }

    if (ret != 0) {
        count = -1;
    }

    return count;
}

cnid_t cnid_sqlite_rebuild_add(struct _cnid_db *cdb _U_,
                               const struct stat *st _U_,
                               cnid_t did _U_, const char *name _U_, size_t len _U_,
                               cnid_t hint _U_)
{
    LOG(log_error, logtype_cnid,
        "cnid_sqlite_rebuild_add(\"%s\"): not supported with sqlite CNID backend",
        name);
    return CNID_INVALID;
}

int cnid_sqlite_wipe(struct _cnid_db *cdb)
{
    EC_INIT;
    CNID_sqlite_private *db = NULL;
    char *sql = NULL;
    int owned = 0;

    if (!cdb || !(db = cdb->cnid_db_private) || !cdb->cnid_db_vol) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_wipe: Parameter error");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_wipe(\"%s\"): BEGIN",
        cdb->cnid_db_vol->v_localname);
    EC_NEG1(cnid_sqlite_begin(db->cnid_sqlite_con, &owned));
    EC_NEG1(asprintf(&sql,
                     "UPDATE volumes SET Depleted = 0 WHERE VolUUID = '%s';",
                     db->cnid_sqlite_voluuid_str));

    if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
        cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
        EC_FAIL;
    }

    free(sql);
    sql = NULL;
    EC_NEG1(asprintf(&sql, "DELETE FROM \"%s\";", db->cnid_sqlite_voluuid_str));

    if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
        cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
        EC_FAIL;
    }

    free(sql);
    sql = NULL;
    EC_NEG1(asprintf(&sql,
                     "UPDATE sqlite_sequence SET seq = 16 WHERE name = '%s';",
                     db->cnid_sqlite_voluuid_str));

    if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
        cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
        EC_FAIL;
    }

    free(sql);
    sql = NULL;
    EC_NEG1(asprintf(&sql,
                     "INSERT INTO sqlite_sequence (name,seq) SELECT '%s', 16 WHERE NOT EXISTS (SELECT changes() AS change FROM sqlite_sequence WHERE change <> 0);",
                     db->cnid_sqlite_voluuid_str));

    if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
        cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
        EC_FAIL;
    }

    free(sql);
    sql = NULL;
    EC_NEG1(cnid_sqlite_commit(db->cnid_sqlite_con, &owned));

    if (init_prepared_stmt(db) != 0) {
        /* The wipe is committed and the handles past the failure are NULL.
         * Finalizing the rest leaves no half-prepared set to bind into, and
         * cnid_sqlite_stmt_ready() then fails every operation until reopen. */
        close_prepared_stmt(db);
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_wipe: could not re-prepare statements for volume "
            "'%s'; the CNID database has to be reopened",
            cdb->cnid_db_vol->v_localname);
        errno = CNID_ERR_CLOSE;
        EC_FAIL;
    }

EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_wipe(\"%s\"): END",
        cdb->cnid_db_vol->v_localname);

    if (sql) {
        free(sql);
    }

    if (db) {
        cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
    }

    EC_EXIT;
}

static struct _cnid_db *cnid_sqlite_new(struct vol *vol)
{
    struct _cnid_db *cdb = (struct _cnid_db *) calloc(1, sizeof(struct _cnid_db));

    if (cdb == NULL) {
        return NULL;
    }

    cdb->cnid_db_vol = vol;
    cdb->cnid_db_flags = CNID_FLAG_PERSISTENT | CNID_FLAG_LAZY_INIT;
    cdb->cnid_add = cnid_sqlite_add;
    cdb->cnid_delete = cnid_sqlite_delete;
    cdb->cnid_get = cnid_sqlite_get;
    cdb->cnid_lookup = cnid_sqlite_lookup;
    cdb->cnid_find = cnid_sqlite_find;
    cdb->cnid_nextid = NULL;
    cdb->cnid_resolve = cnid_sqlite_resolve;
    cdb->cnid_getstamp = cnid_sqlite_getstamp;
    cdb->cnid_update = cnid_sqlite_update;
    cdb->cnid_rebuild_add = cnid_sqlite_rebuild_add;
    cdb->cnid_close = cnid_sqlite_close;
    cdb->cnid_wipe = cnid_sqlite_wipe;
    return cdb;
}

/* ---------------------- */
struct _cnid_db *cnid_sqlite_open(struct cnid_open_args *args)
{
    EC_INIT;
    int owned = 0;
    CNID_sqlite_private *db = NULL;
    struct _cnid_db *cdb = NULL;
    char *sql = NULL;
    struct vol *vol = args->cnid_args_vol;
    sqlite3_stmt *transient_stmt = NULL;
    char dirpath[PATH_MAX];
    bstring dbpath = NULL;
    const char *dbpath_str = NULL;
    int sqlite_return;
    bool is_root = false;
    const bool unprivileged = vol->v_obj != NULL
                              && (vol->v_obj->options.flags & OPTION_UNPRIVILEGED);
    EC_NULL(cdb = cnid_sqlite_new(vol));
    EC_NULL(db =
                (CNID_sqlite_private *) calloc(1,
                    sizeof(CNID_sqlite_private)));
    cdb->cnid_db_private = db;

    if (vol->v_dbpath) {
        snprintf(dirpath, sizeof(dirpath), "%s", vol->v_dbpath);
    } else {
        snprintf(dirpath, sizeof(dirpath), "%sCNID/%s", _PATH_STATEDIR,
                 vol->v_localname);
    }

    become_root();
    is_root = true;

    if (mkdir(dirpath, unprivileged ? 0700 : 01777) != 0) {
        if (errno == EEXIST) {
            int dirfd = open(dirpath, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);

            if (dirfd < 0) {
                LOG(log_error, logtype_cnid, "'%s' exists but is not a directory", dirpath);
                EC_FAIL;
            }

            /* Normal servers share SQLite state with authenticated users and nad.
             * A rootless server has only one user, so retain private state instead. */
            struct stat st;

            if (fstat(dirfd, &st) != 0 || !S_ISDIR(st.st_mode)) {
                LOG(log_error, logtype_cnid, "Can't stat CNID DB directory '%s': %s",
                    dirpath, strerror(errno));
                close(dirfd);
                EC_FAIL;
            }

            if (unprivileged) {
                if (st.st_uid != getuid()) {
                    LOG(log_error, logtype_cnid,
                        "Rootless CNID DB directory '%s' is not owned by the server user",
                        dirpath);
                    close(dirfd);
                    EC_FAIL;
                }

                if ((st.st_mode & 0777) != 0700 && fchmod(dirfd, 0700) != 0) {
                    LOG(log_error, logtype_cnid,
                        "Can't make rootless CNID DB directory '%s' private: %s",
                        dirpath, strerror(errno));
                    close(dirfd);
                    EC_FAIL;
                }
            } else if ((st.st_mode & 01777) != 01777) {
                fchmod(dirfd, 01777);
            }

            close(dirfd);
        } else {
            LOG(log_error, logtype_cnid, "Failed to create CNID DB directory '%s': %s",
                dirpath, strerror(errno));
            EC_FAIL;
        }
    }

    unbecome_root();
    is_root = false;
    EC_NULL(dbpath = bformat("%s/%s.sqlite", dirpath, vol->v_localname));
    dbpath_str = bdata(dbpath);
    EC_NULL(db->cnid_sqlite_voluuid_str = uuid_strip_dashes(vol->v_uuid));

    /* The UUID names this volume's CNID table and is its key in the volumes
     * table. Volumes sharing an empty one would share a single table named ""
     * and collide on that primary key, serving each other's CNIDs. */
    if (db->cnid_sqlite_voluuid_str[0] == '\0') {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_open: volume '%s' has no UUID, so its CNID table "
            "cannot be named; check that %s is writable",
            vol->v_path, vol->v_obj ? vol->v_obj->options.uuidconf : "the UUID file");
        EC_FAIL;
    }

    EC_ZERO(sqlite3_initialize());
    become_root();
    is_root = true;

    if (sqlite3_open_v2(dbpath_str,
                        &db->cnid_sqlite_con,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL)) {
        LOG(log_error, logtype_cnid, "sqlite open error: %s, path: %s",
            sqlite3_errmsg(db->cnid_sqlite_con),
            dbpath_str);
        EC_FAIL;
    }

    /* Without this every result code arrives masked to its primary value and
     * the READONLY sub-codes cnid_sqlite_set_errno() distinguishes can never
     * be seen: a transient recovery state would classify as a read-only
     * volume. Everything else classifies through a & 0xff mask, so the wider
     * codes change nothing there. */
    sqlite3_extended_result_codes(db->cnid_sqlite_con, 1);

    /* Normal servers need a world-writable database so authenticated users and
     * nad can update CNID state. A rootless server has one identity and keeps
     * its database private.
     *
     * At the same time, do not treat a failure to change the permissions as a fatal error,
     * because non-root clients such as 'nad' may open a normal server's database. */
    if (dbpath_str && chmod(dbpath_str, unprivileged ? 0600 : 0666) != 0) {
        if (unprivileged) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_open: can't make rootless DB file %s private: %s",
                dbpath_str, strerror(errno));
            EC_FAIL;
        } else if (errno == EPERM || errno == EACCES) {
            LOG(log_debug, logtype_cnid,
                "cnid_sqlite_open: Current user has no permissions to set permissions on db file %s: %s",
                dbpath_str, strerror(errno));
        } else {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_open: Failed to set permissions on db file %s: %s",
                dbpath_str, strerror(errno));
        }
    }

    sqlite3_busy_timeout(db->cnid_sqlite_con, CNID_SQLITE_BUSY_TIMEOUT);

    /* Neither pragma is worth refusing the volume over: a contended
     * journal_mode conversion leaves the database in rollback-journal mode,
     * which is slower but correct */
    if (cnid_sqlite_execute(db->cnid_sqlite_con,
                            "PRAGMA synchronous=NORMAL;") < 0) {
        LOG(log_warning, logtype_cnid,
            "cnid_sqlite_open: could not set synchronous=NORMAL for volume '%s'",
            vol->v_path);
    }

    if (cnid_sqlite_execute(db->cnid_sqlite_con, "PRAGMA journal_mode=WAL;") < 0) {
        LOG(log_warning, logtype_cnid,
            "cnid_sqlite_open: could not switch volume '%s' to WAL mode",
            vol->v_path);
    }

    /* SQLite creates WAL and SHM files when WAL mode is enabled. They are
     * shared for normal servers, but private in rootless mode. */
    {
        char auxpath[PATH_MAX];
        snprintf(auxpath, sizeof(auxpath), "%s-wal", dbpath_str);

        if (chmod(auxpath, unprivileged ? 0600 : 0666) != 0 && errno != ENOENT) {
            if (unprivileged) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_open: can't make rootless WAL file %s private: %s",
                    auxpath, strerror(errno));
                EC_FAIL;
            } else {
                LOG(log_debug, logtype_cnid,
                    "cnid_sqlite_open: chmod failed for %s: %s",
                    auxpath, strerror(errno));
            }
        }

        snprintf(auxpath, sizeof(auxpath), "%s-shm", dbpath_str);

        if (chmod(auxpath, unprivileged ? 0600 : 0666) != 0 && errno != ENOENT) {
            if (unprivileged) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_open: can't make rootless SHM file %s private: %s",
                    auxpath, strerror(errno));
                EC_FAIL;
            } else {
                LOG(log_debug, logtype_cnid,
                    "cnid_sqlite_open: chmod failed for %s: %s",
                    auxpath, strerror(errno));
            }
        }
    }

    /* Add volume to volume table */
    if (cnid_sqlite_execute(db->cnid_sqlite_con,
                            "CREATE TABLE IF NOT EXISTS volumes "
                            "(VolUUID CHAR(32) PRIMARY KEY, "
                            "VolPath TEXT(4096), "
                            "Stamp BINARY(8), "
                            "Depleted INT"
                            ")") < 0) {
        EC_FAIL;
    }

    /* Add index to volume */
    if (cnid_sqlite_execute(db->cnid_sqlite_con,
                            "CREATE INDEX IF NOT EXISTS idx_volpath "
                            "ON volumes(VolPath)") < 0) {
        EC_FAIL;
    }

    /*
     * Clean up stale volume entries for the same path but with a different UUID.
     * This can happen when the UUID config file is rewritten without the entry
     * for this volume (e.g. [Homes] volumes not loaded during load_afp_conf_vols),
     * causing a new UUID to be generated on the next open.
     */
    if (sqlite3_prepare_v2(db->cnid_sqlite_con,
                           "SELECT VolUUID FROM volumes WHERE VolPath = ? AND VolUUID != ?",
                           -1, &transient_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(transient_stmt, 1, vol->v_path, -1, SQLITE_STATIC);
        sqlite3_bind_text(transient_stmt, 2, db->cnid_sqlite_voluuid_str, -1,
                          SQLITE_STATIC);
        /* Collect stale UUIDs first, then clean up after finalizing the
         * statement.  Executing DROP TABLE while the SELECT is still
         * stepping would change the schema cookie and could cause
         * sqlite3_step() to return SQLITE_SCHEMA, aborting the loop
         * early and leaving stale entries behind. */
        char *stale_uuids[64];
        int stale_count = 0;
        int stale_return;

        while ((stale_return = sqlite3_step(transient_stmt)) == SQLITE_ROW
                && stale_count < 64) {
            const char *stale_uuid = (const char *)sqlite3_column_text(transient_stmt, 0);

            if (stale_uuid == NULL
                    || (stale_uuids[stale_count] = strdup(stale_uuid)) == NULL) {
                continue;
            }

            stale_count++;
        }

        if (stale_return != SQLITE_DONE) {
            LOG(log_warning, logtype_cnid,
                "cnid_sqlite_open: stale volume scan for path '%s' stopped "
                "early; the remainder is retried at the next open", vol->v_path);
        }

        sqlite3_finalize(transient_stmt);
        transient_stmt = NULL;

        /* Each volumes row is the only record of its table's name, so a row is
         * removed only once its own table and sequence entry are gone: a
         * blanket delete after a failed or truncated drop pass would orphan
         * every table it missed. Deleting per UUID also converges past the
         * collection cap, one batch per open. */
        for (int i = 0; i < stale_count; i++) {
            char *stale_sql = NULL;
            int removed;

            /* A UUID that is not 32 hex digits names no table this code
             * created, so only its row is removed. The row is what brings it
             * back at the next open. */
            if (!cnid_sqlite_uuid_usable(stale_uuids[i])) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_open: refusing to act on malformed stale volume "
                    "UUID for path '%s'; removing its entry", vol->v_path);
                removed = cnid_sqlite_delete_volumes_row(db->cnid_sqlite_con,
                          stale_uuids[i]) == 0;

                if (!removed) {
                    LOG(log_warning, logtype_cnid,
                        "cnid_sqlite_open: could not remove malformed stale volume "
                        "entry for path '%s'", vol->v_path);
                }

                free(stale_uuids[i]);
                continue;
            }

            LOG(log_warning, logtype_cnid,
                "cnid_sqlite_open: removing stale volume entry UUID '%s' for path '%s'",
                stale_uuids[i], vol->v_path);

            if (asprintf(&stale_sql, "DROP TABLE IF EXISTS \"%s\"",
                         stale_uuids[i]) == -1) {
                stale_sql = NULL;
                removed = 0;
            } else {
                removed = cnid_sqlite_execute(db->cnid_sqlite_con, stale_sql) == 0;
                free(stale_sql);
            }

            if (removed) {
                removed = cnid_sqlite_delete_sequence_row(db->cnid_sqlite_con,
                          stale_uuids[i]) == 0;
            }

            if (removed) {
                removed = cnid_sqlite_delete_volumes_row(db->cnid_sqlite_con,
                          stale_uuids[i]) == 0;
            }

            if (!removed) {
                LOG(log_warning, logtype_cnid,
                    "cnid_sqlite_open: could not fully remove stale volume UUID "
                    "'%s' for path '%s'; keeping its entry to retry at the next "
                    "open", stale_uuids[i], vol->v_path);
            }

            free(stale_uuids[i]);
        }
    }

    /* The stamp is binary, so it is bound rather than built into the SQL */
    time_t now = time(NULL);
    char stamp[8];
    memset(stamp, 0, 8);
    memcpy(stamp, &now, sizeof(time_t));
    EC_NEG1(asprintf
            (&sql,
             "INSERT OR IGNORE INTO volumes "
             "(VolUUID, Volpath, Stamp, Depleted) "
             "VALUES(?, ?, ?, 0)"));
    EC_ZERO_LOG(sqlite3_prepare_v2
                (db->cnid_sqlite_con, sql, (int) strlen(sql), &transient_stmt, NULL));
    free(sql);
    sql = NULL;
    EC_ZERO_LOG(sqlite3_bind_text(transient_stmt,
                                  1, db->cnid_sqlite_voluuid_str,
                                  (int) strlen(db->cnid_sqlite_voluuid_str), SQLITE_STATIC));
    EC_ZERO_LOG(sqlite3_bind_text(transient_stmt,
                                  2, vol->v_path,
                                  (int) strlen(vol->v_path), SQLITE_STATIC));
    EC_ZERO_LOG(sqlite3_bind_text(transient_stmt,
                                  3, stamp,
                                  (int) strlen(stamp), SQLITE_STATIC));

    if (sqlite3_step(transient_stmt) != SQLITE_DONE) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_open: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        EC_FAIL;
    }

    sqlite3_reset(transient_stmt);
    sqlite3_clear_bindings(transient_stmt);
    sqlite3_finalize(transient_stmt);
    transient_stmt = NULL;
    unbecome_root();
    is_root = false;

    /*
     * Check whether CNID set overflowed before.
     * If that's the case, in cnid_sqlite_add() we'll ignore the CNID
     * "hint" taken from the AppleDouble file.
     */
    if (sqlite3_prepare_v2(db->cnid_sqlite_con,
                           "SELECT Depleted FROM volumes WHERE VolUUID = ?", -1, &transient_stmt,
                           NULL) != SQLITE_OK) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_open: prepare failed: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        EC_FAIL;
    }

    if (sqlite3_bind_text(transient_stmt, 1, db->cnid_sqlite_voluuid_str, -1,
                          SQLITE_STATIC) != SQLITE_OK) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_open: bind failed: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        sqlite3_reset(transient_stmt);
        EC_FAIL;
    }

    sqlite_return = sqlite3_step(transient_stmt);

    if (sqlite_return == SQLITE_ROW) {
        if (sqlite3_column_int(transient_stmt, 0)) {
            LOG(log_info, logtype_cnid,
                "CNID set for volume '%s' was depleted before, "
                "ignoring CNID hints in AppleDouble metadata\n"
                "It is recommended to rebuild the CNID database with 'dbd -f' "
                "for long-term reliability",
                vol->v_path);
            db->cnid_sqlite_flags |= CNID_SQLITE_FLAG_DEPLETED;
        }
    } else if (sqlite_return != SQLITE_DONE) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_open: step failed: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        sqlite3_reset(transient_stmt);
        EC_FAIL;
    }

    sqlite3_reset(transient_stmt);
    sqlite3_clear_bindings(transient_stmt);
    /* Create volume table */
    EC_NEG1(asprintf(&sql, "CREATE TABLE IF NOT EXISTS \"%s\" ("
                           "Id INTEGER PRIMARY KEY AUTOINCREMENT,"
                           "Name VARCHAR(255) NOT NULL,"
                           "Did INTEGER NOT NULL,"
                           "DevNo INTEGER NOT NULL,"
                           "InodeNo INTEGER NOT NULL,"
                           "UNIQUE (Did, Name),"
                           "UNIQUE (DevNo, InodeNo)"
                           ");",
                     db->cnid_sqlite_voluuid_str));

    if (cnid_sqlite_execute(db->cnid_sqlite_con, sql)) {
        EC_FAIL;
    }

    free(sql);
    sql = NULL;
    /* Whether the table was just created. LIMIT 1 answers that: only its
     * emptiness matters, not the row count. */
    int table_is_empty = -1;
    char *check_sql = NULL;
    sqlite3_stmt *check_table_stmt = NULL;
    EC_NEG1(asprintf(&check_sql, "SELECT 1 FROM \"%s\" LIMIT 1;",
                     db->cnid_sqlite_voluuid_str));

    /* The seeding below needs a definite answer: it rewrites a live sequence on
     * a populated table, and an unseeded empty one hands out the CNIDs AFP
     * reserves. The prepare is inside the loop because it takes the schema lock
     * and can itself return SQLITE_BUSY. */
    for (int attempt = 0; attempt < CNID_SQLITE_EMPTY_PROBE_TRIES
            && table_is_empty < 0; attempt++) {
        if (sqlite3_prepare_v2(db->cnid_sqlite_con, check_sql, -1, &check_table_stmt,
                               NULL) == SQLITE_OK) {
            int check_return = sqlite3_step(check_table_stmt);

            if (check_return == SQLITE_ROW) {
                table_is_empty = 0;
            } else if (check_return == SQLITE_DONE) {
                table_is_empty = 1;
            } else {
                LOG(log_warning, logtype_cnid,
                    "cnid_sqlite_open: emptiness probe for volume '%s' failed: "
                    "%s", vol->v_path, sqlite3_errmsg(db->cnid_sqlite_con));
            }

            sqlite3_finalize(check_table_stmt);
            check_table_stmt = NULL;
        }
    }

    free(check_sql);
    check_sql = NULL;

    if (table_is_empty < 0) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_open: could not determine whether the CNID table for "
            "volume '%s' is empty", vol->v_path);
        EC_FAIL;
    }

    if (table_is_empty == 1) {
        /* Directory IDs from 1 to 16 are reserved.
         * The Directory ID of the root is always 2.
         * The root’s Parent ID is always 1.
         * (The root does not really have a parent;
         * this value is returned only if an AFP command asks
         * for the root’s Parent ID.)
         * Zero (0) is not a valid Directory ID.
         *
         * https://developer.apple.com/library/archive/documentation/Networking/Conceptual/AFP/Concepts/Concepts.html
         */
        LOG(log_debug, logtype_cnid,
            "Creating new CNID table for volume '%s' with ID sequence starting at 16",
            vol->v_path);
        /* Seeding is one unit: a partly seeded sequence would hand out CNIDs
         * from the range AFP reserves */
        EC_NEG1(cnid_sqlite_begin(db->cnid_sqlite_con, &owned));
        EC_NEG1(asprintf(&sql,
                         "UPDATE sqlite_sequence SET seq = 16 WHERE name = '%s';",
                         db->cnid_sqlite_voluuid_str));

        if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
            cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
            EC_FAIL;
        }

        free(sql);
        sql = NULL;
        EC_NEG1(asprintf(&sql, "INSERT INTO sqlite_sequence (name,seq) SELECT '%s',"
                               "16 WHERE NOT EXISTS "
                               "(SELECT changes() AS change "
                               "FROM sqlite_sequence WHERE change <> 0);",
                         db->cnid_sqlite_voluuid_str));

        if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
            cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
            EC_FAIL;
        }

        free(sql);
        sql = NULL;
        EC_NEG1(cnid_sqlite_commit(db->cnid_sqlite_con, &owned));
        /* Verify the sequence was set correctly */
        sqlite3_stmt *verify_stmt = NULL;
        EC_NEG1(asprintf(&sql, "SELECT seq FROM sqlite_sequence WHERE name = '%s';",
                         db->cnid_sqlite_voluuid_str));

        if (sqlite3_prepare_v2(db->cnid_sqlite_con, sql, -1, &verify_stmt,
                               NULL) == SQLITE_OK) {
            if (sqlite3_step(verify_stmt) == SQLITE_ROW) {
                int64_t seq = sqlite3_column_int64(verify_stmt, 0);
                LOG(log_debug, logtype_cnid,
                    "Verified: sqlite_sequence initialized to seq = %" PRId64, seq);
            } else {
                LOG(log_warning, logtype_cnid,
                    "No row found in sqlite_sequence after initialization");
            }

            sqlite3_finalize(verify_stmt);
        } else {
            LOG(log_warning, logtype_cnid,
                "Failed to verify sqlite_sequence initialization");
        }

        free(sql);
        sql = NULL;
    } else {
        LOG(log_info, logtype_cnid,
            "CNID table for volume '%s' is already initialized", vol->v_path);
    }

    EC_ZERO(init_prepared_stmt(db));
    LOG(log_debug, logtype_cnid,
        "Finished initializing sqlite CNID module for volume '%s'",
        vol->v_path);
EC_CLEANUP:

    if (is_root) {
        unbecome_root();
    }

    if (transient_stmt) {
        sqlite3_finalize(transient_stmt);
    }

    if (ret != 0) {
        if (cdb) {
            free(cdb);
        }

        cdb = NULL;

        if (db) {
            /* The connection is unreachable after this, so an abandoned
             * transaction would hold the write lock for the life of the
             * process */
            if (db->cnid_sqlite_con) {
                cnid_sqlite_rollback(db->cnid_sqlite_con, &owned);
                close_prepared_stmt(db);
                sqlite3_close(db->cnid_sqlite_con);
            }

            if (db->cnid_sqlite_voluuid_str) {
                free(db->cnid_sqlite_voluuid_str);
            }

            free(db);
        }
    }

    if (sql) {
        free(sql);
    }

    if (dbpath) {
        bdestroy(dbpath);
    }

    return cdb;
}

struct _cnid_module cnid_sqlite_module = {
    "sqlite",
    { NULL, NULL },
    cnid_sqlite_open,
    0
};
