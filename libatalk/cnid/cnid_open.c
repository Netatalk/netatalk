/*
 * $Id: cnid_open.c,v 1.54 2003-07-10 13:50:39 rlewczuk Exp $
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
 * CNIDs 4-16 are reserved according to page 31 of the AFP 3.0 spec so, 
 * CNID_START begins at 17.
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
#include <atalk/logger.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS_TIME_H */

#include <db.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/util.h>

#include "cnid_private.h"

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif /* ! MIN */

#ifndef MAX
#define MAX(a, b)  ((a) > (b) ? (a) : (b))
#endif /* ! MIN */


#define DBHOME        ".AppleDB"
#define DBCNID        "cnid.db"
#define DBDEVINO      "devino.db"
#define DBDIDNAME     "didname.db"   /* did/full name mapping */
#define DBMANGLE      "mangle.db"    /* filename mangling */
#define DBLOCKFILE    "cnid.lock"
#define DBRECOVERFILE "cnid.dbrecover"
#define DBCLOSEFILE   "cnid.close"

#define DBHOMELEN    8
#define DBLEN        10

/* we version the did/name database so that we can change the format
 * if necessary. the key is in the form of a did/name pair. in this case,
 * we use 0/0. */
#define DBVERSION_KEY    "\0\0\0\0\0"
#define DBVERSION_KEYLEN 5
#define DBVERSION1       0x00000001U
#define DBVERSION        DBVERSION1

#ifdef CNID_DB_CDB
#define DBOPTIONS    (DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL)
#else /* !CNID_DB_CDB */
#if DB_VERSION_MAJOR >= 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR > 1)
#define DBOPTIONS    (DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | \
DB_INIT_LOG | DB_INIT_TXN)
#else /* DB_VERSION_MINOR < 1 */
/*#define DBOPTIONS    (DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | \
DB_INIT_LOG | DB_INIT_TXN | DB_TXN_NOSYNC)*/
#define DBOPTIONS    (DB_CREATE | DB_INIT_MPOOL | DB_INIT_LOCK | \
DB_INIT_LOG | DB_INIT_TXN)
#endif /* DB_VERSION_MINOR */
#endif /* CNID_DB_CDB */

#ifndef CNID_DB_CDB
/* Let's try and use the youngest lock detector if present.
 * If we can't do that, then let BDB use its default deadlock detector. */
#if defined DB_LOCK_YOUNGEST
#define DEAD_LOCK_DETECT DB_LOCK_YOUNGEST
#else /* DB_LOCK_YOUNGEST */
#define DEAD_LOCK_DETECT DB_LOCK_DEFAULT
#endif /* DB_LOCK_YOUNGEST */
#endif /* CNID_DB_CDB */

static cnid_t cnid_rebuild (CNID_private *, mode_t);
static int    cnid_recover (char *, mode_t);

#define MAXITER     0xFFFF /* maximum number of simultaneously open CNID
* databases. */

/* -----------------------
 * bandaid for LanTest performance pb.
*/
static int my_yield(void) 
{
    struct timeval t;
    int ret;

    t.tv_sec = 0;
    t.tv_usec = 1000;
    ret = select(0, NULL, NULL, NULL, &t);
    return 0;
}

/* --------------- */
static int  my_open(DB *p, const char *f, const char *d, DBTYPE t, u_int32_t flags, int mode)
{
#if DB_VERSION_MAJOR > 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 1)
    return p->open(p, NULL, f, d, t, flags, mode);
#else
    return p->open(p,       f, d, t, flags, mode);
#endif
}

