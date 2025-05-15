/*
 * Hash Table Data Type
 * Copyright (C) 1997 Kaz Kylheku <kaz@ashi.footprints.net>
 *
 * Free Software License:
 *
 * All rights are reserved by the author, with the following exceptions:
 * Permission is granted to freely reproduce and distribute this software,
 * possibly in exchange for a fee, provided that this copyright notice appears
 * intact. Permission is also granted to adapt this software to produce
 * derivative works, as long as the modified versions carry this copyright
 * notice and additional notices stating that the work has been modified.
 * This source code may be translated into executable form and incorporated
 * into proprietary software; there is no requirement for such software to
 * contain a copyright notice related to this source.
 *
 * $Name:  $
 */
#define NDEBUG
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#define HASH_IMPLEMENTATION
#include "hash.h"

#ifdef KAZLIB_RCSID
#endif

#define INIT_BITS   6
/* must be power of two */
#define INIT_SIZE   (1UL << (INIT_BITS))
#define INIT_MASK   ((INIT_SIZE) - 1)

#define next hash_next
#define key hash_key
#define data hash_data
#define hkey hash_hkey

#define table hash_table
#define nchains hash_nchains
#define nodecount hash_nodecount
#define maxcount hash_maxcount
#define highmark hash_highmark
#define lowmark hash_lowmark
#define compare hash_compare
#define function hash_function
#define allocnode hash_allocnode
#define freenode hash_freenode
#define context hash_context
#define mask hash_mask
#define dynamic hash_dynamic

#define table hash_table
#define chain hash_chain

static hnode_t *hnode_alloc(void *context);
static void hnode_free(hnode_t *node, void *context);
static hash_val_t hash_fun_default(const void *key);
static int hash_comp_default(const void *key1, const void *key2);

int hash_val_t_bit;

/*
 * Compute the number of bits in the hash_val_t type.  We know that hash_val_t
 * is an unsigned integral type. Thus the highest value it can hold is a
 * Mersenne number (power of two, less one). We initialize a hash_val_t
 * object with this value and then shift bits out one by one while counting.
 * Notes:
 * 1. HASH_VAL_T_MAX is a Mersenne number---one that is one less than a power
 *    of two. This means that its binary representation consists of all one
 *    bits, and hence ``val'' is initialized to all one bits.
 * 2. While bits remain in val, we increment the bit count and shift it to the
 *    right, replacing the topmost bit by zero.
 */

static void compute_bits(void)
{
    /* 1 */
    hash_val_t val = HASH_VAL_T_MAX;
    int bits = 0;

    /* 2 */
    while (val) {
        bits++;
        val >>= 1;
    }

    hash_val_t_bit = bits;
}

/*
 * Verify whether the given argument is a power of two.
 */

static int is_power_of_two(hash_val_t arg)
{
    if (arg == 0) {
        return 0;
    }

    while ((arg & 1) == 0) {
        arg >>= 1;
    }

    return arg == 1;
}

/*
 * Initialize the table of pointers to null.
 */

static void clear_table(hash_t *hash)
{
    hash_val_t i;

    for (i = 0; i < hash->nchains; i++) {
        hash->table[i] = NULL;
    }
}

/*
 * Double the size of a dynamic table. This works as follows. Each chain splits
 * into two adjacent chains.  The shift amount increases by one, exposing an
 * additional bit of each hashed key. For each node in the original chain, the
 * value of this newly exposed bit will decide which of the two new chains will
 * receive the node: if the bit is 1, the chain with the higher index will have
 * the node, otherwise the lower chain will receive the node. In this manner,
 * the hash table will continue to function exactly as before without having to
 * rehash any of the keys.
 * Notes:
 * 1.  Overflow check.
 * 2.  The new number of chains is twice the old number of chains.
 * 3.  The new mask is one bit wider than the previous, revealing a
 *     new bit in all hashed keys.
 * 4.  Allocate a new table of chain pointers that is twice as large as the
 *     previous one.
 * 5.  If the reallocation was successful, we perform the rest of the growth
 *     algorithm, otherwise we do nothing.
 * 6.  The exposed_bit variable holds a mask with which each hashed key can be
 *     AND-ed to test the value of its newly exposed bit.
 * 7.  Now loop over each chain in the table and sort its nodes into two
 *     chains based on the value of each node's newly exposed hash bit.
 * 8.  The low chain replaces the current chain.  The high chain goes
 *     into the corresponding sister chain in the upper half of the table.
 * 9.  We have finished dealing with the chains and nodes. We now update
 *     the various bookeeping fields of the hash structure.
 */

