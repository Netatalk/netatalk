/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (c) 2007 Jeremy Allison
 * Copyright (c) 2013 Ralph Boehme <sloowfranklin@gmail.com>
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * All rights reserved. See COPYRIGHT.
 *
 * dsi_write_file() derives from Samba's sys_recvfile()
 * (source3/lib/recvfile.c):
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * 7 Oct 1997 added checks for 0 data.
 */

/*!
 * @file
 * @brief DSIWrite payload reception, with the recvfile pipe path
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/* this streams writes */
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <atalk/dsi.h>
#include <atalk/logger.h>
#include <atalk/util.h>

/*!
 * @brief Begin a DSIWrite transfer and hand back any payload the read-ahead
 * buffer already holds
 *
 * datasize becomes the payload length from the request header, and the bytes
 * already buffered leave it again. The buffer reference stays valid only until
 * the next dsi call, so the caller consumes it at once. A NULL bufp discards
 * the buffered payload, for callers that only drain the transfer.
 *
 * @param[in,out] dsi   DSI session with the DSIWrite request header parsed
 * @param[out]    bufp  receives a pointer to the buffered payload, or NULL
 * @returns buffered payload bytes, 0 when none, or when the header length falls
 *          below the data offset
 */
size_t dsi_writeinit(DSI *dsi, char **bufp)
{
    size_t bytes = 0;

    if (bufp) {
        *bufp = NULL;
    }

    if (ntohl(dsi->header.dsi_len) < dsi->header.dsi_data.dsi_doff) {
        return 0;
    }

    dsi->datasize = ntohl(dsi->header.dsi_len) - dsi->header.dsi_data.dsi_doff;

    if (dsi->eof > dsi->start) {
        /* We have data in the buffer */
        bytes = MIN((size_t)(dsi->eof - dsi->start), dsi->datasize);

        if (bufp) {
            *bufp = dsi->start;
        }

        dsi->start += bytes;
        dsi->datasize -= bytes;

        if (dsi->start >= dsi->eof) {
            dsi->start = dsi->eof = dsi->buffer;
        }
    }

    LOG(log_maxdebug, logtype_dsi, "dsi_writeinit: remaining DSI datasize: %jd",
        (intmax_t)dsi->datasize);
    return bytes;
}


/*!
 * @brief Receive the next part of a DSIWrite payload into buf
 *
 * Each call reads min(buflen, datasize) bytes. Callers repeat it until
 * datasize reaches 0.
 *
 * @param[in,out] dsi     DSI session after dsi_writeinit
 * @param[out]    buf     receives the bytes
 * @param[in]     buflen  capacity of buf
 * @returns bytes received, or 0 when nothing remains or the read failed
 */
size_t dsi_write(DSI *dsi, void *buf, const size_t buflen)
{
    size_t length;
    LOG(log_maxdebug, logtype_dsi, "dsi_write: remaining DSI datasize: %jd",
        (intmax_t)dsi->datasize);

    if ((length = MIN(buflen, dsi->datasize)) > 0) {
        if ((length = dsi_stream_read(dsi, buf, length)) > 0) {
            LOG(log_maxdebug, logtype_dsi, "dsi_write: received: %ju", (intmax_t)length);
            dsi->datasize -= length;
            return length;
        }
    }

    return 0;
}

/*!
 * @brief Drain the unread remainder of a DSIWrite payload
 *
 * The call reads the bytes into the reply buffer and drops them. A handler
 * that rejected the write thus leaves the stream at the next request boundary.
 *
 * @param[in,out] dsi  DSI session with a DSIWrite in progress
 */
void dsi_writeflush(DSI *dsi)
{
    size_t length;

    while (dsi->datasize > 0) {
        length = dsi_stream_read(dsi, dsi->data,
                                 MIN(sizeof(dsi->data), dsi->datasize));

        if (length > 0) {
            dsi->datasize -= length;
        } else {
            break;
        }
    }
}

/*!
 * @brief Close the recvfile pipe of the session, the next write opens another
 *
 * @param[in,out] dsi  DSI session
 */
void dsi_close_pipe(DSI *dsi)
{
    if (dsi->splice_pipe[0] != -1) {
        close(dsi->splice_pipe[0]);
        close(dsi->splice_pipe[1]);
        dsi->splice_pipe[0] = dsi->splice_pipe[1] = -1;
    }
}

