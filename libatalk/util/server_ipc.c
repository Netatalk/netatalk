/*
 * All rights reserved. See COPYRIGHT.
 */

/*!
 * @file
 * IPC over socketpair between parent and children.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <atalk/dsi.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/server_child.h>
#include <atalk/server_ipc.h>
#include <atalk/util.h>

/* Build-time safety: hint message must fit within PIPE_BUF for atomic pipe delivery */
_Static_assert(IPC_HEADERLEN + sizeof(struct ipc_cache_hint_payload) <=
               PIPE_BUF,
               "Cache hint message must fit within PIPE_BUF for atomic pipe delivery");

/* Build-time safety: verify IPC_HEADERLEN matches actual wire format field sizes */
_Static_assert(IPC_HEADERLEN == sizeof(uint16_t) + sizeof(pid_t) + sizeof(
                   uid_t) + sizeof(uint32_t),
               "IPC_HEADERLEN must match sum of wire format field sizes");

typedef struct ipc_header {
    uint16_t command;
    pid_t	 child_pid;
    uid_t    uid;
    uint32_t len;
    char 	 *msg;
    int      afp_socket;
    uint16_t DSI_requestID;
} ipc_header_t;

static char *ipc_cmd_str[] = { "IPC_DISCOLDSESSION",
                               "IPC_GETSESSION",
                               "IPC_STATE",
                               "IPC_VOLUMES",
                               "IPC_LOGINDONE",
                               "IPC_CACHE_HINT"
                             };

/*!
 * @brief Pass afp_socket to old disconnected session if one has a matching token (token = pid)
 * @returns -1 on error, 0 if no matching session was found, 1 if session was found and socket passed
 */
static int ipc_kill_token(struct ipc_header *ipc, server_child_t *children)
{
    pid_t pid;

    if (ipc->len != sizeof(pid_t)) {
        return -1;
    }

    /* assume signals SA_RESTART set */
    memcpy(&pid, ipc->msg, sizeof(pid_t));
    return server_child_transfer_session(children,
                                         pid,
                                         ipc->uid,
                                         ipc->afp_socket,
                                         ipc->DSI_requestID);
}

/* ----------------- */
static int ipc_get_session(struct ipc_header *ipc, server_child_t *children)
{
    uint32_t boottime;
    uint32_t idlen;
    char     *clientid, *p;

    if (ipc->len < (sizeof(idlen) + sizeof(boottime))) {
        return -1;
    }

    p = ipc->msg;
    memcpy(&idlen, p, sizeof(idlen));
    idlen = ntohl(idlen);
    p += sizeof(idlen);
    memcpy(&boottime, p, sizeof(boottime));
    p += sizeof(boottime);

    if (ipc->len < idlen + sizeof(idlen) + sizeof(boottime)) {
        return -1;
    }

    if (NULL == (clientid = (char *) malloc(idlen))) {
        return -1;
    }

    memcpy(clientid, p, idlen);
    LOG(log_debug, logtype_afpd, "ipc_get_session(pid: %u, uid: %u, time: 0x%08x)",
        ipc->child_pid, ipc->uid, boottime);
    server_child_kill_one_by_id(children,
                                ipc->child_pid,
                                ipc->uid,
                                idlen,
                                clientid,
                                boottime);
    return 0;
}

/* ----------------- */
static int ipc_login_done(const struct ipc_header *ipc,
                          server_child_t *children)
{
    LOG(log_debug, logtype_afpd, "ipc_login_done(pid: %u, uid: %u)",
        ipc->child_pid, ipc->uid);
    server_child_login_done(children,
                            ipc->child_pid,
                            ipc->uid,
                            ipc->msg);
    return 0;
}

static int ipc_set_state(struct ipc_header *ipc, server_child_t *children)
{
    EC_INIT;
    afp_child_t *child;
    pthread_mutex_lock(&children->servch_lock);

    if ((child = server_child_resolve(children, ipc->child_pid)) == NULL) {
        EC_FAIL;
    }

    memcpy(&child->afpch_state, ipc->msg, sizeof(uint16_t));
EC_CLEANUP:
    pthread_mutex_unlock(&children->servch_lock);
    EC_EXIT;
}

