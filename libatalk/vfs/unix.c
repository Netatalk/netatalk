/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <atalk/acl.h>
#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/directory.h>
#include <atalk/ea.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/volume.h>

/*!
 * @brief Whether mode grants the owner read and search permission
 *
 * @param[in] mode  directory mode
 * @returns 1 when mode carries both S_IRUSR and S_IXUSR, 0 otherwise
 */
int dir_rx_set(mode_t mode)
{
    return (mode & (S_IXUSR | S_IRUSR)) == (S_IXUSR | S_IRUSR);
}

/*!
 * @brief Set the permission bits of name and keep its other mode bits
 *
 * The volume umask clears bits from the new mode, and st supplies the file
 * type and special bits. The ochmod call skips a symlink unless the volume
 * follows symlinks. The volume options decide whether it preserves ACLs or
 * skips the change. EPERM counts as success.
 *
 * @param[in] vol   volume holding the file
 * @param[in] name  file to change
 * @param[in] mode  new permission bits
 * @param[in] st    stat whose non-permission bits to keep, or NULL to stat name
 * @returns 0, or -1 with errno set
 */
int setfilmode(const struct vol *vol, const char *name, mode_t mode,
               struct stat *st)
{
    struct stat sb;
    /* rwx for owner group and other, by default */
    mode_t mask = S_IRWXU | S_IRWXG | S_IRWXO;

    if (!st) {
        if (lstat(name, &sb) != 0) {
            return -1;
        }

        st = &sb;
    }

    /* keep other bits from previous mode */
    mode |= st->st_mode & ~mask;

    if (ochmod((char *)name,
               mode & ~vol_umask(vol),
               st,
               vol_syml_opt(vol) | vol_chmod_opt(vol)
              ) < 0 && errno != EPERM) {
        return -1;
    }

    return 0;
}

/*!
 * @brief Delete an empty directory and map the error to an AFP result code
 *
 * Unlike netatalk_rmdir, a missing directory fails with AFPERR_NOOBJ.
 *
 * @param[in] dirfd  directory that name is relative to, or -1 for the working
 *                   directory
 * @param[in] name   directory to delete
 * @returns AFP_OK, AFPERR_NOOBJ, AFPERR_DIRNEMPT, AFPERR_ACCESS, AFPERR_VLOCK,
 *          or AFPERR_PARAM
 */
int netatalk_rmdir_all_errors(int dirfd _U_, const char *name)
{
    int err;

    if (dirfd == -1) {
        dirfd = AT_FDCWD;
    }

    err = unlinkat(dirfd, name, AT_REMOVEDIR);

    if (err < 0) {
        switch (errno) {
        case ENOENT :
            return AFPERR_NOOBJ;

        case ENOTEMPTY :
        case EEXIST:
            return AFPERR_DIRNEMPT;

        case EPERM:
        case EACCES :
            return AFPERR_ACCESS;

        case EROFS:
            return AFPERR_VLOCK;

        default :
            return AFPERR_PARAM;
        }
    }

    return AFP_OK;
}

/*!
 * @brief Delete an empty directory and treat a missing one as success
 *
 * The errors map as in netatalk_rmdir_all_errors, except that AFPERR_NOOBJ
 * becomes AFP_OK.
 *
 * @param[in] dirfd  directory that name is relative to, or -1 for the working
 *                   directory
 * @param[in] name   directory to delete
 * @returns AFP_OK, AFPERR_DIRNEMPT, AFPERR_ACCESS, AFPERR_VLOCK, or
 *          AFPERR_PARAM
 */
int netatalk_rmdir(int dirfd, const char *name)
{
    int ret = netatalk_rmdir_all_errors(dirfd, name);

    if (ret == AFPERR_NOOBJ) {
        return AFP_OK;
    }

    return ret;
}

/*!
 * @brief Delete a file and map the error to an AFP result code
 *
 * A missing file counts as success.
 *
 * @param[in] name  file to delete, relative to the working directory
 * @returns AFP_OK, AFPERR_VLOCK on a read-only file system, AFPERR_ACCESS, or
 *          AFPERR_PARAM
 */
