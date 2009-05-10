/*
 * $Id: dbif.c,v 1.9 2009-05-10 08:08:28 franklahm Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * Copyright (C) Frank Lahm 2009
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/cdefs.h>
#include <unistd.h>
#include <atalk/logger.h>
#include <db.h>
#include "db_param.h"
#include "dbif.h"
#include "pack.h"

#define DB_ERRLOGFILE "db_errlog"

static char *old_dbfiles[] = {"cnid.db", NULL};

/* --------------- */
static int upgrade_required()
{
    int i;
    int found = 0;
    struct stat st;

    for (i = 0; old_dbfiles[i] != NULL; i++) {
        if ( !(stat(old_dbfiles[i], &st) < 0) ) {
            found++;
            continue;
        }
        if (errno != ENOENT) {
            LOG(log_error, logtype_cnid, "cnid_open: Checking %s gave %s", old_dbfiles[i], strerror(errno));
            found++;
        }
    }
    return found;
}

/* --------------- */
int dbif_stamp(DBD *dbd, void *buffer, int size)
{
    struct stat st;
    int         rc;

    if (size < 8)
        return -1;

    if ((rc = stat(dbd->db_table[DBIF_CNID].name, &st)) < 0) {
        LOG(log_error, logtype_cnid, "error stating database %s: %s", dbd->db_table[DBIF_CNID].name, db_strerror(rc));
        return -1;
    }
    memset(buffer, 0, size);
    memcpy(buffer, &st.st_ctime, sizeof(st.st_ctime));

    return 0;
}

/* --------------- */
DBD *dbif_init(const char *filename)
{
    DBD *dbd;

    if ( NULL == (dbd = calloc(sizeof(DBD), 1)) )
        return NULL;

    /* filename == NULL means in memory db */
    if (filename) {
        dbd->db_filename = strdup(filename);
        if (NULL == dbd->db_filename) {
            free(dbd);
            return NULL;
        }
    }
    
    dbd->db_table[DBIF_CNID].name        = "cnid2.db";
    dbd->db_table[DBIF_IDX_DEVINO].name  = "devino.db";
    dbd->db_table[DBIF_IDX_DIDNAME].name = "didname.db";

    dbd->db_table[DBIF_CNID].type        = DB_BTREE;
    dbd->db_table[DBIF_IDX_DEVINO].type  = DB_BTREE;
    dbd->db_table[DBIF_IDX_DIDNAME].type = DB_BTREE;

    dbd->db_table[DBIF_CNID].openflags        = DB_CREATE;
    dbd->db_table[DBIF_IDX_DEVINO].openflags  = DB_CREATE;
    dbd->db_table[DBIF_IDX_DIDNAME].openflags = DB_CREATE;

    return dbd;
}

/* --------------- */
/*
 *  We assume our current directory is already the BDB homedir. Otherwise
 *  opening the databases will not work as expected.
 */
