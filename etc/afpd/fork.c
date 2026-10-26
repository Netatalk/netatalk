/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * Copyright (c) 2010      Frank Lahm
 * Copyright (c) 2026 Andy Lemin (andylemin)
 *
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief Fork commands: FPOpenFork, FPRead, FPWrite, FPCloseFork, byte range
 * locks, and the flush commands
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>

#include <bstrlib.h>

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/asp.h>
#include <atalk/cnid.h>
#include <atalk/dsi.h>
#include <atalk/fce_api.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#ifdef WITH_SPOTLIGHT
#include <atalk/spotlight.h>
#endif
#include <atalk/util.h>

#include "ad_cache.h"
#include "dircache.h"
#include "desktop.h"
#include "directory.h"
#include "file.h"
#include "fork.h"
#include "virtual_icon.h"
#include "volume.h"

static int fork_setmode(const AFPObj *obj, struct adouble *adp, int eid,
                        int access, int ofrefnum);
static int fork_error(const struct ofork *ofork, const char *call,
                      bool is_write);

/*! the reference counts of an adouble's three forks */
struct ad_refs {
    int data;
    int meta;
    int reso;
};

/*!
 * @brief Snapshot the reference counts of an adouble's forks
 *
 * @param[in] ad  adouble about to be opened
 * @returns its counts
 */
static struct ad_refs ad_refs_now(const struct adouble *ad)
{
    return (struct ad_refs) {
        ad->ad_data_refcount, ad->ad_meta_refcount, ad->ad_reso_refcount
    };
}

/*!
 * @brief ADFLAGS_DF, ADFLAGS_HF and ADFLAGS_RF of the forks whose references
 *        rose above a snapshot: what an open took, for its close
 *
 * @param[in] ad      adouble after the open
 * @param[in] before  its counts before the open
 * @returns the ADFLAGS of the forks the open referenced
 */
static int ad_refs_taken(const struct adouble *ad, struct ad_refs before)
{
    return (ad->ad_data_refcount > before.data ? ADFLAGS_DF : 0)
           | (ad->ad_meta_refcount > before.meta ? ADFLAGS_HF : 0)
           | (ad->ad_reso_refcount > before.reso ? ADFLAGS_RF : 0);
}

/*!
 * @brief Whether an AppleDouble descriptor is open read-only
 *
 * @param[in] adf  descriptor and the flags it was opened with
 * @returns 1 for an open read-only descriptor, else 0
 */
static int adf_readonly(const struct ad_fd *adf)
{
    return adf->adf_fd >= 0 && (adf->adf_flags & O_ACCMODE) == O_RDONLY;
}

/*!
 * @brief Turn the virtual Icon file into a real file before a client changes it
 *
 * The write path and FPSetForkParms call it. It creates the Icon file in the
 * volume root, seeds its resource fork with the virtual icon data and marks
 * it invisible; a file another fork created first keeps its content. The
 * call then reopens the fork through ad_open, on the adouble of the
 * session's open fork of the file when there is one, so the open fork holds
 * real descriptors, takes the open-mode locks of its access, and drops its
 * virtual state. A fork whose name a symlink holds on a volume without
 * 'follow symlinks', whose seed, reopen or locks fail, or whose file is
 * write-inhibited where the volume does not ignore it, stays virtual, with
 * nothing open; a file the call created goes again while the name holds it,
 * unless another process claims it, and then counts as created.
 *
 * @param[in]     obj       session object
 * @param[in,out] ofork     open fork on the virtual Icon file
 * @param[in]     is_write  called for an FPWrite, else for FPSetForkParms
 * @returns AFP_OK; AFPERR_LOCK when another open's deny modes refuse the
 *          locks, AFPERR_ACCESS when a symlink holds the name on a volume
 *          without 'follow symlinks', the session's open fork of the file
 *          holds a read-only descriptor the fork needs, or the file is
 *          write-inhibited where the volume does not ignore it, the reply
 *          fork_error() gives a failed create, seed or open, else
 *          AFPERR_MISC
 */
static int materialize_virtual_icon(const AFPObj *obj, struct ofork *ofork,
                                    bool is_write)
{
    const struct vol *vol = ofork->of_vol;
    char path[MAXPATHLEN + 1];
    struct adouble icon_ad;
    struct adouble *own = NULL;
    struct path p = { .u_name = path };
    const struct ofork *opened;
    struct ad_refs before;
    struct stat st;
    struct file_key made = { 0, 0 };
    off_t rlen;
    int meta;
    int err;
    int created = 0;
    snprintf(path, sizeof(path), "%s/%s", vol->v_path, VIRTUAL_ICON_NAME);
    ad_init(&icon_ad, vol);

    /* a file another fork made real first keeps its content */
    if (ad_open(&icon_ad, path, ADFLAGS_DF | ADFLAGS_RDWR | ADFLAGS_CREATE
                | ADFLAGS_EXCL, 0666) == 0) {
        /* without following symlinks, a symlink under the name opens as
         * one, and holds no file for the icon */
        const int islink = ad_data_fileno(&icon_ad) == AD_SYMLINK;

        /* the file the call made, the only one its cleanup removes */
        if (!islink && fstat(ad_data_fileno(&icon_ad), &st) == 0) {
            made.dev = st.st_dev;
            made.inode = st.st_ino;
        }

        ad_close(&icon_ad, ADFLAGS_DF);

        if (islink) {
            return AFPERR_ACCESS;
        }

        created = 1;
    } else if (errno != EEXIST) {
        return fork_error(ofork, "materialize_virtual_icon", is_write);
    } else if (lstat(path, &st) != 0) {
        /* DragonflyBSD answers a refused exclusive create with EEXIST */
        errno = EACCES;
        return fork_error(ofork, "materialize_virtual_icon", is_write);
    }

    /* Seed the resource fork and set FinderInfo invisible flag */
    if (created) {
        if (ad_open(&icon_ad, path,
                    ADFLAGS_HF | ADFLAGS_RF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                    0666) != 0) {
            err = fork_error(ofork, "materialize_virtual_icon", is_write);
            goto stay_virtual;
        }

        const ssize_t n = ad_write(&icon_ad, ADEID_RFORK, 0, 0,
                                   (const char *)ofork->of_virtual_data,
                                   (size_t)ofork->of_virtual_len);
        char *finfo = ad_entry(&icon_ad, ADEID_FINDERI);

        if (finfo) {
            memset(finfo, 0, ADEDLEN_FINDERI);
            uint16_t flags = htons(FINDERINFO_INVISIBLE);
            memcpy(finfo + FINDERINFO_FRFLAGOFF,
                   &flags, sizeof(flags));
        }

        if (n >= 0 && n < (ssize_t)ofork->of_virtual_len) {
            /* pwrite() stops short only at a size limit */
            errno = ENOSPC;
        }

        err = n == (ssize_t)ofork->of_virtual_len && ad_flush(&icon_ad) == 0
              ? AFP_OK
              : fork_error(ofork, "materialize_virtual_icon", is_write);
        ad_close(&icon_ad, ADFLAGS_HF | ADFLAGS_RF);

        if (err != AFP_OK) {
            goto stay_virtual;
        }
    }

    if (!created && (opened = of_findname(vol, &p)) != NULL) {
        /* the fork's own read-write open needs the data descriptor, and a
         * resource fork's also the resource one; a lender's read-only one
         * means the user cannot write it, and the emulated one of a symlink
         * renamed onto the name since the create holds no file */
        if (ad_data_fileno(opened->of_ad) < 0
                || adf_readonly(&opened->of_ad->ad_data_fork)
                || ((ofork->of_flags & AFPFORK_RSRC)
                    && adf_readonly(opened->of_ad->ad_rfp))) {
            return AFPERR_ACCESS;
        }

        /* the session's open fork of the file lends its adouble, as in
         * afp_openfork(), so neither fork's close drops the other's locks */
        own = ofork->of_ad;
        ofork->of_ad = opened->of_ad;
        ad_ref(ofork->of_ad);
    } else {
        /* Re-initialize the adouble for real file I/O */
        char *name = ofork->of_ad->ad_name;
        ad_init(ofork->of_ad, vol);
        ofork->of_ad->ad_name = name;
    }

    before = ad_refs_now(ofork->of_ad);
    /* a failed open of a missing resource fork zeroes a lender's length */
    rlen = ofork->of_ad->ad_rlen;

    /* the data fork first, with no create: a file gone from the name stays
     * gone, and a symlink renamed onto it since the create holds no file
     * for the icon */
    if (ad_open(ofork->of_ad, path,
                ADFLAGS_DF | ADFLAGS_SETSHRMD | ADFLAGS_RDWR) < 0) {
        err = fork_error(ofork, "materialize_virtual_icon", is_write);
        goto release;
    }

    if (ad_data_fileno(ofork->of_ad) == AD_SYMLINK) {
        err = AFPERR_ACCESS;
        goto release;
    }

    /* then a resource fork's own, which creates its AppleDouble file */
    if ((ofork->of_flags & AFPFORK_RSRC)
            && ad_open(ofork->of_ad, path,
                       ADFLAGS_RF | ADFLAGS_RDWR | ADFLAGS_CREATE, 0666) < 0) {
        err = fork_error(ofork, "materialize_virtual_icon", is_write);
        goto release;
    }

    /* Open metadata */
    meta = ad_open(ofork->of_ad, path,
                   ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE, 0666) == 0;

    /* WriteInhibit refuses the write, unless the volume ignores it */
    if (meta && !(vol->v_ignattr & ATTRBIT_NOWRITE)) {
        uint16_t attr;
        ad_getattr(ofork->of_ad, &attr);

        if (attr & htons(ATTRBIT_NOWRITE)) {
            err = AFPERR_ACCESS;
            goto release;
        }
    }

    /* the open-mode locks of the fork's access, as afp_openfork() takes them */
    const int eid = (ofork->of_flags & AFPFORK_RSRC) ? ADEID_RFORK
                    : ADEID_DFORK;
    const int access = OPENACC_WR
                       | ((ofork->of_flags & AFPFORK_ACCRD) ? OPENACC_RD : 0);

    if (fork_setmode(obj, ofork->of_ad, eid, access, ofork->of_refnum) < 0) {
        /* a deny mode conflict is the client's news */
        err = errno == EACCES || errno == EAGAIN || errno == EINVAL
              ? AFPERR_LOCK : AFPERR_MISC;
        LOG((err == AFPERR_LOCK ? log_debug : log_error), logtype_afpd,
            "materialize_virtual_icon: open-mode locks refused: %s",
            strerror(errno));
        goto release;
    }

    ofork->of_adflags = ad_refs_taken(ofork->of_ad, before);

    if (meta) {
        ofork->of_flags |= AFPFORK_META;
    }

    if (own) {
        free(own->ad_name);
        free(own);
    }

    /* from the virtual open's synthetic key to the file's */
    if (fstat(ad_data_fileno(ofork->of_ad), &st) == 0) {
        of_rekey(ofork, &st);
    }

    /* Transition from virtual to real fork */
    ofork->of_flags &= ~AFPFORK_VIRTUAL;
    ofork->of_virtual_data = NULL;
    ofork->of_virtual_len = 0;
    LOG(log_debug, logtype_afpd,
        "materialize_virtual_icon: Icon\\r materialized on disk");
    err = AFP_OK;
    goto count_created;
release:
    ad_unlock(ofork->of_ad, ofork->of_refnum);
    ad_close(ofork->of_ad, ad_refs_taken(ofork->of_ad, before));
    ofork->of_ad->ad_rlen = rlen;

    if (own) {
        ad_unref(ofork->of_ad);
        ofork->of_ad = own;
    }

stay_virtual:

    /* the name may hold another process's file by now; deletefile() keeps
     * the call's own when another process claimed it with a deny mode or a
     * byte lock */
    if (!created || lstat(path, &st) != 0 || st.st_dev != made.dev
            || st.st_ino != made.inode
            || deletefile(vol, -1, path, 0, NULL) == AFP_OK) {
        return err;
    }

count_created:

    if (created) {
        /* the root's cached item count follows a new file */
        vol->v_root->d_offcnt++;
#ifdef WITH_FCE
        fce_register(obj, FCE_FILE_CREATE, path, NULL);
#endif /* WITH_FCE */
#ifdef WITH_SPOTLIGHT
        sl_index_event(obj, vol, SL_INDEX_FILE_CREATE, path, NULL);
#endif /* WITH_SPOTLIGHT */
    }

    return err;
}

