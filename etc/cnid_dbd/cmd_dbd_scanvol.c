/*
  Copyright (c) 2009 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>

#include <atalk/adouble.h>
#include <atalk/unicode.h>
#include <atalk/volinfo.h>
#include <atalk/cnid_dbd_private.h>
#include <atalk/volume.h>
#include <atalk/ea.h>
#include <atalk/util.h>
#include <atalk/acl.h>

#include "cmd_dbd.h"
#include "dbif.h"
#include "db_param.h"
#include "dbd.h"

/* Some defines to ease code parsing */
#define ADDIR_OK (addir_ok == 0)
#define ADFILE_OK (adfile_ok == 0)


static struct volinfo *myvolinfo;
static char           cwdbuf[MAXPATHLEN+1];
static DBD            *dbd;
static DBD            *dbd_rebuild;
static dbd_flags_t    dbd_flags;
static char           stamp[CNID_DEV_LEN];
static char           *netatalk_dirs[] = {
    ".AppleDB",
    ".AppleDesktop",
    NULL
};
static char           *special_dirs[] = {
    ".zfs",
    NULL
};
static struct cnid_dbd_rqst rqst;
static struct cnid_dbd_rply rply;
static jmp_buf jmp;
static struct vol volume; /* fake it for ea_open */
static char pname[MAXPATHLEN] = "../";

/*
  Taken form afpd/desktop.c
*/
static char *utompath(char *upath)
{
    static char  mpath[ MAXPATHLEN + 2]; /* for convert_charset dest_len parameter +2 */
    char         *m, *u;
    uint16_t     flags = CONV_IGNORE | CONV_UNESCAPEHEX;
    size_t       outlen;

    if (!upath)
        return NULL;

    m = mpath;
    u = upath;
    outlen = strlen(upath);

    if ((myvolinfo->v_casefold & AFPVOL_UTOMUPPER))
        flags |= CONV_TOUPPER;
    else if ((myvolinfo->v_casefold & AFPVOL_UTOMLOWER))
        flags |= CONV_TOLOWER;

    if ((myvolinfo->v_flags & AFPVOL_EILSEQ)) {
        flags |= CONV__EILSEQ;
    }

    /* convert charsets */
    if ((size_t)-1 == ( outlen = convert_charset(myvolinfo->v_volcharset,
                                                 CH_UTF8_MAC,
                                                 myvolinfo->v_maccharset,
                                                 u, outlen, mpath, MAXPATHLEN, &flags)) ) {
        dbd_log( LOGSTD, "Conversion from %s to %s for %s failed.",
                 myvolinfo->v_volcodepage, myvolinfo->v_maccodepage, u);
        return NULL;
    }

    return(m);
}

/*
  Taken form afpd/desktop.c
*/
static char *mtoupath(char *mpath)
{
    static char  upath[ MAXPATHLEN + 2]; /* for convert_charset dest_len parameter +2 */
    char    *m, *u;
    size_t       inplen;
    size_t       outlen;
    u_int16_t    flags = 0;

    if (!mpath)
        return NULL;

    if ( *mpath == '\0' ) {
        return( "." );
    }

    /* set conversion flags */
    if (!(myvolinfo->v_flags & AFPVOL_NOHEX))
        flags |= CONV_ESCAPEHEX;
    if (!(myvolinfo->v_flags & AFPVOL_USEDOTS))
        flags |= CONV_ESCAPEDOTS;

    if ((myvolinfo->v_casefold & AFPVOL_MTOUUPPER))
        flags |= CONV_TOUPPER;
    else if ((myvolinfo->v_casefold & AFPVOL_MTOULOWER))
        flags |= CONV_TOLOWER;

    if ((myvolinfo->v_flags & AFPVOL_EILSEQ)) {
        flags |= CONV__EILSEQ;
    }

    m = mpath;
    u = upath;

    inplen = strlen(m);
    outlen = MAXPATHLEN;

    if ((size_t)-1 == (outlen = convert_charset(CH_UTF8_MAC,
                                                myvolinfo->v_volcharset,
                                                myvolinfo->v_maccharset,
                                                m, inplen, u, outlen, &flags)) ) {
        dbd_log( LOGSTD, "conversion from UTF8-MAC to %s for %s failed.",
                 myvolinfo->v_volcodepage, mpath);
        return NULL;
    }

    return( upath );
}

#if 0
/* 
   Check if "name" is pointing to
   a) an object inside the current volume (return 0)
   b) an object outside the current volume (return 1)
   Then stats the pointed to object and if it is a dir ors ADFLAGS_DIR to *adflags
   Return -1 on any serious error.
 */
static int check_symlink(const char *name, int *adflags)
{
    int cwd;
    ssize_t len;
    char pathbuf[MAXPATHLEN + 1];
    char *sep;
    struct stat st;

    if ((len = readlink(name, pathbuf, MAXPATHLEN)) == -1) {
        dbd_log(LOGSTD, "Error reading link info for '%s/%s': %s", 
                cwdbuf, name, strerror(errno));
        return -1;
    }
    pathbuf[len] = 0;

    if ((stat(pathbuf, &st)) != 0) {
        dbd_log(LOGSTD, "stat error '%s': %s", pathbuf, strerror(errno));
    }

    /* Remember cwd */
    if ((cwd = open(".", O_RDONLY)) < 0) {
        dbd_log(LOGSTD, "error opening cwd '%s': %s", cwdbuf, strerror(errno));
        return -1;
    }

    if (S_ISDIR(st.st_mode)) {
        *adflags |= ADFLAGS_DIR;
    } else { /* file */
        /* get basename from path */
        if ((sep = strrchr(pathbuf, '/')) == NULL)
            /* just a file at the same level */
            return 0;
        sep = 0; /* pathbuf now contains the directory*/
    }

    /* fchdir() to pathbuf so we can easily get its path with getcwd() */
    if ((chdir(pathbuf)) != 0) {
        dbd_log(LOGSTD, "Cant chdir to '%s': %s", pathbuf, strerror(errno));
        return -1;
    }

    if ((getcwd(pathbuf, MAXPATHLEN)) == NULL) {
        dbd_log(LOGSTD, "Cant get symlink'ed dir '%s/%s': %s", cwdbuf, pathbuf, strerror(errno));
        if ((fchdir(cwd)) != 0)
            /* We're foobared */
            longjmp(jmp, 1); /* this jumps back to cmd_dbd_scanvol() */
        return -1;
    }

    if ((fchdir(cwd)) != 0)
        /* We're foobared */
        longjmp(jmp, 1); /* this jumps back to cmd_dbd_scanvol() */

    /*
      We now have the symlink target dir as absoulte path in pathbuf
      and can compare it with the currents volume path
    */
    int i = 0;
    while (myvolinfo->v_path[i]) {
        if ((pathbuf[i] == 0) || (myvolinfo->v_path[i] != pathbuf[i])) {
            dbd_log( LOGDEBUG, "extra-share symlink '%s/%s', following", cwdbuf, name);
            return 1;
        }
        i++;
    }

    dbd_log( LOGDEBUG, "intra-share symlink '%s/%s', not following", cwdbuf, name);
    return 0;
}
#endif

