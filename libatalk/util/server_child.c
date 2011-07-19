/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 *
 *
 * handle inserting, removing, and freeing of children.
 * this does it via a hash table. it incurs some overhead over
 * a linear append/remove in total removal and kills, but it makes
 * single-entry removals a fast operation. as total removals occur during
 * child initialization and kills during server shutdown, this is
 * probably a win for a lot of connections and unimportant for a small
 * number of connections.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <signal.h>
#include <errno.h>

/* POSIX.1 sys/wait.h check */
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#include <sys/time.h>

#include <atalk/logger.h>
#include <atalk/errchk.h>
#include <atalk/util.h>
#include <atalk/server_child.h>

#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif /* ! WEXITSTATUS */
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif /* ! WIFEXITED */
#ifndef WIFSTOPPED
#define WIFSTOPPED(status) (((status) & 0xff) == 0x7f)
#endif
#ifndef WIFSIGNALED
#define WIFSIGNALED(status) (!WIFSTOPPED(status) && !WIFEXITED(status))
#endif
#ifndef WTERMSIG
#define WTERMSIG(status)      ((status) & 0x7f)
#endif

/* hash/child functions: hash OR's pid */
#define CHILD_HASHSIZE 32
#define HASH(i) ((((i) >> 8) ^ (i)) & (CHILD_HASHSIZE - 1))

typedef struct server_child_fork {
    struct server_child_data *table[CHILD_HASHSIZE];
    void (*cleanup)(const pid_t);
} server_child_fork;

int parent_or_child; /* 0: parent, 1: child */

static inline void hash_child(struct server_child_data **htable,
                              struct server_child_data *child)
{
    struct server_child_data **table;

    table = &htable[HASH(child->pid)];
    if ((child->next = *table) != NULL)
        (*table)->prevp = &child->next;
    *table = child;
    child->prevp = table;
}

static inline void unhash_child(struct server_child_data *child)
{
    if (child->prevp) {
        if (child->next)
            child->next->prevp = child->prevp;
        *(child->prevp) = child->next;
    }
}

static struct server_child_data *resolve_child(struct server_child_data **table, pid_t pid)
{
    struct server_child_data *child;

    for (child = table[HASH(pid)]; child; child = child->next) {
        if (child->pid == pid)
            break;
    }

    return child;
}

/* initialize server_child structure */
server_child *server_child_alloc(const int connections, const int nforks)
{
    server_child *children;

    children = (server_child *) calloc(1, sizeof(server_child));
    if (!children)
        return NULL;

    children->nsessions = connections;
    children->nforks = nforks;
    children->fork = (void *) calloc(nforks, sizeof(server_child_fork));

    if (!children->fork) {
        free(children);
        return NULL;
    }

    return children;
}

/*!
 * add a child
 * @return pointer to struct server_child_data on success, NULL on error
 */
afp_child_t *server_child_add(server_child *children, int forkid, pid_t pid, uint ipc_fds[2])
{
    server_child_fork *fork;
    afp_child_t *child = NULL;
    sigset_t sig, oldsig;

    /* we need to prevent deletions from occuring before we get a
     * chance to add the child in. */
    sigemptyset(&sig);
    sigaddset(&sig, SIGCHLD);
    pthread_sigmask(SIG_BLOCK, &sig, &oldsig);

    /* it's possible that the child could have already died before the
     * pthread_sigmask. we need to check for this. */
    if (kill(pid, 0) < 0) {
        LOG(log_error, logtype_default, "server_child_add: no such process pid [%d]", pid);
        goto exit;
    }

    fork = (server_child_fork *) children->fork + forkid;

    /* if we already have an entry. just return. */
    if (child = resolve_child(fork->table, pid))
        goto exit;

    if ((child = calloc(1, sizeof(afp_child_t))) == NULL)
        goto exit;

    child->pid = pid;
    child->valid = 0;
    child->killed = 0;
    child->ipc_fds[0] = ipc_fds[0];
    child->ipc_fds[1] = ipc_fds[1];

    hash_child(fork->table, child);
    children->count++;

exit:
    pthread_sigmask(SIG_SETMASK, &oldsig, NULL);
    return child;
}

