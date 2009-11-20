/*
   $Id: uuidtest.c,v 1.3 2009-11-20 17:45:46 franklahm Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ldap.h>

#include <atalk/ldapconfig.h>
#include <atalk/uuid.h>

#define STRNCMP(a, R, b, l) (strncmp(a,b,l) R 0)

int main( int argc, char **argv)
{
    int ret, i;
    uuid_t uuid;
    uuidtype_t type;
    char *uuidstring = NULL;
    char *name = NULL;

    if (argc <  2) {
	printf("Usage: uuidtest <name> | -u<UUID> [<name> ... | -u<UUID> ...] \n");
	return -1;
    }
    
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
	    printf("afp_ldap.conf want SASL which is not yet supported.\n");	
	}
    } else {
	printf("afp_ldap.conf is not ok.\n");
	return 1;
    }

    for (i=1; i+1 <= argc; i++) {
	if (STRNCMP("-u", == , argv[i], 2)) {
	    printf("Searching uuid: %s\n", argv[i]+2);
	    uuid_string2bin(argv[i]+2, uuid);
            ret = getnamefromuuid( uuid, &name, &type);
            if (ret == 0) {
                printf("Got user: %s for uuid: %s, type:%d\n", name, argv[i]+2, type);
                free(uuidstring);
            } else
                printf("uuid %s not found.\n", argv[i]+2);
	} else {
	    printf("Searching user: %s\n", argv[i]);
	    ret = getuuidfromname( argv[i], UUID_USER, uuid);
	    if (ret == 0) {
		uuid_bin2string( uuid, &uuidstring);
		printf("Got uuid: %s for name: %s, type: USER\n", uuidstring, argv[i]);
		free(uuidstring);
	    } else {
		ret = getuuidfromname( argv[i], UUID_GROUP, uuid);
		if (ret == 0) {
		    uuid_bin2string( uuid, &uuidstring);
		    printf("Got uuid: %s for name: %s, type: GROUP\n", uuidstring, argv[i]);
		    free(uuidstring);
		}
		else
		    printf("User %s not found.\n", argv[i]);
	    }
	}
    }

    return 0;
}

#endif	/* HAVE_NFSv4_ACLS */
