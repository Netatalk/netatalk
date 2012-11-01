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

#ifndef SPOTLIGHT_SPARQL_MAP_H
#define SPOTLIGHT_SPARQL_MAP_H

enum ssm_type {
    ssmt_bool,   /* a boolean value that doesn't requires a SPARQL FILTER */
    ssmt_num,    /* a numeric value that requires a SPARQL FILTER */
    ssmt_str,    /* a string value that requieres a SPARQL FILTER */
    ssmt_fts,    /* a string value that will be queried with SPARQL 'fts:match' */
    ssmt_date,   /* date values are handled in a special map function map_daterange() */
    ssmt_cnt     /* kMDItemContentType, requires special mapping */
};

struct spotlight_sparql_map {
    const char *ssm_spotlight_attr;
    enum ssm_type ssm_type;
    const char *ssm_sparql_attr;
};

extern struct spotlight_sparql_map spotlight_sparql_map[];
extern struct spotlight_sparql_map spotlight_sparql_date_map[];

#endif
