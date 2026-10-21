/*
    Copyright (c) 2004 Didier Gautheron
    Copyright (c) 2009 Frank Lahm

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

/*!
 * @file
 * @brief VFS operation tables for the AppleDouble layouts and the EA and ACL
 * layers
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <atalk/acl.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/bstrlib_compat.h>
#include <atalk/compat.h>
#include <atalk/directory.h>
#include <atalk/ea.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/vfs.h>
#include <atalk/volume.h>

struct perm {
    uid_t uid;
    gid_t gid;
};

typedef int (*rf_loop)(const struct vol *, struct dirent *, char *, void *,
                       int);

/*!
 * @brief Call fn for every entry of directory name except . and ..
 *
 * fn receives the entry and its path below name, and the walk stops at the
 * first nonzero result. A directory that fails to open yields 0 under a
 * nonzero flag. Otherwise the call logs an error that names from and fails.
 *
 * @param[in] from  caller name, for the log line
 * @param[in] name  directory to walk
 * @param[in] fn    callback, or NULL to walk without one
 * @param[in] vol   volume, passed to fn
 * @param[in] data  callback data, passed to fn
 * @param[in] flag  nonzero to tolerate an opendir failure, passed to fn
 * @returns 0, the first nonzero fn result, or -1 when opendir failed and flag
 *          is 0
 */
static int
for_each_adouble(const char *from, const char *name, rf_loop fn,
                 const struct vol *vol, void *data, int flag)
{
    char            buf[MAXPATHLEN + 1];
    char            *m;
    DIR             *dp;
    struct dirent   *de;
    int             ret;

    if (NULL == (dp = opendir(name))) {
        if (!flag) {
            LOG(log_error, logtype_afpd, "%s: opendir %s: %s", from, fullpathname(name),
                strerror(errno));
            return -1;
        }

        return 0;
    }

    strlcpy(buf, name, sizeof(buf));
    strlcat(buf, "/", sizeof(buf));
    m = strchr(buf, '\0');
    ret = 0;

    while ((de = readdir(dp))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
            continue;
        }

        strlcat(buf, de->d_name, sizeof(buf));

        if (fn && (ret = fn(vol, de, buf, data, flag))) {
            closedir(dp);
            return ret;
        }

        *m = 0;
    }

    closedir(dp);
    return ret;
}

/*!
 * @brief Whether name differs from a name netatalk reserves, in either ASCII
 *        case where the volume's filesystem ignores case
 *
 * @param[in] vol       volume
 * @param[in] name      directory entry name
 * @param[in] reserved  the reserved name
 * @returns 1 when name differs, 0 when it is the reserved name
 */
static int other_name(const struct vol *vol, const char *name,
                      const char *reserved)
{
    size_t i = 0;

    if (!vol->v_ignorecase) {
        return strcmp(name, reserved) != 0;
    }

    while (name[i] != '\0' && ascii_tolower((unsigned char)name[i])
            == ascii_tolower((unsigned char)reserved[i])) {
        i++;
    }

    return name[i] != reserved[i];
}

/*!
 * @brief Whether name differs from the netatalk database directory names
 *
 * @param[in] vol   volume
 * @param[in] name  directory entry name
 * @returns 0 for .AppleDB and .AppleDesktop, in either case where the
 *          volume's filesystem ignores case, else 1
 */
static int netatalk_name(const struct vol *vol, const char *name)
{
    return other_name(vol, name, ".AppleDB")
           && other_name(vol, name, ".AppleDesktop");
}

/*******************************************************************************
 * classic adouble format
 *******************************************************************************/

/*!
 * @brief Whether clients may see name on a version 2 AppleDouble volume
 *
 * Names without a leading dot always pass. The netatalk directories
 * .AppleDB, .AppleDesktop, and .AppleDouble stay hidden, in either case where
 * the volume's filesystem ignores case, and so does .Parent in any letter
 * case.
 *
 * @param[in] vol   volume
 * @param[in] name  directory entry name
 * @returns 1 when clients may see name, 0 otherwise
 */
static int validupath_adouble(const struct vol *vol, const char *name)
{
    if (name[0] != '.') {
        return 1;
    }

    return netatalk_name(vol, name) && other_name(vol, name, ".AppleDouble")
           && strcasecmp(name, ".Parent");
}

/*!
 * @brief Give the version 2 metadata file of path a new owner
 *
 * chown follows symlinks, and a missing metadata file counts as success.
 *
 * @param[in] vol   volume holding the file
 * @param[in] path  file whose metadata file to change
 * @param[in] uid   new owner
 * @param[in] gid   new group
 * @returns 0, or -1 with errno set
 */
static int RF_chown_adouble(const struct vol *vol, const char *path, uid_t uid,
                            gid_t gid)
{
    const char *ad_p = vol->ad_path(path, ADFLAGS_HF);

    if (chown(ad_p, uid, gid) < 0) {
        if (errno == ENOENT) {
            /* Ignore if file doesn't exist */
            return 0;
        }

        return -1;
    }

    return 0;
}

/*!
 * @brief Rename nothing, because the .AppleDouble directory moves with the
 * directory
 *
 * @param[in] vol      unused
 * @param[in] dirfd    unused
 * @param[in] oldpath  unused
 * @param[in] newpath  unused
 * @returns 0
 */
static int RF_renamedir_adouble(const struct vol *vol _U_, int dirfd _U_,
                                const char *oldpath _U_,
                                const char *newpath _U_)
{
    return 0;
}

/*!
 * @brief Delete one .AppleDouble entry whose file no longer exists in the
 * working directory
 *
 * A file of the same name in the working directory means the directory still
 * has content, and the walk stops with AFPERR_DIRNEMPT. lstat also sees a
 * dangling symlink. Any other entry, the .Parent file among them, goes through
 * netatalk_unlink.
 *
 * @param[in] vol   unused
 * @param[in] de    .AppleDouble entry
 * @param[in] name  path of the entry
 * @param[in] data  unused
 * @param[in] flag  unused
 * @returns AFP_OK, AFPERR_DIRNEMPT, or the netatalk_unlink error
 */
static int deletecurdir_adouble_loop(const struct vol *vol _U_,
                                     struct dirent *de, char *name, void *data _U_, int flag _U_)
{
    struct stat st;
    int         err;

    /* bail if the file exists in the current directory.
     * note: this will not fail with dangling symlinks */

    if (lstat(de->d_name, &st) == 0) {
        return AFPERR_DIRNEMPT;
    }

    if ((err = netatalk_unlink(name))) {
        return err;
    }

    return 0;
}

/*!
 * @brief Delete the .AppleDouble directory of the working directory
 *
 * The walk first deletes every metadata file whose file no longer exists, the
 * .Parent file among them. A metadata file that still has its file stops the
 * call with AFPERR_DIRNEMPT. A missing .AppleDouble directory counts as
 * success.
 *
 * @param[in] vol  volume holding the directory
 * @returns AFP_OK, or the AFP error of the first failure
 */
static int RF_deletecurdir_adouble(const struct vol *vol)
{
    int err;

    /* delete stray .AppleDouble files. this happens to get .Parent files
       as well. */
    if ((err = for_each_adouble("deletecurdir", ".AppleDouble",
                                deletecurdir_adouble_loop, vol, NULL, 1))) {
        return err;
    }

    return netatalk_rmdir(-1, ".AppleDouble");
}

