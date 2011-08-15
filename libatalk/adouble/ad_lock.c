/* 
 * Copyright (c) 1998,1999 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT for more information.
 *
 * Byte-range locks. This uses either whole-file flocks to fake byte
 * locks or fcntl-based actual byte locks. Because fcntl locks are
 * process-oriented, we need to keep around a list of file descriptors
 * that refer to the same file. Currently, this doesn't serialize access 
 * to the locks. as a result, there's the potential for race conditions. 
 *
 * TODO: fix the race when reading/writing.
 *       keep a pool of both locks and reference counters around so that
 *       we can save on mallocs. we should also use a tree to keep things
 *       sorted.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <atalk/adouble.h>
#include <atalk/logger.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <inttypes.h>

#include <string.h>

#include "ad_private.h"

/* translate between ADLOCK styles and specific locking mechanisms */
#define XLATE_FLOCK(type) ((type) == ADLOCK_RD ? LOCK_SH : \
((type) == ADLOCK_WR ? LOCK_EX : \
 ((type) == ADLOCK_CLR ? LOCK_UN : -1)))

#ifdef DISABLE_LOCKING
#define fcntl(a, b, c ) (0)
#endif

/* ----------------------- */
static int set_lock(int fd, int cmd,  struct flock *lock)
{
  if (fd == -2) {
      /* We assign fd = -2 for symlinks -> do nothing */
      if (cmd == F_GETLK)
	    lock->l_type = F_UNLCK;
      return 0;
  }
  return fcntl(fd, cmd, lock);
}

/* ----------------------- */
static int XLATE_FCNTL_LOCK(int type) 
{
    switch(type) {
    case ADLOCK_RD:
        return F_RDLCK;
    case ADLOCK_WR:
         return F_WRLCK;
    case ADLOCK_CLR:
         return F_UNLCK;
    }
    return -1;
}

/* ----------------------- */
static int OVERLAP(off_t a, off_t alen, off_t b, off_t blen) 
{
 return (!alen && a <= b) || 
	(!blen && b <= a) || 
	( (a + alen > b) && (b + blen > a) );
}

/* allocation for lock regions. we allocate aggressively and shrink
 * only in large chunks. */
#define ARRAY_BLOCK_SIZE 10 
#define ARRAY_FREE_DELTA 100

/* remove a lock and compact space if necessary */
static void adf_freelock(struct ad_fd *ad, const int i)
{
    adf_lock_t *lock = ad->adf_lock + i;

    if (--(*lock->refcount) < 1) {
	free(lock->refcount); 
	if (!ad->adf_excl) {
	    lock->lock.l_type = F_UNLCK;
	    set_lock(ad->adf_fd, F_SETLK, &lock->lock); /* unlock */
	}
    }

    ad->adf_lockcount--;

    /* move another lock into the empty space */
    if (i < ad->adf_lockcount) {
        memcpy(lock, lock + ad->adf_lockcount - i, sizeof(adf_lock_t));
    }

    /* free extra cruft if we go past a boundary. we always want to
     * keep at least some stuff around for allocations. this wastes
     * a bit of space to save time on reallocations. */
    if ((ad->adf_lockmax > ARRAY_FREE_DELTA) &&
	(ad->adf_lockcount + ARRAY_FREE_DELTA < ad->adf_lockmax)) {
	    struct adf_lock_t *tmp;

	    tmp = (struct adf_lock_t *) 
		    realloc(ad->adf_lock, sizeof(adf_lock_t)*
			    (ad->adf_lockcount + ARRAY_FREE_DELTA));
	    if (tmp) {
		ad->adf_lock = tmp;
		ad->adf_lockmax = ad->adf_lockcount + ARRAY_FREE_DELTA;
	    }
    }
}


/* this needs to deal with the following cases:
 * 1) fork is the only user of the lock 
 * 2) fork shares a read lock with another open fork
 *
 * i converted to using arrays of locks. everytime a lock
 * gets removed, we shift all of the locks down.
 */
static void adf_unlock(struct ad_fd *ad, const int fork)
{
    adf_lock_t *lock = ad->adf_lock;
    int i;

    for (i = 0; i < ad->adf_lockcount; i++) {
      
      if (lock[i].user == fork) {
	/* we're really going to delete this lock. note: read locks
           are the only ones that allow refcounts > 1 */
	 adf_freelock(ad, i);
	 i--; /* we shifted things down, so we need to backtrack */
	 /* unlikely but realloc may have change adf_lock */
	 lock = ad->adf_lock;       
      }
    }
}

