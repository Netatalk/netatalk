/*
 * $Id: cnid_delete.c,v 1.11 2001-12-10 03:51:56 jmarcus Exp $
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
#include <syslog.h>

#include <db.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/cnid.h>

#include "cnid_private.h"

int cnid_delete(void *CNID, const cnid_t id) {
    CNID_private *db;
    DBT key, data;
    DB_TXN *tid;
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
        case DB_LOCK_DEADLOCK:
            break;
        case DB_NOTFOUND:
#ifdef DEBUG
            syslog(LOG_INFO, "cnid_delete: CNID %u not in database",
                   ntohl(id));
#endif
            return 0;
        default:
            syslog(LOG_ERR, "cnid_delete: Unable to delete entry: %s",
                   db_strerror(rc));
            return rc;
        }
    }

retry:
    if ((rc = txn_begin(db->dbenv, NULL, &tid, 0)) != 0) {
        syslog(LOG_ERR, "cnid_delete: Failed to begin transaction: %s",
               db_strerror(rc));
        return rc;
    }

    /* Now delete from the main CNID database. */
    key.data = (cnid_t *)&id;
    key.size = sizeof(id);
    if ((rc = db->db_cnid->del(db->db_cnid, tid, &key, 0))) {
        int ret;
        if ((ret = txn_abort(tid)) != 0) {
            syslog(LOG_ERR, "cnid_delete: txn_abort: %s", db_strerror(ret));
            return ret;
        }
        switch (rc) {
        case DB_LOCK_DEADLOCK:
            goto retry;
        default:
            goto abort_err;
        }
    }

    /* Now delete from dev/ino database. */
    key.data = data.data;
    key.size = CNID_DEVINO_LEN;
    if ((rc = db->db_devino->del(db->db_devino, tid, &key, 0))) {
        switch (rc) {
        case DB_LOCK_DEADLOCK:
            if ((rc = txn_abort(tid)) != 0) {
                syslog(LOG_ERR, "cnid_delete: txn_abort: %s",
                       db_strerror(rc));
                return rc;
            }
            goto retry;
        case DB_NOTFOUND:
            /* Quietly fall through if the entry isn't found. */
            break;
        default:
            if ((rc = txn_abort(tid)) != 0) {
                syslog(LOG_ERR, "cnid_delete: txn_abort: %s",
                       db_strerror(rc));
                return rc;
            }
            goto abort_err;
        }
    }

    /* Get data from the did/name database.
     * TODO Also handle did/macname, did/shortname, and did/longname. */
    key.data = (char *)data.data + CNID_DEVINO_LEN;
    key.size = data.size - CNID_DEVINO_LEN;
    if ((rc = db->db_didname->del(db->db_didname, tid, &key, 0))) {
        switch (rc) {
        case DB_LOCK_DEADLOCK:
            if ((rc = txn_abort(tid)) != 0) {
                syslog(LOG_ERR, "cnid_delete: txn_abort: %s",
                       db_strerror(rc));
                return rc;
            }
            goto retry;
        case DB_NOTFOUND:
            break;
        default:
            if ((rc = txn_abort(tid)) != 0) {
                syslog(LOG_ERR, "cnid_delete: txn_abort: %s",
                       db_strerror(rc));
                return rc;
            }
            goto abort_err;
        }
    }

#ifdef DEBUG
    syslog(LOG_INFO, "cnid_delete: Deleting CNID %u", ntohl(id));
#endif
    if ((rc = txn_commit(tid, 0)) != 0) {
        syslog(LOG_ERR, "cnid_delete: Failed to commit transaction: %s",
               db_strerror(rc));
        return rc;
    }
    return 0;

abort_err:
    syslog(LOG_ERR, "cnid_delete: Unable to delete CNID %u: %s",
           ntohl(id), db_strerror(rc));
    return rc;
}
#endif /*CNID_DB */
