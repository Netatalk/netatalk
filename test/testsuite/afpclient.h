/* ---------------------------------------------------
*/
#ifndef AFPCLIENT_H
#define AFPCLIENT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>

#ifndef SA_ONESHOT
#define SA_ONESHOT SA_RESETHAND
#endif


#ifndef HAVE_BSWAP_64
#define bswap_64(x) \
    ((((x) & 0xff00000000000000ull) >> 56)                                   \
     | (((x) & 0x00ff000000000000ull) >> 40)                                 \
     | (((x) & 0x0000ff0000000000ull) >> 24)                                 \
     | (((x) & 0x000000ff00000000ull) >> 8)                                  \
     | (((x) & 0x00000000ff000000ull) << 8)                                  \
     | (((x) & 0x0000000000ff0000ull) << 24)                                 \
     | (((x) & 0x000000000000ff00ull) << 40)                                 \
     | (((x) & 0x00000000000000ffull) << 56))
#endif	/* bswap_64 */

#include "dsi.h"
#include "afp.h"
/* AFP functions */
#ifndef AFP_CLOSEVOL
#define AFP_CLOSEVOL     2
#define AFP_ENUMERATE    9

#define AFP_GETSRVINFO  15
#define AFP_GETSRVPARAM 16
#define AFP_LOGIN       18

#define AFP_LOGOUT      20
#define AFP_OPENVOL     24
#define AFP_OPENDIR     25
#define AFP_OPENFORK    26

#define AFP_OPENDT      48
#define AFP_CLOSEDT     49
#endif

/* ----------------------------- */
/* from etc/afpd/directory.h volume attributes */
#define DIRPBIT_ATTR    0
#define DIRPBIT_PDID    1
#define DIRPBIT_CDATE   2
#define DIRPBIT_MDATE   3
#define DIRPBIT_BDATE   4
#define DIRPBIT_FINFO   5
#define DIRPBIT_LNAME   6
#define DIRPBIT_SNAME   7
#define DIRPBIT_DID     8
#define DIRPBIT_OFFCNT  9
#define DIRPBIT_UID     10
#define DIRPBIT_GID     11
#define DIRPBIT_ACCESS  12
#define DIRPBIT_PDINFO  13         /* ProDOS Info /UTF8 name */
#define DIRPBIT_UNIXPR  15

/* directory attribute bits (see file.h for other bits) */
#define ATTRBIT_EXPFOLDER   (1 << 1) /* shared point */
#define ATTRBIT_MOUNTED     (1 << 3) /* mounted share point by non-admin */
#define ATTRBIT_INEXPFOLDER (1 << 4) /* folder in a shared area */

#define FILDIRBIT_ISDIR        (1 << 7) /* is a directory */
#define FILDIRBIT_ISFILE       (0)      /* is a file */

/* reserved directory id's */
#define DIRDID_ROOT_PARENT    htonl(1)  /* parent directory of root */
#define DIRDID_ROOT           htonl(2)  /* root directory */

/* ----------------------------- */
/* from etc/afpd/file.h volume attributes */
#define FILPBIT_ATTR     0
#define FILPBIT_PDID     1
#define FILPBIT_CDATE    2
#define FILPBIT_MDATE    3
#define FILPBIT_BDATE    4
#define FILPBIT_FINFO    5
#define FILPBIT_LNAME    6
#define FILPBIT_SNAME    7
#define FILPBIT_FNUM     8
#define FILPBIT_DFLEN    9
#define FILPBIT_RFLEN    10
#define FILPBIT_EXTDFLEN 11
#define FILPBIT_PDINFO   13    /* ProDOS Info/ UTF8 name */
#define FILPBIT_EXTRFLEN 14
#define FILPBIT_UNIXPR   15

