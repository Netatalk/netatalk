/*
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
#include <setjmp.h>

#include <atalk/dsi.h>
#include <atalk/compat.h>
#include <atalk/util.h>

#include "globals.h"
#include "switch.h"
#include "auth.h"
#include "fork.h"
#include "dircache.h"

#ifdef FORCE_UIDGID
#warning UIDGID
#include "uid.h"
#endif /* FORCE_UIDGID */

/* 
 * We generally pass this from afp_over_dsi to all afp_* funcs, so it should already be
 * available everywhere. Unfortunately some funcs (eg acltoownermode) need acces to it
 * but are deeply nested in the function chain with the caller already without acces to it.
 * Changing this would require adding a reference to the caller which itself might be
 * called in many places (eg acltoownermode is called from accessmode).
 * The only sane way out is providing a copy of it here:
 */
AFPObj *AFPobj = NULL;

typedef struct {
    uint16_t DSIreqID;
    uint8_t  AFPcommand;
    uint32_t result;
} rc_elem_t;

/*
 * AFP replay cache:
 * - fix sized array
 * - indexed just by taking DSIreqID mod REPLAYCACHE_SIZE
 */
static rc_elem_t replaycache[REPLAYCACHE_SIZE];

static sigjmp_buf recon_jmp;
static int oldsock = -1;
static void afp_dsi_close(AFPObj *obj)
{
    DSI *dsi = obj->handle;

    close(obj->ipc_fd);
    obj->ipc_fd = -1;

    /* we may have been called from a signal handler caught when afpd was running
     * as uid 0, that's the wrong user for volume's prexec_close scripts if any,
     * restore our login user
     */
    if (geteuid() != obj->uid) {
        if (seteuid( obj->uid ) < 0) {
            LOG(log_error, logtype_afpd, "can't seteuid(%u) back %s: uid: %u, euid: %u", 
                obj->uid, strerror(errno), getuid(), geteuid());
            exit(EXITERR_SYS);
        }
    }

    close_all_vol();
    if (obj->logout)
        (*obj->logout)();

    LOG(log_note, logtype_afpd, "AFP statistics: %.2f KB read, %.2f KB written",
        dsi->read_count/1024.0, dsi->write_count/1024.0);
    log_dircache_stat();

    dsi_close(dsi);
}

/* -------------------------------
 * SIGTERM
 * a little bit of code duplication. 
 */
