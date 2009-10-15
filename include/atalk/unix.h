/*
   $Id: unix.h,v 1.1 2009-10-15 12:06:07 franklahm Exp $
   Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#ifndef ATALK_UNIX_H
#define ATALK_UNIX_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>

/* vfs/unix.c */
extern int netatalk_unlink(const char *name);
extern char *fullpathname(const char *);
extern int netatalk_rmdir(const char *name);
extern int setfilmode(const char *, mode_t, struct stat *, mode_t);
extern int dir_rx_set(mode_t mode);
extern int stickydirmode(const char *name, const mode_t mode, const int dropbox, const mode_t v_umask);
extern int unix_rename(const char *oldpath, const char *newpath);
extern int copy_file(const char *src, const char *dst, mode_t mode);

#endif  /* ATALK_UNIX_H */
