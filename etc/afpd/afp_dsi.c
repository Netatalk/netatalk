/*
 * $Id: afp_dsi.c,v 1.20 2002-03-24 01:23:40 sibaz Exp $
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

extern struct oforks	*writtenfork;

#define CHILD_DIE         (1 << 0)
#define CHILD_RUNNING     (1 << 1)

static struct {
    AFPObj *obj;
    unsigned char flags;
    int tickle;
} child;


static __inline__ void afp_dsi_close(AFPObj *obj)
{
    DSI *dsi = obj->handle;

    if (obj->logout)
        (*obj->logout)();

    /* UAM had syslog control; afpd needs to reassert itself */
    set_processname("afpd");
    syslog_setup(log_debug, logtype_default, logoption_ndelay | logoption_pid, logfacility_daemon);
    LOG(log_info, logtype_afpd, "%.2fKB read, %.2fKB written",
        dsi->read_count/1024.0, dsi->write_count/1024.0);

    dsi_close(dsi);
}

/* a little bit of code duplication. */
static void afp_dsi_die(int sig)
{
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
        afp_dsi_die(1);
    }

    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = afp_dsi_die;
    sigemptyset( &sv.sa_mask );
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGTERM);
    sv.sa_flags = SA_RESTART;
    if ( sigaction( SIGALRM, &sv, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_timedown: sigaction: %s", strerror(errno) );
        afp_dsi_die(1);
    }
}

#ifdef SERVERTEXT
static void afp_dsi_getmesg (int sig)
{
    readmessage();
    dsi_attention(child.obj->handle, AFPATTN_MESG | AFPATTN_TIME(5));
}
#endif /* SERVERTEXT */

static void alarm_handler()
{
    /* if we're in the midst of processing something,
       don't die. */
    if ((child.flags & CHILD_RUNNING) || (child.tickle++ < child.obj->options.timeout)) {
        dsi_tickle(child.obj->handle);
    } else { /* didn't receive a tickle. close connection */
        LOG(log_error, logtype_afpd, "afp_alarm: child timed out");
        afp_dsi_die(1);
    }
}

/* afp over dsi. this never returns. */
void afp_over_dsi(AFPObj *obj)
{
    DSI *dsi = (DSI *) obj->handle;
    u_int32_t err, cmd;
    u_int8_t function;
    struct sigaction action;

    obj->exit = afp_dsi_die;
    obj->reply = (int (*)()) dsi_cmdreply;
    obj->attention = (int (*)(void *, AFPUserBytes)) dsi_attention;

    child.obj = obj;
    child.tickle = child.flags = 0;

    /* install SIGTERM and SIGHUP */
    memset(&action, 0, sizeof(action));
    action.sa_handler = afp_dsi_timedown;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGALRM);
    sigaddset(&action.sa_mask, SIGTERM);
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGHUP, &action, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(1);
    }

    action.sa_handler = afp_dsi_die;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGALRM);
    sigaddset(&action.sa_mask, SIGHUP);
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGTERM, &action, 0 ) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(1);
    }

#ifdef SERVERTEXT
    /* Added for server message support */
    action.sa_handler = afp_dsi_getmesg;
    sigemptyset( &action.sa_mask );
    sigaddset(&action.sa_mask, SIGUSR2);
    action.sa_flags = SA_RESTART;
    if ( sigaction( SIGUSR2, &action, 0) < 0 ) {
        LOG(log_error, logtype_afpd, "afp_over_dsi: sigaction: %s", strerror(errno) );
        afp_dsi_die(1);
    }
#endif /* SERVERTEXT */

    /* tickle handler */
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGTERM);
    action.sa_flags = SA_RESTART;
    if ((sigaction(SIGALRM, &action, NULL) < 0) ||
            (setitimer(ITIMER_REAL, &dsi->timer, NULL) < 0)) {
        afp_dsi_die(1);
    }

    /* get stuck here until the end */
    while ((cmd = dsi_receive(dsi))) {
        child.tickle = 0;

        if (cmd == DSIFUNC_TICKLE) {
            /* so we don't get killed on the client side. */
            if (child.flags & CHILD_DIE)
                dsi_tickle(dsi);
            continue;
        } else if (!(child.flags & CHILD_DIE)) /* reset tickle timer */
            setitimer(ITIMER_REAL, &dsi->timer, NULL);

        switch(cmd) {
        case DSIFUNC_CLOSE:
            afp_dsi_close(obj);
            LOG(log_info, logtype_afpd, "done");
            if (obj->options.flags & OPTION_DEBUG )
                printf("done\n");
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
            if (obj->options.flags & OPTION_DEBUG ) {
                printf("command: %d (%s)\n", function, AfpNum2name(function));
                bprint((char *) dsi->commands, dsi->cmdlen);
            }

            /* send off an afp command. in a couple cases, we take advantage
             * of the fact that we're a stream-based protocol. */
            if (afp_switch[function]) {
                dsi->datalen = DSI_DATASIZ;
                child.flags |= CHILD_RUNNING;
                err = (*afp_switch[function])(obj,
                                              dsi->commands, dsi->cmdlen,
                                              dsi->data, &dsi->datalen);
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

            if (obj->options.flags & OPTION_DEBUG ) {
                printf( "reply: %d, %d\n", err, dsi->clientID);
                bprint((char *) dsi->data, dsi->datalen);
            }

            if (!dsi_cmdreply(dsi, err)) {
                LOG(log_error, logtype_afpd, "dsi_cmdreply(%d): %s", dsi->socket, strerror(errno) );
                afp_dsi_die(1);
            }
            break;

        case DSIFUNC_WRITE: /* FPWrite and FPAddIcon */
            function = (u_char) dsi->commands[0];
            if ( obj->options.flags & OPTION_DEBUG ) {
                printf("(write) command: %d, %d\n", function, dsi->cmdlen);
                bprint((char *) dsi->commands, dsi->cmdlen);
            }

            if ( afp_switch[ function ] != NULL ) {
                dsi->datalen = DSI_DATASIZ;
                child.flags |= CHILD_RUNNING;
                err = (*afp_switch[function])(obj, dsi->commands, dsi->cmdlen,
                                              dsi->data, &dsi->datalen);
                child.flags &= ~CHILD_RUNNING;
            } else {
                LOG(log_error, logtype_afpd, "(write) bad function %x", function);
                dsi->datalen = 0;
                err = AFPERR_NOOP;
            }

            if (obj->options.flags & OPTION_DEBUG ) {
                printf( "(write) reply code: %d, %d\n", err, dsi->clientID);
                bprint((char *) dsi->data, dsi->datalen);
            }

            if (!dsi_wrtreply(dsi, err)) {
                LOG(log_error, logtype_afpd, "dsi_wrtreply: %s", strerror(errno) );
                afp_dsi_die(1);
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

        if ( obj->options.flags & OPTION_DEBUG ) {
#ifdef notdef
            pdesc( stdout );
#endif /* notdef */
            of_pforkdesc( stdout );
            fflush( stdout );
        }
    }

    /* error */
    afp_dsi_die(1);
}
