/*
  Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>
  Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
  Copyright (c) 2026 Andy Lemin (andylemin)

  dsi_test_header(), dsi_test_cleanup() and the session set-up in
  dsi_test_open() come from test.c.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/dsi.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/queue.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afp_config.h"
#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "filedir.h"
#include "fork.h"
#include "hash.h"
#include "subtests.h"
#include "switch.h"
#include "test.h"
#include "volume.h"


#define rbufsize 128000
static char rbuf[rbufsize];
static size_t rbuflen;

#define ADD(a, b, c) (a) += (c); \
                         (b) += (c)

#define PUSHBUF(p, val, size, len) \
    memcpy((p), (val), (size));    \
    (p) += (size);                 \
    (len) += (size)

#define PUSHVAL(p, t, val, len)         \
    { \
        t type = val;                          \
        memcpy(p, &type, sizeof(t));           \
        (p) += sizeof(t);                      \
        (len) += sizeof(t);                    \
    }

static int push_path(char **bufp, const char *name)
{
    int len = 0;
    int slen = strlen(name);
    char *p = *bufp;
    PUSHVAL(p, uint8_t, 3, len); /* path type */
    PUSHVAL(p, uint32_t, htonl(kTextEncodingUTF8), len); /* text encoding hint */
    PUSHVAL(p, uint16_t, htons(slen), len);

    if (slen) {
        for (int i = 0; i < slen; i++) {
            if (name[i] == '/') {
                p[i] = 0;
            } else {
                p[i] = name[i];
            }
        }

        len += slen;
    }

    *bufp += len;
    return len;
}

/***********************************************************************************
 * Interface
 ***********************************************************************************/

char **cnamewrap(const char *name)
{
    static char buf[256] = { 0 };
    static char *p = buf;
    int len = 0;
    PUSHVAL(p, uint8_t, 3, len); /* path type */
    PUSHVAL(p, uint32_t, htonl(kTextEncodingUTF8), len); /* text encoding hint */
    size_t avail = sizeof(buf) - len - sizeof(uint16_t);
    size_t namelen = strnlen(name, avail);
    PUSHVAL(p, uint16_t, htons(namelen), len);
    memcpy(p, name, namelen);
    p = buf;
    return &p;
}

int getfiledirparms(AFPObj *obj, uint16_t vid, cnid_t did, const char *name)
{
    static char buf[256] = { 0 };
    char *p = buf;
    int len = 0;
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    PUSHVAL(p, uint16_t, htons(FILPBIT_FNUM | FILPBIT_PDINFO), len);
    PUSHVAL(p, uint16_t, htons(DIRPBIT_DID | DIRPBIT_PDINFO), len);
    len += push_path(&p, name);
    return afp_getfildirparams(obj, buf, len, rbuf, &rbuflen);
}

int createdir(AFPObj *obj, uint16_t vid, cnid_t did, const char *name)
{
    static char buf[256] = { 0 };
    char *p = buf;
    int len = 0;
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    len += push_path(&p, name);
    return afp_createdir(obj, buf, len, rbuf, &rbuflen);
}

int createfile(AFPObj *obj, uint16_t vid, cnid_t did, const char *name)
{
    static char buf[256] = { 0 };
    char *p = buf;
    int len = 0;
    PUSHVAL(p, uint16_t, htons(128), len); /* hard create */
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    len += push_path(&p, name);
    return afp_createfile(obj, buf, len, rbuf, &rbuflen);
}

int delete (AFPObj *obj, uint16_t vid, cnid_t did, const char *name)
{
    static char buf[256] = { 0 };
    char *p = buf;
    int len = 0;
    PUSHVAL(p, uint16_t, htons(128), len); /* hard create */
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    len += push_path(&p, name);
    return afp_delete(obj, buf, len, rbuf, &rbuflen);
}

int enumerate(AFPObj *obj, uint16_t vid, cnid_t did)
{
    static char buf[256] = { 0 };
    char *p = buf;
    int len = 0;
    int ret;
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    PUSHVAL(p, uint16_t, htons(FILPBIT_PDID | FILPBIT_FNUM | FILPBIT_PDINFO), len);
    PUSHVAL(p, uint16_t, htons(DIRPBIT_PDID | DIRPBIT_DID | DIRPBIT_PDINFO), len);
    PUSHVAL(p, uint16_t, htons(20), len);       /* reqcount */
    PUSHVAL(p, uint32_t, htonl(1), len);        /* startindex */
    PUSHVAL(p, uint32_t, htonl(rbufsize), len); /* max replysize */
    len += push_path(&p, "");
    ret = afp_enumerate_ext2(obj, buf, len, rbuf, &rbuflen);

    if (ret != AFPERR_NOOBJ && ret != AFP_OK) {
        return -1;
    }

    return 0;
}

