/* 
 * $Id: cnid_delete.c,v 1.2 2001-06-29 14:14:46 rufustfirefly Exp $
 *
 * Copyright (c) 1999. Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 * cnid_delete: delete a CNID from the database 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#include <db.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/cnid.h>

#include "cnid_private.h"

int cnid_delete(void *CNID, const cnid_t id)
{
  CNID_private *db;
  DBT key, data;
  DB_TXN *tid;
  DB_TXNMGR *txnp;

  if (!(db = CNID) || !id || (db->flags & CNIDFLAG_DB_RO))
    return -1;

  txnp = db->dbenv.tx_info;
  memset(&key, 0, sizeof(key));
  memset(&data, 0, sizeof(data));
  

retry:
  if (errno = txn_begin(txnp, NULL, &tid)) {
    return errno;
  }

  /* get from main database */
  key.data = &id;
  key.size = sizeof(id);
  if (errno = db->db_cnid->get(db->db_cnid, tid, &key, &data, 0)) {
    txn_abort(tid);
    switch (errno) {
    case EAGAIN:
      goto retry;
      
    case DB_NOTFOUND:
      syslog(LOG_INFO, "cnid_delete: CNID(%x) not in database", id);
      return 0;
    default:
      syslog(LOG_ERR, "cnid_delete: can't delete entry");
      return errno;
    }
  }
  
  /* now delete from dev/ino database */
  key.data = data.data;
  key.size = CNID_DEVINO_LEN;
  if (errno = db->db_devino->del(db->db_devino, tid, &key, 0)) {
    if (errno == EAGAIN) {
      txn_abort(tid);
      goto retry;
    }

    /* be silent if there isn't an entry */
    if (errno != DB_NOTFOUND) {
      txn_abort(tid);
      goto abort_err;
    }
  }

  /* get data from the did/name database */
  /* free from did/macname, did/shortname, and did/longname databases */

  /* delete from did/name database */
  key.data = data.data + CNID_DEVINO_LEN;
  key.size = data.size - CNID_DEVINO_LEN;
  if (errno = db->db_didname->del(db->db_didname, tid, &key, 0)) {
    if (errno == EAGAIN) {
      txn_abort(tid);
      goto retry;
    }
    
    /* be silent if there isn't an entry */
    if (errno != DB_NOTFOUND) {
      txn_abort(tid);
      goto abort_err;
    }
  }

  /* now delete from main database */
  key.data = &id;
  key.size = sizeof(id);
  if (errno = db->db_cnid->del(db->db_cnid, tid, &key, 0)) {
    txn_abort(tid);
    if (errno == EAGAIN) {
      goto retry;
    }
    goto abort_err;
  }

  return txn_commit(tid);

abort_err:
  syslog(LOG_ERR, "cnid_del: unable to delete CNID(%x)", id);
  return errno;
}
