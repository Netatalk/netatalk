/*
 * $Id: ofork.c,v 1.19 2002-09-06 02:57:49 didg Exp $
 *
 * Copyright (c) 1996 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> /* works around a bug */
#include <sys/param.h>
#include <atalk/logger.h>
#include <errno.h>

#include <atalk/adouble.h>

#include "globals.h"
#include "volume.h"
#include "directory.h"
#include "fork.h"

/* we need to have a hashed list of oforks (by dev inode). just hash
 * by first letter. */
#define OFORK_HASHSIZE  64
static struct ofork     *ofork_table[OFORK_HASHSIZE];

static struct ofork	**oforks = NULL;
static int	        nforks = 0;
static u_short		lastrefnum = 0;


/* OR some of each character for the hash*/
static __inline__ unsigned long hashfn(const struct file_key *key)
{
#if 0
    unsigned long i = 0;
    while (*name) {
        i = ((i << 4) | (8*sizeof(i) - 4)) ^ *name++;
    }
#endif    
    return key->inode & (OFORK_HASHSIZE - 1);
}

static __inline__ void of_hash(struct ofork *of)
{
    struct ofork **table;

    table = &ofork_table[hashfn(&of->key)];
    if ((of->next = *table) != NULL)
        (*table)->prevp = &of->next;
    *table = of;
    of->prevp = table;
}

static __inline__ void of_unhash(struct ofork *of)
{
    if (of->prevp) {
        if (of->next)
            of->next->prevp = of->prevp;
        *(of->prevp) = of->next;
    }
}

void of_pforkdesc( f )
FILE	*f;
{
    int	ofrefnum;

    if (!oforks)
        return;

    for ( ofrefnum = 0; ofrefnum < nforks; ofrefnum++ ) {
        if ( oforks[ ofrefnum ] != NULL ) {
            fprintf( f, "%hu <%s>\n", ofrefnum, oforks[ ofrefnum ]->of_name);
        }
    }
}

int of_flush(const struct vol *vol)
{
    int	refnum;

    if (!oforks)
        return 0;

    for ( refnum = 0; refnum < nforks; refnum++ ) {
        if (oforks[ refnum ] != NULL && (oforks[refnum]->of_vol == vol) &&
                flushfork( oforks[ refnum ] ) < 0 ) {
            LOG(log_error, logtype_afpd, "of_flush: %s", strerror(errno) );
        }
    }
    return( 0 );
}

int of_rename(vol, s_of, olddir, oldpath, newdir, newpath)
const struct vol *vol;
struct ofork *s_of;
struct dir *olddir, *newdir;
const char *oldpath, *newpath;
{
    struct ofork *of, *next, *d_ofork;

    if (!s_of)
        return AFP_OK;
        
    next = ofork_table[hashfn(&s_of->key)];
    while ((of = next)) {
        next = next->next; /* so we can unhash and still be all right. */

        if (vol == of->of_vol && olddir == of->of_dir &&
	         s_of->key.dev == of->key.dev && 
	         s_of->key.inode == of->key.inode ) {
            strncpy( of->of_name, newpath, of->of_namelen);
            if (newdir != olddir) {
                of->of_d_prev->of_d_next = of->of_d_next;
                of->of_d_next->of_d_prev = of->of_d_prev;
                if (of->of_dir->d_ofork == of) {
                    of->of_dir->d_ofork = (of == of->of_d_next) ? NULL : of->of_d_next;
                }	    
                of->of_dir = newdir;
                if (!(d_ofork = newdir->d_ofork)) {
                    newdir->d_ofork = of;
                    of->of_d_next = of->of_d_prev = of;
                } else {
                    of->of_d_next = d_ofork;
                    of->of_d_prev = d_ofork->of_d_prev;
                    of->of_d_prev->of_d_next = of;
                    d_ofork->of_d_prev = of;
                }
            }
        }
    }

    return AFP_OK;
}

#define min(a,b)	((a)<(b)?(a):(b))

struct ofork *
            of_alloc(vol, dir, path, ofrefnum, eid, ad, st)
struct vol      *vol;
struct dir	*dir;
char		*path;
u_int16_t	*ofrefnum;
const int       eid;
struct adouble  *ad;
struct stat     *st;
{
    struct ofork        *of, *d_ofork;
    u_int16_t		refnum, of_refnum;

    int			i;

    if (!oforks) {
        nforks = (getdtablesize() - 10) / 2;
	/* protect against insane ulimit -n */
        nforks = min(nforks, 0xffff);
        oforks = (struct ofork **) calloc(nforks, sizeof(struct ofork *));
        if (!oforks)
            return NULL;
    }

