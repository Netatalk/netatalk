/*
 * $Id: cnid_index.c,v 1.5 2008-08-07 07:39:14 didg Exp $
 *
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#include <sys/param.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */
#include <time.h>
#include <sys/file.h>

#include <netatalk/endian.h>
#include <atalk/cnid_dbd_private.h>
#include <atalk/logger.h>

#define LOCKFILENAME  "lock"
static int exit_sig = 0;

/* Copy and past from etc/cnid_dbd/dbif.c */
#include <db.h>

#define DB_ERRLOGFILE "db_errlog"

static DB_ENV *db_env = NULL;
static DB_TXN *db_txn = NULL;
static FILE   *db_errlog = NULL;

#ifdef CNID_BACKEND_DBD_TXN
#define DBOPTIONS    (DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN)
#else
#define DBOPTIONS    (DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL | DB_PRIVATE) 
#endif

#define DBIF_DB_CNT 3

#define DBIF_IDX_CNID      0
#define DBIF_IDX_DEVINO    1
#define DBIF_IDX_DIDNAME   2

static struct db_table {
     char            *name;
     DB              *db;
     u_int32_t       general_flags;
     DBTYPE          type;
} db_table[] =
{
     { "cnid2.db",       NULL,      0, DB_BTREE},
     { "devino.db",      NULL,      0, DB_BTREE},
     { "didname.db",     NULL,      0, DB_BTREE},
};

static char *old_dbfiles[] = {"cnid.db", NULL};

/* --------------- */
int didname(dbp, pkey, pdata, skey)
DB *dbp _U_;
const DBT *pkey _U_, *pdata;
DBT *skey;
{
int len;
 
    memset(skey, 0, sizeof(DBT));
    skey->data = (char *)pdata->data + CNID_DID_OFS;
    /* FIXME: At least DB 4.0.14 and 4.1.25 pass in the correct length of
       pdata.size. strlen is therefore not needed. Also, skey should be zeroed
       out already. */
    len = strlen((char *)skey->data + CNID_DID_LEN);
    skey->size = CNID_DID_LEN + len + 1;
    return (0);
}
 
/* --------------- */
int devino(dbp, pkey, pdata, skey)
DB *dbp _U_;
const DBT *pkey _U_, *pdata;
DBT *skey;
{
    memset(skey, 0, sizeof(DBT));
    skey->data = (char *)pdata->data + CNID_DEVINO_OFS;
    skey->size = CNID_DEVINO_LEN;
    return (0);
}

/* --------------- */
static int  db_compat_associate (DB *p, DB *s,
                   int (*callback)(DB *, const DBT *,const DBT *, DBT *),
                   u_int32_t flags)
{
#if DB_VERSION_MAJOR > 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1)
    return p->associate(p, db_txn, s, callback, flags);
#else
#if (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR == 0)
    return p->associate(p,       s, callback, flags);
#else
    return 0;
#endif
#endif
}

/* --------------- */
static int db_compat_open(DB *db, char *file, char *name, DBTYPE type, int mode)
{
    int ret;

#if DB_VERSION_MAJOR > 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1)
    ret = db->open(db, db_txn, file, name, type, DB_CREATE, mode); 
#else
    ret = db->open(db,       file, name, type, DB_CREATE, mode); 
#endif

    if (ret) {
        LOG(log_error, logtype_cnid, "error opening database %s: %s", name, db_strerror(ret));
        return -1;
    } else {
        return 0;
    }
}

