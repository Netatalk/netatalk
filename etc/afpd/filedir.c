/*
 * $Id: filedir.c,v 1.62 2009-10-29 12:58:11 didg Exp $
 *
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
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

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <errno.h>
#include <sys/param.h>

#include <atalk/adouble.h>
#include <atalk/vfs.h>
#include <atalk/afp.h>
#include <atalk/util.h>
#include <atalk/cnid.h>
#include <atalk/logger.h>
#include <atalk/unix.h>

#include "directory.h"
#include "desktop.h"
#include "volume.h"
#include "fork.h"
#include "file.h"
#include "globals.h"
#include "filedir.h"
#include "unix.h"

#ifdef DROPKLUDGE
int matchfile2dirperms(
/* Since it's kinda' big; I decided against an
inline function */
    char	*upath,
    struct vol  *vol,
    int		did)
/* The below code changes the way file ownership is determined in the name of
fixing dropboxes.  It has known security problem.  See the netatalk FAQ for
more information */
{
    struct stat	st, sb;
    struct dir	*dir;
    char	*adpath;
    uid_t       uid;
    int         ret = AFP_OK;
#ifdef DEBUG
    LOG(log_info, logtype_afpd, "begin matchfile2dirperms:");
#endif /* DEBUG */

    if (stat(upath, &st ) < 0) {
        LOG(log_error, logtype_afpd, "Could not stat %s: %s", upath, strerror(errno));
        return AFPERR_NOOBJ ;
    }

    adpath = vol->vfs->ad_path( upath, ADFLAGS_HF );
    /* FIXME dirsearch doesn't move cwd to did ! */
    if (( dir = dirlookup( vol, did )) == NULL ) {
        LOG(log_error, logtype_afpd, "matchfile2dirperms: Unable to get directory info.");
        ret = AFPERR_NOOBJ;
    }
    else if (stat(".", &sb) < 0) {
        LOG(log_error, logtype_afpd,
            "matchfile2dirperms: Error checking directory \"%s\": %s",
            dir->d_m_name, strerror(errno));
        ret = AFPERR_NOOBJ;
    }
    else {
        uid=geteuid();
        if ( uid != sb.st_uid )
        {
            seteuid(0);
            if (lchown(upath, sb.st_uid, sb.st_gid) < 0)
            {
                LOG(log_error, logtype_afpd,
                    "matchfile2dirperms(%s): Error changing owner/gid: %s",
                    upath, strerror(errno));
                ret = AFPERR_ACCESS;
            }
            else if (chmod(upath,(st.st_mode&~default_options.umask)| S_IRGRP| S_IROTH) < 0)
            {
                LOG(log_error, logtype_afpd,
                    "matchfile2dirperms(%s): Error adding file read permissions: %s",
                    upath, strerror(errno));
                ret = AFPERR_ACCESS;
            }
            else if (lchown(adpath, sb.st_uid, sb.st_gid) < 0)
            {
                LOG(log_error, logtype_afpd,
                    "matchfile2dirperms(%s): Error changing AppleDouble owner/gid: %s",
                    adpath, strerror(errno));
                ret = AFPERR_ACCESS;
            }
            else if (chmod(adpath, (st.st_mode&~default_options.umask)| S_IRGRP| S_IROTH) < 0)
            {
                LOG(log_error, logtype_afpd,
                    "matchfile2dirperms(%s):  Error adding AD file read permissions: %s",
                    adpath, strerror(errno));
                ret = AFPERR_ACCESS;
            }
            seteuid(uid); 
        }
    } /* end else if stat success */

#ifdef DEBUG
    LOG(log_info, logtype_afpd, "end matchfile2dirperms:");
#endif /* DEBUG */
    return ret;
}
#endif

