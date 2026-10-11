/*
 * Copyright (c) 1998 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 2010,2011,2012 Frank Lahm <franklahm@googlemail.com>
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * All rights reserved. See COPYRIGHT.
 *
 * this file provides the following functions:
 * dsi_stream_write:    just write a bunch of bytes.
 * dsi_stream_read:     just read a bunch of bytes.
 * dsi_stream_send:     send a DSI header + data.
 * dsi_stream_receive:  read a DSI header + data.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#if defined(SENDFILE_FLAVOR_LINUX) || defined(SENDFILE_FLAVOR_SOLARIS)
#include <sys/sendfile.h>
#endif

#include <atalk/dsi.h>
#include <atalk/logger.h>
#include <atalk/util.h>

#ifndef MSG_MORE
#define MSG_MORE 0x8000
#endif

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0x40
#endif

/*! Pack a DSI header in wire format */
static void dsi_header_pack_reply(const DSI *dsi, char *buf)
{
    buf[0] = dsi->header.dsi_flags;
    buf[1] = dsi->header.dsi_command;
    memcpy(buf + 2, &dsi->header.dsi_requestID, sizeof(dsi->header.dsi_requestID));
    memcpy(buf + 4, &dsi->header.dsi_data.dsi_code,
           sizeof(dsi->header.dsi_data.dsi_code));
    memcpy(buf + 8, &dsi->header.dsi_len, sizeof(dsi->header.dsi_len));
    memcpy(buf + 12, &dsi->header.dsi_reserved, sizeof(dsi->header.dsi_reserved));
}

/*!
 * @brief Check if we can write to the DSI socket
 *
 * afpd is sleeping too much while trying to send something.
 * May be there's no reader or the reader is also sleeping in write,
 * look if there's some data for us to read, hopefully it will wake up
 * the reader so we can write again.
 *
 * @returns 0 when is possible to send again, -1 on error
 */
static int dsi_peek(DSI *dsi)
{
    static int warned = 0;
    fd_set readfds, writefds;
    int    len;
    int    maxfd;
    int    ret;
    LOG(log_maxdebug, logtype_dsi, "dsi_peek");

    while (1) {
        if (dsi->socket < 0)
            /* e.g. dsi_disconnect() might have disconnected us */
        {
            return -1;
        }

        if (dsi->socket >= FD_SETSIZE) {
            errno = EINVAL;
            return -1;
        }

        maxfd = dsi->socket + 1;
        FD_ZERO(&readfds);
        FD_ZERO(&writefds);

        if (dsi->eof < dsi->end) {
            /* space in read buffer */
            FD_SET(dsi->socket, &readfds);
        } else {
            if (!warned) {
                warned = 1;
                LOG(log_note, logtype_dsi,
                    "dsi_peek: readahead buffer is full, possibly increase -dsireadbuf option");
                LOG(log_note, logtype_dsi,
                    "dsi_peek: dsireadbuf: %d, DSI quantum: %d, effective buffer size: %d",
                    dsi->dsireadbuf,
                    dsi->server_quantum ? dsi->server_quantum : DSI_SERVQUANT_DEF,
                    dsi->end - dsi->buffer);
            }
        }

        FD_SET(dsi->socket, &writefds);

        /* No timeout: if there's nothing to read nor nothing to write,
         * we've got nothing to do at all */
        if ((ret = select(maxfd, &readfds, &writefds, NULL, NULL)) <= 0) {
            if (ret == -1 && errno == EINTR)
                /* we might have been interrupted by out timer, so restart select */
            {
                continue;
            }

            /* give up */
            LOG(log_error, logtype_dsi, "dsi_peek: unexpected select return: %d %s",
                ret, ret < 0 ? strerror(errno) : "");
            return -1;
        }

        if (FD_ISSET(dsi->socket, &writefds)) {
            /* we can write again */
            LOG(log_maxdebug, logtype_dsi, "dsi_peek: can write again");
            break;
        }

        /* Check if there's sth to read, hopefully reading that will unblock the client */
        if (FD_ISSET(dsi->socket, &readfds)) {
            len = dsi->end - dsi->eof; /* it's ensured above that there's space */

            if ((len = recv(dsi->socket, dsi->eof, len, 0)) <= 0) {
                if (len == 0) {
                    LOG(log_error, logtype_dsi, "dsi_peek: EOF");
                    return -1;
                }

                LOG(log_error, logtype_dsi, "dsi_peek: read: %s", strerror(errno));

                if (errno == EAGAIN) {
                    continue;
                }

                return -1;
            }

            LOG(log_maxdebug, logtype_dsi, "dsi_peek: read %d bytes", len);
            dsi->eof += len;
        }
    }

    return 0;
}