static void grow_table(hash_t *hash)
{
    hnode_t **newtable;
    /* 1 */
    assert(2 * hash->nchains > hash->nchains);
    /* 4 */
    newtable = realloc(hash->table, sizeof(*newtable) * hash->nchains * 2);

    /* 5 */
    if (newtable) {
        /* 3 */
        hash_val_t mask = (hash->mask << 1) | 1;
        /* 6 */
        hash_val_t exposed_bit = mask ^ hash->mask;
        assert(mask != hash->mask);

        /* 7 */
        for (hash_val_t chain = 0; chain < hash->nchains; chain++) {
            hnode_t *low_chain = NULL, *high_chain = NULL, *hptr, *next;

            for (hptr = newtable[chain]; hptr != NULL; hptr = next) {
                next = hptr->next;

                if (hptr->hkey & exposed_bit) {
                    hptr->next = high_chain;
                    high_chain = hptr;
                } else {
                    hptr->next = low_chain;
                    low_chain = hptr;
                }
            }

            /* 8 */
            newtable[chain] = low_chain;
            newtable[chain + hash->nchains] = high_chain;
        }

        /* 9 */
        hash->table = newtable;
        hash->mask = mask;
        hash->nchains *= 2;
        hash->lowmark *= 2;
        hash->highmark *= 2;
    }

    assert(hash_verify(hash));
}

/*
 * Cut a table size in half. This is done by folding together adjacent chains
 * and populating the lower half of the table with these chains. The chains are
 * simply spliced together. Once this is done, the whole table is reallocated
 * to a smaller object.
 * Notes:
 * 1.  It is illegal to have a hash table with one slot. This would mean that
 *     hash->shift is equal to hash_val_t_bit, an illegal shift value.
 *     Also, other things could go wrong, such as hash->lowmark becoming zero.
 * 2.  Looping over each pair of sister chains, the low_chain is set to
 *     point to the head node of the chain in the lower half of the table,
 *     and high_chain points to the head node of the sister in the upper half.
 * 3.  The intent here is to compute a pointer to the last node of the
 *     lower chain into the low_tail variable. If this chain is empty,
 *     low_tail ends up with a null value.
 * 4.  If the lower chain is not empty, we simply tack the upper chain onto it.
 *     If the upper chain is a null pointer, nothing happens.
 * 5.  Otherwise if the lower chain is empty but the upper one is not,
 *     If the low chain is empty, but the high chain is not, then the
 *     high chain is simply transferred to the lower half of the table.
 * 6.  Otherwise if both chains are empty, there is nothing to do.
 * 7.  All the chain pointers are in the lower half of the table now, so
 *     we reallocate it to a smaller object. This, of course, invalidates
 *     all pointer-to-pointers which reference into the table from the
 *     first node of each chain.
 * 8.  Though it's unlikely, the reallocation may fail. In this case we
 *     pretend that the table _was_ reallocated to a smaller object.
 * 9.  Finally, update the various table parameters to reflect the new size.
 */

