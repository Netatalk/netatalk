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

/*
  dbd specs and implementation progress
  =====================================

  St := Status

  Force option
  ------------
  
  St Spec
  -- ----
  OK If -f is requested, ensure -e is too.
     Check if volumes is using AFPVOL_CACHE, then wipe db from disk. Rebuild from ad-files.

  1st pass: Scan volume
  --------------------

  St Type Check
  -- ---- -----
  OK F/D  Make sure ad file exists
  OK D    Make sure .AppleDouble dir exist, create if missing. Error creating
          it is fatal as that shouldn't happen as root.
  OK F/D  Delete orphaned ad-files, log dirs in ad-dir
  OK F/D  Check name encoding by roundtripping, log on error
  OK F/D  try: read CNID from ad file (if cnid caching is on)
          try: fetch CNID from database
          -> on mismatch: use CNID from file, update database (deleting both found CNIDs first)
          -> if no CNID in ad file: write CNID from database to ad file
          -> if no CNID in database: add CNID from ad file to database
          -> on no CNID at all: create one and store in both places
  OK F/D  Add found CNID, DID, filename, dev/inode, stamp to rebuild database
  OK F/D  Check/update stamp (implicitly done while checking CNIDs)


  2nd pass: Delete unused CNIDs
  -----------------------------

  St Spec
  -- ----
  OK Step through dbd (the one on disk) and rebuild-db from pass 1 and delete any CNID from
     dbd not in rebuild db. This in only done in exclusive mode.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#include <atalk/logger.h>
#include <atalk/cnid_dbd_private.h>
#include <atalk/volinfo.h>
#include <atalk/util.h>

#include "cmd_dbd.h"
#include "dbd.h"
#include "dbif.h"
#include "db_param.h"

#define DBOPTIONS (DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN)

int nocniddb = 0;               /* Dont open CNID database, only scan filesystem */
struct volinfo volinfo; /* needed by pack.c:idxname() */
volatile sig_atomic_t alarmed;  /* flags for signals */
int db_locked;                  /* have we got the fcntl lock on lockfile ? */

static DBD *dbd;
static int verbose;             /* Logging flag */
static int exclusive;           /* Exclusive volume access */
static struct db_param db_param = {
    NULL,                       /* Volume dirpath */
    1,                          /* bdb logfile autoremove */
    64 * 1024,                  /* bdb cachesize (64 MB) */
    DEFAULT_MAXLOCKS,           /* maxlocks */
    DEFAULT_MAXLOCKOBJS,        /* maxlockobjs */
    0,                          /* flush_interval */
    0,                          /* flush_frequency */
    0,                          /* usock_file */
    -1,                         /* fd_table_size */
    -1,                         /* idle_timeout */
    -1                          /* max_vols */
};
static char dbpath[MAXPATHLEN+1];   /* Path to the dbd database */

/* 
   Provide some logging
 */
void dbd_log(enum logtype lt, char *fmt, ...)
{
    int len;
    static char logbuffer[1024];
    va_list args;

    if ( (lt == LOGSTD) || (verbose == 1)) {
        va_start(args, fmt);
        len = vsnprintf(logbuffer, 1023, fmt, args);
        va_end(args);
        logbuffer[1023] = 0;

        printf("%s\n", logbuffer);
    }
}

/* 
   SIGNAL handling:
   catch SIGINT and SIGTERM which cause clean exit. Ignore anything else.
 */

static void sig_handler(int signo)
{
    alarmed = 1;
    return;
}

