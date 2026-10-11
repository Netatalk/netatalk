/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * Copyright (C) Jeremy Allison 2007
 * Copyright (c) 2013 Ralph Boehme <sloowfranklin@gmail.com>
 * Copyright (c) 2026 Andy Lemin (andylemin)
 * All rights reserved. See COPYRIGHT.
 *
 * dsi_write_file() derives from Samba's sys_recvfile()
 * (source3/lib/recvfile.c).
 *
 * 7 Oct 1997 added checks for 0 data.
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
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <atalk/dsi.h>
#include <atalk/logger.h>
#include <atalk/util.h>

/*!
 * Begin a DSI write transfer: set datasize and hand back, by reference, any
 * payload already in the read-ahead buffer.  *bufp is valid only until the
 * next dsi_* call; consume it immediately.  A NULL bufp discards the
 * buffered payload, for callers that only drain the transfer.
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


/*! fill up buf and then return. this should be called repeatedly
 * until all the data has been read. i block alarm processing
 * during the transfer to avoid sending unnecessary tickles. */
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

/*! flush any unread buffers. */
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

#ifdef WITH_RECVFILE
/*!
 * @brief Wait up to timeout ms for socket data, or for a TCP urgent byte
 *        splice() stops at
 *
 * @returns 1 when a TCP urgent byte is pending, 0 when not, -1 with errno
 *          set
 */
static int waitfordata(int socket, int timeout)
{
    struct pollfd pfd = { .fd = socket, .events = POLLIN | POLLPRI };

    while (poll(&pfd, 1, timeout) == -1) {
        if (errno != EINTR) {
            LOG(log_error, logtype_dsi, "waitfordata: poll: %s",
                strerror(errno));
            return -1;
        }
    }

    /* recv() steps over the urgent byte that splice() cannot pass */
    return (pfd.revents & POLLPRI) ? 1 : 0;
}

/*! @brief Close the session's recvfile pipe */
static void close_pipe(DSI *dsi)
{
    close(dsi->splice_pipe[0]);
    close(dsi->splice_pipe[1]);
    dsi->splice_pipe[0] = dsi->splice_pipe[1] = -1;
}

/*!
 * @brief Open the session's recvfile pipe, grown to the largest size up to
 *        splice_size that the kernel grants the session
 *
 * @returns 0, or -1 when this write takes the userspace path
 */
static int open_pipe(DSI *dsi, int splice_size)
{
    static bool pipe_logged;
    static bool size_logged;
    int refused;
    int have;
    int size;

    if (pipe2(dsi->splice_pipe, O_CLOEXEC) == -1) {
        /* out of descriptors: this write takes the userspace path */
        if (!pipe_logged) {
            pipe_logged = true;
            LOG(log_warning, logtype_dsi, "recvfile: pipe2: %s", strerror(errno));
        }

        return -1;
    }

    /* splice() takes at most splice_size a round, so a bigger pipe stays */
    if ((have = fcntl(dsi->splice_pipe[1], F_GETPIPE_SZ)) >= splice_size
            || fcntl(dsi->splice_pipe[1], F_SETPIPE_SZ, splice_size) != -1) {
        return 0;
    }

    refused = errno;

    /* over the pipe budget the kernel grants two pages: slower than recv() */
    if (have <= 2 * (int)sysconf(_SC_PAGESIZE)) {
        if (!size_logged) {
            size_logged = true;
            LOG(log_warning, logtype_dsi,
                "recvfile: pipe size %d refused (%s), the pipe budget is spent",
                splice_size, strerror(refused));
        }

        close_pipe(dsi);
        return -1;
    }

    /* fs.pipe-max-size or the budget: the session keeps the size granted */
    for (size = splice_size / 2; size > have; size /= 2) {
        if (fcntl(dsi->splice_pipe[1], F_SETPIPE_SZ, size) != -1) {
            break;
        }
    }

    if (!size_logged) {
        size_logged = true;
        LOG(log_warning, logtype_dsi,
            "recvfile: pipe size %d refused (%s), splice size reduced to %d",
            splice_size, strerror(refused), MAX(size, have));
    }

    return 0;
}

/*!
 * @brief splice() from the pipe into a file through dsi->data, for a file
 *        whose filesystem cannot splice
 *
 * Same contract as splice() with an output offset.
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
 * @param[in,out] dsi          session; its datasize is the payload left to read
 * @param[in]     tofd         destination file
 * @param[in,out] offset       file offset of the first byte, advanced past
 *                             every byte written, also when the call fails
 * @param[in]     splice_size  most bytes per splice() round
 * @param[in,out] nosplice     set when tofd's filesystem cannot splice; the
 *                             round in the pipe is then written through
 *                             dsi->data
 *
 * @returns the bytes written: all of datasize, fewer when the caller is to
 *          finish from the socket, 0 when nothing was read; -1 with errno set
 */
ssize_t dsi_write_file(DSI *dsi, int tofd, off_t *offset, int splice_size,
                       bool *nosplice)
{
    static bool copy_logged;
    static bool socket_logged;
    loff_t splice_offset = *offset;
    size_t written = 0;
    LOG(log_maxdebug, logtype_dsi, "dsi_write_file(off: %jd, len: %u)",
        (intmax_t)*offset, dsi->datasize);

    if (dsi->datasize == 0) {
        return 0;
    }

    if (dsi->splice_pipe[0] == -1 && open_pipe(dsi, splice_size) == -1) {
        return 0;
    }

    while (dsi->datasize > 0) {
        ssize_t nread = splice(dsi->socket, NULL, dsi->splice_pipe[1], NULL,
                               MIN((size_t)dsi->datasize, (size_t)splice_size),
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
                /* a refused splice is logged once; recv() reports socket errors */
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
            ssize_t thistime = *nosplice
                               ? pipe_to_file(dsi, tofd, &splice_offset,
                                              to_write)
                               : splice(dsi->splice_pipe[0], NULL, tofd,
                                        &splice_offset, to_write, 0);
            *offset = splice_offset;

            if (thistime > 0) {
                to_write -= thistime;
                written += thistime;
            } else if (thistime == -1 && errno == EINVAL && !*nosplice) {
                *nosplice = true;

                if (!copy_logged) {
                    copy_logged = true;
                    LOG(log_warning, logtype_dsi,
                        "dsi_write_file: cannot splice into the file, copying");
                }
            } else if (thistime == 0 || errno != EINTR) {
                int saved_errno = thistime == 0 ? EIO : errno;
                /* the next write must not start with this round's bytes */
                close_pipe(dsi);
                errno = saved_errno;
                return -1;
            }
        }

        if (*nosplice) {
            break;
        }
    }

    LOG(log_maxdebug, logtype_dsi, "dsi_write_file: written: %zu", written);
    return (ssize_t)written;
}
#endif /* WITH_RECVFILE */
