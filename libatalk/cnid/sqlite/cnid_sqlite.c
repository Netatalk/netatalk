/*
 * Copyright (C) 2013 Ralph Boehme
 * Copyright (C) 2024-2025 Daniel Markstedt
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
#include <errno.h>
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

#include <sqlite3.h>

#include <atalk/adouble.h>
#include <atalk/bstrlib.h>
#include <atalk/cnid_bdb_private.h>
#include <atalk/cnid_sqlite_private.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/volume.h>

static int init_prepared_stmt_lookup(CNID_sqlite_private * db)
{
    EC_INIT;
    char *sql = NULL;

    if (db->cnid_lookup_stmt) {
        EC_ZERO(sqlite3_finalize(db->cnid_lookup_stmt));
    }

    EC_NEG1(asprintf
            (&sql,
             "SELECT Id,Did,Name,DevNo,InodeNo FROM \"%s\" "
             "WHERE (Name = ? AND Did = ?) OR (DevNo = ? AND InodeNo = ?)",
             db->cnid_sqlite_voluuid_str));
    LOG(log_maxdebug, logtype_cnid, "init_prepared_stmt_lookup: SQL: %s", sql);
    EC_ZERO_LOG(sqlite3_prepare_v2
                (db->cnid_sqlite_con, sql, strlen(sql), &db->cnid_lookup_stmt, NULL));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

static int init_prepared_stmt_add(CNID_sqlite_private * db)
{
    EC_INIT;
    char *sql = NULL;

    if (db->cnid_add_stmt) {
        EC_ZERO(sqlite3_finalize(db->cnid_add_stmt));
    }

    EC_NEG1(asprintf(&sql,
                     "INSERT INTO \"%s\" (Name, Did, DevNo, InodeNo) VALUES(?, ?, ?, ?)",
                     db->cnid_sqlite_voluuid_str));
    LOG(log_maxdebug, logtype_cnid, "init_prepared_stmt_add: SQL: %s", sql);
    EC_ZERO_LOG(sqlite3_prepare_v2
                (db->cnid_sqlite_con, sql, strlen(sql), &db->cnid_add_stmt, NULL));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

static int init_prepared_stmt_put(CNID_sqlite_private * db)
{
    EC_INIT;
    char *sql = NULL;

    if (db->cnid_put_stmt) {
        EC_ZERO(sqlite3_finalize(db->cnid_put_stmt));
    }

    EC_NEG1(asprintf(&sql,
                     "INSERT INTO \"%s\" (Id, Name, Did, DevNo, InodeNo) VALUES(?, ?, ?, ?, ?)",
                     db->cnid_sqlite_voluuid_str));
    LOG(log_maxdebug, logtype_cnid, "init_prepared_stmt_put: SQL: %s", sql);
    EC_ZERO_LOG(sqlite3_prepare_v2
                (db->cnid_sqlite_con, sql, strlen(sql), &db->cnid_put_stmt, NULL));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

static int init_prepared_stmt_get(CNID_sqlite_private *db)
{
    EC_INIT;
    char *sql = NULL;

    if (db->cnid_get_stmt) {
        sqlite3_finalize(db->cnid_get_stmt);
    }

    EC_NEG1(asprintf(&sql, "SELECT Id FROM \"%s\" WHERE Name = ? AND Did = ?",
                     db->cnid_sqlite_voluuid_str));
    LOG(log_maxdebug, logtype_cnid, "init_prepared_stmt_get: SQL: %s", sql);
    EC_ZERO_LOG(sqlite3_prepare_v2(db->cnid_sqlite_con, sql, strlen(sql),
                                   &db->cnid_get_stmt, NULL));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

static int init_prepared_stmt_resolve(CNID_sqlite_private *db)
{
    EC_INIT;
    char *sql = NULL;

    if (db->cnid_resolve_stmt) {
        sqlite3_finalize(db->cnid_resolve_stmt);
    }

    EC_NEG1(asprintf(&sql, "SELECT Did, Name FROM \"%s\" WHERE Id = ?",
                     db->cnid_sqlite_voluuid_str));
    LOG(log_maxdebug, logtype_cnid, "init_prepared_stmt_resolve: SQL: %s", sql);
    EC_ZERO_LOG(sqlite3_prepare_v2(db->cnid_sqlite_con, sql, strlen(sql),
                                   &db->cnid_resolve_stmt, NULL));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

static int init_prepared_stmt_delete(CNID_sqlite_private *db)
{
    EC_INIT;
    char *sql = NULL;

    if (db->cnid_delete_stmt) {
        sqlite3_finalize(db->cnid_delete_stmt);
    }

    EC_NEG1(asprintf(&sql, "DELETE FROM \"%s\" WHERE Id = ?",
                     db->cnid_sqlite_voluuid_str));
    LOG(log_maxdebug, logtype_cnid, "init_prepared_stmt_delete: SQL: %s", sql);
    EC_ZERO_LOG(sqlite3_prepare_v2(db->cnid_sqlite_con, sql, strlen(sql),
                                   &db->cnid_delete_stmt, NULL));
EC_CLEANUP:
    EC_EXIT;
}

static int init_prepared_stmt_getstamp(CNID_sqlite_private *db)
{
    EC_INIT;
    char *sql = "SELECT Stamp FROM volumes WHERE VolPath = ?";

    if (db->cnid_getstamp_stmt) {
        sqlite3_finalize(db->cnid_getstamp_stmt);
    }

    LOG(log_maxdebug, logtype_cnid, "init_prepared_stmt_getstamp: SQL: %s", sql);
    EC_ZERO_LOG(sqlite3_prepare_v2(db->cnid_sqlite_con, sql, strlen(sql),
                                   &db->cnid_getstamp_stmt, NULL));
EC_CLEANUP:
    EC_EXIT;
}

static int init_prepared_stmt(CNID_sqlite_private * db)
{
    EC_INIT;
    EC_ZERO(init_prepared_stmt_lookup(db));
    EC_ZERO(init_prepared_stmt_add(db));
    EC_ZERO(init_prepared_stmt_put(db));
    EC_ZERO(init_prepared_stmt_get(db));
    EC_ZERO(init_prepared_stmt_resolve(db));
    EC_ZERO(init_prepared_stmt_delete(db));
    EC_ZERO(init_prepared_stmt_getstamp(db));
    EC_EXIT;
EC_CLEANUP:
    EC_EXIT;
}

static void close_prepared_stmt(CNID_sqlite_private * db)
{
    sqlite3_finalize(db->cnid_lookup_stmt);
    sqlite3_finalize(db->cnid_add_stmt);
    sqlite3_finalize(db->cnid_put_stmt);
    sqlite3_finalize(db->cnid_get_stmt);
    sqlite3_finalize(db->cnid_resolve_stmt);
    sqlite3_finalize(db->cnid_delete_stmt);
    sqlite3_finalize(db->cnid_getstamp_stmt);
    db->cnid_lookup_stmt = NULL;
    db->cnid_add_stmt = NULL;
    db->cnid_put_stmt = NULL;
    db->cnid_get_stmt = NULL;
    db->cnid_resolve_stmt = NULL;
    db->cnid_delete_stmt = NULL;
    db->cnid_getstamp_stmt = NULL;
}

static int cnid_sqlite_execute(sqlite3 *con, const char *sql)
{
    int rv;
    LOG(log_maxdebug, logtype_cnid, "SQL: %s", sql);
    rv = sqlite3_exec(con, sql, NULL, NULL, NULL);

    if (rv) {
        LOG(log_info, logtype_cnid,
            "sqlite query \"%s\", error: %s", sql, sqlite3_errmsg(con));
        errno = CNID_ERR_DB;
    }

    return rv;
}

int cnid_sqlite_delete(struct _cnid_db *cdb, const cnid_t id)
{
    EC_INIT;
    CNID_sqlite_private *db = cdb->cnid_db_private;
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_delete(id: %" PRIu32 "): BEGIN", ntohl(id));

    if (!cdb || !db || !id) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_delete: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "BEGIN"));
    sqlite3_reset(db->cnid_delete_stmt);
    sqlite3_clear_bindings(db->cnid_delete_stmt);
    sqlite3_bind_int64(db->cnid_delete_stmt, 1, ntohl(id));

    if (sqlite3_step(db->cnid_delete_stmt) == SQLITE_DONE) {
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "COMMIT"));
    } else {
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_delete: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        EC_FAIL;
    }

EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_delete(id: %" PRIu32 "): END", ntohl(id));

    if (cdb && db) {
        sqlite3_reset(db->cnid_delete_stmt);
    }

    EC_EXIT;
}

void cnid_sqlite_close(struct _cnid_db *cdb)
{
    CNID_sqlite_private *db = cdb->cnid_db_private;

    if (!cdb) {
        LOG(log_error, logtype_cnid,
            "cnid_close called with NULL argument !");
        return;
    }

    if (db != NULL) {
        LOG(log_debug, logtype_cnid,
            "closing database connection for volume '%s'",
            cdb->cnid_db_vol->v_localname);
        free(db->cnid_sqlite_voluuid_str);
        close_prepared_stmt(db);

        if (db->cnid_sqlite_con) {
            sqlite3_close(db->cnid_sqlite_con);
            sqlite3_shutdown();
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
    CNID_sqlite_private *db = cdb->cnid_db_private;
    cnid_t update_id = CNID_INVALID;
    uint64_t dev;
    uint64_t ino;
    char stmt_param_name[MAXPATHLEN];
    size_t stmt_param_name_len;
    uint64_t stmt_param_id;
    uint64_t stmt_param_did;
    uint64_t stmt_param_dev;
    uint64_t stmt_param_ino;
    char *sql = NULL;
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_update(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): BEGIN",
        ntohl(id), ntohl(did), name);

    if (!cdb || !db || !id || !st || !name) {
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
        } else if (!name) {
            LOG(log_error, logtype_cnid,
                "cnid_update: Parameter error: name is NULL");
        }

        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid,
            "cnid_update: Path name is too long");
        errno = CNID_ERR_PATH;
        EC_FAIL;
    }

    dev = st->st_dev;
    ino = st->st_ino;

    do {
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "BEGIN"));
        asprintf(&sql, "DELETE FROM \"%s\" WHERE Id=%" PRIu32,
                 db->cnid_sqlite_voluuid_str, ntohl(id));
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, sql));
        free(sql);
        sql = NULL;
        asprintf(&sql, "DELETE FROM \"%s\" WHERE Did = %" PRIu32 " AND Name = \"%s\"",
                 db->cnid_sqlite_voluuid_str, ntohl(did), name);
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, sql));
        free(sql);
        sql = NULL;
        asprintf(&sql, "DELETE FROM \"%s\" WHERE DevNo = %" PRIu64 " AND InodeNo = %"
                 PRIu64, db->cnid_sqlite_voluuid_str, dev, ino);
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, sql));
        free(sql);
        sql = NULL;
        stmt_param_id = ntohl(id);
        strlcpy(stmt_param_name, name, sizeof(stmt_param_name));
        stmt_param_name_len = len;
        stmt_param_did = ntohl(did);
        stmt_param_dev = dev;
        stmt_param_ino = ino;
        sqlite3_reset(db->cnid_put_stmt);
        sqlite3_clear_bindings(db->cnid_put_stmt);
        sqlite3_bind_int64(db->cnid_put_stmt, 1, stmt_param_id);
        sqlite3_bind_text(db->cnid_put_stmt, 2, stmt_param_name,
                          (int)stmt_param_name_len,
                          SQLITE_STATIC);
        sqlite3_bind_int64(db->cnid_put_stmt, 3, stmt_param_did);
        sqlite3_bind_int64(db->cnid_put_stmt, 4, stmt_param_dev);
        sqlite3_bind_int64(db->cnid_put_stmt, 5, stmt_param_ino);

        if (sqlite3_step(db->cnid_put_stmt) == SQLITE_DONE) {
            EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "COMMIT"));
        } else {
            EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_add: sqlite query error: %s",
                sqlite3_errmsg(db->cnid_sqlite_con));
            EC_FAIL;
        }

        update_id = (cnid_t) sqlite3_last_insert_rowid(db->cnid_sqlite_con);
    } while (update_id != ntohl(id));

EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_update(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): END",
        ntohl(id), ntohl(did), name);

    if (cdb && db) {
        sqlite3_reset(db->cnid_put_stmt);
    }

    EC_EXIT;
}

cnid_t cnid_sqlite_lookup(struct _cnid_db *cdb, const struct stat *st,
                          cnid_t did, const char *name, size_t len)
{
    EC_INIT;
    CNID_sqlite_private *db = cdb->cnid_db_private;
    cnid_t id = CNID_INVALID;
    uint64_t dev = st->st_dev;
    uint64_t ino = st->st_ino;
    cnid_t hint = db->cnid_sqlite_hint;
    const unsigned char *retname;
    cnid_t retid;
    uint64_t retdid;
    int sqlite_return;
    char stmt_param_name[MAXPATHLEN];
    size_t stmt_param_name_len;
    uint64_t stmt_param_did;
    uint64_t stmt_param_dev;
    uint64_t stmt_param_ino;
    char lookup_result_name[MAXPATHLEN];
    uint64_t lookup_result_id;
    uint64_t lookup_result_did;
    uint64_t lookup_result_dev;
    uint64_t lookup_result_ino;
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_lookup(did: %" PRIu32 ", name: \"%s\", hint: %" PRIu32 "): BEGIN",
        ntohl(did), name, ntohl(hint));

    if (!cdb || !db || !st || !name) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: Path name is too long");
        errno = CNID_ERR_PATH;
        EC_FAIL;
    }

    strlcpy(stmt_param_name, name, sizeof(stmt_param_name));
    stmt_param_name_len = len;
    stmt_param_did = ntohl(did);
    stmt_param_dev = dev;
    stmt_param_ino = ino;
    sqlite3_reset(db->cnid_lookup_stmt);
    sqlite3_clear_bindings(db->cnid_lookup_stmt);
    sqlite3_bind_text(db->cnid_lookup_stmt, 1, stmt_param_name,
                      (int)stmt_param_name_len, SQLITE_STATIC);
    sqlite3_bind_int64(db->cnid_lookup_stmt, 2, stmt_param_did);
    sqlite3_bind_int64(db->cnid_lookup_stmt, 3, stmt_param_dev);
    sqlite3_bind_int64(db->cnid_lookup_stmt, 4, stmt_param_ino);
    sqlite_return = sqlite3_step(db->cnid_lookup_stmt);

    if (sqlite_return == SQLITE_DONE) {
        /* Not found (no rows) */
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup: name: '%s', did: %u is not in the CNID database",
            name, ntohl(did));
        errno = CNID_DBD_RES_NOTFOUND;
        EC_FAIL;
    } else if (sqlite_return != SQLITE_ROW) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: sqlite query error (1): %s - %i",
            sqlite3_errmsg(db->cnid_sqlite_con), sqlite_return);
        errno = CNID_ERR_DB;
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
    /* Check for additional rows */
    int row_count = 1;
    int delete_error = 0;

    while ((sqlite_return = sqlite3_step(db->cnid_lookup_stmt)) == SQLITE_ROW) {
        uint64_t extra_id = sqlite3_column_int64(db->cnid_lookup_stmt, 0);
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup: multiple matches for did: %u, name: '%s', deleting extra id: %"
            PRIu64,
            ntohl(did), name, extra_id);

        if (cnid_sqlite_delete(cdb, htonl((cnid_t)extra_id))) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_lookup: sqlite query error (delete extra): %s",
                sqlite3_errmsg(db->cnid_sqlite_con));
            delete_error = 1;
        }

        row_count++;
    }

    if (sqlite_return != SQLITE_DONE) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: sqlite query error (step): %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        errno = CNID_ERR_DB;
        EC_FAIL;
    }

    if (row_count > 1) {
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup: deleted %d duplicate rows for did: %u, name: '%s'",
            row_count - 1, ntohl(did), name);

        if (delete_error) {
            errno = CNID_ERR_DB;
            EC_FAIL;
        }

        errno = CNID_DBD_RES_NOTFOUND;
        EC_FAIL;
    }

    if (lookup_result_id == 0) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: Invalid/corrupt row: id=0, did=%" PRIu64
            ", name='%s', dev=%" PRIu64 ", ino=%" PRIu64,
            lookup_result_did, lookup_result_name,
            lookup_result_dev, lookup_result_ino);
        errno = CNID_INVALID;
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
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_lookup: sqlite query error (hint delete): %s",
                    sqlite3_errmsg(db->cnid_sqlite_con));
                errno = CNID_ERR_DB;
                EC_FAIL;
            }

            errno = CNID_DBD_RES_NOTFOUND;
            EC_FAIL;
        }

        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup: server side mv, got hint, updating");

        if (cnid_sqlite_update(cdb, retid, st, did, name, len) != 0) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_lookup: sqlite query error (update): %s",
                sqlite3_errmsg(db->cnid_sqlite_con));
            errno = CNID_ERR_DB;
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
            errno = CNID_ERR_DB;
            EC_FAIL;
        }

        errno = CNID_DBD_RES_NOTFOUND;
        EC_FAIL;
    } else {
        /* everything is good */
        id = retid;
    }

EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_lookup(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): END",
        ntohl(id), ntohl(did), name);

    if (cdb && db) {
        sqlite3_reset(db->cnid_lookup_stmt);
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
    CNID_sqlite_private *db = cdb->cnid_db_private;
    cnid_t id = CNID_INVALID;
    uint64_t lastid;
    uint64_t dev;
    uint64_t ino;
    int sqlite_return;
    int retries;
    int was_depleted = 0;
    struct timespec ts = {0, 10000000};
    char stmt_param_name[MAXPATHLEN];
    size_t stmt_param_name_len;
    uint64_t stmt_param_id;
    uint64_t stmt_param_did;
    uint64_t stmt_param_dev;
    uint64_t stmt_param_ino;
    char *sql = NULL;
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_add(did: %" PRIu32 ", name: \"%s\", hint: %" PRIu32 "): BEGIN",
        ntohl(did), name, ntohl(hint));

    if (!cdb || !db || !st || !name) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_add: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_add: Path name is too long");
        errno = CNID_ERR_PATH;
        EC_FAIL;
    }

    dev = st->st_dev;
    ino = st->st_ino;
    db->cnid_sqlite_hint = hint;

    do {
        id = cnid_sqlite_lookup(cdb, st, did, name, len);
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_add: lookup returned id=%" PRIu32 ", errno=%d", ntohl(id), errno);

        if (id == CNID_INVALID) {
            if (errno == CNID_ERR_DB) {
                LOG(log_error, logtype_cnid, "cnid_sqlite_add: Invalid CNID returned!");
                EC_FAIL;
            }

            strlcpy(stmt_param_name, name, sizeof(stmt_param_name));
            stmt_param_name_len = len;
            stmt_param_did = ntohl(did);
            stmt_param_dev = dev;
            stmt_param_ino = ino;
            LOG(log_debug, logtype_cnid,
                "cnid_sqlite_add: binding name='%s', did=%" PRIu64 ", dev=%" PRIu64 ", ino=%"
                PRIu64,
                stmt_param_name, stmt_param_did, stmt_param_dev, stmt_param_ino);

            if (stmt_param_name[0] == '\0' ||
                    stmt_param_did == 0 ||
                    stmt_param_dev == 0 ||
                    stmt_param_ino == 0) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_add: Refusing to insert invalid/empty entry");
                errno = CNID_ERR_PARAM;
                EC_FAIL;
            }

            EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "BEGIN"));

            /*
             * If the CNID set has previously overflowed
             * (CNID_SQLITE_FLAG_DEPLETED flag) ignore the CNID "hint"
             * read from AppleDouble or Extended Attributes.
             */
            if (!db->cnid_sqlite_hint
                    || (db->cnid_sqlite_flags & CNID_SQLITE_FLAG_DEPLETED)) {
                LOG(log_debug, logtype_cnid,
                    "cnid_sqlite_add: not using CNID hint, CNID set is depleted or hint not set");
                was_depleted = 1;
                sqlite3_reset(db->cnid_add_stmt);
                sqlite3_clear_bindings(db->cnid_put_stmt);
                sqlite3_bind_text(db->cnid_add_stmt, 1, stmt_param_name,
                                  (int)stmt_param_name_len,
                                  SQLITE_STATIC);
                sqlite3_bind_int64(db->cnid_add_stmt, 2, stmt_param_did);
                sqlite3_bind_int64(db->cnid_add_stmt, 3, stmt_param_dev);
                sqlite3_bind_int64(db->cnid_add_stmt, 4, stmt_param_ino);
                retries = 0;

                do {
                    sqlite_return = sqlite3_step(db->cnid_add_stmt);

                    if (sqlite_return == SQLITE_BUSY || sqlite_return == SQLITE_LOCKED) {
                        LOG(log_debug, logtype_cnid,
                            "cnid_sqlite_add: SQLITE_BUSY or SQLITE_LOCKED, retrying cnid_add_stmt (%d)",
                            retries);
                        nanosleep(&ts, NULL);
                        retries++;
                    }
                } while ((sqlite_return == SQLITE_BUSY || sqlite_return == SQLITE_LOCKED)
                         && retries < 10);
            } else {
                stmt_param_id = ntohl(db->cnid_sqlite_hint);
                sqlite3_reset(db->cnid_put_stmt);
                sqlite3_clear_bindings(db->cnid_put_stmt);
                sqlite3_bind_int64(db->cnid_put_stmt, 1, stmt_param_id);
                sqlite3_bind_text(db->cnid_put_stmt, 2, stmt_param_name,
                                  (int)stmt_param_name_len,
                                  SQLITE_STATIC);
                sqlite3_bind_int64(db->cnid_put_stmt, 3, stmt_param_did);
                sqlite3_bind_int64(db->cnid_put_stmt, 4, stmt_param_dev);
                sqlite3_bind_int64(db->cnid_put_stmt, 5, stmt_param_ino);
                retries = 0;

                do {
                    sqlite_return = sqlite3_step(db->cnid_put_stmt);

                    if (sqlite_return == SQLITE_BUSY || sqlite_return == SQLITE_LOCKED) {
                        LOG(log_debug, logtype_cnid,
                            "cnid_sqlite_add: SQLITE_BUSY or SQLITE_LOCKED, retrying cnid_put_stmt (%d)",
                            retries);
                        nanosleep(&ts, NULL);
                        retries++;
                    }
                } while ((sqlite_return == SQLITE_BUSY || sqlite_return == SQLITE_LOCKED)
                         && retries < 10);
            }

            if (sqlite_return == SQLITE_DONE) {
                EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "COMMIT"));
            } else {
                EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));

                if (sqlite_return == SQLITE_CONSTRAINT) {
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
                    EC_FAIL;
                }
            }

            lastid = sqlite3_last_insert_rowid(db->cnid_sqlite_con);

            if ((uint32_t) lastid > UINT32_MAX) {
                /* CNID set is depleted, restart from scratch */
                LOG(log_info, logtype_cnid,
                    "cnid_sqlite_add: CNID set is depleted, resetting ID sequence");
                EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "BEGIN"));
                asprintf(&sql, "UPDATE volumes SET Depleted = 1 WHERE VolUUID = \"%s\"",
                         db->cnid_sqlite_voluuid_str);

                if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
                    EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
                    EC_FAIL;
                }

                free(sql);
                sql = NULL;
                asprintf(&sql, "DELETE FROM \"%s\"", db->cnid_sqlite_voluuid_str);

                if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
                    EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
                    EC_FAIL;
                }

                free(sql);
                sql = NULL;
                asprintf(&sql, "UPDATE sqlite_sequence SET seq = 16 WHERE name = \"%s\";",
                         db->cnid_sqlite_voluuid_str);

                if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
                    EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
                    EC_FAIL;
                }

                free(sql);
                sql = NULL;
                asprintf(&sql, "INSERT INTO sqlite_sequence (name,seq) SELECT \"%s\", "
                               "16 WHERE NOT EXISTS "
                               "(SELECT changes() AS change "
                               "FROM sqlite_sequence WHERE change <> 0);",
                         db->cnid_sqlite_voluuid_str);

                if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
                    EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
                    EC_FAIL;
                }

                free(sql);
                sql = NULL;
                EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "COMMIT"));
                db->cnid_sqlite_flags |= CNID_SQLITE_FLAG_DEPLETED;
            }

            /* Finally assign our result */
            id = htonl((uint32_t) lastid);

            if (id == 0) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_add: Invalid CNID 0 returned after insert!");
                errno = CNID_INVALID;
                EC_FAIL;
            }
        }
    } while (id == CNID_INVALID);

EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_add(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\", hint: %"
        PRIu32 "): END",
        ntohl(id), ntohl(did), name, ntohl(hint));

    if (cdb && db) {
        if (was_depleted) {
            sqlite3_reset(db->cnid_add_stmt);
        } else {
            sqlite3_reset(db->cnid_put_stmt);
        }
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
    CNID_sqlite_private *db = cdb->cnid_db_private;
    cnid_t id = CNID_INVALID;
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_get(did: %" PRIu32 ", name: \"%s\"): BEGIN",
        ntohl(did), name);

    if (!cdb || !db || !name) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_get: Parameter error");
        errno = CNID_ERR_PARAM;
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
    EC_ZERO_LOG(sqlite3_bind_text(db->cnid_get_stmt, 1, name, len,
                                  SQLITE_STATIC));
    EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_get_stmt, 2, ntohl(did)));

    if ((sqlite3_step(db->cnid_get_stmt) != SQLITE_ROW)
            && (sqlite3_errcode(db->cnid_sqlite_con) != 0)) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_get: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        EC_FAIL;
    }

    id = htonl((uint32_t)sqlite3_column_int64(db->cnid_get_stmt, 0));
EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_get(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): END",
        ntohl(id), ntohl(did), name);

    if (cdb && db) {
        sqlite3_reset(db->cnid_get_stmt);
    }

    return id;
}

