---
type: C Header File
title: "etc/afpd/hash.h"
description: "7 functions, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/hash.h"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/hash.h](../../include/atalk/hash.h.md)
* System headers: `limits.h`

# Included by

* [etc/afpd/dircache.c](dircache.c.md)
* [etc/afpd/directory.c](directory.c.md)
* [etc/afpd/hash.c](hash.c.md)
* [etc/afpd/volume.c](volume.c.md)

# Functions

### hash_free

```c
void hash_free(hash_t *)
```

Declared at etc/afpd/hash.h line 31; no definition in the scanned sources.

### hash_init

```c
hash_t * hash_init(hash_t *, hashcount_t, hash_comp_t, hash_fun_t, hnode_t **, hashcount_t)
```

Declared at etc/afpd/hash.h line 32; no definition in the scanned sources.

Mentioned in the documentation of: [hash_scan_next](hash.c.md#hash_scan_next)

### hnode_put

```c
void hnode_put(hnode_t *, void *)
```

Declared at etc/afpd/hash.h line 41; no definition in the scanned sources.

### hash_isfull

```c
int hash_isfull(hash_t *)
```

Declared at etc/afpd/hash.h line 47; no definition in the scanned sources.

### hash_isempty

```c
int hash_isempty(hash_t *)
```

Declared at etc/afpd/hash.h line 48; no definition in the scanned sources.

Called by: [hash_destroy](hash.c.md#hash_destroy)

### hnode_create

```c
hnode_t * hnode_create(void *)
```

Declared at etc/afpd/hash.h line 57; no definition in the scanned sources.

### hnode_destroy

```c
void hnode_destroy(hnode_t *)
```

Declared at etc/afpd/hash.h line 59; no definition in the scanned sources.

# Macros

* Undocumented: `hash_count`, `hash_isempty`, `hash_isfull`, `hash_size`, `hnode_get`, `hnode_getkey`, `hnode_put`
