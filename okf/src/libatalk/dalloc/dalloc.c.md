---
type: C Source File
title: "libatalk/dalloc/dalloc.c"
description: "Typesafe, dynamic object store based on talloc."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dalloc/dalloc.c"
tags: ["libatalk/dalloc"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [libatalk/dalloc](../dalloc.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/dalloc.h](../../include/atalk/dalloc.h.md)
* [atalk/errchk.h](../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `inttypes.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `strings.h`, `talloc.h`

# Functions

### dalloc_add_talloc_chunk

```c
int dalloc_add_talloc_chunk(DALLOC_CTX *dd, void *talloc_chunk, void *obj, size_t size)
```

Defined at lines 181 to 203.

Append an element to a dalloc array, the worker of dalloc_add and dalloc_add_copy.

Use dalloc_add_copy() macro, not this function.

With a talloc chunk the call stores the bytes of obj in that chunk, as dalloc_add_copy needs. Without one it stores obj itself, as dalloc_add needs. The dalloc_add and dalloc_add_copy macros call it.

Parameters:
* `dd`: dalloc array
* `talloc_chunk`: chunk that receives the bytes of obj, or NULL
* `obj`: object to store
* `size`: size of obj

Returns: 0

### dalloc_get

```c
void * dalloc_get(const DALLOC_CTX *d,...)
```

Defined at lines 218 to 262.

Return an element of a dalloc tree by a path of indexes and a type name.

Each "DALLOC_CTX" argument with an index steps into a nested array. The last type name and index select the element, which must carry that talloc name.

Parameters:
* `d`: dalloc tree
* `...`: "DALLOC_CTX" and index pairs, then the type name and the index of the element

Returns: the element, or NULL for an index out of range or a type mismatch

Called by: [afp_spotlight_rpc](../../etc/afpd/spotlight.c.md#afp_spotlight_rpc), [sl_rpc_closeQueryForContext](../../etc/afpd/spotlight.c.md#sl_rpc_closequeryforcontext), [sl_rpc_fetchAttributeNamesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_fetchattributenamesforoidarray), [sl_rpc_fetchAttributesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_rpc_fetchQueryResultsForContext](../../etc/afpd/spotlight.c.md#sl_rpc_fetchqueryresultsforcontext), [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery), [sl_rpc_storeAttributesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_storeattributesforoidarray)

### dalloc_value_for_key

```c
void * dalloc_value_for_key(const DALLOC_CTX *d,...)
```

Defined at lines 278 to 326.

Return the value for a name in a dalloc dictionary, reached by a path of indexes.

Each "DALLOC_CTX" argument with an index steps into a nested array. The dictionary alternates string names and values, and the value must carry the final type name.

Parameters:
* `d`: dalloc tree
* `...`: "DALLOC_CTX" and index pairs, then the name and the type name of the value

Returns: the value, or NULL for a missing name, a name that is not a string, or a type mismatch

Called by: [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery), [sl_rpc_storeAttributesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_storeattributesforoidarray)

### dalloc_strdup

```c
char * dalloc_strdup(const void *ctx, const char *string)
```

Defined at lines 337 to 354.

Duplicate a string in talloc memory under the "char *" type name.

The type name lets dalloc_get and dalloc_value_for_key return the string.

Parameters:
* `ctx`: talloc parent
* `string`: NUL-terminated string

Returns: the new string, or NULL when talloc fails

Called by: [add_filemeta](../../etc/afpd/spotlight.c.md#add_filemeta), [sl_rpc_fetchAttributeNamesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_fetchattributenamesforoidarray), [sl_rpc_fetchPropertiesForContext](../../etc/afpd/spotlight.c.md#sl_rpc_fetchpropertiesforcontext), [sl_sanitize_reqinfo](../../etc/afpd/spotlight.c.md#sl_sanitize_reqinfo)

### dalloc_strndup

```c
char * dalloc_strndup(const void *ctx, const char *string, size_t n)
```

Defined at lines 367 to 384.

Duplicate up to n bytes of a string in talloc memory under the "char *" type name.

The result always ends with a NUL.

Parameters:
* `ctx`: talloc parent
* `string`: string
* `n`: byte limit, without the NUL

Returns: the new string, or NULL when talloc fails

Called by: [sl_unpack_cpx](../../etc/afpd/spotlight_marshalling.c.md#sl_unpack_cpx)
