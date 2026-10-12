---
type: C Source File
title: "libatalk/acl/cache.c"
description: "Name and UUID caches of the ACL UUID lookups."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/acl/cache.c"
tags: ["libatalk/acl"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [libatalk/acl](../acl.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/afp.h](../../include/atalk/afp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/uuid.h](../../include/atalk/uuid.h.md)
* [cache.h](cache.h.md)
* System headers: `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `time.h`

# Functions

### uuidcache_dump

```c
void uuidcache_dump(void)
```

Defined at lines 59 to 115.

Log every entry of the name and UUID caches at debug level.

Each line shows the name, the UUID, the type, a negative marker, and the time the entry entered the cache.

Calls: [uuid_bin2string](uuid.c.md#uuid_bin2string)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `namecache`, `uuidcache`, `uuidtype` in [libatalk/acl/uuid.c](uuid.c.md)

### hashstring

```c
static unsigned char hashstring(unsigned char *str)
```

Defined at lines 125 to 139.

Hash a string into a cache bucket with 32-bit FNV-1a.

The low byte of the hash, XORed with its second byte, picks the bucket.

Parameters:
* `str`: NUL-terminated string

Returns: the bucket, 0 to 255

Called by: [add_cachebyname](cache.c.md#add_cachebyname), [search_cachebyname](cache.c.md#search_cachebyname)

### hashuuid

```c
static unsigned char hashuuid(uuidp_t uuid)
```

Defined at lines 147 to 158.

Hash a 16-byte UUID into a cache bucket.

Parameters:
* `uuid`: binary UUID

Returns: the bucket, 0 to 255

Called by: [add_cachebyuuid](cache.c.md#add_cachebyuuid), [search_cachebyuuid](cache.c.md#search_cachebyuuid)

### add_cachebyname

```c
int add_cachebyname(const char *inname, const uuidp_t inuuid, const uuidtype_t type, const unsigned long uid)
```

Defined at lines 177 to 257.

Add a name to UUID mapping at the head of its name cache bucket.

The call duplicates the name and the UUID and records the current time. A lookup drops an entry after CACHESECONDS seconds.

Parameters:
* `inname`: user or group name
* `inuuid`: UUID of the name
* `type`: UUID_USER or UUID_GROUP, with UUID_ENOENT for a negative entry
* `uid`: unused

Returns: 0, or -1 for a NULL argument or a failed allocation

Calls: [hashstring](cache.c.md#hashstring)

Called by: [getuuidfromname](uuid.c.md#getuuidfromname)

Uses file-scope variables: `namecache`

### search_cachebyname

```c
int search_cachebyname(const char *name, uuidtype_t *type, unsigned char *uuid)
```

Defined at lines 269 to 326.

Search cache by name and uuid type.

Parameters:
* `name`: name to search
* `type`: type (user or group) of name, returns found type here which might mark it as a negative entry
* `uuid`: found uuid is returned here

Returns: 0 on success, entry found; -1 no entry found

Calls: [hashstring](cache.c.md#hashstring)

Called by: [getuuidfromname](uuid.c.md#getuuidfromname)

Uses file-scope variables: `namecache`

### search_cachebyuuid

```c
int search_cachebyuuid(uuidp_t uuidp, char **name, uuidtype_t *type)
```

Defined at lines 340 to 396.

Look up a UUID in the UUID cache.

An entry older than CACHESECONDS seconds leaves the cache, and the lookup fails. The caller frees the returned name.

Parameters:
* `uuidp`: binary UUID
* `name`: receives a newly allocated name
* `type`: receives the type of the entry, with UUID_ENOENT for a negative one

Returns: 0, or -1 when the UUID has no current entry

Calls: [hashuuid](cache.c.md#hashuuid)

Called by: [getnamefromuuid](uuid.c.md#getnamefromuuid)

Uses file-scope variables: `uuidcache`

### add_cachebyuuid

```c
int add_cachebyuuid(uuidp_t inuuid, const char *inname, uuidtype_t type, const unsigned long uid)
```

Defined at lines 411 to 483.

Add a UUID to name mapping at the head of its UUID cache bucket.

The call duplicates the name and the UUID and records the current time. A lookup drops an entry after CACHESECONDS seconds.

Parameters:
* `inuuid`: binary UUID
* `inname`: user or group name of the UUID
* `type`: UUID_USER or UUID_GROUP, with UUID_ENOENT for a negative entry
* `uid`: unused

Returns: 0, or -1 when an allocation fails

Calls: [hashuuid](cache.c.md#hashuuid)

Called by: [getnamefromuuid](uuid.c.md#getnamefromuuid)

Uses file-scope variables: `uuidcache`

# Types

### struct cacheduser

Defined at line 36.
* `unsigned long uid`: for future use
* `uuidtype_t type`
* `unsigned char * uuid`
* `char * name`
* `time_t creationtime`
* `struct cacheduser * prev`
* `struct cacheduser * next`

# Typedefs and enums

* `typedef struct cacheduser cacheduser_t`

# File-scope variables

`namecache`, `uuidcache`
