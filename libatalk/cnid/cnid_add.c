/*
 * $Id: cnid_add.c,v 1.18 2001-12-12 09:16:20 jmarcus Exp $
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
#include <syslog.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#include <db.h>
#include <netatalk/endian.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/util.h>

#include "cnid_private.h"

#define MAX_ABORTS 255

/* add an entry to the CNID databases. we do this as a transaction
 * to prevent messiness. */
static int add_cnid(CNID_private *db, DB_TXN *ptid, DBT *key, DBT *data) {
    DBT altkey, altdata;
    DB_TXN *tid;
    /* We create rc here because using errno is bad.  Why?  Well, if you
     * use errno once, then call another function which resets it, you're
     * screwed. */
    int rc, ret, aborts = 0;

    memset(&altkey, 0, sizeof(altkey));
    memset(&altdata, 0, sizeof(altdata));

    if (0) {
retry:
        if ((rc = txn_abort(tid)) != 0) {
            return rc;
        }
        if (++aborts > MAX_ABORTS) {
            return DB_LOCK_DEADLOCK;
        }
    }

    if ((rc = txn_begin(db->dbenv, ptid, &tid, 0)) != 0) {
        return rc;
    }

    /* main database */
    if ((rc = db->db_cnid->put(db->db_cnid, tid, key, data, DB_NOOVERWRITE))) {
        if (rc == DB_LOCK_DEADLOCK) {
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
            goto retry;
        }

        goto abort;
    }


    /* did/name database */
    altkey.data = (char *) data->data + CNID_DEVINO_LEN;
    altkey.size = data->size - CNID_DEVINO_LEN;
    if ((rc = db->db_didname->put(db->db_didname, tid, &altkey, &altdata, 0))) {
        if (rc == DB_LOCK_DEADLOCK) {
            goto retry;
        }

        goto abort;
    }

    if ((rc = txn_commit(tid, 0)) != 0) {
        syslog(LOG_ERR, "add_cnid: Failed to commit transaction: %s",
               db_strerror(rc));
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
        return 0;
    }

    /* Do a lookup. */
    id = cnid_lookup(db, st, did, name, len);
    /* ... Return id if it is valid, or if Rootinfo is read-only. */
    if (id || (db->flags & CNIDFLAG_DB_RO)) {
#ifdef DEBUG
        syslog(LOG_INFO, "cnid_add: Looked up did %u, name %s as %u",
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
        syslog(LOG_ERR, "cnid_add: Path name is too long");
        goto cleanup_err;
    }

    data.size = CNID_HEADER_LEN + len + 1;

    /* Start off with the hint.  It should be in network byte order.
     * We need to make sure that somebody doesn't add in restricted
     * cnid's to the database. */
    if (ntohl(hint) >= CNID_START) {
        /* If the key doesn't exist, add it in.  Don't fiddle with nextID. */
        rc = add_cnid(db, NULL, &key, &data);
        switch (rc) {
        case DB_KEYEXIST: /* Need to use RootInfo after all. */
            break;
        default:
            syslog(LOG_ERR, "cnid_add: Unable to add CNID %u: %s",
                   ntohl(hint), db_strerror(rc));
            goto cleanup_err;
        case 0:
#ifdef DEBUG
            syslog(LOG_INFO, "cnid_add: Used hint for did %u, name %s as %u",
                   ntohl(did), name, ntohl(hint));
#endif
            return hint;
        }
    }

    /* We need to create a random sleep interval to prevent deadlocks. */
    (void)srand(getpid() ^ time(NULL));
    t.tv_sec = 0;

    memset(&rootinfo_key, 0, sizeof(rootinfo_key));
    memset(&rootinfo_data, 0, sizeof(rootinfo_data));
    rootinfo_key.data = ROOTINFO_KEY;
    rootinfo_key.size = ROOTINFO_KEYLEN;

    /* Get the key. */
retry_get:
    switch (rc = db->db_didname->get(db->db_didname, NULL, &rootinfo_key,
                                     &rootinfo_data, 0)) {
    case DB_LOCK_DEADLOCK:
        if ((rc = txn_abort(tid)) != 0) {
            syslog(LOG_ERR, "cnid_add: txn_abort: %s", db_strerror(rc));
            goto cleanup_err;
        }
        goto retry_get;
    case 0:
        memcpy(&hint, rootinfo_data.data, sizeof(hint));
#ifdef DEBUG
        syslog(LOG_INFO, "cnid_add: Found rootinfo for did %u, name %s as %u", ntohl(did), name, ntohl(hint));
#endif
        break;
    case DB_NOTFOUND:
        hint = htonl(CNID_START);
#ifdef DEBUG
        syslog(LOG_INFO, "cnid_add: Using CNID_START for did %u, name %s",
               ntohl(did), name);
#endif
        break;
    default:
        syslog(LOG_ERR, "cnid_add: Unable to lookup rootinfo: %s",
               db_strerror(rc));
        goto cleanup_err;
    }


	if (0) {
retry:
    	t.tv_usec = rand() % 1000000;
#ifdef DEBUG
    	syslog(LOG_INFO, "cnid_add: Hitting MAX_ABORTS, sleeping");
#endif
    	(void)select(0, NULL, NULL, NULL, &t);
	}
    if ((rc = txn_begin(db->dbenv, NULL, &tid, 0)) != 0) {
        syslog(LOG_ERR, "cnid_add: Failed to begin transaction: %s",
               db_strerror(rc));
        goto cleanup_err;
    }

    /* Search for a new id.  We keep the first id around to check for
     * wrap-around.  NOTE: I do it this way so that we can go back and
     * fill in holes. */
    save = id = ntohl(hint);
    while ((rc = add_cnid(db, tid, &key, &data)) != 0) {
        /* Don't use any special CNIDs. */
        if (++id < CNID_START) {
            id = CNID_START;
        }
        if (rc == DB_LOCK_DEADLOCK) {
            if ((rc = txn_abort(tid)) != 0) {
                syslog(LOG_ERR, "cnid_add: txn_abort: %s", db_strerror(rc));
                goto cleanup_err;
            }
            goto retry;
        }

        if ((rc != DB_KEYEXIST) || (save == id)) {
            syslog(LOG_ERR, "cnid_add: Unable to add CNID %u: %s",
                   ntohl(hint), db_strerror(rc));
            goto cleanup_abort;
        }
        hint = htonl(id);
    }

    rootinfo_data.data = &hint;
    rootinfo_data.size = sizeof(hint);

    switch (rc = db->db_didname->put(db->db_didname, tid, &rootinfo_key, &rootinfo_data, 0)) {
    case DB_LOCK_DEADLOCK:
        if ((rc = txn_abort(tid)) != 0) {
            syslog(LOG_ERR, "cnid_add: txn_abort: %s", db_strerror(rc));
            goto cleanup_err;
        }
        goto retry;
    case 0:
        break;
    default:
        syslog(LOG_ERR, "cnid_add: Unable to update rootinfo: %s",
               db_strerror(rc));
        goto cleanup_abort;
    }

cleanup_commit:
    /* The transaction finished, commit it. */
    if ((rc = txn_commit(tid, 0)) != 0) {
        syslog(LOG_ERR, "cnid_add: Unable to commit transaction: %s",
               db_strerror(rc));
        goto cleanup_err;
    }

#ifdef DEBUG
    syslog(LOG_INFO, "cnid_add: Returned CNID for did %u, name %s as %u",
           ntohl(did), name, ntohl(hint));
#endif
    return hint;

cleanup_abort:
    txn_abort(tid);

cleanup_err:
    return 0;
}
#endif /* CNID_DB */

