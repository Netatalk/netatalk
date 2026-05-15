/*
 * Copyright (C) Joerg Lenneis 2003
 * Copyright (C) Frank Lahm 2010
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>

#include <atalk/adouble.h>
#include <atalk/cnid_bdb_private.h>
#include <atalk/cnid.h>
#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "cnid_dbd.h"

#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif /* ! SOL_TCP */

/*! Wait MAX_DELAY seconds before a request to the CNID server times out */
#define MAX_DELAY 20
#define ONE_DELAY 5

static volatile sig_atomic_t stop_signal = 0;

static void sig_handler(int signo)
{
    stop_signal = signo;
}

static void RQST_RESET(struct cnid_dbd_rqst  *r)
{
    memset(r, 0, sizeof(struct cnid_dbd_rqst));
}

static void delay(int sec)
{
    struct timeval tv;
    tv.tv_usec = 0;
    tv.tv_sec  = sec;
    select(0, NULL, NULL, NULL, &tv);
}

static int tsock_getfd(const char *host, const char *port)
{
    int sock = -1;
    int attr;
    int err;
    struct addrinfo hints, *servinfo, *p;
    int optval = ECONNREFUSED;
    socklen_t optlen = sizeof(optval);
    /* Prepare hint for getaddrinfo */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
#ifdef AI_NUMERICSERV
    hints.ai_flags = AI_NUMERICSERV;
#endif

    if ((err = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        LOG(log_error, logtype_default,
            "tsock_getfd: getaddrinfo: CNID server %s:%s : %s\n",
            host, port, gai_strerror(err));
        return -1;
    }

    /* loop through all the results and bind to the first we can */
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            LOG(log_info, logtype_default, "tsock_getfd: socket CNID server %s:: %s",
                host, strerror(errno));
            continue;
        }

        attr = 1;

        if (setsockopt(sock, SOL_TCP, TCP_NODELAY, &attr, sizeof(attr)) == -1) {
            LOG(log_error, logtype_cnid, "getfd: set TCP_NODELAY CNID server %s: %s",
                host, strerror(errno));
            close(sock);
            sock = -1;
            return -1;
        }

        if (setnonblock(sock, 1) != 0) {
            LOG(log_error, logtype_cnid, "getfd: setnonblock: %s", strerror(errno));
            close(sock);
            sock = -1;
            return -1;
        }

        if (connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
            if (errno == EINPROGRESS) {
                struct timeval tv;
                tv.tv_usec = 0;
                tv.tv_sec  = 5; /* give it five seconds ... */
                fd_set wfds;
                FD_ZERO(&wfds);
                FD_SET(sock, &wfds);

                if ((err = select(sock + 1, NULL, &wfds, NULL, &tv)) == 0) {
                    /* timeout */
                    LOG(log_error, logtype_cnid, "getfd: select timed out for CNID server %s",
                        host);
                    close(sock);
                    sock = -1;
                    continue;
                }

                if (err == -1) {
                    /* select failed */
                    LOG(log_error, logtype_cnid, "getfd: select failed for CNID server %s",
                        host);
                    close(sock);
                    sock = -1;
                    continue;
                }

                if (! FD_ISSET(sock, &wfds)) {
                    /* give up */
                    LOG(log_error, logtype_cnid, "getfd: socket not ready connecting to %s",
                        host);
                    close(sock);
                    sock = -1;
                    continue;
                }

                if ((err = getsockopt(sock, SOL_SOCKET, SO_ERROR, &optval, &optlen)) != 0
                        || optval != 0) {
                    if (err != 0) {
                        /* somethings very wrong */
                        LOG(log_error, logtype_cnid, "getfd: getsockopt error with CNID server %s: %s",
                            host, strerror(errno));
                    } else {
                        errno = optval;
                        LOG(log_error, logtype_cnid, "getfd: getsockopt says: %s",
                            strerror(errno));
                    }

                    close(sock);
                    sock = -1;
                    continue;
                }
            } else {
                LOG(log_error, logtype_cnid, "getfd: connect CNID server %s: %s",
                    host, strerror(errno));
                close(sock);
                sock = -1;
                continue;
            }
        }

        /* We've got a socket */
        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL) {
        errno = optval;
        LOG(log_error, logtype_cnid,
            "tsock_getfd: no suitable network config from CNID server (%s:%s): %s",
            host, port, strerror(errno));
        return -1;
    }

    return sock;
}

/*!
 * @brief Write "towrite" bytes using writev on non-blocking fd
 *
 * Every short write is considered an error, transmit can handle that.
 *
 * @param[in] fd       socket fd which must be non-blocking
 * @param[in] iov      iovec for writev
 * @param[in] towrite  number of bytes in all iovec elements
 * @param[in] vecs     number of iovecs in array
 *
 * @returns "towrite" bytes written or -1 on error
 */
