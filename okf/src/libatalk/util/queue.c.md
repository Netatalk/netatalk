---
type: C Source File
title: "libatalk/util/queue.c"
description: "9 functions, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/queue.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/queue.h](../../include/atalk/queue.h.md)
* System headers: `stdlib.h`

# Functions

### alloc_init_node

```c
static qnode_t * alloc_init_node(void *data)
```

Defined at lines 28 to 38.

Called by: [enqueue](queue.c.md#enqueue), [prequeue](queue.c.md#prequeue), [queue_init](queue.c.md#queue_init)

### queue_init

```c
q_t * queue_init(void)
```

Defined at lines 44 to 54.

Calls: [alloc_init_node](queue.c.md#alloc_init_node)

Called by: [arc_init](../../etc/afpd/dircache.c.md#arc_init), [dircache_init](../../etc/afpd/dircache.c.md#dircache_init)

### enqueue

```c
qnode_t * enqueue(q_t *q, void *data)
```

Defined at lines 57 to 71.

Insert at tail

Calls: [alloc_init_node](queue.c.md#alloc_init_node)

Called by: [arc_case_iv_insert](../../etc/afpd/dircache.c.md#arc_case_iv_insert), [dir_remove](../../etc/afpd/directory.c.md#dir_remove), [dircache_add](../../etc/afpd/dircache.c.md#dircache_add), [dircache_defer_free](../../etc/afpd/dircache.c.md#dircache_defer_free), [dircache_evict](../../etc/afpd/dircache.c.md#dircache_evict), [rfork_cache_store_from_fd](../../etc/afpd/ad_cache.c.md#rfork_cache_store_from_fd)

### queue_move_to_tail_of

```c
qnode_t * queue_move_to_tail_of(q_t *from_q, q_t *to_q, qnode_t *node)
```

Defined at lines 86 to 106.

Move node from one queue to another without memory reallocation.

Removes node from source queue and adds it to tail of destination queue. The node pointer remains valid and unchanged - enables zero-allocation transitions between ARC lists (T1→T2, T1→B1, T2→B2, B1→T2, B2→T2).

Parameters:
* `from_q`: Source queue (node must be in this queue)
* `to_q`: Destination queue
* `node`: Node to move

Returns: The same node pointer (now in dest queue), or NULL on error

Called by: [arc_case_i](../../etc/afpd/dircache.c.md#arc_case_i), [arc_case_ii_adapt_and_replace](../../etc/afpd/dircache.c.md#arc_case_ii_adapt_and_replace), [arc_case_iii_adapt_and_replace](../../etc/afpd/dircache.c.md#arc_case_iii_adapt_and_replace), [arc_evict_to_ghost](../../etc/afpd/dircache.c.md#arc_evict_to_ghost)

Mentioned in the documentation of: [arc_case_ii_adapt_and_replace](../../etc/afpd/dircache.c.md#arc_case_ii_adapt_and_replace), [arc_case_iii_adapt_and_replace](../../etc/afpd/dircache.c.md#arc_case_iii_adapt_and_replace)

### queue_move_to_tail

```c
qnode_t * queue_move_to_tail(q_t *q, qnode_t *node)
```

Defined at lines 123 to 143.

Move existing node to tail (MRU) without reallocation.

This function provides O(1) move-to-tail operation without malloc/free overhead. Used by ARC cache to efficiently update T2 on cache hits.

Parameters:
* `q`: Queue (circular doubly-linked list with sentinel)
* `node`: Node to move to tail (must currently be in queue q)

Returns: The same node pointer (now at tail/MRU), or NULL on error

Note: The node pointer remains valid and unchanged - only its position changes

Note: If node is already at tail, this is a no-op (fast path)

Note: This is thread-safe if external locking is used (same as other queue ops)

Called by: [arc_case_i](../../etc/afpd/dircache.c.md#arc_case_i), [dircache_promote](../../etc/afpd/dircache.c.md#dircache_promote), [rfork_cache_serve_from_buf](../../etc/afpd/fork.c.md#rfork_cache_serve_from_buf)

### prequeue

```c
qnode_t * prequeue(q_t *q, void *data)
```

Defined at lines 146 to 160.

Insert at head

Calls: [alloc_init_node](queue.c.md#alloc_init_node)

### queue_remove

```c
void * queue_remove(qnode_t *node)
```

Defined at lines 163 to 176.

Unlink a node from whichever queue holds it and free it

Called by: [arc_case_iv_make_room](../../etc/afpd/dircache.c.md#arc_case_iv_make_room), [arc_ensure_ghost_capacity](../../etc/afpd/dircache.c.md#arc_ensure_ghost_capacity), [dircache_remove](../../etc/afpd/dircache.c.md#dircache_remove)

### dequeue

```c
void * dequeue(q_t *q)
```

Defined at lines 179 to 195.

Take from head

Called by: [arc_case_iv_make_room](../../etc/afpd/dircache.c.md#arc_case_iv_make_room), [dir_free_invalid_q](../../etc/afpd/directory.c.md#dir_free_invalid_q), [dircache_evict](../../etc/afpd/dircache.c.md#dircache_evict), [queue_destroy](queue.c.md#queue_destroy)

### queue_destroy

```c
void queue_destroy(q_t *q, void(*callback)(void *))
```

Defined at lines 197 to 209.

Calls: [dequeue](queue.c.md#dequeue)

Called by: [arc_destroy](../../etc/afpd/dircache.c.md#arc_destroy)
