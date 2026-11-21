/*
 * Copyright (C) Ralph Boehme 2013
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * All Rights Reserved.  See COPYING.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*!
 * @file
 * @brief MySQL CNID backend
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <time.h>

#include <errmsg.h>
#include <mysql.h>
#include <mysqld_error.h>

#include <atalk/adouble.h>
#include <atalk/cnid_mysql_private.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/sl_names.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/volume.h>

/* A hint binds as an explicit Id, raising the AUTO_INCREMENT high-water mark.
 * Hints come from AppleDouble/EA metadata any file owner can write, so the
 * reserve must be wide enough that a planted hint cannot put the depletion
 * reset — which empties the table and disconnects every session — within reach
 * of ordinary file creation. Hints inside the reserve are not inserted. */
#define CNID_MYSQL_HINT_RESERVE    (1u << 28)
#define CNID_MYSQL_MAX_HINT        (UINT32_MAX - CNID_MYSQL_HINT_RESERVE)

/* Bound on the lookup-then-insert and update-then-verify cycles: a duplicate-key
 * race that keeps repeating would otherwise spin the session forever */
#define CNID_MYSQL_ADD_ATTEMPTS 16

/* Bound on CR_SERVER_LOST recovery: a server that reconnects but keeps dropping
 * the execute would otherwise hold the child inside cnid_add() indefinitely,
 * out of reach of the deferred-die path */
#define CNID_MYSQL_RECONNECT_ATTEMPTS 2

/*
 * The column of folded names: netatalk folds a name for case and diacritics
 * as the matcher does, so a search compares bytes whatever the collation.
 * The 1 in its name is the version of the folding, so a netatalk that folds
 * otherwise never reads it. A row without a folded name, as an older
 * netatalk writes one, holds NULL.
 */
#define CNID_MYSQL_FOLDED_NAME  "Folded1"
#define CNID_MYSQL_FOLDED       CNID_MYSQL_FOLDED_NAME " VARBINARY(1020) NULL"
/* Bytes of a folded name the column holds; a longer one is NULL */
#define CNID_MYSQL_FOLDED_MAX   1020

static MYSQL_BIND lookup_param[4], lookup_result[5];
static MYSQL_BIND add_param[5], put_param[6];
static MYSQL_BIND get_param[2], get_result[1];
static MYSQL_BIND delete_param[1];
static MYSQL_BIND resolve_param[1], resolve_result[2];
static MYSQL_BIND purge_param[5];
static MYSQL_BIND dirlist_param[2], dirlist_result[4];
static unsigned long long dirlist_param_max;
static unsigned long long dirlist_result_id;
static char               dirlist_result_name[MAXPATHLEN];
static unsigned long      dirlist_result_name_len;
static unsigned long long dirlist_result_dev;
static unsigned long long dirlist_result_ino;

/*!
 * Prepared statement parameters
 */
static char               stmt_param_name[MAXPATHLEN];
static unsigned long      stmt_param_name_len;
/* the folded name, 0 bytes for none */
static char               stmt_param_folded[CNID_MYSQL_FOLDED_MAX];
static unsigned long      stmt_param_folded_len;
static unsigned long long stmt_param_id;
static unsigned long long stmt_param_did;
static unsigned long long stmt_param_dev;
static unsigned long long stmt_param_ino;

/*!
 * lookup result parameters
 */
static unsigned long long lookup_result_id;
static unsigned long long lookup_result_did;
static char               lookup_result_name[MAXPATHLEN];
static unsigned long      lookup_result_name_len;
static unsigned long long lookup_result_dev;
static unsigned long long lookup_result_ino;
static unsigned long long get_result_id;
static unsigned long long resolve_result_did;
static char               resolve_result_name[MAXPATHLEN];
static unsigned long      resolve_result_name_len;

/*!
 * @brief Tell whether a CNID hint from AppleDouble or EA metadata is safe to
 * bind
 *
 * The call rejects the reserved range under CNID_START. It also rejects any
 * value above CNID_MYSQL_MAX_HINT, which as an explicit Id would push the
 * AUTO_INCREMENT high-water mark toward the depletion reset.
 *
 * @param[in] hint  CNID in network order
 * @returns true when CNID_START <= hint <= CNID_MYSQL_MAX_HINT in host order
 */
static bool cnid_mysql_hint_usable(cnid_t hint)
{
    uint32_t id = ntohl(hint);
    return id >= CNID_START && id <= CNID_MYSQL_MAX_HINT;
}

/*!
 * @brief Prepare the lookup statement of a MySQL CNID table
 *
 * The statement selects the rows that match the name and parent, or the
 * device and inode, of an object.
 *
 * @param[in,out] db  MySQL backend state with the connection and the table name
 * @returns 0, or -1 when preparing or binding fails
 */