/*!
 * @brief Derive a metadata file mode with ad_hf_mode and apply it to name
 *
 * @param[in] vol   volume holding the file
 * @param[in] name  metadata file
 * @param[in] mode  data file mode
 * @param[in] st    stat whose non-permission bits to keep, or NULL
 * @returns 0, or -1 with errno set
 */
static int adouble_setfilmode(const struct vol *vol, const char *name,
                              mode_t mode, struct stat *st)
{
    return setfilmode(vol, name, ad_hf_mode(mode), st);
}

/*!
 * @brief Apply the mode of a data file to its version 2 metadata file
 *
 * ad_hf_mode derives the metadata mode, so read access also grants write
 * access for fcntl locks.
 *
 * @param[in] vol   volume holding the file
 * @param[in] name  data file
 * @param[in] mode  new data file mode
 * @param[in] st    stat of the data file, or NULL
 * @returns 0, or -1 with errno set
 */
static int RF_setfilmode_adouble(const struct vol *vol, const char *name,
                                 mode_t mode, struct stat *st)
{
    return adouble_setfilmode(vol, vol->ad_path(name, ADFLAGS_HF), mode, st);
}

/*!
 * @brief Apply a directory mode to its .AppleDouble directory and .Parent file
 *
 * The .AppleDouble directory gets DIRBITS plus mode, minus the volume umask,
 * and .Parent gets the ad_hf_mode result. A mode that grants the owner read
 * and search access changes the directory first, otherwise last. That ordering
 * keeps .Parent reachable while it changes.
 *
 * @param[in] vol   volume holding the directory
 * @param[in] name  directory
 * @param[in] mode  new directory mode
 * @param[in] st    stat of name, or NULL
 * @returns 0, or -1 with errno set
 */
static int RF_setdirunixmode_adouble(const struct vol *vol, const char *name,
                                     mode_t mode, struct stat *st)
{
    const char *adouble = vol->ad_path(name, ADFLAGS_DIR);

    if (dir_rx_set(mode)) {
        if (ochmod(ad_dir(adouble),
                   (DIRBITS | mode) & ~vol_umask(vol),
                   st,
                   vol_syml_opt(vol) | vol_chmod_opt(vol)
                  ) < 0) {
            return -1;
        }
    }

    if (adouble_setfilmode(vol, vol->ad_path(name, ADFLAGS_DIR), mode, st) < 0) {
        return -1;
    }

    if (!dir_rx_set(mode)) {
        if (ochmod(ad_dir(adouble),
                   (DIRBITS | mode) & ~vol_umask(vol),
                   st,
                   vol_syml_opt(vol) | vol_chmod_opt(vol)
                  ) < 0) {
            return  -1;
        }
    }

    return 0;
}

/*!
 * @brief Apply the metadata mode in data to one .AppleDouble entry
 *
 * Subdirectories keep their mode. A failed stat logs an error unless flag is
 * nonzero, and a failed mode change passes without notice, so the walk always
 * continues.
 *
 * @param[in] vol   volume holding the directory
 * @param[in] de    unused
 * @param[in] name  path of the entry
 * @param[in] data  pointer to the metadata mode
 * @param[in] flag  nonzero to stay silent about a failed stat
 * @returns 0
 */
static int setdirmode_adouble_loop(const struct vol *vol, struct dirent *de _U_,
                                   char *name, void *data, int flag)
{
    mode_t hf_mode = *(mode_t *)data;
    struct stat st;

    if (ostat(name, &st, vol_syml_opt(vol)) < 0) {
        if (flag) {
            return 0;
        }

        LOG(log_error, logtype_afpd, "setdirmode: stat %s: %s", name, strerror(errno));
    } else if (!S_ISDIR(st.st_mode)) {
        if (setfilmode(vol, name, hf_mode, &st) < 0) {
            /* FIXME what do we do then? */
        }
    }

    return 0;
}

/*!
 * @brief Apply a directory mode to its .AppleDouble directory and the metadata
 * files there
 *
 * The .AppleDouble directory gets DIRBITS plus mode, minus the volume umask.
 * Each metadata file gets the ad_hf_mode result. A mode that grants the owner
 * read and search access changes the directory first, otherwise last, so the
 * metadata files stay reachable while they change.
 *
 * @param[in] vol   volume holding the directory
 * @param[in] name  directory
 * @param[in] mode  new directory mode
 * @param[in] st    stat of name, or NULL
 * @returns 0, or -1 when a chmod or the directory walk failed
 */
static int RF_setdirmode_adouble(const struct vol *vol, const char *name,
                                 mode_t mode, struct stat *st)
{
    mode_t hf_mode = ad_hf_mode(mode);
    const char  *adouble = vol->ad_path(name, ADFLAGS_DIR);
    const char  *adouble_p = ad_dir(adouble);

    if (dir_rx_set(mode)) {
        if (ochmod(ad_dir(adouble),
                   (DIRBITS | mode) & ~vol_umask(vol),
                   st,
                   vol_syml_opt(vol) | vol_chmod_opt(vol)
                  ) < 0) {
            return -1;
        }
    }

    if (for_each_adouble("setdirmode", adouble_p, setdirmode_adouble_loop, vol,
                         &hf_mode, 0)) {
        return -1;
    }

    if (!dir_rx_set(mode)) {
        if (ochmod(ad_dir(adouble),
                   (DIRBITS | mode) & ~vol_umask(vol),
                   st,
                   vol_syml_opt(vol) | vol_chmod_opt(vol)
                  ) < 0) {
            return  -1;
        }
    }

    return 0;
}

/*!
 * @brief Give the .AppleDouble directory of the working directory a new owner
 *
 * The call ignores name and acts on the working directory. EPERM passes
 * without notice, and other failures only log at debug level.
 *
 * @param[in] vol   unused
 * @param[in] name  unused
 * @param[in] uid   new owner
 * @param[in] gid   new group
 * @returns 0
 */
static int RF_setdirowner_adouble(const struct vol *vol _U_,
                                  const char *name _U_,
                                  uid_t uid, gid_t gid)
{
    if (lchown(".AppleDouble", uid, gid) < 0 && errno != EPERM) {
        LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d %s: %s",
            uid, gid, fullpathname(".AppleDouble"), strerror(errno));
    }

    return 0;
}

/*!
 * @brief Delete the version 2 metadata file of file
 *
 * A missing metadata file counts as success.
 *
 * @param[in] vol    volume holding the file
 * @param[in] dirfd  directory that file is relative to, or -1 for the working
 *                   directory
 * @param[in] file   data file
 * @returns AFP_OK, AFPERR_VLOCK on a read-only file system, AFPERR_ACCESS, or
 *          AFPERR_PARAM
 */
static int RF_deletefile_adouble(const struct vol *vol, int dirfd,
                                 const char *file)
{
    return netatalk_unlinkat(dirfd, vol->ad_path(file, ADFLAGS_HF));
}

/*!
 * @brief Rename the version 2 metadata file of src to match dst
 *
 * src is relative to dirfd, and dst is relative to the working directory. A
 * source without a metadata file needs nothing and succeeds. When the rename
 * fails with ENOENT for an existing source, the destination .AppleDouble
 * directory is missing. ad_open then creates it through a separate AppleDouble,
 * and the rename runs again.
 *
 * @param[in] vol    volume holding the file
 * @param[in] dirfd  directory that src is relative to, or -1 for the working
 *                   directory
 * @param[in] src    current file name
 * @param[in] dst    new file name
 * @returns 0, or -1 with errno set
 */
