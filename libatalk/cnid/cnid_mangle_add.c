/*
 * $Id: cnid_mangle_add.c,v 1.1 2002-05-29 18:02:59 jmarcus Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef FILE_MANGLING
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <string.h>
#include <atalk/logger.h>
#include <errno.h>

#include <db.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/cnid.h>

#include "cnid_private.h"

/* Add a mangled filename. */
int
cnid_mangle_add(void *CNID, char *mfilename, char *filename)
{
    CNID_private *db;
    DBT key, data;
    DB_TXN *tid;
    cnid_t id;
    struct stat st;
    int rc, ret;

    if (!(db = CNID)) {
        return -1;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    key.data = mfilename;
    key.size = strlen(mfilename) + 1;
    data.data = filename;
    data.size = strlen(filename) + 1;

retry:
    if ((rc = txn_begin(db->dbenv, NULL, &tid, 0)) != 0) {
	LOG(log_error, logtype_default, "cnid_mangle_add: Failed to begin transaction: %s", db_strerror(rc));
	return -1;
    }

    if ((rc = db->db_mangle->put(db->db_mangle, tid, &key, &data, 0))) {
	if ((ret = txn_abort(tid)) != 0) {
	    LOG(log_error, logtype_default, "cnid_mangle_add: txn_abort: %s", db_strerror(ret));
	    return -1;
	}
	switch (rc) {
	    case DB_LOCK_DEADLOCK:
	    	goto retry;
	    default:
	    	LOG(log_error, logtype_default, "cnid_mangle_add: Failed to add mangled filename to the database: %s", db_strerror(rc));
		return -1;
	}
    }

    return 0;
}
#endif /* FILE_MANGLING */
