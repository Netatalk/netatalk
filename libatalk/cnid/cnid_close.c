/*
 * $Id: cnid_close.c,v 1.11 2001-10-15 02:06:13 jmarcus Exp $
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
#include <syslog.h>
#include <db.h>
#include <errno.h>
#include <string.h>

#include <atalk/cnid.h>

#include "cnid_private.h"

void cnid_close(void *CNID)
{
  CNID_private *db;
  int rc = 0;

  if (!(db = CNID))
    return;

  /* flush the transaction log and delete the log file if we can. */
  if ((db->lockfd > -1) && ((db->flags & CNIDFLAG_DB_RO) == 0)) {
    struct flock lock;
    char wd[MAXPATHLEN+1];

	getcwd(wd, MAXPATHLEN);

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = lock.l_len = 0;
    if (fcntl(db->lockfd, F_SETLK, &lock) == 0) {
      char **list, **first;

      rc = txn_checkpoint(db->dbenv, 0, 0, 0);
      while (rc == DB_INCOMPLETE)
		rc = txn_checkpoint(db->dbenv, 0, 0, 0);

      /* we've checkpointed, so clean up the log files.
       * NOTE: any real problems will make log_archive return an error. */
      chdir(db->dbenv->db_log_dir ? db->dbenv->db_log_dir : db->dbenv->db_home);
#if DB_VERSION_MINOR > 2
      if (!log_archive(db->dbenv, &first, DB_ARCH_LOG)) {
#else /* DB_VERSION_MINOR < 2 */
      if (!log_archive(db->dbenv, &first, DB_ARCH_LOG, NULL)) {
#endif /* DB_VERSION_MINOR */
	list = first;
	while (*list) {
	  if ((rc = remove(*list)) != 0)
	    syslog(LOG_INFO, "cnid_close: failed to remove %s: %s",
		   *list, strerror(rc));
	  list++;
	}
	free(first); 
      }
    }
	/* Change directory back to the old cwd so we don't get the problem where 
	 * the contents of .AppleDB show up in the current share window. */
	chdir( wd );
  }

  db->db_didname->close(db->db_didname, 0);
  db->db_devino->close(db->db_devino, 0);
  db->db_cnid->close(db->db_cnid, 0);
  db->dbenv->close(db->dbenv, 0);
  
  if (db->lockfd > -1)
    close(db->lockfd); /* this will also close any lock we have. */

  free(db);
}
#endif /* CNID_DB */
