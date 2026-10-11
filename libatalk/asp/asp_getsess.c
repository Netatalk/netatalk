/*
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

#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/wait.h>

#include <netatalk/at.h>
#include <atalk/logger.h>
#include <atalk/compat.h>
#include <atalk/dsi.h>
#include <atalk/atp.h>
#include <atalk/asp.h>
#include <atalk/server_child.h>
#include <atalk/util.h>

#include "asp_child.h"

#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif /* ! WEXITSTATUS */
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif /* ! WIFEXITED */

static ASP server_asp;
static server_child_t *children = NULL;
static struct asp_child    **asp_ac = NULL;

/*!
 * @brief Age each session on SIGALRM, tickle the live ones, and end the silent
 * ones
 *
 * Each tick raises the state of every live session slot, and a tickle from
 * the workstation sets it back to ACSTATE_OK. A session that reaches
 * ACSTATE_BAD, six ticks without a tickle, gets SIGTERM and the dead state.
 * Every other live session gets a tickle from the server.
 *
 * @param[in] sig  signal number, unused
 */
static void tickle_handler(int sig _U_)
{
    int sid;

    /* check status */
    for (sid = 0; sid < children->servch_nsessions; sid++) {
        if (asp_ac[sid] == NULL || asp_ac[sid]->ac_state == ACSTATE_DEAD) {
            continue;
        }

        if (++asp_ac[sid]->ac_state >= ACSTATE_BAD) {
            /* kill. if already dead, just continue */
            if (kill(asp_ac[sid]->ac_pid, SIGTERM) == 0)
                LOG(log_info, logtype_default, "asp_alrm: %d timed out",
                    asp_ac[sid]->ac_pid);

            asp_ac[sid]->ac_state = ACSTATE_DEAD;
            continue;
        }

        /* send off a tickle */
        asp_tickle(server_asp, sid, &asp_ac[sid]->ac_sat);
    }
}

/*!
 * @brief Send a signal to every ASP session child of the server
 *
 * @param[in] sig  signal to send
 */
void asp_kill(int sig)
{
    if (children) {
        server_child_kill(children, sig);
    }
}

/*!
 * @brief Stop the tickle timer of the ASP server, once asp_getsession has set
 * it up
 */
void asp_stop_tickle(void)
{
    if (server_asp && server_asp->inited) {
        static const struct itimerval timer = {{0, 0}, {0, 0}};
        setitimer(ITIMER_REAL, &timer, NULL);
    }
}

static void set_asp_ac(int sid, struct asp_child *tmp);

/*!
 * @brief Serve one request on the ASP server socket: a session open, a tickle,
 * or a status request
 *
 * The first call sets up the session slot table and a SIGALRM timer that runs
 * tickle_handler every tickleval seconds. A status request gets the server
 * status, cut to the packets the requester asks for. A tickle refreshes the
 * slot of its session.
 *
 * An open request with a free slot gets a new ATP socket, an IPC socket pair,
 * and a hint pipe, then a fork. The child returns the session, with
 * asp->child set. The parent records the child, answers with the session
 * socket and the session ID, or ASPERR_SERVBUSY on any failure, and returns
 * the server object.
 *
 * @param[in,out] asp              server object
 * @param[in]     server_children  child table of the server
 * @param[in]     tickleval        seconds between tickles, 0 for none
 * @param[out]    childp           receives the new child in the parent, or NULL
 * @returns the session in the child, the server object in the parent, or NULL
 *          for a failed read or setup
 */
