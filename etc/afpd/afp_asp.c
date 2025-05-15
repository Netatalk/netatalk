/*
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
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <netatalk/endian.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/compat.h>
#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/globals.h>
#include <atalk/netatalk_conf.h>

#include "switch.h"
#include "auth.h"
#include "fork.h"
#include "dircache.h"

static AFPObj *child;

static void afp_asp_close(AFPObj *obj) {
    ASP asp = obj->handle;

    if (obj->uid != geteuid()) {
        if (seteuid(obj->uid) < 0) {
            LOG(log_error, logtype_afpd,
                "afp_asp_close: can't seteuid back to %i from %i (%s)",
                obj->uid, geteuid(), strerror(errno));
        }

        exit(EXITERR_SYS);
    }

    close_all_vol(obj);

    if (obj->logout) {
        (*obj->logout)();
    }

    LOG(log_note, logtype_afpd,
        "AFP statistics: %.2f KB read, %.2f KB written via ASP",
        asp->read_count / 1024.0, asp->write_count / 1024.0);
    asp_close(asp);
}

/* ------------------------
 * SIGTERM
*/
static void afp_asp_die(const int sig) {
    ASP asp = child->handle;
    asp_attention(asp, AFPATTN_SHUTDOWN);

    if (asp_shutdown(asp) < 0) {
        LOG(log_error, logtype_afpd, "afp_die: asp_shutdown: %s", strerror(errno));
    }

    afp_asp_close(child);

    if (sig == SIGTERM || sig == SIGALRM) {
        exit(0);
    } else {
        exit(sig);
    }
}

/* -----------------------------
 * SIGUSR1
 */
static void afp_asp_timedown(int sig _U_) {
    struct sigaction	sv;
    struct itimerval	it;
    /* shutdown and don't reconnect. server going down in 5 minutes. */
    asp_attention(child->handle, AFPATTN_SHUTDOWN | AFPATTN_NORECONNECT |
                  AFPATTN_TIME(5));
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 300;
    it.it_value.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &it, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_timedown: setitimer: %s", strerror(errno));
        afp_asp_die(EXITERR_SYS);
    }

    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = afp_asp_die;
    sigemptyset(&sv.sa_mask);
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGTERM);
    sv.sa_flags = SA_RESTART;

    if (sigaction(SIGALRM, &sv, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_timedown: sigaction: %s", strerror(errno));
        afp_asp_die(EXITERR_SYS);
    }

    /* ignore myself */
    sv.sa_handler = SIG_IGN;
    sigemptyset(&sv.sa_mask);
    sv.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sv, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_timedown: sigaction SIGUSR1: %s",
            strerror(errno));
        afp_asp_die(EXITERR_SYS);
    }
}

/* ---------------------------------
 * SIGHUP reload configuration file
*/
extern volatile int reload_request;

static void afp_asp_reload(int sig _U_) {
    reload_request = 1;
}

/* ---------------------- */
#ifdef SERVERTEXT
static void afp_asp_getmesg(int sig _U_) {
    readmessage(child);
    asp_attention(child->handle, AFPATTN_MESG | AFPATTN_TIME(5));
}
#endif /* SERVERTEXT */

/* ---------------------- */
void afp_over_asp(AFPObj *obj) {
    ASP asp;
    struct sigaction  action;
    int		func,  reply = 0;
    int ccnt = 0;
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
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGUSR1);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGHUP, &action, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_over_asp: sigaction: %s", strerror(errno));
        afp_asp_die(EXITERR_SYS);
    }

    /*  install SIGTERM */
    action.sa_handler = afp_asp_die;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGUSR1);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGTERM, &action, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_over_asp: sigaction: %s", strerror(errno));
        afp_asp_die(EXITERR_SYS);
    }

