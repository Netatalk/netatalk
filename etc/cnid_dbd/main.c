/*
 * Copyright (C) Joerg Lenneis 2003
 * Copyright (c) Frank Lahm 2009
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/file.h>
#include <arpa/inet.h>

#include <atalk/cnid_dbd_private.h>
#include <atalk/logger.h>
#include <atalk/errchk.h>
#include <atalk/bstrlib.h>
#include <atalk/netatalk_conf.h>

#include "db_param.h"
#include "dbif.h"
#include "dbd.h"
#include "comm.h"
#include "pack.h"

/* 
   Note: DB_INIT_LOCK is here so we can run the db_* utilities while netatalk is running.
   It's a likey performance hit, but it might we worth it.
 */
#define DBOPTIONS (DB_CREATE | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_LOCK | DB_INIT_TXN)

static DBD *dbd;
static int exit_sig = 0;
static int db_locked;

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
  -1: Fatal error, either from t
  he database or from the socket. Abort the transaction if applicable
  (which might fail as well) and then exit.

  We always try to notify the client process about the outcome, the result field
  of the cnid_dbd_rply structure contains further details.

*/
#ifndef min
#define min(a,b)        ((a)<(b)?(a):(b))
#endif

static int loop(struct db_param *dbp)
{
    struct cnid_dbd_rqst rqst;
    struct cnid_dbd_rply rply;
    time_t timeout;
    int ret, cret;
    int count;
    time_t now, time_next_flush, time_last_rqst;
    char timebuf[64];
    static char namebuf[MAXPATHLEN + 1];
    sigset_t set;

    sigemptyset(&set);
    sigprocmask(SIG_SETMASK, NULL, &set);
    sigdelset(&set, SIGINT);
    sigdelset(&set, SIGTERM);

    count = 0;
    now = time(NULL);
    time_next_flush = now + dbp->flush_interval;
    time_last_rqst = now;

    rqst.name = namebuf;

    strftime(timebuf, 63, "%b %d %H:%M:%S.",localtime(&time_next_flush));
    LOG(log_debug, logtype_cnid, "Checkpoint interval: %d seconds. Next checkpoint: %s",
        dbp->flush_interval, timebuf);

    while (1) {
        timeout = min(time_next_flush, time_last_rqst +dbp->idle_timeout);
        if (timeout > now)
            timeout -= now;
        else
            timeout = 1;

        if ((cret = comm_rcv(&rqst, timeout, &set, &now)) < 0)
            return -1;

        if (cret == 0) {
            /* comm_rcv returned from select without receiving anything. */
            if (exit_sig) {
                /* Received signal (TERM|INT) */
                return 0;
            }
            if (now - time_last_rqst >= dbp->idle_timeout && comm_nbe() <= 0) {
                /* Idle timeout */
                return 0;
            }
            /* still active connections, reset time_last_rqst */
            time_last_rqst = now;
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
                ret = dbd_delete(dbd, &rqst, &rply, DBIF_CNID);
                break;
            case CNID_DBD_OP_GETSTAMP:
                ret = dbd_getstamp(dbd, &rqst, &rply);
                break;
            case CNID_DBD_OP_REBUILD_ADD:
                ret = dbd_rebuild_add(dbd, &rqst, &rply);
                break;
            case CNID_DBD_OP_SEARCH:
                ret = dbd_search(dbd, &rqst, &rply);
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
        if (now >= time_next_flush) {
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
        LOG(log_debug, logtype_cnid, "Setting uid/gid to %i/%i", st.st_uid, st.st_gid);
        if (setgid(st.st_gid) < 0 || setuid(st.st_uid) < 0) {
            LOG(log_error, logtype_cnid, "uid/gid: %s", strerror(errno));
            exit(1);
        }
    }
}


/* ----------------------- */
static void set_signal(void)
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

/* ------------------------ */
int main(int argc, char *argv[])
{
    EC_INIT;
    struct db_param *dbp;
    int delete_bdb = 0;
    int ctrlfd = -1, clntfd = -1;
    char *logconfig;
    AFPObj obj = { 0 };
    struct vol *vol;
    char *volpath = NULL;
    bstring dbpath;

    while (( ret = getopt( argc, argv, "dF:l:p:t:vV")) != -1 ) {
        switch (ret) {
        case 'd':
            delete_bdb = 1;
            break;
        case 'F':
            obj.cmdlineconfigfile = strdup(optarg);
            break;
        case 'p':
            volpath = strdup(optarg);
            break;
        case 'l':
            clntfd = atoi(optarg);
            break;
        case 't':
            ctrlfd = atoi(optarg);
            break;
        case 'v':
        case 'V':
            printf("cnid_dbd (Netatalk %s)\n", VERSION);
            return -1;
        }
    }

    if (ctrlfd == -1 || clntfd == -1 || !volpath) {
        LOG(log_error, logtype_cnid, "main: bad IPC fds");
        exit(EXIT_FAILURE);
    }

    EC_ZERO( afp_config_parse(&obj, "cnid_dbd") );

    EC_ZERO( load_volumes(&obj) );
    EC_NULL( vol = getvolbypath(&obj, volpath) );
    EC_ZERO( load_charset(vol) );
    pack_setvol(vol);

    EC_NULL( dbpath = bfromcstr(vol->v_dbpath) );
    EC_ZERO( bcatcstr(dbpath, "/.AppleDB") );

    LOG(log_debug, logtype_cnid, "db dir: \"%s\"", bdata(dbpath));

    switch_to_user(bdata(dbpath));

    /* Get db lock */
    if ((db_locked = get_lock(LOCK_EXCL, bdata(dbpath))) == -1) {
        LOG(log_error, logtype_cnid, "main: fatal db lock error");
        EC_FAIL;
    }
    if (db_locked != LOCK_EXCL) {
        /* Couldn't get exclusive lock, try shared lock  */
        if ((db_locked = get_lock(LOCK_SHRD, NULL)) != LOCK_SHRD) {
            LOG(log_error, logtype_cnid, "main: fatal db lock error");
            EC_FAIL;
        }
    }

    if (delete_bdb && (db_locked == LOCK_EXCL)) {
        LOG(log_warning, logtype_cnid, "main: too many CNID db opening attempts, wiping the slate clean");
        chdir(bdata(dbpath));
        system("rm -f cnid2.db lock log.* __db.*");
        if ((db_locked = get_lock(LOCK_EXCL, bdata(dbpath))) != LOCK_EXCL) {
            LOG(log_error, logtype_cnid, "main: fatal db lock error");
            EC_FAIL;
        }
    }

    set_signal();

    /* SIGINT and SIGTERM are always off, unless we are in pselect */
    block_sigs_onoff(1);

    if ((dbp = db_param_read(bdata(dbpath))) == NULL)
        EC_FAIL;
    LOG(log_maxdebug, logtype_cnid, "Finished parsing db_param config file");

    if (NULL == (dbd = dbif_init(bdata(dbpath), "cnid2.db")))
        EC_FAIL;

    /* Only recover if we got the lock */
    if (dbif_env_open(dbd,
                      dbp,
                      (db_locked == LOCK_EXCL) ? DBOPTIONS | DB_RECOVER : DBOPTIONS) < 0)
        EC_FAIL;
    LOG(log_debug, logtype_cnid, "Finished initializing BerkeleyDB environment");

    if (dbif_open(dbd, dbp, 0) < 0) {
        ret = -1;
        goto close_db;
    }

    LOG(log_debug, logtype_cnid, "Finished opening BerkeleyDB databases");

    /* Downgrade db lock  */
    if (db_locked == LOCK_EXCL) {
        if (get_lock(LOCK_UNLOCK, NULL) != 0) {
            ret = -1;
            goto close_db;
        }

        if (get_lock(LOCK_SHRD, NULL) != LOCK_SHRD) {
            ret = -1;
            goto close_db;
        }
    }


    if (comm_init(dbp, ctrlfd, clntfd) < 0) {
        ret = -1;
        goto close_db;
    }

    if (loop(dbp) < 0) {
        ret = -1;
        goto close_db;
    }

close_db:
    if (dbif_close(dbd) < 0)
        ret = -1;

    if (dbif_env_remove(bdata(dbpath)) < 0)
        ret = -1;

EC_CLEANUP:
    if (ret != 0)
        exit(1);

    if (exit_sig)
        LOG(log_info, logtype_cnid, "main: Exiting on signal %i", exit_sig);
    else
        LOG(log_info, logtype_cnid, "main: Idle timeout, exiting");

    EC_EXIT;
}
