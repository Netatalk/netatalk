/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <atalk/compat.h>
#include <atalk/util.h>

static struct itimerval itimer;

/*!
 * @brief Check the pid file of a daemon, then fork it into the background and
 * record the pid of the child
 *
 * A pid file that names a live process stops the start, and a pid that a
 * new process reused does too. The child closes stdin, stdout, and stderr,
 * opens /dev/null in their place, and leaves its controlling terminal. With
 * debug the call skips all of this. On Solaris the call returns 0 at once.
 *
 * @param[in] program  daemon name for the error messages
 * @param[in] pidfile  path of the pid file
 * @param[in] debug    nonzero to stay in the foreground without a pid file
 * @returns the pid of the child in the parent, 0 in the child or with debug, or
 *          -1 on error
 */
pid_t server_lock(char *program, char *pidfile, int debug)
{
#ifndef SOLARIS
    char buf[10];
    FILE *pf;
    pid_t pid;
    int mask;

    if (!debug) {
        /* PID files contain only a public process ID; only the owner may write. */
        mask = umask(022); //NOSONAR

        /* check for pid. this can get fooled by stale pid's. */
        if ((pf = fopen(pidfile, "r"))) {
            if (fgets(buf, sizeof(buf), pf) && !kill(pid = atol(buf), 0)) {
                fprintf(stderr,
                        "%s is already running (pid = %d), or the lock file is stale.\n",
                        program, pid);
                fclose(pf);
                umask(mask);
                return -1;
            }

            fclose(pf);
        }

        if ((pf = fopen(pidfile, "w")) == NULL) {
            umask(mask);
            fprintf(stderr, "%s: can't open lock file, \"%s\"\n", program,
                    pidfile);
            return -1;
        }

        umask(mask);
        /*
         * Disassociate from controlling tty.
         */
        int		i _U_;
        getitimer(ITIMER_PROF, &itimer);

        switch (pid = fork()) {
        case 0 :
            setitimer(ITIMER_PROF, &itimer, NULL);
            fclose(stdin);
            fclose(stdout);
            fclose(stderr);
            i = open("/dev/null", O_RDWR);
            i = open("/dev/null", O_RDWR);
            i = open("/dev/null", O_RDWR);
#ifdef TIOCNOTTY

            if ((i = open("/dev/tty", O_RDWR)) >= 0) {
                (void)ioctl(i, TIOCNOTTY, 0);
                setpgid(0, getpid());
                (void) close(i);
            }

#else
            setpgid(0, getpid());
#endif
            break;

        case -1 :  /* error */
            perror("fork");

        default :  /* server */
            fclose(pf);
            return pid;
        }

        fprintf(pf, "%d\n", getpid());
        fclose(pf);
    }

#endif
    return 0;
}

/*!
 * @brief Refuse to start while the pid file names a live process
 *
 * A reused pid in a stale pid file also counts as live. The message goes to
 * stderr. On Solaris the call always passes.
 *
 * @param[in] program  daemon name for the error message
 * @param[in] pidfile  path of the pid file
 * @returns 0, or -1 when the pid file names a live process
 */
int check_lockfile(const char *program, const char *pidfile)
{
#ifndef SOLARIS
    char buf[10];
    FILE *pf;
    pid_t pid;

    /* check for pid. this can get fooled by stale pid's. */
    if ((pf = fopen(pidfile, "r"))) {
        if (fgets(buf, sizeof(buf), pf) && !kill(pid = atol(buf), 0)) {
            fprintf(stderr,
                    "%s is already running (pid = %d), or the lock file is stale.\n",
                    program, pid);
            fclose(pf);
            return -1;
        }

        fclose(pf);
    }

#endif
    return 0;
}

/*!
 * @brief Check the pid file, then write the pid of the calling process into it
 *
 * A new file gets mode 0644 through a umask of 022.
 *
 * @param[in] program  daemon name for the error messages
 * @param[in] pidfile  path of the pid file
 * @returns 0, or -1 when the pid file names a live process or cannot open
 */
int create_lockfile(const char *program, const char *pidfile)
{
#ifndef SOLARIS
    FILE *pf;
    int mask;

    if (check_lockfile(program, pidfile) != 0) {
        return -1;
    }

    /* Write PID to pidfile */
    /* PID files contain only a public process ID; only the owner may write. */
    mask = umask(022); //NOSONAR

    if ((pf = fopen(pidfile, "w")) == NULL) {
        umask(mask);
        fprintf(stderr, "%s: can't open lock file, \"%s\"\n", program,
                pidfile);
        return -1;
    }

    umask(mask);
    fprintf(pf, "%d\n", getpid());
    fclose(pf);
#endif
    return 0;
}