void *cnid_open(const char *dir, mode_t mask) {
    struct stat st;
    struct flock lock;
    char path[MAXPATHLEN + 1];
    CNID_private *db;
    DBT key, data;
    DB_TXN *tid;
    int open_flag, len;
    int rc;
    int can_check = 0, require_rebuild = 0;
    u_int32_t ncnid, ndidname, ndevino;
    DB_HASH_STAT *sp;
    static int first = 0;

    ncnid = ndidname = ndevino = 0;


    if (!dir) {
        return NULL;
    }

    /* this checks .AppleDB */
    if ((len = strlen(dir)) > (MAXPATHLEN - DBLEN - 1)) {
        LOG(log_error, logtype_default, "cnid_open: Pathname too large: %s", dir);
        return NULL;
    }

    if ((db = (CNID_private *)calloc(1, sizeof(CNID_private))) == NULL) {
        LOG(log_error, logtype_default, "cnid_open: Unable to allocate memory for database");
        return NULL;
    }

    db->magic = CNID_DB_MAGIC;

    strcpy(path, dir);
    if (path[len - 1] != '/') {
        strcat(path, "/");
        len++;
    }

    strcpy(path + len, DBHOME);
    if ((stat(path, &st) < 0) && (ad_mkdir(path, 0777 & ~mask) < 0)) {
        LOG(log_error, logtype_default, "cnid_open: DBHOME mkdir failed for %s", path);
        goto fail_adouble;
    }

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    /* Make sure cnid.lock goes in .AppleDB. */
    strcat(path, "/");
    len++;

    strcat(path, DBLOCKFILE);
    strcpy(db->lock_file, path);
    if ( stat (path, &st) == 0)
	can_check = 1;

    if ((db->lockfd = open(path, O_RDWR | O_CREAT, 0666 & ~mask)) > -1) {
        lock.l_start = 0;
        lock.l_len = 1;
        if (fcntl(db->lockfd, F_SETLK, &lock) < 0) {
		/* Now try to get a shared lock */
		lock.l_type = F_RDLCK;

		while (fcntl(db->lockfd, F_SETLKW, &lock) < 0) {
			if ( errno != EINTR )
				goto fail_db;
		}
		can_check = 0;
	}
	else {
		/* We got an exclusive lock, so we're the first/only process accessing the database */
		if ( can_check ) {
			/* cnid.lock existed but we got an exclusive lock */
			/* This means the previous afpd probably crashed  */
			/* or got terminated so we run recovery 	  */

   			path[len + DBHOMELEN] = '\0';
			if ( cnid_recover( path, mask) != 0)
				goto fail_lock;
		}
		else {
			/* We're the only process accesing the db, so lets do some constistency checks */
			can_check = 1; 
		}
	}
    }
    else
    {
        LOG(log_error, logtype_default, "cnid_open: Cannot open cnid.lock file in %s lock (lock failed)", path);
        goto fail_db;
    }

    path[len + DBHOMELEN] = '\0';
    open_flag = DB_CREATE;

    /* We need to be able to open the database environment with full
     * transaction, logging, and locking support if we ever hope to 
     * be a true multi-acess file server. */
    if ((rc = db_env_create(&db->dbenv, 0)) != 0) {
        LOG(log_error, logtype_default, "cnid_open: db_env_create: %s", db_strerror(rc));
        goto fail_lock;
    }

    if ((rc = db->dbenv->open(db->dbenv, path, DB_JOINENV, 0666 & ~mask)) !=0 &&
        (rc = db->dbenv->open(db->dbenv, path, DBOPTIONS, 0666 & ~mask)) != 0) {
        if (rc == DB_RUNRECOVERY) {
            /* This is the mother of all errors.  We _must_ fail here. */
            LOG(log_error, logtype_default, "cnid_open: CATASTROPHIC ERROR opening database environment %s.  Run db_recovery -c immediately", path);
            goto fail_lock;
        }

        /* We can't get a full transactional environment, so multi-access
         * is out of the question.  Let's assume a read-only environment,
         * and try to at least get a shared memory pool. */
        if ((rc = db->dbenv->open(db->dbenv, path, DB_INIT_MPOOL, 0666 & ~mask)) != 0) {
            /* Nope, not a MPOOL, either.  Last-ditch effort: we'll try to
             * open the environment with no flags. */
            if ((rc = db->dbenv->open(db->dbenv, path, 0, 0666 & ~mask)) != 0) {
                LOG(log_error, logtype_default, "cnid_open: dbenv->open of %s failed: %s",
                    path, db_strerror(rc));
                goto fail_lock;
            }
        }
        db->flags |= CNIDFLAG_DB_RO;
        open_flag = DB_RDONLY;
        LOG(log_info, logtype_default, "cnid_open: Obtained read-only database environment %s", path);
    }

    /* did/name reverse mapping.  We use a BTree for this one. */
    if ((rc = db_create(&db->db_didname, db->dbenv, 0)) != 0) {
        LOG(log_error, logtype_default, "cnid_open: Failed to create did/name database: %s",
            db_strerror(rc));
        goto fail_appinit;
    }

    if ((rc = my_open(db->db_didname, DBDIDNAME, NULL, DB_HASH, open_flag, 0666 & ~mask))) {
        LOG(log_error, logtype_default, "cnid_open: Failed to open did/name database: %s",
            db_strerror(rc));
        /* We leak some memory here, but otherwise sometime we run into a SIGSEGV in close */	
        db->db_didname = NULL; 
	if (can_check) {
		require_rebuild = 1;
		goto rebuild;
	}
	else
        	goto fail_appinit;
    }

    /* Check for version.  This way we can update the database if we need
     * to change the format in any way. */
    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));
    key.data = DBVERSION_KEY;
    key.size = DBVERSION_KEYLEN;

    if ((rc = db->db_didname->get(db->db_didname, NULL, &key, &data, 0)) != 0) {
        int ret;
        {
            u_int32_t version = htonl(DBVERSION);

            data.data = &version;
            data.size = sizeof(version);
        }
        if ((ret = db->db_didname->put(db->db_didname, NULL, &key, &data,
                                       DB_NOOVERWRITE))) {
            LOG(log_error, logtype_default, "cnid_open: Error putting new version: %s",
                db_strerror(ret));
            goto fail_appinit;
        }
    }

    /* TODO In the future we might check for version number here. */
