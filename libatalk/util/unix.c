/*
  $Id: unix.c,v 1.1 2010-01-05 13:48:47 franklahm Exp $
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

extern const char *getcwdpath(void)
{
    static char cwd[MAXPATHLEN + 1];
    char *p;

    if ((p = getcwd(&cwd, MAXPATHLEN)) != NULL)
        return p;
    else
        return strerror(errno);
}