static int RF_renamefile_adouble(const struct vol *vol, int dirfd,
                                 const char *src, const char *dst)
{
    char  adsrc[MAXPATHLEN + 1];
    int   err = 0;
    strcpy(adsrc, vol->ad_path(src, 0));

    if (unix_rename(dirfd, adsrc, -1, vol->ad_path(dst, 0)) < 0) {
        struct stat st;
        err = errno;

        if (errno == ENOENT) {
            struct adouble    ad;

            /* source has no resource fork */
            if (ostatat(dirfd, adsrc, &st, vol_syml_opt(vol))) {
                return 0;
            }

            /* We are here  because :
             * -there's no dest folder.
             * -there's no .AppleDouble in the dest folder.
             * if we use the struct adouble passed in parameter it will not
             * create .AppleDouble if the file is already opened, so we
             * use a diff one, it's not a pb,ie it's not the same file, yet.
             */
            ad_init(&ad, vol);

            if (ad_open(&ad, dst, ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE, 0666) == 0) {
                ad_close(&ad, ADFLAGS_HF);

                if (!unix_rename(dirfd, adsrc, -1, vol->ad_path(dst, 0))) {
                    err = 0;
                } else {
                    err = errno;
                }
            } else {
                /* it's something else, bail out */
                err = errno;
            }
        }
    }

    if (err) {
        errno = err;
        return -1;
    }

    return 0;
}

/*!
 * @brief Clone the version 2 metadata file of src for dst
 *
 * The call stats dst first and fails when it does not exist. A directory pair
 * uses the .Parent files, and a file pair uses the files in the .AppleDouble
 * directory beside each. A source without a metadata file needs nothing and
 * succeeds.
 *
 * @param[in] vol  unused
 * @param[in] sfd  directory that src is relative to, or -1 for the working
 *                 directory
 * @param[in] src  source file or directory
 * @param[in] dst  existing destination file or directory
 * @returns 0, or -1 with errno set
 */
static int RF_copyfile_adouble(const struct vol *vol _U_, int sfd,
                               const char *src, const char *dst)
{
    EC_INIT;
    bstring s = NULL;
    bstring d = NULL;
    char src_name[MAXPATHLEN + 1];
    char dst_name[MAXPATHLEN + 1];
    char src_dir[MAXPATHLEN + 1];
    char dst_dir[MAXPATHLEN + 1];
    const char *name = NULL;
    const char *dir = NULL;
    struct stat st;
    EC_ZERO(stat(dst, &st));

    if (S_ISDIR(st.st_mode)) {
        /* build src path to AppleDouble file*/
        EC_NULL(s = bfromcstr(src));
        EC_ZERO(bcatcstr(s, "/.AppleDouble/.Parent"));
        /* build dst path to AppleDouble file*/
        EC_NULL(d = bfromcstr(dst));
        EC_ZERO(bcatcstr(d, "/.AppleDouble/.Parent"));
    } else {
        /* get basename */
        /* build src path to AppleDouble file*/
        strlcpy(src_name, src, sizeof(src_name));
        strlcpy(src_dir, src, sizeof(src_dir));
        EC_NULL(name = basename(src_name));
        EC_NULL(dir = dirname(src_dir));
        EC_NULL(s = bfromcstr(dir));
        EC_ZERO(bcatcstr(s, "/.AppleDouble/"));
        EC_ZERO(bcatcstr(s, name));
        /* build dst path to AppleDouble file*/
        strlcpy(dst_name, dst, sizeof(dst_name));
        strlcpy(dst_dir, dst, sizeof(dst_dir));
        EC_NULL(name = basename(dst_name));
        EC_NULL(dir = dirname(dst_dir));
        EC_NULL(d = bfromcstr(dir));
        EC_ZERO(bcatcstr(d, "/.AppleDouble/"));
        EC_ZERO(bcatcstr(d, name));
    }

    /* ignore errors */
    if (copy_file(sfd, cfrombstr(s), cfrombstr(d), 0666) != 0)
        if (errno != ENOENT) {
            EC_FAIL;
        }

EC_CLEANUP:
    bdestroy(s);
    bdestroy(d);
    EC_EXIT;
}

#ifdef HAVE_NFSV4_ACLS
/*!
 * @brief Apply an NFSv4 ACL change to the AppleDouble file of path
 *
 * A missing path and a directory need nothing and succeed. For a file, acl
 * runs cmd on the metadata path that vol->ad_path returns.
 *
 * @param[in] vol    volume holding the file
 * @param[in] path   file or directory
 * @param[in] cmd    acl command, ACE_SETACL from the callers
 * @param[in] count  number of entries in aces
 * @param[in] aces   NFSv4 access control entries
 * @returns AFP_OK, or AFPERR_MISC when the stat or the acl call failed
 */
static int RF_solaris_acl(const struct vol *vol, const char *path, int cmd,
                          int count, void *aces)
{
    static char buf[MAXPATHLEN + 1];
    struct stat st;
    int len;

    if ((stat(path, &st)) != 0) {
        if (errno == ENOENT) {
            return AFP_OK;
        }

        return AFPERR_MISC;
    }

    if (!S_ISDIR(st.st_mode)) {
        /* set ACL on resource fork */
        if ((acl(vol->ad_path(path, ADFLAGS_HF), cmd, count, aces)) != 0) {
            return AFPERR_MISC;
        }
    }

    return AFP_OK;
}

/*!
 * @brief Keep only the owner@, group@, and everyone@ entries in the NFSv4 ACL
 * of an AppleDouble file
 *
 * The AppleDouble file is the one that vol->ad_path returns for path.
 * Directories need nothing, and a missing AppleDouble file counts as success.
 *
 * @param[in] vol   volume holding the file
 * @param[in] path  file or directory
 * @param[in] dir   nonzero when path is a directory
 * @returns AFP_OK, or the AFP error from remove_nfsv4_acl_vfs
 */
static int RF_solaris_remove_acl(const struct vol *vol, const char *path,
                                 int dir)
{
    int ret;
    static char buf[MAXPATHLEN + 1];
    int len;

    if (dir) {
        return AFP_OK;
    }

    /* remove ACL from resource fork */
    if ((ret = remove_nfsv4_acl_vfs(vol->ad_path(path, ADFLAGS_HF))) != AFP_OK) {
        if (errno == ENOENT) {
            return AFP_OK;
        }

        return ret;
    }

    return AFP_OK;
}
#endif

#ifdef HAVE_POSIX_ACLS
/*!
 * @brief Apply a POSIX ACL to the AppleDouble file of path
 *
 * A directory needs nothing. For a file, acl_set_file sets acl of type on the
 * metadata path that vol->ad_path returns. ENOENT from either the stat or the
 * ACL call counts as success.
 *
 * @param[in] vol    volume holding the file
 * @param[in] path   file or directory
 * @param[in] type   ACL_TYPE_ACCESS or ACL_TYPE_DEFAULT
 * @param[in] count  unused
 * @param[in] acl    POSIX ACL to set
 * @returns AFP_OK, AFPERR_MISC when acl_set_file failed, or -1 when the stat
 *          failed
 */
