/*
 * $Id: cnid_update.c,v 1.2 2001-06-29 14:14:46 rufustfirefly Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

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
int cnid_update(void *CNID, cnid_t id, const struct stat *st, 
		const cnid_t did, const char *name, const int len,
		const char *info, const int infolen)
{
  CNID_private *db;
  DBT key, data, altdata;
  DB_TXN *tid;
  DB_TXNMGR *txnp;
  
  if (!(db = CNID) || !id || !st || !name || (db->flags & CNIDFLAG_DB_RO))
    return -1;

  memset(&key, 0, sizeof(key));
  memset(&data, 0, sizeof(data));
  memset(&altdata, 0, sizeof(altdata));
  txnp = db->dbenv.tx_info;

  /* begin a transaction */
retry:
  if (errno = txn_begin(txnp, NULL, &tid)) {
    return errno;
  }

  /* get the old info */
  key.data = &id;
  key.size = sizeof(id);
  if (errno = db->db_cnid->get(db->db_cnid, tid, &key, &data, 0)) {
    txn_abort(tid);
    if (errno == EAGAIN)
      goto retry;
    goto update_err;
  }

  /* delete the old dev/ino mapping */
  key.data = data.data;
  key.size = CNID_DEVINO_LEN;
  if (errno = db->db_devino->del(db->db_devino, tid, &key, 0)) {
    if (errno == EAGAIN) {
      txn_abort(tid);
      goto retry;
    }
      
    /* silently fail on a non-existent entry */
    if (errno != DB_NOTFOUND) {
      txn_abort(tid);
      goto update_err;
    }
  }

  /* delete the old did/name mapping */
  key.data = data.data + CNID_DEVINO_LEN;
  key.size = data.size - CNID_DEVINO_LEN;
  if (errno = db->db_didname->del(db->db_didname, tid, &key, 0)) {
    if (errno == EAGAIN) {
      txn_abort(tid);
      goto retry;
    }

    /* silently fail on a non-existent entry */
    if (errno != DB_NOTFOUND) {
      txn_abort(tid);
      goto update_err;
    }
  }
  
  /* delete the old aliases if necessary */


  /* make a new entry */
  data.data = make_cnid_data(st, did, name, len);
  data.size = CNID_HEADER_LEN + len + 1;
  
  /* put a new dev/ino mapping in */
  key.data = data.data;
  key.size = CNID_DEVINO_LEN;
  altdata.data = &id;
  altdata.size = sizeof(id);
  if (errno = db->db_devino->put(db->db_devino, tid, &key, &altdata, 0)) {
    txn_abort(tid);
    if (errno == EAGAIN) {
      goto retry;
    }
    goto update_err;
  }
  
  /* put a new did/name mapping in */
  key.data = data.data + CNID_DEVINO_LEN;
  key.size = data.size - CNID_DEVINO_LEN;
  if (errno = db->db_didname->put(db->db_didname, tid, &key, &altdata, 0)) {
    txn_abort(tid);
    if (errno == EAGAIN) {
      goto retry;
    }
    goto update_err;
  }
  
  /* update the old CNID with the new info */
  key.data = &id;
  key.size = sizeof(id);
  if (errno = db->db_cnid->put(db->db_cnid, tid, &key, &data, 0)) {
    txn_abort(tid);
    if (errno == EAGAIN) {
      goto retry;
    }
    goto update_err;
  }
  
  /* end transaction */
  return txn_commit(tid);

update_err:
  syslog(LOG_ERR, "cnid_update: can't update CNID(%x)", id);
  return -1;
}