/*!
 * @brief Pack the file parameters of an open fork
 *
 * Length bits must match the fork. A data length request on a resource fork
 * fails with AFPERR_BITMAP, and a resource length request on a data fork
 * fails too. Virtual forks report synthesized parameters. The stat comes from
 * the fork descriptor when there is one, so a fork that outlived its path
 * still reports the held object.
 *
 * @param[in]  obj     session object
 * @param[in]  ofork   open fork
 * @param[in]  bitmap  AFP file bitmap, FILPBIT_* bits
 * @param[out] buf     reply buffer receiving the parameters
 * @param[out] buflen  receives the bytes written
 * @returns AFP_OK, AFPERR_BITMAP, AFPERR_NOOBJ, AFPERR_MISC, or the result of
 *          getmetadata
 */
static int getforkparams(const AFPObj *obj, struct ofork *ofork,
                         uint16_t bitmap, char *buf, size_t *buflen)
{
    struct path         path;
    struct stat     *st;
    struct adouble  *adp;
    struct dir      *dir;
    struct vol      *vol;
    memset(&path, 0, sizeof(path));

    /* can only get the length of the opened fork */
    if (((bitmap & ((1 << FILPBIT_DFLEN) | (1 << FILPBIT_EXTDFLEN)))
            && (ofork->of_flags & AFPFORK_RSRC))
            ||
            ((bitmap & ((1 << FILPBIT_RFLEN) | (1 << FILPBIT_EXTRFLEN)))
             && (ofork->of_flags & AFPFORK_DATA))) {
        return AFPERR_BITMAP;
    }

    /* Virtual forks use synthesized params */
    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        return virtual_icon_getfilparams(obj, ofork->of_vol, bitmap,
                                         (ofork->of_flags & AFPFORK_RSRC)
                                         ? ATTRBIT_ROPEN : ATTRBIT_DOPEN,
                                         buf, buflen);
    }

    if (! ad_meta_open(ofork->of_ad)) {
        adp = NULL;
    } else {
        adp = ofork->of_ad;
    }

    vol = ofork->of_vol;

    /* The fork can outlive its parent directory's CNID entry (deleted by
     * another session, hint-driven expunge). dirlookup sets afp_errno to
     * distinguish gone (AFPERR_NOOBJ) from transient resolution failure;
     * get_afp_errno screens the internal DID1 sentinel and paths that
     * left it unset. */
    if ((dir = dirlookup(vol, ofork->of_did)) == NULL) {
        return get_afp_errno(AFPERR_NOOBJ);
    }

    if (NULL == (path.u_name = mtoupath(vol, of_name(ofork), dir->d_did,
                                        utf8_encoding(obj)))) {
        return AFPERR_MISC;
    }

    path.m_name = of_name(ofork);
    path.id = 0;
    st = &path.st;

    if (bitmap & ((1 << FILPBIT_DFLEN) | (1 << FILPBIT_EXTDFLEN) |
                  (1 << FILPBIT_FNUM) | (1 << FILPBIT_CDATE) |
                  (1 << FILPBIT_MDATE) | (1 << FILPBIT_BDATE))) {
        if (ad_data_fileno(ofork->of_ad) <= 0) {
            /* 0 is for symlink. A v2 resource-fork-only fork holds no fd
             * on the object itself (the rfork fd is the ._ sidecar, whose
             * inode must not stand in for the object's) — when the held
             * fork's path is gone, proceed with st unfilled: identity
             * comes from the adouble header, dates already do. */
            if (movecwd(vol, dir) < 0) {
                return AFPERR_NOOBJ;
            }

            if (lstat(path.u_name, st) == 0) {
                path.st_valid = 1;
            } else if (adp == NULL || errno != ENOENT) {
                return AFPERR_NOOBJ;
            }
        } else {
            if (fstat(ad_data_fileno(ofork->of_ad), st) < 0) {
                return AFPERR_BITMAP;
            }

            path.st_valid = 1;
            path.st_fd = 1;
        }
    } else if ((bitmap & (1 << FILPBIT_LNAME)) && utf8_encoding(obj)
               && ad_data_fileno(ofork->of_ad) > 0
               && fstat(ad_data_fileno(ofork->of_ad), st) == 0) {
        /* The only name-only bitmap shape whose identity block runs in
         * getmetadata (m_name is always set here, FNUM is in the stat
         * block above): provide the held object's stat so a fork that
         * outlived its path still resolves. Other name-only bitmaps skip
         * the syscall entirely. */
        path.st_valid = 1;
        path.st_fd = 1;
    }

    return getmetadata(obj, vol, bitmap, &path, dir, buf, buflen, adp);
}

/*!
 * @brief Read a fork offset or length from a request
 *
 * A 64-bit field takes 8 bytes in network order. A 32-bit field takes 4
 * bytes, and the call sign-extends it.
 *
 * @param[in,out] ibuf  request cursor, advanced past the field
 * @param[in]     is64  nonzero for the 64-bit field of the Ext commands
 * @returns the value
 */
static off_t get_off_t(char **ibuf, int is64)
{
    uint32_t             temp;
    off_t                 ret;
    ret = 0;
    memcpy(&temp, *ibuf, sizeof(temp));
    ret = ntohl(temp); /* ntohl is unsigned */
    *ibuf += sizeof(temp);

    if (is64) {
        memcpy(&temp, *ibuf, sizeof(temp));
        *ibuf += sizeof(temp);
        ret = ntohl(temp) | ((unsigned long long) ret << 32);
    } else {
        ret = (int)ret; /* sign extend */
    }

    return ret;
}

/*!
 * @brief Write a fork offset or length into a reply in network order
 *
 * @param[in]  offset  value to write
 * @param[out] rbuf    reply position
 * @param[in]  is64    nonzero for the 8-byte field of the Ext commands, 0 for 4
 *                     bytes
 * @returns the number of bytes written, 8 for the 64-bit field and 4 otherwise
 */
static int set_off_t(off_t offset, char *rbuf, int is64)
{
    uint32_t  temp;
    int        ret;
    ret = 0;

    if (is64) {
        temp = htonl((uint64_t) offset >> 32);
        memcpy(rbuf, &temp, sizeof(temp));
        rbuf += sizeof(temp);
        ret = sizeof(temp);
        offset &= 0xffffffff;
    }

    temp = htonl(offset);
    memcpy(rbuf, &temp, sizeof(temp));
    ret += sizeof(temp);
    return ret;
}

/*!
 * @brief Whether an offset or length is negative in the field width of the
 * request
 *
 * With a 64-bit off_t, a 32-bit request also counts a value with bit 31 set
 * as negative.
 *
 * @param[in] is64  nonzero for the 64-bit field of the Ext commands
 * @param[in] val   offset or length
 * @returns 1 when the value is negative, 0 otherwise
 */
static int is_neg(int is64, off_t val)
{
    if (val < 0 || (sizeof(off_t) == 8 && !is64 && (val & 0x80000000U))) {
        return 1;
    }

    return 0;
}

/*!
 * @brief Whether offset plus reqcount passes the largest offset the field
 * width of the request holds, or, in a 64-bit field, passes off_t once the
 * fork's offset in its file is added
 *
 * @param[in] is64      nonzero for the 64-bit field of the Ext commands
 * @param[in] offset    start offset, not negative
 * @param[in] reqcount  byte count, not negative
 * @param[in] base      the fork's offset in its file
 * @returns 1 when the sum passes INT32_MAX for a 32-bit field or INT64_MAX
 *          less base for a 64-bit one, 0 otherwise
 */
static int sum_neg(int is64, off_t offset, off_t reqcount, off_t base)
{
    off_t limit = is64 ? INT64_MAX - base : INT32_MAX;
    return reqcount > limit - offset;
}

/*!
 * @brief F_SHARE deny bits for an FPOpenFork access mode
 *
 * Each deny bit maps separately, and the results combine with a bitwise or.
 * The constants come in as parameters, so a unit test can exercise the
 * mapping without fshare_t.
 *
 * @param[in] access   OPENACC_* access mode
 * @param[in] f_rddny  value for a read deny, F_RDDNY where fshare_t exists
 * @param[in] f_wrdny  value for a write deny, F_WRDNY where fshare_t exists
 * @param[in] f_nodny  value for no deny, F_NODNY where fshare_t exists
 * @returns the combined f_deny value
 */
int fork_setmode_deny(int access, int f_rddny, int f_wrdny, int f_nodny)
{
    return (access & OPENACC_DRD ? f_rddny : f_nodny)
           | (access & OPENACC_DWR ? f_wrdny : f_nodny);
}

/*!
 * @brief Apply the access and deny modes of an FPOpenFork through band locks
 *
 * Each requested access fails with EACCES when another fork denies it, and
 * each requested deny fails when another fork already holds that access. The
 * call then takes a band lock for every access and deny bit, or the OPEN_NONE
 * band lock when the mode names none. With HAVE_FSHARE_T an F_SHARE
 * reservation adds kernel enforcement.
 *
 * @param[in]     obj       session object, unused
 * @param[in,out] adp       AppleDouble of the fork
 * @param[in]     eid       ADEID_DFORK or ADEID_RFORK
 * @param[in]     access    OPENACC_* access and deny bits
 * @param[in]     ofrefnum  reference number of the opening fork
 * @returns 0, or -1 with errno set, EACCES for a mode conflict
 */