char *cnid_sqlite_resolve(struct _cnid_db *cdb, cnid_t * id, void *buffer,
                          size_t len)
{
    EC_INIT;
    CNID_sqlite_private *db = cdb->cnid_db_private;
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_resolve(id: %" PRIu32 "): BEGIN",
        ntohl(*id));

    if (!cdb || !db) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_resolve: Parameter error");
        errno = CNID_ERR_PARAM;
        *id = CNID_INVALID;
        EC_FAIL;
    }

    sqlite3_reset(db->cnid_resolve_stmt);
    sqlite3_clear_bindings(db->cnid_resolve_stmt);
    sqlite3_bind_int64(db->cnid_resolve_stmt, 1, ntohl(*id));
    EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_resolve_stmt, 1, ntohl(*id)));

    if (sqlite3_step(db->cnid_resolve_stmt) != SQLITE_ROW) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_resolve: No result found for id: %" PRIu32, ntohl(*id));
        *id = CNID_INVALID;
        EC_FAIL;
    }

    *id = htonl((uint32_t)sqlite3_column_int64(db->cnid_resolve_stmt, 0));
    strlcpy(buffer, (const char *)sqlite3_column_text(db->cnid_resolve_stmt, 1),
            len);
    ((char *)buffer)[len - 1] = '\0';
    LOG(log_debug, logtype_cnid,
        "cnid_sqlite_resolve: resolved did: %u, name: \"%s\"",
        ntohl(*id), buffer);
EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_resolve(): END");

    if (cdb && db) {
        sqlite3_reset(db->cnid_resolve_stmt);
    }

    if (ret != 0) {
        *id = CNID_INVALID;
        return NULL;
    }

    return buffer;
}

/**
 * Caller passes buffer where we will store the db stamp
 **/
int cnid_sqlite_getstamp(struct _cnid_db *cdb, void *buffer,
                         const size_t len)
{
    EC_INIT;
    CNID_sqlite_private *db = cdb->cnid_db_private;
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_getstamp(): BEGIN");

    if (!cdb || !db) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_getstamp: Parameter error");
        errno = CNID_ERR_PARAM;
        return CNID_INVALID;
    }

    if (!buffer) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_getstamp: bad buffer");
        return CNID_INVALID;
    }

    sqlite3_reset(db->cnid_getstamp_stmt);
    sqlite3_clear_bindings(db->cnid_getstamp_stmt);
    EC_ZERO_LOG(sqlite3_bind_text(db->cnid_getstamp_stmt, 1,
                                  cdb->cnid_db_vol->v_path, strlen(cdb->cnid_db_vol->v_path),
                                  SQLITE_STATIC));

    if ((sqlite3_step(db->cnid_getstamp_stmt) != SQLITE_ROW)
            || (sqlite3_column_text(db->cnid_getstamp_stmt, 0) == NULL)) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_getstamp: Can't get DB stamp for volume \"%s\"",
            cdb->cnid_db_vol->v_path);
        EC_FAIL;
    }

    LOG(log_debug, logtype_cnid,
        "cnid_sqlite_getstamp: Got DB stamp for volume \"%s\"",
        cdb->cnid_db_vol->v_path);
    strlcpy(buffer, (const char *)sqlite3_column_text(db->cnid_getstamp_stmt, 0),
            len);
EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_getstamp(): END");

    if (cdb && db) {
        sqlite3_reset(db->cnid_getstamp_stmt);
    }

    EC_EXIT;
}


int cnid_sqlite_find(struct _cnid_db *cdb _U_, const char *name,
                     size_t namelen _U_,
                     void *buffer _U_, size_t buflen _U_)
{
    LOG(log_error, logtype_cnid,
        "cnid_sqlite_find(\"%s\"): not supported with sqlite CNID backend",
        name);
    return -1;
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
    CNID_sqlite_private *db = cdb->cnid_db_private;
    char *sql = NULL;
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_wipe(\"%s\"): BEGIN",
        cdb->cnid_db_vol->v_localname);

    if (!cdb || !db) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_wipe: Parameter error");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "BEGIN"));
    asprintf(&sql, "UPDATE volumes SET Depleted = 0 WHERE VolUUID = \"%s\";",
             db->cnid_sqlite_voluuid_str);

    if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
        EC_FAIL;
    }

    free(sql);
    sql = NULL;
    asprintf(&sql, "DELETE FROM \"%s\";", db->cnid_sqlite_voluuid_str);

    if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
        EC_FAIL;
    }

    free(sql);
    sql = NULL;
    asprintf(&sql, "UPDATE sqlite_sequence SET seq = 16 WHERE name = \"%s\";",
             db->cnid_sqlite_voluuid_str);

    if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
        EC_FAIL;
    }

    free(sql);
    sql = NULL;
    asprintf(&sql,
             "INSERT INTO sqlite_sequence (name,seq) SELECT \"%s\", 16 WHERE NOT EXISTS (SELECT changes() AS change FROM sqlite_sequence WHERE change <> 0);",
             db->cnid_sqlite_voluuid_str);

    if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
        EC_FAIL;
    }

    free(sql);
    sql = NULL;
    EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "COMMIT"));
    EC_ZERO(init_prepared_stmt(db));
EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_wipe(\"%s\"): END",
        cdb->cnid_db_vol->v_localname);
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

/* Return allocated UUID string with dashes stripped */
static char *uuid_strip_dashes(const char *uuid)
{
    static char stripped[33];
    int i = 0;

    while (*uuid && i < 32) {
        if (*uuid != '-') {
            stripped[i++] = *uuid;
        }

        uuid++;
    }

    stripped[i] = '\0';
    return strdup(stripped);
}

/* ---------------------- */
struct _cnid_db *cnid_sqlite_open(struct cnid_open_args *args)
{
    EC_INIT;
    CNID_sqlite_private *db = NULL;
    struct _cnid_db *cdb = NULL;
    char *sql = NULL;
    struct vol *vol = args->cnid_args_vol;
    sqlite3_stmt *transient_stmt = NULL;
    char dirpath[PATH_MAX];
    bstring dbpath = NULL;
    const char *path_data = NULL;
    int sqlite_return;
    EC_NULL(cdb = cnid_sqlite_new(vol));
    EC_NULL(db =
                (CNID_sqlite_private *) calloc(1,
                    sizeof(CNID_sqlite_private)));
    cdb->cnid_db_private = db;
    snprintf(dirpath, sizeof(dirpath), "%sCNID/%s", _PATH_STATEDIR,
             vol->v_localname);
    become_root();

    if (mkdir(dirpath, 0755) != 0) {
        if (errno == EEXIST) {
            struct stat st;

            if (stat(dirpath, &st) != 0 || !S_ISDIR(st.st_mode)) {
                LOG(log_error, logtype_cnid, "'%s' exists but is not a directory", dirpath);
                EC_FAIL;
            }
        } else {
            LOG(log_error, logtype_cnid, "Failed to create CNID DB directory '%s': %s",
                dirpath, strerror(errno));
            EC_FAIL;
        }
    }

    unbecome_root();
    EC_NULL(dbpath = bformat("%s/%s.sqlite", dirpath, vol->v_localname));
    path_data = bdata(dbpath);
    EC_NULL(db->cnid_sqlite_voluuid_str = uuid_strip_dashes(vol->v_uuid));
    EC_ZERO(sqlite3_initialize());
    become_root();

    if (sqlite3_open_v2(path_data,
                        &db->cnid_sqlite_con,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL)) {
        LOG(log_error, logtype_cnid, "sqlite open error: %s, path: %s",
            sqlite3_errmsg(db->cnid_sqlite_con),
            path_data);
        EC_FAIL;
    }

    /* creating the db file as world-writable
        to permit CNID records to be updated by any authenticated AFP user */
    if (path_data && chmod(path_data, 0666) != 0) {
        LOG(log_error, logtype_cnid, "Failed to set permissions on CNID SQLite DB: %s",
            strerror(errno));
    }