static void shrink_table(hash_t *hash)
{
    hash_val_t chain;
    hash_val_t nchains;
    hnode_t **newtable, *low_tail, *low_chain, *high_chain;
    /* 1 */
    assert(hash->nchains >= 2);
    nchains = hash->nchains / 2;

    for (chain = 0; chain < nchains; chain++) {
        /* 2 */
        low_chain = hash->table[chain];
        high_chain = hash->table[chain + nchains];

        /* 3 */
        for (low_tail = low_chain; low_tail
                && low_tail->next; low_tail = low_tail->next)
            ;

        if (low_chain != NULL) {
            /* 4 */
            low_tail->next = high_chain;
        } else if (high_chain != NULL) {
            /* 5 */
            hash->table[chain] = high_chain;
        } else {
            /* 6 */
            assert(hash->table[chain] == NULL);
        }
    }

    /* 7 */
    newtable = realloc(hash->table, sizeof(*newtable) * nchains);

    /* 8 */
    if (newtable) {
        hash->table = newtable;
    }

    /* 9 */
    hash->mask >>= 1;
    hash->nchains = nchains;
    hash->lowmark /= 2;
    hash->highmark /= 2;
    assert(hash_verify(hash));
}


/*
 * Create a dynamic hash table. Both the hash table structure and the table
 * itself are dynamically allocated. Furthermore, the table is extendible in
 * that it will automatically grow as its load factor increases beyond a
 * certain threshold.
 * Notes:
 * 1. If the number of bits in the hash_val_t type has not been computed yet,
 *    we do so here, because this is likely to be the first function that the
 *    user calls.
 * 2. Allocate a hash table control structure.
 * 3. If a hash table control structure is successfully allocated, we
 *    proceed to initialize it. Otherwise we return a null pointer.
 * 4. We try to allocate the table of hash chains.
 * 5. If we were able to allocate the hash chain table, we can finish
 *    initializing the hash structure and the table. Otherwise, we must
 *    backtrack by freeing the hash structure.
 * 6. INIT_SIZE should be a power of two. The high and low marks are always set
 *    to be twice the table size and half the table size respectively. When the
 *    number of nodes in the table grows beyond the high size (beyond load
 *    factor 2), it will double in size to cut the load factor down to about
 *    about 1. If the table shrinks down to or beneath load factor 0.5,
 *    it will shrink, bringing the load up to about 1. However, the table
 *    will never shrink beneath INIT_SIZE even if it's emptied.
 * 7. This indicates that the table is dynamically allocated and dynamically
 *    resized on the fly. A table that has this value set to zero is
 *    assumed to be statically allocated and will not be resized.
 * 8. The table of chains must be properly reset to all null pointers.
 */

hash_t *hash_create(hashcount_t maxcount, hash_comp_t compfun,
                    hash_fun_t hashfun)
{
    hash_t *hash;

    /* 1 */
    if (hash_val_t_bit == 0) {
        compute_bits();
    }

    /* 2 */
    hash = malloc(sizeof(*hash));

    /* 3 */
    if (hash) {
        /* 4 */
        hash->table = malloc(sizeof(*hash->table) * INIT_SIZE);

        /* 5 */
        if (hash->table) {
            /* 6 */
            hash->nchains = INIT_SIZE;
            hash->highmark = INIT_SIZE * 2;
            hash->lowmark = INIT_SIZE / 2;
            hash->nodecount = 0;
            hash->maxcount = maxcount;
            hash->compare = compfun ? compfun : hash_comp_default;
            hash->function = hashfun ? hashfun : hash_fun_default;
            hash->allocnode = hnode_alloc;
            hash->freenode = hnode_free;
            hash->context = NULL;
            hash->mask = INIT_MASK;
            /* 7 */
            hash->dynamic = 1;
            /* 8 */
            clear_table(hash);
            assert(hash_verify(hash));
            return hash;
        }

        free(hash);
    }

    return NULL;
}

/*
 * Select a different set of node allocator routines.
 */

void hash_set_allocator(hash_t *hash, hnode_alloc_t al,
                        hnode_free_t fr, void *context)
{
    assert(hash_count(hash) == 0);
    assert((al == 0 && fr == 0) || (al != 0 && fr != 0));
    hash->allocnode = al ? al : hnode_alloc;
    hash->freenode = fr ? fr : hnode_free;
    hash->context = context;
}

/*
 * Free every node in the hash using the hash->freenode() function pointer, and
 * cause the hash to become empty.
 */

