/*
  Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

/*!
 * @file
 * Netatalk utility functions
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>

#include <atalk/adouble.h>
#include <atalk/ea.h>
#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/vfs.h>
#include <atalk/util.h>
#include <atalk/unix.h>
#include <atalk/compat.h>
#include <atalk/errchk.h>

/* close all FDs >= a specified value */
static void closeall(int fd)
{
    int fdlimit = sysconf(_SC_OPEN_MAX);

    while (fd < fdlimit)
        close(fd++);
}

/*!
 * Run command in a child and wait for it to finish
 */
int run_cmd(const char *cmd, char **cmd_argv)
{
    EC_INIT;
    pid_t pid, wpid;
    sigset_t sigs, oldsigs;
	int status = 0;

    sigfillset(&sigs);
    pthread_sigmask(SIG_SETMASK, &sigs, &oldsigs);

    if ((pid = fork()) < 0) {
        LOG(log_error, logtype_default, "run_cmd: fork: %s", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        /* child */
        closeall(3);
        execvp("mv", cmd_argv);
    }

    /* parent */
	while ((wpid = waitpid(pid, &status, 0)) < 0) {
	    if (errno == EINTR)
            continue;
	    break;
	}
	if (wpid != pid) {
	    LOG(log_error, logtype_default, "waitpid(%d): %s", (int)pid, strerror(errno));
        EC_FAIL;
	}

    if (WIFEXITED(status))
        status = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        status = WTERMSIG(status);

    LOG(log_note, logtype_default, "run_cmd(\"%s\"): status: %d", cmd, status);

EC_CLEANUP:
    if (status != 0)
        ret = status;
    pthread_sigmask(SIG_SETMASK, &oldsigs, NULL);
    EC_EXIT;
}

/*!
 * Daemonize
 *
 * Fork, exit parent, setsid(), optionally chdir("/"), optionally close all fds
 *
 * returns -1 on failure, but you can't do much except exit in that case
 * since we may already have forked
 */
int daemonize(int nochdir, int noclose)
{
    switch (fork()) {
    case 0:
        break;
    case -1:
        return -1;
    default:
        _exit(0);
    }

    if (setsid() < 0)
        return -1;

    switch (fork()) {
    case 0: 
        break;
    case -1:
        return -1;
    default:
        _exit(0);
    }

    if (!nochdir)
        chdir("/");

    if (!noclose) {
        closeall(0);
        open("/dev/null",O_RDWR);
        dup(0);
        dup(0);
    }

    return 0;
}

static uid_t saved_uid = -1;

/*
 * seteuid(0) and back, if either fails and panic != 0 we PANIC
 */
void become_root(void)
{
    if (getuid() == 0) {
        saved_uid = geteuid();
        if (seteuid(0) != 0)
            AFP_PANIC("Can't seteuid(0)");
    }
}

void unbecome_root(void)
{
    if (getuid() == 0) {
        if (saved_uid == -1 || seteuid(saved_uid) < 0)
            AFP_PANIC("Can't seteuid back");
        saved_uid = -1;
    }
}

/*!
 * @brief get cwd in static buffer
 *
 * @returns pointer to path or pointer to error messages on error
 */
const char *getcwdpath(void)
{
    static char cwd[MAXPATHLEN + 1];
    char *p;

    if ((p = getcwd(cwd, MAXPATHLEN)) != NULL)
        return p;
    else
        return strerror(errno);
}

/*!
 * @brief Request absolute path
 *
 * @returns Absolute filesystem path to object
 */
const char *fullpathname(const char *name)
{
    static char wd[MAXPATHLEN + 1];

    if (name[0] == '/')
        return name;

    if (getcwd(wd , MAXPATHLEN)) {
        strlcat(wd, "/", MAXPATHLEN);
        strlcat(wd, name, MAXPATHLEN);
    } else {
        strlcpy(wd, name, MAXPATHLEN);
    }

    return wd;
}

/*!
 * Takes a buffer with a path, strips slashs, returns basename
 *
 * @param p (rw) path
 *        path may be
 *          "[/][dir/[...]]file"
 *        or
 *          "[/][dir/[...]]dir/[/]"
 *        Result is "file" or "dir" 
 *
 * @returns pointer to basename in path buffer, buffer is possibly modified
 */
char *stripped_slashes_basename(char *p)
{
    int i = strlen(p) - 1;
    while (i > 0 && p[i] == '/')
        p[i--] = 0;
    return (strrchr(p, '/') ? strrchr(p, '/') + 1 : p);
}

/*!
 * @brief symlink safe chdir replacement
 *
 * Only chdirs to dir if it doesn't contain symlinks.
 *
 * @returns 1 if a path element is a symlink, 0 otherwise, -1 on syserror
 */
int lchdir(const char *dir)
{
    char buf[MAXPATHLEN+1];
    char cwd[MAXPATHLEN+1];
    char *test;
    int  i;

    /*
     dir is a canonical path (without "../" "./" "//" )
     but may end with a / 
    */
    *cwd = 0;
    if (*dir != '/') {
        if (getcwd(cwd, MAXPATHLEN) == NULL)
            return -1;
    }
    if (chdir(dir) != 0)
        return -1;

    /* 
     * Cases:
     * chdir request   | realpath result | ret
     * (after getwcwd) |                 |
     * =======================================
     * /a/b/.          | /a/b            | 0
     * /a/b/.          | /c              | 1
     * /a/b/.          | /c/d/e/f        | 1
     */
    if (getcwd(buf, MAXPATHLEN) == NULL)
        return 1;

    i = 0;
    if (*cwd) {
        /* relative path requested, 
         * Same directory?
        */
        for (; cwd[i]; i++) {
            if (buf[i] != cwd[i])
                return 1;
        }
        if (buf[i]) {
            if (buf[i] != '/')
                return 1;
            i++;
        }                    
    }

    test = &buf[i];    
    for (i = 0; test[i]; i++) {
        if (test[i] != dir[i]) {
            return 1;
        }
    }
    /* trailing '/' ? */
    if (!dir[i])
        return 0;

    if (dir[i] != '/')
        return 1;

    i++;
    if (dir[i])
        return 1;

    return 0;
}

/*!
 * Store n random bytes an buf
 */
void randombytes(void *buf, int n)
{
    char *p = (char *)buf;
    int fd, i;
    struct timeval tv;

    if ((fd = open("/dev/urandom", O_RDONLY)) != -1) {
        /* generate from /dev/urandom */
        if (read(fd, buf, n) != n) {
            close(fd);
            fd = -1;
        } else {
            close(fd);
            /* fd now != -1, so srandom wont be called below */
        }
    }

    if (fd == -1) {
        gettimeofday(&tv, NULL);
        srandom((unsigned int)tv.tv_usec);
        for (i=0 ; i < n ; i++)
            p[i] = random() & 0xFF;
    }

    return;
}

int gmem(gid_t gid, int ngroups, gid_t *groups)
{
    int		i;

    for ( i = 0; i < ngroups; i++ ) {
        if ( groups[ i ] == gid ) {
            return( 1 );
        }
    }
    return( 0 );
}

/*
 * realpath() replacement that always allocates storage for returned path
 */
char *realpath_safe(const char *path)
{
    char *resolved_path;

#ifdef REALPATH_TAKES_NULL
    if ((resolved_path = realpath(path, NULL)) == NULL) {
        LOG(log_error, logtype_afpd, "realpath() cannot resolve path \"%s\"", path);
        return NULL;
    }
    return resolved_path;
#else
    if ((resolved_path = malloc(MAXPATHLEN+1)) == NULL)
        return NULL;
    if (realpath(path, resolved_path) == NULL) {
        free(resolved_path);
        LOG(log_error, logtype_afpd, "realpath() cannot resolve path \"%s\"", path);
        return NULL;
    }
    /* Safe some memory */
    char *tmp;
    if ((tmp = strdup(resolved_path)) == NULL) {
        free(resolved_path);
        return NULL;
    }
    free(resolved_path);
    resolved_path = tmp;
    return resolved_path;
#endif
}
