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

#include <arpa/inet.h>
#include <errno.h>
#include <grp.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <atalk/afp.h>
#include <atalk/ldapconfig.h>
#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/uuid.h>

#include "aclldap.h"
#include "cache.h"

char *uuidtype[] = {"", "USER", "GROUP", "LOCAL"};

/********************************************************
 * Public helper function
 ********************************************************/

static unsigned char local_group_uuid[] = {0xab, 0xcd, 0xef,
                                           0xab, 0xcd, 0xef,
                                           0xab, 0xcd, 0xef,
                                           0xab, 0xcd, 0xef
                                          };

static unsigned char local_user_uuid[] = {0xff, 0xff, 0xee, 0xee, 0xdd, 0xdd,
                                          0xcc, 0xcc, 0xbb, 0xbb, 0xaa, 0xaa
                                         };

/*!
 * @brief Build a local UUID for a user or group without a directory service
 * UUID
 *
 * A fixed 12-byte prefix for users or for groups precedes the ID in network
 * order.
 *
 * @param[out] buf   16-byte destination
 * @param[in]  type  UUID_GROUP for a group, any other type for a user
 * @param[in]  id    uid or gid
 */
void localuuid_from_id(unsigned char *buf, uuidtype_t type, unsigned int id)
{
    uint32_t tmp;

    switch (type) {
    case UUID_GROUP:
        memcpy(buf, local_group_uuid, 12);
        break;

    case UUID_USER:
    default:
        memcpy(buf, local_user_uuid, 12);
        break;
    }

    tmp = htonl(id);
    memcpy(buf + 12, &tmp, 4);
    return;
}

/*!
 * @brief Convert a hex UUID string, dashes allowed, into 16 bytes
 *
 * The call skips dashes and stops after 16 bytes. It does not validate the
 * digits.
 *
 * @param[in]  uuidstring  hex string
 * @param[out] uuid        16-byte destination
 */
void uuid_string2bin(const char *uuidstring, unsigned char *uuid)
{
    int nibble = 1;
    int i = 0;
    unsigned char c, val = 0;

    while (*uuidstring && i < UUID_BINSIZE) {
        c = *uuidstring;

        if (c == '-') {
            uuidstring++;
            continue;
        } else if (c <= '9') {  /* 0-9 */
            c -= '0';
        } else if (c <= 'F') { /* A-F */
            c -= 'A' - 10;
        } else if (c <= 'f') {  /* a-f */
            c -= 'a' - 10;
        }

        if (nibble) {
            val = c * 16;
        } else {
            uuid[i++] = val + c;
        }

        nibble ^= 1;
        uuidstring++;
    }
}

/*!
 * @brief Format a 16-byte UUID as uppercase hex with dashes
 *
 * The ldap uuid string option can set the dash layout as a mask of x and -
 * characters. A mask without exactly 32 x characters falls back to the
 * standard 8-4-4-4-12 layout.
 *
 * @param[in] uuid  binary UUID
 * @returns the string in a static buffer
 */
const char *uuid_bin2string(const unsigned char *uuid)
{
    static char uuidstring[64];
    static const char default_mask[] = "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
    static const char hex[] = "0123456789ABCDEF";
    const char *uuidmask = default_mask;
    size_t i, nibbles = 0;
#ifdef HAVE_LDAP

    if (ldap_uuid_string) {
        uuidmask = ldap_uuid_string;
    }

#endif

    /* A mask must describe exactly 16 bytes and fit with its terminator.
     * Fall back to the standard representation for malformed configuration. */
    for (i = 0; i < sizeof(uuidstring) && uuidmask[i] != '\0'; i++) {
        if (uuidmask[i] == 'x') {
            nibbles++;
        } else if (uuidmask[i] != '-') {
            break;
        }
    }

    if (i == sizeof(uuidstring) || uuidmask[i] != '\0' ||
            nibbles != UUID_BINSIZE * 2) {
        LOG(log_error, logtype_afpd,
            "uuid_bin2string: invalid UUID mask; using default");
        uuidmask = default_mask;
    }

    LOG(log_debug, logtype_afpd, "uuid_bin2string{uuid}: mask: %s", uuidmask);

    for (i = 0, nibbles = 0; uuidmask[i] != '\0'; i++) {
        if (uuidmask[i] == '-') {
            uuidstring[i] = '-';
        } else {
            unsigned char c = uuid[nibbles / 2];
            uuidstring[i] = hex[(nibbles % 2 == 0) ? c >> 4 : c & 0x0f];
            nibbles++;
        }
    }

    uuidstring[i] = 0;
    return uuidstring;
}

/********************************************************
 * Interface
 ********************************************************/

