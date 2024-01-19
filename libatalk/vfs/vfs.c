/*
    Copyright (c) 2004 Didier Gautheron
    Copyright (c) 2009 Frank Lahm
 
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include <atalk/afp.h>    
#include <atalk/adouble.h>
#include <atalk/ea.h>
#include <atalk/acl.h>
#include <atalk/logger.h>
#include <atalk/util.h>
#include <atalk/volume.h>
#include <atalk/vfs.h>
#include <atalk/directory.h>
#include <atalk/unix.h>
#include <atalk/errchk.h>
#include <atalk/bstrlib.h>
#include <atalk/bstradd.h>

struct perm {
    uid_t uid;
    gid_t gid;
};

typedef int (*rf_loop)(const struct vol *, struct dirent *, char *, void *, int);

/* ----------------------------- */
static int 
for_each_adouble(const char *from, const char *name, rf_loop fn, const struct vol *vol, void *data, int flag)
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
        if (fn && (ret = fn(vol, de, buf, data, flag))) {
           closedir(dp);
           return ret;
        }
        *m = 0;
    }
    closedir(dp);
    return ret;
}

/*******************************************************************************
 * classic adouble format 
 *******************************************************************************/

static int netatalk_name(const char *name)
{
    return strcasecmp(name,".AppleDouble") &&
        strcasecmp(name,".AppleDB") &&
        strcasecmp(name,".AppleDesktop");
}

static int validupath_adouble(VFS_FUNC_ARGS_VALIDUPATH)
{
    if (name[0] != '.')
        return 1;
    
    if (!(vol->v_flags & AFPVOL_USEDOTS))
        return 0;
        
    return netatalk_name(name) && strcasecmp(name,".Parent");
}                                           

/* ----------------- */
static int RF_chown_adouble(VFS_FUNC_ARGS_CHOWN)
{
    struct stat st;
    char        *ad_p;

    ad_p = vol->ad_path(path, ADFLAGS_HF );

    if ( stat( ad_p, &st ) < 0 )
        return 0; /* ignore */

    return chown( ad_p, uid, gid );
}

/* ----------------- */
static int RF_renamedir_adouble(VFS_FUNC_ARGS_RENAMEDIR)
{
    return 0;
}

/* ----------------- */
static int deletecurdir_adouble_loop(const struct vol *vol, struct dirent *de, char *name, void *data _U_, int flag _U_)
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

static int RF_deletecurdir_adouble(VFS_FUNC_ARGS_DELETECURDIR)
{
    int err;

    /* delete stray .AppleDouble files. this happens to get .Parent files
       as well. */
    if ((err = for_each_adouble("deletecurdir", ".AppleDouble", deletecurdir_adouble_loop, vol, NULL, 1))) 
        return err;
    return netatalk_rmdir(-1, ".AppleDouble" );
}

/* ----------------- */
static int adouble_setfilmode(const struct vol *vol, const char *name, mode_t mode, struct stat *st)
{
    return setfilmode(vol, name, ad_hf_mode(mode), st);
}

static int RF_setfilmode_adouble(VFS_FUNC_ARGS_SETFILEMODE)
{
    return adouble_setfilmode(vol, vol->ad_path(name, ADFLAGS_HF ), mode, st);
}

/* ----------------- */
static int RF_setdirunixmode_adouble(VFS_FUNC_ARGS_SETDIRUNIXMODE)
{
    if (adouble_setfilmode(vol, vol->ad_path(name, ADFLAGS_DIR ), mode, st) < 0) 
        return -1;

    return 0;
}

/* ----------------- */
static int setdirmode_adouble_loop(const struct vol *vol, struct dirent *de _U_, char *name, void *data, int flag)
{
    mode_t hf_mode = *(mode_t *)data;
    struct stat st;

    if (ostat(name, &st, vol_syml_opt(vol)) < 0 ) {
        if (flag)
            return 0;
        LOG(log_error, logtype_afpd, "setdirmode: stat %s: %s", name, strerror(errno) );
    }
    else if (!S_ISDIR(st.st_mode)) {
        if (setfilmode(vol, name, hf_mode, &st) < 0) {
               /* FIXME what do we do then? */
        }
    }
    return 0;
}

