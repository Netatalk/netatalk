/*
 * $Id: filedir.c,v 1.28 2002-08-29 18:57:26 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <atalk/logger.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#ifdef CNID_DB
#include <atalk/cnid.h>
#endif /* CNID_DB */
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <dirent.h>

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

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
    LOG(log_info, logtype_afpd, "begin matchfile2dirperms:");
#endif /* DEBUG */

    if (stat(upath, &st ) < 0)
        LOG(log_error, logtype_afpd, "Could not stat %s: %s", upath, strerror(errno));
    strcpy (adpath, "./.AppleDouble/");
    strcat (adpath, upath);
    if (( dir = dirsearch( vol, did )) == NULL ) {
        LOG(log_error, logtype_afpd, "matchfile2dirperms: Unable to get directory info.");
        return( AFPERR_NOOBJ );
    }
    else if (stat(".", &sb) < 0) {
        LOG(log_error, logtype_afpd,
            "matchfile2dirperms: Error checking directory \"%s\": %s",
            dir->d_name, strerror(errno));
        return(AFPERR_NOOBJ );
    }
    else {
        uid=geteuid();
        if ( uid != sb.st_uid )
        {
            seteuid(0);
            if (lchown(upath, sb.st_uid, sb.st_gid) < 0)
            {
                LOG(log_error, logtype_afpd,
                    "matchfile2dirperms: Error changing owner/gid of %s: %s",
                    upath, strerror(errno));
                return (AFPERR_ACCESS);
            }
            if (chmod(upath,(st.st_mode&~default_options.umask)| S_IRGRP| S_IROTH) < 0)
            {
                LOG(log_error, logtype_afpd,
                    "matchfile2dirperms:  Error adding file read permissions: %s",
                    strerror(errno));
                return (AFPERR_ACCESS);
            }
#ifdef DEBUG
            else
                LOG(log_info, logtype_afpd,
                    "matchfile2dirperms:  Added S_IRGRP and S_IROTH: %s",
                    strerror(errno));
#endif /* DEBUG */
            if (lchown(adpath, sb.st_uid, sb.st_gid) < 0)
            {
                LOG(log_error, logtype_afpd,
                    "matchfile2dirperms: Error changing AppleDouble owner/gid %s: %s",
                    adpath, strerror(errno));
                return (AFPERR_ACCESS);
            }
            if (chmod(adpath, (st.st_mode&~default_options.umask)| S_IRGRP| S_IROTH) < 0)
            {
                LOG(log_error, logtype_afpd,
                    "matchfile2dirperms:  Error adding AD file read permissions: %s",
                    strerror(errno));
                return (AFPERR_ACCESS);
            }
#ifdef DEBUG
            else
                LOG(log_info, logtype_afpd,
                    "matchfile2dirperms:  Added S_IRGRP and S_IROTH to AD: %s",
                    strerror(errno));
#endif /* DEBUG */
        }
#ifdef DEBUG
        else
            LOG(log_info, logtype_afpd,
                "matchfile2dirperms: No ownership change necessary.");
#endif /* DEBUG */
    } /* end else if stat success */
    seteuid(uid); /* Restore process ownership to normal */
#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end matchfile2dirperms:");
#endif /* DEBUG */

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
    LOG(log_info, logtype_afpd, "begin afp_getfildirparams:");
#endif /* DEBUG */

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (( vol = getvolbyvid( vid )) == NULL ) {
        return( AFPERR_PARAM );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );

    if (( dir = dirlookup( vol, did )) == NULL ) {
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
        if (dbitmap) {
            ret = getdirparams(vol, dbitmap, ".", curdir,
                               &st, rbuf + 3 * sizeof( u_int16_t ), &buflen );
            if (ret != AFP_OK )
                return( ret );
        }
        /* this is a directory */
        *(rbuf + 2 * sizeof( u_int16_t )) = (char) FILDIRBIT_ISDIR;
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
    LOG(log_info, logtype_afpd, "end afp_getfildirparams:");
#endif /* DEBUG */

    return( AFP_OK );
}

/*
 * We can't use unix file's perm to support Apple's inherited protection modes.
 * If we aren't the file's owner we can't change its perms when moving it and smb
 * nfs,... don't even try.
*/
#define AFP_CHECK_ACCESS 

int check_access(char *path, int mode)
{
#ifdef AFP_CHECK_ACCESS
struct maccess ma;
char *p;

    p = ad_dir(path);
    if (!p)
       return -1;

    accessmode(p, &ma, curdir, NULL);
    if ((mode & OPENACC_WR) && !(ma.ma_user & AR_UWRITE))
        return -1;
    if ((mode & OPENACC_RD) && !(ma.ma_user & AR_UREAD))
        return -1;
#endif
    return 0;
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
    LOG(log_info, logtype_afpd, "begin afp_setfildirparams:");
#endif /* DEBUG */

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
    LOG(log_info, logtype_afpd, "end afp_setfildirparams:");
#endif /* DEBUG */

    return( rc );
}

