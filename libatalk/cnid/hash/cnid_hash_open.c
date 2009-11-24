/*
 * $Id: cnid_hash_open.c,v 1.3 2009-11-24 12:18:20 didg Exp $
 *
 * Copyright (c) 1999. Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef CNID_BACKEND_HASH
#include <sys/param.h>   

#include "cnid_hash.h"
#include <atalk/logger.h>
#include <stdlib.h>
#define DBHOME       ".AppleDB" 
#define DBNAME       "private_tdb.%sX"
#define DBHOMELEN    9
#define DBLEN        24 

static struct _cnid_db *cnid_hash_new(const char *volpath)
{
    struct _cnid_db *cdb;
    struct _cnid_hash_private *priv;

    if ((cdb = (struct _cnid_db *) calloc(1, sizeof(struct _cnid_db))) == NULL)
        return NULL;

    if ((cdb->volpath = strdup(volpath)) == NULL) {
        free(cdb);
        return NULL;
    }

    if ((cdb->_private = calloc(1, sizeof(struct _cnid_hash_private))) == NULL) {
        free(cdb->volpath);
        free(cdb);
        return NULL;
    }

    /* Set up private state */
    priv = (struct _cnid_hash_private *) (cdb->_private);

    /* Set up standard fields */
    cdb->flags = CNID_FLAG_PERSISTENT;

    cdb->cnid_add = cnid_hash_add;
    cdb->cnid_delete = cnid_hash_delete;
    cdb->cnid_get = cnid_hash_get;
    cdb->cnid_lookup = cnid_hash_lookup;
    cdb->cnid_nextid = NULL;    /*cnid_hash_nextid;*/
    cdb->cnid_resolve = cnid_hash_resolve;
    cdb->cnid_update = cnid_hash_update;
    cdb->cnid_close = cnid_hash_close;
    
    return cdb;
}

/* ---------------------------- */
struct _cnid_db *cnid_hash_open(const char *dir, mode_t mask, u_int32_t flags _U_)
{
    struct stat               st;
    struct _cnid_db           *cdb;
    struct _cnid_hash_private *db;
    size_t                    len;
    char                      path[MAXPATHLEN + 1];
    
    if (!dir) {
        return NULL;
    }

    if ((len = strlen(dir)) > (MAXPATHLEN - DBLEN - 1)) {
        LOG(log_error, logtype_default, "cnid_open: Pathname too large: %s", dir);
        return NULL;
    }
    
    if ((cdb = cnid_hash_new(dir)) == NULL) {
        LOG(log_error, logtype_default, "cnid_open: Unable to allocate memory for hash");
        return NULL;
    }
    strcpy(path, dir);
    if (path[len - 1] != '/') {
        strcat(path, "/");
        len++;
    }
 
    strcpy(path + len, DBHOME);
    if ((stat(path, &st) < 0) && (ad_mkdir(path, 0777 & ~mask) < 0)) {
        LOG(log_error, logtype_default, "cnid_open: DBHOME mkdir failed for %s", path);
        goto fail;
    }
    strcat(path, "/");
 
    path[len + DBHOMELEN] = '\0';
    strcat(path, DBNAME);
    db = (struct _cnid_hash_private *)cdb->_private;
    db->tdb = tdb_open(path, 0, TDB_CLEAR_IF_FIRST | TDB_INTERNAL, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (!db->tdb) {
        LOG(log_error, logtype_default, "cnid_open: unable to open tdb", path);
        goto fail;
    }

    return cdb;

fail:
    free(cdb->_private);
    free(cdb->volpath);
    free(cdb);
    
    return NULL;
}

struct _cnid_module cnid_hash_module = {
    "hash",
    {NULL, NULL},
    cnid_hash_open,
};


#endif /* CNID_BACKEND_HASH */
