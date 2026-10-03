/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atalk/dsi.h>
#include <atalk/logger.h>
#include <atalk/server_child.h>
#include <atalk/unix.h>
#include <atalk/util.h>

/* descriptor held in reserve so a master out of descriptors can still accept */
static int dsi_spare_fd = -1;
/* the master's descriptor limit, read when the spare is reserved */
static int dsi_fdlimit = 0;
/* connections refused for lack of descriptors, for log backoff */
static unsigned int dsi_starved_refusals = 0;
/* descriptor-use warning thresholds reached, see usage_level_update() */
static int dsi_fd_level = 0;

/*!
 * @brief Hold one descriptor in reserve for refusing connections
 *
 * A master with no descriptor left closes the spare to accept the waiting
 * connection and refuse it, so the connection does not stay queued with
 * poll() reporting the listening socket ready again at once.
 */
void dsi_reserve_spare_fd(void)
{
    dsi_fdlimit = getdtablesize();

    if (dsi_spare_fd < 0) {
        dsi_spare_fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
}

/*!
 * @brief Release the spare descriptor, in a session child or before a refusal
 */
void dsi_close_spare_fd(void)
{
    if (dsi_spare_fd >= 0) {
        close(dsi_spare_fd);
        dsi_spare_fd = -1;
    }
}

/*!
 * @brief Accept the waiting connection and reply that the server is busy
 *
 * @returns false when accept() fails, with errno set
 *
 * The reply echoes the client's request ID when its DSIOpenSession header
 * has already arrived, else uses 0.
 *
 * @param[in,out] dsi  listening DSI handle
 */
static bool dsi_reject_busy(DSI *dsi)
{
    socklen_t addrlen = sizeof(dsi->client);
    uint8_t block[DSI_BLOCKSIZ];
    ssize_t n;
    dsi->socket = accept(dsi->serversock,
                         (struct sockaddr *)&dsi->client, &addrlen);

    if (dsi->socket < 0) {
        return false;
    }

    n = recv(dsi->socket, block, DSI_BLOCKSIZ, MSG_DONTWAIT);
    memset(&dsi->header, 0, sizeof(dsi->header));
    dsi->header.dsi_flags = DSIFL_REPLY;
    dsi->header.dsi_command = DSIFUNC_OPEN;

    if (n == DSI_BLOCKSIZ) {
        memcpy(&dsi->header.dsi_requestID, block + 2,
               sizeof(dsi->header.dsi_requestID));
    } else {
        dsi->header.dsi_requestID = 0;
    }

    dsi->header.dsi_data.dsi_code = htonl(DSIERR_SERVBUSY);
    dsi->cmdlen = 0;
    dsi_send(dsi);
    close(dsi->socket);
    dsi->socket = -1;
    return true;
}

/*!
 * @brief Refuse the waiting connection when the master is out of descriptors
 *
 * @param[in,out] dsi       listening DSI handle
 * @param[in]     children  the master's session table
 * @param[in]     err       errno of the failed descriptor allocation
 */
static void dsi_refuse_starved(DSI *dsi, const server_child_t *children,
                               int err)
{
    dsi_close_spare_fd();
    dsi_reject_busy(dsi);
    dsi_reserve_spare_fd();

    if (log_backoff(&dsi_starved_refusals)) {
        int fdlimit;
        int fds = count_open_fds(&fdlimit);
        LOG(log_error, logtype_dsi,
            "dsi_getsess: %s, refused a connection as busy (refusal %u): "
            "%d sessions, %d fds open of %d", strerror(err),
            dsi_starved_refusals, children->servch_count, fds, fdlimit);
    }

    errno = err;
}

/*!
 * @brief Warn as the master's descriptor use climbs toward its limit
 *
 * accept(2) returns the lowest free descriptor, so the accepted socket's
 * number is a lower bound on the master's descriptors in use, at no cost.
 *
 * @param[in] children  the master's session table
 * @param[in] fd        descriptor the master just accepted
 */
static void dsi_note_master_fd(const server_child_t *children, int fd)
{
    if (usage_level_update(&dsi_fd_level, fd + 1, dsi_fdlimit)) {
        int fdlimit;
        int fds = count_open_fds(&fdlimit);
        LOG(log_warning, logtype_dsi,
            "master fd use over %d%% of the limit: %d sessions, "
            "%d fds open of %d", usage_level_pct(dsi_fd_level),
            children->servch_count, fds, fdlimit);
    }
}

/*!
 * @brief Start a DSI session, fork an afpd process
 *
 * @param[in,out] dsi           DSI structure
 * @param[in,out] serv_children pointer to our structure with all childs
 * @param[in] tickleval         tickle interval in seconds
 * @param[out] childp           after fork: parent return pointer to child, child returns NULL
 * @returns                     0 on success, any other value denotes failure
 */
int dsi_getsession(DSI *dsi, server_child_t *serv_children, int tickleval,
                   afp_child_t **childp)
{
    pid_t pid;
    int ipc_fds[2];
    int hint_pipe[2];  /* Dedicated pipe for parent→child dircache hint delivery */
    afp_child_t *child;

    /* Pre-fork session limit check: reject without forking to avoid a
     * fork+SIGKILL cycle per over-cap connection.  MSG_DONTWAIT keeps the
     * parent non-blocking; if the client's DSIOpenSession header hasn't
     * arrived yet we still send SERVBUSY but with requestID=0 instead of
     * echoing the client's ID.  The post-fork check in server_child_add
     * remains as defence-in-depth for the narrow race window. */
    if (serv_children->servch_count >= serv_children->servch_nsessions) {
        LOG(log_note, logtype_dsi,
            "dsi_getsess: at session cap (%d/%d), rejecting without fork",
            serv_children->servch_count, serv_children->servch_nsessions);

        /* out of descriptors as well, the spare is what lets it refuse */
        if (!dsi_reject_busy(dsi) && (errno == EMFILE || errno == ENFILE)) {
            dsi_refuse_starved(dsi, serv_children, errno);
            return -1;
        }

        errno = 0;
        return -1;
    }

    if (socketpair(PF_UNIX, SOCK_STREAM, 0, ipc_fds) < 0) {
        int err = errno;

        if (err == EMFILE || err == ENFILE) {
            dsi_refuse_starved(dsi, serv_children, err);
        } else {
            LOG(log_error, logtype_dsi, "dsi_getsess: %s", strerror(err));
        }

        return -1;
    }

    if (setnonblock(ipc_fds[0], 1) != 0 || setnonblock(ipc_fds[1], 1) != 0) {
        LOG(log_error, logtype_dsi, "dsi_getsess: setnonblock: %s", strerror(errno));
        close(ipc_fds[0]);
        close(ipc_fds[1]);
        return -1;
    }

    /* Create dedicated pipe for parent→child dircache hint delivery.
     * Separate from IPC socketpair to avoid interfering with session
     * transfer send_fd()/recv_fd() + SIGURG signaling. Pipe writes
     * ≤ PIPE_BUF (4096) are POSIX-guaranteed atomic — our 22-byte
     * dircache hint messages always arrive as complete messages. */
    if (pipe(hint_pipe) < 0) {
        int err = errno;
        close(ipc_fds[0]);
        close(ipc_fds[1]);

        if (err == EMFILE || err == ENFILE) {
            dsi_refuse_starved(dsi, serv_children, err);
        } else {
            LOG(log_error, logtype_dsi, "dsi_getsess: pipe: %s", strerror(err));
        }

        return -1;
    }

    /* Set both pipe ends non-blocking to prevent parent relay from
     * blocking on a slow/full child pipe, and child from blocking
     * when no hints are queued */
    if (setnonblock(hint_pipe[0], 1) != 0 || setnonblock(hint_pipe[1], 1) != 0) {
        LOG(log_error, logtype_dsi, "dsi_getsess: hint pipe setnonblock: %s",
            strerror(errno));
        close(ipc_fds[0]);
        close(ipc_fds[1]);
        close(hint_pipe[0]);
        close(hint_pipe[1]);
        return -1;
    }

    LOG(log_debug, logtype_dsi, "dsi_getsess: about to fork child (proto_open)");

    switch (pid = dsi->proto_open(dsi)) { /* in libatalk/dsi/dsi_tcp.c */
    case -1: {
        int err = errno;
        close(ipc_fds[0]);
        close(ipc_fds[1]);
        close(hint_pipe[0]);
        close(hint_pipe[1]);

        /* accept() succeeded but fork() did not */
        if (dsi->socket >= 0) {
            close(dsi->socket);
            dsi->socket = -1;
        }

        if (err == EMFILE || err == ENFILE) {
            dsi_refuse_starved(dsi, serv_children, err);
        } else {
            LOG(log_error, logtype_dsi, "dsi_getsess: %s", strerror(err));
        }

        return -1;
    }

    case 0: /* child. mostly handled below. */
        LOG(log_debug, logtype_dsi, "dsi_getsess: child process started (pid %d)",
            getpid());
        break;

    default: /* parent */
        LOG(log_debug, logtype_dsi, "dsi_getsess: forked child pid %d", pid);
        /* using SIGKILL is hokey, but the child might not have
         * re-established its signal handler for SIGTERM yet. */
        close(ipc_fds[1]);
        close(hint_pipe[0]);

        if ((child = server_child_add(serv_children, pid, ipc_fds[0],
                                      hint_pipe[1])) == NULL) {
            LOG(log_error, logtype_dsi, "dsi_getsess: %s", strerror(errno));
            close(ipc_fds[0]);
            close(hint_pipe[1]);
            dsi->header.dsi_flags = DSIFL_REPLY;
            dsi->header.dsi_data.dsi_code = htonl(DSIERR_SERVBUSY);
            dsi_send(dsi);
            dsi->header.dsi_data.dsi_code = DSIERR_OK;
            kill(pid, SIGKILL);
            dsi->proto_close(dsi);
            return -1;
        }

        dsi_note_master_fd(serv_children, dsi->socket);
        dsi->proto_close(dsi);
        *childp = child;
        return 0;
    }

    /* Cache forked child PID and EUID at fork time.
     * PID never changes for this child process.
     * EUID is set to whatever the process inherited,
     * EUID is then updated again after login in auth.c to reflect the logged-in user. */
    dsi->AFPobj->pid = getpid();
    dsi->AFPobj->euid = geteuid();
    /* Save number of existing and maximum connections */
    dsi->AFPobj->cnx_cnt = serv_children->servch_count;
    dsi->AFPobj->cnx_max = serv_children->servch_nsessions;
    /* get rid of some stuff */
    dsi->AFPobj->ipc_fd = ipc_fds[1];
    dsi->AFPobj->hint_fd = hint_pipe[0];
    close(ipc_fds[0]);
    close(hint_pipe[1]);
    close(dsi->serversock);
    dsi->serversock = -1;
    dsi_close_spare_fd();
    server_child_free(serv_children);

    switch (dsi->header.dsi_command) {
    case DSIFUNC_STAT: { /* send off status and return */
        /* OpenTransport 1.1.2 bug workaround:
         *
         * OT code doesn't currently handle close sockets well. urk.
         * the workaround: wait for the client to close its
         * side. timeouts prevent indefinite resource use.
         */
        static struct timeval timeout = {120, 0};
        fd_set readfds;
        dsi_getstatus(dsi);

        if (dsi->socket >= FD_SETSIZE) {
            free(dsi);
            exit(0);
        }

        FD_ZERO(&readfds);
        FD_SET(dsi->socket, &readfds);
        free(dsi);
        select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
        exit(0);
    }

    case DSIFUNC_OPEN: /* setup session */
        LOG(log_debug, logtype_dsi,
            "dsi_getsess: child setting up session (DSIFUNC_OPEN)");
        /* set up the tickle timer */
        dsi->timer.it_interval.tv_sec = dsi->timer.it_value.tv_sec = tickleval;
        dsi->timer.it_interval.tv_usec = dsi->timer.it_value.tv_usec = 0;
        dsi_opensession(dsi);
        LOG(log_debug, logtype_dsi,
            "dsi_getsess: child session opened, returning to afp_over_dsi");
        *childp = NULL;
        return 0;

    default: /* just close */
        LOG(log_warning, logtype_dsi, "DSIUnknown %d", dsi->header.dsi_command);
        dsi->proto_close(dsi);
        exit(EXITERR_CLNT);
    }
}
