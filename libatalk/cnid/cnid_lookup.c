/*
 * $Id: cnid_lookup.c,v 1.5 2001-08-16 14:30:29 uhees Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <syslog.h>
#include <errno.h>

#include <db.h>
#include <netatalk/endian.h>
#include <atalk/adouble.h>
#include <atalk/cnid.h>

#include "cnid_private.h"

#define LOGFILEMAX    100  /* kbytes */
#define CHECKTIMEMAX   30  /* minutes */

/* this returns the cnid corresponding to a particular file. it will
   also fix up the various databases if there's a problem. */
cnid_t cnid_lookup(void *CNID,
		   const struct stat *st, const cnid_t did, 
		   const char *name, const int len)
{
  char *buf;
  CNID_private *db;
  DBT key, devdata, diddata;
  int devino = 1, didname = 1;
  cnid_t id = 0;

  int debug = 0;

  if (!(db = CNID) || !st || !name)
    return 0;

  /* do a little checkpointing if necessary. i stuck it here as
   * cnid_lookup gets called when we do directory lookups. only do
   * this if we're using a read-write database. */
  if ((db->flags & CNIDFLAG_DB_RO) == 0) {
    errno = txn_checkpoint(db->dbenv, LOGFILEMAX, CHECKTIMEMAX, 0);
    while (errno == DB_INCOMPLETE)
      errno = txn_checkpoint(db->dbenv, 0, 0, 0);
  }

 if ((buf = make_cnid_data(st, did, name, len)) == NULL) {
    syslog(LOG_ERR, "cnid_lookup: path name too long");
    return 0;
  }

  memset(&key, 0, sizeof(key));
  memset(&devdata, 0, sizeof(devdata));

  /* look for a CNID. we have two options: dev/ino or did/name. if we
     only get a match on one of them, that means a file has moved. */
  key.data = buf; /* dev/ino is the first part of the buffer */
  key.size = CNID_DEVINO_LEN;
  while ((errno = db->db_devino->get(db->db_devino, NULL,
				    &key, &devdata, 0))) {
    if (errno == DB_LOCK_DEADLOCK)
      continue;

    if (errno == DB_NOTFOUND) {
      devino = 0;
      break;
    }

    syslog(LOG_ERR, "cnid_lookup: can't get CNID(%u/%u)",
	   st->st_dev, st->st_ino);
    return 0;
  }

  /* did/name is right afterwards. */
  key.data = buf + CNID_DEVINO_LEN;
  key.size = CNID_DID_LEN + len + 1;
  memset(&diddata, 0, sizeof(diddata));
  while ((errno = db->db_didname->get(db->db_didname, NULL,
				       &key, &diddata, 0))) {
    if (errno == DB_LOCK_DEADLOCK)
      continue;

    if (errno == DB_NOTFOUND) {
      didname = 0;
      break;
    }

    syslog(LOG_ERR, "cnid_lookup: can't get CNID(%u:%s)",
	   did, name);
    return 0;
  }

  /* set id. honor did/name over dev/ino as dev/ino isn't necessarily
   * 1-1. */
  if (didname) {
    memcpy(&id, diddata.data, sizeof(id));
  } else if (devino) {
    memcpy(&id, devdata.data, sizeof(id));
  }

  /* either entries in both databases exist or neither of them do. */
  if ((devino && didname) || !(devino || didname)) {
    if (debug)
      syslog(LOG_ERR, "cnid_lookup: looked up did %d, name %s as %d", did, name, id);
    return id;
  }
  /* fix up the databases */
  cnid_update(db, id, st, did, name, len);
  if (debug)
    syslog(LOG_ERR, "cnid_lookup: looked up did %d, name %s as %d (needed update)", did, name, id);
  return id;
}
