/*
    Copyright (c) 2004 Didier Gautheron
 
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef STDC_HEADERS
#include <string.h>
#endif

#include <stdio.h>
    
#include <atalk/adouble.h>
#include <atalk/logger.h>
#include <atalk/util.h>

#include "directory.h"
#include "volume.h"
#include "unix.h"

#ifdef HAVE_NFSv4_ACLS
extern int remove_acl(const char *name);
#endif

struct perm {
    uid_t uid;
    gid_t gid;
};

typedef int (*rf_loop)(struct dirent *, char *, void *, int );

/* ----------------------------- */
static int 
for_each_adouble(const char *from, const char *name, rf_loop fn, void *data, int flag)
{
    char            buf[ MAXPATHLEN + 1];
    char            *m;
    DIR             *dp;
    struct dirent   *de;
    int             ret;
    

    if (NULL == ( dp = opendir( name)) ) {
        if (!flag) {
            LOG(log_error, logtype_afpd, "%s: opendir %s: %s", from, fullpathname(name),strerror(errno) );
            return -1;
        }
        return 0;
    }
    strlcpy( buf, name, sizeof(buf));
    strlcat( buf, "/", sizeof(buf) );
    m = strchr( buf, '\0' );
    ret = 0;
    while ((de = readdir(dp))) {
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) {
                continue;
        }
        
        strlcat(buf, de->d_name, sizeof(buf));
        if (fn && (ret = fn(de, buf, data, flag))) {
           closedir(dp);
           return ret;
        }
        *m = 0;
    }
    closedir(dp);
    return ret;
}

/* ------------------------------ */
static int ads_chown_loop(struct dirent *de _U_, char *name, void *data, int flag _U_)
{
    struct perm   *owner  = data;
    
    if (chown( name , owner->uid, owner->gid ) < 0) {
        return -1;
    }
    return 0;
}

static int RF_chown_ads(const struct vol *vol, const char *path, uid_t uid, gid_t gid)

{
    struct        stat st;
    char          *ad_p;
    struct perm   owner;
    
    owner.uid = uid;
    owner.gid = gid;


    ad_p = ad_dir(vol->vfs->ad_path(path, ADFLAGS_HF ));

    if ( stat( ad_p, &st ) < 0 ) {
	/* ignore */
        return 0;
    }
    
    if (chown( ad_p, uid, gid ) < 0) {
    	return -1;
    }
    return for_each_adouble("chown_ads", ad_p, ads_chown_loop, &owner, 1);
}

/* --------------------------------- */
static int deletecurdir_ads1_loop(struct dirent *de _U_, char *name, void *data _U_, int flag _U_)
{
    return netatalk_unlink(name);
}

static int ads_delete_rf(char *name) 
{
    int err;

    if ((err = for_each_adouble("deletecurdir", name, deletecurdir_ads1_loop, NULL, 1))) 
        return err;
    /* FIXME 
     * it's a problem for a nfs mounted folder, there's .nfsxxx around
     * for linux the following line solve it.
     * but it could fail if rm .nfsxxx  create a new .nfsyyy :(
    */
    if ((err = for_each_adouble("deletecurdir", name, deletecurdir_ads1_loop, NULL, 1))) 
        return err;
    return netatalk_rmdir(name);
}

static int deletecurdir_ads_loop(struct dirent *de, char *name, void *data _U_, int flag _U_)
{
    struct stat st;
    
    /* bail if the file exists in the current directory.
     * note: this will not fail with dangling symlinks */
    
    if (stat(de->d_name, &st) == 0) {
        return AFPERR_DIRNEMPT;
    }
    return ads_delete_rf(name);
}

static int RF_deletecurdir_ads(const struct vol *vol _U_)
{
    int err;
    
    /* delete stray .AppleDouble files. this happens to get .Parent files as well. */
    if ((err = for_each_adouble("deletecurdir", ".AppleDouble", deletecurdir_ads_loop, NULL, 1))) 
        return err;
    return netatalk_rmdir( ".AppleDouble" );
}

