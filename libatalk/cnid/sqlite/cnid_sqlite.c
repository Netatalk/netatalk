/*
 * Copyright (C) Ralph Boehme 2013
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif				/* HAVE_CONFIG_H */

#undef _FORTIFY_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
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

#include <sqlite3.h>

#include <atalk/logger.h>
#include <atalk/adouble.h>
#include <atalk/util.h>
#include <atalk/cnid_sqlite_private.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/volume.h>


sqlite3_stmt *stmt = NULL;
// CK static MYSQL_BIND lookup_param[4], lookup_result[5];
// CK static MYSQL_BIND add_param[4], put_param[5];

/*
 * Prepared statement parameters
 */
static char stmt_param_name[MAXPATHLEN];
static u_int32_t stmt_param_name_len;
static u_int64_t stmt_param_id;
static u_int64_t stmt_param_did;
static u_int64_t stmt_param_dev;
static u_int64_t stmt_param_ino;

/*
 * lookup result parameters
 */
static u_int64_t lookup_result_id;
static u_int64_t lookup_result_did;
static char lookup_result_name[MAXPATHLEN];
static unsigned long lookup_result_name_len;
static u_int64_t lookup_result_dev;
static u_int64_t lookup_result_ino;

static int init_prepared_stmt_lookup(CNID_sqlite_private * db)
{
	EC_INIT;
	char *sql = NULL;

	EC_ZERO(sqlite3_finalize(db->cnid_lookup_stmt));
	EC_NEG1(asprintf
		(&sql,
		 "SELECT Id,Did,Name,DevNo,InodeNo FROM `%s` "
		 "WHERE (Name=? AND Did=?) OR (DevNo=? AND InodeNo=?)",
		 db->cnid_sqlite_voluuid_str));
	EC_ZERO_LOG(sqlite3_prepare_v2(db->cnid_sqlite_con, sql, strlen(sql), db->cnid_lookup_stmt, NULL));

	EC_ZERO_LOG(sqlite3_bind_text(db->cnid_lookup_stmt, 1, stmt_param_name, strlen(stmt_param_name), SQLITE_STATIC) );
	EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_lookup_stmt, 2, stmt_param_did) );
	EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_lookup_stmt, 3, stmt_param_dev) );
	EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_lookup_stmt, 4, stmt_param_ino) );

      EC_CLEANUP:
	if (sql)
		free(sql);
	EC_EXIT;
}

static int init_prepared_stmt_add(CNID_sqlite_private * db)
{
	EC_INIT;
	char *sql = NULL;

	EC_ZERO(sqlite3_finalize(db->cnid_add_stmt));
	EC_NEG1(ret = asprintf(&sql,
			 "INSERT INTO `%s` (Name,Did,DevNo,InodeNo) VALUES(?,?,?,?,?)",
			 db->cnid_sqlite_voluuid_str) );

	EC_ZERO_LOG(sqlite3_prepare_v2
		    (db->cnid_sqlite_con, sql, strlen(sql), db->cnid_put_stmt, NULL));
	EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_add_stmt,
		    1, stmt_param_id) );
	EC_ZERO_LOG(sqlite3_bind_text(db->cnid_add_stmt,
		    2, stmt_param_name, strlen(stmt_param_name), SQLITE_STATIC) );
	EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_add_stmt,
		    3, stmt_param_did) );
	EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_add_stmt,
		    4, stmt_param_dev) );
	EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_add_stmt,
		    5, stmt_param_ino) );

      EC_CLEANUP:
	if (sql)
		free(sql);
	EC_EXIT;
}

