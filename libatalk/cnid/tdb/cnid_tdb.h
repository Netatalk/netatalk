/* 
 * interface for database access to cnids. i do it this way to abstract
 * things a bit in case we want to change the underlying implementation.
 */

#ifndef _ATALK_CNID_TDB__H
#define _ATALK_CNID_TDB__H 1

#include <sys/cdefs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <sys/param.h>

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

#define TDB_ERROR_LINK  1
#define TDB_ERROR_DEV   2
#define TDB_ERROR_INODE 4

#define TDB_DB_MAGIC   0x434E4944U  /* CNID */
#define TDB_DATA_MAGIC 0x434E4945U  /* CNIE */

#define TDB_DEVINO_LEN          8
#define TDB_DID_LEN             4
#define TDB_HEADER_LEN          (TDB_DEVINO_LEN + TDB_DID_LEN)

#define TDB_START               17

#define TDBFLAG_ROOTINFO_RO     (1 << 0)
#define TDBFLAG_DB_RO           (1 << 1)

/* the key is in the form of a did/name pair. in this case,
 * we use 0/RootInfo. */
#define ROOTINFO_KEY    "\0\0\0\0RootInfo"
#define ROOTINFO_KEYLEN 12

struct _cnid_tdb_private {
    dev_t  st_dev;
    int    st_set;
    int    error;
    int    flags;
    TDB_CONTEXT *tdb_cnid;
    TDB_CONTEXT *tdb_didname;
    TDB_CONTEXT *tdb_devino;

};

/* cnid_open.c */
extern struct _cnid_module cnid_tdb_module;
extern struct _cnid_db *cnid_tdb_open __P((const char *, mode_t));

/* cnid_close.c */
extern void cnid_tdb_close __P((struct _cnid_db *));

/* cnid_add.c */
extern cnid_t cnid_tdb_add __P((struct _cnid_db *, const struct stat *, const cnid_t,
                                 char *, const size_t, cnid_t));

/* cnid_get.c */
extern cnid_t cnid_tdb_get __P((struct _cnid_db *, const cnid_t, char *, const size_t));
extern char *cnid_tdb_resolve __P((struct _cnid_db *, cnid_t *, void *, size_t));
extern cnid_t cnid_tdb_lookup __P((struct _cnid_db *, const struct stat *, const cnid_t, char *, const size_t));

/* cnid_update.c */
extern int cnid_tdb_update __P((struct _cnid_db *, const cnid_t, const struct stat *,
                                 const cnid_t, char *, size_t));

/* cnid_delete.c */
extern int cnid_tdb_delete __P((struct _cnid_db *, const cnid_t));

/* cnid_nextid.c */
extern cnid_t cnid_tdb_nextid __P((struct _cnid_db *));

/* construct db_cnid data. NOTE: this is not re-entrant.  */
static __inline__ char *make_tdb_data(const struct stat *st,
                                       const cnid_t did,
                                       const char *name, const size_t len)
{
    static char start[TDB_HEADER_LEN + MAXPATHLEN + 1];
    char *buf = start;
    u_int32_t i;

    if (len > MAXPATHLEN)
        return NULL;

    i = htonl(st->st_dev);
    buf = memcpy(buf, &i, sizeof(i));
    i = htonl(st->st_ino);
    buf = memcpy(buf + sizeof(i), &i, sizeof(i));
    /* did is already in network byte order */
    buf = memcpy(buf + sizeof(i), &did, sizeof(did));
    buf = memcpy(buf + sizeof(did), name, len);
    *(buf + len) = '\0';
    buf += len + 1;

    return start;
}

#endif /* include/atalk/cnid_tdb.h */