/* remove a child and free it */
int server_child_remove(server_child *children, const int forkid, pid_t pid)
{
    int fd;
    server_child_fork *fork;
    struct server_child_data *child;

    fork = (server_child_fork *) children->fork + forkid;
    if (!(child = resolve_child(fork->table, pid)))
        return -1;

    unhash_child(child);
    if (child->clientid) {
        free(child->clientid);
        child->clientid = NULL;
    }

    /* In main:child_handler() we need the fd in order to remove it from the pollfd set */
    fd = child->ipc_fds[0];
    if (child->ipc_fds[0] != -1) {
        close(child->ipc_fds[0]);
        child->ipc_fds[0] = -1;
    }
    if (child->ipc_fds[1] != -1) {
        close(child->ipc_fds[1]);
        child->ipc_fds[1] = -1;
    }

    free(child);
    children->count--;

    if (fork->cleanup)
        fork->cleanup(pid);

    return fd;
}

/* free everything: by using a hash table, this increases the cost of
 * this part over a linked list by the size of the hash table */
void server_child_free(server_child *children)
{
    server_child_fork *fork;
    struct server_child_data *child, *tmp;
    int i, j;

    for (i = 0; i < children->nforks; i++) {
        fork = (server_child_fork *) children->fork + i;
        for (j = 0; j < CHILD_HASHSIZE; j++) {
            child = fork->table[j]; /* start at the beginning */
            while (child) {
                tmp = child->next;
                if (child->clientid) {
                    free(child->clientid);
                }
                free(child);
                child = tmp;
            }
        }
    }
    free(children->fork);
    free(children);
}

/* send signal to all child processes */
void server_child_kill(server_child *children, int forkid, int sig)
{
    server_child_fork *fork;
    struct server_child_data *child, *tmp;
    int i;

    fork = (server_child_fork *) children->fork + forkid;
    for (i = 0; i < CHILD_HASHSIZE; i++) {
        child = fork->table[i];
        while (child) {
            tmp = child->next;
            kill(child->pid, sig);
            child = tmp;
        }
    }
}

/* send kill to a child processes.
 * a plain-old linked list
 * FIXME use resolve_child ?
 */
static int kill_child(struct server_child_data *child)
{
    if (!child->killed) {
        kill(child->pid, SIGTERM);
        /* we don't wait because there's no guarantee that we can really kill it */
        child->killed = 1;
        return 1;
    } else {
        LOG(log_info, logtype_default, "Unresponsive child[%d], sending SIGKILL", child->pid);
        kill(child->pid, SIGKILL);
    }
    return 1;
}

/*!
 * Try to find an old session and pass socket
 * @returns -1 on error, 0 if no matching session was found, 1 if session was found and socket passed
 */
