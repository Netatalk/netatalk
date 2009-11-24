/* 
 * interface for database access to cnids. i do it this way to abstract
 * things a bit in case we want to change the underlying implementation.
 */

#ifndef _ATALK_CNID_HASH__H
#define _ATALK_CNID_HASH__H 1

#include <sys/cdefs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <netatalk/endian.h>
#include <atalk/cnid.h>
#define STANDALONE 1

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <atalk/tdb.h>

#define HASH_ERROR_LINK  1
#define HASH_ERROR_DEV   2
#define HASH_ERROR_INODE 4

struct _cnid_hash_private {
    dev_t  st_dev;
    int    st_set;
    int    error;
    TDB_CONTEXT *tdb;  
};

/* cnid_open.c */
extern struct _cnid_module cnid_hash_module;
extern struct _cnid_db *cnid_hash_open (const char *, mode_t, u_int32_t flags);

/* cnid_close.c */
extern void cnid_hash_close (struct _cnid_db *);

/* cnid_add.c */
extern cnid_t cnid_hash_add (struct _cnid_db *, const struct stat *, const cnid_t,
                                 char *, const int, cnid_t);

/* cnid_get.c */
extern cnid_t cnid_hash_get (struct _cnid_db *, const cnid_t, char *, const int);
extern char *cnid_hash_resolve (struct _cnid_db *, cnid_t *, void *, u_int32_t);
extern cnid_t cnid_hash_lookup (struct _cnid_db *, const struct stat *, const cnid_t, char *, const int);

/* cnid_update.c */
extern int cnid_hash_update (struct _cnid_db *, const cnid_t, const struct stat *,
                                 const cnid_t, char *, int);

/* cnid_delete.c */
extern int cnid_hash_delete (struct _cnid_db *, const cnid_t);

/* cnid_nextid.c */
extern cnid_t cnid_hash_nextid (struct _cnid_db *);

#endif /* include/atalk/cnid_hash.h */