int dbif_env_open(DBD *dbd, struct db_param *dbp, uint32_t dbenv_oflags)
{
    int ret;
    char **logfiles = NULL;
    char **file;

    /* Refuse to do anything if this is an old version of the CNID database */
    if (upgrade_required()) {
        LOG(log_error, logtype_cnid, "Found version 1 of the CNID database. Please upgrade to version 2");
        return -1;
    }

    if ((dbd->db_errlog = fopen(DB_ERRLOGFILE, "a")) == NULL)
        LOG(log_warning, logtype_cnid, "error creating/opening DB errlogfile: %s", strerror(errno));

    if ((ret = db_env_create(&dbd->db_env, 0))) {
        LOG(log_error, logtype_cnid, "error creating DB environment: %s",
            db_strerror(ret));
        dbd->db_env = NULL;
        return -1;
    }

    if (dbd->db_errlog != NULL) {
        dbd->db_env->set_errfile(dbd->db_env, dbd->db_errlog);
        dbd->db_env->set_msgfile(dbd->db_env, dbd->db_errlog);
    }

    dbd->db_env->set_verbose(dbd->db_env, DB_VERB_RECOVERY, 1);

    if (dbenv_oflags & DB_RECOVER) {
        /* Open the database for recovery using DB_PRIVATE option which is faster */
        if ((ret = dbd->db_env->open(dbd->db_env, ".", dbenv_oflags | DB_PRIVATE, 0))) {
            LOG(log_error, logtype_cnid, "error opening DB environment: %s",
                db_strerror(ret));
            dbd->db_env->close(dbd->db_env, 0);
            dbd->db_env = NULL;
            return -1;
        }
        dbenv_oflags = (dbenv_oflags & ~DB_RECOVER);

        if (dbd->db_errlog != NULL)
            fflush(dbd->db_errlog);

        if ((ret = dbd->db_env->close(dbd->db_env, 0))) {
            LOG(log_error, logtype_cnid, "error closing DB environment after recovery: %s",
                db_strerror(ret));
            dbd->db_env = NULL;
            return -1;
        }

        if ((ret = db_env_create(&dbd->db_env, 0))) {
            LOG(log_error, logtype_cnid, "error creating DB environment after recovery: %s",
                db_strerror(ret));
            dbd->db_env = NULL;
            return -1;
        }
    }

    if ((ret = dbd->db_env->set_cachesize(dbd->db_env, 0, 1024 * dbp->cachesize, 0))) {
        LOG(log_error, logtype_cnid, "error setting DB environment cachesize to %i: %s",
            dbp->cachesize, db_strerror(ret));
        dbd->db_env->close(dbd->db_env, 0);
        dbd->db_env = NULL;
        return -1;
    }

    if (dbd->db_errlog != NULL) {
        dbd->db_env->set_errfile(dbd->db_env, dbd->db_errlog);
        dbd->db_env->set_msgfile(dbd->db_env, dbd->db_errlog);
    }
    if ((ret = dbd->db_env->open(dbd->db_env, ".", dbenv_oflags, 0))) {
        LOG(log_error, logtype_cnid, "error opening DB environment after recovery: %s",
            db_strerror(ret));
        dbd->db_env->close(dbd->db_env, 0);
        dbd->db_env = NULL;
        return -1;
    }

    if ((ret = dbd->db_env->set_flags(dbd->db_env, DB_AUTO_COMMIT, 1))) {
        LOG(log_error, logtype_cnid, "error setting DB_AUTO_COMMIT flag: %s",
            db_strerror(ret));
        dbd->db_env->close(dbd->db_env, 0);
        dbd->db_env = NULL;
        return -1;
    }

    if (dbp->logfile_autoremove) {
        if (dbd->db_env->log_archive(dbd->db_env, &logfiles, 0)) {
            LOG(log_error, logtype_cnid, "error getting list of stale logfiles: %s",
                db_strerror(ret));
            dbd->db_env->close(dbd->db_env, 0);
            dbd->db_env = NULL;
            return -1;
        }
        if (logfiles != NULL) {
            for (file = logfiles; *file != NULL; file++) {
                if (unlink(*file) < 0)
                    LOG(log_warning, logtype_cnid, "Error removing stale logfile %s: %s", *file, strerror(errno));
            }
            free(logfiles);
        }

#if DB_VERSION_MAJOR > 4 || (DB_VERSION_MAJOR == 4 && DB_VERSION_MINOR >= 7)
        if ((ret = dbd->db_env->log_set_config(dbd->db_env, DB_LOG_AUTO_REMOVE, 1))) {
            LOG(log_error, logtype_cnid, "error setting DB_LOG_AUTO_REMOVE flag: %s",
            db_strerror(ret));
            dbd->db_env->close(dbd->db_env, 0);
            dbd->db_env = NULL;
            return -1;
        }
#else
        if ((ret = dbd->db_env->set_flags(dbd->db_env, DB_LOG_AUTOREMOVE, 1))) {
            LOG(log_error, logtype_cnid, "error setting DB_LOG_AUTOREMOVE flag: %s",
                db_strerror(ret));
            dbd->db_env->close(dbd->db_env, 0);
            dbd->db_env = NULL;
            return -1;
        }
#endif
    }

    return 0;
}

