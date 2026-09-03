/*
 * Copyright (c) 2025-2026 Andy Lemin (andylemin)
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
#include <poll.h>
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
                               "IPC_CACHE_HINT",
                               "IPC_SESSIONTOKEN"
                             };

/***********************************************************************************
 * Cache hint batching infrastructure
 * Main thread appends hints to buffer, poll-driven flush writes to sibling afpd
 * processes — single-threaded, no locks needed
 ***********************************************************************************/

#define HINT_RATE_LIMIT        1000  /* Max hints/second per child (attack guard) */

/* Resets skip the drop paths, so they need their own ceiling. A child raises
 * SIGTERM right after resetting, so it sends at most one. */
#define HINT_RESET_RATE_LIMIT  8

/* Volumes whose reset can be deduped at once — more than a handful resetting
 * inside one second is not a case worth holding memory for */
#define RESET_SEEN_SIZE        8

/* Waiting for pipe space stalls the master, so each flush gets a fixed wait
 * budget regardless of how many siblings are backed up */
#define HINT_RESET_WAIT_BUDGET 4

/* One wait for IPC space when a reset cannot be written at once. The child is
 * the only sender, so a lost write costs every sibling the notification. */
#define HINT_RESET_WAIT_MS     20

/* Per-child rate tracking — fixed array indexed by PID % RATE_TRACK_SIZE.
 * Hash collisions are benign — worst case a child gets a slightly wrong
 * rate count from sharing a slot with another child. */
#define RATE_TRACK_SIZE 256

/* Intentionally NOT packed — natural alignment gives better performance
 * sizeof typically 16 bytes on LP64 due to padding after vid (2→4) and event (1→4). */
struct hint_entry {
    uint16_t vid;
    cnid_t   cnid;
    uint8_t  event;      /* Hint type */
    pid_t    source_pid; /* Exclude source from relay */
};

/* Hint accumulation buffer */
static struct {
    struct hint_entry entries[HINT_BUF_SIZE];
    int count;
} hint_buf;

static struct {
    pid_t  pid;
    time_t window_start;
    int    count_in_window;
    /* Counted apart from the hints above: a reset shares no budget with the
     * best-effort traffic, which a working session emits far faster */
    int    resets_in_window;
} rate_track[RATE_TRACK_SIZE];

/* Parent-side statistics */
static unsigned long long hints_batched = 0;
static unsigned long long hints_rate_dropped = 0;
static unsigned long long flush_count = 0;

/* Start this child's window, or reuse the one already open. Called from main
 * thread only. */
static int rate_slot(pid_t child_pid)
{
    int idx = child_pid % RATE_TRACK_SIZE;
    time_t now = time(NULL);

    if (rate_track[idx].pid != child_pid || rate_track[idx].window_start != now) {
        rate_track[idx].pid = child_pid;
        rate_track[idx].window_start = now;
        rate_track[idx].count_in_window = 0;
        rate_track[idx].resets_in_window = 0;
    }

    return idx;
}

/* Returns current hint count for this child in the current 1-second window. */
static int check_and_increment_rate(pid_t child_pid)
{
    return ++rate_track[rate_slot(child_pid)].count_in_window;
}

static int check_and_increment_reset_rate(pid_t child_pid)
{
    return ++rate_track[rate_slot(child_pid)].resets_in_window;
}

/* Volume tags whose reset was broadcast, and when */
static struct {
    uint32_t tag;
    time_t   when;
} reset_seen[RESET_SEEN_SIZE];

/*!
 * @brief Whether this volume's reset has just been broadcast
 *
 * One notification per volume is all a sibling needs, and each one costs a
 * forced flush to every session, so a repeat inside the same second is
 * dropped. Records the tag when it is new.
 *
 * @param[in] tag  cnid_volume_tag() of the reset volume, network byte order
 */
static int reset_already_broadcast(uint32_t tag)
{
    const time_t now = time(NULL);
    int oldest = 0;

    for (int i = 0; i < RESET_SEEN_SIZE; i++) {
        if (reset_seen[i].tag == tag && reset_seen[i].when == now) {
            return 1;
        }

        if (reset_seen[i].when < reset_seen[oldest].when) {
            oldest = i;
        }
    }

    reset_seen[oldest].tag = tag;
    reset_seen[oldest].when = now;
    return 0;
}