static int fork_setmode(const AFPObj *obj _U_, struct adouble *adp, int eid,
                        int access, int ofrefnum)
{
    int ret;
    int readset = 0;
    int writeset = 0;
    int denyreadset;
    int denywriteset;

    if (!(access & (OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD))) {
        return ad_lock(adp, eid, ADLOCK_RD | ADLOCK_FILELOCK, AD_FILELOCK_OPEN_NONE, 1,
                       ofrefnum);
    }

    if (access & (OPENACC_RD | OPENACC_DRD)) {
        /* the open-read band answers the deny-read request alone */
        if ((access & OPENACC_DRD)
                && (readset = ad_testlock(adp, eid, AD_FILELOCK_OPEN_RD)) < 0) {
            return readset;
        }

        if ((denyreadset = ad_testlock(adp, eid, AD_FILELOCK_DENY_RD)) < 0) {
            return denyreadset;
        }

        if ((access & OPENACC_RD) && denyreadset) {
            errno = EACCES;
            return -1;
        }

        if ((access & OPENACC_DRD) && readset) {
            errno = EACCES;
            return -1;
        }

        /* boolean logic is not enough, because getforkmode is not always telling the
         * true
         */
        if (access & OPENACC_RD) {
            ret = ad_lock(adp, eid, ADLOCK_RD | ADLOCK_FILELOCK, AD_FILELOCK_OPEN_RD, 1,
                          ofrefnum);

            if (ret) {
                return ret;
            }
        }

        if (access & OPENACC_DRD) {
            ret = ad_lock(adp, eid, ADLOCK_RD | ADLOCK_FILELOCK, AD_FILELOCK_DENY_RD, 1,
                          ofrefnum);

            if (ret) {
                return ret;
            }
        }
    }

    /* ------------same for writing -------------- */
    if (access & (OPENACC_WR | OPENACC_DWR)) {
        /* the open-write band answers the deny-write request alone */
        if ((access & OPENACC_DWR)
                && (writeset = ad_testlock(adp, eid,
                                           AD_FILELOCK_OPEN_WR)) < 0) {
            return writeset;
        }

        if ((denywriteset = ad_testlock(adp, eid, AD_FILELOCK_DENY_WR)) < 0) {
            return denywriteset;
        }

        if ((access & OPENACC_WR) && denywriteset) {
            errno = EACCES;
            return -1;
        }

        if ((access & OPENACC_DWR) && writeset) {
            errno = EACCES;
            return -1;
        }

        if (access & OPENACC_WR) {
            ret = ad_lock(adp, eid, ADLOCK_RD | ADLOCK_FILELOCK, AD_FILELOCK_OPEN_WR, 1,
                          ofrefnum);

            if (ret) {
                return ret;
            }
        }

        if (access & OPENACC_DWR) {
            ret = ad_lock(adp, eid, ADLOCK_RD | ADLOCK_FILELOCK, AD_FILELOCK_DENY_WR, 1,
                          ofrefnum);

            if (ret) {
                return ret;
            }
        }
    }

    /*
     * The Solaris share reservation follows the open-mode locks, so another
     * client's lock can land between the two; a refused reservation returns
     * with those locks set, and the callers release them by refnum.
     */
#ifdef HAVE_FSHARE_T
    fshare_t shmd;

    if (obj->options.flags & OPTION_SHARE_RESERV) {
        shmd.f_access = (access & OPENACC_RD ? F_RDACC : 0) | (access & OPENACC_WR ?
                                                               F_WRACC : 0);

        if (shmd.f_access == 0)
            /* we must give an access mode, otherwise fcntl will complain */
        {
            shmd.f_access = F_RDACC;
        }

        shmd.f_deny = fork_setmode_deny(access, F_RDDNY, F_WRDNY, F_NODNY);
        shmd.f_id = ofrefnum;
        int fd = (eid == ADEID_DFORK) ? ad_data_fileno(adp) : ad_reso_fileno(adp);

        if (fd != -1 && fd != AD_SYMLINK && fcntl(fd, F_SHARE, &shmd) != 0) {
            LOG(log_debug, logtype_afpd, "fork_setmode: fcntl: %s", strerror(errno));
            errno = EACCES;
            return -1;
        }
    }

#endif
    return 0;
}

/*!
 * @brief FPOpenFork: open the data or resource fork of a file with access and
 * deny modes
 *
 * OPENFORK_RSCS, bit 7 of the fork byte, selects the resource fork. A write
 * open on a read-only volume fails with AFPERR_VLOCK, and a directory with
 * AFPERR_BADTYPE. The virtual Icon file of the volume root opens without a
 * descriptor. A real fork opens through ad_open with share modes, and the
 * metadata opens too, created for a write open.
 *
 * The write-inhibit attribute refuses a write open with AFPERR_OLOCK, and a
 * deny mode conflict fails with AFPERR_DENYCONF. The reply carries the
 * bitmap, the fork reference number, and the fork parameters. Running out of
 * descriptors fails with AFPERR_NFILE.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, AFPERR_DENYCONF, AFPERR_OLOCK, AFPERR_ACCESS, AFPERR_LOCK,
 *          AFPERR_NFILE, AFPERR_VLOCK, AFPERR_BADTYPE, AFPERR_NOOBJ, or
 *          AFPERR_PARAM
 */