/* ------------------- */
struct set_mode {
    mode_t mode;
    struct stat *st;
};

static int ads_setfilmode_loop(struct dirent *de _U_, char *name, void *data, int flag _U_)
{
    struct set_mode *param = data;

    return setfilmode(name, param->mode, param->st);
}

static int ads_setfilmode(const char * name, mode_t mode, struct stat *st)
{
    mode_t file_mode = ad_hf_mode(mode);
    mode_t dir_mode = file_mode;
    struct set_mode param;

    if ((dir_mode & (S_IRUSR | S_IWUSR )))
        dir_mode |= S_IXUSR;
    if ((dir_mode & (S_IRGRP | S_IWGRP )))
        dir_mode |= S_IXGRP;
    if ((dir_mode & (S_IROTH | S_IWOTH )))
        dir_mode |= S_IXOTH;	
    
	/* change folder */
	dir_mode |= DIRBITS;
    if (dir_rx_set(dir_mode)) {
        if (chmod( name,  dir_mode ) < 0)
            return -1;
    }
    param.st = st;
    param.mode = file_mode;
    if (for_each_adouble("setfilmode_ads", name, ads_setfilmode_loop, &param, 0) < 0)
        return -1;

    if (!dir_rx_set(dir_mode)) {
        if (chmod( name,  dir_mode ) < 0)
            return -1;
    }

    return 0;
}

static int RF_setfilmode_ads(const struct vol *vol, const char * name, mode_t mode, struct stat *st)
{
    return ads_setfilmode(ad_dir(vol->vfs->ad_path( name, ADFLAGS_HF )), mode, st);
}

/* ------------------- */
static int RF_setdirunixmode_ads(const struct vol *vol, const char * name, mode_t mode, struct stat *st)
{
    char *adouble = vol->vfs->ad_path( name, ADFLAGS_DIR );
    char   ad_p[ MAXPATHLEN + 1];
    int dropbox = vol->v_flags;

    strlcpy(ad_p,ad_dir(adouble), MAXPATHLEN + 1);

    if (dir_rx_set(mode)) {

        /* .AppleDouble */
        if (stickydirmode(ad_dir(ad_p), DIRBITS | mode, dropbox) < 0) 
            return -1;

        /* .AppleDouble/.Parent */
        if (stickydirmode(ad_p, DIRBITS | mode, dropbox) < 0) 
            return -1;
    }

    if (ads_setfilmode(ad_dir(vol->vfs->ad_path( name, ADFLAGS_DIR)), mode, st) < 0)
        return -1;

    if (!dir_rx_set(mode)) {
        if (stickydirmode(ad_p, DIRBITS | mode, dropbox) < 0) 
            return  -1 ;
        if (stickydirmode(ad_dir(ad_p), DIRBITS | mode, dropbox) < 0) 
            return -1;
    }
    return 0;
}

/* ------------------- */
struct dir_mode {
    mode_t mode;
    int    dropbox;
};

static int setdirmode_ads_loop(struct dirent *de _U_, char *name, void *data, int flag)
{

    struct dir_mode *param = data;
    int    ret = 0; /* 0 ignore error, -1 */

    if (dir_rx_set(param->mode)) {
        if (stickydirmode(name, DIRBITS | param->mode, param->dropbox) < 0) {
            if (flag) {
                return 0;
            }
            return ret;
        }
    }
    if (ads_setfilmode(name, param->mode, NULL) < 0)
        return ret;

    if (!dir_rx_set(param->mode)) {
        if (stickydirmode(name, DIRBITS | param->mode, param->dropbox) < 0) {
            if (flag) {
                return 0;
            }
            return ret;
        }
    }
    return 0;
}

