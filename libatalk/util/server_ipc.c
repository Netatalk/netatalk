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
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
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

/***********************************************************************************
 * Cache hint batching infrastructure
 * Main thread appends hints to buffer under lock, flush thread copies+clears under
 * same lock, then writes to sibling afpd processes from copy without any lock held
 ***********************************************************************************/

/* Max hints per buffer. Must satisfy HINT_BATCH_SIZE >= arrival_rate × write_phase_time.
 * Worst case: 50 siblings × 1000 hints/sec rate limit = 50K hints/sec. write phase for 50
 * siblings on macOS (PIPE_BUF=512 * 6 writes/sibling × 5μs/write) = 1.5ms; hints during
 * write = 50000 × 0.0015 = 75. Buffer = 128 provides 1.7× headroom for worst case. */
#define HINT_BATCH_SIZE         128
#define HINT_FLUSH_INTERVAL_MS   50  /* Flush thread wakeup interval */
_Static_assert(HINT_FLUSH_INTERVAL_MS < 1000,
               "Use a while loop for nanosecond normalization if HINT_FLUSH_INTERVAL_MS >= 1s");
#define HINT_RATE_LIMIT        1000  /* Max hints/second per child (attack guard) */

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

/* Single accumulation buffer — protected by flush_mutex */
static struct {
    struct hint_entry entries[HINT_BATCH_SIZE];
    int count;
} hint_buf;

/* Flush thread synchronization */
static pthread_t flush_tid;
static pthread_mutex_t flush_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t flush_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t flush_drained = PTHREAD_COND_INITIALIZER;
static _Atomic int flush_shutdown = 0;

static struct {
    pid_t  pid;
    time_t window_start;
    int    count_in_window;
} rate_track[RATE_TRACK_SIZE];

/* Parent-side statistics — logged periodically from flush thread */
static _Atomic unsigned long long hints_batched = 0;
static _Atomic unsigned long long hints_rate_dropped = 0;
static _Atomic unsigned long long flush_count = 0;

/* Returns current hint count for this child in the current 1-second window.
 * Resets window if second has changed. Called from main thread only. */
static int check_and_increment_rate(pid_t child_pid)
{
    int idx = child_pid % RATE_TRACK_SIZE;
    time_t now = time(NULL);

    if (rate_track[idx].pid != child_pid || rate_track[idx].window_start != now) {
        rate_track[idx].pid = child_pid;
        rate_track[idx].window_start = now;
        rate_track[idx].count_in_window = 1;
        return 1;
    }

    return ++rate_track[idx].count_in_window;
}

/*!
 * @brief Snapshot all sibling hint pipe fds and PIDs under lock.
 *
 * Allocates arrays for fds and pids, iterates all CHILD_HASHSIZE hash
 * buckets collecting afpch_hint_fd and afpch_pid for each child with
 * a valid hint fd. Caller must free both arrays.
 *
 * @param[in]  children    Server child table
 * @param[out] out_fds     Allocated array of hint pipe fds
 * @param[out] out_pids    Allocated array of corresponding PIDs
 * @param[out] out_count   Number of entries in both arrays
 */
