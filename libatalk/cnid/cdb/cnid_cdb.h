/* 
 * interface for database access to cnids. i do it this way to abstract
 * things a bit in case we want to change the underlying implementation.
 */

#ifndef _ATALK_CNID_CDB__H
#define _ATALK_CNID_CDB__H 1

#include <sys/cdefs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <netatalk/endian.h>
#include <atalk/cnid.h>

/* cnid_open.c */
extern struct _cnid_module cnid_cdb_module;
extern struct _cnid_db *cnid_cdb_open __P((const char *, mode_t));

/* cnid_close.c */
extern void cnid_cdb_close __P((struct _cnid_db *));

/* cnid_add.c */
extern cnid_t cnid_cdb_add __P((struct _cnid_db *, const struct stat *, const cnid_t,
			    char *, const int, cnid_t));
extern int cnid_cdb_getstamp __P((struct _cnid_db *, void *, const int ));

/* cnid_get.c */
extern cnid_t cnid_cdb_get __P((struct _cnid_db *, const cnid_t, char *, const int)); 
extern char *cnid_cdb_resolve __P((struct _cnid_db *, cnid_t *, void *, u_int32_t )); 
extern cnid_t cnid_cdb_lookup __P((struct _cnid_db *, const struct stat *, const cnid_t,
			       char *, const int));

/* cnid_update.c */
extern int cnid_cdb_update __P((struct _cnid_db *, const cnid_t, const struct stat *,
			    const cnid_t, char *, int));

/* cnid_delete.c */
extern int cnid_cdb_delete __P((struct _cnid_db *, const cnid_t));

/* cnid_nextid.c */
extern cnid_t cnid_cdb_nextid __P((struct _cnid_db *));

extern int cnid_cdb_lock   __P((void *));
extern int cnid_cdb_unlock __P((void *));

extern cnid_t cnid_cdb_rebuild_add __P((struct _cnid_db *, const struct stat *,
                const cnid_t, const char *, const int, cnid_t));


#endif /* include/atalk/cnid_cdb.h */

