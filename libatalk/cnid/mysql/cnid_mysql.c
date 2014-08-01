/*
 * Copyright (C) Ralph Boehme 2013
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#undef _FORTIFY_SOURCE

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <errno.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>
#include <arpa/inet.h>

#include <mysql.h>
#include <mysqld_error.h>
#include <errmsg.h>

#include <atalk/logger.h>
#include <atalk/adouble.h>
#include <atalk/util.h>
#include <atalk/cnid_mysql_private.h>
#include <atalk/cnid_bdb_private.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/volume.h>


static MYSQL_BIND lookup_param[4], lookup_result[5];
static MYSQL_BIND add_param[4], put_param[5];

/*
 * Prepared statement parameters
 */
static char               stmt_param_name[MAXPATHLEN];
static unsigned long      stmt_param_name_len;
static unsigned long long stmt_param_id;
static unsigned long long stmt_param_did;
static unsigned long long stmt_param_dev;
static unsigned long long stmt_param_ino;

/*
 * lookup result parameters
 */
static unsigned long long lookup_result_id;
static unsigned long long lookup_result_did;
static char               lookup_result_name[MAXPATHLEN];
static unsigned long      lookup_result_name_len;
static unsigned long long lookup_result_dev;
static unsigned long long lookup_result_ino;

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

    EC_NULL( db->cnid_lookup_stmt = mysql_stmt_init(db->cnid_mysql_con) );
    EC_NEG1( asprintf(&sql,
                      "SELECT Id,Did,Name,DevNo,InodeNo FROM %s "
                      "WHERE (Name=? AND Did=?) OR (DevNo=? AND InodeNo=?)",
                      db->cnid_mysql_voluuid_str) );
    EC_ZERO_LOG( mysql_stmt_prepare(db->cnid_lookup_stmt, sql, strlen(sql)) );
    EC_ZERO_LOG( mysql_stmt_bind_param(db->cnid_lookup_stmt, lookup_param) );

EC_CLEANUP:
    if (sql)
        free(sql);
    EC_EXIT;
}

static int init_prepared_stmt_add(CNID_mysql_private *db)
{
    EC_INIT;
    char *sql = NULL;

    EC_NULL( db->cnid_add_stmt = mysql_stmt_init(db->cnid_mysql_con) );
    EC_NEG1( asprintf(&sql,
                      "INSERT INTO %s (Name,Did,DevNo,InodeNo) VALUES(?,?,?,?)",
                      db->cnid_mysql_voluuid_str) );

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

    EC_ZERO_LOG( mysql_stmt_prepare(db->cnid_add_stmt, sql, strlen(sql)) );
    EC_ZERO_LOG( mysql_stmt_bind_param(db->cnid_add_stmt, add_param) );

EC_CLEANUP:
    if (sql)
        free(sql);
    EC_EXIT;
}

static int init_prepared_stmt_put(CNID_mysql_private *db)
{
    EC_INIT;
    char *sql = NULL;

    EC_NULL( db->cnid_put_stmt = mysql_stmt_init(db->cnid_mysql_con) );
    EC_NEG1( asprintf(&sql,
                      "INSERT INTO %s (Id,Name,Did,DevNo,InodeNo) VALUES(?,?,?,?,?)",
                      db->cnid_mysql_voluuid_str) );

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

    EC_ZERO_LOG( mysql_stmt_prepare(db->cnid_put_stmt, sql, strlen(sql)) );
    EC_ZERO_LOG( mysql_stmt_bind_param(db->cnid_put_stmt, put_param) );

EC_CLEANUP:
    if (sql)
        free(sql);
    EC_EXIT;
}

static int init_prepared_stmt(CNID_mysql_private *db)
{
    EC_INIT;

    EC_ZERO( init_prepared_stmt_lookup(db) );
    EC_ZERO( init_prepared_stmt_add(db) );
    EC_ZERO( init_prepared_stmt_put(db) );

EC_CLEANUP:
    EC_EXIT;
}

static void close_prepared_stmt(CNID_mysql_private *db)
{
    mysql_stmt_close(db->cnid_lookup_stmt);
    mysql_stmt_close(db->cnid_add_stmt);
    mysql_stmt_close(db->cnid_put_stmt);
}