static void afp_dsi_die(int sig)
{
    DSI *dsi = (DSI *)AFPobj->handle;
    static volatile int in_handler;
    
    if (in_handler) {
    	return;
    }
    /* it's not atomic but we don't care because it's an exit function
     * ie if a signal is received here, between the test and the affectation,
     * it will not return.
    */
    in_handler = 1;

    if (dsi->flags & DSI_RECONINPROG) {
        /* Primary reconnect succeeded, got SIGTERM from afpd parent */
        dsi->flags &= ~DSI_RECONINPROG;
        return; /* this returns to afp_disconnect */
    }

    dsi_attention(AFPobj->handle, AFPATTN_SHUTDOWN);
    afp_dsi_close(AFPobj);
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

static void afp_dsi_transfer_session(int sig _U_)
{
    uint16_t dsiID;
    int socket;
    DSI *dsi = (DSI *)AFPobj->handle;

    LOG(log_note, logtype_afpd, "afp_dsi_transfer_session: got SIGURG, trying to receive session");

    if (readt(AFPobj->ipc_fd, &dsiID, 2, 0, 2) != 2) {
        LOG(log_error, logtype_afpd, "afp_dsi_transfer_session: couldn't receive DSI id, goodbye");
        afp_dsi_close(AFPobj);
        exit(EXITERR_SYS);
    }

    if ((socket = recv_fd(AFPobj->ipc_fd, 1)) == -1) {
        LOG(log_error, logtype_afpd, "afp_dsi_transfer_session: couldn't receive session fd, goodbye");
        afp_dsi_close(AFPobj);
        exit(EXITERR_SYS);
    }

    LOG(log_note, logtype_afpd, "afp_dsi_transfer_session: received socket fd: %i", socket);

    dsi->socket = socket;
    dsi->flags |= DSI_RECONSOCKET;
    dsi->flags &= ~DSI_DISCONNECTED;
    dsi->datalen = 0;
    dsi->header.dsi_requestID = dsiID;
    dsi->header.dsi_command = DSIFUNC_CMD;

    /*
     * The session transfer happens in the middle of FPDisconnect old session, thus we
     * have to send the reply now.
     */
    if (!dsi_cmdreply(dsi, AFP_OK)) {
        LOG(log_error, logtype_afpd, "dsi_cmdreply: %s", strerror(errno) );
        afp_dsi_close(AFPobj);
        exit(EXITERR_CLNT);
    }

    LOG(log_note, logtype_afpd, "afp_dsi_transfer_session: succesfull primary reconnect");
    /* 
     * Now returning from this signal handler return to dsi_receive which should start
     * reading/continuing from the connected socket that was passed via the parent from
     * another session. The parent will terminate that session.
     */
    siglongjmp(recon_jmp, 1);
}

/* */
static void afp_dsi_sleep(void)
{
    dsi_sleep(AFPobj->handle, 1);
}

/* ------------------- */
static void afp_dsi_timedown(int sig _U_)
{
    struct sigaction	sv;
    struct itimerval	it;
    DSI                 *dsi = (DSI *)AFPobj->handle;
    dsi->flags |= DSI_DIE;
    /* shutdown and don't reconnect. server going down in 5 minutes. */
    setmessage("The server is going down for maintenance.");
    if (dsi_attention(AFPobj->handle, AFPATTN_SHUTDOWN | AFPATTN_NORECONNECT |
                  AFPATTN_MESG | AFPATTN_TIME(5)) < 0) {
        DSI *dsi = (DSI *)AFPobj->handle;
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
static void afp_dsi_getmesg (int sig _U_)
{
    DSI *dsi = (DSI *)AFPobj->handle;

    dsi->msg_request = 1;
    if (dsi_attention(AFPobj->handle, AFPATTN_MESG | AFPATTN_TIME(5)) < 0)
        dsi->msg_request = 2;
}

static void alarm_handler(int sig _U_)
{
    int err;
    DSI *dsi = (DSI *)AFPobj->handle;

    /* we have to restart the timer because some libraries may use alarm() */
    setitimer(ITIMER_REAL, &dsi->timer, NULL);

    /* we got some traffic from the client since the previous timer tick. */
    if ((dsi->flags & DSI_DATA)) {
        dsi->flags &= ~DSI_DATA;
        dsi->flags &= ~DSI_DISCONNECTED;
       return;
    }

    dsi->tickle++;

    if (dsi->flags & DSI_SLEEPING) {
        if (dsi->tickle < AFPobj->options.sleep) {
            LOG(log_error, logtype_afpd, "afp_alarm: sleep time ended");
            afp_dsi_die(EXITERR_CLNT);
        }
        return;
    } 

    if (dsi->flags & DSI_DISCONNECTED) {
        if (dsi->tickle > AFPobj->options.disconnected) {
             LOG(log_error, logtype_afpd, "afp_alarm: no reconnect within 10 minutes, goodbye");
            afp_dsi_die(EXITERR_CLNT);
        }
        return;
    }

    /* if we're in the midst of processing something, don't die. */        
    if ( !(dsi->flags & DSI_RUNNING) && (dsi->tickle > AFPobj->options.timeout)) {
        LOG(log_error, logtype_afpd, "afp_alarm: child timed out, entering disconnected state");
        dsi->flags |= DSI_DISCONNECTED;
        return;
    }

    if ((err = pollvoltime(AFPobj)) == 0)
        err = dsi_tickle(AFPobj->handle);
    if (err <= 0) {
        LOG(log_error, logtype_afpd, "afp_alarm: connection problem, entering disconnected state");
        dsi->flags |= DSI_DISCONNECTED;
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
            dsi_attention(AFPobj->handle, AFPATTN_MESG | AFPATTN_TIME(5));
        }
        dsi->msg_request = 0;
        readmessage(AFPobj);
    }
    if (dsi->down_request) {
        dsi->down_request = 0;
        dsi_attention(AFPobj->handle, AFPATTN_SHUTDOWN | AFPATTN_NORECONNECT |
                  AFPATTN_MESG | AFPATTN_TIME(5));
    }
}

/* -------------------------------------------
 afp over dsi. this never returns. 
*/
void afp_over_dsi(AFPObj *obj)
{
    DSI *dsi = (DSI *) obj->handle;
    int rc_idx;
    u_int32_t err, cmd;
    u_int8_t function;
    struct sigaction action;

    AFPobj = obj;
    obj->exit = afp_dsi_die;
    obj->reply = (int (*)()) dsi_cmdreply;
    obj->attention = (int (*)(void *, AFPUserBytes)) dsi_attention;
    obj->sleep = afp_dsi_sleep;
    dsi->tickle = 0;

    memset(&action, 0, sizeof(action));

    /* install SIGHUP */
    action.sa_handler = afp_dsi_reload;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGALRM);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGUSR1);
    sigaddset(&action.sa_mask, SIGINT);
    sigaddset(&action.sa_mask, SIGUSR2);
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGHUP, &action, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }

    /* install SIGURG */
    action.sa_handler = afp_dsi_transfer_session;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGALRM);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGUSR1);
    sigaddset(&action.sa_mask, SIGINT);
    sigaddset(&action.sa_mask, SIGUSR2);
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGURG, &action, NULL ) < 0 ) {
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
    sigaddset(&action.sa_mask, SIGUSR2);
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGTERM, &action, NULL ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(EXITERR_SYS);
    }

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

    /*  SIGUSR1 - set down in 5 minutes  */
    action.sa_handler = afp_dsi_timedown;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGALRM);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGINT);
    sigaddset(&action.sa_mask, SIGUSR2);
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
    sigaddset(&action.sa_mask, SIGUSR2);
    action.sa_flags = SA_RESTART;
    if ((sigaction(SIGALRM, &action, NULL) < 0) ||
            (setitimer(ITIMER_REAL, &dsi->timer, NULL) < 0)) {
        afp_dsi_die(EXITERR_SYS);
    }