static int write_vec(int fd, struct iovec *iov, ssize_t towrite, int vecs)
{
    ssize_t len;
    int slept = 0;
    int sleepsecs;

    while (1) {
        if ((len = writev(fd, iov, vecs)) == -1 && errno == EINTR) {
            continue;
        }

        if ((! slept) && len == -1 && errno == EAGAIN) {
            sleepsecs = 2;

            while ((sleepsecs = sleep(sleepsecs)));

            slept = 1;
            continue;
        }

        if (len == towrite) { /* wrote everything out */
            break;
        }

        if (len == -1) {
            LOG(log_error, logtype_cnid, "write_vec: %s", strerror(errno));
        } else {
            LOG(log_error, logtype_cnid, "write_vec: short write: %d", len);
        }

        return len;
    }

    LOG(log_maxdebug, logtype_cnid, "write_vec: wrote %d bytes", len);
    return len;
}

/* --------------------- */
static int init_tsock(CNID_bdb_private *db)
{
    int fd;
    int len[DBD_NUM_OPEN_ARGS];
    int iovecs;
    struct iovec iov[DBD_NUM_OPEN_ARGS + 1] = {{0}};
    struct vol *vol = db->vol;
    ssize_t iovlen;
    LOG(log_debug, logtype_cnid, "connecting to CNID server: %s:%s",
        vol->v_cnidserver, vol->v_cnidport);

    if ((fd = tsock_getfd(vol->v_cnidserver, vol->v_cnidport)) < 0) {
        return -1;
    }

    LOG(log_debug, logtype_cnid, "connecting volume '%s', path: %s, user: %s",
        vol->v_configname, vol->v_path,
        vol->v_obj->username[0] ? vol->v_obj->username : "-");
    iovecs = 1 + DBD_NUM_OPEN_ARGS - 1;
    len[0] = strlen(vol->v_configname) + 1;
    len[1] = strlen(vol->v_path) + 1;
    len[2] = strlen(vol->v_obj->username);
    iov[0].iov_base = &len[0];
    iov[0].iov_len  = DBD_NUM_OPEN_ARGS * sizeof(int);
    iov[1].iov_base = vol->v_configname;
    iov[1].iov_len  = len[0];
    iov[2].iov_base = vol->v_path;
    iov[2].iov_len  = len[1];

    if (len[2] > 0) {
        len[2] += 1;
        iovecs++;
        iov[3].iov_base = vol->v_obj->username;
        iov[3].iov_len  = len[2];
    }

    iovlen = iov[0].iov_len + iov[1].iov_len + iov[2].iov_len + iov[3].iov_len;

    if (write_vec(fd, iov, iovlen, iovecs) != iovlen) {
        LOG(log_error, logtype_cnid, "init_tsock: Error/short write: %s",
            strerror(errno));
        close(fd);
        return -1;
    }

    LOG(log_debug, logtype_cnid, "init_tsock: ok");
    return fd;
}

/* --------------------- */
static int send_packet(CNID_bdb_private *db, struct cnid_dbd_rqst *rqst)
{
    struct iovec iov[2];
    size_t towrite;
    int vecs;
    iov[0].iov_base = rqst;
    iov[0].iov_len  = sizeof(struct cnid_dbd_rqst);
    towrite = sizeof(struct cnid_dbd_rqst);
    vecs = 1;

    if (rqst->namelen) {
        iov[1].iov_base = (char *)rqst->name;
        iov[1].iov_len  = rqst->namelen;
        towrite += rqst->namelen;
        vecs++;
    }

    if (write_vec(db->fd, iov, towrite, vecs) != towrite) {
        LOG(log_warning, logtype_cnid, "send_packet: Error writev rqst (volume %s): %s",
            db->vol->v_localname, strerror(errno));
        return -1;
    }

    LOG(log_maxdebug, logtype_cnid, "send_packet: {done}");
    return 0;
}

/* ------------------- */
static void dbd_initstamp(struct cnid_dbd_rqst *rqst)
{
    RQST_RESET(rqst);
    rqst->op = CNID_DBD_OP_GETSTAMP;
}

/* ------------------- */
static int dbd_reply_stamp(struct cnid_dbd_rply *rply)
{
    switch (rply->result) {
    case CNID_DBD_RES_OK:
        break;

    case CNID_DBD_RES_NOTFOUND:
        return -1;

    case CNID_DBD_RES_ERR_DB:
    default:
        errno = CNID_ERR_DB;
        return -1;
    }

    return 0;
}

/*!
 * @brief send a request and get reply
 * @note assume send is non blocking
 * @note if no answer after sometime (at least MAX_DELAY secondes) return an error
 */
static int dbd_rpc(CNID_bdb_private *db, struct cnid_dbd_rqst *rqst,
                   struct cnid_dbd_rply *rply)
{
    ssize_t ret;
    char *nametmp;
    size_t len;

    if (send_packet(db, rqst) < 0) {
        return -1;
    }

    len = rply->namelen;
    nametmp = rply->name;
    ret = readt(db->fd, rply, sizeof(struct cnid_dbd_rply), 0, ONE_DELAY);

    if (ret != sizeof(struct cnid_dbd_rply)) {
        LOG(log_debug, logtype_cnid,
            "dbd_rpc: Error reading header from fd (volume %s): %s",
            db->vol->v_localname, ret == -1 ? strerror(errno) : "closed");
        rply->name = nametmp;
        return -1;
    }

