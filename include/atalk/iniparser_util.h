/*
 * Copyright (C) 2024-2025 Daniel Markstedt
 * All Rights Reserved.  See COPYING.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef AFPD_INIPARSER_UTIL_H
#define AFPD_INIPARSER_UTIL_H 1

#ifdef HAVE_INIPARSER_INIPARSER_H
#include <iniparser/iniparser.h>
#else
#include <iniparser.h>
#endif

#ifdef HAVE_INIPARSER_CONST_DICTIONARY
#define INIPARSER_DICTIONARY const dictionary
#else
#define INIPARSER_DICTIONARY dictionary
#endif

/**********************************************************************************************
 * Ini config manipulation macros
 **********************************************************************************************/

#define INISEC_GLOBAL "global"
#define INISEC_HOMES  "homes"

#define INIPARSER_GETSTR(config, section, key, default) ({            \
    char _option[MAXOPTLEN];                                          \
    snprintf(_option, sizeof(_option), "%s:%s", section, key);        \
    iniparser_getstring(config, _option, default);                    \
})

#define INIPARSER_GETSTRDUP(config, section, key, default) ({         \
    char _option[MAXOPTLEN];                                          \
    snprintf(_option, sizeof(_option), "%s:%s", section, key);        \
    const char *_tmp = iniparser_getstring(config, _option, default); \
    _tmp ? strdup(_tmp) : NULL;                                       \
})

#define CONFIG_ARG_FREE(a) do {                     \
    free(a);                                        \
    a = NULL;                                       \
    } while (0);

#endif