/*!
 * @brief Make a volume's reset broadcastable again
 *
 * A sibling that missed a reset batch has no other redelivery path, so a
 * delivery failure must not leave the tag deduped against the next duplicate.
 *
 * @param[in] tag  cnid_volume_tag() of the reset volume, network byte order
 */
static void reset_broadcast_forget(uint32_t tag)
{
    for (int i = 0; i < RESET_SEEN_SIZE; i++) {
        if (reset_seen[i].tag == tag) {
            reset_seen[i].tag = 0;
            reset_seen[i].when = 0;
        }
    }
}

/*!
 * @brief Write a sibling's hint batch, waiting once if it carries a reset
 *
 * A full sibling pipe drops a best-effort batch: those hints only cost a
 * lookup. A batch carrying a reset waits briefly for pipe space instead, since
 * the sibling would otherwise keep resolving recycled CNIDs. Waits are budgeted
 * per second across flushes so backed-up siblings cannot stall the master.
 *
 * @param[in]     fd          sibling's hint pipe
 * @param[in]     buf         serialized batch
 * @param[in]     len         bytes to write
 * @param[in]     has_reset   batch contains a CACHE_HINT_VOLUME_RESET
 * @param[in,out] waits_left  this second's remaining waits, spent here
 * @returns 0 on success, -1 if the batch was not delivered
 */
static int hint_write_batch(int fd, const char *buf, int len, int has_reset,
                            int *waits_left)
{
    ssize_t ret = write(fd, buf, len);
    int write_errno = errno;

    if (ret == len) {
        return 0;
    }

    if (has_reset && ret == -1 && *waits_left > 0
            && (write_errno == EAGAIN || write_errno == EWOULDBLOCK)) {
        struct pollfd pfd = { .fd = fd, .events = POLLOUT };
        int pret;
        (*waits_left)--;

        do {
            pret = poll(&pfd, 1, HINT_RESET_WAIT_MS);
        } while (pret < 0 && errno == EINTR);

        if (pret > 0 && (pfd.revents & POLLOUT)) {
            ret = write(fd, buf, len);
            write_errno = errno;
        }

        if (ret == len) {
            return 0;
        }
    }

    /* Classified on the write's own errno: poll() has since overwritten it */
    if (ret == -1 && write_errno != EAGAIN && write_errno != EWOULDBLOCK) {
        LOG(log_debug, logtype_afpd, "hint_flush: write failed fd=%d: %s",
            fd, strerror(write_errno));
    } else if (has_reset) {
        LOG(log_warning, logtype_afpd,
            "hint_flush: could not deliver a volume reset to fd=%d; that "
            "session may serve stale CNIDs until it reconnects", fd);
    }

    return -1;
}

/*!
 * @brief Serialize a hint_entry to IPC wire format.
 *
 * Writes IPC_HEADERLEN + sizeof(ipc_cache_hint_payload) = 22 bytes
 * to the output buffer. The header PID/UID fields are set to the
 * source child's PID (for receiver logging) with UID 0.
 *
 * @param[out] buf   Output buffer (must have ≥ 22 bytes available)
 * @param[in]  e     Hint entry to serialize
 * @returns    Number of bytes written (always 22)
 */
static int serialize_hint(char *buf, const struct hint_entry *e)
{
    char *p = buf;
    uint16_t cmd = IPC_CACHE_HINT;
    uint32_t len = sizeof(struct ipc_cache_hint_payload);
    uid_t uid = 0;
    memset(p, 0, IPC_HEADERLEN + sizeof(struct ipc_cache_hint_payload));
    memcpy(p, &cmd, sizeof(cmd));
    p += sizeof(cmd);
    memcpy(p, &e->source_pid, sizeof(pid_t));
    p += sizeof(pid_t);
    memcpy(p, &uid, sizeof(uid_t));
    p += sizeof(uid_t);
    memcpy(p, &len, sizeof(uint32_t));
    p += sizeof(uint32_t);
    struct ipc_cache_hint_payload payload = {
        .event = e->event,
        .reserved = 0,
        .vid = e->vid,
        .cnid = e->cnid,
    };
    memcpy(p, &payload, sizeof(payload));
    return IPC_HEADERLEN + sizeof(struct ipc_cache_hint_payload);
}


/*!
 * @brief Pass afp_socket to old disconnected session if one has a matching token
 * @returns -1 on error, 0 if no matching session was found,
 *          1 if session was found and socket passed
 */
