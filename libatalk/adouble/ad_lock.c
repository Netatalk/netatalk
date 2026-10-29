/*
 * Copyright (c) 1998,1999 Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT for more information.
 *
 * Because fcntl locks are process-oriented, we need to keep around a list of
 * file descriptors that refer to the same file.
 */

/*!
 * @file
 * @brief Byte range and share mode locks on AppleDouble forks
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/adouble.h>
#include <atalk/compat.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/util.h>

/*!
 * @brief Name the share-mode lock slot that a lock-band offset stands for, for
 * log output
 *
 * @param[in] off  byte offset of a lock in the share-mode band
 * @returns a constant string such as "OPEN_WR_DATA", or "-" for an offset
 *          outside the band
 */
static const char *shmdstrfromoff(off_t off)
{
    switch (off) {
    case AD_FILELOCK_OPEN_WR:
        return "OPEN_WR_DATA";

    case AD_FILELOCK_OPEN_RD:
        return "OPEN_RD_DATA";

    case AD_FILELOCK_RSRC_OPEN_WR:
        return "OPEN_WR_RSRC";

    case AD_FILELOCK_RSRC_OPEN_RD:
        return "OPEN_RD_RSRC";

    case AD_FILELOCK_DENY_WR:
        return "DENY_WR_DATA";

    case AD_FILELOCK_DENY_RD:
        return "DENY_RD_DATA";

    case AD_FILELOCK_RSRC_DENY_WR:
        return "DENY_WR_RSRC";

    case AD_FILELOCK_RSRC_DENY_RD:
        return "DENY_RD_RSRC";

    case AD_FILELOCK_OPEN_NONE:
        return "OPEN_NONE_DATA";

    case AD_FILELOCK_RSRC_OPEN_NONE:
        return "OPEN_NONE_RSRC";

    default:
        return "-";
    }
}

/*!
 * @brief Apply an fcntl lock operation to a fork descriptor
 *
 * The AD_SYMLINK pseudo-descriptor holds no locks: F_GETLK reports F_UNLCK and
 * F_SETLK succeeds without a system call.
 *
 * @param[in]     fd    fork descriptor, or AD_SYMLINK
 * @param[in]     cmd   F_SETLK or F_GETLK
 * @param[in,out] lock  lock to set or probe, which F_GETLK overwrites with the
 *                      blocking lock
 * @returns 0, or -1 with errno from fcntl
 */
static int set_lock(int fd, int cmd,  struct flock *lock)
{
    EC_INIT;
    LOG(log_debug, logtype_ad,
        "set_lock(fd: %d, %s, %s, off: %jd (%s), len: %jd): BEGIN",
        fd, cmd == F_SETLK ? "F_SETLK" : "F_GETLK",
        lock->l_type == F_RDLCK ? "F_RDLCK" : lock->l_type == F_WRLCK ? "F_WRLCK" :
        "F_UNLCK",
        (intmax_t)lock->l_start,
        shmdstrfromoff(lock->l_start),
        (intmax_t)lock->l_len);

    if (fd == AD_SYMLINK) {
        if (cmd == F_GETLK) {
            lock->l_type = F_UNLCK;
        }

        return 0;
    }

    EC_NEG1(fcntl(fd, cmd, lock));
EC_CLEANUP:
    EC_EXIT;
}

/*!
 * @brief Translate an ADLOCK_* lock type into the fcntl lock type
 *
 * @param[in] type  ADLOCK_RD, ADLOCK_WR, or ADLOCK_CLR
 * @returns F_RDLCK, F_WRLCK, or F_UNLCK, or -1 for any other value
 */
static int XLATE_FCNTL_LOCK(int type)
{
    switch (type) {
    case ADLOCK_RD:
        return F_RDLCK;

    case ADLOCK_WR:
        return F_WRLCK;

    case ADLOCK_CLR:
        return F_UNLCK;
    }

    return -1;
}

/*!
 * @brief Tell whether two byte ranges overlap, where a zero length runs to the
 * end of the file
 *
 * Subtracting the starts instead of adding the lengths keeps the comparison
 * within off_t for ranges at or past byte zero, one at the largest offset
 * included.
 *
 * @param[in] a     start of the first range
 * @param[in] alen  length of the first range, or 0 for the rest of the file
 * @param[in] b     start of the second range
 * @param[in] blen  length of the second range, or 0 for the rest of the file
 * @returns non-zero when the ranges share at least one byte
 */
static int OVERLAP(off_t a, off_t alen, off_t b, off_t blen)
{
    return (!alen && a <= b) ||
           (!blen && b <= a) ||
           ((alen > b - a) && (blen > a - b));
}

/* allocation for lock regions. we allocate aggressively and shrink
 * only in large chunks. */
#define ARRAY_BLOCK_SIZE 10
#define ARRAY_FREE_DELTA 100

/* the largest offset off_t holds, where the band's last slot sits: no range
 * ends past it */
#define LOCK_TOP ((off_t)((UINT64_C(1) << (sizeof(off_t) * CHAR_BIT - 1)) - 1))

/*!
 * @brief Tell whether a range of positive length ends at or below the largest
 * offset
 *
 * @param[in] start  first byte of the range, at or past byte zero
 * @param[in] len    length of the range, positive
 * @returns non-zero when the range's last byte fits in off_t
 */
static int adf_lock_fits(off_t start, off_t len)
{
    return len - 1 <= LOCK_TOP - start;
}

/*!
 * @brief Last byte of a range that fits, where a zero length reaches the end
 * of the file
 *
 * @param[in] start  first byte of the range
 * @param[in] len    length of the range, or 0 to the end of the file
 * @returns the last byte, the largest offset for a zero length
 */
