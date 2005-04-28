/*
 * $Id: cnid_tdb_get.c,v 1.2 2005-04-28 20:50:02 bfernhomberg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef CNID_BACKEND_TDB

#include "cnid_tdb.h"

/* Return CNID for a given did/name. */
cnid_t cnid_tdb_get(struct _cnid_db *cdb, const cnid_t did, char *name, const int len)
{
    char start[TDB_DID_LEN + MAXPATHLEN + 1], *buf;
    struct _cnid_tdb_private *db;
    TDB_DATA key, data;
    cnid_t id;

    if (!cdb || !(db = cdb->_private) || (len > MAXPATHLEN)) {
        return 0;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    buf = start;
    memcpy(buf, &did, sizeof(did));
    buf += sizeof(did);
    memcpy(buf, name, len);
    *(buf + len) = '\0'; /* Make it a C-string. */
    key.dptr = start;
    key.dsize = TDB_DID_LEN + len + 1;
    data = tdb_fetch(db->tdb_didname, key);
    if (!data.dptr)
        return 0;

    memcpy(&id, data.dptr, sizeof(id));
    free(data.dptr);
    return id;
}

#endif