    rply->name = nametmp;

    if (rply->namelen && rply->namelen > len) {
        LOG(log_error, logtype_cnid,
            "dbd_rpc: Error reading name (volume %s): %s name too long: %d. only wanted %d, garbage?",
            db->vol->v_localname, rply->name, rply->namelen, len);
        return -1;
    }

    if (rply->namelen
            && (ret = readt(db->fd, rply->name, rply->namelen, 0,
                            ONE_DELAY)) != (ssize_t)rply->namelen) {
        LOG(log_error, logtype_cnid,
            "dbd_rpc: Error reading name from fd (volume %s): %s",
            db->vol->v_localname, ret == -1 ? strerror(errno) : "closed");
        return -1;
    }

    LOG(log_maxdebug, logtype_cnid, "dbd_rpc: {done}");
    return 0;
}

/* -------------------- */
/*!
 * @brief Inner transmit body, signal handlers must already be installed
 *
 * Caller must have installed sig_handler for SIGTERM/SIGINT and reset
 * stop_signal to 0; otherwise the first loop iteration breaks immediately
 * and returns -1 (which the caller would misread as an I/O failure).
 *
 * Used directly by cnid_dbd_find()'s pagination loop, which installs
 * sigaction once across all batches.
 */
static int transmit_locked(CNID_bdb_private *db,
                           struct cnid_dbd_rqst *rqst,
                           struct cnid_dbd_rply *rply)
{
    time_t orig, t;
    /* No errors so far - prevents sleep on first try. */
    int clean = 1;

    while (1) {
        if (stop_signal) {
            break;
        }

        if (db->fd == -1) {
            LOG(log_maxdebug, logtype_cnid,
                "transmit_locked: connecting to cnid_dbd ...");

            if ((db->fd = init_tsock(db)) < 0) {
                goto transmit_fail;
            }

            if (db->notfirst) {
                LOG(log_debug7, logtype_cnid,
                    "transmit_locked: reconnected to cnid_dbd");
            } else {
                db->notfirst = 1;
            }

            LOG(log_debug, logtype_cnid,
                "transmit_locked: attached to '%s'", db->vol->v_localname);
        }

        if (!dbd_rpc(db, rqst, rply)) {
            LOG(log_maxdebug, logtype_cnid, "transmit_locked: {done}");
            return 0;
        }

transmit_fail:

        if (db->fd != -1) {
            close(db->fd);
            /* FD not valid... will need to reconnect */
            db->fd = -1;
        }

        /* errno carefully injected in tsock_getfd */
        if (errno == ECONNREFUSED) {
            LOG(log_error, logtype_cnid,
                "transmit_locked: connection refused (volume %s)",
                db->vol->v_localname);
            return -1;
        }

        /* Don't sleep if just got disconnected by cnid server. */
        if (!clean) {
            time(&t);

            if (t - orig > MAX_DELAY) {
                LOG(log_error, logtype_cnid,
                    "transmit_locked: Request to dbd daemon "
                    "(volume %s) timed out.", db->vol->v_localname);
                return -1;
            }

            /* Sleep a little before retry. */
            delay(1);
        } else {
            /* False; next time sleep. */
            clean = 0;
            time(&orig);
        }
    }

    return -1;
}

/* -------------------- */
static int transmit(CNID_bdb_private *db, struct cnid_dbd_rqst *rqst,
                    struct cnid_dbd_rply *rply)
{
    int ret;
    struct sigaction sa, old_sa_term, old_sa_int;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, &old_sa_term);
    sigaction(SIGINT, &sa, &old_sa_int);
    stop_signal = 0;
    ret = transmit_locked(db, rqst, rply);
    sigaction(SIGTERM, &old_sa_term, NULL);
    sigaction(SIGINT, &old_sa_int, NULL);

    if (stop_signal) {
        raise(stop_signal);
    }

    return ret;
}

/* ---------------------- */
static struct _cnid_db *cnid_dbd_new(struct vol *vol)
{
    struct _cnid_db *cdb;

    if ((cdb = (struct _cnid_db *)calloc(1, sizeof(struct _cnid_db))) == NULL) {
        return NULL;
    }

    cdb->cnid_db_vol = vol;
    cdb->cnid_db_flags = CNID_FLAG_PERSISTENT | CNID_FLAG_LAZY_INIT;
    cdb->cnid_add = cnid_dbd_add;
    cdb->cnid_delete = cnid_dbd_delete;
    cdb->cnid_get = cnid_dbd_get;
    cdb->cnid_lookup = cnid_dbd_lookup;
    cdb->cnid_find = cnid_dbd_find;
    cdb->cnid_nextid = NULL;
    cdb->cnid_resolve = cnid_dbd_resolve;
    cdb->cnid_getstamp = cnid_dbd_getstamp;
    cdb->cnid_update = cnid_dbd_update;
    cdb->cnid_rebuild_add = cnid_dbd_rebuild_add;
    cdb->cnid_close = cnid_dbd_close;
    cdb->cnid_wipe = cnid_dbd_wipe;
    return cdb;
}

