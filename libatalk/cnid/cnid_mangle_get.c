/*
 * $Id: cnid_mangle_get.c,v 1.2 2002-05-30 06:41:19 jmarcus Exp $
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

/* Find a mangled filename entry. */
char *
cnid_mangle_get(void *CNID, char *mfilename)
{
    CNID_private *db;
    DBT key, data;
    DB_TXN *tid;
    cnid_t id;
    struct stat st;
    char *filename;
    int rc;

    if (!(db = CNID)) {
        return NULL;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    key.data = mfilename;
    key.size = strlen(mfilename);

    while ((rc = db->db_mangle->get(db->db_mangle, NULL, &key, &data, 0))) {
        if (rc == DB_LOCK_DEADLOCK) {
            continue;
        }

        if (rc == DB_NOTFOUND) {
	    LOG(log_error, logtype_default, "cnid_mangle_get: Failed to find mangled entry for %s", mfilename);
	    return NULL;

        }

        return NULL;
    }

    filename = strrchr((char *)data.data, '/');
    filename++; /* Skip the leading '/' */

    if (stat((char *)data.data, &st) < 0) {
	if (errno == ENOENT) {
	    /* The file no longer exists.  Purge it from the database. */
/*retry:
	    if ((rc = txn_begin(db->dbenv, NULL, &tid, 0)) != 0) {
		LOG(log_error, logtype_default, "cnid_mangle_get: Failed to begin transaction: %s", db_strerror(rc));
		return filename;
	    }

	    if ((rc = db->db_mangle->del(db->db_mangle, tid, &key, 0))) {
		int ret;
		if ((ret = txn_abort(tid)) != 0) {
		    LOG(log_error, logtype_default, "cnid_mangle_get: txn_abort: %s", db_strerror(ret));
		    return filename;
		}
		switch (rc) {
		    case DB_LOCK_DEADLOCK:
		    	goto retry;
		    default:
		    	LOG(log_error, logtype_default, "cnid_mangle_get: Unable to delete mangled file entry: %s", mfilename);
			return filename;
		}
	    }*/
	    return NULL;
	}
    }

    return filename;
}
#endif /* FILE_MANGLING */
