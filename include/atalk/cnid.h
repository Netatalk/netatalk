/*
 * Copyright (c) 2003 the Netatalk Team
 * Copyright (c) 2003 Rafal Lewczuk <rlewczuk@pronet.pl>
 * Copyright (c) 2010 Frank Lahm
 *
 * This program is free software; you can redistribute and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation version 2 of the License or later
 * version if explicitly stated by any of above copyright holders.
 *
 * 2024/07/05: Rafal Lewczuk gave permission to change the license
 * to a later verion of the GNU GPL in
 * https://github.com/Netatalk/netatalk/issues/1193
 *
 */

/*!
 * @file
 * @brief CNID database types, backend operations and catalog lookup interface
 */

#ifndef _ATALK_CNID__H
#define _ATALK_CNID__H 1

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <atalk/adouble.h>
#include <atalk/uuid.h>

/* CNID object flags */
#define CNID_FLAG_PERSISTENT   0x01      /*!< This backend implements DID persistence */
#define CNID_FLAG_NODEV        0x10      /*!< don't use device number only inode */

#define CNID_INVALID   0
#define CNID_START     17    /*!< first valid ID */

#define CNID_ERR_PARAM 0x80000001
#define CNID_ERR_PATH  0x80000002
#define CNID_ERR_DB    0x80000003   /*!< the backend itself is gone */
#define CNID_ERR_CLOSE 0x80000004   /*!< the db was not open */
#define CNID_ERR_MAX   0x80000005
#define CNID_ERR_RESET 0x80000006   /*!< CNID table was reset, all CNIDs stale */
#define CNID_ERR_BUSY  0x80000007   /*!< contended or transient, retryable */
#define CNID_ERR_CORRUPT 0x80000008 /*!< unusable data returned for a CNID */
#define CNID_ERR_NOTFOUND 0x80000009 /*!< no row for the query */

/* CNID_ERR_* exceed INT_MAX; compare against errno through this unsigned view */
#define CNID_ERRNO() ((unsigned int) errno)

/*!
 * @brief Whether errno says the backend could not answer at all
 *
 * The object's existence is unknown, so the reply is the retryable AFPERR_MISC:
 * AFPERR_NOOBJ and AFPERR_NOID are permanent and a client does not retry them.
 */
#define CNID_ERRNO_IS_BACKEND_FAILURE()         \
    (CNID_ERRNO() == CNID_ERR_BUSY              \
     || CNID_ERRNO() == CNID_ERR_CORRUPT        \
     || CNID_ERRNO() == CNID_ERR_DB)

/* Smallest result buffer the cnid_find() wrapper accepts for any backend */
#define CNID_FIND_MIN_RESULTS  100
#define CNID_FIND_MIN_BUFLEN   (CNID_FIND_MIN_RESULTS * sizeof(uint32_t))

/* the cnid_add calls a listing sends the backend before its folder's rows
 * load */
#define CNID_DIRTAB_MISSES 16
/* the rows a folder's table holds at most */
#define CNID_DIRTAB_ROWS   65536
/* the slots a folder's table tries for a name; names that collide past
 * them turn the table off */
#define CNID_DIRTAB_PROBES 128

/* receives one row of a folder; nonzero stops the rows */
typedef int (*cnid_dirlist_fn)(void *ctx, uint64_t id, const char *name,
                               size_t len, uint64_t dev, uint64_t ino);

struct cnid_dirtab;

/*!
 * This is instance of CNID database object.
 */
typedef struct _cnid_db {
    /* Flags describing some CNID backend aspects. */
    uint32_t      cnid_db_flags;
    struct vol   *cnid_db_vol;
    /* back-end speficic data */
    void         *cnid_db_private;

    cnid_t (*cnid_add)(struct _cnid_db *cdb, const struct stat *st, cnid_t did,
                       const char *name, size_t, cnid_t hint);
    int (*cnid_delete)(struct _cnid_db *cdb, cnid_t id);
    cnid_t (*cnid_get)(struct _cnid_db *cdb, cnid_t did, const char *name,
                       size_t);
    cnid_t (*cnid_lookup)(struct _cnid_db *cdb, const struct stat *st,
                          cnid_t did, const char *name, size_t);
    char *(*cnid_resolve)(struct _cnid_db *cdb, cnid_t *id, void *buffer,
                          size_t len);
    int (*cnid_update)(struct _cnid_db *cdb, cnid_t id, const struct stat *st,
                       cnid_t did, const char *name, size_t len);
    void (*cnid_close)(struct _cnid_db *cdb);
    int (*cnid_getstamp)(struct _cnid_db *cdb, void *buffer, const size_t len);
    int (*cnid_find)(struct _cnid_db *cdb, const char *name, size_t namelen,
                     cnid_t scope_did,
                     void *buffer, size_t buflen, bool *more_available);
    int (*cnid_wipe)(struct _cnid_db *cdb);
    int (*cnid_dirlist)(struct _cnid_db *cdb, cnid_t did, size_t max,
                        cnid_dirlist_fn fn, void *ctx);
    struct cnid_dirtab *cnid_dirtabs;
    cnid_t cnid_dirtab_did;     /*!< folder whose listing batch runs */
} cnid_db;

/*
 * Consolidation of args passed from main cnid_open to modules cnid_XXX_open, so
 * that it's easier to add aditional args as required.
 */
struct cnid_open_args {
    uint32_t cnid_args_flags;
    struct vol *cnid_args_vol;
};

/*
 * CNID module - represents particular CNID implementation
 */
struct _cnid_module {
    char *name;
    struct _cnid_module *next; /*!< Permanent registry link, owned by cnid.c. */
    struct _cnid_db *(*cnid_open)(struct cnid_open_args *args);
};

typedef struct _cnid_module cnid_module;

/* Inititalize the CNID backends */
void cnid_init(void);

/* Registers new CNID backend module */
void cnid_register(struct _cnid_module *module);

/* This function opens a CNID database for selected volume. */
struct _cnid_db *cnid_open(struct vol *vol, char *type, int flags);
/* Whether a registered backend carries the scheme name (case-insensitive) */
bool cnid_scheme_registered(const char *name);
cnid_t cnid_add(struct _cnid_db *cdb, const struct stat *st, const cnid_t did,
                const char *name, const size_t len, cnid_t hint);
int    cnid_delete(struct _cnid_db *cdb, cnid_t id);
cnid_t cnid_get(struct _cnid_db *cdb, const cnid_t did, char *name,
                const size_t len);
int    cnid_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len);
cnid_t cnid_lookup(struct _cnid_db *cdb, const struct stat *st,
                   const cnid_t did, char *name, const size_t len);
char  *cnid_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer,
                    size_t len);
int    cnid_update(struct _cnid_db *cdb, const cnid_t id,
                   const struct stat *st, const cnid_t did, char *name,
                   const size_t len);
int    cnid_find(struct _cnid_db *cdb, const char *name, size_t namelen,
                 void *buffer, size_t buflen, bool *more_available);
int    cnid_find_scoped(struct _cnid_db *cdb, const char *name,
                        size_t namelen, cnid_t scope_did,
                        void *buffer, size_t buflen,
                        bool *more_available);
int    cnid_wipe(struct _cnid_db *cdb);
void   cnid_close(struct _cnid_db *db);
void   cnid_dir_arm(struct _cnid_db *cdb, cnid_t did);
void   cnid_dir_serve(struct _cnid_db *cdb, cnid_t did);
void   cnid_dir_forget(struct _cnid_db *cdb, cnid_t did);

#endif
