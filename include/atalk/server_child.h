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

/* server_child.c */
extern server_child *server_child_alloc (const int, const int);
extern int server_child_add (server_child *, const int, const pid_t);
extern int server_child_remove (server_child *, const int, const pid_t);
extern void server_child_free (server_child *);

extern void server_child_kill (server_child *, const int, const int);
extern void server_child_kill_one (server_child *children, const int forkid, const pid_t, const uid_t);
extern void server_child_kill_one_by_id (server_child *children, const int forkid, const pid_t pid, const uid_t,
                                               const u_int32_t len, char *id, u_int32_t boottime);

extern void server_child_setup (server_child *, const int, void (*)(const pid_t));
extern void server_child_handler (server_child *);
extern void server_reset_signal (void);

#endif