#endif /* DEBUGGING */

    if (dircache_init(obj->options.dircachesize) != 0)
        afp_dsi_die(EXITERR_SYS);

    /* get stuck here until the end */
    while (1) {
        if (sigsetjmp(recon_jmp, 1) != 0) {
            LOG(log_note, logtype_afpd, "Resuming operation after succesfull primary reconnect");
            dsi->flags &= ~(DSI_RUNNING | DSI_DATA);
            dsi->eof = dsi->start = dsi->buffer;
            dsi->in_write = 0;
            continue;
        }
        cmd = dsi_receive(dsi);
        if (cmd == 0) {
            if (dsi->flags & DSI_RECONSOCKET) {
                /* we just got a reconnect so we immediately try again to receive on the new fd */
                dsi->flags &= ~DSI_RECONSOCKET;
                continue;
            }
            /* Some error on the client connection, enter disconnected state */
            dsi->flags |= DSI_DISCONNECTED;
            pause(); /* gets interrupted by SIGALARM or SIGURG tickle */
            continue; /* continue receiving until disconnect timer expires
                       * or a primary reconnect succeeds  */
        } else {
            if (oldsock != -1) {
                /* Now, after successfully reading from the new socket after a reconnect       *
                 * we can close the old socket without taking the risk of confusing the client */
                close(oldsock);
                oldsock = -1;
            }
        }

        dsi->tickle = 0;
        dsi_sleep(dsi, 0); /* wake up */

        if (reload_request) {
            reload_request = 0;
            load_volumes(AFPobj);
            dircache_dump();
            log_dircache_stat();
        }

        /* The first SIGINT enables debugging, the next restores the config */
        if (debug_request) {
            static int debugging = 0;
            debug_request = 0;

            if (debugging) {
                if (obj->options.logconfig)
                    setuplog(obj->options.logconfig);
                else
                    setuplog("default log_note");
                debugging = 0;
            } else {
                char logstr[50];
                debugging = 1;
                sprintf(logstr, "default log_maxdebug /tmp/afpd.%u.XXXXXX", getpid());
                setuplog(logstr);
            }
        }

        if (cmd == DSIFUNC_TICKLE) {
            /* timer is not every 30 seconds anymore, so we don't get killed on the client side. */
            if ((dsi->flags & DSI_DIE))
                dsi_tickle(dsi);
            pending_request(dsi);
            continue;
        } 

        dsi->flags |= DSI_DATA;
        switch(cmd) {
        case DSIFUNC_CLOSE:
            afp_dsi_close(obj);
            LOG(log_note, logtype_afpd, "done");
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

            /* AFP replay cache */
            rc_idx = REPLAYCACHE_SIZE % dsi->clientID;
            LOG(log_debug, logtype_afpd, "DSI request ID: %u", dsi->clientID);

            if (replaycache[rc_idx].DSIreqID == dsi->clientID
                && replaycache[rc_idx].AFPcommand == function) {
                LOG(log_debug, logtype_afpd, "AFP Replay Cache match: id: %u / cmd: %s",
                    dsi->clientID, AfpNum2name(function));
                err = replaycache[rc_idx].result;
            /* AFP replay cache end */
            } else {
                /* send off an afp command. in a couple cases, we take advantage
                 * of the fact that we're a stream-based protocol. */
                if (afp_switch[function]) {
                    dsi->datalen = DSI_DATASIZ;
                    dsi->flags |= DSI_RUNNING;

                    LOG(log_debug, logtype_afpd, "<== Start AFP command: %s", AfpNum2name(function));

                    err = (*afp_switch[function])(obj,
                                                  (char *)&dsi->commands, dsi->cmdlen,
                                                  (char *)&dsi->data, &dsi->datalen);

                    LOG(log_debug, logtype_afpd, "==> Finished AFP command: %s -> %s",
                        AfpNum2name(function), AfpErr2name(err));

                    dir_free_invalid_q();

#ifdef FORCE_UIDGID
                    /* bring everything back to old euid, egid */
                    if (obj->force_uid)
                        restore_uidgid ( &obj->uidgid );
#endif /* FORCE_UIDGID */
                    dsi->flags &= ~DSI_RUNNING;

                    /* Add result to the AFP replay cache */
                    replaycache[rc_idx].DSIreqID = dsi->clientID;
                    replaycache[rc_idx].AFPcommand = function;
                    replaycache[rc_idx].result = err;
                } else {
                    LOG(log_error, logtype_afpd, "bad function %X", function);
                    dsi->datalen = 0;
                    err = AFPERR_NOOP;
                }
            }

            /* single shot toggle that gets set by dsi_readinit. */
            if (dsi->flags & DSI_NOREPLY) {
                dsi->flags &= ~DSI_NOREPLY;
                break;
            }

            if (!dsi_cmdreply(dsi, err)) {
                LOG(log_error, logtype_afpd, "dsi_cmdreply(%d): %s", dsi->socket, strerror(errno) );
                dsi->flags |= DSI_DISCONNECTED;
            }
            break;

        case DSIFUNC_WRITE: /* FPWrite and FPAddIcon */
            function = (u_char) dsi->commands[0];
            if ( afp_switch[ function ] != NULL ) {
                dsi->datalen = DSI_DATASIZ;
                dsi->flags |= DSI_RUNNING;

                LOG(log_debug, logtype_afpd, "<== Start AFP command: %s", AfpNum2name(function));

                err = (*afp_switch[function])(obj,
                                              (char *)&dsi->commands, dsi->cmdlen,
                                              (char *)&dsi->data, &dsi->datalen);

                LOG(log_debug, logtype_afpd, "==> Finished AFP command: %s -> %s",
                    AfpNum2name(function), AfpErr2name(err));

                dsi->flags &= ~DSI_RUNNING;
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
                dsi->flags |= DSI_DISCONNECTED;
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