static int init_prepared_stmt_put(CNID_sqlite_private * db)
{
	EC_INIT;
	char *sql = NULL;

	EC_ZERO(sqlite3_finalize(db->cnid_put_stmt));
	EC_NEG1(asprintf(&sql,
			 "INSERT INTO `%s` (Id,Name,Did,DevNo,InodeNo) VALUES(?,?,?,?,?)",
			 db->cnid_sqlite_voluuid_str));

	EC_ZERO_LOG(sqlite3_prepare_v2
		    (db->cnid_sqlite_con, sql, strlen(sql), db->cnid_put_stmt, NULL));
	EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_put_stmt,
		    1, stmt_param_id) );
	EC_ZERO_LOG(sqlite3_bind_text(db->cnid_put_stmt,
		    2, stmt_param_name, strlen(stmt_param_name), SQLITE_STATIC) );
	EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_put_stmt,
		    3, stmt_param_did) );
	EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_put_stmt,
		    4, stmt_param_dev) );
	EC_ZERO_LOG(sqlite3_bind_int64(db->cnid_put_stmt,
		    5, stmt_param_ino) );

      EC_CLEANUP:
	if (sql)
		free(sql);
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
	if (vasprintf(&sql, fmt, ap) == -1)
		return -1;
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

	if (!cdb || !(db = cdb->cnid_db_private) || !id) {
		LOG(log_error, logtype_cnid,
		    "cnid_sqlite_delete: Parameter error");
		errno = CNID_ERR_PARAM;
		EC_FAIL;
	}

	LOG(log_debug, logtype_cnid,
	    "cnid_sqlite_delete(%" PRIu32 "): BEGIN", ntohl(id));

	EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con,
				    "DELETE FROM `%s` WHERE Id=%" PRIu32,
				    db->cnid_sqlite_voluuid_str,
				    ntohl(id)));

	LOG(log_debug, logtype_cnid,
	    "cnid_sqlite_delete(%" PRIu32 "): END", ntohl(id));

      EC_CLEANUP:
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
		      const cnid_t did, char *name, size_t len)
{
	EC_INIT;
	CNID_sqlite_private *db;
	cnid_t update_id;

	if (!cdb || !(db = cdb->cnid_db_private) || !id || !st || !name) {
		LOG(log_error, logtype_cnid,
		    "cnid_update: Parameter error");
		errno = CNID_ERR_PARAM;
		EC_FAIL;
	}

	if (len > MAXPATHLEN) {
		LOG(log_error, logtype_cnid,
		    "cnid_update: Path name is too long");
		errno = CNID_ERR_PATH;
		EC_FAIL;
	}

	uint64_t dev = st->st_dev;
	uint64_t ino = st->st_ino;

	do {
		EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con,
					    "DELETE FROM `%s` WHERE Id=%"
					    PRIu32,
					    db->cnid_sqlite_voluuid_str,
					    ntohl(id)));
		EC_NEG1(cnid_sqlite_execute
			(db->cnid_sqlite_con,
			 "DELETE FROM `%s` WHERE Did=%" PRIu32
			 " AND Name='%s'", db->cnid_sqlite_voluuid_str,
			 ntohl(did), name));
		EC_NEG1(cnid_sqlite_execute
			(db->cnid_sqlite_con,
			 "DELETE FROM `%s` WHERE DevNo=%" PRIu64
			 " AND InodeNo=%" PRIu64,
			 db->cnid_sqlite_voluuid_str, dev, ino));

		stmt_param_id = ntohl(id);
		strncpy(stmt_param_name, name, sizeof(stmt_param_name)-1);
		stmt_param_name_len = len;
		stmt_param_did = ntohl(did);
		stmt_param_dev = dev;
		stmt_param_ino = ino;

		if (sqlite3_step(db->cnid_put_stmt) != SQLITE_ROW) {
			switch (sqlite3_errcode(db->cnid_sqlite_con)) {
//			case ER_DUP_ENTRY:
//				/*
//				 * Race condition:
//				 * between deletion and insert another process
//				 * may have inserted this entry.
//				 */
//				continue;
			default:
				EC_FAIL;
			}
		}
		update_id = sqlite3_last_insert_rowid(db->cnid_put_stmt);
	} while (update_id != ntohl(id));

      EC_CLEANUP:
	EC_EXIT;
}

