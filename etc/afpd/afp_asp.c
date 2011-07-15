/*
 * $Id: afp_asp.c,v 1.29 2010-03-09 06:55:12 franklahm Exp $
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
#include <atalk/globals.h>

#include "switch.h"
#include "auth.h"
#include "fork.h"

#ifdef FORCE_UIDGID
#warning UIDGID
#include "uid.h"
#endif /* FORCE_UIDGID */

static AFPObj *child;

static void afp_authprint_remove(AFPObj *);

static void afp_asp_close(AFPObj *obj)
{
    ASP asp = obj->handle;

    if (seteuid( obj->uid ) < 0) {
        LOG(log_error, logtype_afpd, "can't seteuid back %s", strerror(errno));
        exit(EXITERR_SYS);
    }
    close_all_vol();
    if (obj->options.authprintdir) afp_authprint_remove(obj);

    if (obj->logout)
        (*obj->logout)();

    LOG(log_info, logtype_afpd, "%.2fKB read, %.2fKB written",
        asp->read_count / 1024.0, asp->write_count / 1024.0);
    asp_close( asp );
}

/* removes the authprint trailing when appropriate */
static void afp_authprint_remove(AFPObj *obj)
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

    if(lstat(addr_filename, &cap_st) == 0) {
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
			    LOG(log_info, logtype_afpd, "removed %s", addr_filename);
			} else {
			    LOG(log_info, logtype_afpd, "error removing %s: %s",
				    addr_filename, strerror(errno));
			}
		    } else {
			LOG(log_info, logtype_afpd, "%s belongs to another pid %d",
			     addr_filename, file_pid );
		    }
		} else { /* no pid info */
		    if (unlink(addr_filename) == 0) {
			LOG(log_info, logtype_afpd, "removed %s", addr_filename );
		    } else {
			LOG(log_info, logtype_afpd, "error removing %s: %s",
				addr_filename, strerror(errno));
		    }
		}
	    } else {
		LOG(log_info, logtype_afpd, "couldn't read data from %s", addr_filename );
	    }
	} else {
	    LOG(log_info, logtype_afpd, "%s is not a regular file", addr_filename );
	}
    } else {
        LOG(log_info, logtype_afpd, "error stat'ing %s: %s",
                   addr_filename, strerror(errno));
    }
}

/* ------------------------
 * SIGTERM
*/
static void afp_asp_die(const int sig)
{
    ASP asp = child->handle;

    asp_attention(asp, AFPATTN_SHUTDOWN);
    if ( asp_shutdown( asp ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_die: asp_shutdown: %s", strerror(errno) );
    }

    afp_asp_close(child);
    if (sig == SIGTERM || sig == SIGALRM)
        exit( 0 );
    else
        exit(sig);
}

/* -----------------------------
 * SIGUSR1
 */
static void afp_asp_timedown(int sig _U_)
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
    if ( setitimer( ITIMER_REAL, &it, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_timedown: setitimer: %s", strerror(errno) );
        afp_asp_die(EXITERR_SYS);
    }

    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = afp_asp_die;
    sigemptyset( &sv.sa_mask );
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGTERM);
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGALRM, &sv, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_timedown: sigaction: %s", strerror(errno) );
        afp_asp_die(EXITERR_SYS);
    }

    /* ignore myself */
    sv.sa_handler = SIG_IGN;
    sigemptyset( &sv.sa_mask );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR1, &sv, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_timedown: sigaction SIGUSR1: %s", strerror(errno) );
        afp_asp_die(EXITERR_SYS);
    }
}

/* ---------------------------------
 * SIGHUP reload configuration file
*/
extern volatile int reload_request;

static void afp_asp_reload(int sig _U_)
{
    reload_request = 1;
}

/* ---------------------- */
#ifdef SERVERTEXT
static void afp_asp_getmesg (int sig _U_)
{
    readmessage(child);
    asp_attention(child->handle, AFPATTN_MESG | AFPATTN_TIME(5));
}
#endif /* SERVERTEXT */

/* ---------------------- */
void afp_over_asp(AFPObj *obj)
{
    ASP asp;
    struct sigaction  action;
    int		func,  reply = 0;
#ifdef DEBUG1
    int ccnt = 0;
#endif    

    AFPobj = obj;
    obj->exit = afp_asp_die;
    obj->reply = (int (*)()) asp_cmdreply;
    obj->attention = (int (*)(void *, AFPUserBytes)) asp_attention;
    child = obj;
    asp = (ASP) obj->handle;

    /* install signal handlers 
     * With ASP tickle handler is done in the parent process
    */
    memset(&action, 0, sizeof(action));

    /* install SIGHUP */
    action.sa_handler = afp_asp_reload; 
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGUSR1);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif    
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGHUP, &action, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_asp: sigaction: %s", strerror(errno) );
        afp_asp_die(EXITERR_SYS);
    }

    /*  install SIGTERM */
    action.sa_handler = afp_asp_die;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGUSR1);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif    
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGTERM, &action, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_asp: sigaction: %s", strerror(errno) );
        afp_asp_die(EXITERR_SYS);
    }