/* relock any byte lock that overlaps off/len. unlock everything
 * else. */
static void adf_relockrange(struct ad_fd *ad, int fd,
				       const off_t off, const off_t len)
{
    adf_lock_t *lock = ad->adf_lock;
    int i;
    
    if (!ad->adf_excl) for (i = 0; i < ad->adf_lockcount; i++) {
      if (OVERLAP(off, len, lock[i].lock.l_start, lock[i].lock.l_len)) 
	set_lock(fd, F_SETLK, &lock[i].lock);
    }
}


/* find a byte lock that overlaps off/len for a particular open fork */
static int adf_findlock(struct ad_fd *ad,
				   const int fork, const int type,
				   const off_t off,
				   const off_t len)
{
  adf_lock_t *lock = ad->adf_lock;
  int i;
  
  for (i = 0; i < ad->adf_lockcount; i++) {
    if ((((type & ADLOCK_RD) && (lock[i].lock.l_type == F_RDLCK)) ||
	((type & ADLOCK_WR) && (lock[i].lock.l_type == F_WRLCK))) &&
	(lock[i].user == fork) && 
	OVERLAP(off, len, lock[i].lock.l_start, lock[i].lock.l_len)) {
      return i;
    }
  }

  return -1;
}


/* search other fork lock lists */
static int adf_findxlock(struct ad_fd *ad, 
				     const int fork, const int type,
				     const off_t off,
				     const off_t len)
{
  adf_lock_t *lock = ad->adf_lock;
  int i;
  
  for (i = 0; i < ad->adf_lockcount; i++) {
    if ((((type & ADLOCK_RD) && (lock[i].lock.l_type == F_RDLCK)) ||
	 ((type & ADLOCK_WR) && (lock[i].lock.l_type == F_WRLCK))) &&
	(lock[i].user != fork) && 
	OVERLAP(off, len, lock[i].lock.l_start, lock[i].lock.l_len)) 
	    return i;
  } 
  return -1;
}

/* okay, this needs to do the following:
 * 1) check current list of locks. error on conflict.
 * 2) apply the lock. error on conflict with another process.
 * 3) update the list of locks this file has.
 * 
 * NOTE: this treats synchronization locks a little differently. we
 *       do the following things for those:
 *       1) if the header file exists, all the locks go in the beginning
 *          of that.
 *       2) if the header file doesn't exist, we stick the locks
 *          in the locations specified by AD_FILELOCK_RD/WR.
 */
#define LOCK_DATA_WR (0)
#define LOCK_DATA_RD (1)
#define LOCK_RSRC_WR (2)
#define LOCK_RSRC_RD (3)

#define LOCK_RSRC_DRD (4)
#define LOCK_RSRC_DWR (5)
#define LOCK_DATA_DRD (6)
#define LOCK_DATA_DWR (7)

#define LOCK_RSRC_NONE (8)
#define LOCK_DATA_NONE (9)

/* -------------- 
	translate a data fork lock to an offset
*/

static off_t df2off(off_t off)
{
    off_t start = off;
	if (off == AD_FILELOCK_OPEN_WR)
		start = LOCK_DATA_WR;
	else if (off == AD_FILELOCK_OPEN_RD)
		start = LOCK_DATA_RD;
    else if (off == AD_FILELOCK_DENY_RD)
		start = LOCK_DATA_DRD;
	else if (off == AD_FILELOCK_DENY_WR)
		start = LOCK_DATA_DWR;
	else if (off == AD_FILELOCK_OPEN_NONE)
		start = LOCK_DATA_NONE;
	return start;
}

/* -------------- 
	translate a resource fork lock to an offset
*/

static off_t hf2off(off_t off)
{
    off_t start = off;
	if (off == AD_FILELOCK_OPEN_WR)
		start = LOCK_RSRC_WR;
	else if (off == AD_FILELOCK_OPEN_RD)
		start = LOCK_RSRC_RD;
    else if (off == AD_FILELOCK_DENY_RD)
		start = LOCK_RSRC_DRD;
	else if (off == AD_FILELOCK_DENY_WR)
		start = LOCK_RSRC_DWR;
	else if (off == AD_FILELOCK_OPEN_NONE)
		start = LOCK_RSRC_NONE;
	return start;
}

