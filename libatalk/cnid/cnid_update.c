/*
 * $Id: cnid_update.c,v 1.11 2001-10-18 02:30:45 jmarcus Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#ifdef CNID_DB
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <syslog.h>

#include <db.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/cnid.h>

#include "cnid_private.h"


/* cnid_update: takes the given cnid and updates the metadata. to
   handle the did/name data, there are a bunch of functions to get
   and set the various fields. */
int cnid_update(void *CNID, const cnid_t id, const struct stat *st,
		const cnid_t did, const char *name, const int len/*,
		const char *info, const int infolen*/)
{
  CNID_private *db;
  DBT key, data, altdata;
  DB_TXN *tid;
  int rc = 0;
  
  if (!(db = CNID) || !id || !st || !name || (db->flags & CNIDFLAG_DB_RO))
    return -1;

  memset(&key, 0, sizeof(key));
  memset(&data, 0, sizeof(data));
  memset(&altdata, 0, sizeof(altdata));

  /* begin a transaction */
retry:
  if ((rc = txn_begin(db->dbenv, NULL, &tid, 0))) {
	syslog(LOG_ERR, "cnid_update: txn_begin failed with: %d", rc);
    return rc;
  }

  /* get the old info */
  key.data = (cnid_t *) &id;
  key.size = sizeof(id);
  if ((rc = db->db_cnid->get(db->db_cnid, tid, &key, &data, 0))) {
    txn_abort(tid);
    if (rc == DB_LOCK_DEADLOCK) {
      goto retry;
	}
    goto update_err;
  }


  /* delete the old dev/ino mapping */
  memset(&key, 0, sizeof(key));
  key.data = data.data;
  key.size = CNID_DEVINO_LEN;
  if ((rc = db->db_devino->del(db->db_devino, tid, &key, 0))) {
    if (rc == DB_LOCK_DEADLOCK) {
      txn_abort(tid);
      goto retry;
    }

    /* silently fail on a non-existent entry */
    if (rc != DB_NOTFOUND) {
      txn_abort(tid);
      goto update_err;
    }
  }


  /* delete the old did/name mapping */
  memset(&key, 0, sizeof(key));
  key.data = (char *) data.data + CNID_DEVINO_LEN;
  key.size = data.size - CNID_DEVINO_LEN;
  if ((rc = db->db_didname->del(db->db_didname, tid, &key, 0))) {
    if (rc == DB_LOCK_DEADLOCK) {
      txn_abort(tid);
      goto retry;
    }

    /* silently fail on a non-existent entry */
    if (rc != DB_NOTFOUND) {
      txn_abort(tid);
      goto update_err;
    }
  }

  /* delete the old aliases if necessary */


  /* make a new entry */
  memset(&data, 0, sizeof(data));
  data.data = make_cnid_data(st, did, name, len);
  data.size = CNID_HEADER_LEN + len + 1;

  /* put a new dev/ino mapping in */
  memset(&key, 0, sizeof(key));
  key.data = data.data;
  key.size = CNID_DEVINO_LEN;
  altdata.data = (cnid_t *) &id;
  altdata.size = sizeof(id);
  if ((rc = db->db_devino->put(db->db_devino, tid, &key, &altdata, 0))) {
    txn_abort(tid);
    if (rc == DB_LOCK_DEADLOCK) {
      goto retry;
    }
    goto update_err;
  }

  /* put a new did/name mapping in */
  memset(&key, 0, sizeof(key));
  key.data = (char *) data.data + CNID_DEVINO_LEN;
  key.size = data.size - CNID_DEVINO_LEN;
  if ((rc = db->db_didname->put(db->db_didname, tid, &key, &altdata, 0))) {
    txn_abort(tid);
    if (rc == DB_LOCK_DEADLOCK) {
      goto retry;
    }
    goto update_err;
  }

  /* update the old CNID with the new info */
  memset(&key, 0, sizeof(key));
  key.data = (cnid_t *) &id;
  key.size = sizeof(id);
  if ((rc = db->db_cnid->put(db->db_cnid, tid, &key, &data, 0))) {
    txn_abort(tid);
    if (rc == DB_LOCK_DEADLOCK) {
      goto retry;
    }
    goto update_err;
  }

  /* end transaction */
  return txn_commit(tid, 0);

update_err:
  syslog(LOG_ERR, "cnid_update: can't update CNID(%x) (%d)", id, rc);
  return -1;
}
#endif /* CNID_DB */