cnid_t cnid_sqlite_lookup(struct _cnid_db *cdb,
			  const struct stat *st,
			  const cnid_t did, char *name, const size_t len)
{
	EC_INIT;

	CNID_sqlite_private *db;
	cnid_t id = CNID_INVALID;
	bool have_result = false;
	uint64_t dev = st->st_dev;
	uint64_t ino = st->st_ino;

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

	LOG(log_maxdebug, logtype_cnid,
	    "cnid_sqlite_lookup(did: %" PRIu32 ", name: \"%s\"): START",
	     ntohl(did), name);

	strncpy(stmt_param_name, name, sizeof(stmt_param_name)-1);
	stmt_param_name_len = len;
	stmt_param_did = ntohl(did);
	stmt_param_dev = dev;
	stmt_param_ino = ino;


	uint64_t retdev, retino;
	cnid_t retid, retdid;
	char *retname;
	int sqlite_return;

	sqlite_return = sqlite3_step(stmt);
	if (sqlite_return == SQLITE_DONE) {
		/* not found (no rows) */
		LOG(log_debug, logtype_cnid,
		    "cnid_sqlite_lookup: name: '%s', did: %u is not in the CNID database",
		    name, ntohl(did));
		errno = CNID_INVALID;
		EC_FAIL;
	}
	/* got at least one row, store result in lookup_result_X */
	lookup_result_id = sqlite3_column_int64(stmt, 1);
	lookup_result_did = sqlite3_column_int64(stmt, 2);
	snprintf(lookup_result_name, MAXPATHLEN-1, (const char *)sqlite3_column_text(stmt, 3));
	lookup_result_name_len = strlen(lookup_result_name);
	lookup_result_dev = sqlite3_column_int64(stmt, 4);
	lookup_result_ino = sqlite3_column_int64(stmt, 5);

	sqlite_return = sqlite3_step(stmt);
	if (sqlite_return != SQLITE_DONE) {
		/* We have more than one row */
		/* a mismatch, delete both and return not found */
		while (sqlite3_step(db->cnid_lookup_stmt) != SQLITE_DONE) {
			if (cnid_sqlite_delete
			    (cdb, htonl((cnid_t) lookup_result_id))) {
				LOG(log_error, logtype_cnid,
				    "sqlite query error: %s",
				    sqlite3_errmsg(db->cnid_sqlite_con));
				errno = CNID_ERR_DB;
				EC_FAIL;
			}
		}
		errno = CNID_INVALID;
		EC_FAIL;
	}

	/* handle 3+ matches? */
#if 0
	default:
		errno = CNID_ERR_DB;
		EC_FAIL;
	}