static int cnid_mysql_execute(MYSQL *con, char *fmt, ...)
{
    char *sql = NULL;
    va_list ap;
    int rv;

    va_start(ap, fmt);
    if (vasprintf(&sql, fmt, ap) == -1)
        return -1;
    va_end(ap);

    LOG(log_maxdebug, logtype_cnid, "SQL: %s", sql);

    rv = mysql_query(con, sql);

    if (rv) {
        LOG(log_info, logtype_cnid, "MySQL query \"%s\", error: %s", sql, mysql_error(con));
        errno = CNID_ERR_DB;
    }
    free(sql);
    return rv;
}

int cnid_mysql_delete(struct _cnid_db *cdb, const cnid_t id)
{
    EC_INIT;
    CNID_mysql_private *db;

    if (!cdb || !(db = cdb->cnid_db_private) || !id) {
        LOG(log_error, logtype_cnid, "cnid_mysql_delete: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    LOG(log_debug, logtype_cnid, "cnid_mysql_delete(%" PRIu32 "): BEGIN", ntohl(id));
    
    EC_NEG1( cnid_mysql_execute(db->cnid_mysql_con,
                                "DELETE FROM %s WHERE Id=%" PRIu32,
                                db->cnid_mysql_voluuid_str,
                                ntohl(id)) );

    LOG(log_debug, logtype_cnid, "cnid_mysql_delete(%" PRIu32 "): END", ntohl(id));

EC_CLEANUP:
    EC_EXIT;
}

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

        close_prepared_stmt(db);

        if (db->cnid_mysql_con)
            mysql_close(db->cnid_mysql_con);
        free(db);
    }

    free(cdb);

    return;
}

int cnid_mysql_update(struct _cnid_db *cdb,
                      cnid_t id,
                      const struct stat *st,
                      cnid_t did,
                      const char *name,
                      size_t len)
{
    EC_INIT;
    CNID_mysql_private *db;
    cnid_t update_id;

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

    uint64_t dev = st->st_dev;
    uint64_t ino = st->st_ino;

    do {
        EC_NEG1( cnid_mysql_execute(db->cnid_mysql_con,
                                    "DELETE FROM %s WHERE Id=%" PRIu32,
                                    db->cnid_mysql_voluuid_str,
                                    ntohl(id)) );
        EC_NEG1( cnid_mysql_execute(db->cnid_mysql_con,
                                    "DELETE FROM %s WHERE Did=%" PRIu32 " AND Name='%s'",
                                    db->cnid_mysql_voluuid_str, ntohl(did), name) );
        EC_NEG1( cnid_mysql_execute(db->cnid_mysql_con,
                                    "DELETE FROM %s WHERE DevNo=%" PRIu64 " AND InodeNo=%" PRIu64,
                                    db->cnid_mysql_voluuid_str, dev, ino) );

        stmt_param_id = ntohl(id);
        strncpy(stmt_param_name, name, sizeof(stmt_param_name));
        stmt_param_name_len = len;
        stmt_param_did = ntohl(did);
        stmt_param_dev = dev;
        stmt_param_ino = ino;

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
                EC_ZERO( init_prepared_stmt(db) );
                continue;
            default:
                EC_FAIL;
            }
        }
        update_id = mysql_stmt_insert_id(db->cnid_put_stmt);
    } while (update_id != ntohl(id));

EC_CLEANUP:
    EC_EXIT;
}

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

    uint64_t dev = st->st_dev;
    uint64_t ino = st->st_ino;
    cnid_t hint = db->cnid_mysql_hint;

    LOG(log_maxdebug, logtype_cnid, "cnid_mysql_lookup(did: %" PRIu32 ", name: \"%s\", hint: %" PRIu32 "): START",
        ntohl(did), name, ntohl(hint));

    strncpy(stmt_param_name, name, sizeof(stmt_param_name));
    stmt_param_name_len = len;
    stmt_param_did = ntohl(did);
    stmt_param_dev = dev;
    stmt_param_ino = ino;

