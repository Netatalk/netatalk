/*
 * Copyright (c) 1990,1996 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netatalk/at.h>
#include <atalk/compat.h>
#include <atalk/atp.h>
#include <atalk/pap.h>

#include <atalk/server_child.h>
#include "pap_child.h"

static PAP server_pap;
static struct server_child *children = NULL;
static struct pap_child    **pap_ac = NULL;

/* send tickles and check tickle status of connections */
static void tickle_handler()
{
  int sid;
  
  /* check status */
  for (sid = 0; sid < children->nsessions; sid++) {
    if (pap_ac[sid] == NULL || pap_ac[sid]->ac_state == ACSTATE_DEAD) 
      continue;
    
    if (++pap_ac[sid]->ac_state >= ACSTATE_BAD) {
      /* kill. if already dead, just continue */
      if (kill( pap_ac[ sid ]->ac_pid, SIGTERM) == 0)
	syslog( LOG_INFO, "pap_alrm: %d timed out",
		pap_ac[ sid ]->ac_pid );

      pap_ac[ sid ]->ac_state = ACSTATE_DEAD;
      continue;
    }

    /* send off a tickle */
    pap_tickle(server_pap, sid, &pap_ac[sid]->ac_sat);
  }
}

static void child_cleanup(const pid_t pid)
{
  int i;

  for (i = 0; i < children->nsessions; i++)
    if (pap_ac[i] && (pap_ac[i]->ac_pid == pid)) {
      pap_ac[i]->ac_state = ACSTATE_DEAD;
      break;
    }
}


/* kill children */
void pap_kill(int sig)
{
  if (children) 
    server_child_kill(children, CHILD_PAPFORK, sig);
}


/*
 * This call handles open, tickle, and getstatus requests. On a
 * successful open, it forks a child process. 
 * It returns an PAP to the child and parent and NULL if there is
 * an error.
 */
PAP pap_slinit(PAP pap, server_child *server_children, 
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
    u_short		paperr;

    if (!pap->inited) {
      if (!(children = server_children))
	return NULL;

      if ((pap_ac = (struct pap_child **) 
	   calloc(server_children->nsessions, sizeof(struct pap_child *)))
	   == NULL)
	return NULL;

      server_pap = pap;

      /* install cleanup pointer */
      server_child_setup(children, CHILD_PAPFORK, child_cleanup);

      /* install tickle handler */
      action.sa_handler = tickle_handler;
      sigemptyset(&action.sa_mask);
      action.sa_flags = SA_RESTART;

      timer.it_interval.tv_sec = timer.it_value.tv_sec = tickleval;
      timer.it_interval.tv_usec = timer.it_value.tv_usec = 0;
      if ((setitimer(ITIMER_REAL, &timer, NULL) < 0) ||
	  (sigaction(SIGALRM, &action, NULL) < 0)) {
	exit(1);
      }

      pap->inited = 1;
    }
		    
    memset( &sat, 0, sizeof( struct sockaddr_at ));
#ifdef BSD4_4
    sat.sat_len = sizeof( struct sockaddr_at );
#endif BSD4_4
    sat.sat_family = AF_APPLETALK;
    sat.sat_addr.s_net = ATADDR_ANYNET;
    sat.sat_addr.s_node = ATADDR_ANYNODE;
    sat.sat_port = ATADDR_ANYPORT;
    atpb.atp_saddr = &sat;
    atpb.atp_rreqdata = pap->cmdbuf;
    atpb.atp_rreqdlen = sizeof( pap->cmdbuf );
    while ( atp_rreq( pap->pap_atp, &atpb ) < 0 ) {
      if ( errno == EINTR || errno == EAGAIN ) {
	continue;
      }
      return( NULL );
    }
    
    switch ( pap->cmdbuf[ 0 ] ) {
    case PAPFUNC_TICKLE:
      sid = pap->cmdbuf[1];
      if ((pap_ac[sid] != NULL) && (pap_ac[sid]->ac_state != ACSTATE_DEAD))
	pap_ac[sid]->ac_state = ACSTATE_OK;
      break;

    case PAPFUNC_STAT:
#ifdef EBUG
      printf( "pap stat\n" );
#endif EBUG
      if ( pap->pap_slen > 0 ) {
	pap->cmdbuf[0] = 0;
	bcopy( pap->pap_status, pap->cmdbuf + 4, pap->pap_slen );
	iov[ 0 ].iov_base = pap->cmdbuf;
	iov[ 0 ].iov_len = 4 + pap->pap_slen;
	atpb.atp_sresiov = iov;
	atpb.atp_sresiovcnt = 1;
	atp_sresp( pap->pap_atp, &atpb );
      }
      break;

    case PAPFUNC_OPEN :
      if (children->count < children->nsessions) {

	/* find a slot */
	for (sid = 0; sid < children->nsessions; sid++) {
	  if (pap_ac[sid] == NULL)
	    break;

	  if (pap_ac[sid]->ac_state == ACSTATE_DEAD) {
	    free(pap_ac[sid]);
	    pap_ac[sid] = NULL;
	    break;
	  }
	}

	if ((atp = atp_open(0)) == NULL) 
	  return NULL;

	switch (pid = fork()) {
	case 0 : /* child */
	  signal(SIGTERM, SIG_DFL);
	  signal(SIGHUP, SIG_DFL);
	  /* free/close some things */
	  for (i = 0; i < children->nsessions; i++ ) {
	    if ( pap_ac[i] != NULL )
	      free( pap_ac[i] );
	  }
	  free(pap_ac);
	  
	  server_child_free(children);
	  children = NULL;
	  atp_close(pap->pap_atp);

	  pap->child = 1;
	  pap->pap_atp = atp;
	  pap->pap_sat = sat;
	  pap->pap_wss = pap->cmdbuf[1];
	  pap->pap_seq = 0;
	  pap->pap_sid = sid;
	  pap->pap_flags = PAPFL_SSS;
	  return pap;
	  
	case -1 : /* error */
	  pap->cmdbuf[ 0 ] = 0;
	  pap->cmdbuf[ 1 ] = 0;
	  paperr = PAPERR_SERVBUSY;
	  break;
	  
	default : /* parent process */
	  switch (server_child_add(children, CHILD_PAPFORK, pid)) {
	  case 0: /* added child */
	    if (pap_ac[sid] = (struct pap_child *) 
		malloc(sizeof(struct pap_child))) {
	      pap_ac[sid]->ac_pid = pid;
	      pap_ac[sid]->ac_state = ACSTATE_OK;
	      pap_ac[sid]->ac_sat = sat;
	      pap_ac[sid]->ac_sat.sat_port = pap->cmdbuf[1];
	      
	      pap->cmdbuf[0] = atp_sockaddr(atp)->sat_port;
	      pap->cmdbuf[1] = sid;
	      paperr = PAPERR_OK;
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
	pap->cmdbuf[0] = pap->cmdbuf[1] = 0;
	paperr = PAPERR_SERVBUSY;
      }

      bcopy( &paperr, pap->cmdbuf + 2, sizeof( u_short ));
      iov[ 0 ].iov_base = pap->cmdbuf;
      iov[ 0 ].iov_len = 4;
      atpb.atp_sresiov = iov;
      atpb.atp_sresiovcnt = 1;
      atp_sresp( pap->pap_atp, &atpb );
      break;

    default:
      syslog(LOG_INFO, "PAPUnknown %d", pap->cmdbuf[0]);
      break;
    }

    return pap;
}