static int RF_posix_acl(const struct vol *vol, const char *path,
                        acl_type_t type, int count _U_, acl_t acl)
{
    EC_INIT;
    struct stat st;

    if (stat(path, &st) == -1) {
        EC_FAIL;
    }

    if (!S_ISDIR(st.st_mode)) {
        /* set ACL on resource fork */
        EC_ZERO_ERR(acl_set_file(vol->ad_path(path, ADFLAGS_HF), type, acl),
                    AFPERR_MISC);
    }

EC_CLEANUP:

    if (errno == ENOENT) {
        EC_STATUS(AFP_OK);
    }

    EC_EXIT;
}

/*!
 * @brief Drop the ACL_USER, ACL_GROUP, ACL_MASK, and default entries from an
 * AppleDouble file
 *
 * The AppleDouble file is the one that vol->ad_path returns for path.
 * Directories need nothing, and a missing AppleDouble file counts as success.
 *
 * @param[in] vol   volume holding the file
 * @param[in] path  file or directory
 * @param[in] dir   nonzero when path is a directory
 * @returns AFP_OK, or AFPERR_MISC when remove_posix_acl_vfs failed
 */
static int RF_posix_remove_acl(const struct vol *vol, const char *path, int dir)
{
    EC_INIT;

    if (dir) {
        EC_EXIT_STATUS(AFP_OK);
    }

    /* remove ACL from resource fork */
    EC_ZERO_ERR(remove_posix_acl_vfs(vol->ad_path(path, ADFLAGS_HF)), AFPERR_MISC);
EC_CLEANUP:

    if (errno == ENOENT) {
        EC_STATUS(AFP_OK);
    }

    EC_EXIT;
}
#endif

/*************************************************************************
 * EA adouble format
 ************************************************************************/
/*!
 * @brief Whether clients may see name on an ea AppleDouble volume
 *
 * Names without a leading dot always pass. Of the dot names, .AppleDB and
 * .AppleDesktop stay hidden, in either case where the volume's filesystem
 * ignores case. Without HAVE_EAFD a ._ file stays hidden when it carries a
 * netatalk AppleDouble header, because it is a resource fork sidecar.
 *
 * @param[in] vol   volume
 * @param[in] name  directory entry name
 * @returns 1 when clients may see name, 0 otherwise
 */
static int validupath_ea(const struct vol *vol, const char *name)
{
    if (name[0] != '.') {
        return 1;
    }

#ifndef HAVE_EAFD

    if (name[1] == '_') {
        return ad_valid_header_osx(name);
    }

#endif
    return netatalk_name(vol, name);
}

/*!
 * @brief Give the ._ resource fork sidecar of path a new owner
 *
 * Without HAVE_EAFD chown follows symlinks, and a missing sidecar fails with
 * ENOENT. With HAVE_EAFD the resource fork lives in an EA, so nothing changes.
 *
 * @param[in] vol   volume holding the file
 * @param[in] path  file whose sidecar to change
 * @param[in] uid   new owner
 * @param[in] gid   new group
 * @returns 0, or -1 with errno set
 */
static int RF_chown_ea(const struct vol *vol, const char *path, uid_t uid,
                       gid_t gid)
{
#ifndef HAVE_EAFD
    return chown(vol->ad_path(path, ADFLAGS_HF), uid, gid);
#else
    (void)vol;
    (void)path;
    (void)uid;
    (void)gid;
    return 0;
#endif
}

/*!
 * @brief Rename nothing, because the ._ sidecars inside the directory move with
 * it
 *
 * @param[in] vol      unused
 * @param[in] dirfd    unused
 * @param[in] oldpath  unused
 * @param[in] newpath  unused
 * @returns 0
 */
static int RF_renamedir_ea(const struct vol *vol _U_, int dirfd _U_,
                           const char *oldpath _U_, const char *newpath _U_)
{
    return 0;
}

/*!
 * @brief Report a directory entry whose name has no leading dot
 *
 * @param[in] vol   unused
 * @param[in] de    directory entry
 * @param[in] name  unused
 * @param[in] data  unused
 * @param[in] flag  unused
 * @returns 1 when the name lacks a leading dot, 0 for every dot name
 */
static int deletecurdir_ea_osx_chkifempty_loop(const struct vol *vol,
                                               struct dirent *de, char *name, void *data _U_, int flag _U_)
{
    if (de->d_name[0] != '.' || de->d_name[0] == '_') {
        return 1;
    }

    return 0;
}

/*!
 * @brief Delete name when the path starts with ._ and names a regular file
 *
 * @param[in] vol   unused
 * @param[in] de    unused
 * @param[in] name  path of the entry, including the prefix of the walked
 *                  directory
 * @param[in] data  unused
 * @param[in] flag  unused
 * @returns 0, -1 when lstat failed, or the netatalk_unlink error
 */
static int deletecurdir_ea_osx_loop(const struct vol *vol _U_,
                                    struct dirent *de _U_, char *name, void *data _U_, int flag _U_)
{
    int ret;
    struct stat sb;

    if (strncmp(name, "._", strlen("._")) == 0) {
        if (lstat(name, &sb) != 0) {
            return -1;
        }

        if (S_ISREG(sb.st_mode))
            if ((ret = netatalk_unlink(name)) != 0) {
                return ret;
            }
    }

    return 0;
}

/*!
 * @brief Run deletecurdir_ea_osx_loop over the working directory before afpd
 * deletes it
 *
 * Without HAVE_EAFD the walk hands every entry path to deletecurdir_ea_osx_loop
 * and stops at the first error. With HAVE_EAFD the resource forks live in EAs,
 * so the call does nothing.
 *
 * @param[in] vol  volume holding the directory
 * @returns 0, or the first error of the walk
 */
static int RF_deletecurdir_ea(const struct vol *vol)
{
#ifndef HAVE_EAFD
    int err;

    /* delete stray ._AppleDouble files */
    if ((err = for_each_adouble("deletecurdir_ea_osx", ".",
                                deletecurdir_ea_osx_loop,
                                vol, NULL, 0)) != 0) {
        return err;
    }

#endif
    return 0;
}

/*!
 * @brief Change nothing, because the ea layout keeps directory metadata in an
 * EA of the directory
 *
 * @param[in] vol   unused
 * @param[in] name  unused
 * @param[in] mode  unused
 * @param[in] st    unused
 * @returns 0
 */
static int RF_setdirunixmode_ea(const struct vol *vol _U_,
                                const char *name _U_, mode_t mode _U_,
                                struct stat *st _U_)
{
#ifndef HAVE_EAFD
#endif
    return 0;
}

/*!
 * @brief Apply the mode of a data file to its ._ resource fork sidecar
 *
 * ad_hf_mode derives the sidecar mode. With HAVE_EAFD the resource fork lives
 * in an EA, so nothing changes.
 *
 * @param[in] vol   volume holding the file
 * @param[in] name  data file
 * @param[in] mode  new data file mode
 * @param[in] st    stat of the data file, or NULL
 * @returns 0, or -1 with errno set
 */
