/* 
 * $Id: cnid_open.c,v 1.2 2001-06-29 14:14:46 rufustfirefly Exp $
 *
 * Copyright (c) 1999. Adrian Sun (asun@zoology.washington.edu)
 * All Rights Reserved. See COPYRIGHT.
 *
 * CNID database support. 
 *
 * here's the deal:
 *  1) afpd already caches did's. 
 *  2) the database stores cnid's as both did/name and dev/ino pairs. 
 *  3) RootInfo holds the value of the NextID.
 *  4) the cnid database gets called in the following manner --
 *     start a database:
 *     cnid = cnid_open(root_dir);
 *
 *     allocate a new id: 
 *     newid = cnid_add(cnid, dev, ino, parent did,
 *     name, id); id is a hint for a specific id. pass 0 if you don't
 *     care. if the id is already assigned, you won't get what you
 *     requested.
 *
 *     given an id, get a did/name and dev/ino pair.
 *     name = cnid_get(cnid, &id); given an id, return the corresponding
 *     info.
 *     return code = cnid_delete(cnid, id); delete an entry. 
 *
 * with AFP, CNIDs 0-2 have special meanings. here they are:
 * 0 -- invalid cnid
 * 1 -- parent of root directory (handled by afpd) 
 * 2 -- root directory (handled by afpd)
 *
 * so, CNID_START begins at 3.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <sys/param.h>
#include <sys/stat.h>
#include <syslog.h>

#include <db.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/util.h>

#include "cnid_private.h"

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif /* ! MIN */

#define ROOTINFO     "RootInfo"
#define ROOTINFO_LEN 8

#define DBHOME       ".AppleDB"
#define DBCNID       "cnid.db"
#define DBDEVINO     "devino.db"
#define DBDIDNAME    "didname.db"   /* did/full name mapping */
#define DBSHORTNAME  "shortname.db" /* did/8+3 mapping */
#define DBMACNAME    "macname.db"   /* did/31 mapping */
#define DBLONGNAME   "longname.db"  /* did/unicode mapping */
#define DBLOCKFILE   "/cnid.lock"

#define DBHOMELEN    8
#define DBLEN        10

/* we version the did/name database so that we can change the format
 * if necessary. the key is in the form of a did/name pair. in this case,
 * we use 0/0. */
#define DBVERSION_KEY    "\0\0\0\0\0"
#define DBVERSION_KEYLEN 5
#define DBVERSION1       0x00000001U
#define DBVERSION        DBVERSION1

#define DBOPTIONS    (DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | \
DB_INIT_LOG | DB_INIT_TXN | DB_TXN_NOSYNC | DB_RECOVER)

#define MAXITER     0xFFFF /* maximum number of simultaneously open CNID 
			    * databases. */

/* the first compare that's always done. */
static __inline__ int compare_did(const DBT *a, const DBT *b)
{
  u_int32_t dida, didb;

  memcpy(&dida, a->data, sizeof(dida));
  memcpy(&didb, b->data, sizeof(didb));
  return dida - didb;
}

/* sort did's and then names. this is for unix paths.
 * i.e., did/unixname lookups. */
static int compare_unix(const DBT *a, const DBT *b)
{
  u_int8_t *sa, *sb;
  int len, ret;

  /* sort by did */
  if (ret = compare_did(a, b))
    return ret;

  sa = a->data + 4; /* shift past did */
  sb = b->data + 4;
  for (len = MIN(a->size, b->size); len-- > 4; sa++, sb++)
    if (ret = (*sa - *sb))
      return ret; /* sort by lexical ordering */
  return a->size - b->size; /* sort by length */
}

/* sort did's and then names. this is for macified paths (i.e.,
 * did/macname, and did/shortname. i think did/longname needs a
 * unicode table to work. also, we can't use strdiacasecmp as that
 * returns a match if a < b. */
static int compare_mac(const DBT *a, const DBT *b) 
{
  u_int8_t *sa, *sb;
  int len, ret;
  
  /* sort by did */
  if (ret = compare_did(a, b))
    return ret;

  sa = a->data + 4;
  sb = b->data + 4;
  for (len = MIN(a->size, b->size); len-- > 4; sa++, sb++)
    if (ret = (_diacasemap[*sa] - _diacasemap[*sb]))
      return ret; /* sort by lexical ordering */
  return a->size - b->size; /* sort by length */
}


