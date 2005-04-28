/*
 * $Id: cnid_hash_close.c,v 1.2 2005-04-28 20:50:01 bfernhomberg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_BACKEND_HASH

#include "cnid_hash.h"

void cnid_hash_close(struct _cnid_db *cdb)
{
    struct _cnid_hash_private *db;

    free(cdb->volpath);
    db = (struct _cnid_hash_private *)cdb->_private;
    tdb_close(db->tdb);    
    free(cdb->_private);
    free(cdb);
}

#endif /* CNID_BACKEND_HASH */
