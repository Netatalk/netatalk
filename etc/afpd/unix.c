/*
 * $Id: unix.c,v 1.44 2003-06-05 09:17:12 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <atalk/logger.h>
#include <netatalk/endian.h>
#include <dirent.h>
#include <limits.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
/* STDC check */
#if STDC_HEADERS
#include <string.h>
#else /* STDC_HEADERS */
#ifndef HAVE_STRCHR
#define strchr index
#define strrchr index
#endif /* HAVE_STRCHR */
char *strchr (), *strrchr ();
#ifndef HAVE_MEMCPY
#define memcpy(d,s,n) bcopy ((s), (d), (n))
#define memmove(d,s,n) bcopy ((s), (d), (n))
#endif /* ! HAVE_MEMCPY */
#endif /* STDC_HEADERS */

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include "auth.h"
#include "directory.h"
#include "volume.h"
#include "unix.h"
#include "fork.h"

/*
 * Get the free space on a partition.
 */
int ustatfs_getvolspace( vol, bfree, btotal, bsize )
const struct vol	*vol;
VolSpace    *bfree, *btotal;
u_int32_t   *bsize;
{
    VolSpace maxVolSpace = (~(VolSpace)0);

#ifdef ultrix
    struct fs_data	sfs;
#else /*ultrix*/
    struct statfs	sfs;
#endif /*ultrix*/


    if ( statfs( vol->v_path, &sfs ) < 0 ) {
        LOG(log_error, logtype_afpd, "ustatfs_getvolspace unable to stat %s", vol->v_path);
        return( AFPERR_PARAM );
    }

#ifdef ultrix
    *bfree = (VolSpace) sfs.fd_req.bfreen;
    *bsize = 1024;
#else /* !ultrix */
    *bfree = (VolSpace) sfs.f_bavail;
    *bsize = sfs.f_frsize;
#endif /* ultrix */

    if ( *bfree > maxVolSpace / *bsize ) {
        *bfree = maxVolSpace;
    } else {
        *bfree *= *bsize;
    }

#ifdef ultrix
    *btotal = (VolSpace)
              ( sfs.fd_req.btot - ( sfs.fd_req.bfree - sfs.fd_req.bfreen ));
#else /* !ultrix */
    *btotal = (VolSpace)
              ( sfs.f_blocks - ( sfs.f_bfree - sfs.f_bavail ));
#endif /* ultrix */

    /* see similar block above comments */
    if ( *btotal > maxVolSpace / *bsize ) {
        *btotal = maxVolSpace;
    } else {
        *btotal *= *bsize;
    }

    return( AFP_OK );
}

static __inline__ int utombits( bits )
mode_t	bits;
{
    int		mbits;

    mbits = 0;

    mbits |= ( bits & ( S_IREAD >> 6 ))  ? AR_UREAD  : 0;
    mbits |= ( bits & ( S_IWRITE >> 6 )) ? AR_UWRITE : 0;
    /* Do we really need this? */
    mbits |= ( bits & ( S_IEXEC >> 6) ) ? AR_USEARCH : 0;

    return( mbits );
}

/* --------------------------------
    cf AFP 3.0 page 63
*/
void utommode( stat, ma )
struct stat		*stat;
struct maccess	*ma;
{
mode_t mode;

    mode = stat->st_mode;
    ma->ma_world = utombits( mode );
    mode = mode >> 3;

    ma->ma_group = utombits( mode );
    mode = mode >> 3;

    ma->ma_owner = utombits( mode );

    /* ma_user is a union of all permissions but we must follow
     * unix perm
    */
    if ( (uuid == stat->st_uid) || (uuid == 0)) {
        ma->ma_user = ma->ma_owner | AR_UOWN;
    }
    else if ( gmem( stat->st_gid )) {
        ma->ma_user = ma->ma_group;
    }
    else {
        ma->ma_user = ma->ma_world;
    }

