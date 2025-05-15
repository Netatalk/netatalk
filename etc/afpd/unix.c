/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
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
#include <sys/param.h>

#include <atalk/acl.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/vfs.h>

#ifdef HAVE_ACLS
#include "acls.h"
#endif

#include "auth.h"
#include "directory.h"
#include "fork.h"
#include "unix.h"
#include "volume.h"

/*
 * Get the free space on a partition.
 */
int ustatfs_getvolspace(const struct vol *vol, VolSpace *bfree,
                        VolSpace *btotal, uint32_t *bsize) {
    VolSpace maxVolSpace = UINT64_MAX;
    struct statfs	sfs;

    if (statfs(vol->v_path, &sfs) < 0) {
        LOG(log_error, logtype_afpd, "ustatfs_getvolspace unable to stat %s",
            vol->v_path);
        return AFPERR_PARAM;
    }

    *bfree = (VolSpace) sfs.f_bavail;
    *bsize = sfs.f_frsize;

    if (*bfree > maxVolSpace / *bsize) {
        *bfree = maxVolSpace;
    } else {
        *bfree *= *bsize;
    }

    *btotal = (VolSpace) sfs.f_blocks;

    /* see similar block above comments */
    if (*btotal > maxVolSpace / *bsize) {
        *btotal = maxVolSpace;
    } else {
        *btotal *= *bsize;
    }

    return AFP_OK;
}

static int utombits(mode_t bits) {
    int		mbits;
    mbits = 0;
    mbits |= (bits & (S_IREAD >> 6))  ? AR_UREAD  : 0;
    mbits |= (bits & (S_IWRITE >> 6)) ? AR_UWRITE : 0;
    /* Do we really need this? */
    mbits |= (bits & (S_IEXEC >> 6)) ? AR_USEARCH : 0;
    return mbits;
}

/* --------------------------------
    cf AFP 3.0 page 63
*/
static void utommode(const AFPObj *obj, const struct stat *stat,
                     struct maccess *ma) {
    mode_t mode;
    mode = stat->st_mode;
    ma->ma_world = utombits(mode);
    mode = mode >> 3;
    ma->ma_group = utombits(mode);
    mode = mode >> 3;
    ma->ma_owner = utombits(mode);

    /* ma_user is a union of all permissions but we must follow
     * unix perm
    */
    if (obj->euid == 0) {
        ma->ma_user = AR_UREAD | AR_UWRITE | AR_USEARCH | AR_UOWN;
    } else if (obj->uid == stat->st_uid) {
        ma->ma_user = ma->ma_owner | AR_UOWN;
    } else if (gmem(stat->st_gid, obj->ngroups, obj->groups)) {
        ma->ma_user = ma->ma_group;
    } else {
        ma->ma_user = ma->ma_world;
    }

    /*
     * There are certain things the mac won't try if you don't have
     * the "owner" bit set, even tho you can do these things on unix wiht
     * only write permission.  What were the things?
     *
     * FIXME
     * ditto seems to care if st_uid is 0 ?
     * was ma->ma_user & AR_UWRITE
     * but 0 as owner is a can of worms.
     */
    if (!stat->st_uid) {
        ma->ma_user |= AR_UOWN;
    }
}

/*
 * Calculate the mode for a directory using a stat() call to
 * estimate permission.
 *
 * Note: the previous method, using access(), does not work correctly
 * over NFS.
 */
void accessmode(const AFPObj *obj, const struct vol *vol, char *path,
                struct maccess *ma, struct dir *dir _U_, struct stat *st) {
    struct stat     sb;
    ma->ma_user = ma->ma_owner = ma->ma_world = ma->ma_group = 0;

    if (!st) {
        if (ostat(path, &sb, vol_syml_opt(vol)) != 0) {
            return;
        }

        st = &sb;
    }

    utommode(obj, st, ma);
#ifdef HAVE_ACLS
    acltoownermode(obj, vol, path, st, ma);
#endif
}

