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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/dsi.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/queue.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "acls.h"
#include "afp_config.h"
#include "dircache.h"
#include "directory.h"
#include "extattrs.h"
#include "file.h"
#include "filedir.h"
#include "fork.h"
#include "hash.h"
#include "subtests.h"
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

/*!
 * @brief Push an EA name: a pad byte when its length word would start at
 *        an odd address, the length, the bytes
 */
static int push_eaname(char **bufp, const char *attr)
{
    uint16_t attrlen = strlen(attr);
    char *p = *bufp;
    int len = 0;

    if ((unsigned long)p & 1) {
        p++;
        len++;
    }

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
 * @brief A DSI session on one end of an AF_UNIX socketpair, its command
 *        buffer one quantum as afpd sizes it and its read-ahead buffer one
 *        quantum; peer gets the other end
 *
 * @returns 0, or -1 with nothing left open
 */
int dsi_test_open(DSI *dsi, uint32_t quantum, int *peer)
{
    int fds[2];
    memset(dsi, 0, sizeof(*dsi));
    dsi->socket = -1;
    dsi->splice_pipe[0] = dsi->splice_pipe[1] = -1;

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

    if (dsi->splice_pipe[0] != -1) {
        close(dsi->splice_pipe[0]);
        close(dsi->splice_pipe[1]);
    }

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

#ifdef HAVE_ACLS
/*!
 * @brief FPAccess: does the session's user hold rights on name under did?
 *
 * The UUID field stays zero: the POSIX ACL path takes the user from obj.
 */
int checkaccess(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                uint32_t rights)
{
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
}
#endif /* HAVE_ACLS */

/*!
 * @brief FPSetForkParms setting the length of refnum's fork, its data or
 *        resource fork as opened
 */
int setforkparams(AFPObj *obj, uint16_t refnum, uint32_t len)
{
    static char buf[10] = { 0 };
    const struct ofork *of = of_find(refnum);
    uint16_t bitmap = htons(1 << (of && (of->of_flags & AFPFORK_RSRC)
                                  ? FILPBIT_RFLEN : FILPBIT_DFLEN));
    uint32_t len_be = htonl(len);
    memcpy(buf + 2, &refnum, sizeof(refnum));
    memcpy(buf + 4, &bitmap, sizeof(bitmap));
    memcpy(buf + 6, &len_be, sizeof(len_be));
    return afp_setforkparams(obj, buf, sizeof(buf), rbuf, &rbuflen);
}

/*!
 * @brief Touch a file until its change time reaches a new second
 *
 * File times come from the kernel's coarse clock, which trails time().
 *
 * @param[in] path  file to touch
 *
 * @returns 0, or -1 if the file cannot be touched or its change time does
 *          not move within five seconds
 */
int next_second(const char *path)
{
    const struct timespec tick = { .tv_sec = 0, .tv_nsec = 10 * 1000 * 1000 };
    struct stat st;
    time_t start = -1;
    int tries = 500;

    while (tries-- > 0 && utimensat(AT_FDCWD, path, NULL, 0) == 0
            && stat(path, &st) == 0) {
        if (start == -1) {
            start = st.st_ctime;
        } else if (st.st_ctime != start) {
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

    if ((uintptr_t)p & 1) {
        ADD(p, reqlen, 1);
    }

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
 * @brief FPListExtAttrs of name under did, the reply into reply
 */
int listextattr(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
                uint32_t maxreply, char *reply, size_t *replylen)
{
    static char buf[1024] = { 0 };
    char *p = buf;
    int len = 0;
    ADD(p, len, 2);
    PUSHVAL(p, uint16_t, vid, len);
    PUSHVAL(p, cnid_t, did, len);
    /* bitmap, reqcount, startindex, maxreply */
    PUSHVAL(p, uint16_t, 0, len);
    PUSHVAL(p, uint16_t, 0, len);
    PUSHVAL(p, uint32_t, 0, len);
    PUSHVAL(p, uint32_t, htonl(maxreply), len);
    len += push_path(&p, name);
    return afp_listextattr(obj, buf, len, reply, replylen);
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
 * @brief FPWrite of len bytes at offset to refnum; the harness has no
 *        session stream, so no data moves
 */
int writefork(AFPObj *obj, uint16_t refnum, uint32_t offset, uint32_t len)
{
    static char buf[12] = { 0 };
    uint32_t offset_be = htonl(offset);
    uint32_t len_be = htonl(len);
    memcpy(buf + 2, &refnum, sizeof(refnum));
    memcpy(buf + 4, &offset_be, sizeof(offset_be));
    memcpy(buf + 8, &len_be, sizeof(len_be));
    return afp_write(obj, buf, sizeof(buf), rbuf, &rbuflen);
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

#ifdef HAVE_ACLS
/*!
 * @brief FPSetACL on name under did: bitmap, then len bytes of UUIDs and
 *        ACL
 */
int setacl(AFPObj *obj, uint16_t vid, cnid_t did, const char *name,
           uint16_t bitmap, const void *body, size_t len)
{
    static char buf[256] = { 0 };
    int reqlen = params_request(buf, vid, did, name, bitmap, body, len);
    return afp_setacl(obj, buf, reqlen, rbuf, &rbuflen);
}
#endif /* HAVE_ACLS */