    /*
     * There are certain things the mac won't try if you don't have
     * the "owner" bit set, even tho you can do these things on unix wiht
     * only write permission.  What were the things?
     * 
     * FIXME and so what ?
     */
#if 0
    if ( ma->ma_user & AR_UWRITE ) {
        ma->ma_user |= AR_UOWN;
    }
#endif    
}


/*
 * Calculate the mode for a directory using a stat() call to
 * estimate permission.
 *
 * Note: the previous method, using access(), does not work correctly
 * over NFS.
 * FIXME what about ACL?
 */
void accessmode( path, ma, dir, st )
char		*path;
struct maccess	*ma;
struct dir	*dir;
struct stat     *st;

{
struct stat     sb;

    ma->ma_user = ma->ma_owner = ma->ma_world = ma->ma_group = 0;
    if (!st) {
        if (stat(path, &sb) != 0)
            return;
        st = &sb;
    }
    utommode( st, ma );
    return;
}

int gmem( gid )
const gid_t	gid;
{
    int		i;

    for ( i = 0; i < ngroups; i++ ) {
        if ( groups[ i ] == gid ) {
            return( 1 );
        }
    }
    return( 0 );
}

static __inline__ mode_t mtoubits( bits )
u_char	bits;
{
    mode_t	mode;

    mode = 0;

    mode |= ( bits & AR_UREAD ) ? ( (S_IREAD | S_IEXEC) >> 6 ) : 0;
    mode |= ( bits & AR_UWRITE ) ? ( (S_IWRITE | S_IEXEC) >> 6 ) : 0;
    /* I don't think there's a way to set the SEARCH bit by itself on a Mac
        mode |= ( bits & AR_USEARCH ) ? ( S_IEXEC >> 6 ) : 0; */

    return( mode );
}

/* ----------------------------------
   from the finder's share windows (menu--> File--> sharing...)
   and from AFP 3.0 spec page 63 
   the mac mode should be save somewhere 
*/
mode_t mtoumode( ma )
struct maccess	*ma;
{
    mode_t		mode;

    mode = 0;
    mode |= mtoubits( ma->ma_owner |ma->ma_world);
    mode = mode << 3;

    mode |= mtoubits( ma->ma_group |ma->ma_world);
    mode = mode << 3;

    mode |= mtoubits( ma->ma_world );

    return( mode );
}