static int RF_setfilmode_ea(const struct vol *vol, const char *name,
                            mode_t mode, struct stat *st)
{
#ifndef HAVE_EAFD
    return adouble_setfilmode(vol, vol->ad_path(name, ADFLAGS_HF), mode, st);
#else
    (void)vol;
    (void)name;
    (void)mode;
    (void)st;
    return 0;
#endif
}

/*!
 * @brief Change nothing, because the ea layout keeps directory metadata in an
 * EA of the directory
 *
 * @param[in] vol   unused
 * @param[in] name  unused
 * @param[in] mode  unused
 * @param[in] st    unused
 * @returns 0
 */
static int RF_setdirmode_ea(const struct vol *vol _U_, const char *name _U_,
                            mode_t mode _U_, struct stat *st _U_)
{
#ifndef HAVE_EAFD
#endif
    return 0;
}

/*!
 * @brief Change nothing, because the ea layout keeps directory metadata in an
 * EA of the directory
 *
 * @param[in] vol   unused
 * @param[in] name  unused
 * @param[in] uid   unused
 * @param[in] gid   unused
 * @returns 0
 */
static int RF_setdirowner_ea(const struct vol *vol _U_, const char *name _U_,
                             uid_t uid _U_, gid_t gid _U_)
{
#ifndef HAVE_EAFD
#endif
    return 0;
}

/*!
 * @brief Delete the ._ resource fork sidecar of file
 *
 * A missing sidecar counts as success. With HAVE_EAFD the resource fork lives
 * in an EA, so nothing needs deleting.
 *
 * @param[in] vol    volume holding the file
 * @param[in] dirfd  directory that file is relative to, or -1 for the working
 *                   directory
 * @param[in] file   data file
 * @returns AFP_OK, AFPERR_VLOCK on a read-only file system, AFPERR_ACCESS, or
 *          AFPERR_PARAM
 */
static int RF_deletefile_ea(const struct vol *vol, int dirfd, const char *file)
{
#ifndef HAVE_EAFD
    return netatalk_unlinkat(dirfd, vol->ad_path(file, ADFLAGS_HF));
#else
    (void)vol;
    (void)dirfd;
    (void)file;
    return 0;
#endif
}
/*!
 * @brief Clone the resource fork sidecar of src for dst
 *
 * The sidecar is ._name beside the file, or name/..namedfork/rsrc on macOS. A
 * source without a sidecar needs nothing and succeeds. On Solaris with
 * HAVE_EAFD the EA module transfers the resource fork, so nothing happens here.
 *
 * @param[in] vol  unused
 * @param[in] sfd  directory that src is relative to, or -1 for the working
 *                 directory
 * @param[in] src  source file
 * @param[in] dst  destination file
 * @returns 0, or -1 with errno set
 */
static int RF_copyfile_ea(const struct vol *vol _U_, int sfd, const char *src,
                          const char *dst)
{
#if defined (HAVE_EAFD) && defined (SOLARIS)
    /* the EA VFS module does this all for us */
    return 0;
#endif
    EC_INIT;
    bstring s = NULL;
    bstring d = NULL;
    char src_name[MAXPATHLEN + 1];
    char dst_name[MAXPATHLEN + 1];
    char src_dir[MAXPATHLEN + 1];
    char dst_dir[MAXPATHLEN + 1];
    const char *name = NULL;
    const char *dir = NULL;
    /* get basename */
    /* build src path to ._ file*/
    strlcpy(src_name, src, sizeof(src_name));
    strlcpy(src_dir, src, sizeof(src_dir));
    EC_NULL(name = basename(src_name));
    EC_NULL(dir = dirname(src_dir));
    EC_NULL(s = bfromcstr(dir));
#ifdef __APPLE__
    EC_ZERO(bcatcstr(s, "/"));
    EC_ZERO(bcatcstr(s, name));
    EC_ZERO(bcatcstr(s, "/..namedfork/rsrc"));
#else
    EC_ZERO(bcatcstr(s, "/._"));
    EC_ZERO(bcatcstr(s, name));
#endif
    /* build dst path to ._file*/
    strlcpy(dst_name, dst, sizeof(dst_name));
    strlcpy(dst_dir, dst, sizeof(dst_dir));
    EC_NULL(name = basename(dst_name));
    EC_NULL(dir = dirname(dst_dir));
    EC_NULL(d = bfromcstr(dir));
#ifdef __APPLE__
    EC_ZERO(bcatcstr(d, "/"));
    EC_ZERO(bcatcstr(d, name));
    EC_ZERO(bcatcstr(d, "/..namedfork/rsrc"));
#else
    EC_ZERO(bcatcstr(d, "/._"));
    EC_ZERO(bcatcstr(d, name));
#endif

    if (copy_file(sfd, cfrombstr(s), cfrombstr(d), 0666) != 0) {
        switch (errno) {
        case ENOENT:
            break;

        default:
            LOG(log_error, logtype_afpd, "[VFS] copyfile(\"%s\" -> \"%s\"): %s",
                cfrombstr(s), cfrombstr(d), strerror(errno));
            EC_FAIL;
        }
    }

EC_CLEANUP:
    bdestroy(s);
    bdestroy(d);
    EC_EXIT;
}

/*!
 * @brief Rename the ._ resource fork sidecar of src to match dst
 *
 * src is relative to dirfd, and dst is relative to the working directory. A
 * source without a sidecar needs nothing and succeeds. With HAVE_EAFD the
 * resource fork lives in an EA and moves with the file.
 *
 * @param[in] vol    volume holding the file
 * @param[in] dirfd  directory that src is relative to, or -1 for the working
 *                   directory
 * @param[in] src    current file name
 * @param[in] dst    new file name
 * @returns 0, or -1 with errno set
 */
static int RF_renamefile_ea(const struct vol *vol, int dirfd, const char *src,
                            const char *dst)
{
#ifndef HAVE_EAFD
    char  adsrc[MAXPATHLEN + 1];
    int   err = 0;
    strcpy(adsrc, vol->ad_path(src, 0));

    if (unix_rename(dirfd, adsrc, -1, vol->ad_path(dst, 0)) < 0) {
        struct stat st;
        err = errno;

        /* source has no resource fork */
        if (errno == ENOENT && ostatat(dirfd, adsrc, &st, vol_syml_opt(vol))) {
            return 0;
        }

        errno = err;
        return -1;
    }

    return 0;
#else
    (void)vol;
    (void)dirfd;
    (void)src;
    (void)dst;
    return 0;
#endif
}

/********************************************************************************************
 * VFS chaining
 ********************************************************************************************/

/*
 * Up until we really start stacking many VFS modules on top of one another or use
 * dynamic module loading like we do for UAMs, up until then we just stack VFS modules
 * via an fixed size array.
 * All VFS funcs must return AFP_ERR codes. When a func in the chain returns an error
 * this error code will be returned to the caller, BUT the chain in followed and all
 * following funcs are called in order to give them a chance.
 */

/*
 * Most VFS funcs have similar logic.
 * Only "ad_path" and "validupath" will NOT do stacking and only
 * call the func from the first module.
 */

