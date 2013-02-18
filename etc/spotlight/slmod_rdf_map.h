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

#ifndef SLMOD_RDF_MAP_H
#define SLMOD_RDF_MAP_H

/* Spotlight -> RDF mapping (srm) */
enum srm_type {
    srmt_bool,   /* a boolean value */
    srmt_num,    /* a numeric value */
    srmt_str,    /* a string value */
    srmt_fts,    /* a string value for full text search */
    srmt_date,   /* date values are handled in a special map function map_daterange() */
    srmt_type    /* kMDItemContentType, requires special mapping */
};

struct spotlight_rdf_map {
    const char    *srm_spotlight_attr;
    enum srm_type  srm_type;
    const char    *srm_rdf_attr;
};

struct MDTypeMap {
    const char  *mdtm_value;     /* Value of '_kMDItemGroupId' or 'kMDItemContentTypeTree' */
    const char  *mdtm_type;      /* MIME type */
};

extern struct spotlight_rdf_map spotlight_rdf_map[];
extern struct MDTypeMap MDTypeMap[];
#endif
