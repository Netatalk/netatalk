/*
 * $Id: cnid_open.c,v 1.19 2001-11-05 16:06:32 jmarcus Exp $
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

#ifdef CNID_DB
#include <errno.h>
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

#define DBHOME       ".AppleDB"
#define DBCNID       "cnid.db"
#define DBDEVINO     "devino.db"
#define DBDIDNAME    "didname.db"   /* did/full name mapping */
#define DBSHORTNAME  "shortname.db" /* did/8+3 mapping */
#define DBMACNAME    "macname.db"   /* did/31 mapping */
#define DBLONGNAME   "longname.db"  /* did/unicode mapping */
#define DBLOCKFILE   "cnid.lock"

#define DBHOMELEN    8
#define DBLEN        10

/* we version the did/name database so that we can change the format
 * if necessary. the key is in the form of a did/name pair. in this case,
 * we use 0/0. */
#define DBVERSION_KEY    "\0\0\0\0\0"
#define DBVERSION_KEYLEN 5
#define DBVERSION1       0x00000001U
#define DBVERSION        DBVERSION1

#if DB_VERSION_MINOR > 1
#define DBOPTIONS    (DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | \
DB_INIT_LOG | DB_INIT_TXN)
#else /* DB_VERSION_MINOR < 1 */
/*#define DBOPTIONS    (DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | \
DB_INIT_LOG | DB_INIT_TXN | DB_TXN_NOSYNC)*/
#define DBOPTIONS    (DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | \
DB_INIT_LOG | DB_INIT_TXN)
#endif /* DB_VERSION_MINOR */

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
#if DB_VERSION_MINOR > 1
static int compare_unix(DB *db, const DBT *a, const DBT *b)
#else /* DB_VERSION_MINOR < 1 */
static int compare_unix(const DBT *a, const DBT *b)
#endif /* DB_VERSION_MINOR */
{
  u_int8_t *sa, *sb;
  int len, ret;

  /* sort by did */
  if ((ret = compare_did(a, b)))
    return ret;

  sa = (u_int8_t *) a->data + 4; /* shift past did */
  sb = (u_int8_t *) b->data + 4;
  for (len = MIN(a->size, b->size); len-- > 4; sa++, sb++)
    if ((ret = (*sa - *sb)))
      return ret; /* sort by lexical ordering */

  return a->size - b->size; /* sort by length */
}

/* sort did's and then names. this is for macified paths (i.e.,
 * did/macname, and did/shortname. i think did/longname needs a
 * unicode table to work. also, we can't use strdiacasecmp as that
 * returns a match if a < b. */
#if DB_VERSION_MINOR > 1
static int compare_mac(DB *db, const DBT *a, const DBT *b)
#else /* DB_VERSION_MINOR < 1 */
static int compare_mac(const DBT *a, const DBT *b)
#endif /* DB_VERSION_MINOR */
{
  u_int8_t *sa, *sb;
  int len, ret;

  /* sort by did */
  if ((ret = compare_did(a, b)))
    return ret;

  sa = (u_int8_t *) a->data + 4;
  sb = (u_int8_t *) b->data + 4;
  for (len = MIN(a->size, b->size); len-- > 4; sa++, sb++)
    if ((ret = (_diacasemap[*sa] - _diacasemap[*sb])))
      return ret; /* sort by lexical ordering */

  return a->size - b->size; /* sort by length */
}


/* for unicode names -- right now it's the same as compare_mac. */
#if DB_VERSION_MINOR > 1
static int compare_unicode(DB *db, const DBT *a, const DBT *b)
#else /* DB_VERSION_MINOR < 1 */
static int compare_unicode(const DBT *a, const DBT *b)
#endif /* DB_VERSION_MINOR */
{
#if DB_VERSION_MINOR > 1
    return compare_mac(db,a,b);
#else /* DB_VERSION_MINOR < 1 */
    return compare_mac(a,b);
#endif /* DB_VERSION_MINOR */
}

static int have_lock = 0;

