---
type: C Source File
title: "etc/afpd/hash.c"
description: "Chained hash table for the directory cache."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/hash.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-22T19:27:15+11:00 }
---

Part of the [etc/afpd](../afpd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [hash.h](hash.h.md)
* System headers: `assert.h`, `stddef.h`, `stdlib.h`, `string.h`

# Functions

### compute_bits

```c
static void compute_bits(void)
```

Defined at lines 99 to 112.

Compute the number of bits in the hash_val_t type.

We know that hash_val_t is an unsigned integral type. Thus the highest value it can hold is a Mersenne number (power of two, less one). We initialize a hash_val_t object with this value and then shift bits out one by one while counting.

Note: 1. HASH_VAL_T_MAX is a Mersenne numberone that is one less than a power of two. This means that its binary representation consists of all one bits, and hence 'val' is initialized to all one bits.
1. While bits remain in val, we increment the bit count and shift it to the right, replacing the topmost bit by zero.

Called by: [hash_create](hash.c.md#hash_create)

Uses file-scope variables: `hash_val_t_bit`

### is_power_of_two

```c
static int is_power_of_two(hash_val_t arg)
```

Defined at lines 120 to 131.

Test whether a value is a power of two.

Parameters:
* `arg`: value to test

Returns: 1 for a power of two, 0 otherwise, including for 0

Called by: [hash_verify](hash.c.md#hash_verify)

### clear_table

```c
static void clear_table(hash_t *hash)
```

Defined at lines 138 to 145.

Set every chain head of a hash table to NULL.

Parameters:
* `hash`: hash table

Called by: [hash_create](hash.c.md#hash_create), [hash_free_nodes](hash.c.md#hash_free_nodes)

### grow_table

```c
static void grow_table(hash_t *hash)
```

Defined at lines 168 to 214.

Double the number of chains of a dynamic hash table.

Each chain splits into itself and a sister chain in the upper half. The mask widens by 1 bit. That bit of the stored hash value then picks the chain of each node, so no node needs a new hash. A failed realloc leaves the table as it was.

1. Assert that doubling the chain count does not overflow.
1. The new chain count is twice the old one.
1. The new mask is 1 bit wider and exposes 1 more bit of each hash value.
1. Reallocate the chain table at twice its size.
1. A failed realloc changes nothing.
1. The exposed_bit mask selects the new bit of each hash value.
1. Sort the nodes of each chain by that bit.
1. Nodes with the bit clear stay in the chain, and the others form the sister chain.
1. Update the mask, the chain count, and the marks.

Parameters:
* `hash`: dynamic hash table

Called by: [hash_insert](hash.c.md#hash_insert)

### shrink_table

```c
static void shrink_table(hash_t *hash)
```

Defined at lines 234 to 279.

Halve the number of chains of a dynamic hash table.

Each chain of the second half joins the end of its sister in the first half. The chain table then shrinks to its first half.

1. Assert at least 2 chains, so the low mark stays above 0.
1. Take the head of each chain of the first half and of its sister.
1. Find the tail of the first chain, NULL for an empty one.
1. Append the sister chain to a first chain that has nodes.
1. Move the sister chain into an empty slot.
1. Two empty chains need nothing.
1. Reallocate the chain table at half its size.
1. A failed realloc keeps the old allocation, which still works.
1. Update the mask, the chain count, and the marks.

Parameters:
* `hash`: dynamic hash table with at least 2 chains

Called by: [hash_delete](hash.c.md#hash_delete)

### hash_create

```c
hash_t * hash_create(hashcount_t, hash_comp_t, hash_fun_t)
```

Defined at lines 303 to 347. Declared in [etc/afpd/hash.h](hash.h.md).

Create a dynamic hash table.

The table starts with INIT_SIZE chains and doubles when the node count reaches twice the chain count. A deletion halves it at half the chain count, but never while the node count is at or below INIT_SIZE.

1. Count the bits of hash_val_t on the first call.
1. Allocate the control structure.
1. Return NULL when that allocation fails.
1. Allocate the chain table.
1. Free the control structure when the chain allocation fails.
1. Set the marks to twice and half the chain count.
1. Mark the table dynamic, so it resizes.
1. Clear the chain heads.

Parameters:
* `maxcount`: maximum number of nodes
* `compfun`: compare function, or NULL for strcmp
* `hashfun`: hash function, or NULL for the default string hash

Returns: the table, or NULL when an allocation fails

Calls: [clear_table](hash.c.md#clear_table), [compute_bits](hash.c.md#compute_bits), [hash_comp_default](hash.c.md#hash_comp_default), [hash_fun_default](hash.c.md#hash_fun_default), [hnode_alloc](hash.c.md#hnode_alloc), [hnode_free](hash.c.md#hnode_free)

Called by: [dircache_init](dircache.c.md#dircache_init)

Uses file-scope variables: `hash_val_t_bit`

### hash_set_allocator

```c
void hash_set_allocator(hash_t *, hnode_alloc_t, hnode_free_t, void *)
```

Defined at lines 359 to 367. Declared in [etc/afpd/hash.h](hash.h.md).

Replace the node allocator of an empty hash table.

Both functions must be set, or both NULL for the defaults.

Parameters:
* `hash`: empty hash table
* `al`: node allocator, or NULL
* `fr`: node release function, or NULL
* `context`: value passed to both functions

Calls: [hash_count](hash.c.md#hash_count), [hnode_alloc](hash.c.md#hnode_alloc), [hnode_free](hash.c.md#hnode_free)

### hash_free_nodes

```c
void hash_free_nodes(hash_t *)
```

Defined at lines 376 to 389. Declared in [etc/afpd/hash.h](hash.h.md).

Free every node of a hash table through its freenode function.

The table stays allocated and empty.

Parameters:
* `hash`: hash table

Calls: [clear_table](hash.c.md#clear_table), [hash_scan_begin](hash.c.md#hash_scan_begin), [hash_scan_delete](hash.c.md#hash_scan_delete), [hash_scan_next](hash.c.md#hash_scan_next)

### hash_destroy

```c
void hash_destroy(hash_t *)
```

Defined at lines 396 to 402. Declared in [etc/afpd/hash.h](hash.h.md).

Free an empty dynamic hash table and its chain table.

Parameters:
* `hash`: hash table without nodes

Calls: [hash_isempty](hash.h.md#hash_isempty)

Uses file-scope variables: `hash_val_t_bit`

### hash_scan_begin

```c
void hash_scan_begin(hscan_t *, hash_t *)
```

Defined at lines 415 to 433. Declared in [etc/afpd/hash.h](hash.h.md).

Start a scan of a hash table at its first node.

1. Find the first chain with a node.
1. Point the scan at the head of that chain.
1. An empty table leaves the scan with no next node, so hash_scan_next returns NULL.

Parameters:
* `scan`: scanner to start
* `hash`: hash table

Called by: [dircache_dump](dircache.c.md#dircache_dump), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [dircache_remove_children](dircache.c.md#dircache_remove_children), [hash_free_nodes](hash.c.md#hash_free_nodes)

### hash_scan_next

```c
hnode_t * hash_scan_next(hscan_t *)
```

Defined at lines 451 to 492. Declared in [etc/afpd/hash.h](hash.h.md).

Return the next node of a scan and advance the scan past it.

The scan moves on before the node returns, so the caller may unlink the node with hash_scan_delete.

1. Keep the node to return.
1. Assert that a table creation counted the bits of hash_val_t.
1. Advance only past a real node.
1. Take the next node of the same chain when there is one.
1. Otherwise look for the next chain with a node.
1. Take the head of that chain, or end the scan with NULL.

Parameters:
* `scan`: scanner from hash_scan_begin

Returns: the next node, or NULL at the end of the table

Called by: [dircache_dump](dircache.c.md#dircache_dump), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [dircache_remove_children](dircache.c.md#dircache_remove_children), [hash_free_nodes](hash.c.md#hash_free_nodes)

Uses file-scope variables: `hash_val_t_bit`

### hash_insert

```c
void hash_insert(hash_t *, hnode_t *, const void *)
```

Defined at lines 506 to 531. Declared in [etc/afpd/hash.h](hash.h.md).

Insert a node under a new `key`, ahead of the rest of its chain.

1. Assert that the table has room under maxcount.
1. Assert that `key` is not in the table yet.
1. Grow a dynamic table whose node count reached highmark.
1. The low bits of the hash value under the mask pick the chain.

Parameters:
* `hash`: hash table
* `node`: node with no chain link
* `key`: value to find the node by, kept by pointer

Calls: [grow_table](hash.c.md#grow_table), [hash_lookup](hash.c.md#hash_lookup)

Called by: [hash_alloc_insert_node](hash.c.md#hash_alloc_insert_node)

Uses file-scope variables: `hash_val_t_bit`

Mentioned in the documentation of: [arc_case_iv_make_room](dircache.c.md#arc_case_iv_make_room)

### hash_lookup

```c
hnode_t * hash_lookup(hash_t *, const void *)
```

Defined at lines 545 to 562. Declared in [etc/afpd/hash.h](hash.h.md).

Find the node of `key` in a hash table.

1. Hash `key` once and keep the full value.
1. The low bits of the hash value under the mask pick the chain.
1. Compare the stored hash values first, and call the compare function only on a match.

Parameters:
* `hash`: hash table
* `key`: value to find

Returns: the node, or NULL when no node matches

Called by: [dircache_add](dircache.c.md#dircache_add), [dircache_lookup_did](dircache.c.md#dircache_lookup_did), [dircache_reindex_didname](dircache.c.md#dircache_reindex_didname), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [hash_delete](hash.c.md#hash_delete), [hash_insert](hash.c.md#hash_insert), [hash_scan_delete](hash.c.md#hash_scan_delete), [process_cache_hints](dircache.c.md#process_cache_hints)

### hash_delete

```c
hnode_t * hash_delete(hash_t *, hnode_t *)
```

Defined at lines 582 to 619. Declared in [etc/afpd/hash.h](hash.h.md).

Unlink a node from a hash table.

The chains link one way, so the call walks the chain of the node to find its predecessor. A dynamic table halves first when the node count is at or below the low mark and above INIT_SIZE.

1. Assert that hash_lookup still finds the node.
1. Shrink the table before the unlink.
1. Find the chain of the node from its stored hash value.
1. Replace the chain head when the node is the head.
1. Otherwise unlink the node from its predecessor.
1. Clear the link of the node, so it belongs to no table.

Parameters:
* `hash`: hash table
* `node`: node in the table

Returns: node

Calls: [hash_lookup](hash.c.md#hash_lookup), [shrink_table](hash.c.md#shrink_table)

Called by: [hash_delete_free](hash.c.md#hash_delete_free)

Uses file-scope variables: `hash_val_t_bit`

### hash_alloc_insert

```c
int hash_alloc_insert(hash_t *, const void *, void *)
```

Defined at lines 629 to 632. Declared in [etc/afpd/hash.h](hash.h.md).

Allocate a node for `data` and insert it under `key`.

Parameters:
* `hash`: hash table
* `key`: value to find the node by, kept by pointer
* `data`: value of the node

Returns: 1, or 0 when the node allocation fails

Calls: [hash_alloc_insert_node](hash.c.md#hash_alloc_insert_node)

### hash_alloc_insert_node

```c
hnode_t * hash_alloc_insert_node(hash_t *, const void *, void *)
```

Defined at lines 644 to 654. Declared in [etc/afpd/hash.h](hash.h.md).

Allocate a node for `data`, insert it under `key`, and return it.

A caller that keeps the node can unlink it later without a lookup.

Parameters:
* `hash`: hash table
* `key`: value to find the node by, kept by pointer
* `data`: value of the node

Returns: the node, or NULL when the allocation fails

Calls: [hash_insert](hash.c.md#hash_insert), [hnode_init](hash.c.md#hnode_init)

Called by: [dircache_add](dircache.c.md#dircache_add), [dircache_reindex_didname](dircache.c.md#dircache_reindex_didname), [hash_alloc_insert](hash.c.md#hash_alloc_insert)

### hash_delete_free

```c
void hash_delete_free(hash_t *, hnode_t *)
```

Defined at lines 663 to 667. Declared in [etc/afpd/hash.h](hash.h.md).

Unlink a node through hash_delete and free it through the freenode function.

Parameters:
* `hash`: hash table
* `node`: node in the table

Calls: [hash_delete](hash.c.md#hash_delete)

Called by: [dircache_add](dircache.c.md#dircache_add), [dircache_remove](dircache.c.md#dircache_remove)

### hash_scan_delete

```c
hnode_t * hash_scan_delete(hash_t *, hnode_t *)
```

Defined at lines 679 to 702. Declared in [etc/afpd/hash.h](hash.h.md).

Unlink a node like hash_delete, but never shrink the table.

A scan may unlink the node that hash_scan_next just returned, because the chain table keeps its size.

Parameters:
* `hash`: hash table
* `node`: node in the table

Returns: node

Calls: [hash_lookup](hash.c.md#hash_lookup)

Called by: [hash_free_nodes](hash.c.md#hash_free_nodes), [hash_scan_delfree](hash.c.md#hash_scan_delfree)

Uses file-scope variables: `hash_val_t_bit`

### hash_scan_delfree

```c
void hash_scan_delfree(hash_t *, hnode_t *)
```

Defined at lines 711 to 715. Declared in [etc/afpd/hash.h](hash.h.md).

Unlink a node through hash_scan_delete and free it through the freenode function.

Parameters:
* `hash`: hash table
* `node`: node in the table

Calls: [hash_scan_delete](hash.c.md#hash_scan_delete)

Called by: [dircache_remove](dircache.c.md#dircache_remove)

### hash_verify

```c
int hash_verify(hash_t *)
```

Defined at lines 729 to 765. Declared in [etc/afpd/hash.h](hash.h.md).

Check the invariants of a hash table.

Debug builds assert it after each change of a table.

1. A dynamic table needs lowmark under highmark, both powers of two.
1. Each node must sit in the chain that its stored hash value picks, and the node count must match.

Parameters:
* `hash`: hash table

Returns: 1 for a valid table, 0 otherwise

Calls: [is_power_of_two](hash.c.md#is_power_of_two)

### hnode_alloc

```c
static hnode_t * hnode_alloc(void *context)
```

Defined at lines 773 to 776.

Allocate a node with malloc, the default node allocator.

Parameters:
* `context`: unused

Returns: the node, or NULL when malloc fails

Called by: [hash_create](hash.c.md#hash_create), [hash_set_allocator](hash.c.md#hash_set_allocator)

### hnode_free

```c
static void hnode_free(hnode_t *node, void *context)
```

Defined at lines 784 to 787.

Free a node with free, the default node release function.

Parameters:
* `node`: node to free
* `context`: unused

Called by: [hash_create](hash.c.md#hash_create), [hash_set_allocator](hash.c.md#hash_set_allocator)

### hnode_init

```c
hnode_t * hnode_init(hnode_t *, void *)
```

Defined at lines 796 to 801. Declared in [etc/afpd/hash.h](hash.h.md).

Set up a node that the caller allocated.

Parameters:
* `hnode`: node to set up
* `data`: value of the node

Returns: hnode, with data stored and no chain link

Called by: [hash_alloc_insert_node](hash.c.md#hash_alloc_insert_node)

### hnode_get

```c
void * hnode_get(hnode_t *)
```

Defined at lines 810 to 813. Declared in [etc/afpd/hash.h](hash.h.md).

Return the value of a node.

Parameters:
* `node`: hash node

Returns: the data pointer of the node

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [dircache_add](dircache.c.md#dircache_add), [dircache_dump](dircache.c.md#dircache_dump), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_lookup_did](dircache.c.md#dircache_lookup_did), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [dircache_reindex_didname](dircache.c.md#dircache_reindex_didname), [dircache_remove](dircache.c.md#dircache_remove), [dircache_remove_children](dircache.c.md#dircache_remove_children), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [process_cache_hints](dircache.c.md#process_cache_hints)

### hnode_getkey

```c
const void * hnode_getkey(hnode_t *)
```

Defined at lines 822 to 825. Declared in [etc/afpd/hash.h](hash.h.md).

Return the lookup value that hash_insert stored in a node.

Parameters:
* `node`: hash node

Returns: the hash_key pointer of the node

### hash_count

```c
hashcount_t hash_count(hash_t *)
```

Defined at lines 834 to 837. Declared in [etc/afpd/hash.h](hash.h.md).

Return the number of nodes in a hash table.

Parameters:
* `hash`: hash table

Returns: the node count

Called by: [hash_set_allocator](hash.c.md#hash_set_allocator)

### hash_size

```c
hashcount_t hash_size(hash_t *)
```

Defined at lines 846 to 849. Declared in [etc/afpd/hash.h](hash.h.md).

Return the number of chains of a hash table.

Parameters:
* `hash`: hash table

Returns: the chain count

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [dircache_init](dircache.c.md#dircache_init), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

### hash_fun_default

```c
static hash_val_t hash_fun_default(const void *key)
```

Defined at lines 860 to 881.

Hash a NUL-terminated string, the default hash function.

Each byte mixes in 2 entries of a 16-entry random table, and the accumulator rotates after each entry and stays within 32 bits.

Parameters:
* `key`: string to hash

Returns: the 32-bit hash value

Called by: [hash_create](hash.c.md#hash_create)

### hash_comp_default

```c
static int hash_comp_default(const void *key1, const void *key2)
```

Defined at lines 899 to 902.

Compare two strings with strcmp, the default compare function.

Parameters:
* `key1`: first string
* `key2`: second string

Returns: the strcmp result, 0 for equal strings

Called by: [hash_create](hash.c.md#hash_create)

# Macros

* Undocumented: `HASH_ASSERT_VERIFY`, `HASH_IMPLEMENTATION`, `INIT_BITS`, `INIT_MASK`, `INIT_SIZE`, `allocnode`, `chain`, `compare`, `context`, `data`, `dynamic`, `freenode`, `function`, `get16bits`, `highmark`, `hkey`, `key`, `lowmark`, `mask`, `maxcount`, `nchains`, `next`, `nodecount`, `table`, `table`

# File-scope variables

`hash_val_t_bit`