/*
   a dropbox is a folder where w is set but not r eg:
   rwx-wx-wx or rwx-wx-- 
   rwx----wx (is not asked by a Mac with OS >= 8.0 ?)
*/
static int stickydirmode(name, mode, dropbox)
char * name;
const mode_t mode;
const int dropbox;
{
    int retval = 0;

#ifdef DROPKLUDGE
    /* Turn on the sticky bit if this is a drop box, also turn off the setgid bit */
    if (dropbox) {
        int uid;

        if ( ( (mode & S_IWOTH) && !(mode & S_IROTH)) ||
             ( (mode & S_IWGRP) && !(mode & S_IRGRP)) )
        {
            uid=geteuid();
            if ( seteuid(0) < 0) {
                LOG(log_error, logtype_afpd, "stickydirmode: unable to seteuid root: %s", strerror(errno));
            }
            if ( (retval=chmod( name, ( (DIRBITS | mode | S_ISVTX) & ~default_options.umask) )) < 0) {
                LOG(log_error, logtype_afpd, "stickydirmode: chmod \"%s\": %s", name, strerror(errno) );
            } else {
#ifdef DEBUG
                LOG(log_info, logtype_afpd, "stickydirmode: (debug) chmod \"%s\": %s", name, strerror(retval) );
#endif /* DEBUG */
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
    if ( (chmod( name, (DIRBITS | mode) & ~default_options.umask ) < 0) && errno != EPERM)  {
        LOG(log_error, logtype_afpd, "stickydirmode: chmod \"%s\": %s",name, strerror(errno) );
        retval = -1;
    }

    return retval;
}

int setdeskmode( mode )
const mode_t	mode;
{
    char		wd[ MAXPATHLEN + 1];
    struct stat         st;
    char		modbuf[ 12 + 1], *m;
    struct dirent	*deskp, *subp;
    DIR			*desk, *sub;

    if ( getcwd( wd , MAXPATHLEN) == NULL ) {
        return( -1 );
    }
    if ( chdir( ".AppleDesktop" ) < 0 ) {
        return( -1 );
    }
    if (( desk = opendir( "." )) == NULL ) {
        if ( chdir( wd ) < 0 ) {
            LOG(log_error, logtype_afpd, "setdeskmode: chdir %s: %s", wd, strerror(errno) );
        }
        return( -1 );
    }
    for ( deskp = readdir( desk ); deskp != NULL; deskp = readdir( desk )) {
        if ( strcmp( deskp->d_name, "." ) == 0 ||
                strcmp( deskp->d_name, ".." ) == 0 || strlen( deskp->d_name ) > 2 ) {
            continue;
        }
        strcpy( modbuf, deskp->d_name );
        strcat( modbuf, "/" );
        m = strchr( modbuf, '\0' );
        if (( sub = opendir( deskp->d_name )) == NULL ) {
            continue;
        }
        for ( subp = readdir( sub ); subp != NULL; subp = readdir( sub )) {
            if ( strcmp( subp->d_name, "." ) == 0 ||
                    strcmp( subp->d_name, ".." ) == 0 ) {
                continue;
            }
            *m = '\0';
            strcat( modbuf, subp->d_name );
            /* XXX: need to preserve special modes */
            if (stat(modbuf, &st) < 0) {
                LOG(log_error, logtype_afpd, "setdeskmode: stat %s: %s",
                    modbuf, strerror(errno) );
                continue;
            }

            if (S_ISDIR(st.st_mode)) {
                if ( chmod( modbuf,  (DIRBITS | mode) & ~default_options.umask ) < 0 && errno != EPERM ) {
                    LOG(log_error, logtype_afpd, "setdeskmode: chmod %s: %s",
                        modbuf, strerror(errno) );
                }
            } else if ( chmod( modbuf,  mode & ~default_options.umask ) < 0 && errno != EPERM ) {
                LOG(log_error, logtype_afpd, "setdeskmode: chmod %s: %s",
                    modbuf, strerror(errno) );
            }

        }
        closedir( sub );
        /* XXX: need to preserve special modes */
        if ( chmod( deskp->d_name,  (DIRBITS | mode) & ~default_options.umask ) < 0 && errno != EPERM ) {
            LOG(log_error, logtype_afpd, "setdeskmode: chmod %s: %s",
                deskp->d_name, strerror(errno) );
        }
    }
    closedir( desk );
    if ( chdir( wd ) < 0 ) {
        LOG(log_error, logtype_afpd, "setdeskmode: chdir %s: %s", wd, strerror(errno) );
        return -1;
    }
    /* XXX: need to preserve special modes */
    if ( chmod( ".AppleDesktop",  (DIRBITS | mode) & ~default_options.umask ) < 0 && errno != EPERM ) {
        LOG(log_error, logtype_afpd, "setdeskmode: chmod .AppleDesktop: %s", strerror(errno) );
    }
    return( 0 );
}

/* --------------------- */
int setfilemode (path, mode)
struct path* path;
mode_t mode;
{
    if (!path->st_valid) {
        of_stat(path);
    }

    if (path->st_errno) {
        return -1;
    }
        
    if (setfilmode( path->u_name, mode, &path->st) < 0)
        return -1;
    /* we need to set write perm if read set for resource fork */
    return setfilmode(ad_path( path->u_name, ADFLAGS_HF ), ad_hf_mode(mode), &path->st);
}

/* --------------------- */
int setfilmode(name, mode, st)
char * name;
mode_t mode;
struct stat *st;
{
struct stat sb;
mode_t mask = S_IRUSR |S_IWUSR | S_IRGRP | S_IWGRP |S_IROTH | S_IWOTH;

    if (!st) {
        if (stat(name, &sb) != 0)
            return -1;
        st = &sb;
    }
   mode &= mask;	/* keep only rw-rw-rw in mode */
   mode |= st->st_mode & ~mask; /* keep other bits from previous mode */
   if ( chmod( name,  mode & ~default_options.umask ) < 0 && errno != EPERM ) {
       return -1;
   }
   return 0;
}

/* --------------------- */
int setdirunixmode( mode, noadouble, dropbox )
const mode_t mode;
const int noadouble;
const int dropbox;
{
    if ( stickydirmode(".AppleDouble", DIRBITS | mode, dropbox) < 0 && !noadouble)
        return  -1 ;

    if ( stickydirmode(".", DIRBITS | mode, dropbox) < 0 )
        return -1;
    return 0;
}

/* --------------------- */
int setdirmode( mode, noadouble, dropbox )
const mode_t mode;
const int noadouble;
const int dropbox;
{
    char		buf[ MAXPATHLEN + 1];
    struct stat		st;
    char		*m;
    struct dirent	*dirp;
    DIR			*dir;
    
    if (( dir = opendir( "." )) == NULL ) {
        LOG(log_error, logtype_afpd, "setdirmode: opendir .: %s", strerror(errno) );
        return( -1 );
    }

    for ( dirp = readdir( dir ); dirp != NULL; dirp = readdir( dir )) {
        if ( *dirp->d_name == '.' ) {
            continue;
        }
        if ( stat( dirp->d_name, &st ) < 0 ) {
            LOG(log_error, logtype_afpd, "setdirmode: stat %s: %s",
                dirp->d_name, strerror(errno) );
            continue;
        }

        if (!S_ISDIR(st.st_mode)) {
           if (setfilmode(dirp->d_name, mode, &st) < 0) {
                LOG(log_error, logtype_afpd, "setdirmode: chmod %s: %s",
                    dirp->d_name, strerror(errno) );
                return -1;
           }
        }
#if 0
            /* XXX: need to preserve special modes */
        else if (S_ISDIR(st.st_mode)) {
                if (stickydirmode(dirp->d_name, DIRBITS | mode, dropbox) < 0)
                    return (-1);
            } else if (stickydirmode(dirp->d_name, mode, dropbox) < 0)
                return (-1);
        }
#endif
    }
    closedir( dir );

    /* change perm of .AppleDouble's files
    */
    if (( dir = opendir( ".AppleDouble" )) == NULL ) {
        if (noadouble)
            goto setdirmode_noadouble;
        LOG(log_error, logtype_afpd, "setdirmode: opendir .AppleDouble: %s", strerror(errno) );
        return( -1 );
    }
    strcpy( buf, ".AppleDouble" );
    strcat( buf, "/" );
    m = strchr( buf, '\0' );
    for ( dirp = readdir( dir ); dirp != NULL; dirp = readdir( dir )) {
        if ( strcmp( dirp->d_name, "." ) == 0 ||
                strcmp( dirp->d_name, ".." ) == 0 ) {
            continue;
        }
        *m = '\0';
        strcat( buf, dirp->d_name );

        if ( stat( buf, &st ) < 0 ) {
            LOG(log_error, logtype_afpd, "setdirmode: stat %s: %s", buf, strerror(errno) );
            continue;
        }
        if (!S_ISDIR(st.st_mode)) {
           if (setfilmode(buf, ad_hf_mode(mode), &st) < 0) {
               /* FIXME what do we do then? */
           }
        }
    } /* end for */
    closedir( dir );

    /* XXX: use special bits to tag directory permissions */

    /* XXX: need to preserve special modes */
    if ( stickydirmode(".AppleDouble", DIRBITS | mode, dropbox) < 0 )
        return( -1 );

setdirmode_noadouble:
    /* XXX: need to preserve special modes */
    if ( stickydirmode(".", DIRBITS | mode, dropbox) < 0 )
        return( -1 );
    return( 0 );
}

int setdeskowner( uid, gid )
const uid_t	uid;
const gid_t	gid;
{
    char		wd[ MAXPATHLEN + 1];
    char		modbuf[12 + 1], *m;
    struct dirent	*deskp, *subp;
    DIR			*desk, *sub;

    if ( getcwd( wd, MAXPATHLEN ) == NULL ) {
        return( -1 );
    }
    if ( chdir( ".AppleDesktop" ) < 0 ) {
        return( -1 );
    }
    if (( desk = opendir( "." )) == NULL ) {
        if ( chdir( wd ) < 0 ) {
            LOG(log_error, logtype_afpd, "setdeskowner: chdir %s: %s", wd, strerror(errno) );
        }
        return( -1 );
    }
    for ( deskp = readdir( desk ); deskp != NULL; deskp = readdir( desk )) {
        if ( strcmp( deskp->d_name, "." ) == 0 ||
                strcmp( deskp->d_name, ".." ) == 0 ||
                strlen( deskp->d_name ) > 2 ) {
            continue;
        }
        strcpy( modbuf, deskp->d_name );
        strcat( modbuf, "/" );
        m = strchr( modbuf, '\0' );
        if (( sub = opendir( deskp->d_name )) == NULL ) {
            continue;
        }
        for ( subp = readdir( sub ); subp != NULL; subp = readdir( sub )) {
            if ( strcmp( subp->d_name, "." ) == 0 ||
                    strcmp( subp->d_name, ".." ) == 0 ) {
                continue;
            }
            *m = '\0';
            strcat( modbuf, subp->d_name );
            /* XXX: add special any uid, ignore group bits */
            if ( chown( modbuf, uid, gid ) < 0 && errno != EPERM ) {
                LOG(log_error, logtype_afpd, "setdeskown: chown %s: %s",
                    modbuf, strerror(errno) );
            }
        }
        closedir( sub );
        /* XXX: add special any uid, ignore group bits */
        if ( chown( deskp->d_name, uid, gid ) < 0 && errno != EPERM ) {
            LOG(log_error, logtype_afpd, "setdeskowner: chown %s: %s",
                deskp->d_name, strerror(errno) );
        }
    }
    closedir( desk );
    if ( chdir( wd ) < 0 ) {
        LOG(log_error, logtype_afpd, "setdeskowner: chdir %s: %s", wd, strerror(errno) );
        return -1;
    }
    if ( chown( ".AppleDesktop", uid, gid ) < 0 && errno != EPERM ) {
        LOG(log_error, logtype_afpd, "setdeskowner: chown .AppleDesktop: %s",
            strerror(errno) );
    }
    return( 0 );
}


/* uid/gid == 0 need to be handled as special cases. they really mean
 * that user/group should inherit from other, but that doesn't fit
 * into the unix permission scheme. we can get around this by
 * co-opting some bits. */
int setdirowner( uid, gid, noadouble )
const uid_t	uid;
const gid_t	gid;
const int   noadouble;
{
    char		buf[ MAXPATHLEN + 1];
    struct stat		st;
    char		*m;
    struct dirent	*dirp;
    DIR			*dir;

    if (( dir = opendir( "." )) == NULL ) {
        return( -1 );
    }
    for ( dirp = readdir( dir ); dirp != NULL; dirp = readdir( dir )) {
        if ( *dirp->d_name == '.' ) {
            continue;
        };
        if ( stat( dirp->d_name, &st ) < 0 ) {
            LOG(log_error, logtype_afpd, "setdirowner: stat %s: %s",
                dirp->d_name, strerror(errno) );
            continue;
        }
        if (( st.st_mode & S_IFMT ) == S_IFREG ) {
            if ( chown( dirp->d_name, uid, gid ) < 0 && errno != EPERM ) {
                LOG(log_debug, logtype_afpd, "setdirowner: chown %s: %s",
                    dirp->d_name, strerror(errno) );
                /* return ( -1 ); Sometimes this is okay */
            }
        }
    }
    closedir( dir );
    if (( dir = opendir( ".AppleDouble" )) == NULL ) {
        if (noadouble)
            goto setdirowner_noadouble;
        return( -1 );
    }
    strcpy( buf, ".AppleDouble" );
    strcat( buf, "/" );
    m = strchr( buf, '\0' );
    for ( dirp = readdir( dir ); dirp != NULL; dirp = readdir( dir )) {
        if ( strcmp( dirp->d_name, "." ) == 0 ||
                strcmp( dirp->d_name, ".." ) == 0 ) {
            continue;
        }
        *m = '\0';
        strcat( buf, dirp->d_name );
        if ( chown( buf, uid, gid ) < 0 && errno != EPERM ) {
            LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d %s: %s",
                uid, gid, buf, strerror(errno) );
            /* return ( -1 ); Sometimes this is okay */
        }
    }
    closedir( dir );

    /*
     * We cheat: we know that chown doesn't do anything.
     */
    if ( stat( ".AppleDouble", &st ) < 0 ) {
        LOG(log_error, logtype_afpd, "setdirowner: stat .AppleDouble: %s", strerror(errno) );
        return( -1 );
    }
    if ( gid && gid != st.st_gid && chown( ".AppleDouble", uid, gid ) < 0 &&
            errno != EPERM ) {
        LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d .AppleDouble: %s",
            uid, gid, strerror(errno) );
        /* return ( -1 ); Sometimes this is okay */
    }

setdirowner_noadouble:
    if ( stat( ".", &st ) < 0 ) {
        return( -1 );
    }
    if ( gid && gid != st.st_gid && chown( ".", uid, gid ) < 0 &&
            errno != EPERM ) {
        LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d .: %s",
            uid, gid, strerror(errno) );
    }

    return( 0 );
}

