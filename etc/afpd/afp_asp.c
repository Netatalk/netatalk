/* 
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 *
 * modified from main.c. this handles afp over asp. 
 */

#ifndef NO_DDP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <netatalk/endian.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/compat.h>
#include <atalk/util.h>

#include "globals.h"
#include "switch.h"
#include "auth.h"
#include "fork.h"

extern struct oforks	*writtenfork;

/* for CAP style authenticated printing */
#ifdef CAPDIR
extern int addr_net, addr_node, addr_uid;
#endif /* CAPDIR */

static AFPObj *child;

static __inline__ void afp_asp_close(AFPObj *obj)
{
    ASP asp = obj->handle;
    
    if (obj->logout)
      (*obj->logout)();

    asp_close( asp );
    syslog(LOG_INFO, "%.2fKB read, %.2fKB written",
	   asp->read_count / 1024.0, asp->write_count / 1024.0);
}

static void afp_asp_die(const int sig)
{
    ASP asp = child->handle;

    asp_attention(asp, AFPATTN_SHUTDOWN);
    if ( asp_shutdown( asp ) < 0 ) {
	syslog( LOG_ERR, "afp_die: asp_shutdown: %m" );
    }

    afp_asp_close(child);
    if (sig == SIGTERM || sig == SIGALRM)
      exit( 0 );
    else
      exit(sig);
}

static void afp_asp_timedown()
{
    struct sigaction	sv;
    struct itimerval	it;

    /* shutdown and don't reconnect. server going down in 5 minutes. */
    asp_attention(child->handle, AFPATTN_SHUTDOWN | AFPATTN_NORECONNECT |
		  AFPATTN_TIME(5));

    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 300;
    it.it_value.tv_usec = 0;
    if ( setitimer( ITIMER_REAL, &it, 0 ) < 0 ) {
	syslog( LOG_ERR, "afp_timedown: setitimer: %m" );
	afp_asp_die(1);
    }

    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = afp_asp_die;
    sigemptyset( &sv.sa_mask );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGALRM, &sv, 0 ) < 0 ) {
	syslog( LOG_ERR, "afp_timedown: sigaction: %m" );
	afp_asp_die(1);
    }
}

void afp_over_asp(AFPObj *obj)
{
    ASP asp;
    struct sigaction  action;
    int		func, ccnt = 0, reply = 0;

#ifdef CAPDIR
    char addr_filename[256];
    struct stat cap_st;
#endif /* CAPDIR */

    obj->exit = afp_asp_die;
    obj->reply = (int (*)()) asp_cmdreply;
    obj->attention = (int (*)(void *, AFPUserBytes)) asp_attention;
    child = obj;
    asp = (ASP) obj->handle;

    /* install signal handlers */
    memset(&action, 0, sizeof(action));
    action.sa_handler = afp_asp_timedown;
    sigemptyset( &action.sa_mask );
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGHUP, &action, 0 ) < 0 ) {
	syslog( LOG_ERR, "afp_over_asp: sigaction: %m" );
	afp_asp_die(1);
    }

    action.sa_handler = afp_asp_die;
    sigemptyset( &action.sa_mask );
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGTERM, &action, 0 ) < 0 ) {
	syslog( LOG_ERR, "afp_over_asp: sigaction: %m" );
	afp_asp_die(1);
    }

#ifdef CAPDIR
    addr_net = ntohs( asp->asp_sat.sat_addr.s_net );
    addr_node  = asp->asp_sat.sat_addr.s_node;
