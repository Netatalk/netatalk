/*
 * $Id: afp_asp.c,v 1.15 2002-03-23 17:43:10 sibaz Exp $
 *
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 *
 * modified from main.c. this handles afp over asp. 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef NO_DDP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <atalk/logger.h>
#include <errno.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */

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

static AFPObj *child;

static __inline__ void afp_authprint_remove(AFPObj *);

static __inline__ void afp_asp_close(AFPObj *obj)
{
    ASP asp = obj->handle;

    if (obj->options.authprintdir) afp_authprint_remove(obj);

    if (obj->logout)
        (*obj->logout)();

    LOG(log_info, logtype_default, "%.2fKB read, %.2fKB written",
        asp->read_count / 1024.0, asp->write_count / 1024.0);
    asp_close( asp );
}

/* removes the authprint trailing when appropriate */
static __inline__ void afp_authprint_remove(AFPObj *obj)
{
    ASP asp = obj->handle;
    char addr_filename[256];
    char addr_filename_buff[256];
    struct stat cap_st;

    sprintf(addr_filename, "%s/net%d.%dnode%d", obj->options.authprintdir,
                ntohs( asp->asp_sat.sat_addr.s_net )/256,
                ntohs( asp->asp_sat.sat_addr.s_net )%256,
                asp->asp_sat.sat_addr.s_node );

    memset( addr_filename_buff, 0, 256 );

    if(stat(addr_filename, &cap_st) == 0) {
	if( S_ISREG(cap_st.st_mode) ) {
	    int len;
	    int capfd = open( addr_filename, O_RDONLY );
	    if ((len = read( capfd, addr_filename_buff, 256 )) > 0) {
		int file_pid;
		char *p_filepid;
		close(capfd);
		addr_filename_buff[len] = 0;
		if ( (p_filepid = strrchr(addr_filename_buff, ':')) != NULL) {
		    *p_filepid = '\0';
		    p_filepid++;
		    file_pid = atoi(p_filepid);
		    if (file_pid == (int)getpid()) {
			if(unlink(addr_filename) == 0) {
			    LOG(log_info, logtype_default, "removed %s", addr_filename);
			} else {
			    LOG(log_info, logtype_default, "error removing %s: %s",
				    addr_filename, strerror(errno));
			}
		    } else {
			LOG(log_info, logtype_default, "%s belongs to another pid %d",
			     addr_filename, file_pid );
		    }
		} else { /* no pid info */
		    if (unlink(addr_filename) == 0) {
			LOG(log_info, logtype_default, "removed %s", addr_filename );
		    } else {
			LOG(log_info, logtype_default, "error removing %s: %s",
				addr_filename, strerror(errno));
		    }
		}
	    } else {
		LOG(log_info, logtype_default, "couldn't read data from %s", addr_filename );
	    }
	} else {
	    LOG(log_info, logtype_default, "%s is not a regular file", addr_filename );
	}
    } else {
        LOG(log_info, logtype_default, "error stat'ing %s: %s",
                   addr_filename, strerror(errno));
    }
}

static void afp_asp_die(const int sig)
{
    ASP asp = child->handle;

    asp_attention(asp, AFPATTN_SHUTDOWN);
    if ( asp_shutdown( asp ) < 0 ) {
        LOG(log_error, logtype_default, "afp_die: asp_shutdown: %s", strerror(errno) );
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
        LOG(log_error, logtype_default, "afp_timedown: setitimer: %s", strerror(errno) );
        afp_asp_die(1);
    }

    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = afp_asp_die;
    sigemptyset( &sv.sa_mask );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGALRM, &sv, 0 ) < 0 ) {
        LOG(log_error, logtype_default, "afp_timedown: sigaction: %s", strerror(errno) );
        afp_asp_die(1);
    }
}

void afp_over_asp(AFPObj *obj)
{
    ASP asp;
    struct sigaction  action;
    int		func, ccnt = 0, reply = 0;

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
        LOG(log_error, logtype_default, "afp_over_asp: sigaction: %s", strerror(errno) );
        afp_asp_die(1);
    }

    action.sa_handler = afp_asp_die;
    sigemptyset( &action.sa_mask );
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGTERM, &action, 0 ) < 0 ) {
        LOG(log_error, logtype_default, "afp_over_asp: sigaction: %s", strerror(errno) );
        afp_asp_die(1);
    }

    LOG(log_info, logtype_default, "session from %u.%u:%u on %u.%u:%u",
        ntohs( asp->asp_sat.sat_addr.s_net ),
        asp->asp_sat.sat_addr.s_node, asp->asp_sat.sat_port,
        ntohs( atp_sockaddr( asp->asp_atp )->sat_addr.s_net ),
        atp_sockaddr( asp->asp_atp )->sat_addr.s_node,
        atp_sockaddr( asp->asp_atp )->sat_port );

    while ((reply = asp_getrequest(asp))) {
        switch (reply) {
        case ASPFUNC_CLOSE :
            afp_asp_close(obj);
            LOG(log_info, logtype_default, "done" );

            if ( obj->options.flags & OPTION_DEBUG ) {
                printf( "done\n" );
            }
            return;
            break;

        case ASPFUNC_CMD :
#ifdef AFS
            if ( writtenfork ) {
                if ( flushfork( writtenfork ) < 0 ) {
                    LOG(log_error, logtype_default, "main flushfork: %s",
						strerror(errno));
                }
                writtenfork = NULL;
            }
#endif /* AFS */
            func = (u_char) asp->commands[0];
            if ( obj->options.flags & OPTION_DEBUG ) {
                printf("command: %d (%s)\n", func, AfpNum2name(func));
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
                LOG(log_error, logtype_default, "bad function %X", func );
                asp->datalen = 0;
                reply = AFPERR_NOOP;
            }
            if ( obj->options.flags & OPTION_DEBUG ) {
                printf( "reply: %d, %d\n", reply, ccnt++ );
                bprint( asp->data, asp->datalen );
            }

            if ( asp_cmdreply( asp, reply ) < 0 ) {
                LOG(log_error, logtype_default, "asp_cmdreply: %s", strerror(errno) );
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
                LOG(log_error, logtype_default, "(write) bad function %X", func );
                asp->datalen = 0;
                reply = AFPERR_NOOP;
            }
            if ( obj->options.flags & OPTION_DEBUG ) {
                printf( "(write) reply code: %d, %d\n", reply, ccnt++ );
                bprint( asp->data, asp->datalen );
            }
            if ( asp_wrtreply( asp, reply ) < 0 ) {
                LOG(log_error, logtype_default, "asp_wrtreply: %s", strerror(errno) );
                afp_asp_die(1);
            }
            break;
        default:
            /*
               * Bad asp packet.  Probably should have asp filter them,
               * since they are typically things like out-of-order packet.
               */
            LOG(log_info, logtype_default, "main: asp_getrequest: %d", reply );
            break;
        }

        if ( obj->options.flags & OPTION_DEBUG ) {
#ifdef notdef
            pdesc( stdout );
#endif /* notdef */
            of_pforkdesc( stdout );
            fflush( stdout );
        }
    }
}

#endif
