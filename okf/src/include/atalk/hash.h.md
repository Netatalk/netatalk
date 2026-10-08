---
type: C Header File
title: "include/atalk/hash.h"
description: "3 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/include/atalk/hash.h"
tags: ["include/atalk"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at line 139.
* `struct hnode_t ** hash_table`
* `hashcount_t hash_nchains`
* `hashcount_t hash_nodecount`
* `hashcount_t hash_maxcount`
* `hashcount_t hash_highmark`
* `hashcount_t hash_lowmark`
* `hash_comp_t hash_compare`
* `hash_fun_t hash_function`
* `hnode_alloc_t hash_allocnode`
* `hnode_free_t hash_freenode`
* `void * hash_context`
* `hash_val_t hash_mask`
* `int hash_dynamic`

### struct hnode_t

Hash chain node structure.

Defined at line 63.
* `struct hnode_t * hash_next`
* `const void * hash_key`
* `void * hash_data`
* `hash_val_t hash_hkey`

### struct hscan_t

Defined at line 169.
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
