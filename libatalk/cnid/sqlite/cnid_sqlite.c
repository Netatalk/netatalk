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

#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <inttypes.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <stdio.h>
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
#include <atalk/util.h>
#include <atalk/volume.h>

static int init_prepared_stmt_lookup(CNID_sqlite_private * db)
{
    EC_INIT;
    char *sql = NULL;

    if (db->cnid_lookup_stmt) {
        LOG(log_debug, logtype_cnid,
            "init_prepared_stmt_lookup: Finalizing existing lookup statement");
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
        LOG(log_debug, logtype_cnid,
            "init_prepared_stmt_add: Finalizing existing add statement");
        EC_ZERO(sqlite3_finalize(db->cnid_add_stmt));
    }

    EC_NEG1(asprintf(&sql,
                     "INSERT INTO \"%s\" (Name,Did,DevNo,InodeNo) VALUES(?,?,?,?)",
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
        LOG(log_debug, logtype_cnid,
            "init_prepared_stmt_put: Finalizing existing put statement");
        EC_ZERO(sqlite3_finalize(db->cnid_put_stmt));
    }

    EC_NEG1(asprintf(&sql,
                     "INSERT INTO \"%s\" (Id,Name,Did,DevNo,InodeNo) VALUES(?,?,?,?,?)",
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

static int init_prepared_stmt(CNID_sqlite_private * db)
{
    EC_INIT;
    EC_ZERO(init_prepared_stmt_lookup(db));
    EC_ZERO(init_prepared_stmt_add(db));
    EC_ZERO(init_prepared_stmt_put(db));
EC_CLEANUP:
    EC_EXIT;
}

static void close_prepared_stmt(CNID_sqlite_private * db)
{
    sqlite3_finalize(db->cnid_lookup_stmt);
    sqlite3_finalize(db->cnid_add_stmt);
    sqlite3_finalize(db->cnid_put_stmt);
}

static int cnid_sqlite_execute(sqlite3 * con, char *fmt, ...)
{
    char *sql = NULL;
    va_list ap;
    int rv;
    va_start(ap, fmt);

    if (vasprintf(&sql, fmt, ap) == -1) {
        return -1;
    }

    va_end(ap);
    LOG(log_maxdebug, logtype_cnid, "SQL: %s", sql);
    rv = sqlite3_exec(con, sql, NULL, NULL, NULL);

    if (rv) {
        LOG(log_info, logtype_cnid,
            "sqlite query \"%s\", error: %s", sql, sqlite3_errmsg(con));
        errno = CNID_ERR_DB;
    }

    free(sql);
    return rv;
}

int cnid_sqlite_delete(struct _cnid_db *cdb, const cnid_t id)
{
    EC_INIT;
    CNID_sqlite_private *db;
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_delete(id: %" PRIu32 "): BEGIN", ntohl(id));

    if (!cdb || !(db = cdb->cnid_db_private) || !id) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_delete: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con,
                                "DELETE FROM \"%s\" WHERE Id = %" PRIu32,
                                db->cnid_sqlite_voluuid_str,
                                ntohl(id)));
EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_delete(id: %" PRIu32 "): END", ntohl(id));
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

    if ((db = cdb->cnid_db_private) != NULL) {
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
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_update(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): BEGIN",
        ntohl(id), ntohl(did), name);

    if (!cdb || !db || !id || !st || !name) {
        if (!cdb) {
            LOG(log_error, logtype_cnid,
                "cnid_update: Parameter error: cdb is NULL");
        } else if (!db) {
            LOG(log_error, logtype_cnid,
                "cnid_update: Parameter error: (db = cdb->cnid_db_private) is NULL");
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
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con,
                                    "DELETE FROM \"%s\" WHERE Id=%"
                                    PRIu32,
                                    db->cnid_sqlite_voluuid_str,
                                    ntohl(id)));
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con,
                                    "DELETE FROM \"%s\" WHERE Did = %" PRIu32
                                    " AND Name = \"%s\"", db->cnid_sqlite_voluuid_str,
                                    ntohl(did), name));
        EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con,
                                    "DELETE FROM \"%s\" WHERE DevNo = %" PRIu64
                                    " AND InodeNo = %" PRIu64,
                                    db->cnid_sqlite_voluuid_str, dev, ino));
        stmt_param_id = ntohl(id);
        strlcpy(stmt_param_name, name, sizeof(stmt_param_name));
        stmt_param_name_len = len;
        stmt_param_did = ntohl(did);
        stmt_param_dev = dev;
        stmt_param_ino = ino;
        sqlite3_reset(db->cnid_put_stmt);
        sqlite3_bind_int64(db->cnid_put_stmt, 1, stmt_param_id);
        sqlite3_bind_text(db->cnid_put_stmt, 2, stmt_param_name,
                          (int)stmt_param_name_len,
                          SQLITE_STATIC);
        sqlite3_bind_int64(db->cnid_put_stmt, 3, stmt_param_did);
        sqlite3_bind_int64(db->cnid_put_stmt, 4, stmt_param_dev);
        sqlite3_bind_int64(db->cnid_put_stmt, 5, stmt_param_ino);

        if (sqlite3_step(db->cnid_put_stmt) != SQLITE_DONE) {
            switch (sqlite3_errcode(db->cnid_sqlite_con)) {
            default:
                EC_FAIL;
            }
        }

        update_id = (cnid_t) sqlite3_last_insert_rowid(db->cnid_sqlite_con);
    } while (update_id != ntohl(id));

EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_update(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): END",
        ntohl(id), ntohl(did), name);
    EC_EXIT;
}

cnid_t cnid_sqlite_lookup(struct _cnid_db *cdb,
                          const struct stat *st,
                          cnid_t did, const char *name, size_t len)
{
    EC_INIT;
    CNID_sqlite_private *db;
    cnid_t id = CNID_INVALID;
    uint64_t dev = st->st_dev;
    uint64_t ino = st->st_ino;
    cnid_t retid;
    uint64_t retdid;
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
        "cnid_sqlite_lookup(did: %" PRIu32 ", name: \"%s\"): BEGIN",
        ntohl(did), name);

    if (!cdb || !(db = cdb->cnid_db_private) || !st || !name) {
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
    const char *retname;
    const unsigned char *col_text;
    int sqlite_return;
    sqlite3_reset(db->cnid_lookup_stmt);
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
        errno = CNID_INVALID;
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
    col_text = sqlite3_column_text(db->cnid_lookup_stmt, 2);

    if (col_text) {
        strlcpy(lookup_result_name, (const char *)col_text, MAXPATHLEN);
    } else {
        lookup_result_name[0] = '\0';
    }

    lookup_result_dev = sqlite3_column_int64(db->cnid_lookup_stmt, 3);
    lookup_result_ino = sqlite3_column_int64(db->cnid_lookup_stmt, 4);
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_lookup: id=%i, did=%i, name=%s, dev=%i, ino=%i",
        htonl(lookup_result_id), htonl(lookup_result_did), lookup_result_name,
        lookup_result_dev, lookup_result_ino);

    if (lookup_result_id < 17 ||
            lookup_result_did == 0 ||
            lookup_result_name[0] == '\0' ||
            lookup_result_dev == 0 ||
            lookup_result_ino == 0) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: Invalid/corrupt row: id=%" PRIu64 ", did=%" PRIu64
            ", name='%s', dev=%" PRIu64 ", ino=%" PRIu64,
            lookup_result_id, lookup_result_did, lookup_result_name,
            lookup_result_dev, lookup_result_ino);
        errno = CNID_INVALID;
        EC_FAIL;
    }

    /* Check for additional rows */
    sqlite_return = sqlite3_step(db->cnid_lookup_stmt);

    if (sqlite_return == SQLITE_ROW) {
        /* We have more than one row */
        do {
            uint64_t extra_id = sqlite3_column_int64(db->cnid_lookup_stmt, 0);

            if (cnid_sqlite_delete(cdb, htonl((cnid_t)extra_id))) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_lookup: sqlite query error (2): %s",
                    sqlite3_errmsg(db->cnid_sqlite_con));
                errno = CNID_ERR_DB;
                EC_FAIL;
            }

            sqlite_return = sqlite3_step(db->cnid_lookup_stmt);
        } while (sqlite_return == SQLITE_ROW);

        if (sqlite_return != SQLITE_DONE) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_lookup: sqlite query error (3): %s",
                sqlite3_errmsg(db->cnid_sqlite_con));
            errno = CNID_ERR_DB;
            EC_FAIL;
        }

        errno = CNID_INVALID;
        EC_FAIL;
    }

    retid = htonl(lookup_result_id);
    retdid = htonl(lookup_result_did);

    if (retid == 0) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_lookup: Invalid CNID 0 returned! This should never happen.");
        errno = CNID_INVALID;
        EC_FAIL;
    }

    if (retdid != did || STRCMP(lookup_result_name, !=, name)) {
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup(CNID %" PRIu32 ", DID: %" PRIu32
            ", name: \"%s\"): server side mv oder reused inode",
            ntohl(did), name);
        LOG(log_debug, logtype_cnid,
            "cnid_sqlite_lookup: server side mv, got hint, updating");

        if (lookup_result_name[0] == '\0') {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_lookup: Refusing to update with empty name");
            errno = CNID_ERR_PARAM;
            EC_FAIL;
        }

        if (cnid_sqlite_update(cdb, retid, st, did, name, len) != 0) {
            LOG(log_error, logtype_cnid,
                "cnid_sqlite_lookup: sqlite query error (4): %s",
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
                "cnid_sqlite_lookup: sqlite query error (5): %s",
                sqlite3_errmsg(db->cnid_sqlite_con));
            errno = CNID_ERR_DB;
            EC_FAIL;
        }

        errno = CNID_INVALID;
        EC_FAIL;
    } else {
        /* everything is good */
        id = retid;
    }

EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_lookup(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): END",
        ntohl(id), ntohl(did), name);

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
    CNID_sqlite_private *db;
    cnid_t id = CNID_INVALID;
    uint64_t lastid;
    uint64_t dev;
    uint64_t ino;
    int sqlite_return;
    char stmt_param_name[MAXPATHLEN];
    size_t stmt_param_name_len;
    uint64_t stmt_param_id;
    uint64_t stmt_param_did;
    uint64_t stmt_param_dev;
    uint64_t stmt_param_ino;
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_add(did: %" PRIu32 ", name: \"%s\", hint: %" PRIu32 "): BEGIN",
        ntohl(did), name, ntohl(hint));

    if (!cdb || !(db = cdb->cnid_db_private) || !st || !name) {
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

            if (stmt_param_name[0] == '\0' ||
                    stmt_param_did == 0 ||
                    stmt_param_dev == 0 ||
                    stmt_param_ino == 0) {
                LOG(log_error, logtype_cnid,
                    "cnid_sqlite_add: Refusing to insert invalid/empty entry: name='%s', did=%"
                    PRIu64 ", dev=%" PRIu64 ", ino=%" PRIu64,
                    stmt_param_name, stmt_param_did, stmt_param_dev, stmt_param_ino);
                errno = CNID_ERR_PARAM;
                EC_FAIL;
            }

            /*
             * If the CNID set overflowed before
             * (CNID_SQLITE_FLAG_DEPLETED) ignore the CNID "hint"
             * taken from the AppleDouble file
             */
            if (!db->cnid_sqlite_hint
                    || (db->cnid_sqlite_flags & CNID_SQLITE_FLAG_DEPLETED)) {
                LOG(log_debug, logtype_cnid,
                    "cnid_sqlite_add: CNID set is depleted, ignoring hint");
                sqlite3_reset(db->cnid_add_stmt);
                sqlite3_bind_text(db->cnid_add_stmt, 1, stmt_param_name,
                                  (int)stmt_param_name_len,
                                  SQLITE_STATIC);
                sqlite3_bind_int64(db->cnid_add_stmt, 2, stmt_param_did);
                sqlite3_bind_int64(db->cnid_add_stmt, 3, stmt_param_dev);
                sqlite3_bind_int64(db->cnid_add_stmt, 4, stmt_param_ino);
                sqlite_return = sqlite3_step(db->cnid_add_stmt);
            } else {
                LOG(log_debug, logtype_cnid,
                    "cnid_sqlite_add: CNID set is not depleted, using hint: %" PRIu32,
                    ntohl(db->cnid_sqlite_hint));
                stmt_param_id = ntohl(db->cnid_sqlite_hint);
                sqlite3_reset(db->cnid_put_stmt);
                sqlite3_bind_int64(db->cnid_put_stmt, 1, stmt_param_id);
                sqlite3_bind_text(db->cnid_put_stmt, 2, stmt_param_name,
                                  (int)stmt_param_name_len,
                                  SQLITE_STATIC);
                sqlite3_bind_int64(db->cnid_put_stmt, 3, stmt_param_did);
                sqlite3_bind_int64(db->cnid_put_stmt, 4, stmt_param_dev);
                sqlite3_bind_int64(db->cnid_put_stmt, 5, stmt_param_ino);
                sqlite_return = sqlite3_step(db->cnid_put_stmt);
            }

            LOG(log_debug, logtype_cnid,
                "cnid_sqlite_add: binding name='%s', did=%" PRIu64 ", dev=%" PRIu64 ", ino=%"
                PRIu64,
                stmt_param_name, stmt_param_did, stmt_param_dev, stmt_param_ino);

            if (sqlite_return != SQLITE_DONE) {
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

            if ((uint32_t) lastid > 0xffffffff) {
                /* CNID set is depleted, restart from scratch */
                EC_NEG1(cnid_sqlite_execute
                        (db->cnid_sqlite_con,
                         "BEGIN TRANSACTION;"
                         "UPDATE volumes SET Depleted = 1 WHERE VolUUID = \"%s\";"
                         "DELETE FROM \"%s\";"
                         "COMMIT;",
                         db->cnid_sqlite_voluuid_str,
                         db->cnid_sqlite_voluuid_str));
                db->cnid_sqlite_flags |= CNID_SQLITE_FLAG_DEPLETED;

                if (cnid_sqlite_execute(db->cnid_sqlite_con,
                                        "BEGIN TRANSACTION;"
                                        "UPDATE sqlite_sequence SET seq = 16 WHERE name = \"%s\";"
                                        "INSERT INTO sqlite_sequence (name,seq) SELECT \"%s\", "
                                        "16 WHERE NOT EXISTS "
                                        "(SELECT changes() AS change "
                                        "FROM sqlite_sequence WHERE change <> 0);"
                                        "COMMIT;",
                                        db->cnid_sqlite_voluuid_str,
                                        db->cnid_sqlite_voluuid_str)) {
                    LOG(log_error, logtype_cnid, "cnid_sqlite_open: sqlite query error: %s",
                        sqlite3_errmsg(db->cnid_sqlite_con));
                    EC_FAIL;
                }
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
    return id;
}

cnid_t cnid_sqlite_get(struct _cnid_db *cdb, cnid_t did, const char *name,
                       size_t len)
{
    EC_INIT;
    CNID_sqlite_private *db;
    cnid_t id = CNID_INVALID;
    char *sql = NULL;
    sqlite3_stmt *transient_stmt = NULL;
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_get(did: %" PRIu32 ", name: \"%s\"): BEGIN",
        ntohl(did), name);

    if (!cdb || !(db = cdb->cnid_db_private) || !name) {
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

    EC_NEG1(ret = asprintf
                  (&sql,
                   "SELECT Id FROM \"%s\" WHERE Name = \"%s\" AND Did = ?",
                   db->cnid_sqlite_voluuid_str, name));
    EC_ZERO_LOG(sqlite3_prepare_v2
                (db->cnid_sqlite_con, sql, strlen(sql), &transient_stmt, NULL));
    EC_ZERO_LOG(sqlite3_bind_int64(transient_stmt, 1, ntohl(did)));

    if ((sqlite3_step(transient_stmt) != SQLITE_ROW)
            && (sqlite3_errcode(db->cnid_sqlite_con) != 0)) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_get: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        EC_FAIL;
    }

    sqlite3_int64 id_int64 = htonl(sqlite3_column_int64(transient_stmt, 0));
    id = (cnid_t)id_int64;
EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid,
        "cnid_sqlite_get(id: %" PRIu32 ", did: %" PRIu32 ", name: \"%s\"): END",
        ntohl(id), ntohl(did), name);

    if (transient_stmt) {
        sqlite3_finalize(transient_stmt);
    }

    return id;
}

char *cnid_sqlite_resolve(struct _cnid_db *cdb, cnid_t * id, void *buffer,
                          size_t len)
{
    EC_INIT;
    char *sql = NULL;
    sqlite3_stmt *transient_stmt = NULL;
    CNID_sqlite_private *db;
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_resolve(id: %u): BEGIN", *id);

    if (!cdb || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_resolve: Parameter error");
        errno = CNID_ERR_PARAM;
        *id = CNID_INVALID;
        EC_FAIL;
    }

    EC_NEG1(ret = asprintf
                  (&sql,
                   "SELECT Did, Name FROM \"%s\" WHERE Id=?",
                   db->cnid_sqlite_voluuid_str));
    EC_ZERO_LOG(sqlite3_prepare_v2
                (db->cnid_sqlite_con, sql, strlen(sql), &transient_stmt, NULL));
    EC_ZERO_LOG(sqlite3_bind_int64(transient_stmt, 1, ntohl(*id)));

    if (sqlite3_step(transient_stmt) != SQLITE_ROW) {
        LOG(log_error, logtype_cnid,
            "cnid_sqlite_resolve: No result found for Id: %" PRIu32, ntohl(*id));
        *id = CNID_INVALID;
        EC_FAIL;
    }

    sqlite3_int64 id_int64 = htonl(sqlite3_column_int64(transient_stmt, 0));
    *id = (cnid_t)id_int64;
    strlcpy(buffer, (const char *)sqlite3_column_text(transient_stmt, 1), len);
    ((char *)buffer)[len - 1] = '\0';
EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_resolve(id: %u): END", *id);

    if (transient_stmt) {
        sqlite3_finalize(transient_stmt);
    }

    if (sql) {
        free(sql);
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
    char *sql = NULL;
    sqlite3_stmt *transient_stmt = NULL;
    CNID_sqlite_private *db;
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_getstamp(): BEGIN");

    if (!cdb || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid, "cnid_find: Parameter error");
        errno = CNID_ERR_PARAM;
        return CNID_INVALID;
    }

    if (!buffer) {
        LOG(log_error, logtype_cnid, "cnid_getstamp: bad buffer");
        return CNID_INVALID;
    }

    EC_NEG1(asprintf
            (&sql,
             "SELECT Stamp FROM volumes WHERE VolPath=\"%s\"",
             cdb->cnid_db_vol->v_path));
    EC_ZERO_LOG(sqlite3_prepare_v2
                (db->cnid_sqlite_con, sql, strlen(sql), &transient_stmt, NULL));

    if ((sqlite3_step(transient_stmt) != SQLITE_ROW)
            || (sqlite3_column_text(transient_stmt, 0) == NULL)) {
        LOG(log_error, logtype_cnid,
            "Can't get DB stamp for volumes \"%s\"",
            cdb->cnid_db_vol->v_path);
        EC_FAIL;
    }

    strlcpy(buffer, (const char *)sqlite3_column_text(transient_stmt, 0), len);
EC_CLEANUP:
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_getstamp(): END");

    if (transient_stmt) {
        sqlite3_finalize(transient_stmt);
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
    LOG(log_maxdebug, logtype_cnid, "cnid_sqlite_wipe(\"%s\"): BEGIN",
        cdb->cnid_db_vol->v_localname);

    if (!cdb || !db) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_wipe: Parameter error");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    EC_NEG1(cnid_sqlite_execute(
                db->cnid_sqlite_con,
                "UPDATE volumes SET Depleted = 0 WHERE VolUUID = \"%s\";"
                "DELETE FROM \"%s\";"
                "UPDATE sqlite_sequence SET seq = 16 WHERE name = \"%s\";"
                "INSERT INTO sqlite_sequence (name,seq) SELECT \"%s\","
                "16 WHERE NOT EXISTS "
                "(SELECT changes() AS change FROM sqlite_sequence WHERE change <> 0);",
                db->cnid_sqlite_voluuid_str,
                db->cnid_sqlite_voluuid_str,
                db->cnid_sqlite_voluuid_str,
                db->cnid_sqlite_voluuid_str));
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
    bstring dbpath = NULL;
    int sqlite_return;
    EC_NULL(cdb = cnid_sqlite_new(vol));
    EC_NULL(db =
                (CNID_sqlite_private *) calloc(1,
                    sizeof
                    (CNID_sqlite_private)));
    cdb->cnid_db_private = db;
    EC_NULL(dbpath = bformat("%sCNID/sqlite/%s.sqlite", _PATH_STATEDIR,
                             vol->v_localname));
    EC_NULL(db->cnid_sqlite_voluuid_str = uuid_strip_dashes(vol->v_uuid));
    /* Initialize and connect to sqlite3 database */
    EC_ZERO(sqlite3_initialize());

    if (sqlite3_open_v2(bdata(dbpath),
                        &db->cnid_sqlite_con,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL)) {
        LOG(log_error, logtype_cnid, "sqlite open error: %s, path: %s",
            sqlite3_errmsg(db->cnid_sqlite_con),
            bdata(dbpath));
        EC_FAIL;
    }

    /* Add volume to volume table */
    if (cnid_sqlite_execute(db->cnid_sqlite_con,
                            "CREATE TABLE IF NOT EXISTS volumes "
                            "(VolUUID CHAR(32) PRIMARY KEY, "
                            "VolPath TEXT(4096), "
                            "Stamp BINARY(8), "
                            "Depleted INT"
                            ")")) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_open: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        EC_FAIL;
    }

    /* Add index to volume */
    if (cnid_sqlite_execute(db->cnid_sqlite_con,
                            "CREATE INDEX IF NOT EXISTS idx_volpath "
                            "ON volumes(VolPath)")) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_open: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        EC_FAIL;
    }

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
        int depleted = sqlite3_column_int(transient_stmt, 0);

        if (depleted) {
            LOG(log_debug, logtype_cnid,
                "CNID set for volume '%s' was depleted before, "
                "ignoring CNID hint AppleDouble file",
                vol->v_path);
            db->cnid_sqlite_flags |= CNID_SQLITE_FLAG_DEPLETED;
        } else {
            LOG(log_debug, logtype_cnid,
                "CNID set for volume '%s' has never been depleted",
                vol->v_path);
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
    if (cnid_sqlite_execute(db->cnid_sqlite_con,
                            "CREATE TABLE IF NOT EXISTS \"%s\" ("
                            "Id INTEGER PRIMARY KEY AUTOINCREMENT,"
                            "Name VARCHAR(255) NOT NULL,"
                            "Did INTEGER NOT NULL,"
                            "DevNo INTEGER NOT NULL,"
                            "InodeNo INTEGER NOT NULL,"
                            "UNIQUE (Did, Name),"
                            "UNIQUE (DevNo, InodeNo)"
                            ");",
                            db->cnid_sqlite_voluuid_str)) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_open: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        EC_FAIL;
    }

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
    if (cnid_sqlite_execute(db->cnid_sqlite_con,
                            "UPDATE sqlite_sequence SET seq = 16 WHERE name = \"%s\";"
                            "INSERT INTO sqlite_sequence (name,seq) SELECT \"%s\","
                            "16 WHERE NOT EXISTS "
                            "(SELECT changes() AS change "
                            "FROM sqlite_sequence WHERE change <> 0);",
                            db->cnid_sqlite_voluuid_str,
                            db->cnid_sqlite_voluuid_str)) {
        LOG(log_error, logtype_cnid, "cnid_sqlite_open: sqlite query error: %s",
            sqlite3_errmsg(db->cnid_sqlite_con));
        EC_FAIL;
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

    return cdb;
}

struct _cnid_module cnid_sqlite_module = {
    "sqlite",
    { NULL, NULL },
    cnid_sqlite_open,
    0
};
