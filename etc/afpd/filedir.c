/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/cnid.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

#include "directory.h"
#include "desktop.h"
#include "volume.h"
#include "fork.h"
#include "file.h"
#include "globals.h"
#include "filedir.h"

int matchfile2dirperms(upath, vol, did)
                              /* Since it's kinda' big; I decided against an
                              inline function */
    char	*upath;
    struct vol  *vol;
    int		did;
  /* The below code changes the way file ownership is determined in the name of
  fixing dropboxes.  It has known security problem.  See the netatalk FAQ for
  more information */
{
    struct stat	st, sb;
    struct dir	*dir;
    char	adpath[50];
    int		uid;

#ifdef DEBUG
    syslog (LOG_INFO, "begin matchfile2dirperms:");
#endif DEBUG

    if (stat(upath, &st ) < 0)
      syslog(LOG_ERR, "Could not stat %s: %m", upath);
    strcpy (adpath, "./.AppleDouble/");
    strcat (adpath, upath);
    if (( dir = dirsearch( vol, did )) == NULL ) {
      syslog (LOG_ERR, "matchfile2dirperms: Unable to get directory info.");
      return( AFPERR_NOOBJ );
    }
    else if (stat(".", &sb) < 0) {
        syslog (LOG_ERR, 
	   "matchfile2dirperms: Error checking directory \"%s\": %m", 
	   dir->d_name);
        return(AFPERR_NOOBJ );
      }
    else {
	uid=geteuid();
	if ( uid != sb.st_uid )
        {
	  seteuid(0);
	  if (lchown(upath, sb.st_uid, sb.st_gid) < 0)
          {
            syslog (LOG_ERR, 
	      "matchfile2dirperms: Error changing owner/gid of %s: %m", upath);
            return (AFPERR_ACCESS);
          }
          if (chmod(upath,(st.st_mode&0x0FFFF)| S_IRGRP| S_IROTH) < 0)
          {
            syslog (LOG_ERR, 
	      "matchfile2dirperms:  Error adding file read permissions: %m");
            return (AFPERR_ACCESS);
          }
#ifdef DEBUG
          else 
	    syslog (LOG_INFO, 
	      "matchfile2dirperms:  Added S_IRGRP and S_IROTH: %m");
#endif DEBUG
          if (lchown(adpath, sb.st_uid, sb.st_gid) < 0)
          {
	    syslog (LOG_ERR, 
	      "matchfile2dirperms: Error changing AppleDouble owner/gid %s: %m",
	      adpath);
            return (AFPERR_ACCESS);
	  }
          if (chmod(adpath, (st.st_mode&0x0FFFF)| S_IRGRP| S_IROTH) < 0)
          {
            syslog (LOG_ERR, 
	      "matchfile2dirperms:  Error adding AD file read permissions: %m");
            return (AFPERR_ACCESS);
          }
#ifdef DEBUG
          else 
	    syslog (LOG_INFO, 
	      "matchfile2dirperms:  Added S_IRGRP and S_IROTH to AD: %m");
#endif DEBUG
	}
#ifdef DEBUG
        else
	  syslog (LOG_INFO, 
	    "matchfile2dirperms: No ownership change necessary.");
#endif DEBUG
    } /* end else if stat success */
    seteuid(uid); /* Restore process ownership to normal */
#ifdef DEBUG
    syslog (LOG_INFO, "end matchfile2dirperms:");
#endif DEBUG

    return (AFP_OK);

}
    