int afp_getfildirparams(AFPObj *obj _U_, char *ibuf, size_t ibuflen _U_, char *rbuf, size_t *rbuflen)
{
    struct stat		*st;
    struct vol		*vol;
    struct dir		*dir;
    u_int32_t           did;
    int			ret;
    size_t		buflen;
    u_int16_t		fbitmap, dbitmap, vid;
    struct path         *s_path;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        /* was AFPERR_PARAM but it helps OS 10.3 when a volume has been removed
         * from the list.
         */ 
        return( AFPERR_ACCESS );
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );

    if (NULL == ( dir = dirlookup( vol, did )) ) {
        return afp_errno;
    }

    memcpy( &fbitmap, ibuf, sizeof( fbitmap ));
    fbitmap = ntohs( fbitmap );
    ibuf += sizeof( fbitmap );
    memcpy( &dbitmap, ibuf, sizeof( dbitmap ));
    dbitmap = ntohs( dbitmap );
    ibuf += sizeof( dbitmap );

    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        return get_afp_errno(AFPERR_NOOBJ); 
    }

    st   = &s_path->st;
    if (!s_path->st_valid) {
        /* it's a dir and it should be there
         * because we chdir in it in cname or
         * it's curdir (maybe deleted, but then we can't know).
         * So we need to try harder.
         */
        of_statdir(vol, s_path);
    }
    if ( s_path->st_errno != 0 ) {
        return( AFPERR_NOOBJ );
    }

    buflen = 0;
    if (S_ISDIR(st->st_mode)) {
        if (dbitmap) {
            dir = s_path->d_dir;
            if (!dir) 
                return AFPERR_NOOBJ;

            ret = getdirparams(vol, dbitmap, s_path, dir,
                                 rbuf + 3 * sizeof( u_int16_t ), &buflen );
            if (ret != AFP_OK )
                return( ret );
        }
        /* this is a directory */
        *(rbuf + 2 * sizeof( u_int16_t )) = (char) FILDIRBIT_ISDIR;
    } else {
        if (fbitmap && AFP_OK != (ret = getfilparams(vol, fbitmap, s_path, curdir, 
                                            rbuf + 3 * sizeof( u_int16_t ), &buflen )) ) {
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

    return( AFP_OK );
}

int afp_setfildirparams(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct stat	*st;
    struct vol	*vol;
    struct dir	*dir;
    struct path *path;
    u_int16_t	vid, bitmap;
    int		did, rc;

    *rbuflen = 0;
    ibuf += 2;
    memcpy( &vid, ibuf, sizeof(vid));
    ibuf += sizeof( vid );

    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy( &did, ibuf, sizeof( did));
    ibuf += sizeof( did);

    if (NULL == ( dir = dirlookup( vol, did )) ) {
	return afp_errno;    
    }

    memcpy( &bitmap, ibuf, sizeof( bitmap ));
    bitmap = ntohs( bitmap );
    ibuf += sizeof( bitmap );

    if (NULL == ( path = cname( vol, dir, &ibuf ))) {
        return get_afp_errno(AFPERR_NOOBJ); 
    }

    st   = &path->st;
    if (!path->st_valid) {
        /* it's a dir and it should be there
         * because we chdir in it in cname
         */
        of_statdir(vol, path);
    }

    if ( path->st_errno != 0 ) {
        return( AFPERR_NOOBJ );
    }
    /*
     * If ibuf is odd, make it even.
     */
    if ((u_long)ibuf & 1 ) {
        ibuf++;
    }

    if (S_ISDIR(st->st_mode)) {
        rc = setdirparams(vol, path, bitmap, ibuf );
    } else {
        rc = setfilparams(vol, path, bitmap, ibuf );
    }
    if ( rc == AFP_OK ) {
        setvoltime(obj, vol );
    }

    return( rc );
}

/* -------------------------------------------- 
   Factorise some checks on a pathname
*/
int check_name(const struct vol *vol, char *name)
{
    /* check for illegal characters in the unix filename */
    if (!wincheck(vol, name))
        return AFPERR_PARAM;

    if ((vol->v_flags & AFPVOL_NOHEX) && strchr(name, '/'))
        return AFPERR_PARAM;

    if (!vol->vfs->vfs_validupath(vol, name)) {
        LOG(log_info, logtype_afpd, "check_name: illegal name: '%s'", name);
        return AFPERR_EXIST;
    }

    /* check for vetoed filenames */
    if (veto_file(vol->v_veto, name))
        return AFPERR_EXIST;
    return 0;
}

/* ------------------------- 
    move and rename sdir:oldname to curdir:newname in volume vol
   
    special care is needed for lock   
*/
static int moveandrename(const struct vol *vol, struct dir *sdir, char *oldname, char *newname, int isdir)
{
    char            *p;
    char            *upath;
    int             rc;
    struct stat     *st, nst;
    int             adflags;
    struct adouble	ad;
    struct adouble	*adp;
    struct ofork	*opened = NULL;
    struct path         path;
    cnid_t      id;

    ad_init(&ad, vol->v_adouble, vol->v_ad_options);
    adp = &ad;
    adflags = 0;
    
    if (!isdir) {
        p = mtoupath(vol, oldname, sdir->d_did, utf8_encoding());
        if (!p) { 
            return AFPERR_PARAM; /* can't convert */
        }
        id = cnid_get(vol->v_cdb, sdir->d_did, p, strlen(p));
        p = ctoupath( vol, sdir, oldname );
        if (!p) { 
            return AFPERR_PARAM; /* pathname too long */
        }
        path.st_valid = 0;
        path.u_name = p;
        if ((opened = of_findname(&path))) {
            /* reuse struct adouble so it won't break locks */
            adp = opened->of_ad;
        }
    }
    else {
        id = sdir->d_did; /* we already have the CNID */
        p = ctoupath( vol, sdir->d_parent, oldname );
        if (!p) {
            return AFPERR_PARAM;
        }
        adflags = ADFLAGS_DIR;
    }
    /*
     * p now points to the full pathname of the source fs object.
     * 
     * we are in the dest folder so we need to use p for ad_open
    */
    
    if (!ad_metadata(p, adflags, adp)) {
        u_int16_t bshort;

        ad_getattr(adp, &bshort);
        ad_close_metadata( adp);
        if ((bshort & htons(ATTRBIT_NORENAME))) 
            return(AFPERR_OLOCK);
    }

    if (NULL == (upath = mtoupath(vol, newname, curdir->d_did, utf8_encoding()))){ 
        return AFPERR_PARAM;
    }
    path.u_name = upath;
    st = &path.st;    
    if (0 != (rc = check_name(vol, upath))) {
            return  rc;
    }

    /* source == destination. we just silently accept this. */
    if ((!isdir && curdir == sdir) || (isdir && curdir == sdir->d_parent)) {
        if (strcmp(oldname, newname) == 0)
            return AFP_OK;

        if (stat(upath, st) == 0 || caseenumerate(vol, &path, curdir) == 0) {
            if (!stat(p, &nst) && !(nst.st_dev == st->st_dev && nst.st_ino == st->st_ino) ) {
                /* not the same file */
                return AFPERR_EXIST;
            }
            errno = 0;
        }
    } else if (stat(upath, st ) == 0 || caseenumerate(vol, &path, curdir) == 0)
        return AFPERR_EXIST;

    if ( !isdir ) {
        path.st_valid = 1;
        path.st_errno = errno;
        if (of_findname(&path)) {
            rc = AFPERR_EXIST; /* was AFPERR_BUSY; */
        } else {
            rc = renamefile(vol, p, upath, newname, adp );
            if (rc == AFP_OK)
                of_rename(vol, opened, sdir, oldname, curdir, newname);
        }
    } else {
        rc = renamedir(vol, p, upath, sdir, curdir, newname);
    }
    if ( rc == AFP_OK && id ) {
        /* renaming may have moved the file/dir across a filesystem */
        if (stat(upath, st) < 0)
            return AFPERR_MISC;

        /* fix up the catalog entry */
        cnid_update(vol->v_cdb, id, st, curdir->d_did, upath, strlen(upath));
    }

    return rc;
}

/* -------------------------------------------- */
int afp_rename(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct vol	*vol;
    struct dir	*sdir;
    char        *oldname, *newname;
    struct path *path;
    u_int32_t	did;
    int         plen;
    u_int16_t	vid;
    int         isdir = 0;
    int         rc;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( did );
    if (NULL == ( sdir = dirlookup( vol, did )) ) {
	return afp_errno;    
    }

    /* source pathname */
    if (NULL == ( path = cname( vol, sdir, &ibuf )) ) {
        return get_afp_errno(AFPERR_NOOBJ); 
    }

    sdir = curdir;
    newname = obj->newtmp;
    oldname = obj->oldtmp;
    isdir = path_isadir(path);
    if ( *path->m_name != '\0' ) {
        strcpy(oldname, path->m_name); /* an extra copy for of_rename */
        if (isdir) {
            /* curdir parent dir, need to move sdir back */
            sdir = path->d_dir;
        }
    }
    else {
        if ( sdir->d_parent == NULL ) { /* root directory */
            return( AFPERR_NORENAME );
        }
        /* move to destination dir */
        if ( movecwd( vol, sdir->d_parent ) < 0 ) {
            return afp_errno;
        }
        strcpy(oldname, sdir->d_m_name);
    }

    /* another place where we know about the path type */
    if ((plen = copy_path_name(vol, newname, ibuf)) < 0) {
        return( AFPERR_PARAM );
    }

    if (!plen) {
        return AFP_OK; /* newname == oldname same dir */
    }
    
    rc = moveandrename(vol, sdir, oldname, newname, isdir);

    if ( rc == AFP_OK ) {
        setvoltime(obj, vol );
    }

    return( rc );
}

/* ------------------------------- */
int afp_delete(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct vol		*vol;
    struct dir		*dir;
    struct path         *s_path;
    char		*upath;
    int			did, rc;
    u_int16_t		vid;

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( int );
    if (NULL == ( dir = dirlookup( vol, did )) ) {
	return afp_errno;    
    }

    if (NULL == ( s_path = cname( vol, dir, &ibuf )) ) {
        return get_afp_errno(AFPERR_NOOBJ); 
    }

    upath = s_path->u_name;
    if ( path_isadir( s_path) ) {
    	if (*s_path->m_name != '\0') {
    	    rc = AFPERR_ACCESS;
    	}
    	else {
            rc = deletecurdir( vol);
        }
    } else if (of_findname(s_path)) {
        rc = AFPERR_BUSY;
    } else {
        /* it's a file st_valid should always be true
         * only test for ENOENT because EACCES needs
         * to read meta data in deletefile
         */
        if (s_path->st_valid && s_path->st_errno == ENOENT) {
            rc = AFPERR_NOOBJ;
        }
        else {
            rc = deletefile(vol, upath, 1);
        }
    }
    if ( rc == AFP_OK ) {
	curdir->offcnt--;
        setvoltime(obj, vol );
    }

    return( rc );
}
/* ------------------------ */
char *absupath(const struct vol *vol, struct dir *dir, char *u)
{
    struct dir	*d;
    static char	path[ MAXPATHLEN + 1];
    char	*p;
    int		len;

    if (u == NULL)
        return NULL;
        
    p = path + sizeof( path ) - 1;
    *p = '\0';
    len = strlen( u );
    p -= len;
    memcpy( p, u, len );
    if (dir) for ( d = dir; d->d_parent; d = d->d_parent ) {
        u = d->d_u_name;
        len = strlen( u );
        if (p -len -1 < path) {
            /* FIXME 
               rather rare so LOG error and/or client message ?
            */
            return NULL;
        }
        *--p = '/';
        p -= len;
        memcpy( p, u, len );
    }
    len = strlen( vol->v_path );
    if (p -len -1 < path) {
        return NULL;
    }
    *--p = '/';
    p -= len;
    memcpy( p, vol->v_path, len );

    return( p );
}

/* ------------------------
 * FIXME dir could be NULL
*/
char *ctoupath(const struct vol *vol, struct dir *dir, char *name)
{
    return absupath(vol, dir, mtoupath(vol, name, dir->d_did, utf8_encoding()));
}

/* ------------------------- */
int afp_moveandrename(AFPObj *obj, char *ibuf, size_t ibuflen _U_, char *rbuf _U_, size_t *rbuflen)
{
    struct vol	*vol;
    struct dir	*sdir, *ddir;
    int         isdir;
    char	*oldname, *newname;
    struct path *path;
    int		did;
    int		pdid;
    int         plen;
    u_int16_t	vid;
    int         rc;
#ifdef DROPKLUDGE
    int		retvalue;
#endif /* DROPKLUDGE */

    *rbuflen = 0;
    ibuf += 2;

    memcpy( &vid, ibuf, sizeof( vid ));
    ibuf += sizeof( vid );
    if (NULL == ( vol = getvolbyvid( vid )) ) {
        return( AFPERR_PARAM );
    }

    if (vol->v_flags & AFPVOL_RO)
        return AFPERR_VLOCK;

    /* source did followed by dest did */
    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( int );
    if (NULL == ( sdir = dirlookup( vol, did )) ) {
        return afp_errno; /* was AFPERR_PARAM */
    }

    memcpy( &did, ibuf, sizeof( did ));
    ibuf += sizeof( int );

    /* source pathname */
    if (NULL == ( path = cname( vol, sdir, &ibuf )) ) {
        return get_afp_errno(AFPERR_NOOBJ); 
    }

    sdir = curdir;
    newname = obj->newtmp;
    oldname = obj->oldtmp;
    
    isdir = path_isadir(path);
    if ( *path->m_name != '\0' ) {
        if (isdir) {
            sdir = path->d_dir;
	}
        strcpy(oldname, path->m_name); /* an extra copy for of_rename */
    } else {
        strcpy(oldname, sdir->d_m_name);
    }

    /* get the destination directory */
    if (NULL == ( ddir = dirlookup( vol, did )) ) {
        return afp_errno; /*  was AFPERR_PARAM */
    }
    if (NULL == ( path = cname( vol, ddir, &ibuf ))) {
        return( AFPERR_NOOBJ );
    }
    pdid = curdir->d_did;
    if ( *path->m_name != '\0' ) {
        return path_error(path, AFPERR_NOOBJ);
    }

    /* one more place where we know about path type */
    if ((plen = copy_path_name(vol, newname, ibuf)) < 0) {
        return( AFPERR_PARAM );
    }

    if (!plen) {
        strcpy(newname, oldname);
    }

    rc = moveandrename(vol, sdir, oldname, newname, isdir);

    if ( rc == AFP_OK ) {
        char *upath = mtoupath(vol, newname, pdid, utf8_encoding());
        
        if (NULL == upath) {
            return AFPERR_PARAM;
        }
        curdir->offcnt++;
        sdir->offcnt--;
#ifdef DROPKLUDGE
        if (vol->v_flags & AFPVOL_DROPBOX) {
            /* FIXME did is not always the source id */
            if ((retvalue=matchfile2dirperms (upath, vol, did)) != AFP_OK) {
                return retvalue;
            }
        }
        else
#endif /* DROPKLUDGE */
            /* if unix priv don't try to match perm with dest folder */
            if (!isdir && !vol_unix_priv(vol)) {
                int  admode = ad_mode("", 0777) | vol->v_fperm;

                setfilmode(upath, admode, NULL, vol->v_umask);
                vol->vfs->vfs_setfilmode(vol, upath, admode, NULL);
            }
        setvoltime(obj, vol );
    }

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

    for(i=0, j=0; veto_str[i] != '\0'; i++) {
        if (veto_str[i] == '/') {
            if ((j>0) && (path[j] == '\0')) {
                LOG(log_info, logtype_afpd, "vetoed file:'%s'", path);
                return 1;
            }
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

