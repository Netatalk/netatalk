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

/*!
 * @file
 * @brief DSI stream input and output on the session socket
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#if defined(SENDFILE_FLAVOR_LINUX) || defined(SENDFILE_FLAVOR_SOLARIS)
#include <sys/sendfile.h>
#endif

#include <atalk/dsi.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/util.h>

/*!
 * @brief Write the session DSI header into buf in wire format
 *
 * The fields already hold network byte order, so the call only lays them out.
 *
 * @param[in]  dsi  DSI session whose header to write
 * @param[out] buf  receives DSI_BLOCKSIZ bytes
 */
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
 * @brief Wait until the socket accepts writes, reading pending input meanwhile
 *
 * A blocked send can mean the client itself blocks while it sends to afpd.
 * The call therefore selects for both directions and reads into the
 * read-ahead buffer. Reading may unblock the client. A full read-ahead buffer
 * logs a note once and leaves only the write test. End of file, a select
 * failure, a read error, or a socket at FD_SETSIZE or above ends the wait.
 *
 * @param[in,out] dsi  DSI session
 * @returns 0 when the socket accepts writes again, -1 on failure or a closed
 *          socket
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
 * @brief Take up to count bytes from the read-ahead buffer
 *
 * An emptied buffer rewinds to its start. The master afpd has no read-ahead
 * buffer, so it gets 0.
 *
 * @param[in,out] dsi    DSI session
 * @param[out]    buf    receives the bytes
 * @param[in]     count  capacity of buf
 * @returns bytes taken, 0 when the buffer is empty
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
 * @brief Read bytes from the read-ahead buffer, or from the socket when it is
 * empty
 *
 * A non-empty buffer returns only the bytes it holds, up to count. Only an
 * empty buffer leads to a socket read, which waits for input through
 * dsi_wait_input().
 *
 * @param[in,out] dsi    DSI session
 * @param[out]    buf    receives the bytes
 * @param[in]     count  capacity of buf
 * @returns bytes read, 0 for a zero count or end of file, or -1 with errno set
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

    while ((len = recv(dsi->socket, buf, count, 0)) == -1 /* 3. */
            && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        if (dsi_wait_input(dsi) != 0) {
            return -1;
        }
    }

    LOG(log_maxdebug, logtype_dsi, "buf_read(%u bytes): got: %d", count, len);
    return len;
}

/*!
 * @brief Read length bytes and refill the read-ahead buffer in 8192-byte chunks
 *
 * The call first drains the read-ahead buffer. When that leaves a gap, one
 * recv tops the buffer up by at most 8192 bytes, so later small requests come
 * from memory. The dsi_stream_read call then supplies the rest.
 *
 * @param[in,out] dsi     DSI session
 * @param[out]    data    receives the bytes
 * @param[in]     length  bytes to read
 * @returns length, or 0 when the read failed
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

/*!
 * @brief Mark the start of a multi-packet write, which no tickle, attention
 * or CloseSession frame may interleave
 *
 * @param[in,out] dsi  DSI session
 */
static void block_sig(DSI *dsi)
{
    dsi->in_write++;
}

/*!
 * @brief Mark the end of a multi-packet write that block_sig started
 *
 * @param[in,out] dsi  DSI session
 */
static void unblock_sig(DSI *dsi)
{
    dsi->in_write--;
}

/*!
 * @brief Read the monotonic clock
 *
 * @returns milliseconds since an arbitrary start
 */
static long long monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/*********************************************************************************
 * Public functions
 *********************************************************************************/

/*!
 * @brief Enter the disconnected state after a communication error with the
 * client
 *
 * The call closes the transport and the recvfile pipe, clears the sleep flags,
 * and sets DSI_DISCONNECTED. A session that never logged in reports failure.
 * login refuses uid 0 and stores the uid on success, so a zero uid means no
 * login. The UAM logout hook is no usable mark, because a UAM may register
 * none. A call on a session already disconnected logs nothing.
 *
 * @param[in,out] dsi  DSI session
 * @returns 0, or -1 when no user logged in to the session
 */
int dsi_disconnect(DSI *dsi)
{
    if (!(dsi->flags & DSI_DISCONNECTED)) {
        LOG(log_note, logtype_dsi, "dsi_disconnect: entering disconnected state");
    }

    dsi->proto_close(dsi);          /* 1 */
    dsi_close_pipe(dsi);
    dsi->flags &= ~(DSI_SLEEPING | DSI_EXTSLEEP); /* 2 */
    dsi->flags |= DSI_DISCONNECTED;

    if (dsi->AFPobj == NULL || dsi->AFPobj->uid == 0) {
        return -1;
    }

    return 0;
}

