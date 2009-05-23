/*
  $Id: cmd_dbd_scanvol.c,v 1.5 2009-05-23 06:28:27 franklahm Exp $

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
#include "cmd_dbd.h"
#include "dbif.h"
#include "db_param.h"
#include "dbd.h"

/* Some defines to ease code parsing */
#define ADDIR_OK (addir_ok == 0)
#define ADFILE_OK (adfile_ok == 0)

/* These must be accessible for cmd_dbd_* funcs */
struct volinfo        *volinfo;
char                  cwdbuf[MAXPATHLEN+1];

/* Some static vars */
static DBD            *dbd;
static DBD            *dbd_rebuild;
static dbd_flags_t    dbd_flags;
static char           stamp[CNID_DEV_LEN];
static char           *netatalk_dirs[] = {
    ".AppleDB",
    ".AppleDesktop",
    NULL
};
static struct cnid_dbd_rqst rqst;
static struct cnid_dbd_rply rply;
static jmp_buf jmp;

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
  Check for .AppleDouble file, create if missing
*/
static int check_adfile(const char *fname, const struct stat *st)
{
    int ret, adflags;
    struct adouble ad;
    char *adname;

    if (S_ISREG(st->st_mode))
        adflags = 0;
    else
        adflags = ADFLAGS_DIR;

    adname = volinfo->ad_path(fname, adflags);

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
            return -1;

        /* Create ad file */
        ad_init(&ad, volinfo->v_adouble, volinfo->v_ad_options);

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
    }
    return 0;
}

