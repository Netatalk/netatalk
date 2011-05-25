/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved.
 */

#ifndef _ATALK_SERVER_CHILD_H
#define _ATALK_SERVER_CHILD_H 1

#include <sys/cdefs.h>
#include <sys/types.h>
#include <netatalk/endian.h>

/* useful stuff for child processes. most of this is hidden in 
 * server_child.c to ease changes in implementation */

#define CHILD_NFORKS   2
#define CHILD_ASPFORK  0
#define CHILD_PAPFORK  0
#define CHILD_DSIFORK  1

typedef struct server_child {
  void *fork;
  int count, nsessions, nforks;
} server_child;

typedef struct server_child_data {
  pid_t     pid; 		/* afpd worker process pid (from the worker afpd process )*/
  uid_t     uid;		/* user id of connected client (from the worker afpd process) */
  int       valid;		/* 1 if we have a clientid */
  int       killed;		/* 1 if we already tried to kill the client */
  int       disasociated; /* 1 if this is not a child, but a child from a previous afpd master */
  uint32_t  time;		/* client boot time (from the mac client) */
  uint32_t  idlen;		/* clientid len (from the Mac client) */
  char      *clientid;  /* clientid (from the Mac client) */
  int       ipc_fds[2]; /* socketpair for IPC bw */
  struct server_child_data **prevp, *next;
} afp_child_t;

extern int parent_or_child;

/* server_child.c */
extern server_child *server_child_alloc (const int, const int);
extern afp_child_t *server_child_add (server_child *, int, pid_t, uint ipc_fds[2]);
extern int  server_child_remove (server_child *, const int, const pid_t);
extern void server_child_free (server_child *);

extern void server_child_kill (server_child *, const int, const int);
extern void server_child_kill_one_by_id (server_child *children, const int forkid, const pid_t pid, const uid_t,
                                               const u_int32_t len, char *id, u_int32_t boottime);
extern int  server_child_transfer_session(server_child *children, int forkid, pid_t, uid_t, int, uint16_t);
extern void server_child_setup (server_child *, const int, void (*)(const pid_t));
extern void server_child_handler (server_child *);
extern void server_reset_signal (void);

#endif