#ifdef SERVERTEXT
    /* Added for server message support */
    action.sa_handler = afp_asp_getmesg;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGTERM);
    sigaddset(&action.sa_mask, SIGUSR1);
    sigaddset(&action.sa_mask, SIGHUP);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR2, &action, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_over_asp: sigaction: %s", strerror(errno));
        afp_asp_die(EXITERR_SYS);
    }

#endif /* SERVERTEXT */
    /*  SIGUSR1 - set down in 5 minutes  */
    action.sa_handler = afp_asp_timedown;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGTERM);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &action, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_over_asp: sigaction: %s", strerror(errno));
        afp_asp_die(EXITERR_SYS);
    }

    if (dircache_init(obj->options.dircachesize) != 0) {
        LOG(log_error, logtype_afpd, "afp_over_asp: dircache_init error");
        afp_asp_die(EXITERR_SYS);
    }

    LOG(log_info, logtype_afpd, "session from %u.%u:%u on %u.%u:%u",
        ntohs(asp->asp_sat.sat_addr.s_net),
        asp->asp_sat.sat_addr.s_node, asp->asp_sat.sat_port,
        ntohs(atp_sockaddr(asp->asp_atp)->sat_addr.s_net),
        atp_sockaddr(asp->asp_atp)->sat_addr.s_node,
        atp_sockaddr(asp->asp_atp)->sat_port);

    while ((reply = asp_getrequest(asp))) {
        if (reload_request) {
            reload_request = 0;
            load_volumes(child, LV_FORCE);
        }

        switch (reply) {
        case ASPFUNC_CLOSE :
            afp_asp_close(obj);
            LOG(log_note, logtype_afpd, "done");
            return;
            break;

        case ASPFUNC_CMD :
            func = (unsigned char) asp->commands[0];
            LOG(log_debug9, logtype_afpd, "command: %d (%s)\n", func, AfpNum2name(func));

            if (afp_switch[func] != NULL) {
                /*
                 * The function called from afp_switch is expected to
                 * read its parameters out of buf, put its
                 * results in replybuf (updating rbuflen), and
                 * return an error code.
                */
                asp->datalen = ASP_DATASIZ;
                reply = (*afp_switch[func])(obj,
                                            asp->commands, asp->cmdlen,
                                            asp->data, &asp->datalen);
            } else {
                LOG(log_error, logtype_afpd, "bad function %X", func);
                asp->datalen = 0;
                reply = AFPERR_NOOP;
            }

            LOG(log_debug9, logtype_afpd, "reply: %d, %d\n", reply, ccnt++);

            if (asp_cmdreply(asp, reply) < 0) {
                LOG(log_error, logtype_afpd, "asp_cmdreply: %s", strerror(errno));
                afp_asp_die(EXITERR_CLNT);
            }

            break;

        case ASPFUNC_WRITE :
            func = (unsigned char) asp->commands[0];
            LOG(log_debug9, logtype_afpd, "(write) command: %d\n", func);

            if (afp_switch[func] != NULL) {
                asp->datalen = ASP_DATASIZ;
                reply = (*afp_switch[func])(obj,
                                            asp->commands, asp->cmdlen,
                                            asp->data, &asp->datalen);
            } else {
                LOG(log_error, logtype_afpd, "(write) bad function %X", func);
                asp->datalen = 0;
                reply = AFPERR_NOOP;
            }

            LOG(log_debug9, logtype_afpd, "(write) reply code: %d, %d\n", reply, ccnt++);

            if (asp_wrtreply(asp, reply) < 0) {
                LOG(log_error, logtype_afpd, "asp_wrtreply: %s", strerror(errno));
                afp_asp_die(EXITERR_CLNT);
            }

            break;

        default:
            /*
               * Bad asp packet.  Probably should have asp filter them,
               * since they are typically things like out-of-order packet.
               */
            LOG(log_info, logtype_afpd, "main: asp_getrequest: %d", reply);
            break;
        }

        if (obj->options.flags & OPTION_DEBUG) {
            of_pforkdesc(stdout);
            fflush(stdout);
        }
    }
}

#endif
