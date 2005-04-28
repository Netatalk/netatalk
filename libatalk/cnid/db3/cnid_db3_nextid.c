/*
 * $Id: cnid_db3_nextid.c,v 1.2 2005-04-28 20:49:59 bfernhomberg Exp $
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_BACKEND_DB3

#ifdef unused

#ifdef HAVE_DB4_DB_H
#include <db4/db.h>
#else
#include <db.h>
#endif

#include <atalk/adouble.h>
#include "cnid_db3.h"

#include <atalk/logger.h>

#include "cnid_db3_private.h"

/* return the next id. we use the fact that ad files are memory
 * mapped. */
cnid_t cnid_db3_nextid(struct _cnid_db *cdb)
{
    CNID_private *db;
    cnid_t id;

    if (!cdb || !(db = cdb->_private))
        return 0;

    memcpy(&id, ad_entry(&db->rootinfo, ADEID_DID), sizeof(id));
    return id;
}
#endif

#endif /* CNID_BACKEND_DB3 */