void hash_free_nodes(hash_t *hash)
{
    hscan_t hs;
    hnode_t *node;
    hash_scan_begin(&hs, hash);

    while ((node = hash_scan_next(&hs))) {
        hash_scan_delete(hash, node);
        hash->freenode(node, hash->context);
    }

    hash->nodecount = 0;
    clear_table(hash);
}

/*
 * Free a dynamic hash table structure.
 */

void hash_destroy(hash_t *hash)
{
    assert(hash_val_t_bit != 0);
    assert(hash_isempty(hash));
    free(hash->table);
    free(hash);
}

/*
 * Reset the hash scanner so that the next element retrieved by
 * hash_scan_next() shall be the first element on the first non-empty chain.
 * Notes:
 * 1. Locate the first non empty chain.
 * 2. If an empty chain is found, remember which one it is and set the next
 *    pointer to refer to its first element.
 * 3. Otherwise if a chain is not found, set the next pointer to NULL
 *    so that hash_scan_next() shall indicate failure.
 */

void hash_scan_begin(hscan_t *scan, hash_t *hash)
{
    hash_val_t nchains = hash->nchains;
    hash_val_t chain;
    scan->table = hash;

    /* 1 */
    for (chain = 0; chain < nchains && hash->table[chain] == NULL; chain++)
        ;

    if (chain < nchains) {
        /* 2 */
        scan->chain = chain;
        scan->next = hash->table[chain];
    } else {
        /* 3 */
        scan->next = NULL;
    }
}

/*
 * Retrieve the next node from the hash table, and update the pointer
 * for the next invocation of hash_scan_next().
 * Notes:
 * 1. Remember the next pointer in a temporary value so that it can be
 *    returned.
 * 2. This assertion essentially checks whether the module has been properly
 *    initialized. The first point of interaction with the module should be
 *    either hash_create() or hash_init(), both of which set hash_val_t_bit to
 *    a non zero value.
 * 3. If the next pointer we are returning is not NULL, then the user is
 *    allowed to call hash_scan_next() again. We prepare the new next pointer
 *    for that call right now. That way the user is allowed to delete the node
 *    we are about to return, since we will no longer be needing it to locate
 *    the next node.
 * 4. If there is a next node in the chain (next->next), then that becomes the
 *    new next node, otherwise ...
 * 5. We have exhausted the current chain, and must locate the next subsequent
 *    non-empty chain in the table.
 * 6. If a non-empty chain is found, the first element of that chain becomes
 *    the new next node. Otherwise there is no new next node and we set the
 *    pointer to NULL so that the next time hash_scan_next() is called, a null
 *    pointer shall be immediately returned.
 */


hnode_t *hash_scan_next(hscan_t *scan)
{
    hnode_t *next;
    hash_t *hash;
    hash_val_t chain;
    hash_val_t nchains;

    if (scan == NULL || scan->table == NULL || scan->next == NULL) {
        return NULL;
    }

    /* 1 */
    next = scan->next;
    hash = scan->table;
    chain = scan->chain + 1;
    nchains = (hash_val_t) hash->nchains;
    /* 2 */
    assert(hash_val_t_bit != 0);

    /* 3 */
    if (next) {
        /* 4 */
        if (next->next) {
            scan->next = next->next;
        } else {
            /* 5 */
            while (chain < nchains && hash->table[chain] == NULL) {
                chain++;
            }

            /* 6 */
            if (chain < nchains) {
                scan->chain = chain;
                scan->next = hash->table[chain];
            } else {
                scan->next = NULL;
            }
        }
    }

    return next;
}

/*
 * Insert a node into the hash table.
 * Notes:
 * 1. It's illegal to insert more than the maximum number of nodes. The client
 *    should verify that the hash table is not full before attempting an
 *    insertion.
 * 2. The same key may not be inserted into a table twice.
 * 3. If the table is dynamic and the load factor is already at >= 2,
 *    grow the table.
 * 4. We take the bottom N bits of the hash value to derive the chain index,
 *    where N is the base 2 logarithm of the size of the hash table.
 */

