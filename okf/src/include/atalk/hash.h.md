---
type: C Header File
title: "include/atalk/hash.h"
description: "Shared hash table node, allocator and callback types."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/hash.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [include/atalk](../atalk.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* System headers: `limits.h`, `stdint.h`

# Included by

* [etc/afpd/hash.h](../../etc/afpd/hash.h.md)
* [include/atalk/directory.h](directory.h.md)
* [include/atalk/volume.h](volume.h.md)

# Types

### struct hash_t

This is the hash table control structure.

Defined at line 144.
* `struct hnode_t ** hash_table`: 1
* `hashcount_t hash_nchains`: 2
* `hashcount_t hash_nodecount`: 3
* `hashcount_t hash_maxcount`: 4
* `hashcount_t hash_highmark`: 5
* `hashcount_t hash_lowmark`: 6
* `hash_comp_t hash_compare`: 7
* `hash_fun_t hash_function`: 8
* `hnode_alloc_t hash_allocnode`
* `hnode_free_t hash_freenode`
* `void * hash_context`
* `hash_val_t hash_mask`: 9
* `int hash_dynamic`: 10

### struct hnode_t

Hash chain node structure.

Defined at line 68.
* `struct hnode_t * hash_next`
* `const void * hash_key`
* `void * hash_data`
* `hash_val_t hash_hkey`

### struct hscan_t

Hash scanner structure, used for traversals of the data structure.

Defined at line 174.
* `hash_t * hash_table`
* `hash_val_t hash_chain`
* `hnode_t * hash_next`

# Typedefs and enums

* `typedef int(* hash_comp_t`
* `typedef hash_val_t(* hash_fun_t`
* `typedef struct hash_t hash_t`
* `typedef uint32_t hash_val_t`
* `typedef unsigned long hashcount_t`
* `typedef hnode_t *(* hnode_alloc_t`
* `typedef void(* hnode_free_t`
* `typedef struct hnode_t hnode_t`
* `typedef struct hscan_t hscan_t`

# Macros

* Undocumented: `HASHCOUNT_T_MAX`, `HASH_VAL_T_BIT`, `HASH_VAL_T_MAX`
