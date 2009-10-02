/*
 * $Id: unix.c,v 1.1 2009-10-02 09:32:41 franklahm Exp $
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
        if (stat(name, &sb) != 0)
            return -1;
        st = &sb;
    }
   
   mode |= st->st_mode & ~mask; /* keep other bits from previous mode */
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
