/*
 * $Id: afp_dsi.c,v 1.51 2010-02-16 02:37:38 didg Exp $
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

#define CHILD_DIE         (1 << 0)
#define CHILD_RUNNING     (1 << 1)
#define CHILD_SLEEPING    (1 << 2)
#define CHILD_DATA        (1 << 3)

static struct {
    AFPObj *obj;
    unsigned char flags;
    int tickle;
} child;


static void afp_dsi_close(AFPObj *obj)
{
    DSI *dsi = obj->handle;

    /* we may have been called from a signal handler caught when afpd was running
     * as uid 0, that's the wrong user for volume's prexec_close scripts if any,
     * restore our login user
     */
    if (seteuid( obj->uid ) < 0) {
        LOG(log_error, logtype_afpd, "can't seteuid back %s", strerror(errno));
        exit(EXITERR_SYS);
    }
    close_all_vol();
    if (obj->logout)
        (*obj->logout)();

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
static void afp_dsi_timedown(int sig _U_)
{
    struct sigaction	sv;
    struct itimerval	it;

    child.flags |= CHILD_DIE;
    /* shutdown and don't reconnect. server going down in 5 minutes. */
    setmessage("The server is going down for maintenance.");
    if (dsi_attention(child.obj->handle, AFPATTN_SHUTDOWN | AFPATTN_NORECONNECT |
                  AFPATTN_MESG | AFPATTN_TIME(5)) < 0) {
        DSI *dsi = (DSI *) child.obj->handle;
        dsi->down_request = 1;
    }                  

    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 300;
    it.it_value.tv_usec = 0;

    if ( setitimer( ITIMER_REAL, &it, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_timedown: setitimer: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }
    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = afp_dsi_die;
    sigemptyset( &sv.sa_mask );
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGTERM);
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGALRM, &sv, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_timedown: sigaction: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }

    /* ignore myself */
    sv.sa_handler = SIG_IGN;
    sigemptyset( &sv.sa_mask );
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR1, &sv, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_timedown: sigaction SIGHUP: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }
}

/* ---------------------------------
 * SIGHUP reload configuration file
 * FIXME here or we wait ?
*/
volatile int reload_request = 0;

static void afp_dsi_reload(int sig _U_)
{
    reload_request = 1;
}

/* ---------------------------------
 * SIGINT: enable max_debug LOGging
 */
static volatile sig_atomic_t debug_request = 0;

static void afp_dsi_debug(int sig _U_)
{
    debug_request = 1;
}

/* ---------------------- */
#ifdef SERVERTEXT
static void afp_dsi_getmesg (int sig _U_)
{
    DSI *dsi = (DSI *) child.obj->handle;

    dsi->msg_request = 1;
    if (dsi_attention(child.obj->handle, AFPATTN_MESG | AFPATTN_TIME(5)) < 0)
        dsi->msg_request = 2;
}
#endif /* SERVERTEXT */

static void alarm_handler(int sig _U_)
{
    int err;
    DSI *dsi = (DSI *) child.obj->handle;

    /* we have to restart the timer because some libraries 
     * may use alarm() */
    setitimer(ITIMER_REAL, &dsi->timer, NULL);

    /* we got some traffic from the client since the previous timer 
     * tick. */
    if ((child.flags & CHILD_DATA)) {
        child.flags &= ~CHILD_DATA;
        return;
    }

    /* if we're in the midst of processing something,
       don't die. */
    if ((child.flags & CHILD_SLEEPING) && child.tickle++ < child.obj->options.sleep) {
        return;
    } 
        
    if ((child.flags & CHILD_RUNNING) || (child.tickle++ < child.obj->options.timeout)) {
        if (!(err = pollvoltime(child.obj)))
            err = dsi_tickle(child.obj->handle);
        if (err <= 0) 
            afp_dsi_die(EXITERR_CLNT);
        
    } else { /* didn't receive a tickle. close connection */
        LOG(log_error, logtype_afpd, "afp_alarm: child timed out");
        afp_dsi_die(EXITERR_CLNT);
    }
}

