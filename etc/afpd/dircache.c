/*
  $Id: dircache.c,v 1.1.2.4 2010-02-05 10:27:58 franklahm Exp $
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include <atalk/util.h>
#include <atalk/cnid.h>
#include <atalk/logger.h>
#include <atalk/volume.h>
#include <atalk/directory.h>
#include <atalk/queue.h>
#include <atalk/bstrlib.h>
#include <atalk/bstradd.h>

#include "dircache.h"
#include "directory.h"
#include "hash.h"
#include "globals.h"

/*
 * Dircache and indexes
 * ====================
 * The maximum dircache size is:
 * max(DEFAULT_MAX_DIRCACHE_SIZE, min(size, MAX_POSSIBLE_DIRCACHE_SIZE)).
 * It is a hashtable which we use to store "struct dir"s in. If the cache get full, oldest
 * entries are evicted in chunks of DIRCACHE_FREE.
 * We have/need two indexes:
 * - a DID/name index on the main dircache, another hashtable
 * - a queue index on the dircache, for evicting the oldest entries
 * The cache supports locking of struct dir elements through the DIRF_CACHELOCK flag. A dir
 * locked this way wont ever be removed from the cache, so be careful.
 */

/********************************************************
 * Local funcs and variables
 ********************************************************/

/*****************************
 *       THE dircache        */

static hash_t       *dircache;        /* The actual cache */
static unsigned int dircache_maxsize; /* cache maximum size */

/* FNV 1a */
static hash_val_t hash_vid_did(const void *key)
{
    const struct dir *k = (const struct dir *)key;
    hash_val_t hash = 2166136261;

    hash ^= k->d_vid >> 8;
    hash *= 16777619;
    hash ^= k->d_vid;
    hash *= 16777619;

    hash ^= k->d_did >> 24;
    hash *= 16777619;
    hash ^= (k->d_did >> 16) & 0xff;
    hash *= 16777619;
    hash ^= (k->d_did >> 8) & 0xff;
    hash *= 16777619;
    hash ^= (k->d_did >> 0) & 0xff;
    hash *= 16777619;

    return hash;
}

static int hash_comp_vid_did(const void *key1, const void *key2)
{
    const struct dir *k1 = key1;
    const struct dir *k2 = key2;

    return !(k1->d_did == k2->d_did && k1->d_vid == k2->d_vid);
}

/**************************************************
 * DID/name index on dircache (another hashtable) */

static hash_t *index_didname;

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__)    \
    || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)    \
                      +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