exec_stmt:
    if (mysql_stmt_execute(db->cnid_lookup_stmt)) {
        switch (mysql_stmt_errno(db->cnid_lookup_stmt)) {
        case CR_SERVER_LOST:
            close_prepared_stmt(db);
            EC_ZERO( init_prepared_stmt(db) );
            goto exec_stmt;
        default:
            EC_FAIL;
        }
    }
    EC_ZERO_LOG( mysql_stmt_store_result(db->cnid_lookup_stmt) );
    have_result = true;
    EC_ZERO_LOG( mysql_stmt_bind_result(db->cnid_lookup_stmt, lookup_result) );

    uint64_t retdev, retino;
    cnid_t retid, retdid;
    char *retname;

    switch (mysql_stmt_num_rows(db->cnid_lookup_stmt)) {

    case 0:
        /* not found */
        LOG(log_debug, logtype_cnid, "cnid_mysql_lookup: name: '%s', did: %u is not in the CNID database", 
            name, ntohl(did));
        errno = CNID_DBD_RES_NOTFOUND;
        EC_FAIL;

    case 1:
        /* either both OR clauses matched the same id or only one matched, handled below */
        EC_ZERO( mysql_stmt_fetch(db->cnid_lookup_stmt) );
        break;

    case 2:
        /* a mismatch, delete both and return not found */
        while (mysql_stmt_fetch(db->cnid_lookup_stmt) == 0) {
            if (cnid_mysql_delete(cdb, htonl((cnid_t)lookup_result_id))) {
                LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
                errno = CNID_ERR_DB;
                EC_FAIL;
            }
        }
        errno = CNID_DBD_RES_NOTFOUND;
        EC_FAIL;

    default:
        errno = CNID_ERR_DB;
        EC_FAIL;
    }

    retid = htonl(lookup_result_id);
    retdid = htonl(lookup_result_did);
    retname = lookup_result_name;
    retdev = lookup_result_dev;
    retino = lookup_result_ino;

    if (retdid != did || STRCMP(retname, !=, name)) {
        LOG(log_debug, logtype_cnid,
            "cnid_mysql_lookup(CNID hint: %" PRIu32 ", DID: %" PRIu32 ", name: \"%s\"): server side mv oder reused inode",
            ntohl(hint), ntohl(did), name);
        if (hint != retid) {
            if (cnid_mysql_delete(cdb, retid) != 0) {
                LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
                errno = CNID_ERR_DB;
                EC_FAIL;
            }
            errno = CNID_DBD_RES_NOTFOUND;
            EC_FAIL;
        }
        LOG(log_debug, logtype_cnid, "cnid_mysql_lookup: server side mv, got hint, updating");
        if (cnid_mysql_update(cdb, retid, st, did, name, len) != 0) {
            LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
            errno = CNID_ERR_DB;
            EC_FAIL;
        }
        id = retid;
    } else if (retdev != dev || retino != ino) {
        LOG(log_debug, logtype_cnid, "cnid_mysql_lookup(DID:%u, name: \"%s\"): changed dev/ino",
            ntohl(did), name);
        if (cnid_mysql_delete(cdb, retid) != 0) {
            LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
            errno = CNID_ERR_DB;
            EC_FAIL;
        }
        errno = CNID_DBD_RES_NOTFOUND;
        EC_FAIL;
    } else {
        /* everythings good */
        id = retid;
    }

EC_CLEANUP:
    LOG(log_debug, logtype_cnid, "cnid_mysql_lookup: id: %" PRIu32, ntohl(id));
    if (have_result)
        mysql_stmt_free_result(db->cnid_lookup_stmt);
    if (ret != 0)
        id = CNID_INVALID;
    return id;
}