/*!
 * Return all bytes up to count from dsi->buffer
 * if there are any buffered there
 */
static size_t from_buf(DSI *dsi, uint8_t *buf, size_t count)
{
    size_t nbe = 0;

    if (dsi->buffer == NULL)
        /* afpd master has no DSI buffering */
    {
        return 0;
    }

    LOG(log_maxdebug, logtype_dsi, "from_buf: %u bytes", count);
    nbe = dsi->eof - dsi->start;

    if (nbe > 0) {
        nbe = MIN((size_t)nbe, count);
        memcpy(buf, dsi->start, nbe);
        dsi->start += nbe;

        if (dsi->eof == dsi->start) {
            dsi->start = dsi->eof = dsi->buffer;
        }
    }

    LOG(log_maxdebug, logtype_dsi,
        "from_buf(read: %u, unread:%u , space left: %u): returning %u",
        dsi->start - dsi->buffer, dsi->eof - dsi->start, dsi->end - dsi->eof, nbe);
    return nbe;
}

/*!
 * @brief Get bytes from buffer dsi->buffer or read from socket
 *
 * 1. Check if there are bytes in the the dsi->buffer buffer.
 * 2. Return bytes from (1) if yes.
 *    Note: this may return fewer bytes then requested in count !!
 * 3. If the buffer was empty, read from the socket.
 */
static ssize_t buf_read(DSI *dsi, uint8_t *buf, size_t count)
{
    ssize_t len;
    LOG(log_maxdebug, logtype_dsi, "buf_read(%u bytes)", count);

    if (!count) {
        return 0;
    }

    len = from_buf(dsi, buf, count); /* 1. */

    if (len) {
        return len;    /* 2. */
    }

    len = readt(dsi->socket, buf, count, 0, 0); /* 3. */
    LOG(log_maxdebug, logtype_dsi, "buf_read(%u bytes): got: %d", count, len);
    return len;
}

/*!
 * @brief Get "length" bytes from buffer and/or socket
 *
 * In order to avoid frequent small reads this tries to read
 * larger chunks (8192 bytes) into a buffer.
 */
static size_t dsi_buffered_stream_read(DSI *dsi, uint8_t *data,
                                       const size_t length)
{
    size_t len;
    size_t buflen;
    LOG(log_maxdebug, logtype_dsi, "dsi_buffered_stream_read: %u bytes", length);
    /* read from buffer dsi->buffer */
    len = from_buf(dsi, data, length);
    dsi->read_count += len;

    /* got enough bytes from there ? */
    if (len == length) {
        /* yes */
        return len;
    }

    /* fill the buffer with 8192 bytes or until buffer is full */
    buflen = MIN(8192, dsi->end - dsi->eof);

    if (buflen > 0) {
        ssize_t ret;
        ret = recv(dsi->socket, dsi->eof, buflen, 0);

        if (ret > 0) {
            dsi->eof += ret;
        }
    }

    /* now get the remaining data */
    if ((buflen = dsi_stream_read(dsi, data + len, length - len)) != length - len) {
        return 0;
    }

    len += buflen;
    return len;
}

/* ---------------------------------------
*/
static void block_sig(DSI *dsi)
{
    dsi->in_write++;
}

/* ---------------------------------------
*/
static void unblock_sig(DSI *dsi)
{
    dsi->in_write--;
}

/*********************************************************************************
 * Public functions
 *********************************************************************************/

/*!
 * @brief Communication error with the client, enter disconnected state
 *
 * 1. close the socket
 * 2. set the DSI_DISCONNECTED flag, remove possible sleep flags
 *
 * @returns  0 if successfully entered disconnected state
 * @returns -1 if ppid is 1 which means afpd master died,
 *             or the session has not logged in: login() refuses uid 0 and
 *             sets AFPobj->uid on success, so a zero uid is no login (the
 *             UAM's logout hook is no mark, most UAMs register none)
 */