/* -------------------------------------------- 
   Factorise some check on a pathname
*/
int check_name(const struct vol *vol, char *name)
{
    /* check for illegal characters in the unix filename */
    if (!wincheck(vol, name))
        return AFPERR_PARAM;

    if ((vol->v_flags & AFPVOL_NOHEX) && strchr(name, '/'))
        return AFPERR_PARAM;

    if (!validupath(vol, name))
        return AFPERR_EXIST;

    /* check for vetoed filenames */
    if (veto_file(vol->v_veto, name))
        return AFPERR_EXIST;
    return 0;
}

/* ------------------------- 
    move and rename sdir:oldname to curdir:newname in volume vol
   
    special care is needed for lock   
*/
static int moveandrename(vol, sdir, oldname, newname, isdir)
const struct vol	*vol;
struct dir	*sdir;
char        *oldname;
char        *newname;
int         isdir;
{
    char            *p;
    char            *upath;
    int             rc;
    struct stat     st;
    int             adflags;
    struct adouble	ad;
    struct adouble	*adp;
    struct ofork	*opened;

#ifdef CNID_DB
    cnid_t      id;
#endif /* CNID_DB */

    memset(&ad, 0, sizeof(ad));
    adp = &ad;
    adflags = 0;
    
    if (!isdir) {
#ifdef CNID_DB
        p = mtoupath(vol, oldname);
        id = cnid_get(vol->v_db, sdir->d_did, p, strlen(p));
#endif /* CNID_DB */
        p = ctoupath( vol, sdir, oldname );
        if ((opened = of_findname(vol, sdir, oldname))) {
            /* reuse struct adouble so it won't break locks */
            adp = opened->of_ad;
        }
    }
    else {
#ifdef CNID_DB
        id = sdir->d_did; /* we already have the CNID */
#endif /* CNID_DB */
        p = ctoupath( vol, sdir->d_parent, oldname );
        adflags = ADFLAGS_DIR;
    }
    /*
     * p now points to the full pathname of the source fs object.
     * 
     * we are in the dest folder so we need to use p for ad_open
    */
    
    if (!ad_open(p, ADFLAGS_HF |adflags, O_RDONLY, 0666, adp)) {
    u_int16_t bshort;

        ad_getattr(adp, &bshort);
        ad_close( adp, ADFLAGS_HF );
        if ((bshort & htons(ATTRBIT_NORENAME))) 
            return(AFPERR_OLOCK);
    }

    upath = mtoupath(vol, newname);
    if (0 != (rc = check_name(vol, upath))) {
            return  rc;
    }

    /* source == destination. we just silently accept this. */
    if (curdir == sdir) {
        if (strcmp(oldname, newname) == 0)
            return AFP_OK;

        /* deal with case insensitive, case-preserving filesystems. */
        if ((stat(upath, &st) == 0) && strdiacasecmp(oldname, newname))
            return AFPERR_EXIST;

    } else if (stat(upath, &st ) == 0)
        return AFPERR_EXIST;

    if ( !isdir ) {
        if (of_findname(vol, curdir, newname)) {
            rc = AFPERR_EXIST; /* was AFPERR_BUSY; */
        } else if ((rc = renamefile( p, upath, newname,
                                     vol_noadouble(vol), adp )) == AFP_OK) {
            /* if it's still open, rename the ofork as well. */
            rc = of_rename(vol, sdir, oldname, curdir, newname);

        }
    } else {
        rc = renamedir(p, upath, sdir, curdir, newname, vol_noadouble(vol));
    }
    if ( rc == AFP_OK ) {
#ifdef CNID_DB
        /* renaming may have moved the file/dir across a filesystem */
        if (stat(upath, &st) < 0)
            return AFPERR_MISC;

        /* fix up the catalog entry */
        cnid_update(vol->v_db, id, &st, curdir->d_did, upath, strlen(upath));
#endif /* CNID_DB */
    }

    return rc;
}