static void snapshot_sibling_fds(server_child_t *children,
                                 int **out_fds, pid_t **out_pids,
                                 int *out_count)
{
    *out_fds = NULL;
    *out_pids = NULL;
    *out_count = 0;
    int *fds = malloc(children->servch_nsessions * sizeof(int));
    pid_t *pids = malloc(children->servch_nsessions * sizeof(pid_t));

    if (!fds || !pids) {
        free(fds);
        free(pids);
        return;
    }

    int count = 0;
    pthread_mutex_lock(&children->servch_lock);

    for (int i = 0; i < CHILD_HASHSIZE; i++) {
        for (afp_child_t *child = children->servch_table[i]; child;
                child = child->afpch_next) {
            if (child->afpch_hint_fd >= 0) {
                /* dup() under lock — our copy survives even if the child
                 * exits and the parent closes the original fd before we
                 * finish writing. Without dup(), fd reuse could direct
                 * writes to an unrelated descriptor. */
                int dfd = dup(child->afpch_hint_fd);

                if (dfd >= 0) {
                    fds[count] = dfd;
                    pids[count] = child->afpch_pid;
                    count++;
                }
            }
        }
    }

    pthread_mutex_unlock(&children->servch_lock);
    *out_fds = fds;
    *out_pids = pids;
    *out_count = count;
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

static void *hint_flush_thread(void *arg)
{
    server_child_t *children = (server_child_t *)arg;
    struct hint_entry local_buf[HINT_BATCH_SIZE];
    /* Block all signals — flush thread must not receive process signals */
    sigset_t sigs;
    sigfillset(&sigs);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);

    while (!atomic_load(&flush_shutdown)) {
        /* Sleep for 50ms or until signaled */
        struct timespec ts;
        pthread_mutex_lock(&flush_mutex);

        if (hint_buf.count == 0 && !atomic_load(&flush_shutdown)) {
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_nsec += HINT_FLUSH_INTERVAL_MS * 1000000L;

            if (ts.tv_nsec >= 1000000000L) {
                ts.tv_sec++;
                ts.tv_nsec -= 1000000000L;
            }

            pthread_cond_timedwait(&flush_cond, &flush_mutex, &ts);
        }

        /* Copy buffer contents to local array and clear.
         * Signal flush_drained to wake any blocked producer. */
        int local_count = hint_buf.count;

        if (local_count > 0) {
            memcpy(local_buf, hint_buf.entries,
                   local_count * sizeof(struct hint_entry));
            hint_buf.count = 0;
            pthread_cond_signal(&flush_drained);
        }

        pthread_mutex_unlock(&flush_mutex);

        if (local_count == 0) {
            continue;
        }

        /* Sort by priority: REFRESH(0) first, DELETE(1), DELETE_CHILDREN(2) last.
         * O(n) counting + scatter pass — no allocations. */
        struct hint_entry sorted[HINT_BATCH_SIZE];
        int counts[3] = {0};

        for (int h = 0; h < local_count; h++) {
            counts[local_buf[h].event]++;
        }

        int offsets[3] = {0, counts[0], counts[0] + counts[1]};

        for (int h = 0; h < local_count; h++) {
            int e = local_buf[h].event;
            sorted[offsets[e]++] = local_buf[h];
        }

        /* Snapshot sibling fds ONCE for the entire batch */
        int *child_fds = NULL;
        pid_t *child_pids = NULL;
        int child_count = 0;
        snapshot_sibling_fds(children, &child_fds, &child_pids,
                             &child_count);

        if (child_count == 0 || !child_fds) {
            free(child_fds);
            free(child_pids);
            continue;
        }

        /* Build and send per-sibling batch in PIPE_BUF-safe chunks.
         * Each chunk contains only complete 22-byte messages, so
         * writes ≤ PIPE_BUF are POSIX-guaranteed atomic — no partial */
        const size_t msg_size = IPC_HEADERLEN +
                                sizeof(struct ipc_cache_hint_payload);
        const int msgs_per_chunk = PIPE_BUF / msg_size;
        const size_t chunk_limit = msgs_per_chunk * msg_size;
        char write_buf[HINT_BATCH_SIZE * (IPC_HEADERLEN +
                                          sizeof(struct ipc_cache_hint_payload))];

        for (int s = 0; s < child_count; s++) {
            int write_len = 0;

            for (int h = 0; h < local_count; h++) {
                if (child_pids[s] == sorted[h].source_pid) {
                    continue;    /* Skip source child */
                }

                write_len += serialize_hint(write_buf + write_len,
                                            &sorted[h]);

                /* Flush chunk when it reaches PIPE_BUF-safe limit */
                if ((size_t)write_len >= chunk_limit) {
                    ssize_t ret = write(child_fds[s], write_buf,
                                        write_len);

                    if (ret < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            LOG(log_debug, logtype_afpd,
                                "hint_flush: write failed fd=%d: %s",
                                child_fds[s], strerror(errno));
                        }

                        write_len = 0;
                        break;  /* Pipe full or error — next sibling */
                    }

                    write_len = 0;
                }
            }

            /* Flush any remaining partial chunk */
            if (write_len > 0) {
                ssize_t ret = write(child_fds[s], write_buf, write_len);

                if (ret < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    LOG(log_debug, logtype_afpd,
                        "hint_flush: write failed fd=%d: %s",
                        child_fds[s], strerror(errno));
                }
            }
        }

        atomic_fetch_add(&hints_batched, local_count);
        atomic_fetch_add(&flush_count, 1);

        /* Close dup'd fds — our private copies from snapshot_sibling_fds */
        for (int s = 0; s < child_count; s++) {
            close(child_fds[s]);
        }

        free(child_fds);
        free(child_pids);
    }

    return NULL;
}

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
 * @brief Buffer a dircache hint for batched relay to siblings
 *
 * Main thread appends to hint_buf under flush_mutex; the flush thread
 * drains and writes batches to siblings.
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

    /* Validate event type — reject unknown values at ingestion to prevent
     * out-of-bounds access in the flush thread's counting sort. */
    if (hint.event > CACHE_HINT_DELETE_CHILDREN) {
        LOG(log_warning, logtype_afpd,
            "ipc_relay_cache_hint: invalid event %u from pid %u, dropped",
            hint.event, ipc->child_pid);
        return 0;
    }

    /* Rate-limit check (attack guard) */
    if (check_and_increment_rate(ipc->child_pid) > HINT_RATE_LIMIT) {
        atomic_fetch_add(&hints_rate_dropped, 1);

        /* Log only first exceed per window to avoid log flooding */
        if (rate_track[ipc->child_pid % RATE_TRACK_SIZE].count_in_window
                == HINT_RATE_LIMIT + 1) {
            LOG(log_warning, logtype_afpd,
                "ipc_relay_cache_hint: rate limit exceeded for pid %u",
                ipc->child_pid);
        }

        return 0;
    }

    /* Append to buffer under lock */
    pthread_mutex_lock(&flush_mutex);

    /* Buffer full — signal flush thread and wait for it to drain.
     * flush thread sleeping: wait bounded by wake latency + memcpy time (~1-10μs).
     * flush thread mid-write: wait bounded by remaining write cycle time + memcpy */
    if (hint_buf.count >= HINT_BATCH_SIZE) {
        pthread_cond_signal(&flush_cond);

        while (hint_buf.count >= HINT_BATCH_SIZE
                && !atomic_load(&flush_shutdown)) {
            pthread_cond_wait(&flush_drained, &flush_mutex);
        }
    }

    hint_buf.entries[hint_buf.count] = (struct hint_entry) {
        .vid = hint.vid,
        .cnid = hint.cnid,
        .event = hint.event,
        .source_pid = ipc->child_pid,
    };
    hint_buf.count++;

    /* Signal flush thread if buffer is now full */
    if (hint_buf.count >= HINT_BATCH_SIZE) {
        pthread_cond_signal(&flush_cond);
    }

    pthread_mutex_unlock(&flush_mutex);
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
 * Hint flush thread lifecycle (parent-side only)
 ***********************************************************************************/