void *cnid_open(const char *dir) {
	struct stat st;
	struct flock lock;
	char path[MAXPATHLEN + 1];
	CNID_private *db;
	DBT key, data;
	DB_TXN *tid;
	u_int32_t DBEXTRAS = 0;
	int open_flag, len;
	int rc;

	if (!dir) {
		return NULL;
	}

	/* this checks .AppleDB */
	if ((len = strlen(dir)) > (MAXPATHLEN - DBLEN - 1)) {
		syslog(LOG_ERR, "cnid_open: Pathname too large: %s", dir);
		return NULL;
	}

	if ((db = (CNID_private *)calloc(1, sizeof(CNID_private))) == NULL) {
		syslog(LOG_ERR, "cnid_open: Unable to allocate memory for database");
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

	strcpy(path + len, DBHOME);
	if ((stat(path, &st) < 0) && (ad_mkdir(path, 0777) < 0)) {
		syslog(LOG_ERR, "cnid_open: DBHOME mkdir failed for %s", path);
		goto fail_adouble;
	}

	/* Search for a byte lock.  This allows us to cleanup the log files
	 * at cnid_close() in a clean fashion.
	 *
	 * NOTE: This won't work if multiple volumes for the same user refer
	 * to the sahe directory. */
	strcat(path, DBLOCKFILE);
	if ((db->lockfd = open(path, O_RDWR | O_CREAT, 0666)) > -1) {
		lock.l_start = 0;
		lock.l_len = 1;
		while (fcntl(db->lockfd, F_SETLK, &lock) < 0) {
			if (++lock.l_start > MAXITER) {
				syslog(LOG_INFO, "cnid_open: Cannot establish logfile cleanup for database environment %s lock (lock failed)", path);
				close(db->lockfd);
				db->lockfd = -1;
				break;
			}
		}
	}
	else {
		syslog(LOG_INFO, "cnid_open: Cannot establish logfile cleanup lock for database environment %s (open() failed)", path);
	}
	
	if (!have_lock && db->lockfd > -1 && lock.l_start == 0) {
		/* We test to see if we have exclusive database access.  If we do, we
		 * will open the database with the DB_RECOVER flag.
		 */
#ifdef DEBUG
		syslog(LOG_INFO, "cnid_open: Opening database environment %s with DB_RECOVER flag", path);	
#endif
		DBEXTRAS |= DB_RECOVER;
		have_lock = 1;
	}


	path[len + DBHOMELEN] = '\0';
	open_flag = DB_CREATE;

	/* We need to be able to open the database environment with full 
	 * transaction, logging, and locking support if we ever hope to 
	 * be a true multi-acess file server. */
	if ((rc = db_env_create(&db->dbenv, 0)) != 0) {
		syslog(LOG_ERR, "cnid_open: db_env_create: %s", db_strerror(rc));
		goto fail_lock;
	}

	/* Setup internal deadlock detection. */
	if ((rc = db->dbenv->set_lk_detect(db->dbenv, DB_LOCK_DEFAULT)) != 0) {
		syslog(LOG_ERR, "cnid_open: set_lk_detect: %s", db_strerror(rc));
		goto fail_lock;
	}

#if DB_VERSION_MINOR > 1
	/* Take care of setting the DB_TXN_NOSYNC flag in db3 > 3.1.x. */
/*	if ((rc = db->dbenv->set_flags(db->dbenv, DB_TXN_NOSYNC, 1)) != 0) {
		syslog(LOG_ERR, "cnid_open: set_flags: %s", db_strerror(rc));
		goto fail_lock;
	}*/
#endif /* DB_VERSION_MINOR > 1 */

	/* Open the database environment. */
	if ((rc = db->dbenv->open(db->dbenv, path, DBOPTIONS | DBEXTRAS, 0666)) != 0) {
		if (rc == DB_RUNRECOVERY) {
			/* This is the mother of all errors.  We _must_ fail here. */
			syslog(LOG_ERR, "cnid_open: CATASTROPHIC ERROR opening database environment %s.  Run db_recovery -c immediately", path);
			goto fail_lock;
		}

		/* We can't get a full transactional environment, so multi-access
		 * is out of the question.  Let's assume a read-only environment,
		 * and trry to at least get a shared memory pool. */
		if ((rc = db->dbenv->open(db->dbenv, path, DB_INIT_MPOOL, 0666)) != 0) {
			/* Nope, not a MPOOL, either.  Last-ditch effort: we'll try to
			 * open the environment with no flags. */
			if ((rc = db->dbenv->open(db->dbenv, path, 0, 0666)) != 0) {
				syslog(LOG_ERR, "cnid_open: dbenv->open of %s failed: %s",
				       path, db_strerror(rc));
				goto fail_lock;
			}
		}
		db->flags |= CNIDFLAG_DB_RO;
		open_flag = DB_RDONLY;
		syslog(LOG_INFO, "cnid_open: Obtained read-only database environment %s", path);
	}

	/* did/name reverse mapping.  We use a BTree for this one. */
	if ((rc = db_create(&db->db_didname, db->dbenv, 0)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to create did/name database: %s", 
		       db_strerror(rc));
		goto fail_appinit;
	}

	db->db_didname->set_bt_compare(db->db_didname, &compare_unix);
	if ((rc = db->db_didname->open(db->db_didname, DBDIDNAME, NULL,
		 DB_BTREE, open_flag, 0666))) {
		syslog(LOG_ERR, "cnid_open: Failed to open did/name database: %s",
		       db_strerror(rc));
		goto fail_appinit;
	}

	/* Check for version.  This way we can update the database if we need
	 * to change the format in any way. */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = DBVERSION_KEY;
	key.size = DBVERSION_KEYLEN;

dbversion_retry:
	if ((rc = txn_begin(db->dbenv, NULL, &tid, 0)) != 0) {
		syslog(LOG_ERR, "cnid_open: txn_begin: failed to check db version: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
		goto fail_appinit;
	}

	while ((rc = db->db_didname->get(db->db_didname, tid, &key, &data, DB_RMW))) {
		int ret;
		switch (rc) {
			case DB_LOCK_DEADLOCK:
				if ((ret = txn_abort(tid)) != 0) {
					syslog(LOG_ERR, "cnid_open: txn_abort: %s", db_strerror(ret));
					db->db_didname->close(db->db_didname, 0);
					goto fail_appinit;
				}
				goto dbversion_retry;
			case DB_NOTFOUND:
			{
				u_int32_t version = htonl(DBVERSION);

				data.data = &version;
				data.size = sizeof(version);
			}

			if ((ret = db->db_didname->put(db->db_didname, tid, &key, &data,
				      DB_NOOVERWRITE))) {
				if (ret == DB_LOCK_DEADLOCK) {
					if ((ret = txn_abort(tid)) != 0) {
						syslog(LOG_ERR, "cnid_open: txn_abort: %s",
						       db_strerror(ret));
						db->db_didname->close(db->db_didname, 0);
						goto fail_appinit;
					}
					goto dbversion_retry;
				}
				else if (ret == DB_RUNRECOVERY) {
					/* At this point, we don't care if the transaction aborts
					 * successfully or not. */
					txn_abort(tid);
					syslog(LOG_ERR, "cnid_open: Error putting new version: %s",
					       db_strerror(ret));
					db->db_didname->close(db->db_didname, 0);
					goto fail_appinit;
				}
			}
				break; /* while loop */
			default:
				txn_abort(tid);
				syslog(LOG_ERR, "cnid_open: Failed to check db version: %s",
				       db_strerror(rc));
				db->db_didname->close(db->db_didname, 0);
				goto fail_appinit;
		}
	}

	if ((rc = txn_commit(tid, 0)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to commit db version: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
		goto fail_appinit;
	}

	/* TODO In the future we might check for version number here. */
#if 0
	memcpy(&version, data.data, sizeof(version));
	if (version != ntohl(DBVERSION)) {
		/* Do stuff here. */
	}
#endif /* 0 */

#ifdef EXTENDED_DB
	/* did/macname (31 character) mapping.  Use a BTree for this one. */
	if ((rc = db_create(&db->db_macname, db->dbenv, 0)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to create did/macname database: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
		goto fail_appinit;
	}

	db->db_macname->set_bt_compare(db->db_macname, &compare_mac);
	if ((rc = db->db_macname->open(db->db_macname, DBMACNAME, NULL, DB_BTREE, open_flag, 0666)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to open did/macname database: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
		goto fail_appinit;
	}

	/* did/shortname (DOS 8.3) mapping.  Use a BTree for this one. */
	if ((rc = db_create(&db->db_shortname, db->dbenv, 0)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to create did/shortname database: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
		db->db_macname->close(db->db_macname, 0);
		goto fail_appinit;
	}

	db->db_shortname->set_bt_compare(db->db_shortname, &compare_mac);
	if ((rc = db->db_shortname->open(db->db_shortname, DBSHORTNAME, NULL, DB_BTREE, open_flag, 0666)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to open did/shortname database: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
		db->db_macname->close(db->db_macname, 0);
		goto fail_appinit;
	}

	/* did/longname (Unicode) mapping.  Use a BTree for this one. */
	if ((rc = db_create(&db->db_longname, db->dbenv, 0)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to create did/longname database: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
		db->db_macname->close(db->db_macname, 0);
		db->db_shortname->close(db->db_shortname, 0);
		goto fail_appinit;
	}

	db->db_longname->set_bt_compare(db->db_longname, &compare_unicode);
	if ((rc = db->db_longname->open(db->db_longname, DBLONGNAME, NULL, DB_BTREE, open_flag, 0666)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to open did/longname database: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
		db->db_macname->close(db->db_macname, 0);
		db->db_shortname->close(db->db_shortname, 0);
		goto fail_appinit;
	}
#endif /* EXTENDED_DB */

	/* dev/ino reverse mapping.  Use a hash for this one. */
	if ((rc = db_create(&db->db_devino, db->dbenv, 0)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to create dev/ino database: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
#ifdef EXTENDED_DB
		db->db_macname->close(db->db_macname, 0);
		db->db_shortname->close(db->db_shortname, 0);
		db->db_longname->close(db->db_longname, 0);
#endif /* EXTENDED_DB */
		goto fail_appinit;
	}

	if ((rc = db->db_devino->open(db->db_devino, DBDEVINO, NULL, DB_HASH, open_flag, 0666)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to open devino database: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
#ifdef EXTENDED_DB
		db->db_macname->close(db->db_macname, 0);
		db->db_shortname->close(db->db_shortname, 0);
		db->db_longname->close(db->db_longname, 0);
#endif /* EXTENDED_DB */
		goto fail_appinit;
	}

	/* Main CNID database.  Use a hash for this one. */
	if ((rc = db_create(&db->db_cnid, db->dbenv, 0)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to create cnid database: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
#ifdef EXTENDED_DB
		db->db_macname->close(db->db_macname, 0);
		db->db_shortname->close(db->db_shortname, 0);
		db->db_longname->close(db->db_longname, 0);
#endif /* EXTENDED_DB */
		db->db_devino->close(db->db_devino, 0);
		goto fail_appinit;
	}


	if ((rc = db->db_cnid->open(db->db_cnid, DBCNID, NULL, DB_HASH, open_flag, 0666)) != 0) {
		syslog(LOG_ERR, "cnid_open: Failed to open dev/ino database: %s",
		       db_strerror(rc));
		db->db_didname->close(db->db_didname, 0);
#ifdef EXTENDED_DB
		db->db_macname->close(db->db_macname, 0);
		db->db_shortname->close(db->db_shortname, 0);
		db->db_longname->close(db->db_longname, 0);
#endif /* EXTENDED_DB */
		goto fail_appinit;
	}

	return db;

fail_appinit:
	syslog(LOG_ERR, "cnid_open: Failed to setup CNID DB environment");
	db->dbenv->close(db->dbenv, 0);

fail_lock:
	if (db->lockfd > -1) {
		close(db->lockfd);
	}

fail_adouble:

fail_db:
	free(db);
	return NULL;
}
#endif /* CNID_DB */


