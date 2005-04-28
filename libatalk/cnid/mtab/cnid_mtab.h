
/* 
 * interface for database access to cnids. i do it this way to abstract
 * things a bit in case we want to change the underlying implementation.
 */

#ifndef _ATALK_CNID_MTAB__H
#define _ATALK_CNID_MTAB__H 1

#include <sys/cdefs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <netatalk/endian.h>
#include <atalk/cnid.h>

extern struct _cnid_module cnid_mtab_module;

extern struct _cnid_db *cnid_mtab_open __P((const char *, mode_t));
extern void cnid_mtab_close __P((struct _cnid_db *));
extern cnid_t cnid_mtab_add __P((struct _cnid_db *, const struct stat *, const cnid_t,
                                 char *, const int, cnid_t));
extern cnid_t cnid_mtab_get __P((struct _cnid_db *, const cnid_t, char *, const int));
extern char *cnid_mtab_resolve __P((struct _cnid_db *, cnid_t *, void *, u_int32_t));
extern cnid_t cnid_mtab_lookup __P((struct _cnid_db *, const struct stat *, const cnid_t, char *, const int));
extern int cnid_mtab_update __P((struct _cnid_db *, const cnid_t, const struct stat *,
                                 const cnid_t, char *, int));
extern int cnid_mtab_delete __P((struct _cnid_db *, const cnid_t));

#endif /* include/atalk/cnid_mtab.h */
