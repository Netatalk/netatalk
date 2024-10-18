/*
   Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

#ifndef ATALK_COMPAT_H
#define ATALK_COMPAT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_STRLCPY
extern size_t strlcpy(char *d, const char *s, size_t bufsize);
#endif

#ifndef HAVE_STRLCAT
extern size_t strlcat(char *d, const char *s, size_t bufsize);
#endif

#endif
