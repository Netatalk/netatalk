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

#include <unistd.h>

#include "spotlight_SPARQL_map.h"

struct spotlight_sparql_map spotlight_sparql_map[] = {
/*   ssm_spotlight_attr                    ssm_sparql_attr              ssm_sparql_query_fmtstr */
    {"*",                                  "fts:match",                 "?x fts:match '%s'"},
    {"kMDItemTextContent",                 "fts:match",                 "?x fts:match '%s'"},
    {"kMDItemDisplayName",                 "nfo:fileName",              "?x nfo:fileName ?y FILTER(regex(?y, '%s'))"},
    {"kMDItemContentCreationDate",         "nfo:fileCreated",           "?x nfo:fileCreated '%s'"},
    {"kMDItemFSContentChangeDate",         "nfo:fileLastModified",      "?x nfo:fileLastModified '%s'"},
    {"kMDItemContentModificationDate",     "nfo:fileLastModified",      "?x nfo:fileLastModified '%s'"},
    {NULL, NULL, NULL}
};