#if 0
    memcpy(&version, data.data, sizeof(version));
    if (version != ntohl(DBVERSION)) {
        /* Do stuff here. */
    }
#endif /* 0 */

    /* dev/ino reverse mapping.  Use a hash for this one. */
    if ((rc = db_create(&db->db_devino, db->dbenv, 0)) != 0) {
        LOG(log_error, logtype_default, "cnid_open: Failed to create dev/ino database: %s",
            db_strerror(rc));
        goto fail_appinit;
    }

    if ((rc = my_open(db->db_devino, DBDEVINO, NULL, DB_HASH, open_flag, 0666 & ~mask))) {
        LOG(log_error, logtype_default, "cnid_open: Failed to open devino database: %s",
            db_strerror(rc));
	db->db_devino = NULL; 
	if (can_check) {
		require_rebuild = 1;
		goto rebuild;
	}
	else
        	goto fail_appinit;
    }

rebuild:

    /* Main CNID database.  Use a hash for this one. */
    if ((rc = db_create(&db->db_cnid, db->dbenv, 0)) != 0) {
        LOG(log_error, logtype_default, "cnid_open: Failed to create cnid database: %s",
            db_strerror(rc));
        goto fail_appinit;
    }

    if ((rc = my_open(db->db_cnid, DBCNID, NULL, DB_HASH, open_flag, 0666 & ~mask))) {
        LOG(log_error, logtype_default, "cnid_open: Failed to open cnid database: %s",
            db_strerror(rc));
	if ( require_rebuild )
        	LOG(log_error, logtype_default, "cnid_open: CNID Databases are corrupted beyond repair, your probably should delete all files in %s", path);
        goto fail_appinit;
    }

    /* Check database consistency */
    if ( can_check ) {
	if ((db->db_didname) && ((rc = db->db_didname->stat(db->db_didname, &sp, 0)) != 0)) {
        	LOG(log_error, logtype_default, "cnid_open: Failed to stat did/macname database: %s",
	            db_strerror(rc));
		require_rebuild = 1;
	}
	if (sp) {
	    	ndidname = (sp->hash_nkeys > 2) ? sp->hash_nkeys-2 : 0;
	    	free (sp);
	}

    	if ((db->db_devino) && ((rc = db->db_devino->stat(db->db_devino, &sp, 0)) != 0)) {
        	LOG(log_error, logtype_default, "cnid_open: Failed to stat dev/ino database: %s",
	            db_strerror(rc));
		require_rebuild = 1;
	}
	if (sp) {
    		ndevino = sp->hash_nkeys;
	    	free (sp);
	}

    	if ((rc = db->db_cnid->stat(db->db_cnid, &sp, 0)) != 0) {
        	LOG(log_error, logtype_default, "cnid_open: Failed to stat cnid database: %s",
            	    db_strerror(rc));
        	LOG(log_error, logtype_default, "cnid_open: CNID Databases are corrupted beyond repair, your probably should delete all files in %s", path);
		goto fail_appinit;
    	}
  	ncnid = sp->hash_nkeys;

      	if ( require_rebuild || ndidname != ncnid || ndevino != ncnid ) {
        	LOG(log_error, logtype_default, "cnid_open: databases corrupt, number of entries mismatch (didname: %u, devino: %u, cnid: %u)", ndidname, ndevino, ncnid);
		if ( CNID_INVALID == cnid_rebuild(db, mask) ) 
        		goto fail_appinit;
    	}
	/* now downgrade to a shared lock */

	lock.l_type = F_RDLCK;
	if (fcntl(db->lockfd, F_SETLK, &lock) < 0) {
		LOG (log_error, logtype_default, "cnid_open: Cannot set shared lock");
	}
	can_check = 0;
    }

