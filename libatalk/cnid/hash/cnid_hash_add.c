/*
 * $Id: cnid_hash_add.c,v 1.2 2005-04-28 20:50:01 bfernhomberg Exp $
 *
 * Copyright (c) 1999. Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_BACKEND_HASH

#include "cnid_hash.h"
#include <atalk/util.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <atalk/logger.h>

/* ------------------------ */
cnid_t cnid_hash_add(struct _cnid_db *cdb, const struct stat *st,
                     const cnid_t did, char *name, const int len, cnid_t hint)
{
    struct stat lst;
    const struct stat *lstp;
    cnid_t aint;
    struct _cnid_hash_private *priv;
    static char buffer[sizeof(cnid_t) + MAXPATHLEN + 1];        
    TDB_DATA key, data;       
    
    if (!cdb || !(cdb->_private))
        return CNID_INVALID;

    priv = (struct _cnid_hash_private *) (cdb->_private);
    lstp = lstat(name, &lst) < 0 ? st : &lst;
    aint = lstp->st_ino & 0xffffffff;

    if (!priv->st_set) {
        priv->st_set = 1;
        priv->st_dev = lstp->st_dev;
    }
    if (!(priv->error & HASH_ERROR_DEV)) {
        if (lstp->st_dev != priv->st_dev) {
            priv->error |= HASH_ERROR_DEV;
            LOG(log_error, logtype_default, "cnid_hash_add: %s not on the same device", name);
        }
    }
    if (!(priv->error & HASH_ERROR_LINK)) {
        if (!S_ISDIR(lstp->st_mode) && lstp->st_nlink > 1) {
            priv->error |= HASH_ERROR_DEV;
            LOG(log_error, logtype_default, "cnid_hash_add: %s more than one hardlink", name);
        }
    }
    if (sizeof(ino_t) > 4 && !(priv->error & HASH_ERROR_INODE)) {
        if (aint != lstp->st_ino) {
            priv->error |= HASH_ERROR_INODE;
            LOG(log_error, logtype_default, "cnid_hash_add: %s high bits set, duplicate", name);
        }
    }
    key.dptr = (char *)&aint;
    key.dsize = sizeof(cnid_t);
                
    memcpy(buffer, &did, sizeof(cnid_t));
    memcpy(buffer+sizeof(cnid_t), name, len +1);
    data.dptr = buffer;
    data.dsize = len+1 +sizeof(cnid_t);
    if (tdb_store(priv->tdb, key, data, TDB_REPLACE)) {
        LOG(log_error, logtype_default, "cnid_hash_add: unable to add %s", name);
    }
    return aint;
}

#endif /* CNID_BACKEND_HASH */