/*!
 * @brief Wait until the client sends more of the request being read
 *
 * The tickle timer that disconnects a client silent for the timeout option's
 * count of tickleval intervals runs in the main loop, which a request being
 * read never reaches, so the wait applies the same bound from the last bytes
 * received; past it the session enters the disconnected state, from which the
 * client can reconnect. The flag stop_wait points to, set for a reconnect's
 * hand-over, ends the wait at once the same way, as the client has left the
 * connection. Other signals leave the wait waiting, so a termination lets a
 * request whose client still sends finish. A session without its options
 * waits without a bound.
 *
 * @param[in,out] dsi  DSI session
 * @returns 0 once the socket has input; -1 with errno ETIMEDOUT at the bound or
 *          ECONNABORTED for a hand-over, both after the session disconnected,
 *          or with the errno of poll
 */
int dsi_wait_input(DSI *dsi)
{
    struct pollfd pfd[2] = {
        { .fd = dsi->socket, .events = POLLIN },
        { .fd = atalk_sigpipe_readfd(), .events = POLLIN },
    };
    long long limit = -1;
    long long end = 0;

    if (dsi->AFPobj != NULL) {
        limit = MIN((long long)dsi->AFPobj->options.timeout
                    * dsi->AFPobj->options.tickleval, INT_MAX / 1000) * 1000;
        end = monotonic_ms() + limit;
    }

    while (dsi->stop_wait == NULL || !*dsi->stop_wait) {
        long long now = monotonic_ms();
        int ret = poll(pfd, pfd[1].fd == -1 ? 1 : 2,
                       limit < 0 ? -1 : (int)(end > now ? end - now : 0));

        if (ret > 0 && pfd[0].revents != 0) {
            return 0;
        }

        if (ret > 0) {
            /* a signal's wake-up; a pipe that fails is watched no more */
            if (pfd[1].revents & POLLIN) {
                atalk_sigpipe_drain();
            } else {
                pfd[1].fd = -1;
            }
        } else if (ret == 0) {
            LOG(log_error, logtype_dsi,
                "dsi_wait_input: the client sent nothing for %lld s of a request",
                limit / 1000);
            dsi_disconnect(dsi);
            errno = ETIMEDOUT;
            return -1;
        } else if (errno != EINTR) {
            LOG(log_error, logtype_dsi, "dsi_wait_input: poll: %s",
                strerror(errno));
            return -1;
        }
    }

    LOG(log_note, logtype_dsi, "dsi_wait_input: a reconnect ended the request");
    dsi_disconnect(dsi);
    errno = ECONNABORTED;
    return -1;
}

/*!
 * @brief Send length raw bytes to the client socket
 *
 * The loop retries after EINTR. On EAGAIN it reads pending input through
 * dsi_peek and then writes again. That keeps a client that itself blocks on
 * sending from deadlocking the session. DSI_NOWAIT gives up at once when
 * nothing went out yet, so an attention or a tickle never waits on a full
 * socket. in_write rises for the duration of the call.
 *
 * @param[in,out] dsi     DSI session
 * @param[in]     data    bytes to send
 * @param[in]     length  bytes to send
 * @param[in]     mode    0 or DSI_NOWAIT
 * @returns bytes sent, or -1 for a disconnected session or a failed send
 */
