/* 
 * $Id: cnid.h,v 1.14 2009-11-28 13:09:25 didg Exp $
 *
 * Copyright (c) 2003 the Netatalk Team
 * Copyright (c) 2003 Rafal Lewczuk <rlewczuk@pronet.pl>
 * 
 * This program is free software; you can redistribute and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation version 2 of the License or later
 * version if explicitly stated by any of above copyright holders.
 *
 */

/* 
 * This file contains all generic CNID related stuff 
 * declarations. Included:
 * - CNID factory, which retrieves (eventually instantiates) 
 *   CNID objects on demand
 * - selection of CNID backends (default, detected by volume)
 * - full set of CNID operations needed by server core.
 */

#ifndef _ATALK_CNID__H
#define _ATALK_CNID__H 1

#include <atalk/adouble.h>
#include <atalk/list.h>

/* CNID object flags */
#define CNID_FLAG_PERSISTENT   0x01      /* This backend implements DID persistence */
#define CNID_FLAG_MANGLING     0x02      /* This backend has name mangling feature. */
#define CNID_FLAG_SETUID       0x04      /* Set db owner to parent folder owner. */
#define CNID_FLAG_BLOCK        0x08      /* block signals in update. */
#define CNID_FLAG_NODEV        0x10      /* don't use device number only inode */
#define CNID_FLAG_LAZY_INIT    0x20      /* */
#define CNID_FLAG_MEMORY       0x40	 /* this is a memory only db */
#define CNID_FLAG_INODE        0x80	 /* in cnid_add the inode is authoritative */

#define CNID_INVALID   0
/* first valid ID */
#define CNID_START     17

#define CNID_ERR_PARAM 0x80000001
#define CNID_ERR_PATH  0x80000002
#define CNID_ERR_DB    0x80000003
#define CNID_ERR_CLOSE 0x80000004   /* the db was not open */
#define CNID_ERR_MAX   0x80000005

/*
 * This is instance of CNID database object.
 */
struct _cnid_db {
	
	u_int32_t flags;             /* Flags describing some CNID backend aspects. */
	char *volpath;               /* Volume path this particular CNID db refers to. */
	void *_private;              /* back-end speficic data */

	cnid_t (*cnid_add)(struct _cnid_db *cdb, const struct stat *st, const cnid_t did, 
			char *name, const size_t, cnid_t hint);
	int (*cnid_delete)(struct _cnid_db *cdb, cnid_t id);
	cnid_t (*cnid_get)(struct _cnid_db *cdb, const cnid_t did, char *name, const  size_t);
	cnid_t (*cnid_lookup)(struct _cnid_db *cdb, const struct stat *st, const cnid_t did,
			char *name, const size_t);
	cnid_t (*cnid_nextid)(struct _cnid_db *cdb);
	char *(*cnid_resolve)(struct _cnid_db *cdb, cnid_t *id, void *buffer, size_t len);
	int (*cnid_update)(struct _cnid_db *cdb, const cnid_t id, const struct stat *st, 
			const cnid_t did, char *name, const size_t len);	
	void (*cnid_close)(struct _cnid_db *cdb);
	int  (*cnid_getstamp)(struct _cnid_db *cdb, void *buffer, const size_t len);
        cnid_t (*cnid_rebuild_add)(struct _cnid_db *, const struct stat *, const cnid_t,
                        char *, const size_t, cnid_t);
};
typedef struct _cnid_db cnid_db;

/*
 * CNID module - represents particular CNID implementation
 */
struct _cnid_module {
	char *name;
	struct list_head db_list;   /* CNID modules are also stored on a bidirectional list. */
	struct _cnid_db *(*cnid_open)(const char *dir, mode_t mask, u_int32_t flags);
	u_int32_t flags;            /* Flags describing some CNID backend aspects. */

};
typedef struct _cnid_module cnid_module;

/* Inititalize the CNID backends */
void cnid_init();

/* Registers new CNID backend module */
void cnid_register(struct _cnid_module *module);

/* This function opens a CNID database for selected volume. */
struct _cnid_db *cnid_open(const char *volpath, mode_t mask, char *type, int flags);

cnid_t cnid_add(struct _cnid_db *cdb, const struct stat *st, const cnid_t did, 
			char *name, const size_t len, cnid_t hint);

int    cnid_delete(struct _cnid_db *cdb, cnid_t id);

cnid_t cnid_get   (struct _cnid_db *cdb, const cnid_t did, char *name,const size_t len);

int    cnid_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len);

cnid_t cnid_lookup(struct _cnid_db *cdb, const struct stat *st, const cnid_t did,
			char *name, const size_t len);

char *cnid_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer, size_t len);

int cnid_update   (struct _cnid_db *cdb, const cnid_t id, const struct stat *st, 
			const cnid_t did, char *name, const size_t len);

cnid_t cnid_rebuild_add(struct _cnid_db *cdb, const struct stat *st, const cnid_t did,
                        char *name, const size_t len, cnid_t hint);


/* This function closes a CNID database and frees all resources assigned to it. */ 
void cnid_close(struct _cnid_db *db);

#endif

/*
 * $Log: cnid.h,v $
 * Revision 1.14  2009-11-28 13:09:25  didg
 * guard against confused DB returning junk values
 *
 * Revision 1.13  2009/11/24 12:18:19  didg
 * add a flag parameter to cnid open functions
 *
 * Revision 1.12  2005/09/07 15:23:21  didg
 *
 * lazy init dbd database, help with pre tiger OS and a lot of volumes.
 *
 * Revision 1.11  2005/05/03 14:55:12  didg
 *
 * remove gcc warning
 *
 * Revision 1.10  2005/04/28 20:49:51  bfernhomberg
 *
 * - merge branch-netatalk-afp-3x-dev, HEAD was tagged before
 *
 * Revision 1.9.6.8  2005/04/25 22:33:24  lenneis
 * Add a new interface to the cdb and dbd backends: cnid_rebuild_add. It
 * takes dev, ino, did, name and cnid and writes these values unconditionally
 * into the cnid database. To be used in a recovery tool that writes cnids
 * cached in AppleDouble files back into the database. Not used yet by
 * any daemons or command line utilities.
 *
 * Revision 1.9.6.7  2005/02/08 11:46:59  didg
 *
 * warnings fixes from 2.0 branch
 *
 * Revision 1.9.6.6  2004/02/22 18:36:37  didg
 *
 * small clean up
 *
 * Revision 1.9.6.5  2004/01/14 23:15:19  lenneis
 * Check if we can get a DB stamp sucessfully in afs_openvol and fail
 * the open if not.
 *
 * Revision 1.9.6.4  2004/01/10 07:19:31  bfernhomberg
 * add cnid_init prototype
 *
 * Revision 1.9.6.3  2004/01/03 22:42:55  didg
 *
 * better errors handling in afpd for dbd cnid.
 *
 * Revision 1.9.6.2  2004/01/03 22:21:09  didg
 *
 * add nodev volume option (always use 0 for device number).
 *
 * Revision 1.9.6.1  2003/09/09 16:42:20  didg
 *
 * big merge for db frontend and unicode.
 *
 * Revision 1.9.4.2  2003/06/11 15:29:11  rlewczuk
 * Removed obsolete parameter from cnid_add. Spotted by Didier.
 *
 * Revision 1.9.4.1  2003/05/29 07:53:19  rlewczuk
 * Selectable CNIDs. Some refactoring. Propably needs more of refactoring, mainly
 * a well designed API (current API is just an old cnid_* API enclosed in VMT).
 *
 */