#ifdef WITH_RECVFILE
/*!
 * @brief Wait up to timeout milliseconds for socket data and report a TCP
 * urgent mark
 *
 * The splice call stops at the urgent mark even before the urgent byte
 * arrives, and POLLPRI waits for that byte. The recv call steps over the mark.
 *
 * @param[in] socket   session socket
 * @param[in] timeout  poll timeout in milliseconds, -1 to wait without limit
 * @returns 1 at a TCP urgent mark, 0 otherwise, or -1 with errno set when poll
 *          failed
 */
static int waitfordata(int socket, int timeout)
{
    struct pollfd pfd = { .fd = socket, .events = POLLIN | POLLPRI };
    int atmark = 0;

    while (poll(&pfd, 1, timeout) == -1) {
        if (errno != EINTR) {
            LOG(log_error, logtype_dsi, "waitfordata: poll: %s",
                strerror(errno));
            return -1;
        }
    }

    /* splice() stops at the mark even before the urgent byte arrives, which
     * POLLPRI waits for; recv() steps over it */
    return ioctl(socket, SIOCATMARK, &atmark) == 0 && atmark;
}

/*!
 * @brief Open the recvfile pipe of the session at the largest size the kernel
 * grants
 *
 * The goal is splice_size, and the session keeps the pipe across writes. When
 * the kernel refuses that size, the request halves until it succeeds or
 * reaches the current size, and splice_size shrinks to the result. A pipe
 * that stays at 2 pages or less marks a spent pipe budget. Such a pipe is
 * slower than recv, so the call closes it. Each warning logs once per process.
 *
 * @param[in,out] dsi  DSI session
 * @returns 0, or -1 when this write takes the userspace path
 */
static int open_pipe(DSI *dsi)
{
    static bool pipe_logged;
    static bool budget_logged;
    static bool size_logged;
    int refused;
    int have;
    int size;
    int fd;

    if (pipe2(dsi->splice_pipe, O_CLOEXEC) == -1) {
        /* out of descriptors: this write takes the userspace path */
        if (!pipe_logged) {
            pipe_logged = true;
            LOG(log_warning, logtype_dsi, "recvfile: pipe2: %s",
                strerror(errno));
        }

        return -1;
    }

    fd = dsi->splice_pipe[1];

    /* splice() takes at most splice_size a round, so a bigger pipe stays */
    if ((have = fcntl(fd, F_GETPIPE_SZ)) >= dsi->splice_size
            || fcntl(fd, F_SETPIPE_SZ, dsi->splice_size) != -1) {
        return 0;
    }

    refused = errno;

    /* over the pipe budget the kernel grants two pages: slower than recv() */
    if (have <= 2 * (int)sysconf(_SC_PAGESIZE)) {
        if (!budget_logged) {
            budget_logged = true;
            LOG(log_warning, logtype_dsi,
                "recvfile: pipe size %d refused (%s), pipe budget spent",
                dsi->splice_size, strerror(refused));
        }

        dsi_close_pipe(dsi);
        return -1;
    }

    for (size = dsi->splice_size / 2; size > have; size /= 2) {
        if (fcntl(fd, F_SETPIPE_SZ, size) != -1) {
            break;
        }
    }

    if (!size_logged) {
        size_logged = true;
        LOG(log_warning, logtype_dsi,
            "recvfile: pipe size %d refused (%s), splice size reduced to %d",
            dsi->splice_size, strerror(refused), MAX(size, have));
    }

    dsi->splice_size = MAX(size, have);
    return 0;
}

/*!
 * @brief Move bytes from the recvfile pipe into a file through the reply buffer
 *
 * It serves files on filesystems that cannot splice. One read takes up to len
 * bytes, limited by the reply buffer, and the loop writes them all with
 * pwrite. The contract matches splice with an output offset.
 *
 * @param[in,out] dsi     DSI session whose pipe and reply buffer to use
 * @param[in]     tofd    destination file
 * @param[in,out] offset  file offset, advanced by the bytes written
 * @param[in]     len     limit for the bytes to move
 * @returns bytes moved, 0 at end of input, or -1 with errno set
 */