/* attribute bits. (d) = directory attribute bit as well. */
#define ATTRBIT_INVISIBLE (1<<0)  /* invisible (d) */
#define ATTRBIT_MULTIUSER (1<<1)  /* multiuser */
#define ATTRBIT_SYSTEM    (1<<2)  /* system (d) */
#define ATTRBIT_DOPEN     (1<<3)  /* data fork already open */
#define ATTRBIT_ROPEN     (1<<4)  /* resource fork already open */
#define ATTRBIT_SHARED    (1<<4)  /* shared area (d) */
#define ATTRBIT_NOWRITE   (1<<5)  /* write inhibit(v2)/read-only(v1) bit */
#define ATTRBIT_BACKUP    (1<<6)  /* backup needed (d) */
#define ATTRBIT_NORENAME  (1<<7)  /* rename inhibit (d) */
#define ATTRBIT_NODELETE  (1<<8)  /* delete inhibit (d) */
#define ATTRBIT_NOCOPY    (1<<10) /* copy protect */
#define ATTRBIT_SETCLR    (1<<15) /* set/clear bits (d) */

/* ----------------------------- */
/* from etc/afpd/volume.h volume attributes */
#define VOLPBIT_ATTR_RO           (1 << 0)
#define VOLPBIT_ATTR_PASSWD       (1 << 1)
#define VOLPBIT_ATTR_FILEID       (1 << 2)
#define VOLPBIT_ATTR_CATSEARCH    (1 << 3)
#define VOLPBIT_ATTR_BLANKACCESS  (1 << 4)
#define VOLPBIT_ATTR_UNIXPRIV     (1 << 5)
#define VOLPBIT_ATTR_UTF8         (1 << 6)
#define VOLPBIT_ATTR_NONETUID     (1 << 7)
#define VOLPBIT_ATTR_PRIVPARENT   (1 << 8)
#define VOLPBIT_ATTR_NOEXCHANGE   (1 << 9)
#define VOLPBIT_ATTR_EXTATTRS     (1 << 10)
#define VOLPBIT_ATTR_ACLS         (1 << 11)

#define VOLPBIT_ATTR    0
#define VOLPBIT_SIG     1
#define VOLPBIT_CDATE   2
#define VOLPBIT_MDATE   3
#define VOLPBIT_BDATE   4
#define VOLPBIT_VID     5
#define VOLPBIT_BFREE   6
#define VOLPBIT_BTOTAL  7
#define VOLPBIT_NAME    8
/* handle > 4GB volumes */
#define VOLPBIT_XBFREE  9
#define VOLPBIT_XBTOTAL 10
#define VOLPBIT_BSIZE   11        /* block size */
/* ----------------------------- */

/* from etc/afpd/fork.h */
#define AFPOF_DFORK 0x00
#define AFPOF_RFORK 0x80

#define OPENFORK_DATA   (0)
#define OPENFORK_RSCS   (1<<7)

#define OPENACC_RD      (1<<0)
#define OPENACC_WR      (1<<1)
#define OPENACC_DRD     (1<<4)
#define OPENACC_DWR     (1<<5)

#define AFPFORK_OPEN    (1<<0)
#define AFPFORK_RSRC    (1<<1)
#define AFPFORK_DATA    (1<<2)
#define AFPFORK_DIRTY   (1<<3)
#define AFPFORK_ACCRD   (1<<4)
#define AFPFORK_ACCWR   (1<<5)
#define AFPFORK_ACCMASK (AFPFORK_ACCRD | AFPFORK_ACCWR)

/* we use this so that we can use the same mechanism for both byte
 * locks and file synchronization locks. */
#if _FILE_OFFSET_BITS == 64
#define AD_FILELOCK_BASE (UINT64_C(0x7FFFFFFFFFFFFFFF) - 9)
#else
#define AD_FILELOCK_BASE (UINT32_C(0x7FFFFFFF) - 9)
#endif

typedef struct CONN {
    DSI	dsi;
#if 0
    ASP asp;
#endif
    int type;
    int afp_version;
} CONN;

#define min(a,b)  ((a) < (b) ? (a) : (b))

#define PASSWDLEN 8

#define dsi_clientID(x)   ((x)->clientID++)

#define my_dsi_send(x)       do { \
    (x)->header.dsi_len = htonl((x)->cmdlen); \
    my_dsi_stream_send((x), (x)->commands, (x)->cmdlen); \
} while (0)

int my_dsi_cmd_receive(DSI *x);
int my_dsi_data_receive(DSI *x);

/* from
   modified
 */
