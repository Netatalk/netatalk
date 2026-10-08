---
type: C Source File
title: "libatalk/acl/cache.c"
description: "7 functions, 1 type, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/acl/cache.c"
tags: ["libatalk/acl"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 48 to 104.

Calls: [uuid_bin2string](uuid.c.md#uuid_bin2string)

Called by: [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi)

Uses file-scope variables: `namecache`, `uuidcache`, `uuidtype` in [libatalk/acl/uuid.c](uuid.c.md)

### hashstring

```c
static unsigned char hashstring(unsigned char *str)
```

Defined at lines 107 to 121.

hash string it into unsigned char using FNV-1a algorithm

Called by: [add_cachebyname](cache.c.md#add_cachebyname), [search_cachebyname](cache.c.md#search_cachebyname)

### hashuuid

```c
static unsigned char hashuuid(uuidp_t uuid)
```

Defined at lines 124 to 135.

hash atalk_uuid_t into unsigned char

Called by: [add_cachebyuuid](cache.c.md#add_cachebyuuid), [search_cachebyuuid](cache.c.md#search_cachebyuuid)

### add_cachebyname

```c
int add_cachebyname(const char *inname, const uuidp_t inuuid, const uuidtype_t type, const unsigned long uid)
```

Defined at lines 141 to 221.

Calls: [hashstring](cache.c.md#hashstring)

Called by: [getuuidfromname](uuid.c.md#getuuidfromname)

Uses file-scope variables: `namecache`

### search_cachebyname

```c
int search_cachebyname(const char *name, uuidtype_t *type, unsigned char *uuid)
```

Defined at lines 233 to 290.

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

Defined at lines 295 to 351.

Caller must free allocated name

Calls: [hashuuid](cache.c.md#hashuuid)

Called by: [getnamefromuuid](uuid.c.md#getnamefromuuid)

Uses file-scope variables: `uuidcache`

### add_cachebyuuid

```c
int add_cachebyuuid(uuidp_t inuuid, const char *inname, uuidtype_t type, const unsigned long uid)
```

Defined at lines 353 to 425.

Calls: [hashuuid](cache.c.md#hashuuid)

Called by: [getnamefromuuid](uuid.c.md#getnamefromuuid)

Uses file-scope variables: `uuidcache`

# Types

### struct cacheduser

Defined at line 31.
* `unsigned long uid`
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
