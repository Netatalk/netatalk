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
    /* ssm_spotlight_attr               ssm_type,   ssm_sparql_attr */
    {"*",                               ssmt_fts,   "fts:match"},

    /* Filesystem metadata */
    {"kMDItemFSLabel", ssmt_num, ""},
    {"kMDItemDisplayName",              ssmt_str,   "nfo:fileName"},
    {"kMDItemFSName",                   ssmt_str,   "nfo:fileName"},
    {"kMDItemFSContentChangeDate",      ssmt_date,  "nfo:fileLastModified"},

    /* Common metadata */
    {"kMDItemTextContent",              ssmt_fts,   "fts:match"},
    {"kMDItemContentCreationDate",      ssmt_date,  "nie:contentCreated"},
    {"kMDItemContentModificationDate",  ssmt_date,  "nfo:fileLastModified"},
    {"kMDItemAttributeChangeDate",      ssmt_date,  "nfo:fileLastModified"},
    {"kMDItemAuthors",                  ssmt_str,   "dc:creator"},
    {"kMDItemContentType",              ssmt_cnt,   "nie:mimeType"},
    {"kMDItemCopyright",                ssmt_str,   "nie:copyright"},
    {"kMDItemCountry",                  ssmt_str,   "nco:country"},
    {"kMDItemCreator",                  ssmt_str,   "dc:creator"},
    {"kMDItemDurationSeconds",          ssmt_num,   "nfo:duration"},
    {"kMDItemNumberOfPages",            ssmt_num,   "nfo:pageCount"},
    {"kMDItemTitle",                    ssmt_str,   "nie:title"},

    /* Image metadata */
    {"kMDItemPixelWidth",               ssmt_num,   "nfo:width"},
    {"kMDItemPixelHeight",              ssmt_num,   "nfo:height"},
    {"kMDItemColorSpace",               ssmt_str,   "nexif:colorSpace"},
    {"kMDItemBitsPerSample",            ssmt_num,   "nfo:colorDepth"},
    {"kMDItemFocalLength",              ssmt_num,   "nmm:focalLength"},
    {"kMDItemISOSpeed",                 ssmt_num,   "nmm:isoSpeed"},
    {"kMDItemOrientation",              ssmt_bool,  "nfo:orientation"},
    {"kMDItemResolutionWidthDPI",       ssmt_num,   "nfo:horizontalResolution"},
    {"kMDItemResolutionHeightDPI",      ssmt_num,   "nfo:verticalResolution"},
    {"kMDItemExposureTimeSeconds",      ssmt_num,   "nmm:exposureTime"},

    /* Audio metadata */
    {"kMDItemComposer",                 ssmt_str,   "nmm:composer"},
    {"kMDItemMusicalGenre",             ssmt_str,   "nfo:genre"},

    {NULL, ssmt_str, NULL}
};