int server_child_transfer_session(server_child *children,
                                  int forkid,
                                  pid_t pid,
                                  uid_t uid,
                                  int afp_socket,
                                  uint16_t DSI_requestID)
{
    EC_INIT;
    server_child_fork *fork;
    struct server_child_data *child;
    int i;

    fork = (server_child_fork *) children->fork + forkid;
    if ((child = resolve_child(fork->table, pid)) == NULL) {
        LOG(log_note, logtype_default, "Reconnect: no child[%u]", pid);
        if (kill(pid, 0) == 0) {
            LOG(log_note, logtype_default, "Reconnect: terminating old session[%u]", pid);
            kill(pid, SIGTERM);
            sleep(2);
            if (kill(pid, 0) == 0) {
                LOG(log_error, logtype_default, "Reconnect: killing old session[%u]", pid);
                kill(pid, SIGKILL);
                sleep(2);
            }
        }
        return 0;
    }

    if (!child->valid) {
        /* hmm, client 'guess' the pid, rogue? */
        LOG(log_error, logtype_default, "Reconnect: invalidated child[%u]", pid);
        return 0;
    } else if (child->uid != uid) {
        LOG(log_error, logtype_default, "Reconnect: child[%u] not the same user", pid);
        return 0;
    }

    LOG(log_note, logtype_default, "Reconnect: transfering session to child[%u]", pid);
    
    if (writet(child->ipc_fds[0], &DSI_requestID, 2, 0, 2) != 2) {
        LOG(log_error, logtype_default, "Reconnect: error sending DSI id to child[%u]", pid);
        EC_STATUS(-1);
        goto EC_CLEANUP;
    }
    EC_ZERO_LOG(send_fd(child->ipc_fds[0], afp_socket));
    EC_ZERO_LOG(kill(pid, SIGURG));

    EC_STATUS(1);

EC_CLEANUP:
    EC_EXIT;
}


/* see if there is a process for the same mac     */
/* if the times don't match mac has been rebooted */
void server_child_kill_one_by_id(server_child *children, int forkid, pid_t pid,
                                 uid_t uid, uint32_t idlen, char *id, uint32_t boottime)
{
    server_child_fork *fork;
    struct server_child_data *child, *tmp;
    int i;

    fork = (server_child_fork *)children->fork + forkid;

    for (i = 0; i < CHILD_HASHSIZE; i++) {
        child = fork->table[i];
        while (child) {
            tmp = child->next;
            if ( child->pid != pid) {
                if (child->idlen == idlen && memcmp(child->clientid, id, idlen) == 0) {
                    if ( child->time != boottime ) {
                        /* Client rebooted */
                        if (uid == child->uid) {
                            kill_child(child);
                            LOG(log_warning, logtype_default,
                                "Terminated disconnected child[%u], client rebooted.",
                                child->pid);
                        } else {
                            LOG(log_warning, logtype_default,
                                "Session with different pid[%u]", child->pid);
                        }
                    } else {
                        /* One client with multiple sessions */
                        LOG(log_debug, logtype_default,
                            "Found another session[%u] for client[%u]", child->pid, pid);
                    }
                }
            } else {
                /* update childs own slot */
                child->time = boottime;
                if (child->clientid)
                    free(child->clientid);
                LOG(log_debug, logtype_default, "Setting client ID for %u", child->pid);
                child->uid = uid;
                child->valid = 1;
                child->idlen = idlen;
                child->clientid = id;
            }
            child = tmp;
        }
    }
}

/* for extra cleanup if necessary */
void server_child_setup(server_child *children, const int forkid,
                        void (*fcn)(const pid_t))
{
    server_child_fork *fork;

    fork = (server_child_fork *) children->fork + forkid;
    fork->cleanup = fcn;
}


/* ---------------------------
 * reset children signals
 */
void server_reset_signal(void)
{
    struct sigaction    sv;
    sigset_t            sigs;
    const struct itimerval none = {{0, 0}, {0, 0}};

    setitimer(ITIMER_REAL, &none, NULL);
    memset(&sv, 0, sizeof(sv));
    sv.sa_handler =  SIG_DFL;
    sigemptyset( &sv.sa_mask );

    sigaction(SIGALRM, &sv, NULL );
    sigaction(SIGHUP,  &sv, NULL );
    sigaction(SIGTERM, &sv, NULL );
    sigaction(SIGUSR1, &sv, NULL );
    sigaction(SIGCHLD, &sv, NULL );

    sigemptyset(&sigs);
    sigaddset(&sigs, SIGALRM);
    sigaddset(&sigs, SIGHUP);
    sigaddset(&sigs, SIGUSR1);
    sigaddset(&sigs, SIGCHLD);
    pthread_sigmask(SIG_UNBLOCK, &sigs, NULL);

}