/* --------------- */
int dbif_open(DBD *dbd, struct db_param *dbp _U_, int do_truncate)
{
    int ret;
    int i;
    u_int32_t count;

    for (i = 0; i != DBIF_DB_CNT; i++) {
        if ((ret = db_create(&dbd->db_table[i].db, dbd->db_env, 0))) {
            LOG(log_error, logtype_cnid, "error creating handle for database %s: %s",
                dbd->db_table[i].name, db_strerror(ret));
            return -1;
        }

        if (dbd->db_table[i].flags) {
            if ((ret = dbd->db_table[i].db->set_flags(dbd->db_table[i].db,
                                                      dbd->db_table[i].flags))) {
                LOG(log_error, logtype_cnid, "error setting flags for database %s: %s",
                    dbd->db_table[i].name, db_strerror(ret));
                return -1;
            }
        }

        if (dbd->db_table[i].db->open(dbd->db_table[i].db,
                                      dbd->db_txn,
                                      dbd->db_filename,
                                      dbd->db_table[i].name,
                                      dbd->db_table[i].type,
                                      dbd->db_table[i].openflags,
                                      0664) < 0) {
            LOG(log_error, logtype_cnid, "Cant open database");
            return -1;
        }
        if (dbd->db_errlog != NULL)
            dbd->db_table[i].db->set_errfile(dbd->db_table[i].db, dbd->db_errlog);

        if (do_truncate && i > 0) {
            LOG(log_info, logtype_cnid, "Truncating CNID index.");
            if ((ret = dbd->db_table[i].db->truncate(dbd->db_table[i].db, NULL, &count, 0))) {
                LOG(log_error, logtype_cnid, "error truncating database %s: %s",
                    dbd->db_table[i].name, db_strerror(ret));
                return -1;
            }
        }
    }

    /* TODO: Implement CNID DB versioning info on new databases. */

    /* Associate the secondary with the primary. */
    if (do_truncate)
        LOG(log_info, logtype_cnid, "Reindexing did/name index...");
    if ((ret = dbd->db_table[0].db->associate(dbd->db_table[DBIF_CNID].db,
                                              dbd->db_txn,
                                              dbd->db_table[DBIF_IDX_DIDNAME].db, 
                                              didname,
                                              (do_truncate) ? DB_CREATE : 0))
         != 0) {
        LOG(log_error, logtype_cnid, "Failed to associate didname database: %s",db_strerror(ret));
        return -1;
    }
    if (do_truncate)
        LOG(log_info, logtype_cnid, "... done.");

    if (do_truncate)
        LOG(log_info, logtype_cnid, "Reindexing dev/ino index...");
    if ((ret = dbd->db_table[0].db->associate(dbd->db_table[0].db, 
                                              dbd->db_txn,
                                              dbd->db_table[DBIF_IDX_DEVINO].db, 
                                              devino,
                                              (do_truncate) ? DB_CREATE : 0))
        != 0) {
        LOG(log_error, logtype_cnid, "Failed to associate devino database: %s",db_strerror(ret));
        return -1;
    }
    if (do_truncate)
        LOG(log_info, logtype_cnid, "... done.");
    
    return 0;
}

/* ------------------------ */
int dbif_closedb(DBD *dbd)
{
    int i;
    int ret;
    int err = 0;

    for (i = DBIF_DB_CNT -1; i >= 0; i--) {
        if (dbd->db_table[i].db != NULL && (ret = dbd->db_table[i].db->close(dbd->db_table[i].db, 0))) {
            LOG(log_error, logtype_cnid, "error closing database %s: %s", dbd->db_table[i].name, db_strerror(ret));
            err++;
        }
    }
    if (err)
        return -1;
    return 0;
}

/* ------------------------ */
int dbif_close(DBD *dbd)
{
    int ret;
    int err = 0;

    if (dbif_closedb(dbd))
        err++;

    if (dbd->db_env != NULL && (ret = dbd->db_env->close(dbd->db_env, 0))) {
        LOG(log_error, logtype_cnid, "error closing DB environment: %s", db_strerror(ret));
        err++;
    }
    if (dbd->db_errlog != NULL && fclose(dbd->db_errlog) == EOF) {
        LOG(log_error, logtype_cnid, "error closing DB logfile: %s", strerror(errno));
        err++;
    }

    free(dbd->db_filename);
    free(dbd);
    dbd = NULL;

    if (err)
        return -1;
    return 0;
}