#endif

	retid = htonl(lookup_result_id);
	retdid = htonl(lookup_result_did);
	retname = lookup_result_name;
	retdev = lookup_result_dev;
	retino = lookup_result_ino;

	if (retdid != did || STRCMP(retname, !=, name)) {
		LOG(log_debug, logtype_cnid,
		    "cnid_sqlite_lookup(CNID %" PRIu32 ", DID: %"
		    PRIu32
		    ", name: \"%s\"): server side mv oder reused inode",
		    ntohl(did), name);
		LOG(log_debug, logtype_cnid,
		    "cnid_sqlite_lookup: server side mv, got hint, updating");
		if (cnid_sqlite_update(cdb, retid, st, did, name, len) != 0) {
			LOG(log_error, logtype_cnid,
			    "sqlite query error: %s",
			    sqlite3_errmsg(db->cnid_sqlite_con));
			errno = CNID_ERR_DB;
			EC_FAIL;
		}
		id = retid;
	} else if (retdev != dev || retino != ino) {
		LOG(log_debug, logtype_cnid,
		    "cnid_sqlite_lookup(DID:%u, name: \"%s\"): changed dev/ino",
		    ntohl(did), name);
		if (cnid_sqlite_delete(cdb, retid) != 0) {
			LOG(log_error, logtype_cnid,
			    "sqlite query error: %s",
			    sqlite3_errmsg(db->cnid_sqlite_con));
			errno = CNID_ERR_DB;
			EC_FAIL;
		}
		errno = CNID_INVALID;
		EC_FAIL;
	} else {
		/* everythings good */
		id = retid;
	}

      EC_CLEANUP:
	LOG(log_debug, logtype_cnid, "cnid_sqlite_lookup: id: %" PRIu32,
	    ntohl(id));
	if (have_result)
		sqlite3_finalize(db->cnid_lookup_stmt);
	if (ret != 0)
		id = CNID_INVALID;
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

	uint64_t dev = st->st_dev;
	uint64_t ino = st->st_ino;
	db->cnid_sqlite_hint = hint;

	LOG(log_maxdebug, logtype_cnid,
	    "cnid_sqlite_add(did: %" PRIu32 ", name: \"%s\", hint: %" PRIu32
	    "): START", ntohl(did), name, ntohl(hint));

	do {
		if ((id =
		     cnid_sqlite_lookup(cdb, st, did, (char *) name,
				       len)) == CNID_INVALID) {
			if (errno == CNID_ERR_DB)
				EC_FAIL;
			/*
			 * If the CNID set overflowed before
			 * (CNID_SQLITE_FLAG_DEPLETED) ignore the CNID "hint"
			 * taken from the AppleDouble file
			 */
			if (!db->cnid_sqlite_hint
			    || (db->
				cnid_sqlite_flags &
				CNID_SQLITE_FLAG_DEPLETED)) {
				stmt = db->cnid_add_stmt;
			} else {
				stmt = db->cnid_put_stmt;
				stmt_param_id = ntohl(db->cnid_sqlite_hint);
			}
			strncpy(stmt_param_name, name,
				sizeof(stmt_param_name)-1);
			stmt_param_name_len = len;
			stmt_param_did = ntohl(did);
			stmt_param_dev = dev;
			stmt_param_ino = ino;

			ret = sqlite3_step(stmt);
			if (ret != SQLITE_ROW) {
				EC_FAIL;
			}
#if 0
				/*
				 * Race condition:
				 * between lookup and insert another process may have inserted
				 * this entry.
				 */
				if (db->cnid_sqlite_hint)
					db->cnid_sqlite_hint = CNID_INVALID;
				continue;
			}
#endif

			lastid = sqlite3_last_insert_rowid(db->cnid_sqlite_con);

			if (lastid > 0xffffffff) { /* CK this is wrong */
				/* CNID set is depleted, restart from scratch */
				EC_NEG1(cnid_sqlite_execute
					(db->cnid_sqlite_con,
					 "START TRANSACTION;"
					 "UPDATE volumes SET Depleted=1 WHERE VolUUID='%s';"
					 "TRUNCATE TABLE %s;"
					 "ALTER TABLE %s AUTO_INCREMENT = 17;"
					 "COMMIT;",
					 db->cnid_sqlite_voluuid_str,
					 db->cnid_sqlite_voluuid_str,
					 db->cnid_sqlite_voluuid_str));
				db->cnid_sqlite_flags |=
				    CNID_SQLITE_FLAG_DEPLETED;
				hint = CNID_INVALID;
#if 0 // again, really wrong
				do {
					result =
					    sqlite_store_result(db->
							       cnid_sqlite_con);
					if (result)
						sqlite_free_result(result);
				} while (sqlite_next_result
					 (db->cnid_sqlite_con) == 0);
				continue;
#endif
			}

			/* Finally assign our result */
			id = htonl((uint32_t) lastid);
		}
	} while (id == CNID_INVALID);

      EC_CLEANUP:
	LOG(log_debug, logtype_cnid, "cnid_sqlite_add: id: %" PRIu32,
	    ntohl(id));

	return id;
}

