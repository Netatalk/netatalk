/*
 * $Id: cnid_add.c,v 1.24 2002-01-18 04:51:27 jmarcus Exp $
 *
 * Copyright (c) 1999. Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 * cnid_add (db, dev, ino, did, name, hint):
 * add a name to the CNID database. we use both dev/ino and did/name
 * to keep track of things.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_DB
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <errno.h>
#include <atalk/logger.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#include <db.h>
#include <netatalk/endian.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/util.h>

#include "cnid_private.h"

/* add an entry to the CNID databases. we do this as a transaction
 * to prevent messiness. */
static int add_cnid(CNID_private *db, DBT *key, DBT *data) {
    DBT altkey, altdata;
    DB_TXN *tid;
    /* We create rc here because using errno is bad.  Why?  Well, if you
     * use errno once, then call another function which resets it, you're
     * screwed. */
    int rc, ret;

    memset(&altkey, 0, sizeof(altkey));
    memset(&altdata, 0, sizeof(altdata));

retry:
    if ((rc = txn_begin(db->dbenv, NULL, &tid, 0)) != 0) {
        return rc;
    }

    /* main database */
    if ((rc = db->db_cnid->put(db->db_cnid, tid, key, data, DB_NOOVERWRITE))) {
        if (rc == DB_LOCK_DEADLOCK) {
            if ((ret = txn_abort(tid)) != 0) {
                return ret;
            }
            goto retry;
        }
        goto abort;
    }

    /* dev/ino database */
    altkey.data = data->data;
    altkey.size = CNID_DEVINO_LEN;
    altdata.data = key->data;
    altdata.size = key->size;
    if ((rc = db->db_devino->put(db->db_devino, tid, &altkey, &altdata, 0))) {
        if (rc == DB_LOCK_DEADLOCK) {
            if ((ret = txn_abort(tid)) != 0) {
                return ret;
            }
            goto retry;
        }
        goto abort;
    }

    /* did/name database */
    altkey.data = (char *) data->data + CNID_DEVINO_LEN;
    altkey.size = data->size - CNID_DEVINO_LEN;
    if ((rc = db->db_didname->put(db->db_didname, tid, &altkey, &altdata, 0))) {
        if (rc == DB_LOCK_DEADLOCK) {
            if ((ret = txn_abort(tid)) != 0) {
                return ret;
            }
            goto retry;
        }
        goto abort;
    }

    if ((rc = txn_commit(tid, 0)) != 0) {
        LOG(log_error, logtype_default, "add_cnid: Failed to commit transaction: %s", db_strerror(rc));
        return rc;
    }

    return 0;

abort:
    if ((ret = txn_abort(tid)) != 0) {
        return ret;
    }
    return rc;
}