int afp_openfork(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf,
                 size_t *rbuflen)
{
    struct vol      *vol;
    struct dir      *dir;
    struct ofork    *ofork, *opened;
    struct adouble  *adsame = NULL;
    size_t          buflen = 0;
    int             ret, adflags, eid;
    int             nfile_err = 0;
    uint32_t        did;
    uint16_t        vid, bitmap, access, ofrefnum;
    char            fork, *path, *upath;
    struct stat     *st;
    uint16_t        bshort;
    struct path     *s_path;
    ibuf++;
    fork = *ibuf++;
    memcpy(&vid, ibuf, sizeof(vid));
    ibuf += sizeof(vid);
    *rbuflen = 0;

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    memcpy(&did, ibuf, sizeof(did));
    ibuf += sizeof(int);

    if (NULL == (dir = dirlookup(vol, did))) {
        return afp_errno;
    }

    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);
    memcpy(&access, ibuf, sizeof(access));
    access = ntohs(access);
    ibuf += sizeof(access);

    if ((vol->v_flags & AFPVOL_RO) && (access & OPENACC_WR)) {
        return AFPERR_VLOCK;
    }

    /* Save ibuf position for invalid-on-use retry */
    char *saved_ibuf = ibuf;

    if (NULL == (s_path = cname(vol, dir, &ibuf))) {
        return get_afp_errno(AFPERR_PARAM);
    }

    if (*s_path->m_name == '\0') {
        /* it's a dir ! */
        return  AFPERR_BADTYPE;
    }

    /* stat() data fork st is set because it's not a dir */
    switch (s_path->st_errno) {
    case 0:
        break;

    case ENOENT:

        /* Virtual Icon\r file at volume root */
        if (curdir->d_did == DIRDID_ROOT
                && is_virtual_icon_name(s_path->u_name)
                && virtual_icon_enabled(vol)) {
            size_t vlen;
            const unsigned char *vdata = virtual_icon_get_rfork(vol, &vlen);

            if (!vdata) {
                return AFPERR_MISC;
            }

            /* the first write creates the file in the volume root */
            if ((access & OPENACC_WR)
                    && check_access(obj, vol, s_path->u_name, OPENACC_WR) < 0) {
                return AFPERR_ACCESS;
            }

            /* Allocate a minimal ofork for the virtual file on a synthetic
             * stat; of_alloc() builds its adouble, which of_dealloc() frees. */
            struct stat vst;
            memset(&vst, 0, sizeof(vst));
            vst.st_mode = S_IFREG | 0444;
            vst.st_ino = VIRTUAL_ICON_CNID;  /* synthetic inode */
            vst.st_dev = 0;
            eid = (fork == OPENFORK_DATA) ? ADEID_DFORK : ADEID_RFORK;
            ofork = of_alloc(vol, curdir, (char *)VIRTUAL_ICON_NAME,
                             s_path->u_name, &ofrefnum, eid, NULL, &vst);

            if (!ofork) {
                return AFPERR_NFILE;
            }

            ofork->of_flags |= AFPFORK_VIRTUAL;

            if (access & OPENACC_RD) {
                ofork->of_flags |= AFPFORK_ACCRD;
            }

            if (access & OPENACC_WR) {
                ofork->of_flags |= AFPFORK_ACCWR;
            }

            ofork->of_virtual_data = vdata;
            ofork->of_virtual_len = (off_t)vlen;
            /* Fill reply with file params */
            buflen = 0;
            ret = getforkparams(obj, ofork, bitmap,
                                rbuf + 2 * sizeof(uint16_t), &buflen);

            if (ret != AFP_OK) {
                of_dealloc(ofork);
                return ret;
            }

            *rbuflen = buflen + 2 * sizeof(uint16_t);
            bitmap = htons(bitmap);
            memcpy(rbuf, &bitmap, sizeof(uint16_t));
            rbuf += sizeof(uint16_t);
            memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
            /* a virtual fork takes a slot but opens no descriptor */
            of_note_open(-1);
            return AFP_OK;
        }

        /* Invalid-on-use recovery: stale cached path may have caused
         * cname() to resolve to a non-existent location. Retry with
         * dirlookup_strict() which validates via stat+inode check,
         * evicts stale entries, and rebuilds from CNID. */
        if (dir->d_did != DIRDID_ROOT && dir->d_did != DIRDID_ROOT_PARENT) {
            LOG(log_debug, logtype_afpd,
                "afp_openfork: ENOENT for did:%u, retrying with strict lookup",
                ntohl(did));
            ibuf = saved_ibuf;
            dir_remove_tree(vol, dir, 0);
            dir = dirlookup_strict(vol, did);

            if (dir != NULL) {
                s_path = cname(vol, dir, &ibuf);

                if (s_path != NULL && s_path->st_errno == 0) {
                    break;  /* Recovery succeeded — continue normally */
                }
            }
        }

        return AFPERR_NOOBJ;

    case EACCES:
        return (access & OPENACC_WR) ? AFPERR_LOCK : AFPERR_ACCESS;

    default:
        LOG(log_error, logtype_afpd, "afp_openfork(%s): %s", s_path->m_name,
            strerror(errno));
        return AFPERR_PARAM;
    }

    upath = s_path->u_name;
    path = s_path->m_name;
    st = &s_path->st;

    if (!vol_unix_priv(vol)) {
        if (check_access(obj, vol, upath, access) < 0) {
            return AFPERR_ACCESS;
        }
    } else {
        if (file_access(obj, vol, s_path, access) < 0) {
            return AFPERR_ACCESS;
        }
    }

    if ((opened = of_findname(vol, s_path))) {
        adsame = opened->of_ad;
    }

    adflags = ADFLAGS_SETSHRMD;

    if (fork == OPENFORK_DATA) {
        eid = ADEID_DFORK;
        adflags |= ADFLAGS_DF;
    } else {
        eid = ADEID_RFORK;
        adflags |= ADFLAGS_RF;
    }

    if (access & OPENACC_WR) {
        adflags |= ADFLAGS_RDWR;

        if (fork != OPENFORK_DATA)
            /*
             * We only try to create the resource
             * fork if the user wants to open it for write acess.
             */
        {
            adflags |= ADFLAGS_CREATE;
        }
    } else {
        adflags |= ADFLAGS_RDONLY;
    }

    if ((ofork = of_alloc(vol, curdir, path, upath, &ofrefnum, eid, adsame,
                          st)) == NULL) {
        return AFPERR_NFILE;
    }

    /* a shared adouble keeps its descriptors; the open may add others */
    int had_data = ad_data_fileno(ofork->of_ad);
    int had_meta = ad_meta_fileno(ofork->of_ad);
    int had_reso = ad_reso_fileno(ofork->of_ad);
    const struct ad_refs before = ad_refs_now(ofork->of_ad);
    LOG(log_debug, logtype_afpd, "afp_openfork(\"%s\", %s, %s)",
        fullpathname(s_path->u_name),
        (fork == OPENFORK_DATA) ? "data" : "reso",
        !(access & OPENACC_WR) ? "O_RDONLY" : "O_RDWR");

    /* First ad_open(), opens data or resource fork */
    if (ad_open(ofork->of_ad, upath, adflags, 0666) < 0) {
        /* Set ret per case (no fall-through): the cleanup label returns ret and
         * must not consult errno, which the cleanup ad_close()/close(2) clobbers. */
        switch (errno) {
        case EROFS:
            ret = AFPERR_VLOCK;
            goto openfork_err;

        case EACCES:
            ret = (access & OPENACC_WR) ? AFPERR_LOCK : AFPERR_ACCESS;
            goto openfork_err;

        case ENOENT:
            ret = AFPERR_NOOBJ;
            goto openfork_err;

        case EMFILE :
        case ENFILE :
            nfile_err = errno;
            ret = AFPERR_NFILE;
            goto openfork_err;

        case EISDIR :
            ret = AFPERR_BADTYPE;
            goto openfork_err;

        default:
            LOG(log_error, logtype_afpd, "afp_openfork(%s): ad_open: %s", s_path->m_name,
                strerror(errno));
            ret = AFPERR_PARAM;
            goto openfork_err;
        }
    }

    /*
     * Create metadata if we open rw, otherwise only open existing metadata
     */
    if (access & OPENACC_WR) {
        adflags = ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE;
    } else {
        adflags = ADFLAGS_HF | ADFLAGS_RDONLY;
    }

    /* the resource fork's open above measured the fork it holds */
    if (eid == ADEID_RFORK && ad_reso_fileno(ofork->of_ad) != -1) {
        adflags |= ADFLAGS_NORLEN;
    }

    if (ad_open(ofork->of_ad, upath, adflags, 0666) == 0) {
        ofork->of_flags |= AFPFORK_META;

        if (ad_get_MD_flags(ofork->of_ad) & O_CREAT) {
            LOG(log_debug, logtype_afpd, "afp_openfork(\"%s\"): setting CNID", upath);
            cnid_t id;

            if ((id = get_id(vol, ofork->of_ad, st, curdir->d_did, upath,
                             (int)strlen(upath))) == CNID_INVALID) {
                LOG(log_error, logtype_afpd,
                    "afp_openfork(\"%s\"): CNID error", upath);
                ret = AFPERR_MISC;
                goto openfork_err;
            }

            (void)ad_setid(ofork->of_ad, st->st_dev, st->st_ino, id,
                           curdir->d_did, vol->v_stamp);
            ad_flush(ofork->of_ad);
        }
    } else {
        switch (errno) {
        case EACCES:
        case ENOENT:
            /* no metadata? We don't care! */
            break;

        case EROFS:
            ret = AFPERR_VLOCK;
            goto openfork_err;

        case EMFILE :
        case ENFILE :
            nfile_err = errno;
            ret = AFPERR_NFILE;
            goto openfork_err;

        case EISDIR :
            ret = AFPERR_BADTYPE;
            goto openfork_err;

        default:
            LOG(log_error, logtype_afpd, "afp_openfork(%s): ad_open: %s", s_path->m_name,
                strerror(errno));
            ret = AFPERR_PARAM;
            goto openfork_err;
        }
    }

    if ((ret = getforkparams(obj, ofork, bitmap, rbuf + 2 * sizeof(int16_t),
                             &buflen)) != AFP_OK) {
        /* No local ad_close(): the openfork_err label releases what this open
         * took. */
        goto openfork_err;
    }

    *rbuflen = buflen + 2 * sizeof(uint16_t);
    bitmap = htons(bitmap);
    memcpy(rbuf, &bitmap, sizeof(uint16_t));
    rbuf += sizeof(uint16_t);

    /* check WriteInhibit bit if we have a resource fork
     * the test is done here, after some client traffic capture
     */
    if (ad_meta_fileno(ofork->of_ad) != -1) {   /* META */
        ad_getattr(ofork->of_ad, &bshort);

        if ((bshort & htons(ATTRBIT_NOWRITE)) && (access & OPENACC_WR)) {
            ret = AFPERR_OLOCK;
            ofrefnum = 0;
            memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
            /* No local ad_close()/of_dealloc(): the openfork_err label does it. */
            goto openfork_err;
        }
    }

    /*
     * synchronization locks:
     */

    /* don't try to lock non-existent rforks. */
    if ((eid == ADEID_DFORK)
            || (ad_reso_fileno(ofork->of_ad) != -1)
            || (ofork->of_ad->ad_vers == AD_VERSION_EA)) {
        ret = fork_setmode(obj, ofork->of_ad, eid, access, ofrefnum);

        /* can we access the fork? */
        if (ret < 0) {
            ofork->of_flags |= AFPFORK_ERROR;

            /* Map errno before the goto: the cleanup ad_close()/close(2)
             * clobbers it. */
            switch (errno) {
            case EAGAIN: /* return data anyway */
            case EACCES:
            case EINVAL:
                ofrefnum = 0;
                memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
                ret = AFPERR_DENYCONF;
                break;

            default:
                *rbuflen = 0;
                LOG(log_error, logtype_afpd, "afp_openfork(%s): ad_lock: %s",
                    s_path->m_name, strerror(errno));
                ret = AFPERR_PARAM;
                break;
            }

            goto openfork_err;
        }

        if (access & OPENACC_WR) {
            ofork->of_flags |= AFPFORK_ACCWR;
        }
    }

    /* the file may be open read only without resource fork */
    if (access & OPENACC_RD) {
        ofork->of_flags |= AFPFORK_ACCRD;
    }

    /* the close of an emptied Icon file tells a cut from a new file by it */
    if (eid == ADEID_RFORK && ofork->of_ad->ad_rlen >= RFORK_HEADER_SIZE) {
        ofork->of_flags |= AFPFORK_RFHEADER;
    }

    int newfd = -1;

    if (had_data < 0) {
        newfd = MAX(newfd, ad_data_fileno(ofork->of_ad));
    }

    if (had_meta < 0) {
        newfd = MAX(newfd, ad_meta_fileno(ofork->of_ad));
    }

    if (had_reso < 0) {
        newfd = MAX(newfd, ad_reso_fileno(ofork->of_ad));
    }

    of_note_open(newfd);
    ofork->of_adflags = ad_refs_taken(ofork->of_ad, before);
    LOG(log_debug, logtype_afpd, "afp_openfork(\"%s\"): fork: %" PRIu16,
        fullpathname(s_path->m_name), ofork->of_refnum);
    memcpy(rbuf, &ofrefnum, sizeof(ofrefnum));
    return AFP_OK;
openfork_err:
    /* the adouble may be another open fork's: release only what this open
     * took. ret was set per case. */
    ad_unlock(ofork->of_ad, ofork->of_refnum);
    ad_close(ofork->of_ad, ad_refs_taken(ofork->of_ad, before));
    of_dealloc(ofork);

    /* the refused fork is released, so the counts cover open forks only */
    if (nfile_err) {
        of_log_nfile(s_path->m_name, nfile_err);
    }

    return ret;
}

/*!
 * @brief Mark an open fork changed: its volume is marked, a resource fork's
 *        cached content goes under every name the session has the file
 *        open by, and the fork's first change sends sibling sessions a
 *        refresh hint, of_closefork() the last
 *
 * @param[in]     obj    session object
 * @param[in,out] ofork  open fork a write or a length set reached
 */
static void fork_modified(const AFPObj *obj, struct ofork *ofork)
{
    setvoltime_write(ofork->of_vol->v_obj, ofork->of_vol);

    if ((ofork->of_flags & AFPFORK_RSRC)
            && obj->options.dircache_rfork_budget > 0) {
        of_rfork_changed(ofork);
    }

    if (!(ofork->of_flags & AFPFORK_MODIFIED)) {
        ofork->of_flags |= AFPFORK_MODIFIED;
        of_hint_siblings(obj, ofork, of_dircache_entry(ofork));
    }
}

/*!
 * @brief Map the errno of a failed fork write, truncate or header flush, or
 *        of the virtual Icon file's create or open, to the AFP reply
 *
 * @param[in] ofork     fork being changed
 * @param[in] call      libatalk call that failed, named in the log
 * @param[in] is_write  an FPWrite, whose replies hold no AFPERR_VLOCK
 *
 * @returns AFPERR_DFULL or AFPERR_ACCESS, AFPERR_VLOCK for a read-only file
 *          system outside an FPWrite, otherwise AFPERR_MISC
 */
static int fork_error(const struct ofork *ofork, const char *call,
                      bool is_write)
{
    int err;

    switch (errno) {
    case EDQUOT:
    case EFBIG:
    case ENOSPC:
        LOG(log_error, logtype_afpd, "%s(%s): DISK FULL", call, of_name(ofork));
        return AFPERR_DFULL;

    case EACCES:
        return AFPERR_ACCESS;

    case EROFS:
        err = is_write ? AFPERR_ACCESS : AFPERR_VLOCK;
        break;

    case EPERM:
        err = AFPERR_ACCESS;
        break;

    default:
        err = AFPERR_MISC;
        break;
    }

    LOG(log_error, logtype_afpd, "%s(%s): %s", call, of_name(ofork),
        strerror(errno));
    return err;
}

/*!
 * @brief FPSetForkParms: change the length of an open fork
 *
 * The bitmap must name exactly the length of the open fork type, and the
 * 64-bit form needs AFP 3.0. A negative length fails. Shrinking takes a
 * temporary write lock on the cut range, so a conflicting byte lock fails the
 * call with AFPERR_LOCK. The virtual Icon file becomes a real file first. A
 * resource fork change invalidates the cached resource fork data, and a
 * length change marks the volume.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length
 * @param[out]    rbuf     reply block, unused
 * @param[in,out] rbuflen  reply buffer capacity on entry, 0 on return
 * @returns AFP_OK, AFPERR_BITMAP, AFPERR_LOCK, AFPERR_ACCESS, AFPERR_VLOCK,
 *          AFPERR_PARAM, or the AFPERR_* code of fork_error
 */