/*
  Check for wrong encoding e.g. "." at the beginning is not CAP encoded (:2e) although volume is default !AFPVOL_USEDOTS.
  We do it by roundtripiping from volcharset to UTF8-MAC and back and then compare the result.
*/
static int check_name_encoding(char *uname)
{
    char *roundtripped;

    roundtripped = mtoupath(utompath(uname));
    if (!roundtripped) {
        dbd_log( LOGSTD, "Error checking encoding for '%s/%s'", cwdbuf, uname);
        return -1;
    }

    if ( STRCMP(uname, !=, roundtripped)) {
        dbd_log( LOGSTD, "Bad encoding for '%s/%s'", cwdbuf, uname);
        return -1;
    }

    return 0;
}

/*
  Check for netatalk special folders e.g. ".AppleDB" or ".AppleDesktop"
  Returns pointer to name or NULL.
*/
static const char *check_netatalk_dirs(const char *name)
{
    int c;

    for (c=0; netatalk_dirs[c]; c++) {
        if ((strcmp(name, netatalk_dirs[c])) == 0)
            return netatalk_dirs[c];
    }
    return NULL;
}

/*
  Check for special names
  Returns pointer to name or NULL.
*/
static const char *check_special_dirs(const char *name)
{
    int c;

    for (c=0; special_dirs[c]; c++) {
        if ((strcmp(name, special_dirs[c])) == 0)
            return special_dirs[c];
    }
    return NULL;
}

/*
  Check for .AppleDouble file, create if missing
*/
static int check_adfile(const char *fname, const struct stat *st)
{
    int ret, adflags;
    struct adouble ad;
    char *adname;

    if (dbd_flags & DBD_FLAGS_CLEANUP)
        return 0;

    if (S_ISREG(st->st_mode))
        adflags = 0;
    else
        adflags = ADFLAGS_DIR;

    adname = myvolinfo->ad_path(fname, adflags);

    if ((ret = access( adname, F_OK)) != 0) {
        if (errno != ENOENT) {
            dbd_log(LOGSTD, "Access error for ad-file '%s/%s': %s",
                    cwdbuf, adname, strerror(errno));
            return -1;
        }
        /* Missing. Log and create it */
        dbd_log(LOGSTD, "Missing AppleDouble file '%s/%s'", cwdbuf, adname);

        if (dbd_flags & DBD_FLAGS_SCAN)
            /* Scan only requested, dont change anything */
            return -1;

        /* Create ad file */
        ad_init(&ad, myvolinfo->v_adouble, myvolinfo->v_ad_options);

        if ((ret = ad_open_metadata( fname, adflags, O_CREAT, &ad)) != 0) {
            dbd_log( LOGSTD, "Error creating AppleDouble file '%s/%s': %s",
                     cwdbuf, adname, strerror(errno));

            return -1;
        }

        /* Set name in ad-file */
        ad_setname(&ad, utompath((char *)fname));
        ad_flush(&ad);
        ad_close_metadata(&ad);

        chown(adname, st->st_uid, st->st_gid);
        /* FIXME: should we inherit mode too here ? */
#if 0
        chmod(adname, st->st_mode);
#endif
    } else {
        ad_init(&ad, myvolinfo->v_adouble, myvolinfo->v_ad_options);
        if (ad_open_metadata( fname, adflags, O_RDONLY, &ad) != 0) {
            dbd_log( LOGSTD, "Error opening AppleDouble file for '%s/%s'", cwdbuf, fname);
            return -1;
        }
        ad_close_metadata(&ad);
    }
    return 0;
}

/* 
   Remove all files with file::EA* from adouble dir
*/
static void remove_eafiles(const char *name, struct ea *ea)
{
    DIR *dp = NULL;
    struct dirent *ep;
    char eaname[MAXPATHLEN];

    strlcpy(eaname, name, sizeof(eaname));
    if (strlcat(eaname, "::EA", sizeof(eaname)) >= sizeof(eaname)) {
        dbd_log(LOGSTD, "name too long: '%s/%s/%s'", cwdbuf, ADv2_DIRNAME, name);
        return;
    }

    if ((chdir(ADv2_DIRNAME)) != 0) {
        dbd_log(LOGSTD, "Couldn't chdir to '%s/%s': %s",
                cwdbuf, ADv2_DIRNAME, strerror(errno));
        return;
    }

    if ((dp = opendir(".")) == NULL) {
        dbd_log(LOGSTD, "Couldn't open the directory '%s/%s': %s",
                cwdbuf, ADv2_DIRNAME, strerror(errno));
        goto exit;
    }

    while ((ep = readdir(dp))) {
        if (strstr(ep->d_name, eaname) != NULL) {
            dbd_log(LOGSTD, "Removing EA file: '%s/%s/%s'",
                    cwdbuf, ADv2_DIRNAME, ep->d_name);
            if ((unlink(ep->d_name)) != 0) {
                dbd_log(LOGSTD, "Error unlinking EA file '%s/%s/%s': %s",
                        cwdbuf, ADv2_DIRNAME, ep->d_name, strerror(errno));
            }
        } /* if */
    } /* while */

exit:
    if (dp)
        closedir(dp);
    if ((chdir("..")) != 0) {
        dbd_log(LOGSTD, "Couldn't chdir to '%s': %s", cwdbuf, strerror(errno));
        /* we can't proceed */
        longjmp(jmp, 1); /* this jumps back to cmd_dbd_scanvol() */
    }    
}

