
/*
 * $Id: cnid_mtab.c,v 1.2 2005-04-28 20:50:01 bfernhomberg Exp $
 *
 * Copyright (c) 1999. Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_BACKEND_MTAB

#include "cnid_mtab.h"
#include <atalk/logger.h>
#include <stdlib.h>

#ifndef AFS
#define CNID_XOR(a) (((a) >> 16) ^ (a))
#define CNID_DEV(a) ((((CNID_XOR(major((a)->st_dev)) & 0xf) << 3) |\
		(CNID_XOR(minor((a)->st_dev)) & 0x7)) << 24)
#define CNID_INODE(a) (((a)->st_ino ^ (((a)->st_ino & 0xff000000) >> 8)) \
		& 0x00ffffff)
#define CNID_FILE(a) (((a) & 0x1) << 31)
#define CNID(a,b) (CNID_DEV(a) | CNID_INODE(a) | CNID_FILE(b))
#else
#define CNID(a,b) (((a)->st_ino & 0x7fffffff) | CNID_FILE(b))
#endif

/* ------------------------ */
cnid_t cnid_mtab_add(struct _cnid_db *cdb, const struct stat *st,
                     const cnid_t did, char *name, const int len, cnid_t hint)
{
    struct stat lst;
    const struct stat *lstp;

    lstp = lstat(name, &lst) < 0 ? st : &lst;

    return htonl(CNID(lstp, 1));
}



void cnid_mtab_close(struct _cnid_db *cdb)
{
    free(cdb->volpath);
    free(cdb);
}



int cnid_mtab_delete(struct _cnid_db *cdb, const cnid_t id)
{
    return CNID_INVALID;
}



/* Return CNID for a given did/name. */
cnid_t cnid_mtab_get(struct _cnid_db *cdb, const cnid_t did, char *name, const int len)
{
    return CNID_INVALID;
}


cnid_t cnid_mtab_lookup(struct _cnid_db *cdb, const struct stat *st, const cnid_t did, char *name, const int len)
{
    return CNID_INVALID;
}


static struct _cnid_db *cnid_mtab_new(const char *volpath)
{
    struct _cnid_db *cdb;

    if ((cdb = (struct _cnid_db *) calloc(1, sizeof(struct _cnid_db))) == NULL)
        return NULL;

    if ((cdb->volpath = strdup(volpath)) == NULL) {
        free(cdb);
        return NULL;
    }

    cdb->flags = 0;
    cdb->cnid_add = cnid_mtab_add;
    cdb->cnid_delete = cnid_mtab_delete;
    cdb->cnid_get = cnid_mtab_get;
    cdb->cnid_lookup = cnid_mtab_lookup;
    cdb->cnid_nextid = NULL;    //cnid_mtab_nextid;
    cdb->cnid_resolve = cnid_mtab_resolve;
    cdb->cnid_update = cnid_mtab_update;
    cdb->cnid_close = cnid_mtab_close;
    
    return cdb;
}

struct _cnid_db *cnid_mtab_open(const char *dir, mode_t mask)
{
    struct _cnid_db *cdb;

    if (!dir) {
        return NULL;
    }

    if ((cdb = cnid_mtab_new(dir)) == NULL) {
        LOG(log_error, logtype_default, "cnid_open: Unable to allocate memory for database");
        return NULL;
    }

    cdb->_private = NULL;
    return cdb;
}

struct _cnid_module cnid_mtab_module = {
    "mtab",
    {NULL, NULL},
    cnid_mtab_open,
};


/* Return the did/name pair corresponding to a CNID. */
char *cnid_mtab_resolve(struct _cnid_db *cdb, cnid_t * id, void *buffer, u_int32_t len)
{
    return NULL;
}


int cnid_mtab_update(struct _cnid_db *cdb, const cnid_t id, const struct stat *st,
                     const cnid_t did, char *name, const int len
                     /*, const char *info, const int infolen */ )
{
    return 0;
}

#endif /* CNID_BACKEND_MTAB */