static int ipc_kill_token(struct ipc_header *ipc, server_child_t *children)
{
    if (ipc->len == 0) {
        return -1;
    }

    return server_child_transfer_session(children,
                                         ipc->uid,
                                         ipc->msg,
                                         ipc->len,
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

static int ipc_set_session_token(const struct ipc_header *ipc,
                                 server_child_t *children)
{
    LOG(log_debug, logtype_afpd, "ipc_set_session_token(pid: %u, uid: %u)",
        ipc->child_pid, ipc->uid);
    return server_child_set_session_token(children,
                                          ipc->child_pid,
                                          ipc->uid,
                                          ipc->msg,
                                          ipc->len);
}

static int ipc_set_state(struct ipc_header *ipc, server_child_t *children)
{
    EC_INIT;
    afp_child_t *child;

    if ((child = server_child_resolve(children, ipc->child_pid)) == NULL) {
        EC_FAIL;
    }

    memcpy(&child->afpch_state, ipc->msg, sizeof(uint16_t));
EC_CLEANUP:
    EC_EXIT;
}

static int ipc_set_volumes(struct ipc_header *ipc, server_child_t *children)
{
    EC_INIT;
    afp_child_t *child;

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
    EC_EXIT;
}

/*!
 * @brief Buffer a dircache hint for batched relay to siblings.
 *
 * Appends to hint_buf array. Single-threaded — no lock needed.
 * If buffer is full, the hint is dropped (caller should flush first).
 */
static int ipc_relay_cache_hint(struct ipc_header *ipc,
                                server_child_t *children)
{
    /* Validate payload length before accessing */
    if (ipc->len < sizeof(struct ipc_cache_hint_payload)) {
        LOG(log_warning, logtype_afpd,
            "ipc_relay_cache_hint: short payload (%u < %zu) from pid %u",
            ipc->len, sizeof(struct ipc_cache_hint_payload),
            ipc->child_pid);
        return 0;
    }

    struct ipc_cache_hint_payload hint;

    memcpy(&hint, ipc->msg, sizeof(hint));

    /* Skip relay when no siblings exist */
    if (children->servch_count <= 1) {
        return 0;
    }

    /* Validate event type */
    if (hint.event >= CACHE_HINT_COUNT) {
        LOG(log_warning, logtype_afpd,
            "ipc_relay_cache_hint: invalid event %u from pid %u, dropped",
            hint.event, ipc->child_pid);
        return 0;
    }

    /* Losing a reset leaves siblings resolving recycled CNIDs, so it skips the
     * drop paths below — but not the guard itself, or one child could force
     * unmetered disconnection of every session on the volume. */
    const int is_reset = (hint.event == CACHE_HINT_VOLUME_RESET);

    if (is_reset) {
        if (check_and_increment_reset_rate(ipc->child_pid)
                > HINT_RESET_RATE_LIMIT) {
            hints_rate_dropped++;
            LOG(log_warning, logtype_afpd,
                "ipc_relay_cache_hint: pid %u exceeded the reset limit, dropped",
                ipc->child_pid);
            return 0;
        }

        /* Siblings need to hear a volume was reset, not how often. Deduped
         * against what was already broadcast, not against hint_buf: each reset
         * forces a flush below, so the buffer never holds one by the time the
         * next arrives. Keyed on the tag alone — vid is per-process, so two
         * children reporting one volume disagree on it. */
        if (reset_already_broadcast(hint.cnid)) {
            return 0;
        }
    }

    /* Rate-limit check (attack guard) */
    if (!is_reset && check_and_increment_rate(ipc->child_pid) > HINT_RATE_LIMIT) {
        hints_rate_dropped++;

        if (rate_track[ipc->child_pid % RATE_TRACK_SIZE].count_in_window
                == HINT_RATE_LIMIT + 1) {
            LOG(log_warning, logtype_afpd,
                "ipc_relay_cache_hint: rate limit exceeded for pid %u",
                ipc->child_pid);
        }

        return 0;
    }

    /* Buffer full — drop hint. Caller will flush after ipc_server_read returns. */
    if (hint_buf.count >= HINT_BUF_SIZE) {
        if (!is_reset) {
            LOG(log_debug, logtype_afpd,
                "ipc_relay_cache_hint: buffer full, hint dropped from pid %u",
                ipc->child_pid);
            return 0;
        }

        /* A best-effort hint yields its slot: flushing first would send the
         * batch the reset has to precede */
        hint_buf.count--;
    }

    hint_buf.entries[hint_buf.count] = (struct hint_entry) {
        .vid = hint.vid,
        .cnid = hint.cnid,
        .event = hint.event,
        .source_pid = ipc->child_pid,
    };
    hint_buf.count++;

    if (is_reset) {
        /* A busy master can starve the batching trigger indefinitely */
        hint_flush_pending(children);
    }

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

    case IPC_SESSIONTOKEN:
        if (ipc_set_session_token(&ipc, children) != 0) {
            return -1;
        }

        break;

    default:
        /* Don't destroy IPC channel for unrecognized commands */
        LOG(log_error, logtype_afpd, "ipc_read: unhandled command: %d", ipc.command);
        return 0;
    }

    return 0;
}

/* ----------------- */
int ipc_child_write(AFPObj *obj, uint16_t command, size_t len, void *msg)
{
    char block[IPC_MAXMSGSIZE], *p;
    uint32_t msglen;
    p = block;
    memset(p, 0, IPC_MAXMSGSIZE);

    if (len + IPC_HEADERLEN > IPC_MAXMSGSIZE) {
        return -1;
    }

    msglen = (uint32_t)len;
    memcpy(p, &command, sizeof(command));
    p   += sizeof(command);
    memcpy(p, &obj->pid, sizeof(pid_t));
    p += sizeof(pid_t);
    /* FIXME
     * using uid is wrong. It will not disconnect if the new connection
     * is with a different user.
     * But we really don't want a remote kill command.
    */
    memcpy(p, &obj->euid, sizeof(uid_t));
    p += sizeof(uid_t);
    memcpy(p, &msglen, sizeof(msglen));
    p += 4;
    memcpy(p, msg, len);

    /* Bounds check before accessing ipc_cmd_str[] */
    if (command < sizeof(ipc_cmd_str) / sizeof(ipc_cmd_str[0])) {
        LOG(log_debug, logtype_afpd, "ipc_child_write(%s)", ipc_cmd_str[command]);
    } else {
        LOG(log_debug, logtype_afpd, "ipc_child_write(cmd=%u)", command);
    }

    if (writet(obj->ipc_fd, block, len + IPC_HEADERLEN, 0,
               1) != (ssize_t)(len + IPC_HEADERLEN)) {
        return -1;
    }

    return 0;
}

int ipc_child_state(AFPObj *obj, uint16_t state)
{
    return ipc_child_write(obj, IPC_STATE, sizeof(uint16_t), &state);
}

/***********************************************************************************
 * Poll-driven hint flush (parent-side only)
 ***********************************************************************************/

/*!
 * @brief Return current number of buffered hints.
 *
 * Used by main event loop to decide poll timeout:
 * - count > 0: use HINT_FLUSH_INTERVAL_MS timeout
 * - count == 0: use -1 (infinite, block until event)
 */
int hint_buf_count(void)
{
    return hint_buf.count;
}

/*!
 * @brief Flush all buffered hints to sibling children.
 *
 * Called from the parent main event loop when:
 * 1. The 50ms poll timeout expires and hint_buf.count > 0
 * 2. hint_buf.count reaches HINT_BUF_SIZE after ipc_server_read
 *
 * Iterates the child table directly:
 * - Signals are blocked (no SIGCHLD can modify table)
 * - SIGCHLD already processed before flush (dead children removed)
 * - No other thread modifies the table
 *
 * Performs priority sorting, PIPE_BUF-safe chunked writes.
 *
 * While this function writes to child pipes, new IPC messages from
 * children accumulate in the kernel socket buffer and are read on
 * the next poll() iteration.
 */
void hint_flush_pending(server_child_t *children)
{
    if (hint_buf.count == 0) {
        return;
    }

    int local_count = hint_buf.count;
    /* Shared across flushes: each reset forces one, and a per-flush budget
     * would let a burst of distinct-volume resets stall the single-threaded
     * master for its multiple. */
    static time_t wait_window;
    static int waits_left;
    const time_t now = time(NULL);

    if (now != wait_window) {
        wait_window = now;
        waits_left = HINT_RESET_WAIT_BUDGET;
    }

    int reset_lost = 0;
    struct hint_entry local_buf[HINT_BUF_SIZE];
    memcpy(local_buf, hint_buf.entries,
           local_count * sizeof(struct hint_entry));
    hint_buf.count = 0;
    /* Sort by delivery priority. VOLUME_RESET ends the receiving session, so it
     * precedes the cache-mend events that a reset makes moot; those keep their
     * REFRESH, DELETE, DELETE_CHILDREN order. O(n) counting + scatter pass —
     * no allocations. Events are validated against CACHE_HINT_COUNT before
     * being buffered, so they are safe to use as indices here. */
    static const int priority[CACHE_HINT_COUNT] = {
        [CACHE_HINT_VOLUME_RESET]    = 0,
        [CACHE_HINT_REFRESH]         = 1,
        [CACHE_HINT_DELETE]          = 2,
        [CACHE_HINT_DELETE_CHILDREN] = 3,
    };
    struct hint_entry sorted[HINT_BUF_SIZE] = {0};
    int counts[CACHE_HINT_COUNT] = {0};

    for (int h = 0; h < local_count; h++) {
        counts[priority[local_buf[h].event]]++;
    }

    int offsets[CACHE_HINT_COUNT] = {0};

    for (int p = 1; p < CACHE_HINT_COUNT; p++) {
        offsets[p] = offsets[p - 1] + counts[p - 1];
    }

    for (int h = 0; h < local_count; h++) {
        sorted[offsets[priority[local_buf[h].event]]++] = local_buf[h];
    }

    /* Build and send per-sibling batch in PIPE_BUF-safe chunks */
    const size_t msg_size = IPC_HEADERLEN +
                            sizeof(struct ipc_cache_hint_payload);
    const int msgs_per_chunk = PIPE_BUF / msg_size;
    const size_t chunk_limit = msgs_per_chunk * msg_size;
    char write_buf[HINT_BUF_SIZE * (IPC_HEADERLEN +
                                    sizeof(struct ipc_cache_hint_payload))];

    for (int i = 0; i < CHILD_HASHSIZE; i++) {
        for (afp_child_t *child = children->servch_table[i]; child;
                child = child->afpch_next) {
            if (child->afpch_hint_fd < 0) {
                continue;
            }

            int write_len = 0;
            int chunk_has_reset = 0;

            for (int h = 0; h < local_count; h++) {
                if (child->afpch_pid == sorted[h].source_pid) {
                    continue;
                }

                write_len += serialize_hint(write_buf + write_len,
                                            &sorted[h]);

                if (sorted[h].event == CACHE_HINT_VOLUME_RESET) {
                    chunk_has_reset = 1;
                }

                /* Flush chunk when it reaches PIPE_BUF-safe limit */
                if ((size_t)write_len >= chunk_limit) {
                    if (hint_write_batch(child->afpch_hint_fd, write_buf,
                                         write_len, chunk_has_reset,
                                         &waits_left) != 0) {
                        reset_lost |= chunk_has_reset;
                        write_len = 0;
                        /* Pipe full or error — next sibling */
                        break;
                    }

                    write_len = 0;
                    chunk_has_reset = 0;
                }
            }

            /* Flush any remaining partial chunk */
            if (write_len > 0
                    && hint_write_batch(child->afpch_hint_fd, write_buf,
                                        write_len, chunk_has_reset,
                                        &waits_left) != 0) {
                reset_lost |= chunk_has_reset;
            }
        }
    }

    if (reset_lost) {
        /* At least one sibling missed a reset it can only hear again through
         * a duplicate; siblings that did hear it end their sessions either
         * way, so the re-broadcast is idempotent. */
        for (int h = 0; h < local_count; h++) {
            if (local_buf[h].event == CACHE_HINT_VOLUME_RESET) {
                reset_broadcast_forget(local_buf[h].cnid);
            }
        }
    }

    hints_batched += local_count;
    flush_count++;
}

/***********************************************************************************
 * Cross-process dircache hint sender (child → parent via IPC socketpair)
 ***********************************************************************************/

/* Sender-side statistics counters — exposed via getters for log_dircache_stat() */
static unsigned long long hints_sent = 0;
static unsigned long long hints_dropped = 0;

unsigned long long ipc_get_hints_sent(void)
{
    return hints_sent;
}

unsigned long long ipc_get_hints_dropped(void)
{
    return hints_dropped;
}

static const char *cache_hint_name(uint8_t event)
{
    switch (event) {
    case CACHE_HINT_REFRESH:
        return "REFRESH";

    case CACHE_HINT_DELETE:
        return "DELETE";

    case CACHE_HINT_DELETE_CHILDREN:
        return "DELETE_CHILDREN";

    case CACHE_HINT_VOLUME_RESET:
        return "VOLUME_RESET";

    default:
        return "?";
    }
}

/*!
 * @brief Send a dircache invalidation hint from child to parent
 *
 * Called directly from AFP command handlers that modify dircache state.
 * Independent of the external FCE system — always active when IPC is available.
 *
 * Uses a direct non-blocking write() to the IPC socketpair instead of
 * ipc_child_write()/writet() — this ensures the AFP command handler is
 * never blocked waiting for IPC buffer space. If the kernel socket buffer
 * is full (parent not draining fast enough), the hint is silently dropped.
 * Hints are best-effort optimizations, and the dircache validation mechanism
 * & graceful fail-on-use detection makes drops safe.
 *
 * CACHE_HINT_VOLUME_RESET is the exception: a sibling that misses it keeps
 * resolving CNIDs that the wipe has recycled onto other files, so it waits
 * briefly for pipe space and reports failure to the caller.
 *
 * The 22-byte message (14-byte IPC header + 8-byte payload) is well under
 * the kernel socket buffer size, so partial writes cannot occur when space
 * is available.
 *
 * @param[in] obj    AFPObj with ipc_fd
 * @param[in] vid    Volume ID (network byte order, matches vol->v_vid)
 * @param[in] cnid   CNID of affected file/dir (network byte order)
 * @param[in] event  Hint type: one of the CACHE_HINT_* values
 * @returns 0 on success (or graceful drop), -1 on fatal error or an
 *          undeliverable CACHE_HINT_VOLUME_RESET
 */
int ipc_send_cache_hint(const AFPObj *obj, uint16_t vid, cnid_t cnid,
                        uint8_t event)
{
    if (obj->ipc_fd < 0) {
        /* A reset nobody can be told about is a failure; a cache-mend hint
         * is best-effort */
        return event == CACHE_HINT_VOLUME_RESET ? -1 : 0;
    }

    /* Both vid and cnid are already network byte order in the codebase */
    struct ipc_cache_hint_payload hint = {
        .event = event,
        .reserved = 0,
        .vid = vid,
        .cnid = cnid,
    };
    /* Build the IPC wire message inline — same format as ipc_child_write()
     * but we avoid writet() which retries on EAGAIN with select() timeout */
    char block[IPC_MAXMSGSIZE];
    char *p = block;
    uint16_t command = IPC_CACHE_HINT;
    pid_t pid = obj->pid;
    uid_t uid = obj->euid;
    uint32_t len = sizeof(hint);
    memset(block, 0, IPC_MAXMSGSIZE);
    memcpy(p, &command, sizeof(command));
    p += sizeof(command);
    memcpy(p, &pid, sizeof(pid_t));
    p += sizeof(pid_t);
    memcpy(p, &uid, sizeof(uid_t));
    p += sizeof(uid_t);
    memcpy(p, &len, 4);
    p += 4;
    memcpy(p, &hint, sizeof(hint));
    ssize_t total = IPC_HEADERLEN + sizeof(hint);
    ssize_t ret = write(obj->ipc_fd, block, total);

    if (ret == total) {
        hints_sent++;
        LOG(log_debug, logtype_afpd,
            "ipc_send_cache_hint: sent %s for vid:%u did:%u",
            cache_hint_name(event), ntohs(vid), ntohl(cnid));
        return 0;
    }

    if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        if (event != CACHE_HINT_VOLUME_RESET) {
            /* Best-effort: a lost cache-mend hint costs a lookup */
            hints_dropped++;
            LOG(log_debug, logtype_afpd,
                "ipc_send_cache_hint: buffer full, %s dropped",
                cache_hint_name(event));
            return 0;
        }

        /* A lost reset costs every sibling the notification */
        struct pollfd pfd = { .fd = obj->ipc_fd, .events = POLLOUT };
        int pret;

        do {
            pret = poll(&pfd, 1, HINT_RESET_WAIT_MS);
        } while (pret < 0 && errno == EINTR);

        if (pret > 0 && (pfd.revents & POLLOUT)) {
            ret = write(obj->ipc_fd, block, total);
        }

        if (ret == total) {
            hints_sent++;
            return 0;
        }
    }

    /* Unexpected error (not EAGAIN) or partial write */
    if (ret == -1) {
        LOG(log_warning, logtype_afpd,
            "ipc_send_cache_hint: write error: %s", strerror(errno));
    } else {
        LOG(log_warning, logtype_afpd,
            "ipc_send_cache_hint: partial write (%zd of %zd)", ret, total);
    }

    return -1;
}
