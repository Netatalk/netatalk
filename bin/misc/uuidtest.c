/*
  Copyright (c) 2008,2009 Frank Lahm <franklahm@gmail.com>

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

#ifdef HAVE_INIPARSER_INIPARSER_H
#include <iniparser/iniparser.h>
#else
#include <iniparser.h>
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_LDAP
#define LDAP_DEPRECATED 1
#include <ldap.h>
#endif

#include <atalk/ldapconfig.h>
#include <atalk/logger.h>
#include <atalk/uuid.h>

#define STRNCMP(a, R, b, l) (strncmp(a,b,l) R 0)

/*!
 * @brief Print the usage of afpldaptest to stdout
 */
static void usage(void)
{
    printf("Usage: afpldaptest -u <user> | -g <group> | -i <UUID>\n");
}

/*!
 * @brief Read the LDAP settings of afp.conf on the first call and report
 * whether they are valid
 *
 * The call loads afp.conf from the configuration directory and passes it to
 * acl_ldap_readconfig. It prints whether the settings are valid and which
 * bind they use, and an unsupported authentication method exits with
 * EXIT_FAILURE. A build without LDAP only prints a notice.
 */
static void parse_ldapconf(void)
{
    static int inited = 0;
    dictionary *iniconfig _U_;

    if (! inited) {
#ifdef HAVE_LDAP
        /* Parse afp.conf */
        printf("Start parsing afp.conf\n");
        iniconfig = iniparser_load(_PATH_CONFDIR "afp.conf");
        acl_ldap_readconfig(iniconfig);
        printf("Finished parsing afp.conf\n");

        if (ldap_config_valid) {
            if (ldap_auth_method == LDAP_AUTH_NONE) {
                printf("afp.conf is ok. Using anonymous bind.\n");
            } else if (ldap_auth_method == LDAP_AUTH_SIMPLE) {
                printf("afp.conf is ok. Using simple bind.\n");
            } else {
                ldap_config_valid = 0;
                printf("afp.conf defines an unsupported LDAP authentication method.\n");
                exit(EXIT_FAILURE);
            }
        } else {
            printf("afp.conf is not ok, not using LDAP. Only local UUID testing available.\n");
        }

#else
        printf("Built without LDAP support, only local UUID testing available.\n");
#endif
        inited = 1;
    }
}

/*!
 * @brief Map user and group names to UUIDs and back through the netatalk UUID
 * layer
 *
 * The options run in order. The -u and -g options print the UUID of a user
 * or a group, and -i prints the name and the type behind a UUID. The first
 * lookup reads the LDAP settings of afp.conf. The lookups log to /dev/tty at
 * the info level, or at maxdebug after -v.
 *
 * @param[in] argc  argument count
 * @param[in] argv  argument vector
 * @returns 0, also when a lookup fails, while an unknown option or a missing
 *          argument exits with EXIT_FAILURE
 */
int main(int argc, char **argv)
{
    int ret, c;
    int verbose = 0;
    atalk_uuid_t uuid;
    int logsetup = 0;
    uuidtype_t type;
    char *name = NULL;

    while ((c = getopt(argc, argv, ":vu:g:i:")) != -1) {
        switch (c) {
        case 'v':
            if (! verbose) {
                verbose = 1;
                setuplog("default:maxdebug", "/dev/tty", true);
                logsetup = 1;
            }

            break;

        case 'u':
            if (! logsetup) {
                setuplog("default:info", "/dev/tty", true);
            }

            parse_ldapconf();
            printf("Searching user: %s\n", optarg);
            ret = getuuidfromname(optarg, UUID_USER, uuid);

            if (ret == 0) {
                printf("User: %s ==> UUID: %s\n", optarg, uuid_bin2string(uuid));
            } else {
                printf("User %s not found.\n", optarg);
            }

            break;

        case 'g':
            if (! logsetup) {
                setuplog("default:info", "/dev/tty", true);
            }

            parse_ldapconf();
            printf("Searching group: %s\n", optarg);
            ret = getuuidfromname(optarg, UUID_GROUP, uuid);

            if (ret == 0) {
                printf("Group: %s ==> UUID: %s\n", optarg, uuid_bin2string(uuid));
            } else {
                printf("Group %s not found.\n", optarg);
            }

            break;

        case 'i':
            if (! logsetup) {
                setuplog("default:info", "/dev/tty", true);
            }

            parse_ldapconf();
            printf("Searching uuid: %s\n", optarg);
            uuid_string2bin(optarg, uuid);
            ret = getnamefromuuid(uuid, &name, &type);

            if (ret == 0) {
                switch (type) {
                case UUID_USER:
                    printf("UUID: %s ==> User: %s\n", optarg, name);
                    break;

                case UUID_GROUP:
                    printf("UUID: %s ==> Group: %s\n", optarg, name);
                    break;

                default:
                    printf("???: %s\n", optarg);
                    break;
                }

                free(name);
            } else {
                printf("UUID: %s not found.\n", optarg);
            }

            break;

        case ':':
        case '?':
        case 'h':
            usage();
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