void hash_insert(hash_t *hash, hnode_t *node, const void *key)
{
    hash_val_t hkey;
    hash_val_t chain;
    assert(hash_val_t_bit != 0);
    assert(node->next == NULL);
    /* 1 */
    assert(hash->nodecount < hash->maxcount);
    /* 2 */
    assert(hash_lookup(hash, key) == NULL);

    /* 3 */
    if (hash->dynamic && hash->nodecount >= hash->highmark) {
        grow_table(hash);
    }

    hkey = hash->function(key);
    /* 4 */
    chain = hkey & hash->mask;
    node->key = key;
    node->hkey = hkey;
    node->next = hash->table[chain];
    hash->table[chain] = node;
    hash->nodecount++;
    assert(hash_verify(hash));
}

/*
 * Find a node in the hash table and return a pointer to it.
 * Notes:
 * 1. We hash the key and keep the entire hash value. As an optimization, when
 *    we descend down the chain, we can compare hash values first and only if
 *    hash values match do we perform a full key comparison.
 * 2. To locate the chain from among 2^N chains, we look at the lower N bits of
 *    the hash value by anding them with the current mask.
 * 3. Looping through the chain, we compare the stored hash value inside each
 *    node against our computed hash. If they match, then we do a full
 *    comparison between the unhashed keys. If these match, we have located the
 *    entry.
 */

hnode_t *hash_lookup(hash_t *hash, const void *key)
{
    hash_val_t hkey;
    hash_val_t chain;
    /* 1 */
    hkey = hash->function(key);
    /* 2 */
    chain = hkey & hash->mask;

    /* 3 */
    for (hnode_t *nptr = hash->table[chain]; nptr; nptr = nptr->next) {
        if (nptr->hkey == hkey && hash->compare(nptr->key, key) == 0) {
            return nptr;
        }
    }

    return NULL;
}

/*
 * Delete the given node from the hash table.  Since the chains
 * are singly linked, we must locate the start of the node's chain
 * and traverse.
 * Notes:
 * 1. The node must belong to this hash table, and its key must not have
 *    been tampered with.
 * 2. If this deletion will take the node count below the low mark, we
 *    shrink the table now.
 * 3. Determine which chain the node belongs to, and fetch the pointer
 *    to the first node in this chain.
 * 4. If the node being deleted is the first node in the chain, then
 *    simply update the chain head pointer.
 * 5. Otherwise advance to the node's predecessor, and splice out
 *    by updating the predecessor's next pointer.
 * 6. Indicate that the node is no longer in a hash table.
 */

hnode_t *hash_delete(hash_t *hash, hnode_t *node)
{
    hash_val_t chain;
    hnode_t *hptr;
    /* 1 */
    assert(hash_lookup(hash, node->key) == node);
    assert(hash_val_t_bit != 0);

    if (hash->dynamic && hash->nodecount <= hash->lowmark
            && hash->nodecount > INIT_SIZE) {
        /* 2 */
        shrink_table(hash);
    }

    /* 3 */
    chain = node->hkey & hash->mask;
    hptr = hash->table[chain];

    /* 4 */
    if (hptr == node) {
        hash->table[chain] = node->next;
    } else {
        /* 5 */
        while (hptr->next != node) {
            assert(hptr != 0);
            hptr = hptr->next;
        }

        assert(hptr->next == node);
        hptr->next = node->next;
    }

    hash->nodecount--;
    assert(hash_verify(hash));
    /* 6 */
    node->next = NULL;
    return node;
}

int hash_alloc_insert(hash_t *hash, const void *key, void *data)
{
    hnode_t *node = hash->allocnode(hash->context);

    if (node) {
        hnode_init(node, data);
        hash_insert(hash, node, key);
        return 1;
    }

    return 0;
}

void hash_delete_free(hash_t *hash, hnode_t *node)
{
    hash_delete(hash, node);
    hash->freenode(node, hash->context);
}

/*
 *  Exactly like hash_delete, except does not trigger table shrinkage. This is to be
 *  used from within a hash table scan operation. See notes for hash_delete.
 */