int dsi_disconnect(DSI *dsi)
{
    LOG(log_note, logtype_dsi, "dsi_disconnect: entering disconnected state");
    dsi->proto_close(dsi);          /* 1 */
    dsi->flags &= ~(DSI_SLEEPING | DSI_EXTSLEEP); /* 2 */
    dsi->flags |= DSI_DISCONNECTED;

    if (dsi->AFPobj == NULL || dsi->AFPobj->uid == 0) {
        return -1;
    }

    return 0;
}

/*!
 * @brief write raw DSI data
 *
 * Checks against EINTR aren't necessary if all of the signals have SA_RESTART
 * specified.
 *
 * @returns actual bytes written, -1 on error
 */
ssize_t dsi_stream_write(DSI *dsi, void *data, const size_t length, int mode)
{
    size_t written;
    ssize_t len;
    unsigned int flags;
    dsi->in_write++;
    written = 0;
    LOG(log_maxdebug, logtype_dsi, "dsi_stream_write(send: %zd bytes): START",
        length);

    if (dsi->flags & DSI_DISCONNECTED) {
        return -1;
    }

    if (mode & DSI_MSG_MORE) {
        flags = MSG_MORE;
    } else {
        flags = 0;
    }

    while (written < length) {
        len = send(dsi->socket, (uint8_t *) data + written, length - written, flags);

        if (len >= 0) {
            written += len;
            continue;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            LOG(log_debug, logtype_dsi, "dsi_stream_write: send: %s", strerror(errno));

            if (mode == DSI_NOWAIT && written == 0) {
                /* DSI_NOWAIT is used by attention give up in this case. */
                written = -1;
                goto exit;
            }

            /* Try to read sth. in order to break up possible deadlock */
            if (dsi_peek(dsi) != 0) {
                written = -1;
                goto exit;
            }

            /* Now try writing again */
            continue;
        }

        LOG(log_error, logtype_dsi, "dsi_stream_write: %s", strerror(errno));
        written = -1;
        goto exit;
    }

    dsi->write_count += written;
    LOG(log_maxdebug, logtype_dsi, "dsi_stream_write(send: %zd bytes): END",
        length);
exit:
    dsi->in_write--;
    return written;
}

#ifdef WITH_SENDFILE
/*!
 * @brief One sendfile call: the hdrlen unsent header bytes at hdr, then up to
 *        count bytes of fromfd from pos
 *
 * Linux sendfile() takes no header, so there the header goes out first in a
 * send() of its own, which MSG_MORE holds back until the file data follows.
 * The bytes the call put on the socket, header bytes first, land in *sent,
 * on EAGAIN and EINTR as well.
 *
 * @returns 0, or -1 with errno
 */
static int sendfile_call(const DSI *dsi, int fromfd, off_t pos, size_t count,
                         char *hdr, size_t hdrlen, size_t *sent)
{
#if defined(SENDFILE_FLAVOR_LINUX)
    ssize_t len;

    if (hdrlen > 0) {
        len = send(dsi->socket, hdr, hdrlen, MSG_MORE);
    } else {
        len = sendfile(dsi->socket, fromfd, &pos, count);
    }

    *sent = len > 0 ? (size_t)len : 0;
    return len < 0 ? -1 : 0;
#elif defined(SENDFILE_FLAVOR_FREEBSD)
    struct iovec iov = { .iov_base = hdr, .iov_len = hdrlen };
    struct sf_hdtr hdtr = { .headers = &iov, .hdr_cnt = 1 };
    off_t sbytes = 0;
    int ret;
    ret = sendfile(fromfd, dsi->socket, pos, count, hdrlen > 0 ? &hdtr : NULL,
                   &sbytes, 0);
    *sent = (size_t)sbytes;
    return ret;
#elif defined(SENDFILE_FLAVOR_SOLARIS)
    struct sendfilevec vec[2];
    size_t xferred = 0;
    ssize_t len;
    int cnt = 0;

    if (hdrlen > 0) {
        vec[cnt].sfv_fd = SFV_FD_SELF;
        vec[cnt].sfv_flag = 0;
        /* Cast to unsigned long to prevent sign extension of the
         * pointer value for the LFS case; see Apache PR 39463. */
        vec[cnt].sfv_off = (unsigned long)hdr;
        vec[cnt].sfv_len = hdrlen;
        cnt++;
    }

    vec[cnt].sfv_fd = fromfd;
    vec[cnt].sfv_flag = 0;
    vec[cnt].sfv_off = pos;
    vec[cnt].sfv_len = count;
    cnt++;
    len = sendfilev(dsi->socket, vec, cnt, &xferred);
    *sent = len >= 0 ? (size_t)len : xferred;
    return len < 0 ? -1 : 0;
#endif
}