    for ( refnum = ++lastrefnum, i = 0; i < nforks; i++, refnum++ ) {
        /* cf AFP3.0.pdf, File fork page 40 */
        if (!refnum)
            refnum++;
        if ( oforks[ refnum % nforks ] == NULL ) {
            break;
        }
    }
    /* grr, Apple and their 'uniquely identifies'
          the next line is a protection against 
          of_alloc()
             refnum % nforks = 3 
             lastrefnum = 3
             oforks[3] != NULL 
             refnum = 4
             oforks[4] == NULL
             return 4
         
          close(oforks[4])
      
          of_alloc()
             refnum % nforks = 4
             ...
             return 4
         same if lastrefnum++ rather than ++lastrefnum. 
    */
    lastrefnum = refnum;
    if ( i == nforks ) {
        LOG(log_error, logtype_afpd, "of_alloc: maximum number of forks exceeded.");
        return( NULL );
    }

    of_refnum = refnum % nforks;
    if (( oforks[ of_refnum ] =
                (struct ofork *)malloc( sizeof( struct ofork ))) == NULL ) {
        LOG(log_error, logtype_afpd, "of_alloc: malloc: %s", strerror(errno) );
        return NULL;
    }
    of = oforks[of_refnum];

    /* see if we need to allocate space for the adouble struct */
    if (!ad) {
        ad = malloc( sizeof( struct adouble ) );
        if (!ad) {
            LOG(log_error, logtype_afpd, "of_alloc: malloc: %s", strerror(errno) );
            return NULL;
        }

        /* initialize to zero. This is important to ensure that
           ad_open really does reinitialize the structure. */
        memset( ad, 0, sizeof( struct adouble ) );
    } else {
        /* Increase the refcount on this struct adouble. This is
           decremented again in oforc_dealloc. */
        ad->ad_refcount++;
    }

    of->of_ad = ad;

    of->of_vol = vol;
    of->of_dir = dir;

    if (!(d_ofork = dir->d_ofork)) {
        dir->d_ofork = of;
        of->of_d_next = of->of_d_prev = of;
    } else {
        of->of_d_next = d_ofork;
        of->of_d_prev = d_ofork->of_d_prev;
        d_ofork->of_d_prev->of_d_next = of;
        d_ofork->of_d_prev = of;
    }

    /* here's the deal: we allocate enough for the standard mac file length.
     * in the future, we'll reallocate in fairly large jumps in case
     * of long unicode names */
    if (( of->of_name =(char *)malloc(255 + 1)) == NULL ) {
        LOG(log_error, logtype_afpd, "of_alloc: malloc: %s", strerror(errno) );
        if (!ad)
            free(of->of_ad);
        free(of);
        oforks[ of_refnum ] = NULL;
        return NULL;
    }
    strncpy( of->of_name, path, of->of_namelen = 255 + 1);
    *ofrefnum = refnum;
    of->of_refnum = refnum;
    of->key.dev = st->st_dev;
    of->key.inode = st->st_ino;
    if (eid == ADEID_DFORK)
        of->of_flags = AFPFORK_DATA;
    else
        of->of_flags = AFPFORK_RSRC;

    of_hash(of);
    return( of );
}

struct ofork *of_find(const u_int16_t ofrefnum )
{
    if (!oforks || !nforks)
        return NULL;

    return( oforks[ ofrefnum % nforks ] );
}

/* --------------------------
*/
struct ofork *
            of_findname(const char *name, struct stat *st)
{
    struct ofork *of;
    struct file_key key;
    struct stat buffer;
    char   *p;
    
    if (st == NULL) {
        st = &buffer;
        if (stat(name, st) < 0)
            return NULL;
    }
    key.dev = st->st_dev;
    key.inode = st->st_ino;

    for (of = ofork_table[hashfn(&key)]; of; of = of->next) {
        if (key.dev == of->key.dev && key.inode == of->key.inode ) {
            return of;
        }
    }

    return NULL;
}

void of_dealloc( of )
struct ofork	*of;
{
    if (!oforks)
        return;

    of_unhash(of);

    /* detach ofork */
    of->of_d_prev->of_d_next = of->of_d_next;
    of->of_d_next->of_d_prev = of->of_d_prev;
    if (of->of_dir->d_ofork == of) {
        of->of_dir->d_ofork = (of == of->of_d_next) ? NULL : of->of_d_next;
    }

    oforks[ of->of_refnum % nforks ] = NULL;
    free( of->of_name );

    /* decrease refcount */
    of->of_ad->ad_refcount--;

    if ( of->of_ad->ad_refcount <= 0) {
        free( of->of_ad);
    } else {/* someone's still using it. just free this user's locks */
        ad_unlock(of->of_ad, of->of_refnum);
    }

    free( of );
}
