/*
 * $Id: cnid_mangle_add.c,v 1.5 2003-01-04 20:49:33 jmarcus Exp $
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

#ifdef CNID_DB_CDB
    #define tid    NULL
#endif /* CNID_DB_CDB */

/* Add a mangled filename. */
int
cnid_mangle_add(void *CNID, char *mfilename, char *filename)
{
    CNID_private *db;
    DBT key, data;
#ifndef CNID_DB_CDB
    DB_TXN *tid;
#endif /* CNID_DB_CDB */
    cnid_t id;
    int rc, ret;

    if (!(db = CNID)) {
        return -1;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    key.data = mfilename;
    key.size = strlen(mfilename);
    data.data = filename;
    data.size = strlen(filename) + 1;

#ifndef CNID_DB_CDB
retry:
    if ((rc = txn_begin(db->dbenv, NULL, &tid, 0)) != 0) {
        LOG(log_error, logtype_default, "cnid_mangle_add: Failed to begin transaction: %s", db_strerror(rc));
        return -1;
    }
#endif /* CNID_DB_CDB */

    if ((rc = db->db_mangle->put(db->db_mangle, tid, &key, &data, 0))) {
#ifndef CNID_DB_CDB
        if ((ret = txn_abort(tid)) != 0) {
            LOG(log_error, logtype_default, "cnid_mangle_add: txn_abort: %s", db_strerror(ret));
            return -1;
        }
#endif /* CNID_DB_CDB */
        switch (rc) {
#ifndef CNID_DB_CDB
        case DB_LOCK_DEADLOCK:
            goto retry;
#endif /* CNID_DB_CDB */
        default:
            LOG(log_error, logtype_default, "cnid_mangle_add: Failed to add mangled filename to the database: %s", db_strerror(rc));
            return -1;
        }
    }

#ifndef CNID_DB_CDB
    if ((rc = txn_commit(tid, 0)) != 0) {
        LOG(log_error, logtype_default, "cnid_mangle_add: Unable to commit transaction: %s", db_strerror(rc));
        return -1;
    }
#endif /* CNID_DB_CDB */

    return 0;
}
#endif /* FILE_MANGLING */