/* ---------------------- */
struct _cnid_db *cnid_dbd_open(struct cnid_open_args *args)
{
    CNID_bdb_private *db = NULL;
    struct _cnid_db *cdb = NULL;

    if ((cdb = cnid_dbd_new(args->cnid_args_vol)) == NULL) {
        LOG(log_error, logtype_cnid,
            "cnid_open: Unable to allocate memory for database");
        return NULL;
    }

    if ((db = (CNID_bdb_private *)calloc(1, sizeof(CNID_bdb_private))) == NULL) {
        LOG(log_error, logtype_cnid,
            "cnid_open: Unable to allocate memory for database");
        goto cnid_dbd_open_fail;
    }

    cdb->cnid_db_private = db;
    db->fd = -1;
    db->vol = args->cnid_args_vol;
    LOG(log_debug, logtype_cnid,
        "Finished initializing CNID dbd module for volume '%s'",
        args->cnid_args_vol->v_localname);
    return cdb;
cnid_dbd_open_fail:

    if (cdb != NULL) {
        free(cdb);
    }

    if (db != NULL) {
        free(db);
    }

    return NULL;
}

/* ---------------------- */
void cnid_dbd_close(struct _cnid_db *cdb)
{
    CNID_bdb_private *db;

    if (!cdb) {
        LOG(log_error, logtype_cnid, "cnid_close called with NULL argument !");
        return;
    }

    if ((db = cdb->cnid_db_private) != NULL) {
        LOG(log_debug, logtype_cnid, "closing database connection for volume '%s'",
            db->vol->v_localname);

        if (db->fd >= 0) {
            close(db->fd);
        }

        free(db);
    }

    free(cdb);
    return;
}

/*!
 * Get the db stamp
 */
static int cnid_dbd_stamp(CNID_bdb_private *db)
{
    struct cnid_dbd_rqst rqst_stamp;
    struct cnid_dbd_rply rply_stamp;
    char  stamp[ADEDLEN_PRIVSYN];
    dbd_initstamp(&rqst_stamp);
    memset(stamp, 0, ADEDLEN_PRIVSYN);
    rply_stamp.name = stamp;
    rply_stamp.namelen = ADEDLEN_PRIVSYN;

    if (transmit(db, &rqst_stamp, &rply_stamp) < 0) {
        return -1;
    }

    if (dbd_reply_stamp(&rply_stamp) < 0) {
        return -1;
    }

    if (db->client_stamp) {
        memcpy(db->client_stamp, stamp, ADEDLEN_PRIVSYN);
    }

    memcpy(db->stamp, stamp, ADEDLEN_PRIVSYN);
    return 0;
}

/* ---------------------- */
cnid_t cnid_dbd_add(struct _cnid_db *cdb, const struct stat *st,
                    cnid_t did, const char *name, size_t len, cnid_t hint)
{
    CNID_bdb_private *db;
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply = { 0 };
    cnid_t id;

    if (!cdb || !(db = cdb->cnid_db_private) || !st || !name) {
        LOG(log_error, logtype_cnid, "cnid_add: Parameter error");
        errno = CNID_ERR_PARAM;
        return CNID_INVALID;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid, "cnid_add: Path name is too long");
        errno = CNID_ERR_PATH;
        return CNID_INVALID;
    }

    RQST_RESET(&rqst);
    rqst.op = CNID_DBD_OP_ADD;

    if (!(cdb->cnid_db_flags & CNID_FLAG_NODEV)) {
        rqst.dev = st->st_dev;
    }

    rqst.ino = st->st_ino;
    rqst.type = S_ISDIR(st->st_mode) ? 1 : 0;
    rqst.cnid = hint;
    rqst.did = did;
    rqst.name = name;
    rqst.namelen = len;
    LOG(log_debug, logtype_cnid,
        "cnid_dbd_add: CNID: %u, name: '%s', dev: 0x%llx, inode: 0x%llx, type: %s",
        ntohl(did), name, (long long)rqst.dev, (long long)st->st_ino,
        rqst.type ? "dir" : "file");
    rply.namelen = 0;

    if (transmit(db, &rqst, &rply) < 0) {
        errno = CNID_ERR_DB;
        return CNID_INVALID;
    }

    switch (rply.result) {
    case CNID_DBD_RES_OK:
        id = rply.cnid;
        LOG(log_debug, logtype_cnid, "cnid_dbd_add: got CNID: %u", ntohl(id));
        break;

    case CNID_DBD_RES_ERR_MAX:
        errno = CNID_ERR_MAX;
        id = CNID_INVALID;
        break;

    case CNID_DBD_RES_ERR_DB:
    case CNID_DBD_RES_ERR_DUPLCNID:
        errno = CNID_ERR_DB;
        id = CNID_INVALID;
        break;

    default:
        LOG(log_error, logtype_cnid,
            "cnid_dbd_add: unexpected result code %d from CNID server", rply.result);
        abort();
    }

    return id;
}

