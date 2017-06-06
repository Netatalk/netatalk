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

#include <atalk/adouble.h>
#include <atalk/ea.h>
#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/vfs.h>
#include <atalk/util.h>
#include <atalk/unix.h>

/* close all FDs >= a specified value */
static void closeall(int fd)
{
    int fdlimit = sysconf(_SC_OPEN_MAX);

    while (fd < fdlimit)
        close(fd++);
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

void become_root(void)
{
    saved_uid = geteuid();
    if (seteuid(0) != 0)
        AFP_PANIC("Can't seteuid(0)");
}

void unbecome_root(void)
{
    if (saved_uid == -1 || seteuid(saved_uid) < 0)
        AFP_PANIC("Can't seteuid back");
    saved_uid = -1;
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

/*********************************************************************************
 * chdir(), chmod(), chown(), stat() wrappers taking an additional option.
 * Currently the only used options are O_NOFOLLOW, used to switch between symlink
 * behaviour, and O_NETATALK_ACL for ochmod() indicating chmod_acl() shall be
 * called which does special ACL handling depending on the filesytem
 *********************************************************************************/

int ostat(const char *path, struct stat *buf, int options)
{
    if (options & O_NOFOLLOW)
        return lstat(path, buf);
    else
        return stat(path, buf);
}

int ochown(const char *path, uid_t owner, gid_t group, int options)
{
    if (options & O_NOFOLLOW)
        return lchown(path, owner, group);
    else
        return chown(path, owner, group);
}

/*!
 * chmod() wrapper for symlink and ACL handling
 *
 * @param path       (r) path
 * @param mode       (r) requested mode
 * @param sb         (r) stat() of path or NULL
 * @param option     (r) O_NOFOLLOW | O_NETATALK_ACL
 *
 * Options description:
 * O_NOFOLLOW: don't chmod() symlinks, do nothing, return 0
 * O_NETATALK_ACL: call chmod_acl() instead of chmod()
 */
int ochmod(const char *path, mode_t mode, const struct stat *st, int options)
{
    struct stat sb;

    if (!st) {
        if (lstat(path, &sb) != 0)
            return -1;
        st = &sb;
    }

    if (options & O_NOFOLLOW)
        if (S_ISLNK(st->st_mode))
            return 0;

    if (options & O_NETATALK_ACL) {
        return chmod_acl(path, mode);
    } else {
        return chmod(path, mode);
    }
}

/* 
 * @brief ostat/fsstatat multiplexer
 *
 * ostatat mulitplexes ostat and fstatat. If we don't HAVE_ATFUNCS, dirfd is ignored.
 *
 * @param dirfd   (r) Only used if HAVE_ATFUNCS, ignored else, -1 gives AT_FDCWD
 * @param path    (r) pathname
 * @param st      (rw) pointer to struct stat
 */
int ostatat(int dirfd, const char *path, struct stat *st, int options)
{
#ifdef HAVE_ATFUNCS
    if (dirfd == -1)
        dirfd = AT_FDCWD;
    return fstatat(dirfd, path, st, (options & O_NOFOLLOW) ? AT_SYMLINK_NOFOLLOW : 0);
#else
    return ostat(path, st, options);
#endif            

    /* DEADC0DE */
    return -1;
}

/*!
 * @brief symlink safe chdir replacement
 *
 * Only chdirs to dir if it doesn't contain symlinks or if symlink checking
 * is disabled
 *
 * @returns 1 if a path element is a symlink, 0 otherwise, -1 on syserror
 */
int ochdir(const char *dir, int options)
{
    char buf[MAXPATHLEN+1];
    char cwd[MAXPATHLEN+1];
    char *test;
    int  i;

    if (!(options & O_NOFOLLOW))
        return chdir(dir);

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