/* for unicode names -- right now it's the same as compare_mac. */
#define compare_unicode(a, b) compare_mac((a), (b))

void *cnid_open(const char *dir)
{
  struct stat st;
  struct flock lock;
  char path[MAXPATHLEN + 1];
  CNID_private *db;
  DB_INFO dbi;
  DBT key, data;
  int open_flag, len;

  if (!dir)
    return NULL;

  /* this checks both RootInfo and .AppleDB */
  if ((len = strlen(dir)) > (MAXPATHLEN - DBLEN - 1)) {
    syslog(LOG_ERR, "cnid_open: path too large");
    return NULL;
  }
  
  if ((db = (CNID_private *) calloc(1, sizeof(CNID_private))) == NULL) {
    syslog(LOG_ERR, "cnid_open: unable to allocate memory");
    return NULL;
  }
  db->magic = CNID_DB_MAGIC;
    
  strcpy(path, dir);
  if (path[len - 1] != '/') {
    strcat(path, "/");
    len++;
  }

  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;

  /* we create and initialize RootInfo if it doesn't exist. */
  strcat(path, ROOTINFO);
  if (ad_open(path, ADFLAGS_HF, O_RDWR, 0666, &db->rootinfo) < 0) {
    cnid_t id;

    /* see if we can open it read-only. if it's read-only, we can't
     * add CNIDs. */
    memset(&db->rootinfo, 0, sizeof(db->rootinfo));
    if (ad_open(path, ADFLAGS_HF, O_RDONLY, 0666, &db->rootinfo) == 0) {
      db->flags = CNIDFLAG_ROOTINFO_RO;
      syslog(LOG_INFO, "cnid_open: read-only RootInfo");
      goto mkdir_appledb;
    }

    /* create the file */
    memset(&db->rootinfo, 0, sizeof(db->rootinfo));
    if (ad_open(path, ADFLAGS_HF, O_CREAT | O_RDWR, 0666, 
		&db->rootinfo) < 0) {
      syslog(LOG_ERR, "cnid_open: ad_open(RootInfo)");
      goto fail_db;
    }

    /* lock the RootInfo file. this and cnid_add are the only places
     * that should fiddle with RootInfo. */
    lock.l_start = ad_getentryoff(&db->rootinfo, ADEID_DID);
    lock.l_len = ad_getentrylen(&db->rootinfo, ADEID_DID);
    if (fcntl(ad_hfileno(&db->rootinfo), F_SETLKW,  &lock) < 0) {
      syslog(LOG_ERR, "cnid_open: can't establish lock: %m");
      goto fail_adouble;
    }
    
    /* store the beginning CNID */
    id = htonl(CNID_START);
    memcpy(ad_entry(&db->rootinfo, ADEID_DID), &id, sizeof(id));
    ad_flush(&db->rootinfo, ADFLAGS_HF);

    /* unlock it */
    lock.l_type = F_UNLCK;
    fcntl(ad_hfileno(&db->rootinfo), F_SETLK, &lock);
    lock.l_type = F_WRLCK;
  }

mkdir_appledb:
  strcpy(path + len, DBHOME);
  if ((stat(path, &st) < 0) && (ad_mkdir(path, 0777) < 0)) {
    syslog(LOG_ERR, "cnid_open: mkdir failed");
    goto fail_adouble;
  }

  /* search for a byte lock. this allows us to cleanup the log files
   * at cnid_close() in a clean fashion.
   *
   * NOTE: this won't work if multiple volumes for the same user refer
   * to the same directory. */
  strcat(path, DBLOCKFILE);
  if ((db->lockfd = open(path, O_RDWR | O_CREAT, 0666)) > -1) {
    lock.l_start = 0;
    lock.l_len = 1;
    while (fcntl(db->lockfd, F_SETLK, &lock) < 0) {
      if (++lock.l_start > MAXITER) {
	syslog(LOG_INFO, "cnid_open: can't establish logfile cleanup lock.");
	close(db->lockfd);
	db->lockfd = -1;
	break;
      }      
    }
  } 

  path[len + DBHOMELEN] = '\0';
  open_flag = DB_CREATE;
  /* try a full-blown transactional environment */
  if (db_appinit(path, NULL, &db->dbenv, DBOPTIONS)) {

    /* try with a shared memory pool */
    memset(&db->dbenv, 0, sizeof(db->dbenv));
    if (db_appinit(path, NULL, &db->dbenv, DB_INIT_MPOOL)) {

      /* try without any options. */
      memset(&db->dbenv, 0, sizeof(db->dbenv));
      if (db_appinit(path, NULL, &db->dbenv, 0)) {
	syslog(LOG_ERR, "cnid_open: db_appinit failed");
	goto fail_lock;
      }
    }
    db->flags |= CNIDFLAG_DB_RO;
    open_flag = DB_RDONLY;
    syslog(LOG_INFO, "cnid_open: read-only CNID database");
  }

  memset(&dbi, 0, sizeof(dbi));

  /* did/name reverse mapping. we use a btree for this one. */
  dbi.bt_compare = compare_unix;
  if (db_open(DBDIDNAME, DB_BTREE, open_flag, 0666, &db->dbenv, &dbi,
	      &db->db_didname)) {
    goto fail_appinit;
  }

  /* check for version. this way we can update the database if we need
     to change the format in any way. */
  memset(&key, 0, sizeof(key));
  memset(&data, 0, sizeof(data));
  key.data = DBVERSION_KEY;
  key.len = DBVERSION_KEY_LEN;
  while (errno = db->db_didname->get(db->db_didname, NULL, &key, &data, 0)) {
    switch (errno) {
    case EAGAIN:
      continue;
    
    case DB_NOTFOUND:
      u_int32_t version = htonl(DBVERSION);

      data.data = &version;
      data.len = sizeof(version);
dbversion_retry:
      if (db->db_didname->put(db->db_didname, NULL, &key, &data,
			      DB_NOOVERWRITE))
	if (errno == EAGAIN)
	  goto dbversion_retry;
      break;
    default:
      /* uh oh. something bad happened. bail. */
      db->db_didname->close(db->db_didname, 0);
      goto fail_appinit;
    }
  }
  
  /* XXX: in the future, we might check for version number here. */
#if 0
  memcpy(&version, data.data, sizeof(version));
  if (version != htonl(DBVERSION)) {
    /* fix up stuff */
  }
#endif /* 0 */

  /* did/macname mapping. btree this one. */
  dbi.bt_compare = compare_mac;
  if (db_open(DBMACNAME, DB_BTREE, open_flag, 0666, &db->dbenv, &dbi,
	      &db->db_macname)) {
    db->db_didname->close(db->db_didname, 0);
    goto fail_appinit;
  }

  /* did/shortname mapping */
  if (db_open(DBSHORTNAME, DB_BTREE, open_flag, 0666, &db->dbenv, &dbi,
	      &db->db_shortname)) {
    db->db_didname->close(db->db_didname, 0);
    db->db_macname->close(db->db_macname, 0);
    goto fail_appinit;
  }

  /* did/longname mapping */
  dbi.bt_compare = compare_unicode;
  if (db_open(DBLONGNAME, DB_BTREE, open_flag, 0666, &db->dbenv, &dbi,
	      &db->db_longname)) {
    db->db_didname->close(db->db_didname, 0);
    db->db_macname->close(db->db_macname, 0);
    db->db_shortname->close(db->db_shortname, 0);
    goto fail_appinit;
  }

  /* dev/ino reverse mapping. we hash this one. */
  dbi.bt_compare = NULL;
  if (db_open(DBDEVINO, DB_HASH, open_flag, 0666, &db->dbenv, &dbi,
	      &db->db_devino)) {
    db->db_didname->close(db->db_didname, 0);
    db->db_macname->close(db->db_macname, 0);
    db->db_shortname->close(db->db_shortname, 0);
    db->db_longname->close(db->db_longname, 0);
    goto fail_appinit;
  }

  /* main cnid database. we hash this one as well. */
  if (db_open(DBCNID, DB_HASH, open_flag, 0666, &db->dbenv, &dbi,
	      &db->db_cnid)) {
    db->db_didname->close(db->db_didname, 0);
    db->db_macname->close(db->db_macname, 0);
    db->db_shortname->close(db->db_shortname, 0);
    db->db_longname->close(db->db_longname, 0);
    db->db_devino->close(db->db_devino, 0);
    goto fail_appinit;
  }
  return db;
  
fail_appinit:
  syslog(LOG_ERR, "cnid_open: db_open failed");
  db_appexit(&db->dbenv);

fail_lock:
  if (db->lockfd > -1)
    close(db->lockfd);

fail_adouble:
  ad_close(&db->rootinfo, ADFLAGS_HF);

fail_db:
  free(db);
  return NULL;
}