/*
  Check Extended Attributes files
*/
static int check_eafiles(const char *fname)
{
    unsigned int  count = 0;
    int ret = 0, remove;
    struct ea ea;
    struct stat st;
    char *eaname;

    if ((ret = ea_open(&volume, fname, EA_RDWR, &ea)) != 0) {
        if (errno == ENOENT)
            return 0;
        dbd_log(LOGSTD, "Error calling ea_open for file: %s/%s, removing EA files", cwdbuf, fname);
        if ( ! (dbd_flags & DBD_FLAGS_SCAN))
            remove_eafiles(fname, &ea);
        return -1;
    }

    /* Check all EAs */
    while (count < ea.ea_count) {        
        dbd_log(LOGDEBUG, "EA: %s", (*ea.ea_entries)[count].ea_name);
        remove = 0;

        eaname = ea_path(&ea, (*ea.ea_entries)[count].ea_name, 0);

        if (lstat(eaname, &st) != 0) {
            if (errno == ENOENT)
                dbd_log(LOGSTD, "Missing EA: %s/%s", cwdbuf, eaname);
            else
                dbd_log(LOGSTD, "Bogus EA: %s/%s", cwdbuf, eaname);
            remove = 1;
        } else if (st.st_size != (*ea.ea_entries)[count].ea_size) {
            dbd_log(LOGSTD, "Bogus EA: %s/%s, removing it...", cwdbuf, eaname);
            remove = 1;
            if ((unlink(eaname)) != 0)
                dbd_log(LOGSTD, "Error removing EA file '%s/%s': %s",
                        cwdbuf, eaname, strerror(errno));
        }

        if (remove) {
            /* Be CAREFUL here! This should do what ea_delentry does. ea_close relies on it !*/
            free((*ea.ea_entries)[count].ea_name);
            (*ea.ea_entries)[count].ea_name = NULL;
        }

        count++;
    } /* while */

    ea_close(&ea);
    return ret;
}

/*
  Check for .AppleDouble folder and .Parent, create if missing
*/
static int check_addir(int volroot)
{
    int addir_ok, adpar_ok;
    struct stat st;
    struct adouble ad;
    char *mname = NULL;

    if (dbd_flags & DBD_FLAGS_CLEANUP)
        return 0;

    /* Check for ad-dir */
    if ( (addir_ok = access(ADv2_DIRNAME, F_OK)) != 0) {
        if (errno != ENOENT) {
            dbd_log(LOGSTD, "Access error in directory %s: %s", cwdbuf, strerror(errno));
            return -1;
        }
        dbd_log(LOGSTD, "Missing %s for '%s'", ADv2_DIRNAME, cwdbuf);
    }

    /* Check for ".Parent" */
    if ( (adpar_ok = access(myvolinfo->ad_path(".", ADFLAGS_DIR), F_OK)) != 0) {
        if (errno != ENOENT) {
            dbd_log(LOGSTD, "Access error on '%s/%s': %s",
                    cwdbuf, myvolinfo->ad_path(".", ADFLAGS_DIR), strerror(errno));
            return -1;
        }
        dbd_log(LOGSTD, "Missing .AppleDouble/.Parent for '%s'", cwdbuf);
    }

    /* Is one missing ? */
    if ((addir_ok != 0) || (adpar_ok != 0)) {
        /* Yes, but are we only scanning ? */
        if (dbd_flags & DBD_FLAGS_SCAN) {
            /* Yes:  missing .Parent is not a problem, but missing ad-dir
               causes later checking of ad-files to fail. So we have to return appropiately */
            if (addir_ok != 0)
                return -1;
            else  /* (adpar_ok != 0) */
                return 0;
        }

        /* Create ad dir and set name */
        ad_init(&ad, myvolinfo->v_adouble, myvolinfo->v_ad_options);

        if (ad_open_metadata( ".", ADFLAGS_DIR, O_CREAT, &ad) != 0) {
            dbd_log( LOGSTD, "Error creating AppleDouble dir in %s: %s", cwdbuf, strerror(errno));
            return -1;
        }

        /* Get basename of cwd from cwdbuf */
        mname = utompath(strrchr(cwdbuf, '/') + 1);

        /* Update name in ad file */
        ad_setname(&ad, mname);
        ad_flush(&ad);
        ad_close_metadata(&ad);

        /* Inherit owner/group from "." to ".AppleDouble" and ".Parent" */
        if ((lstat(".", &st)) != 0) {
            dbd_log( LOGSTD, "Couldnt stat %s: %s", cwdbuf, strerror(errno));
            return -1;
        }
        chown(ADv2_DIRNAME, st.st_uid, st.st_gid);
        chown(myvolinfo->ad_path(".", ADFLAGS_DIR), st.st_uid, st.st_gid);
    }

    return 0;
}

/*
  Check if file cotains "::EA" and if it does check if its correspondig data fork exists.
  Returns:
  0 = name is not an EA file
  1 = name is an EA file and no problem was found
  -1 = name is an EA file and data fork is gone
 */
