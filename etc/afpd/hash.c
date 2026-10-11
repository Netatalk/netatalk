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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

/* assert() follows the b_ndebug build option; hash_insert() asserts key
 * uniqueness */
#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#define HASH_IMPLEMENTATION
#include "hash.h"

/* hash_verify() walks every chain and node, so asserting it per mutation
 * turns O(1) operations into O(table); confine it to debug buildtype and
 * leave the O(chain) asserts to b_ndebug */
#ifdef DEBUG
#define HASH_ASSERT_VERIFY(h) assert(hash_verify(h))
#else
#define HASH_ASSERT_VERIFY(h) ((void)0)
#endif

#ifdef KAZLIB_RCSID
#endif

/* Initial hash table INIT_SIZE = 1024 (matches MIN_DIRCACHE_SIZE) */
#define INIT_BITS   10
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

/*!
 * @brief Compute the number of bits in the hash_val_t type.
 *
 * We know that hash_val_t is an unsigned integral type.
 * Thus the highest value it can hold is a Mersenne number (power of two, less one).
 * We initialize a hash_val_t object with this value
 * and then shift bits out one by one while counting.
 * @note
 * 1. HASH_VAL_T_MAX is a Mersenne number---one that is one less than a power
 *    of two. This means that its binary representation consists of all one
 *    bits, and hence 'val' is initialized to all one bits.
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

/*!
 * @brief Test whether a value is a power of two
 *
 * @param[in] arg  value to test
 * @returns 1 for a power of two, 0 otherwise, including for 0
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

/*!
 * @brief Set every chain head of a hash table to NULL
 *
 * @param[in,out] hash  hash table
 */
static void clear_table(hash_t *hash)
{
    hash_val_t i;

    for (i = 0; i < hash->nchains; i++) {
        hash->table[i] = NULL;
    }
}

/*!
 * @brief Double the number of chains of a dynamic hash table
 *
 * Each chain splits into itself and a sister chain in the upper half. The
 * mask widens by 1 bit. That bit of the stored hash value then picks the
 * chain of each node, so no node needs a new hash. A failed realloc leaves
 * the table as it was.
 *
 * 1. Assert that doubling the chain count does not overflow.
 * 2. The new chain count is twice the old one.
 * 3. The new mask is 1 bit wider and exposes 1 more bit of each hash value.
 * 4. Reallocate the chain table at twice its size.
 * 5. A failed realloc changes nothing.
 * 6. The exposed_bit mask selects the new bit of each hash value.
 * 7. Sort the nodes of each chain by that bit.
 * 8. Nodes with the bit clear stay in the chain, and the others form the
 *    sister chain.
 * 9. Update the mask, the chain count, and the marks.
 *
 * @param[in,out] hash  dynamic hash table
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

    HASH_ASSERT_VERIFY(hash);
}

/*!
 * @brief Halve the number of chains of a dynamic hash table
 *
 * Each chain of the second half joins the end of its sister in the first
 * half. The chain table then shrinks to its first half.
 *
 * 1. Assert at least 2 chains, so the low mark stays above 0.
 * 2. Take the head of each chain of the first half and of its sister.
 * 3. Find the tail of the first chain, NULL for an empty one.
 * 4. Append the sister chain to a first chain that has nodes.
 * 5. Move the sister chain into an empty slot.
 * 6. Two empty chains need nothing.
 * 7. Reallocate the chain table at half its size.
 * 8. A failed realloc keeps the old allocation, which still works.
 * 9. Update the mask, the chain count, and the marks.
 *
 * @param[in,out] hash  dynamic hash table with at least 2 chains
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
    HASH_ASSERT_VERIFY(hash);
}


/*!
 * @brief Create a dynamic hash table
 *
 * The table starts with INIT_SIZE chains and doubles when the node count
 * reaches twice the chain count. A deletion halves it at half the chain
 * count, but never while the node count is at or below INIT_SIZE.
 *
 * 1. Count the bits of hash_val_t on the first call.
 * 2. Allocate the control structure.
 * 3. Return NULL when that allocation fails.
 * 4. Allocate the chain table.
 * 5. Free the control structure when the chain allocation fails.
 * 6. Set the marks to twice and half the chain count.
 * 7. Mark the table dynamic, so it resizes.
 * 8. Clear the chain heads.
 *
 * @param[in] maxcount  maximum number of nodes
 * @param[in] compfun   compare function, or NULL for strcmp
 * @param[in] hashfun   hash function, or NULL for the default string hash
 * @returns the table, or NULL when an allocation fails
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
            HASH_ASSERT_VERIFY(hash);
            return hash;
        }

        free(hash);
    }

    return NULL;
}

/*!
 * @brief Replace the node allocator of an empty hash table
 *
 * Both functions must be set, or both NULL for the defaults.
 *
 * @param[in,out] hash     empty hash table
 * @param[in]     al       node allocator, or NULL
 * @param[in]     fr       node release function, or NULL
 * @param[in]     context  value passed to both functions
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

/*!
 * @brief Free every node of a hash table through its freenode function
 *
 * The table stays allocated and empty.
 *
 * @param[in,out] hash  hash table
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

/*!
 * @brief Free an empty dynamic hash table and its chain table
 *
 * @param[in] hash  hash table without nodes
 */