/* -------------------------------------------- */
int afp_rename(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    struct dir	*sdir;
    char		*path, *oldname, *newname;
    u_int32_t	did;
    int			plen;
    u_int16_t	vid;
    int         isdir = 0;
    int         rc;
#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_rename:");
#endif /* DEBUG */

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
    if (( sdir = dirsearch( vol, did )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    /* source pathname */
    if (( path = cname( vol, sdir, &ibuf )) == NULL ) {
        return( AFPERR_NOOBJ );
    }

    sdir = curdir;
    newname = obj->newtmp;
    oldname = obj->oldtmp;
    if ( *path != '\0' ) {
        strcpy(oldname, path); /* an extra copy for of_rename */
    }
    else {
        if ( sdir->d_parent == NULL ) { /* root directory */
            return( AFPERR_NORENAME );
        }
        /* move to destination dir */
        if ( movecwd( vol, sdir->d_parent ) < 0 ) {
            return( AFPERR_NOOBJ );
        }
        isdir = 1;
        strcpy(oldname, sdir->d_name);
    }

    /* another place where we know about the path type */
    if ( *ibuf++ != 2 ) {
        return( AFPERR_PARAM );
    }

    if (( plen = (unsigned char)*ibuf++ ) != 0 ) {
        strncpy( newname, ibuf, plen );
        newname[ plen ] = '\0';
        if (strlen(newname) != plen) {
            return( AFPERR_PARAM );
        }
    }
    else {
        return AFP_OK; /* newname == oldname same dir */
    }
    
    rc = moveandrename(vol, sdir, oldname, newname, isdir);

    if ( rc == AFP_OK ) {
        setvoltime(obj, vol );
    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end afp_rename:");
#endif /* DEBUG */

    return( rc );
}

/* ------------------------------- */
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
    LOG(log_info, logtype_afpd, "begin afp_delete:");
#endif /* DEBUG */ 

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
    } else if ((rc = deletefile( upath = mtoupath(vol, path ), 1)) == AFP_OK) {
#ifdef CNID_DB /* get rid of entry */
        cnid_t id = cnid_get(vol->v_db, curdir->d_did, upath, strlen(upath));
        cnid_delete(vol->v_db, id);
#endif /* CNID_DB */
    }
    if ( rc == AFP_OK ) {
        setvoltime(obj, vol );
    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end afp_delete:");
#endif /* DEBUG */

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

/* ------------------------- */
int afp_moveandrename(obj, ibuf, ibuflen, rbuf, rbuflen )
AFPObj      *obj;
char	*ibuf, *rbuf;
int		ibuflen, *rbuflen;
{
    struct vol	*vol;
    struct dir	*sdir, *ddir;
    int         isdir = 0;
    char	    *oldname, *newname;
    char        *path;
    int		did;
    int		plen;
    u_int16_t	vid;
    int         rc;
#ifdef DROPKLUDGE
    int		retvalue;
#endif /* DROPKLUDGE */

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin afp_moveandrename:");
#endif /* DEBUG */

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
        strcpy(oldname, path); /* an extra copy for of_rename */
    } else {
        isdir = 1;
        strcpy(oldname, sdir->d_name);
    }

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
        if (strlen(newname) != plen) {
            return( AFPERR_PARAM );
        }
    }
    else {
        strcpy(newname, oldname);
    }
    
    rc = moveandrename(vol, sdir, oldname, newname, isdir);

    if ( rc == AFP_OK ) {
#ifdef DROPKLUDGE
        if (vol->v_flags & AFPVOL_DROPBOX) {
            if (retvalue=matchfile2dirperms (newname, vol, did) != AFP_OK) {
                return retvalue;
            }
        }
        else
#endif /* DROPKLUDGE */
            if (!isdir) {
                char *upath = mtoupath(vol, newname);
                int  admode = ad_mode("", 0777);

                setfilmode(upath, admode, NULL);
                setfilmode(ad_path( upath, ADFLAGS_HF ), ad_hf_mode(admode), NULL);
            }
        setvoltime(obj, vol );
    }

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end afp_moveandrename:");
#endif /* DEBUG */

    return( rc );
}

int veto_file(const char*veto_str, const char*path)
/* given a veto_str like "abc/zxc/" and path "abc", return 1
 * veto_str should be '/' delimited
 * if path matches any one of the veto_str elements exactly, then 1 is returned
 * otherwise, 0 is returned.
 */
{
    int i;	/* index to veto_str */
    int j;	/* index to path */

    if ((veto_str == NULL) || (path == NULL))
        return 0;
    /*
    #ifdef DEBUG
    	LOG(log_debug, logtype_afpd, "veto_file \"%s\", \"%s\"", veto_str, path);
    #endif
    */
    for(i=0, j=0; veto_str[i] != '\0'; i++) {
        if (veto_str[i] == '/') {
            if ((j>0) && (path[j] == '\0'))
                return 1;
            j = 0;
        } else {
            if (veto_str[i] != path[j]) {
                while ((veto_str[i] != '/')
                        && (veto_str[i] != '\0'))
                    i++;
                j = 0;
                continue;
            }
            j++;
        }
    }
    return 0;
}