static int check_eafile_in_adouble(const char *name)
{
    int ret = 0;
    char *namep, *namedup = NULL;

    /* Check if this is an AFPVOL_EA_AD vol */
    if (myvolinfo->v_vfs_ea == AFPVOL_EA_AD) {
        /* Does the filename contain "::EA" ? */
        namedup = strdup(name);
        if ((namep = strstr(namedup, "::EA")) == NULL) {
            ret = 0;
            goto ea_check_done;
        } else {
            /* File contains "::EA" so it's an EA file. Check for data file  */

            /* Get string before "::EA" from EA filename */
            namep[0] = 0;
            strlcpy(pname + 3, namedup, sizeof(pname)); /* Prepends "../" */

            if ((access( pname, F_OK)) == 0) {
                ret = 1;
                goto ea_check_done;
            } else {
                ret = -1;
                if (errno != ENOENT) {
                    dbd_log(LOGSTD, "Access error for file '%s/%s': %s",
                            cwdbuf, name, strerror(errno));
                    goto ea_check_done;
                }

                /* Orphaned EA file*/
                dbd_log(LOGSTD, "Orphaned Extended Attribute file '%s/%s/%s'",
                        cwdbuf, ADv2_DIRNAME, name);

                if (dbd_flags & DBD_FLAGS_SCAN)
                    /* Scan only requested, dont change anything */
                    goto ea_check_done;

                if ((unlink(name)) != 0) {
                    dbd_log(LOGSTD, "Error unlinking orphaned Extended Attribute file '%s/%s/%s'",
                            cwdbuf, ADv2_DIRNAME, name);
                }
            } /* if (access) */
        } /* if strstr */
    } /* if AFPVOL_EA_AD */

ea_check_done:
    if (namedup)
        free(namedup);

    return ret;
}

/*
  Check files and dirs inside .AppleDouble folder:
  - remove orphaned files
  - bail on dirs
*/
static int read_addir(void)
{
    DIR *dp;
    struct dirent *ep;
    struct stat st;

    if ((chdir(ADv2_DIRNAME)) != 0) {
        dbd_log(LOGSTD, "Couldn't chdir to '%s/%s': %s",
                cwdbuf, ADv2_DIRNAME, strerror(errno));
        return -1;
    }

    if ((dp = opendir(".")) == NULL) {
        dbd_log(LOGSTD, "Couldn't open the directory '%s/%s': %s",
                cwdbuf, ADv2_DIRNAME, strerror(errno));
        return -1;
    }

    while ((ep = readdir(dp))) {
        /* Check if its "." or ".." */
        if (DIR_DOT_OR_DOTDOT(ep->d_name))
            continue;
        /* Skip ".Parent" */
        if (STRCMP(ep->d_name, ==, ".Parent"))
            continue;

        if ((lstat(ep->d_name, &st)) < 0) {
            dbd_log( LOGSTD, "Lost file or dir while enumeratin dir '%s/%s/%s', probably removed: %s",
                     cwdbuf, ADv2_DIRNAME, ep->d_name, strerror(errno));
            continue;
        }

        /* Check for dirs */
        if (S_ISDIR(st.st_mode)) {
            dbd_log( LOGSTD, "Unexpected directory '%s' in AppleDouble dir '%s/%s'",
                     ep->d_name, cwdbuf, ADv2_DIRNAME);
            continue;
        }

        /* Check if for orphaned and corrupt Extended Attributes file */
        if (check_eafile_in_adouble(ep->d_name) != 0)
            continue;

        /* Check for data file */
        strcpy(pname + 3, ep->d_name);
        if ((access( pname, F_OK)) != 0) {
            if (errno != ENOENT) {
                dbd_log(LOGSTD, "Access error for file '%s/%s': %s",
                        cwdbuf, pname, strerror(errno));
                continue;
            }
            /* Orphaned ad-file*/
            dbd_log(LOGSTD, "Orphaned AppleDoube file '%s/%s/%s'",
                    cwdbuf, ADv2_DIRNAME, ep->d_name);

            if (dbd_flags & DBD_FLAGS_SCAN)
                /* Scan only requested, dont change anything */
                continue;;

            if ((unlink(ep->d_name)) != 0) {
                dbd_log(LOGSTD, "Error unlinking orphaned AppleDoube file '%s/%s/%s'",
                        cwdbuf, ADv2_DIRNAME, ep->d_name);

            }
        }
    }

    if ((chdir("..")) != 0) {
        dbd_log(LOGSTD, "Couldn't chdir back to '%s' from AppleDouble dir: %s",
                cwdbuf, strerror(errno));
        /* This really is EOT! */
        longjmp(jmp, 1); /* this jumps back to cmd_dbd_scanvol() */
    }

    closedir(dp);

    return 0;
}