static int ipc_set_volumes(struct ipc_header *ipc, server_child_t *children)
{
    EC_INIT;
    afp_child_t *child;
    pthread_mutex_lock(&children->servch_lock);

    if ((child = server_child_resolve(children, ipc->child_pid)) == NULL) {
        EC_FAIL;
    }

    if (child->afpch_volumes) {
        free(child->afpch_volumes);
        child->afpch_volumes = NULL;
    }

    if (ipc->len) {
        child->afpch_volumes = strdup(ipc->msg);
    }

EC_CLEANUP:
    pthread_mutex_unlock(&children->servch_lock);
    EC_EXIT;
}

/*!
 * @brief Relay a dircache hint from one child to all siblings
 *
 * Iterates ALL hash buckets in the child table (CHILD_HASHSIZE = 32).
 * Writes to each sibling's dedicated hint pipe (afpch_hint_fd) using
 * non-blocking write(). Hints dropped on EAGAIN (graceful degradation).
 * Pipe writes ≤ PIPE_BUF are POSIX-guaranteed atomic — our 22-byte
 * messages always arrive complete without framing concerns.
 *
 * Releases servch_lock BEFORE write loop to prevent blocking other IPC
 * operations during relay.
 */
static int ipc_relay_cache_hint(struct ipc_header *ipc,
                                server_child_t *children)
{
    /* Pre-build the wire message OUTSIDE lock — reduces critical section */
    char block[IPC_MAXMSGSIZE];
    char *p = block;
    uint16_t cmd = IPC_CACHE_HINT;
    uint32_t len = ipc->len;
    memset(block, 0, IPC_MAXMSGSIZE);
    memcpy(p, &cmd, sizeof(cmd));
    p += sizeof(cmd);
    memcpy(p, &ipc->child_pid, sizeof(pid_t));
    p += sizeof(pid_t);
    memcpy(p, &ipc->uid, sizeof(uid_t));
    p += sizeof(uid_t);
    memcpy(p, &len, sizeof(uint32_t));
    p += sizeof(uint32_t);
    memcpy(p, ipc->msg, ipc->len);
    ssize_t total_len = IPC_HEADERLEN + ipc->len;
    /* Snapshot sibling hint pipe fds under lock, then release before writing.
     * Sized to configured max sessions for safety margin. */
    int *child_fds = malloc(children->servch_nsessions * sizeof(int));

    if (!child_fds) {
        return 0;    /* OOM: drop hint gracefully */
    }

    int child_count = 0;
    pthread_mutex_lock(&children->servch_lock);

    /* Iterate ALL hash buckets — servch_table is a hash table, not a single list */
    for (int i = 0; i < CHILD_HASHSIZE; i++) {
        for (afp_child_t *child = children->servch_table[i]; child;
                child = child->afpch_next) {
            if (child->afpch_pid != ipc->child_pid && child->afpch_hint_fd >= 0) {
                child_fds[child_count++] = child->afpch_hint_fd;
            }
        }
    }

    pthread_mutex_unlock(&children->servch_lock);  /* Release BEFORE write loop */
    /* Write to all siblings' hint pipes WITHOUT holding lock.
     * Pipe writes ≤ PIPE_BUF are POSIX-guaranteed atomic. */
    LOG(log_debug, logtype_afpd,
        "ipc_relay_cache_hint: relaying to %d siblings", child_count);

    for (int i = 0; i < child_count; i++) {
        ssize_t ret = write(child_fds[i], block, total_len);

        if (ret > 0 && ret != total_len) {
            /* Partial write should never happen for ≤ PIPE_BUF writes */
            LOG(log_warning, logtype_afpd,
                "ipc_relay_cache_hint: partial write (%zd of %zd)", ret, total_len);
        } else if (ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            /* EBADF/EPIPE: child crashed or closed pipe — non-fatal */
            LOG(log_debug, logtype_afpd,
                "ipc_relay_cache_hint: write failed fd=%d: %s",
                child_fds[i], strerror(errno));
        }

        /* EAGAIN/EWOULDBLOCK: pipe buffer full, hint silently dropped */
    }

    free(child_fds);
    /* Always return 0 — relay failures are handled gracefully via drop */
    return 0;
}

/***********************************************************************************
 * Public functions
 ***********************************************************************************/

/* -----------------
 * Ipc format
 * command
 * pid
 * uid
 *
 */