/* Files and directories */
struct afp_filedir_parms {
    int isdir;
    uint16_t bitmap;  /* Parameters already taken from svr */
    uint16_t attr;
    uint16_t vid;
    uint32_t pdid;
    uint32_t did;
    uint32_t bdate, mdate, cdate;
    uint32_t dflen, rflen;
    uint64_t ext_dflen;
    uint64_t ext_rflen;
    uint16_t offcnt;
    uint32_t uid, gid;
    uint32_t unix_priv;   /* FIXME what if mode_t != uint32_t */
    uint8_t access[4];    /* Access bits */
    uint8_t pdinfo[6];    /* ProDOS info... */
    char finder_info[32];            // FIXME: Finder info !
    int  name_type;
    char *lname;
    char *sname;
    char *utf8_name;
};

struct afp_volume_parms {
    uint8_t state;   // FIXME: keep state across calls here (OPENED/CLOSED)
    uint8_t flags;
    uint16_t attr;
    uint16_t sig;
    uint32_t cdate, bdate, mdate;
    uint16_t vid;
    uint32_t bfree, btotal, bsize;
    char *name;
    char *utf8_name;
};

void afp_volume_unpack(struct afp_volume_parms *parms, unsigned char *b,
                       uint16_t rbitmap);

void afp_filedir_unpack(struct afp_filedir_parms *filedir, unsigned char *b,
                        uint16_t rfbitmap, uint16_t rdbitmap);
int afp_filedir_pack(unsigned char *b, struct afp_filedir_parms *filedir,
                     uint16_t rfbitmap, uint16_t rdbitmap);

/*
 afpcli.c
*/
int OpenClientSocket(char* host, int port);
int CloseClientSocket(int fd);


size_t my_dsi_stream_read(DSI *dsi, void *data, const size_t length);
int my_dsi_stream_receive(DSI *dsi, void *buf, const size_t ilength,
                          size_t *rlength);
size_t my_dsi_stream_write(DSI *dsi, void *data, const size_t length);
int my_dsi_stream_send(DSI *dsi, void *buf, size_t length);
uint16_t my_dsi_cmd_nwriterply_async(CONN *conn, uint64_t n);

unsigned int DSIOpenSession(CONN *conn);
unsigned int DSIGetStatus(CONN *conn);
unsigned int DSICloseSession(CONN *conn);

unsigned int AFPopenLogin(CONN *conn, char *vers, char *uam, char *usr,
                          char *pwd);
unsigned int AFPopenLoginExt(CONN *conn, char *vers, char *uam, char *usr,
                             char *pwd);
unsigned int AFPLogOut(CONN *conn);
unsigned int AFPChangePW(CONN *conn, char *uam, char *usr, char *opwd,
                         char *pwd);

unsigned int AFPzzz(CONN *conn, int);

unsigned int AFPGetSrvrInfo(CONN *conn);
unsigned int AFPGetSrvrParms(CONN *conn);
unsigned int AFPGetSrvrMsg(CONN *conn, uint16_t type, uint16_t bitmap);

unsigned int AFPCloseVol(CONN *conn, uint16_t vol);
unsigned int AFPCloseDT(CONN *conn, uint16_t vol);

unsigned int AFPByteLock(CONN *conn, uint16_t fork, int end, int mode,
                         int offset, int size);
unsigned int AFPByteLock_ext(CONN *conn, uint16_t fork, int end, int mode,
                             off_t offset, off_t size);
unsigned int AFPCloseFork(CONN *conn, uint16_t fork);
unsigned int AFPFlush(CONN *conn, uint16_t vol);
unsigned int AFPFlushFork(CONN *conn, uint16_t fork);
unsigned int AFPDelete(CONN *conn, uint16_t vol, int did, char *name);

unsigned int AFPGetComment(CONN *conn, uint16_t vol, int did, char *name);
unsigned int AFPRemoveComment(CONN *conn, uint16_t vol, int did, char *name);
unsigned int AFPAddComment(CONN *conn, uint16_t vol, int did, char *name,
                           char *cmt);

uint16_t AFPOpenVol(CONN *conn, char *vol, uint16_t bitmap);
uint16_t AFPOpenFork(CONN *conn, uint16_t vol, char type, uint16_t bitmap,
                     int did, char *name, uint16_t access);

unsigned int AFPGetVolParam(CONN *conn, uint16_t vol, uint16_t bitmap);
unsigned int AFPSetVolParam(CONN *conn, uint16_t vol, uint16_t bitmap,
                            struct afp_volume_parms *parms);

