/*
 * $Id: asp_getsess.c,v 1.4 2001-09-06 19:04:40 rufustfirefly Exp $
 *
 * Copyright (c) 1990,1996 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <syslog.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/uio.h>

/* POSIX.1 sys/wait.h check */
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif /* ! WEXITSTATUS */
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif /* ! WIFEXITED */

#include <sys/socket.h>
#include <sys/param.h>
#include <netatalk/at.h>
#include <atalk/compat.h>
#include <atalk/atp.h>
#include <atalk/asp.h>

#include <atalk/server_child.h>
#include "asp_child.h"

static ASP server_asp;
static struct server_child *children = NULL;
static struct asp_child    **asp_ac = NULL;

/* send tickles and check tickle status of connections
 * thoughts on using a hashed list:
 * + child_cleanup, finding slots 
 * - tickle_handler, freeing, tickles 
 * if setup for a large number of connections,
 * + space: if actual connections < potential
 * - space: actual connections ~ potential
 */
static void tickle_handler()
{
  int sid;
  
  /* check status */
  for (sid = 0; sid < children->nsessions; sid++) {
    if (asp_ac[sid] == NULL || asp_ac[sid]->ac_state == ACSTATE_DEAD) 
      continue;
    
    if (++asp_ac[sid]->ac_state >= ACSTATE_BAD) {
      /* kill. if already dead, just continue */
      if (kill( asp_ac[ sid ]->ac_pid, SIGTERM) == 0)
	syslog( LOG_INFO, "asp_alrm: %d timed out",
		asp_ac[ sid ]->ac_pid );

      asp_ac[ sid ]->ac_state = ACSTATE_DEAD;
      continue;
    }

    /* send off a tickle */
    asp_tickle(server_asp, sid, &asp_ac[sid]->ac_sat);
  }
}

static void child_cleanup(const pid_t pid)
{
  int i;

  for (i = 0; i < children->nsessions; i++)
    if (asp_ac[i] && (asp_ac[i]->ac_pid == pid)) {
      asp_ac[i]->ac_state = ACSTATE_DEAD;
      break;
    }
}


/* kill children */
void asp_kill(int sig)
{
  if (children) 
    server_child_kill(children, CHILD_ASPFORK, sig);
}


/*
 * This call handles open, tickle, and getstatus requests. On a
 * successful open, it forks a child process. 
 * It returns an ASP to the child and parent and NULL if there is
 * an error.
 */