#endif /* CAPDIR */

    syslog( LOG_INFO, "session from %u.%u:%u on %u.%u:%u",
	    ntohs( asp->asp_sat.sat_addr.s_net ),
	    asp->asp_sat.sat_addr.s_node, asp->asp_sat.sat_port,
	    ntohs( atp_sockaddr( asp->asp_atp )->sat_addr.s_net ),
	    atp_sockaddr( asp->asp_atp )->sat_addr.s_node,
	    atp_sockaddr( asp->asp_atp )->sat_port );

    while ((reply = asp_getrequest(asp))) {
      switch (reply) {
      case ASPFUNC_CLOSE :
	afp_asp_close(obj);
	syslog( LOG_INFO, "done" );

#ifdef CAPDIR
	sprintf(addr_filename, "%s/net%d.%dnode%d", CAPDIR, addr_net/256, addr_net%256, addr_node);
	if(stat(addr_filename, &cap_st) == 0) {
		if(unlink(addr_filename) == 0) {
			syslog(LOG_INFO, "removed %s", addr_filename);
		} else {
			syslog(LOG_INFO, "error removing %s: %m", addr_filename);
		}
	} else {
		syslog(LOG_INFO, "error stat'ing %s: %m", addr_filename);
	}
#endif /* CAPDIR */

	if ( obj->options.flags & OPTION_DEBUG ) {
	  printf( "done\n" );
	}
	return;
	break;

      case ASPFUNC_CMD :
#ifdef AFS
	if ( writtenfork ) {
	  if ( flushfork( writtenfork ) < 0 ) {
	    syslog( LOG_ERR, "main flushfork: %m" );
	  }
	  writtenfork = NULL;
	}
#endif AFS
	func = (u_char) asp->commands[0];
	if ( obj->options.flags & OPTION_DEBUG ) {
	  printf( "command: %d\n", func );
	  bprint( asp->commands, asp->cmdlen );
	}
	if ( afp_switch[ func ] != NULL ) {
	  /*
	   * The function called from afp_switch is expected to
	   * read its parameters out of buf, put its
	   * results in replybuf (updating rbuflen), and
	   * return an error code.
		 */
	  asp->datalen = ASP_DATASIZ;
	  reply = (*afp_switch[ func ])(obj,
					asp->commands, asp->cmdlen,
					asp->data, &asp->datalen);
	} else {
	  syslog( LOG_ERR, "bad function %X", func );
	  asp->datalen = 0;
	  reply = AFPERR_NOOP;
	}
	if ( obj->options.flags & OPTION_DEBUG ) {
	  printf( "reply: %d, %d\n", reply, ccnt++ );
	  bprint( asp->data, asp->datalen );
	}

	if ( asp_cmdreply( asp, reply ) < 0 ) {
	  syslog( LOG_ERR, "asp_cmdreply: %m" );
	  afp_asp_die(1);
	}
	break;

      case ASPFUNC_WRITE :
	func = (u_char) asp->commands[0];
	if ( obj->options.flags & OPTION_DEBUG ) {
	  printf( "(write) command: %d\n", func );
	  bprint( asp->commands, asp->cmdlen );
	}
	if ( afp_switch[ func ] != NULL ) {
	  asp->datalen = ASP_DATASIZ;
	  reply = (*afp_switch[ func ])(obj, 
					asp->commands, asp->cmdlen,
					asp->data, &asp->datalen);
	} else {
	  syslog( LOG_ERR, "(write) bad function %X", func );
	  asp->datalen = 0;
	  reply = AFPERR_NOOP;
	}
	if ( obj->options.flags & OPTION_DEBUG ) {
	  printf( "(write) reply code: %d, %d\n", reply, ccnt++ );
	  bprint( asp->data, asp->datalen );
	}
	if ( asp_wrtreply( asp, reply ) < 0 ) {
	  syslog( LOG_ERR, "asp_wrtreply: %m" );
	  afp_asp_die(1);
	}
	break;
      default:
	/*
	   * Bad asp packet.  Probably should have asp filter them,
	   * since they are typically things like out-of-order packet.
	   */
	syslog( LOG_INFO, "main: asp_getrequest: %d", reply );
	break;
      }

      if ( obj->options.flags & OPTION_DEBUG ) {
#ifdef notdef
	pdesc( stdout );
#endif notdef
	of_pforkdesc( stdout );
	fflush( stdout );
      }
    }
}

#endif
