/* 
   $Id: dbd.c,v 1.3 2009-05-06 11:54:24 franklahm Exp $

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
#include <atalk/volinfo.h>
#include "cmd_dbd.h"
#include "dbif.h"
#include "db_param.h"

#define LOCKFILENAME  "lock"
#define DBOPTIONS (DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN)

static DBD *dbd;

static volatile sig_atomic_t alarmed;
static int verbose;             /* Logging flag */
static int exclusive;           /* Exclusive volume access */
static struct db_param db_param = {
    NULL,                       /* Volume dirpath */
    1,                          /* bdb logfile autoremove */
    16384,                      /* bdb cachesize */
    -1,                         /* not used ... */
    -1,
    "",
    -1,
    -1,
    -1
};

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
   ignore everything except SIGTERM which we catch and which causes
   a clean exit.
 */

static void sig_handler(int signo)
{
    alarmed = 1;
    return;
}

void set_signal(void)
{
    struct sigaction sv;

    sv.sa_handler = sig_handler;
    sv.sa_flags = SA_RESTART;
    sigemptyset(&sv.sa_mask);
    sigaddset(&sv.sa_mask, SIGTERM);
    if (sigaction(SIGTERM, &sv, NULL) < 0) {
        dbd_log( LOGSTD, "error in sigaction(SIGTERM): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        

    memset(&sv, 0, sizeof(struct sigaction));
    sv.sa_handler = SIG_IGN;
    sigemptyset(&sv.sa_mask);

    if (sigaction(SIGINT, &sv, NULL) < 0) {
        dbd_log( LOGSTD, "error in sigaction(SIGINT): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
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

#if 0
static void block_sigs_onoff(int block)
{
    sigset_t set;
    
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGTERM);
    if (block)
        sigprocmask(SIG_BLOCK, &set, NULL);
    else
        sigprocmask(SIG_UNBLOCK, &set, NULL);
    return;
}
#endif

int get_lock(void)
{
    int lockfd;
    struct flock lock;

    if ((lockfd = open(LOCKFILENAME, O_RDWR | O_CREAT, 0644)) < 0) {
        dbd_log( LOGSTD, "Error opening lockfile: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    lock.l_start  = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len    = 0;
    lock.l_type   = F_WRLCK;

    if (fcntl(lockfd, F_SETLK, &lock) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            if (exclusive) {
                dbd_log( LOGSTD, "Database is in use and exlusive was requested", strerror(errno));        
                exit(EXIT_FAILURE);
            };
        } else {
            dbd_log( LOGSTD, "Error getting fcntl F_WRLCK on lockfile: %s", strerror(errno));
            exit(EXIT_FAILURE);
       }
    }
    
    return lockfd;
}

void free_lock(int lockfd)
{
    struct flock lock;

    lock.l_start  = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len    = 0;
    lock.l_type = F_UNLCK;
    fcntl(lockfd, F_SETLK, &lock);
    close(lockfd);
}

static void usage ()
{
    printf("Usage: dbd [-e|-v|-x] -d [-i] | -s | -r [-f] <path to netatalk volume>\n"
           "dbd can dump, scan, reindex and rebuild Netatalk dbd CNID databases.\n"
           "dbd must be run with appropiate permissions i.e. as root.\n\n"
           "Main commands are:\n"
           "   -d Dump CNID database\n"
           "      Option: -i dump indexes too\n"
           "   -s Scan volume:\n"
           "      1. Compare database with volume\n"
           "      2. Check if .AppleDouble dirs exist\n"
           "      3. Check if  AppleDouble file exist\n"
           "      4. Report orphaned AppleDouble files\n"
           "      5. Check for directories inside AppleDouble directories\n"
           "      6. Check name encoding by roundtripping, log on error\n"
           "   -r Rebuild volume:\n"
           "      1. Sync database with volume\n"
           "      2. Make sure .AppleDouble dir exist, create if missing\n"
           "      3. Make sure AppleDouble file exists, create if missing\n"
           "      4. Delete orphaned AppleDouble files\n"
           "      5. Check for directories inside AppleDouble directories\n"
           "      6. Check name encoding by roundtripping, log on error\n"
           "      Option: -f wipe database and rebuild from IDs stored in AppleDouble files,\n"
           "                 only available for volumes with 'cachecnid' option\n\n"
           "General options:\n"
           "   -e only work on inactive volumes and lock them (exclusive)\n"
           "   -x rebuild indexes\n"
           "   -v verbose\n"
           "\n"
           "05-05-2009: -s and -r already check/repair the AppleDouble stuff and encoding,\n"
           "            no CNID database maintanance is done yet\n"
        );
}

int main(int argc, char **argv)
{
    int c, lockfd;
    int dump=0, scan=0, rebuild=0, rebuildindexes=0, dumpindexes=0, force=0;
    dbd_flags_t flags = 0;
    char *volpath;
    struct volinfo volinfo;

    if (geteuid() != 0) {
        usage();
        exit(EXIT_FAILURE);
    }

    while ((c = getopt(argc, argv, ":dsrvxife")) != -1) {
        switch(c) {
        case 'd':
            dump = 1;
            break;
        case 'i':
            dumpindexes = 1;
            break;
        case 's':
            scan = 1;
            flags = DBD_FLAGS_SCAN;
            break;
        case 'r':
            rebuild = 1;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'e':
            exclusive = 1;
            break;
        case 'x':
            rebuildindexes = 1;
            break;
        case 'f':
            force = 1;
            flags = DBD_FLAGS_FORCE;
            break;
        case ':':
        case '?':
            usage();
            exit(EXIT_FAILURE);
            break;
        }
    }

    if ((dump + scan + rebuild) != 1) {
        usage();
        exit(EXIT_FAILURE);
    }

    if ( (optind + 1) != argc ) {
        usage();
        exit(EXIT_FAILURE);
    }
    volpath = argv[optind];

    /* Put "/.AppleDB" at end of volpath */
    if ( (strlen(volpath) + strlen("/.AppleDB")) > (PATH_MAX -1) ) {
        dbd_log( LOGSTD, "Volume pathname too long");
        exit(EXIT_FAILURE);        
    }
    char dbpath[PATH_MAX];
    strncpy(dbpath, volpath, PATH_MAX - 1);
    strcat(dbpath, "/.AppleDB");

    /* cd to .AppleDB dir */
    int cdir;
    if ((cdir = open(".", O_RDONLY)) < 0) {
        dbd_log( LOGSTD, "Can't open dir: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (chdir(dbpath) < 0) {
        dbd_log( LOGSTD, "chdir to %s failed: %s", dbpath, strerror(errno));
        exit(EXIT_FAILURE);
    }
        
    /* 
       Before we do anything else, check if there is an instance of cnid_dbd
       running already and silently exit if yes.
    */
    lockfd = get_lock();
    
    /* Ignore everything except SIGTERM */
    set_signal();

    /* Setup logging. Should be portable among *NIXes */
    if (!verbose)
        setuplog("default log_info /dev/tty");
    else
        setuplog("default log_debug /dev/tty");

    /* Load .volinfo file */
    if ( -1 == loadvolinfo(volpath, &volinfo)) {
        dbd_log( LOGSTD, "Unkown volume options!");
        exit(EXIT_FAILURE);
    }
    if ( -1 == vol_load_charsets(&volinfo)) {
        dbd_log( LOGSTD, "Error loading charsets!");
        exit(EXIT_FAILURE);
    }

    /* 
       Lets start with the BerkeleyDB stuff
    */
    if (NULL == (dbd = dbif_init("cnid2.db")))
        exit(2);

    if (dbif_env_open(dbd, &db_param, DBOPTIONS) < 0) {
        dbd_log( LOGSTD, "error opening database!");
        exit(EXIT_FAILURE);
    }

    dbd_log( LOGDEBUG, "Finished opening BerkeleyDB databases including recovery.");

    if (dbif_open(dbd, &db_param, rebuildindexes) < 0) {
        dbif_close(dbd);
        exit(EXIT_FAILURE);
    }

    if ((fchdir(cdir)) < 0) {
        dbd_log(LOGSTD, "fchdir: %s", strerror(errno));        
        goto cleanup;
    }

    if (dump) {
        if (dbif_dump(dbd, dumpindexes) < 0) {
            dbd_log( LOGSTD, "Error dumping database");
        }
    } else if (rebuild || scan) {
        if (cmd_dbd_scanvol(dbd, &volinfo, flags) < 0) {
            dbd_log( LOGSTD, "Fatal error repairing database. Please rm -r it.");
        }
    }

cleanup:
    if (dbif_close(dbd) < 0)
        exit(EXIT_FAILURE);

    free_lock(lockfd);

    return 0;
}