cnid_t cnid_sqlite_get(struct _cnid_db *cdb, const cnid_t did, char *name,
		      const size_t len)
{
	EC_INIT;
	CNID_sqlite_private *db;
	cnid_t id = CNID_INVALID;
	char *sql = NULL;
	sqlite3_stmt *transient_stmt = NULL;

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

	LOG(log_debug, logtype_cnid,
	    "cnid_sqlite_get(did: %" PRIu32 ", name: \"%s\"): START",
	    ntohl(did), name);

        EC_NEG1(ret = asprintf
                (&sql,
		  "SELECT Id FROM `%s` WHERE Name='%s' AND Did=?",
		  db->cnid_sqlite_voluuid_str, name) );
        EC_ZERO_LOG(sqlite3_prepare_v2
                    (db->cnid_sqlite_con, sql, strlen(sql), &transient_stmt, NULL));
        EC_ZERO_LOG(sqlite3_bind_int64(transient_stmt,
                    1, ntohl(did) ) );

        if (sqlite3_step(transient_stmt) != SQLITE_ROW) {
                if (sqlite3_errcode(db->cnid_sqlite_con) != 0) {
                        LOG(log_error, logtype_cnid,
                            "sqlite query error: %s",
                            sqlite3_errmsg(db->cnid_sqlite_con));
                        EC_FAIL;
                }
        }

	id = htonl(sqlite3_column_int64(transient_stmt, 1));

      EC_CLEANUP:
	LOG(log_debug, logtype_cnid, "cnid_sqlite_get: id: %" PRIu32,
	    ntohl(id));

	if (transient_stmt)
		sqlite3_finalize(transient_stmt);

	return id;
}

char *cnid_sqlite_resolve(struct _cnid_db *cdb, cnid_t * id, void *buffer,
			 size_t len)
{
	EC_INIT;
	char *sql = NULL;
	sqlite3_stmt *transient_stmt = NULL;
	CNID_sqlite_private *db;

	if (!cdb || !(db = cdb->cnid_db_private)) {
		LOG(log_error, logtype_cnid,
		    "cnid_sqlite_get: Parameter error");
		errno = CNID_ERR_PARAM;
		EC_FAIL;
	}

        EC_NEG1(ret = asprintf
                (&sql,
		  "SELECT Did,Name FROM `%s` WHERE Id=?",
		  db->cnid_sqlite_voluuid_str) );

        EC_ZERO_LOG(sqlite3_prepare_v2
                    (db->cnid_sqlite_con, sql, strlen(sql), &transient_stmt, NULL));
        EC_ZERO_LOG(sqlite3_bind_int64(transient_stmt,
                    1, ntohl(*id) ) );

        if (sqlite3_step(transient_stmt) != SQLITE_ROW) {
		EC_FAIL;
        }

        *id = htonl(sqlite3_column_int64(transient_stmt, 1));
	strncpy(buffer, (const char *)sqlite3_column_text(transient_stmt, 2), len);

      EC_CLEANUP:
	if (transient_stmt)
		sqlite3_finalize(transient_stmt);

#if 0 // handle this CK
	if (ret != 0) {
		*id = CNID_INVALID;
		return NULL;
	}
#endif
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
                  "SELECT Stamp FROM volumes WHERE VolPath='%s'",
                                cdb->cnid_db_vol->v_path));

        EC_ZERO_LOG(sqlite3_prepare_v2
                    (db->cnid_sqlite_con, sql, strlen(sql), &transient_stmt, NULL));

        if (sqlite3_step(transient_stmt) != SQLITE_ROW) {
		LOG(log_error, logtype_cnid,
		    "Can't get DB stamp for volumes \"%s\"",
		    cdb->cnid_db_vol->v_path);
                EC_FAIL;
        }

        strncpy(buffer, (const char *)sqlite3_column_text(transient_stmt, 1), len);

      EC_CLEANUP:
	if (transient_stmt)
		sqlite3_finalize(transient_stmt);
	EC_EXIT;
}


