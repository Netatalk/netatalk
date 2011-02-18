/*
 * $Id: asp_getsess.c,v 1.9 2009-10-13 22:55:37 didg Exp $
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
#include <errno.h>
#include <signal.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/param.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif /* HAVE_SYS_WAIT_H */

#include <netatalk/at.h>
#include <atalk/logger.h>
#include <atalk/compat.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/server_child.h>

#include "asp_child.h"

#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif /* ! WEXITSTATUS */
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif /* ! WIFEXITED */

#ifndef MIN
#define MIN(a,b)     ((a)<(b)?(a):(b))
#endif /* ! MIN */

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
static void tickle_handler(int sig _U_)
{
  int sid;
  
  /* check status */
  for (sid = 0; sid < children->nsessions; sid++) {
    if (asp_ac[sid] == NULL || asp_ac[sid]->ac_state == ACSTATE_DEAD) 
      continue;
    
    if (++asp_ac[sid]->ac_state >= ACSTATE_BAD) {
      /* kill. if already dead, just continue */
      if (kill( asp_ac[ sid ]->ac_pid, SIGTERM) == 0)
	LOG(log_info, logtype_default, "asp_alrm: %d timed out",
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

void asp_stop_tickle(void)
{
    if (server_asp && server_asp->inited) {
    	static const struct itimerval timer = {{0, 0}, {0, 0}};
	
	setitimer(ITIMER_REAL, &timer, NULL);
    }
}

/*
 * This call handles open, tickle, and getstatus requests. On a
 * successful open, it forks a child process. 
 * It returns an ASP to the child and parent and NULL if there is
 * an error.
 */
static void set_asp_ac(int sid, struct asp_child *tmp);

ASP asp_getsession(ASP asp, server_child *server_children, 
		   const int tickleval)
{
    struct sigaction    action;
    struct itimerval    timer;
    struct sockaddr_at  sat;
    struct atp_block    atpb;
    ATP                 atp;
    struct iovec        iov[ 8 ];
    pid_t               pid;
    int                 i, iovcnt, sid;
    u_int16_t           asperr;
    char                *buf;
    int                 buflen;

    if (!asp->inited) {
      if (!(children = server_children))
	return NULL;

      /* only calloc once */
      if (!asp_ac && (asp_ac = (struct asp_child **) 
	   calloc(server_children->nsessions, sizeof(struct asp_child *)))
	   == NULL)
	return NULL;

      server_asp = asp;

      /* install cleanup pointer */
      server_child_setup(children, CHILD_ASPFORK, child_cleanup);

      /* install tickle handler 
       * we are the parent process
       */
      memset(&action, 0, sizeof(action));
      action.sa_handler = tickle_handler;
      sigemptyset(&action.sa_mask);
      sigaddset(&action.sa_mask, SIGHUP);
      sigaddset(&action.sa_mask, SIGTERM);
      sigaddset(&action.sa_mask, SIGCHLD);
      action.sa_flags = SA_RESTART;

      timer.it_interval.tv_sec = timer.it_value.tv_sec = tickleval;
      timer.it_interval.tv_usec = timer.it_value.tv_usec = 0;
      if ((sigaction(SIGALRM, &action, NULL) < 0) ||
	  (setitimer(ITIMER_REAL, &timer, NULL) < 0)) {
	free(asp_ac);
	server_asp = NULL;
	asp_ac = NULL;
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
        i = 0;
        while(atpb.atp_bitmap) {
            i++;
            atpb.atp_bitmap >>= 1;
        }

	/* asp->data is big enough ... */
        memcpy( asp->data, asp->asp_status, MIN(asp->asp_slen, i*ASP_CMDSIZ));
	
        buflen = MIN(asp->asp_slen, i*ASP_CMDSIZ);
        buf = asp->data;
        iovcnt = 0;

        /* If status information is too big to fit into the available
         * ASP packets, we simply send as much as we can.
         * Older client versions will most likely not be able to use
         * the additional information anyway, like directory services
         * or UTF8 server name. A very long fqdn could be a problem,
         * we could end up with an invalid address list.
         */
        do {
            iov[ iovcnt ].iov_base = buf;
            memmove(buf + ASP_HDRSIZ, buf, buflen);
            memset( iov[ iovcnt ].iov_base, 0, ASP_HDRSIZ );

           if ( buflen > ASP_CMDSIZ ) {
                buf += ASP_CMDMAXSIZ;
                buflen -= ASP_CMDSIZ;
                iov[ iovcnt ].iov_len = ASP_CMDMAXSIZ;
            } else {
                iov[ iovcnt ].iov_len = buflen + ASP_HDRSIZ;
                buflen = 0;
            }
            iovcnt++;
        } while ( iovcnt < i && buflen > 0 );

        atpb.atp_sresiovcnt = iovcnt;
        atpb.atp_sresiov = iov;
        atp_sresp( asp->asp_atp, &atpb );
      }
      break;

    case ASPFUNC_OPEN :
      if (children->count < children->nsessions) {
      struct asp_child    *asp_ac_tmp;

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

    int dummy[2];
	switch ((pid = fork())) {
	case 0 : /* child */
	  server_reset_signal();
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
	  /* we need atomic setting or pb with tickle_handler */ 
      if (server_child_add(children, CHILD_ASPFORK, pid, dummy)) {
	    if ((asp_ac_tmp = malloc(sizeof(struct asp_child))) == NULL) {
            kill(pid, SIGQUIT); 
            break;
        }
        asp_ac_tmp->ac_pid = pid;
        asp_ac_tmp->ac_state = ACSTATE_OK;
        asp_ac_tmp->ac_sat = sat;
        asp_ac_tmp->ac_sat.sat_port = asp->cmdbuf[1];
	    
        asp->cmdbuf[0] = atp_sockaddr(atp)->sat_port;
        asp->cmdbuf[1] = sid;
        set_asp_ac(sid, asp_ac_tmp);
        asperr = ASPERR_OK;
        break;
      } else {
	    kill(pid, SIGQUIT); 
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
      LOG(log_info, logtype_default, "ASPUnknown %d", asp->cmdbuf[0]);
      break;
    }

    return asp;
}

/* with fn defined after use, assume it's not optimized by the compiler */
static void set_asp_ac(int sid, struct asp_child *tmp)
{
    asp_ac[sid] = tmp;
}
