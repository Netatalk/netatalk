/* 
 * $Id: cnid_add.c,v 1.2 2001-06-29 14:14:46 rufustfirefly Exp $
 *
 * Copyright (c) 1999. Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 * cnid_add (db, dev, ino, did, name, hint): 
 * add a name to the CNID database. we use both dev/ino and did/name
 * to keep track of things.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <errno.h>
#include <syslog.h>

#include <db.h>
#include <netatalk/endian.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/util.h>

#include "cnid_private.h"

/* add an entry to the CNID databases. we do this as a transaction
 * to prevent messiness. */
static int add_cnid(CNID_private *db, DBT *key, DBT *data)
{
  DBT altkey, altdata;
  DB_TXN *tid;
  DB_TXNMGR *txnp;

  txnp = db->dbenv.tx_info;
  memset(&altkey, 0, sizeof(altkey));
  memset(&altdata, 0, sizeof(altdata));
  
retry:
  if (errno = txn_begin(txnp, NULL, &tid)) {
    return errno;
  }

  /* main database */
  if (errno = db->db_cnid->put(db->db_cnid, tid, 
			       key, data, DB_NOOVERWRITE)) {
    txn_abort(tid);
    if (errno == EAGAIN)
      goto retry;

    return errno;
  }

  /* dev/ino database */
  altkey.data = data->data;
  altkey.size = CNID_DEVINO_LEN;
  altdata.data = key->data;
  altdata.size = key->size;
  if ((errno = db->db_devino->put(db->db_devino, tid,
				  &altkey, &altdata, 0))) {
    txn_abort(tid);
    if (errno == EAGAIN)
      goto retry;

    return errno;
  }

  /* did/name database */
  altkey.data = data->data + CNID_DEVINO_LEN;
  altkey.size = data->size - CNID_DEVINO_LEN;
  if (errno = db->db_didname->put(db->db_didname, tid,
				    &altkey, &altdata, 0)) {
    txn_abort(tid);
    if (errno == EAGAIN)
      goto retry;

    return errno;
  }

  return txn_commit(tid);
}
		    
/* 0 is not a valid cnid. this will do a cnid_lookup beforehand and
   return that cnid if it exists.  */
cnid_t cnid_add(void *CNID, const struct stat *st, 
		const cnid_t did, const char *name, const int len,
		cnid_t hint)
{
  CNID_private *db;
  DBT key, data;
  struct flock lock;
  cnid_t id, save;
  
  
  if (!(db = CNID) || !st || !name)
    return 0;
  
  /* just do a lookup if RootInfo is read-only. */
  if (db->flags & (CNIDFLAG_ROOTINFO_RO | CNIDFLAG_DB_RO))
    return cnid_lookup(db, st, did, name, len);

  /* initialize everything */
  memset(&key, 0, sizeof(key));
  memset(&data, 0, sizeof(data));

  /* acquire a lock on RootInfo. as the cnid database is the only user 
   * of RootInfo, we just use our own locks. 
   *
   * NOTE: we lock it here to serialize access to the database. */
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;
  lock.l_start = ad_getentryoff(&db->rootinfo, ADEID_DID);
  lock.l_len = ad_getentrylen(&db->rootinfo, ADEID_DID);
  if (fcntl(ad_hfileno(&db->rootinfo), F_SETLKW,  &lock) < 0) {
    syslog(LOG_ERR, "cnid_add: can't establish lock: %m");
    goto cleanup_err;
  }

  /* if it's already stored, just return it */
  if ((id = cnid_lookup(db, st, did, name, len))) {
    hint = id;
    goto cleanup_unlock;
  }
  
  /* just set hint, and the key will change. */
  key.data = &hint;
  key.size = sizeof(hint);
  
  if ((data.data = 
       make_cnid_data(st, did, name, len)) == NULL) {
    syslog(LOG_ERR, "cnid_add: path name too long.");
    goto cleanup_unlock;
  }
  data.size = CNID_HEADER_LEN + len + 1;
  
  /* start off with the hint. it should be in network byte order. 
   * we need to make sure that somebody doesn't add in restricted
   * cnid's to the database. */
  if (ntohl(hint) >= CNID_START) {
    /* if the key doesn't exist, add it in. don't fiddle with nextID. */
    errno = add_cnid(db, &key, &data);
    switch (errno) {
    case DB_KEYEXIST: /* need to use RootInfo after all. */
      break;
    default:
      syslog(LOG_ERR, "cnid_add: unable to add CNID(%x)", hint); 
      hint = 0;
      /* fall through */
    case 0:
      goto cleanup_unlock;
    }
  }    
    
  /* no need to refresh the header file */
  memcpy(&hint, ad_entry(&db->rootinfo, ADEID_DID), sizeof(hint));

  /* search for a new id. we keep the first id around to check for
   * wrap-around. NOTE: i do it this way so that we can go back and
   * fill in holes. */
  save = id = ntohl(hint);
  while (errno = add_cnid(db, &key, &data)) {
    /* don't use any of the special CNIDs */
    if (++id < CNID_START)
      id = CNID_START;

    if ((errno != DB_KEYEXIST) || (save == id)) {
      syslog(LOG_ERR, "cnid_add: unable to add CNID(%x)", hint);
      hint = 0;
      goto cleanup_unlock;
    }
    hint = htonl(id);
  }

  /* update RootInfo with the next id. */
  id = htonl(++id);
  memcpy(ad_entry(&db->rootinfo, ADEID_DID), &id, sizeof(id));
  ad_flush(&db->rootinfo, ADFLAGS_HF);

cleanup_unlock:
  lock.l_type = F_UNLCK;
  fcntl(ad_hfileno(&db->rootinfo), F_SETLK, &lock);
  return hint;

cleanup_err:
  return 0;
}