int afp_setforkparams(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf _U_,
                      size_t *rbuflen)
{
    struct ofork    *ofork;
    struct vol      *vol;
    struct dir      *dir;
    off_t       size;
    uint16_t       ofrefnum, bitmap;
    int                 err;
    int                 is64;
    int                 eid;
    off_t       st_size;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);
    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);
    *rbuflen = 0;

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_setforkparams: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        return AFPERR_PARAM;
    }

    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        if ((ofork->of_flags & AFPFORK_ACCWR) == 0) {
            return AFPERR_ACCESS;
        }

        int mret = materialize_virtual_icon(obj, ofork, false);

        if (mret != AFP_OK) {
            return mret;
        }
    }

    vol = ofork->of_vol;

    if ((dir = dirlookup(vol, ofork->of_did)) == NULL) {
        LOG(log_error, logtype_afpd, "%s: bad fork did",  of_name(ofork));
        return AFPERR_MISC;
    }

    if (movecwd(vol, dir) != 0) {
        LOG(log_error, logtype_afpd, "%s: bad fork directory",
            dir->d_fullpath ? cfrombstr(dir->d_fullpath) : "(null)");
        return afp_errno;
    }

    if (ofork->of_vol->v_flags & AFPVOL_RO) {
        return AFPERR_VLOCK;
    }

    if ((ofork->of_flags & AFPFORK_ACCWR) == 0) {
        return AFPERR_ACCESS;
    }

    if (ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else {
        return AFPERR_PARAM;
    }

    if (((bitmap & ((1 << FILPBIT_DFLEN) | (1 << FILPBIT_EXTDFLEN)))
            && eid == ADEID_RFORK
        ) ||
            ((bitmap & ((1 << FILPBIT_RFLEN) | (1 << FILPBIT_EXTRFLEN)))
             && eid == ADEID_DFORK)) {
        return AFPERR_BITMAP;
    }

    is64 = 0;

    if (bitmap & ((1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN))) {
        if (obj->afp_version >= 30) {
            is64 = 4;
        } else {
            return AFPERR_BITMAP;
        }
    }

    if (ibuflen < 2 + sizeof(ofrefnum) + sizeof(bitmap) + is64 + 4) {
        return AFPERR_PARAM;
    }

    size = get_off_t(&ibuf, is64);

    if (size < 0) {
        /* Some MacOS don't return an error they just don't change the size! */
        return AFPERR_PARAM;
    }

    if (bitmap == (1 << FILPBIT_DFLEN) || bitmap == (1 << FILPBIT_EXTDFLEN)) {
        st_size = ad_size(ofork->of_ad, eid);
        err = -2;

        if (st_size > size &&
                ad_tmplock(ofork->of_ad, eid, ADLOCK_WR, size, st_size - size,
                           ofork->of_refnum) < 0) {
            goto afp_setfork_err;
        }

        err = ad_dtruncate(ofork->of_ad, size);

        if (st_size > size) {
            ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, size, st_size - size,
                       ofork->of_refnum);
        }

        if (err < 0) {
            goto afp_setfork_err;
        }
    } else if (bitmap == (1 << FILPBIT_RFLEN)
               || bitmap == (1 << FILPBIT_EXTRFLEN)) {
        ad_refresh(NULL, ofork->of_ad);
        st_size = ad_size(ofork->of_ad, eid);
        err = -2;

        if (st_size > size &&
                ad_tmplock(ofork->of_ad, eid, ADLOCK_WR, size, st_size - size,
                           ofork->of_refnum) < 0) {
            goto afp_setfork_err;
        }

        err = ad_rtruncate(ofork->of_ad, ofork->of_uname, size);

        if (st_size > size) {
            ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, size, st_size - size,
                       ofork->of_refnum);
        }

        if (err < 0) {
            goto afp_setfork_err;
        }

        /* a failed flush leaves the fork truncated, which the tail marks */
        if (ad_flush(ofork->of_ad) < 0) {
            err = fork_error(ofork, "ad_flush", false);
        }
    } else {
        return AFPERR_BITMAP;
    }

    if (st_size != size) {
        /* a grow counts against the volume size limit, as a write past the
         * end does */
        if (vol->v_limitsize && size > st_size) {
            vol->v_appended += size - st_size;
        }

        fork_modified(obj, ofork);
    }

    return err;
afp_setfork_err:

    if (err == -2) {
        return AFPERR_LOCK;
    }

    return fork_error(ofork, eid == ADEID_DFORK ? "ad_dtruncate"
                      : "ad_rtruncate", false);
}

/* for this to work correctly, we need to check for locks before each
 * read and write. that's most easily handled by always doing an
 * appropriate check before each ad_read/ad_write. other things
 * that can change files like truncate are handled internally to those
 * functions.
 */
#define ENDBIT(a)  ((a) & 0x80)
#define UNLOCKBIT(a) ((a) & 0x01)


/*!
 * @brief Shared body of FPByteRangeLock and FPByteRangeLockExt
 *
 * The flag byte chooses between taking and releasing the lock, and whether the
 * offset counts from the end of the fork. A length of -1 extends the range to
 * BYTELOCK_MAX, below the share mode band. Locks on virtual forks succeed
 * without effect. The reply carries the start offset of the range.
 *
 * @param[in]     obj      session object, unused
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block, receives the range start
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @param[in]     is64     nonzero for FPByteRangeLockExt
 * @returns AFP_OK, AFPERR_LOCK, AFPERR_NORANGE, AFPERR_NLOCK, AFPERR_RANGEOVR,
 *          or AFPERR_PARAM
 */
static int byte_lock(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                     char *rbuf, size_t *rbuflen, int is64)
{
    struct ofork    *ofork;
    off_t               offset, length;
    int                 eid;
    uint16_t       ofrefnum;
    uint8_t            flags;
    int                 lockop;
    *rbuflen = 0;
    /* figure out parameters */
    ibuf++;
    flags = *ibuf; /* first bit = endflag, lastbit = lockflag */
    ibuf++;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "byte_lock: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        return AFPERR_PARAM;
    }

    /* Virtual forks don't support byte locking */
    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        return AFP_OK;
    }

    if (ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else {
        return AFPERR_PARAM;
    }

    offset = get_off_t(&ibuf, is64);
    length = get_off_t(&ibuf, is64);

    if (length == -1) {
        length = BYTELOCK_MAX;
    } else if (!length || is_neg(is64, length)) {
        return AFPERR_PARAM;
    } else if ((length >= AD_FILELOCK_BASE)
               && -1 == (ad_reso_fileno(ofork->of_ad))) { /* HF ?*/
        return AFPERR_LOCK;
    }

    if (ENDBIT(flags)) {
        offset += ad_size(ofork->of_ad, eid);
        /* FIXME what do we do if file size > 2 GB and
           it's not byte_lock_ext?
        */
    }

    /* error if we have a negative offset */
    if (offset < 0) {
        return AFPERR_PARAM;
    }

    /* if the file is a read-only file, we use read locks instead of
     * write locks. that way, we can prevent anyone from initiating
     * a write lock. */
    lockop = UNLOCKBIT(flags) ? ADLOCK_CLR : ADLOCK_WR;

    if (ad_lock(ofork->of_ad, eid, lockop, offset, length,
                ofork->of_refnum) < 0) {
        switch (errno) {
        case EACCES:
        case EAGAIN:
            return UNLOCKBIT(flags) ? AFPERR_NORANGE : AFPERR_LOCK;

        case ENOLCK:
            return AFPERR_NLOCK;

        case EINVAL:
            return UNLOCKBIT(flags) ? AFPERR_NORANGE : AFPERR_RANGEOVR;

        case EBADF:
        default:
            return AFPERR_PARAM;
        }
    }

    *rbuflen = set_off_t (offset, rbuf, is64);
    return AFP_OK;
}

/*!
 * @brief FPByteRangeLock: take or release a byte range lock on an open fork
 * with 32-bit fields
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length
 * @param[out]    rbuf     reply block, receives the range start
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, or the AFPERR_* code of byte_lock
 */
int afp_bytelock(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                 size_t *rbuflen)
{
    return byte_lock(obj, ibuf, ibuflen, rbuf, rbuflen, 0);
}

/*!
 * @brief FPByteRangeLockExt: take or release a byte range lock on an open fork
 * with 64-bit fields
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length
 * @param[out]    rbuf     reply block, receives the range start
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, or the AFPERR_* code of byte_lock
 */
int afp_bytelock_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                     size_t *rbuflen)
{
    return byte_lock(obj, ibuf, ibuflen, rbuf, rbuflen, 1);
}

#undef UNLOCKBIT

/*!
 * @brief Read *rbuflen bytes from fork at offset
 *
 * @param[in] ofork        fork handle
 * @param[in] eid          data fork or resource fork entry id
 * @param[in] offset       offset
 * @param[in] rbuf         data buffer
 * @param[in,out] rbuflen  in: number of bytes to read, out: bytes read
 *
 * @returns AFP_OK, AFPERR_EOF when the fork ends first, AFPERR_PARAM when the
 *          request is shorter than the rest of a symlink's target, or
 *          AFPERR_MISC when the read fails
 */
static int read_file(const struct ofork *ofork, int eid, off_t offset,
                     char *rbuf, size_t *rbuflen)
{
    ssize_t cc;
    int eof = 0;
    cc = ad_read(ofork->of_ad, eid, offset, rbuf, *rbuflen);

    if (cc < 0) {
        int err = errno;
        LOG(log_error, logtype_afpd, "afp_read(%s): ad_read: %s", of_name(ofork),
            strerror(err));
        *rbuflen = 0;
        return err == ERANGE ? AFPERR_PARAM : AFPERR_MISC;
    }

    if ((size_t)cc < *rbuflen) {
        eof = 1;
    }

    *rbuflen = cc;

    if (eof) {
        return AFPERR_EOF;
    }

    return AFP_OK;
}

/*!
 * @brief Serve resource fork data from Tier 2 cache via DSI framing
 *
 * Shared helper for cache-hit and just-populated cache paths in read_fork().
 * Sends the reply from the cache buffer in one dsi_readinit() and promotes
 * the entry in the rfork LRU.
 *
 * @param[in]  dsi           DSI connection
 * @param[in]  cached_entry  dir entry with populated dcache_rfork_buf
 * @param[in]  offset        Starting byte offset in the rfork
 * @param[in]  reqcount      Requested byte count
 * @param[in]  err           AFP error code (AFPERR_EOF or AFP_OK)
 *
 * @returns 0 when the reply went out, -1 when a send failed or the entry
 *          cannot serve the range, which read_fork()'s checks exclude
 */