/*!
 * @brief Resolve a user or group name to its UUID
 *
 * Looks in the UUID cache, then in LDAP when built with it, and finally builds
 * a local UUID from the Unix uid or gid. A miss enters the cache as a negative
 * entry.
 *
 * @param[in]  name  user or group name
 * @param[in]  type  UUID_USER or UUID_GROUP
 * @param[out] uuid  storage of 16 bytes the caller provides
 * @returns 0 on success, -1 when the name is NULL or unknown
 */
int getuuidfromname(const char *name, uuidtype_t type, unsigned char *uuid)
{
    int ret = 0;
    uuidtype_t mytype = type;
    char nulluuid[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    /* Local copy to prevent corruption */
    char name_copy[256];
    /* Save original pointer and value to detect if caller's buffer was corrupted */
    char original_check[256];
    const char *original_ptr = name;
#ifdef HAVE_LDAP
    char *uuid_string = NULL;
#endif

    if (name == NULL) {
        LOG(log_error, logtype_afpd, "getuuidfromname: name is NULL");
        return -1;
    }

    /* Copy name to local buffer to avoid pointer corruptions and improve security.
     * Protects against the non-reentrant nature of getpwuid()/getgrgid().
     * name pointer corrupted in race-condition for some ACL test cases */
    snprintf(name_copy, sizeof(name_copy), "%s", name);
    /* Save for corruption check */
    snprintf(original_check, sizeof(original_check), "%s", name);
    /* Use local copy from now on */
    name = name_copy;
    LOG(log_debug, logtype_afpd,
        "getuuidfromname: START - name='%s', type=%d", name, type);
    ret = search_cachebyname(name, &mytype, uuid);

    if (ret == 0) {
        /* found in cache */
        LOG(log_debug, logtype_afpd,
            "getuuidfromname{cache}: name: %s, type%s: %s -> UUID: %s",
            name,
            (mytype & UUID_ENOENT) == UUID_ENOENT ? "[negative]" : "",
            uuidtype[type & UUIDTYPESTR_MASK],
            uuid_bin2string(uuid));

        if ((mytype & UUID_ENOENT) == UUID_ENOENT) {
            LOG(log_debug, logtype_afpd,
                "getuuidfromname: returning -1 due to UUID_ENOENT for '%s'", name);
            return -1;
        }
    } else  {
        /* if not found in cache */
#ifdef HAVE_LDAP
        if ((ret = ldap_getuuidfromname(name, type, &uuid_string)) == 0) {
            uuid_string2bin(uuid_string, uuid);
            LOG(log_debug, logtype_afpd,
                "getuuidfromname{LDAP}: name: %s, type: %s -> UUID: %s",
                name, uuidtype[type & UUIDTYPESTR_MASK], uuid_bin2string(uuid));
        } else {
            LOG(log_debug, logtype_afpd,
                "getuuidfromname(\"%s\",t:%u): no result from ldap search",
                name, type);
        }

#endif

        if (ret != 0) {
            /* Build a local UUID */
            if (type == UUID_USER) {
                struct passwd *pwd;
                LOG(log_debug, logtype_afpd,
                    "getuuidfromname: calling getpwnam('%s')", name);

                if ((pwd = getpwnam(name)) == NULL) {
                    LOG(log_error, logtype_afpd,
                        "getuuidfromname(\"%s\",t:%u): unknown user (getpwnam returned NULL)",
                        name, uuidtype[type & UUIDTYPESTR_MASK]);
                    mytype |= UUID_ENOENT;
                    memcpy(uuid, nulluuid, 16);
                } else {
                    LOG(log_debug, logtype_afpd,
                        "getuuidfromname: getpwnam('%s') succeeded, uid=%d", name, pwd->pw_uid);
                    localuuid_from_id(uuid, UUID_USER, pwd->pw_uid);
                    ret = 0;
                    LOG(log_debug, logtype_afpd,
                        "getuuidfromname{local}: name: %s, type: %s -> UUID: %s",
                        name, uuidtype[type & UUIDTYPESTR_MASK], uuid_bin2string(uuid));
                }
            } else {
                struct group *grp;

                if ((grp = getgrnam(name)) == NULL) {
                    LOG(log_error, logtype_afpd, "getuuidfromname(\"%s\",t:%u): unknown user",
                        name, uuidtype[type & UUIDTYPESTR_MASK]);
                    mytype |= UUID_ENOENT;
                    memcpy(uuid, nulluuid, 16);
                } else {
                    localuuid_from_id(uuid, UUID_GROUP, grp->gr_gid);
                    ret = 0;
                    LOG(log_debug, logtype_afpd,
                        "getuuidfromname{local}: name: %s, type: %s -> UUID: %s",
                        name, uuidtype[type & UUIDTYPESTR_MASK], uuid_bin2string(uuid));
                }
            }
        }

        add_cachebyname(name, uuid, mytype, 0);
    }

#ifdef HAVE_LDAP

    if (uuid_string) {
        free(uuid_string);
    }

#endif

    /* Check if the original name pointer was corrupted during execution */
    if (original_ptr != NULL && strcmp(original_ptr, original_check) != 0) {
        LOG(log_error, logtype_afpd,
            "uuid.c:getuuidfromname: CORRUPTION DETECTED - caller's name buffer overwritten!"
            "Original='%s', Now='%s'. Error handled, but caller should be fixed.",
            original_check, original_ptr);
    }

    LOG(log_debug, logtype_afpd,
        "getuuidfromname: END - name='%s', ret=%d", name, ret);
    return ret;
}


/*!
 * @brief Resolve a UUID to a user or group name and its type
 *
 * Looks in the UUID cache, then recognizes the server's local user and group
 * UUIDs, and finally asks LDAP when built with it. A miss enters the cache as a
 * negative entry.
 *
 * @param[in]  uuidp  UUID to resolve
 * @param[out] name   receives a newly allocated name that the caller frees
 * @param[out] type   receives UUID_USER or UUID_GROUP
 * @returns 0 on success, -1 when the UUID is unknown
 */
int getnamefromuuid(const uuidp_t uuidp, char **name, uuidtype_t *type)
{
    int ret;
    uid_t uid;
    gid_t gid;
    uint32_t tmp;
    struct passwd *pwd;
    struct group *grp;

    if (search_cachebyuuid(uuidp, name, type) == 0) {
        /* found in cache */
        LOG(log_debug, logtype_afpd,
            "getnamefromuuid{cache}: UUID: %s -> name: %s, type%s: %s",
            uuid_bin2string(uuidp),
            *name,
            (*type & UUID_ENOENT) == UUID_ENOENT ? "[negative]" : "",
            uuidtype[(*type) & UUIDTYPESTR_MASK]);

        if ((*type & UUID_ENOENT) == UUID_ENOENT) {
            return -1;
        }

        return 0;
    }

    /* not found in cache */

    /* Check if UUID is a client local one */
    if (memcmp(uuidp, local_user_uuid, 12) == 0) {
        *type = UUID_USER;
        memcpy(&tmp, uuidp + 12, sizeof(uint32_t));
        uid = ntohl(tmp);

        if ((pwd = getpwuid(uid)) == NULL) {
            /* not found, add negative entry to cache */
            *name = NULL;
            add_cachebyuuid(uuidp, "UUID_ENOENT", UUID_ENOENT, 0);
            ret = -1;
        } else {
            *name = strdup(pwd->pw_name);
            add_cachebyuuid(uuidp, *name, *type, 0);
            ret = 0;
        }

        LOG(log_debug, logtype_afpd,
            "getnamefromuuid{local}: UUID: %s -> name: %s, type:%s",
            uuid_bin2string(uuidp), *name ? *name : "-",
            uuidtype[(*type) & UUIDTYPESTR_MASK]);
        return ret;
    } else if (memcmp(uuidp, local_group_uuid, 12) == 0) {
        *type = UUID_GROUP;
        memcpy(&tmp, uuidp + 12, sizeof(uint32_t));
        gid = ntohl(tmp);

        if ((grp = getgrgid(gid)) == NULL) {
            /* not found, add negative entry to cache */
            add_cachebyuuid(uuidp, "UUID_ENOENT", UUID_ENOENT, 0);
            ret = -1;
        } else {
            *name = strdup(grp->gr_name);
            add_cachebyuuid(uuidp, *name, *type, 0);
            ret = 0;
        }

        return ret;
    }

#ifdef HAVE_LDAP
    ret = ldap_getnamefromuuid(uuid_bin2string(uuidp), name, type);
#else
    ret = -1;
#endif

    if (ret != 0) {
        LOG(log_debug, logtype_afpd, "getnamefromuuid(%s): not found",
            uuid_bin2string(uuidp));
        add_cachebyuuid(uuidp, "UUID_ENOENT", UUID_ENOENT, 0);
        return -1;
    }

    add_cachebyuuid(uuidp, *name, *type, 0);
    LOG(log_debug, logtype_afpd,
        "getnamefromuuid{LDAP}: UUID: %s -> name: %s, type:%s",
        uuid_bin2string(uuidp), *name, uuidtype[(*type) & UUIDTYPESTR_MASK]);
    return 0;
}