/* ---------------------- */
cnid_t cnid_dbd_get(struct _cnid_db *cdb, cnid_t did, const char *name,
                    size_t len)
{
    CNID_bdb_private *db;
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply;
    cnid_t id;

    if (!cdb || !(db = cdb->cnid_db_private) || !name) {
        LOG(log_error, logtype_cnid, "cnid_dbd_get: Parameter error");
        errno = CNID_ERR_PARAM;
        return CNID_INVALID;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid, "cnid_dbd_get: Path name is too long");
        errno = CNID_ERR_PATH;
        return CNID_INVALID;
    }

    LOG(log_debug, logtype_cnid, "cnid_dbd_get: DID: %u, name: '%s'", ntohl(did),
        name);
    RQST_RESET(&rqst);
    rqst.op = CNID_DBD_OP_GET;
    rqst.did = did;
    rqst.name = name;
    rqst.namelen = len;
    rply.namelen = 0;

    if (transmit(db, &rqst, &rply) < 0) {
        errno = CNID_ERR_DB;
        return CNID_INVALID;
    }

    switch (rply.result) {
    case CNID_DBD_RES_OK:
        id = rply.cnid;
        LOG(log_debug, logtype_cnid, "cnid_dbd_get: got CNID: %u", ntohl(id));
        break;

    case CNID_DBD_RES_NOTFOUND:
        id = CNID_INVALID;
        break;

    case CNID_DBD_RES_ERR_DB:
        id = CNID_INVALID;
        errno = CNID_ERR_DB;
        break;

    default:
        LOG(log_error, logtype_cnid,
            "cnid_dbd_get: unexpected result code %d from CNID server", rply.result);
        abort();
    }

    return id;
}

/* ---------------------- */
char *cnid_dbd_resolve(struct _cnid_db *cdb, cnid_t *id, void *buffer,
                       size_t len)
{
    CNID_bdb_private *db;
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply;
    char *name;

    if (!cdb || !(db = cdb->cnid_db_private) || !id || !(*id)) {
        LOG(log_error, logtype_cnid, "cnid_resolve: Parameter error");
        errno = CNID_ERR_PARAM;
        return NULL;
    }

    LOG(log_debug, logtype_cnid, "cnid_dbd_resolve: resolving CNID: %u",
        ntohl(*id));
    /* TODO: We should maybe also check len. At the moment we rely on the caller
       to provide a buffer that is large enough for MAXPATHLEN plus
       CNID_HEADER_LEN plus 1 byte, which is large enough for the maximum that
       can come from the database. */
    RQST_RESET(&rqst);
    rqst.op = CNID_DBD_OP_RESOLVE;
    rqst.cnid = *id;
    /* Pass buffer to transmit so it can stuff the reply data there */
    rply.name = (char *)buffer;
    rply.namelen = len;

    if (transmit(db, &rqst, &rply) < 0) {
        LOG(log_error, logtype_cnid,
            "cnid_dbd_resolve: transmit failed for CNID %u: %s",
            ntohl(*id), strerror(errno));
        errno = CNID_ERR_DB;
        *id = CNID_INVALID;
        return NULL;
    }

    switch (rply.result) {
    case CNID_DBD_RES_OK:
        *id = rply.did;
        name = rply.name + CNID_NAME_OFS;
        LOG(log_debug, logtype_cnid, "cnid_dbd_resolve: resolved did: %u, name: '%s'",
            ntohl(*id), name);
        break;

    case CNID_DBD_RES_NOTFOUND:
        LOG(log_debug, logtype_cnid,
            "cnid_dbd_resolve: CNID %u not found in database (likely deleted)",
            ntohl(*id));
        *id = CNID_INVALID;
        name = NULL;
        break;

    case CNID_DBD_RES_ERR_DB:
        errno = CNID_ERR_DB;
        *id = CNID_INVALID;
        name = NULL;
        break;

    default:
        LOG(log_error, logtype_cnid,
            "cnid_dbd_resolve: unexpected result code %d from CNID server", rply.result);
        abort();
    }

    return name;
}

/*!
 * Caller passes buffer where we will store the db stamp
 */