ASP asp_getsession(ASP asp, server_child_t *server_children,
                   const int tickleval, afp_child_t **childp)
{
    struct sigaction    action;
    struct itimerval    timer;
    struct sockaddr_at  sat;
    struct atp_block    atpb;
    ATP                 atp;
    struct iovec        iov[8];
    pid_t               pid;
    int                 i, iovcnt, sid;
    uint16_t           asperr = 0;
    char                *buf;
    int                 buflen;
    afp_child_t         *child;

    if (childp) {
        *childp = NULL;
    }

    if (!asp->inited) {
        if (!(children = server_children)) {
            return NULL;
        }

        /* only calloc once */
        if (!asp_ac && (asp_ac = (struct asp_child **)
                                 calloc(server_children->servch_nsessions, sizeof(struct asp_child *)))
                == NULL) {
            return NULL;
        }

        server_asp = asp;
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

        if (tickleval && ((sigaction(SIGALRM, &action, NULL) < 0) ||
                          (setitimer(ITIMER_REAL, &timer, NULL) < 0))) {
            free(asp_ac);
            server_asp = NULL;
            asp_ac = NULL;
            return NULL;
        }

        asp->inited = 1;
    }

    memset(&sat, 0, sizeof(struct sockaddr_at));
#ifdef BSD4_4
    sat.sat_len = sizeof(struct sockaddr_at);
#endif /* BSD4_4 */
    sat.sat_family = AF_APPLETALK;
    sat.sat_addr.s_net = ATADDR_ANYNET;
    sat.sat_addr.s_node = ATADDR_ANYNODE;
    sat.sat_port = ATADDR_ANYPORT;
    atpb.atp_saddr = &sat;
    atpb.atp_rreqdata = asp->cmdbuf;
    atpb.atp_rreqdlen = sizeof(asp->cmdbuf);

    while (atp_rreq(asp->asp_atp, &atpb) < 0) {
        if (errno == EINTR || errno == EAGAIN) {
            continue;
        }

        return NULL;
    }

    switch (asp->cmdbuf[0]) {
    case ASPFUNC_TICKLE:
        sid = (unsigned char)asp->cmdbuf[1];

        if (sid >= children->servch_nsessions) {
            break;
        }

        if ((asp_ac[sid] != NULL) && (asp_ac[sid]->ac_state != ACSTATE_DEAD)) {
            asp_ac[sid]->ac_state = ACSTATE_OK;
        }

        break;

    case ASPFUNC_STAT :
#ifdef EBUG
        printf("asp stat\n");
#endif /* EBUG */

        if (asp->asp_slen > 0) {
            i = 0;

            while (atpb.atp_bitmap) {
                i++;
                atpb.atp_bitmap >>= 1;
            }

            /* asp->data is big enough ... */
            memcpy(asp->data, asp->asp_status, MIN(asp->asp_slen, i * ASP_CMDSIZ));
            buflen = MIN(asp->asp_slen, i * ASP_CMDSIZ);
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
                iov[iovcnt].iov_base = buf;
                memmove(buf + ASP_HDRSIZ, buf, buflen);
                memset(iov[iovcnt].iov_base, 0, ASP_HDRSIZ);

                if (buflen > ASP_CMDSIZ) {
                    buf += ASP_CMDMAXSIZ;
                    buflen -= ASP_CMDSIZ;
                    iov[iovcnt].iov_len = ASP_CMDMAXSIZ;
                } else {
                    iov[iovcnt].iov_len = buflen + ASP_HDRSIZ;
                    buflen = 0;
                }

                iovcnt++;
            } while (iovcnt < i && buflen > 0);

            atpb.atp_sresiovcnt = iovcnt;
            atpb.atp_sresiov = iov;
            atp_sresp(asp->asp_atp, &atpb);
        }

        break;

    case ASPFUNC_OPEN :
        if (children->servch_count < children->servch_nsessions) {
            struct asp_child    *asp_ac_tmp;

            /* find a slot */
            for (sid = 0; sid < children->servch_nsessions; sid++) {
                if (asp_ac[sid] == NULL) {
                    break;
                }

                if (asp_ac[sid]->ac_state == ACSTATE_DEAD) {
                    free(asp_ac[sid]);
                    asp_ac[sid] = NULL;
                    break;
                }
            }

            if ((atp = atp_open(ATADDR_ANYPORT,
                                &(atp_sockaddr(asp->asp_atp)->sat_addr))) == NULL) {
                return NULL;
            }

            int ipc_fds[2] = { -1, -1 };
            int hint_pipe[2] = { -1, -1 };
            int ipc_ok = 0;

            if (socketpair(PF_UNIX, SOCK_STREAM, 0, ipc_fds) < 0) {
                LOG(log_error, logtype_default,
                    "asp_getsess: socketpair: %s", strerror(errno));
            } else if (pipe(hint_pipe) < 0) {
                LOG(log_error, logtype_default,
                    "asp_getsess: pipe: %s", strerror(errno));
                close(ipc_fds[0]);
                close(ipc_fds[1]);
                ipc_fds[0] = ipc_fds[1] = -1;
            } else if (setnonblock(ipc_fds[0], 1) != 0
                       || setnonblock(ipc_fds[1], 1) != 0
                       || setnonblock(hint_pipe[0], 1) != 0
                       || setnonblock(hint_pipe[1], 1) != 0) {
                LOG(log_error, logtype_default,
                    "asp_getsess: setnonblock: %s", strerror(errno));
                close(ipc_fds[0]);
                close(ipc_fds[1]);
                close(hint_pipe[0]);
                close(hint_pipe[1]);
                ipc_fds[0] = ipc_fds[1] = -1;
                hint_pipe[0] = hint_pipe[1] = -1;
            } else {
                ipc_ok = 1;
            }

            if (!ipc_ok) {
                atp_close(atp);
                asp->cmdbuf[0] = 0;
                asp->cmdbuf[1] = 0;
                asperr = ASPERR_SERVBUSY;
                memcpy(asp->cmdbuf + 2, &asperr, sizeof(asperr));
                iov[0].iov_base = asp->cmdbuf;
                iov[0].iov_len = 4;
                atpb.atp_sresiov = iov;
                atpb.atp_sresiovcnt = 1;
                atp_sresp(asp->asp_atp, &atpb);
                break;
            }

            switch ((pid = fork())) {
            case 0 : /* child */
                server_reset_signal();
                asp->asp_cnx_cnt = children->servch_count;
                asp->asp_cnx_max = children->servch_nsessions;

                /* free/close some things */
                for (i = 0; i < children->servch_nsessions; i++) {
                    if (asp_ac[i] != NULL) {
                        free(asp_ac[i]);
                    }
                }

                free(asp_ac);
                server_child_free(children);
                children = NULL;
                atp_close(asp->asp_atp);
                close(ipc_fds[0]);
                close(hint_pipe[1]);
                /* the master's spare descriptor, which only it needs */
                dsi_close_spare_fd();
                asp->child = 1;
                asp->asp_atp = atp;
                asp->asp_sat = sat;
                asp->asp_wss = asp->cmdbuf[1];
                asp->asp_seq = 0;
                asp->asp_sid = sid;
                asp->asp_flags = ASPFL_SSS;
                asp->asp_ipc_fd = ipc_fds[1];
                asp->asp_hint_fd = hint_pipe[0];
                return asp;

            case -1 : /* error */
                close(ipc_fds[0]);
                close(ipc_fds[1]);
                close(hint_pipe[0]);
                close(hint_pipe[1]);
                atp_close(atp);
                asp->cmdbuf[0] = 0;
                asp->cmdbuf[1] = 0;
                asperr = ASPERR_SERVBUSY;
                break;

            default : /* parent process */
                close(ipc_fds[1]);
                close(hint_pipe[0]);

                /* we need atomic setting or pb with tickle_handler */
                if ((child = server_child_add(children, pid, ipc_fds[0],
                                              hint_pipe[1]))) {
                    if ((asp_ac_tmp = malloc(sizeof(struct asp_child))) == NULL) {
                        /* Roll back the just-added entry; server_child_remove
                         * closes both the IPC and hint fds and frees the
                         * record, so we don't leak the parent ends. */
                        (void)server_child_remove(children, pid);
                        kill(pid, SIGQUIT);
                        asp->cmdbuf[0] = 0;
                        asp->cmdbuf[1] = 0;
                        asperr = ASPERR_SERVBUSY;
                        break;
                    }

                    asp_ac_tmp->ac_pid = pid;
                    asp_ac_tmp->ac_state = ACSTATE_OK;
                    asp_ac_tmp->ac_sat = sat;
                    asp_ac_tmp->ac_sat.sat_port = asp->cmdbuf[1];
                    asp->cmdbuf[0] = atp_sockaddr(atp)->sat_port;
                    asp->cmdbuf[1] = sid;
                    set_asp_ac(sid, asp_ac_tmp);

                    if (childp) {
                        *childp = child;
                    }

                    asperr = ASPERR_OK;
                } else {
                    close(ipc_fds[0]);
                    close(hint_pipe[1]);
                    kill(pid, SIGQUIT);
                    asp->cmdbuf[0] = 0;
                    asp->cmdbuf[1] = 0;
                    asperr = ASPERR_SERVBUSY;
                }

                atp_close(atp);
                break;
            }
        } else {
            asp->cmdbuf[0] = asp->cmdbuf[1] = 0;
            asperr = ASPERR_SERVBUSY;
        }

        memcpy(asp->cmdbuf + 2, &asperr, sizeof(asperr));
        iov[0].iov_base = asp->cmdbuf;
        iov[0].iov_len = 4;
        atpb.atp_sresiov = iov;
        atpb.atp_sresiovcnt = 1;
        atp_sresp(asp->asp_atp, &atpb);
        break;

    default:
        LOG(log_info, logtype_default, "ASPUnknown %d", asp->cmdbuf[0]);
        break;
    }

    return asp;
}

/*!
 * @brief Store a child record in the session slot table
 *
 * The store sits in a separate function, defined after its use, to keep the
 * compiler from moving it ahead of the record setup. The SIGALRM handler
 * tickle_handler reads the table at any time.
 *
 * @param[in] sid  session slot
 * @param[in] tmp  filled child record
 */
static void set_asp_ac(int sid, struct asp_child *tmp)
{
    asp_ac[sid] = tmp;
}