/*
  Check for .AppleDouble folder and .Parent, create if missing
*/
static int check_addir(int volroot)
{
    int addir_ok, adpar_ok;
    struct stat st;
    struct adouble ad;

    /* Check for ad-dir */
    if ( (addir_ok = access(ADv2_DIRNAME, F_OK)) != 0) {
        if (errno != ENOENT) {
            dbd_log(LOGSTD, "Access error in directory %s: %s", cwdbuf, strerror(errno));
            return -1;
        }
        dbd_log(LOGSTD, "Missing %s for '%s'", ADv2_DIRNAME, cwdbuf);
    }

    /* Check for ".Parent" */
    if ( (adpar_ok = access(volinfo->ad_path(".", ADFLAGS_DIR), F_OK)) != 0) {
        if (errno != ENOENT) {
            dbd_log(LOGSTD, "Access error on '%s/%s': %s",
                    cwdbuf, volinfo->ad_path(".", ADFLAGS_DIR), strerror(errno));
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
        ad_init(&ad, volinfo->v_adouble, volinfo->v_ad_options);

        if (ad_open_metadata( ".", ADFLAGS_DIR, O_CREAT, &ad) != 0) {
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
    static char fname[MAXPATHLEN] = "../";

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

        if ((stat(ep->d_name, &st)) < 0) {
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

/* 
   Check CNID for a file/dir, both from db and from ad-file.
   For detailed specs see intro.
*/
static cnid_t check_cnid(const char *name, cnid_t did, struct stat *st, int adfile_ok, int adflags)
{
    int ret;
    cnid_t db_cnid, ad_cnid;
    struct adouble ad;

    /* Get CNID from ad-file if volume is using AFPVOL_CACHE */
    ad_cnid = 0;
    if ( (volinfo->v_flags & AFPVOL_CACHE) && ADFILE_OK) {
        ad_init(&ad, volinfo->v_adouble, volinfo->v_ad_options);
        if (ad_open_metadata( name, adflags, O_RDWR, &ad) != 0) {
            dbd_log( LOGSTD, "Error opening AppleDouble file for '%s/%s': %s", cwdbuf, name, strerror(errno));
            return 0;
        }

        if (dbd_flags & DBD_FLAGS_FORCE) {
            ad_cnid = ad_forcegetid(&ad);
            /* This ensures the changed stamp is written */
            ad_setid( &ad, st->st_dev, st->st_ino, ad_cnid, did, stamp);
            ad_flush(&ad);
        }
        else
            ad_cnid = ad_getid(&ad, st->st_dev, st->st_ino, did, stamp);            
        if (ad_cnid == 0)
            dbd_log( LOGSTD, "Incorrect CNID data in .AppleDouble data for '%s/%s' (bad stamp?)", cwdbuf, name);

        ad_close_metadata(&ad);
    }

    /* Get CNID from database */

    /* Prepare request data */
    memset(&rqst, 0, sizeof(struct cnid_dbd_rqst));
    memset(&rply, 0, sizeof(struct cnid_dbd_rply));
    rqst.did = did;
    if ( ! (volinfo->v_flags & AFPVOL_NODEV))
        rqst.dev = st->st_dev;
    rqst.ino = st->st_ino;
    rqst.type = S_ISDIR(st->st_mode)?1:0;
    rqst.name = (char *)name;
    rqst.namelen = strlen(name);

    /* Query the database */
    ret = cmd_dbd_lookup(dbd, &rqst, &rply, (dbd_flags & DBD_FLAGS_SCAN) ? 1 : 0);
    dbif_txn_close(dbd, ret);
    if (rply.result == CNID_DBD_RES_OK) {
        db_cnid = rply.cnid;
    } else if (rply.result == CNID_DBD_RES_NOTFOUND) {
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
            ret = dbd_delete(dbd, &rqst, &rply);
            dbif_txn_close(dbd, ret);

            rqst.cnid = ad_cnid;
            ret = dbd_delete(dbd, &rqst, &rply);
            dbif_txn_close(dbd, ret);

            ret = dbd_rebuild_add(dbd, &rqst, &rply);
            dbif_txn_close(dbd, ret);
        }
        return ad_cnid;
    } else if (ad_cnid && (db_cnid == 0)) {
        /* in ad-file but not in db */
        if ( ! (dbd_flags & DBD_FLAGS_SCAN)) {
            dbd_log( LOGSTD, "CNID rebuild add for '%s/%s', adding with CNID from ad-file: %u", cwdbuf, name, ntohl(ad_cnid));
            rqst.cnid = ad_cnid;
            ret = dbd_delete(dbd, &rqst, &rply);
            dbif_txn_close(dbd, ret);
            ret = dbd_rebuild_add(dbd, &rqst, &rply);
            dbif_txn_close(dbd, ret);
        }
        return ad_cnid;
    } else if ((db_cnid == 0) && (ad_cnid == 0)) {
        /* No CNID at all, we clearly have to allocate a fresh one... */
        /* Note: the next test will use this new CNID too! */
        if ( ! (dbd_flags & DBD_FLAGS_SCAN)) {
            /* add to db */
            ret = cmd_dbd_add(dbd, &rqst, &rply);
            dbif_txn_close(dbd, ret);
            db_cnid = rply.cnid;
            dbd_log( LOGSTD, "New CNID for '%s/%s': %u", cwdbuf, name, ntohl(db_cnid));
        }
    }
    
    if ((ad_cnid == 0) && db_cnid) {
        /* in db but zeroID in ad-file, write it to ad-file if AFPVOL_CACHE */
        if ((volinfo->v_flags & AFPVOL_CACHE) && ADFILE_OK) {
            if ( ! (dbd_flags & DBD_FLAGS_SCAN)) {
                dbd_log( LOGSTD, "Writing CNID data for '%s/%s' to AppleDouble file", cwdbuf, name, ntohl(db_cnid));
                ad_init(&ad, volinfo->v_adouble, volinfo->v_ad_options);
                if (ad_open_metadata( name, adflags, O_RDWR, &ad) != 0) {
                    dbd_log( LOGSTD, "Error opening AppleDouble file for '%s/%s': %s", cwdbuf, name, strerror(errno));
                    return 0;
                }
                ad_setid( &ad, st->st_dev, st->st_ino, db_cnid, did, stamp);
                ad_flush(&ad);
                ad_close_metadata(&ad);
            }
        }
        return db_cnid;
    }

    return 0;
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
    cnid_t cnid;
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

        /* Skip .AppleDouble dir in this loop */
        if (STRCMP(ep->d_name, == , ADv2_DIRNAME))
            continue;

        if ((ret = stat(ep->d_name, &st)) < 0) {
            dbd_log( LOGSTD, "Lost file while reading dir '%s/%s', probably removed: %s", cwdbuf, ep->d_name, strerror(errno));
            continue;
        }
        if (S_ISREG(st.st_mode))
            adflags = 0;
        else
            adflags = ADFLAGS_DIR;

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

        /* Check CNIDs */
        cnid = check_cnid(ep->d_name, did, &st, adfile_ok, adflags);

        /* Now add this object to our rebuild dbd */
        if (cnid) {
            rqst.cnid = rply.cnid;
            dbd_rebuild_add(dbd_rebuild, &rqst, &rply);
            if (rply.result != CNID_DBD_RES_OK) {
                dbd_log( LOGDEBUG, "Fatal error adding CNID: %u for '%s/%s' to in-memory rebuild-db",
                         cnid, cwdbuf, ep->d_name);
                exit(EXIT_FAILURE);
            }
        }

        /**************************************************************************
          Recursion
        **************************************************************************/
        if (S_ISDIR(st.st_mode) && cnid) { /* If we have no cnid for it we cant recur */
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
                continue;
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
    if (dbd_readdir(1, htonl(2)) < 0)  /* 2 = volumeroot CNID */
        return -1;

    return 0;
}

/* 
   Remove all CNIDs from dbd that are not in dbd_rebuild
*/
void delete_orphaned_cnids(DBD *dbd, DBD *dbd_rebuild, dbd_flags_t flags)
{
    int ret, deleted = 0;
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

        if (deleted > 50) {
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
                        ret = dbd_delete(dbd, &rqst, &rply);
                        dbif_txn_close(dbd, ret);
                        deleted++;
                    }
                }
                return;
            } else
                /* Normal case (ret=1): continue while loop */
                continue;
        }

        if (dbd_cnid < rebuild_cnid) {
            /* CNID is orphaned -> delete */
            dbd_log(LOGSTD, "Orphaned CNID in database: %u.", dbd_cnid);
            if ( ! (dbd_flags & DBD_FLAGS_SCAN)) {
                rqst.cnid = htonl(dbd_cnid);
                ret = dbd_delete(dbd, &rqst, &rply);
                dbif_txn_close(dbd, ret);
                deleted++;
            }
            continue;
        }

        if (dbd_cnid > rebuild_cnid) {
            dbd_log(LOGSTD, "Ghost CNID: %u. This is fatal! Dumping rebuild db:\n", rebuild_cnid);
            dbif_dump(dbd_rebuild, 0);
            dbd_log(LOGSTD, "Send this dump and a `dbd -d ...` dump to the Netatalk Dev team!");
            dbif_txn_close(dbd, ret);
            dbif_idwalk(dbd, NULL, 1); /* Close cursor */
            dbif_idwalk(dbd_rebuild, NULL, 1); /* Close cursor */
            goto cleanup;
        }
    }

cleanup:
    dbif_idwalk(dbd, NULL, 1); /* Close cursor */
    dbif_idwalk(dbd_rebuild, NULL, 1); /* Close cursor */
    return;
}

/*
  Main func called from cmd_dbd.c
*/
int cmd_dbd_scanvol(DBD *dbd_ref, struct volinfo *volinfo, dbd_flags_t flags)
{
    int ret = 0;

    /* Make it accessible for all funcs */
    dbd = dbd_ref;

    /* We only support unicode volumes ! */
    if ( volinfo->v_volcharset != CH_UTF8) {
        dbd_log( LOGSTD, "Not a Unicode volume: %s, %u != %u", volinfo->v_volcodepage, volinfo->v_volcharset, CH_UTF8);
        return -1;
    }

    /* Get volume stamp */
    dbd_getstamp(dbd, &rqst, &rply);
    if (rply.result != CNID_DBD_RES_OK)
        goto exit_cleanup;
    memcpy(stamp, rply.name, CNID_DEV_LEN);

    /* open/create rebuild dbd, copy rootinfo key */
    if (NULL == (dbd_rebuild = dbif_init(NULL, NULL)))
        return -1;
    if (0 != (dbif_open(dbd_rebuild, NULL, 0)))
        return -1;
    if (0 != (dbif_copy_rootinfokey(dbd, dbd_rebuild)))
        goto exit_cleanup;

    if (setjmp(jmp) != 0)
        goto exit_cleanup;      /* Got signal, jump from dbd_readdir */

    /* scanvol */
    if ( (scanvol(volinfo, flags)) != 0)
        return -1;

    /* We can only do this in excluse mode, otherwise we might delete CNIDs added from
       other clients in between our pass 1 and 2 */
    if (flags & DBD_FLAGS_EXCL)
        delete_orphaned_cnids(dbd, dbd_rebuild, flags);

exit_cleanup:
    dbif_close(dbd_rebuild);
    return ret;
}
