/*
 * $Id: afp_dsi.c,v 1.33 2008-12-03 18:35:44 didg Exp $
 *
 * Copyright (c) 1999 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 *
 * modified from main.c. this handles afp over tcp.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/socket.h>
#include <sys/time.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <atalk/logger.h>

#include <atalk/dsi.h>
#include <atalk/compat.h>
#include <atalk/util.h>

#include "globals.h"
#include "switch.h"
#include "auth.h"
#include "fork.h"

#ifdef FORCE_UIDGID
#warning UIDGID
#include "uid.h"
#endif /* FORCE_UIDGID */

extern struct oforks	*writtenfork;

#define CHILD_DIE         (1 << 0)
#define CHILD_RUNNING     (1 << 1)
#define CHILD_SLEEPING    (1 << 2)

static struct {
    AFPObj *obj;
    unsigned char flags;
    int tickle;
} child;


static void afp_dsi_close(AFPObj *obj)
{
    DSI *dsi = obj->handle;

    close_all_vol();
    if (obj->logout)
        (*obj->logout)();

    /* UAM had syslog control; afpd needs to reassert itself */
    set_processname("afpd");
    syslog_setup(log_debug, logtype_default, logoption_ndelay | logoption_pid, logfacility_daemon);
    LOG(log_info, logtype_afpd, "%.2fKB read, %.2fKB written",
        dsi->read_count/1024.0, dsi->write_count/1024.0);

    dsi_close(dsi);
}

/* -------------------------------
 * SIGTERM
 * a little bit of code duplication. 
 */
static void afp_dsi_die(int sig)
{
static volatile int in_handler;
    
    if (in_handler) {
    	return;
    }
    /* it's not atomic but we don't care because it's an exit function
     * ie if a signal is received here, between the test and the affectation,
     * it will not return.
    */
    in_handler = 1;

    dsi_attention(child.obj->handle, AFPATTN_SHUTDOWN);
    afp_dsi_close(child.obj);
    if (sig) /* if no signal, assume dieing because logins are disabled &
                don't log it (maintenance mode)*/
        LOG(log_info, logtype_afpd, "Connection terminated");
    if (sig == SIGTERM || sig == SIGALRM) {
        exit( 0 );
    }
    else {
        exit(sig);
    }
}

/* */
static void afp_dsi_sleep(void)
{
    child.flags |= CHILD_SLEEPING;
    dsi_sleep(child.obj->handle, 1);
}

/* ------------------- */
static void afp_dsi_timedown()
{
    struct sigaction	sv;
    struct itimerval	it;

    child.flags |= CHILD_DIE;
    /* shutdown and don't reconnect. server going down in 5 minutes. */
    setmessage("The server is going down for maintenance.");
    dsi_attention(child.obj->handle, AFPATTN_SHUTDOWN | AFPATTN_NORECONNECT |
                  AFPATTN_MESG | AFPATTN_TIME(5));

    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 300;
    it.it_value.tv_usec = 0;

    if ( setitimer( ITIMER_REAL, &it, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_timedown: setitimer: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }
    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = afp_dsi_die;
    sigemptyset( &sv.sa_mask );
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGTERM);
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGALRM, &sv, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_timedown: sigaction: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }

    /* ignore myself */
    sv.sa_handler = SIG_IGN;
    sigemptyset( &sv.sa_mask );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR1, &sv, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_timedown: sigaction SIGHUP: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }

}

/* ---------------------------------
 * SIGHUP reload configuration file
 * FIXME here or we wait ?
*/
volatile int reload_request = 0;

static void afp_dsi_reload()
{
    reload_request = 1;
}

/* ---------------------- */
#ifdef SERVERTEXT
static void afp_dsi_getmesg (int sig _U_)
{
    readmessage(child.obj);
    dsi_attention(child.obj->handle, AFPATTN_MESG | AFPATTN_TIME(5));
}
#endif /* SERVERTEXT */

