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
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <atalk/afp_util.h>
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
#include "directory.h"

static AFPObj *child;

static void afp_asp_close(AFPObj *obj)
{
    ASP asp = obj->handle;

    /* euid may not be the login user if an error path exited mid-command
     * (e.g. after become_root()); volume prexec_close scripts need the login
     * user, so restore it and carry on. Only a failed restore is fatal —
     * exiting on success would skip close_all_vol() and the PAM logout. */
    if (obj->uid != geteuid() && seteuid(obj->uid) < 0) {
        LOG(log_error, logtype_afpd,
            "afp_asp_close: can't seteuid back to %i from %i (%s)",
            obj->uid, geteuid(), strerror(errno));
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

/*
 * Signals are recorded and acted on from the request loop, not inside the
 * handler: the work these used to do in signal context — LOG(), close_all_vol(),
 * free(), PAM logout, blocking network I/O — deadlocks or corrupts the heap if
 * it interrupts the main flow mid-allocation or holding the syslog lock.
 *
 * That deferral only works because poll() is the loop's one blocking point, so
 * the self-pipe can always wake it. The request read is entered only once poll()
 * has reported the socket readable, and consumes a single datagram before
 * returning, which is why the handlers can keep SA_RESTART as the DSI handlers
 * do: nothing here waits anywhere the pipe is not watched.
 */
/* Consecutive asp_getrequest() read errors tolerated before the session is
 * given up on */
#define ASP_MAX_READ_ERRORS 10
/* Stray packets between log lines. They are remotely driven and harmless, so
 * they are counted rather than logged one line each. */
#define ASP_STRAY_LOG_INTERVAL 1000

static volatile sig_atomic_t asp_die_pending = 0;
static volatile sig_atomic_t asp_timedown_pending = 0;
#ifdef SERVERTEXT
static volatile sig_atomic_t asp_getmesg_pending = 0;
#endif /* SERVERTEXT */

/* ------------------------
 * SIGTERM
*/
static void afp_asp_die_handler(int sig);

static void afp_asp_die_now(const int sig)
{
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
static void afp_asp_timedown_now(void)
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

    if (setitimer(ITIMER_REAL, &it, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_timedown: setitimer: %s", strerror(errno));
        afp_asp_die_now(EXITERR_SYS);
    }

    memset(&sv, 0, sizeof(sv));
    sv.sa_handler = afp_asp_die_handler;
    sigemptyset(&sv.sa_mask);
    sigaddset(&sv.sa_mask, SIGHUP);
    sigaddset(&sv.sa_mask, SIGTERM);
    sv.sa_flags = SA_RESTART;

    if (sigaction(SIGALRM, &sv, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_timedown: sigaction: %s", strerror(errno));
        afp_asp_die_now(EXITERR_SYS);
    }

    /* ignore myself */
    sv.sa_handler = SIG_IGN;
    sigemptyset(&sv.sa_mask);
    sv.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sv, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_timedown: sigaction SIGUSR1: %s",
            strerror(errno));
        afp_asp_die_now(EXITERR_SYS);
    }
}

/* ---------------------------------
 * SIGHUP reload configuration file
*/
extern volatile sig_atomic_t reload_request;

static void afp_asp_reload(int sig _U_)
{
    reload_request = 1;
    atalk_sigpipe_notify();
}

/* ---------------------- */
#ifdef SERVERTEXT
static void afp_asp_getmesg_now(void)
{
    readmessage(child);
    asp_attention(child->handle, AFPATTN_MESG | AFPATTN_TIME(5));
}
#endif /* SERVERTEXT */

/* --- handlers: record and wake, nothing more ---
 *
 * All of these keep SA_RESTART, as the DSI handlers do. Clearing it would make
 * every interruptible syscall in the child fail with EINTR: a SIGHUP config
 * reload delivered while atp_sresp() blocks in netddp_sendto() would surface as
 * asp_cmdreply() < 0, which the loop treats as a dead client, so a reload would
 * terminate healthy sessions. The self-pipe is what makes the wake-up reliable,
 * and it is sufficient because the loop only ever blocks in poll(). */
static void afp_asp_die_handler(int sig)
{
    asp_die_pending = (sig == 0) ? SIGTERM : sig;
    atalk_sigpipe_notify();
}

static void afp_asp_timedown(int sig _U_)
{
    asp_timedown_pending = 1;
    atalk_sigpipe_notify();
}

#ifdef SERVERTEXT
static void afp_asp_getmesg(int sig _U_)
{
    asp_getmesg_pending = 1;
    atalk_sigpipe_notify();
}
#endif /* SERVERTEXT */

/*!
 * @brief Act on signals recorded while the loop was busy or waiting
 */
static void asp_process_deferred_signals(AFPObj *obj)
{
    if (asp_die_pending) {
        int sig = asp_die_pending;
        asp_die_pending = 0;
        afp_asp_die_now(sig);            /* exits */
    }

    if (asp_timedown_pending) {
        asp_timedown_pending = 0;
        afp_asp_timedown_now();
    }

    if (reload_request) {
        reload_request = 0;
        load_afp_conf_vols(obj, LV_FORCE);
    }

#ifdef SERVERTEXT

    if (asp_getmesg_pending) {
        asp_getmesg_pending = 0;
        afp_asp_getmesg_now();
    }

#endif /* SERVERTEXT */
}

/* ---------------------- */
void afp_over_asp(AFPObj *obj)
{
    ASP asp;
    struct sigaction  action;
    int		func,  reply = 0;
    int ccnt = 0;
    int read_errors = 0;
    unsigned long stray_packets = 0;
    AFPobj = obj;
    obj->exit = afp_asp_die_now;
    obj->reply = (int (*)()) asp_cmdreply;
    obj->attention = (int (*)(void *, AFPUserBytes)) asp_attention;
    child = obj;
    asp = (ASP) obj->handle;
    /* Adopt the connection snapshot, IPC socketpair and dircache hint pipe
     * that asp_getsess created before fork(). This mirrors what dsi_getsess
     * does for DSI children and keeps login limits plus IPC reporting valid
     * for ASP sessions. */
    obj->cnx_cnt = asp->asp_cnx_cnt;
    obj->cnx_max = asp->asp_cnx_max;
    obj->ipc_fd  = asp->asp_ipc_fd;
    obj->hint_fd = asp->asp_hint_fd;

    if (atalk_sigpipe_init() != 0) {
        LOG(log_error, logtype_afpd, "afp_over_asp: signal pipe: %s",
            strerror(errno));
        exit(EXITERR_SYS);
    }

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
        afp_asp_die_now(EXITERR_SYS);
    }

    /*  install SIGTERM */
    action.sa_handler = afp_asp_die_handler;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGUSR1);
#ifdef SERVERTEXT
    sigaddset(&action.sa_mask, SIGUSR2);
#endif
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGTERM, &action, NULL) < 0) {
        LOG(log_error, logtype_afpd, "afp_over_asp: sigaction: %s", strerror(errno));
        afp_asp_die_now(EXITERR_SYS);
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
        afp_asp_die_now(EXITERR_SYS);
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
        afp_asp_die_now(EXITERR_SYS);
    }

    /* Disable rfork caching for ASP sessions — the rfork cache tier-2
     * layer is designed for the DSI (TCP) transport path and provides no
     * benefit over AppleTalk. Zero the budget so dircache_init() skips
     * rfork subsystem allocation while still initializing the core
     * directory cache that all AFP operations depend on. */
    obj->options.dircache_rfork_budget = 0;
    obj->options.dircache_rfork_maxentry = 0;

    if (dircache_init(obj->options.dircachesize) != 0) {
        LOG(log_error, logtype_afpd, "afp_over_asp: dircache_init error");
        afp_asp_die_now(EXITERR_SYS);
    }

    LOG(log_info, logtype_afpd, "session from %u.%u:%u on %u.%u:%u",
        ntohs(asp->asp_sat.sat_addr.s_net),
        asp->asp_sat.sat_addr.s_node, asp->asp_sat.sat_port,
        ntohs(atp_sockaddr(asp->asp_atp)->sat_addr.s_net),
        atp_sockaddr(asp->asp_atp)->sat_addr.s_node,
        atp_sockaddr(asp->asp_atp)->sat_port);

    while (1) {
        /*
         * The inner loop waits; the outer loop serves. Sections [A0]-[I] are the
         * DSI loop's, so the two transports can be read against each other.
         * DSI's [A] (stream data already buffered) and [B] (disconnected or
         * dying) have no counterpart here: ASP has neither a stream buffer nor a
         * reconnect state machine. [A1] is ASP's own.
         */
        while (1) {
            /* [A0] Process deferred signals before any blocking I/O */
            atalk_sigpipe_drain();
            asp_process_deferred_signals(obj);
            /* [A1] Release what the hints pruned. DSI does this after each
             * command; ASP does it here so an idle session still sheds other
             * clients' churn while it is receiving nothing but hints. */
            dir_free_invalid_q();
            /* [C] Setup poll fds — includes self-pipe for signal wake-up */
            struct pollfd pfds[3];
            int nfds = 0;
            int hint_idx = -1;
            int sigpipe_idx;
            pfds[nfds].fd = atp_fileno(asp->asp_atp);
            pfds[nfds].events = POLLIN;
            nfds++;

            if (obj->hint_fd >= 0) {
                hint_idx = nfds;
                pfds[nfds].fd = obj->hint_fd;
                pfds[nfds].events = POLLIN;
                nfds++;
            }

            sigpipe_idx = nfds;
            pfds[nfds].fd = atalk_sigpipe_readfd();
            pfds[nfds].events = POLLIN;
            nfds++;
            /* [D] BLOCK IN POLL */
            int pollret = poll(pfds, nfds, -1);

            /* [E] poll error handling */
            if (pollret < 0) {
                if (errno == EINTR) {
                    continue;               /* re-check the flags at [A0] */
                }

                LOG(log_error, logtype_afpd, "afp_over_asp: poll: %s",
                    strerror(errno));
                afp_asp_close(obj);
                return;
            }

            /* [E1] Signal pipe — drain and act, as [A0] would. Nothing can
             * clear a bad one: a POLLHUP/POLLNVAL/POLLERR that no read
             * consumes makes poll() return immediately for the rest of the
             * session, and no deferred signal could ever wake the loop. */
            if (pfds[sigpipe_idx].revents & (POLLNVAL | POLLERR | POLLHUP)) {
                LOG(log_error, logtype_afpd,
                    "afp_over_asp: signal pipe fd %d invalid, ending session",
                    atalk_sigpipe_readfd());
                afp_asp_close(obj);
                return;
            }

            if (pfds[sigpipe_idx].revents & POLLIN) {
                atalk_sigpipe_drain();
                asp_process_deferred_signals(obj);
            }

            /* [F] Hint pipe handling — ahead of [H], so a sibling's
             * invalidation cannot be overtaken by the command it invalidates.
             * An invalid fd reports POLLNVAL forever and read() cannot clear
             * it, so it is closed here rather than in the hint reader. */
            if (hint_idx >= 0) {
                if (pfds[hint_idx].revents & POLLNVAL) {
                    LOG(log_error, logtype_afpd,
                        "afp_over_asp: hint pipe fd %d invalid (POLLNVAL)",
                        obj->hint_fd);
                    close(obj->hint_fd);
                    obj->hint_fd = -1;
                } else if (pfds[hint_idx].revents & (POLLIN | POLLHUP | POLLERR)) {
                    process_cache_hints(obj);
                }
            }

            /* [G] ATP socket error handling */
            if (pfds[0].revents & POLLNVAL) {
                LOG(log_error, logtype_afpd,
                    "afp_over_asp: ATP socket fd %d invalid (POLLNVAL)",
                    atp_fileno(asp->asp_atp));
                afp_asp_close(obj);
                return;
            }

            /* [H] Request ready. DSI ends the session on POLLHUP/POLLERR
             * because TCP makes them definitive; on a datagram socket they are
             * not, so the read still runs — it returns the pending error, and
             * the bounded read-error counter below ends the session if it
             * persists. Logged so the cause is not invisible. */
            if (pfds[0].revents & (POLLHUP | POLLERR)) {
                LOG(log_debug, logtype_afpd,
                    "afp_over_asp: ATP socket %s reported during wait",
                    (pfds[0].revents & POLLERR) ? "error" : "hangup");
            }

            if (pfds[0].revents & (POLLIN | POLLHUP | POLLERR)) {
                break;
            }

            /* [I] Only the hint or signal pipe had data — back to poll() */
        }

        /* Read one datagram, which cannot block: [H] established that the
         * socket is readable and asp_getrequest() makes a single attempt. A
         * datagram that yields no request comes back to the wait rather than
         * parking the loop here with the signal and hint pipes unwatched, and
         * that is the guarantee the deferred handlers rely on. */
        reply = asp_getrequest(asp);

        if (reply == ASP_NOREQUEST) {
            continue;                       /* stray or replayed, not ours */
        }

        if (!reply) {
            LOG(log_note, logtype_afpd,
                "afp_over_asp: session ended without a close request");
            afp_asp_close(obj);
            return;
        }

        /* A read error repeats, so it is bounded rather than retried forever.
         * The bound counts read errors only: sequence and session-id mismatches
         * are stray or replayed packets, and they neither raise nor clear it,
         * because a peer interleaving them with genuine errors would otherwise
         * hold the counter below the limit indefinitely. Every failure is
         * reported with its errno — this is a local fault, not remote noise, so
         * it is not rate-limited and must not reach the stray-packet arm of the
         * switch below. */
        if (reply == ASP_ERR_READ) {
            read_errors++;

            if (read_errors >= ASP_MAX_READ_ERRORS) {
                LOG(log_error, logtype_afpd,
                    "afp_over_asp: read error %d of %d, ending session: %s",
                    read_errors, ASP_MAX_READ_ERRORS, strerror(errno));
                afp_asp_close(obj);
                return;
            }

            LOG(log_note, logtype_afpd, "afp_over_asp: read error %d of %d: %s",
                read_errors, ASP_MAX_READ_ERRORS, strerror(errno));
            continue;
        }

        if (reply >= 0) {
            read_errors = 0;
        }

        switch (reply) {
        case ASPFUNC_CLOSE :
            afp_asp_close(obj);
            LOG(log_note, logtype_afpd, "done");
            return;

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
                afp_asp_die_now(EXITERR_CLNT);
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
                afp_asp_die_now(EXITERR_CLNT);
            }

            break;

        default:

            /*
               * Bad asp packet.  Probably should have asp filter them,
               * since they are typically things like out-of-order packet.
               */
            if ((stray_packets++ % ASP_STRAY_LOG_INTERVAL) == 0) {
                LOG(log_info, logtype_afpd,
                    "main: asp_getrequest: %d (%lu stray)", reply,
                    stray_packets);
            }

            break;
        }

        if (obj->options.flags & OPTION_DEBUG) {
            of_pforkdesc(stdout);
            fflush(stdout);
        }
    }
}

#endif
