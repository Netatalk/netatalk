/* 
   $Id: cmd_dbd_scanvol.c,v 1.1 2009-05-06 11:54:24 franklahm Exp $

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

/* 
   dbd specs and implementation progress
   =====================================

   St Type Check (Status of implementation progress, type: file/dir)
   -- ---- -----
   OK D    Make sure .AppleDouble dir exist, create if missing. Error creating
           it is fatal as that shouldn't happen as root
   OK F    Make sure ad file exists
   OK F/D  Delete orphaned ad-files, log dirs in ad-dir
   OK F/D  Check name encoding by roundtripping, log on error
   .. F/D  try: read CNID from ad file (if cnid caching is on)
           try: fetch CNID from database
           on mismatch: keep CNID from file, update database
           on no CNID id ad file: write CNID from database to ad file
           on no CNID in database: add CNID from ad file to database
           on no CNID at all: create one and store in both places
*/

/* FIXME: set id */
#if 0
        ad_setid( &ad, s_path->st.st_dev, s_path->st.st_ino, dir->d_did, did, vol->v_stamp);
#endif

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

#include <atalk/adouble.h>
#include <atalk/unicode.h>
#include <atalk/volinfo.h>
#include "cmd_dbd.h" 
#include "dbif.h"
#include "db_param.h"

static DBD            *dbd_rebuild;
static struct volinfo *volinfo;
static dbd_flags_t    dbd_flags;
static char           cwdbuf[MAXPATHLEN+1];
static char           *netatalk_dirs[] = {
    ".AppleDB",
    ".AppleDesktop",
    NULL
};

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

    if ((volinfo->v_casefold & AFPVOL_UTOMUPPER))
        flags |= CONV_TOUPPER;
    else if ((volinfo->v_casefold & AFPVOL_UTOMLOWER))
        flags |= CONV_TOLOWER;

    if ((volinfo->v_flags & AFPVOL_EILSEQ)) {
        flags |= CONV__EILSEQ;
    }

    /* convert charsets */
    if ((size_t)-1 == ( outlen = convert_charset(volinfo->v_volcharset,
                                                 CH_UTF8_MAC, 
                                                 volinfo->v_maccharset,
                                                 u, outlen, mpath, MAXPATHLEN, &flags)) ) { 
        dbd_log( LOGSTD, "Conversion from %s to %s for %s failed.",
                 volinfo->v_volcodepage, volinfo->v_maccodepage, u);
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
    char	*m, *u;
    size_t       inplen;
    size_t       outlen;
    u_int16_t	 flags = 0;

    if (!mpath)
        return NULL;
        
    if ( *mpath == '\0' ) {
        return( "." );
    }

    /* set conversion flags */
    if (!(volinfo->v_flags & AFPVOL_NOHEX))
        flags |= CONV_ESCAPEHEX;
    if (!(volinfo->v_flags & AFPVOL_USEDOTS))
        flags |= CONV_ESCAPEDOTS;

    if ((volinfo->v_casefold & AFPVOL_MTOUUPPER))
        flags |= CONV_TOUPPER;
    else if ((volinfo->v_casefold & AFPVOL_MTOULOWER))
        flags |= CONV_TOLOWER;

    if ((volinfo->v_flags & AFPVOL_EILSEQ)) {
        flags |= CONV__EILSEQ;
    }

    m = mpath;
    u = upath;

    inplen = strlen(m);
    outlen = MAXPATHLEN;

    if ((size_t)-1 == (outlen = convert_charset(CH_UTF8_MAC,
                                                volinfo->v_volcharset,
                                                volinfo->v_maccharset,
                                                m, inplen, u, outlen, &flags)) ) {
        dbd_log( LOGSTD, "conversion from UTF8-MAC to %s for %s failed.",
                 volinfo->v_volcodepage, mpath);
	    return NULL;
    }

    return( upath );
}