static void alarm_handler()
{
    int err;

    /* if we're in the midst of processing something,
       don't die. */
    if ((child.flags & CHILD_SLEEPING) && child.tickle++ < child.obj->options.sleep) {
        return;
    } else if ((child.flags & CHILD_RUNNING) || (child.tickle++ < child.obj->options.timeout)) {
        if (!(err = pollvoltime(child.obj)))
            err = dsi_tickle(child.obj->handle);
        if (err <= 0) 
            afp_dsi_die(EXITERR_CLNT);
        
    } else { /* didn't receive a tickle. close connection */
        LOG(log_error, logtype_afpd, "afp_alarm: child timed out");
        afp_dsi_die(EXITERR_CLNT);
    }
}


#ifdef DEBUG1
/*  ---------------------------------
 *  old signal handler for SIGUSR1 - set the debug flag and 
 *  redirect stdout to <tmpdir>/afpd-debug-<pid>.
 */
void afp_set_debug (int sig)
{
    char	fname[MAXPATHLEN];

    snprintf(fname, MAXPATHLEN-1, "%safpd-debug-%d", P_tmpdir, getpid());
    freopen(fname, "w", stdout);
    child.obj->options.flags |= OPTION_DEBUG;

    return;
}
#endif

/* -------------------------------------------
 afp over dsi. this never returns. 
*/
void afp_over_dsi(AFPObj *obj)
{
    DSI *dsi = (DSI *) obj->handle;
    u_int32_t err, cmd;
    u_int8_t function;
    struct sigaction action;

    obj->exit = afp_dsi_die;
    obj->reply = (int (*)()) dsi_cmdreply;
    obj->attention = (int (*)(void *, AFPUserBytes)) dsi_attention;

    obj->sleep = afp_dsi_sleep;
    child.obj = obj;
    child.tickle = child.flags = 0;

    memset(&action, 0, sizeof(action));

    /* install SIGHUP */
    action.sa_handler = afp_dsi_reload;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGALRM);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGUSR1);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif    
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGHUP, &action, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }

    /* install SIGTERM */
    action.sa_handler = afp_dsi_die;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGALRM);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGUSR1);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif    
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGTERM, &action, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }

#ifdef SERVERTEXT
    /* Added for server message support */
    action.sa_handler = afp_dsi_getmesg;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGALRM);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGUSR1);
    sigaddset(&action.sa_mask, SIGHUP);
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR2, &action, 0) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }
#endif /* SERVERTEXT */

    /*  SIGUSR1 - set down in 5 minutes  */
    action.sa_handler = afp_dsi_timedown;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGALRM);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGTERM);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif    
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR1, &action, 0) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }

    /* tickle handler */
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGUSR1);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif    
    action.sa_flags = SA_RESTART;
    if ((sigaction(SIGALRM, &action, NULL) < 0) ||
            (setitimer(ITIMER_REAL, &dsi->timer, NULL) < 0)) {
        afp_dsi_die(EXITERR_SYS);
    }

#ifdef DEBUG1
    fault_setup((void (*)(void *))afp_dsi_die);
