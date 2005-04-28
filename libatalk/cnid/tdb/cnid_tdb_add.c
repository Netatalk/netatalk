/*
 * $Id: cnid_tdb_add.c,v 1.2 2005-04-28 20:50:02 bfernhomberg Exp $
 *
 * Copyright (c) 1999. Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef CNID_BACKEND_TDB
#include "cnid_tdb.h"
#include <atalk/util.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <atalk/logger.h>

/* add an entry to the CNID databases. we do this as a transaction
 * to prevent messiness. */

static int add_cnid (struct _cnid_tdb_private *db, TDB_DATA *key, TDB_DATA *data) {
    TDB_DATA altkey, altdata;

    memset(&altkey, 0, sizeof(altkey));
    memset(&altdata, 0, sizeof(altdata));


    /* main database */
    if (tdb_store(db->tdb_cnid, *key, *data, TDB_REPLACE)) {
        goto abort;
    }

    /* dev/ino database */
    altkey.dptr = data->dptr;
    altkey.dsize = TDB_DEVINO_LEN;
    altdata.dptr = key->dptr;
    altdata.dsize = key->dsize;
    if (tdb_store(db->tdb_devino, altkey, altdata, TDB_REPLACE)) {
        goto abort;
    }

    /* did/name database */
    altkey.dptr = (char *) data->dptr + TDB_DEVINO_LEN;
    altkey.dsize = data->dsize - TDB_DEVINO_LEN;
    if (tdb_store(db->tdb_didname, altkey, altdata, TDB_REPLACE)) {
        goto abort;
    }
    return 0;

abort:
    return -1;
}

/* ----------------------- */
static cnid_t get_cnid(struct _cnid_tdb_private *db)
{
    TDB_DATA rootinfo_key, data;
    cnid_t hint,id;
    
    memset(&rootinfo_key, 0, sizeof(rootinfo_key));
    memset(&data, 0, sizeof(data));
    rootinfo_key.dptr = ROOTINFO_KEY;
    rootinfo_key.dsize = ROOTINFO_KEYLEN;
    
    tdb_chainlock(db->tdb_didname, rootinfo_key);  
    data = tdb_fetch(db->tdb_didname, rootinfo_key);
    if (data.dptr)
    {
        memcpy(&hint, data.dptr, sizeof(cnid_t));
        free(data.dptr);
        id = ntohl(hint);
        /* If we've hit the max CNID allowed, we return a fatal error.  CNID
         * needs to be recycled before proceding. */
        if (++id == CNID_INVALID) {
            LOG(log_error, logtype_default, "cnid_add: FATAL: CNID database has reached its limit.");
            errno = CNID_ERR_MAX;
            goto cleanup;
        }
        hint = htonl(id);
    }
    else {
        hint = htonl(TDB_START);
    }
    
    memset(&data, 0, sizeof(data));
    data.dptr = (char *)&hint;
    data.dsize = sizeof(hint);
    if (tdb_store(db->tdb_didname, rootinfo_key, data, TDB_REPLACE)) {
        goto cleanup;
    }

    tdb_chainunlock(db->tdb_didname, rootinfo_key );  
    return hint;
cleanup:
    tdb_chainunlock(db->tdb_didname, rootinfo_key);  
    return CNID_INVALID;
}


/* ------------------------ */
cnid_t cnid_tdb_add(struct _cnid_db *cdb, const struct stat *st,
                     const cnid_t did, char *name, const int len, cnid_t hint)
{
    const struct stat *lstp;
    cnid_t id;
    struct _cnid_tdb_private *priv;
    TDB_DATA key, data; 
    int rc;      
    
    if (!cdb || !(priv = cdb->_private) || !st || !name) {
        errno = CNID_ERR_PARAM;
        return CNID_INVALID;
    }
    /* Do a lookup. */
    id = cnid_tdb_lookup(cdb, st, did, name, len);
    /* ... Return id if it is valid, or if Rootinfo is read-only. */
    if (id || (priv->flags & TDBFLAG_DB_RO)) {
        return id;
    }

#if 0
    struct stat lst;
    lstp = lstat(name, &lst) < 0 ? st : &lst;
#endif
    lstp = st;

    /* Initialize our DBT data structures. */
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    key.dptr = (char *)&hint;
    key.dsize = sizeof(cnid_t);
    if ((data.dptr = make_tdb_data(lstp, did, name, len)) == NULL) {
        LOG(log_error, logtype_default, "tdb_add: Path name is too long");
        errno = CNID_ERR_PATH;
        return CNID_INVALID;
    }
    data.dsize = TDB_HEADER_LEN + len + 1;
    hint = get_cnid(priv);
    if (hint == 0) {
        errno = CNID_ERR_DB;
        return CNID_INVALID;
    }
    
    /* Now we need to add the CNID data to the databases. */
    rc = add_cnid(priv, &key, &data);
    if (rc) {
        LOG(log_error, logtype_default, "tdb_add: Failed to add CNID for %s to database using hint %u", name, ntohl(hint));
        errno = CNID_ERR_DB;
        return CNID_INVALID;
    }

    return hint;
}

#endif
