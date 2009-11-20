/*
 * $Id: cnid_tdb_lookup.c,v 1.4 2009-11-20 17:37:14 didg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef CNID_BACKEND_TDB

#include "cnid_tdb.h"
#include <atalk/logger.h>

cnid_t cnid_tdb_lookup(struct _cnid_db *cdb, const struct stat *st, const cnid_t did, char *name, const size_t len)
{
    char *buf;
    struct _cnid_tdb_private *db;
    TDB_DATA key, devdata, diddata;
    int devino = 1, didname = 1;
    cnid_t id = 0;

    if (!cdb || !(db = cdb->_private) || !st || !name) {
        return 0;
    }

    if ((buf = make_tdb_data(cdb->flags, st, did, name, len)) == NULL) {
        LOG(log_error, logtype_default, "tdb_lookup: Pathname is too long");
        return 0;
    }

    memset(&key, 0, sizeof(key));
    memset(&devdata, 0, sizeof(devdata));
    memset(&diddata, 0, sizeof(diddata));

    /* Look for a CNID.  We have two options: dev/ino or did/name.  If we
    * only get a match in one of them, that means a file has moved. */
    key.dptr = buf +CNID_DEVINO_OFS;
    key.dsize  = CNID_DEVINO_LEN;
    devdata = tdb_fetch(db->tdb_devino, key);
    if (!devdata.dptr) {
         devino = 0;
    }
    /* did/name now */
    key.dptr = buf + +CNID_DID_OFS;
    key.dsize = CNID_DID_LEN + len + 1;
    diddata = tdb_fetch(db->tdb_didname, key);
    if (!diddata.dptr) {
        didname = 0;
    }
    /* Set id.  Honor did/name over dev/ino as dev/ino isn't necessarily
     * 1-1. */
    if (didname) {
        memcpy(&id, diddata.dptr, sizeof(id));
    }
    else if (devino) {
        memcpy(&id, devdata.dptr, sizeof(id));
    }
    free(devdata.dptr);
    free(diddata.dptr);
    /* Either entries are in both databases or neither of them. */
    if ((devino && didname) || !(devino || didname)) {
        return id;
    }

    /* Fix up the database. */
    cnid_tdb_update(cdb, id, st, did, name, len);
    return id;
}

#endif