static void set_signal(void)
{
    struct sigaction sv;

    sv.sa_handler = sig_handler;
    sv.sa_flags = SA_RESTART;
    sigemptyset(&sv.sa_mask);
    if (sigaction(SIGTERM, &sv, NULL) < 0) {
        dbd_log( LOGSTD, "error in sigaction(SIGTERM): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGINT, &sv, NULL) < 0) {
        dbd_log( LOGSTD, "error in sigaction(SIGINT): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        

    memset(&sv, 0, sizeof(struct sigaction));
    sv.sa_handler = SIG_IGN;
    sigemptyset(&sv.sa_mask);

    if (sigaction(SIGABRT, &sv, NULL) < 0) {
        dbd_log( LOGSTD, "error in sigaction(SIGABRT): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGHUP, &sv, NULL) < 0) {
        dbd_log( LOGSTD, "error in sigaction(SIGHUP): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGQUIT, &sv, NULL) < 0) {
        dbd_log( LOGSTD, "error in sigaction(SIGQUIT): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
}

static void usage (void)
{
    printf("Usage: dbd [-e|-t|-v|-x] -d [-i] | -s [-c|-n]| -r [-c|-f] | -u <path to netatalk volume>\n"
           "dbd can dump, scan, reindex and rebuild Netatalk dbd CNID databases.\n"
           "dbd must be run with appropiate permissions i.e. as root.\n\n"
           "Main commands are:\n"
           "   -d Dump CNID database\n"
           "      Option: -i dump indexes too\n\n"
           "   -s Scan volume:\n"
           "      1. Compare CNIDs in database with volume\n"
           "      2. Check if .AppleDouble dirs exist\n"
           "      3. Check if  AppleDouble file exist\n"
           "      4. Report orphaned AppleDouble files\n"
           "      5. Check for directories inside AppleDouble directories\n"
           "      6. Check name encoding by roundtripping, log on error\n"
           "      7. Check for orphaned CNIDs in database (requires -e)\n"
           "      8. Open and close adouble files\n"
           "      Options: -c Don't check .AppleDouble stuff, only ckeck orphaned.\n"
           "               -n Don't open CNID database, skip CNID checks\n\n"
           "   -r Rebuild volume:\n"
           "      1. Sync CNIDSs in database with volume\n"
           "      2. Make sure .AppleDouble dir exist, create if missing\n"
           "      3. Make sure AppleDouble file exists, create if missing\n"
           "      4. Delete orphaned AppleDouble files\n"
           "      5. Check for directories inside AppleDouble directories\n"
           "      6. Check name encoding by roundtripping, log on error\n"
           "      7. Check for orphaned CNIDs in database (requires -e)\n"
           "      8. Open and close adouble files\n"
           "      Options: -c Don't create .AppleDouble stuff, only cleanup orphaned.\n"
           "               -f wipe database and rebuild from IDs stored in AppleDouble files,\n"
           "                  only available for volumes without 'nocnidcache' option. Implies -e.\n\n"
           "   -u Upgrade:\n"
           "      Opens the database which triggers any necessary upgrades, then closes and exits.\n\n"
           "General options:\n"
           "   -e only work on inactive volumes and lock them (exclusive)\n"
           "   -x rebuild indexes (just for completeness, mostly useless!)\n"
           "   -t show statistics while running\n"
           "   -v verbose\n\n"
           "WARNING:\n"
           "For -r -f restore of the CNID database from the adouble files, the CNID must of course\n"
           "be synched to them files first with a plain -r rebuild !\n"
        );
}

int main(int argc, char **argv)
{
    int c, lockfd, ret = -1;
    int dump=0, scan=0, rebuild=0, prep_upgrade=0, rebuildindexes=0, dumpindexes=0, force=0;
    dbd_flags_t flags = 0;
    char *volpath;
    int cdir;

    if (geteuid() != 0) {
        usage();
        exit(EXIT_FAILURE);
    }
    /* Inhereting perms in ad_mkdir etc requires this */
    ad_setfuid(0);

    while ((c = getopt(argc, argv, ":cdefinrstuvx")) != -1) {
        switch(c) {
        case 'c':
            flags |= DBD_FLAGS_CLEANUP;
            break;
        case 'd':
            dump = 1;
            break;
        case 'i':
            dumpindexes = 1;
            break;
        case 's':
            scan = 1;
            flags |= DBD_FLAGS_SCAN;
            break;
        case 'n':
            nocniddb = 1; /* FIXME: this could/should be a flag too for consistency */
            break;
        case 'r':
            rebuild = 1;
            break;
        case 't':
            flags |= DBD_FLAGS_STATS;
            break;
        case 'u':
            prep_upgrade = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'e':
            exclusive = 1;
            flags |= DBD_FLAGS_EXCL;
            break;
        case 'x':
            rebuildindexes = 1;
            break;
        case 'f':
            force = 1;
            exclusive = 1;
            flags |= DBD_FLAGS_FORCE | DBD_FLAGS_EXCL;
            break;
        case ':':
        case '?':
            usage();
            exit(EXIT_FAILURE);
            break;
        }
    }

    if ((dump + scan + rebuild + prep_upgrade) != 1) {
        usage();
        exit(EXIT_FAILURE);
    }

    if ( (optind + 1) != argc ) {
        usage();
        exit(EXIT_FAILURE);
    }
    volpath = argv[optind];

    setvbuf(stdout, (char *) NULL, _IONBF, 0);

    /* Remember cwd */
    if ((cdir = open(".", O_RDONLY)) < 0) {
        dbd_log( LOGSTD, "Can't open dir: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
        
    /* Setup signal handling */
    set_signal();

    /* Setup logging. Should be portable among *NIXes */
    if (!verbose)
        setuplog("default log_info /dev/tty");
    else
        setuplog("default log_debug /dev/tty");

    /* Load .volinfo file */
    if (loadvolinfo(volpath, &volinfo) == -1) {
        dbd_log( LOGSTD, "Not a Netatalk volume at '%s', no .volinfo file at '%s/.AppleDesktop/.volinfo' or unknown volume options", volpath, volpath);
        exit(EXIT_FAILURE);
    }
    if (vol_load_charsets(&volinfo) == -1) {
        dbd_log( LOGSTD, "Error loading charsets!");
        exit(EXIT_FAILURE);
    }

    /* Sanity checks to ensure we can touch this volume */
    if (volinfo.v_vfs_ea != AFPVOL_EA_AD && volinfo.v_vfs_ea != AFPVOL_EA_SYS) {
        dbd_log( LOGSTD, "Unknown Extended Attributes option: %u", volinfo.v_vfs_ea);
        exit(EXIT_FAILURE);        
    }

    /* Enuser dbpath is there, create if necessary */
    struct stat st;
    if (stat(volinfo.v_dbpath, &st) != 0) {
        if (errno != ENOENT) {
            dbd_log( LOGSTD, "Can't stat dbpath \"%s\": %s", volinfo.v_dbpath, strerror(errno));
            exit(EXIT_FAILURE);        
        }
        if ((mkdir(volinfo.v_dbpath, 0755)) != 0) {
            dbd_log( LOGSTD, "Can't create dbpath \"%s\": %s", dbpath, strerror(errno));
            exit(EXIT_FAILURE);
        }        
    }

    /* Put "/.AppleDB" at end of volpath, get path from volinfo file */
    if ( (strlen(volinfo.v_dbpath) + strlen("/.AppleDB")) > MAXPATHLEN ) {
        dbd_log( LOGSTD, "Volume pathname too long");
        exit(EXIT_FAILURE);        
    }
    strncpy(dbpath, volinfo.v_dbpath, MAXPATHLEN - strlen("/.AppleDB"));
    strcat(dbpath, "/.AppleDB");

    /* Check or create dbpath */
    int dbdirfd = open(dbpath, O_RDONLY);
    if (dbdirfd == -1 && errno == ENOENT) {
        if (errno == ENOENT) {
            if ((mkdir(dbpath, 0755)) != 0) {
                dbd_log( LOGSTD, "Can't create .AppleDB for \"%s\": %s", dbpath, strerror(errno));
                exit(EXIT_FAILURE);
            }
        } else {
            dbd_log( LOGSTD, "Somethings wrong with .AppleDB for \"%s\", giving up: %s", dbpath, strerror(errno));
            exit(EXIT_FAILURE);
        }
    } else {
        close(dbdirfd);
    }

    /* Get db lock */
    if ((db_locked = get_lock(LOCK_EXCL, dbpath)) == -1)
        goto exit_failure;
    if (db_locked != LOCK_EXCL) {
        /* Couldn't get exclusive lock, try shared lock if -e wasn't requested */
        if (exclusive) {
            dbd_log(LOGSTD, "Database is in use and exlusive was requested");
            goto exit_failure;
        }
        if ((db_locked = get_lock(LOCK_SHRD, NULL)) != LOCK_SHRD)
            goto exit_failure;
    }

    /* Check if -f is requested and wipe db if yes */
    if ((flags & DBD_FLAGS_FORCE) && rebuild && (volinfo.v_flags & AFPVOL_CACHE)) {
        char cmd[8 + MAXPATHLEN];
        if ((db_locked = get_lock(LOCK_FREE, NULL)) != 0)
            goto exit_failure;

        snprintf(cmd, 8 + MAXPATHLEN, "rm -rf \"%s\"", dbpath);
        dbd_log( LOGDEBUG, "Removing old database of volume: '%s'", volpath);
        system(cmd);
        if ((mkdir(dbpath, 0755)) != 0) {
            dbd_log( LOGSTD, "Can't create dbpath \"%s\": %s", dbpath, strerror(errno));
            exit(EXIT_FAILURE);
        }
        dbd_log( LOGDEBUG, "Removed old database.");
        if ((db_locked = get_lock(LOCK_EXCL, dbpath)) == -1)
            goto exit_failure;
    }

    /* 
       Lets start with the BerkeleyDB stuff
    */
    if ( ! nocniddb) {
        if ((dbd = dbif_init(dbpath, "cnid2.db")) == NULL)
            goto exit_failure;
        
        if (dbif_env_open(dbd,
                          &db_param,
                          (db_locked == LOCK_EXCL) ? (DBOPTIONS | DB_RECOVER) : DBOPTIONS) < 0) {
            dbd_log( LOGSTD, "error opening database!");
            goto exit_failure;
        }

        if (db_locked == LOCK_EXCL)
            dbd_log( LOGDEBUG, "Finished recovery.");

        if (dbif_open(dbd, NULL, rebuildindexes) < 0) {
            dbif_close(dbd);
            goto exit_failure;
        }

        /* Prepare upgrade ? We're done */
        if (prep_upgrade) {
            (void)dbif_txn_close(dbd, 1);
            goto cleanup;
        }
    }

    /* Downgrade db lock if not running exclusive */
    if (!exclusive && (db_locked == LOCK_EXCL)) {
        if (get_lock(LOCK_UNLOCK, NULL) != 0)
            goto exit_failure;
        if (get_lock(LOCK_SHRD, NULL) != LOCK_SHRD)
            goto exit_failure;
    }

    /* Now execute given command scan|rebuild|dump */
    if (dump && ! nocniddb) {
        if (dbif_dump(dbd, dumpindexes) < 0) {
            dbd_log( LOGSTD, "Error dumping database");
        }
    } else if ((rebuild && ! nocniddb) || scan) {
        if (cmd_dbd_scanvol(dbd, &volinfo, flags) < 0) {
            dbd_log( LOGSTD, "Error repairing database.");
        }
    }

cleanup:
    /* Cleanup */
    dbd_log(LOGDEBUG, "Closing db");
    if (! nocniddb) {
        if (dbif_close(dbd) < 0) {
            dbd_log( LOGSTD, "Error closing database");
            goto exit_failure;
        }
    }

exit_success:
    ret = 0;

exit_failure:
    get_lock(0, NULL);
    
    if ((fchdir(cdir)) < 0)
        dbd_log(LOGSTD, "fchdir: %s", strerror(errno));

    if (ret == 0)
        exit(EXIT_SUCCESS);
    else
        exit(EXIT_FAILURE);
}