/*
  Check CNID for a file/dir, both from db and from ad-file.
  For detailed specs see intro.

  @return Correct CNID of object or CNID_INVALID (ie 0) on error
*/
static cnid_t check_cnid(const char *name, cnid_t did, struct stat *st, int adfile_ok, int adflags)
{
    int ret;
    cnid_t db_cnid, ad_cnid;
    struct adouble ad;

    /* Force checkout every X items */
    static int cnidcount = 0;
    cnidcount++;
    if (cnidcount > 10000) {
        cnidcount = 0;
        if (dbif_txn_checkpoint(dbd, 0, 0, 0) < 0) {
            dbd_log(LOGSTD, "Error checkpointing!");
            return CNID_INVALID;
        }
    }

    /* Get CNID from ad-file if volume is using AFPVOL_CACHE */
    ad_cnid = 0;
    if ( (myvolinfo->v_flags & AFPVOL_CACHE) && ADFILE_OK) {
        ad_init(&ad, myvolinfo->v_adouble, myvolinfo->v_ad_options);
        if (ad_open_metadata( name, adflags, O_RDWR, &ad) != 0) {
            
            if (dbd_flags & DBD_FLAGS_CLEANUP)
                return CNID_INVALID;

            dbd_log( LOGSTD, "Error opening AppleDouble file for '%s/%s': %s", cwdbuf, name, strerror(errno));
            return CNID_INVALID;
        }

        if (dbd_flags & DBD_FLAGS_FORCE) {
            ad_cnid = ad_forcegetid(&ad);
            /* This ensures the changed stamp is written */
            ad_setid( &ad, st->st_dev, st->st_ino, ad_cnid, did, stamp);
            ad_flush(&ad);
        }
        else
            ad_cnid = ad_getid(&ad, st->st_dev, st->st_ino, 0, stamp);

        if (ad_cnid == 0)
            dbd_log( LOGSTD, "Bad CNID in adouble file of '%s/%s'", cwdbuf, name);
        else
            dbd_log( LOGDEBUG, "CNID from .AppleDouble file for '%s/%s': %u", cwdbuf, name, ntohl(ad_cnid));

        ad_close_metadata(&ad);
    }

    /* Get CNID from database */

    /* Prepare request data */
    memset(&rqst, 0, sizeof(struct cnid_dbd_rqst));
    memset(&rply, 0, sizeof(struct cnid_dbd_rply));
    rqst.did = did;
    rqst.cnid = ad_cnid;
    if ( ! (myvolinfo->v_flags & AFPVOL_NODEV))
        rqst.dev = st->st_dev;
    rqst.ino = st->st_ino;
    rqst.type = S_ISDIR(st->st_mode)?1:0;
    rqst.name = (char *)name;
    rqst.namelen = strlen(name);

    /* Query the database */
    ret = dbd_lookup(dbd, &rqst, &rply, (dbd_flags & DBD_FLAGS_SCAN) ? 1 : 0);
    if (dbif_txn_close(dbd, ret) != 0)
        return CNID_INVALID;
    if (rply.result == CNID_DBD_RES_OK) {
        db_cnid = rply.cnid;
    } else if (rply.result == CNID_DBD_RES_NOTFOUND) {
        if ( ! (dbd_flags & DBD_FLAGS_FORCE))
            dbd_log( LOGSTD, "No CNID for '%s/%s' in database", cwdbuf, name);
        db_cnid = 0;
    } else {
        dbd_log( LOGSTD, "Fatal error resolving '%s/%s'", cwdbuf, name);
        db_cnid = 0;
    }

    /* Compare results from both CNID searches */
    if (ad_cnid && db_cnid && (ad_cnid == db_cnid)) {
        /* Everything is fine */
        return db_cnid;
    } else if (ad_cnid && db_cnid && (ad_cnid != db_cnid)) {
        /* Mismatch ? Delete both from db and re-add data from file */
        dbd_log( LOGSTD, "CNID mismatch for '%s/%s', db: %u, ad-file: %u", cwdbuf, name, ntohl(db_cnid), ntohl(ad_cnid));
        if ( ! (dbd_flags & DBD_FLAGS_SCAN)) {
            rqst.cnid = db_cnid;
            ret = dbd_delete(dbd, &rqst, &rply, DBIF_CNID);
            if (dbif_txn_close(dbd, ret) != 0)
                return CNID_INVALID;

            rqst.cnid = ad_cnid;
            ret = dbd_delete(dbd, &rqst, &rply, DBIF_CNID);
            if (dbif_txn_close(dbd, ret) != 0)
                return CNID_INVALID;

            ret = dbd_rebuild_add(dbd, &rqst, &rply);
            if (dbif_txn_close(dbd, ret) != 0)
                return CNID_INVALID;
        }
        return ad_cnid;
    } else if (ad_cnid && (db_cnid == 0)) {
        /* in ad-file but not in db */
        if ( ! (dbd_flags & DBD_FLAGS_SCAN)) {
            /* Ensure the cnid from the ad-file is not already occupied by another file */
            dbd_log(LOGDEBUG, "Checking whether CNID %u from ad-file is occupied",
                    ntohl(ad_cnid));

            rqst.cnid = ad_cnid;
            ret = dbd_resolve(dbd, &rqst, &rply);
            if (ret == CNID_DBD_RES_OK) {
                /* Occupied! Choose another, update ad-file */
                ret = dbd_add(dbd, &rqst, &rply, 1);
                if (dbif_txn_close(dbd, ret) != 0)
                    return CNID_INVALID;
                db_cnid = rply.cnid;
                dbd_log(LOGSTD, "New CNID for '%s/%s': %u", cwdbuf, name, ntohl(db_cnid));

                if ((myvolinfo->v_flags & AFPVOL_CACHE)
                    && ADFILE_OK
                    && ( ! (dbd_flags & DBD_FLAGS_SCAN))) {
                    dbd_log(LOGSTD, "Writing CNID data for '%s/%s' to AppleDouble file",
                            cwdbuf, name, ntohl(db_cnid));
                    ad_init(&ad, myvolinfo->v_adouble, myvolinfo->v_ad_options);
                    if (ad_open_metadata( name, adflags, O_RDWR, &ad) != 0) {
                        dbd_log(LOGSTD, "Error opening AppleDouble file for '%s/%s': %s",
                                cwdbuf, name, strerror(errno));
                        return CNID_INVALID;
                    }
                    ad_setid( &ad, st->st_dev, st->st_ino, db_cnid, did, stamp);
                    ad_flush(&ad);
                    ad_close_metadata(&ad);
                }
                return db_cnid;
            }

            dbd_log(LOGDEBUG, "CNID rebuild add '%s/%s' with CNID from ad-file %u",
                    cwdbuf, name, ntohl(ad_cnid));
            rqst.cnid = ad_cnid;
            ret = dbd_rebuild_add(dbd, &rqst, &rply);
            if (dbif_txn_close(dbd, ret) != 0)
                return CNID_INVALID;
        }
        return ad_cnid;
    } else if ((db_cnid == 0) && (ad_cnid == 0)) {
        /* No CNID at all, we clearly have to allocate a fresh one... */
        /* Note: the next test will use this new CNID too! */
        if ( ! (dbd_flags & DBD_FLAGS_SCAN)) {
            /* add to db */
            ret = dbd_add(dbd, &rqst, &rply, 1);
            if (dbif_txn_close(dbd, ret) != 0)
                return CNID_INVALID;
            db_cnid = rply.cnid;
            dbd_log(LOGSTD, "New CNID for '%s/%s': %u", cwdbuf, name, ntohl(db_cnid));
        }
    }

    if ((ad_cnid == 0) && db_cnid) {
        /* in db but zeroID in ad-file, write it to ad-file if AFPVOL_CACHE */
        if ((myvolinfo->v_flags & AFPVOL_CACHE) && ADFILE_OK) {
            if ( ! (dbd_flags & DBD_FLAGS_SCAN)) {
                dbd_log(LOGSTD, "Writing CNID data for '%s/%s' to AppleDouble file",
                        cwdbuf, name, ntohl(db_cnid));
                ad_init(&ad, myvolinfo->v_adouble, myvolinfo->v_ad_options);
                if (ad_open_metadata( name, adflags, O_RDWR, &ad) != 0) {
                    dbd_log(LOGSTD, "Error opening AppleDouble file for '%s/%s': %s",
                            cwdbuf, name, strerror(errno));
                    return CNID_INVALID;
                }
                ad_setid( &ad, st->st_dev, st->st_ino, db_cnid, did, stamp);
                ad_flush(&ad);
                ad_close_metadata(&ad);
            }
        }
        return db_cnid;
    }

    return CNID_INVALID;
}

