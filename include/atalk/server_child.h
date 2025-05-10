/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved.
 */

#ifndef _ATALK_SERVER_CHILD_H
#define _ATALK_SERVER_CHILD_H 1

#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>

/* useful stuff for child processes. most of this is hidden in
 * server_child.c to ease changes in implementation */

#define CHILD_HASHSIZE 32

/* One AFP session child process */
typedef struct afp_child {
    /* afpd worker process pid (from the worker afpd process )*/
    pid_t afpch_pid;
    /* user id of connected client (from the worker afpd process) */
    uid_t afpch_uid;
    /* 1 if we have a clientid */
    int afpch_valid;
    /* 1 if we already tried to kill the client */
    int afpch_killed;
    /* client boot time (from the mac client) */
    uint32_t afpch_boottime;
    /* time the child was added */
    time_t afpch_logintime;
    /* clientid len (from the Mac client) */
    uint32_t afpch_idlen;
    /* clientid (from the Mac client) */
    char *afpch_clientid;
    /* socket for IPC bw afpd parent and childs */
    int afpch_ipc_fd;
    /* state of AFP session (eg active, sleeping, disconnected) */
    int16_t afpch_state;
    /* mounted volumes */
    char *afpch_volumes;
    struct afp_child **afpch_prevp;
    struct afp_child *afpch_next;
} afp_child_t;

/* Info and table with all AFP session child processes */
typedef struct {
    /* Lock */
    pthread_mutex_t servch_lock;
    /* Current count of active AFP sessions */
    int servch_count;
    /* Number of allowed AFP sessions */
    int servch_nsessions;
    /* Hashtable with data of AFP sesssions */
    afp_child_t *servch_table[CHILD_HASHSIZE];
} server_child_t;

/* server_child.c */
extern server_child_t *server_child_alloc(int);
extern afp_child_t *server_child_add(server_child_t *, pid_t, int ipc_fd);
extern int  server_child_remove(server_child_t *, pid_t);
extern void server_child_free(server_child_t *);
extern afp_child_t *server_child_resolve(server_child_t *childs, id_t pid);
extern void server_child_kill(server_child_t *, int);
extern void server_child_kill_one_by_id(server_child_t *children, pid_t pid,
                                        uid_t, uint32_t len, char *id,
                                        uint32_t boottime);
extern int  server_child_transfer_session(server_child_t *children, pid_t,
        uid_t, int, uint16_t);
extern void server_child_handler(server_child_t *);
extern void server_child_login_done(server_child_t *children, pid_t pid,
                                    uid_t);
extern void server_reset_signal(void);

#endif