/* --------------- */
static int dbif_open(int do_truncate)
{
    int ret;
    int i;
    u_int32_t count;

    for (i = 0; i != DBIF_DB_CNT; i++) {
        if ((ret = db_create(&(db_table[i].db), db_env, 0))) {
            LOG(log_error, logtype_cnid, "error creating handle for database %s: %s", 
                db_table[i].name, db_strerror(ret));
            return -1;
        }

        if (db_table[i].general_flags) { 
            if ((ret = db_table[i].db->set_flags(db_table[i].db, db_table[i].general_flags))) {
                LOG(log_error, logtype_cnid, "error setting flags for database %s: %s", 
                    db_table[i].name, db_strerror(ret));
                return -1;
            }
        }

        if (db_compat_open(db_table[i].db, db_table[0].name, db_table[i].name, db_table[i].type, 0664) < 0)
            return -1;
        if (db_errlog != NULL)
            db_table[i].db->set_errfile(db_table[i].db, db_errlog);

        if (do_truncate && i > 0) {
	    if ((ret = db_table[i].db->truncate(db_table[i].db, db_txn, &count, 0))) {
                LOG(log_error, logtype_cnid, "error truncating database %s: %s", 
                    db_table[i].name, db_strerror(ret));
                return -1;
            }
        }
    }

    /* TODO: Implement CNID DB versioning info on new databases. */
    /* TODO: Make transaction support a runtime option. */
    /* Associate the secondary with the primary. */
    if ((ret = db_compat_associate(db_table[0].db, db_table[DBIF_IDX_DIDNAME].db, didname, (do_truncate)?DB_CREATE:0)) != 0) {
        LOG(log_error, logtype_cnid, "Failed to associate didname database: %s",db_strerror(ret));
        return -1;
    }
 
    if ((ret = db_compat_associate(db_table[0].db, db_table[DBIF_IDX_DEVINO].db, devino, (do_truncate)?DB_CREATE:0)) != 0) {
        LOG(log_error, logtype_cnid, "Failed to associate devino database: %s",db_strerror(ret));
	return -1;
    }

    return 0;
}

/* ------------------------ */
static int dbif_closedb(void)
{
    int i;
    int ret;
    int err = 0;

    for (i = DBIF_DB_CNT -1; i >= 0; i--) {
        if (db_table[i].db != NULL && (ret = db_table[i].db->close(db_table[i].db, 0))) {
            LOG(log_error, logtype_cnid, "error closing database %s: %s", db_table[i].name, db_strerror(ret));
            err++;
        }
    }
    if (err)
        return -1;
    return 0;
}

/* ------------------------ */
static int dbif_close(void)
{
    int ret;
    int err = 0;
    
    if (dbif_closedb()) 
    	err++;
     
    if (db_env != NULL && (ret = db_env->close(db_env, 0))) { 
        LOG(log_error, logtype_cnid, "error closing DB environment: %s", db_strerror(ret));
        err++;
    }
    if (db_errlog != NULL && fclose(db_errlog) == EOF) {
        LOG(log_error, logtype_cnid, "error closing DB logfile: %s", strerror(errno));
        err++;
    }
    if (err)
        return -1;
    return 0;
}
/* --------------- */
static int upgrade_required(void)
{
    int i;
    int found = 0;
    struct stat st;
    
    for (i = 0; old_dbfiles[i] != NULL; i++) {
	if ( !(stat(old_dbfiles[i], &st) < 0) ) {
	    found++;
	    continue;
	}
	if (errno != ENOENT) {
	    LOG(log_error, logtype_cnid, "cnid_open: Checking %s gave %s", old_dbfiles[i], strerror(errno));
	    found++;
	}
    }
    return found;
}

/* -------------------------- */
static int dbif_sync(void)
{
    int i;
    int ret;
    int err = 0;
     
    for (i = 0; i != /* DBIF_DB_CNT*/ 1; i++) {
        if ((ret = db_table[i].db->sync(db_table[i].db, 0))) {
            LOG(log_error, logtype_cnid, "error syncing database %s: %s", db_table[i].name, db_strerror(ret));
            err++;
        }
    }
 
    if (err)
        return -1;
    else
        return 0;
}

/* -------------------------- */
static int dbif_count(const int dbi, u_int32_t *count) 
{
    int ret;
    DB_BTREE_STAT *sp;
    DB *db = db_table[dbi].db;

#if DB_VERSION_MAJOR > 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 3)
    ret = db->stat(db, db_txn, &sp, 0);
#else
    ret = db->stat(db, &sp, 0);
#endif

    if (ret) {
        LOG(log_error, logtype_cnid, "error getting stat infotmation on database: %s", db_strerror(errno));
        return -1;
    }

    *count = sp->bt_ndata;
    free(sp);

    return 0;
}

