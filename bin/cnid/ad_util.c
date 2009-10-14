/* 
   $Id: ad_util.c,v 1.2 2009-10-14 02:30:42 didg Exp $

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

#include <atalk/cnid.h>
#include <atalk/volinfo.h>
#include "ad.h"


int newvol(const char *path, afpvol_t *vol)
{
//    char *pathdup;

    memset(vol, 0, sizeof(afpvol_t));

//    pathdup = strdup(path);
//    vol->dirname = strdup(dirname(pathdup));
//    free(pathdup);

//    pathdup = strdup(path);
//    vol->basename = strdup(basename(pathdup));
//    free(pathdup);

    loadvolinfo((char *)path, &vol->volinfo);

    return 0;
}

void freevol(afpvol_t *vol)
{
#if 0
    if (vol->dirname) {
        free(vol->dirname);
        vol->dirname = NULL;
    }
    if (vol->basename) {
        free(vol->basename);
        vol->basename = NULL;
    }
#endif
}
