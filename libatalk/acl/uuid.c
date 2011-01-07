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
#include <inttypes.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>

#include <atalk/logger.h>
#include <atalk/afp.h>
#include <atalk/uuid.h>
#include <atalk/util.h>

#include "aclldap.h"
#include "cache.h"

char *uuidtype[] = {"NULL","USER", "GROUP", "LOCAL"};

/********************************************************
 * Public helper function
 ********************************************************/

/* 
 * convert ascii string that can include dashes to binary uuid.
 * caller must provide a buffer.
 */
void uuid_string2bin( const char *uuidstring, unsigned char *uuid) {
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

/*! 
 * Convert 16 byte binary uuid to neat ascii represantation including dashes.
 * 
 * Returns pointer to static buffer.
 */
const char *uuid_bin2string(const unsigned char *uuid) {
    static char uuidstring[UUID_STRINGSIZE + 1];

    int i = 0;
    unsigned char c;

    while (i < UUID_STRINGSIZE) {
        c = *uuid;
        uuid++;
        sprintf(uuidstring + i, "%02X", c);
        i += 2;
        if (i==8 || i==13 || i==18 || i==23)
            uuidstring[i++] = '-';
    }
    uuidstring[i] = 0;
    return uuidstring;
}

/********************************************************
 * Interface
 ********************************************************/

static unsigned char local_group_uuid[] = {0xab, 0xcd, 0xef,
                                           0xab, 0xcd, 0xef,
                                           0xab, 0xcd, 0xef, 
                                           0xab, 0xcd, 0xef};

static unsigned char local_user_uuid[] = {0xff, 0xff, 0xee, 0xee, 0xdd, 0xdd,
                                          0xcc, 0xcc, 0xbb, 0xbb, 0xaa, 0xaa};

/*
 *   name: give me his name
 *   type: and type (UUID_USER or UUID_GROUP)
 *   uuid: pointer to uuid_t storage that the caller must provide
 * returns 0 on success !=0 on errror
 */  
int getuuidfromname( const char *name, uuidtype_t type, unsigned char *uuid) {
    int ret = 0;
#ifdef HAVE_LDAP
    char *uuid_string = NULL;
#endif
    ret = search_cachebyname( name, type, uuid);
    if (ret == 0) {
        /* found in cache */
        LOG(log_debug, logtype_afpd, "getuuidfromname{cache}: name: %s, type: %s -> UUID: %s",
            name, uuidtype[type], uuid_bin2string(uuid));
    } else  {
        /* if not found in cache */
#ifdef HAVE_LDAP
        if ((ret = ldap_getuuidfromname( name, type, &uuid_string)) == 0) {
            uuid_string2bin( uuid_string, uuid);
            LOG(log_debug, logtype_afpd, "getuuidfromname{local}: name: %s, type: %s -> UUID: %s",
                name, uuidtype[type], uuid_bin2string(uuid));
        } else {
            LOG(log_debug, logtype_afpd, "getuuidfromname(\"%s\",t:%u): no result from ldap search",
                name, type);
        }
#endif
        if (ret != 0) {
            /* Build a local UUID */
            if (type == UUID_USER) {
                memcpy(uuid, local_user_uuid, 12);
                struct passwd *pwd;
                if ((pwd = getpwnam(name)) == NULL) {
                    LOG(log_error, logtype_afpd, "getuuidfromname(\"%s\",t:%u): unknown user",
                        name, uuidtype[type]);
                    goto cleanup;
                }
                uint32_t id = pwd->pw_uid;
                id = htonl(id);
                memcpy(uuid + 12, &id, 4);
            } else {
                memcpy(uuid, &local_group_uuid, 12);
                struct group *grp;
                if ((grp = getgrnam(name)) == NULL) {
                    LOG(log_error, logtype_afpd, "getuuidfromname(\"%s\",t:%u): unknown user",
                        name, uuidtype[type]);
                    goto cleanup;
                }
                uint32_t id = grp->gr_gid;
                id = htonl(id);
                memcpy(uuid + 12, &id, 4);
            }
            LOG(log_debug, logtype_afpd, "getuuidfromname{local}: name: %s, type: %s -> UUID: %s",
                name, uuidtype[type], uuid_bin2string(uuid));
        }
        ret = 0;
        add_cachebyname( name, uuid, type, 0);
    }

cleanup:
#ifdef HAVE_LDAP
    if (uuid_string) free(uuid_string);
#endif
    return ret;
}


/*
 * uuidp: pointer to a uuid
 * name: returns allocated buffer from ldap_getnamefromuuid
 * type: returns USER, GROUP or LOCAL
 * return 0 on success !=0 on errror
 *
 * Caller must free name appropiately.
 */
int getnamefromuuid(uuidp_t uuidp, char **name, uuidtype_t *type) {
    int ret;

    ret = search_cachebyuuid( uuidp, name, type);
    if (ret == 0) {
        /* found in cache */
        LOG(log_debug9, logtype_afpd, "getnamefromuuid{cache}: UUID: %s -> name: %s, type:%s",
            uuid_bin2string(uuidp), *name, uuidtype[*type]);
    } else {
        /* not found in cache */

        /* Check if UUID is a client local one */
        if (memcmp(uuidp, local_user_uuid, 12) == 0
            || memcmp(uuidp, local_group_uuid, 12) == 0) {
            LOG(log_debug, logtype_afpd, "getnamefromuuid: local UUID: %" PRIu32 "",
                ntohl(*(uint32_t *)(uuidp + 12)));
            *type = UUID_LOCAL;
            *name = strdup("UUID_LOCAL");
            return 0;
        }

#ifdef HAVE_LDAP
        ret = ldap_getnamefromuuid(uuid_bin2string(uuidp), name, type);
        if (ret != 0) {
            LOG(log_warning, logtype_afpd, "getnamefromuuid(%s): no result from ldap_getnamefromuuid",
                uuid_bin2string(uuidp));
            goto cleanup;
        }
        add_cachebyuuid( uuidp, *name, *type, 0);
        LOG(log_debug, logtype_afpd, "getnamefromuuid{LDAP}: UUID: %s -> name: %s, type:%s",
            uuid_bin2string(uuidp), *name, uuidtype[*type]);
#endif
    }

cleanup:
    return ret;
}