int netatalk_unlink(const char *name)
{
    if (unlink(name) < 0) {
        switch (errno) {
        case ENOENT :
            break;

        case EROFS:
            return AFPERR_VLOCK;

        case EPERM:
        case EACCES :
            return AFPERR_ACCESS;

        default :
            return AFPERR_PARAM;
        }
    }

    return AFP_OK;
}

/**************************************************************************
 * *at semnatics support functions (like openat, renameat standard funcs)
 **************************************************************************/

/*!
 * @brief Transfer all remaining data from one descriptor to another
 *
 * The loop reads and writes in NETATALK_DIOSZ_STACK chunks and retries after
 * EINTR.
 *
 * @param[in] sfd  source descriptor, read from its current position
 * @param[in] dfd  destination descriptor, written at its current position
 * @returns 0, or -1 when a read or write failed
 */
int copy_file_fd(int sfd, int dfd)
{
    EC_INIT;
    ssize_t cc;
    size_t  buflen;
    char   filebuf[NETATALK_DIOSZ_STACK];

    while ((cc = read(sfd, filebuf, sizeof(filebuf)))) {
        if (cc < 0) {
            if (errno == EINTR) {
                continue;
            }

            LOG(log_error, logtype_afpd, "copy_file_fd: %s", strerror(errno));
            EC_FAIL;
        }

        buflen = cc;

        while (buflen > 0) {
            if ((cc = write(dfd, filebuf, buflen)) < 0) {
                if (errno == EINTR) {
                    continue;
                }

                LOG(log_error, logtype_afpd, "copy_file_fd: %s", strerror(errno));
                EC_FAIL;
            }

            buflen -= cc;
        }
    }

EC_CLEANUP:
    EC_EXIT;
}

/*!
 * @brief Duplicate the data of file src into dst
 *
 * The open creates dst or truncates an existing one. A missing source logs
 * only at debug level, because callers probe AppleDouble sidecars that plain
 * files lack and ignore ENOENT.
 *
 * @param[in] dirfd  directory that src is relative to, or -1 for the working
 *                   directory
 * @param[in] src    source file
 * @param[in] dst    destination file, relative to the working directory
 * @param[in] mode   creation mode of dst
 * @returns 0, or -1 with errno set
 */
int copy_file(int dirfd _U_, const char *src, const char *dst, mode_t mode)
{
    int    ret = 0;
    int    sfd = -1;
    int    dfd = -1;

    if (dirfd == -1) {
        dirfd = AT_FDCWD;
    }

    sfd = openat(dirfd, src, O_RDONLY);

    if (sfd < 0) {
        /* A missing source is routine here: callers copying AppleDouble
         * sidecars (vfs_copyfile, ea_ad) probe for files that plain data
         * files don't have and ignore ENOENT. */
        LOG((errno == ENOENT ? log_debug : log_info), logtype_afpd,
            "copy_file('%s'/'%s'): open '%s' error: %s",
            src, dst, src, strerror(errno));
        return -1;
    }

    if ((dfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode)) < 0) {
        LOG(log_info, logtype_afpd, "copy_file('%s'/'%s'): open '%s' error: %s",
            src, dst, dst, strerror(errno));
        ret = -1;
        goto exit;
    }

    ret = copy_file_fd(sfd, dfd);
exit:

    if (sfd != -1) {
        close(sfd);
    }

    if (dfd != -1) {
        int err;
        err = close(dfd);

        if (!ret && err) {
            /* don't bother to report an error if there's already one */
            LOG(log_error, logtype_afpd, "copy_file('%s'/'%s'): close '%s' error: %s",
                src, dst, dst, strerror(errno));
            ret = -1;
        }
    }

    return ret;
}

/*!
 * @brief Transfer one EA from src to an existing dst
 *
 * A zero-length EA leaves dst unchanged.
 *
 * @param[in] ea     EA name
 * @param[in] dirfd  directory that src is relative to, or -1 for the working
 *                   directory
 * @param[in] src    source file
 * @param[in] dst    destination file, relative to the working directory
 * @param[in] mode   unused, because the open of dst does not create it
 * @returns 0, or -1 when an open, the EA read, or the EA write failed
 */