#ifdef FILE_MANGLING
    /* filename mangling database.  Use a hash for this one. */
    if ((rc = db_create(&db->db_mangle, db->dbenv, 0)) != 0) {
        LOG(log_error, logtype_default, "cnid_open: Failed to create mangle database: %s", db_strerror(rc));
        goto fail_appinit;
    }

    if ((rc = my_open(db->db_mangle, DBMANGLE, NULL, DB_HASH, open_flag, 0666 & ~mask))) {
        LOG(log_error, logtype_default, "cnid_open: Failed to open mangle database: %s", db_strerror(rc));
        goto fail_appinit;
    }
#endif /* FILE_MANGLING */

    /* Print out the version of BDB we're linked against. only once */
    if (!first) {
        first = 1;
        LOG(log_info, logtype_default, "CNID DB initialized using %s", db_version(NULL, NULL, NULL));
    }

    db_env_set_func_yield(my_yield);
    return db;

fail_appinit:
    if (db->db_didname) db->db_didname->close(db->db_didname, 0);
    if (db->db_devino)  db->db_devino->close(db->db_devino, 0);
    if (db->db_cnid)    db->db_cnid->close(db->db_cnid, 0);
    LOG(log_error, logtype_default, "cnid_open: Failed to setup CNID DB environment");
    db->dbenv->close(db->dbenv, 0);

fail_lock:
    if (db->lockfd > -1 && !can_check ) {
        close(db->lockfd);
        (void)remove(db->lock_file);
    }

fail_adouble:

fail_db:
    free(db);
    return NULL;
}


/*-------------------------*/