static off_t adf_lock_last(off_t start, off_t len)
{
    return len > 0 ? start + (len - 1) : LOCK_TOP;
}

/*!
 * @brief First index of the lock array whose entry starts, or with by_reach
 * reaches, at or past target
 *
 * Entries are in order of start, and their reach, the farthest last byte of an
 * entry and the ones before it, rises with the index.
 *
 * @param[in] adf       fork descriptor
 * @param[in] target    the offset to search for
 * @param[in] by_reach  nonzero to search the entries' reach, zero their start
 * @returns the index, or adf_lockcount when no entry qualifies
 */
static int adf_lock_lower(const struct ad_fd *adf, off_t target, int by_reach)
{
    int lo = 0;
    int hi = adf->adf_lockcount;

    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        const adf_lock_t *entry = adf->adf_lock + mid;
        off_t key = by_reach ? entry->reach : entry->lock.l_start;

        if (key < target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    return lo;
}

/*!
 * @brief Index window of the lock array that can hold entries overlapping a
 * byte range
 *
 * An entry before the first one whose reach is at or past off ends before the
 * range starts, and an entry at or past the first one that starts past the
 * range's last byte begins after it ends. The range has passed the callers'
 * checks: it starts at or past byte zero and its last byte fits in off_t. A
 * range of zero length reaches the end of the file, and no entry starts past
 * the largest offset, so both take the array's end as their upper bound.
 *
 * @param[in]  adf  fork descriptor
 * @param[in]  off  start of the range
 * @param[in]  len  length of the range, or 0 to the end of the file
 * @param[out] lo   first index that can overlap the range
 * @param[out] hi   index after the last entry that can overlap the range
 */
static void adf_lock_window(const struct ad_fd *adf, off_t off, off_t len,
                            int *lo, int *hi)
{
    off_t last = adf_lock_last(off, len);
    *lo = adf_lock_lower(adf, off, 1);
    *hi = last < LOCK_TOP ? adf_lock_lower(adf, last + 1, 0)
          : adf->adf_lockcount;
}

/*!
 * @brief Recompute the reach of the entries from index from to the end
 *
 * @param[in,out] adf   fork descriptor
 * @param[in]     from  first index to recompute
 */
static void adf_lock_reach(struct ad_fd *adf, int from)
{
    off_t reach = from > 0 ? adf->adf_lock[from - 1].reach : -1;

    for (int i = from; i < adf->adf_lockcount; i++) {
        adf_lock_t *entry = adf->adf_lock + i;
        off_t last = adf_lock_last(entry->lock.l_start, entry->lock.l_len);

        if (last > reach) {
            reach = last;
        }

        entry->reach = reach;
    }
}

/*!
 * @brief Apply a lock operation to the bytes of a range that no recorded entry
 * covers
 *
 * The window's entries are in order of start, so the uncovered bytes are the
 * stretches between them. An entry covers its bytes whatever its type: below
 * the band the conflict scan has refused another fork's conflicting entry, and
 * a read lock is recorded there only on a read-only descriptor, where a write
 * request degrades to a read lock too; in the band the fork's own share-mode
 * entries are read locks, so a write reaching them takes no write lock there.
 * A take that fails on a stretch releases the stretches before it; a release
 * that fails on one logs it and goes on.
 *
 * @param[in] adf  fork descriptor
 * @param[in] r    the range, past the callers' checks, with l_type F_RDLCK or
 *                 F_WRLCK to take it or F_UNLCK to release it
 * @returns 0, or -1 with errno from fcntl for a refused take, or -1 after a
 *          failed release
 */
static int adf_lock_gaps(const struct ad_fd *adf, const struct flock *r)
{
    const off_t last = adf_lock_last(r->l_start, r->l_len);
    struct flock gap = *r;
    off_t done = r->l_start - 1;    /* the last byte covered or dealt with */
    int ret = 0;
    int lo, hi;
    adf_lock_window(adf, r->l_start, r->l_len, &lo, &hi);

    for (int i = lo; i <= hi && done < last; i++) {
        /* past the window's last entry the stretch runs to the range's end */
        off_t stop = i < hi ? adf->adf_lock[i].lock.l_start - 1 : last;

        if (stop > done) {
            gap.l_start = done + 1;
            /* a length of 0 runs to the end of the file */
            gap.l_len = stop < LOCK_TOP ? stop - done : 0;

            if (set_lock(adf->adf_fd, F_SETLK, &gap) < 0) {
                if (gap.l_type != F_UNLCK) {
                    int saved = errno;

                    if (done >= r->l_start) {
                        struct flock undo = *r;
                        undo.l_type = F_UNLCK;
                        undo.l_len = done - r->l_start + 1;
                        (void)adf_lock_gaps(adf, &undo);
                    }

                    errno = saved;
                    return -1;
                }

                LOG(log_error, logtype_ad,
                    "adf_lock_gaps: F_UNLCK fd=%d off=%jd len=%jd: %s - "
                    "kernel lock may be stranded until this process's last "
                    "close of the file", adf->adf_fd, (intmax_t)gap.l_start,
                    (intmax_t)gap.l_len, strerror(errno));
                ret = -1;
            }
        }

        /* the entries before the window end before the range, so an entry's
         * reach is the last byte covered up to it */
        if (i < hi && adf->adf_lock[i].reach > done) {
            done = adf->adf_lock[i].reach;
        }
    }

    return ret;
}

/*!
 * @brief Drop lock i of a fork descriptor and compact the lock array
 *
 * The release covers the bytes of the entry's range that no surviving entry
 * holds, so the survivors keep their locks whatever the kernel coalesced. The
 * array shrinks once more than 100 slots stand unused, the ARRAY_FREE_DELTA
 * margin.
 *
 * @param[in,out] ad  fork descriptor holding the lock array
 * @param[in]     i   index of the lock to drop
 */
static void adf_freelock(struct ad_fd *ad, const int i)
{
    adf_lock_t *lock = ad->adf_lock + i;
    struct flock ul = lock->lock;   /* l_whence SEEK_SET */
    ul.l_type = F_UNLCK;
    ad->adf_lockcount--;
    /* the entries stay in order of start */
    memmove(lock, lock + 1,
            (size_t)(ad->adf_lockcount - i) * sizeof(adf_lock_t));
    adf_lock_reach(ad, i);
    /* a failed release is logged by the walk */
    (void)adf_lock_gaps(ad, &ul);

    /* free extra cruft if we go past a boundary. we always want to
     * keep at least some stuff around for allocations. this wastes
     * a bit of space to save time on reallocations. */
    if ((ad->adf_lockmax > ARRAY_FREE_DELTA) &&
            (ad->adf_lockcount + ARRAY_FREE_DELTA < ad->adf_lockmax)) {
        struct adf_lock_t *tmp;
        tmp = (struct adf_lock_t *)
              realloc(ad->adf_lock, sizeof(adf_lock_t) *
                      (ad->adf_lockcount + ARRAY_FREE_DELTA));

        if (tmp) {
            ad->adf_lock = tmp;
            ad->adf_lockmax = ad->adf_lockcount + ARRAY_FREE_DELTA;
        }
    }
}


/*!
 * @brief Release every lock one fork holds on a fork descriptor
 *
 * The AFP specification releases all locks of a user when the user closes the
 * fork. A user here is one OForkRefNum, the fork of lock[i].user. Only this
 * fork's locks go. A sibling fork on the same shared ad_fd keeps its locks. The
 * locks live in an array that adf_freelock() compacts. The scan runs from the
 * top down, so a removal moves only the entries above it, and re-reads the
 * array pointer after each one, which realloc may have moved.
 *
 * @param[in]     ad    unused
 * @param[in,out] adf   fork descriptor whose lock array shrinks
 * @param[in]     fork  OForkRefNum whose locks go
 */
static void adf_unlock(struct adouble *ad _U_, struct ad_fd *adf,
                       const int fork)
{
    adf_lock_t *lock = adf->adf_lock;

    for (int i = adf->adf_lockcount - 1; i >= 0; i--) {
        if (lock[i].user == fork) {
            adf_freelock(adf, i);
            lock = adf->adf_lock;      /* realloc may have moved the array */
        }
    }
}

/*!
 * @brief Initialize lock tracking for an ad_fd structure
 *
 * Initializes the lock tracking fields in an ad_fd structure to their
 * default empty state. This should be called when initializing a new
 * ad_fd or after freeing all locks.
 *
 * @param[in,out] adf  File descriptor structure to initialize
 */
void adf_lock_init(struct ad_fd *adf)
{
    if (!adf) {
        LOG(log_error, logtype_ad, "adf_lock_init: NULL parameter");
        return;
    }

    adf->adf_lockmax = 0;
    adf->adf_lockcount = 0;
    adf->adf_lock = NULL;
}

/*!
 * @brief Free all locks in an ad_fd structure
 *
 * Releases every lock in the ad_fd lock array, each over the bytes no other
 * entry holds, and frees the array.
 *
 * This function is called when closing file descriptors to ensure all locks
 * are properly released and memory is freed.
 *
 * @param[in,out] adf  File descriptor structure containing locks to free
 */
void adf_lock_free(struct ad_fd *adf)
{
    if (!adf) {
        LOG(log_error, logtype_ad, "adf_lock_free: NULL parameter");
        return;
    }

    if (!adf->adf_lock) {
        return;
    }

    /* a closed descriptor holds no lock; the top entry goes first, so none
     * moves */
    if (adf->adf_fd >= 0) {
        while (adf->adf_lockcount > 0) {
            adf_freelock(adf, adf->adf_lockcount - 1);
        }
    }

    free(adf->adf_lock);
    adf_lock_init(adf);
}

/*!
 * @brief Find a lock of one open fork that overlaps a byte range
 *
 * @param[in] ad    fork descriptor holding the lock array
 * @param[in] fork  reference number of the open fork that owns the lock
 * @param[in] type  ADLOCK_RD, ADLOCK_WR, or both
 * @param[in] off   start of the range
 * @param[in] len   length of the range
 * @returns the index of the first matching lock, or -1
 */
static int adf_findlock(struct ad_fd *ad,
                        const int fork, const int type,
                        const off_t off,
                        const off_t len)
{
    adf_lock_t *lock = ad->adf_lock;
    int lo, hi;
    adf_lock_window(ad, off, len, &lo, &hi);

    for (int i = lo; i < hi; i++) {
        if ((((type & ADLOCK_RD) && (lock[i].lock.l_type == F_RDLCK)) ||
                ((type & ADLOCK_WR) && (lock[i].lock.l_type == F_WRLCK))) &&
                (lock[i].user == fork) &&
                OVERLAP(off, len, lock[i].lock.l_start, lock[i].lock.l_len)) {
            return i;
        }
    }

    return -1;
}


/*!
 * @brief Find a lock of any other open fork that overlaps a byte range
 *
 * @param[in] ad    fork descriptor holding the lock array
 * @param[in] fork  reference number of the open fork whose locks to skip
 * @param[in] type  ADLOCK_RD, ADLOCK_WR, or both
 * @param[in] off   start of the range
 * @param[in] len   length of the range
 * @returns the index of the first matching lock, or -1
 */
static int adf_findxlock(struct ad_fd *ad,
                         const int fork, const int type,
                         const off_t off,
                         const off_t len)
{
    adf_lock_t *lock = ad->adf_lock;
    int lo, hi;
    adf_lock_window(ad, off, len, &lo, &hi);

    for (int i = lo; i < hi; i++) {
        if ((((type & ADLOCK_RD) && (lock[i].lock.l_type == F_RDLCK))
                ||
                ((type & ADLOCK_WR) && (lock[i].lock.l_type == F_WRLCK)))
                &&
                (lock[i].user != fork)
                &&
                OVERLAP(off, len, lock[i].lock.l_start, lock[i].lock.l_len)) {
            return i;
        }
    }

    return -1;
}

/*!
 * @brief Map a data fork band lock offset to the matching resource fork band
 * offset
 *
 * Offsets outside the five data fork band offsets pass through unchanged.
 *
 * @param[in] off  lock offset
 * @returns the resource fork band offset, or off
 */
static off_t rf2off(off_t off)
{
    off_t start = off;

    if (off == AD_FILELOCK_OPEN_WR) {
        start = AD_FILELOCK_RSRC_OPEN_WR;
    } else if (off == AD_FILELOCK_OPEN_RD) {
        start = AD_FILELOCK_RSRC_OPEN_RD;
    } else if (off == AD_FILELOCK_DENY_RD) {
        start = AD_FILELOCK_RSRC_DENY_RD;
    } else if (off == AD_FILELOCK_DENY_WR) {
        start = AD_FILELOCK_RSRC_DENY_WR;
    } else if (off == AD_FILELOCK_OPEN_NONE) {
        start = AD_FILELOCK_RSRC_OPEN_NONE;
    }

    return start;
}

/*!
 * @brief Test a lock
 *
 * 1. Test against our own locks array
 * 2. Test fcntl lock, locks from other processes
 *
 * @param[in] adf     handle
 * @param[in] off     offset
 * @param[in] len     length
 *
 * @returns           1 if there's an existing lock, 0 if there's no lock,
 *                    -1 in case any error occured
 */
static int testlock(const struct ad_fd *adf, off_t off, off_t len)
{
    struct flock lock;
    adf_lock_t *plock;
    int i;
    lock.l_start = off;
    plock = adf->adf_lock;
    lock.l_whence = SEEK_SET;
    lock.l_len = len;

    /* (1) Do we have a lock ? */
    for (i = 0; i < adf->adf_lockcount; i++) {
        if (OVERLAP(lock.l_start, 1, plock[i].lock.l_start, plock[i].lock.l_len)) {
            return 1;
        }
    }

    /* (2) Does another process have a lock? F_GETLK only tests, so a write
     *     probe sees the band's read locks through a read-only fd too */
    lock.l_type = F_WRLCK;

    if (set_lock(adf->adf_fd, F_GETLK, &lock) < 0) {
        /* is that kind of error possible ?*/
        return (errno == EACCES || errno == EAGAIN) ? 1 : -1;
    }

    if (lock.l_type == F_UNLCK) {
        return 0;
    }

    return 1;
}

#define LTYPE2STRBUFSIZ 128
/*!
 * @brief Render an ADLOCK_* type mask as text, for log output
 *
 * @param[in] type  lock type mask
 * @returns a static buffer holding "CLR", or the set bits as "RD", "WR", "UPG",
 *          and "FILELOCK" joined by "|"
 */
static const char *locktypetostr(int type)
{
    int first = 1;
    static char buf[LTYPE2STRBUFSIZ];
    buf[0] = 0;

    if (type == 0) {
        strlcat(buf, "CLR", LTYPE2STRBUFSIZ);
        first = 0;
        return buf;
    }

    if (type & ADLOCK_RD) {
        if (!first) {
            strlcat(buf, "|", LTYPE2STRBUFSIZ);
        }

        strlcat(buf, "RD", LTYPE2STRBUFSIZ);
        first = 0;
    }

    if (type & ADLOCK_WR) {
        if (!first) {
            strlcat(buf, "|", LTYPE2STRBUFSIZ);
        }

        strlcat(buf, "WR", LTYPE2STRBUFSIZ);
        first = 0;
    }

    if (type & ADLOCK_UPGRADE) {
        if (!first) {
            strlcat(buf, "|", LTYPE2STRBUFSIZ);
        }

        strlcat(buf, "UPG", LTYPE2STRBUFSIZ);
        first = 0;
    }

    if (type & ADLOCK_FILELOCK) {
        if (!first) {
            strlcat(buf, "|", LTYPE2STRBUFSIZ);
        }

        strlcat(buf, "FILELOCK", LTYPE2STRBUFSIZ);
        first = 0;
    }

    return buf;
}

/******************************************************************************
 * Public functions
 ******************************************************************************/

/*!
 * @brief Take or release a byte-range or share-mode lock on a fork for one AFP
 * fork
 *
 * Locks are process-wide fcntl locks, so the descriptor's array records the
 * ranges each fork holds, in order of start, and is checked before the kernel.
 * A write lock on a read-only descriptor degrades to a read lock, which still
 * blocks a deny on the file. Resource fork ranges shift by the resource fork
 * entry offset; with ADLOCK_FILELOCK they move into the data fork's share-mode
 * band instead. BYTELOCK_MAX locks to the end of the file. Overlapping read
 * locks of different forks are each recorded; the kernel merges them, and a
 * release frees the bytes no other entry holds. A lock the kernel took but the
 * array could not record is released again.
 *
 * @param[in,out] ad        AppleDouble whose forks are open
 * @param[in]     eid       ADEID_DFORK or ADEID_RFORK
 * @param[in]     locktype  ADLOCK_RD, ADLOCK_WR, or ADLOCK_CLR, plus
 *                          ADLOCK_UPGRADE or ADLOCK_FILELOCK
 * @param[in]     off       first byte of the range, or the share-mode slot with
 *                          ADLOCK_FILELOCK
 * @param[in]     len       length of the range, BYTELOCK_MAX for the rest of
 *                          the file, 1 with ADLOCK_FILELOCK
 * @param[in]     fork      OForkRefNum the lock belongs to
 * @returns 0, or -1 with errno EACCES for a conflicting lock of another fork;
 *          EINVAL for a range below byte zero, one the entry offset moves past
 *          the largest offset, one without length, a release of a range no
 *          entry of the fork holds as it is, an upgrade whose range differs
 *          from its entry's, or a take without ADLOCK_UPGRADE over bytes an
 *          entry of the fork already holds; EOVERFLOW for a take whose last
 *          byte passes the largest offset; else the errno of fcntl or realloc
 */
int ad_lock(struct adouble *ad, uint32_t eid, int locktype, off_t off,
            off_t len, int fork)
{
    struct flock lock;
    struct ad_fd *adf;
    adf_lock_t *adflock;
    off_t shift;
    int i;
    int type;
    int ret = 0;
    LOG(log_debug, logtype_ad, "ad_lock(%s, %s, off: %jd (%s), len: %jd): BEGIN",
        eid == ADEID_DFORK ? "data" : "reso",
        locktypetostr(locktype),
        (intmax_t)off,
        shmdstrfromoff(off),
        (intmax_t)len);

    if ((locktype & ADLOCK_FILELOCK) && (len != 1)) {
        AFP_PANIC("lock API error");
    }

    type = locktype;
    shift = 0;

    if (eid == ADEID_DFORK) {
        adf = &ad->ad_data_fork;
    } else if (type & ADLOCK_FILELOCK) {
        adf = &ad->ad_data_fork;
        off = rf2off(off);
    } else {
        adf = ad->ad_rfp;
        shift = ad_getentryoff(ad, ADEID_RFORK);
    }

    /* the entry offset is added below: the start must leave room for it */
    if (off < 0 || off > LOCK_TOP - shift) {
        errno = EINVAL;
        ret = -1;
        goto exit;
    }

    /* NOTE: we can't write lock a read-only file. on those, we just
     * make sure that we have a read lock set. that way, we at least prevent
     * someone else from really setting a deny read/write on the file.
     */
    if (!(adf->adf_flags & O_RDWR) && (type & ADLOCK_WR)) {
        type = (type & ~ADLOCK_WR) | ADLOCK_RD;
    }

    lock.l_start = off + shift;
    lock.l_type = XLATE_FCNTL_LOCK(type & ADLOCK_MASK);
    lock.l_whence = SEEK_SET;
    lock.l_len = len;

    /* byte_lock(len=-1) lock whole file */
    if (len == BYTELOCK_MAX) {
        lock.l_len -= lock.l_start; /* otherwise  EOVERFLOW error */
    }

    /* a whole-file lock at or past the last lockable offset has no length;
     * EINVAL is byte_lock()'s AFPERR_RANGEOVR for a take and AFPERR_NORANGE
     * for a release */
    if (lock.l_len <= 0) {
        errno = EINVAL;
        ret = -1;
        goto exit;
    }

    /* EOVERFLOW is the kernel's answer to such a take; a release of such a
     * range names no entry */
    if (!adf_lock_fits(lock.l_start, lock.l_len)) {
        errno = lock.l_type == F_UNLCK ? EINVAL : EOVERFLOW;
        ret = -1;
        goto exit;
    }

    /* see if it's locked by another fork; a release holds its entry itself.
     * NOTE: this guarantees that any existing locks must be at most
     * read locks. we use ADLOCK_WR/RD because F_RD/WRLCK aren't
     * guaranteed to be ORable. */
    if (lock.l_type != F_UNLCK &&
            adf_findxlock(adf, fork,
                          ADLOCK_WR | ((type & ADLOCK_WR) ? ADLOCK_RD : 0),
                          lock.l_start, lock.l_len) > -1) {
        errno = EACCES;
        ret = -1;
        goto exit;
    }

    /* look for any existing lock that we may have */
    i = adf_findlock(adf, fork, ADLOCK_RD | ADLOCK_WR, lock.l_start, lock.l_len);
    adflock = (i < 0) ? NULL : adf->adf_lock + i;

    /* here's what we check for:
       1) we're trying to re-lock a lock, but we didn't specify an update.
       2) we're trying to free only part of a lock.
       3) we're trying to free a non-existent lock.
       4) we're trying to move or resize a lock with an update. */
    if ((!adflock && (lock.l_type == F_UNLCK))
            ||
            (adflock
             && ((adflock->lock.l_start != lock.l_start)
                 || (adflock->lock.l_len != lock.l_len)
                 || (!(type & ADLOCK_UPGRADE)
                     && (lock.l_type != F_UNLCK))))
       ) {
        errno = EINVAL;
        ret = -1;
        goto exit;
    }

    /* now, update our list of locks */
    /* clear the lock */
    if (lock.l_type == F_UNLCK) {
        adf_freelock(adf, i);
        goto exit;
    }

    /* attempt to lock the file. */
    if (set_lock(adf->adf_fd, F_SETLK, &lock) < 0) {
        ret = -1;
        goto exit;
    }

    /* an upgrade changes the entry's type over the same range, so its reach
     * stands */
    if (adflock && (type & ADLOCK_UPGRADE)) {
        memcpy(&adflock->lock, &lock, sizeof(lock));
        goto exit;
    }

    /* no more space. this will also happen if lockmax == lockcount == 0 */
    if (adf->adf_lockmax == adf->adf_lockcount) {
        adf_lock_t *tmp = (adf_lock_t *)
                          realloc(adf->adf_lock, sizeof(adf_lock_t) *
                                  (adf->adf_lockmax + ARRAY_BLOCK_SIZE));

        if (!tmp) {
            /* the kernel holds the lock: release the bytes no entry records;
             * a failed stretch is logged by the walk */
            int saved = errno;
            lock.l_type = F_UNLCK;
            (void)adf_lock_gaps(adf, &lock);
            errno = saved;
            ret = -1;
            goto exit;
        }

        adf->adf_lock = tmp;
        adf->adf_lockmax += ARRAY_BLOCK_SIZE;
    }

    /* the array stays in order of start */
    i = adf_lock_lower(adf, lock.l_start, 0);
    memmove(adf->adf_lock + i + 1, adf->adf_lock + i,
            (size_t)(adf->adf_lockcount - i) * sizeof(adf_lock_t));
    adflock = adf->adf_lock + i;
    memcpy(&adflock->lock, &lock, sizeof(lock));
    adflock->user = fork;
    adf->adf_lockcount++;
    adf_lock_reach(adf, i);
exit:
    LOG(log_debug, logtype_ad, "ad_lock: END: %d", ret);
    return ret;
}

/*!
 * @brief Take or release a temporary lock that the lock array does not record
 *
 * A temporary lock guards a read, a write, a truncate or the conversion of an
 * AppleDouble file and goes away with its F_UNLCK. It covers only the bytes of
 * its range no recorded entry holds, so the recorded ranges stay as they are.
 * A missing resource fork counts as success before any check, and a write lock
 * on a read-only descriptor degrades to a read lock. ADLOCK_FILELOCK locks the
 * resource fork's AppleDouble file from byte zero instead of the fork's entry
 * offset; the conversion takes it over the whole file with fork 0, on a
 * descriptor that records no lock, so nothing is left out of it.
 *
 * @param[in,out] ad        AppleDouble whose forks are open
 * @param[in]     eid       ADEID_DFORK or ADEID_RFORK
 * @param[in]     locktype  ADLOCK_RD, ADLOCK_WR, or ADLOCK_CLR, plus
 *                          ADLOCK_FILELOCK
 * @param[in]     off       first byte of the range
 * @param[in]     len       length of the range, or 0 to the end of the file
 * @param[in]     fork      OForkRefNum asking, or 0 to skip the check against
 *                          other forks' locks
 * @returns 0, or -1 with errno EINVAL for a range below byte zero, one whose
 *          start the resource fork's entry offset moves past the largest
 *          offset or one of negative length, EOVERFLOW for a last byte past
 *          the largest offset, EACCES when another fork holds a conflicting
 *          lock, or the errno of fcntl
 */
int ad_tmplock(struct adouble *ad, uint32_t eid, int locktype, off_t off,
               off_t len, int fork)
{
    struct flock lock;
    struct ad_fd *adf;
    off_t shift;
    int err;
    int type;
    LOG(log_debug, logtype_ad, "ad_tmplock(%s, %s, off: %jd (%s), len: %jd): BEGIN",
        eid == ADEID_DFORK ? "data" : "reso",
        locktypetostr(locktype),
        (intmax_t)off,
        shmdstrfromoff(off),
        (intmax_t)len);
    type = locktype;
    shift = 0;

    if (eid == ADEID_DFORK) {
        adf = &ad->ad_data_fork;
    } else {
        adf = ad->ad_rfp;                  /* match ad_lock(): backend indirection */

        if (adf->adf_fd == -1) {
            /* there's no resource fork. return success */
            err = 0;
            goto exit;
        }

        /* ADLOCK_FILELOCK locks the AppleDouble file from byte zero instead of
         * from the fork's entry offset: the conversion takes the whole file */
        if (!(type & ADLOCK_FILELOCK)) {
            shift = ad_getentryoff(ad, eid);
        }
    }

    /* the entry offset is added below: the start must leave room for it */
    if (off < 0 || off > LOCK_TOP - shift || len < 0) {
        errno = EINVAL;
        err = -1;
        goto exit;
    }

    if (!(adf->adf_flags & O_RDWR) && (type & ADLOCK_WR)) {
        type = (type & ~ADLOCK_WR) | ADLOCK_RD;
    }

    lock.l_start = off + shift;
    lock.l_type = XLATE_FCNTL_LOCK(type & ADLOCK_MASK);
    lock.l_whence = SEEK_SET;
    lock.l_len = len;

    /* EOVERFLOW is the kernel's answer to such a range */
    if (len > 0 && !adf_lock_fits(lock.l_start, len)) {
        errno = EOVERFLOW;
        err = -1;
        goto exit;
    }

    /* see if it's locked by another fork; a release conflicts with nothing,
     * as its take passed this check */
    if (fork && lock.l_type != F_UNLCK &&
            adf_findxlock(adf, fork,
                          ADLOCK_WR | ((type & ADLOCK_WR) ? ADLOCK_RD : 0),
                          lock.l_start, lock.l_len) > -1) {
        errno = EACCES;
        err = -1;
        goto exit;
    }

    err = adf_lock_gaps(adf, &lock);
exit:
    LOG(log_debug, logtype_ad, "ad_tmplock: END: %d", err);
    return err;
}

/*!
 * @brief Release every lock one fork holds on the data and resource forks of an
 * AppleDouble
 *
 * @param[in,out] ad    AppleDouble whose forks may be open
 * @param[in]     fork  OForkRefNum whose locks go
 */
void ad_unlock(struct adouble *ad, const int fork)
{
    LOG(log_debug, logtype_ad, "ad_unlock(fork: %d): BEGIN", fork);

    if (ad_data_fileno(ad) != -1) {
        adf_unlock(ad, &ad->ad_data_fork, fork);
    }

    if (ad_reso_fileno(ad) != -1) {
        adf_unlock(ad, ad->ad_rfp, fork);   /* rfork indirection, matching ad_lock() */
    }

    LOG(log_debug, logtype_ad, "ad_unlock: END");
}

/*!
 * @brief Test for a share mode lock
 *
 * @param[in,out] ad    handle
 * @param[in] eid       datafork or resource fork
 * @param[in] off       sharemode lock to test
 *
 * @returns           1 if there's an existing lock, 0 if there's no lock,
 *                    -1 in case any error occured
 */
int ad_testlock(struct adouble *ad, int eid, const off_t off)
{
    int ret = 0;
    off_t lock_offset;
    LOG(log_debug, logtype_ad, "ad_testlock(%s, off: %jd (%s): BEGIN",
        eid == ADEID_DFORK ? "data" : "reso",
        (intmax_t)off,
        shmdstrfromoff(off));

    if (eid == ADEID_DFORK) {
        lock_offset = off;
    } else { /* rfork */
        lock_offset = rf2off(off);
    }

    ret = testlock(&ad->ad_data_fork, lock_offset, 1);
    LOG(log_debug, logtype_ad, "ad_testlock: END: %d", ret);
    return ret;
}

/*!
 * @brief GET-only range probe: does another process hold a conflicting lock?
 *
 * Tests whether another process holds a conflicting lock on [off, off+len) of
 * fork @p eid.  Unlike ad_testlock()/testlock() this does not scan our own
 * adf_lock[] array first: it is kernel F_GETLK only.  By POSIX, F_GETLK never
 * reports the calling process's own locks, so this can be called through a fd of
 * a fork we hold open without self-reporting our own band/byte entries — exactly
 * the conflict read a delete needs (it must see only other holders).  It never
 * issues F_SETLK, so it has no acquire/release lifetime and cannot strand a lock.
 *
 * Key behaviours:
 *
 *  - No adf_lock[] array scan — kernel F_GETLK only (no self-report).
 *  - Always probes with an explicit F_WRLCK.  A write probe conflicts with a
 *    peer's read or write lock, so it sees the F_RDLCK share-mode band entries.
 *    F_GETLK only tests, so the probe type is independent of the fd's
 *    O_RDWR/O_RDONLY mode, as in testlock().
 *  - eid dispatch: ADEID_DFORK uses the data fd at the literal offset, which also
 *    covers the whole share-mode band (OPEN, DENY and RSRC mirrors), since the
 *    band lives on the data fd via rf2off().  ADEID_RFORK uses the resource fork's
 *    own fd (ad_rfp) at off + ad_getentryoff(), matching ad_lock()'s rfork
 *    non-FILELOCK branch.  Band offsets are never passed with ADEID_RFORK; a band
 *    offset is recognised as off >= AD_FILELOCK_BASE.
 *  - Zone clamp on data-zone requests only (off < AD_FILELOCK_BASE): a len == 0
 *    ("whole data zone") or over-long request is bounded to the data zone, never
 *    a POSIX l_len == 0 ("to infinity") that would sweep the share-mode band.
 *    An explicit band probe (off >= AD_FILELOCK_BASE) passes through unclamped.
 *
 * @param[in,out] ad   handle
 * @param[in] eid      ADEID_DFORK (data + band) or ADEID_RFORK (rfork content)
 * @param[in] off      offset (content) or a band offset (>= AD_FILELOCK_BASE)
 * @param[in] len      length; 0 == whole data zone (data-zone requests only)
 *
 * @returns  1 = a conflicting lock exists (incl. F_GETLK EACCES/EAGAIN, reported
 *               as locked/indeterminate — fail safe, matching testlock());
 *           0 = no conflict (incl. no fd to probe);
 *          -1 = hard error (caller must refuse the operation, never proceed).
 */
int ad_testlock_range(struct adouble *ad, int eid, off_t off, off_t len)
{
    const struct ad_fd *adf;
    struct flock  lock;
    LOG(log_debug, logtype_ad,
        "ad_testlock_range(%s, off: %jd (%s), len: %jd): BEGIN",
        eid == ADEID_DFORK ? "data" : "reso",
        (intmax_t)off, shmdstrfromoff(off), (intmax_t)len);

    /* dispatch to the correct fd and offset */
    if (eid == ADEID_DFORK) {
        adf = &ad->ad_data_fork;
        lock.l_start = off;
    } else {
        adf = ad->ad_rfp;
        lock.l_start = off + ad_getentryoff(ad, ADEID_RFORK);
    }

    /* No fd to probe (rfork not open / no sidecar): nothing of another process
     * can be locked here through this handle. */
    if (adf->adf_fd == -1) {
        return 0;
    }

    /* zone clamp — data-zone requests only, by subtraction against the actual
     * lock position (for ADEID_RFORK l_start already includes the resource-fork
     * entry offset).  An explicit band probe (off >= AD_FILELOCK_BASE) reads the
     * sentinel zone and is left unclamped. */
    if (off < AD_FILELOCK_BASE) {
        if (lock.l_start >= BYTELOCK_MAX) {
            /* nothing in the data zone left to test */
            return 0;
        }

        /* compare by subtraction: `l_start + len` overflows off_t for huge len */
        off_t zone_max = BYTELOCK_MAX - lock.l_start;   /* >= 1 */

        if (len == 0 || len > zone_max) {
            len = zone_max;
        }

        if (len <= 0) {
            return 0;
        }
    }

    /* explicit F_WRLCK (sees a peer's F_RDLCK band entry on any fd mode), kernel
     * F_GETLK only, no adf_lock[] scan */
    lock.l_type   = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_len    = len;

    if (set_lock(adf->adf_fd, F_GETLK, &lock) < 0) {
        /* EACCES/EAGAIN: kernel declined to answer — treat as locked/
         * indeterminate (matches testlock()).  Any other error is a hard -1;
         * the caller must refuse, never proceed. */
        LOG(log_debug, logtype_ad,
            "ad_testlock_range: F_GETLK fd=%d off=%jd len=%jd: %s",
            adf->adf_fd, (intmax_t)lock.l_start, (intmax_t)lock.l_len,
            strerror(errno));
        return (errno == EACCES || errno == EAGAIN) ? 1 : -1;
    }

    LOG(log_debug, logtype_ad, "ad_testlock_range: END: %d",
        (lock.l_type == F_UNLCK) ? 0 : 1);
    return (lock.l_type == F_UNLCK) ? 0 : 1;
}

/*!
 * @brief Probe with one F_GETLK whether another process holds any lock on a
 * fork descriptor
 *
 * The negative-case fast path. One F_GETLK spans the entire descriptor of fork
 * eid, unclamped: l_start 0 and l_len 0 run to infinity. On the data descriptor
 * it covers the data content zone and the entire share-mode band, a superset of
 * every ad_testlock_range dimension there. A clear result proves the descriptor
 * holds nothing and lets the caller skip the per-dimension content and band
 * probes. A hit forces the per-dimension resolution.
 *
 * This is the one probe that must span the band, so unlike ad_testlock_range it
 * is not zone-clamped. That is sound only because its sole use is the
 * all-or-nothing fast path, never a per-dimension result. Like
 * ad_testlock_range it asks only the kernel and probes with an explicit F_WRLCK
 * so a peer's F_RDLCK band shows. It fails safe: F_GETLK failing with EACCES or
 * EAGAIN counts as a lock.
 *
 * Callers pass ADEID_DFORK. The resource fork descriptor carries only a content
 * range and no band, so a whole-descriptor probe there would force the range
 * probe anyway. ad_testlock_range reads it directly.
 *
 * @param[in] ad   AppleDouble whose forks are open
 * @param[in] eid  ADEID_DFORK, or ADEID_RFORK
 * @returns 1 when a lock exists, 0 when the descriptor is clear or absent, -1
 *          on a hard error the caller fails closed on
 */
int ad_testlock_whole(struct adouble *ad, int eid)
{
    const struct ad_fd *adf;
    struct flock  lock;
    adf = (eid == ADEID_DFORK) ? &ad->ad_data_fork : ad->ad_rfp;

    if (adf->adf_fd == -1) {
        return 0;
    }

    lock.l_type   = F_WRLCK;   /* sees a peer's F_RDLCK band too */
    lock.l_whence = SEEK_SET;
    lock.l_start  = 0;
    lock.l_len    = 0;         /* whole fd to infinity — deliberately unclamped */

    if (set_lock(adf->adf_fd, F_GETLK, &lock) < 0) {
        return (errno == EACCES || errno == EAGAIN) ? 1 : -1;   /* fail safe */
    }

    return (lock.l_type == F_UNLCK) ? 0 : 1;
}

/*!
 * @brief Return if a file is open by another process.
 *
 * Optimized for the common case:
 * - there's no locks held by another process (clients)
 * - or we already know the answer and don't need to test (attrbits)
 *
 * @param[in,out] ad       handle
 * @param[in] attrbits     forks opened by us
 * @returns                bitflags ATTRBIT_DOPEN | ATTRBIT_ROPEN if
 *                         other process has fork of file opened
 */
uint16_t ad_openforks(struct adouble *ad, uint16_t attrbits)
{
    uint16_t ret = 0;
    off_t off;
    off_t len;

    if (ad_data_fileno(ad) == -1) {
        return 0;
    }

    if (!(attrbits & (ATTRBIT_DOPEN | ATTRBIT_ROPEN))) {
        /* Test all 4 locks at once */
        off = AD_FILELOCK_OPEN_WR;
        len = 4;

        if (testlock(&ad->ad_data_fork, off, len) == 0) {
            return 0;
        }
    }

    /* either there's a lock or we already know one fork is open */

    if (!(attrbits & ATTRBIT_DOPEN)) {
        off = AD_FILELOCK_OPEN_WR;
        ret = testlock(&ad->ad_data_fork, off, 2) > 0 ? ATTRBIT_DOPEN : 0;
    }

    if (!(attrbits & ATTRBIT_ROPEN)) {
        off = AD_FILELOCK_RSRC_OPEN_WR;
        ret |= testlock(&ad->ad_data_fork, off, 2) > 0 ? ATTRBIT_ROPEN : 0;
    }

    return ret;
}
