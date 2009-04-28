/* 
   $Id: dbd.c,v 1.2 2009-04-28 14:09:00 franklahm Exp $

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
#include "dbif.h"
#include "db_param.h"

#define LOCKFILENAME  "lock"
#define DBOPTIONS (DB_CREATE | DB_INIT_LOCK | DB_INIT_MPOOL | DB_INIT_TXN)

enum logtype {LOGSTD, LOGDEBUG};

static volatile sig_atomic_t alarmed;
static int verbose; /* Logging flag */
static int exclusive;  /* How to open the bdb database */
static struct volinfo volinfo;
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
static void dblog(enum logtype lt, char *fmt, ...)
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
        dblog( LOGSTD, "error in sigaction(SIGTERM): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        

    memset(&sv, 0, sizeof(struct sigaction));
    sv.sa_handler = SIG_IGN;
    sigemptyset(&sv.sa_mask);

    if (sigaction(SIGINT, &sv, NULL) < 0) {
        dblog( LOGSTD, "error in sigaction(SIGINT): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGABRT, &sv, NULL) < 0) {
        dblog( LOGSTD, "error in sigaction(SIGABRT): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGHUP, &sv, NULL) < 0) {
        dblog( LOGSTD, "error in sigaction(SIGHUP): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGQUIT, &sv, NULL) < 0) {
        dblog( LOGSTD, "error in sigaction(SIGQUIT): %s", strerror(errno));
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
        dblog( LOGSTD, "Error opening lockfile: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    lock.l_start  = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len    = 0;
    lock.l_type   = F_WRLCK;

    if (fcntl(lockfd, F_SETLK, &lock) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            if (exclusive) {
                dblog( LOGDEBUG, "Database is in use and exlusive was requested", strerror(errno));        
                exit(EXIT_FAILURE);
            };
        } else {
            dblog( LOGSTD, "Error getting fcntl F_WRLCK on lockfile: %s", strerror(errno));
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
           "dbd must run with appropiate permissions i.e. as root.\n\n"
           "Main commands are:\n"
           "   -d dump\n"
           "      Dump CNID database\n"
           "      Option: -i dump indexes too\n"
           "   -s scan\n"
           "      Compare database with volume\n"
           "   -r rebuild\n"
           "      Rebuild CNID database\n"
           "      Option: -f wipe database and rebuild from IDs stored in AppleDouble files\n\n"
           "General options:\n"
           "   -e only work on inactive volumes and lock them (exclusive)\n"
           "   -x rebuild indexes\n"
           "   -v verbose\n"
           "\n"
           "28-04-2009: -s and -r and not yet implemented\n"
        );
}

int main(int argc, char **argv)
{
    int c, ret, lockfd;
    int dump=0, scan=0, rebuild=0, rebuildindexes=0, dumpindexes=0, force=0;
    char *volpath;

    if (geteuid() != 0) {
        usage();
        exit(EXIT_FAILURE);
    }

    while ((c = getopt(argc, argv, ":dsrvxif")) != -1) {
        switch(c) {
        case 'd':
            dump = 1;
            break;
        case 'i':
            dumpindexes = 1;
            break;
        case 's':
            scan = 1;
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
        dblog( LOGSTD, "Volume pathname too long");
        exit(EXIT_FAILURE);        
    }
    char dbpath[PATH_MAX];
    strncpy(dbpath, volpath, PATH_MAX - 1);
    strcat(dbpath, "/.AppleDB");

    if ( -1 == (ret = loadvolinfo(volpath, &volinfo)) ) {
        dblog( LOGSTD, "Unkown volume options!");
        exit(EXIT_FAILURE);
    }
    
    if (chdir(dbpath) < 0) {
        dblog( LOGSTD, "chdir to %s failed: %s", dbpath, strerror(errno));
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

    /* 
       Lets start with the BerkeleyDB stuff
    */
    if (dbif_env_init(&db_param, DBOPTIONS) < 0) {
        dblog( LOGSTD, "error opening database!");
        exit(EXIT_FAILURE);
    }

    dblog( LOGDEBUG, "Finished opening BerkeleyDB databases including recovery.");

    if (dbif_open(&db_param, rebuildindexes) < 0) {
        dbif_close();
        exit(EXIT_FAILURE);
    }

    if (dump) {
        if (dbif_dump(dumpindexes) < 0) {
            dblog( LOGSTD, "Error dumping database");
        }
    }

    if (dbif_close() < 0)
        exit(EXIT_FAILURE);

    free_lock(lockfd);

    return 0;
}