uint16_t openvol(AFPObj *obj, const char *name)
{
    int ret;
    uint16_t bitmap;
    uint16_t vid;
    static char buf[32] = { 0 };
    char *p = buf;
    char len = strlen(name);
    p += 2;
    /* bitmap */
    bitmap = htons(1 << VOLPBIT_VID);
    memcpy(p, &bitmap, 2);
    p += 2;
    /* name */
    *p = len;
    p++;
    memcpy(p, name, len);
    p += len;
    len += 2 + 2 + 1; /* (command+pad) + bitmap + len */

    if (len & 1) {
        len++;
    }

    rbuflen = 0;

    if ((ret = afp_openvol(obj, buf, len, rbuf, &rbuflen)) != AFP_OK) {
        return 0;
    }

    p = rbuf;
    memcpy(&bitmap, p, 2);
    p += 2;
    bitmap = ntohs(bitmap);

    if (!(bitmap & 1 << VOLPBIT_VID)) {
        return 0;
    }

    memcpy(&vid, p, 2);
    return vid;
}

/*! @brief Pack a DSI request header in wire format */
void dsi_test_header(uint8_t *block, uint8_t command, uint16_t request_id,
                     uint32_t code_or_doff, uint32_t len)
{
    uint16_t request_id_be;
    uint32_t code_or_doff_be;
    uint32_t len_be;
    uint32_t reserved_be;
    memset(block, 0, DSI_BLOCKSIZ);
    block[0] = DSIFL_REQUEST;
    block[1] = command;
    request_id_be = htons(request_id);
    code_or_doff_be = htonl(code_or_doff);
    len_be = htonl(len);
    reserved_be = 0;
    memcpy(block + 2, &request_id_be, sizeof(request_id_be));
    memcpy(block + 4, &code_or_doff_be, sizeof(code_or_doff_be));
    memcpy(block + 8, &len_be, sizeof(len_be));
    memcpy(block + 12, &reserved_be, sizeof(reserved_be));
}

/*!
 * @brief Pack a request frame at p: a header of command for request_id with
 *        data offset doff and length len, then len bytes of zeros, the first
 *        afp_command
 *
 * @returns the frame's length
 */
size_t dsi_test_frame(uint8_t *p, uint8_t command, uint16_t request_id,
                      uint32_t doff, uint32_t len, uint8_t afp_command)
{
    dsi_test_header(p, command, request_id, doff, len);
    memset(p + DSI_BLOCKSIZ, 0, len);

    if (len > 0) {
        p[DSI_BLOCKSIZ] = afp_command;
    }

    return DSI_BLOCKSIZ + len;
}

/*!
 * @brief A DSI session on one end of an AF_UNIX socketpair, its command
 *        buffer one quantum as afpd sizes it, its read-ahead buffer one
 *        quantum and its attention quantum the default; peer gets the other
 *        end
 *
 * @returns 0, or -1 with nothing left open
 */
int dsi_test_open(DSI *dsi, uint32_t quantum, int *peer)
{
    int fds[2];
    memset(dsi, 0, sizeof(*dsi));
    dsi->socket = -1;
    dsi->attn_quantum = DSI_DEFQUANT;
    dsi->splice_pipe[0] = dsi->splice_pipe[1] = -1;
    dsi->splice_size = DEFAULT_SPLICE_SIZE;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        return -1;
    }

    dsi->server_quantum = quantum;
    dsi->commands = calloc(1, quantum);
    dsi->buffer = calloc(1, quantum);

    if (dsi->commands == NULL || dsi->buffer == NULL) {
        free(dsi->commands);
        free(dsi->buffer);
        dsi->commands = NULL;
        dsi->buffer = NULL;
        close(fds[0]);
        close(fds[1]);
        return -1;
    }

    dsi->socket = fds[1];
    dsi->start = dsi->eof = dsi->buffer;
    dsi->end = dsi->buffer + quantum;
    *peer = fds[0];
    return 0;
}

/*! @brief Close the session's socket and pipe and free its buffers */
void dsi_test_cleanup(DSI *dsi)
{
    if (dsi->socket != -1) {
        close(dsi->socket);
    }

    dsi_close_pipe(dsi);
    free(dsi->commands);
    free(dsi->buffer);
}

/*!
 * @brief Replace the session's socketpair with a loopback TCP connection,
 *        the transport of a real session; peer gets the client end
 *
 * @returns 0, or -1 with the socketpair left in place
 */