/* ------------------ */
int ad_fcntl_lock(struct adouble *ad, const u_int32_t eid, const int locktype,
		  const off_t off, const off_t len, const int fork)
{
  struct flock lock;
  struct ad_fd *adf;
  adf_lock_t *adflock;
  int oldlock;
  int i;
  int type;  

  lock.l_start = off;
  type = locktype;
  if (eid == ADEID_DFORK) {
    adf = &ad->ad_data_fork;
    if ((type & ADLOCK_FILELOCK)) {
        if (ad_meta_fileno(ad) != -1) { /* META */
            adf = ad->ad_md;
            lock.l_start = df2off(off);
        }
    }
  } else { /* rfork */
    if (ad_meta_fileno(ad) == -1 || ad_reso_fileno(ad) == -1) {
        /* there's no meta data. return a lock error 
         * otherwise if a second process is able to create it
         * locks are a mess.
         */
        errno = EACCES;
        return -1;
    }
    if (type & ADLOCK_FILELOCK) {
      adf = ad->ad_md;			/* either resource or meta data (set in ad_open) */
      lock.l_start = hf2off(off);
    }
    else {
      /* we really want the resource fork it's a byte lock */
      adf = &ad->ad_resource_fork;
      lock.l_start += ad_getentryoff(ad, eid);
    }
  }
  /* NOTE: we can't write lock a read-only file. on those, we just
    * make sure that we have a read lock set. that way, we at least prevent
    * someone else from really setting a deny read/write on the file. 
    */
  if (!(adf->adf_flags & O_RDWR) && (type & ADLOCK_WR)) {
      type = (type & ~ADLOCK_WR) | ADLOCK_RD;
  }
  
  lock.l_type = XLATE_FCNTL_LOCK(type & ADLOCK_MASK);
  lock.l_whence = SEEK_SET;
  lock.l_len = len;

  /* byte_lock(len=-1) lock whole file */
  if (len == BYTELOCK_MAX) {
      lock.l_len -= lock.l_start; /* otherwise  EOVERFLOW error */
  }

  /* see if it's locked by another fork. 
   * NOTE: this guarantees that any existing locks must be at most
   * read locks. we use ADLOCK_WR/RD because F_RD/WRLCK aren't
   * guaranteed to be ORable. */
  if (adf_findxlock(adf, fork, ADLOCK_WR | 
		    ((type & ADLOCK_WR) ? ADLOCK_RD : 0), 
		    lock.l_start, lock.l_len) > -1) {
    errno = EACCES;
    return -1;
  }
  
  /* look for any existing lock that we may have */
  i = adf_findlock(adf, fork, ADLOCK_RD | ADLOCK_WR, lock.l_start, lock.l_len);
  adflock = (i < 0) ? NULL : adf->adf_lock + i;

  /* here's what we check for:
     1) we're trying to re-lock a lock, but we didn't specify an update.
     2) we're trying to free only part of a lock. 
     3) we're trying to free a non-existent lock. */
  if ( (!adflock && (lock.l_type == F_UNLCK))
       ||
       (adflock
        && !(type & ADLOCK_UPGRADE)
        && ((lock.l_type != F_UNLCK)
            || (adflock->lock.l_start != lock.l_start)
            || (adflock->lock.l_len != lock.l_len) ))
      ) {
      errno = EINVAL;
      return -1;
  }


  /* now, update our list of locks */
  /* clear the lock */
  if (lock.l_type == F_UNLCK) { 
    adf_freelock(adf, i);
    return 0;
  }

  /* attempt to lock the file. */
  if (!adf->adf_excl && set_lock(adf->adf_fd, F_SETLK, &lock) < 0) 
    return -1;

  /* we upgraded this lock. */
  if (adflock && (type & ADLOCK_UPGRADE)) {
    memcpy(&adflock->lock, &lock, sizeof(lock));
    return 0;
  } 

  /* it wasn't an upgrade */
  oldlock = -1;
  if (lock.l_type == F_RDLCK) {
    oldlock = adf_findxlock(adf, fork, ADLOCK_RD, lock.l_start, lock.l_len);
  } 
    
  /* no more space. this will also happen if lockmax == lockcount == 0 */
  if (adf->adf_lockmax == adf->adf_lockcount) { 
    adf_lock_t *tmp = (adf_lock_t *) 
	    realloc(adf->adf_lock, sizeof(adf_lock_t)*
		    (adf->adf_lockmax + ARRAY_BLOCK_SIZE));
    if (!tmp)
      goto fcntl_lock_err;
    adf->adf_lock = tmp;
    adf->adf_lockmax += ARRAY_BLOCK_SIZE;
  } 
  adflock = adf->adf_lock + adf->adf_lockcount;

  /* fill in fields */
  memcpy(&adflock->lock, &lock, sizeof(lock));
  adflock->user = fork;
  if (oldlock > -1) {
    adflock->refcount = (adf->adf_lock + oldlock)->refcount;
  } else if ((adflock->refcount = calloc(1, sizeof(int))) == NULL) {
    goto fcntl_lock_err;
  }
  
  (*adflock->refcount)++;
  adf->adf_lockcount++;
  return 0;

fcntl_lock_err:
  lock.l_type = F_UNLCK;
  if (!adf->adf_excl) set_lock(adf->adf_fd, F_SETLK, &lock);
  return -1;
}

