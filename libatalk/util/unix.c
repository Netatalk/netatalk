/*
  $Id: unix.c,v 1.4 2010-02-19 11:29:52 franklahm Exp $
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

#include <atalk/adouble.h>
#include <atalk/ea.h>
#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/volume.h>
#include <atalk/vfs.h>
#include <atalk/util.h>
#include <atalk/unix.h>

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
 * @brief symlink safe chdir replacement
 *
 * Only chdirs to dir if it doesn't contain symlinks.
 *
 * @returns 1 if a path element is a symlink, 0 otherwise, -1 on syserror
 */
int lchdir(const char *dir)
{
    int ret = 0;
    char buf[MAXPATHLEN+1];
#ifdef REALPATH_TAKES_NULL
    char *rpath = NULL;
#else
    char rpath[MAXPATHLEN+1];
#endif

    /* dir might be an relative or an absolute path */
    if (dir[0] == '/') {
        /* absolute path, just make sure buf is prepared for strlcat */
        buf[0] = 0;
    } else {
        /* relative path, push cwd int buf */
        if (getcwd(buf, MAXPATHLEN) == NULL)
            return -1;
        if (strlcat(buf, "/", MAXPATHLEN) >= MAXPATHLEN)
            return -1;
    }

    if (strlcat(buf, dir, MAXPATHLEN) >= MAXPATHLEN)
        return -1;

#ifdef REALPATH_TAKES_NULL
    if ((rpath = realpath(dir, NULL)) == NULL) {
#else
    if (realpath(dir, rpath) == NULL) {
#endif
        ret = -1;
        goto exit;
    }

    /* 
     * Cases:
     * chdir request   | realpath result | ret
     * (after getwcwd) |                 |
     * =======================================
     * /a/b/.          | /a/b            | 0
     * /a/b/.          | /c              | 1
     * /a/b/.          | /c/d/e/f        | 1
     */
    ret = 0;
    for (int i = 0; rpath[i]; i++) {
        if (buf[i] != rpath[i]) {
            ret = 1;
            goto exit;
        }
    }

    if (chdir(dir) != 0) {
        ret = -1;
        goto exit;
    }

exit:
#ifdef REALPATH_TAKES_NULL
    free(rpath);
#endif
    return ret;
}
