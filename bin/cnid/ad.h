/* 
   $Id: ad.h,v 1.1 2009-09-01 14:28:07 franklahm Exp $

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

#ifndef AD_H
#define AD_H

#include <atalk/volinfo.h>

#define STRCMP(a,b,c) (strcmp(a,c) b 0)

#define DIR_DOT_OR_DOTDOT(a) \
        ((strcmp(a, ".") == 0) || (strcmp(a, "..") == 0))

typedef struct {
    struct volinfo volinfo;
//    char *dirname;
//    char *basename;
//    int adflags;                /* file:0, dir:ADFLAGS_DIR */
} afpvol_t;


extern int newvol(const char *path, afpvol_t *vol);
extern void freevol(afpvol_t *vol);

extern int ad_ls(int argc, char **argv);
extern int ad_cp(int argc, char **argv);

#endif /* AD_H */
