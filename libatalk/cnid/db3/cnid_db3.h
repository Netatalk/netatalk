/* 
 * interface for database access to cnids. i do it this way to abstract
 * things a bit in case we want to change the underlying implementation.
 */

#ifndef _ATALK_CNID_DB3__H
#define _ATALK_CNID_DB3__H 1

#include <sys/cdefs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <netatalk/endian.h>
#include <atalk/cnid.h>

static __inline__ int db3_txn_abort(DB_TXN *db_txn)
{
    int ret;
#if DB_VERSION_MAJOR >= 4
    ret = db_txn->abort(db_txn);
#else
    ret = txn_abort(db_txn);
#endif
    return ret;
}

/* -------------------- */
static __inline__ int db3_txn_begin(DB_ENV *db_env, DB_TXN *parent, DB_TXN **db_txn, u_int32_t flags)
{
    int ret;
#if DB_VERSION_MAJOR >= 4
    ret = db_env->txn_begin(db_env, parent, db_txn, flags);
#else
    ret = txn_begin(db_env, paren, db_txn, flags);
#endif
    return ret;
}
            
/* -------------------- */
static __inline__ int db3_txn_commit(DB_TXN *db_txn, u_int32_t flags)
{
    int ret;
#if DB_VERSION_MAJOR >= 4
    ret = db_txn->commit(db_txn, flags);
#else
    ret = txn_commit(db_txn, flags);
#endif
    return ret;
}

/* -----------------------
   cnid_open.c 
*/
extern struct _cnid_module cnid_db3_module;
extern struct _cnid_db *cnid_db3_open __P((const char *, mode_t));

/* cnid_close.c */
extern void cnid_db3_close __P((struct _cnid_db *));

/* cnid_add.c */
extern cnid_t cnid_db3_add __P((struct _cnid_db *, const struct stat *, const cnid_t,
			    char *, const int, cnid_t));

/* cnid_get.c */
extern cnid_t cnid_db3_get __P((struct _cnid_db *, const cnid_t, char *, const int)); 
extern char *cnid_db3_resolve __P((struct _cnid_db *, cnid_t *, void *, u_int32_t )); 
extern cnid_t cnid_db3_lookup __P((struct _cnid_db *, const struct stat *, const cnid_t,
			       char *, const int));

/* cnid_update.c */
extern int cnid_db3_update __P((struct _cnid_db *, const cnid_t, const struct stat *,
			    const cnid_t, char *, int));

/* cnid_delete.c */
extern int cnid_db3_delete __P((struct _cnid_db *, const cnid_t));

/* cnid_nextid.c */
extern cnid_t cnid_db3_nextid __P((struct _cnid_db *));

extern int cnid_db3_lock   __P((void *));
extern int cnid_db3_unlock __P((void *));

#endif /* include/atalk/cnid_db3.h */