/* ----------------- 
   if dsi->in_write is set attention, tickle (and close?) msg
   aren't sent. We don't care about tickle 
*/
static void pending_request(DSI *dsi)
{
    /* send pending attention */

    /* read msg if any, it could be done in afp_getsrvrmesg */
    if (dsi->msg_request) {
        if (dsi->msg_request == 2) {
            /* didn't send it in signal handler */
            dsi_attention(child.obj->handle, AFPATTN_MESG | AFPATTN_TIME(5));
        }
        dsi->msg_request = 0;
        readmessage(child.obj);
    }
    if (dsi->down_request) {
        dsi->down_request = 0;
        dsi_attention(child.obj->handle, AFPATTN_SHUTDOWN | AFPATTN_NORECONNECT |
                  AFPATTN_MESG | AFPATTN_TIME(5));
    }
}

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
    sigaddset(&action.sa_mask, SIGINT);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif    
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGHUP, &action, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }

    /* install SIGTERM */
    action.sa_handler = afp_dsi_die;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGALRM);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGUSR1);
    sigaddset(&action.sa_mask, SIGINT);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif    
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGTERM, &action, NULL ) < 0 ) {
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
    sigaddset(&action.sa_mask, SIGINT);
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR2, &action, NULL) < 0 ) {
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
    sigaddset(&action.sa_mask, SIGINT);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif    
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR1, &action, NULL) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }

    /*  SIGINT - enable max_debug LOGging to /tmp/afpd.PID.XXXXXX */
    action.sa_handler = afp_dsi_debug;
    sigfillset( &action.sa_mask );
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGINT, &action, NULL) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }

#ifndef DEBUGGING
    /* tickle handler */
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGUSR1);
    sigaddset(&action.sa_mask, SIGINT);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif    
    action.sa_flags = SA_RESTART;
    if ((sigaction(SIGALRM, &action, NULL) < 0) ||
            (setitimer(ITIMER_REAL, &dsi->timer, NULL) < 0)) {
        afp_dsi_die(EXITERR_SYS);
    }
#endif /* DEBUGGING */

    /* get stuck here until the end */
    while ((cmd = dsi_receive(dsi))) {
        child.tickle = 0;
        child.flags &= ~CHILD_SLEEPING;
        dsi_sleep(dsi, 0); /* wake up */

        if (reload_request) {
            reload_request = 0;
            load_volumes(child.obj);
        }

        if (debug_request) {
            char logstr[50];
            debug_request = 0;

            /* The first SIGINT enables debugging, the second one kills us */
            action.sa_handler = afp_dsi_die;
            sigfillset( &action.sa_mask );
            action.sa_flags = SA_RESTART;
            if ( sigaction( SIGINT, &action, NULL ) < 0 ) {
                LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
                afp_dsi_die(EXITERR_SYS);
            }

            sprintf(logstr, "default log_maxdebug /tmp/afpd.%u.XXXXXX", getpid());
            setuplog(logstr);
        }

        if (cmd == DSIFUNC_TICKLE) {
            /* timer is not every 30 seconds anymore, so we don't get killed on the client side. */
            if ((child.flags & CHILD_DIE))
                dsi_tickle(dsi);
            pending_request(dsi);
            continue;
        } 

        child.flags |= CHILD_DATA;
        switch(cmd) {
        case DSIFUNC_CLOSE:
            afp_dsi_close(obj);
            LOG(log_info, logtype_afpd, "done");
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

            /* send off an afp command. in a couple cases, we take advantage
             * of the fact that we're a stream-based protocol. */
            if (afp_switch[function]) {
                dsi->datalen = DSI_DATASIZ;
                child.flags |= CHILD_RUNNING;

                LOG(log_debug, logtype_afpd, "<== Start AFP command: %s", AfpNum2name(function));

                err = (*afp_switch[function])(obj,
                                              (char *)&dsi->commands, dsi->cmdlen,
                                              (char *)&dsi->data, &dsi->datalen);

                LOG(log_debug, logtype_afpd, "==> Finished AFP command: %s -> %s",
                    AfpNum2name(function), AfpErr2name(err));
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

            if (!dsi_cmdreply(dsi, err)) {
                LOG(log_error, logtype_afpd, "dsi_cmdreply(%d): %s", dsi->socket, strerror(errno) );
                afp_dsi_die(EXITERR_CLNT);
            }
            break;

        case DSIFUNC_WRITE: /* FPWrite and FPAddIcon */
            function = (u_char) dsi->commands[0];
            if ( afp_switch[ function ] != NULL ) {
                dsi->datalen = DSI_DATASIZ;
                child.flags |= CHILD_RUNNING;
                err = (*afp_switch[function])(obj,
                                              (char *)&dsi->commands, dsi->cmdlen,
                                              (char *)&dsi->data, &dsi->datalen);
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

            if (!dsi_wrtreply(dsi, err)) {
                LOG(log_error, logtype_afpd, "dsi_wrtreply: %s", strerror(errno) );
                afp_dsi_die(EXITERR_CLNT);
            }
            break;

        case DSIFUNC_ATTN: /* attention replies */
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
        pending_request(dsi);
    }

    /* error */
    afp_dsi_die(EXITERR_CLNT);
}
