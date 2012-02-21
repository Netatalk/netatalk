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
#include <atalk/globals.h>
#include <atalk/netatalk_conf.h>
#include <atalk/util.h>

#include "cmd_dbd.h"
#include "dbd.h"
#include "dbif.h"
#include "db_param.h"
#include "pack.h"

#define DBOPTIONS (DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN)

int nocniddb = 0;               /* Dont open CNID database, only scan filesystem */
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
    printf("dbd (Netatalk %s)\n"
           "Usage: dbd [-CeFtvx] -d [-i] | -s [-c|-n]| -r [-c|-f] | -u <path to netatalk volume>\n"
           "dbd can dump, scan, reindex and rebuild Netatalk dbd CNID databases.\n"
           "dbd must be run with appropiate permissions i.e. as root.\n\n"
           "Main commands are:\n"
           "   -d Dump CNID database\n"
           "      Option: -i dump indexes too\n\n"
           "   -s Scan volume:\n"
           "      Options: -c Don't check .AppleDouble stuff, only ckeck orphaned.\n"
           "               -n Don't open CNID database, skip CNID checks.\n\n"
           "   -r Rebuild volume:\n"
           "      Options: -c Don't create .AppleDouble stuff, only cleanup orphaned.\n"
           "               -f wipe database and rebuild from IDs stored in AppleDouble\n"
           "                  metadata file or EA. Implies -e.\n\n"
           "   -u Upgrade:\n"
           "      Opens the database which triggers any necessary upgrades,\n"
           "      then closes and exits.\n\n"
           "General options:\n"
           "   -C convert from adouble:v2 to adouble:ea (use with -r)\n"
           "   -F location of the afp.conf config file\n"
           "   -e only work on inactive volumes and lock them (exclusive)\n"
           "   -x rebuild indexes (just for completeness, mostly useless!)\n"
           "   -t show statistics while running\n"
           "   -v verbose\n\n"
           , VERSION
        );
}