/*!
 * @brief Read a IPC message from a child
 *
 * This is using an fd with non-blocking IO, so EAGAIN is not an error
 *
 * @param[in,out] children   pointer to our structure with all childs
 * @param[in] fd             IPC socket with child
 *
 * @returns -1 on error, 0 on success
 */
int ipc_server_read(server_child_t *children, int fd)
{
    int       ret;
    struct ipc_header ipc;
    char      buf[IPC_MAXMSGSIZE], *p;

    if ((ret = read(fd, buf, IPC_HEADERLEN)) != IPC_HEADERLEN) {
        if (ret != 0) {
            if (errno == EAGAIN) {
                return 0;
            }

            LOG(log_error, logtype_afpd,
                "Reading IPC header failed (%i of %u bytes read): %s",
                ret, IPC_HEADERLEN, strerror(errno));
        }

        return -1;
    }

    p = buf;
    memcpy(&ipc.command, p, sizeof(ipc.command));
    p += sizeof(ipc.command);
    memcpy(&ipc.child_pid, p, sizeof(ipc.child_pid));
    p += sizeof(ipc.child_pid);
    memcpy(&ipc.uid, p, sizeof(ipc.uid));
    p += sizeof(ipc.uid);
    memcpy(&ipc.len, p, sizeof(ipc.len));

    /* This should never happen */
    if (ipc.len > (IPC_MAXMSGSIZE - IPC_HEADERLEN)) {
        LOG(log_info, logtype_afpd, "IPC message exceeds allowed size (%u)", ipc.len);
        return -1;
    }

    memset(buf, 0, IPC_MAXMSGSIZE);

    if (ipc.len != 0) {
        if ((ret = read(fd, buf, ipc.len)) != (int) ipc.len) {
            LOG(log_info, logtype_afpd,
                "Reading IPC message failed (%u of %u  bytes read): %s",
                ret, ipc.len, strerror(errno));
            return -1;
        }
    }

    ipc.msg = buf;

    /* Bounds check before accessing ipc_cmd_str[] — corrupted IPC messages with
     * command > array size would cause undefined behavior without this guard */
    if (ipc.command >= sizeof(ipc_cmd_str) / sizeof(ipc_cmd_str[0])) {
        LOG(log_warning, logtype_afpd,
            "ipc_server_read: unknown command %u from pid %u",
            ipc.command, ipc.child_pid);
        return 0;  /* Don't destroy IPC channel for one unknown message */
    }

    LOG(log_debug, logtype_afpd, "ipc_server_read(%s): pid: %u",
        ipc_cmd_str[ipc.command], ipc.child_pid);

    switch (ipc.command) {
    case IPC_DISCOLDSESSION:
        if (readt(fd, &ipc.DSI_requestID, 2, 0, 2) != 2) {
            LOG(log_error, logtype_afpd,
                "ipc_read(%s:child[%u]): couldn't read DSI id: %s",
                ipc_cmd_str[ipc.command], ipc.child_pid, strerror(errno));
            return -1;
        }

        if ((ipc.afp_socket = recv_fd(fd, 1)) == -1) {
            LOG(log_error, logtype_afpd, "ipc_read(%s:child[%u]): recv_fd: %s",
                ipc_cmd_str[ipc.command], ipc.child_pid, strerror(errno));
            return -1;
        }

        if (ipc_kill_token(&ipc, children) == 1) {
            /* Transfered session (ie afp_socket) to old disconnected child, now kill the new one */
            LOG(log_note, logtype_afpd,
                "Reconnect: killing new session child[%u] after transfer",
                ipc.child_pid);
            kill(ipc.child_pid, SIGTERM);
        }

        close(ipc.afp_socket);
        break;

    case IPC_GETSESSION:
        if (ipc_get_session(&ipc, children) != 0) {
            return -1;
        }

        break;

    case IPC_STATE:
        if (ipc_set_state(&ipc, children) != 0) {
            return -1;
        }

        break;

    case IPC_VOLUMES:
        if (ipc_set_volumes(&ipc, children) != 0) {
            return -1;
        }

        break;

    case IPC_LOGINDONE:
        if (ipc_login_done(&ipc, children) != 0) {
            return -1;
        }

        break;

    case IPC_CACHE_HINT:
        /* Relay dircache hint to all children except the source */
        ipc_relay_cache_hint(&ipc, children);
        break;

    default:
        /* Don't destroy IPC channel for unrecognized commands */
        LOG(log_error, logtype_afpd, "ipc_read: unhandled command: %d", ipc.command);
        return 0;
    }

    return 0;
}

