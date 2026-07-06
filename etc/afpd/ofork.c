/*
 * Copyright (c) 1996 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h> /* works around a bug */
#include <unistd.h>

#include <bstrlib.h>

#include <atalk/ea.h>
#include <atalk/fce_api.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/server_ipc.h>
#ifdef WITH_SPOTLIGHT
#include <atalk/spotlight.h>
#endif
#include <atalk/unix.h>
#include <atalk/util.h>

#include "ad_cache.h"
#include "desktop.h"
#include "dircache.h"
#include "directory.h"
#include "fork.h"
#include "virtual_icon.h"
#include "volume.h"

/* we need to have a hashed list of oforks (by dev inode) */
#define OFORK_HASHSIZE  64
/* forks hashed by dev/inode */
static struct ofork *ofork_table[OFORK_HASHSIZE];
/* point to allocated table of open forks pointers */
static struct ofork **oforks = NULL;
static int          nforks = 0;
static u_short      lastrefnum = 0;


/* OR some of each character for the hash */
static unsigned long hashfn(const struct file_key *key)
{
    return key->inode & (OFORK_HASHSIZE - 1);
}

static void of_hash(struct ofork *of)
{
    struct ofork **table;
    table = &ofork_table[hashfn(&of->key)];

    if ((of->next = *table) != NULL) {
        (*table)->prevp = &of->next;
    }

    *table = of;
    of->prevp = table;
}

static void of_unhash(struct ofork *of)
{
    if (of->prevp) {
        if (of->next) {
            of->next->prevp = of->prevp;
        }

        *(of->prevp) = of->next;
        /* Idempotent: a second call short-circuits on the prevp guard. */
        of->prevp = NULL;
        of->next = NULL;
    }
}

void of_pforkdesc(FILE *f)
{
    int ofrefnum;

    if (!oforks) {
        return;
    }

    for (ofrefnum = 0; ofrefnum < nforks; ofrefnum++) {
        if (oforks[ofrefnum] != NULL) {
            fprintf(f, "%d <%s>\n", ofrefnum, of_name(oforks[ofrefnum]));
        }
    }
}

int of_flush(const struct vol *vol)
{
    int refnum;

    if (!oforks) {
        return 0;
    }

    for (refnum = 0; refnum < nforks; refnum++) {
        if (oforks[refnum] != NULL && (oforks[refnum]->of_vol == vol) &&
                flushfork(oforks[refnum]) < 0) {
            LOG(log_error, logtype_afpd, "of_flush: %s", strerror(errno));
        }
    }

    return 0;
}

int of_rename(const struct vol *vol,
              struct ofork *s_of,
              struct dir *olddir, const char *oldpath _U_,
              struct dir *newdir, const char *newpath)
{
    struct ofork *of, *next;
    int done = 0;

    if (!s_of) {
        return AFP_OK;
    }

    next = ofork_table[hashfn(&s_of->key)];

    while ((of = next)) {
        next = next->next; /* so we can unhash and still be all right. */

        if (vol == of->of_vol
                && olddir->d_did == of->of_did
                && s_of->key.dev == of->key.dev
                && s_of->key.inode == of->key.inode) {
            if (!done) {
                free(of_name(of));

                if ((of_name(of) = strdup(newpath)) == NULL) {
                    return AFPERR_MISC;
                }

                done = 1;
            }

            if (newdir != olddir) {
                of->of_did = newdir->d_did;
            }
        }
    }

    return AFP_OK;
}

struct ofork *
of_alloc(struct vol *vol,
         struct dir    *dir,
         char      *path,
         uint16_t     *ofrefnum,
         const int      eid,
         struct adouble *ad,
         struct stat    *st)
{
    struct ofork        *of;
    uint16_t       refnum, of_refnum;
    int         i;

    if (!oforks) {
        /* Two ceilings: the 16-bit protocol max (0xffff), and half the realised
         * fd limit -- a fork costs ~2 fds (data fork + sidecar/resource fd) on
         * both AD and EA backends -- so we never advertise a slot whose backing
         * open(2)s would EMFILE.  getdtablesize() read once. */
        int fdlimit = getdtablesize();
        int fdcap   = fdlimit / 2;

        /* Floor at 1 so a degenerate/-1 fd limit can't make nforks 0 (would
         * divide by zero at "refnum % nforks" below). */
        if (fdcap < 1) {
            fdcap = 1;
        }

        nforks = (fdcap < 0xffff) ? fdcap : 0xffff;
        oforks = (struct ofork **) calloc(nforks, sizeof(struct ofork *));

        if (!oforks) {
            LOG(log_error, logtype_afpd,
                "of_alloc: cannot allocate %d-slot ofork table: %s",
                nforks, strerror(errno));
            return NULL;
        }

        LOG(log_debug, logtype_afpd,
            "of_alloc: ofork table sized to %d slots (protocol max 0xffff, "
            "fd limit %d, ~2 fds/fork)", nforks, fdlimit);
    }