/*
  This is called recursively for all dirs.
  volroot=1 means we're in the volume root dir, 0 means we aren't.
  We use this when checking for netatalk private folders like .AppleDB.
  did is our parents CNID.
*/
static int dbd_readdir(int volroot, cnid_t did)
{
    int cwd, ret = 0, adflags, adfile_ok, addir_ok, encoding_ok;
    cnid_t cnid = 0;
    const char *name;
    DIR *dp;
    struct dirent *ep;
    static struct stat st;      /* Save some stack space */

    /* Check again for .AppleDouble folder, check_adfile also checks/creates it */
    if ((addir_ok = check_addir(volroot)) != 0)
        if ( ! (dbd_flags & DBD_FLAGS_SCAN))
            /* Fatal on rebuild run, continue if only scanning ! */
            return -1;

    /* Check AppleDouble files in AppleDouble folder, but only if it exists or could be created */
    if (ADDIR_OK)
        if ((read_addir()) != 0)
            if ( ! (dbd_flags & DBD_FLAGS_SCAN))
                /* Fatal on rebuild run, continue if only scanning ! */
                return -1;

    if ((dp = opendir (".")) == NULL) {
        dbd_log(LOGSTD, "Couldn't open the directory: %s",strerror(errno));
        return -1;
    }

    while ((ep = readdir (dp))) {
        /* Check if we got a termination signal */
        if (alarmed)
            longjmp(jmp, 1); /* this jumps back to cmd_dbd_scanvol() */

        /* Check if its "." or ".." */
        if (DIR_DOT_OR_DOTDOT(ep->d_name))
            continue;

        /* Check for netatalk special folders e.g. ".AppleDB" or ".AppleDesktop" */
        if ((name = check_netatalk_dirs(ep->d_name)) != NULL) {
            if (! volroot)
                dbd_log(LOGSTD, "Nested %s in %s", name, cwdbuf);
            continue;
        }

        /* Check for special folders in volume root e.g. ".zfs" */
        if (volroot) {
            if ((name = check_special_dirs(ep->d_name)) != NULL) {
                dbd_log(LOGSTD, "Ignoring special dir \"%s\"", name);
                continue;
            }
        }

        /* Skip .AppleDouble dir in this loop */
        if (STRCMP(ep->d_name, == , ADv2_DIRNAME))
            continue;

        if ((ret = lstat(ep->d_name, &st)) < 0) {
            dbd_log( LOGSTD, "Lost file while reading dir '%s/%s', probably removed: %s",
                     cwdbuf, ep->d_name, strerror(errno));
            continue;
        }
        
        switch (st.st_mode & S_IFMT) {
        case S_IFREG:
            adflags = 0;
            break;
        case S_IFDIR:
            adflags = ADFLAGS_DIR;
            break;
        case S_IFLNK:
            dbd_log(LOGDEBUG, "Ignoring symlink %s/%s", cwdbuf, ep->d_name);
#if 0
            ret = check_symlink(ep->d_name, &adflags);
            if (ret == 1)
                break;
            if (ret == -1)
                dbd_log(LOGSTD, "Error checking symlink %s/%s", cwdbuf, ep->d_name);
#endif
            continue;
        default:
            dbd_log(LOGSTD, "Bad filetype: %s/%s", cwdbuf, ep->d_name);
            if ( ! (dbd_flags & DBD_FLAGS_SCAN)) {
                if ((unlink(ep->d_name)) != 0) {
                    dbd_log(LOGSTD, "Error removing: %s/%s: %s", cwdbuf, ep->d_name, strerror(errno));
                }
            }
            continue;
        }

        /**************************************************************************
           Statistics
         **************************************************************************/
        static unsigned long long statcount = 0;
        static time_t t = 0;

        if (t == 0)
            t = time(NULL);

        statcount++;
        if ((statcount % 10000) == 0) {
            if (dbd_flags & DBD_FLAGS_STATS)            
                dbd_log(LOGSTD, "Scanned: %10llu, time: %10llu s",
                        statcount, (unsigned long long)(time(NULL) - t));
        }

        /**************************************************************************
           Tests
        **************************************************************************/

        /* Check encoding */
        if ( -1 == (encoding_ok = check_name_encoding(ep->d_name)) ) {
            /* If its a file: skipp all other tests now ! */
            /* For dirs we could try to get a CNID for it and recurse, but currently I prefer not to */
            continue;
        }

        /* Check for appledouble file, create if missing, but only if we have addir */
        adfile_ok = -1;
        if (ADDIR_OK)
            adfile_ok = check_adfile(ep->d_name, &st);

        if ( ! nocniddb) {
            /* Check CNIDs */
            cnid = check_cnid(ep->d_name, did, &st, adfile_ok, adflags);

            /* Now add this object to our rebuild dbd */
            if (cnid && dbd_rebuild) {
                static uint count = 0;
                rqst.cnid = rply.cnid;
                ret = dbd_rebuild_add(dbd_rebuild, &rqst, &rply);
                if (dbif_txn_close(dbd_rebuild, ret) != 0)
                    return -1;
                if (rply.result != CNID_DBD_RES_OK) {
                    dbd_log( LOGSTD, "Fatal error adding CNID: %u for '%s/%s' to in-memory rebuild-db",
                             cnid, cwdbuf, ep->d_name);
                    return -1;
                }
                count++;
                if (count == 10000) {
                    if (dbif_txn_checkpoint(dbd_rebuild, 0, 0, 0) < 0) {
                        dbd_log(LOGSTD, "Error checkpointing!");
                        return -1;
                    }
                    count = 0;
                }
            }
        }

        /* Check EA files */
        if (myvolinfo->v_vfs_ea == AFPVOL_EA_AD)
            check_eafiles(ep->d_name);

        /**************************************************************************
          Recursion
        **************************************************************************/
        if (S_ISDIR(st.st_mode) && (cnid || nocniddb)) { /* If we have no cnid for it we cant recur */
            strcat(cwdbuf, "/");
            strcat(cwdbuf, ep->d_name);
            dbd_log( LOGDEBUG, "Entering directory: %s", cwdbuf);
            if (-1 == (cwd = open(".", O_RDONLY))) {
                dbd_log( LOGSTD, "Cant open directory '%s': %s", cwdbuf, strerror(errno));
                continue;
            }
            if (0 != chdir(ep->d_name)) {
                dbd_log( LOGSTD, "Cant chdir to directory '%s': %s", cwdbuf, strerror(errno));
                close(cwd);
                continue;
            }

            ret = dbd_readdir(0, cnid);

            fchdir(cwd);
            close(cwd);
            *(strrchr(cwdbuf, '/')) = 0;
            if (ret < 0)
                return -1;
        }
    }

    /*
      Use results of previous checks
    */

    closedir(dp);
    return ret;
}