int dsi_test_connect_tcp(DSI *dsi, int *peer)
{
    struct sockaddr_in addr = { .sin_family = AF_INET };
    socklen_t alen = sizeof(addr);
    int lsock;
    int client = -1;
    int server = -1;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return -1;
    }

    if (bind(lsock, (struct sockaddr *)&addr, sizeof(addr)) == 0
            && listen(lsock, 1) == 0
            && getsockname(lsock, (struct sockaddr *)&addr, &alen) == 0
            && (client = socket(AF_INET, SOCK_STREAM, 0)) != -1
            && connect(client, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        server = accept(lsock, NULL, NULL);
    }

    close(lsock);

    if (server == -1 || setnonblock(server, 1) != 0) {
        if (client != -1) {
            close(client);
        }

        if (server != -1) {
            close(server);
        }

        return -1;
    }

    close(dsi->socket);
    close(*peer);
    dsi->socket = server;
    *peer = client;
    return 0;
}

/*!
 * @brief Tell whether errnum, a loopback connection's failure, means there is
 *        no loopback, as in a sandbox without a network of its own or one
 *        that filters it
 */
int dsi_test_no_loopback(int errnum)
{
    return errnum == EADDRNOTAVAIL || errnum == ENETUNREACH
           || errnum == EAFNOSUPPORT || errnum == EACCES || errnum == EPERM
           || errnum == ECONNREFUSED || errnum == ETIMEDOUT;
}

/*! @brief proto_close of a test session: close its socket */
void dsi_test_close_socket(DSI *dsi)
{
    close(dsi->socket);
    dsi->socket = -1;
}

/*!
 * @brief cnid_get() of a session process's CNID backend, which knows no file
 *
 * @returns CNID_INVALID
 */
static cnid_t dsi_test_cnid_get(struct _cnid_db *cdb _U_, cnid_t did _U_,
                                const char *name _U_, size_t len _U_)
{
    return CNID_INVALID;
}

/*!
 * @brief cnid_resolve() of a session process's CNID backend, which knows no
 *        file, so a directory the session's dircache misses is not found
 *
 * @returns NULL
 */
static char *dsi_test_cnid_resolve(struct _cnid_db *cdb _U_, cnid_t *id _U_,
                                   void *buffer _U_, size_t len _U_)
{
    return NULL;
}

/*!
 * @brief cnid_close() of a session process's CNID backend, which holds
 *        nothing
 */
static void dsi_test_cnid_close(struct _cnid_db *cdb _U_)
{
    /* the backend opened nothing */
}

/* the CNID backend of a session process: the calls a fork's and a volume's
 * close make, answered without a database */
static struct _cnid_db dsi_test_cnid = {
    .cnid_get = dsi_test_cnid_get,
    .cnid_resolve = dsi_test_cnid_resolve,
    .cnid_close = dsi_test_cnid_close,
};

/*!
 * @brief Serve a copy of obj in a session process: afp_over_dsi() on the
 *        server end of a loopback TCP connection, with flags added to the
 *        session's DSI flags; peer gets the client end
 *
 * The session process gives each volume with a CNID database a backend that
 * knows no file, so it closes its forks and volumes as a session does: an
 * SQLite connection is unusable across fork(), and a new one in the child
 * meets the SQLite state fork() copied from the harness, which fails on
 * macOS. With uid 0 it ends as afp_over_dsi() ends a session that never
 * logged in, at the end of its stream; with another uid it is logged in, and
 * enters the disconnected state there until a termination. ipc_fd stands for
 * its connection to the master afpd, -1 for none.
 *
 * @returns the session process's pid, or -1 with errno and nothing started
 */
pid_t dsi_test_serve_as(const AFPObj *obj, uid_t uid, int ipc_fd,
                        uint32_t flags, int *peer)
{
    DSI *dsi = calloc(1, sizeof(*dsi));
    pid_t pid = -1;
    int err;

    if (dsi == NULL || dsi_test_open(dsi, DSI_DATASIZ, peer) != 0) {
        free(dsi);
        return -1;
    }

    if (dsi_test_connect_tcp(dsi, peer) == 0) {
        /* nothing buffered is written twice */
        fflush(NULL);
        pid = fork();
    }

    if (pid == 0) {
        AFPObj session = *obj;
        close(*peer);

        for (struct vol *vol = getvolumes(); vol != NULL; vol = vol->v_next) {
            if (vol->v_cdb != NULL) {
                vol->v_cdb = &dsi_test_cnid;
            }
        }

        session.dsi = dsi;
        session.proto = AFPPROTO_DSI;
        session.ipc_fd = ipc_fd;
        session.hint_fd = -1;
        session.uid = uid;
        dsi->AFPobj = &session;
        dsi->proto_close = dsi_test_close_socket;
        dsi->flags |= flags;
        /* the table login sets up for an AFP 3 client */
        afp_switch = postauth_switch;
        postauth_switch[AFP_READ_EXT] = afp_read_ext;
        postauth_switch[AFP_WRITE_EXT] = afp_write_ext;
        afp_over_dsi(&session);
        _exit(0);
    }

    err = errno;

    if (pid == -1) {
        close(*peer);
    }

    dsi_test_cleanup(dsi);
    free(dsi);
    errno = err;
    return pid;
}