hnode_t *hash_scan_delete(hash_t *hash, hnode_t *node)
{
    hash_val_t chain;
    hnode_t *hptr;
    assert(hash_lookup(hash, node->key) == node);
    assert(hash_val_t_bit != 0);
    chain = node->hkey & hash->mask;
    hptr = hash->table[chain];

    if (hptr == node) {
        hash->table[chain] = node->next;
    } else {
        while (hptr->next != node) {
            hptr = hptr->next;
        }

        hptr->next = node->next;
    }

    hash->nodecount--;
    assert(hash_verify(hash));
    node->next = NULL;
    return node;
}

/*
 * Like hash_delete_free but based on hash_scan_delete.
 */

void hash_scan_delfree(hash_t *hash, hnode_t *node)
{
    hash_scan_delete(hash, node);
    hash->freenode(node, hash->context);
}

/*
 * Verify whether the given object is a valid hash table. This means
 * Notes:
 * 1. If the hash table is dynamic, verify whether the high and
 *    low expansion/shrinkage thresholds are powers of two.
 * 2. Count all nodes in the table, and test each hash value
 *    to see whether it is correct for the node's chain.
 */

int hash_verify(hash_t *hash)
{
    hashcount_t count = 0;
    hnode_t *hptr;

    /* 1 */
    if (hash->dynamic) {
        if (hash->lowmark >= hash->highmark) {
            return 0;
        }

        if (!is_power_of_two(hash->highmark)) {
            return 0;
        }

        if (!is_power_of_two(hash->lowmark)) {
            return 0;
        }
    }

    /* 2 */
    for (hash_val_t chain = 0; chain < hash->nchains; chain++) {
        for (hptr = hash->table[chain]; hptr != NULL; hptr = hptr->next) {
            if ((hptr->hkey & hash->mask) != chain) {
                return 0;
            }

            count++;
        }
    }

    if (count != hash->nodecount) {
        return 0;
    }

    return 1;
}

static hnode_t *hnode_alloc(void *context _U_)
{
    return malloc(sizeof(*hnode_alloc(NULL)));
}

static void hnode_free(hnode_t *node, void *context _U_)
{
    free(node);
}

/*
 * Initialize a client-supplied node
 */

hnode_t *hnode_init(hnode_t *hnode, void *data)
{
    hnode->data = data;
    hnode->next = NULL;
    return hnode;
}

#undef hnode_get
void *hnode_get(hnode_t *node)
{
    return node->data;
}

#undef hnode_getkey
const void *hnode_getkey(hnode_t *node)
{
    return node->key;
}

#undef hash_count
hashcount_t hash_count(hash_t *hash)
{
    return hash->nodecount;
}

#undef hash_size
hashcount_t hash_size(hash_t *hash)
{
    return hash->nchains;
}

static hash_val_t hash_fun_default(const void *key)
{
    static unsigned long randbox[] = {
        0x49848f1bU, 0xe6255dbaU, 0x36da5bdcU, 0x47bf94e9U,
        0x8cbcce22U, 0x559fc06aU, 0xd268f536U, 0xe10af79aU,
        0xc1af4d69U, 0x1d2917b5U, 0xec4c304dU, 0x9ee5016cU,
        0x69232f74U, 0xfead7bb3U, 0xe9089ab6U, 0xf012f6aeU,
    };
    const unsigned char *str = key;
    hash_val_t acc = 0;

    while (*str) {
        acc ^= randbox[(*str + acc) & 0xf];
        acc = (acc << 1) | (acc >> 31);
        acc &= 0xffffffffU;
        acc ^= randbox[((*str++ >> 4) + acc) & 0xf];
        acc = (acc << 2) | (acc >> 30);
        acc &= 0xffffffffU;
    }

    return acc;
}

/* From http://www.azillionmonkeys.com/qed/hash.html */
#undef get16bits
#if defined(__i386__)
#define get16bits(d) (*((const uint16_t *) (d)))
#else
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)    \
                      +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

static int hash_comp_default(const void *key1, const void *key2)
{
    return strcmp(key1, key2);
}