static int rfork_cache_serve_from_buf(DSI *dsi, struct dir *cached_entry,
                                      off_t offset, off_t reqcount, int err)
{
    if (cached_entry->dcache_rfork_buf == NULL) {
        LOG(log_error, logtype_afpd,
            "rfork_cache_serve_from_buf: NULL buffer (did:%u)",
            ntohl(cached_entry->d_did));
        return -1;
    }

    /* Defense-in-depth: validate bounds before sending from the rfork buffer.
     * Use subtraction-based validation: offset + reqcount may overflow for a
     * malicious FPReadExt count before it can be compared with dcache_rlen. */
    if (!fork_range_within(offset, reqcount, cached_entry->dcache_rlen)) {
        LOG(log_error, logtype_afpd,
            "rfork_cache_serve_from_buf: bounds violation "
            "(offset=%lld, reqcount=%lld, rlen=%lld, did:%u)",
            (long long)offset, (long long)reqcount,
            (long long)cached_entry->dcache_rlen,
            ntohl(cached_entry->d_did));
        return -1;
    }

    LOG(log_debug, logtype_afpd,
        "rfork_cache_serve_from_buf: serving rfork from cache "
        "(offset:%lld, reqcount:%lld, rlen:%lld, did:%u)",
        (long long)offset, (long long)reqcount,
        (long long)cached_entry->dcache_rlen,
        ntohl(cached_entry->d_did));

    if (dsi_readinit(dsi, (char *)cached_entry->dcache_rfork_buf + offset,
                     (size_t)reqcount, (size_t)reqcount, err) < 0) {
        return -1;
    }

    dsi_readdone(dsi);

    /* Promote in rfork LRU */
    if (cached_entry->rfork_lru_node) {
        queue_move_to_tail(rfork_lru, cached_entry->rfork_lru_node);
    }

    return 0;
}

/*!
 * @brief Shared body of FPRead and FPReadExt
 *
 * The call reads reqcount bytes from offset, clamped at the fork size with
 * AFPERR_EOF, then cut at UINT32_MAX, the most a DSI reply frames. It
 * ignores the newline mask and character of the request.
 * Strict locking takes a temporary read lock on the range. Virtual forks
 * serve their in-memory data, and resource fork reads can come from the
 * resource fork cache. Data goes out through sendfile when available,
 * otherwise in dsi_read chunks. A failure in the middle of the reply ends the
 * session.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @param[in]     is64     nonzero for FPReadExt
 * @returns AFP_OK, AFPERR_EOF, AFPERR_ACCESS, AFPERR_LOCK, AFPERR_MISC, or
 *          AFPERR_PARAM
 */
static int read_fork(AFPObj *obj, char *ibuf, size_t ibuflen _U_,
                     char *rbuf, size_t *rbuflen, int is64)
{
    DSI *dsi;
    struct ofork *ofork;
    off_t        offset;
    off_t        saveoff = 0;
    off_t        reqcount = 0;
    off_t        savereqcount = 0;
    off_t        size;
    ssize_t      cc;
    ssize_t      dsi_cc;
    ssize_t      err;
    int          eid;
    uint16_t     ofrefnum;
    /* we break the AFP spec here by not supporting nlmask and nlchar anymore */
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(u_short);

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_read: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        err = AFPERR_PARAM;
        goto afp_read_err;
    }

    if ((ofork->of_flags & AFPFORK_ACCRD) == 0) {
        err = AFPERR_ACCESS;
        goto afp_read_err;
    }

    if (ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else { /* fork wasn't opened. this should never really happen. */
        err = AFPERR_ACCESS;
        goto afp_read_err;
    }

    offset   = get_off_t(&ibuf, is64);
    reqcount = get_off_t(&ibuf, is64);

    if (reqcount < 0 || offset < 0) {
        err = AFPERR_PARAM;
        goto afp_read_err;
    }

    /* zero request count */
    err = AFP_OK;

    if (!reqcount) {
        goto afp_read_err;
    }

    /* Virtual fork: serve data from in-memory buffer */
    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        if (ofork->of_flags & AFPFORK_DATA) {
            /* Data fork is empty for virtual Icon\r */
            err = AFPERR_EOF;
            goto afp_read_err;
        }

        /* Resource fork: serve from virtual data buffer */
        if (offset >= ofork->of_virtual_len) {
            err = AFPERR_EOF;
            goto afp_read_err;
        }

        off_t avail = ofork->of_virtual_len - offset;

        if (reqcount > avail) {
            reqcount = avail;
            err = AFPERR_EOF;
        }

        size_t tocopy = (size_t)reqcount;
        /* the reply buffer holds the whole fork on both transports */
        *rbuflen = MIN(tocopy, *rbuflen);
        memcpy(rbuf, ofork->of_virtual_data + offset, *rbuflen);
        return (int)err;
    }

    AFP_READ_START((long)reqcount);
    struct dir *cached_entry = NULL;

    switch (obj->proto) {
#ifndef NO_DDP

    case AFPPROTO_ASP:
        savereqcount = reqcount;
        saveoff = offset;

        if (obj->options.flags & OPTION_STRICT_LOCKING) {
            if (ad_tmplock(ofork->of_ad, eid, ADLOCK_RD, offset, reqcount,
                           ofork->of_refnum) < 0) {
                err = AFPERR_LOCK;
                goto afp_read_err;
            }
        }

        /* for asp, just send a packet at the requested buffer size */
        *rbuflen = MIN((size_t)reqcount, *rbuflen);
        cc = read_file(ofork, eid, offset, rbuf, rbuflen);

        if (cc < 0) {
            err = cc;
            goto afp_read_done;
        }

        break;
#endif

    case AFPPROTO_DSI:
        dsi = obj->dsi;
        /* request block parsed: the whole command buffer stages the reply */
        ibuf = (char *)dsi->commands;
        /* a ReqCount past the fork's end stops at its size; a resource fork
         * the open found no file for has nothing to read, whatever
         * descriptor or length a later metadata open put into the adouble */
        size = eid == ADEID_RFORK && !ad_rsrc_open(ofork->of_ad)
               ? 0 : ad_size(ofork->of_ad, eid);
        LOG(log_maxdebug, logtype_afpd,
            "afp_read(fork: %" PRIu16 " [%s], off: %" PRIu64 ", len: %" PRIu64 ", size: %"
            PRIu64 ")",
            ofork->of_refnum, (ofork->of_flags & AFPFORK_DATA) ? "data" : "reso", offset,
            reqcount, size);

        if (offset >= size) {
            err = AFPERR_EOF;
            goto afp_read_err;
        }

        /* Subtract only after offset has been checked against size.  Adding a
         * client-controlled FPReadExt count here can overflow signed off_t and
         * leave reqcount unbounded on the resource-fork cache path. */
        if (!fork_range_within(offset, reqcount, size)) {
            reqcount = size - offset;
            err = AFPERR_EOF;
        }

        /* the most a DSI header's 32-bit length frames; a reply cut here
         * ends before the fork does, so it is no EOF */
        if (reqcount > UINT32_MAX) {
            reqcount = UINT32_MAX;
            err = AFP_OK;
        }

        savereqcount = reqcount;
        saveoff = offset;

        if (obj->options.flags & OPTION_STRICT_LOCKING) {
            if (ad_tmplock(ofork->of_ad, eid, ADLOCK_RD, offset, reqcount,
                           ofork->of_refnum) < 0) {
                err = AFPERR_LOCK;
                goto afp_read_err;
            }
        }

        if (eid == ADEID_RFORK
                && !(ofork->of_flags & AFPFORK_ACCWR)
                && (!(ofork->of_vol->v_flags & AFPVOL_MULTIPROTO)
                    || obj->options.dircache_rfork_budget_explicit)
                && obj->options.dircache_rfork_budget > 0
                && (cached_entry = of_dircache_entry(ofork)) != NULL) {
            rfork_stat_lookups++;
        }

        /* === Tier 2 rfork cache check — before sendfile === */
        if (cached_entry && cached_entry->dcache_rfork_buf
                && cached_entry->dcache_rlen > 0
                && cached_entry->dcache_rlen == size) {
            /* Serve entire request from Tier 2 cache — zero I/O. */
            if (rfork_cache_serve_from_buf(dsi, cached_entry, offset,
                                           reqcount, (int)err) < 0) {
                goto afp_read_exit;
            }

            rfork_stat_hits++;
            goto afp_read_done;
        }

        /* Size mismatch: cache stale — invalidate any rfork buffer + Tier 1
         * AD, so no fill below takes a length the fork no longer has */
        if (cached_entry && ad_rlen_known(cached_entry->dcache_rlen)
                && cached_entry->dcache_rlen != size) {
            LOG(log_debug, logtype_afpd,
                "rfork_cache: size mismatch (cached:%lld vs live:%lld), "
                "invalidating (did:%u)",
                (long long)cached_entry->dcache_rlen, (long long)size,
                ntohl(cached_entry->d_did));
            dir_modify(ofork->of_vol, cached_entry, &(struct dir_modify_args) {
                .flags = DCMOD_AD_INV | DCMOD_NO_PROMOTE,
            });
            rfork_stat_misses++;     /* Size mismatch counts as a miss */
            cached_entry = NULL;
        }

        /* Cache miss: upgrade to full-fork ad_read if cacheable, from the
         * entry's own name's resource fork: a hard link's forks share the
         * adouble of the name that opened first */
        if (cached_entry && cached_entry->dcache_rlen > 0
                && (size_t)cached_entry->dcache_rlen <= rfork_max_entry_size
                && !cached_entry->dcache_rfork_buf
                && of_rfork_held(ofork, bdata(cached_entry->d_fullpath))
                && rfork_cache_store_from_fd(cached_entry, ofork->of_ad,
                                             ADEID_RFORK) == 0) {
            rfork_stat_added++;

            /* Serve from newly-populated cache */
            if (rfork_cache_serve_from_buf(dsi, cached_entry, offset,
                                           reqcount, (int)err) < 0) {
                goto afp_read_exit;
            }

            rfork_stat_hits++;
            goto afp_read_done;
        }

        /* not filled: another session writes the fork, or the fill failed */

        /* Count miss: any rfork cache lookup that didn't result in a hit */
        if (cached_entry) {
            rfork_stat_misses++;
        }

#ifdef WITH_SENDFILE

        if (!(eid == ADEID_DFORK && ad_data_fileno(ofork->of_ad) == AD_SYMLINK) &&
                !(obj->options.flags & OPTION_NOSENDFILE)) {
            int fd = ad_fork_fileno(ofork->of_ad, eid, &offset);

            if (dsi_stream_read_file(dsi, fd, offset, reqcount, (int)err) < 0) {
                goto afp_read_exit;
            }

            goto afp_read_done;
        }

#endif
        *rbuflen = MIN((size_t)reqcount, (size_t)dsi->server_quantum);
        cc = read_file(ofork, eid, offset, ibuf, rbuflen);

        if (cc < 0) {
            err = cc;

            if (*rbuflen == 0) {
                goto afp_read_done;
            }

            /* the fork ends before reqcount: the bytes read are the reply */
            reqcount = *rbuflen;
        }

        LOG(log_maxdebug, logtype_afpd,
            "afp_read(name: \"%s\", offset: %jd, reqcount: %jd): got %jd bytes from file",
            of_name(ofork), (intmax_t)offset, (intmax_t)reqcount, (intmax_t)*rbuflen);
        offset += *rbuflen;
        /*
         * dsi_readinit() returns size of next read buffer. by this point,
         * we know that we're sending some data. if we fail, something
         * horrible happened.
         */
        dsi_cc = dsi_readinit(dsi, ibuf, *rbuflen, reqcount, (int)err);

        if (dsi_cc < 0) {
            goto afp_read_exit;
        }

        *rbuflen = (size_t)dsi_cc;

        while (*rbuflen > 0) {
            /* the pieces after the first, each at most a quantum */
            cc = read_file(ofork, eid, offset, ibuf, rbuflen);

            if (cc < 0) {
                goto afp_read_exit;
            }

            offset += *rbuflen;
            /* dsi_read() also returns buffer size of next allocation */
            dsi_cc = dsi_read(dsi, ibuf, *rbuflen); /* send it off */

            if (dsi_cc < 0) {
                goto afp_read_exit;
            }

            *rbuflen = (size_t)dsi_cc;
        }

        dsi_readdone(dsi);
        goto afp_read_done;
afp_read_exit:
        LOG(log_error, logtype_afpd, "afp_read(%s): %s", of_name(ofork),
            strerror(errno));

        if (obj->options.flags & OPTION_STRICT_LOCKING) {
            ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, savereqcount,
                       ofork->of_refnum);
        }

        /* in_write stays raised: nothing may follow an unfinished reply */
        obj->exit(EXITERR_CLNT);
        break;
    } /* switch */

