/*
 * $Id: cnid_delete.c,v 1.14 2002-08-30 03:12:52 jmarcus Exp $
 *
 * Copyright (c) 1999. Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 * cnid_delete: delete a CNID from the database 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_DB
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <atalk/logger.h>

#include <db.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/cnid.h>

#include "cnid_private.h"

#ifdef CNID_DB_CDB
    #define tid    NULL
#endif /* CNID_DB_CDB */

int cnid_delete(void *CNID, const cnid_t id) {
    CNID_private *db;
    DBT key, data;
#ifndef CNID_DB_CDB
    DB_TXN *tid;
#endif /* CNID_DB_CDB */
    int rc, found = 0;

    if (!(db = CNID) || !id || (db->flags & CNIDFLAG_DB_RO)) {
        return -1;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    /* Get from ain CNID database. */
    key.data = (cnid_t *)&id;
    key.size = sizeof(id);
    while (!found) {
        rc = db->db_cnid->get(db->db_cnid, NULL, &key, &data, 0);
        switch (rc) {
        case 0:
            found = 1;
            break;
#ifndef CNID_DB_CDB
        case DB_LOCK_DEADLOCK:
            break;
#endif /* CNID_DB_CDB */
        case DB_NOTFOUND:
#ifdef DEBUG
            LOG(log_info, logtype_default, "cnid_delete: CNID %u not in database",
                ntohl(id));
#endif
            return 0;
        default:
            LOG(log_error, logtype_default, "cnid_delete: Unable to delete entry: %s",
                db_strerror(rc));
            return rc;
        }
    }

#ifndef CNID_DB_CDB
retry:
    if ((rc = txn_begin(db->dbenv, NULL, &tid, 0)) != 0) {
        LOG(log_error, logtype_default, "cnid_delete: Failed to begin transaction: %s",
            db_strerror(rc));
        return rc;
    }
#endif /* CNID_DB_CDB */

    /* Now delete from the main CNID database. */
    key.data = (cnid_t *)&id;
    key.size = sizeof(id);
    if ((rc = db->db_cnid->del(db->db_cnid, tid, &key, 0))) {
#ifndef CNID_DB_CDB
        int ret;
        if ((ret = txn_abort(tid)) != 0) {
            LOG(log_error, logtype_default, "cnid_delete: txn_abort: %s", db_strerror(ret));
            return ret;
        }
        switch (rc) {
        case DB_LOCK_DEADLOCK:
            goto retry;
        default:
#endif /* CNID_DB_CDB */
            goto abort_err;
#ifndef CNID_DB_CDB
        }
#endif /* CNID_DB_CDB */
    }

    /* Now delete from dev/ino database. */
    key.data = data.data;
    key.size = CNID_DEVINO_LEN;
    if ((rc = db->db_devino->del(db->db_devino, tid, &key, 0))) {
        switch (rc) {
#ifndef CNID_DB_CDB
        case DB_LOCK_DEADLOCK:
            if ((rc = txn_abort(tid)) != 0) {
                LOG(log_error, logtype_default, "cnid_delete: txn_abort: %s",
                    db_strerror(rc));
                return rc;
            }
            goto retry;
#endif /* CNID_DB_CDB */
        case DB_NOTFOUND:
            /* Quietly fall through if the entry isn't found. */
            break;
        default:
#ifndef CNID_DB_CDB
            if ((rc = txn_abort(tid)) != 0) {
                LOG(log_error, logtype_default, "cnid_delete: txn_abort: %s",
                    db_strerror(rc));
                return rc;
            }
#endif /* CNID_DB_CDB */
            goto abort_err;
        }
    }

    /* Get data from the did/name database.
     * TODO Also handle did/macname, did/shortname, and did/longname. */
    key.data = (char *)data.data + CNID_DEVINO_LEN;
    key.size = data.size - CNID_DEVINO_LEN;
    if ((rc = db->db_didname->del(db->db_didname, tid, &key, 0))) {
        switch (rc) {
#ifndef CNID_DB_CDB
        case DB_LOCK_DEADLOCK:
            if ((rc = txn_abort(tid)) != 0) {
                LOG(log_error, logtype_default, "cnid_delete: txn_abort: %s",
                    db_strerror(rc));
                return rc;
            }
            goto retry;
#endif /* CNID_DB_CDB */
        case DB_NOTFOUND:
            break;
        default:
#ifndef CNID_DB_CDB
            if ((rc = txn_abort(tid)) != 0) {
                LOG(log_error, logtype_default, "cnid_delete: txn_abort: %s",
                    db_strerror(rc));
                return rc;
            }
#endif /* CNID_DB_CDB */
            goto abort_err;
        }
    }

#ifdef DEBUG
    LOG(log_info, logtype_default, "cnid_delete: Deleting CNID %u", ntohl(id));
#endif
#ifndef CNID_DB_CDB
    if ((rc = txn_commit(tid, 0)) != 0) {
        LOG(log_error, logtype_default, "cnid_delete: Failed to commit transaction: %s",
            db_strerror(rc));
        return rc;
    }
#endif /* CNID_DB_CDB */
    return 0;

abort_err:
    LOG(log_error, logtype_default, "cnid_delete: Unable to delete CNID %u: %s",
        ntohl(id), db_strerror(rc));
    return rc;
}
#endif /*CNID_DB */
