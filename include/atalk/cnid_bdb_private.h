/*!
 * @file
 * Interface to cnid_dbd daemon. The below structs are on-the-wire layout
 * of a private TCP/IP RPC. All fields are written in NATIVE byte order.
 *
 * SEARCH wire format: for op == CNID_DBD_OP_SEARCH the variable-length
 * `name` payload is prefixed by a 4-byte native-byte-order srch_offset
 * before the search-name bytes; namelen = 4 + actual_name_length.
 * sizeof(struct cnid_dbd_rqst) is unchanged on the wire — non-SEARCH
 * opcodes remain wire-compatible with previous releases.
 */


#ifndef _ATALK_CNID_DBD_PRIVATE_H
#define _ATALK_CNID_DBD_PRIVATE_H 1

#include <sys/param.h>
#include <sys/stat.h>

#include <atalk/adouble.h>
#include <atalk/cnid_private.h>

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
#define CNID_DBD_OP_SEARCH      0x0d
#define CNID_DBD_OP_WIPE        0x0e

#define CNID_DBD_RES_OK            0x00
#define CNID_DBD_RES_NOTFOUND      0x01
#define CNID_DBD_RES_ERR_DB        0x02
#define CNID_DBD_RES_ERR_MAX       0x03
#define CNID_DBD_RES_ERR_DUPLCNID  0x04
#define CNID_DBD_RES_SRCH_CNT      0x05
#define CNID_DBD_RES_SRCH_DONE     0x06

/* DBD_MAX_SRCH_RSLTS is the wire-protocol maximum batch size for the
 * SEARCH op. It must equal CNID_FIND_MIN_RESULTS in <atalk/cnid.h>;
 * the wrapper's lower-bound buflen check uses the latter so that the
 * sqlite/mysql backends do not have to include this DBD-private header. */
#define DBD_MAX_SRCH_RSLTS    100
#define DBD_SEARCH_MAX_OFFSET 50000
#define DBD_FIND_DEADLINE_SEC 10
#define DBD_NUM_OPEN_ARGS     3

/* NOTE: Spotlight's per-term minimum length (formerly named
 * DBD_SEARCH_MIN_NAMELEN here) is intentionally NOT defined in this
 * header. It is a Spotlight-backend policy, not a DBD wire-protocol
 * constant; placing it here would force etc/spotlight/cnid/sl_cnid.c
 * to include this DBD-private header. The constant lives as
 * `SL_CNID_MIN_TERMLEN` inside etc/spotlight/cnid/sl_cnid.c */

struct cnid_dbd_rqst {
    int     op;
    cnid_t  cnid;
    dev_t   dev;
    ino_t   ino;
    uint32_t type;
    cnid_t  did;
    const char *name;
    size_t  namelen;
};

struct cnid_dbd_rply {
    int     result;
    cnid_t  cnid;
    cnid_t  did;
    char    *name;
    size_t  namelen;
};

typedef struct CNID_bdb_private {
    struct vol *vol;
    int       fd;		/*!< File descriptor to cnid_dbd */
    char      stamp[ADEDLEN_PRIVSYN]; /*!< db timestamp */
    char      *client_stamp;
    size_t    stamp_size;
    int       notfirst;   /*!< already open before */
    int       changed;  /*!< stamp differ */
} CNID_bdb_private;


#endif /* include/atalk/cnid_dbd.h */