afp_read_done:

    if (obj->options.flags & OPTION_STRICT_LOCKING) {
        ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, savereqcount,
                   ofork->of_refnum);
    }

    AFP_READ_DONE();
    return (int)err;
afp_read_err:
    *rbuflen = 0;
    return (int)err;
}

/*!
 * @brief FPRead: read from an open fork with 32-bit offset and count
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, or the AFPERR_* code of read_fork
 */
int afp_read(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
             size_t *rbuflen)
{
    return read_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 0);
}

/*!
 * @brief FPReadExt: read from an open fork with 64-bit offset and count
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, or the AFPERR_* code of read_fork
 */
int afp_read_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                 size_t *rbuflen)
{
    return read_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 1);
}

/*!
 * @brief FPFlush: flush every open fork of a volume to stable storage
 *
 * Failures of single forks only log, and the reply still reports success.
 *
 * @param[in]     obj      session object, unused
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block, unused
 * @param[in,out] rbuflen  reply buffer capacity on entry, 0 on return
 * @returns AFP_OK, or AFPERR_PARAM for an unknown volume
 */
int afp_flush(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf _U_,
              size_t *rbuflen)
{
    struct vol *vol;
    uint16_t vid;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&vid, ibuf, sizeof(vid));

    if (NULL == (vol = getvolbyvid(vid))) {
        return AFPERR_PARAM;
    }

    of_flush(vol);
    return AFP_OK;
}

/*!
 * @brief FPFlushFork: flush an open fork to stable storage
 *
 * A flush failure only logs, and the reply still reports success.
 *
 * @param[in]     obj      session object, unused
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block, unused
 * @param[in,out] rbuflen  reply buffer capacity on entry, 0 on return
 * @returns AFP_OK, or AFPERR_PARAM for an unknown fork reference
 */
int afp_flushfork(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                  char *rbuf _U_, size_t *rbuflen)
{
    struct ofork    *ofork;
    uint16_t       ofrefnum;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_flushfork: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        return AFPERR_PARAM;
    }

    LOG(log_debug, logtype_afpd, "afp_flushfork(fork: %s)",
        (ofork->of_flags & AFPFORK_DATA) ? "d" : "r");

    if (flushfork(ofork) < 0) {
        LOG(log_error, logtype_afpd, "afp_flushfork(%s): %s", of_name(ofork),
            strerror(errno));
    }

    return AFP_OK;
}

/*!
 * @brief FPSyncFork: flush an open fork to stable storage
 *
 * @bug fsync, fdatasync, and F_FULLFSYNC differ in what they guarantee, and the
 * macOS fsync behaves unlike the fsync of other platforms.
 * @sa http://mirror.linux.org.au/pub/linux.conf.au/2007/video/talks/278.pdf
 *
 * @param[in]     obj      session object, unused
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block, unused
 * @param[in,out] rbuflen  reply buffer capacity on entry, 0 on return
 * @returns AFP_OK, AFPERR_PARAM for an unknown fork reference, or AFPERR_MISC
 *          when the flush fails
 */
int afp_syncfork(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_,
                 char *rbuf _U_, size_t *rbuflen)
{
    struct ofork        *ofork;
    uint16_t           ofrefnum;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_syncfork: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        return AFPERR_PARAM;
    }

    LOG(log_debug, logtype_afpd, "afp_syncfork(fork: %s)",
        (ofork->of_flags & AFPFORK_DATA) ? "d" : "r");

    if (flushfork(ofork) < 0) {
        LOG(log_error, logtype_afpd, "flushfork(%s): %s", of_name(ofork),
            strerror(errno));
        return AFPERR_MISC;
    }

    return AFP_OK;
}

/*!
 * @brief Flush the descriptors of an open fork to stable storage
 *
 * The data fork gets an fsync. For a resource fork the call rereads the
 * resource fork length and stamps the modification date of a dirty fork. It
 * then writes the header and syncs the metadata file. A written header
 * leaves the fork clean and marks the volume changed; after a failed one
 * the fork stays dirty for the next flush. Virtual forks need nothing.
 *
 * @param[in,out] ofork  open fork
 * @returns 0, or -1 when an fsync or the header write failed
 */
int flushfork(struct ofork *ofork)
{
    struct timeval tv;
    int err = 0, doflush = 0;

    /* Nothing to flush for virtual forks */
    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        return 0;
    }

    if (ad_data_fileno(ofork->of_ad) != -1 &&
            fsync(ad_data_fileno(ofork->of_ad)) < 0) {
        LOG(log_error, logtype_afpd, "flushfork(%s): dfile(%d) %s",
            of_name(ofork), ad_data_fileno(ofork->of_ad), strerror(errno));
        err = -1;
    }

    if (ad_reso_fileno(ofork->of_ad) != -1 &&     /* HF */
            (ofork->of_flags & AFPFORK_RSRC)) {
        /* read in the rfork length */
        ad_refresh(NULL, ofork->of_ad);

        /* set the date if we're dirty */
        if ((ofork->of_flags & AFPFORK_DIRTY) && !gettimeofday(&tv, NULL)) {
            ad_setdate(ofork->of_ad, AD_DATE_MODIFY | AD_DATE_UNIX, tv.tv_sec);
            doflush++;
        }

        /* flush the header */
        if (doflush) {
            if (ad_flush(ofork->of_ad) < 0) {
                err = -1;
            } else {
                ofork->of_flags &= ~AFPFORK_DIRTY;
                /* the mark's own calls leave the fsync error for the log */
                int saved = errno;
                setvoltime(ofork->of_vol->v_obj, ofork->of_vol);
                errno = saved;
            }
        }

        if (fsync(ad_reso_fileno(ofork->of_ad)) < 0) {
            err = -1;
        }

        if (err < 0)
            LOG(log_error, logtype_afpd, "flushfork(%s): hfile(%d) %s",
                of_name(ofork), ad_reso_fileno(ofork->of_ad), strerror(errno));
    }

    return err;
}

/*!
 * @brief FPCloseFork: close an open fork
 *
 * An unknown reference number logs a warning, because a fork it targeted
 * would stay open for the rest of the session.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block, unused
 * @param[in,out] rbuflen  reply buffer capacity on entry, 0 on return
 * @returns AFP_OK, AFPERR_PARAM for an unknown fork reference, or AFPERR_MISC
 *          when of_closefork failed
 */
int afp_closefork(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_,
                  size_t *rbuflen)
{
    struct ofork    *ofork;
    uint16_t       ofrefnum;
    *rbuflen = 0;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));

    if (NULL == (ofork = of_find(ofrefnum))) {
        /* A close we cannot match leaves any real fork it targeted tracked
         * forever (and its inode undeletable this session). */
        LOG(log_warning, logtype_afpd,
            "afp_closefork: of_find(%" PRIu16 ") could not locate fork"
            " (wire bytes: 0x%02x%02x; stale close from client, or a fork"
            " this session leaked)",
            ofrefnum,
            (unsigned char)ibuf[0], (unsigned char)ibuf[1]);
        return AFPERR_PARAM;
    }

    LOG(log_debug, logtype_afpd, "afp_closefork(fork: %" PRIu16 " [%s])",
        ofork->of_refnum, (ofork->of_flags & AFPFORK_DATA) ? "data" : "rsrc");

    if (of_closefork(obj, ofork) < 0) {
        LOG(log_error, logtype_afpd, "afp_closefork: of_closefork: %s",
            strerror(errno));
        return AFPERR_MISC;
    }

    return AFP_OK;
}


/*!
 * @brief Write rbuflen bytes into the fork at offset
 *
 * The offset advances past every byte written, also when a later write
 * fails. A short write repeats, so the retry reports why pwrite stopped.
 *
 * @param[in,out] ofork    open fork
 * @param[in]     eid      ADEID_DFORK or ADEID_RFORK
 * @param[in,out] offset   fork offset, advanced by the bytes written
 * @param[in]     rbuf     bytes to write
 * @param[in]     rbuflen  bytes in rbuf
 * @returns AFP_OK, or the AFPERR_* code of fork_error for the failed write
 */
static int write_file(struct ofork *ofork, int eid, off_t *offset,
                      char *rbuf, size_t rbuflen)
{
    ssize_t cc;
    LOG(log_maxdebug, logtype_afpd, "write_file(off: %ju, size: %zu)",
        (uintmax_t)*offset, rbuflen);

    /* pwrite() stops short at a size limit; the retry reports why */
    while (rbuflen > 0) {
        cc = ad_write(ofork->of_ad, eid, *offset, 0, rbuf, rbuflen);

        if (cc <= 0) {
            if (cc == 0) {
                errno = EIO;
            }

            return fork_error(ofork, "ad_write", true);
        }

        *offset += cc;
        rbuf += cc;
        rbuflen -= (size_t)cc;
    }

    return AFP_OK;
}

/*!
 * @brief Tell whether an FPWrite's payload of len bytes fits its ReqCount,
 *        logging the refusal when not
 *
 * @param[in] ofork     the fork written, named in the log line
 * @param[in] len       payload length
 * @param[in] reqcount  the request's ReqCount
 * @returns true when len fits reqcount, false when the write is refused
 */
static bool write_fits_count(const struct ofork *ofork, size_t len,
                             off_t reqcount)
{
    if ((off_t)len <= reqcount) {
        return true;
    }

    LOG(log_error, logtype_afpd,
        "afp_write(%s): %zu bytes for a count of %jd", of_name(ofork), len,
        (intmax_t)reqcount);
    return false;
}

/*!
 * @brief Shared body of FPWrite and FPWriteExt
 *
 * The end flag makes offset count from the end of the fork. A virtual Icon
 * fork becomes a real file first. A block too short for the command fails
 * with AFPERR_PARAM, a sum beyond the field width with AFPERR_DFULL, and
 * strict locking takes a temporary write lock. The payload arrives through
 * recvfile where available, otherwise in dsi_write chunks through
 * write_file. A payload longer than a nonzero ReqCount fails with
 * AFPERR_PARAM before any byte is written, and a ReqCount of 0 replies AFP_OK
 * with the payload drained. A failure before afpd reads the payload drains
 * the entire request, and a later failure drains the rest of it. The reply
 * carries the offset after the last byte written.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length
 * @param[out]    rbuf     reply block, receives the new offset
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @param[in]     is64     nonzero for FPWriteExt
 * @returns AFP_OK, AFPERR_ACCESS, AFPERR_PARAM, AFPERR_DFULL, AFPERR_LOCK, or
 *          the AFPERR_* code of fork_error
 */
