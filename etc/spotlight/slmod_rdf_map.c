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

#include "slmod_rdf_map.h"

#define NOTSUPPORTED NULL
#define SPECIAL      NULL

struct spotlight_rdf_map spotlight_rdf_map[] = {
    /* srm_spotlight_attr               srm_type,   srm_rdf_attr */
    {"*",                               srmt_str,   "File:Name"},
    {"kMDItemTextContent",              srmt_fts,   ""},

    /* Filesystem metadata */
    {"kMDItemFSLabel",                  srmt_num,   NOTSUPPORTED},
    {"kMDItemDisplayName",              srmt_str,   "File:Name"},
    {"kMDItemFSName",                   srmt_str,   "File:Name"},
#if 0
    {"kMDItemFSContentChangeDate",      ssmt_date,  "nfo:fileLastModified"},

    /* Common metadata */
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
#endif
    {NULL, srmt_str, NULL}
};

struct MDTypeMap MDTypeMap[] = {
    {"1",                       "http://www.semanticdesktop.org/ontologies/2007/03/22/nmo#Email"},
    {"2",                       "http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Contact"},
    {"3",                       NULL}, /* PrefPane */
    {"4",                       "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Font"},
    {"5",                       "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Bookmark"},
    {"6",                       "http://www.semanticdesktop.org/ontologies/2007/03/22/nco#Contact"},
    {"7",                       "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Video"},
    {"8",                       "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Executable"},
    {"9",                       "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Folder"},
    {"10",                      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Audio"},
    {"11",                      "application/pdf"},
    {"12",                      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Presentation"},
    {"13",                      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Image"},
    {"public.jpeg",             "image/jpeg"},
    {"public.tiff",             "image/tiff"},
    {"com.compuserve.gif",      "image/gif"},
    {"public.png",              "image/png"},
    {"com.microsoft.bmp",       "image/bmp"},
    {"public.content",          "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Document"},
    {"public.mp3",              "audio/mpeg"},
    {"public.mpeg-4-audio",     "audio/x-aac"},
    {"com.apple.application",   "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#Software"},
    {"public.text",             "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#TextDocument"},
    {"public.plain-text",       "text/plain"},
    {"public.rtf",              "text/rtf"},
    {"public.html",             "text/html"},
    {"public.xml",              "text/xml"},
    {"public.source-code",      "http://www.semanticdesktop.org/ontologies/2007/03/22/nfo#SourceCode"},
    {NULL,                      NULL}
};
