/*
 * $Id: main.c,v 1.2 2005-04-28 20:49:48 bfernhomberg Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
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

#include "db_param.h"
#include "dbif.h"
#include "dbd.h"
#include "comm.h"


#define LOCKFILENAME  "lock"

static int exit_sig = 0;


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

/* 
   The dbd_XXX and comm_XXX functions all obey the same protocol for return values:
   
   1: Success, if transactions are used commit.
   0: Failure, but we continue to serve requests. If transactions are used abort/rollback.
  -1: Fatal error, either from the database or from the socket. Abort the transaction if applicable 
      (which might fail as well) and then exit.

  We always try to notify the client process about the outcome, the result field
  of the cnid_dbd_rply structure contains further details.

 */

static int loop(struct db_param *dbp)
{
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply;
    int ret, cret;
    time_t now, time_next_flush, time_last_rqst;
    int count;
    static char namebuf[MAXPATHLEN + 1];
    u_int32_t checkp_flags;

    count = 0;
    now = time(NULL);
    time_next_flush = now + dbp->flush_interval;
    time_last_rqst = now;
    if (dbp->nosync)
        checkp_flags = DB_FORCE;
    else 
        checkp_flags = 0;
    
    rqst.name = namebuf;

    while (1) {
        if ((cret = comm_rcv(&rqst)) < 0)
            return -1;

        now = time(NULL);
        
        if (count > dbp->flush_frequency || now > time_next_flush) {
#ifdef CNID_BACKEND_DBD_TXN
            if (dbif_txn_checkpoint(0, 0, checkp_flags) < 0)
                return -1;
#else
            if (dbif_sync() < 0)
                return -1;
#endif
            count = 0;
            time_next_flush = now + dbp->flush_interval;
        }

        if (cret == 0) {
            block_sigs_onoff(0);
            block_sigs_onoff(1);
            if (exit_sig)
                return 0;
	    if (dbp->idle_timeout && comm_nbe() <= 0 && (now - time_last_rqst) > dbp->idle_timeout)
		return 0;
	    continue;
        }
        /* We got a request */
	time_last_rqst = now;
        count++;
        
#ifdef CNID_BACKEND_DBD_TXN
        if (dbif_txn_begin() < 0) 
            return -1;
#endif /* CNID_BACKEND_DBD_TXN */

	memset(&rply, 0, sizeof(rply));
        switch(rqst.op) {
            /* ret gets set here */
        case CNID_DBD_OP_OPEN:
        case CNID_DBD_OP_CLOSE:
            /* open/close are noops for now. */
            rply.namelen = 0; 
            ret = 1;
            break;
        case CNID_DBD_OP_ADD:
            ret = dbd_add(&rqst, &rply);
            break;
        case CNID_DBD_OP_GET:
            ret = dbd_get(&rqst, &rply);
            break;
        case CNID_DBD_OP_RESOLVE:
            ret = dbd_resolve(&rqst, &rply);
            break;
        case CNID_DBD_OP_LOOKUP:
            ret = dbd_lookup(&rqst, &rply);
            break;
        case CNID_DBD_OP_UPDATE:
            ret = dbd_update(&rqst, &rply);
            break;
        case CNID_DBD_OP_DELETE:
            ret = dbd_delete(&rqst, &rply);
            break;
        case CNID_DBD_OP_GETSTAMP:
            ret = dbd_getstamp(&rqst, &rply);
            break;
        case CNID_DBD_OP_REBUILD_ADD:
            ret = dbd_rebuild_add(&rqst, &rply);
            break;
        default:
            LOG(log_error, logtype_cnid, "loop: unknown op %d", rqst.op);
            ret = -1;
            break;
        }       

        if ((cret = comm_snd(&rply)) < 0 || ret < 0) {
#ifdef CNID_BACKEND_DBD_TXN
            dbif_txn_abort();
#endif /* CNID_BACKEND_DBD_TXN */
            return -1;
        }
#ifdef CNID_BACKEND_DBD_TXN
        if (ret == 0 || cret == 0) {
            if (dbif_txn_abort() < 0) 
                return -1;
        } else {
            if (dbif_txn_commit() < 0) 
                return -1;
        }
#endif /* CNID_BACKEND_DBD_TXN */
    }
}