/* -------------------------- */
static int dbd_check(char *dbdir)
{
    u_int32_t c_didname = 0, c_devino = 0, c_cnid = 0;

    LOG(log_debug, logtype_cnid, "CNID database at `%s' is being checked (quick)", dbdir);

    if (dbif_count(DBIF_IDX_CNID, &c_cnid)) 
        return -1;

    if (dbif_count(DBIF_IDX_DEVINO, &c_devino))
        return -1;

    /* bailout after the first error */
    if ( c_cnid != c_devino) {
        LOG(log_error, logtype_cnid, "CNID database at `%s' corrupted (%u/%u)", dbdir, c_cnid, c_devino);
        return 1;
    }

    if (dbif_count(DBIF_IDX_DIDNAME, &c_didname)) 
        return -1;

    if ( c_cnid != c_didname) {
        LOG(log_error, logtype_cnid, "CNID database at `%s' corrupted (%u/%u)", dbdir, c_cnid, c_didname);
        return 1;
    }

    LOG(log_debug, logtype_cnid, "CNID database at `%s' seems ok, %u entries.", dbdir, c_cnid);
    return 0;  
}

/* --------------- */
/*
 *  We assume our current directory is already the BDB homedir. Otherwise
 *  opening the databases will not work as expected. If we use transactions,
 *  dbif_env_init(), dbif_close() and dbif_stamp() are the only interface
 *  functions that can be called without a valid transaction handle in db_txn.
 */
static int dbif_env_init(void)
{
    int ret;
#ifdef CNID_BACKEND_DBD_TXN
    char **logfiles = NULL;
    char **file;
#endif

    /* Refuse to do anything if this is an old version of the CNID database */
    if (upgrade_required()) {
	LOG(log_error, logtype_cnid, "Found version 1 of the CNID database. Please upgrade to version 2");
	return -1;
    }

    if ((db_errlog = fopen(DB_ERRLOGFILE, "a")) == NULL)
        LOG(log_warning, logtype_cnid, "error creating/opening DB errlogfile: %s", strerror(errno));

#ifdef CNID_BACKEND_DBD_TXN
    if ((ret = db_env_create(&db_env, 0))) {
        LOG(log_error, logtype_cnid, "error creating DB environment: %s", 
            db_strerror(ret));
        db_env = NULL;
        return -1;
    }    
    if (db_errlog != NULL)
        db_env->set_errfile(db_env, db_errlog); 
    db_env->set_verbose(db_env, DB_VERB_RECOVERY, 1);
    if ((ret = db_env->open(db_env, ".", DBOPTIONS | DB_PRIVATE | DB_RECOVER, 0))) {
        LOG(log_error, logtype_cnid, "error opening DB environment: %s", 
            db_strerror(ret));
        db_env->close(db_env, 0);
        db_env = NULL;
        return -1;
    }

    if (db_errlog != NULL)
        fflush(db_errlog);

    if ((ret = db_env->close(db_env, 0))) {
        LOG(log_error, logtype_cnid, "error closing DB environment after recovery: %s", 
            db_strerror(ret));
        db_env = NULL;
        return -1;
    }
#endif
    if ((ret = db_env_create(&db_env, 0))) {
        LOG(log_error, logtype_cnid, "error creating DB environment after recovery: %s",
            db_strerror(ret));
        db_env = NULL;
        return -1;
    }
    if (db_errlog != NULL)
        db_env->set_errfile(db_env, db_errlog);
    if ((ret = db_env->open(db_env, ".", DBOPTIONS , 0))) {
        LOG(log_error, logtype_cnid, "error opening DB environment after recovery: %s",
            db_strerror(ret));
        db_env->close(db_env, 0);
        db_env = NULL;
        return -1;      
    }

#ifdef CNID_BACKEND_DBD_TXN
    if (db_env->log_archive(db_env, &logfiles, 0)) {
	LOG(log_error, logtype_cnid, "error getting list of stale logfiles: %s",
            db_strerror(ret));
        db_env->close(db_env, 0);
        db_env = NULL;
        return -1;
    }
    if (logfiles != NULL) {
	for (file = logfiles; *file != NULL; file++) {
	    if (unlink(*file) < 0)
		LOG(log_warning, logtype_cnid, "Error removing stale logfile %s: %s", *file, strerror(errno));
	}
	free(logfiles);
    }
#endif
    return 0;
}


static void sig_exit(int signo)
{
    exit_sig = signo;
    return;
}

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