/* 
   Check for wrong encoding e.g. "." at the beginning is not CAP encoded (:2e) although volume is default !AFPVOL_USEDOTS.
   We do it by roundtripiping from volcharset to UTF8-MAC and back and then compare the result.
*/
static int check_name_encoding(char *uname)
{
    char *roundtripped;

    roundtripped = mtoupath(utompath(uname));
    if (!roundtripped) {
        dbd_log( LOGSTD, "Error checking encoding for %s/%s", cwdbuf, uname);
        return -1;
    }

    if ( STRCMP(uname, !=, roundtripped)) {
        dbd_log( LOGSTD, "Bad encoding for %s/%s", cwdbuf, uname);
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
   Check for .AppleDouble file, create if missing
*/
static int check_adfile(const char *fname, const struct stat *st)
{
    int ret;
    struct adouble ad;
    char *adname;

    adname = volinfo->ad_path(fname, 0);

    if ((ret = access( adname, F_OK)) != 0) {
        if (errno != ENOENT) {
            dbd_log(LOGSTD, "Access error for ad-file '%s/%s': %s",
                    cwdbuf, adname, strerror(errno));
            return -1;
        }
        /* Missing. Log and create it */
        dbd_log(LOGSTD, "Missing AppleDoube file '%s/%s'", cwdbuf, adname);

        if (dbd_flags & DBD_FLAGS_SCAN)
            /* Scan only requested, dont change anything */
            return 0;

        /* Create ad file */
        ad_init(&ad, volinfo->v_adouble, volinfo->v_flags);
        
        if ((ret = ad_open_metadata( fname, 0, O_CREAT, &ad)) != 0) {
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
    }
    return 0;
}

/* 
   Check for .AppleDouble folder, create if missing
*/
static int check_addir(int volroot)
{
    int ret;
    struct stat st;
    struct adouble ad;

    if ((ret = access(ADv2_DIRNAME, F_OK)) != 0) {
        if (errno != ENOENT) {
            dbd_log(LOGSTD, "Access error in directory %s: %s", cwdbuf, strerror(errno));
            return -1;
        }
        /* Missing. Log and create it */
        dbd_log(LOGSTD, "Missing %s directory %s", ADv2_DIRNAME, cwdbuf);

        if (dbd_flags & DBD_FLAGS_SCAN)
            /* Scan only requested, dont change anything */
            return 0;

        /* Create ad dir and set name and id */
        ad_init(&ad, volinfo->v_adouble, volinfo->v_flags);
        
        if ((ret = ad_open_metadata( ".", ADFLAGS_DIR, O_CREAT, &ad)) != 0) {
            dbd_log( LOGSTD, "Error creating AppleDouble dir in %s: %s", cwdbuf, strerror(errno));
            return -1;
        }

        /* Get basename of cwd from cwdbuf */
        char *mname = utompath(strrchr(cwdbuf, '/') + 1);

        /* Update name in ad file */
        ad_setname(&ad, mname);
        ad_flush(&ad);
        ad_close_metadata(&ad);
        
        /* Inherit owner/group from "." to ".AppleDouble" and ".Parent" */
        if ((stat(".", &st)) != 0) {
            dbd_log( LOGSTD, "Couldnt stat %s: %s", cwdbuf, strerror(errno));
            return -1;
        }
        chown(ADv2_DIRNAME, st.st_uid, st.st_gid);
        chown(volinfo->ad_path(".", ADFLAGS_DIR), st.st_uid, st.st_gid);
    }

    return 0;
}

static int read_addir(void)
{
    DIR *dp;
    struct dirent *ep;
    struct stat st;
    static char fname[NAME_MAX] = "../";

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

        if ((stat(ep->d_name, &st)) < 0) {
            dbd_log( LOGSTD, "Lost file while reading dir '%s/%s/%s', probably removed: %s",
                     cwdbuf, ADv2_DIRNAME, ep->d_name, strerror(errno));
            continue;
        }

        /* Check for dirs */
        if (S_ISDIR(st.st_mode)) {
            dbd_log( LOGSTD, "Unexpected directory '%s' in AppleDouble dir '%s/%s'",
                     ep->d_name, cwdbuf, ADv2_DIRNAME);
            continue;
        }

        /* Skip ".Parent" */
        if (STRCMP(ep->d_name, ==, ".Parent"))
            continue;

        /* Check for data file */
        strcpy(fname+3, ep->d_name);
        if ((access( fname, F_OK)) != 0) {
            if (errno != ENOENT) {
                dbd_log(LOGSTD, "Access error for file '%s/%s': %s", 
                        cwdbuf, ep->d_name, strerror(errno));
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
        exit(1);
    }

    closedir(dp);

    return 0;
}

static int dbd_readdir(int volroot, cnid_t did)
{
    int cwd, ret = 0, encoding_ok;
    const char *name;
    DIR *dp;
    struct dirent *ep;
    static struct stat st;      /* Save some stack space */

    /* Check for .AppleDouble folder */
    if ((check_addir(volroot)) != 0)
        return -1;

    /* Check AppleDouble files in AppleDouble folder */
    if ((ret = read_addir()) != 0)
        return -1;

    if ((dp = opendir (".")) == NULL) {
        dbd_log(LOGSTD, "Couldn't open the directory: %s",strerror(errno));
        return -1;
    }

    while ((ep = readdir (dp))) {
        /* Check if its "." or ".." */
        if (DIR_DOT_OR_DOTDOT(ep->d_name))
            continue;

        /* Check for netatalk special folders e.g. ".AppleDB" or ".AppleDesktop" */
        if ((name = check_netatalk_dirs(ep->d_name)) != NULL) {
            if (! volroot)
                dbd_log(LOGSTD, "Nested %s in %s", name, cwdbuf);
            continue;
        }

        /* Skip .AppleDouble dir in this loop */
        if (STRCMP(ep->d_name, == , ADv2_DIRNAME))
            continue;

        if ((ret = stat(ep->d_name, &st)) < 0) {
            dbd_log( LOGSTD, "Lost file while reading dir '%s/%s', probably removed: %s", cwdbuf, ep->d_name, strerror(errno));
            continue;
        }

        /**************************************************************************
           Tests
        **************************************************************************/

        /* Check encoding */
        if ( -1 == (encoding_ok = check_name_encoding(ep->d_name)) ) {
            /* If its a file: skipp all other tests now ! */
            /* For dirs we skip tests too, but later */
            if (S_ISREG(st.st_mode))
                continue;
        }

        /* Check for appledouble file, create if missing */
        if (S_ISREG(st.st_mode)) {
            if ( 0 != check_adfile(ep->d_name, &st))
                continue;
        }
        
        /**************************************************************************
          Recursion
        **************************************************************************/
        if (S_ISDIR(st.st_mode)) {
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

            ret = dbd_readdir(0, did);

            fchdir(cwd);
            close(cwd);
            *(strrchr(cwdbuf, '/')) = 0;
            if (ret < 0)
                continue;
        }
    }

    /* 
       Use results of previous checks
    */

exit_cleanup:
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
    volinfo = vi;
    dbd_flags = flags;

    /* Run with umask 0 */
    umask(0);

    /* chdir to vol */
    strcpy(cwdbuf, volinfo->v_path);
    if (cwdbuf[strlen(cwdbuf) - 1] == '/')
        cwdbuf[strlen(cwdbuf) - 1] = 0;
    chdir(volinfo->v_path);

    /* Start recursion */
    if (dbd_readdir(1, 2) < 0)  /* 2 = volumeroot CNID */
        return -1;

    return 0;
}

int cmd_dbd_scanvol(DBD *dbd, struct volinfo *volinfo, dbd_flags_t flags)
{
    int ret = 0;

    /* We only support unicode volumes ! */
    if ( volinfo->v_volcharset != CH_UTF8) {
        dbd_log( LOGSTD, "Not a Unicode volume: %s, %u != %u", volinfo->v_volcodepage, volinfo->v_volcharset, CH_UTF8);
        return -1;
    }

    /* open/create dbd, copy rootinfo key */
    if (NULL == (dbd_rebuild = dbif_init(NULL)))
        return -1;
    if (0 != (dbif_open(dbd_rebuild, NULL, 0)))
        return -1;
    if (0 != (ret = dbif_copy_rootinfokey(dbd, dbd_rebuild)))
        goto exit_cleanup;

    dbd_log( LOGSTD, "dumping rebuilddb");
    dbif_dump(dbd_rebuild, 0);

    /* scanvol */
    if ( (scanvol(volinfo, flags)) != 0)
        return -1;

exit_cleanup:
    dbif_close(dbd_rebuild);
    return ret;
}