cnid_t cnid_add(void *CNID, const struct stat *st,
                const cnid_t did, const char *name, const int len,
                cnid_t hint)
{
    CNID_private *db;
    DBT key, data, rootinfo_key, rootinfo_data;
    DB_TXN *tid;
    struct timeval t;
    cnid_t id, save;
    int rc;

    if (!(db = CNID) || !st || !name) {
        return CNID_ERR_PARAM;
    }

    /* Do a lookup. */
    id = cnid_lookup(db, st, did, name, len);
    /* ... Return id if it is valid, or if Rootinfo is read-only. */
    if (id || (db->flags & CNIDFLAG_DB_RO)) {
#ifdef DEBUG
        LOG(log_info, logtype_default, "cnid_add: Looked up did %u, name %s as %u",
            ntohl(did), name, ntohl(id));
#endif
        return id;
    }

    /* Initialize our DBT data structures. */
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    /* Just tickle hint, and the key will change (gotta love pointers). */
    key.data = &hint;
    key.size = sizeof(hint);

    if ((data.data = make_cnid_data(st, did, name, len)) == NULL) {
        LOG(log_error, logtype_default, "cnid_add: Path name is too long");
        return CNID_ERR_PATH;
    }

    data.size = CNID_HEADER_LEN + len + 1;

    /* Start off with the hint.  It should be in network byte order.
     * We need to make sure that somebody doesn't add in restricted
     * cnid's to the database. */
    if (ntohl(hint) >= CNID_START) {
        /* If the key doesn't exist, add it in.  Don't fiddle with nextID. */
        rc = add_cnid(db, &key, &data);
        switch (rc) {
        case DB_KEYEXIST: /* Need to use RootInfo after all. */
            break;
        default:
            LOG(log_error, logtype_default, "cnid_add: Unable to add CNID %u: %s",
                ntohl(hint), db_strerror(rc));
            return CNID_ERR_DB;
        case 0:
#ifdef DEBUG
            LOG(log_info, logtype_default, "cnid_add: Used hint for did %u, name %s as %u",
                ntohl(did), name, ntohl(hint));
#endif
            return hint;
        }
    }

    /* We need to create a random sleep interval to prevent deadlocks. */
    /*(void)srand(getpid() ^ time(NULL));
    t.tv_sec = 0;*/

    memset(&rootinfo_key, 0, sizeof(rootinfo_key));
    memset(&rootinfo_data, 0, sizeof(rootinfo_data));
    rootinfo_key.data = ROOTINFO_KEY;
    rootinfo_key.size = ROOTINFO_KEYLEN;

retry:
    if ((rc = txn_begin(db->dbenv, NULL, &tid, 0)) != 0) {
        LOG(log_error, logtype_default, "cnid_add: Failed to begin transaction: %s", db_strerror(rc));
        return CNID_ERR_DB;
    }

    /* Get the key. */
    switch (rc = db->db_didname->get(db->db_didname, tid, &rootinfo_key,
                                     &rootinfo_data, DB_RMW)) {
    case DB_LOCK_DEADLOCK:
        if ((rc = txn_abort(tid)) != 0) {
            LOG(log_error, logtype_default, "cnid_add: txn_abort: %s", db_strerror(rc));
            return CNID_ERR_DB;
        }
        goto retry;
    case 0:
        memcpy(&hint, rootinfo_data.data, sizeof(hint));
#ifdef DEBUG
        LOG(log_info, logtype_default, "cnid_add: Found rootinfo for did %u, name %s as %u", ntohl(did), name, ntohl(hint));
#endif
        break;
    case DB_NOTFOUND:
        hint = htonl(CNID_START);
#ifdef DEBUG
        LOG(log_info, logtype_default, "cnid_add: Using CNID_START for did %u, name %s",
            ntohl(did), name);
#endif
        break;
    default:
        LOG(log_error, logtype_default, "cnid_add: Unable to lookup rootinfo: %s", db_strerror(rc));
        goto cleanup_abort;
    }

    id = ntohl(hint);
    if (id <= CNID_START) {
        id = CNID_START;
    }
    else {
        id++;
    }

    /* If we've hit the MAX CNID allowed, we return a fatal error.  CNID needs
     * to be recycled before proceding. */
    if (id == CNID_MAX) {
        txn_abort(tid);
        LOG(log_error, logtype_default, "cnid_add: FATAL: Cannot add CNID for %s.  CNID database has reached its limit.", name);
        return CNID_ERR_MAX;
    }

    hint = htonl(id);

    rootinfo_data.data = &hint;
    rootinfo_data.size = sizeof(hint);

    switch (rc = db->db_didname->put(db->db_didname, tid, &rootinfo_key, &rootinfo_data, 0)) {
    case DB_LOCK_DEADLOCK:
        if ((rc = txn_abort(tid)) != 0) {
            LOG(log_error, logtype_default, "cnid_add: txn_abort: %s", db_strerror(rc));
            return CNID_ERR_DB;
        }
        goto retry;
    case 0:
        break;
    default:
        LOG(log_error, logtype_default, "cnid_add: Unable to update rootinfo: %s", db_strerror(rc));
        goto cleanup_abort;
    }

    /* The transaction finished, commit it. */
    if ((rc = txn_commit(tid, 0)) != 0) {
        LOG(log_error, logtype_default, "cnid_add: Unable to commit transaction: %s", db_strerror(rc));
        return CNID_ERR_DB;
    }

    /* Now we need to add the CNID data to the databases. */
    rc = add_cnid(db, &key, &data);
    if (rc) {
        LOG(log_error, logtype_default, "cnid_add: Failed to add CNID for %s to database using hint %u: %s", name, ntohl(hint), db_strerror(rc));
        return CNID_ERR_DB;
    }

#ifdef DEBUG
    LOG(log_info, logtype_default, "cnid_add: Returned CNID for did %u, name %s as %u", ntohl(did), name, ntohl(hint));
#endif

    return hint;

cleanup_abort:
    txn_abort(tid);

    return CNID_ERR_DB;
}
#endif /* CNID_DB */