int afp_getfildirparams(obj, ibuf, ibuflen, rbuf, rbuflen )
    AFPObj      *obj;
    char	*ibuf, *rbuf;
    int		ibuflen, *rbuflen;
{
    struct stat		st;
    struct vol		*vol;
    struct dir		*dir;
    u_int32_t           did;
    int			buflen, ret;
    char		*path;
    u_int16_t		fbitmap, dbitmap, vid;

#ifdef DEBUG
    syslog(LOG_INFO, "begin afp_getfildirparams:");
#endif DEBUG

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (( vol = getvolbyvid( vid )) == NULL ) {
	return( AFPERR_PARAM );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );

    if (( dir = dirsearch( vol, did )) == NULL ) {
	return( AFPERR_NOOBJ );
    }

    memcpy( &fbitmap, ibuf, sizeof( fbitmap ));
    fbitmap = ntohs( fbitmap );
    ibuf += sizeof( fbitmap );
    memcpy( &dbitmap, ibuf, sizeof( dbitmap ));
    dbitmap = ntohs( dbitmap );
    ibuf += sizeof( dbitmap );

    if (( path = cname( vol, dir, &ibuf )) == NULL) {
	return( AFPERR_NOOBJ );
    }

    if ( stat( mtoupath(vol, path ), &st ) < 0 ) {
	return( AFPERR_NOOBJ );
    }

    buflen = 0;
    if (S_ISDIR(st.st_mode)) {
	if (dbitmap && ( ret = getdirparams(vol, dbitmap, ".", curdir,
		&st, rbuf + 3 * sizeof( u_int16_t ), &buflen )) != AFP_OK ) {
	    return( ret );
	}
        /* this is a directory */
	*(rbuf + 2 * sizeof( u_int16_t )) = FILDIRBIT_ISDIR; 
    } else {
	if (fbitmap && ( ret = getfilparams(vol, fbitmap, path, curdir, &st,
		rbuf + 3 * sizeof( u_int16_t ), &buflen )) != AFP_OK ) {
	    return( ret );
	}
        /* this is a file */
	*(rbuf + 2 * sizeof( u_int16_t )) = FILDIRBIT_ISFILE;
    }
    *rbuflen = buflen + 3 * sizeof( u_int16_t );
    fbitmap = htons( fbitmap );
    memcpy( rbuf, &fbitmap, sizeof( fbitmap ));
    rbuf += sizeof( fbitmap );
    dbitmap = htons( dbitmap );
    memcpy( rbuf, &dbitmap, sizeof( dbitmap ));
    rbuf += sizeof( dbitmap ) + sizeof( u_char );
    *rbuf = 0;

#ifdef DEBUG
    syslog(LOG_INFO, "end afp_getfildirparams:");
#endif DEBUG

    return( AFP_OK );
}

