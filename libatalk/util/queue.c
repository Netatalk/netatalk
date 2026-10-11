/*
  Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

/*!
 * @file
 * Netatalk utility functions: queue
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>

#include <atalk/queue.h>

/*!
 * @brief Allocate a queue node that holds an element, with its links unset
 *
 * @param[in] data  element of the node
 * @returns the node, or NULL when malloc fails
 */
static qnode_t *alloc_init_node(void *data)
{
    qnode_t *node;

    if ((node = malloc(sizeof(qnode_t))) == NULL) {
        return NULL;
    }

    node->data = data;
    return node;
}

/********************************************************************************
 * Interface
 *******************************************************************************/

/*!
 * @brief Create an empty queue, a sentinel node that links to itself
 *
 * @returns the queue, or NULL when malloc fails
 */
q_t *queue_init(void)
{
    q_t *queue;

    if ((queue = alloc_init_node(NULL)) == NULL) {
        return NULL;
    }

    queue->prev = queue->next = queue;
    return queue;
}

/*!
 * @brief Append an element at the tail of a queue
 *
 * @param[in,out] q     queue
 * @param[in]     data  element
 * @returns the new node, or NULL when malloc fails
 */
qnode_t *enqueue(q_t *q, void *data)
{
    qnode_t *node;

    if ((node = alloc_init_node(data)) == NULL) {
        return NULL;
    }

    /* insert at tail */
    node->next = q;
    node->prev = q->prev;
    q->prev->next = node;
    q->prev = node;
    return node;
}

/*!
 * @brief Move node from one queue to another without memory reallocation
 *
 * Removes node from source queue and adds it to tail of destination queue.
 * The node pointer remains valid and unchanged - enables zero-allocation
 * transitions between ARC lists (T1→T2, T1→B1, T2→B2, B1→T2, B2→T2).
 *
 * @param[in] from_q  Source queue (node must be in this queue)
 * @param[in] to_q    Destination queue
 * @param[in] node    Node to move
 *
 * @returns The same node pointer (now in dest queue), or NULL on error
 */
qnode_t *queue_move_to_tail_of(q_t *from_q, q_t *to_q, qnode_t *node)
{
    if (!from_q || !to_q || !node) {
        return NULL;
    }

    /* Cannot move sentinel nodes */
    if (node == from_q || node == to_q) {
        return NULL;
    }

    /* Unlink from source queue */
    node->prev->next = node->next;
    node->next->prev = node->prev;
    /* Relink to tail of destination queue */
    node->next = to_q;
    node->prev = to_q->prev;
    to_q->prev->next = node;
    to_q->prev = node;
    return node;
}

/*!
 * @brief Move existing node to tail (MRU) without reallocation
 *
 * This function provides O(1) move-to-tail operation without malloc/free overhead.
 * Used by ARC cache to efficiently update T2 on cache hits.
 *
 * @param[in] q     Queue (circular doubly-linked list with sentinel)
 * @param[in] node  Node to move to tail (must currently be in queue q)
 *
 * @returns The same node pointer (now at tail/MRU), or NULL on error
 *
 * @note The node pointer remains valid and unchanged - only its position changes
 * @note If node is already at tail, this is a no-op (fast path)
 * @note This is thread-safe if external locking is used (same as other queue ops)
 */
qnode_t *queue_move_to_tail(q_t *q, qnode_t *node)
{
    if (!q || !node || node == q) {
        return NULL;  /* Invalid parameters or trying to move sentinel */
    }

    /* Fast path: already at tail (MRU)? */
    if (node->next == q) {
        return node;
    }

    /* Unlink node from current position */
    node->prev->next = node->next;
    node->next->prev = node->prev;
    /* Relink at tail (before sentinel) */
    node->next = q;
    node->prev = q->prev;
    q->prev->next = node;
    q->prev = node;
    return node;
}

/*!
 * @brief Insert an element at the head of a queue
 *
 * @param[in,out] q     queue
 * @param[in]     data  element
 * @returns the new node, or NULL when malloc fails
 */
qnode_t *prequeue(q_t *q, void *data)
{
    qnode_t *node;

    if ((node = alloc_init_node(data)) == NULL) {
        return NULL;
    }

    /* insert at head */
    q->next->prev = node;
    node->next = q->next;
    node->prev = q;
    q->next = node;
    return node;
}

/*!
 * @brief Unlink a node from the queue that holds it and free the node
 *
 * @param[in] node  node, or NULL
 * @returns the element of the node, or NULL for a NULL node
 */
void *queue_remove(qnode_t *node)
{
    void *data;

    if (node == NULL) {
        return NULL;
    }

    data = node->data;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    free(node);
    return data;
}

/*!
 * @brief Remove the element at the head of a queue
 *
 * A NULL element also returns NULL.
 *
 * @param[in,out] q  queue, or NULL
 * @returns the element, or NULL for an empty or NULL queue
 */
void *dequeue(q_t *q)
{
    qnode_t *node;
    void *data;

    if (q == NULL || q->next == q) {
        return NULL;
    }

    /* take first node from head */
    node = q->next;
    data = node->data;
    q->next = node->next;
    node->next->prev = node->prev;
    free(node);
    return data;
}

/*!
 * @brief Free a queue and its nodes, passing each element to a callback
 *
 * A NULL element ends the walk early, so the nodes after it leak.
 *
 * @param[in] q         queue
 * @param[in] callback  function that receives each element, or NULL
 */
void queue_destroy(q_t *q, void (*callback)(void *))
{
    void *p;

    while ((p = dequeue(q)) != NULL) {
        if (callback) {
            callback(p);
        }
    }

    free(q);
    q = NULL;
}

