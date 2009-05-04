/*
 * $Id: dbd_update.c,v 1.4 2009-05-04 09:09:43 franklahm Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <errno.h>
#include <atalk/logger.h>
#include <netatalk/endian.h>
#include <atalk/cnid_dbd_private.h>


#include "pack.h"
#include "dbif.h"
#include "dbd.h"


/* cnid_update: takes the given cnid and updates the metadata. */

/* FIXME: This calls pack_cnid_data(rqst) three times without modifying rqst */
/* FIXME: (Only tested with DB 4.1.25):

      dbif_pget on the secondary index followed by dbif_del with the CNID on the
      main cnid db could be replaced by a single dbif_del on the secondary index. That 
      deletes the secondary, the corresponding entry from the main cnid db as well as the 
      other secondary index.
*/   
   
int dbd_update(struct cnid_dbd_rqst *rqst, struct cnid_dbd_rply *rply)
{
    DBT key,pkey, data;
    int rc;
    unsigned char *buf;                            
    int notfound = 0;
    char getbuf[CNID_HEADER_LEN + MAXPATHLEN +1];
    cnid_t tmpcnid;

    memset(&key, 0, sizeof(key));
    memset(&pkey, 0, sizeof(pkey));
    memset(&data, 0, sizeof(data));

    rply->namelen = 0;

    buf = pack_cnid_data(rqst);
    key.data = buf +CNID_DEVINO_OFS;
    key.size = CNID_DEVINO_LEN;

    data.data = getbuf;
    data.size = CNID_HEADER_LEN + MAXPATHLEN + 1;
    if ((rc = dbif_pget(DBIF_IDX_DEVINO, &key, &pkey, &data, 0)) < 0 ) {
        goto err_db;
    }
    else if  (rc > 0) {
        memcpy(&tmpcnid, pkey.data, sizeof(cnid_t));
        LOG(log_debug, logtype_cnid, "dbd_update: Deleting %u corresponding to dev/ino 0x%llx/0x%llx from cnid2.db",
            ntohl(tmpcnid), ntoh64((unsigned long long int)rqst->dev), ntoh64((unsigned long long int)rqst->ino));

        if ((rc = dbif_del(DBIF_IDX_CNID, &pkey, 0)) < 0 ) {
            goto err_db;
        }
        else if (!rc) {
            LOG(log_error, logtype_cnid, "dbd_update: delete DEVINO %u %s", ntohl(rqst->cnid), db_strerror(errno));
        }
    }
    if (!rc) {
       notfound = 1;
    }

    buf = pack_cnid_data(rqst);
    key.data = buf + CNID_DID_OFS;
    key.size = CNID_DID_LEN + rqst->namelen +1;
    memset(&pkey, 0, sizeof(pkey));

    if ((rc = dbif_pget(DBIF_IDX_DIDNAME, &key, &pkey, &data, 0)) < 0) {
        goto err_db;
    }
    else if  (rc > 0) {

	memcpy(&tmpcnid, pkey.data, sizeof(cnid_t));

	LOG(log_debug, logtype_cnid, "dbd_update: Deleting %u corresponding to did %u name %s from cnid2.db",
	    ntohl(tmpcnid), ntohl(rqst->did), rqst->name);

        if ((rc = dbif_del(DBIF_IDX_CNID, &pkey, 0)) < 0) {
            goto err_db;
        }
        else if (!rc) {
    		LOG(log_error, logtype_cnid, "dbd_update: delete DIDNAME %u %s", ntohl(rqst->cnid), db_strerror(errno));
        }
    }
    if (!rc) {
       notfound |= 2;
    }

    memset(&key, 0, sizeof(key));
    key.data = (cnid_t *) &rqst->cnid;
    key.size = sizeof(rqst->cnid);

    memset(&data, 0, sizeof(data));
    /* Make a new entry. */
    data.data = pack_cnid_data(rqst);
    memcpy(data.data, &rqst->cnid, sizeof(rqst->cnid));
    data.size = CNID_HEADER_LEN + rqst->namelen + 1;

    if (dbif_put(DBIF_IDX_CNID, &key, &data, 0) < 0)
        goto err_db;

    LOG(log_info, logtype_cnid, "dbd_update: Updated cnid2.db with dev/ino 0x%llx/0x%llx did %u name %s cnid %u",
        ntoh64((unsigned long long int)rqst->dev), ntoh64((unsigned long long int)rqst->ino),
        ntohl(rqst->did), rqst->name, ntohl(rqst->cnid));

    rply->result = CNID_DBD_RES_OK;
    return 1;

err_db:
    LOG(log_error, logtype_cnid, "dbd_update: Unable to update CNID %u dev/ino 0x%llx/0x%llx, DID %ul: %s",
        ntohl(rqst->cnid),
        ntoh64((unsigned long long int)rqst->dev), ntoh64((unsigned long long int)rqst->ino),
        rqst->did, rqst->name);

    rply->result = CNID_DBD_RES_ERR_DB;
    return -1;
}