/*!
 * @brief Send the rest of a read reply through the request buffer, a
 *        session quantum at a time as the buffered path reads: the header's
 *        unsent bytes, then the file's from pos until *left is zero or the
 *        file ends
 *
 * @returns 0, or -1 with errno
 */
static int dsi_stream_copy_file(DSI *dsi, int fromfd, off_t pos, size_t *left,
                                char *hdr, size_t hdrlen)
{
    if (hdrlen > 0
            && dsi_stream_write(dsi, hdr, hdrlen, 0) != (ssize_t)hdrlen) {
        return -1;
    }

    while (*left > 0) {
        ssize_t len = pread(fromfd, dsi->commands,
                            MIN(*left, (size_t)dsi->server_quantum), pos);

        if (len == -1 && errno == EINTR) {
            continue;
        }

        if (len == -1) {
            LOG(log_error, logtype_dsi, "dsi_stream_read_file: pread: %s",
                strerror(errno));
            return -1;
        }

        if (len == 0) {
            break;
        }

        if (dsi_stream_write(dsi, dsi->commands, (size_t)len, 0) != len) {
            return -1;
        }

        pos += len;
        *left -= (size_t)len;
    }

    return 0;
}

/*!
 * @brief Send a read reply: a DSI header carrying err and length, then
 *        length bytes of fromfd from offset, at least one
 *
 * @returns 0, or -1 with errno, EIO when the file ends first; a failure
 *          leaves in_write raised for the caller's dsi_readdone(), as
 *          dsi_readinit() does
 */
int dsi_stream_read_file(DSI *dsi, const int fromfd, off_t offset,
                         const size_t length, const int err)
{
    static int warned = 0;
    char block[DSI_BLOCKSIZ];
    size_t hdrleft = sizeof(block);
    size_t left = length;
    LOG(log_maxdebug, logtype_dsi, "dsi_stream_read_file(off: %jd, len: %zu)",
        (intmax_t)offset, length);
    dsi->in_write++;

    if (dsi->flags & DSI_DISCONNECTED) {
        return -1;
    }

    if ((uint64_t)length > UINT32_MAX) {
        errno = EMSGSIZE;
        return -1;
    }

    dsi->flags |= DSI_NOREPLY;
    dsi->header.dsi_flags = DSIFL_REPLY;
    dsi->header.dsi_len = htonl(length);
    dsi->header.dsi_data.dsi_code = htonl(err);
    dsi_header_pack_reply(dsi, block);

    while (left > 0) {
        size_t sent;
        size_t hdrsent;
        /* the call's count and header together stay within a ssize_t */
        int ret = sendfile_call(dsi, fromfd, offset,
                                MIN(left, (size_t)SSIZE_MAX - hdrleft),
                                block + sizeof(block) - hdrleft, hdrleft,
                                &sent);
        hdrsent = MIN(sent, hdrleft);
        hdrleft -= hdrsent;
        offset += sent - hdrsent;
        left -= sent - hdrsent;
        dsi->write_count += sent;

        if (ret == 0) {
            if (sent == 0) {
                break;
            }

            continue;
        }

        if (errno == EINTR || errno == EAGAIN) {
            if (dsi_peek(dsi) != 0) {
                return -1;
            }

            continue;
        }

        if (errno == EINVAL || errno == ENOSYS || errno == EOPNOTSUPP) {
            /* the kernel will not send the rest of this file: the request
             * buffer carries it */
            if (!warned) {
                warned = 1;
                LOG(log_note, logtype_dsi,
                    "dsi_stream_read_file: sendfile: %s, copying the rest",
                    strerror(errno));
            }

            if (dsi_stream_copy_file(dsi, fromfd, offset, &left,
                                     block + sizeof(block) - hdrleft,
                                     hdrleft) != 0) {
                return -1;
            }

            break;
        }

        LOG(log_error, logtype_dsi, "dsi_stream_read_file: %s",
            strerror(errno));
        return -1;
    }

    if (left > 0) {
        LOG(log_error, logtype_dsi,
            "dsi_stream_read_file: the file ends %zu bytes before the reply",
            left);
        errno = EIO;
        return -1;
    }

    dsi->in_write--;
    return 0;
}
#endif