int cnid_sqlite_find(struct _cnid_db *cdb, const char *name, size_t namelen,
		    void *buffer, size_t buflen)
{
	LOG(log_error, logtype_cnid,
	    "cnid_sqlite_find(\"%s\"): not supported with sqlite CNID backend",
	    name);
	return -1;
}

cnid_t cnid_sqlite_rebuild_add(struct _cnid_db *cdb, const struct stat *st,
			      const cnid_t did, char *name, const size_t len,
			      cnid_t hint)
{
	LOG(log_error, logtype_cnid,
	    "cnid_sqlite_rebuild_add(\"%s\"): not supported with sqlite CNID backend",
	    name);
	return CNID_INVALID;
}

int cnid_sqlite_wipe(struct _cnid_db *cdb)
{
	EC_INIT;
	CNID_sqlite_private *db;

	if (!cdb || !(db = cdb->cnid_db_private)) {
		LOG(log_error, logtype_cnid, "cnid_wipe: Parameter error");
		errno = CNID_ERR_PARAM;
		return -1;
	}

	LOG(log_debug, logtype_cnid, "cnid_dbd_wipe");

	EC_NEG1(cnid_sqlite_execute(db->cnid_sqlite_con,
				    "START TRANSACTION;"
				    "UPDATE volumes SET Depleted=0 WHERE VolUUID='%s';"
				    "TRUNCATE TABLE `%s`;"
				    "ALTER TABLE `%s` AUTO_INCREMENT = 17;"
				    "COMMIT;",
				    db->cnid_sqlite_voluuid_str,
				    db->cnid_sqlite_voluuid_str,
				    db->cnid_sqlite_voluuid_str));


      EC_CLEANUP:
	EC_EXIT;
}

static struct _cnid_db *cnid_sqlite_new(const char *volpath)
{
	struct _cnid_db *cdb;

	if ((cdb =
	     (struct _cnid_db *) calloc(1,
					sizeof(struct _cnid_db))) == NULL)
		return NULL;

        if ((cdb->cnid_db_vol = strdup(volpath)) == NULL) {
                free(cdb);
                return NULL;
        }

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
//	cdb->cnid_wipe = cnid_sqlite_wipe;
	return cdb;
}

/* ---------------------- */
struct _cnid_db *cnid_sqlite_open(struct cnid_open_args *args)
{
	EC_INIT;
	CNID_sqlite_private *db = NULL;
	struct _cnid_db *cdb = NULL;
	char *sql = NULL;
	const char *vol = args->cnid_args_vol;
	sqlite3_stmt *transient_stmt = NULL;

	EC_NULL(cdb = cnid_sqlite_new(vol));
	EC_NULL(db =
		(CNID_sqlite_private *) calloc(1,
					       sizeof
					       (CNID_sqlite_private)));
	cdb->cnid_db_private = db;

	/* Initialize and connect to sqlite3 database */
	sqlite3_initialize();
	EC_ZERO( sqlite3_open_v2("file:/var/db/netatalk/cnid.sqlite",
			    &db->cnid_sqlite_con,
			    SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, NULL));


