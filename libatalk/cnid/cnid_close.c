#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <db.h>
#include <errno.h>

#include <atalk/cnid.h>

#include "cnid_private.h"

void cnid_close(void *CNID)
{
  CNID_private *db;

  if (!(db = CNID))
    return;

  /* flush the transaction log and delete the log file if we can. */
  if ((db->lockfd > -1) && ((db->flags & CNIDFLAG_DB_RO) == 0)) {
    struct flock lock;

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = lock.l_len = 0;
    if (fcntl(db->lockfd, F_SETLK, &lock) == 0) {
      DB_TXNMGR *txnp;
      char **list, **first;
      
      txnp = db->dbenv.tx_info;
      errno = txn_checkpoint(txnp, 0, 0);
      while (errno == DB_INCOMPLETE)
	errno = txn_checkpoint(txnp, 0, 0);
      
      /* we've checkpointed, so clean up the log files. 
       * NOTE: any real problems will make log_archive return an error. */
      chdir(db->dbenv.db_log_dir ? db->dbenv.db_log_dir : db->dbenv.db_home);
      if (!log_archive(db->dbenv.lg_info, &first, DB_ARCH_LOG, NULL)) {
	list = first;
	while (*list) {
	  if (truncate(*list, 0) < 0)
	    syslog(LOG_INFO, "cnid_close: failed to truncate %s: %m", *list);
	  list++;
	}
	free(first); 
      }
    }
  }

  db->db_didname->close(db->db_didname, 0);
  db->db_devino->close(db->db_devino, 0);
  db->db_cnid->close(db->db_cnid, 0);
  db_appexit(&db->dbenv);
  if (db->lockfd > -1)
    close(db->lockfd); /* this will also close any lock we have. */
  ad_close(&db->rootinfo, ADFLAGS_HF);
  free(db);
}
