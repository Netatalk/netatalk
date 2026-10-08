---
type: C Source File
title: "libatalk/dalloc/dalloc.c"
description: "Typesafe, dynamic object store based on talloc."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/dalloc/dalloc.c"
tags: ["libatalk/dalloc"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 168 to 190.

Use dalloc_add_copy() macro, not this function

### dalloc_get

```c
void * dalloc_get(const DALLOC_CTX *d,...)
```

Defined at lines 199 to 243.

Get pointer to value from a DALLOC object.

Returns pointer to object from a DALLOC object. Nested object interation is supported by using the type string "DALLOC_CTX". Any other type string designates the requested objects type.

Called by: [afp_spotlight_rpc](../../etc/afpd/spotlight.c.md#afp_spotlight_rpc), [sl_rpc_closeQueryForContext](../../etc/afpd/spotlight.c.md#sl_rpc_closequeryforcontext), [sl_rpc_fetchAttributeNamesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_fetchattributenamesforoidarray), [sl_rpc_fetchAttributesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_fetchattributesforoidarray), [sl_rpc_fetchQueryResultsForContext](../../etc/afpd/spotlight.c.md#sl_rpc_fetchqueryresultsforcontext), [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery), [sl_rpc_storeAttributesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_storeattributesforoidarray)

### dalloc_value_for_key

```c
void * dalloc_value_for_key(const DALLOC_CTX *d,...)
```

Defined at lines 245 to 293.

Called by: [sl_rpc_openQuery](../../etc/afpd/spotlight.c.md#sl_rpc_openquery), [sl_rpc_storeAttributesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_storeattributesforoidarray)

### dalloc_strdup

```c
char * dalloc_strdup(const void *ctx, const char *string)
```

Defined at lines 295 to 312.

Called by: [add_filemeta](../../etc/afpd/spotlight.c.md#add_filemeta), [sl_rpc_fetchAttributeNamesForOIDArray](../../etc/afpd/spotlight.c.md#sl_rpc_fetchattributenamesforoidarray), [sl_rpc_fetchPropertiesForContext](../../etc/afpd/spotlight.c.md#sl_rpc_fetchpropertiesforcontext), [sl_sanitize_reqinfo](../../etc/afpd/spotlight.c.md#sl_sanitize_reqinfo)

### dalloc_strndup

```c
char * dalloc_strndup(const void *ctx, const char *string, size_t n)
```

Defined at lines 314 to 331.

Called by: [sl_unpack_cpx](../../etc/afpd/spotlight_marshalling.c.md#sl_unpack_cpx)