/*!
 * @brief Read data from DSI buffer
 *
 * Essentially a loop around buf_read() to ensure "length" bytes are read
 * from dsi->buffer and/or the socket.
 *
 * @returns length on success, some value smaller than length indicates an error
 */
size_t dsi_stream_read(DSI *dsi, void *data, const size_t length)
{
    size_t stored;
    ssize_t len;

    if (dsi->flags & DSI_DISCONNECTED) {
        return 0;
    }

    LOG(log_maxdebug, logtype_dsi, "dsi_stream_read(%u bytes)", length);
    stored = 0;

    while (stored < length) {
        len = buf_read(dsi, (uint8_t *) data + stored, length - stored);

        if (len == -1 && (errno == EINTR || errno == EAGAIN)) {
            LOG(log_maxdebug, logtype_dsi, "dsi_stream_read: select read loop");
            continue;
        } else if (len > 0) {
            stored += len;
        } else { /* eof or error */
            /* don't log EOF error if it's just after connect (OSX 10.3 probe) */
            if (len || stored || dsi->read_count) {
                if (!(dsi->flags & DSI_DISCONNECTED || dsi->flags & DSI_AFP_LOGGED_OUT)) {
                    LOG(log_error, logtype_dsi, "dsi_stream_read: len:%d, %s",
                        len, (len < 0) ? strerror(errno) : "unexpected EOF");
                }

                return 0;
            }

            break;
        }
    }

    dsi->read_count += stored;
    LOG(log_maxdebug, logtype_dsi, "dsi_stream_read(%u bytes): got: %u", length,
        stored);
    return stored;
}

/*!
 * @brief write data.
 * @returns 0 on failure.
 * @note this assumes that dsi_len will never
 * cause an overflow in the data buffer.
 */
int dsi_stream_send(DSI *dsi, void *buf, size_t length)
{
    char block[DSI_BLOCKSIZ];
    struct iovec iov[2];
    int iovecs = 2;
    size_t towrite;
    ssize_t len;
    LOG(log_maxdebug, logtype_dsi, "dsi_stream_send(%u bytes): START", length);

    if (dsi->flags & DSI_DISCONNECTED) {
        return 0;
    }

    dsi_header_pack_reply(dsi, block);

    if (!length) { /* just write the header */
        LOG(log_maxdebug, logtype_dsi, "dsi_stream_send(%u bytes): DSI header, no data",
            sizeof(block));
        length = (dsi_stream_write(dsi, block, sizeof(block), 0) == sizeof(block));
        return length; /* really 0 on failure, 1 on success */
    }

    /* block signals */
    block_sig(dsi);
    iov[0].iov_base = block;
    iov[0].iov_len = sizeof(block);
    iov[1].iov_base = buf;
    iov[1].iov_len = length;
    towrite = sizeof(block) + length;
    dsi->write_count += towrite;

    while (towrite > 0) {
        if (((len = writev(dsi->socket, iov, iovecs)) == -1 && errno == EINTR)
                || (len == 0)) {
            continue;
        }

        if ((size_t)len == towrite) { /* wrote everything out */
            break;
        } else if (len < 0) { /* error */
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (dsi_peek(dsi) == 0) {
                    continue;
                }
            }

            LOG(log_error, logtype_dsi, "dsi_stream_send: %s", strerror(errno));
            unblock_sig(dsi);
            return 0;
        }

        towrite -= len;

        if (towrite > length) { /* skip part of header */
            iov[0].iov_base = (char *) iov[0].iov_base + len;
            iov[0].iov_len -= len;
        } else { /* skip to data */
            if (iovecs == 2) {
                iovecs = 1;
                len -= iov[0].iov_len;
                iov[0] = iov[1];
            }

            iov[0].iov_base = (char *) iov[0].iov_base + len;
            iov[0].iov_len -= len;
        }
    }

    LOG(log_maxdebug, logtype_dsi, "dsi_stream_send(%u bytes): END", length);
    unblock_sig(dsi);
    return 1;
}


/*!
 * @brief Read DSI command and data
 *
 * @param  dsi   (rw) DSI handle
 *
 * @returns    DSI function on success, 0 on failure
 */
