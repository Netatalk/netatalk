/* 
 * interface for database access to cnids. i do it this way to abstract
 * things a bit in case we want to change the underlying implementation.
 */

#ifndef _ATALK_CNID_H
#define _ATALK_CNID_H 1

#include <sys/cdefs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <netatalk/endian.h>

#define CNID_INVALID   0

#define CNID_ERR_PARAM 0x80000001
#define CNID_ERR_PATH  0x80000002
#define CNID_ERR_DB    0x80000003
#define CNID_ERR_MAX   0x80000004

typedef u_int32_t cnid_t;

/* cnid_open.c */
extern void *cnid_open __P((const char *));

/* cnid_close.c */
extern void cnid_close __P((void *));

/* cnid_add.c */
extern cnid_t cnid_add __P((void *, const struct stat *, const cnid_t,
			    const char *, const int, cnid_t));

/* cnid_get.c */
extern cnid_t cnid_get __P((void *, const cnid_t, const char *, const int)); 
extern char *cnid_resolve __P((void *, cnid_t *)); 
extern cnid_t cnid_lookup __P((void *, const struct stat *, const cnid_t,
			       const char *, const int));

/* cnid_update.c */
extern int cnid_update __P((void *, const cnid_t, const struct stat *,
			    const cnid_t, const char *, int));

/* cnid_delete.c */
extern int cnid_delete __P((void *, const cnid_t));

/* cnid_nextid.c */
extern cnid_t cnid_nextid __P((void *));

#endif /* include/atalk/cnid.h */
