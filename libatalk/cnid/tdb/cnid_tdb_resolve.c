/*
 * $Id: cnid_tdb_resolve.c,v 1.4 2009-11-22 14:14:05 franklahm Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef CNID_BACKEND_TDB

#include "cnid_tdb.h"

/* Return the did/name pair corresponding to a CNID. */
char *cnid_tdb_resolve(struct _cnid_db *cdb, cnid_t * id, void *buffer, size_t len)
{
    struct _cnid_tdb_private *db;
    TDB_DATA key, data;      

    if (!cdb || !(db = cdb->_private) || !id || !(*id)) {
        return NULL;
    }
    key.dptr  = (char *)id;
    key.dsize = sizeof(cnid_t);
    data = tdb_fetch(db->tdb_cnid, key);
    if (data.dptr) 
    {
        if (data.dsize < len && data.dsize > sizeof(cnid_t)) {
            memcpy(id, (char *)data.dptr + +CNID_DID_OFS, sizeof(cnid_t));
            strcpy(buffer, (char *)data.dptr + CNID_NAME_OFS);
            free(data.dptr);
            return buffer;
        }
        free(data.dptr);
    }
    return NULL;
}

#endif
