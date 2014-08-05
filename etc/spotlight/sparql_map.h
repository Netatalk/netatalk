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
    ssmt_type    /* kMDItemContentType, requires special mapping */
};

enum kMDTypeMap {
    kMDTypeMapNotSup,           /* not supported */
    kMDTypeMapRDF,              /* query with rdf:type */
    kMDTypeMapMime              /* query with nie:mimeType */
};

struct spotlight_sparql_map {
    const char *ssm_spotlight_attr;
    bool ssm_enabled;
    enum ssm_type ssm_type;
    const char *ssm_sparql_attr;
};

struct MDTypeMap {
    const char *mdtm_value;     /* MD query value of attributes '_kMDItemGroupId' and 'kMDItemContentTypeTree' */
    enum kMDTypeMap mdtm_type;   /* whether SPARQL query must search attribute rdf:type or nie:mime_Type */
    const char *mdtm_sparql;    /* the SPARQL query match string */
};

extern struct spotlight_sparql_map spotlight_sparql_map[];
extern struct spotlight_sparql_map spotlight_sparql_date_map[];
extern struct MDTypeMap MDTypeMap[];
#endif
