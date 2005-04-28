/*
 * $Id: dbd_delete.c,v 1.2 2005-04-28 20:49:48 bfernhomberg Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <errno.h>
#include <netatalk/endian.h>
#include <atalk/logger.h>
#include <atalk/cnid_dbd_private.h>

#include "dbif.h"
#include "dbd.h"
#include "pack.h"

int dbd_delete(struct cnid_dbd_rqst *rqst, struct cnid_dbd_rply *rply)
{
    DBT key;
    int rc;

    memset(&key, 0, sizeof(key));

    rply->namelen = 0;

    key.data = (void *) &rqst->cnid;
    key.size = sizeof(rqst->cnid);

    if ((rc = dbif_del(DBIF_IDX_CNID, &key, 0)) < 0) {
        LOG(log_error, logtype_cnid, "dbd_delete: Unable to delete entry for CNID %u", ntohl(rqst->cnid));
        rply->result = CNID_DBD_RES_ERR_DB;
        return -1;
    }

    if (rc) {
#ifdef DEBUG
        LOG(log_info, logtype_cnid, "cnid_delete: CNID %u deleted", ntohl(rqst->cnid));
#endif
        rply->result = CNID_DBD_RES_OK;
    } else {
#ifdef DEBUG
        LOG(log_info, logtype_cnid, "cnid_delete: CNID %u not in database", ntohl(rqst->cnid));
#endif
        rply->result = CNID_DBD_RES_NOTFOUND;
    }
    return 1;
}
