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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <atalk/logger.h>
#include <atalk/afp.h>
#include <atalk/uuid.h>
#include <atalk/util.h>

#include "aclldap.h"
#include "cache.h"

char *uuidtype[] = {"NULL","USER", "GROUP"};

/********************************************************
 * Public helper function
 ********************************************************/

/* 
 * convert ascii string that can include dashes to binary uuid.
 * caller must provide a buffer.
 */
void uuid_string2bin( const char *uuidstring, uuidp_t uuid) {
    int nibble = 1;
    int i = 0;
    unsigned char c, val = 0;

    while (*uuidstring) {
        c = *uuidstring;
        if (c == '-') {
            uuidstring++;
            continue;
        }
        else if (c <= '9')      /* 0-9 */
            c -= '0';
        else if (c <= 'F')  /* A-F */
            c -= 'A' - 10;
        else if (c <= 'f')      /* a-f */
            c-= 'a' - 10;

        if (nibble)
            val = c * 16;
        else
            uuid[i++] = val + c;

        nibble ^= 1;
        uuidstring++;
    }

}

/* 
 * convert 16 byte binary uuid to neat ascii represantation including dashes
 * string is allocated and pointer returned. caller must freee.
 */
int uuid_bin2string( uuidp_t uuid, char **uuidstring) {
    char ascii[16] = { "0123456789ABCDEF" };
    int nibble = 1;
    int i = 0;
    unsigned char c;
    char *s;

    *uuidstring = calloc(1, UUID_STRINGSIZE + 1);
    if (*uuidstring == NULL) {
        LOG(log_error, logtype_default, "uuid_bin2string: %s: error calloc'ing",strerror(errno));
        return -1;
    }
    s = *uuidstring;

    while (i < UUID_STRINGSIZE) {
        c = *uuid;
        if (nibble)
            c = c >> 4;
        else {
            c &= 0x0f;
            uuid++;
        }
        s[i] = ascii[c];
        nibble ^= 1;
        i++;
        if (i==8 || i==13 || i==18 || i==23)
            s[i++] = '-';
    }
    return 0;
}

/********************************************************
 * Interface
 ********************************************************/

/*
 *   name: give me his name
 *   type: and type (UUID_USER or UUID_GROUP)
 *   uuid: pointer to uuid_t storage that the caller must provide
 * returns 0 on success !=0 on errror
 */  
int getuuidfromname( const char *name, uuidtype_t type, uuidp_t uuid) {
    int ret = 0;
    char *uuid_string = NULL;

    ret = search_cachebyname( name, type, uuid);
    if (ret == 0) {     /* found in cache */
#ifdef DEBUG
        uuid_bin2string( uuid, &uuid_string);
        LOG(log_debug, logtype_afpd, "getuuidfromname{cache}: name: %s, type: %s -> UUID: %s",
            name, uuidtype[type], uuid_string);
#else
        LOG(log_debug, logtype_afpd, "getuuidfromname{cache}: name: %s, type: %s",
            name, uuidtype[type]);
#endif
    } else  {                   /* if not found in cache */
        ret = ldap_getuuidfromname( name, type, &uuid_string);
        if (ret != 0) {
            LOG(log_note, logtype_afpd, "getuuidfromname(\"%s\",t:%u): no result from ldap search",
                name, type);
            goto cleanup;
        }
        uuid_string2bin( uuid_string, uuid);
        add_cachebyname( name, uuid, type, 0);
        LOG(log_debug, logtype_afpd, "getuuidfromname{LDAP}: name: %s, type: %s -> UUID: %s",name, uuidtype[type], uuid_string);
    }

cleanup:
    free(uuid_string);
    return ret;
}

/* 
 * uuidp: pointer to a uuid
 * name: returns allocated buffer from ldap_getnamefromuuid
 * type: returns USER or GROUP
 * return 0 on success !=0 on errror
 *
 * Caller must free name appropiately.
 */
int getnamefromuuid(const uuidp_t uuidp, char **name, uuidtype_t *type) {
    int ret;
    char *uuid_string = NULL;

    ret = search_cachebyuuid( uuidp, name, type);
    if (ret == 0) {     /* found in cache */
#ifdef DEBUG
        uuid_bin2string( uuidp, &uuid_string);
        LOG(log_debug9, logtype_afpd, "getnamefromuuid{cache}: UUID: %s -> name: %s, type:%s",
            uuid_string, *name, uuidtype[*type]);
        free(uuid_string);
        uuid_string = NULL;
#endif
    } else  {                   /* if not found in cache */
        uuid_bin2string( uuidp, &uuid_string);
        ret = ldap_getnamefromuuid( uuid_string, name, type);
        if (ret != 0) {
            LOG(log_warning, logtype_afpd, "getnamefromuuid(%s): no result from ldap_getnamefromuuid",
                uuid_string);
            goto cleanup;
        }
        add_cachebyuuid( uuidp, *name, *type, 0);
        LOG(log_debug, logtype_afpd, "getnamefromuuid{LDAP}: UUID: %s -> name: %s, type:%s",
            uuid_string, *name, uuidtype[*type]);
    }

cleanup:
    free(uuid_string);
    return ret;
}
