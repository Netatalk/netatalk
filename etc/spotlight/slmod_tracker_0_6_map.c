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

#include "slmod_tracker_0_6_map.h"

struct spotlight_tracker_map spotlight_tracker_map[] = {
    /* stm_spotlight_attr,              stm_type */
    {"*",                               stmt_fts},
    {"kMDItemTextContent",              stmt_fts},
    {"kMDItemDisplayName",              stmt_name},
    {"kMDItemFSName",                   stmt_name},
    {"_kMDItemGroupId",                 stmt_type},
    {"kMDItemContentTypeTree",          stmt_type},
    {NULL, -1}
};

struct MDTypeMap MDTypeMap[] = {
    {"1",                       SERVICE_EMAILS},
    {"2",                       SERVICE_CONTACTS},
    {"3",                       -1}, /* Prefpane */
    {"4",                       -1}, /* Fonts */
    {"5",                       SERVICE_BOOKMARKS},
    {"6",                       SERVICE_CONTACTS},
    {"7",                       SERVICE_VIDEOS},
    {"8",                       SERVICE_APPLICATIONS},
    {"9",                       SERVICE_FOLDERS},
    {"10",                      SERVICE_MUSIC},
    {"11",                      SERVICE_DOCUMENTS}, /* PDF */
    {"12",                      -1},                /* Presentation */
    {"13",                      SERVICE_IMAGES},
    {"public.jpeg",             SERVICE_IMAGES},
    {"public.tiff",             SERVICE_IMAGES},
    {"com.compuserve.gif",      SERVICE_IMAGES},
    {"public.png",              SERVICE_IMAGES},
    {"com.microsoft.bmp",       SERVICE_IMAGES},
    {"public.content",          SERVICE_DOCUMENTS},
    {"public.mp3",              SERVICE_VIDEOS},
    {"public.mpeg-4-audio",     -1},
    {"com.apple.application",   SERVICE_APPLICATIONS},
    {"public.text",             SERVICE_TEXT_FILES},
    {"public.plain-text",       SERVICE_TEXT_FILES},
    {"public.rtf",              SERVICE_TEXT_FILES},
    {"public.html",             -1},
    {"public.xml",              -1},
    {"public.source-code",      SERVICE_DEVELOPMENT_FILES},
    {NULL,                      -1}
};
