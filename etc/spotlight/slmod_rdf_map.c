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
    {"kMDItemFSContentChangeDate",      srmt_date,  "File:Modified"},

    /* Common metadata */
    {"kMDItemContentCreationDate",      srmt_date,  "Doc:Created"},
    {"kMDItemContentModificationDate",  srmt_date,  "File:Modified"},
    {"kMDItemAttributeChangeDate",      srmt_date,  "File:Modified"},
    {"kMDItemAuthors",                  srmt_str,   "Doc:Author"},
    {"kMDItemCopyright",                srmt_str,   "File:Copyright"},
    {"kMDItemCountry",                  srmt_str,   "Image:Country"},
    {"kMDItemCreator",                  srmt_str,   "DC:Creator"},
    {"kMDItemDurationSeconds",          srmt_num,   "Audio:Duration"},
    {"kMDItemNumberOfPages",            srmt_num,   "Doc:PageCount"},
    {"kMDItemTitle",                    srmt_str,   "DC:Title"},
    {"_kMDItemGroupId",                 srmt_type,  "File:Mime"},
    {"kMDItemContentTypeTree",          srmt_type,  "File:Mime"},

    /* Image metadata */
    {"kMDItemPixelWidth",               srmt_num,   "Image:Width"},
    {"kMDItemPixelHeight",              srmt_num,   "Image:Height"},
    {"kMDItemColorSpace",               srmt_str,   NOTSUPPORTED},
    {"kMDItemBitsPerSample",            srmt_num,   NOTSUPPORTED},
    {"kMDItemFocalLength",              srmt_num,   "Image:FocalLength"}, /* RDF: float */
    {"kMDItemISOSpeed",                 srmt_num,   "Image:ISOSpeed"},
    {"kMDItemOrientation",              srmt_bool,  "Image:Orientation"},
    {"kMDItemResolutionWidthDPI",       srmt_num,   NOTSUPPORTED},
    {"kMDItemResolutionHeightDPI",      srmt_num,   NOTSUPPORTED},
    {"kMDItemExposureTimeSeconds",      srmt_num,   "Image:ExposureTime"}, /* RDF: fload */

    /* Audio metadata */
    {"kMDItemComposer",                 srmt_str,   NOTSUPPORTED},
    {"kMDItemMusicalGenre",             srmt_str,   "Audio:Genre"},

    {NULL, srmt_str, NULL}
};

struct MDTypeMap MDTypeMap[] = {
    {"1",                       "equals",      "message/rfc822"},
    {"2",                       "equals",      "text/x-vcard"},
    {"3",                       NOTSUPPORTED}, /* PrefPane */
    {"4",                       NOTSUPPORTED}, /* Font. There's no single mime type to match all font formats, ugh! */
    {"5",                       NOTSUPPORTED}, /* Bookmark */
    {"6",                       "equals",      "text/x-vcard"},
    {"7",                       "startsWith",  "video"},
    {"8",                       NOTSUPPORTED}, /* Executable */
    {"9",                       NOTSUPPORTED}, /* Folder */
    {"10",                      "startsWith",  "audio"},
    {"11",                      "equals",      "application/pdf"},
    {"12",                      NOTSUPPORTED}, /* Presentation */
    {"13",                      "startsWith",  "image"},
    {"public.jpeg",             "equals",      "image/jpeg"},
    {"public.tiff",             "equals",      "image/tiff"},
    {"com.compuserve.gif",      "equals",      "image/gif"},
    {"public.png",              "equals",      "image/png"},
    {"com.microsoft.bmp",       "equals",      "image/bmp"},
    {"public.content",          "inSet",       "application/msword,application/pdf,application/vnd.ms-excel,application/vnd.oasis.opendocument.text,application/vnd.sun.xml.writer"},
    {"public.mp3",              "equals",      "audio/mpeg"},
    {"public.mpeg-4-audio",     "equals",      "audio/x-aac"},
    {"com.apple.application",   NOTSUPPORTED},
    {"public.text",             "startsWith",  "text"},
    {"public.plain-text",       "equals",      "text/plain"},
    {"public.rtf",              "equals",      "text/rtf"},
    {"public.html",             "equals",      "text/html"},
    {"public.xml",              "equals",      "text/xml"},
    {"public.source-code",      NOTSUPPORTED},
    {NULL,                      NULL}
};