/*
 *  The following three functions are wrappers for DB->get(), DB->put() and DB->del().
 *  All three return -1 on error. dbif_get()/dbif_del return 1 if the key was found and 0
 *  otherwise. dbif_put() returns 0 if key/val was successfully updated and 1 if
 *  the DB_NOOVERWRITE flag was specified and the key already exists.
 *
 *  All return codes other than DB_NOTFOUND and DB_KEYEXIST from the DB->()
 *  functions are not expected and therefore error conditions.
 */

int dbif_get(DBD *dbd, const int dbi, DBT *key, DBT *val, u_int32_t flags)
{
    int ret;

    ret = dbd->db_table[dbi].db->get(dbd->db_table[dbi].db,
                                     dbd->db_txn,
                                     key,
                                     val,
                                     flags);

    if (ret == DB_NOTFOUND)
        return 0;
    if (ret) {
        LOG(log_error, logtype_cnid, "error retrieving value from %s: %s",
            dbd->db_table[dbi].name, db_strerror(errno));
        return -1;
    } else
        return 1;
}

/* search by secondary return primary */
int dbif_pget(DBD *dbd, const int dbi, DBT *key, DBT *pkey, DBT *val, u_int32_t flags)
{
    int ret;

    ret = dbd->db_table[dbi].db->pget(dbd->db_table[dbi].db,
                                      dbd->db_txn,
                                      key,
                                      pkey,
                                      val,
                                      flags);

    if (ret == DB_NOTFOUND || ret == DB_SECONDARY_BAD) {
        return 0;
    }
    if (ret) {
        LOG(log_error, logtype_cnid, "error retrieving value from %s: %s",
            dbd->db_table[dbi].name, db_strerror(errno));
        return -1;
   } else
        return 1;
}

/* -------------------------- */
int dbif_put(DBD *dbd, const int dbi, DBT *key, DBT *val, u_int32_t flags)
{
    int ret;

    if (dbif_txn_begin(dbd) < 0) {
        LOG(log_error, logtype_cnid, "error setting key/value in %s: %s",
            dbd->db_table[dbi].name, db_strerror(errno));
        return -1;
    }

    ret = dbd->db_table[dbi].db->put(dbd->db_table[dbi].db,
                                     dbd->db_txn,
                                     key,
                                     val,
                                     flags);
    
    if (ret) {
        if ((flags & DB_NOOVERWRITE) && ret == DB_KEYEXIST) {
            return 1;
        } else {
            LOG(log_error, logtype_cnid, "error setting key/value in %s: %s",
                dbd->db_table[dbi].name, db_strerror(errno));
            return -1;
        }
    } else
        return 0;
}

int dbif_del(DBD *dbd, const int dbi, DBT *key, u_int32_t flags)
{
    int ret;

    if (dbif_txn_begin(dbd) < 0) {
        LOG(log_error, logtype_cnid, "error deleting key/value from %s: %s", 
            dbd->db_table[dbi].name, db_strerror(errno));
        return -1;
    }

    ret = dbd->db_table[dbi].db->del(dbd->db_table[dbi].db,
                                     dbd->db_txn,
                                     key,
                                     flags);
    
    if (ret == DB_NOTFOUND || ret == DB_SECONDARY_BAD)
        return 0;
    if (ret) {
        LOG(log_error, logtype_cnid, "error deleting key/value from %s: %s",
            dbd->db_table[dbi].name, db_strerror(errno));
        return -1;
    } else
        return 1;
}

int dbif_txn_begin(DBD *dbd)
{
    int ret;

    /* If we already have an active txn, just return */
    if (dbd->db_txn)
        return 0;

    /* If our DBD has no env, just return (-> in memory db) */
    if (dbd->db_env == NULL)
        return 0;

    ret = dbd->db_env->txn_begin(dbd->db_env, NULL, &dbd->db_txn, 0);

    if (ret) {
        LOG(log_error, logtype_cnid, "error starting transaction: %s", db_strerror(errno));
        return -1;
    } else
        return 0;
}

int dbif_txn_commit(DBD *dbd)
{
    int ret;

    if (! dbd->db_txn)
        return 0;

    /* If our DBD has no env, just return (-> in memory db) */
    if (dbd->db_env == NULL)
        return 0;

    ret = dbd->db_txn->commit(dbd->db_txn, 0);
    dbd->db_txn = NULL;
    
    if (ret) {
        LOG(log_error, logtype_cnid, "error committing transaction: %s", db_strerror(errno));
        return -1;
    } else
        return 1;
}

