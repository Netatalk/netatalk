/*
 * $Id: cmd_dbd_lookup.c,v 1.3 2009-10-14 01:38:28 didg Exp $
 *
 * Copyright (C) Frank Lahm 2009
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */


#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <errno.h>
#include <netatalk/endian.h>
#include <atalk/logger.h>
#include <atalk/cnid_dbd_private.h>

#include "pack.h"
#include "dbif.h"
#include "dbd.h"
#include "cmd_dbd.h"

/* 
   ATTENTION:
   whatever you change here, also change cmd_dbd_lookup.c !
   cmd_dbd_lookup has an read-only mode, but besides that it's the same.
 */


/*
 *  This returns the CNID corresponding to a particular file and logs any inconsitencies.
 *  If roflags == 1 we only scan, if roflag == 0, we modify i.e. call dbd_update
 *  FIXME: realign this wih dbd_lookup.
 */

int cmd_dbd_lookup(DBD *dbd, struct cnid_dbd_rqst *rqst, struct cnid_dbd_rply *rply, int roflag)
{
    unsigned char *buf;
    DBT key, devdata, diddata;
    uint64_t dev = 0;
    int devino = 1, didname = 1; 
    int rc;
    cnid_t id_devino, id_didname;
    u_int32_t type_devino  = (unsigned)-1;
    u_int32_t type_didname = (unsigned)-1;
    int update = 0;
    
    memset(&key, 0, sizeof(key));
    memset(&diddata, 0, sizeof(diddata));
    memset(&devdata, 0, sizeof(devdata));

    rply->namelen = 0; 
    rply->cnid = 0;
    
    buf = pack_cnid_data(rqst); 

    /* Look for a CNID.  We have two options: dev/ino or did/name.  If we
       only get a match in one of them, that means a file has moved. */
    key.data = buf + CNID_DEVINO_OFS;
    key.size = CNID_DEVINO_LEN;

    if ((rc = dbif_get(dbd, DBIF_IDX_DEVINO, &key, &devdata, 0))  < 0) {
        dbd_log( LOGSTD, "dbd_search: Unable to get CNID %u, name %s",
                ntohl(rqst->did), rqst->name);
        rply->result = CNID_DBD_RES_ERR_DB;
        return -1;
    }
    if (rc == 0) {
        devino = 0;
    }
    else {
        memcpy(&id_devino, devdata.data, sizeof(rply->cnid));
        memcpy(&type_devino, (char *)devdata.data +CNID_TYPE_OFS, sizeof(type_devino));
        type_devino = ntohl(type_devino);
    }

    key.data = buf + CNID_DID_OFS;
    key.size = CNID_DID_LEN + rqst->namelen + 1;

    if ((rc = dbif_get(dbd, DBIF_IDX_DIDNAME, &key, &diddata, 0))  < 0) {
        dbd_log( LOGSTD, "dbd_search: Unable to get CNID %u, name %s",
                ntohl(rqst->did), rqst->name);
        rply->result = CNID_DBD_RES_ERR_DB;
        return -1;
    }
    if (rc == 0) {
        didname = 0;
    }
    else {
        memcpy(&id_didname, diddata.data, sizeof(rply->cnid));
        memcpy(&type_didname, (char *)diddata.data +CNID_TYPE_OFS, sizeof(type_didname));
        type_didname = ntohl(type_didname);
    }
    
    if (!devino && !didname) {  
        /* not found */
        dbd_log( LOGDEBUG, "name: '%s/%s', did: %u, dev/ino: 0x%llx/0x%llx is not in the CNID database", 
                 cwdbuf, rqst->name, ntohl(rqst->did), (unsigned long long)rqst->dev, (unsigned long long)rqst->ino);
        rply->result = CNID_DBD_RES_NOTFOUND;
        return 1;
    }

    if (devino && didname && id_devino == id_didname && type_devino == rqst->type) {
        /* the same */
        rply->cnid = id_didname;
        rply->result = CNID_DBD_RES_OK;
        return 1;
    }

    /* 
       Order matters for the next 2 ifs because both found a CNID but they didn't match.
       So in order to pick the CNID from "didname" it must come after devino.
       See test cases laid out in dbd_lookup.c.
    */
    if (devino) {
        dbd_log( LOGSTD, "CNID resolve problem: server side rename oder reused inode for '%s/%s'", cwdbuf, rqst->name);
        rqst->cnid = id_devino;
        if (type_devino != rqst->type) {
            /* same dev/inode but not same type: it's an inode reused, delete the record */
            if (! roflag)
                if (dbd_delete(dbd, rqst, rply, DBIF_CNID) < 0)
                    return -1;
        }
        else {
            update = 1;
        }
    }
    if (didname) {
        dbd_log( LOGSTD, "CNID resolve problem: changed dev/ino for '%s/%s'", cwdbuf, rqst->name);
        rqst->cnid = id_didname;
        /* we have a did/name.
           If it's the same dev or not the same type, e.g. a remove followed by a new file
           with the same name */
        if (!memcmp(&dev, (char *)diddata.data + CNID_DEV_OFS, CNID_DEV_LEN) ||
            type_didname != rqst->type) {
            if (! roflag)
                if (dbd_delete(dbd, rqst, rply, DBIF_CNID) < 0)
                    return -1;
        }
        else {
            update = 1;
        }
    }

    if (!update || roflag) {
        rply->result = CNID_DBD_RES_NOTFOUND;
        return 1;
    }
    /* Fix up the database. assume it was a file move and rename */
    rc = dbd_update(dbd, rqst, rply);
    if (rc >0) {
        rply->cnid = rqst->cnid;
    }

    dbd_log( LOGSTD, "CNID database needed update: dev/ino: 0x%llx/0x%llx, did: %u, name: '%s/%s' --> CNID: %u", 
             (unsigned long long)rqst->dev, (unsigned long long)rqst->ino,
             ntohl(rqst->did), cwdbuf, rqst->name, ntohl(rply->cnid));

    return rc;
}

/* 
   This is taken from dbd_add.c, but this func really is only "add", dbd_add calls dbd_lookup
   before adding.
   FIXME: realign with dbd_add using e.g. using a new arg like "int lookup".
*/
int cmd_dbd_add(DBD *dbd, struct cnid_dbd_rqst *rqst, struct cnid_dbd_rply *rply)
{
    if (get_cnid(dbd, rply) < 0) {
        if (rply->result == CNID_DBD_RES_ERR_MAX) {
            dbd_log( LOGSTD, "FATAL: CNID database has reached its limit.");
            /* This will cause an abort/rollback if transactions are used */
            return 0;
        } else {
            dbd_log( LOGSTD, "Failed to compute CNID for %s, error reading/updating Rootkey", rqst->name);
            return -1;
        }
    }
    
    if (add_cnid(dbd, rqst, rply) < 0) {
        if (rply->result == CNID_DBD_RES_ERR_DUPLCNID) {
            dbd_log( LOGSTD, "Cannot add CNID %u. Corrupt/invalid Rootkey?.", ntohl(rply->cnid));
            /* abort/rollback, see above */
            return 0;
        } else {
            dbd_log( LOGSTD, "Failed to add CNID for %s to database", rqst->name);
            return -1;
        }
    }
    dbd_log( LOGDEBUG, "Add to CNID database: did: %u, cnid: %u, name: '%s', dev/ino: 0x%llx/0x%llx",
             ntohl(rqst->did), ntohl(rply->cnid), rqst->name,
             (unsigned long long)rqst->dev, (unsigned long long)rqst->ino);

    rply->result = CNID_DBD_RES_OK;
    return 1;
}