/*!
 * @brief Prevent child from touching flush infrastructure after fork().
 *
 * Single atomic store — no mutex/condvar reinit needed since the child
 * never accesses the flush thread's synchronization primitives.
 */
static void hint_flush_child_atfork(void)
{
    atomic_store(&flush_shutdown, 1);
}

/*!
 * @brief Start the hint flush thread. Called from parent after child table init.
 */
int hint_flush_start(server_child_t *children)
{
    pthread_atfork(NULL, NULL, hint_flush_child_atfork);
    int ret = pthread_create(&flush_tid, NULL, hint_flush_thread, children);

    if (ret != 0) {
        LOG(log_error, logtype_afpd,
            "hint_flush_start: pthread_create failed: %s", strerror(ret));
        return -1;
    }

    LOG(log_info, logtype_afpd, "hint_flush_start: flush thread started");
    return 0;
}

/*!
 * @brief Stop the hint flush thread. Best-effort — called during parent shutdown.
 *
 * Note: The parent's SIGTERM handler calls _exit(0) directly, so this
 * may not be reached. That is acceptable — SIGTERM also kills all children,
 */
void hint_flush_stop(void)
{
    atomic_store(&flush_shutdown, 1);
    pthread_mutex_lock(&flush_mutex);
    pthread_cond_signal(&flush_cond);
    pthread_cond_signal(&flush_drained);
    pthread_mutex_unlock(&flush_mutex);
    pthread_join(flush_tid, NULL);
    /* Final statistics */
    LOG(log_debug, logtype_afpd,
        "hint_flush_stop: final stats: flushes=%llu, hints_batched=%llu, "
        "rate_dropped=%llu",
        atomic_load(&flush_count),
        atomic_load(&hints_batched),
        atomic_load(&hints_rate_dropped));
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