ASP asp_getsession(ASP asp, server_child *server_children, 
		   const int tickleval)
{
    struct sigaction action;
    struct itimerval timer;
    struct sockaddr_at	sat;
    struct atp_block	atpb;
    ATP                 atp;
    struct iovec	iov[ 8 ];
    pid_t               pid;
    int			i, sid;
    u_int16_t		asperr;

    if (!asp->inited) {
      if (!(children = server_children))
	return NULL;

      if ((asp_ac = (struct asp_child **) 
	   calloc(server_children->nsessions, sizeof(struct asp_child *)))
	   == NULL)
	return NULL;

      server_asp = asp;

      /* install cleanup pointer */
      server_child_setup(children, CHILD_ASPFORK, child_cleanup);

      /* install tickle handler */
      memset(&action, 0, sizeof(action));
      action.sa_handler = tickle_handler;
      sigemptyset(&action.sa_mask);
      action.sa_flags = SA_RESTART;

      timer.it_interval.tv_sec = timer.it_value.tv_sec = tickleval;
      timer.it_interval.tv_usec = timer.it_value.tv_usec = 0;
      if ((sigaction(SIGALRM, &action, NULL) < 0) ||
	  (setitimer(ITIMER_REAL, &timer, NULL) < 0)) {
	free(asp_ac);
	return NULL;
      }

      asp->inited = 1;
    }
		    
    memset( &sat, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
    sat.sat_len = sizeof( struct sockaddr_at );
#endif /* BSD4_4 */
    sat.sat_family = AF_APPLETALK;
    sat.sat_addr.s_net = ATADDR_ANYNET;
    sat.sat_addr.s_node = ATADDR_ANYNODE;
    sat.sat_port = ATADDR_ANYPORT;
    atpb.atp_saddr = &sat;
    atpb.atp_rreqdata = asp->cmdbuf;
    atpb.atp_rreqdlen = sizeof( asp->cmdbuf );
    while ( atp_rreq( asp->asp_atp, &atpb ) < 0 ) {
      if ( errno == EINTR || errno == EAGAIN ) {
	continue;
      }
      return( NULL );
    }
    
    switch ( asp->cmdbuf[ 0 ] ) {
    case ASPFUNC_TICKLE:
      sid = asp->cmdbuf[1];
      if ((asp_ac[sid] != NULL) && (asp_ac[sid]->ac_state != ACSTATE_DEAD))
	asp_ac[sid]->ac_state = ACSTATE_OK;
      break;

    case ASPFUNC_STAT :
#ifdef EBUG
      printf( "asp stat\n" );
#endif /* EBUG */
      if ( asp->asp_slen > 0 ) {
	asp->cmdbuf[0] = 0;
	memcpy( asp->cmdbuf + 4, asp->asp_status, asp->asp_slen );
	iov[ 0 ].iov_base = asp->cmdbuf;
	iov[ 0 ].iov_len = 4 + asp->asp_slen;
	atpb.atp_sresiov = iov;
	atpb.atp_sresiovcnt = 1;
	atp_sresp( asp->asp_atp, &atpb );
      }
      break;

    case ASPFUNC_OPEN :
      if (children->count < children->nsessions) {

	/* find a slot */
	for (sid = 0; sid < children->nsessions; sid++) {
	  if (asp_ac[sid] == NULL)
	    break;

	  if (asp_ac[sid]->ac_state == ACSTATE_DEAD) {
	    free(asp_ac[sid]);
	    asp_ac[sid] = NULL;
	    break;
	  }
	}

	if ((atp = atp_open(ATADDR_ANYPORT, 
			    &(atp_sockaddr(asp->asp_atp)->sat_addr))) == NULL) 
	  return NULL;

	switch ((pid = fork())) {
	case 0 : /* child */
	  signal(SIGTERM, SIG_DFL);
	  signal(SIGHUP, SIG_DFL);
	  /* free/close some things */
	  for (i = 0; i < children->nsessions; i++ ) {
	    if ( asp_ac[i] != NULL )
	      free( asp_ac[i] );
	  }
	  free(asp_ac);
	  
	  server_child_free(children);
	  children = NULL;
	  atp_close(asp->asp_atp);

	  asp->child = 1;
	  asp->asp_atp = atp;
	  asp->asp_sat = sat;
	  asp->asp_wss = asp->cmdbuf[1];
	  asp->asp_seq = 0;
	  asp->asp_sid = sid;
	  asp->asp_flags = ASPFL_SSS;
	  return asp;
	  
	case -1 : /* error */
	  asp->cmdbuf[ 0 ] = 0;
	  asp->cmdbuf[ 1 ] = 0;
	  asperr = ASPERR_SERVBUSY;
	  break;
	  
	default : /* parent process */
	  switch (server_child_add(children, CHILD_ASPFORK, pid)) {
	  case 0: /* added child */
	    if ((asp_ac[sid] = (struct asp_child *) 
		 malloc(sizeof(struct asp_child)))) {
	      asp_ac[sid]->ac_pid = pid;
	      asp_ac[sid]->ac_state = ACSTATE_OK;
	      asp_ac[sid]->ac_sat = sat;
	      asp_ac[sid]->ac_sat.sat_port = asp->cmdbuf[1];
	      
	      asp->cmdbuf[0] = atp_sockaddr(atp)->sat_port;
	      asp->cmdbuf[1] = sid;
	      asperr = ASPERR_OK;
	      break;
	    } /* fall through if malloc fails */
	  case -1: /* bad error */
	    kill(pid, SIGQUIT); 
	    break;
	  default: /* non-fatal error */
	    break;
	  }
	  atp_close(atp);
	  break;
	}
	
      } else {
	asp->cmdbuf[0] = asp->cmdbuf[1] = 0;
	asperr = ASPERR_SERVBUSY;
      }

      memcpy( asp->cmdbuf + 2, &asperr, sizeof(asperr));
      iov[ 0 ].iov_base = asp->cmdbuf;
      iov[ 0 ].iov_len = 4;
      atpb.atp_sresiov = iov;
      atpb.atp_sresiovcnt = 1;
      atp_sresp( asp->asp_atp, &atpb );
      break;

    default:
      syslog(LOG_INFO, "ASPUnknown %d", asp->cmdbuf[0]);
      break;
    }

    return asp;
}
