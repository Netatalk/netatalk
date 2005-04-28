/*
 * $Id: cnid_cdb_private.h,v 1.2 2005-04-28 20:49:59 bfernhomberg Exp $
 */

#ifndef LIBATALK_CDB_PRIVATE_H
#define LIBATALK_CDB_PRIVATE_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <netatalk/endian.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <sys/param.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#include <sys/cdefs.h>
#include <db.h>

#include <atalk/logger.h>
#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/util.h>
#include "cnid_cdb.h"

#define CNID_DB_MAGIC   0x434E4944U  /* CNID */
#define CNID_DATA_MAGIC 0x434E4945U  /* CNIE */

/* record structure 
   cnid                4
   dev                 8
   inode               8
   type/last cnid      4 Not used    
   did                 4
   name                strlen(name) +1

primary key 
cnid
secondary keys
dev/inode
did/name   

*/

typedef struct cnid_record { /* helper for debug don't use */
  unsigned int  cnid;
  dev_t    dev;
  ino_t    inode;
  unsigned int  type;
  unsigned int  did;
  char          name[50];
} cnid_record;

#define CNID_OFS                 0
#define CNID_LEN                 4

#define CNID_DEV_OFS             CNID_LEN
#define CNID_DEV_LEN             8

#define CNID_INO_OFS             (CNID_DEV_OFS + CNID_DEV_LEN)
#define CNID_INO_LEN             8

#define CNID_DEVINO_OFS          CNID_LEN
#define CNID_DEVINO_LEN          (CNID_DEV_LEN +CNID_INO_LEN)

#define CNID_TYPE_OFS            (CNID_DEVINO_OFS +CNID_DEVINO_LEN)
#define CNID_TYPE_LEN            4

#define CNID_DID_OFS             (CNID_TYPE_OFS +CNID_TYPE_LEN)
#define CNID_DID_LEN             CNID_LEN

#define CNID_NAME_OFS            (CNID_DID_OFS + CNID_DID_LEN)
#define CNID_HEADER_LEN          (CNID_NAME_OFS)

#define CNID_START               17

#define CNIDFLAG_ROOTINFO_RO     (1 << 0)
#define CNIDFLAG_DB_RO           (1 << 1)

/* the key is cnid 0 
 * we use 0/RootInfo. */
#define ROOTINFO_KEY    "\0\0\0\0"
#define ROOTINFO_KEYLEN 4

/*                         cnid   - dev        - inode     - type  - did  - name */
#define ROOTINFO_DATA    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0RootInfo"
#define ROOTINFO_DATALEN (3*4 +2*8  +9)


typedef struct CNID_private {
    u_int32_t magic;
    DB *db_cnid;
    DB *db_didname;
    DB *db_devino;
    DB_ENV *dbenv;
    int lockfd, flags;
    char lock_file[MAXPATHLEN + 1];
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
#ifndef ATACC
static void make_devino_data(unsigned char *buf, dev_t dev, ino_t ino)
{
    buf[CNID_DEV_LEN - 1] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 2] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 3] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 4] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 5] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 6] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 7] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 8] = dev;

    buf[CNID_DEV_LEN + CNID_INO_LEN - 1] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 2] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 3] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 4] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 5] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 6] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 7] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 8] = ino;    
}

static __inline__ char *make_cnid_data(const struct stat *st,
                                       const cnid_t did,
                                       const char *name, const int len)
{
    static char start[CNID_HEADER_LEN + MAXPATHLEN + 1];
    char *buf = start  +CNID_LEN;
    u_int32_t i;

    if (len > MAXPATHLEN)
        return NULL;

    make_devino_data(buf, st->st_dev, st->st_ino);
    buf += CNID_DEVINO_LEN;

    i = S_ISDIR(st->st_mode)?1:0;
    i = htonl(i);
    memcpy(buf, &i, sizeof(i));
    buf += sizeof(i);
    
    /* did is already in network byte order */
    memcpy(buf, &did, sizeof(did));
    buf += sizeof(did);

    memcpy(buf, name, len);
    *(buf + len) = '\0';

    return start;
}
#else
extern char *make_cnid_data __P((const struct stat *,const cnid_t ,
                                       const char *, const int ));
#endif

#endif /* atalk/cnid/cnid_private.h */