/* ----------------- */
int ipc_child_write(int fd, uint16_t command, size_t len, void *msg)
{
    char block[IPC_MAXMSGSIZE], *p;
    pid_t pid;
    uid_t uid;
    ssize_t ret;
    p = block;
    memset(p, 0, IPC_MAXMSGSIZE);

    if (len + IPC_HEADERLEN > IPC_MAXMSGSIZE) {
        return -1;
    }

    memcpy(p, &command, sizeof(command));
    p   += sizeof(command);
    pid = getpid();
    memcpy(p, &pid, sizeof(pid_t));
    p += sizeof(pid_t);
    /* FIXME
     * using uid is wrong. It will not disconnect if the new connection
     * is with a different user.
     * But we really don't want a remote kill command.
    */
    uid = geteuid();
    memcpy(p, &uid, sizeof(uid_t));
    p += sizeof(uid_t);
    memcpy(p, &len, 4);
    p += 4;
    memcpy(p, msg, len);

    /* Bounds check before accessing ipc_cmd_str[] */
    if (command < sizeof(ipc_cmd_str) / sizeof(ipc_cmd_str[0])) {
        LOG(log_debug, logtype_afpd, "ipc_child_write(%s)", ipc_cmd_str[command]);
    } else {
        LOG(log_debug, logtype_afpd, "ipc_child_write(cmd=%u)", command);
    }

    if ((ret = writet(fd, block, len + IPC_HEADERLEN, 0,
                      1)) != len + IPC_HEADERLEN) {
        return -1;
    }

    return 0;
}

int ipc_child_state(AFPObj *obj, uint16_t state)
{
    return ipc_child_write(obj->ipc_fd, IPC_STATE, sizeof(uint16_t), &state);
}

/***********************************************************************************
 * Cross-process dircache hint sender (child → parent via IPC socketpair)
 ***********************************************************************************/

/* Sender-side statistics counter — exposed via getter for log_dircache_stat() */
static unsigned long long hints_sent = 0;

unsigned long long ipc_get_hints_sent(void)
{
    return hints_sent;
}

/*!
 * @brief Send a dircache invalidation hint from child to parent
 *
 * Called directly from AFP command handlers that modify dircache state.
 * Independent of the external FCE system — always active when IPC is available.
 *
 * @param[in] obj    AFPObj with ipc_fd
 * @param[in] vid    Volume ID (network byte order, matches vol->v_vid)
 * @param[in] cnid   CNID of affected file/dir (network byte order)
 * @param[in] event  Hint type: CACHE_HINT_REFRESH, CACHE_HINT_DELETE,
 *                   or CACHE_HINT_DELETE_CHILDREN
 * @returns 0 on success, -1 on error
 */
int ipc_send_cache_hint(const AFPObj *obj, uint16_t vid, cnid_t cnid,
                        uint8_t event)
{
    if (obj->ipc_fd < 0) {
        return 0;    /* No IPC channel */
    }

    /* Both vid and cnid are already network byte order in the codebase */
    struct ipc_cache_hint_payload hint = {
        .event = event,
        .reserved = 0,
        .vid = vid,
        .cnid = cnid,
    };
    /* Uses existing ipc_child_write() which handles header construction
     * and stamps our PID in the header for self-filtering by the relay */
    int ret = ipc_child_write(obj->ipc_fd, IPC_CACHE_HINT,
                              sizeof(hint), &hint);

    if (ret == 0) {
        hints_sent++;
        LOG(log_debug, logtype_afpd,
            "ipc_send_cache_hint: sent %s for vid:%u did:%u",
            event == CACHE_HINT_REFRESH ? "REFRESH" :
            event == CACHE_HINT_DELETE ? "DELETE" :
            event == CACHE_HINT_DELETE_CHILDREN ? "DELETE_CHILDREN" : "?",
            ntohs(vid), ntohl(cnid));
    }

    return ret;
}
