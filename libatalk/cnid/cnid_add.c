/*
 * $Id: cnid_add.c,v 1.11 2001-10-18 02:28:56 jmarcus Exp $
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

#ifdef CNID_DB
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
static int add_cnid(CNID_private *db, DB_TXN *ptid, DBT *key, DBT *data)
{
  DBT altkey, altdata;
  DB_TXN *tid;
  /* We create rc here because using errno is bad.  Why?  Well, if you 
   * use errno once, then call another function which resets it, you're
   * screwed. */
  int rc = 0;

  memset(&altkey, 0, sizeof(altkey));
  memset(&altdata, 0, sizeof(altdata));
  
retry:
  if ((rc = txn_begin(db->dbenv, ptid, &tid,0))) {
    return rc;
  }

  /* main database */
  if ((rc = db->db_cnid->put(db->db_cnid, tid,
			        key, data, DB_NOOVERWRITE))) {
    txn_abort(tid);
    if (rc == DB_LOCK_DEADLOCK)
      goto retry;

    return rc;
  }

  /* did/name database */
  altkey.data = (char *) data->data + CNID_DEVINO_LEN;
  altkey.size = data->size - CNID_DEVINO_LEN;
  if ((rc = db->db_didname->put(db->db_didname, tid,
				   &altkey, &altdata, 0))) {
    txn_abort(tid);
    if (rc == DB_LOCK_DEADLOCK)
      goto retry;

    return rc;
  }

  /* dev/ino database */
  altkey.data = data->data;
  altkey.size = CNID_DEVINO_LEN;
  altdata.data = key->data;
  altdata.size = key->size;
  if ((rc = db->db_devino->put(db->db_devino, tid,
				  &altkey, &altdata, 0))) {
    txn_abort(tid);
    if (rc == DB_LOCK_DEADLOCK)
      goto retry;

    return rc;
  }


  return txn_commit(tid, 0);
}

/* 0 is not a valid cnid. this will do a cnid_lookup beforehand and
   return that cnid if it exists.  */
cnid_t cnid_add(void *CNID, const struct stat *st,
		const cnid_t did, const char *name, const int len,
		cnid_t hint)
{
  CNID_private *db;
  DBT key, data;
  DBT rootinfo_key, rootinfo_data;
  DB_TXN *tid;
  cnid_t id, save;
  int rc = 0;

  int debug = 0;


  if (!(db = CNID) || !st || !name) {
    return 0;
  }

  /* do a lookup... */
  id = cnid_lookup(db, st, did, name, len);
  /* ...return id if it is valid or if RootInfo is read-only. */
  if (id || (db->flags & CNIDFLAG_DB_RO)) {
    if (debug)
      syslog(LOG_ERR, "cnid_add: looked up did %u, name %s as %u", ntohl(did), name, ntohl(id));
    return id;
  }

  /* initialize everything */
  memset(&key, 0, sizeof(key));
  memset(&data, 0, sizeof(data));

  /* just set hint, and the key will change. */
  key.data = &hint;
  key.size = sizeof(hint);

  if ((data.data =
       make_cnid_data(st, did, name, len)) == NULL) {
    syslog(LOG_ERR, "cnid_add: path name too long.");
    goto cleanup_err;
  }
  data.size = CNID_HEADER_LEN + len + 1;

  /* start off with the hint. it should be in network byte order.
   * we need to make sure that somebody doesn't add in restricted
   * cnid's to the database. */
  if (ntohl(hint) >= CNID_START) {
    /* if the key doesn't exist, add it in. don't fiddle with nextID. */
    rc = add_cnid(db, NULL, &key, &data);
    switch (rc) {
    case DB_KEYEXIST: /* need to use RootInfo after all. */
      break;
    default:
      syslog(LOG_ERR, "cnid_add: unable to add CNID %u (%d)", ntohl(hint), rc);
      goto cleanup_err;
    case 0:
      if (debug)
        syslog(LOG_ERR, "cnid_add: used hint for did %u, name %s as %u", ntohl(did), name, ntohl(hint));
      return hint;
    }
  }


  memset(&rootinfo_key, 0, sizeof(rootinfo_key));
  memset(&rootinfo_data, 0, sizeof(rootinfo_data));

  /* just set hint, and the key will change. */
  rootinfo_key.data = ROOTINFO_KEY;
  rootinfo_key.size = ROOTINFO_KEYLEN;

  /* Get the key. */
  switch (rc = db->db_didname->get(db->db_didname, NULL, &rootinfo_key, &rootinfo_data, 0)) {
  case DB_LOCK_DEADLOCK:
          goto retry;
  case 0:
          memcpy (&hint, rootinfo_data.data, sizeof(hint));
          if (debug)
            syslog(LOG_ERR, "cnid_add: found rootinfo for did %u, name %s as %u", ntohl(did), name, ntohl(hint));
          break;
  case DB_NOTFOUND:
          hint = htonl(CNID_START);
          if (debug)
            syslog(LOG_ERR, "cnid_add: using CNID_START for did %u, name %s as %u", ntohl(did), name, ntohl(hint));
          break;
  default:
          syslog(LOG_ERR, "cnid_add: unable to lookup rootinfo (%d)", rc);
		  goto cleanup_err;
 }

  /* Abort and retry the modification. */
  if (0) {
retry:    if ((rc = txn_abort(tid)) != 0)
              syslog(LOG_ERR, "cnid_add: txn_abort failed (%d)", rc);
          /* FALLTHROUGH */
  }

  /* Begin the transaction. */
  if ((rc = txn_begin(db->dbenv, NULL, &tid, 0)) != 0) {
    syslog(LOG_ERR, "cnid_add: txn_begin failed (%d)", rc);
    goto cleanup_err;
  }


  /* search for a new id. we keep the first id around to check for
   * wrap-around. NOTE: i do it this way so that we can go back and
   * fill in holes. */
  save = id = ntohl(hint);
  while ((rc = add_cnid(db, tid, &key, &data))) {
    /* don't use any of the special CNIDs */
    if (++id < CNID_START)
      id = CNID_START;

    if ((rc != DB_KEYEXIST) || (save == id)) {
      syslog(LOG_ERR, "cnid_add: unable to add CNID %u (%d)", ntohl(hint), rc);
      hint = 0;
      goto cleanup_abort;
    }
    hint = htonl(id);
  }

  /* update RootInfo with the next id. */
  rootinfo_data.data = &hint;
  rootinfo_data.size = sizeof(hint);

  switch (rc = db->db_didname->put(db->db_didname, tid, &rootinfo_key, &rootinfo_data, 0)) {
  case DB_LOCK_DEADLOCK:
          goto retry;
  case 0:
          break;
  default:
          syslog(LOG_ERR, "cnid_add: unable to update rootinfo (%d)", rc);
          goto cleanup_abort;
  }


cleanup_commit:
  /* The transaction finished, commit it. */
  if ((rc = txn_commit(tid, 0)) != 0) {
    syslog(LOG_ERR, "cnid_add: txn_commit failed (%d)", rc);
    goto cleanup_err;
  }

  if (debug)
    syslog(LOG_ERR, "cnid_add: returned cnid for did %u, name %s as %u", ntohl(did), name, ntohl(hint));
  return hint;

cleanup_abort:
  txn_abort(tid);

cleanup_err:
  return 0;
}
#endif /* CNID_DB */
