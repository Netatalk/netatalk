/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/directory.h>
#include <atalk/volume.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/acl.h>

/* -----------------------------
   a dropbox is a folder where w is set but not r eg:
   rwx-wx-wx or rwx-wx--
   rwx----wx (is not asked by a Mac with OS >= 8.0 ?)
*/
int stickydirmode(const char *name, const mode_t mode, const int dropbox, const mode_t v_umask)
{
    int retval = 0;

#ifdef DROPKLUDGE
    /* Turn on the sticky bit if this is a drop box, also turn off the setgid bit */
    if ((dropbox & AFPVOL_DROPBOX)) {
        int uid;

        if ( ( (mode & S_IWOTH) && !(mode & S_IROTH)) ||
             ( (mode & S_IWGRP) && !(mode & S_IRGRP)) )
        {
            uid=geteuid();
            if ( seteuid(0) < 0) {
                LOG(log_error, logtype_afpd, "stickydirmode: unable to seteuid root: %s", strerror(errno));
            }
            if ( (retval=chmod( name, ( (DIRBITS | mode | S_ISVTX) & ~v_umask) )) < 0) {
                LOG(log_error, logtype_afpd, "stickydirmode: chmod \"%s\": %s", fullpathname(name), strerror(errno) );
            } else {
                LOG(log_debug, logtype_afpd, "stickydirmode: chmod \"%s\": %s", fullpathname(name), strerror(retval) );
            }
            seteuid(uid);
            return retval;
        }
    }
#endif /* DROPKLUDGE */

    /*
     *  Ignore EPERM errors:  We may be dealing with a directory that is
     *  group writable, in which case chmod will fail.
     */
    if ( (chmod( name, (DIRBITS | mode) & ~v_umask ) < 0) && errno != EPERM &&
         !(errno == ENOENT && (dropbox & AFPVOL_NOADOUBLE)) )
    {
        LOG(log_error, logtype_afpd, "stickydirmode: chmod \"%s\": %s", fullpathname(name), strerror(errno) );
        retval = -1;
    }

    return retval;
}

/* ------------------------- */
int dir_rx_set(mode_t mode)
{
    return (mode & (S_IXUSR | S_IRUSR)) == (S_IXUSR | S_IRUSR);
}

/* --------------------- */
int setfilmode(const char * name, mode_t mode, struct stat *st, mode_t v_umask)
{
    struct stat sb;
    mode_t mask = S_IRWXU | S_IRWXG | S_IRWXO;  /* rwx for owner group and other, by default */

    if (!st) {
        if (lstat(name, &sb) != 0)
            return -1;
        st = &sb;
    }

    if (S_ISLNK(st->st_mode))
        return 0; /* we don't want to change link permissions */
    
    mode |= st->st_mode & ~mask; /* keep other bits from previous mode */

    if ( chmod( name,  mode & ~v_umask ) < 0 && errno != EPERM ) {
        return -1;
    }
    return 0;
}

/*
 * @brief system rmdir with afp error code.
 *
 * Supports *at semantics (cf openat) if HAVE_ATFUNCS. Pass dirfd=-1 to ignore this.
 */
