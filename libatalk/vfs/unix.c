/*
 * $Id: unix.c,v 1.7 2010-01-20 13:22:13 franklahm Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <errno.h>
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
    mode_t result = mode;
    mode_t mask = S_IRWXU | S_IRWXG | S_IRWXO;  /* rwx for owner group and other, by default */

    if (!st) {
        if (stat(name, &sb) != 0)
            return -1;
        st = &sb;
    }

    result |= st->st_mode & ~mask; /* keep other bits from previous mode */

    LOG(log_debug, logtype_afpd, "setfilmode('%s', mode:%04o, vmask:%04o) {st_mode:%04o, chmod:%04o}",
        fullpathname(name), mode, v_umask, st->st_mode, result);

    if ( chmod( name,  mode & ~v_umask ) < 0 && errno != EPERM ) {
        return -1;
    }
    return 0;
}

/* -------------------
   system rmdir with afp error code.
   ENOENT is not an error.
*/
int netatalk_rmdir(const char *name)
{
    if (rmdir(name) < 0) {
        switch ( errno ) {
        case ENOENT :
            break;
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

int copy_file(const char *src, const char *dst, mode_t mode)
{
    int    ret = 0;
    int    sfd = -1;
    int    dfd = -1;
    ssize_t cc;
    size_t  buflen;
    char   filebuf[8192];

    if ((sfd = open(src, O_RDONLY)) < 0) {
        LOG(log_error, logtype_afpd, "copy_file('%s'/'%s'): open '%s' error: %s",
            src, dst, src, strerror(errno));
        return -1;
    }

    if ((dfd = open(dst, O_WRONLY | O_CREAT | O_EXCL, mode)) < 0) {
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

/* This is equivalent of unix rename(). */
int unix_rename(const char *oldpath, const char *newpath)
{
#if 0
    char pd_name[PATH_MAX+1];
    int i;
    struct stat pd_stat;
    uid_t uid;
#endif

    if (rename(oldpath, newpath) < 0)
        return -1;
#if 0
    for (i = 0; i <= PATH_MAX && newpath[i] != '\0'; i++)
        pd_name[i] = newpath[i];
    pd_name[i] = '\0';

    while (i > 0 && pd_name[i] != '/') i--;
    if (pd_name[i] == '/') i++;

    pd_name[i++] = '.'; pd_name[i++] = '\0';

    if (stat(pd_name, &pd_stat) < 0) {
        LOG(log_error, logtype_afpd, "stat() of parent dir failed: pd_name = %s, uid = %d: %s",
            pd_name, geteuid(), strerror(errno));
        return 0;
    }

    /* So we have SGID bit set... */
    if ((S_ISGID & pd_stat.st_mode) != 0) {
        uid = geteuid();
        if (seteuid(0) < 0)
            LOG(log_error, logtype_afpd, "seteuid() failed: %s", strerror(errno));
        if (recursive_chown(newpath, uid, pd_stat.st_gid) < 0)
            LOG(log_error, logtype_afpd, "chown() of parent dir failed: newpath=%s, uid=%d: %s",
                pd_name, geteuid(), strerror(errno));
        seteuid(uid);
    }
#endif
    return 0;
}