/*!
 * @brief Run the vfs_chown hook of every VFS module of vol
 *
 * The modules run in order: AppleDouble layout, EA backend, and ACL support.
 * Every module runs even after a failure.
 *
 * @param[in] vol   volume
 * @param[in] path  file whose metadata to change
 * @param[in] uid   new owner
 * @param[in] gid   new group
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_chown(const struct vol *vol, const char *path, uid_t uid,
                     gid_t gid)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_chown) {
            int curr_ret = vol->vfs_modules[i]->vfs_chown(vol, path, uid, gid);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_renamedir hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol      volume
 * @param[in] dirfd    directory that oldpath is relative to, or -1 for the
 *                     working directory
 * @param[in] oldpath  current directory name
 * @param[in] newpath  new directory name
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_renamedir(const struct vol *vol, int dirfd, const char *oldpath,
                         const char *newpath)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_renamedir) {
            int curr_ret = vol->vfs_modules[i]->vfs_renamedir(vol, dirfd, oldpath, newpath);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_deletecurdir hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol  volume whose working directory afpd is about to delete
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_deletecurdir(const struct vol *vol)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_deletecurdir) {
            int curr_ret = vol->vfs_modules[i]->vfs_deletecurdir(vol);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_setfilmode hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol   volume
 * @param[in] name  data file
 * @param[in] mode  new data file mode
 * @param[in] st    stat of the data file, or NULL
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_setfilmode(const struct vol *vol, const char *name, mode_t mode,
                          struct stat *st)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_setfilmode) {
            int curr_ret = vol->vfs_modules[i]->vfs_setfilmode(vol, name, mode, st);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_setdirmode hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol   volume
 * @param[in] name  directory
 * @param[in] mode  new directory mode
 * @param[in] st    stat of the directory, or NULL
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_setdirmode(const struct vol *vol, const char *name, mode_t mode,
                          struct stat *st)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_setdirmode) {
            int curr_ret = vol->vfs_modules[i]->vfs_setdirmode(vol, name, mode, st);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_setdirunixmode hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol   volume
 * @param[in] name  directory
 * @param[in] mode  new directory mode
 * @param[in] st    stat of the directory, or NULL
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_setdirunixmode(const struct vol *vol, const char *name,
                              mode_t mode, struct stat *st)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_setdirunixmode) {
            int curr_ret = vol->vfs_modules[i]->vfs_setdirunixmode(vol, name, mode, st);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_setdirowner hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol   volume
 * @param[in] name  directory
 * @param[in] uid   new owner
 * @param[in] gid   new group
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_setdirowner(const struct vol *vol, const char *name, uid_t uid,
                           gid_t gid)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_setdirowner) {
            int curr_ret = vol->vfs_modules[i]->vfs_setdirowner(vol, name, uid, gid);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_deletefile hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol    volume
 * @param[in] dirfd  directory that file is relative to, or -1 for the working
 *                   directory
 * @param[in] file   data file whose metadata to delete
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_deletefile(const struct vol *vol, int dirfd, const char *file)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_deletefile) {
            int curr_ret = vol->vfs_modules[i]->vfs_deletefile(vol, dirfd, file);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_renamefile hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol    volume
 * @param[in] dirfd  directory that src is relative to, or -1 for the working
 *                   directory
 * @param[in] src    current file name
 * @param[in] dst    new file name, relative to the working directory
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_renamefile(const struct vol *vol, int dirfd, const char *src,
                          const char *dst)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_renamefile) {
            int curr_ret = vol->vfs_modules[i]->vfs_renamefile(vol, dirfd, src, dst);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_copyfile hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol  volume
 * @param[in] sfd  directory that src is relative to, or -1 for the working
 *                 directory
 * @param[in] src  source file
 * @param[in] dst  destination file
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_copyfile(const struct vol *vol, int sfd, const char *src,
                        const char *dst)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_copyfile) {
            int curr_ret = vol->vfs_modules[i]->vfs_copyfile(vol, sfd, src, dst);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

#ifdef HAVE_ACLS
#ifdef HAVE_NFSV4_ACLS
/*!
 * @brief Run the vfs_solaris_acl hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol    volume
 * @param[in] path   file or directory
 * @param[in] cmd    acl command
 * @param[in] count  number of entries in aces
 * @param[in] aces   NFSv4 access control entries
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_solaris_acl(const struct vol *vol, const char *path, int cmd,
                           int count, void *aces)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_solaris_acl) {
            int curr_ret = vol->vfs_modules[i]->vfs_solaris_acl(vol, path, cmd, count,
                                                                aces);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}
#endif /* HAVE_NFSV4_ACLS */
#ifdef HAVE_POSIX_ACLS
/*!
 * @brief Run the vfs_posix_acl hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol    volume
 * @param[in] path   file or directory
 * @param[in] type   ACL_TYPE_ACCESS or ACL_TYPE_DEFAULT
 * @param[in] count  number of entries in acl
 * @param[in] acl    POSIX ACL to set
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_posix_acl(const struct vol *vol, const char *path,
                         acl_type_t type, int count, acl_t acl)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_posix_acl) {
            int curr_ret = vol->vfs_modules[i]->vfs_posix_acl(vol, path, type, count, acl);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}
#endif /* HAVE_POSIX_ACLS */

/*!
 * @brief Run the vfs_remove_acl hook of every VFS module of vol
 *
 * Every module runs even after a failure.
 *
 * @param[in] vol   volume
 * @param[in] path  file or directory
 * @param[in] dir   nonzero when path is a directory
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_remove_acl(const struct vol *vol, const char *path, int dir)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_remove_acl) {
            int curr_ret = vol->vfs_modules[i]->vfs_remove_acl(vol, path, dir);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}
#endif /* HAVE_ACLS */

/*!
 * @brief Reject a missing EA name with a log line that names the calling
 * function
 *
 * @param[in] func       name of the calling function, for the log line
 * @param[in] attruname  EA name to test
 * @returns 1 when attruname is NULL, 0 otherwise
 */
static int vfs_ea_attrname_invalid(const char *func, const char *attruname)
{
    if (attruname == NULL) {
        LOG(log_error, logtype_afpd, "%s: missing EA name", func);
        return 1;
    }

    return 0;
}

/*!
 * @brief Run the vfs_ea_getsize hook of every VFS module of vol
 *
 * A missing EA name fails before any module runs. Only the EA backend module
 * implements the hook, and it appends the EA size to rbuf.
 *
 * @param[in]     vol        volume
 * @param[out]    rbuf       reply buffer receiving the size
 * @param[in,out] rbuflen    reply length, advanced by the bytes appended
 * @param[in]     uname      file name
 * @param[in]     oflag      O_NOFOLLOW to act on a symlink itself
 * @param[in]     attruname  EA name
 * @param[in]     fd         open descriptor of the file, or -1 to use uname
 * @returns AFP_OK, AFPERR_PARAM for a missing EA name, or the result of the
 *          last module that failed
 */
