/*
 * $Id: ad_size.c,v 1.5 2002-10-11 14:18:38 didg Exp $
 *
 * Copyright (c) 1997 Adrian Sun (asun@zoology.washington.edu)
 * All rights reserved. See COPYRIGHT.
 *
 * if we could depend upon inline functions, this would be one.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/types.h>
#include <sys/stat.h>
#include <atalk/logger.h>

#include <atalk/adouble.h>

off_t ad_size(const struct adouble *ad, const u_int32_t eid)
{
  if (eid == ADEID_DFORK) {
    struct stat st;
    
    if (fstat(ad_dfileno(ad), &st) < 0)
      return 0;
    return st.st_size;
  }  
#if 0
  return ad_getentrylen(ad, eid);
#endif
  return ad->ad_rlen;  
} 
