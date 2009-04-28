/*
 * $Id: dbif.h,v 1.4 2009-04-28 13:01:24 franklahm Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * Copyright (C) Frank Lahm 2009
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_DBIF_H
#define CNID_DBD_DBIF_H 1

#include <sys/cdefs.h>
#include <db.h>
#include "db_param.h"

#define DBIF_DB_CNT 3

#define DBIF_IDX_CNID      0
#define DBIF_IDX_DEVINO    1
#define DBIF_IDX_DIDNAME   2

extern int dbif_stamp(void *, int);
extern int dbif_env_init(struct db_param *, uint32_t);
extern int dbif_open(struct db_param *, int);
extern int dbif_close(void);
extern int dbif_closedb(void);
extern int dbif_get(const int, DBT *, DBT *, u_int32_t);
extern int dbif_pget(const int, DBT *, DBT *, DBT *, u_int32_t);
extern int dbif_put(const int, DBT *, DBT *, u_int32_t);
extern int dbif_del(const int, DBT *, u_int32_t);

extern int dbif_count(const int, u_int32_t *);

extern int dbif_txn_begin(void);
extern int dbif_txn_commit(void);
extern int dbif_txn_abort(void);
extern int dbif_txn_checkpoint(u_int32_t, u_int32_t, u_int32_t);

extern int dbif_dump(int dumpindexes);
#endif