static int RF_setdirmode_adouble(VFS_FUNC_ARGS_SETDIRMODE)
{
    mode_t hf_mode = ad_hf_mode(mode);
    char  *adouble = vol->ad_path(name, ADFLAGS_DIR );
    char  *adouble_p = ad_dir(adouble);

    if (for_each_adouble("setdirmode", adouble_p, setdirmode_adouble_loop, vol, &hf_mode, vol_noadouble(vol)))
        return -1;

    return 0;
}

/* ----------------- */
static int setdirowner_adouble_loop(const struct vol *vol, struct dirent *de _U_, char *name, void *data, int flag _U_)
{
    struct perm   *owner  = data;

    if ( chown( name, owner->uid, owner->gid ) < 0 && errno != EPERM ) {
         LOG(log_debug, logtype_afpd, "setdirowner: chown %d/%d %s: %s",
                owner->uid, owner->gid, fullpathname(name), strerror(errno) );
         /* return ( -1 ); Sometimes this is okay */
    }
    return 0;
}

static int RF_setdirowner_adouble(VFS_FUNC_ARGS_SETDIROWNER)
{
    int           noadouble = vol_noadouble(vol);
    char          *adouble_p;
    struct stat   st;
    struct perm   owner;
    
    owner.uid = uid;
    owner.gid = gid;

    adouble_p = ad_dir(vol->ad_path(name, ADFLAGS_DIR ));

    if (for_each_adouble("setdirowner", adouble_p, setdirowner_adouble_loop, vol, &owner, noadouble))
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
static int RF_deletefile_adouble(VFS_FUNC_ARGS_DELETEFILE)
{
	return netatalk_unlinkat(dirfd, vol->ad_path(file, ADFLAGS_HF));
}

/* ----------------- */
static int RF_renamefile_adouble(VFS_FUNC_ARGS_RENAMEFILE)
{
    char  adsrc[ MAXPATHLEN + 1];
    int   err = 0;

    strcpy( adsrc, vol->ad_path(src, 0 ));
    if (unix_rename(dirfd, adsrc, -1, vol->ad_path(dst, 0 )) < 0) {
        struct stat st;

        err = errno;
        if (errno == ENOENT) {
	        struct adouble    ad;

            if (ostatat(dirfd, adsrc, &st, vol_syml_opt(vol))) /* source has no ressource fork, */
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
    	        if (!unix_rename(dirfd, adsrc, -1, vol->ad_path(dst, 0 )) ) 
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

static int RF_copyfile_adouble(VFS_FUNC_ARGS_COPYFILE)
/* const struct vol *vol, int sfd, const char *src, const char *dst */
{
    EC_INIT;
    bstring s = NULL, d = NULL;
    char *dup1 = NULL;
    char *dup2 = NULL;
    char *dup3 = NULL;
    char *dup4 = NULL;
    const char *name = NULL;
    const char *dir = NULL;

    struct stat st;
    EC_ZERO(stat(dst, &st));

    if (S_ISDIR(st.st_mode)) {
        /* build src path to AppleDouble file*/
        EC_NULL(s = bfromcstr(src));
        EC_ZERO(bcatcstr(s, "/.AppleDouble/.Parent"));

        /* build dst path to AppleDouble file*/
        EC_NULL(d = bfromcstr(dst));
        EC_ZERO(bcatcstr(d, "/.AppleDouble/.Parent"));
    } else {
        /* get basename */

        /* build src path to AppleDouble file*/
        EC_NULL(dup1 = strdup(src));
        EC_NULL(name = basename(strdup(dup1)));

        EC_NULL(dup2 = strdup(src));
        EC_NULL(dir = dirname(dup2));
        EC_NULL(s = bfromcstr(dir));
        EC_ZERO(bcatcstr(s, "/.AppleDouble/"));
        EC_ZERO(bcatcstr(s, name));

        /* build dst path to AppleDouble file*/
        EC_NULL(dup4 = strdup(dst));
        EC_NULL(name = basename(strdup(dup4)));

        EC_NULL(dup3 = strdup(dst));
        EC_NULL(dir = dirname(dup3));
        EC_NULL(d = bfromcstr(dir));
        EC_ZERO(bcatcstr(d, "/.AppleDouble/"));
        EC_ZERO(bcatcstr(d, name));
    }

    EC_ZERO(copy_file(sfd, cfrombstr(s), cfrombstr(d), 0666));

EC_CLEANUP:
    bdestroy(s);
    bdestroy(d);
    if (dup1) free(dup1);
    if (dup2) free(dup2);
    if (dup3) free(dup3);
    if (dup4) free(dup4);

    EC_EXIT;
}

#ifdef HAVE_SOLARIS_ACLS
static int RF_solaris_acl(VFS_FUNC_ARGS_ACL)
{
    static char buf[ MAXPATHLEN + 1];
    struct stat st;
    int len;

    if ((stat(path, &st)) != 0)
	return -1;
    if (S_ISDIR(st.st_mode)) {
	len = snprintf(buf, MAXPATHLEN, "%s/.AppleDouble",path);
	if (len < 0 || len >=  MAXPATHLEN)
	    return -1;
	/* set acl on .AppleDouble dir first */
	if ((acl(buf, cmd, count, aces)) != 0)
	    return -1;
	/* now set ACL on ressource fork */
	if ((acl(vol->ad_path(path, ADFLAGS_DIR), cmd, count, aces)) != 0)
	    return -1;
    } else
	/* set ACL on ressource fork */
	if ((acl(vol->ad_path(path, ADFLAGS_HF), cmd, count, aces)) != 0)
	    return -1;

    return 0;
}

static int RF_solaris_remove_acl(VFS_FUNC_ARGS_REMOVE_ACL)
{
    int ret;
    static char buf[ MAXPATHLEN + 1];
    int len;

    if (dir) {
	len = snprintf(buf, MAXPATHLEN, "%s/.AppleDouble",path);
	if (len < 0 || len >=  MAXPATHLEN)
	    return AFPERR_MISC;
	/* remove ACL from .AppleDouble/.Parent first */
	if ((ret = remove_acl_vfs(vol->ad_path(path, ADFLAGS_DIR))) != AFP_OK)
	    return ret;
	/* now remove from .AppleDouble dir */
	if ((ret = remove_acl_vfs(buf)) != AFP_OK)
	    return ret;
    } else
	/* remove ACL from ressource fork */
	if ((ret = remove_acl_vfs(vol->ad_path(path, ADFLAGS_HF))) != AFP_OK)
	    return ret;

    return AFP_OK;
}
#endif

#ifdef HAVE_POSIX_ACLS
static int RF_posix_acl(VFS_FUNC_ARGS_ACL)
{
    EC_INIT;
    static char buf[ MAXPATHLEN + 1];
    struct stat st;
    int len;

    if (S_ISDIR(st.st_mode)) {
        len = snprintf(buf, MAXPATHLEN, "%s/.AppleDouble",path);
        if (len < 0 || len >=  MAXPATHLEN)
            EC_FAIL;
        /* set acl on .AppleDouble dir first */
        EC_ZERO_LOG(acl_set_file(buf, type, acl));

        if (type == ACL_TYPE_ACCESS)
            /* set ACL on ressource fork (".Parent") too */
            EC_ZERO_LOG(acl_set_file(vol->ad_path(path, ADFLAGS_DIR), type, acl));
    } else {
        /* set ACL on ressource fork */
        EC_ZERO_LOG(acl_set_file(vol->ad_path(path, ADFLAGS_HF), type, acl));
    }
    
EC_CLEANUP:
    if (ret != 0)
        return AFPERR_MISC;
    return AFP_OK;
}

static int RF_posix_remove_acl(VFS_FUNC_ARGS_REMOVE_ACL)
{
    EC_INIT;
    static char buf[ MAXPATHLEN + 1];
    int len;

    if (dir) {
        len = snprintf(buf, MAXPATHLEN, "%s/.AppleDouble",path);
        if (len < 0 || len >=  MAXPATHLEN)
            return AFPERR_MISC;
        /* remove ACL from .AppleDouble/.Parent first */
        EC_ZERO_LOG_ERR(remove_acl_vfs(vol->ad_path(path, ADFLAGS_DIR)), AFPERR_MISC);

        /* now remove from .AppleDouble dir */
        EC_ZERO_LOG_ERR(remove_acl_vfs(buf), AFPERR_MISC);
    } else {
        /* remove ACL from ressource fork */
        EC_ZERO_LOG_ERR(remove_acl_vfs(vol->ad_path(path, ADFLAGS_HF)), AFPERR_MISC);
    }

EC_CLEANUP:
    EC_EXIT;
}
#endif

/********************************************************************************************
 * VFS chaining
 ********************************************************************************************/

/* 
 * Up until we really start stacking many VFS modules on top of one another or use
 * dynamic module loading like we do for UAMs, up until then we just stack VFS modules
 * via an fixed size array.
 * All VFS funcs must return AFP_ERR codes. When a func in the chain returns an error
 * this error code will be returned to the caller, BUT the chain in followed and all
 * following funcs are called in order to give them a chance.
 */

/* 
 * Define most VFS funcs with macros as they all do the same.
 * Only "ad_path" and "validupath" will NOT do stacking and only
 * call the func from the first module.
 */

#define VFS_MFUNC(name, args, vars) \
    static int vfs_ ## name(args) \
    { \
        int i = 0, ret = AFP_OK, err; \
        while (vol->vfs_modules[i]) { \
            if (vol->vfs_modules[i]->vfs_ ## name) { \
                err = vol->vfs_modules[i]->vfs_ ## name (vars); \
                if ((ret == AFP_OK) && (err != AFP_OK)) \
                    ret = err; \
            } \
            i ++; \
        } \
        return ret; \
    }

VFS_MFUNC(chown, VFS_FUNC_ARGS_CHOWN, VFS_FUNC_VARS_CHOWN)
VFS_MFUNC(renamedir, VFS_FUNC_ARGS_RENAMEDIR, VFS_FUNC_VARS_RENAMEDIR) 
VFS_MFUNC(deletecurdir, VFS_FUNC_ARGS_DELETECURDIR, VFS_FUNC_VARS_DELETECURDIR)
VFS_MFUNC(setfilmode, VFS_FUNC_ARGS_SETFILEMODE, VFS_FUNC_VARS_SETFILEMODE)
VFS_MFUNC(setdirmode, VFS_FUNC_ARGS_SETDIRMODE, VFS_FUNC_VARS_SETDIRMODE)
VFS_MFUNC(setdirunixmode, VFS_FUNC_ARGS_SETDIRUNIXMODE, VFS_FUNC_VARS_SETDIRUNIXMODE)
VFS_MFUNC(setdirowner, VFS_FUNC_ARGS_SETDIROWNER, VFS_FUNC_VARS_SETDIROWNER)
VFS_MFUNC(deletefile, VFS_FUNC_ARGS_DELETEFILE, VFS_FUNC_VARS_DELETEFILE)
VFS_MFUNC(renamefile, VFS_FUNC_ARGS_RENAMEFILE, VFS_FUNC_VARS_RENAMEFILE)
VFS_MFUNC(copyfile, VFS_FUNC_ARGS_COPYFILE, VFS_FUNC_VARS_COPYFILE)
#ifdef HAVE_ACLS
VFS_MFUNC(acl, VFS_FUNC_ARGS_ACL, VFS_FUNC_VARS_ACL)
VFS_MFUNC(remove_acl, VFS_FUNC_ARGS_REMOVE_ACL, VFS_FUNC_VARS_REMOVE_ACL)
#endif
VFS_MFUNC(ea_getsize, VFS_FUNC_ARGS_EA_GETSIZE, VFS_FUNC_VARS_EA_GETSIZE)
VFS_MFUNC(ea_getcontent, VFS_FUNC_ARGS_EA_GETCONTENT, VFS_FUNC_VARS_EA_GETCONTENT)
VFS_MFUNC(ea_list, VFS_FUNC_ARGS_EA_LIST, VFS_FUNC_VARS_EA_LIST)
VFS_MFUNC(ea_set, VFS_FUNC_ARGS_EA_SET, VFS_FUNC_VARS_EA_SET)
VFS_MFUNC(ea_remove, VFS_FUNC_ARGS_EA_REMOVE, VFS_FUNC_VARS_EA_REMOVE)

static int vfs_validupath(VFS_FUNC_ARGS_VALIDUPATH)
{
    return vol->vfs_modules[0]->vfs_validupath(VFS_FUNC_VARS_VALIDUPATH);
}

/*
 * These function pointers get called from the lib users via vol->vfs->func.
 * These funcs are defined via the macros above.
 */
static struct vfs_ops vfs_master_funcs = {
    vfs_validupath,
    vfs_chown,
    vfs_renamedir,
    vfs_deletecurdir,
    vfs_setfilmode,
    vfs_setdirmode,
    vfs_setdirunixmode,
    vfs_setdirowner,
    vfs_deletefile,
    vfs_renamefile,
    vfs_copyfile,
#ifdef HAVE_ACLS
    vfs_acl,
    vfs_remove_acl,
#endif
    vfs_ea_getsize,
    vfs_ea_getcontent,
    vfs_ea_list,
    vfs_ea_set,
    vfs_ea_remove
};

/* 
 * Primary adouble module
 */

static struct vfs_ops netatalk_adouble = {
    /* vfs_validupath:    */ validupath_adouble,
    /* vfs_chown:         */ RF_chown_adouble,
    /* vfs_renamedir:     */ RF_renamedir_adouble,
    /* vfs_deletecurdir:  */ RF_deletecurdir_adouble,
    /* vfs_setfilmode:    */ RF_setfilmode_adouble,
    /* vfs_setdirmode:    */ RF_setdirmode_adouble,
    /* vfs_setdirunixmode:*/ RF_setdirunixmode_adouble,
    /* vfs_setdirowner:   */ RF_setdirowner_adouble,
    /* vfs_deletefile:    */ RF_deletefile_adouble,
    /* vfs_renamefile:    */ RF_renamefile_adouble,
    /* vfs_copyfile:      */ RF_copyfile_adouble,
    NULL
};

/* 
 * Secondary vfs modules for Extended Attributes
 */

static struct vfs_ops netatalk_ea_adouble = {
    /* vfs_validupath:    */ NULL,
    /* vfs_chown:         */ ea_chown,
    /* vfs_renamedir:     */ NULL, /* ok */
    /* vfs_deletecurdir:  */ NULL, /* ok */
    /* vfs_setfilmode:    */ ea_chmod_file,
    /* vfs_setdirmode:    */ NULL, /* ok */
    /* vfs_setdirunixmode:*/ ea_chmod_dir,
    /* vfs_setdirowner:   */ NULL, /* ok */
    /* vfs_deletefile:    */ ea_deletefile,
    /* vfs_renamefile:    */ ea_renamefile,
    /* vfs_copyfile       */ ea_copyfile,
#ifdef HAVE_ACLS
    /* vfs_acl:           */ NULL,
    /* vfs_remove_acl     */ NULL,
#endif
    /* vfs_getsize        */ get_easize,
    /* vfs_getcontent     */ get_eacontent,
    /* vfs_list           */ list_eas,
    /* vfs_set            */ set_ea,
    /* vfs_remove         */ remove_ea
};

static struct vfs_ops netatalk_ea_sys = {
    /* validupath:        */ NULL,
    /* rf_chown:          */ NULL,
    /* rf_renamedir:      */ NULL,
    /* rf_deletecurdir:   */ NULL,
    /* rf_setfilmode:     */ NULL,
    /* rf_setdirmode:     */ NULL,
    /* rf_setdirunixmode: */ NULL,
    /* rf_setdirowner:    */ NULL,
    /* rf_deletefile:     */ NULL,
    /* rf_renamefile:     */ NULL,
    /* vfs_copyfile:      */ sys_ea_copyfile,
#ifdef HAVE_ACLS
    /* rf_acl:            */ NULL,
    /* rf_remove_acl      */ NULL,
#endif
    /* ea_getsize         */ sys_get_easize,
    /* ea_getcontent      */ sys_get_eacontent,
    /* ea_list            */ sys_list_eas,
    /* ea_set             */ sys_set_ea,
    /* ea_remove          */ sys_remove_ea
};

/* 
 * Tertiary VFS modules for ACLs
 */

#ifdef HAVE_SOLARIS_ACLS
static struct vfs_ops netatalk_solaris_acl_adouble = {
    /* validupath:        */ NULL,
    /* rf_chown:          */ NULL,
    /* rf_renamedir:      */ NULL,
    /* rf_deletecurdir:   */ NULL,
    /* rf_setfilmode:     */ NULL,
    /* rf_setdirmode:     */ NULL,
    /* rf_setdirunixmode: */ NULL,
    /* rf_setdirowner:    */ NULL,
    /* rf_deletefile:     */ NULL,
    /* rf_renamefile:     */ NULL,
    /* vfs_copyfile       */ NULL,
    /* rf_acl:            */ RF_solaris_acl,
    /* rf_remove_acl      */ RF_solaris_remove_acl,
    NULL
};
#endif

#ifdef HAVE_POSIX_ACLS
static struct vfs_ops netatalk_posix_acl_adouble = {
    /* validupath:        */ NULL,
    /* rf_chown:          */ NULL,
    /* rf_renamedir:      */ NULL,
    /* rf_deletecurdir:   */ NULL,
    /* rf_setfilmode:     */ NULL,
    /* rf_setdirmode:     */ NULL,
    /* rf_setdirunixmode: */ NULL,
    /* rf_setdirowner:    */ NULL,
    /* rf_deletefile:     */ NULL,
    /* rf_renamefile:     */ NULL,
    /* vfs_copyfile       */ NULL,
    /* rf_acl:            */ RF_posix_acl,
    /* rf_remove_acl      */ RF_posix_remove_acl,
    NULL
};
#endif

/* ---------------- */
void initvol_vfs(struct vol *vol)
{
    vol->vfs = &vfs_master_funcs;

    /* Default adouble stuff */
    vol->vfs_modules[0] = &netatalk_adouble;
    vol->ad_path = ad_path;

    /* Extended Attributes */
    if (vol->v_vfs_ea == AFPVOL_EA_SYS) {
        LOG(log_debug, logtype_afpd, "initvol_vfs: enabling EA support with native EAs");
        vol->vfs_modules[1] = &netatalk_ea_sys;
    } else if (vol->v_vfs_ea == AFPVOL_EA_AD) {
        LOG(log_debug, logtype_afpd, "initvol_vfs: enabling EA support with adouble files");
        vol->vfs_modules[1] = &netatalk_ea_adouble;
    } else {
        LOG(log_debug, logtype_afpd, "initvol_vfs: volume without EA support");
    }

    /* ACLs */
#ifdef HAVE_SOLARIS_ACLS
    vol->vfs_modules[2] = &netatalk_solaris_acl_adouble;
#endif
#ifdef HAVE_POSIX_ACLS
    vol->vfs_modules[2] = &netatalk_posix_acl_adouble;
#endif

}