static int scanvol(struct volinfo *vi, dbd_flags_t flags)
{
    /* Dont scanvol on no-appledouble vols */
    if (vi->v_flags & AFPVOL_NOADOUBLE) {
        dbd_log( LOGSTD, "Volume without AppleDouble support: skipping volume scanning.");
        return 0;
    }

    /* Make this stuff accessible from all funcs easily */
    myvolinfo = vi;
    dbd_flags = flags;

    /* Init a fake struct vol with just enough so we can call ea_open and friends */
    volume.v_adouble = AD_VERSION2;
    volume.v_vfs_ea = myvolinfo->v_vfs_ea;
    initvol_vfs(&volume);

    /* Run with umask 0 */
    umask(0);

    /* Remove trailing slash from volume, chdir to vol */
    if (myvolinfo->v_path[strlen(myvolinfo->v_path) - 1] == '/')
        myvolinfo->v_path[strlen(myvolinfo->v_path) - 1] = 0;
    strcpy(cwdbuf, myvolinfo->v_path);
    chdir(myvolinfo->v_path);

    /* Start recursion */
    if (dbd_readdir(1, htonl(2)) < 0)  /* 2 = volumeroot CNID */
        return -1;

    return 0;
}

/*
  Remove all CNIDs from dbd that are not in dbd_rebuild
*/
static void delete_orphaned_cnids(DBD *dbd, DBD *dbd_rebuild, dbd_flags_t flags)
{
    int ret = 0, deleted = 0;
    cnid_t dbd_cnid = 0, rebuild_cnid = 0;
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply;

    /* jump over rootinfo key */
    if ( dbif_idwalk(dbd, &dbd_cnid, 0) != 1)
        return;
    if ( dbif_idwalk(dbd_rebuild, &rebuild_cnid, 0) != 1)
        return;

    /* Get first id from dbd_rebuild */
    if ((dbif_idwalk(dbd_rebuild, &rebuild_cnid, 0)) == -1)
        return;

    /* Start main loop through dbd: get CNID from dbd */
    while ((dbif_idwalk(dbd, &dbd_cnid, 0)) == 1) {
        /* Check if we got a termination signal */
        if (alarmed)
            longjmp(jmp, 1); /* this jumps back to cmd_dbd_scanvol() */

        if (deleted > 1000) {
            deleted = 0;
            if (dbif_txn_checkpoint(dbd, 0, 0, 0) < 0) {
                dbd_log(LOGSTD, "Error checkpointing!");
                goto cleanup;
            }
        }

        /* This should be the normal case: CNID is in both dbs */
        if (dbd_cnid == rebuild_cnid) {
            /* Get next CNID from rebuild db */
            if ((ret = dbif_idwalk(dbd_rebuild, &rebuild_cnid, 0)) == -1) {
                /* Some error */
                goto cleanup;
            } else if (ret == 0) {
                /* end of rebuild_cnid, delete all remaining CNIDs from dbd */
                while ((dbif_idwalk(dbd, &dbd_cnid, 0)) == 1) {
                    dbd_log(LOGSTD, "Orphaned CNID in database: %u", dbd_cnid);
                    if ( ! (dbd_flags & DBD_FLAGS_SCAN)) {
                        rqst.cnid = htonl(dbd_cnid);
                        if ((ret = dbd_delete(dbd, &rqst, &rply, DBIF_CNID)) == -1) {
                            dbd_log(LOGSTD, "Error deleting CNID %u", dbd_cnid);
                            (void)dbif_txn_abort(dbd);
                            goto cleanup;
                        }
                        
                        if (dbif_txn_close(dbd, ret) != 0)
                            return;
                        deleted++;
                    }
                    /* Check if we got a termination signal */
                    if (alarmed)
                        longjmp(jmp, 1); /* this jumps back to cmd_dbd_scanvol() */
                }
                return;
            } else
                /* Normal case (ret=1): continue while loop */
                continue;
        }

        if (dbd_cnid < rebuild_cnid) {
            /* CNID is orphaned -> delete */
            dbd_log(LOGSTD, "One orphaned CNID in database: %u.", dbd_cnid);
            if ( ! (dbd_flags & DBD_FLAGS_SCAN)) {
                rqst.cnid = htonl(dbd_cnid);
                if ((ret = dbd_delete(dbd, &rqst, &rply, DBIF_CNID)) == -1) {
                    dbd_log(LOGSTD, "Error deleting CNID %u", dbd_cnid);
                    (void)dbif_txn_abort(dbd);
                    goto cleanup;
                }
                if (dbif_txn_close(dbd, ret) != 0)
                    return;
                deleted++;
            }
            continue;
        }

        if (dbd_cnid > rebuild_cnid) {
            dbif_idwalk(dbd, NULL, 1); /* Close cursor */
            dbif_idwalk(dbd_rebuild, NULL, 1); /* Close cursor */
            (void)dbif_txn_close(dbd, 2);
            (void)dbif_txn_close(dbd_rebuild, 2);                
            dbd_log(LOGSTD, "Ghost CNID: %u. This is fatal! Dumping rebuild db:\n", rebuild_cnid);
            dbif_dump(dbd_rebuild, 0);
            dbd_log(LOGSTD, "Send this dump and a `dbd -d ...` dump to the Netatalk Dev team!");
            goto cleanup;
        }
    } /* while ((dbif_idwalk(dbd, &dbd_cnid, 0)) == 1) */

cleanup:
    dbif_idwalk(dbd, NULL, 1); /* Close cursor */
    dbif_idwalk(dbd_rebuild, NULL, 1); /* Close cursor */
    return;
}

