/*
 * $Id: cnid_tdb_open.c,v 1.2 2005-04-28 20:50:02 bfernhomberg Exp $
 *
 * Copyright (c) 1999. Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef CNID_BACKEND_TDB
#include <sys/param.h>   

#include "cnid_tdb.h"
#include <atalk/logger.h>
#include <stdlib.h>
#define DBHOME       ".AppleDB"
#define DBNAME       "private_tdb.%sX"
#define DBHOMELEN    9                  /* strlen(DBHOME) +1 for / */
#define DBLEN        12
#define DBCNID       "cnid.tdb"
#define DBDEVINO     "devino.tdb"
#define DBDIDNAME    "didname.tdb"      /* did/full name mapping */

#define DBVERSION_KEY    "\0\0\0\0\0"
#define DBVERSION_KEYLEN 5
#define DBVERSION1       0x00000001U
#define DBVERSION        DBVERSION1

static struct _cnid_db *cnid_tdb_new(const char *volpath)
{
    struct _cnid_db *cdb;
    struct _cnid_tdb_private *priv;

    if ((cdb = (struct _cnid_db *) calloc(1, sizeof(struct _cnid_db))) == NULL)
        return NULL;

    if ((cdb->volpath = strdup(volpath)) == NULL) {
        free(cdb);
        return NULL;
    }

    if ((cdb->_private = calloc(1, sizeof(struct _cnid_tdb_private))) == NULL) {
        free(cdb->volpath);
        free(cdb);
        return NULL;
    }

    /* Set up private state */
    priv = (struct _cnid_tdb_private *) (cdb->_private);

    /* Set up standard fields */
    cdb->flags = CNID_FLAG_PERSISTENT;

    cdb->cnid_add = cnid_tdb_add;
    cdb->cnid_delete = cnid_tdb_delete;
    cdb->cnid_get = cnid_tdb_get;
    cdb->cnid_lookup = cnid_tdb_lookup;
    cdb->cnid_nextid = NULL;    /*cnid_tdb_nextid;*/
    cdb->cnid_resolve = cnid_tdb_resolve;
    cdb->cnid_update = cnid_tdb_update;
    cdb->cnid_close = cnid_tdb_close;
    
    return cdb;
}

/* ---------------------------- */
struct _cnid_db *cnid_tdb_open(const char *dir, mode_t mask)
{
    struct stat               st;
    struct _cnid_db           *cdb;
    struct _cnid_tdb_private *db;
    size_t                    len;
    char                      path[MAXPATHLEN + 1];
    TDB_DATA                  key, data;
    
    if (!dir) {
        return NULL;
    }

    if ((len = strlen(dir)) > (MAXPATHLEN - DBLEN - 1)) {
        LOG(log_error, logtype_default, "tdb_open: Pathname too large: %s", dir);
        return NULL;
    }
    
    if ((cdb = cnid_tdb_new(dir)) == NULL) {
        LOG(log_error, logtype_default, "tdb_open: Unable to allocate memory for tdb");
        return NULL;
    }
    strcpy(path, dir);
    if (path[len - 1] != '/') {
        strcat(path, "/");
        len++;
    }
 
    strcpy(path + len, DBHOME);
    if ((stat(path, &st) < 0) && (ad_mkdir(path, 0777 & ~mask) < 0)) {
        LOG(log_error, logtype_default, "tdb_open: DBHOME mkdir failed for %s", path);
        goto fail;
    }
    strcat(path, "/");
 
    db = (struct _cnid_tdb_private *)cdb->_private;

    path[len + DBHOMELEN] = '\0';
    strcat(path, DBCNID);
    db->tdb_cnid = tdb_open(path, 0, 0 , O_RDWR | O_CREAT, 0666 & ~mask);
    if (!db->tdb_cnid) {
        LOG(log_error, logtype_default, "tdb_open: unable to open tdb", path);
        goto fail;
    }
    /* ------------- */

    path[len + DBHOMELEN] = '\0';
    strcat(path, DBDIDNAME);
    db->tdb_didname = tdb_open(path, 0, 0 , O_RDWR | O_CREAT, 0666 & ~mask);
    if (!db->tdb_cnid) {
        LOG(log_error, logtype_default, "tdb_open: unable to open tdb", path);
        goto fail;
    }
    /* Check for version.  This way we can update the database if we need
     * to change the format in any way. */
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.dptr = DBVERSION_KEY;
    key.dsize = DBVERSION_KEYLEN;

    data = tdb_fetch(db->tdb_didname, key);
    if (!data.dptr) {
        u_int32_t version = htonl(DBVERSION);

        data.dptr = (char *)&version;
        data.dsize = sizeof(version);
        if (tdb_store(db->tdb_didname, key, data, TDB_REPLACE)) {
            LOG(log_error, logtype_default, "tdb_open: Error putting new version");
            goto fail;
        }
    }
    else {
        free(data.dptr);
    }
        
    /* ------------- */
    path[len + DBHOMELEN] = '\0';
    strcat(path, DBDEVINO);
    db->tdb_devino = tdb_open(path, 0, 0 , O_RDWR | O_CREAT, 0666 & ~mask);
    if (!db->tdb_devino) {
        LOG(log_error, logtype_default, "tdb_open: unable to open tdb", path);
        goto fail;
    }

    return cdb;

fail:
    free(cdb->_private);
    free(cdb->volpath);
    free(cdb);
    
    return NULL;
}

struct _cnid_module cnid_tdb_module = {
    "tdb",
    {NULL, NULL},
    cnid_tdb_open,
    CNID_FLAG_SETUID
};


#endif