int afp_setfildirparams(obj, ibuf, ibuflen, rbuf, rbuflen )
    AFPObj      *obj;
    char	*ibuf, *rbuf;
    int		ibuflen, *rbuflen;
{
    struct stat	st;
    struct vol	*vol;
    struct dir	*dir;
    char	*path;
    u_int16_t	vid, bitmap;
    int		did, rc;

#ifdef DEBUG
    syslog(LOG_INFO, "begin afp_setfildirparams:");
#endif DEBUG

    *rbuflen = 0;
    ibuf += 2;
    memcpy( &vid, ibuf, sizeof(vid));
    ibuf += sizeof( vid );

    if (( vol = getvolbyvid( vid )) == NULL ) {
	return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy( &did, ibuf, sizeof( did));
    ibuf += sizeof( did);

    if (( dir = dirsearch( vol, did )) == NULL ) {
	return( AFPERR_NOOBJ );
    }

    memcpy( &bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
	return( AFPERR_NOOBJ );
    }

    if ( stat( mtoupath(vol, path ), &st ) < 0 ) {
	return( AFPERR_NOOBJ );
    }

    /*
     * If ibuf is odd, make it even.
     */
    if ((u_long)ibuf & 1 ) {
	ibuf++;
    }

    if (S_ISDIR(st.st_mode)) {
	rc = setdirparams(vol, path, bitmap, ibuf );
    } else {
	rc = setfilparams(vol, path, bitmap, ibuf );
    }
    if ( rc == AFP_OK ) {
	setvoltime(obj, vol );
    }

#ifdef DEBUG
    syslog(LOG_INFO, "end afp_setfildirparams:");
#endif DEBUG

    return( rc );
}

int afp_rename(obj, ibuf, ibuflen, rbuf, rbuflen )
    AFPObj      *obj;
    char	*ibuf, *rbuf;
    int		ibuflen, *rbuflen;
{
    struct adouble	ad;
    struct stat		st;
    struct vol		*vol;
    struct dir		*dir, *odir = NULL;
    char		*path, *buf, *upath, *newpath;
    char		*newadpath;
    u_int32_t		did;
    int			plen;
    u_int16_t		vid;
#if AD_VERSION > AD_VERSION1
    cnid_t              id;
#endif

#ifdef DEBUG
    syslog(LOG_INFO, "begin afp_rename:");
#endif DEBUG

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (( vol = getvolbyvid( vid )) == NULL ) {
	return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (( dir = dirsearch( vol, did )) == NULL ) {
	return( AFPERR_NOOBJ );
    }

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
	return( AFPERR_NOOBJ );
    }

    /* another place where we know about the path type */
    if ( *ibuf++ != 2 ) {
	return( AFPERR_PARAM );
    }
    plen = (unsigned char) *ibuf++;
    *( ibuf + plen ) = '\0';

    if ( *path == '\0' ) {
        if ( curdir->d_parent == NULL ) { /* root directory */
	    return( AFPERR_NORENAME );
	}
	odir = curdir;
	path = curdir->d_name;
	if ( movecwd( vol, curdir->d_parent ) < 0 ) {
	    return( AFPERR_NOOBJ );
	}
    }

#ifdef notdef
    if ( strcasecmp( path, ibuf ) == 0 ) {
	return( AFP_OK );
    }
#endif notdef

    /* if a curdir/newname ofork exists, return busy */
    if (of_findname(vol, curdir, ibuf))
        return AFPERR_BUSY;

    /* source == destination. just say okay. */
    if (strcmp(path, ibuf) == 0)
        return AFP_OK;

    /* check for illegal characters */
    if ((vol->v_flags & AFPVOL_MSWINDOWS) && 
	strpbrk(ibuf, MSWINDOWS_BADCHARS))
        return AFPERR_PARAM;

    newpath = obj->oldtmp;
    strcpy( newpath, mtoupath(vol, ibuf ));

    if ((vol->v_flags & AFPVOL_NOHEX) && strchr(newpath, '/'))
      return AFPERR_PARAM;

    if (!validupath(vol, newpath))
      return AFPERR_EXIST;

    /* the strdiacasecmp deals with case-insensitive, case preserving
       filesystems */
    if (stat( newpath, &st ) == 0 && strdiacasecmp(path, ibuf))
	return( AFPERR_EXIST );

    upath = mtoupath(vol, path);

#if AD_VERSION > AD_VERSION1
    id = cnid_get(vol->v_db, curdir->d_did, upath, strlen(upath));
#endif

    if ( rename( upath, newpath ) < 0 ) {
	switch ( errno ) {
	case ENOENT :
	    return( AFPERR_NOOBJ );
	case EACCES :
	    return( AFPERR_ACCESS );
	default :
	    return( AFPERR_PARAM );
	}
    }

#if AD_VERSION > AD_VERSION1
    if (stat(newpath, &st) < 0) /* this shouldn't fail */
      return AFPERR_MISC;
    cnid_update(vol->v_db, id, &st, curdir->d_did, newpath, strlen(newpath));
#endif

    if ( !odir ) {
        newadpath = obj->newtmp;
	strcpy( newadpath, ad_path( newpath, 0 ));
	if ( rename( ad_path( upath, 0 ), newadpath ) < 0 ) {
	    if ( errno == ENOENT ) {	/* no adouble header file */
		if (( unlink( newadpath ) < 0 ) && ( errno != ENOENT )) {
		    return( AFPERR_PARAM );
		}
		goto out;
	    }
	    return( AFPERR_PARAM );
	}

	memset(&ad, 0, sizeof(ad));
	if ( ad_open( newpath, ADFLAGS_HF, O_RDWR|O_CREAT, 0666,
		      &ad) < 0 ) {
	    return( AFPERR_PARAM );
	}
    } else {
        int isad = 1;

	memset(&ad, 0, sizeof(ad));
	if ( ad_open( newpath, vol_noadouble(vol)|ADFLAGS_HF|ADFLAGS_DIR, 
		      O_RDWR|O_CREAT, 0666, &ad) < 0 ) {
	    if (!((errno == ENOENT) && vol_noadouble(vol)))
	      return( AFPERR_PARAM );
	    isad = 0;
	}
	if ((buf = realloc( odir->d_name, plen + 1 )) == NULL ) {
	    syslog( LOG_ERR, "afp_rename: realloc: %m" );
	    if (isad) {
	      ad_flush(&ad, ADFLAGS_HF); /* in case of create */
	      ad_close(&ad, ADFLAGS_HF);
	    }
	    return AFPERR_MISC;
	}
	odir->d_name = buf;
	strcpy( odir->d_name, ibuf );
	if (!isad)
	  goto out;
    }

    ad_setentrylen( &ad, ADEID_NAME, plen );
    memcpy( ad_entry( &ad, ADEID_NAME ), ibuf, plen );
    ad_flush( &ad, ADFLAGS_HF );
    ad_close( &ad, ADFLAGS_HF );

out:
    setvoltime(obj, vol );

    /* if it's still open, rename the ofork as well. */
    if (of_rename(vol, curdir, path, curdir, ibuf) < 0)
	return AFPERR_MISC;

#ifdef DEBUG
    syslog(LOG_INFO, "end afp_rename:");
#endif DEBUG

    return( AFP_OK );
}


int afp_delete(obj, ibuf, ibuflen, rbuf, rbuflen )
    AFPObj      *obj;
    char	*ibuf, *rbuf;
    int		ibuflen, *rbuflen;
{
    struct vol		*vol;
    struct dir		*dir;
    char		*path, *upath;
    int			did, rc;
    u_int16_t		vid;

#ifdef DEBUG
    syslog(LOG_INFO, "begin afp_delete:");
#endif DEBUG

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (( vol = getvolbyvid( vid )) == NULL ) {
	return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( int );
    if (( dir = dirsearch( vol, did )) == NULL ) {
	return( AFPERR_NOOBJ );
    }

    if (( path = cname( vol, dir, &ibuf )) == NULL ) {
	return( AFPERR_NOOBJ );
    }

    if ( *path == '\0' ) {
	rc = deletecurdir( vol, obj->oldtmp, AFPOBJ_TMPSIZ);
    } else if (of_findname(vol, curdir, path)) {
        rc = AFPERR_BUSY;
    } else if ((rc = deletefile( upath = mtoupath(vol, path ))) == AFP_OK) {
#if AD_VERSION > AD_VERSION1 /* get rid of entry */
        cnid_t id = cnid_get(vol->v_db, curdir->d_did, upath, strlen(upath));
	cnid_delete(vol->v_db, id);
#endif
    }
    if ( rc == AFP_OK ) {
	setvoltime(obj, vol );
    }

#ifdef DEBUG
    syslog(LOG_INFO, "end afp_delete:");
#endif DEBUG

    return( rc );
}

char *ctoupath( vol, dir, name )
    const struct vol	*vol;
    struct dir	*dir;
    char	*name;
{
    struct dir	*d;
    static char	path[ MAXPATHLEN + 1];
    char	*p, *u;
    int		len;

    p = path + sizeof( path ) - 1;
    *p = '\0';
    u = mtoupath(vol, name );
    len = strlen( u );
    p -= len;
    strncpy( p, u, len );
    for ( d = dir; d->d_parent; d = d->d_parent ) {
	*--p = '/';
	u = mtoupath(vol, d->d_name );
	len = strlen( u );
	p -= len;
	strncpy( p, u, len );
    }
    *--p = '/';
    len = strlen( vol->v_path );
    p -= len;
    strncpy( p, vol->v_path, len );

    return( p );
}


int afp_moveandrename(obj, ibuf, ibuflen, rbuf, rbuflen )
    AFPObj      *obj;
    char	*ibuf, *rbuf;
    int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    struct dir	*sdir, *ddir, *odir = NULL;
    struct stat st;
    char	*oldname, *newname;
    char        *path, *p, *upath; 
    int		did, rc;
    int		plen;
    u_int16_t	vid;
#if AD_VERSION > AD_VERSION1
    cnid_t      id;
#endif

#ifdef DEBUG
    syslog(LOG_INFO, "begin afp_moveandrename:");
#endif DEBUG

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (( vol = getvolbyvid( vid )) == NULL ) {
	return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    /* source did followed by dest did */
    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( int );
    if (( sdir = dirsearch( vol, did )) == NULL ) {
	return( AFPERR_PARAM );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( int );

    /* source pathname */
    if (( path = cname( vol, sdir, &ibuf )) == NULL ) {
	return( AFPERR_NOOBJ );
    }

    sdir = curdir;
    newname = obj->newtmp;
    oldname = obj->oldtmp;
    if ( *path != '\0' ) {
        /* not a directory */
	strcpy(newname, path);
	strcpy(oldname, path); /* an extra copy for of_rename */
#if AD_VERSION > AD_VERSION1
	p = mtoupath(vol, path);
	id = cnid_get(vol->v_db, sdir->d_did, p, strlen(p));
#endif
	p = ctoupath( vol, sdir, newname );
    } else {
	odir = curdir;
	strcpy( newname, odir->d_name );
	strcpy(oldname, odir->d_name); 
	p = ctoupath( vol, odir->d_parent, newname );
#if AD_VERSION > AD_VERSION1
	id = curdir->d_did; /* we already have the CNID */
#endif
    }
    /*
     * p now points to the full pathname of the source fs object.
     */

    /* get the destination directory */
    if (( ddir = dirsearch( vol, did )) == NULL ) {
	return( AFPERR_PARAM );
    }
    if (( path = cname( vol, ddir, &ibuf )) == NULL ) {
	return( AFPERR_NOOBJ );
    }
    if ( *path != '\0' ) {
	return( AFPERR_BADTYPE );
    }

    /* one more place where we know about path type */
    if ( *ibuf++ != 2 ) {
	return( AFPERR_PARAM );
    }

    if (( plen = (unsigned char)*ibuf++ ) != 0 ) {
	strncpy( newname, ibuf, plen );
	newname[ plen ] = '\0';
    }

    /* check for illegal characters */
    if ((vol->v_flags & AFPVOL_MSWINDOWS) && 
	strpbrk(newname, MSWINDOWS_BADCHARS))
        return AFPERR_PARAM;

    upath = mtoupath(vol, newname);

    if ((vol->v_flags & AFPVOL_NOHEX) && strchr(upath, '/'))
      return AFPERR_PARAM;

    if (!validupath(vol, upath))
      return AFPERR_EXIST;

    /* source == destination. we just silently accept this. */
    if (curdir == sdir) {
      if (strcmp(oldname, newname) == 0)
	return AFP_OK;
      
      /* deal with case insensitive, case-preserving filesystems. */
      if ((stat(upath, &st) == 0) && strdiacasecmp(oldname, newname)) 
	return AFPERR_EXIST;
      
    } else if (stat(upath, &st ) == 0)
      return( AFPERR_EXIST );
      
    if ( !odir ) {
      if (of_findname(vol, curdir, newname)) {
	rc = AFPERR_BUSY;
      } else if ((rc = renamefile( p, upath, newname, 
				   vol_noadouble(vol) )) == AFP_OK) {
	/* if it's still open, rename the ofork as well. */
	rc = of_rename(vol, sdir, oldname, curdir, newname);
      }
    } else {
	rc = renamedir(p, upath, odir, curdir, newname, vol_noadouble(vol));
    }

#ifdef DROPKLUDGE
    if (vol->v_flags & AFPVOL_DROPBOX) {
        if (retvalue=matchfile2dirperms (newname, vol, did) != AFP_OK) {
	    return retvalue;
        }
    }
#endif DROPKLUDGE

    if ( rc == AFP_OK ) {
#if AD_VERSION > AD_VERSION1
        /* renaming may have moved the file/dir across a filesystem */
        if (stat(upath, &st) < 0) 
	  return AFPERR_MISC;
	
	/* fix up the catalog entry */
	cnid_update(vol->v_db, id, &st, curdir->d_did, upath, strlen(upath));
#endif      
	setvoltime(obj, vol );
    }

#ifdef DEBUG
    syslog(LOG_INFO, "end afp_moveandrename:");
#endif DEBUG

    return( rc );
}

