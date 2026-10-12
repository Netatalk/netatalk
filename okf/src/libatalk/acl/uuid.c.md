---
type: C Source File
title: "libatalk/acl/uuid.c"
description: "UUID to name and name to UUID resolution for ACLs."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/acl/uuid.c"
tags: ["libatalk/acl"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 70 to 88.

Build a local UUID for a user or group without a directory service UUID.

A fixed 12-byte prefix for users or for groups precedes the ID in network order.

Parameters:
* `buf`: 16-byte destination
* `type`: UUID_GROUP for a group, any other type for a user
* `id`: uid or gid

Called by: [afp_getacl](../../etc/afpd/acls.c.md#afp_getacl), [getuuidfromname](uuid.c.md#getuuidfromname)

Uses file-scope variables: `local_group_uuid`, `local_user_uuid`

### uuid_string2bin

```c
void uuid_string2bin(const char *uuidstring, unsigned char *uuid)
```

Defined at lines 99 to 128.

Convert a hex UUID string, dashes allowed, into 16 bytes.

The call skips dashes and stops after 16 bytes. It does not validate the digits.

Parameters:
* `uuidstring`: hex string
* `uuid`: 16-byte destination

Called by: [getuuidfromname](uuid.c.md#getuuidfromname), [main](../../bin/misc/uuidtest.c.md#main)

### uuid_bin2string

```c
const char * uuid_bin2string(const unsigned char *uuid)
```

Defined at lines 140 to 186.

Format a 16-byte UUID as uppercase hex with dashes.

The ldap uuid string option can set the dash layout as a mask of x and - characters. A mask without exactly 32 x characters falls back to the standard 8-4-4-4-12 layout.

Parameters:
* `uuid`: binary UUID

Returns: the string in a static buffer

Called by: [afp_getuserinfo](../../etc/afpd/auth.c.md#afp_getuserinfo), [get_vol_uuid](../util/netatalk_conf.c.md#get_vol_uuid), [getnamefromuuid](uuid.c.md#getnamefromuuid), [getuuidfromname](uuid.c.md#getuuidfromname), [main](../../bin/misc/uuidtest.c.md#main), [map_aces_darwin_to_posix](../../etc/afpd/acls.c.md#map_aces_darwin_to_posix), [uuidcache_dump](cache.c.md#uuidcache_dump)

Uses file-scope variables: `ldap_uuid_string` in [libatalk/acl/ldap.c](ldap.c.md)

### getuuidfromname

```c
int getuuidfromname(const char *name, uuidtype_t type, unsigned char *uuid)
```

Defined at lines 204 to 327.

Resolve a user or group name to its UUID.

Looks in the UUID cache, then in LDAP when built with it, and finally builds a local UUID from the Unix uid or gid. A miss enters the cache as a negative entry.

Parameters:
* `name`: user or group name
* `type`: UUID_USER or UUID_GROUP
* `uuid`: storage of 16 bytes the caller provides

Returns: 0 on success, -1 when the name is NULL or unknown

Calls: [add_cachebyname](cache.c.md#add_cachebyname), [ldap_getuuidfromname](ldap.c.md#ldap_getuuidfromname), [localuuid_from_id](uuid.c.md#localuuid_from_id), [search_cachebyname](cache.c.md#search_cachebyname), [uuid_bin2string](uuid.c.md#uuid_bin2string), [uuid_string2bin](uuid.c.md#uuid_string2bin)

Called by: [afp_getacl](../../etc/afpd/acls.c.md#afp_getacl), [afp_getuserinfo](../../etc/afpd/auth.c.md#afp_getuserinfo), [afp_mapname](../../etc/afpd/directory.c.md#afp_mapname), [main](../../bin/misc/uuidtest.c.md#main), [map_aces_solaris_to_darwin](../../etc/afpd/acls.c.md#map_aces_solaris_to_darwin), [map_acl_posix_to_darwin](../../etc/afpd/acls.c.md#map_acl_posix_to_darwin)

Uses file-scope variables: `uuidtype`

### getnamefromuuid

```c
int getnamefromuuid(const uuidp_t uuidp, char **name, uuidtype_t *type)
```

Defined at lines 342 to 427.

Resolve a UUID to a user or group name and its type.

Looks in the UUID cache, then recognizes the server's local user and group UUIDs, and finally asks LDAP when built with it. A miss enters the cache as a negative entry.

Parameters:
* `uuidp`: UUID to resolve
* `name`: receives a newly allocated name that the caller frees
* `type`: receives UUID_USER or UUID_GROUP

Returns: 0 on success, -1 when the UUID is unknown

Calls: [add_cachebyuuid](cache.c.md#add_cachebyuuid), [ldap_getnamefromuuid](ldap.c.md#ldap_getnamefromuuid), [search_cachebyuuid](cache.c.md#search_cachebyuuid), [uuid_bin2string](uuid.c.md#uuid_bin2string)

Called by: [afp_mapid](../../etc/afpd/directory.c.md#afp_mapid), [main](../../bin/misc/uuidtest.c.md#main), [map_aces_darwin_to_posix](../../etc/afpd/acls.c.md#map_aces_darwin_to_posix), [map_aces_darwin_to_solaris](../../etc/afpd/acls.c.md#map_aces_darwin_to_solaris)

Uses file-scope variables: `local_group_uuid`, `local_user_uuid`, `uuidtype`

# File-scope variables

`local_group_uuid`, `local_user_uuid`, `uuidtype`
