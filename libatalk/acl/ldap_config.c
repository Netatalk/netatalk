/*
  Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

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

#ifdef HAVE_LDAP

#include <ctype.h>
#include <errno.h>
#ifdef HAVE_INIPARSER_INIPARSER_H
#include <iniparser/iniparser.h>
#else
#include <iniparser.h>
#endif
#include <ldap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/globals.h>
#include <atalk/iniparser_util.h>
#include <atalk/ldapconfig.h>
#include <atalk/logger.h>

/*!
 * @brief Free the string LDAP preferences and restore the default validity of
 * every preference
 */
void acl_ldap_freeconfig(void)
{
    int i = 0;

    for (i = 0; ldap_prefs[i].name != NULL; i++) {
        if (ldap_prefs[i].intfromarray == 0 && ldap_prefs[i].strorint == 0) {
            free(*((char **)(ldap_prefs[i].pref)));
            *((char **)(ldap_prefs[i].pref)) = NULL;
        }

        ldap_prefs[i].valid = ldap_prefs[i].valid_save;
    }
}

/*!
 * @brief Read the LDAP preferences from the global section of afp.conf
 *
 * Each preference takes a string, an integer, or one of its mapped names. The
 * LDAP setup counts as valid only with every required preference set and an
 * authentication method of LDAP_AUTH_NONE or LDAP_AUTH_SIMPLE.
 *
 * @param[in] iniconfig  parsed afp.conf
 * @returns 0, with ldap_config_valid set when the setup is usable
 */
int acl_ldap_readconfig(dictionary *iniconfig)
{
    int i, j;
    const char *val;
    i = 0;

    /* now see if its a correct pref */
    for (i = 0; ldap_prefs[i].name != NULL; i++) {
        char option[MAXOPTLEN];
        snprintf(option, sizeof(option), "%s:%s", INISEC_GLOBAL, ldap_prefs[i].name);

        if ((val = iniparser_getstring(iniconfig, option, NULL))) {
            /* check if we have pre-defined values */
            if (ldap_prefs[i].intfromarray == 0) {
                /* no, its just a string */
                ldap_prefs[i].valid = 0;

                if (ldap_prefs[i].strorint)
                    /* store as int */
                {
                    *((int *)(ldap_prefs[i].pref)) = atoi(val);
                } else
                    /* store string as string */
                {
                    *((const char **)(ldap_prefs[i].pref)) = strdup(val);
                }
            } else {
                /* ok, we have string to int mapping for this pref */
                for (j = 0; prefs_array[j].pref != NULL; j++) {
                    if ((strcmp(prefs_array[j].pref, ldap_prefs[i].name) == 0)
                            && (strcmp(prefs_array[j].valuestring, val) == 0)) {
                        ldap_prefs[i].valid = 0;
                        *((int *)(ldap_prefs[i].pref)) = prefs_array[j].value;
                        break;
                    }
                }
            }
        }
    }

    /* check if the config is sane and complete */
    i = 0;
    ldap_config_valid = 1;

    while (ldap_prefs[i].pref != NULL) {
        if (ldap_prefs[i].valid != 0) {
            LOG(log_debug, logtype_afpd, "LDAP: Missing option: \"%s\"",
                ldap_prefs[i].name);
            ldap_config_valid = 0;
            break;
        }

        i++;
    }

    if (ldap_config_valid) {
        if (ldap_auth_method == LDAP_AUTH_NONE) {
            LOG(log_debug, logtype_afpd, "LDAP: Using anonymous bind.");
        } else if (ldap_auth_method == LDAP_AUTH_SIMPLE) {
            LOG(log_debug, logtype_afpd, "LDAP: Using simple bind.");
        } else {
            ldap_config_valid = 0;
            LOG(log_error, logtype_afpd, "LDAP: Unsupported authentication method.");
        }
    } else {
        LOG(log_info, logtype_afpd, "LDAP: not used");
    }

    return 0;
}
#endif /* HAVE_LDAP */
