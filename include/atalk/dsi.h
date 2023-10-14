/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved.
 */

#ifndef _ATALK_DSI_H 
#define _ATALK_DSI_H

#include <sys/types.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <atalk/afp.h>
#include <atalk/server_child.h>
#include <atalk/globals.h>
#include <netatalk/endian.h>


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

/* these need to be kept in sync w/ AFPTRANS_* in <atalk/afp.h>. 
 * convention: AFPTRANS_* = (1 << DSI_*) */
typedef enum {
  DSI_MIN = 1,
  DSI_TCPIP = 1,
  DSI_MAX = 1
} dsi_proto;

#define DSI_BLOCKSIZ 16
struct dsi_block {
  u_int8_t dsi_flags;       /* packet type: request or reply */
  u_int8_t dsi_command;     /* command */
  u_int16_t dsi_requestID;  /* request ID */
  u_int32_t dsi_code;       /* error code or data offset */
  u_int32_t dsi_len;        /* total data length */
  u_int32_t dsi_reserved;   /* reserved field */
};

#define DSI_CMDSIZ        8192 
#define DSI_DATASIZ       8192

/* child and parent processes might interpret a couple of these
 * differently. */
typedef struct DSI {
  AFPObj *AFPobj;
  dsi_proto protocol;
  struct dsi_block header;
  struct sockaddr_storage server, client;
  struct itimerval timer;
  int      tickle;        /* tickle count */
  int	   in_write;	  /* in the middle of writing multiple packets,
                             signal handlers can't write to the socket */
  int      msg_request;   /* pending message to the client */
  int      down_request;  /* pending SIGUSR1 down in 5 mn */

  u_int32_t attn_quantum, datasize, server_quantum;
  u_int16_t serverID, clientID;
  char      *status;
  u_int8_t  commands[DSI_CMDSIZ], data[DSI_DATASIZ];
  size_t statuslen;
  size_t datalen, cmdlen;
  off_t  read_count, write_count;
  uint32_t flags;             /* DSI flags like DSI_SLEEPING, DSI_DISCONNECTED */
  const char *program; 
  int socket, serversock;

  /* protocol specific open/close, send/receive
   * send/receive fill in the header and use dsi->commands.
   * write/read just write/read data */
  pid_t  (*proto_open)(struct DSI *);
  void   (*proto_close)(struct DSI *);

  /* url registered with slpd */
#ifdef USE_SRVLOC
  char srvloc_url[512];
#endif 

#ifdef USE_ZEROCONF
  char *bonjourname;      /* server name as UTF8 maxlen MAXINSTANCENAMELEN */
  int zeroconf_registered;
#endif

  /* DSI readahead buffer used for buffered reads in dsi_peek */
  size_t dsireadbuf; /* size of the DSI readahead buffer used in dsi_peek() */
  char *buffer;
  char *start;
  char *eof;
  char *end;
} DSI;
  
/* DSI flags */
#define DSIFL_REQUEST    0x00
#define DSIFL_REPLY      0x01
#define DSIFL_MAX        0x01

/* DSI session options */
#define DSIOPT_SERVQUANT 0x00   /* server request quantum */
#define DSIOPT_ATTNQUANT 0x01   /* attention quantum */
#define DSIOPT_REPLCSIZE 0x02   /* AFP replaycache size supported by the server (that's us) */

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
#define DSI_SERVQUANT_MAX   0xffffffff  /* server quantum */
#define DSI_SERVQUANT_MIN   32000       /* minimum server quantum */
#define DSI_SERVQUANT_DEF   0x0004A2E0L /* default server quantum */

/* default port number */
#define DSI_AFPOVERTCP_PORT 548

/* DSI session State flags */
#define DSI_DATA             (1 << 0) /* we have received a DSI command */
#define DSI_RUNNING          (1 << 1) /* we have received a AFP command */
#define DSI_SLEEPING         (1 << 2) /* we're sleeping after FPZzz */
#define DSI_EXTSLEEP         (1 << 3) /* we're sleeping after FPZzz */
#define DSI_DISCONNECTED     (1 << 4) /* we're in diconnected state after a socket error */
#define DSI_DIE              (1 << 5) /* SIGUSR1, going down in 5 minutes */
#define DSI_NOREPLY          (1 << 6) /* in dsi_write we generate our own replies */
#define DSI_RECONSOCKET      (1 << 7) /* we have a new socket from primary reconnect */
#define DSI_RECONINPROG      (1 << 8) /* used in the new session in reconnect */
#define DSI_AFP_LOGGED_OUT   (1 << 9) /* client called afp_logout, quit on next EOF from socket */
#if 0
#define DSI_GOT_ECONNRESET   (1 << 10) /* got ECONNRESET from client => exit */
#endif

/* basic initialization: dsi_init.c */
extern DSI *dsi_init (const dsi_proto /*protocol*/,
			  const char * /*program*/, 
			  const char * /*host*/, const char * /*address*/,
			  const char * /*port*/, const int /*proxy*/,
			  const u_int32_t /* server quantum */);
extern void dsi_setstatus (DSI *, char *, const size_t);

/* in dsi_getsess.c */
extern afp_child_t *dsi_getsession (DSI *, server_child *, const int);
extern void dsi_kill (int);


/* DSI Commands: individual files */
extern void dsi_opensession (DSI *);
extern int  dsi_attention (DSI *, AFPUserBytes);
extern int  dsi_cmdreply (DSI *, const int);
extern int dsi_tickle (DSI *);
extern void dsi_getstatus (DSI *);
extern void dsi_close (DSI *);

#define DSI_NOWAIT 1
/* low-level stream commands -- in dsi_stream.c */
extern ssize_t dsi_stream_write (DSI *, void *, const size_t, const int mode);
extern size_t dsi_stream_read (DSI *, void *, const size_t);
extern int dsi_stream_send (DSI *, void *, size_t);
extern int dsi_stream_receive (DSI *);
extern int dsi_disconnect(DSI *dsi);

#ifdef WITH_SENDFILE
extern ssize_t dsi_stream_read_file(DSI *, int, off_t off, const size_t len);
#endif

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

#endif /* atalk/dsi.h */
