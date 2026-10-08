---
type: C Source File
title: "libatalk/acl/uuid.c"
description: "5 functions, includes 7 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/acl/uuid.c"
tags: ["libatalk/acl"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/acl](../acl.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [aclldap.h](aclldap.h.md)
* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/ldapconfig.h](../../include/atalk/ldapconfig.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [atalk/uuid.h](../../include/atalk/uuid.h.md)
* [cache.h](cache.h.md)
* System headers: `arpa/inet.h`, `errno.h`, `grp.h`, `inttypes.h`, `pwd.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/types.h`

# Functions

### localuuid_from_id

```c
void localuuid_from_id(unsigned char *buf, uuidtype_t type, unsigned int id)
```

Defined at lines 54 to 72.

Called by: [afp_getacl](../../etc/afpd/acls.c.md#afp_getacl), [getuuidfromname](uuid.c.md#getuuidfromname)

Uses file-scope variables: `local_group_uuid`, `local_user_uuid`

### uuid_string2bin

```c
void uuid_string2bin(const char *uuidstring, unsigned char *uuid)
```

Defined at lines 78 to 107.

convert ascii string that can include dashes to binary uuid.

Note: caller must provide a buffer.

Called by: [getuuidfromname](uuid.c.md#getuuidfromname), [main](../../bin/misc/uuidtest.c.md#main)

### uuid_bin2string

```c
const char * uuid_bin2string(const unsigned char *uuid)
```

Defined at lines 115 to 161.

Convert 16 byte binary uuid to neat ascii represantation including dashes.

Use defined or default ascii mask for dash placement

Returns: pointer to static buffer

Called by: [afp_getuserinfo](../../etc/afpd/auth.c.md#afp_getuserinfo), [get_vol_uuid](../util/netatalk_conf.c.md#get_vol_uuid), [getnamefromuuid](uuid.c.md#getnamefromuuid), [getuuidfromname](uuid.c.md#getuuidfromname), [main](../../bin/misc/uuidtest.c.md#main), [map_aces_darwin_to_posix](../../etc/afpd/acls.c.md#map_aces_darwin_to_posix), [uuidcache_dump](cache.c.md#uuidcache_dump)

Uses file-scope variables: `ldap_uuid_string` in [libatalk/acl/ldap.c](ldap.c.md)

### getuuidfromname

```c
int getuuidfromname(const char *name, uuidtype_t type, unsigned char *uuid)
```

Defined at lines 173 to 296.

Parameters:
* `name`: give me his name
* `type`: and type (UUID_USER or UUID_GROUP)
* `uuid`: pointer to uuid_t storage that the caller must provide

Returns: 0 on success !=0 on errror

Calls: [add_cachebyname](cache.c.md#add_cachebyname), [ldap_getuuidfromname](ldap.c.md#ldap_getuuidfromname), [localuuid_from_id](uuid.c.md#localuuid_from_id), [search_cachebyname](cache.c.md#search_cachebyname), [uuid_bin2string](uuid.c.md#uuid_bin2string), [uuid_string2bin](uuid.c.md#uuid_string2bin)

Called by: [afp_getacl](../../etc/afpd/acls.c.md#afp_getacl), [afp_getuserinfo](../../etc/afpd/auth.c.md#afp_getuserinfo), [afp_mapname](../../etc/afpd/directory.c.md#afp_mapname), [main](../../bin/misc/uuidtest.c.md#main), [map_aces_solaris_to_darwin](../../etc/afpd/acls.c.md#map_aces_solaris_to_darwin), [map_acl_posix_to_darwin](../../etc/afpd/acls.c.md#map_acl_posix_to_darwin)

Uses file-scope variables: `uuidtype`

### getnamefromuuid

```c
int getnamefromuuid(const uuidp_t uuidp, char **name, uuidtype_t *type)
```

Defined at lines 307 to 392.

Parameters:
* `uuidp`: pointer to a uuid
* `name`: returns allocated buffer from ldap_getnamefromuuid
* `type`: returns USER, GROUP or LOCAL

Returns: 0 on success !=0 on errror

Note: Caller must free name appropriately.

Calls: [add_cachebyuuid](cache.c.md#add_cachebyuuid), [ldap_getnamefromuuid](ldap.c.md#ldap_getnamefromuuid), [search_cachebyuuid](cache.c.md#search_cachebyuuid), [uuid_bin2string](uuid.c.md#uuid_bin2string)

Called by: [afp_mapid](../../etc/afpd/directory.c.md#afp_mapid), [main](../../bin/misc/uuidtest.c.md#main), [map_aces_darwin_to_posix](../../etc/afpd/acls.c.md#map_aces_darwin_to_posix), [map_aces_darwin_to_solaris](../../etc/afpd/acls.c.md#map_aces_darwin_to_solaris)

Uses file-scope variables: `local_group_uuid`, `local_user_uuid`, `uuidtype`

# File-scope variables

`local_group_uuid`, `local_user_uuid`, `uuidtype`