/* ------------------------ */
static void switch_to_user(char *dir)
{
    struct stat st;

    if (chdir(dir) < 0) {
        LOG(log_error, logtype_cnid, "chdir to %s failed: %s", dir, strerror(errno));
        exit(1);
    }
    
    if (stat(".", &st) < 0) {
        LOG(log_error, logtype_cnid, "error in stat for %s: %s", dir, strerror(errno));
        exit(1);
    }
    if (!getuid()) {
        LOG(log_info, logtype_cnid, "Setting uid/gid to %i/%i", st.st_uid, st.st_gid);
        if (setgid(st.st_gid) < 0 || setuid(st.st_uid) < 0) {
            LOG(log_error, logtype_cnid, "uid/gid: %s", strerror(errno));
            exit(1);
        }
    }
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
    struct db_param *dbp;
    int err = 0;
    int ret;
    int lockfd, ctrlfd, clntfd;
    char *dir;
       
    set_processname("cnid_dbd");
    syslog_setup(log_debug, logtype_default, logoption_ndelay | logoption_pid, logfacility_daemon);
    
    if (argc  != 4) {
        LOG(log_error, logtype_cnid, "main: not enough arguments");
        exit(1);
    }

    dir = argv[1];
    ctrlfd = atoi(argv[2]);
    clntfd = atoi(argv[3]);

    switch_to_user(dir);
    
    /* Before we do anything else, check if there is an instance of cnid_dbd
       running already and silently exit if yes. */
    lockfd = get_lock();
    
    LOG(log_info, logtype_cnid, "Startup, DB dir %s", dir);
    
    set_signal();
    
    /* SIGINT and SIGTERM are always off, unless we get a return code of 0 from
       comm_rcv (no requests for one second, see above in loop()). That means we
       only shut down after one second of inactivity. */
    block_sigs_onoff(1);

    if ((dbp = db_param_read(dir)) == NULL)
        exit(1);

    if (dbif_env_init(dbp) < 0)
        exit(2); /* FIXME: same exit code as failure for dbif_open() */  
    
#ifdef CNID_BACKEND_DBD_TXN
    if (dbif_txn_begin() < 0)
	exit(6);
#endif
    
    if (dbif_open(dbp, 0) < 0) {
#ifdef CNID_BACKEND_DBD_TXN
	dbif_txn_abort();
#endif
        dbif_close();
        exit(2);
    }

#ifndef CNID_BACKEND_DBD_TXN
    if (dbp->check && (ret = dbd_check(dir))) {
        if (ret < 0) {
            dbif_close();
            exit(2);
        }
        dbif_closedb();
	LOG(log_info, logtype_cnid, "main: re-opening, secondaries will be rebuilt. This may take some time");
        if (dbif_open(dbp, 1) < 0) {
	    LOG(log_info, logtype_cnid, "main: re-opening databases failed");
            dbif_close();
            exit(2);
        }
	LOG(log_info, logtype_cnid, "main: rebuilt done");
    }
#endif

    if (dbd_stamp() < 0) {
#ifdef CNID_BACKEND_DBD_TXN
	dbif_txn_abort();
#endif
        dbif_close();
        exit(5);
    }
#ifdef CNID_BACKEND_DBD_TXN
    if (dbif_txn_commit() < 0)
	exit(6);
#endif

    if (comm_init(dbp, ctrlfd, clntfd) < 0) {
        dbif_close();
        exit(3);
    }

    if (loop(dbp) < 0)
        err++;

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
    else if (exit_sig)
	LOG(log_info, logtype_cnid, "main: Exiting on signal %i", exit_sig);
    else
	LOG(log_info, logtype_cnid, "main: Idle timeout, exiting");
    
    return 0;
}