#ifdef SERVERTEXT
    /* Added for server message support */
    action.sa_handler = afp_asp_getmesg;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGUSR1);
    sigaddset(&action.sa_mask, SIGHUP);
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR2, &action, NULL) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_asp: sigaction: %s", strerror(errno) );
        afp_asp_die(EXITERR_SYS);
    }
#endif /* SERVERTEXT */

    /*  SIGUSR1 - set down in 5 minutes  */
    action.sa_handler = afp_asp_timedown; 
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGTERM);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif    
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR1, &action, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_asp: sigaction: %s", strerror(errno) );
        afp_asp_die(EXITERR_SYS);
    }

    LOG(log_info, logtype_afpd, "session from %u.%u:%u on %u.%u:%u",
        ntohs( asp->asp_sat.sat_addr.s_net ),
        asp->asp_sat.sat_addr.s_node, asp->asp_sat.sat_port,
        ntohs( atp_sockaddr( asp->asp_atp )->sat_addr.s_net ),
        atp_sockaddr( asp->asp_atp )->sat_addr.s_node,
        atp_sockaddr( asp->asp_atp )->sat_port );

    while ((reply = asp_getrequest(asp))) {
        if (reload_request) {
            reload_request = 0;
            load_volumes(child);
        }
        switch (reply) {
        case ASPFUNC_CLOSE :
            afp_asp_close(obj);
            LOG(log_info, logtype_afpd, "done" );

#ifdef DEBUG1
            if ( obj->options.flags & OPTION_DEBUG ) {
                printf( "done\n" );
            }
#endif
            return;
            break;

        case ASPFUNC_CMD :
#ifdef AFS
            if ( writtenfork ) {
                if ( flushfork( writtenfork ) < 0 ) {
                    LOG(log_error, logtype_afpd, "main flushfork: %s",
						strerror(errno));
                }
                writtenfork = NULL;
            }
#endif /* AFS */
            func = (u_char) asp->commands[0];
#ifdef DEBUG1
            if ( obj->options.flags & OPTION_DEBUG ) {
                printf("command: %d (%s)\n", func, AfpNum2name(func));
                bprint( asp->commands, asp->cmdlen );
            }
#endif            
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
#ifdef FORCE_UIDGID
            	/* bring everything back to old euid, egid */
		if (obj->force_uid)
            	    restore_uidgid ( &obj->uidgid );
#endif /* FORCE_UIDGID */
            } else {
                LOG(log_error, logtype_afpd, "bad function %X", func );
                asp->datalen = 0;
                reply = AFPERR_NOOP;
            }
#ifdef DEBUG1
            if ( obj->options.flags & OPTION_DEBUG ) {
                printf( "reply: %d, %d\n", reply, ccnt++ );
                bprint( asp->data, asp->datalen );
            }
#endif
            if ( asp_cmdreply( asp, reply ) < 0 ) {
                LOG(log_error, logtype_afpd, "asp_cmdreply: %s", strerror(errno) );
                afp_asp_die(EXITERR_CLNT);
            }
            break;

        case ASPFUNC_WRITE :
            func = (u_char) asp->commands[0];
#ifdef DEBUG1
            if ( obj->options.flags & OPTION_DEBUG ) {
                printf( "(write) command: %d\n", func );
                bprint( asp->commands, asp->cmdlen );
            }
#endif
            if ( afp_switch[ func ] != NULL ) {
                asp->datalen = ASP_DATASIZ;
                reply = (*afp_switch[ func ])(obj,
                                              asp->commands, asp->cmdlen,
                                              asp->data, &asp->datalen);
#ifdef FORCE_UIDGID
            	/* bring everything back to old euid, egid */
		if (obj->force_uid)
            	    restore_uidgid ( &obj->uidgid );
#endif /* FORCE_UIDGID */
            } else {
                LOG(log_error, logtype_afpd, "(write) bad function %X", func );
                asp->datalen = 0;
                reply = AFPERR_NOOP;
            }
#ifdef DEBUG1
            if ( obj->options.flags & OPTION_DEBUG ) {
                printf( "(write) reply code: %d, %d\n", reply, ccnt++ );
                bprint( asp->data, asp->datalen );
            }
#endif
            if ( asp_wrtreply( asp, reply ) < 0 ) {
                LOG(log_error, logtype_afpd, "asp_wrtreply: %s", strerror(errno) );
                afp_asp_die(EXITERR_CLNT);
            }
            break;
        default:
            /*
               * Bad asp packet.  Probably should have asp filter them,
               * since they are typically things like out-of-order packet.
               */
            LOG(log_info, logtype_afpd, "main: asp_getrequest: %d", reply );
            break;
        }
#ifdef DEBUG1
        if ( obj->options.flags & OPTION_DEBUG ) {
#ifdef notdef
            pdesc( stdout );
#endif /* notdef */
            of_pforkdesc( stdout );
            fflush( stdout );
        }
#endif
    }
}

#endif