ssize_t dsi_stream_write(DSI *dsi, void *data, const size_t length, int mode)
{
    size_t written;
    ssize_t len;
    dsi->in_write++;
    written = 0;
    LOG(log_maxdebug, logtype_dsi, "dsi_stream_write(send: %zd bytes): START",
        length);

    if (dsi->flags & DSI_DISCONNECTED) {
        return -1;
    }

    while (written < length) {
        len = send(dsi->socket, (uint8_t *) data + written, length - written,
                   0);

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
 * @brief Make one sendfile call: the unsent header bytes, then file bytes
 *
 * Linux sendfile takes no header, so the header goes out in a send of its
 * own, held back by MSG_MORE until the file data follows. sent counts header
 * bytes first, and is set after EAGAIN and EINTR too.
 *
 * @param[in]  dsi     DSI session whose socket to send on
 * @param[in]  fromfd  file to send from
 * @param[in]  pos     file offset of the first file byte
 * @param[in]  count   file bytes to send at most
 * @param[in]  hdr     unsent header bytes
 * @param[in]  hdrlen  bytes at hdr, 0 once the header is out
 * @param[out] sent    bytes the call put on the socket
 * @returns 0, or -1 with errno set
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
 * @brief Send the rest of a read reply through the request buffer
 *
 * The unsent header bytes go first, then the file bytes from pos a session
 * quantum at a time, until left reaches 0 or the file ends.
 *
 * @param[in,out] dsi     DSI session whose socket and request buffer to use
 * @param[in]     fromfd  file to send from
 * @param[in]     pos     file offset of the next file byte
 * @param[in,out] left    file bytes the reply still owes, reduced by those sent
 * @param[in]     hdr     unsent header bytes
 * @param[in]     hdrlen  bytes at hdr
 * @returns 0, or -1 with errno set, which stays EAGAIN when the client ends
 *          the stream during a wait
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
 * @brief Send a streamed FPRead reply whose payload comes from a file through
 * sendfile
 *
 * The reply header carries length and the result code err. Each
 * sendfile_call sends header bytes before file bytes; the loop retries after
 * EINTR and EAGAIN, reading pending input through dsi_peek, and stops at a
 * call that sends nothing or an EINVAL at the end of the file. A file the
 * kernel will not send from (EINVAL, ENOSYS, EOPNOTSUPP) goes through the
 * request buffer instead. DSI_NOREPLY marks the reply as sent. A failure
 * leaves in_write raised, as dsi_readinit does, so no tickle, attention or
 * CloseSession frame follows the broken reply.
 *
 * @param[in,out] dsi     DSI session
 * @param[in]     fromfd  file to send from
 * @param[in]     offset  file offset to start at
 * @param[in]     length  payload bytes to send, at least one and at most
 *                        UINT32_MAX, the DSI header's 32-bit length
 * @param[in]     err     AFP result code
 * @returns 0, or -1 with errno set: ENOTCONN for a disconnected session, EIO
 *          when the file ends first, or the error of the failed call, which
 *          stays EAGAIN or EINTR when the client ends the stream during a wait
 */
int dsi_stream_read_file(DSI *dsi, const int fromfd, off_t offset,
                         const size_t length, const int err)
{
    static unsigned int copied_replies;
    char block[DSI_BLOCKSIZ];
    size_t hdrleft = sizeof(block);
    size_t left = length;
    LOG(log_maxdebug, logtype_dsi, "dsi_stream_read_file(off: %jd, len: %zu)",
        (intmax_t)offset, length);
    dsi->in_write++;

    if (dsi->flags & DSI_DISCONNECTED) {
        errno = ENOTCONN;
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
        LOG(log_maxdebug, logtype_dsi, "dsi_stream_read_file: sent: %zu",
            sent);

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

        if (errno == EINVAL) {
            struct stat st;

            /* illumos fails a call that meets the end of the file with
             * EINVAL as well */
            if (fstat(fromfd, &st) == 0 && st.st_size <= offset) {
                break;
            }

            errno = EINVAL;
        }

        if (errno == EINVAL || errno == ENOSYS || errno == EOPNOTSUPP) {
            /* the kernel will not sendfile from this file */
            if (log_backoff(&copied_replies)) {
                LOG(log_warning, logtype_dsi,
                    "dsi_stream_read_file: cannot sendfile from the file "
                    "(%s), copying (reply %u)", strerror(errno),
                    copied_replies);
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
 * @brief Read length bytes from the read-ahead buffer and the socket
 *
 * buf_read supplies the bytes, and the loop continues after EINTR and EAGAIN
 * until length bytes arrived. End of file or a read error ends the call with
 * 0. A disconnected session also yields 0.
 *
 * @param[in,out] dsi     DSI session
 * @param[out]    data    receives the bytes
 * @param[in]     length  bytes to read
 * @returns length, or 0 on end of file or a read error
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
 * @brief Send the reply header and length payload bytes as one DSI message
 *
 * The header comes from the session header fields. A zero length sends the
 * header alone. Otherwise writev sends header and payload together, retries
 * after EINTR, and reads pending input through dsi_peek on EAGAIN. block_sig
 * holds attentions off for the duration.
 *
 * @param[in,out] dsi     DSI session with the header fields set
 * @param[in]     buf     payload bytes
 * @param[in]     length  payload bytes to send, matching dsi_len in the header
 * @returns 1, or 0 for a disconnected session or a failed send
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
