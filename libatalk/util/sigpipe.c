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

/*
 * Self-pipe for waking a poll() loop from a signal handler.
 *
 * A handler records what happened in a volatile sig_atomic_t and calls
 * atalk_sigpipe_notify(); the loop keeps the read end in its poll() set, so the
 * wake-up does not depend on whether the platform restarts poll() for a handler
 * installed with SA_RESTART. POSIX says poll() is not affected by SA_RESTART,
 * but some platforms have historically restarted it.
 *
 * This only works for a loop whose only blocking point is poll(). A loop that
 * blocks in a read the pipe is not part of will not be woken.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <atalk/util.h>

static int sigpipe_fd[2] = { -1, -1 };

int atalk_sigpipe_init(void)
{
    if (sigpipe_fd[0] >= 0) {
        return 0;                       /* already set up for this process */
    }

    if (pipe(sigpipe_fd) == -1) {
        sigpipe_fd[0] = sigpipe_fd[1] = -1;
        return -1;
    }

    /* Write end non-blocking so a handler never stalls, read end so the drain
     * never blocks. A full pipe means a wake-up is already pending, which is
     * all the loop needs to know. Leaving a half-configured pipe in place would
     * be worse than none: a blocking write end stalls inside a handler, and a
     * blocking read end makes the drain wait for a signal that never comes. */
    for (int i = 0; i < 2; i++) {
        if (setnonblock(sigpipe_fd[i], 1) != 0
                || fcntl(sigpipe_fd[i], F_SETFD, FD_CLOEXEC) == -1) {
            int saved_errno = errno;
            close(sigpipe_fd[0]);
            close(sigpipe_fd[1]);
            sigpipe_fd[0] = sigpipe_fd[1] = -1;
            errno = saved_errno;
            return -1;
        }
    }

    return 0;
}

int atalk_sigpipe_readfd(void)
{
    return sigpipe_fd[0];
}

/* async-signal-safe: write() only, errno preserved */
void atalk_sigpipe_notify(void)
{
    int saved_errno = errno;
    char c = 1;
    (void) write(sigpipe_fd[1], &c, 1);
    errno = saved_errno;
}

void atalk_sigpipe_drain(void)
{
    char buf[64];

    while (read(sigpipe_fd[0], buf, sizeof(buf)) > 0) {
        /* drain */
    }
}