static const char *get_tmpdb_path(void)
{
    pid_t pid = getpid();
    static char path[MAXPATHLEN];
    snprintf(path, MAXPATHLEN, "/tmp/tmpdb-dbd.%u", pid);
    if (mkdir(path, 0755) != 0)
        return NULL;
    return path;
}

/*
  Main func called from cmd_dbd.c
*/
int cmd_dbd_scanvol(DBD *dbd_ref, struct volinfo *vi, dbd_flags_t flags)
{
    int ret = 0;
    struct db_param db_param = { 0 };
    const char *tmpdb_path = NULL;

    /* Set cachesize for in-memory rebuild db */
    db_param.cachesize = 64 * 1024;         /* 64 MB */
    db_param.maxlocks = DEFAULT_MAXLOCKS;
    db_param.maxlockobjs = DEFAULT_MAXLOCKOBJS;
    db_param.logfile_autoremove = 1;

    /* Make it accessible for all funcs */
    dbd = dbd_ref;

    /* We only support unicode volumes ! */
    if ( vi->v_volcharset != CH_UTF8) {
        dbd_log( LOGSTD, "Not a Unicode volume: %s, %u != %u", vi->v_volcodepage, vi->v_volcharset, CH_UTF8);
        return -1;
    }

    /* Get volume stamp */
    dbd_getstamp(dbd, &rqst, &rply);
    if (rply.result != CNID_DBD_RES_OK) {
        ret = -1;
        goto exit;
    }
    memcpy(stamp, rply.name, CNID_DEV_LEN);

    /* temporary rebuild db, used with -re rebuild to delete unused CNIDs, not used with -f */
    if (! nocniddb && (flags & DBD_FLAGS_EXCL) && !(flags & DBD_FLAGS_FORCE)) {
        tmpdb_path = get_tmpdb_path();
        if (NULL == (dbd_rebuild = dbif_init(tmpdb_path, "cnid2.db"))) {
            ret = -1;
            goto exit;
        }

        if (dbif_env_open(dbd_rebuild,
                          &db_param,
                          DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN) < 0) {
            dbd_log(LOGSTD, "error opening tmp database!");
            goto exit;
        }

        if (0 != (dbif_open(dbd_rebuild, NULL, 0))) {
            ret = -1;
            goto exit;
        }

        if (0 != (dbif_copy_rootinfokey(dbd, dbd_rebuild))) {
            ret = -1;
            goto exit;
        }
    }

    if (setjmp(jmp) != 0) {
        ret = 0;                /* Got signal, jump from dbd_readdir */
        goto exit;
    }

    /* scanvol */
    if ( (scanvol(vi, flags)) != 0) {
        ret = -1;
        goto exit;
    }

exit:
    if (! nocniddb) {
        if (dbif_txn_close(dbd, ret == 0 ? 1 : 0) != 0)
            ret = -1;
        if (dbd_rebuild)
            if (dbif_txn_close(dbd_rebuild, ret == 0 ? 1 : 0) != 0)
                ret = -1;
        if ((ret == 0) && dbd_rebuild && (flags & DBD_FLAGS_EXCL) && !(flags & DBD_FLAGS_FORCE))
            /* We can only do this in exclusive mode, otherwise we might delete CNIDs added from
               other clients in between our pass 1 and 2 */
            delete_orphaned_cnids(dbd, dbd_rebuild, flags);
    }

    if (dbd_rebuild) {
        dbd_log(LOGDEBUG, "Closing tmp db");
        dbif_close(dbd_rebuild);

        if (tmpdb_path) {
            char cmd[8 + MAXPATHLEN];
            snprintf(cmd, 8 + MAXPATHLEN, "rm -f %s/*", tmpdb_path);
            dbd_log( LOGDEBUG, "Removing temp database '%s'", tmpdb_path);
            system(cmd);
            snprintf(cmd, 8 + MAXPATHLEN, "rmdir %s", tmpdb_path);
            system(cmd);
        }        
    }
    return ret;
}