int cnid_dbd_getstamp(struct _cnid_db *cdb, void *buffer, const size_t len)
{
    CNID_bdb_private *db;

    if (!cdb || !(db = cdb->cnid_db_private) || len != ADEDLEN_PRIVSYN) {
        LOG(log_error, logtype_cnid, "cnid_getstamp: Parameter error");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    db->client_stamp = buffer;
    db->stamp_size = len;
    return cnid_dbd_stamp(db);
}

/* ---------------------- */
cnid_t cnid_dbd_lookup(struct _cnid_db *cdb, const struct stat *st, cnid_t did,
                       const char *name, size_t len)
{
    CNID_bdb_private *db;
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply;
    cnid_t id;

    if (!cdb || !(db = cdb->cnid_db_private) || !st || !name) {
        LOG(log_error, logtype_cnid, "cnid_lookup: Parameter error");
        errno = CNID_ERR_PARAM;
        return CNID_INVALID;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid, "cnid_lookup: Path name is too long");
        errno = CNID_ERR_PATH;
        return CNID_INVALID;
    }

    RQST_RESET(&rqst);
    rqst.op = CNID_DBD_OP_LOOKUP;

    if (!(cdb->cnid_db_flags & CNID_FLAG_NODEV)) {
        rqst.dev = st->st_dev;
    }

    rqst.ino = st->st_ino;
    rqst.type = S_ISDIR(st->st_mode) ? 1 : 0;
    rqst.did = did;
    rqst.name = name;
    rqst.namelen = len;
    LOG(log_debug, logtype_cnid,
        "cnid_dbd_lookup: CNID: %u, name: '%s', inode: 0x%llx, type: %d (0=file, 1=dir)",
        ntohl(did), name, (long long)st->st_ino, rqst.type);
    rply.namelen = 0;

    if (transmit(db, &rqst, &rply) < 0) {
        errno = CNID_ERR_DB;
        return CNID_INVALID;
    }

    switch (rply.result) {
    case CNID_DBD_RES_OK:
        id = rply.cnid;
        LOG(log_debug, logtype_cnid, "cnid_dbd_lookup: got CNID: %u", ntohl(id));
        break;

    case CNID_DBD_RES_NOTFOUND:
        id = CNID_INVALID;
        break;

    case CNID_DBD_RES_ERR_DB:
        errno = CNID_ERR_DB;
        id = CNID_INVALID;
        break;

    default:
        LOG(log_error, logtype_cnid,
            "cnid_dbd_lookup: unexpected result code %d from CNID server", rply.result);
        abort();
    }

    return id;
}

/* ---------------------- */
/*!
 * @brief Backend implementation of cnid_find() for the DBD daemon backend
 *
 * Parameter validation is performed once in the wrapper at libatalk/cnid/cnid.c.
 * This function trusts that cdb, name, namelen, buffer and buflen are sane and
 * focuses on driving the paginated SEARCH RPC to cnid_dbd.
 *
 * The single cleanup path (`goto out`) ensures signal-handler restoration and
 * `more_available` write-back happen in exactly one place — every error arm
 * goes through it. *more_available is set to true if the result set was
 * truncated by the daemon offset cap, the wall-clock deadline, signal-induced
 * abort, or the buffer-fit guard; written only on success (the wrapper has
 * already pre-zeroed it for error paths).
 */
int cnid_dbd_find(struct _cnid_db *cdb, const char *name, size_t namelen,
                  void *buffer, size_t buflen, bool *more_available)
{
    CNID_bdb_private *db = cdb->cnid_db_private;
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply;
    int      total = 0;
    int      batch;
    int      max_results;
    /* Default: error. */
    int      rv = -1;
    uint32_t offset = 0;
    bool     more_local = false;
    /* Signal-induced loop exit. */
    bool     stop_aborted = false;
    /* 4-byte offset prefix + name. */
    char     payload[sizeof(uint32_t) + MAXPATHLEN];
    time_t   deadline_ts;
    struct sigaction sa, old_sa_term, old_sa_int;
    max_results = (int)(buflen / sizeof(cnid_t));
    deadline_ts = time(NULL) + DBD_FIND_DEADLINE_SEC;
    /* %.*s is critical: name is documented as not necessarily NUL-
     * terminated. namelen is bounded in [1, MAXPATHLEN - sizeof(uint32_t)]
     * by the cnid_find() wrapper, so the (int) cast is safe. */
    LOG(log_debug, logtype_cnid,
        "cnid_find(\"%.*s\"), max_results=%d, deadline=%ds",
        (int)namelen, name, max_results, DBD_FIND_DEADLINE_SEC);
    /* Install sigaction once across all batches; ~100x fewer syscalls vs
     * per-batch transmit() install/restore. */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, &old_sa_term);
    sigaction(SIGINT,  &sa, &old_sa_int);
    stop_signal = 0;

    while (total < max_results) {
        /* Loop exit due to signal: treat the partial total as a successful
         * return; cleanup raises the signal afterwards. stop_aborted=true
         * forces more_available=true so a 0-result signal-aborted search is
         * not misreported as "no matches". */
        if (stop_signal) {
            stop_aborted = true;
            break;
        }

        if (time(NULL) >= deadline_ts) {
            /* %.*s: see comment at function entry. */
            LOG(log_warning, logtype_cnid,
                "cnid_find(\"%.*s\"): wall-clock %ds exceeded after %d "
                "results; returning partial set",
                (int)namelen, name, DBD_FIND_DEADLINE_SEC, total);
            more_local = true;
            break;
        }

        if (offset >= (uint32_t)DBD_SEARCH_MAX_OFFSET) {
            /* %.*s: see comment at function entry. */
            LOG(log_warning, logtype_cnid,
                "cnid_find(\"%.*s\"): reached daemon offset cap (%u)",
                (int)namelen, name, (unsigned)DBD_SEARCH_MAX_OFFSET);
            more_local = true;
            break;
        }

        /* Buffer-fit guard: stop if remaining buffer cannot hold a full
         * batch. dbd_rpc() rejects oversize replies. Always allow at least
         * one iteration so a 100-cnid buffer still gets its full reply. */
        if ((max_results - total) < DBD_MAX_SRCH_RSLTS && total > 0) {
            more_local = true;
            break;
        }

        /* Build SEARCH payload: 4-byte native-byte-order offset prefix
         * followed by the search-name bytes. */
        memcpy(payload, &offset, sizeof(offset));
        memcpy(payload + sizeof(offset), name, namelen);
        RQST_RESET(&rqst);
        /* Defensive: zero rply between iterations so a future partial-write
         * of rply (e.g. on short read) cannot leak stale fields from the
         * prior batch into this one. */
        memset(&rply, 0, sizeof(rply));
        rqst.op      = CNID_DBD_OP_SEARCH;
        rqst.name    = payload;
        rqst.namelen = sizeof(offset) + namelen;
        rply.name    = (char *)buffer + ((size_t)total * sizeof(cnid_t));
        rply.namelen = (size_t)(max_results - total) * sizeof(cnid_t);

        if (transmit_locked(db, &rqst, &rply) < 0) {
            /*
             * transmit_locked() returns -1 for both genuine I/O failures
             * (ECONNREFUSED, MAX_DELAY timeout) and signal-driven exits.
             * The only side-channel that disambiguates them is stop_signal
             * itself, which is left non-zero only by the signal arm.
             *
             * Treat the signal case as a partial-success exit: report the
             * partial total to the caller with more_available = true.
             * Test stop_signal first so a real I/O error coincident with
             * a stale stop_signal still surfaces a partial-but-aborted
             * return rather than a spurious CNID_ERR_DB.
             */
            if (stop_signal) {
                stop_aborted = true;
                break;
            }

            errno = CNID_ERR_DB;
            /* rv stays -1 */
            goto out;
        }

        switch (rply.result) {
        case CNID_DBD_RES_SRCH_CNT:
            batch = (int)(rply.namelen / sizeof(cnid_t));
            total  += batch;
            offset += (uint32_t)batch;
            more_local = true;
            LOG(log_debug, logtype_cnid,
                "cnid_find: batch of %d (total %d), continuing",
                batch, total);
            break;

        case CNID_DBD_RES_SRCH_DONE:
            batch = (int)(rply.namelen / sizeof(cnid_t));
            total += batch;
            more_local = false;
            LOG(log_debug, logtype_cnid,
                "cnid_find: final batch of %d (total %d)", batch, total);
            goto success;

        case CNID_DBD_RES_ERR_DB:
            LOG(log_error, logtype_cnid, "cnid_find: daemon DB error");
            errno = CNID_ERR_DB;
            /* rv stays -1 */
            goto out;

        default:
            /* Per operator policy, mixed-version deployments are out of
             * scope; any unexpected reply code (including the legacy
             * RES_OK/RES_NOTFOUND from a pre-pagination daemon) is a
             * protocol violation that aborts the worker, matching the
             * convention of every other cnid_dbd_* op in this file. */
            LOG(log_error, logtype_cnid,
                "cnid_dbd_find: unexpected result code %d", rply.result);
            abort();
        }
    }

