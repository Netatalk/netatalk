/*
 * $Id: cnid_db3_resolve.c,v 1.4 2009-10-29 13:17:29 didg Exp $
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

/* Return the did/name pair corresponding to a CNID. */
char *cnid_db3_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer, size_t len) {
    CNID_private *db;
    DBT key, data;
    int rc;

    if (!cdb || !(db = cdb->_private) || !id || !(*id)) {
        return NULL;
    }

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    data.data = buffer;
    data.ulen = len;
    data.flags = DB_DBT_USERMEM;

    key.data = id;
    key.size = sizeof(cnid_t);
    while ((rc = db->db_cnid->get(db->db_cnid, NULL, &key, &data, 0))) {
        if (rc == DB_LOCK_DEADLOCK) {
            continue;
        }

        if (rc != DB_NOTFOUND) {
            LOG(log_error, logtype_default, "cnid_resolve: Unable to get did/name: %s",
                db_strerror(rc));
        }

        *id = 0;
        return NULL;
    }

    memcpy(id, (char *)data.data + CNID_DEVINO_LEN, sizeof(cnid_t));
#ifdef DEBUG
    LOG(log_debug, logtype_default, "cnid_resolve: Returning id = %u, did/name = %s",
        ntohl(*id), (char *)data.data + CNID_HEADER_LEN);
#endif
    return (char *)data.data + CNID_HEADER_LEN;
}

#endif /* CNID_BACKEND_DB3 */
