/*
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef CNID_BACKEND_TDB

#include "cnid_tdb.h"

void cnid_tdb_close(struct _cnid_db *cdb)
{
    struct _cnid_tdb_private *db;

    db = (struct _cnid_tdb_private *)cdb->cnid_db_private;
    tdb_close(db->tdb_cnid);
    free(cdb->cnid_db_private);
    free(cdb);
}

#endif