#endif

    /* get stuck here until the end */
    while ((cmd = dsi_receive(dsi))) {
        child.tickle = 0;
        child.flags &= ~CHILD_SLEEPING;
        dsi_sleep(dsi, 0); /* wake up */
        if (reload_request) {
            reload_request = 0;
            load_volumes(child.obj);
        }

        if (cmd == DSIFUNC_TICKLE) {
            /* timer is not every 30 seconds anymore, so we don't get killed on the client side. */
            if ((child.flags & CHILD_DIE))
                dsi_tickle(dsi);
            continue;
        } else if (!(child.flags & CHILD_DIE)) { /* reset tickle timer */
            setitimer(ITIMER_REAL, &dsi->timer, NULL);
        }
        switch(cmd) {
        case DSIFUNC_CLOSE:
            afp_dsi_close(obj);
            LOG(log_info, logtype_afpd, "done");
#ifdef DEBUG1
            if (obj->options.flags & OPTION_DEBUG )
                printf("done\n");
#endif                
            return;
            break;

        case DSIFUNC_CMD:
#ifdef AFS
            if ( writtenfork ) {
                if ( flushfork( writtenfork ) < 0 ) {
                    LOG(log_error, logtype_afpd, "main flushfork: %s", strerror(errno) );
                }
                writtenfork = NULL;
            }
#endif /* AFS */

            function = (u_char) dsi->commands[0];
#ifdef DEBUG1
            if (obj->options.flags & OPTION_DEBUG ) {
                printf("command: %d (%s)\n", function, AfpNum2name(function));
                bprint((char *) dsi->commands, dsi->cmdlen);
            }
#endif            

            /* send off an afp command. in a couple cases, we take advantage
             * of the fact that we're a stream-based protocol. */
            if (afp_switch[function]) {
                dsi->datalen = DSI_DATASIZ;
                child.flags |= CHILD_RUNNING;

                err = (*afp_switch[function])(obj,
                                              dsi->commands, dsi->cmdlen,
                                              dsi->data, &dsi->datalen);
#ifdef FORCE_UIDGID
            	/* bring everything back to old euid, egid */
		if (obj->force_uid)
            	    restore_uidgid ( &obj->uidgid );
#endif /* FORCE_UIDGID */
                child.flags &= ~CHILD_RUNNING;
            } else {
                LOG(log_error, logtype_afpd, "bad function %X", function);
                dsi->datalen = 0;
                err = AFPERR_NOOP;
            }

            /* single shot toggle that gets set by dsi_readinit. */
            if (dsi->noreply) {
                dsi->noreply = 0;
                break;
            }

#ifdef DEBUG1
            if (obj->options.flags & OPTION_DEBUG ) {
                printf( "reply: %d, %d\n", err, dsi->clientID);
                bprint((char *) dsi->data, dsi->datalen);
            }
#endif
            if (!dsi_cmdreply(dsi, err)) {
                LOG(log_error, logtype_afpd, "dsi_cmdreply(%d): %s", dsi->socket, strerror(errno) );
                afp_dsi_die(EXITERR_CLNT);
            }
            break;

        case DSIFUNC_WRITE: /* FPWrite and FPAddIcon */
            function = (u_char) dsi->commands[0];
#ifdef DEBUG1
            if ( obj->options.flags & OPTION_DEBUG ) {
                printf("(write) command: %d, %d\n", function, dsi->cmdlen);
                bprint((char *) dsi->commands, dsi->cmdlen);
            }
#endif
            if ( afp_switch[ function ] != NULL ) {
                dsi->datalen = DSI_DATASIZ;
                child.flags |= CHILD_RUNNING;
                err = (*afp_switch[function])(obj, dsi->commands, dsi->cmdlen,
                                              dsi->data, &dsi->datalen);
                child.flags &= ~CHILD_RUNNING;
#ifdef FORCE_UIDGID
            	/* bring everything back to old euid, egid */
		if (obj->force_uid)
            	    restore_uidgid ( &obj->uidgid );
#endif /* FORCE_UIDGID */
            } else {
                LOG(log_error, logtype_afpd, "(write) bad function %x", function);
                dsi->datalen = 0;
                err = AFPERR_NOOP;
            }

#ifdef DEBUG1
            if (obj->options.flags & OPTION_DEBUG ) {
                printf( "(write) reply code: %d, %d\n", err, dsi->clientID);
                bprint((char *) dsi->data, dsi->datalen);
            }
#endif
            if (!dsi_wrtreply(dsi, err)) {
                LOG(log_error, logtype_afpd, "dsi_wrtreply: %s", strerror(errno) );
                afp_dsi_die(EXITERR_CLNT);
            }
            break;

        case DSIFUNC_ATTN: /* attention replies */
            continue;
            break;

            /* error. this usually implies a mismatch of some kind
             * between server and client. if things are correct,
             * we need to flush the rest of the packet if necessary. */
        default:
            LOG(log_info, logtype_afpd,"afp_dsi: spurious command %d", cmd);
            dsi_writeinit(dsi, dsi->data, DSI_DATASIZ);
            dsi_writeflush(dsi);
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

    /* error */
    afp_dsi_die(EXITERR_CLNT);
}
