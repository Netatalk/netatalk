/*
 * $Id: cnid_resolve.c,v 1.13 2002-08-30 03:12:52 jmarcus Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_DB
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <atalk/logger.h>
#include <errno.h>

#include <db.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/cnid.h>

#include "cnid_private.h"

/* Return the did/name pair corresponding to a CNID. */
char *cnid_resolve(void *CNID, cnid_t *id, void *buffer, u_int32_t len) {
    CNID_private *db;
    DBT key, data;
    int rc;

    if (!(db = CNID) || !id || !(*id)) {
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
#ifndef CNID_DB_CDB
        if (rc == DB_LOCK_DEADLOCK) {
            continue;
        }
#endif /* CNID_DB_CDB */

        if (rc != DB_NOTFOUND) {
            LOG(log_error, logtype_default, "cnid_resolve: Unable to get did/name: %s",
                db_strerror(rc));
        }

        *id = 0;
        return NULL;
    }

    memcpy(id, (char *)data.data + CNID_DEVINO_LEN, sizeof(cnid_t));
#ifdef DEBUG
    LOG(log_info, logtype_default, "cnid_resolve: Returning id = %u, did/name = %s",
        ntohl(*id), (char *)data.data + CNID_HEADER_LEN);
#endif
    return (char *)data.data + CNID_HEADER_LEN;
}
#endif /* CNID_DB */