int dbif_txn_abort(DBD *dbd)
{
    int ret;

    if (! dbd->db_txn)
        return 0;

    /* If our DBD has no env, just return (-> in memory db) */
    if (dbd->db_env == NULL)
        return 0;

    ret = dbd->db_txn->abort(dbd->db_txn);
    dbd->db_txn = NULL;
    
    if (ret) {
        LOG(log_error, logtype_cnid, "error aborting transaction: %s", db_strerror(errno));
        return -1;
    } else
        return 0;
}

int dbif_txn_checkpoint(DBD *dbd, u_int32_t kbyte, u_int32_t min, u_int32_t flags)
{
    int ret;
    ret = dbd->db_env->txn_checkpoint(dbd->db_env, kbyte, min, flags);
    if (ret) {
        LOG(log_error, logtype_cnid, "error checkpointing transaction susystem: %s", db_strerror(errno));
        return -1;
    } else
        return 0;
}

int dbif_count(DBD *dbd, const int dbi, u_int32_t *count)
{
    int ret;
    DB_BTREE_STAT *sp;
    DB *db = dbd->db_table[dbi].db;

    ret = db->stat(db, NULL, &sp, 0);

    if (ret) {
        LOG(log_error, logtype_cnid, "error getting stat infotmation on database: %s", db_strerror(errno));
        return -1;
    }

    *count = sp->bt_ndata;
    free(sp);

    return 0;
}

int dbif_copy_rootinfokey(DBD *srcdbd, DBD *destdbd)
{
    DBT key, data;
    int rc;

    memset(&key, 0, sizeof(key));
    memset(&data, 0, sizeof(data));

    key.data = ROOTINFO_KEY;
    key.size = ROOTINFO_KEYLEN;

    if ((rc = dbif_get(srcdbd, DBIF_CNID, &key, &data, 0)) <= 0) {
        LOG(log_error, logtype_cnid, "dbif_copy_rootinfokey: Error getting rootinfo record");
        return -1;
    }

    memset(&key, 0, sizeof(key));
    key.data = ROOTINFO_KEY;
    key.size = ROOTINFO_KEYLEN;

    if ((rc = dbif_put(destdbd, DBIF_CNID, &key, &data, 0))) {
        LOG(log_error, logtype_cnid, "dbif_copy_rootinfokey: Error writing rootinfo key");
        return -1;
    }

    return 0;
}

