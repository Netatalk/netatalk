/*
  Copyright (c) 2026 Andy Lemin (andylemin)

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
#endif

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "peer_lock.h"

/*!
 * @brief Fork a peer that holds an fcntl lock until released
 *
 * F_SETLK/F_GETLK never conflict with or report the calling process's
 * own locks, so a test that needs a genuinely contending lock forks a
 * peer that takes it and parks.
 *
 * @param[out] p      filled with the peer's pid and control pipes
 * @param[in]  path   file to lock
 * @param[in]  type   F_RDLCK or F_WRLCK
 * @param[in]  start  first byte of the range
 * @param[in]  len    range length (0 = to EOF)
 *
 * @returns 0 with the lock held by the time this returns, -1 on failure
 */
int peer_hold_lock(struct peer *p, const char *path, short type,
                   off_t start, off_t len)
{
    int up[2], down[2];
    char b;

    if (pipe(up) < 0) {
        return -1;
    }

    if (pipe(down) < 0) {
        close(up[0]);
        close(up[1]);
        return -1;
    }

    p->pid = fork();

    if (p->pid < 0) {
        close(up[0]);
        close(up[1]);
        close(down[0]);
        close(down[1]);
        return -1;
    }

    if (p->pid == 0) {
        /* child: take the lock, signal "held", wait for the release byte */
        struct flock fl = {0};
        int fd = open(path, O_RDWR);

        if (fd < 0) {
            _exit(1);
        }

        fl.l_type = type;
        fl.l_whence = SEEK_SET;
        fl.l_start = start;
        fl.l_len = len;

        if (fcntl(fd, F_SETLK, &fl) < 0) {
            _exit(2);
        }

        b = 1;

        if (write(up[1], &b, 1) != 1) {
            _exit(3);
        }

        (void)read(down[0], &b, 1);   /* park until released */
        _exit(0);
    }

    /* parent */
    close(up[1]);
    close(down[0]);
    p->to_child = down[1];
    p->from_child = up[0];

    /* block until the peer reports the lock is held */
    if (read(p->from_child, &b, 1) != 1) {
        int status;
        close(p->to_child);
        close(p->from_child);
        (void)waitpid(p->pid, &status, 0);
        return -1;
    }

    return 0;
}

/*!
 * @brief Release a held peer lock and reap the peer
 *
 * @param[in] p  peer filled by peer_hold_lock()
 */
void peer_release(struct peer *p)
{
    char b = 1;
    int status;
    (void)write(p->to_child, &b, 1);
    close(p->to_child);
    close(p->from_child);
    (void)waitpid(p->pid, &status, 0);
}

/*!
 * @brief Probe a lock range from a forked child
 *
 * Cross-process, so it sees locks this process holds (which its own
 * F_GETLK never would).
 *
 * @param[in] path   file to probe
 * @param[in] type   F_RDLCK or F_WRLCK
 * @param[in] start  first byte of the range
 * @param[in] len    range length (0 = to EOF)
 *
 * @returns 1 if the lock was acquired (range free), 0 if it conflicted
 *          (range held by another process), -1 on a fork/open error
 */
int peer_try_lock(const char *path, short type, off_t start, off_t len)
{
    int pfd[2];
    pid_t pid;
    int status;
    char res;

    if (pipe(pfd) < 0) {
        return -1;
    }

    pid = fork();

    if (pid < 0) {
        close(pfd[0]);
        close(pfd[1]);
        return -1;
    }

    if (pid == 0) {
        struct flock fl = {0};
        int fd = open(path, O_RDWR);
        char r;

        if (fd < 0) {
            r = 2;                   /* could not open */
        } else {
            fl.l_type = type;
            fl.l_whence = SEEK_SET;
            fl.l_start = start;
            fl.l_len = len;
            /* 1 = acquired (range free), 0 = conflict (range held) */
            r = (fcntl(fd, F_SETLK, &fl) == 0) ? 1 : 0;
        }

        (void)write(pfd[1], &r, 1);
        _exit(0);
    }

    close(pfd[1]);

    if (read(pfd[0], &res, 1) != 1) {
        res = 2;                     /* child died without reporting */
    }

    close(pfd[0]);
    (void)waitpid(pid, &status, 0);

    if (res == 2) {
        return -1;                   /* child could not open the file */
    }

    return res == 1 ? 1 : 0;
}
