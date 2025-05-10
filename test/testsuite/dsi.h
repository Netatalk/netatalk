/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved.
 *
 * modified for test-suite
 */

#ifndef _ATALK_DSI_H
#define _ATALK_DSI_H

#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#include <netinet/in.h>

/* What a DSI packet looks like:
 0                               32
 |-------------------------------|
 |flags  |command| requestID     |
 |-------------------------------|
 |error code/enclosed data offset|
 |-------------------------------|
 |total data length              |
 |-------------------------------|
 |reserved field                 |
 |-------------------------------|

 CONVENTION: anything with a dsi_ prefix is kept in network byte order.
*/

#define DSI_BLOCKSIZ 16
struct dsi_block {
    uint8_t dsi_flags;       /* packet type: request or reply */
    uint8_t dsi_command;     /* command */
    uint16_t dsi_requestID;  /* request ID */
    uint32_t dsi_code;       /* error code or data offset */
    uint32_t dsi_len;        /* total data length */
    uint32_t dsi_reserved;   /* reserved field */
};

#define DSI_CMDSIZ        800
#define DSI_DATASIZ       8192
/* child and parent processes might interpret a couple of these
 * differently. */
typedef struct DSI {
    struct dsi_block header;
    struct sockaddr_in server, client;
    sigset_t sigblockset;
    struct itimerval timer, savetimer;
    uint32_t attn_quantum, datasize, server_quantum;
    uint16_t serverID, clientID;
    uint8_t *status, commands[DSI_CMDSIZ], data[DSI_DATASIZ];
    int statuslen;
    size_t datalen, cmdlen;
    size_t read_count, write_count;
    int asleep; /* client won't reply AFP 0x7a ? */
    /* inited = initialized?, child = a child?, noreply = send reply? */
    char child, inited, noreply;
    const char *program;
    int socket, serversock;

    /* protocol specific open/close, send/receive
     * send/receive fill in the header and use dsi->commands.
     * write/read just write/read data */
    pid_t  (*proto_open)(struct DSI *);
    void   (*proto_close)(struct DSI *);
} DSI;

/* DSI flags */
#define DSIFL_REQUEST    0x00
#define DSIFL_REPLY      0x01
#define DSIFL_MAX        0x01

/* DSI session options */
#define DSIOPT_SERVQUANT 0x00   /* server request quantum */
#define DSIOPT_ATTNQUANT 0x01   /* attention quantum */

/* DSI Commands */
#define DSIFUNC_CLOSE   1       /* DSICloseSession */
#define DSIFUNC_CMD     2       /* DSICommand */
#define DSIFUNC_STAT    3       /* DSIGetStatus */
#define DSIFUNC_OPEN    4       /* DSIOpenSession */
#define DSIFUNC_TICKLE  5       /* DSITickle */
#define DSIFUNC_WRITE   6       /* DSIWrite */
#define DSIFUNC_ATTN    8       /* DSIAttention */
#define DSIFUNC_MAX     8       /* largest command */

/* DSI Error codes: most of these aren't used. */
#define DSIERR_OK	0x0000
#define DSIERR_BADVERS	0xfbd6
#define DSIERR_BUFSMALL	0xfbd5
#define DSIERR_NOSESS	0xfbd4
#define DSIERR_NOSERV	0xfbd3
#define DSIERR_PARM	0xfbd2
#define DSIERR_SERVBUSY	0xfbd1
#define DSIERR_SESSCLOS	0xfbd0
#define DSIERR_SIZERR	0xfbcf
#define DSIERR_TOOMANY	0xfbce
#define DSIERR_NOACK	0xfbcd

/* server and client quanta */
#define DSI_DEFQUANT        2           /* default attention quantum size */
#define DSI_SERVQUANT_MAX   0xffffffffL /* server quantum */
#define DSI_SERVQUANT_MIN   0x0004A2E0L /* minimum server quantum */
#define DSI_SERVQUANT_DEF   DSI_SERVQUANT_MIN /* default server quantum */

/* default port number */
#define DSI_AFPOVERTCP_PORT 548

/* basic initialization: dsi_init.c */
extern DSI *dsi_init (
    const char * /*program*/,
    const char * /*host*/, const char * /*address*/,
    const int /*port*/, const int /*proxy*/,
    const uint32_t /* server quantum */);
extern void dsi_setstatus (DSI *, uint8_t *, const int);

/* in dsi_getsess.c */
extern void dsi_kill (int);

/* low-level stream commands -- in dsi_stream.c */
extern size_t dsi_stream_write (DSI *, void *, const size_t);
extern size_t dsi_stream_read (DSI *, void *, const size_t);
extern int dsi_stream_send (DSI *, void *, size_t);
extern int dsi_stream_receive (DSI *, void *, const size_t, size_t *);

/* client writes -- dsi_write.c */
extern size_t dsi_writeinit (DSI *, void *, const size_t);
extern size_t dsi_write (DSI *, void *, const size_t);
extern void   dsi_writeflush (DSI *);
#define dsi_wrtreply(a,b)  dsi_cmdreply(a,b)

/* client reads -- dsi_read.c */
extern ssize_t dsi_readinit (DSI *, void *, const size_t, const size_t,
                             const int);
extern ssize_t dsi_read (DSI *, void *, const size_t);
extern void dsi_readdone (DSI *);

/* some useful macros */
#define dsi_serverID(x)   ((x)->serverID++)
#define dsi_send(x)       do { \
    (x)->header.dsi_len = htonl((x)->cmdlen); \
    dsi_stream_send((x), (x)->commands, (x)->cmdlen); \
} while (0)
#define dsi_receive(x)    (dsi_stream_receive((x), (x)->commands, \
					      DSI_CMDSIZ, &(x)->cmdlen))
#endif /* atalk/dsi.h */
