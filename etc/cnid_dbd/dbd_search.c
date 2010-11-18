/*
 * Copyright (C) Frank Lahm 2010
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

int dbd_search(DBD *dbd, struct cnid_dbd_rqst *rqst, struct cnid_dbd_rply *rply)
{
    DBT key;
    int rc;
    static char resbuf[DBD_MAX_SRCH_RPLY_PAYLOAD];
    memset(&key, 0, sizeof(key));
    rply->namelen = 0;

    key.data = rqst->name;
    key.size = rqst->namelen;

    if ((rc = dbif_del(dbd, DBIF_IDX_DEVINO, &key, 0)) < 0) {
        LOG(log_error, logtype_cnid, "dbd_delete: Unable to delete entry for dev/ino: 0x%llx/0x%llx",
            (unsigned long long)rqst->dev, (unsigned long long)rqst->ino);
        rply->result = CNID_DBD_RES_ERR_DB;
        return -1;
    }
    if (rc) {
        LOG(log_debug, logtype_cnid, "cnid_delete: deleted dev/ino: 0x%llx/0x%llx",
            (unsigned long long)rqst->dev, (unsigned long long)rqst->ino);
        rply->result = CNID_DBD_RES_OK;
    } else {
        LOG(log_debug, logtype_cnid, "cnid_delete: dev/ino: 0x%llx/0x%llx not in database", 
            (unsigned long long)rqst->dev, (unsigned long long)rqst->ino);
        rply->result = CNID_DBD_RES_NOTFOUND;
    }

    return 1;
}
