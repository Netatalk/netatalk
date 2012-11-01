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
/*   ssm_spotlight_attr                 ssm_type,   ssm_sparql_attr */
    {"*",                               ssmt_fts,   "fts:match"},
    {"kMDItemTextContent",              ssmt_fts,   "fts:match"},
    {"kMDItemDisplayName",              ssmt_str,   "nfo:fileName"},
    {"kMDItemContentCreationDate",      ssmt_date,  "nfo:fileCreated"},
    {"kMDItemFSContentChangeDate",      ssmt_date,  "nfo:fileLastModified"},
    {"kMDItemContentModificationDate",  ssmt_date,  "nfo:fileLastModified"},
    {"kMDItemPixelWidth",               ssmt_num,   "nfo:width"},
    {"kMDItemPixelHeight",              ssmt_num,   "nfo:height"},
    {NULL, ssmt_str, NULL}
};