static int RF_setdirmode_ads(const struct vol *vol, const char * name, mode_t mode, struct stat *st _U_)
{
    char *adouble = vol->vfs->ad_path( name, ADFLAGS_DIR );
    char   ad_p[ MAXPATHLEN + 1];
    struct dir_mode param;

    param.mode = mode;
    param.dropbox = vol->v_flags;

    strlcpy(ad_p,ad_dir(adouble), sizeof(ad_p));

    if (dir_rx_set(mode)) {
        /* .AppleDouble */
        if (stickydirmode(ad_dir(ad_p), DIRBITS | mode, param.dropbox) < 0) 
            return -1;
    }

    if (for_each_adouble("setdirmode_ads", ad_dir(ad_p), setdirmode_ads_loop, &param, vol_noadouble(vol)))
        return -1;

    if (!dir_rx_set(mode)) {
        if (stickydirmode(ad_dir(ad_p), DIRBITS | mode, param.dropbox) < 0 ) 
            return -1;
    }
    return 0;
}

/* ------------------- */
static int setdirowner_ads1_loop(struct dirent *de _U_, char *name, void *data, int flag _U_)
{
    struct perm   *owner  = data;

    if ( chown( name, owner->uid, owner->gid ) < 0 && errno != EPERM ) {
         LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d %s: %s",
                owner->uid, owner->gid, fullpathname(name), strerror(errno) );
         /* return ( -1 ); Sometimes this is okay */
    }
    return 0;
}

static int setdirowner_ads_loop(struct dirent *de _U_, char *name, void *data, int flag)
{
    struct perm   *owner  = data;

    if (for_each_adouble("setdirowner", name, setdirowner_ads1_loop, data, flag) < 0)
        return -1;

    if ( chown( name, owner->uid, owner->gid ) < 0 && errno != EPERM ) {
         LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d %s: %s",
                owner->uid, owner->gid, fullpathname(name), strerror(errno) );
         /* return ( -1 ); Sometimes this is okay */
    }
    return 0;
}

static int RF_setdirowner_ads(const struct vol *vol, const char *name, uid_t uid, gid_t gid)
{
    int           noadouble = vol_noadouble(vol);
    char          adouble_p[ MAXPATHLEN + 1];
    struct stat   st;
    struct perm   owner;
    
    owner.uid = uid;
    owner.gid = gid;

    strlcpy(adouble_p, ad_dir(vol->vfs->ad_path( name, ADFLAGS_DIR )), sizeof(adouble_p));

    if (for_each_adouble("setdirowner", ad_dir(adouble_p), setdirowner_ads_loop, &owner, noadouble)) 
        return -1;

    /*
     * We cheat: we know that chown doesn't do anything.
     */
    if ( stat( ".AppleDouble", &st ) < 0) {
        if (errno == ENOENT && noadouble)
            return 0;
        LOG(log_error, logtype_afpd, "setdirowner: stat %s: %s", fullpathname(".AppleDouble"), strerror(errno) );
        return -1;
    }
    if ( gid && gid != st.st_gid && chown( ".AppleDouble", uid, gid ) < 0 && errno != EPERM ) {
        LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d %s: %s",
            uid, gid,fullpathname(".AppleDouble"), strerror(errno) );
        /* return ( -1 ); Sometimes this is okay */
    }
    return 0;
}

/* ------------------- */
static int RF_deletefile_ads(const struct vol *vol, const char *file )
{
    char *ad_p = ad_dir(vol->vfs->ad_path(file, ADFLAGS_HF ));

    return ads_delete_rf(ad_p);
}