#if 0
/* recursive chown()ing of a directory */
static int recursive_chown(const char *path, uid_t uid, gid_t gid) {
    struct stat sbuf;
    DIR *odir = NULL;
    struct dirent *entry;
    char *name;
    int ret = 0;
    char newpath[PATH_MAX+1];
    newpath[PATH_MAX] = '\0';
    
    if (chown(path, uid, gid) < 0) {
        LOG(log_error, logtype_afpd, "cannot chown() file [%s] (uid = %d): %s\n", path, uid, strerror(errno));
	return -1;
    }

    if (stat(path, &sbuf) < 0) {
	LOG(log_error, logtype_afpd, "cannot chown() file [%s] (uid = %d): %s\n", path, uid, strerror(errno));
	return -1;
    }
	
    if (S_ISDIR(sbuf.st_mode)) {
	odir = opendir(path);
	if (odir == NULL) {
	    LOG(log_error, logtype_afpd, "cannot opendir() [%s] (uid = %d): %s\n", path, uid, strerror(errno));
	    goto recursive_chown_end;
	}
	while (NULL != (entry=readdir(odir)) ) {
	    name = entry->d_name;
	    if (name[0] == '.' && name[1] == '\0')
		continue;
	    if (name[0] == '.' && name[1] == '.' && name[2] == '\0')
		continue;
	    sprintf(newpath, "%s/%s", path, name);
	    if (recursive_chown(newpath, uid, gid) < 0)
		ret = -1;
	} /* while */
    } /* if */

recursive_chown_end:
    if (odir != NULL) {
	closedir(odir);
    }
    return ret;
}
#endif

/* This is equivalent of unix rename(). */
int unix_rename(const char *oldpath, const char *newpath)
{
	char pd_name[PATH_MAX+1];
	int i;
        struct stat pd_stat;
        uid_t uid;

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
	    LOG(log_error, logtype_afpd, "stat() of parent dir failed: pd_name = %s, uid = %d: %s\n",
		pd_name, geteuid(), strerror(errno));
		return 0;
	}

	/* So we have SGID bit set... */
        if ((S_ISGID & pd_stat.st_mode)	!= 0) {
            uid = geteuid();
            if (seteuid(0) < 0)
		LOG(log_error, logtype_afpd, "seteuid() failed: %s\n", strerror(errno));
            if (recursive_chown(newpath, uid, pd_stat.st_gid) < 0)
		LOG(log_error, logtype_afpd, "chown() of parent dir failed: newpath=%s, uid=%d: %s\n",
		    pd_name, geteuid(), strerror(errno));
            seteuid(uid);
	}
#endif
	return 0;
}