cnid_t cnid_mysql_add(struct _cnid_db *cdb,
                      const struct stat *st,
                      cnid_t did,
                      const char *name,
                      size_t len,
                      cnid_t hint)
{
    EC_INIT;
    CNID_mysql_private *db;
    cnid_t id = CNID_INVALID;
    MYSQL_RES *result = NULL;
    MYSQL_STMT *stmt;
    my_ulonglong lastid;

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

    uint64_t dev = st->st_dev;
    uint64_t ino = st->st_ino;
    db->cnid_mysql_hint = hint;

    LOG(log_maxdebug, logtype_cnid, "cnid_mysql_add(did: %" PRIu32 ", name: \"%s\", hint: %" PRIu32 "): START",
        ntohl(did), name, ntohl(hint));

    do {
        if ((id = cnid_mysql_lookup(cdb, st, did, name, len)) == CNID_INVALID) {
            if (errno == CNID_ERR_DB)
                EC_FAIL;
            /*
             * If the CNID set overflowed before (CNID_MYSQL_FLAG_DEPLETED)
             * ignore the CNID "hint" taken from the AppleDouble file
             */
            if (!db->cnid_mysql_hint || (db->cnid_mysql_flags & CNID_MYSQL_FLAG_DEPLETED)) {
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

            if (mysql_stmt_execute(stmt)) {
                switch (mysql_stmt_errno(stmt)) {
                case ER_DUP_ENTRY:
                    break;
                case CR_SERVER_LOST:
                    close_prepared_stmt(db);
                    EC_ZERO( init_prepared_stmt(db) );
                    continue;
                default:
                    EC_FAIL;
                }
                /*
                 * Race condition:
                 * between lookup and insert another process may have inserted
                 * this entry.
                 */
                if (db->cnid_mysql_hint)
                    db->cnid_mysql_hint = CNID_INVALID;
                continue;
            }

            lastid = mysql_stmt_insert_id(stmt);

            if (lastid > 0xffffffff) {
                /* CNID set ist depleted, restart from scratch */
                EC_NEG1( cnid_mysql_execute(db->cnid_mysql_con,
                                            "START TRANSACTION;"
                                            "UPDATE volumes SET Depleted=1 WHERE VolUUID='%s';"
                                            "TRUNCATE TABLE %s;"
                                            "ALTER TABLE %s AUTO_INCREMENT = 17;" 
                                            "COMMIT;",
                                            db->cnid_mysql_voluuid_str,
                                            db->cnid_mysql_voluuid_str,
                                            db->cnid_mysql_voluuid_str) );
                db->cnid_mysql_flags |= CNID_MYSQL_FLAG_DEPLETED;
                hint = CNID_INVALID;
                do {
                    result = mysql_store_result(db->cnid_mysql_con);
                    if (result)
                        mysql_free_result(result);
                } while (mysql_next_result(db->cnid_mysql_con) == 0);
                continue;
            }

            /* Finally assign our result */
            id = htonl((uint32_t)lastid);
        }
    } while (id == CNID_INVALID);

EC_CLEANUP:
    LOG(log_debug, logtype_cnid, "cnid_mysql_add: id: %" PRIu32, ntohl(id));

    if (result)
        mysql_free_result(result);
    return id;
}

cnid_t cnid_mysql_get(struct _cnid_db *cdb, cnid_t did, const char *name, size_t len)
{
    EC_INIT;
    CNID_mysql_private *db;
    cnid_t id = CNID_INVALID;
    MYSQL_RES *result = NULL;
    MYSQL_ROW row;

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

    LOG(log_debug, logtype_cnid, "cnid_mysql_get(did: %" PRIu32 ", name: \"%s\"): START",
        ntohl(did),name);

    EC_NEG1( cnid_mysql_execute(db->cnid_mysql_con,
                                "SELECT Id FROM %s "
                                "WHERE Name='%s' AND Did=%" PRIu32,
                                db->cnid_mysql_voluuid_str,
                                name,
                                ntohl(did)) );

    if ((result = mysql_store_result(db->cnid_mysql_con)) == NULL) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
        errno = CNID_ERR_DB;
        EC_FAIL;
    }

    if (mysql_num_rows(result)) {
        row = mysql_fetch_row(result);
        id = htonl(atoi(row[0]));
    }

EC_CLEANUP:
    LOG(log_debug, logtype_cnid, "cnid_mysql_get: id: %" PRIu32, ntohl(id));

    if (result)
        mysql_free_result(result);

    return id;
}

char *cnid_mysql_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer, size_t len)
{
    EC_INIT;
    CNID_mysql_private *db;
    MYSQL_RES *result = NULL;
    MYSQL_ROW row;

    if (!cdb || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid, "cnid_mysql_get: Parameter error");
        errno = CNID_ERR_PARAM;
        EC_FAIL;
    }

    EC_NEG1( cnid_mysql_execute(
                 db->cnid_mysql_con,
                 "SELECT Did,Name FROM %s WHERE Id=%" PRIu32,
                 db->cnid_mysql_voluuid_str, ntohl(*id)) );

    EC_NULL( result = mysql_store_result(db->cnid_mysql_con) );

    if (mysql_num_rows(result) != 1)
        EC_FAIL;

    row = mysql_fetch_row(result);

    *id = htonl(atoi(row[0]));
    strncpy(buffer, row[1], len);

EC_CLEANUP:             
    if (result)
        mysql_free_result(result);

    if (ret != 0) {
        *id = CNID_INVALID;
        return NULL;
    }
    return buffer;
}

/**
 * Caller passes buffer where we will store the db stamp
 **/