int main(int argc, char **argv)
{
    int c, lockfd, ret = -1;
    int dump=0, scan=0, rebuild=0, prep_upgrade=0, rebuildindexes=0, dumpindexes=0, force=0;
    dbd_flags_t flags = 0;
    char *volpath;
    int cdir;
    AFPObj obj = { 0 };
    struct vol *vol;

    if (geteuid() != 0) {
        usage();
        exit(EXIT_FAILURE);
    }
    /* Inhereting perms in ad_mkdir etc requires this */
    ad_setfuid(0);

    while ((c = getopt(argc, argv, ":cCdefFinrstuvx")) != -1) {
        switch(c) {
        case 'c':
            flags |= DBD_FLAGS_CLEANUP;
            break;
        case 'C':
            flags |= DBD_FLAGS_V2TOEA;
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
        case 'F':
            obj.cmdlineconfigfile = strdup(optarg);
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
        setuplog("default:info", "/dev/tty");
    else
        setuplog("default:debug", "/dev/tty");

    /* Load config */
    if (afp_config_parse(&obj) != 0) {
        dbd_log( LOGSTD, "Couldn't load afp.conf");
        exit(EXIT_FAILURE);
    }

    if (load_volumes(&obj, NULL) != 0) {
        dbd_log( LOGSTD, "Couldn't load volumes");
        exit(EXIT_FAILURE);
    }

    if ((vol = getvolbypath(volpath)) == NULL) {
        dbd_log( LOGSTD, "Couldn't find volume for '%s'", volpath);
        exit(EXIT_FAILURE);
    }

    if (load_charset(vol) != 0) {
        dbd_log( LOGSTD, "Couldn't load charsets for '%s'", volpath);
        exit(EXIT_FAILURE);
    }

    pack_setvol(vol);

    if (vol->v_adouble == AD_VERSION_EA)
        dbd_log( LOGDEBUG, "adouble:ea volume");
    else if (vol->v_adouble == AD_VERSION2)
        dbd_log( LOGDEBUG, "adouble:v2 volume");
    else {
        dbd_log( LOGSTD, "unknown adouble volume");
        exit(EXIT_FAILURE);
    }

    /* -C v2 to ea conversion only on adouble:ea volumes */
    if ((flags & DBD_FLAGS_V2TOEA) && (vol->v_adouble!= AD_VERSION_EA)) {
        dbd_log( LOGSTD, "Can't run adouble:v2 to adouble:ea conversion because not an adouble:ea volume");
        exit(EXIT_FAILURE);
    }

    /* Sanity checks to ensure we can touch this volume */
    if (vol->v_vfs_ea != AFPVOL_EA_AD && vol->v_vfs_ea != AFPVOL_EA_SYS) {
        dbd_log( LOGSTD, "Unknown Extended Attributes option: %u", vol->v_vfs_ea);
        exit(EXIT_FAILURE);        
    }

    /* Enuser dbpath is there, create if necessary */
    struct stat st;
    if (stat(vol->v_dbpath, &st) != 0) {
        if (errno != ENOENT) {
            dbd_log( LOGSTD, "Can't stat dbpath \"%s\": %s", vol->v_dbpath, strerror(errno));
            exit(EXIT_FAILURE);        
        }
        if ((mkdir(vol->v_dbpath, 0755)) != 0) {
            dbd_log( LOGSTD, "Can't create dbpath \"%s\": %s", vol->v_dbpath, strerror(errno));
            exit(EXIT_FAILURE);
        }        
    }

    /* Put "/.AppleDB" at end of volpath, get path from volinfo file */
    if ( (strlen(vol->v_dbpath) + strlen("/.AppleDB")) > MAXPATHLEN ) {
        dbd_log( LOGSTD, "Volume pathname too long");
        exit(EXIT_FAILURE);        
    }
    strncpy(dbpath, vol->v_dbpath, MAXPATHLEN - strlen("/.AppleDB"));
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
        goto exit_noenv;
    if (db_locked != LOCK_EXCL) {
        /* Couldn't get exclusive lock, try shared lock if -e wasn't requested */
        if (exclusive) {
            dbd_log(LOGSTD, "Database is in use and exlusive was requested");
            goto exit_noenv;
        }
        if ((db_locked = get_lock(LOCK_SHRD, NULL)) != LOCK_SHRD)
            goto exit_noenv;
    }

    /* Check if -f is requested and wipe db if yes */
    if ((flags & DBD_FLAGS_FORCE) && rebuild) {
        char cmd[8 + MAXPATHLEN];
        if ((db_locked = get_lock(LOCK_FREE, NULL)) != 0)
            goto exit_noenv;

        snprintf(cmd, 8 + MAXPATHLEN, "rm -rf \"%s\"", dbpath);
        dbd_log( LOGDEBUG, "Removing old database of volume: '%s'", volpath);
        system(cmd);
        if ((mkdir(dbpath, 0755)) != 0) {
            dbd_log( LOGSTD, "Can't create dbpath \"%s\": %s", dbpath, strerror(errno));
            exit(EXIT_FAILURE);
        }
        dbd_log( LOGDEBUG, "Removed old database.");
        if ((db_locked = get_lock(LOCK_EXCL, dbpath)) == -1)
            goto exit_noenv;
    }

    /* 
       Lets start with the BerkeleyDB stuff
    */
    if ( ! nocniddb) {
        if ((dbd = dbif_init(dbpath, "cnid2.db")) == NULL)
            goto exit_noenv;
        
        if (dbif_env_open(dbd,
                          &db_param,
                          (db_locked == LOCK_EXCL) ? (DBOPTIONS | DB_RECOVER) : DBOPTIONS) < 0) {
            dbd_log( LOGSTD, "error opening database!");
            goto exit_noenv;
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
        if (cmd_dbd_scanvol(dbd, vol, flags) < 0) {
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
    if (dbif_env_remove(dbpath) < 0) {
        dbd_log( LOGSTD, "Error removing BerkeleyDB database environment");
        ret++;
    }
    get_lock(0, NULL);

exit_noenv:    
    if ((fchdir(cdir)) < 0)
        dbd_log(LOGSTD, "fchdir: %s", strerror(errno));

    if (ret == 0)
        exit(EXIT_SUCCESS);
    else
        exit(EXIT_FAILURE);
}
