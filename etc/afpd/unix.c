/*
 * $Id: unix.c,v 1.21 2001-09-06 20:00:59 rufustfirefly Exp $
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
#include <sys/syslog.h>
#include <netatalk/endian.h>
#include <dirent.h>
#include <limits.h>
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
	 syslog(LOG_ERR, "ustatfs_getvolspace unable to stat %s", vol->v_path);
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

    // see similar block above comments
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

    mbits |= ( bits & ( S_IREAD >> 6 )) ? (AR_UREAD | AR_USEARCH) : 0;
    mbits |= ( bits & ( S_IWRITE >> 6 )) ? AR_UWRITE : 0;
/* Do we really need this?
    mbits |= ( bits & ( S_IEXEC >> 6) ) ? AR_USEARCH : 0; */

    return( mbits );
}

void utommode( stat, ma )
    struct stat		*stat;
    struct maccess	*ma;
{
    mode_t		mode;

    mode = stat->st_mode;

    ma->ma_world = utombits( mode );
    mode = mode >> 3;

    ma->ma_group = utombits( mode );
    mode = mode >> 3;

    ma->ma_owner = utombits( mode );

    if ( (uuid == stat->st_uid) || (uuid == 0)) {
	ma->ma_user = ma->ma_owner | AR_UOWN;
    } else if ( gmem( stat->st_gid )) {
	ma->ma_user = ma->ma_group;
    } else {
	ma->ma_user = ma->ma_world;
    }

    /*
     * There are certain things the mac won't try if you don't have
     * the "owner" bit set, even tho you can do these things on unix wiht
     * only write permission.  What were the things?
     */
    if ( ma->ma_user & AR_UWRITE ) {
	ma->ma_user |= AR_UOWN;
    }
}


/*
 * Calculate the mode for a directory using Posix access() calls to
 * estimate permission, a la mdw.
 */
void accessmode( path, ma, dir )
    char		*path;
    struct maccess	*ma;
    struct dir		*dir;
{
    if ( access( path, R_OK|W_OK|X_OK ) == 0 ) {
	ma->ma_user = AR_UREAD|AR_UWRITE|AR_USEARCH|AR_UOWN;
	ma->ma_owner = AR_UREAD|AR_UWRITE|AR_USEARCH;
    } else if ( access( path, R_OK|X_OK ) == 0 ) {
	ma->ma_user = AR_UREAD|AR_USEARCH;
	ma->ma_owner = AR_UREAD|AR_USEARCH;
    } else {
	ma->ma_user = ma->ma_owner = 0;
	if ( access( path, R_OK ) == 0 ) {
	    ma->ma_user |= AR_UREAD;
	    ma->ma_owner |= AR_UREAD;
	}
	if ( access( path, X_OK ) == 0 ) {
	    ma->ma_user |= AR_USEARCH;
	    ma->ma_owner |= AR_USEARCH;
	}
	if ( access( path, W_OK ) == 0 ) {
	    ma->ma_user |= AR_UWRITE|AR_UOWN;
	    ma->ma_owner |= AR_UWRITE;
	}
    }
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

mode_t mtoumode( ma )
    struct maccess	*ma;
{
    mode_t		mode;

    mode = 0;
    mode |= mtoubits( ma->ma_owner );
    mode = mode << 3;

    mode |= mtoubits( ma->ma_group );
    mode = mode << 3;

    mode |= mtoubits( ma->ma_world );

    return( mode );
}

inline int stickydirmode(name, mode, dropbox)
    char * name;
    const mode_t mode;
    const int dropbox;
{
  int retval;
#ifdef DROPKLUDGE
  int uid;
#endif /* DROPKLUDGE */
  
/* Turn on the sticky bit if this is a drop box, also turn off the setgid bit */
   retval=0;
#ifdef DROPKLUDGE
   if (dropbox) {
    if (mode & S_IWOTH) { 
      if (mode & S_IROTH); 
      else { /* if S_IWOTH and not S_IROTH */
        uid=geteuid();
        if ( seteuid(0) < 0) {
	   syslog( LOG_ERR, "stickydirmode: unable to seteuid root: %m");
        }
        if ( retval=chmod( name, ( DIRBITS | mode | S_ISVTX) ) < 0) {
           syslog( LOG_ERR, "stickydirmode: chmod \"%s\": %m", name );
           return(AFPERR_ACCESS);
        } else {
#ifdef DEBUG
           syslog( LOG_INFO, "stickydirmode: (debug) chmod \"%s\": %m", name );
#endif /* DEBUG */
           seteuid(uid);
        } /* end getting retval */
      } /* end if not & S_IROTH */
   } else { /* end if S_IWOTH and not S_IROTH */
#endif /* DROPKLUDGE */
       if ( (retval=chmod( name, DIRBITS | mode )) < 0 )  {
          syslog( LOG_ERR, "stickydirmode: chmod \"%s\": %s",
		  name, strerror(errno) );
       }
#ifdef DROPKLUDGE
     } /* end if not mode */
   } /* end checking for "dropbox" */
#endif /* DROPKLUDGE */
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
	    syslog( LOG_ERR, "setdeskmode: chdir %s: %s", wd, strerror(errno) );
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
	        syslog( LOG_ERR, "setdeskmode: stat %s: %s",
			modbuf, strerror(errno) );
	        continue;
	    }	    