int cnid_mysql_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
{
    EC_INIT;
    CNID_mysql_private *db;
    MYSQL_RES *result = NULL;
    MYSQL_ROW row;

    if (!cdb || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid, "cnid_find: Parameter error");
        errno = CNID_ERR_PARAM;
        return CNID_INVALID;
    }

    if (!buffer)
        EC_EXIT_STATUS(0);

    if (cnid_mysql_execute(db->cnid_mysql_con,
                           "SELECT Stamp FROM volumes WHERE VolPath='%s'",
                           cdb->cnid_db_vol->v_path)) {
        if (mysql_errno(db->cnid_mysql_con) != ER_DUP_ENTRY) {
            LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
            EC_FAIL;
        }
    }

    if ((result = mysql_store_result(db->cnid_mysql_con)) == NULL) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
        errno = CNID_ERR_DB;
        EC_FAIL;
    }
    if (!mysql_num_rows(result)) {
        LOG(log_error, logtype_cnid, "Can't get DB stamp for volumes \"%s\"", cdb->cnid_db_vol->v_path);
        EC_FAIL;
    }
    row = mysql_fetch_row(result);
    memcpy(buffer, row[0], len);

EC_CLEANUP:
    if (result)
        mysql_free_result(result);
    EC_EXIT;
}


int cnid_mysql_find(struct _cnid_db *cdb, const char *name, size_t namelen, void *buffer, size_t buflen)
{
    LOG(log_error, logtype_cnid,
        "cnid_mysql_find(\"%s\"): not supported with MySQL CNID backend", name);
    return -1;
}

cnid_t cnid_mysql_rebuild_add(struct _cnid_db *cdb, const struct stat *st,
                              cnid_t did, const char *name, size_t len, cnid_t hint)
{
    LOG(log_error, logtype_cnid,
        "cnid_mysql_rebuild_add(\"%s\"): not supported with MySQL CNID backend", name);
    return CNID_INVALID;
}

int cnid_mysql_wipe(struct _cnid_db *cdb)
{
    EC_INIT;
    CNID_mysql_private *db;
    MYSQL_RES *result = NULL;

    if (!cdb || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid, "cnid_wipe: Parameter error");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    LOG(log_debug, logtype_cnid, "cnid_dbd_wipe");

    EC_NEG1( cnid_mysql_execute(
                 db->cnid_mysql_con,
                 "START TRANSACTION;"
                 "UPDATE volumes SET Depleted=0 WHERE VolUUID='%s';"
                 "TRUNCATE TABLE %s;"
                 "ALTER TABLE %s AUTO_INCREMENT = 17;"
                 "COMMIT;",
                 db->cnid_mysql_voluuid_str,
                 db->cnid_mysql_voluuid_str,
                 db->cnid_mysql_voluuid_str) );

    do {
        result = mysql_store_result(db->cnid_mysql_con);
        if (result)
            mysql_free_result(result);
    } while (mysql_next_result(db->cnid_mysql_con) == 0);

EC_CLEANUP:
    EC_EXIT;
}

static struct _cnid_db *cnid_mysql_new(struct vol *vol)
{
    struct _cnid_db *cdb;

    if ((cdb = (struct _cnid_db *)calloc(1, sizeof(struct _cnid_db))) == NULL)
        return NULL;

    cdb->cnid_db_vol = vol;
    cdb->cnid_db_flags = CNID_FLAG_PERSISTENT | CNID_FLAG_LAZY_INIT;
    cdb->cnid_add = cnid_mysql_add;
    cdb->cnid_delete = cnid_mysql_delete;
    cdb->cnid_get = cnid_mysql_get;
    cdb->cnid_lookup = cnid_mysql_lookup;
    cdb->cnid_find = cnid_mysql_find;
    cdb->cnid_nextid = NULL;
    cdb->cnid_resolve = cnid_mysql_resolve;
    cdb->cnid_getstamp = cnid_mysql_getstamp;
    cdb->cnid_update = cnid_mysql_update;
    cdb->cnid_rebuild_add = cnid_mysql_rebuild_add;
    cdb->cnid_close = cnid_mysql_close;
    cdb->cnid_wipe = cnid_mysql_wipe;
    return cdb;
}

static const char *printuuid(const unsigned char *uuid) {
    static char uuidstring[64];
    int i = 0;
    unsigned char c;

    while ((c = *uuid)) {
        uuid++;
        sprintf(uuidstring + i, "%02X", c);
        i += 2;
    }
    uuidstring[i] = 0;
    return uuidstring;
}

/* ---------------------- */
struct _cnid_db *cnid_mysql_open(struct cnid_open_args *args)
{
    EC_INIT;
    CNID_mysql_private *db = NULL;
    struct _cnid_db *cdb = NULL;
    MYSQL_RES *result = NULL;
    MYSQL_ROW row;
    struct vol *vol = args->cnid_args_vol;

