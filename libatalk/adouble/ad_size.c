/*
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 *
 * if we could depend upon inline functions, this would be one.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>

#include <atalk/adouble.h>

off_t ad_size(const struct adouble *ad, const u_int32_t eid)
{
  if (eid == ADEID_DFORK) {
    struct stat st;
    
    if (fstat(ad_dfileno(ad), &st) < 0)
      return 0;
    return st.st_size;
  }  

  return ad_getentrylen(ad, eid);
} 