static ssize_t pipe_to_file(DSI *dsi, int tofd, loff_t *offset, size_t len)
{
    ssize_t n = read(dsi->splice_pipe[0], dsi->data,
                     MIN(len, sizeof(dsi->data)));

    for (ssize_t done = 0; done < n;) {
        ssize_t w = pwrite(tofd, dsi->data + done, (size_t)(n - done),
                           *offset);

        if (w == -1 && errno == EINTR) {
            continue;
        }

        if (w <= 0) {
            return w;
        }

        done += w;
        *offset += w;
    }

    return n;
}

/*!
 * @brief Receive the rest of a DSIWrite payload straight into a file
 *
 * dsi->datasize falls as the socket is read, so dsi_write() and
 * dsi_writeflush() continue where this stops.
 *
 * @param[in,out] dsi       session; its datasize is the payload left to read
 * @param[in]     tofd      destination file
 * @param[in,out] offset    file offset of the first byte, advanced past every
 *                          byte written, also when the call fails
 * @param[in,out] nosplice  set when tofd's filesystem cannot splice, once the
 *                          round in the pipe is written through dsi->data
 *
 * @returns the bytes written: all of datasize, fewer when the caller is to
 *          finish from the socket, 0 when nothing was read; -1 with errno set
 */
ssize_t dsi_write_file(DSI *dsi, int tofd, off_t *offset, bool *nosplice)
{
    static bool copy_logged;
    static bool socket_logged;
    loff_t splice_offset = *offset;
    size_t written = 0;
    bool copy = false;
    LOG(log_maxdebug, logtype_dsi, "dsi_write_file(off: %jd, len: %u)",
        (intmax_t)*offset, dsi->datasize);

    if (dsi->datasize == 0) {
        return 0;
    }

    if (dsi->splice_pipe[0] == -1 && open_pipe(dsi) == -1) {
        return 0;
    }

    while (dsi->datasize > 0 && !copy) {
        ssize_t nread = splice(dsi->socket, NULL, dsi->splice_pipe[1], NULL,
                               MIN(dsi->datasize, (size_t)dsi->splice_size),
                               SPLICE_F_NONBLOCK);

        if (nread == -1) {
            if (errno == EINTR) {
                continue;
            }

            if (errno == EAGAIN) {
                int ready = waitfordata(dsi->socket, -1);

                if (ready == -1) {
                    return -1;
                }

                if (ready == 1) {
                    break;
                }

                continue;
            }

            /* nothing consumed: the userspace path takes the whole write */
            if (written == 0) {
                /* a refused splice() is logged once; recv() reports the rest */
                if (!socket_logged && (errno == EINVAL || errno == ENOSYS
                                       || errno == EPERM || errno == EBADF)) {
                    socket_logged = true;
                    LOG(log_warning, logtype_dsi,
                        "dsi_write_file: splice() from the socket: %s",
                        strerror(errno));
                }

                return 0;
            }

            return -1;
        }

        if (nread == 0) {
            /* after the peer's FIN splice() reads 0 at an urgent mark too */
            if (waitfordata(dsi->socket, 0) == 1) {
                break;
            }

            errno = ECONNRESET;
            return -1;
        }

        dsi->datasize -= nread;
        dsi->read_count += nread;

        for (size_t to_write = (size_t)nread; to_write > 0;) {
            ssize_t thistime = copy
                               ? pipe_to_file(dsi, tofd, &splice_offset,
                                              to_write)
                               : splice(dsi->splice_pipe[0], NULL, tofd,
                                        &splice_offset, to_write, 0);
            *offset = splice_offset;

            if (thistime > 0) {
                to_write -= thistime;
                written += thistime;
            } else if (thistime == -1 && errno == EINVAL && !copy) {
                copy = true;
            } else if (thistime == 0 || errno != EINTR) {
                int saved_errno = thistime == 0 ? EIO : errno;
                /* the next write must not start with this round's bytes */
                dsi_close_pipe(dsi);
                errno = saved_errno;
                return -1;
            }
        }
    }

    /* the copied round shows the file, not the offset, refuses splice() */
    if (copy) {
        *nosplice = true;

        if (!copy_logged) {
            copy_logged = true;
            LOG(log_warning, logtype_dsi,
                "dsi_write_file: cannot splice into the file, copying");
        }
    }

    LOG(log_maxdebug, logtype_dsi, "dsi_write_file: written: %zu", written);
    return (ssize_t)written;
}
#endif /* WITH_RECVFILE */
