/*
 * $Id: cnid_db3_get.c,v 1.4 2009-10-29 13:17:29 didg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_BACKEND_DB3

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <atalk/logger.h>
#include <errno.h>

#ifdef HAVE_DB4_DB_H
#include <db4/db.h>
#else
#include <db.h>
#endif
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include "cnid_db3.h"

#include "cnid_db3_private.h"

/* Return CNID for a given did/name. */
cnid_t cnid_db3_get(struct _cnid_db *cdb, const cnid_t did, char *name, const size_t len)
{
    char start[CNID_DID_LEN + MAXPATHLEN + 1], *buf;
    CNID_private *db;
    DBT key, data;
    cnid_t id;
    int rc;

    if (!cdb || !(db = cdb->_private) || (len > MAXPATHLEN)) {
        // FIXME: shall we report some error !
        return 0;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    buf = start;
    memcpy(buf, &did, sizeof(did));
    buf += sizeof(did);
    memcpy(buf, name, len);
    *(buf + len) = '\0'; /* Make it a C-string. */
    key.data = start;
    key.size = CNID_DID_LEN + len + 1;

    while ((rc = db->db_didname->get(db->db_didname, NULL, &key, &data, 0))) {
        if (rc == DB_LOCK_DEADLOCK) {
            continue;
        }

        if (rc != DB_NOTFOUND) {
            LOG(log_error, logtype_default, "cnid_get: Unable to get CNID %u, name %s: %s",
                ntohl(did), name, db_strerror(rc));
        }

        return 0;
    }

    memcpy(&id, data.data, sizeof(id));
#ifdef DEBUG
    LOG(log_debug, logtype_default, "cnid_get: Returning CNID for %u, name %s as %u",
        ntohl(did), name, ntohl(id));
#endif
    return id;
}

#endif /* CNID_BACKEND_DB3 */