    sqlite3_busy_timeout(db->cnid_sqlite_con, 5000);
    EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "PRAGMA journal_mode=WAL;"));

    /* Add volume to volume table */
    if (cnid_sqlite_execute(db->cnid_sqlite_con,
                            "CREATE TABLE IF NOT EXISTS volumes "
                            "(VolUUID CHAR(32) PRIMARY KEY, "
                            "VolPath TEXT(4096), "
                            "Stamp BINARY(8), "
                            "Depleted INT"
                            ")") < 0) {
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
        EC_FAIL;
    }

    /* Add index to volume */
    if (cnid_sqlite_execute(db->cnid_sqlite_con,
                            "CREATE INDEX IF NOT EXISTS idx_volpath "
                            "ON volumes(VolPath)") < 0) {
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
        EC_FAIL;
    }

    unbecome_root();
    /* Create a blob.  The MySQL code used an escape string function,
           but we're going to use a prepared statement */
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
                (db->cnid_sqlite_con, sql, strlen(sql), &transient_stmt, NULL));
    EC_ZERO_LOG(sqlite3_bind_text(transient_stmt,
                                  1, db->cnid_sqlite_voluuid_str,
                                  strlen(db->cnid_sqlite_voluuid_str), SQLITE_STATIC));
    EC_ZERO_LOG(sqlite3_bind_text(transient_stmt,
                                  2, vol->v_path,
                                  strlen(vol->v_path), SQLITE_STATIC));
    EC_ZERO_LOG(sqlite3_bind_text(transient_stmt,
                                  3, stamp,
                                  strlen(stamp), SQLITE_STATIC));

    if (sqlite3_step(transient_stmt) != SQLITE_DONE) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_open: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        EC_FAIL;
    }

    sqlite3_reset(transient_stmt);
    sqlite3_clear_bindings(transient_stmt);

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
    asprintf(&sql, "CREATE TABLE IF NOT EXISTS \"%s\" ("
                   "Id INTEGER PRIMARY KEY AUTOINCREMENT,"
                   "Name VARCHAR(255) NOT NULL,"
                   "Did INTEGER NOT NULL,"
                   "DevNo INTEGER NOT NULL,"
                   "InodeNo INTEGER NOT NULL,"
                   "UNIQUE (Did, Name),"
                   "UNIQUE (DevNo, InodeNo)"
                   ");",
             db->cnid_sqlite_voluuid_str);

    if (cnid_sqlite_execute(db->cnid_sqlite_con, sql)) {
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
        EC_FAIL;
    }

    free(sql);
    sql = NULL;
    /* Check if the table was just created (not just exists) */
    int table_row_count = -1;
    char *check_sql = NULL;
    sqlite3_stmt *check_table_stmt = NULL;

    if (asprintf(&check_sql, "SELECT count(*) FROM \"%s\";",
                 db->cnid_sqlite_voluuid_str) != -1) {
        if (sqlite3_prepare_v2(db->cnid_sqlite_con, check_sql, -1, &check_table_stmt,
                               NULL) == SQLITE_OK) {
            if (sqlite3_step(check_table_stmt) == SQLITE_ROW) {
                table_row_count = sqlite3_column_int(check_table_stmt, 0);
            }

            sqlite3_finalize(check_table_stmt);
        }

        free(check_sql);
    }

    if (table_row_count == 0) {
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
        asprintf(&sql, "UPDATE sqlite_sequence SET seq = 16 WHERE name = \"%s\";",
                 db->cnid_sqlite_voluuid_str);

        if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
            EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
            EC_FAIL;
        }

        free(sql);
        sql = NULL;
        asprintf(&sql, "INSERT INTO sqlite_sequence (name,seq) SELECT \"%s\","
                       "16 WHERE NOT EXISTS "
                       "(SELECT changes() AS change "
                       "FROM sqlite_sequence WHERE change <> 0);",
                 db->cnid_sqlite_voluuid_str);

        if (cnid_sqlite_execute(db->cnid_sqlite_con, sql) < 0) {
            EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con, "ROLLBACK"));
            EC_FAIL;
        }

        free(sql);
        sql = NULL;
    }

    EC_ZERO(init_prepared_stmt(db));
    LOG(log_debug, logtype_cnid,
        "Finished initializing sqlite CNID module for volume '%s'",
        vol->v_path);
EC_CLEANUP:

    if (transient_stmt) {
        sqlite3_finalize(transient_stmt);
    }

    if (ret != 0) {
        if (cdb) {
            free(cdb);
        }

        cdb = NULL;

        if (db) {
            free(db);
        }
    }

    if (sql) {
        free(sql);
    }

    return cdb;
}

struct _cnid_module cnid_sqlite_module = {
    "sqlite",
    { NULL, NULL },
    cnid_sqlite_open,
    0
};
