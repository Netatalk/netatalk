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
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
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

#include "acls.h"
#include "afp_config.h"
#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "extattrs.h"
#include "file.h"
#include "filedir.h"
#include "fork.h"
#include "hash.h"
#include "subtests.h"
#include "switch.h"
#include "test.h"
#include "virtual_icon.h"
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

/*! @brief Step a request pointer to an even address, where afpd reads words */
static void pad_even(char **bufp, int *len)
{
    char *p = *bufp;

    if ((uintptr_t)p & 1) {
        *bufp = p + 1;
        (*len)++;
    }
}

/*!
 * @brief Push an EA name: a pad byte when its length word would start at
 *        an odd address, the length, the bytes
 */
static int push_eaname(char **bufp, const char *attr)
{
    uint16_t attrlen = (uint16_t)strlen(attr);
    char *p = *bufp;
    int len = 0;
    pad_even(&p, &len);
    PUSHVAL(p, uint16_t, htons(attrlen), len);
    PUSHBUF(p, attr, attrlen, len);
    *bufp = p;
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

/*!
 * @brief FPGetFileDirParms for name under did with the given bitmaps
 *
 * @param[in] obj      the test session
 * @param[in] vid      the open volume
 * @param[in] did      the parent folder
 * @param[in] name     the object, "" for the folder itself
 * @param[in] fbitmap  the file bitmap
 * @param[in] dbitmap  the directory bitmap
 * @returns the AFP result
 */
int getfiledirparms_bitmap(AFPObj *obj, uint16_t vid, cnid_t did,
                           const char *name, uint16_t fbitmap,
                           uint16_t dbitmap)
{
    static char buf[256] = { 0 };
    char *p = buf;
    int len = 0;
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    PUSHVAL(p, uint16_t, htons(fbitmap), len);
    PUSHVAL(p, uint16_t, htons(dbitmap), len);
    len += push_path(&p, name);
    return afp_getfildirparams(obj, buf, len, rbuf, &rbuflen);
}

/*!
 * @brief FPGetFileDirParms for name under did, asking for the CNID and name
 *
 * @param[in] obj   the test session
 * @param[in] vid   the open volume
 * @param[in] did   the parent folder
 * @param[in] name  the object
 * @returns the AFP result
 */
int getfiledirparms(AFPObj *obj, uint16_t vid, cnid_t did, const char *name)
{
    return getfiledirparms_bitmap(obj, vid, did, name,
                                  (1 << FILPBIT_FNUM) | (1 << FILPBIT_PDINFO),
                                  (1 << DIRPBIT_DID) | (1 << DIRPBIT_PDINFO));
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

/*! @brief FPRename of name in did to newname */
int renameobj(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
              const char *newname)
{
    static char buf[512] = { 0 };
    char *p = buf;
    int len = 0;
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    len += push_path(&p, name);
    len += push_path(&p, newname);
    return afp_rename(obj, buf, len, rbuf, &rbuflen);
}

/*!
 * @brief FPEnumerateExt2 of did with the given bitmaps and range
 *
 * @param[in] obj      the test session
 * @param[in] vid      the open volume
 * @param[in] did      the folder to list
 * @param[in] fbitmap  the file bitmap
 * @param[in] dbitmap  the directory bitmap
 * @param[in] sindex   the first entry, 1-based
 * @param[in] reqcnt   the number of entries asked for
 * @returns the AFP result
 */
int enumerate_range(AFPObj *obj, uint16_t vid, cnid_t did, uint16_t fbitmap,
                    uint16_t dbitmap, uint32_t sindex, uint16_t reqcnt)
{
    static char buf[256] = { 0 };
    char *p = buf;
    int len = 0;
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    PUSHVAL(p, uint16_t, htons(fbitmap), len);
    PUSHVAL(p, uint16_t, htons(dbitmap), len);
    PUSHVAL(p, uint16_t, htons(reqcnt), len);
    PUSHVAL(p, uint32_t, htonl(sindex), len);
    PUSHVAL(p, uint32_t, htonl(rbufsize), len);
    len += push_path(&p, "");
    /* enumerate() reads *rbuflen as the reply buffer's capacity */
    rbuflen = rbufsize;
    return afp_enumerate_ext2(obj, buf, len, rbuf, &rbuflen);
}

/*!
 * @brief FPEnumerateExt2 of the first twenty entries of did, asking for the
 *        parent, CNID and name of each
 *
 * @param[in] obj  the test session
 * @param[in] vid  the open volume
 * @param[in] did  the folder to list
 * @returns 0 when the listing answered, with or without entries, else -1
 */
int enumerate(AFPObj *obj, uint16_t vid, cnid_t did)
{
    int ret = enumerate_range(obj, vid, did,
                              (1 << FILPBIT_PDID) | (1 << FILPBIT_FNUM) |
                              (1 << FILPBIT_PDINFO),
                              (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                              (1 << DIRPBIT_PDINFO),
                              1, 20);

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
 * @brief Serve a built-in icon as the virtual icon file at the volume root
 *
 * @returns 0, or -1 when the volume builds no icon
 */
int vi_lend(struct vol *vol, struct vi_lent *lent)
{
    lent->legacy = vol->v_legacyicon;
    lent->icon = vol->v_icon_rfork;
    lent->icon_len = vol->v_icon_rfork_len;
    vol->v_icon_rfork = NULL;
    vol->v_legacyicon = "globe";
    virtual_icon_init(vol);
    return vol->v_icon_rfork != NULL ? 0 : -1;
}

/*! @brief Free the lent icon and give the volume its own settings back */
void vi_return(struct vol *vol, const struct vi_lent *lent)
{
    free(vol->v_icon_rfork);
    vol->v_icon_rfork = lent->icon;
    vol->v_icon_rfork_len = lent->icon_len;
    vol->v_legacyicon = lent->legacy;
}

/*!
 * @brief FPOpenFork asking for the file parameters in bitmap; on AFP_OK
 *        refnum holds the fork reference as afp_openfork() writes it and
 *        of_find() reads it
 */
int openfork_bitmap(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                    uint8_t fork, uint16_t access, uint16_t bitmap,
                    uint16_t *refnum)
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
    PUSHVAL(p, uint16_t, htons(bitmap), len);
    PUSHVAL(p, uint16_t, htons(access), len);
    len += push_path(&p, name);
    rbuflen = sizeof(rbuf);
    ret = afp_openfork(obj, buf, len, rbuf, &rbuflen);

    if (ret == AFP_OK) {
        memcpy(refnum, rbuf + 2, sizeof(*refnum));
    }

    return ret;
}

/*!
 * @brief FPOpenFork with no parameters in the reply
 */
int openfork(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
             uint8_t fork, uint16_t access, uint16_t *refnum)
{
    return openfork_bitmap(obj, vid, did, name, fork, access, 0, refnum);
}

/*! @brief FPCloseFork of refnum */
int closefork(AFPObj *obj, uint16_t refnum)
{
    static char buf[4] = { 0 };
    memcpy(buf + 2, &refnum, sizeof(refnum));
    return afp_closefork(obj, buf, sizeof(buf), rbuf, &rbuflen);
}

/*!
 * @brief FPAccess: does the session's user hold rights on name under did?
 *
 * The UUID field stays zero: check_acl_access() never reads it, so both ACL
 * paths take the user from obj.
 *
 * @param[in] obj     the test session
 * @param[in] vid     the open volume
 * @param[in] did     the parent folder
 * @param[in] name    the object, "" for the folder itself
 * @param[in] rights  the Darwin rights asked for
 * @returns the AFP result; -1 where no ACL can be read, which a build
 *          without ACL support answers for every request
 */
int checkaccess(AFPObj *obj _U_, uint16_t vid _U_, cnid_t did _U_,
                const char *name _U_, uint32_t rights _U_)
{
#ifdef HAVE_ACLS
    static char buf[1024] = { 0 };
    char *p = buf;
    int len = 0;
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    /* bitmap, UUID */
    PUSHVAL(p, uint16_t, 0, len);
    ADD(p, len, UUID_BINSIZE);
    PUSHVAL(p, uint32_t, htonl(rights), len);
    len += push_path(&p, name);
    return afp_access(obj, buf, len, rbuf, &rbuflen);
#else
    return -1;
#endif
}

/*!
 * @brief FPResolveID: the parameters of the file a CNID names
 *
 * @param[in] obj   the test session
 * @param[in] vid   the open volume
 * @param[in] cnid  the file's ID
 * @returns the AFP result
 */
int resolveid(AFPObj *obj, uint16_t vid, cnid_t cnid)
{
    static char buf[256] = { 0 };
    char *p = buf;
    int len = 0;
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, cnid, len);
    PUSHVAL(p, uint16_t, htons(1 << FILPBIT_FNUM), len);
    return afp_resolveid(obj, buf, len, rbuf, &rbuflen);
}

/*!
 * @brief Touch a file until its modification time reaches a new second,
 *        then wait a tick
 *
 * A file's stamp may trail or lead time() by a tick of the kernel's coarse
 * clock; the wait after the stamp moves lets every clock reach its second.
 *
 * @param[in] path  file to touch
 *
 * @returns 0, or -1 if the file cannot be touched or its modification time
 *          does not move within five seconds
 */
int next_second(const char *path)
{
    const struct timespec tick = { .tv_sec = 0, .tv_nsec = 20 * 1000 * 1000 };
    const time_t deadline = time(NULL) + 5;
    struct stat st;
    time_t start = -1;

    while (time(NULL) < deadline && utimensat(AT_FDCWD, path, NULL, 0) == 0
            && stat(path, &st) == 0) {
        if (start == -1) {
            start = st.st_mtime;
        } else if (st.st_mtime != start) {
            nanosleep(&tick, NULL);
            return 0;
        }

        nanosleep(&tick, NULL);
    }

    return -1;
}

/*!
 * @brief Build the request FPSetDirParms and FPSetACL share: pad, volume,
 *        directory, bitmap, path, then len bytes of parameters from an
 *        even address
 *
 * @returns the request's length
 */
static int params_request(char *buf, uint16_t vid, cnid_t did,
                          const char *name, uint16_t bitmap,
                          const void *params, size_t len)
{
    char *p = buf;
    int reqlen = 0;
    ADD(p, reqlen, 2);
    PUSHVAL(p, uint16_t, vid, reqlen);
    PUSHVAL(p, cnid_t, did, reqlen);
    PUSHVAL(p, uint16_t, htons(bitmap), reqlen);
    reqlen += push_path(&p, name);
    pad_even(&p, &reqlen);

    if (len > 0) {
        PUSHBUF(p, params, len, reqlen);
    }

    return reqlen;
}

/*!
 * @brief FPSetDirParms of name under did: bitmap, then len bytes of
 *        parameters
 */
int setdirparms(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                uint16_t bitmap, const void *params, size_t len)
{
    static char buf[256] = { 0 };
    int reqlen = params_request(buf, vid, did, name, bitmap, params, len);
    return afp_setdirparams(obj, buf, reqlen, rbuf, &rbuflen);
}

/*!
 * @brief FPGetExtAttr of attr on name under did, the reply into reply
 */
int getextattr(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
               const char *attr, uint32_t maxreply, char *reply,
               size_t *replylen)
{
    static char buf[1024] = { 0 };
    char *p = buf;
    int len = 0;
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    /* bitmap, offset (always 0), reqcount (always -1), maxreply */
    PUSHVAL(p, uint16_t, 0, len);
    PUSHVAL(p, int64_t, 0, len);
    PUSHVAL(p, int64_t, -1, len);
    PUSHVAL(p, uint32_t, htonl(maxreply), len);
    len += push_path(&p, name);
    len += push_eaname(&p, attr);
    return afp_getextattr(obj, buf, len, reply, replylen);
}

/*!
 * @brief FPSetExtAttr of attr on name under did to len bytes of value,
 *        up to MAX_EA_SIZE
 */
int setextattr(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
               const char *attr, const char *value, uint32_t len)
{
    static char buf[8192] = { 0 };
    char *p = buf;
    int reqlen = 0;
    ADD(p, reqlen, 2);
    PUSHVAL(p, uint16_t, vid, reqlen);
    PUSHVAL(p, cnid_t, did, reqlen);
    /* bitmap, offset (always 0) */
    PUSHVAL(p, uint16_t, 0, reqlen);
    PUSHVAL(p, int64_t, 0, reqlen);
    reqlen += push_path(&p, name);
    reqlen += push_eaname(&p, attr);
    PUSHVAL(p, uint32_t, htonl(len), reqlen);

    if (len > sizeof(buf) - (size_t)reqlen) {
        return AFPERR_PARAM;
    }

    PUSHBUF(p, value, len, reqlen);
    return afp_setextattr(obj, buf, reqlen, rbuf, &rbuflen);
}

/*!
 * @brief FPRemoveExtAttr of attr on name under did
 */
int remextattr(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
               const char *attr)
{
    static char buf[1024] = { 0 };
    char *p = buf;
    int len = 0;
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    /* bitmap */
    PUSHVAL(p, uint16_t, 0, len);
    len += push_path(&p, name);
    len += push_eaname(&p, attr);
    return afp_remextattr(obj, buf, len, rbuf, &rbuflen);
}

/*!
 * @brief FPExchangeFiles of name1 under sid with name2 under did
 */
int exchangefiles(AFPObj *obj, uint16_t vid, cnid_t sid, cnid_t did,
                  const char *name1, const char *name2)
{
    static char buf[256] = { 0 };
    char *p = buf;
    int len = 0;
    /* command byte, then a pad byte */
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, sid, len);
    PUSHVAL(p, cnid_t, did, len);
    len += push_path(&p, name1);
    len += push_path(&p, name2);
    return afp_exchangefiles(obj, buf, len, rbuf, &rbuflen);
}

/*!
 * @brief FPSetACL on name under did: bitmap, then len bytes of UUIDs and
 *        ACL
 *
 * @returns the AFP result, or -1 in a build without ACLs
 */
int setacl(AFPObj *obj _U_, uint16_t vid _U_, cnid_t did _U_,
           const char *name _U_, uint16_t bitmap _U_, const void *body _U_,
           size_t len _U_)
{
#ifdef HAVE_ACLS
    static char buf[256] = { 0 };
    int reqlen = params_request(buf, vid, did, name, bitmap, body, len);
    return afp_setacl(obj, buf, reqlen, rbuf, &rbuflen);
#else
    return -1;
#endif
}

/*!
 * @brief FPFlush of the forks open on volume vid
 */
int flushvol(AFPObj *obj, uint16_t vid)
{
    static char buf[4] = { 0 };
    memcpy(buf + 2, &vid, sizeof(vid));
    return afp_flush(obj, buf, sizeof(buf), rbuf, &rbuflen);
}

/*!
 * @brief FPSetForkParms setting the length of refnum's fork, its data or
 *        resource fork as opened
 */
int setforkparams(AFPObj *obj, uint16_t refnum, uint32_t len)
{
    static char buf[10] = { 0 };
    const struct ofork *of = of_find(refnum);
    uint16_t bitmap = (of && (of->of_flags & AFPFORK_RSRC))
                      ? htons(1 << FILPBIT_RFLEN) : htons(1 << FILPBIT_DFLEN);
    uint32_t len_be = htonl(len);
    memcpy(buf + 2, &refnum, sizeof(refnum));
    memcpy(buf + 4, &bitmap, sizeof(bitmap));
    memcpy(buf + 6, &len_be, sizeof(len_be));
    return afp_setforkparams(obj, buf, sizeof(buf), rbuf, &rbuflen);
}

/*! @brief FPGetForkParms of refnum for the bitmap, the reply in rbuf */
int getforkparams(AFPObj *obj, uint16_t refnum, uint16_t bitmap)
{
    static char buf[6] = { 0 };
    uint16_t bitmap_be = htons(bitmap);
    memcpy(buf + 2, &refnum, sizeof(refnum));
    memcpy(buf + 4, &bitmap_be, sizeof(bitmap_be));
    rbuflen = sizeof(rbuf);
    return afp_getforkparams(obj, buf, sizeof(buf), rbuf, &rbuflen);
}

/*! @brief FPGetVolParms of vid for the bitmap, the reply in rbuf */
int getvolparams(AFPObj *obj, uint16_t vid, uint16_t bitmap)
{
    static char buf[6] = { 0 };
    uint16_t bitmap_be = htons(bitmap);
    memcpy(buf + 2, &vid, sizeof(vid));
    memcpy(buf + 4, &bitmap_be, sizeof(bitmap_be));
    rbuflen = sizeof(rbuf);
    return afp_getvolparams(obj, buf, sizeof(buf), rbuf, &rbuflen);
}
