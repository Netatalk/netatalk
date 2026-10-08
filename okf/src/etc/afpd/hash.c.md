---
type: C Source File
title: "etc/afpd/hash.c"
description: "29 functions, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/afpd/hash.c"
tags: ["etc/afpd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 93 to 106.

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

Defined at lines 111 to 122.

Verify whether the given argument is a power of two.

Called by: [hash_verify](hash.c.md#hash_verify)

### clear_table

```c
static void clear_table(hash_t *hash)
```

Defined at lines 127 to 134.

Initialize the table of pointers to null.

Called by: [hash_create](hash.c.md#hash_create), [hash_free_nodes](hash.c.md#hash_free_nodes)

### grow_table

```c
static void grow_table(hash_t *hash)
```

Defined at lines 166 to 212.

Double the size of a dynamic table.

This works as follows. Each chain splits into two adjacent chains. The shift amount increases by one, exposing an additional bit of each hashed key. For each node in the original chain, the value of this newly exposed bit will decide which of the two new chains will receive the node: if the bit is 1, the chain with the higher index will have the node, otherwise the lower chain will receive the node. In this manner, the hash table will continue to function exactly as before without having to rehash any of the keys.

Note: 1. Overflow check.
1. The new number of chains is twice the old number of chains.
1. The new mask is one bit wider than the previous, revealing a new bit in all hashed keys.
1. Allocate a new table of chain pointers that is twice as large as the previous one.
1. If the reallocation was successful, we perform the rest of the growth algorithm, otherwise we do nothing.
1. The exposed_bit variable holds a mask with which each hashed key can be AND-ed to test the value of its newly exposed bit.
1. Now loop over each chain in the table and sort its nodes into two chains based on the value of each node's newly exposed hash bit.
1. The low chain replaces the current chain. The high chain goes into the corresponding sister chain in the upper half of the table.
1. We have finished dealing with the chains and nodes. We now update the various bookeeping fields of the hash structure.

Called by: [hash_insert](hash.c.md#hash_insert)

### shrink_table

```c
static void shrink_table(hash_t *hash)
```

Defined at lines 246 to 291.

Cut a table size in half.

This is done by folding together adjacent chains and populating the lower half of the table with these chains. The chains are simply spliced together. Once this is done, the whole table is reallocated to a smaller object.

Note: 1. It is illegal to have a hash table with one slot. This would mean that hash->shift is equal to hash_val_t_bit, an illegal shift value. Also, other things could go wrong, such as hash->lowmark becoming zero.
1. Looping over each pair of sister chains, the low_chain is set to point to the head node of the chain in the lower half of the table, and high_chain points to the head node of the sister in the upper half.
1. The intent here is to compute a pointer to the last node of the lower chain into the low_tail variable. If this chain is empty, low_tail ends up with a null value.
1. If the lower chain is not empty, we simply tack the upper chain onto it. If the upper chain is a null pointer, nothing happens.
1. Otherwise if the lower chain is empty but the upper one is not, If the low chain is empty, but the high chain is not, then the high chain is simply transferred to the lower half of the table.
1. Otherwise if both chains are empty, there is nothing to do.
1. All the chain pointers are in the lower half of the table now, so we reallocate it to a smaller object. This, of course, invalidates all pointer-to-pointers which reference into the table from the first node of each chain.
1. Though it's unlikely, the reallocation may fail. In this case we pretend that the table *was* reallocated to a smaller object.
1. Finally, update the various table parameters to reflect the new size.

Called by: [hash_delete](hash.c.md#hash_delete)

### hash_create

```c
hash_t * hash_create(hashcount_t, hash_comp_t, hash_fun_t)
```

Defined at lines 324 to 368. Declared in [etc/afpd/hash.h](hash.h.md).

Create a dynamic hash table.

Both the hash table structure and the table itself are dynamically allocated. Furthermore, the table is extendible in that it will automatically grow as its load factor increases beyond a certain threshold.

Note: 1. If the number of bits in the hash_val_t type has not been computed yet, we do so here, because this is likely to be the first function that the user calls.
1. Allocate a hash table control structure.
1. If a hash table control structure is successfully allocated, we proceed to initialize it. Otherwise we return a null pointer.
1. We try to allocate the table of hash chains.
1. If we were able to allocate the hash chain table, we can finish initializing the hash structure and the table. Otherwise, we must backtrack by freeing the hash structure.
1. INIT_SIZE should be a power of two. The high and low marks are always set to be twice the table size and half the table size respectively. When the number of nodes in the table grows beyond the high size (beyond load factor 2), it will double in size to cut the load factor down to about about 1. If the table shrinks down to or beneath load factor 0.5, it will shrink, bringing the load up to about 1. However, the table will never shrink beneath INIT_SIZE even if it's emptied.
1. This indicates that the table is dynamically allocated and dynamically resized on the fly. A table that has this value set to zero is assumed to be statically allocated and will not be resized.
1. The table of chains must be properly reset to all null pointers.

Calls: [clear_table](hash.c.md#clear_table), [compute_bits](hash.c.md#compute_bits), [hash_comp_default](hash.c.md#hash_comp_default), [hash_fun_default](hash.c.md#hash_fun_default), [hnode_alloc](hash.c.md#hnode_alloc), [hnode_free](hash.c.md#hnode_free)

Called by: [dircache_init](dircache.c.md#dircache_init)

Uses file-scope variables: `hash_val_t_bit`

Mentioned in the documentation of: [hash_scan_next](hash.c.md#hash_scan_next)

### hash_set_allocator

```c
void hash_set_allocator(hash_t *, hnode_alloc_t, hnode_free_t, void *)
```

Defined at lines 373 to 381. Declared in [etc/afpd/hash.h](hash.h.md).

Select a different set of node allocator routines.

Calls: [hash_count](hash.c.md#hash_count), [hnode_alloc](hash.c.md#hnode_alloc), [hnode_free](hash.c.md#hnode_free)

### hash_free_nodes

```c
void hash_free_nodes(hash_t *)
```

Defined at lines 387 to 400. Declared in [etc/afpd/hash.h](hash.h.md).

Free every node in the hash using the hash->freenode() function pointer, and cause the hash to become empty.

Calls: [clear_table](hash.c.md#clear_table), [hash_scan_begin](hash.c.md#hash_scan_begin), [hash_scan_delete](hash.c.md#hash_scan_delete), [hash_scan_next](hash.c.md#hash_scan_next)

### hash_destroy

```c
void hash_destroy(hash_t *)
```

Defined at lines 405 to 411. Declared in [etc/afpd/hash.h](hash.h.md).

Free a dynamic hash table structure.

Calls: [hash_isempty](hash.h.md#hash_isempty)

Uses file-scope variables: `hash_val_t_bit`

### hash_scan_begin

```c
void hash_scan_begin(hscan_t *, hash_t *)
```

Defined at lines 426 to 444. Declared in [etc/afpd/hash.h](hash.h.md).

Initialize a hash scanner.

Reset the hash scanner so that the next element retrieved by [hash_scan_next()](hash.c.md#hash_scan_next) shall be the first element on the first non-empty chain.

Note: 1. Locate the first non empty chain.
1. If an empty chain is found, remember which one it is and set the next pointer to refer to its first element.
1. Otherwise if a chain is not found, set the next pointer to NULL so that [hash_scan_next()](hash.c.md#hash_scan_next) shall indicate failure.

Called by: [dircache_dump](dircache.c.md#dircache_dump), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [dircache_remove_children](dircache.c.md#dircache_remove_children), [hash_free_nodes](hash.c.md#hash_free_nodes)

### hash_scan_next

```c
hnode_t * hash_scan_next(hscan_t *)
```

Defined at lines 471 to 512. Declared in [etc/afpd/hash.h](hash.h.md).

Retrieve the next node from the hash table, and update the pointer for the next invocation of [hash_scan_next()](hash.c.md#hash_scan_next).

Note: 1. Remember the next pointer in a temporary value so that it can be returned.
1. This assertion essentially checks whether the module has been properly initialized. The first point of interaction with the module should be either [hash_create()](hash.c.md#hash_create) or [hash_init()](hash.h.md#hash_init), both of which set hash_val_t_bit to a non zero value.
1. If the next pointer we are returning is not NULL, then the user is allowed to call [hash_scan_next()](hash.c.md#hash_scan_next) again. We prepare the new next pointer for that call right now. That way the user is allowed to delete the node we are about to return, since we will no longer be needing it to locate the next node.
1. If there is a next node in the chain (next->next), then that becomes the new next node, otherwise ...
1. We have exhausted the current chain, and must locate the next subsequent non-empty chain in the table.
1. If a non-empty chain is found, the first element of that chain becomes the new next node. Otherwise there is no new next node and we set the pointer to NULL so that the next time [hash_scan_next()](hash.c.md#hash_scan_next) is called, a null pointer shall be immediately returned.

Called by: [dircache_dump](dircache.c.md#dircache_dump), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [dircache_remove_children](dircache.c.md#dircache_remove_children), [hash_free_nodes](hash.c.md#hash_free_nodes)

Uses file-scope variables: `hash_val_t_bit`

Mentioned in the documentation of: [hash_scan_begin](hash.c.md#hash_scan_begin)

### hash_insert

```c
void hash_insert(hash_t *, hnode_t *, const void *)
```

Defined at lines 527 to 552. Declared in [etc/afpd/hash.h](hash.h.md).

Insert a node into the hash table.

Note: 1. It's illegal to insert more than the maximum number of nodes. The client should verify that the hash table is not full before attempting an insertion.
1. The same key may not be inserted into a table twice.
1. If the table is dynamic and the load factor is already at >= 2, grow the table.
1. We take the bottom N bits of the hash value to derive the chain index, where N is the base 2 logarithm of the size of the hash table.

Calls: [grow_table](hash.c.md#grow_table), [hash_lookup](hash.c.md#hash_lookup)

Called by: [hash_alloc_insert_node](hash.c.md#hash_alloc_insert_node)

Uses file-scope variables: `hash_val_t_bit`

Mentioned in the documentation of: [arc_case_iv_make_room](dircache.c.md#arc_case_iv_make_room)

### hash_lookup

```c
hnode_t * hash_lookup(hash_t *, const void *)
```

Defined at lines 568 to 585. Declared in [etc/afpd/hash.h](hash.h.md).

Find a node in the hash table and return a pointer to it.

Note: 1. We hash the key and keep the entire hash value. As an optimization, when we descend down the chain, we can compare hash values first and only if hash values match do we perform a full key comparison.
1. To locate the chain from among 2^N chains, we look at the lower N bits of the hash value by anding them with the current mask.
1. Looping through the chain, we compare the stored hash value inside each node against our computed hash. If they match, then we do a full comparison between the unhashed keys. If these match, we have located the entry.

Called by: [dircache_add](dircache.c.md#dircache_add), [dircache_lookup_parent](dircache.c.md#dircache_lookup_parent), [dircache_reindex_didname](dircache.c.md#dircache_reindex_didname), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [hash_delete](hash.c.md#hash_delete), [hash_insert](hash.c.md#hash_insert), [hash_scan_delete](hash.c.md#hash_scan_delete), [process_cache_hints](dircache.c.md#process_cache_hints)

Mentioned in the documentation of: [process_cache_hints](dircache.c.md#process_cache_hints)

### hash_delete

```c
hnode_t * hash_delete(hash_t *, hnode_t *)
```

Defined at lines 605 to 642. Declared in [etc/afpd/hash.h](hash.h.md).

Delete the given node from the hash table. Since the chains are singly linked, we must locate the start of the node's chain and traverse.

Note: 1. The node must belong to this hash table, and its key must not have been tampered with.
1. If this deletion will take the node count below the low mark, we shrink the table now.
1. Determine which chain the node belongs to, and fetch the pointer to the first node in this chain.
1. If the node being deleted is the first node in the chain, then simply update the chain head pointer.
1. Otherwise advance to the node's predecessor, and splice out by updating the predecessor's next pointer.
1. Indicate that the node is no longer in a hash table.

Calls: [hash_lookup](hash.c.md#hash_lookup), [shrink_table](hash.c.md#shrink_table)

Called by: [hash_delete_free](hash.c.md#hash_delete_free)

Uses file-scope variables: `hash_val_t_bit`

### hash_alloc_insert

```c
int hash_alloc_insert(hash_t *, const void *, void *)
```

Defined at lines 644 to 647. Declared in [etc/afpd/hash.h](hash.h.md).

Calls: [hash_alloc_insert_node](hash.c.md#hash_alloc_insert_node)

### hash_alloc_insert_node

```c
hnode_t * hash_alloc_insert_node(hash_t *, const void *, void *)
```

Defined at lines 653 to 663. Declared in [etc/afpd/hash.h](hash.h.md).

Like hash_alloc_insert, but hands back the inserted node, so a caller that keeps it can delete without a lookup.

Calls: [hash_insert](hash.c.md#hash_insert), [hnode_init](hash.c.md#hnode_init)

Called by: [dircache_add](dircache.c.md#dircache_add), [dircache_reindex_didname](dircache.c.md#dircache_reindex_didname), [hash_alloc_insert](hash.c.md#hash_alloc_insert)

### hash_delete_free

```c
void hash_delete_free(hash_t *, hnode_t *)
```

Defined at lines 665 to 669. Declared in [etc/afpd/hash.h](hash.h.md).

Calls: [hash_delete](hash.c.md#hash_delete)

Called by: [dircache_add](dircache.c.md#dircache_add), [dircache_remove](dircache.c.md#dircache_remove)

### hash_scan_delete

```c
hnode_t * hash_scan_delete(hash_t *, hnode_t *)
```

Defined at lines 675 to 698. Declared in [etc/afpd/hash.h](hash.h.md).

Exactly like hash_delete, except does not trigger table shrinkage. This is to be used from within a hash table scan operation. See notes for hash_delete.

Calls: [hash_lookup](hash.c.md#hash_lookup)

Called by: [hash_free_nodes](hash.c.md#hash_free_nodes), [hash_scan_delfree](hash.c.md#hash_scan_delfree)

Uses file-scope variables: `hash_val_t_bit`

### hash_scan_delfree

```c
void hash_scan_delfree(hash_t *, hnode_t *)
```

Defined at lines 703 to 707. Declared in [etc/afpd/hash.h](hash.h.md).

Like hash_delete_free but based on hash_scan_delete.

Calls: [hash_scan_delete](hash.c.md#hash_scan_delete)

Called by: [dircache_remove](dircache.c.md#dircache_remove)

### hash_verify

```c
int hash_verify(hash_t *)
```

Defined at lines 718 to 754. Declared in [etc/afpd/hash.h](hash.h.md).

Verify whether the given object is a valid hash table.

Note: 1. If the hash table is dynamic, verify whether the high and low expansion/shrinkage thresholds are powers of two.
1. Count all nodes in the table, and test each hash value to see whether it is correct for the node's chain.

Calls: [is_power_of_two](hash.c.md#is_power_of_two)

### hnode_alloc

```c
static hnode_t * hnode_alloc(void *context)
```

Defined at lines 756 to 759.

Called by: [hash_create](hash.c.md#hash_create), [hash_set_allocator](hash.c.md#hash_set_allocator)

### hnode_free

```c
static void hnode_free(hnode_t *node, void *context)
```

Defined at lines 761 to 764.

Called by: [hash_create](hash.c.md#hash_create), [hash_set_allocator](hash.c.md#hash_set_allocator)

### hnode_init

```c
hnode_t * hnode_init(hnode_t *, void *)
```

Defined at lines 769 to 774. Declared in [etc/afpd/hash.h](hash.h.md).

Initialize a client-supplied node

Called by: [hash_alloc_insert_node](hash.c.md#hash_alloc_insert_node)

### hnode_get

```c
void * hnode_get(hnode_t *)
```

Defined at lines 777 to 780. Declared in [etc/afpd/hash.h](hash.h.md).

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [dircache_add](dircache.c.md#dircache_add), [dircache_dump](dircache.c.md#dircache_dump), [dircache_flush_deferred_for_vol](dircache.c.md#dircache_flush_deferred_for_vol), [dircache_lookup_parent](dircache.c.md#dircache_lookup_parent), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain), [dircache_purge_vol](dircache.c.md#dircache_purge_vol), [dircache_reindex_didname](dircache.c.md#dircache_reindex_didname), [dircache_remove](dircache.c.md#dircache_remove), [dircache_remove_children](dircache.c.md#dircache_remove_children), [dircache_search_by_did](dircache.c.md#dircache_search_by_did), [dircache_search_by_name](dircache.c.md#dircache_search_by_name), [process_cache_hints](dircache.c.md#process_cache_hints)

### hnode_getkey

```c
const void * hnode_getkey(hnode_t *)
```

Defined at lines 783 to 786. Declared in [etc/afpd/hash.h](hash.h.md).

### hash_count

```c
hashcount_t hash_count(hash_t *)
```

Defined at lines 789 to 792. Declared in [etc/afpd/hash.h](hash.h.md).

Called by: [hash_set_allocator](hash.c.md#hash_set_allocator)

### hash_size

```c
hashcount_t hash_size(hash_t *)
```

Defined at lines 795 to 798. Declared in [etc/afpd/hash.h](hash.h.md).

Called by: [deferred_batch_chain](dircache.c.md#deferred_batch_chain), [dircache_init](dircache.c.md#dircache_init), [dircache_process_deferred_chain](dircache.c.md#dircache_process_deferred_chain)

### hash_fun_default

```c
static hash_val_t hash_fun_default(const void *key)
```

Defined at lines 800 to 821.

Called by: [hash_create](hash.c.md#hash_create)

### hash_comp_default

```c
static int hash_comp_default(const void *key1, const void *key2)
```

Defined at lines 832 to 835.

Called by: [hash_create](hash.c.md#hash_create)

# Macros

* Undocumented: `HASH_ASSERT_VERIFY`, `HASH_IMPLEMENTATION`, `INIT_BITS`, `INIT_MASK`, `INIT_SIZE`, `allocnode`, `chain`, `compare`, `context`, `data`, `dynamic`, `freenode`, `function`, `get16bits`, `highmark`, `hkey`, `key`, `lowmark`, `mask`, `maxcount`, `nchains`, `next`, `nodecount`, `table`, `table`

# File-scope variables

`hash_val_t_bit`
