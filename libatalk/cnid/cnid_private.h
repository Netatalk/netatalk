/*
 * $Id: cnid_private.h,v 1.5 2001-12-14 03:10:37 jmarcus Exp $
 */

#ifndef LIBATALK_CNID_PRIVATE_H
#define LIBATALK_CNID_PRIVATE_H 1

#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <db.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>

#define CNID_DB_MAGIC   0x434E4944U  /* CNID */
#define CNID_DATA_MAGIC 0x434E4945U  /* CNIE */

#define CNID_DEVINO_LEN          8
#define CNID_DID_LEN             4
#define CNID_HEADER_LEN          (CNID_DEVINO_LEN + CNID_DID_LEN)

#define CNID_START               3

#define CNIDFLAG_ROOTINFO_RO     (1 << 0)
#define CNIDFLAG_DB_RO           (1 << 1)

/* the key is in the form of a did/name pair. in this case,
 * we use 0/RootInfo. */
#define ROOTINFO_KEY    "\0\0\0\0RootInfo"
#define ROOTINFO_KEYLEN 12

typedef struct CNID_private {
    u_int32_t magic;
    DB *db_cnid;
    DB *db_didname;
    DB *db_devino;
#ifdef EXTENDED_DB
    DB *db_shortname;
    DB *db_macname;
    DB *db_longname;
#endif /* EXTENDED_DB */
    DB_ENV* dbenv;
    int lockfd, flags;
    char close_file[MAXPATHLEN + 1];
} CNID_private;

/* on-disk data format (in network byte order where appropriate) --
 * db_cnid:      (key: cnid)
 * name          size (in bytes)
 * dev           4
 * ino           4
 * did           4
 * unix name     strlen(name) + 1 
 *
 * db_didname:   (key: did/unix name)
 * -- this also caches the bits of .AppleDouble used by FPGetFilDirParam
 *    so that we don't have to open the header file.
 *    NOTE: FPCatSearch has to search through all of the directories as
 *          this stuff doesn't get entered until needed.
 *          if the entire volume is in the database, though, we can use
 *          cursor operations to make this faster.
 *
 *    version number is stored with did/name key of 0/0
 *
 * cnid          4
 * modfiller     4 (dates only use 4 bytes right now, but we leave space 
 * moddate       4  for 8. moddate is also used to keep this info 
 * createfiller  4  up-to-date.)
 * createdate    4
 * backfiller    4
 * backupdate    4
 * accfiller     4 (unused)
 * accdate       4 (unused)
 * AFP info      4 (stores a couple permission bits as well)
 * finder info   32
 * prodos info   8
 * rforkfiller   4
 * rforklen      4
 * macname       32 (nul-terminated)
 * shortname     12 (nul-terminated)
 * longname      longnamelen (nul-terminated)
 * ---------------
 *             132 bytes + longnamelen
 * 
 * db_devino:    (key: dev/ino) 
 * -- this is only used for consistency checks and isn't 1-1
 * cnid          4 
 *
 * these correspond to the different path types. longname is for the
 * 255 unicode character names (path type == ?), macname is for the
 * 32-character names (path type == 2), and shortname is for the
 * 8+3-character names (path type == 1).
 *
 * db_longname: (key: did/longname)
 * name          namelen = strlen(name) + 1
 *
 * db_macname:   (key: did/macname)
 * name          namelen = strlen(name) + 1
 *
 * db_shortname: (key: did/shortname)
 * name namelen = strlen(name) + 1 
 */

#ifndef __inline__
#define __inline__
#endif /* __inline__ */

/* construct db_cnid data. NOTE: this is not re-entrant.  */
static __inline__ char *make_cnid_data(const struct stat *st,
                                       const cnid_t did,
                                       const char *name, const int len)
{
    static char start[CNID_HEADER_LEN + MAXPATHLEN + 1];
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

#endif /* atalk/cnid/cnid_private.h */