success:
    rv = total;
out:

    /* Contract: write back only on success. On error the caller MUST NOT
     * inspect the value (documented in cnid.c). The wrapper pre-zeroes
     * *more_available, so leaving it untouched on error is safe.
     * stop_aborted ORs into more_local so a signal-aborted search before
     * the first batch still reports the search as incomplete. */
    if (more_available) {
        *more_available = (rv < 0) ? false : (more_local || stop_aborted);
    }

    sigaction(SIGTERM, &old_sa_term, NULL);
    sigaction(SIGINT,  &old_sa_int,  NULL);

    /* Note: stop_signal raised after returning the partial total to the
     * caller, matching transmit()'s pattern. */
    if (stop_signal) {
        raise(stop_signal);
    }

    return rv;
}

/* ---------------------- */
int cnid_dbd_update(struct _cnid_db *cdb, cnid_t id, const struct stat *st,
                    cnid_t did, const char *name, size_t len)
{
    CNID_bdb_private *db;
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply;

    if (!cdb || !(db = cdb->cnid_db_private) || !id || !st || !name) {
        LOG(log_error, logtype_cnid, "cnid_update: Parameter error");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid, "cnid_update: Path name is too long");
        errno = CNID_ERR_PATH;
        return -1;
    }

    RQST_RESET(&rqst);
    rqst.op = CNID_DBD_OP_UPDATE;
    rqst.cnid = id;

    if (!(cdb->cnid_db_flags & CNID_FLAG_NODEV)) {
        rqst.dev = st->st_dev;
    }

    rqst.ino = st->st_ino;
    rqst.type = S_ISDIR(st->st_mode) ? 1 : 0;
    rqst.did = did;
    rqst.name = name;
    rqst.namelen = len;
    LOG(log_debug, logtype_cnid,
        "cnid_dbd_update: CNID: %u, name: '%s', inode: 0x%llx, type: %d (0=file, 1=dir)",
        ntohl(id), name, (long long)st->st_ino, rqst.type);
    rply.namelen = 0;

    if (transmit(db, &rqst, &rply) < 0) {
        errno = CNID_ERR_DB;
        return -1;
    }

    switch (rply.result) {
    case CNID_DBD_RES_OK:
        LOG(log_debug, logtype_cnid, "cnid_dbd_update: updated");

    case CNID_DBD_RES_NOTFOUND:
        return 0;

    case CNID_DBD_RES_ERR_DB:
        errno = CNID_ERR_DB;
        return -1;

    default:
        LOG(log_error, logtype_cnid,
            "cnid_dbd_update: unexpected result code %d from CNID server", rply.result);
        abort();
    }
}