void hash_destroy(hash_t *hash)
{
    assert(hash_val_t_bit != 0);
    assert(hash_isempty(hash));
    free(hash->table);
    free(hash);
}

/*!
 * @brief Start a scan of a hash table at its first node
 *
 * 1. Find the first chain with a node.
 * 2. Point the scan at the head of that chain.
 * 3. An empty table leaves the scan with no next node, so hash_scan_next
 *    returns NULL.
 *
 * @param[out] scan  scanner to start
 * @param[in]  hash  hash table
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

/*!
 * @brief Return the next node of a scan and advance the scan past it
 *
 * The scan moves on before the node returns, so the caller may unlink the
 * node with hash_scan_delete.
 *
 * 1. Keep the node to return.
 * 2. Assert that a table creation counted the bits of hash_val_t.
 * 3. Advance only past a real node.
 * 4. Take the next node of the same chain when there is one.
 * 5. Otherwise look for the next chain with a node.
 * 6. Take the head of that chain, or end the scan with NULL.
 *
 * @param[in,out] scan  scanner from hash_scan_begin
 * @returns the next node, or NULL at the end of the table
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

/*!
 * @brief Insert a node under a new @p key, ahead of the rest of its chain
 *
 * 1. Assert that the table has room under maxcount.
 * 2. Assert that @p key is not in the table yet.
 * 3. Grow a dynamic table whose node count reached highmark.
 * 4. The low bits of the hash value under the mask pick the chain.
 *
 * @param[in,out] hash  hash table
 * @param[in,out] node  node with no chain link
 * @param[in]     key   value to find the node by, kept by pointer
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
    HASH_ASSERT_VERIFY(hash);
}

/*!
 * @brief Find the node of @p key in a hash table
 *
 * 1. Hash @p key once and keep the full value.
 * 2. The low bits of the hash value under the mask pick the chain.
 * 3. Compare the stored hash values first, and call the compare function only
 *    on a match.
 *
 * @param[in] hash  hash table
 * @param[in] key   value to find
 * @returns the node, or NULL when no node matches
 */
inline hnode_t *hash_lookup(hash_t *hash, const void *key)
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

/*!
 * @brief Unlink a node from a hash table
 *
 * The chains link one way, so the call walks the chain of the node to find
 * its predecessor. A dynamic table halves first when the node count is at or
 * below the low mark and above INIT_SIZE.
 *
 * 1. Assert that hash_lookup still finds the node.
 * 2. Shrink the table before the unlink.
 * 3. Find the chain of the node from its stored hash value.
 * 4. Replace the chain head when the node is the head.
 * 5. Otherwise unlink the node from its predecessor.
 * 6. Clear the link of the node, so it belongs to no table.
 *
 * @param[in,out] hash  hash table
 * @param[in,out] node  node in the table
 * @returns node
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
    HASH_ASSERT_VERIFY(hash);
    /* 6 */
    node->next = NULL;
    return node;
}

/*!
 * @brief Allocate a node for @p data and insert it under @p key
 *
 * @param[in,out] hash  hash table
 * @param[in]     key   value to find the node by, kept by pointer
 * @param[in]     data  value of the node
 * @returns 1, or 0 when the node allocation fails
 */
int hash_alloc_insert(hash_t *hash, const void *key, void *data)
{
    return hash_alloc_insert_node(hash, key, data) != NULL;
}

