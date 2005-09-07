/*
 *  Interface to the cnid_dbd daemon that stores/retrieves CNIDs from a database.
 */


#ifndef _ATALK_CNID_DBD_PRIVATE_H
#define _ATALK_CNID_DBD_PRIVATE_H 1

#include <sys/stat.h>
#include <atalk/adouble.h>
#include <sys/param.h>

#define CNID_DBD_OP_OPEN        0x01
#define CNID_DBD_OP_CLOSE       0x02
#define CNID_DBD_OP_ADD         0x03
#define CNID_DBD_OP_GET         0x04
#define CNID_DBD_OP_RESOLVE     0x05
#define CNID_DBD_OP_LOOKUP      0x06
#define CNID_DBD_OP_UPDATE      0x07
#define CNID_DBD_OP_DELETE      0x08
#define CNID_DBD_OP_MANGLE_ADD  0x09
#define CNID_DBD_OP_MANGLE_GET  0x0a
#define CNID_DBD_OP_GETSTAMP    0x0b
#define CNID_DBD_OP_REBUILD_ADD 0x0c

#define CNID_DBD_RES_OK            0x00
#define CNID_DBD_RES_NOTFOUND      0x01
#define CNID_DBD_RES_ERR_DB        0x02
#define CNID_DBD_RES_ERR_MAX       0x03
#define CNID_DBD_RES_ERR_DUPLCNID  0x04

#define CNID_DB_MAGIC   0x434E4944U  /* CNID */
#define CNID_DATA_MAGIC 0x434E4945U  /* CNIE */

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

/* special key/data pair we use to store current cnid and database stamp in cnid2.db */

#define ROOTINFO_KEY    "\0\0\0\0"
#define ROOTINFO_KEYLEN 4

/*                         cnid   - dev        - inode     - type  - did  - name */
#define ROOTINFO_DATA    "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0RootInfo"
#define ROOTINFO_DATALEN (3*4 +2*8  +9)




struct cnid_dbd_rqst {
    int     op;
    cnid_t  cnid;
    dev_t   dev;
    ino_t   ino;
    u_int32_t type;
    cnid_t  did;
    char   *name;
    size_t  namelen;
};

struct cnid_dbd_rply {
    int     result;    
    cnid_t  cnid;
    cnid_t  did;
    char   *name;
    size_t  namelen;
};

typedef struct CNID_private {
    u_int32_t magic;
    char      db_dir[MAXPATHLEN + 1]; /* Database directory without /.AppleDB appended */
    int       fd;		/* File descriptor to cnid_dbd */
    char      stamp[ADEDLEN_PRIVSYN]; /* db timestamp */
    char      *client_stamp;
    size_t    stamp_size;
    int       notfirst;   /* already open before */
    int       changed;  /* stamp differ */
} CNID_private;


#endif /* include/atalk/cnid_dbd.h */
