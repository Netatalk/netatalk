/*
 * Copyright (C) Jeremy Allison 2007
 * Copyright (c) 2013 Ralph Boehme <sloowfranklin@gmail.com>
 * All rights reserved. See COPYRIGHT.
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef WITH_RECVFILE

#include <stdio.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>

#include <atalk/adouble.h>
#include <atalk/logger.h>
#include <atalk/util.h>

static int ad_recvfile_init(const struct adouble *ad, int eid, off_t *off)
{
    int fd;

    if (eid == ADEID_DFORK) {
        fd = ad_data_fileno(ad);
    } else {
        *off += ad_getentryoff(ad, eid);
        fd = ad_reso_fileno(ad);
    }

    return fd;
}

/*
 * If tofd is -1, drain the incoming socket of count bytes without writing to the outgoing fd,
 * if a write fails we do the same.
 *
 * Returns -1 on short reads from fromfd (read error) and sets errno.
 *
 * Returns number of bytes written to 'tofd'  or thrown away if 'tofd == -1'.
 * return != count then sets errno.
 * Returns count if complete success.
 */

#define TRANSFER_BUF_SIZE (128*1024)

static ssize_t default_sys_recvfile(int fromfd,
                                    int tofd,
                                    off_t offset,
                                    size_t count)
{
    int saved_errno = 0;
    size_t total = 0;
    size_t bufsize = MIN(TRANSFER_BUF_SIZE, count);
    size_t total_written = 0;
    char *buffer = NULL;

    if (count == 0) {
        return 0;
    }

    LOG(log_maxdebug, logtype_dsi, "default_recvfile: from = %d, to = %d, offset = %.0f, count = %lu\n",
        fromfd, tofd, (double)offset, (unsigned long)count);

    if ((buffer = malloc(bufsize)) == NULL)
        return -1;

    while (total < count) {
        size_t num_written = 0;
        ssize_t read_ret;
        size_t toread = MIN(bufsize,count - total);

        /* Read from socket - ignore EINTR. */
        read_ret = read(fromfd, buffer, toread);
        if (read_ret <= 0) {
            /* EOF or socket error. */
            free(buffer);
            return -1;
        }

        num_written = 0;

        while (num_written < read_ret) {
            ssize_t write_ret;

            if (tofd == -1) {
                write_ret = read_ret;
            } else {
                /* Write to file - ignore EINTR. */
                write_ret = pwrite(tofd, buffer + num_written, read_ret - num_written, offset);
                if (write_ret <= 0) {
                    /* write error - stop writing. */
                    tofd = -1;
                    saved_errno = errno;
                    continue;
                }
            }
            num_written += (size_t)write_ret;
            total_written += (size_t)write_ret;
        }
        total += read_ret;
    }

    free(buffer);
    if (saved_errno) {
        /* Return the correct write error. */
        errno = saved_errno;
    }
    return (ssize_t)total_written;
}

#ifdef HAVE_SPLICE
static int waitfordata(int socket)
{
    fd_set readfds;
    int maxfd = socket + 1;
    int ret;

    FD_ZERO(&readfds);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(socket, &readfds);
        if ((ret = select(maxfd, &readfds, NULL, NULL, NULL)) <= 0) {
            if (ret == -1 && errno == EINTR)
                continue;
            LOG(log_error, logtype_dsi, "waitfordata: unexpected select return: %d %s",
                ret, ret < 0 ? strerror(errno) : "");
            return -1;
        }
        if (FD_ISSET(socket, &readfds))
            return 0;
        return -1;
    }

}

/*
 * Try and use the Linux system call to do this.
 * Remember we only return -1 if the socket read
 * failed. Else we return the number of bytes
 * actually written. We always read count bytes
 * from the network in the case of return != -1.
 */
static ssize_t sys_recvfile(int fromfd, int tofd, off_t offset, size_t count, int splice_size)
{
    static int pipefd[2] = { -1, -1 };
    static bool try_splice_call = true;
    size_t total_written = 0;
    loff_t splice_offset = offset;

    LOG(log_debug, logtype_dsi, "sys_recvfile: from = %d, to = %d, offset = %.0f, count = %lu",
        fromfd, tofd, (double)offset, (unsigned long)count);

    if (count == 0)
        return 0;

    /*
     * Older Linux kernels have splice for sendfile,
     * but it fails for recvfile. Ensure we only try
     * this once and always fall back to the userspace
     * implementation if recvfile splice fails. JRA.
     */

    if (!try_splice_call) {
        errno = ENOSYS;
        return -1;
    }

    if ((pipefd[0] == -1) && (pipe(pipefd) == -1)) {
        try_splice_call = false;
        errno = ENOSYS;
        return -1;
    }

    while (count > 0) {
        int nread, to_write;

        nread = splice(fromfd, NULL, pipefd[1], NULL, MIN(count, splice_size), SPLICE_F_MOVE | SPLICE_F_NONBLOCK);

        if (nread == -1) {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN) {
                if (waitfordata(fromfd) != -1)
                    continue;
                return -1;
            }
            if (total_written == 0 && (errno == EBADF || errno == EINVAL)) {
                LOG(log_warning, logtype_dsi, "splice() doesn't work for recvfile");
                try_splice_call = false;
                errno = ENOSYS;
                return -1;
            }
            break;
        }

        to_write = nread;
        while (to_write > 0) {
            int thistime;
            thistime = splice(pipefd[0], NULL, tofd, &splice_offset, to_write, SPLICE_F_MOVE);
            if (thistime == -1)
                return -1;
            to_write -= thistime;
        }

        total_written += nread;
        count -= nread;
    }

done:
    LOG(log_maxdebug, logtype_dsi, "sys_recvfile: total_written: %zu", total_written);

    return total_written;
}
#else

/*****************************************************************
 No recvfile system call - use the default 128 chunk implementation.
*****************************************************************/

ssize_t sys_recvfile(int fromfd, int tofd, off_t offset, size_t count)
{
    return default_sys_recvfile(fromfd, tofd, offset, count);
}
#endif

/* read from a socket and write to an adouble file */
ssize_t ad_recvfile(struct adouble *ad, int eid, int sock, off_t off, size_t len, int splice_size)
{
    ssize_t cc;
    int fd;
    off_t off_fork = off;

    fd = ad_recvfile_init(ad, eid, &off_fork);
    if ((cc = sys_recvfile(sock, fd, off_fork, len, splice_size)) != len)
        return -1;

    if ((eid != ADEID_DFORK) && (off > ad_getentrylen(ad, eid)))
        ad_setentrylen(ad, eid, off);

    return cc;
}
#endif