int netatalk_rmdir_all_errors(int dirfd, const char *name)
{
    int err;

#ifdef HAVE_ATFUNCS
    if (dirfd == -1)
        dirfd = AT_FDCWD;
    err = unlinkat(dirfd, name, AT_REMOVEDIR);
#else
    err = rmdir(name);
#endif

    if (err < 0) {
        switch ( errno ) {
        case ENOENT :
            return AFPERR_NOOBJ;
        case ENOTEMPTY :
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

/*
 * @brief System rmdir with afp error code, but ENOENT is not an error.
 *
 * Supports *at semantics (cf openat) if HAVE_ATFUNCS. Pass dirfd=-1 to ignore this.
 */
int netatalk_rmdir(int dirfd, const char *name)
{
    int ret = netatalk_rmdir_all_errors(dirfd, name);
    if (ret == AFPERR_NOOBJ)
        return AFP_OK;
    return ret;
}

/* -------------------
   system unlink with afp error code.
   ENOENT is not an error.
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

char *fullpathname(const char *name)
{
    static char wd[ MAXPATHLEN + 1];

    if ( getcwd( wd , MAXPATHLEN) ) {
        strlcat(wd, "/", MAXPATHLEN);
        strlcat(wd, name, MAXPATHLEN);
    }
    else {
        strlcpy(wd, name, MAXPATHLEN);
    }
    return wd;
}


/**************************************************************************
 * *at semnatics support functions (like openat, renameat standard funcs)
 **************************************************************************/

/* 
 * Supports *at semantics if HAVE_ATFUNCS, pass dirfd=-1 to ignore this
 */
int copy_file(int dirfd, const char *src, const char *dst, mode_t mode)
{
    int    ret = 0;
    int    sfd = -1;
    int    dfd = -1;
    ssize_t cc;
    size_t  buflen;
    char   filebuf[8192];

#ifdef HAVE_ATFUNCS
    if (dirfd == -1)
        dirfd = AT_FDCWD;
    sfd = openat(dirfd, src, O_RDONLY);
#else
    sfd = open(src, O_RDONLY);
#endif
    if (sfd < 0) {
        LOG(log_error, logtype_afpd, "copy_file('%s'/'%s'): open '%s' error: %s",
            src, dst, src, strerror(errno));
        return -1;
    }

    if ((dfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode)) < 0) {
        LOG(log_error, logtype_afpd, "copy_file('%s'/'%s'): open '%s' error: %s",
            src, dst, dst, strerror(errno));
        ret = -1;
        goto exit;
    }

    while ((cc = read(sfd, filebuf, sizeof(filebuf)))) {
        if (cc < 0) {
            if (errno == EINTR)
                continue;
            LOG(log_error, logtype_afpd, "copy_file('%s'/'%s'): read '%s' error: %s",
                src, dst, src, strerror(errno));
            ret = -1;
            goto exit;
        }

        buflen = cc;
        while (buflen > 0) {
            if ((cc = write(dfd, filebuf, buflen)) < 0) {
                if (errno == EINTR)
                    continue;
                LOG(log_error, logtype_afpd, "copy_file('%s'/'%s'): read '%s' error: %s",
                    src, dst, dst, strerror(errno));
                ret = -1;
                goto exit;
            }
            buflen -= cc;
        }
    }

exit:
    if (sfd != -1)
        close(sfd);

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

/* 
 * at wrapper for netatalk_unlink
 */
int netatalk_unlinkat(int dirfd, const char *name)
{
#ifdef HAVE_ATFUNCS
    if (dirfd == -1)
        dirfd = AT_FDCWD;

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
#else
    return netatalk_unlink(name);
#endif

    /* DEADC0DE */
    return 0;
}

/*
 * @brief This is equivalent of unix rename()
 *
 * unix_rename mulitplexes rename and renameat. If we dont HAVE_ATFUNCS, sfd and dfd
 * are ignored.
 *
 * @param sfd        (r) if we HAVE_ATFUNCS, -1 gives AT_FDCWD
 * @param oldpath    (r) guess what
 * @param dfd        (r) same as sfd
 * @param newpath    (r) guess what
 */
int unix_rename(int sfd, const char *oldpath, int dfd, const char *newpath)
{
#ifdef HAVE_ATFUNCS
    if (sfd == -1)
        sfd = AT_FDCWD;
    if (dfd == -1)
        dfd = AT_FDCWD;

    if (renameat(sfd, oldpath, dfd, newpath) < 0)
        return -1;        
#else
    if (rename(oldpath, newpath) < 0)
        return -1;
#endif  /* HAVE_ATFUNCS */

    return 0;
}

/* 
 * @brief stat/fsstatat multiplexer
 *
 * statat mulitplexes stat and fstatat. If we dont HAVE_ATFUNCS, dirfd is ignored.
 *
 * @param dirfd   (r) Only used if HAVE_ATFUNCS, ignored else, -1 gives AT_FDCWD
 * @param path    (r) pathname
 * @param st      (rw) pointer to struct stat
 */
int statat(int dirfd, const char *path, struct stat *st)
{
#ifdef HAVE_ATFUNCS
    if (dirfd == -1)
        dirfd = AT_FDCWD;
    return (fstatat(dirfd, path, st, 0));
#else
    return (stat(path, st));
#endif            

    /* DEADC0DE */
    return -1;
}

/* 
 * @brief lstat/fsstatat multiplexer
 *
 * lstatat mulitplexes lstat and fstatat. If we dont HAVE_ATFUNCS, dirfd is ignored.
 *
 * @param dirfd   (r) Only used if HAVE_ATFUNCS, ignored else, -1 gives AT_FDCWD
 * @param path    (r) pathname
 * @param st      (rw) pointer to struct stat
 */
int lstatat(int dirfd, const char *path, struct stat *st)
{
#ifdef HAVE_ATFUNCS
    if (dirfd == -1)
        dirfd = AT_FDCWD;
    return (fstatat(dirfd, path, st, AT_SYMLINK_NOFOLLOW));
#else
    return (lstat(path, st));
#endif            

    /* DEADC0DE */
    return -1;
}

/* 
 * @brief opendir wrapper for *at semantics support
 *
 * opendirat chdirs to dirfd if dirfd != -1 before calling opendir on path.
 *
 * @param dirfd   (r) if != -1, chdir(dirfd) before opendir(path)
 * @param path    (r) pathname
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
        LOG(log_error, logtype_afpd, "opendirat: cant chdir back. exit!");
        exit(EXITERR_SYS);
    }

exit:
    if (cwd != -1)
        close(cwd);

    return ret;
}