static int cnid_recover(char *path, mode_t mask)
{
	DB_ENV * dbenv;
	int open_flag = 0, rc = 0;

	if ( !path || strlen(path) == 0)
		return -1;

	LOG (log_info, logtype_default, "cnid_open: running recover on %s", path);

	/* Remove any existing environment */

	if ((rc = db_env_create(&dbenv, 0)) != 0) {
       		LOG(log_error, logtype_default, "cnid_recover: db_env_create: %s", db_strerror(rc));
		return rc;
 	}
	if ((rc = dbenv->remove(dbenv, path, DB_FORCE)) != 0) {
		LOG(log_error, logtype_default, "cnid_recover db_env_remove: %s", db_strerror(rc));
		return rc;
	}

	/* Create and open recover environment */

	if ((rc = db_env_create(&dbenv, 0)) != 0) {
       		LOG(log_error, logtype_default, "cnid_recover: db_env_create: %s", db_strerror(rc));
		return rc;
   	}

	open_flag = DB_CREATE | DB_INIT_LOCK | DB_INIT_LOG | DB_INIT_MPOOL | DB_INIT_TXN | DB_RECOVER | DB_PRIVATE;		
    	if ((rc = dbenv->open(dbenv, path, open_flag, 0666 & ~mask)) != 0) {
		LOG (log_error, logtype_default, "cnid_recover: RECOVERY failed with %s", db_strerror(rc));
		return rc;
	}

	if ((rc = dbenv->close(dbenv, 0)) != 0) {
       		LOG(log_error, logtype_default, "cnid_recover: dbenv->close: %s", db_strerror(rc));
		return rc;
   	}

	/* Remove recover environment */
			
	if ((rc = db_env_create(&dbenv, 0)) != 0) {
       		LOG(log_error, logtype_default, "cnid_recover: db_env_create: %s", db_strerror(rc));
		return rc;
  	}

	if ((rc = dbenv->remove(dbenv, path, DB_FORCE)) != 0) {
		LOG(log_error, logtype_default, "cnid_recover: db_env_remove: %s", db_strerror(rc));
		return rc;
	}

	return 0;
}


