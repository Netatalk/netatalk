/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

/*!
 * @file
 * @brief Unix modes, owners, and AFP access rights of files and directories
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

/*!
 * @brief Free and total space of the file system that holds the volume
 *
 * The values come from statfs in bytes, saturated at UINT64_MAX.
 *
 * @param[in]  vol     volume
 * @param[out] bfree   receives the bytes available to the user
 * @param[out] btotal  receives the total bytes
 * @param[out] bsize   receives the fragment size
 * @returns AFP_OK, or AFPERR_PARAM when statfs failed
 */
int ustatfs_getvolspace(const struct vol *vol, VolSpace *bfree,
                        VolSpace *btotal, uint32_t *bsize)
{
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

/*!
 * @brief Map the three permission bits of one Unix class to AFP access rights
 *
 * @param[in] bits  mode, shifted so that the class sits in the low three bits
 * @returns AR_UREAD, AR_UWRITE, and AR_USEARCH as the bits allow
 */
static int utombits(mode_t bits)
{
    int		mbits;
    mbits = 0;
    mbits |= (bits & (S_IREAD >> 6))  ? AR_UREAD  : 0;
    mbits |= (bits & (S_IWRITE >> 6)) ? AR_UWRITE : 0;
    /* Do we really need this? */
    mbits |= (bits & (S_IEXEC >> 6)) ? AR_USEARCH : 0;
    return mbits;
}

/*!
 * @brief Derive the AFP access rights of owner, group, world, and user from a
 * stat
 *
 * Root gets every right. The owner gets the owner bits and AR_UOWN, a group
 * member gets the group bits, and anyone else gets the world bits. A file
 * that root owns also grants AR_UOWN to the user. The mapping follows page 63
 * of the AFP 3.0 specification.
 *
 * @param[in]  obj   session object with the user and group IDs
 * @param[in]  stat  stat of the file or directory
 * @param[out] ma    receives the access rights
 */
static void utommode(const AFPObj *obj, const struct stat *stat,
                     struct maccess *ma)
{
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

/*!
 * @brief AFP access rights of a file or directory from its stat and ACLs
 *
 * The rights come from the Unix mode through utommode, and with ACL support
 * acltoownermode refines them. The access call gives wrong answers over NFS,
 * so the rights come from a stat. A failed stat leaves every right at zero.
 *
 * @param[in]  obj   session object
 * @param[in]  vol   volume of the path
 * @param[in]  path  file or directory
 * @param[out] ma    receives the access rights
 * @param[in]  dir   unused
 * @param[in]  st    stat of path, or NULL to stat it here
 */
void accessmode(const AFPObj *obj, const struct vol *vol, char *path,
                struct maccess *ma, struct dir *dir _U_, struct stat *st)
{
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

/*!
 * @brief Does the session user hold rights on a file or directory?
 *
 * afpd's ACL mappings only add to the user's rights, so the ACL is read
 * only when the mode bits leave a right out.
 *
 * @param[in]     obj     session
 * @param[in]     vol     volume
 * @param[in]     path    file or directory
 * @param[in,out] st      stat of path, whose mode an ACL read can rewrite
 * @param[in]     rights  the AR_U* rights asked
 *
 * @returns true when the user holds every right asked
 */
bool accessmode_has(const AFPObj *obj, const struct vol *vol, char *path,
                    struct stat *st, uint8_t rights)
{
    struct maccess ma;
    utommode(obj, st, &ma);

    if ((ma.ma_user & rights) != rights) {
        accessmode(obj, vol, path, &ma, NULL, st);
    }

    return (ma.ma_user & rights) == rights;
}

/*!
 * @brief Does the session user have read and search rights on a directory?
 *
 * @param[in]     obj   session
 * @param[in]     vol   volume
 * @param[in]     path  directory
 * @param[in,out] st    stat of path, whose mode an ACL read can rewrite
 *
 * @returns true when the user may read and search path
 */
bool accessmode_rx(const AFPObj *obj, const struct vol *vol, char *path,
                   struct stat *st)
{
    return accessmode_has(obj, vol, path, st, AR_UREAD | AR_USEARCH);
}

/*!
 * @brief Map AFP access rights to the three permission bits of one Unix class
 *
 * Read and write each also grant search, because a Mac client cannot set the
 * search right separately.
 *
 * @param[in] bits  AR_UREAD and AR_UWRITE rights
 * @returns the mode bits in the low three positions
 */
static mode_t mtoubits(uint8_t bits)
{
    mode_t	mode;
    mode = 0;
    mode |= (bits & AR_UREAD) ? ((S_IREAD | S_IEXEC) >> 6) : 0;
    mode |= (bits & AR_UWRITE) ? ((S_IWRITE | S_IEXEC) >> 6) : 0;
    /* I don't think there's a way to set the SEARCH bit by itself on a Mac
        mode |= ( bits & AR_USEARCH ) ? ( S_IEXEC >> 6 ) : 0; */
    return mode;
}

/*!
 * @brief Build a Unix mode from AFP access rights
 *
 * The owner bits get the owner and world rights, the group bits the group and
 * world rights, and the other bits the world rights. Read and write each
 * also grant search. The mapping follows the Finder sharing window and page
 * 63 of the AFP 3.0 specification.
 *
 * @param[in] ma  AFP access rights
 * @returns the permission bits of the mode
 */
mode_t mtoumode(struct maccess *ma)
{
    mode_t		mode;
    mode = 0;
    mode |= mtoubits(ma->ma_owner | ma->ma_world);
    mode = mode << 3;
    mode |= mtoubits(ma->ma_group | ma->ma_world);
    mode = mode << 3;
    mode |= mtoubits(ma->ma_world);
    return mode;
}

/*!
 * @brief Apply a Unix mode to a file and to its AppleDouble metadata
 *
 * The mode gains the volume file permissions. The vfs_setfilmode hooks then
 * derive the metadata mode, which needs write access wherever read access
 * exists.
 *
 * @param[in]     vol   volume of the file
 * @param[in,out] path  file, its stat filled when missing
 * @param[in]     mode  new mode
 * @returns 0, or -1 when the stat or a mode change failed
 */
int setfilunixmode(const struct vol *vol, struct path *path, mode_t mode)
{
    if (!path->st_valid) {
        of_stat(vol, path);
    }

    if (path->st_errno) {
        return -1;
    }

    mode |= vol_fperm(vol);

    if (setfilmode(vol, path->u_name, mode, &path->st) < 0) {
        return -1;
    }

    /* we need to set write perm if read set for resource fork */
    return vol->vfs->vfs_setfilmode(vol, path->u_name, mode, &path->st);
}


/*!
 * @brief Apply a Unix mode to a directory and to its AppleDouble metadata
 *
 * The mode gains the volume directory permissions and DIRBITS, minus the
 * volume umask. A mode that grants the owner read and search access changes
 * the directory before the vfs_setdirunixmode hooks. Any other mode changes
 * it after them, so the metadata stays reachable.
 *
 * @param[in] vol   volume of the directory
 * @param[in] name  directory
 * @param[in] mode  new mode
 * @returns 0, or -1 when a mode change failed
 */
int setdirunixmode(const struct vol *vol, char *name, mode_t mode)
{
    LOG(log_debug, logtype_afpd, "setdirunixmode('%s', mode:%04o) {v_dperm:%04o}",
        fullpathname(name), mode, vol_dperm(vol));
    mode |= vol_dperm(vol) | DIRBITS;
    mode &= ~vol_umask(vol);

    if (dir_rx_set(mode)) {
        /* extending right? dir first then .AppleDouble in rf_setdirmode */
        if (ochmod(name, mode, NULL,
                   vol_syml_opt(vol) | vol_chmod_opt(vol)
                  ) < 0) {
            return -1;
        }
    }

    if (vol->vfs->vfs_setdirunixmode(vol, name, mode, NULL) < 0) {
        return  -1;
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

/*!
 * @brief Give a file and its AppleDouble metadata a new owner and group
 *
 * EPERM counts as success for both changes.
 *
 * @param[in] vol   volume of the file
 * @param[in] uid   new owner
 * @param[in] gid   new group
 * @param[in] path  file
 * @returns 0, or -1 when a change failed with another error
 */
int setfilowner(const struct vol *vol, const uid_t uid, const gid_t gid,
                struct path *path)
{
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

/*!
 * @brief Give a directory and its AppleDouble metadata a new owner and group
 *
 * A failed change of the directory itself only logs. The vfs_setdirowner
 * hooks then update the metadata. A uid or gid of 0 gets no special
 * treatment.
 *
 * @param[in] vol   volume of the directory
 * @param[in] name  directory
 * @param[in] uid   new owner
 * @param[in] gid   new group
 * @returns 0, or -1 when a hook failed
 */
int setdirowner(const struct vol *vol, const char *name, const uid_t uid,
                const gid_t gid)
{
    if (ochown(name, uid, gid, vol_syml_opt(vol)) < 0 && errno != EPERM) {
        LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d %s: %s",
            uid, gid, fullpathname(name), strerror(errno));
    }

    if (vol->vfs->vfs_setdirowner(vol, name, uid, gid) < 0) {
        return -1;
    }

    return 0;
}