int dsi_stream_receive(DSI *dsi)
{
    char block[DSI_BLOCKSIZ];
    uint32_t dsi_len;
    uint32_t dsi_doff;
    LOG(log_maxdebug, logtype_dsi, "dsi_stream_receive: START");

    if (dsi->flags & DSI_DISCONNECTED) {
        return 0;
    }

    /* read in the header */
    if (dsi_buffered_stream_read(dsi, (uint8_t *)block,
                                 sizeof(block)) != sizeof(block)) {
        return 0;
    }

    dsi->header.dsi_flags = block[0];
    dsi->header.dsi_command = block[1];

    if (dsi->header.dsi_command == 0) {
        return 0;
    }

    memcpy(&dsi->header.dsi_requestID, block + 2,
           sizeof(dsi->header.dsi_requestID));
    memcpy(&dsi->header.dsi_data.dsi_doff, block + 4,
           sizeof(dsi->header.dsi_data.dsi_doff));
    dsi->header.dsi_data.dsi_doff = htonl(dsi->header.dsi_data.dsi_doff);
    memcpy(&dsi->header.dsi_len, block + 8, sizeof(dsi->header.dsi_len));
    memcpy(&dsi->header.dsi_reserved, block + 12, sizeof(dsi->header.dsi_reserved));
    dsi->clientID = ntohs(dsi->header.dsi_requestID);
    dsi_len = ntohl(dsi->header.dsi_len);
    dsi_doff = dsi->header.dsi_data.dsi_doff;

    if (dsi->header.dsi_command == DSIFUNC_WRITE) {
        /* Work around bug in ASC 3.7.x when client sends a zero byte AFPWrite() */
        if (!dsi_doff) {
            dsi_doff = DSI_WROFF_FPWRITE;
            dsi->header.dsi_data.dsi_doff = dsi_doff;
        }

        if (dsi_doff != DSI_WROFF_FPWRITE && dsi_doff != DSI_WROFF_FPWRITEEXT) {
            LOG(log_error, logtype_dsi,
                "dsi_stream_receive: invalid write data offset: %u",
                (unsigned int)dsi_doff);
            return 0;
        }

        if (dsi_doff > dsi_len) {
            LOG(log_error, logtype_dsi,
                "dsi_stream_receive: write data offset exceeds payload length: %u > %u",
                (unsigned int)dsi_doff, (unsigned int)dsi_len);
            return 0;
        }

        /* The negotiated quantum bounds the write DATA; the AFP request
         * block (dsi_doff bytes) rides on top of it. */
        if (dsi_len - dsi_doff > dsi->server_quantum) {
            LOG(log_error, logtype_dsi,
                "dsi_stream_receive: write data too large: %u > %u",
                (unsigned int)(dsi_len - dsi_doff),
                (unsigned int)dsi->server_quantum);
            return 0;
        }
    } else if (dsi_len > dsi->server_quantum) {
        LOG(log_error, logtype_dsi,
            "dsi_stream_receive: request payload too large: %u > %u",
            (unsigned int)dsi_len, (unsigned int)dsi->server_quantum);
        return 0;
    }

    /* Every check above is an upper bound. A command frame also needs a lower
     * one: the dispatcher reads commands[0] to pick the AFP call, so a
     * zero-length payload would select it from whatever the previous request
     * left in the buffer and then run it with an input length of 0. */
    if ((dsi->header.dsi_command == DSIFUNC_CMD
            || dsi->header.dsi_command == DSIFUNC_WRITE)
            && dsi_len < 1) {
        LOG(log_error, logtype_dsi,
            "dsi_stream_receive: command frame carries no AFP function byte");
        return 0;
    }

    dsi->cmdlen = dsi_len;

    /* Receiving DSIWrite data is done in AFP function, not here */
    if (dsi->header.dsi_command == DSIFUNC_WRITE
            && dsi_doff) {
        LOG(log_maxdebug, logtype_dsi, "dsi_stream_receive: write request");
        dsi->cmdlen = dsi_doff;
    }

    if (dsi_stream_read(dsi, dsi->commands, dsi->cmdlen) != dsi->cmdlen) {
        return 0;
    }

    LOG(log_maxdebug, logtype_dsi, "dsi_stream_receive: DSI cmdlen: %zd",
        dsi->cmdlen);
    return block[1];
}
