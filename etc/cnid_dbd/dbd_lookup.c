/*
 * $Id: dbd_lookup.c,v 1.9 2009-07-12 09:21:34 franklahm Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

/* 
   ATTENTION:
   whatever you change here, also change cmd_dbd_lookup.c !
   cmd_dbd_lookup has an read-only mode but besides that it's the same.
*/

/* 
The lines...

Id  Did T   Dev Inode   Name
============================
a   b   c   d   e       name1
-->
f   g   h   i   h       name2

...are the expected results of certain operations.


1) UNIX rename (mv)
-------------------
Name is changed but inode stays the same. We should try to keep the CNID.

15  2   f   1   1       file
-->
15  2   f   1   1       movedfile

Result in dbd_lookup (-: not found, +: found):
- devino
+ didname

Possible solution:
Update.


2) UNIX copy (cp)
-----------------
Changes inode and name. Result is just a new file which will get a fresh CNID.
Unfortunately the old one gets orphaned.

15  2   f   1   1       file
-->
16  2   f   1   2       copyfile

Result in dbd_lookup:
- devino
- didname

Possible fixup solution:
Not possible. Only dbd -re can delete the orphaned CNID 15


3) Restore from backup
----------------------
15  2   f   1   1       file
-->
15  2   f   1   2       file

Result in dbd_lookup:
- devino
+ didname

Possible fixup solution:
Update.


4) UNIX emacs
-------------
This one is tough:
emacs uses a backup file (file~). When saving because of inode reusage of the fs,
both files exchange inodes. This should just be treated like 1).

15  2   f   1   1       file
16  2   f   1   2       file~
-->
15  2   f   1   2       file
16  2   f   1   1       file~

Result in dbd_lookup:
+ devino
+ didname

Possible fixup solution:
Update CNID entry found via "didname"(!).
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

/*
 *  This returns the CNID corresponding to a particular file.  It will also fix
 *  up the database if there's a problem.
 */

int dbd_lookup(DBD *dbd, struct cnid_dbd_rqst *rqst, struct cnid_dbd_rply *rply)
{
    unsigned char *buf;
    DBT key, devdata, diddata;
    char dev[CNID_DEV_LEN];
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
    memcpy(dev, buf + CNID_DEV_OFS, CNID_DEV_LEN);

    /* Look for a CNID.  We have two options: dev/ino or did/name.  If we
       only get a match in one of them, that means a file has moved. */
    key.data = buf + CNID_DEVINO_OFS;
    key.size = CNID_DEVINO_LEN;

    if ((rc = dbif_get(dbd, DBIF_IDX_DEVINO, &key, &devdata, 0))  < 0) {
        LOG(log_error, logtype_cnid, "dbd_lookup: Unable to get CNID %u, name %s",
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
        LOG(log_error, logtype_cnid, "dbd_lookup: Unable to get CNID %u, name %s",
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

        LOG(log_debug, logtype_cnid, "cnid_lookup: dev/ino 0x%llx/0x%llx did %u name %s neither in devino nor didname", 
            (unsigned long long)rqst->dev, (unsigned long long)rqst->ino, ntohl(rqst->did), rqst->name);

        rply->result = CNID_DBD_RES_NOTFOUND;
        return 1;
    }

    if (devino && didname && id_devino == id_didname && type_devino == rqst->type) {
        /* the same */

        LOG(log_debug, logtype_cnid, "cnid_lookup: Looked up dev/ino 0x%llx/0x%llx did %u name %s as %u", 
            (unsigned long long)rqst->dev, (unsigned long long)rqst->ino, ntohl(rqst->did), rqst->name, ntohl(id_didname));

        rply->cnid = id_didname;
        rply->result = CNID_DBD_RES_OK;
        return 1;
    }
    
    /* 
       Order matters for the next 2 ifs because both found a CNID but they do not match.
       So in order to pick the CNID from "didname" it must come after devino.
       See test cases laid out in dbd_lookup.c.
    */
    if (devino) {
        LOG(log_maxdebug, logtype_cnid, "CNID resolve problem: server side rename oder reused inode for '%s'", rqst->name);
        rqst->cnid = id_devino;
        if (type_devino != rqst->type) {
            /* same dev:inode but not same type one is a folder the other 
             * is a file,it's an inode reused, delete the record
            */
            if (dbd_delete(dbd, rqst, rply, DBIF_CNID) < 0) {
                return -1;
            }
        }
        else {
            update = 1;
        }
    }
    if (didname) {
        LOG(log_maxdebug, logtype_cnid, "CNID resolve problem: changed dev/ino for '%s'", rqst->name);
        rqst->cnid = id_didname;
        /* we have a did:name 
         * if it's the same dev or not the same type
         * just delete it
        */
        if (!memcmp(dev, (char *)diddata.data + CNID_DEV_OFS, CNID_DEV_LEN) ||
                   type_didname != rqst->type) {
            if (dbd_delete(dbd, rqst, rply, DBIF_CNID) < 0) {
                return -1;
            }
        }
        else {
            update = 1;
        }
    }
    if (!update) {
        rply->result = CNID_DBD_RES_NOTFOUND;
        return 1;
    }
    /* Fix up the database. assume it was a file move and rename */
    rc = dbd_update(dbd, rqst, rply);
    if (rc >0) {
        rply->cnid = rqst->cnid;
    }

    LOG(log_debug, logtype_cnid, "cnid_lookup: Looked up dev/ino 0x%llx/0x%llx did %u name %s as %u (needed update)", 
        (unsigned long long)rqst->dev, (unsigned long long)rqst->ino, ntohl(rqst->did), rqst->name, ntohl(rply->cnid));

    return rc;
}
