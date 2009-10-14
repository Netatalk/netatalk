/*
 * $Id: server_child.c,v 1.11 2009-10-14 02:24:05 didg Exp $
 *
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
#include <atalk/logger.h>

/* POSIX.1 sys/wait.h check */
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#include <sys/time.h>

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

#include <atalk/server_child.h>

/* hash/child functions: hash OR's pid */
#define CHILD_HASHSIZE 32
#define HASH(i) ((((i) >> 8) ^ (i)) & (CHILD_HASHSIZE - 1))

struct server_child_data {
  pid_t     pid; 		/* afpd worker process pid (from the worker afpd process )*/
  uid_t     uid;		/* user id of connected client (from the worker afpd process) */
  int       valid;		/* 1 if we have a clientid */
  u_int32_t time;		/* client boot time (from the mac client) */
  int       killed;		/* 1 if we already tried to kill the client */

  u_int32_t idlen;		/* clientid len (from the Mac client) */
  char *clientid;		/* clientid (from the Mac client) */
  struct server_child_data **prevp, *next;
};

typedef struct server_child_fork {
  struct server_child_data *table[CHILD_HASHSIZE];
  void (*cleanup)(const pid_t);
} server_child_fork;

 
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

static inline struct server_child_data 
*resolve_child(struct server_child_data **table, const pid_t pid)
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

/* add a child in. return 0 on success, -1 on serious error, and
 * > 0 for a non-serious error. */
int server_child_add(server_child *children, const int forkid,
		     const pid_t pid)
{
  server_child_fork *fork;
  struct server_child_data *child;
  sigset_t sig, oldsig;

  /* we need to prevent deletions from occuring before we get a 
   * chance to add the child in. */
  sigemptyset(&sig);
  sigaddset(&sig, SIGCHLD);
  sigprocmask(SIG_BLOCK, &sig, &oldsig);

  /* it's possible that the child could have already died before the
   * sigprocmask. we need to check for this. */
  if (kill(pid, 0) < 0) {
    sigprocmask(SIG_SETMASK, &oldsig, NULL);
    return 1;
  }

  fork = (server_child_fork *) children->fork + forkid;

  /* if we already have an entry. just return. */
  if (resolve_child(fork->table, pid)) {
    sigprocmask(SIG_SETMASK, &oldsig, NULL);
    return 0;
  }

  if ((child = (struct server_child_data *) 
       calloc(1, sizeof(struct server_child_data))) == NULL) {
    sigprocmask(SIG_SETMASK, &oldsig, NULL);
    return -1;
  }

  child->pid = pid;
  child->valid = 0;
  child->killed = 0;
  hash_child(fork->table, child);
  children->count++;
  sigprocmask(SIG_SETMASK, &oldsig, NULL);

  return 0;
}

/* remove a child and free it */
int server_child_remove(server_child *children, const int forkid,
			const pid_t pid)
{
  server_child_fork *fork;
  struct server_child_data *child;

  fork = (server_child_fork *) children->fork + forkid;
  if (!(child = resolve_child(fork->table, pid)))
    return 0;
  
  unhash_child(child);
  if (child->clientid) {
      free(child->clientid);
  }
  free(child);
  children->count--;
  return 1;
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

/* send kill to child processes: this also has an increased cost over
 * a plain-old linked list */
void server_child_kill(server_child *children, const int forkid,
		       const int sig)
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
  }
  else {
     LOG(log_info, logtype_default, "Already tried to kill (%d) before! Still there?",  child->pid);
  }
  return 0;
}

/* -------------------- */
void server_child_kill_one(server_child *children, const int forkid, const pid_t pid, const uid_t uid)
{
  server_child_fork *fork;
  struct server_child_data *child, *tmp;
  int i;

  fork = (server_child_fork *) children->fork + forkid;
  for (i = 0; i < CHILD_HASHSIZE; i++) {
    child = fork->table[i];
    while (child) {
      tmp = child->next;
      if (child->pid == pid) {
          if (!child->valid) {
             /* hmm, client 'guess' the pid, rogue? */
             LOG(log_info, logtype_default, "Disconnecting old session (%d) no token, bailout!",  child->pid);
          }
          else if (child->uid != uid) {
             LOG(log_info, logtype_default, "Disconnecting old session (%d) not the same user, bailout!",  child->pid);
          }
          else {
              kill_child(child);
          }
      }
      child = tmp;
    }
  }
}


/* see if there is a process for the same mac     */
/* if the times don't match mac has been rebooted */
void server_child_kill_one_by_id(server_child *children, const int forkid, const pid_t pid,
          const uid_t uid, 
          const u_int32_t idlen, char *id, u_int32_t boottime)
{
  server_child_fork *fork;
  struct server_child_data *child, *tmp;
  int i;

  fork = (server_child_fork *) children->fork + forkid;
  for (i = 0; i < CHILD_HASHSIZE; i++) {
    child = fork->table[i];
    while (child) {
      tmp = child->next;
      if ( child->pid != pid) {
          if ( child->idlen == idlen && !memcmp(child->clientid, id, idlen)) {
	     if ( child->time != boottime ) {
	          if (uid == child->uid) {
	              if (kill_child(child)) {
		          LOG(log_info, logtype_default, "Disconnecting old session %d, client rebooted.",  child->pid);
		      }
		  }
		  else {
		      LOG(log_info, logtype_default, "Disconnecting old session not the same uid, bailout!");
		  }
	     }
	     else if (child->killed) {
	          /* there's case where a Mac close a connection and restart a new one before
	           * the first is 'waited' by the master afpd process
	          */
		  LOG(log_info, logtype_default, 
		      "WARNING: connection (%d) killed but still there.", child->pid);
	     }
	     else {
		  LOG(log_info, logtype_default, 
		      "WARNING: 2 connections (%d, %d), boottime identical, don't know if one needs to be disconnected.",
		       child->pid, pid);
	     } 
		
	  }
      }
      else 
      {
	  child->time = boottime;
	  /* free old token if any */
	  if (child->clientid) {
	      free(child->clientid);
	  }
          child->uid = uid; 
          child->valid = 1;
	  child->idlen = idlen;
          child->clientid = id;
	  LOG(log_info, logtype_default, "Setting clientid (len %d) for %d, boottime %X", idlen, child->pid, boottime);
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


/* keep track of children. */
void server_child_handler(server_child *children)
{
  int status, i;
  pid_t pid;
  
#ifndef WAIT_ANY
#define WAIT_ANY (-1)
#endif /* ! WAIT_ANY */

  while ((pid = waitpid(WAIT_ANY, &status, WNOHANG)) > 0) {
    for (i = 0; i < children->nforks; i++) {
      if (server_child_remove(children, i, pid)) {
        server_child_fork *fork;
        
	fork = (server_child_fork *) children->fork + i;
	if (fork->cleanup)
	  fork->cleanup(pid);
	break;
      }
    }

    if (WIFEXITED(status)) {
      if (WEXITSTATUS(status)) {
	LOG(log_info, logtype_default, "server_child[%d] %d exited %d", i, pid, 
	       WEXITSTATUS(status));
      } else {
	LOG(log_info, logtype_default, "server_child[%d] %d done", i, pid);
      }
    } else {
      if (WIFSIGNALED(status))
      { 
	LOG(log_info, logtype_default, "server_child[%d] %d killed by signal %d", i, pid,  
	       WTERMSIG (status));
      }
      else
      {
	LOG(log_info, logtype_default, "server_child[%d] %d died", i, pid);
      }
    }
  }
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
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
        
}
