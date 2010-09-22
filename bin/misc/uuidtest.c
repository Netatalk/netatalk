/*
  $Id: uuidtest.c,v 1.3 2009-11-28 12:27:24 franklahm Exp $
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

#ifdef HAVE_NFSv4_ACLS

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ldap.h>

#include <atalk/ldapconfig.h>
#include <atalk/uuid.h>
#include <atalk/logger.h>

#define STRNCMP(a, R, b, l) (strncmp(a,b,l) R 0)

static void usage()
{
    printf("Usage: afpldaptest -u <user> | -g <group> | -i <UUID>\n");
}

static void parse_ldapconf()
{
    static int inited = 0;

    if (! inited) {
        /* Parse afp_ldap.conf */
        printf("Start parsing afp_ldap.conf\n");
        acl_ldap_readconfig(_PATH_ACL_LDAPCONF);
        printf("Finished parsing afp_ldap.conf\n");
        if (ldap_config_valid) {
            if (ldap_auth_method == LDAP_AUTH_NONE)
                printf("afp_ldap.conf is ok. Using anonymous bind.\n");
            else if (ldap_auth_method == LDAP_AUTH_SIMPLE)
                printf("afp_ldap.conf is ok. Using simple bind.\n");
            else {
                ldap_config_valid = 0;
                printf("afp_ldap.conf wants SASL which is not yet supported.\n");
                exit(EXIT_FAILURE);
            }
        } else {
            printf("afp_ldap.conf is not ok.\n");
            exit(EXIT_FAILURE);
        }
        inited = 1;
    }
}

int main( int argc, char **argv)
{
    int ret, i, c;
    int verbose = 0;
    int logsetup = 0;
    uuid_t uuid;
    uuidtype_t type;
    char *uuidstring = NULL;
    char *name = NULL;

    while ((c = getopt(argc, argv, ":vu:g:i:")) != -1) {
        switch(c) {

        case 'v':
            if (! verbose) {
                verbose = 1;
                setuplog("default log_maxdebug /dev/tty");
                logsetup = 1;
            }
            break;

        case 'u':
            if (! logsetup)
                setuplog("default log_info /dev/tty");
            parse_ldapconf();
            printf("Searching user: %s\n", optarg);
            ret = getuuidfromname( optarg, UUID_USER, uuid);
            if (ret == 0) {
                uuid_bin2string( uuid, &uuidstring);
                printf("User: %s ==> UUID: %s\n", optarg, uuidstring);
                free(uuidstring);
            } else {
                printf("User %s not found.\n", optarg);
            }
            break;

        case 'g':
            if (! logsetup)
                setuplog("default log_info /dev/tty");
            parse_ldapconf();
            printf("Searching group: %s\n", optarg);
            ret = getuuidfromname( optarg, UUID_GROUP, uuid);
            if (ret == 0) {
                uuid_bin2string( uuid, &uuidstring);
                printf("Group: %s ==> UUID: %s\n", optarg, uuidstring);
                free(uuidstring);
            } else {
                printf("Group %s not found.\n", optarg);
            }
            break;

        case 'i':
            if (! logsetup)
                setuplog("default log_info /dev/tty");
            parse_ldapconf();
            printf("Searching uuid: %s\n", optarg);
            uuid_string2bin(optarg, uuid);
            ret = getnamefromuuid( uuid, &name, &type);
            if (ret == 0) {
                if (type == UUID_USER)
                    printf("UUID: %s ==> User: %s\n", optarg, name);
                else
                    printf("UUID: %s ==> Group: %s\n", optarg, name);
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

#endif  /* HAVE_NFSv4_ACLS */
