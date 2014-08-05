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
#include <string.h>
#include <stdlib.h>

#include <atalk/logger.h>

#include "sparql_map.h"

#define NOTSUPPORTED NULL
#define SPECIAL      NULL

struct spotlight_sparql_map spotlight_sparql_map[] = {
    /* ssm_spotlight_attr               ssm_enabled, ssm_type,   ssm_sparql_attr */
    {"*",                               true, ssmt_fts,   "fts:match"},

    /* Filesystem metadata */
    {"kMDItemFSLabel",                  true, ssmt_num,   NOTSUPPORTED},
    {"kMDItemDisplayName",              true, ssmt_str,   "nfo:fileName"},
    {"kMDItemFSName",                   true, ssmt_str,   "nfo:fileName"},
    {"kMDItemFSContentChangeDate",      true, ssmt_date,  "nfo:fileLastModified"},

    /* Common metadata */
    {"kMDItemTextContent",              true, ssmt_fts,   "fts:match"},
    {"kMDItemContentCreationDate",      true, ssmt_date,  "nie:contentCreated"},
    {"kMDItemContentModificationDate",  true, ssmt_date,  "nfo:fileLastModified"},
    {"kMDItemAttributeChangeDate",      true, ssmt_date,  "nfo:fileLastModified"},
    {"kMDItemLastUsedDate",             true, ssmt_date,  "nfo:fileLastAccessed"},
    {"kMDItemAuthors",                  true, ssmt_str,   "dc:creator"},
    {"kMDItemCopyright",                true, ssmt_str,   "nie:copyright"},
    {"kMDItemCountry",                  true, ssmt_str,   "nco:country"},
    {"kMDItemCreator",                  true, ssmt_str,   "dc:creator"},
    {"kMDItemDurationSeconds",          true, ssmt_num,   "nfo:duration"},
    {"kMDItemNumberOfPages",            true, ssmt_num,   "nfo:pageCount"},
    {"kMDItemTitle",                    true, ssmt_str,   "nie:title"},
    {"_kMDItemGroupId",                 true, ssmt_type,  SPECIAL},
    {"kMDItemContentTypeTree",          true, ssmt_type,  SPECIAL},

    /* Image metadata */
    {"kMDItemPixelWidth",               true, ssmt_num,   "nfo:width"},
    {"kMDItemPixelHeight",              true, ssmt_num,   "nfo:height"},
    {"kMDItemColorSpace",               true, ssmt_str,   "nexif:colorSpace"},
    {"kMDItemBitsPerSample",            true, ssmt_num,   "nfo:colorDepth"},
    {"kMDItemFocalLength",              true, ssmt_num,   "nmm:focalLength"},
    {"kMDItemISOSpeed",                 true, ssmt_num,   "nmm:isoSpeed"},
    {"kMDItemOrientation",              true, ssmt_bool,  "nfo:orientation"},
    {"kMDItemResolutionWidthDPI",       true, ssmt_num,   "nfo:horizontalResolution"},
    {"kMDItemResolutionHeightDPI",      true, ssmt_num,   "nfo:verticalResolution"},
    {"kMDItemExposureTimeSeconds",      true, ssmt_num,   "nmm:exposureTime"},

    /* Audio metadata */
    {"kMDItemComposer",                 true, ssmt_str,   "nmm:composer"},
    {"kMDItemMusicalGenre",             true, ssmt_str,   "nfo:genre"},

    {NULL, false, ssmt_str, NULL}
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

void configure_spotlight_attributes(const char *attributes_in)
{
    char *attr, *attributes;
    int i;

    for (i = 0; spotlight_sparql_map[i].ssm_spotlight_attr != NULL; i++)
        spotlight_sparql_map[i].ssm_enabled = false;

    /*
     * Go through the attribute map and for every element scan
     * attributes_in with strtok(). If it's contained, keep it
     * enabled, otherwise disable it.
     */

    attributes = strdup(attributes_in);

    for (attr = strtok(attributes, ","); attr; attr = strtok(NULL, ",")) {

        for (i = 0; spotlight_sparql_map[i].ssm_spotlight_attr != NULL; i++)

            if (strcmp(attr, spotlight_sparql_map[i].ssm_spotlight_attr) == 0) {
                LOG(log_info, logtype_sl, "Enabling Spotlight attribute: %s",
                    spotlight_sparql_map[i].ssm_spotlight_attr);
                spotlight_sparql_map[i].ssm_enabled = true;
                break;
        }
    }

    free(attributes);
}
