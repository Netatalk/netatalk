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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifndef SLMOD_TRACKER_0_6_MAP_H
#define SLMOD_TRACKER_0_6_MAP_H

#include <tracker.h>

enum stm_type {
    stmt_fts,    /* Full text search */
    stmt_name,   /* Object name */
    stmt_type    /* Object type */
};

struct spotlight_tracker_map {
    const char    *stm_spotlight_attr;
    enum stm_type stm_type;
};

struct MDTypeMap {
    const char  *mdtm_value;     /* Value of '_kMDItemGroupId' or 'kMDItemContentTypeTree' */
    ServiceType mdtm_type;       /* Tracker 0.6 service type */
};

extern struct spotlight_tracker_map spotlight_tracker_map[];
extern struct MDTypeMap MDTypeMap[];
#endif