    EC_NULL( cdb = cnid_mysql_new(vol) );
    EC_NULL( db = (CNID_mysql_private *)calloc(1, sizeof(CNID_mysql_private)) );
    cdb->cnid_db_private = db;

    db->cnid_mysql_voluuid_str = strdup(printuuid(vol->v_uuid));

    /* Initialize and connect to MySQL server */
    EC_NULL( db->cnid_mysql_con = mysql_init(NULL) );
    my_bool my_recon = true;
    EC_ZERO( mysql_options(db->cnid_mysql_con, MYSQL_OPT_RECONNECT, &my_recon) );
    int my_timeout = 600;
    EC_ZERO( mysql_options(db->cnid_mysql_con, MYSQL_OPT_CONNECT_TIMEOUT, &my_timeout) );

    const AFPObj *obj = vol->v_obj;

    EC_NULL( mysql_real_connect(db->cnid_mysql_con,
                                obj->options.cnid_mysql_host,
                                obj->options.cnid_mysql_user,
                                obj->options.cnid_mysql_pw,
                                obj->options.cnid_mysql_db,
                                0, NULL, CLIENT_MULTI_STATEMENTS));

    /* Add volume to volume table */
    if (cnid_mysql_execute(db->cnid_mysql_con,
                           "CREATE TABLE IF NOT EXISTS volumes"
                           "(VolUUID CHAR(32) PRIMARY KEY,VolPath TEXT(4096),Stamp BINARY(8),Depleted INT, INDEX(VolPath(64)))")) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
        EC_FAIL;
    }
    time_t now = time(NULL);
    char stamp[8];
    memset(stamp, 0, 8);
    memcpy(stamp, &now, sizeof(time_t));
    char blob[16+1];
    mysql_real_escape_string(db->cnid_mysql_con, blob, stamp, 8);

    if (cnid_mysql_execute(db->cnid_mysql_con,
                           "INSERT INTO volumes (VolUUID,Volpath,Stamp,Depleted) "
                           "VALUES('%s','%s','%s',0)",
                           db->cnid_mysql_voluuid_str,
                           vol->v_path,
                           blob)) {
        if (mysql_errno(db->cnid_mysql_con) != ER_DUP_ENTRY) {
            LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
            EC_FAIL;
        }
    }

    /*
     * Check whether CNID set overflowed before.
     * If that's the case, in cnid_mysql_add() we'll ignore the CNID "hint" taken from the
     * AppleDouble file.
     */
    if (cnid_mysql_execute(db->cnid_mysql_con,
                           "SELECT Depleted FROM volumes WHERE VolUUID='%s'",
                           db->cnid_mysql_voluuid_str)) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
        EC_FAIL;
    }
    if ((result = mysql_store_result(db->cnid_mysql_con)) == NULL) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
        errno = CNID_ERR_DB;
        EC_FAIL;
    }
    if (mysql_num_rows(result)) {
        row = mysql_fetch_row(result);
        int depleted = atoi(row[0]);
        if (depleted)
            db->cnid_mysql_flags |= CNID_MYSQL_FLAG_DEPLETED;
    }
    mysql_free_result(result);
    result = NULL;

    /* Create volume table */
    if (cnid_mysql_execute(db->cnid_mysql_con,
                           "CREATE TABLE IF NOT EXISTS %s"
                           "(Id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,"
                           "Name VARCHAR(255) NOT NULL,"
                           "Did INT UNSIGNED NOT NULL,"
                           "DevNo BIGINT UNSIGNED NOT NULL,"
                           "InodeNo BIGINT UNSIGNED NOT NULL,"
                           "UNIQUE DidName(Did, Name), UNIQUE DevIno(DevNo, InodeNo)) "
                           "AUTO_INCREMENT=17",
                           db->cnid_mysql_voluuid_str)) {
        LOG(log_error, logtype_cnid, "MySQL query error: %s", mysql_error(db->cnid_mysql_con));
        EC_FAIL;
    }

    EC_ZERO( init_prepared_stmt(db) );

    LOG(log_debug, logtype_cnid, "Finished initializing MySQL CNID module for volume '%s'",
        vol->v_path);

EC_CLEANUP:
    if (result)
        mysql_free_result(result);
    if (ret != 0) {
        if (cdb)
            free(cdb);
        cdb = NULL;
        if (db)
            free(db);
    }
    return cdb;
}

struct _cnid_module cnid_mysql_module = {
    "mysql",
    {NULL, NULL},
    cnid_mysql_open,
    0
};