static int write_fork(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                      size_t *rbuflen, int is64)
{
    struct ofork    *ofork;
    off_t           offset;
    off_t           saveoff;
    off_t           reqcount;
    off_t           oldsize;
    int             endflag;
    int             eid;
    int             err = AFP_OK;
    uint16_t        ofrefnum;
    DSI *dsi = obj->dsi;
    /* figure out parameters */
    ibuf++;
    endflag = ENDBIT(*ibuf);
    ibuf++;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);
    offset   = get_off_t(&ibuf, is64);
    reqcount = get_off_t(&ibuf, is64);

    /* a block too short for its command takes ReqCount from stale bytes */
    if (ibuflen < (is64 ? DSI_WROFF_FPWRITEEXT : DSI_WROFF_FPWRITE)) {
        err = AFPERR_PARAM;
        goto afp_write_err;
    }

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_write: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        err = AFPERR_PARAM;
        goto afp_write_err;
    }

    LOG(log_maxdebug, logtype_afpd,
        "afp_write(fork: %" PRIu16 " [%s], off: %" PRIu64 ", size: %" PRIu64 ")",
        ofork->of_refnum, (ofork->of_flags & AFPFORK_DATA) ? "data" : "reso", offset,
        reqcount);

    if (ofork->of_flags & AFPFORK_VIRTUAL) {
        if ((ofork->of_flags & AFPFORK_ACCWR) == 0) {
            err = AFPERR_ACCESS;
            goto afp_write_err;
        }

        err = materialize_virtual_icon(obj, ofork, true);

        if (err != AFP_OK) {
            goto afp_write_err;
        }
    }

    if ((ofork->of_flags & AFPFORK_ACCWR) == 0) {
        err = AFPERR_ACCESS;
        goto afp_write_err;
    }

    if (ofork->of_flags & AFPFORK_DATA) {
        eid = ADEID_DFORK;
    } else if (ofork->of_flags & AFPFORK_RSRC) {
        eid = ADEID_RFORK;
    } else {
        err = AFPERR_ACCESS; /* should never happen */
        goto afp_write_err;
    }

    /* an fstat() of the fork's file; only an EOF-relative write and the
     * volume size limit use it */
    oldsize = (endflag || ofork->of_vol->v_limitsize)
              ? ad_size(ofork->of_ad, eid) : 0;

    if (endflag) {
        /* an end-relative offset must stay within off_t */
        if (offset > INT64_MAX - oldsize) {
            err = AFPERR_PARAM;
            goto afp_write_err;
        }

        offset += oldsize;
    }

    /* handle bogus parameters */
    if (reqcount < 0 || offset < 0) {
        err = AFPERR_PARAM;
        goto afp_write_err;
    }

    /* offset can overflow on 64-bit capable filesystems.
     * report disk full if that's going to happen. */
    if (sum_neg(is64, offset, reqcount,
                eid == ADEID_DFORK ? 0 : ad_getentryoff(ofork->of_ad, eid))) {
        LOG(log_error, logtype_afpd, "write_fork: DISK FULL");
        err = AFPERR_DFULL;
        goto afp_write_err;
    }

    if (!reqcount) { /* handle request counts of 0 */
        err = AFP_OK;
        goto afp_write_err;
    }

    AFP_WRITE_START((long)reqcount);
    saveoff = offset;

    if (obj->options.flags & OPTION_STRICT_LOCKING) {
        if (ad_tmplock(ofork->of_ad, eid, ADLOCK_WR, saveoff, reqcount,
                       ofork->of_refnum) < 0) {
            err = AFPERR_LOCK;
            AFP_WRITE_DONE();
            goto afp_write_err;
        }
    }

    switch (obj->proto) {
#ifndef NO_DDP

    case AFPPROTO_ASP:
        if (asp_wrtcont(obj->handle, rbuf, rbuflen) < 0) {
            LOG(log_error, logtype_afpd, "afp_write: asp_wrtcont: %s", strerror(errno));
            err = AFPERR_MISC;
            goto afp_write_done;
        }

        if (!write_fits_count(ofork, *rbuflen, reqcount)) {
            err = AFPERR_PARAM;
            goto afp_write_done;
        }

        if (obj->options.flags & OPTION_DEBUG) {
            printf("(write) len: %ld\n", (unsigned long)*rbuflen);
            bprint(rbuf, *rbuflen);
        }

        err = write_file(ofork, eid, &offset, rbuf, *rbuflen);
        break;
#endif /* no afp/asp */

    case AFPPROTO_DSI: {
        /* find out what we have already */
        char *wbuf = NULL;
        size_t len = dsi_writeinit(dsi, &wbuf);

        /* the lock range and the size check are ReqCount's, so no more may
         * land */
        if (!write_fits_count(ofork, len + dsi->datasize, reqcount)) {
            err = AFPERR_PARAM;
            goto afp_write_done;
        }

        if ((err = write_file(ofork, eid, &offset, wbuf, len)) < 0) {
            goto afp_write_done;
        }

#ifdef WITH_RECVFILE

        if (obj->options.flags & OPTION_RECVFILE) {
            LOG(log_maxdebug, logtype_afpd,
                "afp_write(fork: %" PRIu16 " [%s], off: %" PRIu64 ", size: %" PRIu32 ")",
                ofork->of_refnum, (ofork->of_flags & AFPFORK_DATA) ? "data" : "reso", offset,
                dsi->datasize);

            if (ad_recvfile(ofork->of_ad, eid, dsi, &offset) < 0) {
                err = fork_error(ofork, "ad_recvfile", true);
                goto afp_write_done;
            }
        }

#endif

        while ((len = dsi_write(dsi, dsi->commands, dsi->server_quantum)) > 0) {
            if ((err = write_file(ofork, eid, &offset, (char *)dsi->commands,
                                  len)) < 0) {
                goto afp_write_done;
            }

            LOG(log_maxdebug, logtype_afpd,
                "afp_write: wrote: %zu, offset: %jd", len, (intmax_t)offset);
        }

        /* dsi_write() reads 0 at EOF as well as at the end of the payload;
         * a wait that disconnected the session left its own errno */
        if (dsi->datasize > 0) {
            if (!(dsi->flags & DSI_DISCONNECTED)) {
                errno = ECONNRESET;
            }

            err = fork_error(ofork, "dsi_write", true);
        }
    }
    break;
    }

afp_write_done:

    if (err < 0 && obj->proto == AFPPROTO_DSI) {
        dsi_writeflush(dsi);
    }

    if (obj->options.flags & OPTION_STRICT_LOCKING) {
        ad_tmplock(ofork->of_ad, eid, ADLOCK_CLR, saveoff, reqcount,
                   ofork->of_refnum);
    }

    if (offset > saveoff) {
        /* open metadata turns dirty: a resource fork's flush or close stamps
         * the modification date */
        if (ad_meta_fileno(ofork->of_ad) != -1) {
            ofork->of_flags |= AFPFORK_DIRTY;
        }

        fork_modified(obj, ofork);

        /* bytes appended, for the volume size limit */
        if (ofork->of_vol->v_limitsize && offset > oldsize) {
            ofork->of_vol->v_appended += offset - oldsize;
        }
    }

    AFP_WRITE_DONE();

    if (err < 0) {
        *rbuflen = 0;
        return err;
    }

    *rbuflen = set_off_t (offset, rbuf, is64);
    return AFP_OK;
afp_write_err:

    if (obj->proto == AFPPROTO_DSI) {
        dsi_writedrain(dsi);
    }

    /* the drain used the reply buffer; a count of 0 replies its offset */
    *rbuflen = err == AFP_OK ? set_off_t(offset, rbuf, is64) : 0;
    return err;
}

/*!
 * @brief FPWrite: write to an open fork with 32-bit offset and count
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length
 * @param[out]    rbuf     reply block, receives the new offset
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, or the AFPERR_* code of write_fork
 */
int afp_write(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
              size_t *rbuflen)
{
    return write_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 0);
}

/*!
 * @brief FPWriteExt: write to an open fork with 64-bit offset and count
 *
 * afpd ignores SIGXFSZ, so a write past the file size limit fails with EFBIG
 * and replies AFPERR_DFULL.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length
 * @param[out]    rbuf     reply block, receives the new offset
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, or the AFPERR_* code of write_fork
 */
int afp_write_ext(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf,
                  size_t *rbuflen)
{
    return write_fork(obj, ibuf, ibuflen, rbuf, rbuflen, 1);
}

/*!
 * @brief FPGetForkParms: return the parameters of an open fork
 *
 * Open metadata gets a reread first, so the reply reflects changes of other
 * processes. The reply carries the bitmap and the parameters of
 * getforkparams.
 *
 * @param[in]     obj      session object
 * @param[in]     ibuf     request block
 * @param[in]     ibuflen  request block length, unused
 * @param[out]    rbuf     reply block
 * @param[in,out] rbuflen  reply buffer capacity on entry, reply length on
 *                         return
 * @returns AFP_OK, AFPERR_PARAM for an unknown fork reference or a failed
 *          reread, or the error of getforkparams
 */
int afp_getforkparams(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf,
                      size_t *rbuflen)
{
    struct ofork    *ofork;
    int             ret;
    uint16_t       ofrefnum, bitmap;
    size_t          buflen = 0;
    ibuf += 2;
    memcpy(&ofrefnum, ibuf, sizeof(ofrefnum));
    ibuf += sizeof(ofrefnum);
    memcpy(&bitmap, ibuf, sizeof(bitmap));
    bitmap = ntohs(bitmap);
    ibuf += sizeof(bitmap);
    *rbuflen = 0;

    if (NULL == (ofork = of_find(ofrefnum))) {
        LOG(log_error, logtype_afpd,
            "afp_getforkparams: of_find(%" PRIu16 ") could not locate fork"
            " (ntohs: %" PRIu16 ")",
            ofrefnum, ntohs(ofrefnum));
        return AFPERR_PARAM;
    }

    if (ad_meta_open(ofork->of_ad)) {
        if (ad_refresh(NULL, ofork->of_ad) < 0) {
            LOG(log_error, logtype_afpd, "getforkparams(%s): ad_refresh: %s",
                of_name(ofork), strerror(errno));
            return AFPERR_PARAM;
        }
    }

    if (AFP_OK != (ret = getforkparams(obj, ofork, bitmap, rbuf + sizeof(u_short),
                                       &buflen))) {
        return ret;
    }

    *rbuflen = buflen + sizeof(u_short);
    bitmap = htons(bitmap);
    memcpy(rbuf, &bitmap, sizeof(bitmap));
    return AFP_OK;
}