static cnid_t cnid_rebuild (CNID_private *db, mode_t mask)
{

#ifdef CNID_DB_CDB
	DB *tmpdb;
	DBC *cursor;
	DBT key, data, altkey, altdata, dupkey, dupdata;
	int rc;
	cnid_t id, dupid, maxcnid = CNID_START;
	u_int32_t countp;
       	u_int32_t version;

	static char buffer[MAXPATHLEN + CNID_HEADER_LEN + 1];
       	LOG(log_info, logtype_default, "starting rebuild of databases");

    	if ((rc = db->db_cnid->cursor(db->db_cnid, NULL, &cursor, DB_WRITECURSOR) ) != 0) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Unable to get a cursor: %s", db_strerror(rc));
	        return CNID_INVALID;
    	}

	memset(&key, 0, sizeof(key));
    	memset(&data, 0, sizeof(data));
	memset(&altkey, 0, sizeof(key));
    	memset(&altdata, 0, sizeof(data));
    	memset(&dupdata, 0, sizeof(dupdata));
    	memset(&dupkey, 0, sizeof(dupkey));

	/* close didname and devino, then recreate them */
	if (db->db_didname) db->db_didname->close(db->db_didname, 0);
	if (db->db_devino) db->db_devino->close(db->db_devino, 0);

    	if ((rc = db_create(&db->db_didname, db->dbenv, 0)) != 0) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to recreate did/name database: %s",
            		db_strerror(rc));
        	goto abort;
    	}

    	if ((rc = my_open(db->db_didname, DBDIDNAME, NULL, DB_HASH, DB_CREATE|DB_TRUNCATE, 0666 & ~mask))) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to open did/name database: %s",
            		db_strerror(rc));
        	goto abort;
    	}

    	if ((rc = db_create(&db->db_devino, db->dbenv, 0)) != 0) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to recreate dev/ino database: %s",
            		db_strerror(rc));
        	goto abort;
    	}

    	if ((rc = my_open(db->db_devino, DBDEVINO, NULL, DB_HASH, DB_CREATE|DB_TRUNCATE, 0666 & ~mask))) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to open dev/ino database: %s",
            		db_strerror(rc));
        	goto abort;
    	}

	db->db_devino->sync( db->db_devino, 0);
	db->db_didname->sync( db->db_didname, 0);


	/* now create the temporary cnid database */
	if ((rc = db_create(&tmpdb, db->dbenv, 0)) != 0) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to create tmp database: %s",
            		db_strerror(rc));
        	goto abort;
    	}

    	if ((rc = my_open(tmpdb, DBRECOVERFILE, NULL, DB_HASH, DB_CREATE|DB_TRUNCATE, 0666 & ~mask))) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to open recover database: %s",
            		db_strerror(rc));
        	goto abort;
    	}

	/* add rootinfo and version to didname */
	id = 0;
    	key.data = ROOTINFO_KEY;
    	key.size = ROOTINFO_KEYLEN;
    	data.data = &id;
    	data.size = sizeof(id);

	if ((rc = db->db_didname->put(db->db_didname, NULL, &key, &data, 0))) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Error adding ROOTINFO: %s", db_strerror(rc));
	        goto abort;
    	}

    	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
    	key.data = DBVERSION_KEY;
    	key.size = DBVERSION_KEYLEN;
       	version = htonl(DBVERSION);
       	data.data = &version;
       	data.size = sizeof(version);

       	if ((rc = db->db_didname->put(db->db_didname, NULL, &key, &data, 0))) {
            	LOG(log_error, logtype_default, "cnid_rebuild: Error putting version: %s",
                	db_strerror(rc));
            	goto abort;
        }

	/* now recover the databases from cnid.db*/

	memset(&key, 0, sizeof(key));
    	memset(&data, 0, sizeof(data));
	data.data = buffer;
	data.ulen = MAXPATHLEN + CNID_HEADER_LEN +1;
	data.flags = DB_DBT_USERMEM;
	countp = 0;

	while ((rc = cursor->c_get(cursor, &key, &data, DB_NEXT)) == 0) {
		memcpy ( &id, key.data, sizeof(id));
		id = ntohl (id);
		if ( id < CNID_START ) {
            		LOG(log_error, logtype_default, "cnid_rebuild: Dropping invalid entry with id: %u", id);
			continue;
		}
			
		maxcnid = MAX( id, maxcnid);

		/* check for duplicate cnid */

		if ((rc = tmpdb->get(tmpdb, NULL, &key, &dupdata, 0))) {
			if ( rc != DB_NOTFOUND ) {
            			LOG(log_error, logtype_default, "cnid_rebuild: Error getting entry from cnid: %s",
                			db_strerror(rc));
				goto abort;
			}
		}
		else /* duplicate cnid */
		{
			LOG(log_error, logtype_default, "cnid_rebuild: Dropping duplicate file entry: %u", id);
			continue;
		}

		
	   	/* dev/ino database */
    		altkey.data = data.data;
    		altkey.size = CNID_DEVINO_LEN;
    		altdata.data = key.data;
    		altdata.size = key.size;
    		if ((rc = db->db_devino->put(db->db_devino, NULL, &altkey, &altdata, DB_NOOVERWRITE))) {
			if ( rc != DB_KEYEXIST) {
	            		LOG(log_error, logtype_default, "cnid_rebuild: Error adding entry %u to dev/ino: %s",
                		id, db_strerror(rc));
				goto abort;
			}

			/* handle duplicate (file with 2 ids) */

			dupkey.data = data.data;
			dupkey.size = CNID_DEVINO_LEN;
			if (( rc = db->db_devino->get(db->db_devino, NULL, &dupkey, &dupdata, 0))) {
            			LOG(log_error, logtype_default, "cnid_rebuild: Error getting entry from did/name: %s",
                			db_strerror(rc));
				goto abort;
			}
			memcpy ( &dupid, dupdata.data, sizeof(id));
			dupid = ntohl (dupid);
			LOG(log_error, logtype_default, "cnid_rebuild: Dropping duplicate file entry: %u", MIN(id,dupid));
			
			/* Use the entry with a higher cnid */
			if ( id < dupid)
				continue;	
			else
				if ((rc = db->db_devino->put(db->db_devino, NULL, &altkey, &altdata, 0))) {
            				LOG(log_error, logtype_default, "cnid_rebuild: Error adding entry %u to dev/ino: %s",
                			id, db_strerror(rc));
		            		goto abort;
				}
    		}

    		/* did/name database */
    		altkey.data = (char *) data.data + CNID_DEVINO_LEN;
    		altkey.size = data.size - CNID_DEVINO_LEN;
    		if ((rc = db->db_didname->put(db->db_didname, NULL, &altkey, &altdata, 0))) {
            		LOG(log_error, logtype_default, "cnid_rebuild: Error adding entry to did/name: %s",
                		db_strerror(rc));
			goto abort;
    		}

		/* recover database */

    		if ((rc = tmpdb->put(tmpdb, NULL, &key, &data, 0))) {
            		LOG(log_error, logtype_default, "cnid_rebuild: Error adding entry to recoverdb: %s",
                		db_strerror(rc));
            		goto abort;
    		}
		countp++;
	}

	/* set ROOTINFO to maxcnid */
	maxcnid = htonl(maxcnid);
    	key.data = ROOTINFO_KEY;
    	key.size = ROOTINFO_KEYLEN;
    	data.data = &maxcnid;
    	data.size = sizeof(maxcnid);

	if ((rc = db->db_didname->put(db->db_didname, NULL, &key, &data, 0))) {
		LOG (log_error, logtype_default, "cnid_rebuild: Failed to update ROOTINFO %s", db_strerror(rc));
		goto abort;
    	}

	/* delete cnid.db and rename cniddb.recover to cnid.db */

	cursor->c_close(cursor);
	db->db_cnid->close(db->db_cnid, 0);

    	if ((rc = db_create(&db->db_cnid, db->dbenv, 0)) != 0) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to create database: %s", db_strerror(rc));
        	goto fail;
    	}
	if ((rc = db->db_cnid->remove(db->db_cnid, DBCNID, NULL, 0)) != 0) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to remove database: %s", db_strerror(rc));
		db->db_cnid = NULL;
        	goto fail;
    	}

	if ((rc = tmpdb->close(tmpdb, 0)) != 0) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to close database: %s", db_strerror(rc));
        	goto fail;
    	}
   	if ((rc = db_create(&tmpdb, db->dbenv, 0)) != 0) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to create database: %s", db_strerror(rc));
        	goto fail;
    	}
	if ((rc = tmpdb->rename(tmpdb, DBRECOVERFILE, NULL, DBCNID, 0)) != 0) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Rename database failed: %s", db_strerror(rc));
        	goto fail;
    	}

    	if ((rc = db_create(&db->db_cnid, db->dbenv, 0)) != 0) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to recreate cnid database: %s", db_strerror(rc));
        	goto fail;
    	}

    	if ((rc = my_open(db->db_cnid, DBCNID, NULL, DB_HASH, 0, 0666 & ~mask))) {
        	LOG(log_error, logtype_default, "cnid_rebuild: Failed to open cnid database: %s", db_strerror(rc));
        	goto fail;
    	}

	db->db_devino->sync( db->db_devino, 0);
	db->db_didname->sync( db->db_didname, 0);
	db->db_cnid->sync( db->db_cnid, 0);
	
	LOG (log_info, logtype_default, "cnid_rebuild: Recovered %u entries, database rebuild complete", countp);
	return 1;

abort:
	cursor->c_close(cursor);
fail:
	LOG (log_error, logtype_default, "cnid_rebuild: Database rebuild failed");
	return CNID_INVALID;
#else
	return CNID_INVALID;
#endif

}

#endif /* CNID_DB */
