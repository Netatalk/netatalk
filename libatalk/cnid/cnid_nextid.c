/*
 * $Id: cnid_nextid.c,v 1.7 2001-11-27 23:38:18 jmarcus Exp $
 */
#ifdef unused

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_DB
#include <db.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>

#include <syslog.h>

#include "cnid_private.h"

/* return the next id. we use the fact that ad files are memory
 * mapped. */
cnid_t cnid_nextid(void *CNID)
{
    CNID_private *db;
    cnid_t id;

    if (!(db = CNID))
        return 0;

    memcpy(&id, ad_entry(&db->rootinfo, ADEID_DID), sizeof(id));
    return id;
}
#endif /* CNID_DB */
#endif

