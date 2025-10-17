/* This code has been stolen from Linux kernel :) */
/* distributed under GNU GPL version 2. */

#ifndef _ATALK_LIST_H
#define _ATALK_LIST_H

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

struct list_head {
    struct list_head *next, *prev;
};

#define ATALK_LIST_HEAD_INIT(name) { &(name), &(name) }

#define ATALK_LIST_HEAD(name) \
    struct list_head name = ATALK_LIST_HEAD_INIT(name)

#define ATALK_INIT_LIST_HEAD(ptr) do { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

#ifdef USE_LIST
/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/*!
 * @brief list_add - add a new entry
 * @param new new entry to be added
 * @param head list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

/*!
 * @brief list_add_tail - add a new entry
 * @param new new entry to be added
 * @param head list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    __list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head *prev,
                              struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

/*!
 * @brief list_del - deletes entry from list.
 * @param entry the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

/*!
 * @brief list_del_init - deletes entry from list and reinitialize it.
 * @param entry the element to delete from the list.
 */
static inline void list_del_init(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    ATALK_INIT_LIST_HEAD(entry);
}

/*!
 * @brief list_empty - tests whether a list is empty
 * @param head the list to test.
 */
static inline int list_empty(struct list_head *head)
{
    return head->next == head;
}

/*!
 * @brief list_splice - join two lists
 * @param list the new list to add.
 * @param head the place to add it in the first list.
 */
static inline void list_splice(struct list_head *list, struct list_head *head)
{
    struct list_head *first = list->next;

    if (first != list) {
        struct list_head *last = list->prev;
        struct list_head *at = head->next;
        first->prev = head;
        head->next = first;
        last->next = at;
        at->prev = last;
    }
}

#endif
/*!
 * @brief get the struct for this entry
 * @param ptr       the &struct list_head pointer.
 * @param type      the type of the struct this is embedded in.
 * @param member    the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/*!
 * @brief iterate over a list
 * @param pos       the &struct list_head to use as a loop counter.
 * @param head      the head for your list.
 */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); \
        pos = pos->next)

/*!
 * @brief iterate over a list in reverse order
 * @param pos       the &struct list_head to use as a loop counter.
 * @param head      the head for your list.
 */
#define list_for_each_prev(pos, head) \
    for (pos = (head)->prev; pos != (head); \
        pos = pos->prev)
#endif