static int vfs_ea_getsize(const struct vol *vol, char *rbuf,
                          size_t *rbuflen, const char *uname, int oflag,
                          const char *attruname, int fd)
{
    int ret = AFP_OK;

    if (vfs_ea_attrname_invalid("vfs_ea_getsize", attruname)) {
        return AFPERR_PARAM;
    }

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_ea_getsize) {
            int curr_ret = vol->vfs_modules[i]->vfs_ea_getsize(vol, rbuf, rbuflen, uname,
                                                               oflag, attruname, fd);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_ea_getcontent hook of every VFS module of vol
 *
 * A missing EA name fails before any module runs. Only the EA backend module
 * implements the hook, and it appends the EA content to rbuf.
 *
 * @param[in]     vol        volume
 * @param[out]    rbuf       reply buffer receiving the content
 * @param[in,out] rbuflen    reply length, advanced by the bytes appended
 * @param[in]     uname      file name
 * @param[in]     oflag      O_NOFOLLOW to act on a symlink itself
 * @param[in]     attruname  EA name
 * @param[in]     maxreply   reply size limit for the EA content
 * @param[in]     fd         open descriptor of the file, or -1 to use uname
 * @returns AFP_OK, AFPERR_PARAM for a missing EA name, or the result of the
 *          last module that failed
 */
static int vfs_ea_getcontent(const struct vol *vol, char *rbuf, size_t *rbuflen,
                             const char *uname, int oflag, const char *attruname,
                             int maxreply, int fd)
{
    int ret = AFP_OK;

    if (vfs_ea_attrname_invalid("vfs_ea_getcontent", attruname)) {
        return AFPERR_PARAM;
    }

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_ea_getcontent) {
            int curr_ret = vol->vfs_modules[i]->vfs_ea_getcontent(vol, rbuf, rbuflen, uname,
                                                                  oflag, attruname, maxreply, fd);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_ea_list hook of every VFS module of vol
 *
 * Only the EA backend module implements the hook, and it appends the EA names
 * as consecutive C strings.
 *
 * @param[in]     vol          volume
 * @param[out]    attrnamebuf  buffer receiving the names
 * @param[in,out] buflen       bytes in attrnamebuf, advanced by the names
 *                             appended
 * @param[in]     uname        file name
 * @param[in]     oflag        O_NOFOLLOW to act on a symlink itself
 * @param[in]     fd           open descriptor of the file, or -1 to use uname
 * @returns AFP_OK, or the result of the last module that failed
 */
static int vfs_ea_list(const struct vol *vol,
                       char *attrnamebuf, size_t *buflen, const char *uname,
                       int oflag, int fd)
{
    int ret = AFP_OK;

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_ea_list) {
            int curr_ret = vol->vfs_modules[i]->vfs_ea_list(vol, attrnamebuf, buflen, uname,
                                                            oflag, fd);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_ea_set hook of every VFS module of vol
 *
 * A missing EA name fails before any module runs. Only the EA backend module
 * implements the hook.
 *
 * @param[in] vol        volume
 * @param[in] uname      file name
 * @param[in] attruname  EA name
 * @param[in] ibuf       EA content
 * @param[in] attrsize   bytes in ibuf
 * @param[in] oflag      O_CREAT to refuse an existing EA, O_TRUNC to require
 *                       one, O_NOFOLLOW to act on a symlink itself
 * @param[in] fd         open descriptor of the file, or -1 to use uname
 * @returns AFP_OK, AFPERR_PARAM for a missing EA name, or the result of the
 *          last module that failed
 */
static int vfs_ea_set(const struct vol *vol, const char *uname,
                      const char *attruname, const char *ibuf, size_t attrsize,
                      int oflag, int fd)
{
    int ret = AFP_OK;

    if (vfs_ea_attrname_invalid("vfs_ea_set", attruname)) {
        return AFPERR_PARAM;
    }

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_ea_set) {
            int curr_ret = vol->vfs_modules[i]->vfs_ea_set(vol, uname, attruname, ibuf,
                                                           attrsize, oflag, fd);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Run the vfs_ea_remove hook of every VFS module of vol
 *
 * A missing EA name fails before any module runs. Only the EA backend module
 * implements the hook.
 *
 * @param[in] vol        volume
 * @param[in] uname      file name
 * @param[in] attruname  EA name
 * @param[in] oflag      O_NOFOLLOW to act on a symlink itself
 * @param[in] fd         open descriptor of the file, or -1 to use uname
 * @returns AFP_OK, AFPERR_PARAM for a missing EA name, or the result of the
 *          last module that failed
 */
static int vfs_ea_remove(const struct vol *vol, const char *uname,
                         const char *attruname, int oflag, int fd)
{
    int ret = AFP_OK;

    if (vfs_ea_attrname_invalid("vfs_ea_remove", attruname)) {
        return AFPERR_PARAM;
    }

    for (int i = 0; i < VFS_MODULES_MAX; i++) {
        if (vol->vfs_modules[i] && vol->vfs_modules[i]->vfs_ea_remove) {
            int curr_ret = vol->vfs_modules[i]->vfs_ea_remove(vol, uname, attruname, oflag,
                                                              fd);

            if (curr_ret != AFP_OK) {
                ret = curr_ret;
            }
        }
    }

    return ret;
}

/*!
 * @brief Whether clients may see name, as the AppleDouble layout module decides
 *
 * Only the first module answers, because the layout alone knows which names
 * hold its metadata.
 *
 * @param[in] vol   volume
 * @param[in] name  directory entry name
 * @returns nonzero when clients may see name, 0 otherwise
 */
static int vfs_validupath(const struct vol *vol, const char *name)
{
    return vol->vfs_modules[0]->vfs_validupath(vol, name);
}

/*!
 * These function pointers get called from the lib users via vol->vfs->func.
 */
static struct vfs_ops vfs_master_funcs = {
    vfs_validupath,
    vfs_chown,
    vfs_renamedir,
    vfs_deletecurdir,
    vfs_setfilmode,
    vfs_setdirmode,
    vfs_setdirunixmode,
    vfs_setdirowner,
    vfs_deletefile,
    vfs_renamefile,
    vfs_copyfile,
#ifdef HAVE_ACLS
#ifdef HAVE_NFSV4_ACLS
    vfs_solaris_acl,
#endif
#ifdef HAVE_POSIX_ACLS
    vfs_posix_acl,
#endif
    vfs_remove_acl,
#endif
    vfs_ea_getsize,
    vfs_ea_getcontent,
    vfs_ea_list,
    vfs_ea_set,
    vfs_ea_remove
};

/*
 * Primary adouble modules: v2, ea
 */

static struct vfs_ops netatalk_adouble_v2 = {
    .vfs_validupath = validupath_adouble,
    .vfs_chown = RF_chown_adouble,
    .vfs_renamedir = RF_renamedir_adouble,
    .vfs_deletecurdir = RF_deletecurdir_adouble,
    .vfs_setfilmode = RF_setfilmode_adouble,
    .vfs_setdirmode = RF_setdirmode_adouble,
    .vfs_setdirunixmode = RF_setdirunixmode_adouble,
    .vfs_setdirowner = RF_setdirowner_adouble,
    .vfs_deletefile = RF_deletefile_adouble,
    .vfs_renamefile = RF_renamefile_adouble,
    .vfs_copyfile = RF_copyfile_adouble,
#ifdef HAVE_ACLS
#ifdef HAVE_NFSV4_ACLS
    .vfs_solaris_acl = NULL,
#endif
#ifdef HAVE_POSIX_ACLS
    .vfs_posix_acl = NULL,
#endif
    .vfs_remove_acl = NULL,
#endif
    .vfs_ea_getsize = NULL,
    .vfs_ea_getcontent = NULL,
    .vfs_ea_list = NULL,
    .vfs_ea_set = NULL,
    .vfs_ea_remove = NULL
};

static struct vfs_ops netatalk_adouble_ea = {
    .vfs_validupath = validupath_ea,
    .vfs_chown = RF_chown_ea,
    .vfs_renamedir = RF_renamedir_ea,
    .vfs_deletecurdir = RF_deletecurdir_ea,
    .vfs_setfilmode = RF_setfilmode_ea,
    .vfs_setdirmode = RF_setdirmode_ea,
    .vfs_setdirunixmode = RF_setdirunixmode_ea,
    .vfs_setdirowner = RF_setdirowner_ea,
    .vfs_deletefile = RF_deletefile_ea,
    .vfs_renamefile = RF_renamefile_ea,
    .vfs_copyfile = RF_copyfile_ea,
#ifdef HAVE_ACLS
#ifdef HAVE_NFSV4_ACLS
    .vfs_solaris_acl = NULL,
#endif
#ifdef HAVE_POSIX_ACLS
    .vfs_posix_acl = NULL,
#endif
    .vfs_remove_acl = NULL,
#endif
    .vfs_ea_getsize = NULL,
    .vfs_ea_getcontent = NULL,
    .vfs_ea_list = NULL,
    .vfs_ea_set = NULL,
    .vfs_ea_remove = NULL
};

/*
 * Secondary vfs modules for Extended Attributes
 */

static struct vfs_ops netatalk_ea_adouble = {
    .vfs_validupath = NULL,
    .vfs_chown = ea_chown,
    .vfs_renamedir = NULL,
    .vfs_deletecurdir = NULL,
    .vfs_setfilmode = ea_chmod_file,
    .vfs_setdirmode = NULL,
    .vfs_setdirunixmode = ea_chmod_dir,
    .vfs_setdirowner = NULL,
    .vfs_deletefile = ea_deletefile,
    .vfs_renamefile = ea_renamefile,
    .vfs_copyfile = ea_copyfile,
#ifdef HAVE_ACLS
#ifdef HAVE_NFSV4_ACLS
    .vfs_solaris_acl = NULL,
#endif
#ifdef HAVE_POSIX_ACLS
    .vfs_posix_acl = NULL,
#endif
    .vfs_remove_acl = NULL,
#endif
    .vfs_ea_getsize = get_easize,
    .vfs_ea_getcontent = get_eacontent,
    .vfs_ea_list = list_eas,
    .vfs_ea_set = set_ea,
    .vfs_ea_remove = remove_ea
};

static struct vfs_ops netatalk_ea_sys = {
    .vfs_validupath = NULL,
    .vfs_chown = NULL,
    .vfs_renamedir = NULL,
    .vfs_deletecurdir = NULL,
    .vfs_setfilmode = NULL,
    .vfs_setdirmode = NULL,
    .vfs_setdirunixmode = NULL,
    .vfs_setdirowner = NULL,
    .vfs_deletefile = NULL,
    .vfs_renamefile = NULL,
    .vfs_copyfile = sys_ea_copyfile,
#ifdef HAVE_ACLS
#ifdef HAVE_NFSV4_ACLS
    .vfs_solaris_acl = NULL,
#endif
#ifdef HAVE_POSIX_ACLS
    .vfs_posix_acl = NULL,
#endif
    .vfs_remove_acl = NULL,
#endif
    .vfs_ea_getsize = sys_get_easize,
    .vfs_ea_getcontent = sys_get_eacontent,
    .vfs_ea_list = sys_list_eas,
    .vfs_ea_set = sys_set_ea,
    .vfs_ea_remove = sys_remove_ea
};

/*
 * Tertiary VFS modules for ACLs
 */

#ifdef HAVE_NFSV4_ACLS
static struct vfs_ops netatalk_solaris_acl_adouble = {
    .vfs_validupath = NULL,
    .vfs_chown = NULL,
    .vfs_renamedir = NULL,
    .vfs_deletecurdir = NULL,
    .vfs_setfilmode = NULL,
    .vfs_setdirmode = NULL,
    .vfs_setdirunixmode = NULL,
    .vfs_setdirowner = NULL,
    .vfs_deletefile = NULL,
    .vfs_renamefile = NULL,
    .vfs_copyfile = NULL,
    .vfs_solaris_acl = RF_solaris_acl,
    .vfs_remove_acl = RF_solaris_remove_acl,
    .vfs_ea_getsize = NULL,
    .vfs_ea_getcontent = NULL,
    .vfs_ea_list = NULL,
    .vfs_ea_set = NULL,
    .vfs_ea_remove = NULL
};
#endif

#ifdef HAVE_POSIX_ACLS
static struct vfs_ops netatalk_posix_acl_adouble = {
    .vfs_validupath = NULL,
    .vfs_chown = NULL,
    .vfs_renamedir = NULL,
    .vfs_deletecurdir = NULL,
    .vfs_setfilmode = NULL,
    .vfs_setdirmode = NULL,
    .vfs_setdirunixmode = NULL,
    .vfs_setdirowner = NULL,
    .vfs_deletefile = NULL,
    .vfs_renamefile = NULL,
    .vfs_copyfile = NULL,
    .vfs_posix_acl = RF_posix_acl,
    .vfs_remove_acl = RF_posix_remove_acl,
    .vfs_ea_getsize = NULL,
    .vfs_ea_getcontent = NULL,
    .vfs_ea_list = NULL,
    .vfs_ea_set = NULL,
    .vfs_ea_remove = NULL
};
#endif

/*!
 * @brief Install the VFS dispatchers and modules of a volume
 *
 * Module 0 is the AppleDouble layout, version 2 or ea, together with its
 * ad_path function. The EA backend that v_vfs_ea selects, native EAs or
 * AppleDouble EA files, fills slot 1 and leaves it empty without EA support.
 * A build with NFSv4 or POSIX ACLs puts its ACL module in slot 2.
 *
 * @param[in,out] vol  volume to set up
 */
void initvol_vfs(struct vol *vol)
{
    vol->vfs = &vfs_master_funcs;

    /* Default adouble stuff */
    if (vol->v_adouble == AD_VERSION2) {
        vol->vfs_modules[0] = &netatalk_adouble_v2;
        vol->ad_path = ad_path;
    } else {
        vol->vfs_modules[0] = &netatalk_adouble_ea;
#if defined(HAVE_EAFD) && defined(SOLARIS)
        vol->ad_path = ad_path_ea;
#else
        vol->ad_path = ad_path_osx;
#endif
    }

    /* Extended Attributes */
    if (vol->v_vfs_ea == AFPVOL_EA_SYS) {
        LOG(log_debug, logtype_afpd,
            "initvol_vfs: enabling EA support with native EAs");
        vol->vfs_modules[1] = &netatalk_ea_sys;
    } else if (vol->v_vfs_ea == AFPVOL_EA_AD) {
        LOG(log_debug, logtype_afpd,
            "initvol_vfs: enabling EA support with adouble files");
        vol->vfs_modules[1] = &netatalk_ea_adouble;
    } else {
        LOG(log_debug, logtype_afpd, "initvol_vfs: volume without EA support");
    }

    /* ACLs */
#ifdef HAVE_NFSV4_ACLS
    vol->vfs_modules[2] = &netatalk_solaris_acl_adouble;
#endif
#ifdef HAVE_POSIX_ACLS
    vol->vfs_modules[2] = &netatalk_posix_acl_adouble;
#endif
}
