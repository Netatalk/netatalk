
/* 
 * interface for database access to cnids. i do it this way to abstract
 * things a bit in case we want to change the underlying implementation.
 */

#ifndef _ATALK_CNID_LAST__H
#define _ATALK_CNID_LAST__H 1

#include <sys/cdefs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <netatalk/endian.h>
#include <atalk/cnid.h>

struct _cnid_last_private {
    cnid_t last_did;
};

extern struct _cnid_module cnid_last_module;
extern struct _cnid_db *cnid_last_open __P((const char *, mode_t));
extern void cnid_last_close __P((struct _cnid_db *));
extern cnid_t cnid_last_add __P((struct _cnid_db *, const struct stat *, const cnid_t,
                                 char *, const int, cnid_t));
extern cnid_t cnid_last_get __P((struct _cnid_db *, const cnid_t, char *, const int));
extern char *cnid_last_resolve __P((struct _cnid_db *, cnid_t *, void *, u_int32_t));
extern cnid_t cnid_last_lookup __P((struct _cnid_db *, const struct stat *, const cnid_t, char *, const int));
extern int cnid_last_update __P((struct _cnid_db *, const cnid_t, const struct stat *,
                                 const cnid_t, char *, int));
extern int cnid_last_delete __P((struct _cnid_db *, const cnid_t));

#endif /* include/atalk/cnid_last.h */
