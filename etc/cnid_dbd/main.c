/*
 * $Id: main.c,v 1.8 2009-05-20 10:29:22 franklahm Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * Copyright (c) Frank Lahm 2009
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

/* 
   Note: DB_INIT_LOCK is here so we can run the db_* utilities while netatalk is running.
   It's a likey performance hit, but it might we worth it.
 */
#define DBOPTIONS (DB_CREATE | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_LOCK | DB_INIT_TXN | DB_RECOVER)

static DBD *dbd;

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
    int count;
    time_t now, time_next_flush, time_last_rqst;
    char timebuf[64];
    static char namebuf[MAXPATHLEN + 1];

    count = 0;
    now = time(NULL);
    time_next_flush = now + dbp->flush_interval;
    time_last_rqst = now;

    rqst.name = namebuf;

    strftime(timebuf, 63, "%b %d %H:%M:%S.",localtime(&time_next_flush));
    LOG(log_debug, logtype_cnid, "Checkpoint interval: %d seconds. Next checkpoint: %s",
        dbp->flush_interval, timebuf);

    while (1) {
        if ((cret = comm_rcv(&rqst)) < 0)
            return -1;

        now = time(NULL);

        if (cret == 0) {
            /* comm_rcv returned from select without receiving anything. */

            /* Give signals a chance... */
            block_sigs_onoff(0);
            block_sigs_onoff(1);
            if (exit_sig)
                /* Received signal (TERM|INT) */
                return 0;
            if (dbp->idle_timeout && comm_nbe() <= 0 && (now - time_last_rqst) > dbp->idle_timeout)
                /* Idle timeout */
                return 0;
        } else {
            /* We got a request */
            time_last_rqst = now;

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
                ret = dbd_add(dbd, &rqst, &rply);
                break;
            case CNID_DBD_OP_GET:
                ret = dbd_get(dbd, &rqst, &rply);
                break;
            case CNID_DBD_OP_RESOLVE:
                ret = dbd_resolve(dbd, &rqst, &rply);
                break;
            case CNID_DBD_OP_LOOKUP:
                ret = dbd_lookup(dbd, &rqst, &rply);
                break;
            case CNID_DBD_OP_UPDATE:
                ret = dbd_update(dbd, &rqst, &rply);
                break;
            case CNID_DBD_OP_DELETE:
                ret = dbd_delete(dbd, &rqst, &rply);
                break;
            case CNID_DBD_OP_GETSTAMP:
                ret = dbd_getstamp(dbd, &rqst, &rply);
                break;
            case CNID_DBD_OP_REBUILD_ADD:
                ret = dbd_rebuild_add(dbd, &rqst, &rply);
                break;
            default:
                LOG(log_error, logtype_cnid, "loop: unknown op %d", rqst.op);
                ret = -1;
                break;
            }
            
            if ((cret = comm_snd(&rply)) < 0 || ret < 0) {
                dbif_txn_abort(dbd);
                return -1;
            }
            
            if (ret == 0 || cret == 0) {
                if (dbif_txn_abort(dbd) < 0)
                    return -1;
            } else {
                ret = dbif_txn_commit(dbd);
                if (  ret < 0)
                    return -1;
                else if ( ret > 0 )
                    /* We had a designated txn because we wrote to the db */
                    count++;
            }
        } /* got a request */

        /*
          Shall we checkpoint bdb ?
          "flush_interval" seconds passed ?
        */
        if (now > time_next_flush) {
            LOG(log_info, logtype_cnid, "Checkpointing BerkeleyDB for volume '%s'", dbp->dir);
            if (dbif_txn_checkpoint(dbd, 0, 0, 0) < 0)
                return -1;
            count = 0;
            time_next_flush = now + dbp->flush_interval;

            strftime(timebuf, 63, "%b %d %H:%M:%S.",localtime(&time_next_flush));
            LOG(log_debug, logtype_cnid, "Checkpoint interval: %d seconds. Next checkpoint: %s",
                dbp->flush_interval, timebuf);
        }

        /* 
           Shall we checkpoint bdb ?
           Have we commited "count" more changes than "flush_frequency" ?
        */
        if (count > dbp->flush_frequency) {
            LOG(log_info, logtype_cnid, "Checkpointing BerkeleyDB after %d writes for volume '%s'", count, dbp->dir);
            if (dbif_txn_checkpoint(dbd, 0, 0, 0) < 0)
                return -1;
            count = 0;
        }
    } /* while(1) */
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
    int lockfd, ctrlfd, clntfd;
    char *dir, *logconfig;

    set_processname("cnid_dbd");

    /* FIXME: implement -d from cnid_metad */
    if (argc  != 5) {
        LOG(log_error, logtype_cnid, "main: not enough arguments");
        exit(1);
    }

    dir = argv[1];
    ctrlfd = atoi(argv[2]);
    clntfd = atoi(argv[3]);
    logconfig = strdup(argv[4]);
    setuplog(logconfig);

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

    if ((dbp = db_param_read(dir, CNID_DBD)) == NULL)
        exit(1);
    LOG(log_maxdebug, logtype_cnid, "Finished parsing db_param config file");

    if (NULL == (dbd = dbif_init(".", "cnid2.db")))
        exit(2);

    if (dbif_env_open(dbd, dbp, DBOPTIONS) < 0)
        exit(2); /* FIXME: same exit code as failure for dbif_open() */
    LOG(log_debug, logtype_cnid, "Finished initializing BerkeleyDB environment");

    if (dbif_open(dbd, dbp, 0) < 0) {
        dbif_close(dbd);
        exit(2);
    }
    LOG(log_debug, logtype_cnid, "Finished opening BerkeleyDB databases");

    if (dbd_stamp(dbd) < 0) {
        dbif_close(dbd);
        exit(5);
    }
    LOG(log_maxdebug, logtype_cnid, "Finished checking database stamp");

    if (comm_init(dbp, ctrlfd, clntfd) < 0) {
        dbif_close(dbd);
        exit(3);
    }

    if (loop(dbp) < 0)
        err++;

    if (dbif_close(dbd) < 0)
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