static hash_val_t hash_didname(const void *p)
{
    const struct dir *key = (const struct dir *)p;
    const unsigned char *data = key->d_u_name->data;
    int len = key->d_u_name->slen;
    hash_val_t hash = key->d_pdid + key->d_vid;
    hash_val_t tmp;

    int rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
    case 3: hash += get16bits (data);
        hash ^= hash << 16;
        hash ^= data[sizeof (uint16_t)] << 18;
        hash += hash >> 11;
        break;
    case 2: hash += get16bits (data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1: hash += *data;
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

static int hash_comp_didname(const void *k1, const void *k2)
{
    const struct dir *key1 = (const struct dir *)k1;
    const struct dir *key2 = (const struct dir *)k2;

    return ! (key1->d_vid == key2->d_vid
              && key1->d_pdid == key2->d_pdid
              && (bstrcmp(key1->d_u_name, key2->d_u_name) == 0) );
}

/***************************
 * queue index on dircache */

static queue_t *index_queue;    /* the index itself */
static unsigned int queue_count;
static const int dircache_free_quantum = 256; /* number of entries to free */

/*!
 * @brief Remove a fixed number of (oldest) entries from the cache and indexes
 *
 * The default is to remove the 256 oldest entries from the cache.
 * 1. Get the oldest entry
 * 2. If it's in use ie open forks reference it or it's curdir requeue it,
 *    or it's locked (from catsearch) dont remove it
 * 3. Remove the dir from the main cache and the didname index
 * 4. Free the struct dir structure and all its members
 */
static void dircache_evict(void)
{
    int i = dircache_free_quantum;
    struct dir *dir;

    LOG(log_debug, logtype_afpd, "dircache: {starting cache eviction}");

    while (i--) {
        if ((dir = (struct dir *)dequeue(index_queue)) == NULL) { /* 1 */
            dircache_dump();
            exit(EXITERR_SYS);
        }
        queue_count--;

        if (curdir == dir
            || dir->d_ofork
            || (dir->d_flags & DIRF_CACHELOCK)) {     /* 2 */
            if ((dir->qidx_node = enqueue(index_queue, dir)) == NULL) {
                dircache_dump();
                exit(EXITERR_SYS);
            }
            queue_count++;
            continue;
        }

        dircache_remove(NULL, dir, DIRCACHE | DIDNAME_INDEX); /* 3 */
        dir_free(dir);                                        /* 4 */
    }

    assert(queue_count == dircache->hash_nodecount);
    assert(queue_count + dircache_free_quantum <= dircache_maxsize);

    LOG(log_debug, logtype_afpd, "dircache: {finished cache eviction}");
}


/********************************************************
 * Interface
 ********************************************************/

/*!
 * @brief Search the dircache via a DID
 *
 * @param vol    (r) pointer to struct vol
 * @param did    (r) CNID of the directory
 *
 * @returns Pointer to struct dir if found, else NULL
 */
struct dir *dircache_search_by_did(const struct vol *vol, cnid_t did)
{
    struct dir *cdir = NULL;
    struct dir key;
    hnode_t *hn;

    assert(vol);
    assert(ntohl(did) >= CNID_START);

    key.d_vid = vol->v_vid;
    key.d_did = did;
    if ((hn = hash_lookup(dircache, &key)))
        cdir = hnode_get(hn);

    if (cdir)
        LOG(log_debug, logtype_afpd, "dircache(did:%u): {cached: path:'%s'}", ntohl(did), cfrombstring(cdir->d_fullpath));
    else
        LOG(log_debug, logtype_afpd, "dircache(did:%u): {not in cache}", ntohl(did));

    return cdir;
}

/*!
 * @brief Search the cache via did/name hashtable
 *
 * @param vol    (r) volume
 * @param dir    (r) directory
 * @param name   (r) name (server side encoding)
 * @parma len    (r) strlen of name
 *
 * @returns pointer to struct dir if found in cache, else NULL
 */
struct dir *dircache_search_by_name(const struct vol *vol, const struct dir *dir, char *name, int len)
{
    struct dir *cdir = NULL;
    struct dir key;
    hnode_t *hn;
    static_bstring uname = {-1, len, (unsigned char *)name};

    assert(vol);
    assert(dir);
    assert(name);
    assert(len == strlen(name));
    assert(len < 256);

    if (dir->d_did != DIRDID_ROOT_PARENT) {
        key.d_vid = vol->v_vid;
        key.d_pdid = dir->d_did;
        key.d_u_name = &uname;

        if ((hn = hash_lookup(index_didname, &key)))
            cdir = hnode_get(hn);
    }

    if (cdir)
        LOG(log_debug, logtype_afpd, "dircache(pdid:%u, did:%u, '%s'): {found in cache}",
            ntohl(dir->d_did), ntohl(cdir->d_did), cfrombstring(cdir->d_fullpath));
    else
        LOG(log_debug, logtype_afpd, "dircache(pdid:%u,'%s/%s'): {not in cache}",
            ntohl(dir->d_did), cfrombstring(dir->d_fullpath), name);

    return cdir;
}

/*!
 * @brief create struct dir from struct path
 *
 * Add a struct dir to the cache and its indexes.
 *
 * @param dir   (r) pointer to parrent directory
 *
 * @returns 0 on success, -1 on error which should result in an abort
 */
int dircache_add(struct dir *dir)
{
    assert(dir);
    assert(ntohl(dir->d_pdid) >= 2);
    assert(ntohl(dir->d_did) >= CNID_START);
    assert(dir->d_fullpath);
    assert(dir->d_u_name);
    assert(dir->d_vid);
    assert(dircache->hash_nodecount <= dircache_maxsize);

    /* Check if cache is full */
    if (dircache->hash_nodecount == dircache_maxsize)
        dircache_evict();

    /* Add it to the main dircache */
    if (hash_alloc_insert(dircache, dir, dir) == 0) {
        dircache_dump();
        exit(EXITERR_SYS);
    }

    /* Add it to the did/name index */
    if (hash_alloc_insert(index_didname, dir, dir) == 0) {
        dircache_dump();
        exit(EXITERR_SYS);
    }

    /* Add it to the fifo queue index */
    if ((dir->qidx_node = enqueue(index_queue, dir)) == NULL) {
        dircache_dump();
        exit(EXITERR_SYS);
    } else {
        queue_count++;
    }

    LOG(log_debug, logtype_afpd, "dircache(did:%u,'%s'): {added}", ntohl(dir->d_did), cfrombstring(dir->d_fullpath));

    assert(queue_count == index_didname->hash_nodecount 
           && queue_count == dircache->hash_nodecount);

    return 0;
}

/*!
  * @brief Remove an entry from the dircache
  *
  * FIXME what about opened forks with refs to it?
  * it's an afp specs violation because you can't delete
  * an opened forks. Now afpd doesn't care about forks opened by other
  * process. It's fixable within afpd if fnctl_lock, doable with smb and
  * next to impossible for nfs and local filesystem access.
  */
void dircache_remove(const struct vol *vol _U_, struct dir *dir, int flags)
{
    hnode_t *hn;

    assert(dir);
    assert((flags & ~(QUEUE_INDEX | DIDNAME_INDEX | DIRCACHE)) == 0);

    if (dir->d_flags & DIRF_CACHELOCK)
        return;

    if (flags & QUEUE_INDEX) {
        /* remove it from the queue index */
        dequeue(dir->qidx_node->prev); /* this effectively deletes the dequeued node */
        queue_count--;
    }

    if (flags & DIDNAME_INDEX) {
        if ((hn = hash_lookup(index_didname, dir)) == NULL) {
            LOG(log_error, logtype_default, "dircache_remove(%u,%s): not in didname index", 
                ntohl(dir->d_did), cfrombstring(dir->d_fullpath));
            dircache_dump();
            exit(EXITERR_SYS);
        }
        hash_delete(index_didname, hn);
    }

    if (flags & DIRCACHE) {
        if ((hn = hash_lookup(dircache, dir)) == NULL) {
            LOG(log_error, logtype_default, "dircache_remove(%u,%s): not in dircache", 
                ntohl(dir->d_did), cfrombstring(dir->d_fullpath));
            dircache_dump();
            exit(EXITERR_SYS);
        }
        hash_delete(dircache, hn);
    }

    LOG(log_debug, logtype_afpd, "dircache(did:%u,'%s'): {removed}", ntohl(dir->d_did), cfrombstring(dir->d_fullpath));

    assert(queue_count == index_didname->hash_nodecount 
           && queue_count == dircache->hash_nodecount);
}

/*!
 * @brief Initialize the dircache and indexes
 *
 * This is called in child afpd initialisation. The maximum cache size will be
 * max(DEFAULT_MAX_DIRCACHE_SIZE, min(size, MAX_POSSIBLE_DIRCACHE_SIZE)).
 * It initializes a hashtable which we use to store a directory cache in.
 * It also initializes two indexes:
 * - a DID/name index on the main dircache
 * - a queue index on the dircache
 *
 * @param size   (r) requested maximum size from afpd.conf
 *
 * @return 0 on success, -1 on error
 */
int dircache_init(int reqsize)
{
    dircache_maxsize = DEFAULT_MAX_DIRCACHE_SIZE;

    /* Initialize the main dircache */
    if (reqsize > DEFAULT_MAX_DIRCACHE_SIZE && reqsize < MAX_POSSIBLE_DIRCACHE_SIZE) {
        while ((dircache_maxsize < MAX_POSSIBLE_DIRCACHE_SIZE) && (dircache_maxsize < reqsize))
               dircache_maxsize *= 2;
    }
    if ((dircache = hash_create(dircache_maxsize, hash_comp_vid_did, hash_vid_did)) == NULL)
        return -1;
    
    LOG(log_debug, logtype_afpd, "dircache_init: done. max dircache size: %u", dircache_maxsize);

    /* Initialize did/name index hashtable */
    if ((index_didname = hash_create(dircache_maxsize, hash_comp_didname, hash_didname)) == NULL)
        return -1;

    /* Initialize index queue */
    if ((index_queue = queue_init()) == NULL)
        return -1;
    else
        queue_count = 0;

    /* As long as directory.c hasn't got its own initializer call, we do it for it */
    rootParent.d_did = DIRDID_ROOT_PARENT;
    rootParent.d_fullpath = bfromcstr("");
    rootParent.d_u_name = bfromcstr("");
    rootParent.d_m_name = bfromcstr("");

    return 0;
}

/*!
 * @brief Dump dircache to /tmp/dircache.PID
 */
void dircache_dump(void)
{
    char tmpnam[64];
    FILE *dump;
    qnode_t *n = index_queue->next;
    const struct dir *dir;

    LOG(log_warning, logtype_afpd, "Dumping directory cache...");

    sprintf(tmpnam, "/tmp/dircache.%u", getpid());
    if ((dump = fopen(tmpnam, "w+")) == NULL) {
        LOG(log_error, logtype_afpd, "dircache_dump: %s", strerror(errno));
        return;
    }
    setbuf(dump, NULL);

    fprintf(dump, "Number of cache entries: %u\n", queue_count);
    fprintf(dump, "Configured maximum cache size: %u\n", dircache_maxsize);
    fprintf(dump, "==================================================\n\n");

    for (int i = 1; i <= queue_count; i++) {
        if (n == index_queue)
            break;
        dir = (struct dir *)n->data;
        fprintf(dump, "%05u: vid:%u, pdid:%6u, did:%6u, path:%s, locked:%s\n",
                i, ntohs(dir->d_vid), ntohl(dir->d_pdid), ntohl(dir->d_did), cfrombstring(dir->d_fullpath),
                (dir->d_flags & DIRF_CACHELOCK) ? "yes" : "no");
        n = n->next;
    }

    fprintf(dump, "\n");
    return;
}