    for (refnum = ++lastrefnum, i = 0; i < nforks; i++, refnum++) {
        /* cf AFP3.0.pdf, File fork page 40 */
        if (!refnum) {
            refnum++;
        }

        if (oforks[refnum % nforks] == NULL) {
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

    if (i == nforks) {
        LOG(log_error, logtype_afpd, "of_alloc: maximum number of forks exceeded.");
        return NULL;
    }

    of_refnum = refnum % nforks;

    if ((oforks[of_refnum] =
                (struct ofork *)malloc(sizeof(struct ofork))) == NULL) {
        LOG(log_error, logtype_afpd, "of_alloc: malloc: %s", strerror(errno));
        return NULL;
    }

    of = oforks[of_refnum];

    /* see if we need to allocate space for the adouble struct */
    if (!ad) {
        ad = malloc(sizeof(struct adouble));

        if (!ad) {
            LOG(log_error, logtype_afpd, "of_alloc: malloc: %s", strerror(errno));
            free(of);
            oforks[of_refnum] = NULL;
            return NULL;
        }

        /* initialize to zero. This is important to ensure that
           ad_open really does reinitialize the structure. */
        ad_init(ad, vol);

        if ((ad->ad_name = strdup(path)) == NULL) {
            LOG(log_error, logtype_afpd, "of_alloc: malloc: %s", strerror(errno));
            free(ad);
            free(of);
            oforks[of_refnum] = NULL;
            return NULL;
        }
    } else {
        /* Increase the refcount on this struct adouble. This is
           decremented again in oforc_dealloc. */
        ad_ref(ad);
    }

    of->of_ad = ad;
    of->of_vol = vol;
    of->of_did = dir->d_did;
    *ofrefnum = refnum;
    of->of_refnum = refnum;
    of->key.dev = st->st_dev;
    of->key.inode = st->st_ino;

    if (eid == ADEID_DFORK) {
        of->of_flags = AFPFORK_DATA;
    } else {
        of->of_flags = AFPFORK_RSRC;
    }

    of_hash(of);
    return of;
}

struct ofork *of_find(const uint16_t ofrefnum)
{
    struct ofork *of;

    if (!oforks || !nforks) {
        return NULL;
    }

    of = oforks[ofrefnum % nforks];

    if (of && of->of_refnum != ofrefnum) {
        /* Slot collision: different fork now occupies this slot. */
        LOG(log_error, logtype_afpd,
            "of_find(%" PRIu16 "): slot %d occupied by refnum %" PRIu16
            " (\"%s\"), requested fork already closed",
            ofrefnum, ofrefnum % nforks,
            of->of_refnum, of_name(of));
        return NULL;
    }

    return of;
}

/* -------------------------- */
int of_stat(const struct vol *vol, struct path *path)
{
    int ret;
    path->st_errno = 0;
    path->st_valid = 1;

    if ((ret = ostat(path->u_name, &path->st, vol_syml_opt(vol))) < 0) {
        LOG(log_debug, logtype_afpd, "of_stat('%s/%s': %s)",
            cfrombstr(curdir->d_fullpath), path->u_name, strerror(errno));
        path->st_errno = errno;
    }

    return ret;
}

int of_fstatat(int dirfd, struct path *path)
{
    int ret;
    path->st_errno = 0;
    path->st_valid = 1;

    if ((ret = fstatat(dirfd, path->u_name, &path->st, AT_SYMLINK_NOFOLLOW)) < 0) {
        path->st_errno = errno;
    }

    return ret;
}

/*!
   @brief stat the current directory.
   @note stat(".") works even if "." is deleted thus
   we have to stat ../name because we want to know if it's there
*/
int of_statdir(struct vol *vol, struct path *path)
{
    static char pathname[MAXPATHLEN + 1] = "../";
    int ret;
    size_t len;
    struct dir *dir;

    if (*path->m_name) {
        /* not curdir */
        return of_stat(vol, path);
    }

    path->st_errno = 0;
    path->st_valid = 1;
    /* FIXME, what about: we don't have r-x perm anymore ? */
    len = blength(path->d_dir->d_u_name);

    if (len > (MAXPATHLEN - 3)) {
        len = MAXPATHLEN - 3;
    }

    strncpy(pathname + 3, cfrombstr(path->d_dir->d_u_name), len + 1);
    LOG(log_debug, logtype_afpd, "of_statdir: stating: '%s'", pathname);

    if (!(ret = ostat(pathname, &path->st, vol_syml_opt(vol)))) {
        return 0;
    }

    path->st_errno = errno;

    /* hmm, can't stat curdir anymore */
    if (errno == EACCES && (dir = dirlookup(vol, curdir->d_pdid))) {
        if (movecwd(vol, dir)) {
            return -1;
        }

        path->st_errno = 0;

        if ((ret = ostat(cfrombstr(path->d_dir->d_u_name), &path->st,
                         vol_syml_opt(vol))) < 0) {
            path->st_errno = errno;
        }
    }

    return ret;
}

/* -------------------------- */
struct ofork *of_findname(const struct vol *vol, struct path *path)
{
    struct ofork *of;
    struct file_key key;

    if (!path->st_valid) {
        of_stat(vol, path);
    }

    if (path->st_errno) {
        return NULL;
    }

    key.dev = path->st.st_dev;
    key.inode = path->st.st_ino;

    for (of = ofork_table[hashfn(&key)]; of; of = of->next) {
        if (key.dev == of->key.dev && key.inode == of->key.inode) {
            return of;
        }
    }

    return NULL;
}

/*!
 * @brief Search for open fork by dirfd/name
 *
 * Function call of_fstatat with dirfd and path and uses dev and ino
 * to search the open fork table.
 *
 * @param[in] dirfd         directory fd
 * @param[in,out] path      pointer to struct path
 */
struct ofork *of_findnameat(int dirfd, struct path *path)
{
    struct ofork *of;
    struct file_key key;

    if (! path->st_valid) {
        of_fstatat(dirfd, path);
    }

    if (path->st_errno) {
        return NULL;
    }

    key.dev = path->st.st_dev;
    key.inode = path->st.st_ino;

    for (of = ofork_table[hashfn(&key)]; of; of = of->next) {
        if (key.dev == of->key.dev && key.inode == of->key.inode) {
            return of;
        }
    }

    return NULL;
}

/*!
 * @brief Conflict GET (F_GETLK only): read another holder's locks on a file.
 *
 * Reuses a held fork's fd when one exists, else opens+closes a transient one.
 * Reading through a held fd (never a transient open+close on an inode we hold a
 * lock on) is what keeps the held byte locks from being dropped on close.  The
 * rfork is never opened onto of->of_ad (would corrupt ad_rlen / underflow the v2
 * meta refcount).
 *
 * @param[in]  vol          volume the target lives on
 * @param[in]  dirfd        directory fd for openat-style resolution, or -1
 * @param[in]  path         target; both fork inodes derive from it
 * @param[in]  df_get       probe data-fork content if non-zero
 * @param[in]  df_off       data-fork content offset
 * @param[in]  df_len       data-fork content length (0 = whole data zone)
 * @param[in]  rf_get       probe resource-fork content if non-zero
 * @param[in]  rf_off       resource-fork content offset
 * @param[in]  rf_len       resource-fork content length (0 = whole zone)
 * @param[in]  band_request share-mode band bitmap; OR together the per-offset
 *                          *_BIT aliases from adouble.h (or ALL_BAND_BITS for all
 *                          ten).  Bit n maps to offset AD_FILELOCK_BASE + n.
 * @param[in]  try_root     retry the open as root on EACCES (the band is the
 *                          server's own bookkeeping, not a user permission)
 * @param[out] df_locked    set if the data-fork content range is locked
 * @param[out] rf_locked    set if the resource-fork content range is locked
 * @param[out] band_held    positional bitmap of held band bits (same positions
 *                          as band_request)
 *
 * @returns OF_LOCKS_OK (0)    outputs valid (all 0 == nothing locked)
 *          OF_LOCKS_NOENT (1) target absent; outputs stay zeroed
 *          OF_LOCKS_ERROR(-1) lock state indeterminate -> caller must fail closed
 *          Outputs are zeroed on entry and only populated on OF_LOCKS_OK, so a
 *          caller that ignores the return still reads zeros, never stale data -
 *          but the return MUST be checked (zeros do not mean "no locks").
 *
 * Data/band fd is fail-closed (any non-ENOENT failure -> ERROR).  The
 * rfork-content leg is best-effort (rf_locked stays 0 on open failure / the
 * HAVE_EAFD && SOLARIS skip; the rfork band is still read via the data fd).
 */
int of_get_locks(const struct vol *vol, int dirfd, struct path *path,
                 int df_get, off_t df_off, off_t df_len,
                 int rf_get, off_t rf_off, off_t rf_len,
                 uint16_t band_request, int try_root,
                 int *df_locked, int *rf_locked, uint16_t *band_held)
{
    struct ofork   *of;
    struct adouble  addata;
    struct adouble  adrf;
    struct adouble *dadp = NULL;
    struct adouble *radp = NULL;
    uint16_t        held = 0;
    int             d_opened = 0, r_opened = 0, rc = OF_LOCKS_OK;
    int             dflk = 0, rflk = 0;
    int             need_data = (df_get || band_request);
    *df_locked = 0;
    *rf_locked = 0;
    *band_held = 0;
    of = (dirfd != -1) ? of_findnameat(dirfd, path) : of_findname(vol, path);

    /* of_findname[at]() stat()s the target (filling path->st_errno) and returns
     * NULL when it does not exist.  Map a provably-absent target to NOENT here, so
     * the tri-valued contract holds for every request shape - including an
     * rfork-only probe, whose ADFLAGS_NORF open would otherwise succeed (no rfork
     * is not an error) and leave a missing file reported as OF_LOCKS_OK. */
    if (of == NULL && path->st_errno == ENOENT) {
        rc = OF_LOCKS_NOENT;
        goto done;
    }

    /* --- data fd: carries data content + the whole band --- */
    if (need_data) {
        if (of != NULL && (ad_data_fileno(of->of_ad) >= 0
                           || ad_data_fileno(of->of_ad) == AD_SYMLINK)) {
            dadp = of->of_ad;            /* reuse held fd, never close */
        } else if (of != NULL) {
            /* held ofork with a closed data fd: GETting through -1 would
             * short-circuit to "no conflict" (fail-OPEN), so refuse */
            LOG(log_error, logtype_afpd,
                "of_get_locks(\"%s\"): held ofork has no data fd (refnum %" PRIu16
                ") - failing closed", path->u_name, of->of_refnum);
            rc = OF_LOCKS_ERROR;
            goto done;
        }

        if (dadp == NULL) {
            /* DF|RDONLY, no SETSHRMD (a F_GETLK test needs no write access; the
             * probe sets F_WRLCK explicitly regardless of fd mode) and no HF
             * (metadata is not a lock surface). */
            int dflags = ADFLAGS_DF | ADFLAGS_RDONLY;
            int derr, oerr;
            ad_init(&addata, vol);
            derr = ad_openat(&addata, dirfd, path->u_name, dflags);
            oerr = errno;

            if (derr < 0 && oerr == EACCES && try_root) {
                become_root();
                derr = ad_openat(&addata, dirfd, path->u_name, dflags);
                oerr = errno;            /* before unbecome_root: seteuid sets errno */
                unbecome_root();
            }

            if (derr == 0) {
                dadp = &addata;
                d_opened = 1;
            } else if (oerr == ENOENT) {
                rc = OF_LOCKS_NOENT;
                goto done;
            } else {
                rc = OF_LOCKS_ERROR;     /* fail closed */
                goto done;
            }
        }
    }

    /* --- rfork fd: separate inode; never opened onto of->of_ad --- */
    if (rf_get) {
        if (of != NULL && AD_RSRC_OPEN(of->of_ad)) {
            radp = of->of_ad;            /* reuse held rfork, never close */
        } else if (of != NULL) {
            /* hold a fork but not the rfork.  On HAVE_EAFD && SOLARIS the rfork's
             * O_XATTR fd hangs off the data fd, so a private open here would open a
             * second fd to an inode we hold a lock on, and closing it would drop
             * that lock -> skip.  Other backends open the rfork as a separate
             * object and are safe. */
#if !(defined(HAVE_EAFD) && defined(SOLARIS))
            int rerr, roerr;
            ad_init(&adrf, vol);
            rerr = ad_openat(&adrf, dirfd, path->u_name,
                             ADFLAGS_RF | ADFLAGS_NORF | ADFLAGS_RDONLY);
            roerr = errno;

            if (rerr < 0 && roerr == EACCES && try_root) {
                become_root();
                rerr = ad_openat(&adrf, dirfd, path->u_name,
                                 ADFLAGS_RF | ADFLAGS_NORF | ADFLAGS_RDONLY);
                /* roerr is not re-read: any failure here is unconditional ERROR */
                unbecome_root();
            }

            if (rerr == 0) {
                /* ADFLAGS_NORF returns 0 with no rfork opened when none is present;
                 * probe only when the rfork actually opened, but close whatever did
                 * open (the base fd may have, e.g. on Solaris). */
                r_opened = 1;

                if (AD_RSRC_OPEN(&adrf)) {
                    radp = &adrf;
                }
            } else {
                /* ADFLAGS_NORF masks only a missing rfork, not a hard error, so a
                 * failure here means the rfork lock state is indeterminate: fail
                 * closed rather than let the caller read it as "unlocked". */
                rc = OF_LOCKS_ERROR;
                goto done;
            }

#endif
        } else {
            /* no fork held: a transient open strands nothing.  HAVE_EAFD &&
             * SOLARIS needs DF too (the O_XATTR base fd). */
            int rflags = ADFLAGS_RF | ADFLAGS_NORF | ADFLAGS_RDONLY;
            int rerr, roerr;
#if defined(HAVE_EAFD) && defined(SOLARIS)
            rflags |= ADFLAGS_DF;
#endif
            ad_init(&adrf, vol);
            rerr = ad_openat(&adrf, dirfd, path->u_name, rflags);
            roerr = errno;

            if (rerr < 0 && roerr == EACCES && try_root) {
                become_root();
                rerr = ad_openat(&adrf, dirfd, path->u_name, rflags);
                roerr = errno;
                unbecome_root();
            }

            if (rerr == 0) {
                /* close whatever opened (on Solaris ADFLAGS_DF opens the O_XATTR
                 * base fd even when the rfork itself does not open); probe only when
                 * the rfork actually opened. */
                r_opened = 1;

                if (AD_RSRC_OPEN(&adrf)) {
                    radp = &adrf;
                }
            } else if (roerr == ENOENT) {
                /* target vanished after the lookup (race): definitely absent */
                rc = OF_LOCKS_NOENT;
                goto done;
            } else {
                /* hard error (not masked by NORF): indeterminate -> fail closed */
                rc = OF_LOCKS_ERROR;
                goto done;
            }
        }
    }

    /* rfork content: one range probe (no band lives on the rfork fd) */
    if (rf_get && radp != NULL) {
        int r = ad_testlock_range(radp, ADEID_RFORK, rf_off, rf_len);

        if (r < 0) {
            rc = OF_LOCKS_ERROR;
            goto done;
        }

        rflk = (r > 0);
    }

    /* data fd: whole-fd fast path when >= 2 dimensions requested, else probe the
     * one dimension directly.  band_request & (band_request-1) tests ">= 2 bits". */
    if (dadp != NULL) {
        int band_multi = (band_request & (band_request - 1)) != 0;
        int use_whole  = (df_get && band_request != 0) || band_multi;
        int resolve    = 1;

        if (use_whole) {
            int whole = ad_testlock_whole(dadp, ADEID_DFORK);

            if (whole < 0) {
                rc = OF_LOCKS_ERROR;
                goto done;
            }

            resolve = (whole > 0);       /* clear => data fd holds nothing */
        }

        if (resolve) {
            if (df_get) {
                int r = ad_testlock_range(dadp, ADEID_DFORK, df_off, df_len);

                if (r < 0) {
                    rc = OF_LOCKS_ERROR;
                    goto done;
                }

                dflk = (r > 0);
            }

            for (int bit = 0; bit < AD_FILELOCK_BAND_BITS; bit++) {
                int r;

                if (!(band_request & (1U << bit))) {
                    continue;
                }

                r = ad_testlock_range(dadp, ADEID_DFORK, AD_FILELOCK_BASE + bit, 1);

                if (r < 0) {
                    rc = OF_LOCKS_ERROR;
                    goto done;
                }

                if (r > 0) {
                    held |= (uint16_t)(1U << bit);
                }
            }
        }
    }

done:

    if (d_opened) {
        ad_close(&addata, ADFLAGS_DF);
    }

    if (r_opened) {
        ad_close(&adrf, ADFLAGS_RF | ADFLAGS_HF
#if defined(HAVE_EAFD) && defined(SOLARIS)
                 | ADFLAGS_DF
#endif
                );
    }

    if (rc == OF_LOCKS_OK) {
        *df_locked = dflk;
        *rf_locked = rflk;
        *band_held = held;
    }

    return rc;
}

void of_dealloc(struct ofork *of)
{
    if (!oforks) {
        return;
    }

    /* pairs each removal with its afp_openfork open */
    LOG(log_debug, logtype_afpd,
        "of_dealloc(refnum %" PRIu16 ", \"%s\", flags 0x%x, dev/ino %ju/%ju)",
        of->of_refnum, of_name(of), of->of_flags,
        (uintmax_t)of->key.dev, (uintmax_t)of->key.inode);
    of_unhash(of);
    oforks[of->of_refnum % nforks] = NULL;

    /* decrease refcount; guard so a stray double-dealloc on a shared adouble is
     * logged (and asserts in non-NDEBUG builds) instead of silently absorbed. */
    if (of->of_ad->ad_refcount > 0) {
        if (--of->of_ad->ad_refcount == 0) {
            free(of->of_ad->ad_name);
            free(of->of_ad);
        }
    } else {
        LOG(log_error, logtype_afpd,
            "of_dealloc: ad_refcount underflow (already %d) — stray double "
            "of_dealloc()?", of->of_ad->ad_refcount);
        AFP_ASSERT(of->of_ad->ad_refcount > 0);
    }

    free(of);
}

/* --------------------------- */
int of_closefork(const AFPObj *obj, struct ofork *ofork)
{
    struct timeval      tv;
    int         adflags = 0;
    int                 ret;
    int                 saved_errno = 0;
    struct dir *dir;
    bstring forkname = NULL;
    bstring forkpath = NULL;
    adflags = 0;

    /* Virtual forks don't have real file descriptors or adouble data */
    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        of_dealloc(ofork);
        return 0;
    }

    if (ofork->of_flags & AFPFORK_DATA) {
        adflags |= ADFLAGS_DF;
    }

    if (ofork->of_flags & AFPFORK_META) {
        adflags |= ADFLAGS_HF;
    }

    if (ofork->of_flags & AFPFORK_RSRC) {
        adflags |= ADFLAGS_RF;
        /* Only set the rfork's length if we're closing the rfork. */
        ad_refresh(NULL, ofork->of_ad);

        if ((ofork->of_flags & AFPFORK_DIRTY) && !gettimeofday(&tv, NULL)) {
            ad_setdate(ofork->of_ad, AD_DATE_MODIFY | AD_DATE_UNIX, tv.tv_sec);
            ad_flush(ofork->of_ad);
        }
    }

    dir = dirlookup(ofork->of_vol, ofork->of_did);

    if (dir == NULL) {
        LOG(log_debug, logtype_afpd, "dirlookup failed for %ju",
            (uintmax_t)ofork->of_did);
    }

    if (dir) {
        const char *upath = mtoupath(ofork->of_vol, of_name(ofork),
                                     ofork->of_did, utf8_encoding(obj));

        if (upath) {
            forkname = bfromcstr(upath);

            if (forkname) {
                forkpath = bformat("%s/%s", bdata(dir->d_fullpath),
                                   bdata(forkname));
            }
        }
    }

    /* Pre-acquire cache pointer before ad_close changes ctime */
    struct dir *cached = NULL;

    if ((ofork->of_flags & (AFPFORK_DIRTY | AFPFORK_MODIFIED))
            && dir && forkname && forkpath) {
        cached = dircache_search_by_name(ofork->of_vol, dir, bdata(forkname),
                                         blength(forkname));
    }

#ifdef WITH_FCE

    /* Someone has used write_fork, assume the file changed and register an FCE */
    if ((ofork->of_flags & AFPFORK_MODIFIED) && forkpath) {
        fce_register(obj, FCE_FILE_MODIFY, bdata(forkpath), NULL);
    }

#endif /* WITH_FCE */
    ad_unlock(ofork->of_ad, ofork->of_refnum,
              ofork->of_flags & AFPFORK_ERROR ? 0 : 1);
#ifdef HAVE_FSHARE_T

    if (obj->options.flags & OPTION_SHARE_RESERV) {
        fshare_t shmd;
        shmd.f_id = ofork->of_refnum;

        if (AD_DATA_OPEN(ofork->of_ad)
                && fcntl(ad_data_fileno(ofork->of_ad), F_UNSHARE, &shmd) < 0) {
            LOG(log_warning, logtype_afpd,
                "of_closefork: F_UNSHARE on data fork failed for refnum %"
                PRIu16 ": %s", ofork->of_refnum, strerror(errno));
        }

        if (AD_RSRC_OPEN(ofork->of_ad)
                && fcntl(ad_reso_fileno(ofork->of_ad), F_UNSHARE, &shmd) < 0) {
            LOG(log_warning, logtype_afpd,
                "of_closefork: F_UNSHARE on rsrc fork failed for refnum %"
                PRIu16 ": %s", ofork->of_refnum, strerror(errno));
        }
    }

#endif
    ret = 0;

    /*
     * Check for 0 byte size resource forks, delete them.
     * Here's the deal:
     * (1) the size must be 0
     * (2) the fork must refer to a resource fork
     * (3) the refcount must be 1 which means this fork has the last
     *     reference to the adouble struct and the subsequent
     *     ad_close() will close the assoiciated fd.
     * (4) nobody else has the resource fork open
     *
     * We only do this for ._ AppleDouble resource forks, not for
     * xattr resource forks, because the test-suite then fails several
     * tests on Solaris, the reason for that still needs to be
     * determined.
     */
    if ((ofork->of_ad->ad_rlen == 0)
            && (ofork->of_flags & AFPFORK_RSRC)
            && (ofork->of_ad->ad_rfp->adf_refcount == 1)
            && (ad_openforks(ofork->of_ad, ATTRBIT_DOPEN) == 0)) {
#ifndef HAVE_EAFD
        (void)unlink(ofork->of_ad->ad_ops->ad_path(
                         mtoupath(ofork->of_vol,
                                  of_name(ofork),
                                  ofork->of_did,
                                  utf8_encoding(obj)),
                         0));
#endif
    }

    /*
     * When the Finder removes a custom icon it empties the Icon\r
     * resource fork but does not send AFP_DELETE.  Detect this by
     * reading the resource fork header and checking whether the
     * resource data length is zero (i.e. no actual resources).
     * If so, delete the now-useless physical file plus its CNID
     * entry so the volume falls back to the virtual icon.
     */
    int delete_icon = 0;

    if ((ofork->of_flags & AFPFORK_RSRC)
            && ofork->of_did == DIRDID_ROOT
            && is_virtual_icon_name(of_name(ofork))
            && virtual_icon_enabled(ofork->of_vol)
            && ofork->of_ad->ad_rfp->adf_refcount == 1
            && ad_openforks(ofork->of_ad, ATTRBIT_DOPEN) == 0
            && forkpath) {
        /*
         * Read the resource fork header and check the data-length
         * field.  Only proceed with deletion when it is zero,
         * confirming the fork contains no actual resources.
         */
        char rfork_hdr[RFORK_HEADER_SIZE];
        ssize_t n = ad_read(ofork->of_ad, ADEID_RFORK,
                            0, rfork_hdr, sizeof(rfork_hdr));
        uint32_t data_len = 0;

        if (n == RFORK_HEADER_SIZE) {
            memcpy(&data_len, rfork_hdr + RFORK_DATA_LEN_OFF,
                   sizeof(data_len));
            data_len = ntohl(data_len);
        }

        if ((n >= 0 && n < RFORK_HEADER_SIZE) || data_len == 0) {
            /* Look up and delete the CNID for this file */
            cnid_t icon_cnid = cnid_get(ofork->of_vol->v_cdb,
                                        ofork->of_did,
                                        of_name(ofork),
                                        strnlen(of_name(ofork), MAXPATHLEN));

            if (icon_cnid != CNID_INVALID) {
                cnid_delete(ofork->of_vol->v_cdb, icon_cnid);
            }

            /* Remove from dircache */
            if (cached) {
                dircache_remove(ofork->of_vol, cached, DIRCACHE | DIDNAME_INDEX | QUEUE_INDEX);
                /* invalidated by dircache_remove; skip cache refresh and hint lookup below */
                cached = NULL;
            }

            delete_icon = 1;
        }
    }

    /* Refresh cache after fork close if metadata was modified */
    if (cached) {
        struct stat post_st;

        if (ostat(bdata(forkpath), &post_st, 0) == 0) {
            int dflags = DCMOD_STAT;

            /* If resource fork was dirty, AD header was modified too */
            if ((ofork->of_flags & AFPFORK_RSRC) && (ofork->of_flags & AFPFORK_DIRTY)) {
                dflags |= DCMOD_AD;
            }

            dir_modify(ofork->of_vol, cached, &(struct dir_modify_args) {
                .flags = dflags,
                .st = &post_st,
                .adp = (dflags & DCMOD_AD) ? ofork->of_ad : NULL,
            });
        }
    }

    /* Send hint to afpd siblings — only if fork was actually modified.
     * Uses the already-resolved cached->d_did when available to avoid
     * cnid_get() DB lookups. Falls back to cnid_get() when the file
     * was modified but not in the local dircache (rare). */
    if ((ofork->of_flags & (AFPFORK_DIRTY | AFPFORK_MODIFIED))
            && ofork->of_vol) {
        cnid_t hint_cnid = (cached && cached->d_did != CNID_INVALID)
                           ? cached->d_did : CNID_INVALID;

        if (hint_cnid == CNID_INVALID && forkname) {
            size_t ulen = blength(forkname);
            hint_cnid = cnid_get(ofork->of_vol->v_cdb, ofork->of_did,
                                 bdata(forkname), ulen);
        }

        if (hint_cnid != CNID_INVALID) {
            ipc_send_cache_hint(obj, ofork->of_vol->v_vid, hint_cnid,
                                CACHE_HINT_REFRESH);
        }
    }

    if (ad_close(ofork->of_ad, adflags | ADFLAGS_SETSHRMD) < 0) {
        /* A failed close(2) is a deferred flush error: the fd is already
         * detached and locks were released by F_UNLCK before the close, so
         * nothing is stranded.  Log and deallocate normally; the client gets
         * the error code.  Stash errno: the unlink/dealloc/bdestroy below would
         * otherwise clobber it before callers read it. */
        saved_errno = errno;
        ret = -1;
        LOG(log_error, logtype_afpd,
            "of_closefork: ad_close failed for fork %" PRIu16 " (\"%s\") "
            "dev=%ju ino=%ju: %s",
            ofork->of_refnum, of_name(ofork),
            (uintmax_t)ofork->key.dev, (uintmax_t)ofork->key.inode,
            strerror(saved_errno));
    }

#ifdef WITH_SPOTLIGHT

    if (ret == 0 && (ofork->of_flags & AFPFORK_MODIFIED) && forkpath) {
        sl_index_event(obj, ofork->of_vol, SL_INDEX_FILE_MODIFY,
                       bdata(forkpath), NULL);
    }

#endif /* WITH_SPOTLIGHT */

    /* Unlink the emptied Icon\r file after ad_close has released fds */
    if (delete_icon && forkpath && bdata(forkpath)) {
        (void)unlink(bdata(forkpath));
    }

    of_dealloc(ofork);

    if (forkpath) {
        bdestroy(forkpath);
    }

    if (forkname) {
        bdestroy(forkname);
    }

    if (ret < 0) {
        errno = saved_errno;
    }

    return ret;
}

struct adouble *of_ad(const struct vol *vol, struct path *path,
                      struct adouble *ad)
{
    struct ofork        *of;
    struct adouble      *adp;

    if ((of = of_findname(vol, path))) {
        adp = of->of_ad;
    } else {
        ad_init(ad, vol);
        adp = ad;
    }

    return adp;
}

/*!
   close all forks for a volume
*/
void of_closevol(const AFPObj *obj, const struct vol *vol)
{
    int refnum;

    if (!oforks) {
        return;
    }

    for (refnum = 0; refnum < nforks; refnum++) {
        if (oforks[refnum] != NULL && oforks[refnum]->of_vol == vol) {
            if (of_closefork(obj, oforks[refnum]) < 0) {
                LOG(log_error, logtype_afpd, "of_closevol: %s", strerror(errno));
            }
        }
    }

    return;
}

/*!
   close all forks
*/
void of_close_all_forks(const AFPObj *obj)
{
    int refnum;

    if (!oforks) {
        return;
    }

    for (refnum = 0; refnum < nforks; refnum++) {
        if (oforks[refnum] != NULL) {
            if (of_closefork(obj, oforks[refnum]) < 0) {
                LOG(log_error, logtype_afpd, "of_close_all_forks: %s", strerror(errno));
            }
        }
    }

    return;
}
