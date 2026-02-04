#ifndef SPEEDTEST_LOCAL_VFS_H
#define SPEEDTEST_LOCAL_VFS_H

#include <stdint.h>
#include <sys/types.h>  /* for off_t */
#include "afpclient.h"
#include "dsi.h"        /* for DSI type */

/* Local VFS directory/volume tracking */
#define MAXDIR 32
#define MAXVOL 3

/* Heap management - exposed for size sweep cleanup in speedtest.c */
extern char *Dir_heap[MAXVOL][MAXDIR];
extern char *Vol_heap[MAXVOL];

/* Configuration - must be set by caller before using Local VFS */
extern int Local_VFS_Quiet;   /* Suppress verbose output */
extern int Local_VFS_Direct;  /* Enable O_DIRECT flag */

/* Local VFS function declarations */
uint16_t local_openvol(CONN *conn, char *vol);
unsigned int local_closevol(CONN *conn, uint16_t vol);
unsigned int local_getfiledirparams(CONN *conn, uint16_t vol, int did,
                                    char *name, uint16_t f_bitmap, uint16_t d_bitmap);
unsigned int local_createdir(CONN *conn, uint16_t vol, int did, char *name);
unsigned int local_createfile(CONN *conn, uint16_t vol, char type, int did,
                              char *name);
uint16_t local_openfork(CONN *conn, uint16_t vol, int type, uint16_t bitmap,
                        int did, char *name, int access);
unsigned int local_writeheader(DSI *dsi, uint16_t fork, int offset, int size,
                               char *data, char whence);
unsigned int local_writefooter(DSI *dsi, uint16_t fork, int offset, int size,
                               char *data, char whence);
unsigned int local_flushfork(CONN *conn, uint16_t fork);
unsigned int local_closefork(CONN *conn, uint16_t fork);
unsigned int local_delete(CONN *conn, uint16_t vol, int did, char *name);
unsigned int local_setforkparam(CONN *conn, uint16_t fork, uint16_t bitmap,
                                off_t size);
unsigned int local_write(CONN *conn, uint16_t fork, long long offset, int size,
                         char *data, char whence);
unsigned int local_read(CONN *conn, uint16_t fork, long long offset, int size,
                        char *data);
unsigned int local_readheader(DSI *dsi, uint16_t fork, int offset, int size,
                              char *data);
unsigned int local_readfooter(DSI *dsi, uint16_t fork, int offset, int size,
                              char *data);
unsigned int local_copyfile(struct CONN *conn, uint16_t svol, int sdid,
                            uint16_t dvol, int ddid, char *src, char *buf, char *dst);

/* VFS function pointer structure */
struct vfs {
    unsigned int (*getfiledirparams)(CONN *, uint16_t, int, char *, uint16_t,
                                     uint16_t);
    unsigned int (*createdir)(CONN *, uint16_t, int, char *);
    unsigned int (*createfile)(CONN *, uint16_t, char, int, char *);
    uint16_t (*openfork)(CONN *, uint16_t, int, uint16_t, int, char *, int);
    unsigned int (*writeheader)(DSI *, uint16_t, int, int, char *, char);
    unsigned int (*writefooter)(DSI *, uint16_t, int, int, char *, char);
    unsigned int (*flushfork)(CONN *, uint16_t);
    unsigned int (*closefork)(CONN *, uint16_t);
    unsigned int (*delete)(CONN *, uint16_t, int, char *);
    unsigned int (*setforkparam)(CONN *, uint16_t,  uint16_t, off_t);
    unsigned int (*write)(CONN *, uint16_t, long long, int, char *, char);
    unsigned int (*read)(CONN *, uint16_t, long long, int, char *);
    unsigned int (*readheader)(DSI *, uint16_t, int, int, char *);
    unsigned int (*readfooter)(DSI *, uint16_t, int, int, char *);
    unsigned int (*copyfile)(CONN *, uint16_t, int, uint16_t, int, char *, char *,
                             char *);
    uint16_t (*openvol)(CONN *, char *);
    unsigned int (*closevol)(CONN *conn, uint16_t vol);
};

/* VFS structure - populated with local functions */
extern struct vfs local_VFS;

#endif /* SPEEDTEST_LOCAL_VFS_H */
