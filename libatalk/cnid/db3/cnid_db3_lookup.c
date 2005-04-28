
/*
 * $Id: cnid_db3_lookup.c,v 1.2 2005-04-28 20:49:59 bfernhomberg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_BACKEND_DB3

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <atalk/logger.h>
#include <errno.h>

#ifdef HAVE_DB4_DB_H
#include <db4/db.h>
#else
#include <db.h>
#endif
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include "cnid_db3.h"

#include "cnid_db3_private.h"

#define LOGFILEMAX    100       /* kbytes */
#define CHECKTIMEMAX   30       /* minutes */

/* This returns the CNID corresponding to a particular file.  It will
 * also fix up the various databases if there's a problem. */
cnid_t cnid_db3_lookup(struct _cnid_db *cdb, const struct stat *st, const cnid_t did, char *name, const int len)
{
    char *buf;
    CNID_private *db;
    DBT key, devdata, diddata;
    int devino = 1, didname = 1;
    cnid_t id = 0;
    int rc;

    if (!cdb || !(db = cdb->_private) || !st || !name) {
        return 0;
    }

    /* Do a little checkpointing if necessary.  I stuck it here as cnid_lookup
     * gets called when we do directory lookups.  Only do this if we're using
     * a read-write database. */
    if ((db->flags & CNIDFLAG_DB_RO) == 0) {
#ifdef DEBUG
        LOG(log_info, logtype_default, "cnid_lookup: Running database checkpoint");
#endif
#if DB_VERSION_MAJOR > 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1)
        if ((rc = db->dbenv->txn_checkpoint(db->dbenv, LOGFILEMAX, CHECKTIMEMAX, 0))) {
            LOG(log_error, logtype_default, "cnid_lookup: txn_checkpoint: %s", db_strerror(rc));
            return 0;
        }
#else
#if DB_VERSION_MAJOR >= 4
        switch (rc = db->dbenv->txn_checkpoint(db->dbenv, LOGFILEMAX, CHECKTIMEMAX, 0)) {
#else
        switch (rc = txn_checkpoint(db->dbenv, LOGFILEMAX, CHECKTIMEMAX, 0)) {
#endif /* DB_VERSION_MAJOR >= 4 */
        case 0:
        case DB_INCOMPLETE:
            break;
        default:
            LOG(log_error, logtype_default, "cnid_lookup: txn_checkpoint: %s", db_strerror(rc));
            return 0;
        }
#endif /* DB_VERSION_MAJOR > 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1) */
    }

    if ((buf = make_cnid_data(st, did, name, len)) == NULL) {
        LOG(log_error, logtype_default, "cnid_lookup: Pathname is too long");
        return 0;
    }

    memset(&key, 0, sizeof(key));
    memset(&devdata, 0, sizeof(devdata));
    memset(&diddata, 0, sizeof(diddata));

    /* Look for a CNID.  We have two options: dev/ino or did/name.  If we
     * only get a match in one of them, that means a file has moved. */
    key.data = buf;
    key.size = CNID_DEVINO_LEN;
    while ((rc = db->db_devino->get(db->db_devino, NULL, &key, &devdata, 0))) {
        if (rc == DB_LOCK_DEADLOCK) {
            continue;
        }

        if (rc == DB_NOTFOUND) {
            devino = 0;
            break;
        }

        LOG(log_error, logtype_default, "cnid_lookup: Unable to get CNID dev %u, ino %u: %s",
            st->st_dev, st->st_ino, db_strerror(rc));
        return 0;
    }

    /* did/name now */
    key.data = buf + CNID_DEVINO_LEN;
    key.size = CNID_DID_LEN + len + 1;
    while ((rc = db->db_didname->get(db->db_didname, NULL, &key, &diddata, 0))) {
        if (rc == DB_LOCK_DEADLOCK) {
            continue;
        }

        if (rc == DB_NOTFOUND) {
            didname = 0;
            break;
        }

        LOG(log_error, logtype_default, "cnid_lookup: Unable to get CNID %u, name %s: %s",
            ntohl(did), name, db_strerror(rc));
        return 0;
    }

    /* Set id.  Honor did/name over dev/ino as dev/ino isn't necessarily
     * 1-1. */
    if (didname) {
        memcpy(&id, diddata.data, sizeof(id));
    } else if (devino) {
        memcpy(&id, devdata.data, sizeof(id));
    }

    /* Either entries are in both databases or neither of them. */
    if ((devino && didname) || !(devino || didname)) {
#ifdef DEBUG
        LOG(log_info, logtype_default, "cnid_lookup: Looked up did %u, name %s, as %u", ntohl(did), name, ntohl(id));
#endif
        return id;
    }

    /* Fix up the database. */
    cnid_db3_update(cdb, id, st, did, name, len);
#ifdef DEBUG
    LOG(log_info, logtype_default, "cnid_lookup: Looked up did %u, name %s, as %u (needed update)", ntohl(did), name,
        ntohl(id));
#endif
    return id;
}


#endif /* CNID_BACKEND_DB3 */