	    if (S_ISDIR(st.st_mode)) {
	      if ( chmod( modbuf,  DIRBITS | mode ) < 0 ) {
		syslog( LOG_ERR, "setdeskmode: chmod %s: %s",
			modbuf, strerror(errno) );
	      }
	    } else if ( chmod( modbuf,  mode ) < 0 ) {
	        syslog( LOG_ERR, "setdeskmode: chmod %s: %s",
			modbuf, strerror(errno) );
	    }

	}
	closedir( sub );
	/* XXX: need to preserve special modes */
	if ( chmod( deskp->d_name,  DIRBITS | mode ) < 0 ) {
	    syslog( LOG_ERR, "setdeskmode: chmod %s: %s",
		    deskp->d_name, strerror(errno) );
	}
    }
    closedir( desk );
    if ( chdir( wd ) < 0 ) {
	syslog( LOG_ERR, "setdeskmode: chdir %s: %s", wd, strerror(errno) );
	return -1;
    }
    /* XXX: need to preserve special modes */
    if ( chmod( ".AppleDesktop",  DIRBITS | mode ) < 0 ) {
	syslog( LOG_ERR, "setdeskmode: chmod .AppleDesktop: %s", strerror(errno) );
    }
    return( 0 );
}

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
	syslog( LOG_ERR, "setdirmode: opendir .: %s", strerror(errno) );
	return( -1 );
    }

    for ( dirp = readdir( dir ); dirp != NULL; dirp = readdir( dir )) {
	if ( *dirp->d_name == '.' ) {
	    continue;
	}
	if ( stat( dirp->d_name, &st ) < 0 ) {
	    syslog( LOG_ERR, "setdirmode: stat %s: %s",
		    dirp->d_name, strerror(errno) );
	    continue;
	}

	if (S_ISREG(st.st_mode)) {
	    /* XXX: need to preserve special modes */
	    if (S_ISDIR(st.st_mode)) {
              if (stickydirmode(dirp->d_name, DIRBITS | mode, dropbox) < 0)
		return (-1);
	    } else if (stickydirmode(dirp->d_name, mode, dropbox) < 0)
		return (-1);
	}
    }
    closedir( dir );
    if (( dir = opendir( ".AppleDouble" )) == NULL ) {
        if (noadouble)
	  goto setdirmode_noadouble;
	syslog( LOG_ERR, "setdirmode: opendir .AppleDouble: %s", strerror(errno) );
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
	    syslog( LOG_ERR, "setdirmode: stat %s: %s", buf, strerror(errno) );
	    continue;
	}

	if (S_ISDIR(st.st_mode)) {
           stickydirmode( buf, DIRBITS | mode, dropbox );
	} else 
           stickydirmode( buf, mode, dropbox );
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
	    syslog( LOG_ERR, "setdeskowner: chdir %s: %s", wd, strerror(errno) );
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
	    if ( chown( modbuf, uid, gid ) < 0 ) {
		syslog( LOG_ERR, "setdeskown: chown %s: %s",
			modbuf, strerror(errno) );
	    }
	}
	closedir( sub );
	/* XXX: add special any uid, ignore group bits */
	if ( chown( deskp->d_name, uid, gid ) < 0 ) {
	    syslog( LOG_ERR, "setdeskowner: chown %s: %s",
		    deskp->d_name, strerror(errno) );
	}
    }
    closedir( desk );
    if ( chdir( wd ) < 0 ) {
	syslog( LOG_ERR, "setdeskowner: chdir %s: %s", wd, strerror(errno) );
	return -1;
    }
    if ( chown( ".AppleDesktop", uid, gid ) < 0 ) {
	syslog( LOG_ERR, "setdeskowner: chown .AppleDesktop: %s",
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
	    syslog( LOG_ERR, "setdirowner: stat %s: %s",
		    dirp->d_name, strerror(errno) );
	    continue;
	}
	if (( st.st_mode & S_IFMT ) == S_IFREG ) {
	    if ( chown( dirp->d_name, uid, gid ) < 0 ) {
		syslog( LOG_DEBUG, "setdirowner: chown %s: %s",
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
	if ( chown( buf, uid, gid ) < 0 ) {
	    syslog( LOG_DEBUG, "setdirowner: chown %d/%d %s: %s",
		    uid, gid, buf, strerror(errno) );
	    /* return ( -1 ); Sometimes this is okay */
	}
    }
    closedir( dir );

    /*
     * We cheat: we know that chown doesn't do anything.
     */
    if ( stat( ".AppleDouble", &st ) < 0 ) {
	syslog( LOG_ERR, "setdirowner: stat .AppleDouble: %s", strerror(errno) );
	return( -1 );
    }
    if ( gid && gid != st.st_gid && chown( ".AppleDouble", uid, gid ) < 0 ) {
 	syslog( LOG_DEBUG, "setdirowner: chown %d/%d .AppleDouble: %s",
 		uid, gid, strerror(errno) );
	/* return ( -1 ); Sometimes this is okay */
    }

setdirowner_noadouble:
    if ( stat( ".", &st ) < 0 ) {
	return( -1 );
    }
    if ( gid && gid != st.st_gid && chown( ".", uid, gid ) < 0 ) {
        syslog( LOG_DEBUG, "setdirowner: chown %d/%d .: %s",
		uid, gid, strerror(errno) );
    }

    return( 0 );
}