/* --------------------------- */
int RF_renamefile_ads(const struct vol *vol, const char *src, const char *dst)
{
    char  adsrc[ MAXPATHLEN + 1];
    int   err = 0;

    strcpy( adsrc, ad_dir(vol->vfs->ad_path( src, 0 )));
    if (unix_rename( adsrc, ad_dir(vol->vfs->ad_path( dst, 0 ))) < 0) {
        struct stat st;

        err = errno;
        if (errno == ENOENT) {
	        struct adouble    ad;

            if (stat(adsrc, &st)) /* source has no ressource fork, */
                return 0;
            
            /* We are here  because :
             * -there's no dest folder. 
             * -there's no .AppleDouble in the dest folder.
             * if we use the struct adouble passed in parameter it will not
             * create .AppleDouble if the file is already opened, so we
             * use a diff one, it's not a pb,ie it's not the same file, yet.
             */
            ad_init(&ad, vol->v_adouble, vol->v_ad_options); 
            if (!ad_open(dst, ADFLAGS_HF, O_RDWR | O_CREAT, 0666, &ad)) {
            	ad_close(&ad, ADFLAGS_HF);

            	/* We must delete it */
            	RF_deletefile_ads(vol, dst );
    	        if (!unix_rename( adsrc, ad_dir(vol->vfs->ad_path( dst, 0 ))) ) 
                   err = 0;
                else 
                   err = errno;
            }
            else { /* it's something else, bail out */
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

/* ===================================================
 classic adouble format 
*/

static int netatalk_name(const char *name)
{
    return strcasecmp(name,".AppleDB") &&
        strcasecmp(name,".AppleDouble") &&
        strcasecmp(name,".AppleDesktop");
}

static int validupath_adouble(const struct vol *vol, const char *name) 
{
    return (vol->v_flags & AFPVOL_USEDOTS) ? 
        netatalk_name(name) && strcasecmp(name,".Parent"): name[0] != '.';
}                                           

/* ----------------- */
static int RF_chown_adouble(const struct vol *vol, const char *path, uid_t uid, gid_t gid)

{
    struct stat st;
    char        *ad_p;

    ad_p = vol->vfs->ad_path(path, ADFLAGS_HF );

    if ( stat( ad_p, &st ) < 0 )
        return 0; /* ignore */

    return chown( ad_p, uid, gid );
}

/* ----------------- */
int RF_renamedir_adouble(const struct vol *vol _U_, const char *oldpath _U_, const char *newpath _U_)
{
    return 0;
}

/* ----------------- */
static int deletecurdir_adouble_loop(struct dirent *de, char *name, void *data _U_, int flag _U_)
{
    struct stat st;
    int         err;
    
    /* bail if the file exists in the current directory.
     * note: this will not fail with dangling symlinks */
    
    if (stat(de->d_name, &st) == 0)
        return AFPERR_DIRNEMPT;

    if ((err = netatalk_unlink(name)))
        return err;

    return 0;
}

static int RF_deletecurdir_adouble(const struct vol *vol _U_)
{
    int err;

    /* delete stray .AppleDouble files. this happens to get .Parent files
       as well. */
    if ((err = for_each_adouble("deletecurdir", ".AppleDouble", deletecurdir_adouble_loop, NULL, 1))) 
        return err;
    return netatalk_rmdir( ".AppleDouble" );
}

/* ----------------- */
static int adouble_setfilmode(const char * name, mode_t mode, struct stat *st)
{
    return setfilmode(name, ad_hf_mode(mode), st);
}

static int RF_setfilmode_adouble(const struct vol *vol, const char * name, mode_t mode, struct stat *st)
{
    return adouble_setfilmode(vol->vfs->ad_path( name, ADFLAGS_HF ), mode, st);
}

/* ----------------- */
static int RF_setdirunixmode_adouble(const struct vol *vol, const char * name, mode_t mode, struct stat *st)
{
    char *adouble = vol->vfs->ad_path( name, ADFLAGS_DIR );
    int  dropbox = vol->v_flags;

    if (dir_rx_set(mode)) {
        if (stickydirmode(ad_dir(adouble), DIRBITS | mode, dropbox) < 0 ) 
            return -1;
    }

    if (adouble_setfilmode(vol->vfs->ad_path( name, ADFLAGS_DIR ), mode, st) < 0) 
        return -1;

    if (!dir_rx_set(mode)) {
        if (stickydirmode(ad_dir(adouble), DIRBITS | mode, dropbox) < 0 ) 
            return  -1 ;
    }
    return 0;
}

/* ----------------- */
static int setdirmode_adouble_loop(struct dirent *de _U_, char *name, void *data, int flag)
{
    mode_t hf_mode = *(mode_t *)data;
    struct stat st;

    if ( stat( name, &st ) < 0 ) {
        if (flag)
            return 0;
        LOG(log_error, logtype_afpd, "setdirmode: stat %s: %s", name, strerror(errno) );
    }
    else if (!S_ISDIR(st.st_mode)) {
        if (setfilmode(name, hf_mode , &st) < 0) {
               /* FIXME what do we do then? */
        }
    }
    return 0;
}

static int RF_setdirmode_adouble(const struct vol *vol, const char * name, mode_t mode, struct stat *st _U_)
{
    int   dropbox = vol->v_flags;
    mode_t hf_mode = ad_hf_mode(mode);
    char  *adouble = vol->vfs->ad_path( name, ADFLAGS_DIR );
    char  *adouble_p = ad_dir(adouble);

    if (dir_rx_set(mode)) {
        if (stickydirmode(ad_dir(adouble), DIRBITS | mode, dropbox) < 0) 
            return -1;
    }

    if (for_each_adouble("setdirmode", adouble_p, setdirmode_adouble_loop, &hf_mode, vol_noadouble(vol)))
        return -1;

    if (!dir_rx_set(mode)) {
        if (stickydirmode(ad_dir(adouble), DIRBITS | mode, dropbox) < 0) 
            return  -1 ;
    }
    return 0;
}

/* ----------------- */
static int setdirowner_adouble_loop(struct dirent *de _U_, char *name, void *data, int flag _U_)
{
    struct perm   *owner  = data;

    if ( chown( name, owner->uid, owner->gid ) < 0 && errno != EPERM ) {
         LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d %s: %s",
                owner->uid, owner->gid, fullpathname(name), strerror(errno) );
         /* return ( -1 ); Sometimes this is okay */
    }
    return 0;
}

static int RF_setdirowner_adouble(const struct vol *vol, const char *name, uid_t uid, gid_t gid)

{
    int           noadouble = vol_noadouble(vol);
    char          *adouble_p;
    struct stat   st;
    struct perm   owner;
    
    owner.uid = uid;
    owner.gid = gid;

    adouble_p = ad_dir(vol->vfs->ad_path( name, ADFLAGS_DIR ));

    if (for_each_adouble("setdirowner", adouble_p, setdirowner_adouble_loop, &owner, noadouble)) 
        return -1;

    /*
     * We cheat: we know that chown doesn't do anything.
     */
    if ( stat( ".AppleDouble", &st ) < 0) {
        if (errno == ENOENT && noadouble)
            return 0;
        LOG(log_error, logtype_afpd, "setdirowner: stat %s: %s", fullpathname(".AppleDouble"), strerror(errno) );
        return -1;
    }
    if ( gid && gid != st.st_gid && chown( ".AppleDouble", uid, gid ) < 0 && errno != EPERM ) {
        LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d %s: %s",
            uid, gid,fullpathname(".AppleDouble"), strerror(errno) );
        /* return ( -1 ); Sometimes this is okay */
    }
    return 0;
}

/* ----------------- */
static int RF_deletefile_adouble(const struct vol *vol, const char *file )
{
	return netatalk_unlink(vol->vfs->ad_path( file, ADFLAGS_HF));
}

/* ----------------- */
int RF_renamefile_adouble(const struct vol *vol, const char *src, const char *dst)
{
    char  adsrc[ MAXPATHLEN + 1];
    int   err = 0;

    strcpy( adsrc, vol->vfs->ad_path( src, 0 ));
    if (unix_rename( adsrc, vol->vfs->ad_path( dst, 0 )) < 0) {
        struct stat st;

        err = errno;
        if (errno == ENOENT) {
	        struct adouble    ad;

            if (stat(adsrc, &st)) /* source has no ressource fork, */
                return 0;
            
            /* We are here  because :
             * -there's no dest folder. 
             * -there's no .AppleDouble in the dest folder.
             * if we use the struct adouble passed in parameter it will not
             * create .AppleDouble if the file is already opened, so we
             * use a diff one, it's not a pb,ie it's not the same file, yet.
             */
            ad_init(&ad, vol->v_adouble, vol->v_ad_options); 
            if (!ad_open(dst, ADFLAGS_HF, O_RDWR | O_CREAT, 0666, &ad)) {
            	ad_close(&ad, ADFLAGS_HF);
    	        if (!unix_rename( adsrc, vol->vfs->ad_path( dst, 0 )) ) 
                   err = 0;
                else 
                   err = errno;
            }
            else { /* it's something else, bail out */
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

#ifdef HAVE_NFSv4_ACLS
static int RF_acl(const struct vol *vol, const char *path, int cmd, int count, ace_t *aces)
{
    static char buf[ MAXPATHLEN + 1];
    struct stat st;

    if ((stat(path, &st)) != 0)
	return -1;
    if (S_ISDIR(st.st_mode)) {
	if ((snprintf(buf, MAXPATHLEN, "%s/.AppleDouble",path)) < 0)
	    return -1;
	/* set acl on .AppleDouble dir first */
	if ((acl(buf, cmd, count, aces)) != 0)
	    return -1;
	/* now set ACL on ressource fork */
	if ((acl(vol->vfs->ad_path(path, ADFLAGS_DIR), cmd, count, aces)) != 0)
	    return -1;
    } else
	/* set ACL on ressource fork */
	if ((acl(vol->vfs->ad_path(path, ADFLAGS_HF), cmd, count, aces)) != 0)
	    return -1;

    return 0;
}

static int RF_remove_acl(const struct vol *vol, const char *path, int dir)
{
    int ret;
    static char buf[ MAXPATHLEN + 1];

    if (dir) {
	if ((snprintf(buf, MAXPATHLEN, "%s/.AppleDouble",path)) < 0)
	    return AFPERR_MISC;
	/* remove ACL from .AppleDouble/.Parent first */
	if ((ret = remove_acl(vol->vfs->ad_path(path, ADFLAGS_DIR))) != AFP_OK)
	    return ret;
	/* now remove from .AppleDouble dir */
	if ((ret = remove_acl(buf)) != AFP_OK)
	    return ret;
    } else
	/* remove ACL from ressource fork */
	if ((ret = remove_acl(vol->vfs->ad_path(path, ADFLAGS_HF))) != AFP_OK)
	    return ret;

    return AFP_OK;
}
#endif

struct vfs_ops netatalk_adouble = {
    /* ad_path:           */ ad_path,
    /* validupath:        */ validupath_adouble,
    /* rf_chown:          */ RF_chown_adouble,
    /* rf_renamedir:      */ RF_renamedir_adouble,
    /* rf_deletecurdir:   */ RF_deletecurdir_adouble,
    /* rf_setfilmode:     */ RF_setfilmode_adouble,
    /* rf_setdirmode:     */ RF_setdirmode_adouble,
    /* rf_setdirunixmode: */ RF_setdirunixmode_adouble,
    /* rf_setdirowner:    */ RF_setdirowner_adouble,
    /* rf_deletefile:     */ RF_deletefile_adouble,
    /* rf_renamefile:     */ RF_renamefile_adouble,
#ifdef HAVE_NFSv4_ACLS
    /* rf_acl:            */ RF_acl,
    /* rf_remove_acl      */ RF_remove_acl
#endif
};

/* =======================================
 osx adouble format 
 */
static int validupath_osx(const struct vol *vol, const char *name) 
{
    return strncmp(name,"._", 2) && (
      (vol->v_flags & AFPVOL_USEDOTS) ? netatalk_name(name): name[0] != '.');
}             

/* ---------------- */
int RF_renamedir_osx(const struct vol *vol, const char *oldpath, const char *newpath)
{
    /* We simply move the corresponding ad file as well */
    char   tempbuf[258]="._";
    return rename(vol->vfs->ad_path(oldpath,0),strcat(tempbuf,newpath));
}

/* ---------------- */
int RF_deletecurdir_osx(const struct vol *vol)
{
    return netatalk_unlink( vol->vfs->ad_path(".",0) );
}

/* ---------------- */
static int RF_setdirunixmode_osx(const struct vol *vol, const char * name, mode_t mode, struct stat *st)
{
    return adouble_setfilmode(vol->vfs->ad_path( name, ADFLAGS_DIR ), mode, st);
}

/* ---------------- */
static int 
RF_setdirmode_osx(const struct vol *vol _U_, const char *name _U_, mode_t mode _U_, struct stat *st _U_)
{
    return 0;
}

/* ---------------- */
static int 
RF_setdirowner_osx(const struct vol *vol _U_, const char *path _U_, uid_t uid _U_, gid_t gid _U_)
{
	return 0;
}

/* ---------------- */
int RF_renamefile_osx(const struct vol *vol, const char *src, const char *dst)
{
    char  adsrc[ MAXPATHLEN + 1];
    int   err = 0;

    strcpy( adsrc, vol->vfs->ad_path( src, 0 ));

    if (unix_rename( adsrc, vol->vfs->ad_path( dst, 0 )) < 0) {
        struct stat st;

        err = errno;
        if (errno == ENOENT && stat(adsrc, &st)) /* source has no ressource fork, */
            return 0;
        errno = err;
        return -1;
    }
    return 0;
}

struct vfs_ops netatalk_adouble_osx = {
    /* ad_path:          */ ad_path_osx,
    /* validupath:       */ validupath_osx,
    /* rf_chown:         */ RF_chown_adouble,
    /* rf_renamedir:     */ RF_renamedir_osx,
    /* rf_deletecurdir:  */ RF_deletecurdir_osx,
    /* rf_setfilmode:    */ RF_setfilmode_adouble,
    /* rf_setdirmode:    */ RF_setdirmode_osx,
    /* rf_setdirunixmode:*/ RF_setdirunixmode_osx,
    /* rf_setdirowner:   */ RF_setdirowner_osx,
    /* rf_deletefile:    */ RF_deletefile_adouble,
    /* rf_renamefile:    */ RF_renamefile_osx,
};

/* =======================================
   samba ads format 
 */
struct vfs_ops netatalk_adouble_ads = {
    /* ad_path:          */ ad_path_ads,
    /* validupath:       */ validupath_adouble,
    /* rf_chown:         */ RF_chown_ads,
    /* rf_renamedir:     */ RF_renamedir_adouble,
    /* rf_deletecurdir:  */ RF_deletecurdir_ads,
    /* rf_setfilmode:    */ RF_setfilmode_ads,
    /* rf_setdirmode:    */ RF_setdirmode_ads,
    /* rf_setdirunixmode:*/ RF_setdirunixmode_ads,
    /* rf_setdirowner:   */ RF_setdirowner_ads,
    /* rf_deletefile:    */ RF_deletefile_ads,
    /* rf_renamefile:    */ RF_renamefile_ads,
};

/* =======================================
   samba sfm format
   ad_path shouldn't be set here
 */
struct vfs_ops netatalk_adouble_sfm = {
    /* ad_path:          */ ad_path_sfm,
    /* validupath:       */ validupath_adouble,
    /* rf_chown:         */ RF_chown_ads,
    /* rf_renamedir:     */ RF_renamedir_adouble,
    /* rf_deletecurdir:  */ RF_deletecurdir_ads,
    /* rf_setfilmode:    */ RF_setfilmode_ads,
    /* rf_setdirmode:    */ RF_setdirmode_ads,
    /* rf_setdirunixmode:*/ RF_setdirunixmode_ads,
    /* rf_setdirowner:   */ RF_setdirowner_ads,
    /* rf_deletefile:    */ RF_deletefile_ads,
    /* rf_renamefile:    */ RF_renamefile_ads,
};

/* ---------------- */
void initvol_vfs(struct vol *vol)
{
    if (vol->v_adouble == AD_VERSION2_OSX) {
        vol->vfs = &netatalk_adouble_osx;
    }
    else if (vol->v_adouble == AD_VERSION1_ADS) {
        vol->vfs = &netatalk_adouble_ads;
    }
    else if (vol->v_adouble == AD_VERSION1_SFM) {
        vol->vfs = &netatalk_adouble_sfm;
    }
    else {
        vol->vfs = &netatalk_adouble;
    }
}

