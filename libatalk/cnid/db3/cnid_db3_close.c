/*
 * $Id: cnid_db3_close.c,v 1.2 2005-04-28 20:49:59 bfernhomberg Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_BACKEND_DB3

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <stdlib.h>
#include <atalk/logger.h>
#ifdef HAVE_DB4_DB_H
#include <db4/db.h>
#else
#include <db.h>
#endif
#include <errno.h>
#include <string.h>

#include "cnid_db3_private.h"
#include "cnid_db3.h"

void cnid_db3_close(struct _cnid_db *cdb) {
    CNID_private *db;
    int rc;

    if (!cdb) {
	    LOG(log_error, logtype_afpd, "cnid_close called with NULL argument !");
	    return;
    }

    if (!(db = cdb->_private)) {
        return;
    }

    /* Flush the transaction log and delete the log file if we can. */
    if ((db->lockfd > -1) && ((db->flags & CNIDFLAG_DB_RO) == 0)) {
        struct flock lock;

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = lock.l_len = 0;
    if (fcntl(db->lockfd, F_SETLK, &lock) == 0) {
            char **list, **first;


            /* Checkpoint the databases until we can checkpoint no
             * more. */
#if DB_VERSION_MAJOR >= 4
#if DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1
            db->dbenv->txn_checkpoint(db->dbenv, 0, 0, 0);
#else
            rc = db->dbenv->txn_checkpoint(db->dbenv, 0, 0, 0);
#endif /* DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1 */
#else
            rc = txn_checkpoint(db->dbenv, 0, 0, 0);
#endif /* DB_VERSION_MAJOR >= 4 */
#if DB_VERSION_MAJOR < 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 1)
            while (rc == DB_INCOMPLETE) {
#if DB_VERSION_MAJOR >= 4
                rc = db->dbenv->txn_checkpoint(db->dbenv, 0, 0, 0);
#else
                rc = txn_checkpoint(db->dbenv, 0, 0, 0);
#endif /* DB_VERSION_MAJOR >= 4 */
            }
#endif /* DB_VERSION_MAJOR < 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR < 1) */

#if DB_VERSION_MAJOR >= 4
            if ((rc = db->dbenv->log_archive(db->dbenv, &list, DB_ARCH_ABS)) != 0) {
#elif DB_VERSION_MINOR > 2
            if ((rc = log_archive(db->dbenv, &list, DB_ARCH_ABS)) != 0) {
#else /* DB_VERSION_MINOR < 2 */
            if ((rc = log_archive(db->dbenv, &list, DB_ARCH_ABS, NULL)) != 0) {
#endif /* DB_VERSION_MINOR */
                LOG(log_error, logtype_default, "cnid_close: Unable to archive logfiles: %s", db_strerror(rc));
            }

            if (list != NULL) {
                for (first = list; *list != NULL; ++list) {
                    if ((rc = remove(*list)) != 0) {
#ifdef DEBUG
                            LOG(log_info, logtype_default, "cnid_close: failed to remove %s: %s", *list, strerror(rc));
#endif
                        }
                }
                free(first);
            }
        }
        (void)remove(db->lock_file);
    }

    db->db_didname->close(db->db_didname, 0);
    db->db_devino->close(db->db_devino, 0);
    db->db_cnid->close(db->db_cnid, 0);
    db->dbenv->close(db->dbenv, 0);

    if (db->lockfd > -1) {
        close(db->lockfd); /* This will also release any locks we have. */
    }

    free(db);
    free(cdb->volpath);
    free(cdb);
}

#endif /* CNID_BACKEND_DB3 */
