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

#include <bstrlib.h>

#include <atalk/acl.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
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

/* ----------------------------- */
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

static int netatalk_name(const char *name)
{
    return strcmp(name, ".AppleDB") && strcmp(name, ".AppleDesktop");
}

/*******************************************************************************
 * classic adouble format
 *******************************************************************************/

static int validupath_adouble(const struct vol *vol, const char *name)
{
    if (name[0] != '.') {
        return 1;
    }

    return netatalk_name(name) && strcmp(name, ".AppleDouble")
           && strcasecmp(name, ".Parent");
}

/* ----------------- */
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

/* ----------------- */
static int RF_renamedir_adouble(const struct vol *vol, int dirfd,
                                const char *oldpath, const char *newpath)
{
    return 0;
}

/* ----------------- */
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

/* ----------------- */
static int adouble_setfilmode(const struct vol *vol, const char *name,
                              mode_t mode, struct stat *st)
{
    return setfilmode(vol, name, ad_hf_mode(mode), st);
}

static int RF_setfilmode_adouble(const struct vol *vol, const char *name,
                                 mode_t mode, struct stat *st)
{
    return adouble_setfilmode(vol, vol->ad_path(name, ADFLAGS_HF), mode, st);
}

/* ----------------- */
static int RF_setdirunixmode_adouble(const struct vol *vol, const char *name,
                                     mode_t mode, struct stat *st)
{
    const char *adouble = vol->ad_path(name, ADFLAGS_DIR);

    if (dir_rx_set(mode)) {
        if (ochmod(ad_dir(adouble),
                   (DIRBITS | mode) & ~vol->v_umask,
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
                   (DIRBITS | mode) & ~vol->v_umask,
                   st,
                   vol_syml_opt(vol) | vol_chmod_opt(vol)
                  ) < 0) {
            return  -1 ;
        }
    }

    return 0;
}

/* ----------------- */
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

static int RF_setdirmode_adouble(const struct vol *vol, const char *name,
                                 mode_t mode, struct stat *st)
{
    mode_t hf_mode = ad_hf_mode(mode);
    const char  *adouble = vol->ad_path(name, ADFLAGS_DIR);
    const char  *adouble_p = ad_dir(adouble);

    if (dir_rx_set(mode)) {
        if (ochmod(ad_dir(adouble),
                   (DIRBITS | mode) & ~vol->v_umask,
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
                   (DIRBITS | mode) & ~vol->v_umask,
                   st,
                   vol_syml_opt(vol) | vol_chmod_opt(vol)
                  ) < 0) {
            return  -1 ;
        }
    }

    return 0;
}

static int RF_setdirowner_adouble(const struct vol *vol, const char *name,
                                  uid_t uid, gid_t gid)
{
    if (lchown(".AppleDouble", uid, gid) < 0 && errno != EPERM) {
        LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d %s: %s",
            uid, gid, fullpathname(".AppleDouble"), strerror(errno));
    }

    return 0;
}

/* ----------------- */
static int RF_deletefile_adouble(const struct vol *vol, int dirfd,
                                 const char *file)
{
    return netatalk_unlinkat(dirfd, vol->ad_path(file, ADFLAGS_HF));
}

/* ----------------- */
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

static int RF_copyfile_adouble(const struct vol *vol, int sfd, const char *src,
                               const char *dst)
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
static int RF_posix_acl(const struct vol *vol, const char *path,
                        acl_type_t type, int count, acl_t acl)
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
    return netatalk_name(name);
}

/* ----------------- */
static int RF_chown_ea(const struct vol *vol, const char *path, uid_t uid,
                       gid_t gid)
{
#ifndef HAVE_EAFD
    return chown(vol->ad_path(path, ADFLAGS_HF), uid, gid);
#endif
    return 0;
}

/* ---------------- */
static int RF_renamedir_ea(const struct vol *vol, int dirfd,
                           const char *oldpath, const char *newpath)
{
    return 0;
}

/* Returns 1 if the entry is NOT an ._ file */
static int deletecurdir_ea_osx_chkifempty_loop(const struct vol *vol,
        struct dirent *de, char *name, void *data _U_, int flag _U_)
{
    if (de->d_name[0] != '.' || de->d_name[0] == '_') {
        return 1;
    }

    return 0;
}

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

/* ---------------- */
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

/* ---------------- */
static int RF_setdirunixmode_ea(const struct vol *vol, const char *name,
                                mode_t mode, struct stat *st)
{
#ifndef HAVE_EAFD
#endif
    return 0;
}

static int RF_setfilmode_ea(const struct vol *vol, const char *name,
                            mode_t mode, struct stat *st)
{
#ifndef HAVE_EAFD
    return adouble_setfilmode(vol, vol->ad_path(name, ADFLAGS_HF), mode, st);
#endif
    return 0;
}

/* ---------------- */
static int RF_setdirmode_ea(const struct vol *vol, const char *name,
                            mode_t mode, struct stat *st)
{
#ifndef HAVE_EAFD
#endif
    return 0;
}

/* ---------------- */
static int RF_setdirowner_ea(const struct vol *vol, const char *name, uid_t uid,
                             gid_t gid)
{
#ifndef HAVE_EAFD
#endif
    return 0;
}

static int RF_deletefile_ea(const struct vol *vol, int dirfd, const char *file)
{
#ifndef HAVE_EAFD
    return netatalk_unlinkat(dirfd, vol->ad_path(file, ADFLAGS_HF));
#endif
    return 0;
}
static int RF_copyfile_ea(const struct vol *vol, int sfd, const char *src,
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

/* ---------------- */
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
#endif
    return 0;
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

static int vfs_ea_getsize(const struct vol *vol, char *rbuf,
                          size_t *rbuflen, const char *uname, int oflag,
                          const char *attruname, int fd)
{
    int ret = AFP_OK;

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

static int vfs_ea_getcontent(const struct vol *vol, char *rbuf, size_t *rbuflen,
                             const char *uname, int oflag, const char *attruname,
                             int maxreply, int fd)
{
    int ret = AFP_OK;

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

static int vfs_ea_set(const struct vol *vol, const char *uname,
                      const char *attruname, const char *ibuf, size_t attrsize,
                      int oflag, int fd)
{
    int ret = AFP_OK;

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

static int vfs_ea_remove(const struct vol *vol, const char *uname,
                         const char *attruname, int oflag, int fd)
{
    int ret = AFP_OK;

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

static int vfs_validupath(const struct vol *vol, const char *name)
{
    return vol->vfs_modules[0]->vfs_validupath(vol, name);
}

/*
 * These function pointers get called from the lib users via vol->vfs->func.
 * These funcs are defined via the macros above.
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

/* ---------------- */
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
