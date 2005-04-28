/*
 * $Id: cnid_hash_resolve.c,v 1.2 2005-04-28 20:50:01 bfernhomberg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_BACKEND_HASH

#include "cnid_hash.h"

/* Return the did/name pair corresponding to a CNID. */
char *cnid_hash_resolve(struct _cnid_db *cdb, cnid_t * id, void *buffer, u_int32_t len)
{
    struct _cnid_hash_private *db;
    TDB_DATA key, data;      

    if (!cdb || !(db = cdb->_private) || !id || !(*id)) {
        return NULL;
    }
    key.dptr  = (char *)id;
    key.dsize = sizeof(cnid_t);
    data = tdb_fetch(db->tdb, key);
    if (data.dptr) 
    {
        if (data.dsize < len && data.dsize > sizeof(cnid_t)) {
            memcpy(id, data.dptr, sizeof(cnid_t));
            memcpy(buffer, data.dptr +sizeof(cnid_t), data.dsize -sizeof(cnid_t));
            free(data.dptr);
            return buffer;
        }
        free(data.dptr);
    }
    return NULL;
}

#endif /* CNID_BACKEND_HASH */