int dbif_dump(DBD *dbd, int dumpindexes)
{
    int rc;
    uint32_t max = 0, count = 0, cnid, type, did, lastid;
    uint64_t dev, ino;
    time_t stamp;
    DBC *cur;
    DBT key = { 0 }, data = { 0 };
    DB *db = dbd->db_table[DBIF_CNID].db;
    char *typestring[2] = {"f", "d"};
    char timebuf[64];

    printf("CNID database dump:\n");

    rc = db->cursor(db, NULL, &cur, 0);
    if (rc) {
        LOG(log_error, logtype_cnid, "Couldn't create cursor: %s", db_strerror(errno));
        return -1;
    }

    cur->c_get(cur, &key, &data, DB_FIRST);
    while (rc == 0) {
        /* Parse and print data */
        memcpy(&cnid, key.data, 4);
        cnid = ntohl(cnid);
        if (cnid > max)
            max = cnid;

        /* Rootinfo node ? */
        if (cnid == 0) {
            memcpy(&stamp, data.data + 4, sizeof(time_t));
            memcpy(&lastid, data.data + 20, sizeof(cnid_t));
            lastid = ntohl(lastid);
            strftime(timebuf, sizeof(timebuf), "%b %d %Y %H:%M:%S", localtime(&stamp));
            printf("dbd stamp: 0x%08x (%s), next free CNID: %u\n", (unsigned int)stamp, timebuf, lastid + 1);
        } else {
            /* dev */
            memcpy(&dev, data.data + CNID_DEV_OFS, 8);
            dev = ntoh64(dev);
            /* ino */
            memcpy(&ino, data.data + CNID_INO_OFS, 8);
            ino = ntoh64(ino);
            /* type */
            memcpy(&type, data.data + CNID_TYPE_OFS, 4);
            type = ntohl(type);
            /* did */
            memcpy(&did, data.data + CNID_DID_OFS, 4);
            did = ntohl(did);

            count++;
            printf("id: %10u, did: %10u, type: %s, dev: 0x%llx, ino: 0x%016llx, name: %s\n", 
                   cnid, did, typestring[type],
                   (long long unsigned int)dev, (long long unsigned int)ino, 
                   (char *)data.data + CNID_NAME_OFS);

        }

        rc = cur->c_get(cur, &key, &data, DB_NEXT);
    }

    if (rc != DB_NOTFOUND) {
        LOG(log_error, logtype_cnid, "Error iterating over btree: %s", db_strerror(errno));
        return -1;
    }

    rc = cur->c_close(cur);
    if (rc) {
        LOG(log_error, logtype_cnid, "Couldn't close cursor: %s", db_strerror(errno));
        return -1;
    }
    printf("%u CNIDs in database. Max CNID: %u.\n", count, max);

    /* Dump indexes too ? */
    if (dumpindexes) {
        /* DBIF_IDX_DEVINO */
        printf("\ndev/inode index:\n");
        count = 0;
        db = dbd->db_table[DBIF_IDX_DEVINO].db;
        rc = db->cursor(db, NULL, &cur, 0);
        if (rc) {
            LOG(log_error, logtype_cnid, "Couldn't create cursor: %s", db_strerror(errno));
            return -1;
        }
        
        cur->c_get(cur, &key, &data, DB_FIRST);
        while (rc == 0) {
            /* Parse and print data */

            /* cnid */
            memcpy(&cnid, data.data, CNID_LEN);
            cnid = ntohl(cnid);
            if (cnid == 0) {
                /* Rootinfo node */
            } else {
                /* dev */
                memcpy(&dev, key.data, CNID_DEV_LEN);
                dev = ntoh64(dev);
                /* ino */
                memcpy(&ino, key.data + CNID_DEV_LEN, CNID_INO_LEN);
                ino = ntoh64(ino);
                
                printf("id: %10u <== dev: 0x%llx, ino: 0x%016llx\n", 
                       cnid, (unsigned long long int)dev, (unsigned long long int)ino);
                count++;
            }
            rc = cur->c_get(cur, &key, &data, DB_NEXT);
        }
        if (rc != DB_NOTFOUND) {
            LOG(log_error, logtype_cnid, "Error iterating over btree: %s", db_strerror(errno));
            return -1;
        }
        
        rc = cur->c_close(cur);
        if (rc) {
            LOG(log_error, logtype_cnid, "Couldn't close cursor: %s", db_strerror(errno));
            return -1;
        }
        printf("%u items\n", count);

        /* Now dump DBIF_IDX_DIDNAME */
        printf("\ndid/name index:\n");
        count = 0;
        db = dbd->db_table[DBIF_IDX_DIDNAME].db;
        rc = db->cursor(db, NULL, &cur, 0);
        if (rc) {
            LOG(log_error, logtype_cnid, "Couldn't create cursor: %s", db_strerror(errno));
            return -1;
        }
        
        cur->c_get(cur, &key, &data, DB_FIRST);
        while (rc == 0) {
            /* Parse and print data */

            /* cnid */
            memcpy(&cnid, data.data, CNID_LEN);
            cnid = ntohl(cnid);
            if (cnid == 0) {
                /* Rootinfo node */
            } else {
                /* did */
                memcpy(&did, key.data, CNID_LEN);
                did = ntohl(did);

                printf("id: %10u <== did: %10u, name: %s\n", cnid, did, (char *)key.data + CNID_LEN);
                count++;
            }
            rc = cur->c_get(cur, &key, &data, DB_NEXT);
        }
        if (rc != DB_NOTFOUND) {
            LOG(log_error, logtype_cnid, "Error iterating over btree: %s", db_strerror(errno));
            return -1;
        }
        
        rc = cur->c_close(cur);
        if (rc) {
            LOG(log_error, logtype_cnid, "Couldn't close cursor: %s", db_strerror(errno));
            return -1;
        }
        printf("%u items\n", count);
    }

    return 0;
}