unsigned int  AFPCreateFile(CONN *conn, uint16_t vol, char type, int did,
                            char *name);
unsigned int  AFPCreateDir(CONN *conn, uint16_t vol, int did, char *name);

unsigned int AFPWriteHeader(DSI *dsi, uint16_t fork, int offset, int size,
                            char *data, char whence);
unsigned int AFPWriteFooter(DSI *dsi, uint16_t fork, int offset, int size,
                            char *data, char whence);
unsigned int AFPWrite(CONN *conn, uint16_t fork, int offset, int size,
                      char *data, char whence);
unsigned int AFPWrite_ext(CONN *conn, uint16_t fork, off_t offset, off_t size,
                          char *data, char whence);
unsigned int AFPWrite_ext_async(CONN *conn, uint16_t fork, off_t offset,
                                off_t size, char *data, char whence);

unsigned int AFPReadHeader(DSI *dsi, uint16_t fork, int offset, int size,
                           char *data);
unsigned int AFPReadFooter(DSI *dsi, uint16_t fork, int offset, int size,
                           char *data);
unsigned int AFPRead(CONN *conn, uint16_t fork, int offset, int size,
                     char *data);
unsigned int AFPRead_ext(CONN *conn, uint16_t fork, off_t offset, off_t size,
                         char *data);
unsigned int AFPRead_ext_async(CONN *conn, uint16_t fork, off_t offset,
                               off_t size, char *data);

unsigned int AFPGetForkParam(CONN *conn, uint16_t fork, uint16_t bitmap);

unsigned int AFPGetSessionToken(CONN *conn, int type, uint32_t time, int len,
                                char *token);
unsigned int AFPDisconnectOldSession(CONN *conn, uint16_t type, int len,
                                     char *token);

unsigned int AFPMapID(CONN *conn, char fn, int id);
unsigned int AFPMapName(CONN *conn, char fn, char *name);

unsigned int AFPAddAPPL(CONN *conn, uint16_t dt, int did, char *creator,
                        uint32_t tag, char *name);
unsigned int AFPGetAPPL(CONN *conn, uint16_t dt, char *name, uint16_t index,
                        uint16_t f_bitmap);
unsigned int AFPRemoveAPPL(CONN *conn, uint16_t dt, int did, char *creator,
                           char *name);

unsigned int AFPGetUserInfo(CONN *conn, char flag, int id, uint16_t bitmap);
unsigned int AFPBadPacket(CONN *conn, char fn, char *name);

unsigned int AFPCatSearch(CONN *conn, uint16_t vol, uint32_t  nbe, char *pos,
                          uint16_t f_bitmap, uint16_t d_bitmap,
                          uint32_t rbitmap, struct afp_filedir_parms *filedir,
                          struct afp_filedir_parms *filedir2);

unsigned int AFPCatSearchExt(CONN *conn, uint16_t vol, uint32_t  nbe, char *pos,
                             uint16_t f_bitmap, uint16_t d_bitmap,
                             uint32_t rbitmap, struct afp_filedir_parms *filedir,
                             struct afp_filedir_parms *filedir2);

unsigned int AFPSetForkParam(CONN *conn, uint16_t fork,  uint16_t bitmap,
                             off_t size);

unsigned int AFPGetACL(CONN *conn, uint16_t vol, int did, uint16_t bitmap,
                       char *name);
unsigned int AFPListExtAttr(CONN *conn, uint16_t vol, int did, uint16_t bitmap,
                            int maxsize, char *pathname);
unsigned int AFPGetExtAttr(CONN *conn, uint16_t vol, int did, uint16_t bitmap,
                           int maxsize, char *pathname, char *attrname);
unsigned int AFPSetExtAttr(CONN *conn, uint16_t vol, int did, uint16_t bitmap,
                           char *pathname, char *attrname, char *data);
unsigned int AFPRemoveExtAttr(CONN *conn, uint16_t vol, int did,
                              uint16_t bitmap, char *pathname, char *attrname);

int FPset_name(CONN *conn, int ofs, char *name);
void u2mac(uint8_t *dst, char *name, int len);

char *strp2cdup(unsigned char *src);

#endif

/* ---------------------------------
*/