int copy_ea(const char *ea, int dirfd _U_, const char *src, const char *dst,
            mode_t mode)
{
    EC_INIT;
    int    sfd = -1;
    int    dfd = -1;
    size_t easize;
    char   *eabuf = NULL;

    if (dirfd == -1) {
        dirfd = AT_FDCWD;
    }

    EC_NEG1_LOG(sfd = openat(dirfd, src, O_RDONLY));
    EC_NEG1_LOG(dfd = open(dst, O_WRONLY, mode));

    if ((easize = sys_fgetxattr(sfd, ea, NULL, 0)) > 0) {
        EC_NULL_LOG(eabuf = malloc(easize));
        EC_NEG1_LOG(easize = sys_fgetxattr(sfd, ea, eabuf, easize));
        EC_NEG1_LOG(easize = sys_fsetxattr(dfd, ea, eabuf, easize, 0));
    }

EC_CLEANUP:

    if (sfd != -1) {
        close(sfd);
    }

    if (dfd != -1) {
        close(dfd);
    }

    free(eabuf);
    EC_EXIT;
}

/*!
 * @brief Delete a file relative to a directory descriptor and map the error to
 * an AFP result code
 *
 * A missing file counts as success.
 *
 * @param[in] dirfd  directory that name is relative to, or -1 for the working
 *                   directory
 * @param[in] name   file to delete
 * @returns AFP_OK, AFPERR_VLOCK on a read-only file system, AFPERR_ACCESS, or
 *          AFPERR_PARAM
 */
int netatalk_unlinkat(int dirfd _U_, const char *name)
{
    if (dirfd == -1) {
        dirfd = AT_FDCWD;
    }

    if (unlinkat(dirfd, name, 0) < 0) {
        switch (errno) {
        case ENOENT :
            break;

        case EROFS:
            return AFPERR_VLOCK;

        case EPERM:
        case EACCES :
            return AFPERR_ACCESS;

        default :
            return AFPERR_PARAM;
        }
    }

    return AFP_OK;
}

/*!
 * @brief Rename oldpath to newpath through renameat
 *
 * @param[in] sfd      directory that oldpath is relative to, or -1 for the
 *                     working directory
 * @param[in] oldpath  current name
 * @param[in] dfd      directory that newpath is relative to, or -1 for the
 *                     working directory
 * @param[in] newpath  new name
 * @returns 0, or -1 with errno set
 */
int unix_rename(int sfd, const char *oldpath, int dfd, const char *newpath)
{
    if (sfd == -1) {
        sfd = AT_FDCWD;
    }

    if (dfd == -1) {
        dfd = AT_FDCWD;
    }

    if (renameat(sfd, oldpath, dfd, newpath) < 0) {
        return -1;
    }

    return 0;
}

/*!
 * @brief Stat path relative to a directory descriptor, following symlinks
 *
 * @param[in]  dirfd  directory that path is relative to, or -1 for the working
 *                    directory
 * @param[in]  path   file or directory
 * @param[out] st     receives the stat
 * @returns 0, or -1 with errno set
 */
int statat(int dirfd, const char *path, struct stat *st)
{
    if (dirfd == -1) {
        dirfd = AT_FDCWD;
    }

    return fstatat(dirfd, path, st, 0);
}

/*!
 * @brief Open a directory stream relative to a directory descriptor
 *
 * With a dirfd other than -1 the call changes into that directory for the
 * opendir and back afterwards. A failed return ends the process with
 * EXITERR_SYS.
 *
 * @param[in] dirfd  directory that path is relative to, or -1 for the working
 *                   directory
 * @param[in] path   directory to open
 * @returns the directory stream, or NULL with errno set
 */
DIR *opendirat(int dirfd, const char *path)
{
    DIR *ret;
    int cwd = -1;

    if (dirfd != -1) {
        if (((cwd = open(".", O_RDONLY)) == -1) || (fchdir(dirfd) != 0)) {
            ret = NULL;
            goto exit;
        }
    }

    ret = opendir(path);

    if (dirfd != -1 && fchdir(cwd) != 0) {
        LOG(log_error, logtype_afpd, "opendirat: can't chdir back. exit!");
        exit(EXITERR_SYS);
    }

exit:

    if (cwd != -1) {
        close(cwd);
    }

    return ret;
}
