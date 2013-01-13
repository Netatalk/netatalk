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

#include "slmod_sparql_map.h"

#define NOTSUPPORTED NULL
#define SPECIAL      NULL

struct spotlight_sparql_map spotlight_sparql_map[] = {
    /* ssm_spotlight_attr               ssm_type,   ssm_sparql_attr */
    {"*",                               ssmt_fts,   "fts:match"},

    /* Filesystem metadata */
    {"kMDItemFSLabel",                  ssmt_num,   NOTSUPPORTED},
    {"kMDItemDisplayName",              ssmt_str,   "nfo:fileName"},
    {"kMDItemFSName",                   ssmt_str,   "nfo:fileName"},
    {"kMDItemFSContentChangeDate",      ssmt_date,  "nfo:fileLastModified"},

    /* Common metadata */
    {"kMDItemTextContent",              ssmt_fts,   "fts:match"},
    {"kMDItemContentCreationDate",      ssmt_date,  "nie:contentCreated"},
    {"kMDItemContentModificationDate",  ssmt_date,  "nfo:fileLastModified"},
    {"kMDItemAttributeChangeDate",      ssmt_date,  "nfo:fileLastModified"},
    {"kMDItemAuthors",                  ssmt_str,   "dc:creator"},
    {"kMDItemCopyright",                ssmt_str,   "nie:copyright"},
    {"kMDItemCountry",                  ssmt_str,   "nco:country"},
    {"kMDItemCreator",                  ssmt_str,   "dc:creator"},
    {"kMDItemDurationSeconds",          ssmt_num,   "nfo:duration"},
    {"kMDItemNumberOfPages",            ssmt_num,   "nfo:pageCount"},
    {"kMDItemTitle",                    ssmt_str,   "nie:title"},
    {"_kMDItemGroupId",                 ssmt_type,  SPECIAL},
    {"kMDItemContentTypeTree",          ssmt_type,  SPECIAL},

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

struct MDTypeMap MDTypeMap[] = {
    {"1",                       kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#Email"},
    {"2",                       kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Contact"},
    {"3",                       kMDTypeMapNotSup,   NULL}, /* PrefPane */
    {"4",                       kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Font"},
    {"5",                       kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Bookmark"},
    {"6",                       kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Contact"},
    {"7",                       kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Video"},
    {"8",                       kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Executable"},
    {"9",                       kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Folder"},
    {"10",                      kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Audio"},
    {"11",                      kMDTypeMapMime,     "application/pdf"},
    {"12",                      kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Presentation"},
    {"13",                      kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Image"},
    {"public.jpeg",             kMDTypeMapMime,     "image/jpeg"},
    {"public.tiff",             kMDTypeMapMime,     "image/tiff"},
    {"com.compuserve.gif",      kMDTypeMapMime,     "image/gif"},
    {"public.png",              kMDTypeMapMime,     "image/png"},
    {"com.microsoft.bmp",       kMDTypeMapMime,     "image/bmp"},
    {"public.content",          kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Document"},
    {"public.mp3",              kMDTypeMapMime,     "audio/mpeg"},
    {"public.mpeg-4-audio",     kMDTypeMapMime,     "audio/x-aac"},
    {"com.apple.application",   kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Software"},
    {"public.text",             kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#TextDocument"},
    {"public.plain-text",       kMDTypeMapMime,     "text/plain"},
    {"public.rtf",              kMDTypeMapMime,     "text/rtf"},
    {"public.html",             kMDTypeMapMime,     "text/html"},
    {"public.xml",              kMDTypeMapMime,     "text/xml"},
    {"public.source-code",      kMDTypeMapRDF,      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#SourceCode"},
    {NULL,                      kMDTypeMapNotSup,   NULL}
};