static int init_prepared_stmt_lookup(CNID_mysql_private *db)
{
    EC_INIT;
    char *sql = NULL;
    lookup_param[0].buffer_type    = MYSQL_TYPE_STRING;
    lookup_param[0].buffer         = &stmt_param_name;
    lookup_param[0].buffer_length  = sizeof(stmt_param_name);
    lookup_param[0].length         = &stmt_param_name_len;
    lookup_param[1].buffer_type    = MYSQL_TYPE_LONGLONG;
    lookup_param[1].buffer         = &stmt_param_did;
    lookup_param[1].is_unsigned    = true;
    lookup_param[2].buffer_type    = MYSQL_TYPE_LONGLONG;
    lookup_param[2].buffer         = &stmt_param_dev;
    lookup_param[2].is_unsigned    = true;
    lookup_param[3].buffer_type    = MYSQL_TYPE_LONGLONG;
    lookup_param[3].buffer         = &stmt_param_ino;
    lookup_param[3].is_unsigned    = true;
    lookup_result[0].buffer_type   = MYSQL_TYPE_LONGLONG;
    lookup_result[0].buffer        = &lookup_result_id;
    lookup_result[0].is_unsigned   = true;
    lookup_result[1].buffer_type   = MYSQL_TYPE_LONGLONG;
    lookup_result[1].buffer        = &lookup_result_did;
    lookup_result[1].is_unsigned   = true;
    lookup_result[2].buffer_type   = MYSQL_TYPE_STRING;
    lookup_result[2].buffer        = &lookup_result_name;
    lookup_result[2].buffer_length = sizeof(lookup_result_name);
    lookup_result[2].length        = &lookup_result_name_len;
    lookup_result[3].buffer_type   = MYSQL_TYPE_LONGLONG;
    lookup_result[3].buffer        = &lookup_result_dev;
    lookup_result[3].is_unsigned   = true;
    lookup_result[4].buffer_type   = MYSQL_TYPE_LONGLONG;
    lookup_result[4].buffer        = &lookup_result_ino;
    lookup_result[4].is_unsigned   = true;
    EC_NULL(db->cnid_lookup_stmt = mysql_stmt_init(db->cnid_mysql_con));
    EC_NEG1(asprintf(&sql,
                     "SELECT Id,Did,Name,DevNo,InodeNo FROM `%s` "
                     "WHERE (Name=? AND Did=?) OR (DevNo=? AND InodeNo=?)",
                     db->cnid_mysql_voluuid_str));
    EC_ZERO_LOG(mysql_stmt_prepare(db->cnid_lookup_stmt, sql, strlen(sql)));
    EC_ZERO_LOG(mysql_stmt_bind_param(db->cnid_lookup_stmt, lookup_param));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

/*!
 * @brief Point a statement parameter at the folded name
 *
 * NULLIF() in the statements stores the 0 bytes of no fold as NULL.
 *
 * @param[out] param  the parameter
 */
static void cnid_mysql_folded_param(MYSQL_BIND *param)
{
    param->buffer_type   = MYSQL_TYPE_BLOB;
    param->buffer        = stmt_param_folded;
    param->buffer_length = sizeof(stmt_param_folded);
    param->length        = &stmt_param_folded_len;
}

/*!
 * @brief Fold a name into the statement parameter of the folded column
 *
 * A fold longer than the column, or none for want of memory, leaves the
 * column NULL, which every search takes as a candidate.
 *
 * @param[in] name     the name
 * @param[in] len      bytes of name
 * @param[in] charset  the name's charset, the volume's
 */
static void cnid_mysql_fold_param(const char *name, size_t len,
                                  charset_t charset)
{
    char *folded = sl_name_fold_in(name, len, charset);
    size_t n = folded != NULL ? strlen(folded) : 0;
    stmt_param_folded_len = 0;

    if (folded != NULL && n <= sizeof(stmt_param_folded)) {
        memcpy(stmt_param_folded, folded, n);
        stmt_param_folded_len = n;
    }

    free(folded);
}

/*!
 * @brief Prepare the insert statement for a new entry, numbered by
 * AUTO_INCREMENT
 *
 * The prepare tells whether the table has the column of folded names: a
 * table an older netatalk made has none.
 *
 * @param[in,out] db  MySQL backend state with the connection and the table name
 * @returns 0, or -1 when preparing or binding fails
 */
static int init_prepared_stmt_add(CNID_mysql_private *db)
{
    EC_INIT;
    char *sql = NULL;
    EC_NULL(db->cnid_add_stmt = mysql_stmt_init(db->cnid_mysql_con));
    EC_NEG1(asprintf(&sql,
                     "INSERT INTO `%s` (Name,Did,DevNo,InodeNo,"
                     CNID_MYSQL_FOLDED_NAME ") VALUES(?,?,?,?,NULLIF(?,''))",
                     db->cnid_mysql_voluuid_str));
    db->cnid_mysql_folded = false;

    if (mysql_stmt_prepare(db->cnid_add_stmt, sql, strlen(sql)) == 0) {
        db->cnid_mysql_folded = true;
    } else {
        if (mysql_stmt_errno(db->cnid_add_stmt) != ER_BAD_FIELD_ERROR) {
            LOG(log_error, logtype_cnid, "init_prepared_stmt_add: %s",
                mysql_stmt_error(db->cnid_add_stmt));
            EC_FAIL;
        }

        free(sql);
        sql = NULL;
        EC_NEG1(asprintf(&sql,
                         "INSERT INTO `%s` (Name,Did,DevNo,InodeNo) "
                         "VALUES(?,?,?,?)", db->cnid_mysql_voluuid_str));
        EC_ZERO_LOG(mysql_stmt_prepare(db->cnid_add_stmt, sql, strlen(sql)));
    }

    add_param[0].buffer_type    = MYSQL_TYPE_STRING;
    add_param[0].buffer         = &stmt_param_name;
    add_param[0].buffer_length  = sizeof(stmt_param_name);
    add_param[0].length         = &stmt_param_name_len;
    add_param[1].buffer_type    = MYSQL_TYPE_LONGLONG;
    add_param[1].buffer         = &stmt_param_did;
    add_param[1].is_unsigned    = true;
    add_param[2].buffer_type    = MYSQL_TYPE_LONGLONG;
    add_param[2].buffer         = &stmt_param_dev;
    add_param[2].is_unsigned    = true;
    add_param[3].buffer_type    = MYSQL_TYPE_LONGLONG;
    add_param[3].buffer         = &stmt_param_ino;
    add_param[3].is_unsigned    = true;
    cnid_mysql_folded_param(&add_param[4]);
    EC_ZERO_LOG(mysql_stmt_bind_param(db->cnid_add_stmt, add_param));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

/*!
 * @brief Prepare the insert statement for an entry with an explicit CNID
 *
 * The statement writes the folded name where init_prepared_stmt_add() found
 * the column.
 *
 * @param[in,out] db  MySQL backend state with the connection and the table name
 * @returns 0, or -1 when preparing or binding fails
 */
static int init_prepared_stmt_put(CNID_mysql_private *db)
{
    EC_INIT;
    char *sql = NULL;
    EC_NULL(db->cnid_put_stmt = mysql_stmt_init(db->cnid_mysql_con));

    if (db->cnid_mysql_folded) {
        EC_NEG1(asprintf(&sql,
                         "INSERT INTO `%s` (Id,Name,Did,DevNo,InodeNo,"
                         CNID_MYSQL_FOLDED_NAME ") VALUES(?,?,?,?,?,NULLIF(?,''))",
                         db->cnid_mysql_voluuid_str));
    } else {
        EC_NEG1(asprintf(&sql,
                         "INSERT INTO `%s` (Id,Name,Did,DevNo,InodeNo) "
                         "VALUES(?,?,?,?,?)", db->cnid_mysql_voluuid_str));
    }

    put_param[0].buffer_type    = MYSQL_TYPE_LONGLONG;
    put_param[0].buffer         = &stmt_param_id;
    put_param[0].is_unsigned    = true;
    put_param[1].buffer_type    = MYSQL_TYPE_STRING;
    put_param[1].buffer         = &stmt_param_name;
    put_param[1].buffer_length  = sizeof(stmt_param_name);
    put_param[1].length         = &stmt_param_name_len;
    put_param[2].buffer_type    = MYSQL_TYPE_LONGLONG;
    put_param[2].buffer         = &stmt_param_did;
    put_param[2].is_unsigned    = true;
    put_param[3].buffer_type    = MYSQL_TYPE_LONGLONG;
    put_param[3].buffer         = &stmt_param_dev;
    put_param[3].is_unsigned    = true;
    put_param[4].buffer_type    = MYSQL_TYPE_LONGLONG;
    put_param[4].buffer         = &stmt_param_ino;
    put_param[4].is_unsigned    = true;
    cnid_mysql_folded_param(&put_param[5]);
    EC_ZERO_LOG(mysql_stmt_prepare(db->cnid_put_stmt, sql, strlen(sql)));
    EC_ZERO_LOG(mysql_stmt_bind_param(db->cnid_put_stmt, put_param));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

/*!
 * @brief Prepare the statement that finds the CNID of a name in a directory
 *
 * @param[in,out] db  MySQL backend state with the connection and the table name
 * @returns 0, or -1 when preparing or binding fails
 */
static int init_prepared_stmt_get(CNID_mysql_private *db)
{
    EC_INIT;
    char *sql = NULL;
    get_param[0].buffer_type   = MYSQL_TYPE_STRING;
    get_param[0].buffer        = &stmt_param_name;
    get_param[0].buffer_length = sizeof(stmt_param_name);
    get_param[0].length        = &stmt_param_name_len;
    get_param[1].buffer_type   = MYSQL_TYPE_LONGLONG;
    get_param[1].buffer        = &stmt_param_did;
    get_param[1].is_unsigned   = true;
    get_result[0].buffer_type  = MYSQL_TYPE_LONGLONG;
    get_result[0].buffer       = &get_result_id;
    get_result[0].is_unsigned  = true;
    EC_NULL(db->cnid_get_stmt = mysql_stmt_init(db->cnid_mysql_con));
    EC_NEG1(asprintf(&sql, "SELECT Id FROM `%s` WHERE Name=? AND Did=?",
                     db->cnid_mysql_voluuid_str));
    EC_ZERO_LOG(mysql_stmt_prepare(db->cnid_get_stmt, sql, strlen(sql)));
    EC_ZERO_LOG(mysql_stmt_bind_param(db->cnid_get_stmt, get_param));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

/*!
 * @brief Prepare the statement that removes an entry by CNID
 *
 * @param[in,out] db  MySQL backend state with the connection and the table name
 * @returns 0, or -1 when preparing or binding fails
 */
static int init_prepared_stmt_delete(CNID_mysql_private *db)
{
    EC_INIT;
    char *sql = NULL;
    delete_param[0].buffer_type = MYSQL_TYPE_LONGLONG;
    delete_param[0].buffer      = &stmt_param_id;
    delete_param[0].is_unsigned = true;
    EC_NULL(db->cnid_delete_stmt = mysql_stmt_init(db->cnid_mysql_con));
    EC_NEG1(asprintf(&sql, "DELETE FROM `%s` WHERE Id=?",
                     db->cnid_mysql_voluuid_str));
    EC_ZERO_LOG(mysql_stmt_prepare(db->cnid_delete_stmt, sql, strlen(sql)));
    EC_ZERO_LOG(mysql_stmt_bind_param(db->cnid_delete_stmt, delete_param));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

/*!
 * @brief Prepare the statement that finds the parent and the name of a CNID
 *
 * @param[in,out] db  MySQL backend state with the connection and the table name
 * @returns 0, or -1 when preparing or binding fails
 */
static int init_prepared_stmt_resolve(CNID_mysql_private *db)
{
    EC_INIT;
    char *sql = NULL;
    resolve_param[0].buffer_type    = MYSQL_TYPE_LONGLONG;
    resolve_param[0].buffer         = &stmt_param_id;
    resolve_param[0].is_unsigned    = true;
    resolve_result[0].buffer_type   = MYSQL_TYPE_LONGLONG;
    resolve_result[0].buffer        = &resolve_result_did;
    resolve_result[0].is_unsigned   = true;
    resolve_result[1].buffer_type   = MYSQL_TYPE_STRING;
    resolve_result[1].buffer        = &resolve_result_name;
    resolve_result[1].buffer_length = sizeof(resolve_result_name);
    resolve_result[1].length        = &resolve_result_name_len;
    EC_NULL(db->cnid_resolve_stmt = mysql_stmt_init(db->cnid_mysql_con));
    EC_NEG1(asprintf(&sql, "SELECT Did, Name FROM `%s` WHERE Id=?",
                     db->cnid_mysql_voluuid_str));
    EC_ZERO_LOG(mysql_stmt_prepare(db->cnid_resolve_stmt, sql, strlen(sql)));
    EC_ZERO_LOG(mysql_stmt_bind_param(db->cnid_resolve_stmt, resolve_param));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

/*!
 * @brief Prepare the statement that removes every entry clashing with an update
 *
 * The statement removes the rows that match the CNID, the name and parent, or
 * the device and inode.
 *
 * @param[in,out] db  MySQL backend state with the connection and the table name
 * @returns 0, or -1 when preparing or binding fails
 */
static int init_prepared_stmt_purge(CNID_mysql_private *db)
{
    EC_INIT;
    char *sql = NULL;
    purge_param[0].buffer_type   = MYSQL_TYPE_LONGLONG;
    purge_param[0].buffer        = &stmt_param_id;
    purge_param[0].is_unsigned   = true;
    purge_param[1].buffer_type   = MYSQL_TYPE_LONGLONG;
    purge_param[1].buffer        = &stmt_param_did;
    purge_param[1].is_unsigned   = true;
    purge_param[2].buffer_type   = MYSQL_TYPE_STRING;
    purge_param[2].buffer        = &stmt_param_name;
    purge_param[2].buffer_length = sizeof(stmt_param_name);
    purge_param[2].length        = &stmt_param_name_len;
    purge_param[3].buffer_type   = MYSQL_TYPE_LONGLONG;
    purge_param[3].buffer        = &stmt_param_dev;
    purge_param[3].is_unsigned   = true;
    purge_param[4].buffer_type   = MYSQL_TYPE_LONGLONG;
    purge_param[4].buffer        = &stmt_param_ino;
    purge_param[4].is_unsigned   = true;
    EC_NULL(db->cnid_purge_stmt = mysql_stmt_init(db->cnid_mysql_con));
    EC_NEG1(asprintf(&sql,
                     "DELETE FROM `%s` WHERE Id=? OR (Did=? AND Name=?) "
                     "OR (DevNo=? AND InodeNo=?)",
                     db->cnid_mysql_voluuid_str));
    EC_ZERO_LOG(mysql_stmt_prepare(db->cnid_purge_stmt, sql, strlen(sql)));
    EC_ZERO_LOG(mysql_stmt_bind_param(db->cnid_purge_stmt, purge_param));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

/*!
 * @brief Prepare the statement that lists the rows of one folder
 *
 * @param[in,out] db  MySQL backend state with the connection and the table name
 * @returns 0, or -1 when preparing or binding fails
 */
static int init_prepared_stmt_dirlist(CNID_mysql_private *db)
{
    EC_INIT;
    char *sql = NULL;
    dirlist_param[0].buffer_type    = MYSQL_TYPE_LONGLONG;
    dirlist_param[0].buffer         = &stmt_param_did;
    dirlist_param[0].is_unsigned    = true;
    dirlist_param[1].buffer_type    = MYSQL_TYPE_LONGLONG;
    dirlist_param[1].buffer         = &dirlist_param_max;
    dirlist_param[1].is_unsigned    = true;
    dirlist_result[0].buffer_type   = MYSQL_TYPE_LONGLONG;
    dirlist_result[0].buffer        = &dirlist_result_id;
    dirlist_result[0].is_unsigned   = true;
    dirlist_result[1].buffer_type   = MYSQL_TYPE_STRING;
    dirlist_result[1].buffer        = &dirlist_result_name;
    dirlist_result[1].buffer_length = sizeof(dirlist_result_name);
    dirlist_result[1].length        = &dirlist_result_name_len;
    dirlist_result[2].buffer_type   = MYSQL_TYPE_LONGLONG;
    dirlist_result[2].buffer        = &dirlist_result_dev;
    dirlist_result[2].is_unsigned   = true;
    dirlist_result[3].buffer_type   = MYSQL_TYPE_LONGLONG;
    dirlist_result[3].buffer        = &dirlist_result_ino;
    dirlist_result[3].is_unsigned   = true;
    EC_NULL(db->cnid_dirlist_stmt = mysql_stmt_init(db->cnid_mysql_con));
    EC_NEG1(asprintf(&sql,
                     "SELECT Id,Name,DevNo,InodeNo FROM `%s` "
                     "WHERE Did=? LIMIT ?",
                     db->cnid_mysql_voluuid_str));
    EC_ZERO_LOG(mysql_stmt_prepare(db->cnid_dirlist_stmt, sql, strlen(sql)));
    EC_ZERO_LOG(mysql_stmt_bind_param(db->cnid_dirlist_stmt, dirlist_param));
EC_CLEANUP:

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

static void close_prepared_stmt(CNID_mysql_private *db);

/*!
 * @brief Prepare all 8 statements of a MySQL CNID table
 *
 * A failure closes every statement and sets CNID_ERR_DB, which ends the
 * session. The lookup or purge before an add's or an update's insert then
 * finds its statement NULL in cnid_mysql_stmt_execute(), which prepares them
 * all again.
 *
 * @param[in,out] db  MySQL backend state with the connection and the table name
 * @returns 0, or -1 with errno CNID_ERR_DB when one fails
 */
static int init_prepared_stmt(CNID_mysql_private *db)
{
    EC_INIT;
    EC_ZERO(init_prepared_stmt_lookup(db));
    EC_ZERO(init_prepared_stmt_dirlist(db));
    EC_ZERO(init_prepared_stmt_add(db));
    EC_ZERO(init_prepared_stmt_put(db));
    EC_ZERO(init_prepared_stmt_get(db));
    EC_ZERO(init_prepared_stmt_delete(db));
    EC_ZERO(init_prepared_stmt_resolve(db));
    EC_ZERO(init_prepared_stmt_purge(db));
EC_CLEANUP:

    if (ret != 0) {
        close_prepared_stmt(db);
        errno = CNID_ERR_DB;
    }

    EC_EXIT;
}

/*!
 * @brief Close and clear the backend's prepared MySQL statements
 *
 * NULL-safe, and NULLs each statement after closing it. A failed recovery
 * leaves a mix of closed and live statements, and a later close or re-init must
 * neither double-close nor leak the survivors.
 *
 * @param[in,out] db  backend private data holding the statement pointers
 */
static void close_prepared_stmt(CNID_mysql_private *db)
{
    MYSQL_STMT **stmts[] = {
        &db->cnid_lookup_stmt, &db->cnid_add_stmt, &db->cnid_put_stmt,
        &db->cnid_get_stmt, &db->cnid_delete_stmt, &db->cnid_resolve_stmt,
        &db->cnid_purge_stmt, &db->cnid_dirlist_stmt,
    };

    for (size_t i = 0; i < sizeof(stmts) / sizeof(stmts[0]); i++) {
        if (*stmts[i]) {
            mysql_stmt_close(*stmts[i]);
            *stmts[i] = NULL;
        }
    }
}

/*!
 * @brief Classify a MySQL error code as a CNID_ERR_* errno
 *
 * Lock wait timeouts and deadlocks become CNID_ERR_BUSY, because the lock
 * frees again. InnoDB reports a deadlock at once, and concurrent sessions that
 * insert into one CNID table hit both codes routinely. Crashed tables become
 * CNID_ERR_CORRUPT. Any other code becomes CNID_ERR_DB, which ends the session.
 *
 * @param[in] mysql_error_code  MySQL error number, or 0 for a failure without
 *                              one
 */
static void cnid_mysql_set_errno(unsigned int mysql_error_code)
{
    switch (mysql_error_code) {
    case ER_LOCK_WAIT_TIMEOUT:
    case ER_LOCK_DEADLOCK:
        errno = CNID_ERR_BUSY;
        break;

    case ER_CRASHED_ON_USAGE:
    case ER_CRASHED_ON_REPAIR:
        errno = CNID_ERR_CORRUPT;
        break;

    default:
        errno = CNID_ERR_DB;
        break;
    }
}

/*!
 * @brief Drain the remaining results of a multi-statement batch
 *
 * The cnid_mysql_execute call reports only the first statement of a batch. A
 * positive mysql_next_result means a later statement failed, so a wipe whose
 * TRUNCATE or ALTER failed still fails here.
 *
 * @param[in] con  MySQL connection
 * @returns 0 when every statement succeeded, -1 with errno classified otherwise
 */
static int cnid_mysql_drain_results(MYSQL *con)
{
    int next;

    do {
        MYSQL_RES *result = mysql_store_result(con);

        if (result) {
            mysql_free_result(result);
        }
    } while ((next = mysql_next_result(con)) == 0);

    if (next > 0) {
        LOG(log_error, logtype_cnid, "MySQL multi-statement error: %s",
            mysql_error(con));
        cnid_mysql_set_errno(mysql_errno(con));
        return -1;
    }

    return 0;
}

static int cnid_mysql_execute(MYSQL *con, const char *sql);

/*!
 * @brief Empty the volume's table, restart its CNIDs at 17 and write Depleted
 *
 * One batch runs it, but TRUNCATE and ALTER commit on their own, so a
 * TRUNCATE that fails leaves the new mark and the rows in place.
 *
 * @param[in] db        MySQL backend state
 * @param[in] depleted  the mark, 1 or 0
 * @returns 0, or -1 with errno classified when a statement failed
 */
static int cnid_mysql_empty(const CNID_mysql_private *db, int depleted)
{
    const char *uuid = db->cnid_mysql_voluuid_str;
    char *sql = NULL;
    int ret = -1;

    if (asprintf(&sql, "START TRANSACTION;"
                       "UPDATE volumes SET Depleted=%d WHERE VolUUID='%s';"
                       "TRUNCATE TABLE `%s`;"
                       "ALTER TABLE `%s` AUTO_INCREMENT = 17;"
                       "COMMIT;", depleted, uuid, uuid, uuid) < 0) {
        errno = CNID_ERR_DB;
        return -1;
    }

    if (cnid_mysql_execute(db->cnid_mysql_con, sql) == 0) {
        ret = cnid_mysql_drain_results(db->cnid_mysql_con);
    }

    free(sql);
    return ret;
}

/*!
 * @brief Execute a prepared statement and reprepare after CR_SERVER_LOST
 *
 * The stmtp argument must point at the field in db, because a reprepare
 * replaces every statement and the retry runs the new one. A reprepare also
 * drops the stored result of any other statement. CR_SERVER_LOST on
 * CNID_MYSQL_RECONNECT_ATTEMPTS executes in a row ends the retries. A NULL
 * statement from an earlier failed reprepare gets prepared first.
 *
 * @param[in,out] db     MySQL backend state
 * @param[in,out] stmtp  address of the statement field in db
 * @returns 0, or -1 with errno classified
 */
static int cnid_mysql_stmt_execute(CNID_mysql_private *db, MYSQL_STMT **stmtp)
{
    EC_INIT;
    bool retry = true;
    unsigned int stmt_errno = 0;
    int reconnects = 0;

    /* A failed earlier recovery leaves handles NULL; re-prepare before use. */
    if (*stmtp == NULL) {
        close_prepared_stmt(db);
        EC_ZERO(init_prepared_stmt(db));
    }

    while (retry) {
        retry = false;

        if (mysql_stmt_execute(*stmtp)) {
            switch (mysql_stmt_errno(*stmtp)) {
            case CR_SERVER_LOST:
                if (++reconnects >= CNID_MYSQL_RECONNECT_ATTEMPTS) {
                    LOG(log_error, logtype_cnid,
                        "cnid_mysql_stmt_execute: server lost on %d successive "
                        "executes, giving up", reconnects);
                    stmt_errno = CR_SERVER_LOST;
                    EC_FAIL;
                }

                close_prepared_stmt(db);
                EC_ZERO(init_prepared_stmt(db));
                retry = true;
                continue;

            default:
                LOG(log_error, logtype_cnid, "MySQL statement error: %s",
                    mysql_stmt_error(*stmtp));
                stmt_errno = mysql_stmt_errno(*stmtp);
                EC_FAIL;
            }
        }
    }

EC_CLEANUP:

    if (ret != 0) {
        /* 0 for the re-prepare failures above, which classify as CNID_ERR_DB */
        cnid_mysql_set_errno(stmt_errno);
    }

    EC_EXIT;
}

/*!
 * @brief Run an SQL statement or batch on a connection
 *
 * The call returns -1 for any failure of mysql_query, so EC_NEG1 call sites
 * see it.
 *
 * @param[in] con  MySQL connection
 * @param[in] sql  SQL text
 * @returns 0, or -1 with errno classified
 */
static int cnid_mysql_execute(MYSQL *con, const char *sql)
{
    LOG(log_maxdebug, logtype_cnid, "SQL: %s", sql);

    if (mysql_query(con, sql)) {
        LOG(log_debug, logtype_cnid, "MySQL query \"%s\", error: %s", sql,
            mysql_error(con));
        cnid_mysql_set_errno(mysql_errno(con));
        return -1;
    }

    return 0;
}

/*!
 * @brief Escape a string for a quoted literal of SQL text
 *
 * @param[in] con  MySQL connection, whose charset the escaping follows
 * @param[in] s    the string
 * @returns the escaped string, to free(), or NULL when out of memory
 */
static char *cnid_mysql_escape(MYSQL *con, const char *s)
{
    const size_t len = strlen(s);
    char *out = malloc(2 * len + 1);

    /* libmysqlclient fails with -1 under NO_BACKSLASH_ESCAPES, where a
     * backslash is itself and a doubled quote is the escape */
    if (out != NULL
            && mysql_real_escape_string(con, out, s, (unsigned long)len)
            == (unsigned long) -1) {
        char *o = out;

        for (const char *p = s; *p != '\0'; p++) {
            if (*p == '\'') {
                *o++ = '\'';
            }

            *o++ = *p;
        }

        *o = '\0';
    }

    return out;
}

/*!
 * @brief Remove a CNID from the MySQL table
 *
 * @param[in] cdb  CNID database
 * @param[in] id   CNID in network order, not 0
 * @returns 0, or -1 with errno classified
 */
int cnid_mysql_delete(struct _cnid_db *cdb, const cnid_t id)
{
    EC_INIT;
    CNID_mysql_private *db;

    if (!cdb || !(db = cdb->cnid_db_private) || !id) {
        LOG(log_error, logtype_cnid, "cnid_mysql_delete: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    LOG(log_debug, logtype_cnid, "cnid_mysql_delete(%" PRIu32 "): BEGIN",
        ntohl(id));
    stmt_param_id = ntohl(id);
    EC_ZERO(cnid_mysql_stmt_execute(db, &db->cnid_delete_stmt));
    LOG(log_debug, logtype_cnid, "cnid_mysql_delete(%" PRIu32 "): END", ntohl(id));
EC_CLEANUP:
    EC_EXIT;
}

/*!
 * @brief Close the MySQL connection of a CNID database and free the database
 *
 * @param[in] cdb  CNID database, and a NULL one logs an error
 */
void cnid_mysql_close(struct _cnid_db *cdb)
{
    CNID_mysql_private *db;

    if (!cdb) {
        LOG(log_error, logtype_cnid, "cnid_close called with NULL argument !");
        return;
    }

    if ((db = cdb->cnid_db_private) != NULL) {
        LOG(log_debug, logtype_cnid, "closing database connection for volume '%s'",
            cdb->cnid_db_vol->v_localname);
        free(db->cnid_mysql_voluuid_str);
        free(db->cnid_mysql_volpath);
        close_prepared_stmt(db);

        if (db->cnid_mysql_con) {
            mysql_close(db->cnid_mysql_con);
        }

        free(db);
    }

    free(cdb);
    return;
}

/*!
 * @brief Rewrite the entry of a CNID with a new name, parent, and stat
 *
 * The call removes every row that clashes on the CNID, the name and parent,
 * or the device and inode. It then inserts the entry with its CNID.
 * ER_DUP_ENTRY or CR_SERVER_LOST repeats both steps, up to
 * CNID_MYSQL_ADD_ATTEMPTS times.
 *
 * @param[in] cdb   CNID database
 * @param[in] id    CNID to keep
 * @param[in] st    stat of the object
 * @param[in] did   new parent CNID
 * @param[in] name  new name on disk
 * @param[in] len   length of name, up to MAXPATHLEN
 * @returns 0, or -1 with errno classified, CNID_ERR_CORRUPT after the last
 *          attempt
 */
int cnid_mysql_update(struct _cnid_db *cdb,
                      cnid_t id,
                      const struct stat *st,
                      cnid_t did,
                      const char *name,
                      size_t len)
{
    EC_INIT;
    CNID_mysql_private *db;
    cnid_t update_id = 0;
    int attempts = 0;

    if (!cdb || !(db = cdb->cnid_db_private) || !id || !st || !name) {
        LOG(log_error, logtype_cnid, "cnid_update: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid, "cnid_update: Path name is too long");
        errno = CNID_ERR_PATH;
        EC_FAIL;
    }

    uint64_t dev = 0;

    if (!(cdb->cnid_db_flags & CNID_FLAG_NODEV)) {
        dev = st->st_dev;
    }

    uint64_t ino = st->st_ino;

    do {
        stmt_param_id = ntohl(id);
        strncpy(stmt_param_name, name, sizeof(stmt_param_name));
        stmt_param_name_len = len;
        stmt_param_did = ntohl(did);
        stmt_param_dev = dev;
        stmt_param_ino = ino;
        EC_ZERO(cnid_mysql_stmt_execute(db, &db->cnid_purge_stmt));

        /* here, not before the purge: a reconnect prepares the statements
         * again and reads cnid_mysql_folded anew */
        if (db->cnid_mysql_folded) {
            cnid_mysql_fold_param(name, len, cdb->cnid_db_vol->v_volcharset);
        }

        if (mysql_stmt_execute(db->cnid_put_stmt)) {
            switch (mysql_stmt_errno(db->cnid_put_stmt)) {
            case ER_DUP_ENTRY:
                /*
                 * Race condition:
                 * between deletion and insert another process may have inserted
                 * this entry.
                 */
                continue;

            case CR_SERVER_LOST:
                close_prepared_stmt(db);
                EC_ZERO(init_prepared_stmt(db));
                continue;

            default:
                LOG(log_error, logtype_cnid, "cnid_mysql_update: %s",
                    mysql_stmt_error(db->cnid_put_stmt));
                cnid_mysql_set_errno(mysql_stmt_errno(db->cnid_put_stmt));
                EC_FAIL;
            }
        }

        update_id = (cnid_t)mysql_stmt_insert_id(db->cnid_put_stmt);
    } while (update_id != ntohl(id) && ++attempts < CNID_MYSQL_ADD_ATTEMPTS);

    if (update_id != ntohl(id)) {
        LOG(log_error, logtype_cnid,
            "cnid_mysql_update: giving up after %d attempts for id: %" PRIu32
            ", did: %" PRIu32 ", name: \"%s\"",
            attempts, ntohl(id), ntohl(did), name);
        errno = CNID_ERR_CORRUPT;
        EC_FAIL;
    }

EC_CLEANUP:
    EC_EXIT;
}

/*!
 * @brief Find the CNID of an object by name and parent or by device and inode
 *
 * A single matching row is the answer. Two rows, one per index, are stale,
 * and the call removes both.
 *
 * A row with another name or parent means a server-side move or a reused
 * inode. When its CNID equals the hint of the current add, the row takes the
 * new name and parent. Otherwise, and for a row with another device or inode,
 * the call removes the row. Each removal reports CNID_ERR_NOTFOUND, so
 * cnid_mysql_add inserts a new entry.
 *
 * @param[in] cdb   CNID database
 * @param[in] st    stat of the object
 * @param[in] did   CNID of the parent directory
 * @param[in] name  name on disk
 * @param[in] len   length of name, up to MAXPATHLEN
 * @returns the CNID, or CNID_INVALID with errno classified, CNID_ERR_NOTFOUND
 *          for a missing entry
 */
cnid_t cnid_mysql_lookup(struct _cnid_db *cdb,
                         const struct stat *st,
                         cnid_t did,
                         const char *name,
                         size_t len)
{
    EC_INIT;
    CNID_mysql_private *db;
    cnid_t id = CNID_INVALID;
    bool have_result = false;

    if (!cdb || !(db = cdb->cnid_db_private) || !st || !name) {
        LOG(log_error, logtype_cnid, "cnid_mysql_lookup: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid, "cnid_mysql_lookup: Path name is too long");
        errno = CNID_ERR_PATH;
        EC_FAIL;
    }

    uint64_t dev = 0;

    if (!(cdb->cnid_db_flags & CNID_FLAG_NODEV)) {
        dev = st->st_dev;
    }

    uint64_t ino = st->st_ino;
    cnid_t hint = db->cnid_mysql_hint;
    LOG(log_maxdebug, logtype_cnid,
        "cnid_mysql_lookup(did: %" PRIu32 ", name: \"%s\", hint: %" PRIu32 "): START",
        ntohl(did), name, ntohl(hint));
    strncpy(stmt_param_name, name, sizeof(stmt_param_name));
    stmt_param_name_len = len;
    stmt_param_did = ntohl(did);
    stmt_param_dev = dev;
    stmt_param_ino = ino;
    EC_ZERO(cnid_mysql_stmt_execute(db, &db->cnid_lookup_stmt));

    /* Classified rather than left to the caller's stale errno: CNID_INVALID
     * with errno untouched would reach cnid_mysql_add()'s not-found gate.
     * Not-found is carried as CNID_ERR_NOTFOUND, clear of the syscall errno
     * range. */
    if (mysql_stmt_store_result(db->cnid_lookup_stmt)) {
        LOG(log_error, logtype_cnid, "cnid_mysql_lookup: store_result: %s",
            mysql_stmt_error(db->cnid_lookup_stmt));
        cnid_mysql_set_errno(mysql_stmt_errno(db->cnid_lookup_stmt));
        EC_FAIL;
    }

    have_result = true;

    if (mysql_stmt_bind_result(db->cnid_lookup_stmt, lookup_result)) {
        LOG(log_error, logtype_cnid, "cnid_mysql_lookup: bind_result: %s",
            mysql_stmt_error(db->cnid_lookup_stmt));
        cnid_mysql_set_errno(mysql_stmt_errno(db->cnid_lookup_stmt));
        EC_FAIL;
    }

    uint64_t retdev, retino;
    cnid_t retid, retdid;
    const char *retname;

    switch (mysql_stmt_num_rows(db->cnid_lookup_stmt)) {
    case 0:
        /* not found */
        LOG(log_debug, logtype_cnid,
            "cnid_mysql_lookup: name: '%s', did: %u is not in the CNID database",
            name, ntohl(did));
        errno = CNID_ERR_NOTFOUND;
        EC_FAIL;

    case 1:

        /* either both OR clauses matched the same id or only one matched, handled below */
        if (mysql_stmt_fetch(db->cnid_lookup_stmt)) {
            LOG(log_error, logtype_cnid, "cnid_mysql_lookup: fetch: %s",
                mysql_stmt_error(db->cnid_lookup_stmt));
            cnid_mysql_set_errno(mysql_stmt_errno(db->cnid_lookup_stmt));
            EC_FAIL;
        }

        break;

    case 2: {
        /* a mismatch, delete both and return not found. Fetch both ids
         * before deleting: a CR_SERVER_LOST recovery inside delete
         * re-prepares every statement and would invalidate a fetch
         * still in flight on the lookup handle. */
        cnid_t mismatched[2];
        int nfetched = 0;

        while (nfetched < 2
                && mysql_stmt_fetch(db->cnid_lookup_stmt) == 0) {
            mismatched[nfetched++] = htonl((cnid_t)lookup_result_id);
        }

        for (int i = 0; i < nfetched; i++) {
            if (cnid_mysql_delete(cdb, mismatched[i])) {
                /* errno carries the delete's own classification: overwriting it
                 * would turn a lock that will free into a session-fatal error */
                LOG(log_error, logtype_cnid, "MySQL query error: %s",
                    mysql_error(db->cnid_mysql_con));
                EC_FAIL;
            }
        }

        errno = CNID_ERR_NOTFOUND;
        EC_FAIL;
    }

    default:
        /* More rows than the two UNIQUE indexes can produce: unusable data
         * rather than a failing backend, so not session-fatal */
        LOG(log_error, logtype_cnid,
            "cnid_mysql_lookup: %llu rows for did: %" PRIu32 ", name: \"%s\"",
            (unsigned long long)mysql_stmt_num_rows(db->cnid_lookup_stmt),
            ntohl(did), name);
        errno = CNID_ERR_CORRUPT;
        EC_FAIL;
    }

    retid = htonl((uint32_t)lookup_result_id);
    retdid = htonl((uint32_t)lookup_result_did);
    retname = lookup_result_name;
    retdev = lookup_result_dev;
    retino = lookup_result_ino;

    if (retdid != did || STRCMP(retname, !=, name)) {
        LOG(log_debug, logtype_cnid,
            "cnid_mysql_lookup(CNID hint: %" PRIu32 ", DID: %" PRIu32
            ", name: \"%s\"): server side mv oder reused inode",
            ntohl(hint), ntohl(did), name);

        if (hint != retid) {
            if (cnid_mysql_delete(cdb, retid) != 0) {
                /* errno carries the delete's own classification */
                LOG(log_error, logtype_cnid, "MySQL query error: %s",
                    mysql_error(db->cnid_mysql_con));
                EC_FAIL;
            }

            errno = CNID_ERR_NOTFOUND;
            EC_FAIL;
        }

        LOG(log_debug, logtype_cnid,
            "cnid_mysql_lookup: server side mv, got hint, updating");

        if (cnid_mysql_update(cdb, retid, st, did, name, len) != 0) {
            /* errno carries the update's own classification */
            LOG(log_error, logtype_cnid, "MySQL query error: %s",
                mysql_error(db->cnid_mysql_con));
            EC_FAIL;
        }

        id = retid;
    } else if (retdev != dev || retino != ino) {
        LOG(log_debug, logtype_cnid,
            "cnid_mysql_lookup(DID:%u, name: \"%s\"): changed dev/ino",
            ntohl(did), name);

        if (cnid_mysql_delete(cdb, retid) != 0) {
            /* errno carries the delete's own classification */
            LOG(log_error, logtype_cnid, "MySQL query error: %s",
                mysql_error(db->cnid_mysql_con));
            EC_FAIL;
        }

        errno = CNID_ERR_NOTFOUND;
        EC_FAIL;
    } else {
        /* everythings good */
        id = retid;
    }

EC_CLEANUP: {
        /* cnid_mysql_add() inserts only on an exact CNID_ERR_NOTFOUND, so an
         * errno clobbered by the logging or the free below would stop CNID
         * allocation rather than merely misreport the cause */
        const int saved_errno = errno;
        LOG(log_debug, logtype_cnid, "cnid_mysql_lookup: id: %" PRIu32, ntohl(id));

        /* a delete or an update above whose reprepare failed closed every
         * statement, and the stored result with it */
        if (have_result && db->cnid_lookup_stmt != NULL) {
            mysql_stmt_free_result(db->cnid_lookup_stmt);
        }

        if (ret != 0) {
            id = CNID_INVALID;
        }

        errno = saved_errno;
    }
    return id;
}

/*!
 * @brief Find the CNID of an object or add a new entry for it
 *
 * A failed lookup leads to an insert only on CNID_ERR_NOTFOUND. A usable hint
 * goes in as an explicit Id, unless a depletion emptied the table before.
 * ER_DUP_ENTRY from a racing session repeats the lookup, up to
 * CNID_MYSQL_ADD_ATTEMPTS times.
 *
 * Near CNID_MYSQL_MAX_HINT the call warns once. Past UINT32_MAX it empties
 * the table, marks the volume depleted, and fails with CNID_ERR_RESET, so the
 * session ends.
 *
 * @param[in] cdb   CNID database
 * @param[in] st    stat of the object
 * @param[in] did   CNID of the parent directory
 * @param[in] name  name on disk
 * @param[in] len   length of name, up to MAXPATHLEN
 * @param[in] hint  CNID from the metadata of the object, or CNID_INVALID
 * @returns the CNID, or CNID_INVALID with errno classified
 */
cnid_t cnid_mysql_add(struct _cnid_db *cdb,
                      const struct stat *st,
                      cnid_t did,
                      const char *name,
                      size_t len,
                      cnid_t hint)
{
    EC_INIT;
    CNID_mysql_private *db = NULL;
    cnid_t id = CNID_INVALID;
    MYSQL_STMT *stmt;
    my_ulonglong lastid;
    int attempts = 0;

    if (!cdb || !(db = cdb->cnid_db_private) || !st || !name) {
        LOG(log_error, logtype_cnid, "cnid_mysql_add: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid, "cnid_mysql_add: Path name is too long");
        errno = CNID_ERR_PATH;
        EC_FAIL;
    }

    uint64_t dev = 0;

    if (!(cdb->cnid_db_flags & CNID_FLAG_NODEV)) {
        dev = st->st_dev;
    }

    uint64_t ino = st->st_ino;
    /* The reserve gate governs inserting the hint as an explicit Id only.
     * The raw hint still drives lookup's update-in-place vs delete-and-reinsert
     * choice: existing databases hold rows in the reserve range, and a
     * server-side move must not renumber them. */
    bool hint_insertable = hint != CNID_INVALID && cnid_mysql_hint_usable(hint);

    if (hint != CNID_INVALID && !hint_insertable) {
        /* A scan over a volume whose AppleDouble metadata is uniformly out of
         * range would log this per file, so only the first is a warning */
        if (db->cnid_mysql_flags & CNID_MYSQL_FLAG_HINT_RANGE_LOGGED) {
            LOG(log_debug, logtype_cnid,
                "cnid_mysql_add: not inserting out-of-range CNID hint %" PRIu32
                " for name: \"%s\"", ntohl(hint), name);
        } else {
            db->cnid_mysql_flags |= CNID_MYSQL_FLAG_HINT_RANGE_LOGGED;
            LOG(log_warning, logtype_cnid,
                "cnid_mysql_add: not inserting out-of-range CNID hint %" PRIu32
                " for name: \"%s\" (further occurrences logged at debug level)",
                ntohl(hint), name);
        }
    }

    db->cnid_mysql_hint = hint;
    LOG(log_maxdebug, logtype_cnid,
        "cnid_mysql_add(did: %" PRIu32 ", name: \"%s\", hint: %" PRIu32 "): START",
        ntohl(did), name, ntohl(hint));

    do {
        if ((id = cnid_mysql_lookup(cdb, st, did, name, len)) == CNID_INVALID) {
            /* Only a genuine "not in the database" answer justifies an insert.
             * Any other failure — contention, a corrupt row, a bad parameter —
             * cannot be resolved by inserting, so retrying would waste an
             * attempt and then report the wrong cause. errno carries the
             * lookup's classification to the caller unchanged. */
            if (errno != CNID_ERR_NOTFOUND) {
                LOG(log_error, logtype_cnid,
                    "cnid_mysql_add: lookup failed for did: %" PRIu32
                    ", name: \"%s\", errno=%d", ntohl(did), name, errno);
                EC_FAIL;
            }

            /*
             * The CNID "hint" from the AppleDouble file is only bound as an
             * explicit Id when in range and the set has not overflowed
             * before (CNID_MYSQL_FLAG_DEPLETED).
             */
            if (!db->cnid_mysql_hint || !hint_insertable
                    || (db->cnid_mysql_flags & CNID_MYSQL_FLAG_DEPLETED)) {
                stmt = db->cnid_add_stmt;
            } else {
                stmt = db->cnid_put_stmt;
                stmt_param_id = ntohl(db->cnid_mysql_hint);
            }

            strncpy(stmt_param_name, name, sizeof(stmt_param_name));
            stmt_param_name_len = len;
            stmt_param_did = ntohl(did);
            stmt_param_dev = dev;
            stmt_param_ino = ino;

            if (db->cnid_mysql_folded) {
                cnid_mysql_fold_param(name, len,
                                      cdb->cnid_db_vol->v_volcharset);
            }

            if (mysql_stmt_execute(stmt)) {
                switch (mysql_stmt_errno(stmt)) {
                case ER_DUP_ENTRY:
                    break;

                case CR_SERVER_LOST:
                    close_prepared_stmt(db);
                    EC_ZERO(init_prepared_stmt(db));
                    continue;

                default:
                    LOG(log_error, logtype_cnid, "cnid_mysql_add: %s",
                        mysql_stmt_error(stmt));
                    cnid_mysql_set_errno(mysql_stmt_errno(stmt));
                    EC_FAIL;
                }

                /*
                 * Race condition:
                 * between lookup and insert another process may have inserted
                 * this entry.
                 */
                if (db->cnid_mysql_hint) {
                    db->cnid_mysql_hint = CNID_INVALID;
                }

                continue;
            }

            lastid = mysql_stmt_insert_id(stmt);

            if (lastid >= (my_ulonglong) CNID_MYSQL_MAX_HINT
                    && !(db->cnid_mysql_flags & CNID_MYSQL_FLAG_NEAR_DEPLETION)) {
                db->cnid_mysql_flags |= CNID_MYSQL_FLAG_NEAR_DEPLETION;
                LOG(log_warning, logtype_cnid,
                    "cnid_mysql_add: volume '%s' has reached CNID %llu"
                    " of %" PRIu32 "; when the ceiling is reached the CNID table "
                    "is emptied and every session on the volume is "
                    "disconnected. Rebuild with dbd to reclaim the range gracefully.",
                    cdb->cnid_db_vol->v_localname, (unsigned long long) lastid,
                    UINT32_MAX);
            }

            if (lastid > UINT32_MAX) {
                /* CNID set ist depleted, restart from scratch */
                LOG(log_warning, logtype_cnid,
                    "cnid_mysql_add: CNID set is depleted, emptying the CNID "
                    "table for volume '%s'; entries are rebuilt on access",
                    cdb->cnid_db_vol->v_localname);
                /* Checked before the table is called reset: announcing a reset
                 * of a table still holding its rows, with the sequence not
                 * reseeded, would disconnect every session and do it again on
                 * the next insert. */
                EC_NEG1(cnid_mysql_empty(db, 1));
                db->cnid_mysql_flags |= CNID_MYSQL_FLAG_DEPLETED;
                /* CNIDs issued before the reset now name nothing, or name a
                 * different file as the table refills. Report rather than
                 * retry: the caller ends the session. */
                errno = CNID_ERR_RESET;
                EC_FAIL;
            }

            /* Finally assign our result */
            id = htonl((uint32_t)lastid);
        }
    } while (id == CNID_INVALID && ++attempts < CNID_MYSQL_ADD_ATTEMPTS);

    if (id == CNID_INVALID) {
        /* Every attempt ended in a duplicate-key collision that the following
         * lookup then failed to find: a row owns the name or the dev/ino pair
         * but is not reachable through either index. */
        LOG(log_error, logtype_cnid,
            "cnid_mysql_add: giving up after %d attempts for did: %" PRIu32
            ", name: \"%s\": colliding row is not reachable by lookup",
            attempts, ntohl(did), name);
        errno = CNID_ERR_CORRUPT;
        EC_FAIL;
    }

EC_CLEANUP: {
        /* Callers read errno to tell a table reset from an ordinary failure */
        const int saved_errno = errno;
        LOG(log_debug, logtype_cnid, "cnid_mysql_add: id: %" PRIu32, ntohl(id));

        if (db) {
            /* The hint channel spans only this add: a later direct lookup
             * acting on a stale hint would rewrite an unrelated row's identity
             * in the server-side-move branch */
            db->cnid_mysql_hint = CNID_INVALID;
        }

        errno = saved_errno;
    }
    return id;
}

/*!
 * @brief Look up the CNID that a directory holds under a name
 *
 * A contended database reports a classified errno, not a missing name, which
 * the caller would turn into a permanent AFPERR_NOOBJ.
 *
 * @param[in] cdb   CNID database
 * @param[in] did   CNID of the directory
 * @param[in] name  name on disk
 * @param[in] len   length of name, up to MAXPATHLEN
 * @returns the CNID, or CNID_INVALID with errno CNID_ERR_NOTFOUND for a missing
 *          name
 */
cnid_t cnid_mysql_get(struct _cnid_db *cdb, cnid_t did, const char *name,
                      size_t len)
{
    EC_INIT;
    CNID_mysql_private *db;
    cnid_t id = CNID_INVALID;
    bool have_result = false;

    if (!cdb || !(db = cdb->cnid_db_private) || !name) {
        LOG(log_error, logtype_cnid, "cnid_mysql_get: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid, "cnid_mysql_get: name is too long");
        errno = CNID_ERR_PATH;
        return CNID_INVALID;
    }

    LOG(log_debug, logtype_cnid,
        "cnid_mysql_get(did: %" PRIu32 ", name: \"%s\"): START",
        ntohl(did), name);
    strncpy(stmt_param_name, name, sizeof(stmt_param_name));
    stmt_param_name_len = len;
    stmt_param_did = ntohl(did);
    EC_ZERO(cnid_mysql_stmt_execute(db, &db->cnid_get_stmt));

    /* A contended database must not be reported as a missing name: the caller
     * turns that into a permanent AFPERR_NOOBJ for the client */
    if (mysql_stmt_store_result(db->cnid_get_stmt)) {
        LOG(log_error, logtype_cnid, "cnid_mysql_get: store_result: %s",
            mysql_stmt_error(db->cnid_get_stmt));
        cnid_mysql_set_errno(mysql_stmt_errno(db->cnid_get_stmt));
        EC_FAIL;
    }

    have_result = true;

    if (mysql_stmt_bind_result(db->cnid_get_stmt, get_result)) {
        LOG(log_error, logtype_cnid, "cnid_mysql_get: bind_result: %s",
            mysql_stmt_error(db->cnid_get_stmt));
        cnid_mysql_set_errno(mysql_stmt_errno(db->cnid_get_stmt));
        EC_FAIL;
    }

    if (mysql_stmt_num_rows(db->cnid_get_stmt)) {
        if (mysql_stmt_fetch(db->cnid_get_stmt)) {
            LOG(log_error, logtype_cnid, "cnid_mysql_get: fetch: %s",
                mysql_stmt_error(db->cnid_get_stmt));
            cnid_mysql_set_errno(mysql_stmt_errno(db->cnid_get_stmt));
            EC_FAIL;
        }

        id = htonl((uint32_t)get_result_id);
    } else {
        errno = CNID_ERR_NOTFOUND;
    }

EC_CLEANUP:
    LOG(log_debug, logtype_cnid, "cnid_mysql_get: id: %" PRIu32, ntohl(id));

    if (have_result) {
        mysql_stmt_free_result(db->cnid_get_stmt);
    }

    return id;
}

/*!
 * @brief Resolve a CNID to its parent and name in the MySQL table
 *
 * A contended database reports a classified errno, not a missing CNID, which
 * afp_resolveid would turn into a permanent AFPERR_NOID.
 *
 * @param[in]     cdb     CNID database
 * @param[in,out] id      CNID to resolve, receives the parent CNID or
 *                        CNID_INVALID
 * @param[out]    buffer  receives the name
 * @param[in]     len     size of buffer
 * @returns buffer, or NULL with errno classified
 */
char *cnid_mysql_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer,
                         size_t len)
{
    EC_INIT;
    CNID_mysql_private *db;
    bool have_result = false;

    if (!cdb || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid, "cnid_mysql_get: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    stmt_param_id = ntohl(*id);
    EC_ZERO(cnid_mysql_stmt_execute(db, &db->cnid_resolve_stmt));

    /* A contended database must not be reported as a missing id: afp_resolveid
     * turns that into a permanent AFPERR_NOID and the client gives up on the
     * alias, where a retry would have succeeded */
    if (mysql_stmt_store_result(db->cnid_resolve_stmt)) {
        LOG(log_error, logtype_cnid, "cnid_mysql_resolve: store_result: %s",
            mysql_stmt_error(db->cnid_resolve_stmt));
        cnid_mysql_set_errno(mysql_stmt_errno(db->cnid_resolve_stmt));
        EC_FAIL;
    }

    have_result = true;

    if (mysql_stmt_bind_result(db->cnid_resolve_stmt, resolve_result)) {
        LOG(log_error, logtype_cnid, "cnid_mysql_resolve: bind_result: %s",
            mysql_stmt_error(db->cnid_resolve_stmt));
        cnid_mysql_set_errno(mysql_stmt_errno(db->cnid_resolve_stmt));
        EC_FAIL;
    }

    if (mysql_stmt_num_rows(db->cnid_resolve_stmt) != 1) {
        errno = CNID_ERR_NOTFOUND;
        EC_FAIL;
    }

    if (mysql_stmt_fetch(db->cnid_resolve_stmt)) {
        LOG(log_error, logtype_cnid, "cnid_mysql_resolve: fetch: %s",
            mysql_stmt_error(db->cnid_resolve_stmt));
        cnid_mysql_set_errno(mysql_stmt_errno(db->cnid_resolve_stmt));
        EC_FAIL;
    }

    /* The connector is not guaranteed to NUL-terminate string results;
     * Name is VARCHAR(255) against a MAXPATHLEN buffer, so there is room. */
    resolve_result_name[resolve_result_name_len] = '\0';
    *id = htonl((uint32_t)resolve_result_did);
    strncpy(buffer, resolve_result_name, len);
EC_CLEANUP:

    if (have_result) {
        mysql_stmt_free_result(db->cnid_resolve_stmt);
    }

    if (ret != 0) {
        *id = CNID_INVALID;
        return NULL;
    }

    return buffer;
}

/*!
 * @brief Read the stamp of a volume from the volumes table
 *
 * The stamp is the time the volume first entered the table.
 *
 * @param[in]  cdb     CNID database
 * @param[out] buffer  receives len bytes of the stamp
 * @param[in]  len     bytes to read
 * @returns 0, or -1 on error
 */
int cnid_mysql_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
{
    EC_INIT;
    CNID_mysql_private *db;
    char *sql = NULL;
    MYSQL_RES *result = NULL;
    MYSQL_ROW row;

    /* CNID_INVALID is 0, which this int-returning function's callers read as
     * success, so parameter failures have to return -1 like every other path */
    if (!cdb || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid, "cnid_mysql_getstamp: Parameter error");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    if (!buffer) {
        LOG(log_error, logtype_cnid, "cnid_mysql_getstamp: bad buffer");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    /* by the key: a path that an older build stored unescaped reads back
     * other than the volume's, and its open keeps that row */
    EC_NEG1(asprintf(&sql, "SELECT Stamp FROM volumes WHERE VolUUID='%s'",
                     db->cnid_mysql_voluuid_str));

    if (cnid_mysql_execute(db->cnid_mysql_con, sql)
            && mysql_errno(db->cnid_mysql_con) != ER_DUP_ENTRY) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s",
            mysql_error(db->cnid_mysql_con));
        EC_FAIL;
    }

    free(sql);
    sql = NULL;

    if ((result = mysql_store_result(db->cnid_mysql_con)) == NULL) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s",
            mysql_error(db->cnid_mysql_con));
        cnid_mysql_set_errno(mysql_errno(db->cnid_mysql_con));
        EC_FAIL;
    }

    if (!mysql_num_rows(result)) {
        LOG(log_error, logtype_cnid, "Can't get DB stamp for volumes \"%s\"",
            cdb->cnid_db_vol->v_path);
        EC_FAIL;
    }

    row = mysql_fetch_row(result);
    memcpy(buffer, row[0], len);
EC_CLEANUP:

    if (result) {
        mysql_free_result(result);
    }

    if (sql) {
        free(sql);
    }

    EC_EXIT;
}

/* Rows one batch of a volume search fetches at most */
#define CNID_MYSQL_FIND_BATCH 65536

/*!
 * @brief The LIKE pattern a name search sends the server for a seed
 *
 * Folded1 compares bytes, so it takes the seed's pattern over folded names;
 * Name takes the pattern that holds under any collation. Every name the seed
 * matches satisfies the pattern of either column.
 *
 * @param[in] seed    the seed
 * @param[in] folded  whether the pattern compares the folded names
 * @returns the pattern, which the seed holds
 */
static const char *cnid_mysql_like(const struct sl_name_pattern *seed,
                                   bool folded)
{
    return folded ? sl_name_pattern_index_like(seed)
           : sl_name_pattern_like(seed);
}

/*!
 * @brief Tell whether a name search compares the folded names
 *
 * A folder search compares Name, which its walk reads from the DidName
 * index, where the folded names would cost a read of each row of the folder.
 *
 * @param[in] db         MySQL backend state
 * @param[in] scope_did  the search's folder, or CNID_INVALID for the volume
 * @returns true when the table holds folded names and the search covers the
 *          volume, as every search does on a server without recursive CTEs
 */
static bool cnid_mysql_find_folded(const CNID_mysql_private *db,
                                   cnid_t scope_did)
{
    return db->cnid_mysql_folded
           && (scope_did == CNID_INVALID || db->cnid_find_scoped_unsupported);
}

/*!
 * @brief Tell whether a seed's LIKE pattern narrows a name search
 *
 * @param[in] cdb        CNID database
 * @param[in] seed       the seed
 * @param[in] scope_did  the search's folder, or CNID_INVALID for the volume
 * @returns true when the pattern holds a character other than '%'
 */
static bool cnid_mysql_find_narrows(struct _cnid_db *cdb,
                                    const struct sl_name_pattern *seed,
                                    cnid_t scope_did)
{
    const CNID_mysql_private *db = cdb->cnid_db_private;
    const char *like = cnid_mysql_like(seed,
                                       cnid_mysql_find_folded(db, scope_did));
    return like[strspn(like, "%")] != '\0';
}

/*!
 * @brief Tell whether a seed's folded-name pattern narrows a volume search
 *
 * For the volume search a folder search falls back to.
 *
 * @param[in] seed  the seed
 * @param[in] ctx   unused
 * @returns true when the pattern holds a character other than '%'
 */
static bool cnid_mysql_folded_narrows(const struct sl_name_pattern *seed,
                                      void *ctx)
{
    const char *like = sl_name_pattern_index_like(seed);
    (void)ctx;
    return like[strspn(like, "%")] != '\0';
}

/*!
 * @brief Write the WHERE condition of a name search over its LIKE patterns
 *
 * The condition keeps the rows whose names one pattern keeps, every row for
 * none, and a row without a folded name as a candidate for the matcher.
 *
 * @param[in]  folded  whether the patterns compare the folded names
 * @param[in]  npat    LIKE patterns, 0 for every row
 * @param[out] cond    receives the condition
 * @param[in]  size    room in cond
 * @returns true when the condition fits
 */
static bool cnid_mysql_find_cond(bool folded, int npat, char *cond,
                                 size_t size)
{
    const char *column = folded ? CNID_MYSQL_FOLDED_NAME : "Name";
    size_t n = 0;

    if (npat == 0) {
        snprintf(cond, size, "1");
        return true;
    }

    for (int i = 0; i < npat && n < size; i++) {
        n += (size_t)snprintf(cond + n, size - n, "%st.%s LIKE ? ESCAPE '!'",
                              i ? " OR " : "(", column);
    }

    if (n < size) {
        n += (size_t)snprintf(cond + n, size - n, "%s)",
                              folded ? " OR t." CNID_MYSQL_FOLDED_NAME
                              " IS NULL" : "");
    }

    return n < size;
}

/*!
 * @brief Prepare a name search: a batch of the volume, or a folder's subtree
 *
 * A volume search takes the rows after an Id whose names a LIKE pattern
 * keeps. A folder search walks its subtree with a recursive CTE once and
 * streams the rows unordered: a batch after the last Id would walk and sort
 * the whole subtree again for every batch. A server that cannot parse the
 * CTE latches the unscoped search for the connection.
 *
 * @param[in,out] db      MySQL backend state
 * @param[in]     stmt    the statement
 * @param[in]     npat    LIKE patterns, 0 for every name
 * @param[in]     folded  whether the patterns compare the folded names
 * @param[in,out] scoped  whether the search is scoped, cleared on a fallback
 * @returns 0, or -1 when the prepare fails
 */
static int cnid_mysql_find_prepare(CNID_mysql_private *db, MYSQL_STMT *stmt,
                                   int npat, bool folded, bool *scoped)
{
    char cond[CNID_FIND_MAX_SEEDS * 48 + 32];
    char *sql = NULL;
    int ret = -1;

    /* cond has room for CNID_FIND_MAX_SEEDS patterns */
    if (!cnid_mysql_find_cond(folded, npat, cond, sizeof(cond))) {
        return -1;
    }

    if (*scoped) {
        /* STRAIGHT_JOIN keeps the scope the outer loop, so the cost follows
         * the subtree, not the volume; the CTE needs MySQL 8.0 or MariaDB
         * 10.2 */
        if (asprintf(&sql,
                     "WITH RECURSIVE scope (Id) AS ("
                     "  SELECT CAST(? AS UNSIGNED)"
                     "  UNION"
                     "  SELECT t.Id FROM `%s` t JOIN scope s ON t.Did = s.Id"
                     ") "
                     "SELECT t.Id, t.Name FROM scope s "
                     "STRAIGHT_JOIN `%s` t ON t.Did = s.Id WHERE %s",
                     db->cnid_mysql_voluuid_str,
                     db->cnid_mysql_voluuid_str, cond) < 0) {
            return -1;
        }

        LOG(log_maxdebug, logtype_cnid, "cnid_mysql_find: SQL query: %s", sql);
        ret = mysql_stmt_prepare(stmt, sql, strlen(sql));
        free(sql);
        sql = NULL;

        if (ret == 0) {
            return 0;
        }

        /*
         * Only a parse rejection means the server lacks recursive CTEs;
         * latch the fallback for that alone.
         */
        if (mysql_stmt_errno(stmt) != 1064 /* ER_PARSE_ERROR */) {
            LOG(log_error, logtype_cnid, "cnid_mysql_find: prepare failed: %s",
                mysql_stmt_error(stmt));
            return -1;
        }

        LOG(log_warning, logtype_cnid,
            "cnid_mysql_find: server lacks recursive-CTE support (%s); "
            "falling back to unscoped searches for this connection",
            mysql_stmt_error(stmt));
        db->cnid_find_scoped_unsupported = true;
        *scoped = false;
        ret = -1;
    }

    if (asprintf(&sql,
                 "SELECT t.Id, t.Name FROM `%s` t WHERE %s AND t.Id > ? "
                 "ORDER BY t.Id LIMIT ?", db->cnid_mysql_voluuid_str,
                 cond) < 0) {
        return -1;
    }

    LOG(log_maxdebug, logtype_cnid, "cnid_mysql_find: SQL query: %s", sql);

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)) == 0) {
        ret = 0;
    } else {
        LOG(log_error, logtype_cnid, "cnid_mysql_find: prepare failed: %s",
            mysql_stmt_error(stmt));
    }

    free(sql);
    return ret;
}

/*!
 * @brief Tell whether a folder search's walk stopped short of its subtree
 *
 * MariaDB ends a recursion past max_recursive_iterations with the rows so far
 * and warning 1931, which the client library does not count for a prepared
 * statement, so SHOW WARNINGS tells. A server that keeps fewer warnings than
 * it counts, under max_error_count, may have dropped it.
 *
 * @param[in] con  MySQL connection
 * @returns true when the walk stopped short or may have, or its warnings do
 *          not read
 */
static bool cnid_mysql_cut_short(MYSQL *con)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    unsigned long long kept;
    bool cut = false;

    if (mysql_query(con, "SHOW WARNINGS") != 0
            || (res = mysql_store_result(con)) == NULL) {
        return true;
    }

    kept = mysql_num_rows(res);

    while (!cut && (row = mysql_fetch_row(res)) != NULL) {
        cut = row[1] != NULL && strcmp(row[1], "1931") == 0;
    }

    mysql_free_result(res);

    if (cut || mysql_query(con, "SHOW COUNT(*) WARNINGS") != 0
            || (res = mysql_store_result(con)) == NULL) {
        return true;
    }

    row = mysql_fetch_row(res);
    cut = row == NULL || row[0] == NULL || strtoull(row[0], NULL, 10) > kept;
    mysql_free_result(res);
    return cut;
}

/*!
 * @brief Bind a name search's parameters and its result columns
 *
 * @param[in] stmt    the prepared search
 * @param[in] scoped  whether the search takes a scope, and no batches
 * @param[in] scope   CNID of the scope directory in host order
 * @param[in] binary  whether the patterns compare bytes of folded names
 * @param[in] likes   the LIKE patterns
 * @param[in] lens    bytes of each pattern
 * @param[in] npat    number of patterns
 * @param[in] after   Id the rows of a batch follow
 * @param[in] batch   rows one batch fetches
 * @param[in] result  the Id and the name of a row
 * @returns 0, or -1 when a bind fails
 */
static int cnid_mysql_find_bind(MYSQL_STMT *stmt, bool scoped,
                                unsigned long long *scope, bool binary,
                                const char *const *likes, unsigned long *lens,
                                int npat,
                                unsigned long long *after,
                                unsigned long long *batch, MYSQL_BIND *result)
{
    MYSQL_BIND param[CNID_FIND_MAX_SEEDS + 3];
    int i = 0;
    memset(param, 0, sizeof(param));

    if (scoped) {
        param[i].buffer_type = MYSQL_TYPE_LONGLONG;
        param[i].buffer = scope;
        param[i++].is_unsigned = true;
    }

    for (int k = 0; k < npat; k++, i++) {
        param[i].buffer_type = binary ? MYSQL_TYPE_BLOB : MYSQL_TYPE_STRING;
        param[i].buffer = (void *)likes[k]; //NOSONAR: MySQL only reads a parameter's buffer
        param[i].buffer_length = lens[k];
        param[i].length = &lens[k];
    }

    if (!scoped) {
        param[i].buffer_type = MYSQL_TYPE_LONGLONG;
        param[i].buffer = after;
        param[i++].is_unsigned = true;
        param[i].buffer_type = MYSQL_TYPE_LONGLONG;
        param[i].buffer = batch;
        param[i].is_unsigned = true;
    }

    if (mysql_stmt_bind_param(stmt, param) != 0
            || mysql_stmt_bind_result(stmt, result) != 0) {
        LOG(log_error, logtype_cnid, "cnid_mysql_find: bind failed: %s",
            mysql_stmt_error(stmt));
        return -1;
    }

    return 0;
}

/*!
 * @brief Tell whether a row's Id is a CNID, logging one that is not
 *
 * A corrupt row's match drops out of the search.
 *
 * @param[in] id  the Id
 * @returns true for 1 to UINT32_MAX
 */
static bool cnid_mysql_id_ok(unsigned long long id)
{
    if (id != 0 && id <= UINT32_MAX) {
        return true;
    }

    LOG(log_error, logtype_cnid,
        "cnid_mysql_find: out of range id %llu in a name search, skipped", id);
    return false;
}

/*!
 * @brief Keep the lowest Ids of an unordered stream of matches, in a max-heap
 *
 * @param[in,out] heap  the Ids kept, the largest first
 * @param[in,out] n     Ids in heap
 * @param[in]     cap   room in heap
 * @param[in]     id    the Id of a match
 * @returns true when heap was full, so the search holds more matches than it
 *          keeps
 */
static bool cnid_mysql_lowest(uint32_t *heap, int *n, int cap, uint32_t id)
{
    int i;

    if (*n < cap) {
        for (i = (*n)++; i > 0 && heap[(i - 1) / 2] < id; i = (i - 1) / 2) {
            heap[i] = heap[(i - 1) / 2];
        }

        heap[i] = id;
        return false;
    }

    if (id < heap[0]) {
        for (i = 0;;) {
            int c = 2 * i + 1;

            if (c >= cap) {
                break;
            }

            if (c + 1 < cap && heap[c + 1] > heap[c]) {
                c++;
            }

            if (heap[c] <= id) {
                break;
            }

            heap[i] = heap[c];
            i = c;
        }

        heap[i] = id;
    }

    return true;
}

/*!
 * @brief Order two host-order Ids
 *
 * @param[in] a  an Id
 * @param[in] b  another
 * @returns -1, 0 or 1
 */
static int cnid_mysql_id_order(const void *a, const void *b)
{
    uint32_t x = *(const uint32_t *)a;
    uint32_t y = *(const uint32_t *)b;
    return x < y ? -1 : x > y;
}

/* Folder verdicts a folder search over the volume keeps, by CNID; a
 * collision costs only another walk */
#define CNID_MYSQL_SCOPE_CACHE 1024

/* A folder search the server cannot walk: its folder and the verdicts its
 * walks keep */
struct cnid_mysql_scope {
    cnid_t scope;
    struct {
        cnid_t did;
        bool under;
    } seen[CNID_MYSQL_SCOPE_CACHE];
};

/*!
 * @brief Tell whether an entry lies under a folder search's folder
 *
 * The walk ends at the folder, at the root, at a folder whose verdict a walk
 * kept, or past MAXPATHLEN / 2 folders, as many as a path of MAXPATHLEN bytes
 * holds, which only a loop in the table makes and which leaves the entry
 * outside. Each folder it passed then keeps the verdict, so a loop costs one
 * walk a search. An entry whose walk meets a missing row is outside and
 * keeps no verdict, as a rename deletes a row before it adds it.
 *
 * @param[in]     cdb  CNID database
 * @param[in,out] sc   the folder and the verdicts
 * @param[in]     id   CNID of the entry, in network order
 * @returns 1 when the entry lies under the folder, the folder itself not, 0
 *          when it lies outside, -1 when a resolve fails otherwise than for a
 *          missing row, errno classified
 */
static int cnid_mysql_under(struct _cnid_db *cdb, struct cnid_mysql_scope *sc,
                            cnid_t id)
{
    cnid_t path[MAXPATHLEN / 2];
    char name[12 + MAXPATHLEN + 1];
    size_t n = 0;
    bool under = false;
    bool known = false;
    cnid_t did = id;

    while (!known && n < MAXPATHLEN / 2) {
        size_t slot;

        if (cnid_mysql_resolve(cdb, &did, name, sizeof(name)) == NULL) {
            return errno == CNID_ERR_NOTFOUND ? 0 : -1;
        }

        slot = ntohl(did) % CNID_MYSQL_SCOPE_CACHE;

        if (did == sc->scope || ntohl(did) < CNID_START) {
            under = did == sc->scope;
            known = true;
        } else if (sc->seen[slot].did == did) {
            under = sc->seen[slot].under;
            known = true;
        } else {
            path[n++] = did;
        }
    }

    for (size_t i = 0; i < n; i++) {
        const size_t slot = ntohl(path[i]) % CNID_MYSQL_SCOPE_CACHE;
        sc->seen[slot].did = path[i];
        sc->seen[slot].under = under;
    }

    return under;
}

/*!
 * @brief Find the CNIDs whose names match a query, for cnid_find
 *
 * The cnid_find wrapper validates the arguments. One statement takes the rows
 * whose names one of the seeds' LIKE patterns keeps, every row without seeds,
 * and the query decides each name; the patterns bind as statement
 * parameters, never as SQL text. A volume search reads the rows after the
 * last Id the previous batch read, the first batch 1 more than buffer holds
 * and each next eight times larger, CNID_MYSQL_FIND_BATCH at most, until a
 * match past buffer sets more_available. A folder search walks its subtree
 * with a recursive CTE once and streams the rows unordered, keeping the
 * lowest Ids that match, the matches a volume search finds; it compares Name,
 * as cnid_mysql_find_folded() says. A server without CTE support, a failed
 * scoped execute, or a walk cut short with a warning, as MariaDB's past
 * max_recursive_iterations, makes it a volume search that keeps the matches
 * cnid_mysql_under() places under the folder, before they take room in
 * buffer. That search compares the folded names where the table holds them,
 * except on the search that finds the server without CTEs.
 *
 * @param[in]  cdb             CNID database
 * @param[in]  query           the name comparisons
 * @param[in]  seeds           the query's seeds, or NULL for every name
 * @param[in]  nseeds          number of seeds
 * @param[in]  scope_did       CNID of the scope directory, or CNID_INVALID for
 *                             all of the volume
 * @param[out] buffer          receives the CNIDs in network order
 * @param[in]  buflen          size of buffer
 * @param[out] more_available  receives true when more matches exist, or NULL
 * @returns the number of CNIDs, or -1 on error
 */
int cnid_mysql_find(struct _cnid_db *cdb, const struct sl_name_query *query,
                    const struct sl_name_pattern *const *seeds, int nseeds,
                    cnid_t scope_did, void *buffer, size_t buflen,
                    bool *more_available)
{
    EC_INIT;
    CNID_mysql_private *db = cdb->cnid_db_private;
    const struct sl_name_pattern *folded_seeds[CNID_FIND_MAX_SEEDS];
    const char *likes[CNID_FIND_MAX_SEEDS];
    unsigned long lens[CNID_FIND_MAX_SEEDS];
    MYSQL_STMT *stmt = NULL;
    MYSQL_BIND result[2];
    struct cnid_mysql_scope *sc = NULL;
    int under = 0;
    char name[MAXPATHLEN + 1];
    unsigned long name_len = 0;
    unsigned long long result_id = 0;
    unsigned long long scope = ntohl(scope_did);
    unsigned long long after = 0;
    unsigned long long batch;
    cnid_t *cnids = (cnid_t *)buffer;
    unsigned long max_results = buflen / sizeof(cnid_t);
    bool scoped = (scope_did != CNID_INVALID
                   && !db->cnid_find_scoped_unsupported);
    bool folded = cnid_mysql_find_folded(db, scope_did);
    bool more = false;
    bool walked = false;
    int npat = seeds != NULL ? nseeds : 0;
    int fetch_ret = 0;
    int count = 0;

    /* Parameters pre-validated by the libatalk/cnid/cnid.c wrapper:
     *   cdb, query non-NULL; buflen >= CNID_FIND_MIN_BUFLEN.
     * Re-checking here would risk emitting a different errno value than
     * the wrapper for the same input. */

    if (more_available) {
        *more_available = false;
    }

    for (int i = 0; i < npat; i++) {
        likes[i] = cnid_mysql_like(seeds[i], folded);
        lens[i] = strlen(likes[i]);
        LOG(log_debug, logtype_cnid, "cnid_mysql_find: LIKE pattern is '%s'",
            likes[i]);
    }

    batch = MIN(max_results + 1, CNID_MYSQL_FIND_BATCH);
    memset(result, 0, sizeof(result));
    result[0].buffer_type = MYSQL_TYPE_LONGLONG;
    result[0].buffer = &result_id;
    result[0].is_unsigned = true;
    result[1].buffer_type = MYSQL_TYPE_STRING;
    result[1].buffer = name;
    result[1].buffer_length = sizeof(name);
    result[1].length = &name_len;
    EC_NULL(stmt = mysql_stmt_init(db->cnid_mysql_con));
    EC_ZERO(cnid_mysql_find_prepare(db, stmt, npat, folded, &scoped));
    EC_ZERO(cnid_mysql_find_bind(stmt, scoped, &scope, folded, likes, lens,
                                 npat, &after, &batch, result));

    if (scoped && mysql_stmt_execute(stmt) == 0) {
        /* Unbuffered, so the rows come as the fetches read them */
        while ((fetch_ret = mysql_stmt_fetch(stmt)) == 0) {
            if (cnid_mysql_id_ok(result_id)
                    && sl_name_query_match(query, name, name_len)
                    && cnid_mysql_lowest((uint32_t *)cnids, &count,
                                         (int)max_results,
                                         (uint32_t)result_id)) {
                more = true;
            }
        }

        if (fetch_ret != MYSQL_NO_DATA) {
            LOG(log_error, logtype_cnid,
                "cnid_mysql_find: MySQL error after fetch: %s",
                mysql_stmt_error(stmt));
            mysql_stmt_free_result(stmt);
            EC_FAIL;
        }

        mysql_stmt_free_result(stmt);
        walked = !cnid_mysql_cut_short(db->cnid_mysql_con);
    }

    if (walked) {
        qsort(cnids, (size_t)count, sizeof(cnids[0]), cnid_mysql_id_order);

        for (int i = 0; i < count; i++) {
            cnids[i] = htonl(cnids[i]);
        }
    } else {
        if (scoped) {
            /* e.g. the server's CTE recursion-depth cap on an unusually
             * deep subtree */
            LOG(log_warning, logtype_cnid,
                "cnid_mysql_find: scoped query failed or stopped short (%s); "
                "retrying unscoped", mysql_stmt_error(stmt));
            scoped = false;
            count = 0;
            more = false;

            /* the folded patterns pick the seeds again: they can narrow the
             * volume where Name's could not */
            if (db->cnid_mysql_folded) {
                folded = true;
                npat = sl_name_query_seeds(query, folded_seeds,
                                           CNID_FIND_MAX_SEEDS,
                                           cnid_mysql_folded_narrows, NULL);

                for (int i = 0; i < npat; i++) {
                    likes[i] = sl_name_pattern_index_like(folded_seeds[i]);
                    lens[i] = strlen(likes[i]);
                }
            }

            EC_ZERO(cnid_mysql_find_prepare(db, stmt, npat, folded, &scoped));
            EC_ZERO(cnid_mysql_find_bind(stmt, scoped, &scope, folded, likes,
                                         lens, npat, &after, &batch, result));
        }

        if (scope_did != CNID_INVALID) {
            EC_NULL(sc = calloc(1, sizeof(*sc)));
            sc->scope = scope_did;
        }

        while (count <= (int)max_results) {
            unsigned long long rows = 0;

            if (mysql_stmt_execute(stmt) != 0) {
                LOG(log_error, logtype_cnid,
                    "cnid_mysql_find: execute failed: %s",
                    mysql_stmt_error(stmt));
                EC_FAIL;
            }

            EC_ZERO_LOG(mysql_stmt_store_result(stmt));

            while ((fetch_ret = mysql_stmt_fetch(stmt)) == 0) {
                rows++;
                after = result_id;

                if (!cnid_mysql_id_ok(result_id)
                        || !sl_name_query_match(query, name, name_len)) {
                    continue;
                }

                if (sc != NULL
                        && (under = cnid_mysql_under(cdb, sc,
                                                     htonl((uint32_t)result_id)))
                        != 1) {
                    if (under < 0) {
                        mysql_stmt_free_result(stmt);
                        EC_FAIL;
                    }

                    continue;
                }

                if (count < (int)max_results) {
                    cnids[count] = htonl((uint32_t)result_id);
                }

                if (++count > (int)max_results) {
                    break;
                }
            }

            if (fetch_ret != 0 && fetch_ret != MYSQL_NO_DATA) {
                LOG(log_error, logtype_cnid,
                    "cnid_mysql_find: MySQL error after fetch: %s",
                    mysql_stmt_error(stmt));
                mysql_stmt_free_result(stmt);
                EC_FAIL;
            }

            mysql_stmt_free_result(stmt);

            if (rows < batch) {
                break;
            }

            batch = MIN(batch * 8, CNID_MYSQL_FIND_BATCH);
        }

        if (count > (int)max_results) {
            more = true;
            count = (int)max_results;
        }
    }

    if (more && more_available) {
        *more_available = true;
    }

    LOG(log_debug, logtype_cnid, "cnid_mysql_find: returning %d matches", count);
EC_CLEANUP:
    free(sc);

    if (stmt) {
        mysql_stmt_close(stmt);
    }

    if (ret != 0) {
        count = -1;
        LOG(log_debug, logtype_cnid,
            "cnid_mysql_find: returning error, count set to -1");
    }

    return count;
}

/*!
 * @brief Empty the MySQL table of a volume and restart its CNIDs at 17
 *
 * The call also clears the depletion marks of the volume and of the
 * connection. A dbd rebuild then restores the CNIDs from the metadata again.
 *
 * @param[in] cdb  CNID database
 * @returns 0, or -1 with errno classified
 */
int cnid_mysql_wipe(struct _cnid_db *cdb)
{
    EC_INIT;
    CNID_mysql_private *db;

    if (!cdb || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid, "cnid_wipe: Parameter error");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    LOG(log_debug, logtype_cnid, "cnid_mysql_wipe");
    /* Before the latch is cleared: a caller told the wipe succeeded rebuilds
     * against a table that still holds its rows, with explicit-Id hint inserts
     * re-enabled against a sequence that was never reseeded. */
    EC_NEG1(cnid_mysql_empty(db, 0));
    /* The Depleted column is cleared above, but this connection latched the
     * flag at open. Leaving it set would make the rest of a dbd rebuild
     * silently discard every AppleDouble CNID hint it is trying to restore,
     * renumbering the volume from scratch. */
    db->cnid_mysql_flags &= ~(CNID_MYSQL_FLAG_DEPLETED
                              | CNID_MYSQL_FLAG_NEAR_DEPLETION);
EC_CLEANUP:
    EC_EXIT;
}

/*!
 * @brief Hand at most max rows of one folder to fn
 *
 * Every row is fetched, so the connection is free for the next statement
 * when fn stops early.
 *
 * @param[in] cdb  CNID database
 * @param[in] did  the folder
 * @param[in] max  the rows to hand over at most
 * @param[in] fn   receives each row; nonzero stops the rows
 * @param[in] ctx  passed to fn
 * @returns 0, or -1 when the query or fn failed
 */
static int cnid_mysql_dirlist(struct _cnid_db *cdb, cnid_t did, size_t max,
                              cnid_dirlist_fn fn, void *ctx)
{
    CNID_mysql_private *db = cdb->cnid_db_private;
    int stop = 0;
    int rc;

    if (db == NULL || db->cnid_dirlist_stmt == NULL) {
        return -1;
    }

    stmt_param_did = ntohl(did);
    dirlist_param_max = max;

    if (cnid_mysql_stmt_execute(db, &db->cnid_dirlist_stmt) != 0
            || mysql_stmt_bind_result(db->cnid_dirlist_stmt,
                                      dirlist_result) != 0) {
        return -1;
    }

    while ((rc = mysql_stmt_fetch(db->cnid_dirlist_stmt)) == 0
            || rc == MYSQL_DATA_TRUNCATED) {
        if (stop == 0 && rc == 0) {
            stop = fn(ctx, dirlist_result_id, dirlist_result_name,
                      dirlist_result_name_len, dirlist_result_dev,
                      dirlist_result_ino);
        }
    }

    mysql_stmt_free_result(db->cnid_dirlist_stmt);
    return rc == MYSQL_NO_DATA && stop == 0 ? 0 : -1;
}

/*!
 * @brief Allocate a CNID database object with the MySQL function table
 *
 * @param[in] vol  volume of the database
 * @returns the object, or NULL when calloc fails
 */
static struct _cnid_db *cnid_mysql_new(struct vol *vol)
{
    struct _cnid_db *cdb;

    if ((cdb = (struct _cnid_db *)calloc(1, sizeof(struct _cnid_db))) == NULL) {
        return NULL;
    }

    cdb->cnid_db_vol = vol;
    cdb->cnid_db_flags = CNID_FLAG_PERSISTENT;
    cdb->cnid_add = cnid_mysql_add;
    cdb->cnid_dirlist = cnid_mysql_dirlist;
    cdb->cnid_delete = cnid_mysql_delete;
    cdb->cnid_get = cnid_mysql_get;
    cdb->cnid_lookup = cnid_mysql_lookup;
    cdb->cnid_find = cnid_mysql_find;
    cdb->cnid_find_narrows = cnid_mysql_find_narrows;
    cdb->cnid_resolve = cnid_mysql_resolve;
    cdb->cnid_getstamp = cnid_mysql_getstamp;
    cdb->cnid_update = cnid_mysql_update;
    cdb->cnid_close = cnid_mysql_close;
    cdb->cnid_wipe = cnid_mysql_wipe;
    return cdb;
}

/*!
 * @brief Open the MySQL CNID backend for a volume
 *
 * The call creates the state directory of the volume as root, connects to the
 * server, and creates the database when missing. It then creates the volumes
 * table and the CNID table of the volume, named by the volume UUID. It drops
 * the tables and rows of the same path under another UUID. The volume row
 * gets its stamp on first use, and a depleted flag carries into the
 * connection.
 *
 * @param[in] args  open arguments with the volume and the flags
 * @returns the database, or NULL on error
 */
struct _cnid_db *cnid_mysql_open(struct cnid_open_args *args)
{
    EC_INIT;
    CNID_mysql_private *db = NULL;
    struct _cnid_db *cdb = NULL;
    char *sql = NULL;
    char dirpath[PATH_MAX];
    MYSQL_RES *result = NULL;
    MYSQL_ROW row;
    struct vol *vol = args->cnid_args_vol;
    MYSQL *temp_conn = NULL;
    struct stat st;
    int dir_errno = 0;
    snprintf(dirpath, sizeof(dirpath), "%sCNID/%s", _PATH_STATEDIR,
             vol->v_localname);
    become_root();

    /* We need the volume dir under CNID to store .AppleDesktop,
       although this backend does not keep any database file here. */
    if (mkdir(dirpath, 0755) != 0) { //NOSONAR: AFP users need desktop directory access; only the owner can write.
        dir_errno = errno;

        if (dir_errno == EEXIST) {
            dir_errno = stat(dirpath, &st) == 0 && S_ISDIR(st.st_mode) ? 0
                        : ENOTDIR;
        }
    }

    /* before any failure, which would leave the session root */
    unbecome_root();

    if (dir_errno == ENOTDIR) {
        LOG(log_error, logtype_cnid, "'%s' exists but is not a directory", dirpath);
        EC_FAIL;
    }

    if (dir_errno != 0) {
        LOG(log_error, logtype_cnid, "Failed to create volume directory '%s': %s",
            dirpath, strerror(dir_errno));
        EC_FAIL;
    }

    EC_NULL(cdb = cnid_mysql_new(vol));
    EC_NULL(db = (CNID_mysql_private *)calloc(1, sizeof(CNID_mysql_private)));
    cdb->cnid_db_private = db;
    EC_NULL(db->cnid_mysql_voluuid_str = uuid_strip_dashes(vol->v_uuid));
    /* Initialize and connect to MySQL server */
    EC_NULL(db->cnid_mysql_con = mysql_init(NULL));
    /*
     * MySQL connection options for performance and reliability:
     * - MYSQL_OPT_RECONNECT: Auto-reconnect if connection is lost
     * - MYSQL_OPT_CONNECT_TIMEOUT: Connection timeout (30 seconds)
     * - MYSQL_OPT_READ_TIMEOUT: Read operation timeout (30 seconds)
     * - MYSQL_OPT_WRITE_TIMEOUT: Write operation timeout (30 seconds)
     * - MYSQL_SET_CHARSET_NAME: Set UTF-8 charset at connection time
     */
    bool my_recon = true;
    EC_ZERO(mysql_options(db->cnid_mysql_con, MYSQL_OPT_RECONNECT, &my_recon));
    unsigned int connect_timeout = 30;
    EC_ZERO(mysql_options(db->cnid_mysql_con, MYSQL_OPT_CONNECT_TIMEOUT,
                          &connect_timeout));
    unsigned int read_timeout = 30;
    EC_ZERO(mysql_options(db->cnid_mysql_con, MYSQL_OPT_READ_TIMEOUT,
                          &read_timeout));
    unsigned int write_timeout = 30;
    EC_ZERO(mysql_options(db->cnid_mysql_con, MYSQL_OPT_WRITE_TIMEOUT,
                          &write_timeout));

    /*
     * Set UTF-8 charset at connection time only if the volume uses UTF-8.
     * For non-UTF-8 volumes, let MySQL negotiate the charset dynamically.
     * utf8mb4 supports the full Unicode range including 4-byte characters.
     */
    if (vol->v_volcharset == CH_UTF8) {
        EC_ZERO(mysql_options(db->cnid_mysql_con, MYSQL_SET_CHARSET_NAME, "utf8mb4"));
    }

    const AFPObj *obj = vol->v_obj;
    LOG(log_info, logtype_cnid,
        "MySQL CNID backend: host='%s', database='%s'",
        obj->options.cnid_mysql_host ? obj->options.cnid_mysql_host : "(default)",
        obj->options.cnid_mysql_db ? obj->options.cnid_mysql_db : "(none)");
    temp_conn = mysql_init(NULL);

    if (temp_conn == NULL) {
        LOG(log_error, logtype_cnid, "Failed to initialize MySQL connection");
        EC_FAIL;
    }

    /* Set timeout options for temp connection as well */
    mysql_options(temp_conn, MYSQL_OPT_CONNECT_TIMEOUT, &connect_timeout);

    if (mysql_real_connect(temp_conn,
                           obj->options.cnid_mysql_host,
                           obj->options.cnid_mysql_user,
                           obj->options.cnid_mysql_pw,
                           NULL, 0, NULL, 0) == NULL) {
        LOG(log_error, logtype_cnid, "Failed to connect to MySQL server: %s",
            mysql_error(temp_conn));
        EC_FAIL;
    }

    LOG(log_info, logtype_cnid, "MySQL connection info: %s",
        mysql_get_host_info(temp_conn));
    EC_NEG1(asprintf(&sql, "SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA "
                           "WHERE SCHEMA_NAME = '%s'", obj->options.cnid_mysql_db));

    if (cnid_mysql_execute(temp_conn, sql)) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(temp_conn));
        EC_FAIL;
    }

    free(sql);
    sql = NULL;
    result = mysql_store_result(temp_conn);

    if (result == NULL) {
        LOG(log_error, logtype_cnid, "MySQL result error: %s", mysql_error(temp_conn));
        EC_FAIL;
    }

    if (mysql_num_rows(result) == 0) {
        LOG(log_info, logtype_cnid, "Creating MySQL database '%s'",
            obj->options.cnid_mysql_db);
        mysql_free_result(result);
        result = NULL;
        EC_NEG1(asprintf(&sql, "CREATE DATABASE `%s`",
                         obj->options.cnid_mysql_db));

        if (cnid_mysql_execute(temp_conn, sql)) {
            LOG(log_error, logtype_cnid, "Failed to create MySQL database '%s': %s",
                obj->options.cnid_mysql_db, mysql_error(temp_conn));
            EC_FAIL;
        }

        LOG(log_info, logtype_cnid, "Successfully created MySQL database '%s'",
            obj->options.cnid_mysql_db);
        free(sql);
        sql = NULL;
    } else {
        mysql_free_result(result);
        result = NULL;
    }

    mysql_close(temp_conn);
    temp_conn = NULL;
    EC_NULL(mysql_real_connect(db->cnid_mysql_con,
                               obj->options.cnid_mysql_host,
                               obj->options.cnid_mysql_user,
                               obj->options.cnid_mysql_pw,
                               obj->options.cnid_mysql_db,
                               0, NULL, CLIENT_MULTI_STATEMENTS));
#ifdef HAVE_MYSQL_GET_SOCKET

    /*
     * Disable Nagle algorithm (TCP_NODELAY) for TCP connections to reduce latency.
     * Important for the many small queries typical of CNID operations.
     * Only applies to TCP connections - "localhost" uses Unix socket.
     */
    if (obj->options.cnid_mysql_host != NULL &&
            strcasecmp(obj->options.cnid_mysql_host, "localhost") != 0) {
        int sock_fd = mysql_get_socket(db->cnid_mysql_con);

        if (sock_fd >= 0) {
            int flag = 1;

            if (setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) == 0) {
                LOG(log_debug, logtype_cnid, "TCP_NODELAY enabled for MySQL connection");
            } else {
                LOG(log_warning, logtype_cnid, "Failed to set TCP_NODELAY: %s",
                    strerror(errno));
            }
        }
    }

#endif /* HAVE_MYSQL_GET_SOCKET */
    /* Add volume to volume table */
    EC_NEG1(asprintf(&sql, "CREATE TABLE IF NOT EXISTS volumes"
                           "(VolUUID CHAR(32) PRIMARY KEY, VolPath TEXT(4096), Stamp BINARY(8), "
                           "Depleted INT, INDEX(VolPath(64)))"));

    if (cnid_mysql_execute(db->cnid_mysql_con, sql)) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s",
            mysql_error(db->cnid_mysql_con));
        EC_FAIL;
    }

    /* If no warnings, table was just created (not already existing) */
    if (mysql_warning_count(db->cnid_mysql_con) == 0) {
        LOG(log_info, logtype_cnid, "Created 'volumes' table in database '%s'",
            obj->options.cnid_mysql_db);
    }

    /* escaped after the first statement, whose answer carries the sql_mode
     * init_connect set, so a quote or a backslash stays in the path */
    EC_NULL(db->cnid_mysql_volpath = cnid_mysql_escape(db->cnid_mysql_con,
                                                       vol->v_path));
    free(sql);
    sql = NULL;
    /*
     * Clean up stale volume entries for the same path but with a different UUID.
     * This can happen when the UUID config file is rewritten without the entry
     * for this volume (e.g. [Homes] volumes not loaded during load_afp_conf_vols),
     * causing a new UUID to be generated on the next open.
     */
    EC_NEG1(asprintf(&sql,
                     "SELECT VolUUID FROM volumes WHERE VolPath='%s' AND VolUUID != '%s'",
                     db->cnid_mysql_volpath, db->cnid_mysql_voluuid_str));

    if (cnid_mysql_execute(db->cnid_mysql_con, sql) == 0) {
        MYSQL_RES *stale_result = mysql_store_result(db->cnid_mysql_con);

        if (stale_result) {
            MYSQL_ROW stale_row;

            while ((stale_row = mysql_fetch_row(stale_result))) {
                LOG(log_warning, logtype_cnid,
                    "cnid_mysql_open: removing stale volume entry UUID '%s' for path '%s'",
                    stale_row[0], vol->v_path);
                char *drop_sql = NULL;

                if (asprintf(&drop_sql, "DROP TABLE IF EXISTS `%s`", stale_row[0]) != -1) {
                    cnid_mysql_execute(db->cnid_mysql_con, drop_sql);
                    free(drop_sql);
                }
            }

            mysql_free_result(stale_result);
            free(sql);
            sql = NULL;
            EC_NEG1(asprintf(&sql,
                             "DELETE FROM volumes WHERE VolPath='%s' AND VolUUID != '%s'",
                             db->cnid_mysql_volpath, db->cnid_mysql_voluuid_str));
            cnid_mysql_execute(db->cnid_mysql_con, sql);
        }
    }

    free(sql);
    sql = NULL;
    time_t now = time(NULL);
    char stamp[8];
    memset(stamp, 0, 8);
    memcpy(stamp, &now, sizeof(time_t));
    char blob[16 + 1];
    mysql_real_escape_string(db->cnid_mysql_con, blob, stamp, 8);
    EC_NEG1(asprintf(&sql,
                     "INSERT INTO volumes (VolUUID, VolPath, Stamp, Depleted) "
                     "VALUES('%s','%s','%s',0)",
                     db->cnid_mysql_voluuid_str,
                     db->cnid_mysql_volpath,
                     blob));

    if (cnid_mysql_execute(db->cnid_mysql_con, sql)) {
        if (mysql_errno(db->cnid_mysql_con) == ER_DUP_ENTRY) {
            LOG(log_debug, logtype_cnid, "Volume '%s' already registered in database",
                vol->v_path);
        } else {
            LOG(log_error, logtype_cnid, "MySQL query error: %s",
                mysql_error(db->cnid_mysql_con));
            EC_FAIL;
        }
    }

    free(sql);
    sql = NULL;
    /*
     * Check whether CNID set overflowed before.
     * If that's the case, in cnid_mysql_add() we'll ignore the CNID "hint" taken from the
     * AppleDouble file.
     */
    EC_NEG1(asprintf(&sql, "SELECT Depleted FROM volumes WHERE VolUUID='%s'",
                     db->cnid_mysql_voluuid_str));

    if (cnid_mysql_execute(db->cnid_mysql_con, sql)) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s",
            mysql_error(db->cnid_mysql_con));
        EC_FAIL;
    }

    free(sql);
    sql = NULL;

    if ((result = mysql_store_result(db->cnid_mysql_con)) == NULL) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s",
            mysql_error(db->cnid_mysql_con));
        cnid_mysql_set_errno(mysql_errno(db->cnid_mysql_con));
        EC_FAIL;
    }

    if (mysql_num_rows(result)) {
        row = mysql_fetch_row(result);
        int depleted = atoi(row[0]);

        if (depleted) {
            db->cnid_mysql_flags |= CNID_MYSQL_FLAG_DEPLETED;
        }
    }

    mysql_free_result(result);
    result = NULL;
    /* Create CNID table for this volume */
    EC_NEG1(asprintf(&sql, "CREATE TABLE IF NOT EXISTS `%s`"
                           "(Id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,"
                           "Name VARCHAR(255) NOT NULL,"
                           "Did INT UNSIGNED NOT NULL,"
                           "DevNo BIGINT UNSIGNED NOT NULL,"
                           "InodeNo BIGINT UNSIGNED NOT NULL,%s,"
                           "UNIQUE DidName(Did, Name), UNIQUE DevIno(DevNo, InodeNo)) "
                           "AUTO_INCREMENT=17",
                     db->cnid_mysql_voluuid_str, CNID_MYSQL_FOLDED));

    if (cnid_mysql_execute(db->cnid_mysql_con, sql)) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s",
            mysql_error(db->cnid_mysql_con));
        EC_FAIL;
    }

    /* If no warnings, table was just created (not already existing) */
    if (mysql_warning_count(db->cnid_mysql_con) == 0) {
        LOG(log_info, logtype_cnid, "Created CNID table for volume '%s'",
            vol->v_localname);
    }

    EC_ZERO(init_prepared_stmt(db));
    LOG(log_debug, logtype_cnid,
        "Finished initializing MySQL CNID module for volume '%s'",
        vol->v_path);
EC_CLEANUP:

    if (result) {
        mysql_free_result(result);
    }

    if (temp_conn) {
        mysql_close(temp_conn);
    }

    if (sql) {
        free(sql);
    }

    /* the close frees the connection, its statements and the strings */
    if (ret != 0 && cdb != NULL) {
        cnid_mysql_close(cdb);
        cdb = NULL;
    }

    return cdb;
}

struct _cnid_module cnid_mysql_module = {
    .name = "mysql",
    .cnid_open = cnid_mysql_open
};