/* -------------------------
   we are using lock as tristate variable
   
   we have a lock ==> 1
   no             ==> 0
   error          ==> -1
      
*/
static int testlock(struct ad_fd *adf, off_t off, off_t len)
{
  struct flock lock;
  adf_lock_t *plock;
  int i;

  lock.l_start = off;

  plock = adf->adf_lock;
  lock.l_whence = SEEK_SET;
  lock.l_len = len;

  /* Do we have a lock? */
  for (i = 0; i < adf->adf_lockcount; i++) {
    if (OVERLAP(lock.l_start, 1, plock[i].lock.l_start, plock[i].lock.l_len)) 
        return 1;   /* */
  }
  /* Does another process have a lock? 
  */
  lock.l_type = (adf->adf_flags & O_RDWR) ?F_WRLCK : F_RDLCK;

  if (set_lock(adf->adf_fd, F_GETLK, &lock) < 0) {
      /* is that kind of error possible ?*/
      return (errno == EACCES || errno == EAGAIN)?1:-1;
  }
  
  if (lock.l_type == F_UNLCK) {
      return 0;
  }
  return 1;
}

/* --------------- */
int ad_testlock(struct adouble *ad, int eid, const off_t off)
{
  struct ad_fd *adf;
  off_t      lock_offset;

  lock_offset = off;
  if (eid == ADEID_DFORK) {
    adf = &ad->ad_data_fork;
    if (ad_meta_fileno(ad) != -1) {
      	adf = ad->ad_md;
    	lock_offset = df2off(off);
    }
  } 
  else { /* rfork */
    if (ad_meta_fileno(ad) == -1) {
        /* there's no resource fork. return no lock */
        return 0;
    }
    adf = ad->ad_md;
    lock_offset = hf2off(off);
  }
  return testlock(adf, lock_offset, 1);
}

/* -------------------------
   return if a file is open by another process.
   Optimized for the common case:
   - there's no locks held by another process (clients)
   - or we already know the answer and don't need to test.
*/
u_int16_t ad_openforks(struct adouble *ad, u_int16_t attrbits)
{
  u_int16_t ret = 0;
  struct ad_fd *adf;
  off_t off;

  if (!(attrbits & (ATTRBIT_DOPEN | ATTRBIT_ROPEN))) {
      off_t len;
      /* XXX know the locks layout: 
         AD_FILELOCK_OPEN_WR is first 
         and use it for merging requests
      */
      if (ad_meta_fileno(ad) != -1) {
          /* there's a resource fork test the four bytes for
           * data RW/RD and fork RW/RD locks in one request
          */
      	  adf = ad->ad_md;
      	  off = LOCK_DATA_WR;
      	  len = 4;
      }
      else {
          /* no resource fork, only data RD/RW may exist */
          adf = &ad->ad_data_fork;
          off = AD_FILELOCK_OPEN_WR;
          len = 2;
      }
      if (!testlock(adf, off, len))
          return ret;
  }
  /* either there's a lock or we already know one 
     fork is open
  */
  if (!(attrbits & ATTRBIT_DOPEN)) {
      if (ad_meta_fileno(ad) != -1) {
      	  adf = ad->ad_md;
      	  off = LOCK_DATA_WR;
      }
      else {
          adf = &ad->ad_data_fork;
          off = AD_FILELOCK_OPEN_WR;
      }
      ret = testlock(adf, off, 2) > 0? ATTRBIT_DOPEN : 0;
  }

  if (!(attrbits & ATTRBIT_ROPEN)) {
      if (ad_meta_fileno(ad) != -1) {
      	  adf = ad->ad_md;
          off = LOCK_RSRC_WR;
          ret |= testlock(adf, off, 2) > 0? ATTRBIT_ROPEN : 0;
      }
  }

  return ret;
}