/*!
 * @brief dsi_test_serve_as() for a session that never logged in and has no
 *        master afpd
 *
 * @returns the session process's pid, or -1 with errno and nothing started
 */
pid_t dsi_test_serve(const AFPObj *obj, int *peer)
{
    return dsi_test_serve_as(obj, 0, -1, 0, peer);
}

/*!
 * @brief End a session process dsi_test_serve() or dsi_test_serve_as()
 *        started: close the client end, unless peer is -1 for an end the
 *        caller keeps, so the session meets the end of its stream and exits,
 *        then reap it, killing it only after thirty seconds, since a kill
 *        during its exit cuts short what exit() writes; a logged-in session
 *        needs a termination first
 *
 * @returns 0 when the session exited, whatever its status, which valgrind
 *          sets for the harness's memory the session inherits and never
 *          frees; -1 when a signal or the kill ended it
 */
int dsi_test_reap(pid_t pid, int peer)
{
    struct timespec tick = { .tv_nsec = 10 * 1000 * 1000 };
    int status = -1;

    if (peer != -1) {
        close(peer);
    }

    for (int i = 0; i < 3000; i++) {
        if (waitpid(pid, &status, WNOHANG) != 0) {
            return WIFEXITED(status) ? 0 : -1;
        }

        nanosleep(&tick, NULL);
    }

    kill(pid, SIGKILL);
    waitpid(pid, NULL, 0);
    return -1;
}

/*!
 * @brief Limit the process's file size to limit bytes, SIGXFSZ ignored so
 *        that a write past it fails with EFBIG, as on a volume filling up
 *
 * @returns 0, or -1 with the limit and the signal's disposition unchanged
 */
int fsize_cap(struct fsize_cap *cap, rlim_t limit)
{
    struct rlimit capped;

    if (getrlimit(RLIMIT_FSIZE, &cap->saved) != 0) {
        return -1;
    }

    capped = cap->saved;
    capped.rlim_cur = limit;
    cap->xfsz = signal(SIGXFSZ, SIG_IGN);

    if (setrlimit(RLIMIT_FSIZE, &capped) != 0) {
        signal(SIGXFSZ, cap->xfsz);
        return -1;
    }

    return 0;
}

/*!
 * @brief Give back the file size limit and the SIGXFSZ disposition that
 *        fsize_cap() changed
 *
 * @returns 0, or -1 when the limit stays capped
 */
int fsize_uncap(const struct fsize_cap *cap)
{
    int rc = setrlimit(RLIMIT_FSIZE, &cap->saved);
    signal(SIGXFSZ, cap->xfsz);
    return rc;
}

/*!
 * @brief FPOpenFork; on AFP_OK refnum holds the fork reference as
 *        afp_openfork() writes it and of_find() reads it
 */
int openfork(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
             uint8_t fork, uint16_t access, uint16_t *refnum)
{
    static char buf[256] = { 0 };
    char *p = buf;
    int len = 0;
    int ret;
    /* command byte, then the fork selector */
    ADD(p, len, 1);
    PUSHVAL(p, uint8_t, fork, len);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    /* bitmap: no parameters in the reply */
    PUSHVAL(p, uint16_t, 0, len);
    PUSHVAL(p, uint16_t, htons(access), len);
    len += push_path(&p, name);
    ret = afp_openfork(obj, buf, len, rbuf, &rbuflen);

    if (ret == AFP_OK) {
        memcpy(refnum, rbuf + 2, sizeof(*refnum));
    }

    return ret;
}

/*! @brief FPCloseFork of refnum */
int closefork(AFPObj *obj, uint16_t refnum)
{
    static char buf[4] = { 0 };
    memcpy(buf + 2, &refnum, sizeof(refnum));
    return afp_closefork(obj, buf, sizeof(buf), rbuf, &rbuflen);
}