/* ---------------------- */
cnid_t cnid_dbd_rebuild_add(struct _cnid_db *cdb, const struct stat *st,
                            cnid_t did, const char *name, size_t len, cnid_t hint)
{
    CNID_bdb_private *db;
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply;
    cnid_t id;

    if (!cdb || !(db = cdb->cnid_db_private) || !st || !name
            || hint == CNID_INVALID) {
        LOG(log_error, logtype_cnid, "cnid_rebuild_add: Parameter error");
        errno = CNID_ERR_PARAM;
        return CNID_INVALID;
    }

    if (len > MAXPATHLEN) {
        LOG(log_error, logtype_cnid, "cnid_rebuild_add: Path name is too long");
        errno = CNID_ERR_PATH;
        return CNID_INVALID;
    }

    RQST_RESET(&rqst);
    rqst.op = CNID_DBD_OP_REBUILD_ADD;

    if (!(cdb->cnid_db_flags & CNID_FLAG_NODEV)) {
        rqst.dev = st->st_dev;
    }

    rqst.ino = st->st_ino;
    rqst.type = S_ISDIR(st->st_mode) ? 1 : 0;
    rqst.did = did;
    rqst.name = name;
    rqst.namelen = len;
    rqst.cnid = hint;
    LOG(log_debug, logtype_cnid,
        "cnid_dbd_rebuild_add: CNID: %u, name: '%s', inode: 0x%llx, type: %d (0=file, 1=dir), hint: %u",
        ntohl(did), name, (long long)st->st_ino, rqst.type, hint);

    if (transmit(db, &rqst, &rply) < 0) {
        errno = CNID_ERR_DB;
        return CNID_INVALID;
    }

    switch (rply.result) {
    case CNID_DBD_RES_OK:
        id = rply.cnid;
        LOG(log_debug, logtype_cnid, "cnid_dbd_rebuild_add: got CNID: %u", ntohl(id));
        break;

    case CNID_DBD_RES_ERR_MAX:
        errno = CNID_ERR_MAX;
        id = CNID_INVALID;
        break;

    case CNID_DBD_RES_ERR_DB:
    case CNID_DBD_RES_ERR_DUPLCNID:
        errno = CNID_ERR_DB;
        id = CNID_INVALID;
        break;

    default:
        LOG(log_error, logtype_cnid,
            "cnid_dbd_rebuild_add: unexpected result code %d from CNID server",
            rply.result);
        abort();
    }

    return id;
}

/* ---------------------- */
int cnid_dbd_delete(struct _cnid_db *cdb, const cnid_t id)
{
    CNID_bdb_private *db;
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply;

    if (!cdb || !(db = cdb->cnid_db_private) || !id) {
        LOG(log_error, logtype_cnid, "cnid_delete: Parameter error");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    LOG(log_debug, logtype_cnid, "cnid_dbd_delete: delete CNID: %u", ntohl(id));
    RQST_RESET(&rqst);
    rqst.op = CNID_DBD_OP_DELETE;
    rqst.cnid = id;
    rply.namelen = 0;

    if (transmit(db, &rqst, &rply) < 0) {
        errno = CNID_ERR_DB;
        return -1;
    }

    switch (rply.result) {
    case CNID_DBD_RES_OK:
        LOG(log_debug, logtype_cnid, "cnid_dbd_delete: deleted CNID: %u", ntohl(id));

    case CNID_DBD_RES_NOTFOUND:
        return 0;

    case CNID_DBD_RES_ERR_DB:
        errno = CNID_ERR_DB;
        return -1;

    default:
        LOG(log_error, logtype_cnid,
            "cnid_dbd_delete: unexpected result code %d from CNID server", rply.result);
        abort();
    }
}

int cnid_dbd_wipe(struct _cnid_db *cdb)
{
    CNID_bdb_private *db;
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply = { 0 };

    if (!cdb || !(db = cdb->cnid_db_private)) {
        LOG(log_error, logtype_cnid, "cnid_wipe: Parameter error");
        errno = CNID_ERR_PARAM;
        return -1;
    }

    LOG(log_debug, logtype_cnid, "cnid_dbd_wipe");
    RQST_RESET(&rqst);
    rqst.op = CNID_DBD_OP_WIPE;
    rqst.cnid = 0;
    rply.namelen = 0;

    if (transmit(db, &rqst, &rply) < 0) {
        errno = CNID_ERR_DB;
        return -1;
    }

    if (rply.result != CNID_DBD_RES_OK) {
        errno = CNID_ERR_DB;
        return -1;
    }

    LOG(log_debug, logtype_cnid, "cnid_dbd_wipe: ok");
    return cnid_dbd_stamp(db);
}


struct _cnid_module cnid_dbd_module = {
    "dbd",
    {NULL, NULL},
    cnid_dbd_open,
    0
};
