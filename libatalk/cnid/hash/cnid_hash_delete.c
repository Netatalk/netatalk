/*
 * $Id: cnid_hash_delete.c,v 1.2 2005-04-28 20:50:01 bfernhomberg Exp $
 *
 * Copyright (c) 1999. Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 * cnid_delete: delete a CNID from the database 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_BACKEND_HASH

#include "cnid_hash.h"

int cnid_hash_delete(struct _cnid_db *cdb, const cnid_t id)
{
    struct _cnid_hash_private *db;
    TDB_DATA key;      

    if (!cdb || !(db = cdb->_private) || !id) {
        return -1;
    }
    key.dptr  = (char *)&id;
    key.dsize = sizeof(cnid_t);
    tdb_delete(db->tdb, key); 

    return 0;
}

#endif /* CNID_BACKEND_HASH */