	/* Add volume to volume table */
	if (cnid_sqlite_execute(db->cnid_sqlite_con,
				"CREATE TABLE IF NOT EXISTS volumes "
				"( VolUUID CHAR(32) PRIMARY KEY,"
				"VolPath TEXT(4096),"
				"Stamp BINARY(8),"
				"Depleted INT,"
				"INDEX(VolPath(64))"
				")"))
	{
		LOG(log_error, logtype_cnid, "sqlite query error: %s",
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
		  "INSERT INTO volumes "
		  "(VolUUID, Volpath, Stamp, Depleted) "
		  "VALUES(?, ?, ?, 0)"));
        EC_ZERO_LOG(sqlite3_prepare_v2
                    (db->cnid_sqlite_con, sql, strlen(sql), &transient_stmt, NULL));
        EC_ZERO_LOG(sqlite3_bind_text(transient_stmt,
                    1, db->cnid_sqlite_voluuid_str,
		    strlen(db->cnid_sqlite_voluuid_str), SQLITE_STATIC) );
        EC_ZERO_LOG(sqlite3_bind_text(transient_stmt,
                    2, vol,
		    strlen(vol), SQLITE_STATIC) );
        EC_ZERO_LOG(sqlite3_bind_text(transient_stmt,
                    3, stamp,
		    strlen(stamp), SQLITE_STATIC) );

	sqlite3_step(transient_stmt);
#if 0 // CK must figure out how to check for failed insert
	if (sqlite3_step(transient_stmt)) {
		if (mysql_errno(db->cnid_sqlite_con) != ER_DUP_ENTRY) {
			LOG(log_error, logtype_cnid,
			    "sqlite query error: %s",
			    sqlite3_errmsg(db->cnid_sqlite_con));
			EC_FAIL;
		}
	}
#endif
	sqlite3_finalize(transient_stmt);

#if 0 // CK FIX!
	/*
	 * Check whether CNID set overflowed before.
	 * If that's the case, in cnid_sqlite_add() we'll ignore the CNID
	 * "hint" taken from the AppleDouble file.
	 */
	if (cnid_sqlite_execute(db->cnid_sqlite_con,
				"SELECT Depleted FROM volumes WHERE VolUUID='%s'",
				db->cnid_sqlite_voluuid_str)) {
		LOG(log_error, logtype_cnid, "sqlite query error: %s",
		    sqlite3_errmsg(db->cnid_sqlite_con));
		EC_FAIL;
	}
	if ((result = mysql_store_result(db->cnid_sqlite_con)) == NULL) {
		LOG(log_error, logtype_cnid, "sqlite query error: %s",
		    sqlite3_errmsg(db->cnid_sqlite_con));
		errno = CNID_ERR_DB;
		EC_FAIL;
	}
	if (mysql_num_rows(result)) {
		row = mysql_fetch_row(result);
		int depleted = atoi(row[0]);
		if (depleted)
			db->cnid_sqlite_flags |= CNID_SQLITE_FLAG_DEPLETED;
	}
	mysql_free_result(result);
	result = NULL;
#endif

	/* Create volume table */
	if (cnid_sqlite_execute(db->cnid_sqlite_con,
				"CREATE TABLE IF NOT EXISTS `%s`"
				"(Id BIGINT UNSIGNED PRIMARY KEY AUTO_INCREMENT,"
				"Name VARCHAR(255) NOT NULL,"
				"Did INT UNSIGNED NOT NULL,"
				"DevNo BIGINT UNSIGNED NOT NULL,"
				"InodeNo BIGINT UNSIGNED NOT NULL,"
				"UNIQUE DidName(Did, Name), UNIQUE DevIno(DevNo, InodeNo)) "
				"AUTO_INCREMENT=17",
				db->cnid_sqlite_voluuid_str)) {
		LOG(log_error, logtype_cnid, "sqlite query error: %s",
		    sqlite3_errmsg(db->cnid_sqlite_con));
		EC_FAIL;
	}

	EC_ZERO(init_prepared_stmt(db));

	LOG(log_debug, logtype_cnid,
	    "Finished initializing sqlite CNID module for volume '%s'",
	    vol);

      EC_CLEANUP:
        if (transient_stmt)
                sqlite3_finalize(transient_stmt);

	if (ret != 0) {
		if (cdb)
			free(cdb);
		cdb = NULL;
		if (db)
			free(db);
	}
	return cdb;
}

struct _cnid_module cnid_sqlite_module = {
	"sqlite",
	{ NULL, NULL },
	cnid_sqlite_open,
	0
};