static mode_t mtoubits(uint8_t bits) {
    mode_t	mode;
    mode = 0;
    mode |= (bits & AR_UREAD) ? ((S_IREAD | S_IEXEC) >> 6) : 0;
    mode |= (bits & AR_UWRITE) ? ((S_IWRITE | S_IEXEC) >> 6) : 0;
    /* I don't think there's a way to set the SEARCH bit by itself on a Mac
        mode |= ( bits & AR_USEARCH ) ? ( S_IEXEC >> 6 ) : 0; */
    return mode;
}

/* ----------------------------------
   from the finder's share windows (menu--> File--> sharing...)
   and from AFP 3.0 spec page 63
   the mac mode should be save somewhere
*/
mode_t mtoumode(struct maccess *ma) {
    mode_t		mode;
    mode = 0;
    mode |= mtoubits(ma->ma_owner | ma->ma_world);
    mode = mode << 3;
    mode |= mtoubits(ma->ma_group | ma->ma_world);
    mode = mode << 3;
    mode |= mtoubits(ma->ma_world);
    return mode;
}

/* --------------------- */
int setfilunixmode(const struct vol *vol, struct path* path, mode_t mode) {
    if (!path->st_valid) {
        of_stat(vol, path);
    }

    if (path->st_errno) {
        return -1;
    }

    mode |= vol->v_fperm;

    if (setfilmode(vol, path->u_name, mode, &path->st) < 0) {
        return -1;
    }

    /* we need to set write perm if read set for resource fork */
    return vol->vfs->vfs_setfilmode(vol, path->u_name, mode, &path->st);
}


/* --------------------- */
int setdirunixmode(const struct vol *vol, char *name, mode_t mode) {
    LOG(log_debug, logtype_afpd, "setdirunixmode('%s', mode:%04o) {v_dperm:%04o}",
        fullpathname(name), mode, vol->v_dperm);
    mode |= vol->v_dperm | DIRBITS;
    mode &= ~vol->v_umask;

    if (dir_rx_set(mode)) {
        /* extending right? dir first then .AppleDouble in rf_setdirmode */
        if (ochmod(name, mode, NULL,
                   vol_syml_opt(vol) | vol_chmod_opt(vol)
                  ) < 0) {
            return -1;
        }
    }

    if (vol->vfs->vfs_setdirunixmode(vol, name, mode, NULL) < 0) {
        return  -1 ;
    }

    if (!dir_rx_set(mode)) {
        if (ochmod(name, mode, NULL,
                   vol_syml_opt(vol) | vol_chmod_opt(vol)
                  ) < 0) {
            return -1;
        }
    }

    return 0;
}

/* ----------------------------- */
int setfilowner(const struct vol *vol, const uid_t uid, const gid_t gid,
                struct path* path) {
    if (ochown(path->u_name, uid, gid, vol_syml_opt(vol)) < 0 && errno != EPERM) {
        LOG(log_debug, logtype_afpd, "setfilowner: chown %d/%d %s: %s",
            uid, gid, path->u_name, strerror(errno));
        return -1;
    }

    if (vol->vfs->vfs_chown(vol, path->u_name, uid, gid) < 0 && errno != EPERM) {
        LOG(log_debug, logtype_afpd, "setfilowner: rf_chown %d/%d %s: %s",
            uid, gid, path->u_name, strerror(errno));
        return -1;
    }

    return 0;
}

/* ---------------------------------
 * uid/gid == 0 need to be handled as special cases. they really mean
 * that user/group should inherit from other, but that doesn't fit
 * into the unix permission scheme. we can get around this by
 * co-opting some bits. */
int setdirowner(const struct vol *vol, const char *name, const uid_t uid,
                const gid_t gid) {
    if (ochown(name, uid, gid, vol_syml_opt(vol)) < 0 && errno != EPERM) {
        LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d %s: %s",
            uid, gid, fullpathname(name), strerror(errno));
    }

    if (vol->vfs->vfs_setdirowner(vol, name, uid, gid) < 0) {
        return -1;
    }

    return 0;
}