/* -------------------------
*/
int ad_fcntl_tmplock(struct adouble *ad, const u_int32_t eid, const int locktype,
	             const off_t off, const off_t len, const int fork)
{
  struct flock lock;
  struct ad_fd *adf;
  int err;
  int type;  

  lock.l_start = off;
  type = locktype;
  if (eid == ADEID_DFORK) {
    adf = &ad->ad_data_fork;
  } else {
    /* FIXME META */
    adf = &ad->ad_resource_fork;
    if (adf->adf_fd == -1) {
        /* there's no resource fork. return success */
        return 0;
    }
    /* if ADLOCK_FILELOCK we want a lock from offset 0
     * it's used when deleting a file:
     * in open we put read locks on meta datas
     * in delete a write locks on the whole file
     * so if the file is open by somebody else it fails
    */
    if (!(type & ADLOCK_FILELOCK))
        lock.l_start += ad_getentryoff(ad, eid);
  }

  if (!(adf->adf_flags & O_RDWR) && (type & ADLOCK_WR)) {
      type = (type & ~ADLOCK_WR) | ADLOCK_RD;
  }
  
  lock.l_type = XLATE_FCNTL_LOCK(type & ADLOCK_MASK);
  lock.l_whence = SEEK_SET;
  lock.l_len = len;

  /* see if it's locked by another fork. */
  if (fork && adf_findxlock(adf, fork, ADLOCK_WR | 
		    ((type & ADLOCK_WR) ? ADLOCK_RD : 0), 
		    lock.l_start, lock.l_len) > -1) {
    errno = EACCES;
    return -1;
  }

  /* okay, we might have ranges byte-locked. we need to make sure that
   * we restore the appropriate ranges once we're done. so, we check
   * for overlap on an unlock and relock. 
   * XXX: in the future, all the byte locks will be sorted and contiguous.
   *      we just want to upgrade all the locks and then downgrade them
   *      here. */
  if (!adf->adf_excl) {
       err = set_lock(adf->adf_fd, F_SETLK, &lock);
  }
  else {
      err = 0;
  }
  if (!err && (lock.l_type == F_UNLCK))
    adf_relockrange(adf, adf->adf_fd, lock.l_start, len);

  return err;
}

/* -------------------------
   the fork is opened in Read Write, Deny Read, Deny Write mode
   lock the whole file once   
*/
int ad_excl_lock(struct adouble *ad, const u_int32_t eid)
{
  struct ad_fd *adf;
  struct flock lock;
  int    err;
  
  lock.l_start = 0;
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_len = 0;

  if (eid == ADEID_DFORK) {
    adf = &ad->ad_data_fork;
  } else {
    adf = &ad->ad_resource_fork;
    lock.l_start = ad_getentryoff(ad, eid);
  }
  
  err = set_lock(adf->adf_fd, F_SETLK, &lock);
  if (!err)
      adf->adf_excl = 1;
  return err;
}

/* --------------------- */
void ad_fcntl_unlock(struct adouble *ad, const int fork)
{
  if (ad_data_fileno(ad) != -1) {
    adf_unlock(&ad->ad_data_fork, fork);
  }
  if (ad_reso_fileno(ad) != -1) {
    adf_unlock(&ad->ad_resource_fork, fork);
  }

  if (ad->ad_flags != AD_VERSION1_SFM) {
    return;
  }
  if (ad_meta_fileno(ad) != -1) {
    adf_unlock(&ad->ad_metadata_fork, fork);
  }

}