/* ------------------------ */
int get_lock(void)
{
    int lockfd;
    struct flock lock;

    if ((lockfd = open(LOCKFILENAME, O_RDWR | O_CREAT, 0644)) < 0) {
	LOG(log_error, logtype_cnid, "main: error opening lockfile: %s", strerror(errno));
	exit(1);
    }
    
    lock.l_start  = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len    = 0;
    lock.l_type   = F_WRLCK;

    if (fcntl(lockfd, F_SETLK, &lock) < 0) {
	if (errno == EACCES || errno == EAGAIN) {
	    exit(0);
	} else {
	    LOG(log_error, logtype_cnid, "main: fcntl F_WRLCK lockfile: %s", strerror(errno));
	    exit(1);
	}
    }
    
    return lockfd;
}

/* ----------------------- */
void set_signal(void)
{
    struct sigaction sv;

    sv.sa_handler = sig_exit;
    sv.sa_flags = 0;
    sigemptyset(&sv.sa_mask);
    sigaddset(&sv.sa_mask, SIGINT);
    sigaddset(&sv.sa_mask, SIGTERM);
    if (sigaction(SIGINT, &sv, NULL) < 0 || sigaction(SIGTERM, &sv, NULL) < 0) {
        LOG(log_error, logtype_cnid, "main: sigaction: %s", strerror(errno));
        exit(1);
    }
    sv.sa_handler = SIG_IGN;
    sigemptyset(&sv.sa_mask);
    if (sigaction(SIGPIPE, &sv, NULL) < 0) {
        LOG(log_error, logtype_cnid, "main: sigaction: %s", strerror(errno));
        exit(1);
    }
}

/* ----------------------- */
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

/* ------------------------ */
int main(int argc, char *argv[])
{
#ifdef CNID_BACKEND_DBD_TXN

    fprintf(stderr, "%s doesn't work with db transaction enabled\n", argv[0]);
    exit (1);
    
#else
    int err = 0;
    int ret;
    int lockfd;
    char *dir;
       
    set_processname("cnid_index");
    syslog_setup(log_debug, logtype_default, logoption_ndelay | logoption_pid, logfacility_daemon);
    
    if (argc  != 2) {
        LOG(log_error, logtype_cnid, "main: not enough arguments");
        exit(1);
    }

    dir = argv[1];

    if (chdir(dir) < 0) {
        LOG(log_error, logtype_cnid, "chdir to %s failed: %s", dir, strerror(errno));
        exit(1);
    }
    
    /* Before we do anything else, check if there is an instance of cnid_dbd
       running already and silently exit if yes. */
    lockfd = get_lock();
    
    LOG(log_info, logtype_cnid, "Startup, DB dir %s", dir);
    
    set_signal();
    
    /* SIGINT and SIGTERM are always off, unless we get a return code of 0 from
       comm_rcv (no requests for one second, see above in loop()). That means we
       only shut down after one second of inactivity. */
    block_sigs_onoff(1);

    if (dbif_env_init() < 0)
        exit(2); /* FIXME: same exit code as failure for dbif_open() */  
    
#ifdef CNID_BACKEND_DBD_TXN
    if (dbif_txn_begin() < 0)
	exit(6);
#endif
    
    if (dbif_open(0) < 0) {
#ifdef CNID_BACKEND_DBD_TXN
	dbif_txn_abort();
#endif
        dbif_close();
        exit(2);
    }

#ifndef CNID_BACKEND_DBD_TXN
    if ((ret = dbd_check(dir))) {
        if (ret < 0) {
            dbif_close();
            exit(2);
        }
        dbif_closedb();
	LOG(log_info, logtype_cnid, "main: re-opening, secondaries will be rebuilt. This may take some time");
        if (dbif_open(1) < 0) {
	    LOG(log_info, logtype_cnid, "main: re-opening databases failed");
            dbif_close();
            exit(2);
        }
	LOG(log_info, logtype_cnid, "main: rebuilt done");
    }
#endif

#ifdef CNID_BACKEND_DBD_TXN
    if (dbif_txn_commit() < 0)
	exit(6);
#endif

#ifndef CNID_BACKEND_DBD_TXN
    /* FIXME: Do we really need to sync before closing the DB? Just closing it
       should be enough. */
    if (dbif_sync() < 0)
        err++;
#endif

    if (dbif_close() < 0)
        err++;
        
    free_lock(lockfd);
    
    if (err)
        exit(4);
#endif
    return 0;
}