/*!
 * @brief Allocate a node for @p data, insert it under @p key, and return it
 *
 * A caller that keeps the node can unlink it later without a lookup.
 *
 * @param[in,out] hash  hash table
 * @param[in]     key   value to find the node by, kept by pointer
 * @param[in]     data  value of the node
 * @returns the node, or NULL when the allocation fails
 */
hnode_t *hash_alloc_insert_node(hash_t *hash, const void *key, void *data)
{
    hnode_t *node = hash->allocnode(hash->context);

    if (node) {
        hnode_init(node, data);
        hash_insert(hash, node, key);
    }

    return node;
}

/*!
 * @brief Unlink a node through hash_delete and free it through the freenode
 * function
 *
 * @param[in,out] hash  hash table
 * @param[in]     node  node in the table
 */
void hash_delete_free(hash_t *hash, hnode_t *node)
{
    hash_delete(hash, node);
    hash->freenode(node, hash->context);
}

/*!
 * @brief Unlink a node like hash_delete, but never shrink the table
 *
 * A scan may unlink the node that hash_scan_next just returned, because the
 * chain table keeps its size.
 *
 * @param[in,out] hash  hash table
 * @param[in,out] node  node in the table
 * @returns node
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
    HASH_ASSERT_VERIFY(hash);
    node->next = NULL;
    return node;
}

/*!
 * @brief Unlink a node through hash_scan_delete and free it through the
 * freenode function
 *
 * @param[in,out] hash  hash table
 * @param[in]     node  node in the table
 */
void hash_scan_delfree(hash_t *hash, hnode_t *node)
{
    hash_scan_delete(hash, node);
    hash->freenode(node, hash->context);
}

/*!
 * @brief Check the invariants of a hash table
 *
 * Debug builds assert it after each change of a table.
 *
 * 1. A dynamic table needs lowmark under highmark, both powers of two.
 * 2. Each node must sit in the chain that its stored hash value picks, and
 *    the node count must match.
 *
 * @param[in] hash  hash table
 * @returns 1 for a valid table, 0 otherwise
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

/*!
 * @brief Allocate a node with malloc, the default node allocator
 *
 * @param[in] context  unused
 * @returns the node, or NULL when malloc fails
 */
static hnode_t *hnode_alloc(void *context _U_)
{
    return malloc(sizeof(*hnode_alloc(NULL)));
}

/*!
 * @brief Free a node with free, the default node release function
 *
 * @param[in] node     node to free
 * @param[in] context  unused
 */
static void hnode_free(hnode_t *node, void *context _U_)
{
    free(node);
}

/*!
 * @brief Set up a node that the caller allocated
 *
 * @param[out] hnode  node to set up
 * @param[in]  data   value of the node
 * @returns hnode, with data stored and no chain link
 */
hnode_t *hnode_init(hnode_t *hnode, void *data)
{
    hnode->data = data;
    hnode->next = NULL;
    return hnode;
}

#undef hnode_get
/*!
 * @brief Return the value of a node
 *
 * @param[in] node  hash node
 * @returns the data pointer of the node
 */
void *hnode_get(hnode_t *node)
{
    return node->data;
}

#undef hnode_getkey
/*!
 * @brief Return the lookup value that hash_insert stored in a node
 *
 * @param[in] node  hash node
 * @returns the hash_key pointer of the node
 */
const void *hnode_getkey(hnode_t *node)
{
    return node->key;
}

#undef hash_count
/*!
 * @brief Return the number of nodes in a hash table
 *
 * @param[in] hash  hash table
 * @returns the node count
 */
hashcount_t hash_count(hash_t *hash)
{
    return hash->nodecount;
}

#undef hash_size
/*!
 * @brief Return the number of chains of a hash table
 *
 * @param[in] hash  hash table
 * @returns the chain count
 */
hashcount_t hash_size(hash_t *hash)
{
    return hash->nchains;
}

/*!
 * @brief Hash a NUL-terminated string, the default hash function
 *
 * Each byte mixes in 2 entries of a 16-entry random table, and the
 * accumulator rotates after each entry and stays within 32 bits.
 *
 * @param[in] key  string to hash
 * @returns the 32-bit hash value
 */
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

/*!
 * @brief Compare two strings with strcmp, the default compare function
 *
 * @param[in] key1  first string
 * @param[in] key2  second string
 * @returns the strcmp result, 0 for equal strings
 */
static int hash_comp_default(const void *key1, const void *key2)
{
    return strcmp(key1, key2);
}
