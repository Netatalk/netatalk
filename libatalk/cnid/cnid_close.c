/*
 * $Id: cnid_close.c,v 1.20 2002-01-19 21:42:08 jmarcus Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_DB
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <stdlib.h>
#include <atalk/logger.h>
#include <db.h>
#include <errno.h>
#include <string.h>

#include <atalk/cnid.h>

#include "cnid_private.h"

void cnid_close(void *CNID) {
    CNID_private *db;
    int rc;

    if (!(db = CNID)) {
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
            int cfd = -1;

            if ((cfd = open(db->close_file, O_RDWR | O_CREAT, 0666)) > -1) {

                /* Checkpoint the databases until we can checkpoint no
                 * more. */
                rc = txn_checkpoint(db->dbenv, 0, 0, 0);
                while (rc == DB_INCOMPLETE) {
                    rc = txn_checkpoint(db->dbenv, 0, 0, 0);
                }

#if DB_VERSION_MINOR > 2
                if ((rc = log_archive(db->dbenv, &list, DB_ARCH_LOG | DB_ARCH_ABS)) != 0) {
#else /* DB_VERSION_MINOR < 2 */
                if ((rc = log_archive(db->dbenv, &list, DB_ARCH_LOG | DB_ARCH_ABS, NULL)) != 0) {
#endif /* DB_VERSION_MINOR */
                    LOG(log_error, logtype_default, "cnid_close: Unable to archive logfiles: %s",
                        db_strerror(rc));
                }

                if (list != NULL) {
                    for (first = list; *list != NULL; ++list) {
                        if ((rc = remove(*list)) != 0) {
#ifdef DEBUG
                            LOG(log_info, logtype_default, "cnid_close: failed to remove %s: %s",
                                *list, strerror(rc));
#endif
                        }
                    }
                    free(first);
                }
                (void)remove(db->close_file);
                close(cfd);
            }
            else {
                LOG(log_error, logtype_default, "cnid_close: Failed to open database closing lock file: %s", strerror(errno));
            }
        }
    }

    db->db_didname->close(db->db_didname, 0);
    db->db_devino->close(db->db_devino, 0);
    db->db_cnid->close(db->db_cnid, 0);
    db->dbenv->close(db->dbenv, 0);

    if (db->lockfd > -1) {
        close(db->lockfd); /* This will also release any locks we have. */
    }

    free(db);
}
#endif /* CNID_DB */
