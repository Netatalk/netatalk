/*
 * $Id: dbif.h,v 1.2 2005-04-28 20:49:48 bfernhomberg Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
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

extern int        dbif_stamp  __P((void *, int));
extern int        dbif_env_init  __P((struct db_param *));
extern int        dbif_open  __P((struct db_param *, int));
extern int        dbif_close __P((void));
extern int        dbif_closedb __P((void));
extern int        dbif_get __P((const int, DBT *, DBT *, u_int32_t));
extern int        dbif_pget __P((const int, DBT *, DBT *, DBT *, u_int32_t));
extern int        dbif_put __P((const int, DBT *, DBT *, u_int32_t));
extern int        dbif_del __P((const int, DBT *, u_int32_t));

extern int        dbif_count __P((const int, u_int32_t *));


#ifdef CNID_BACKEND_DBD_TXN
extern int        dbif_txn_begin  __P((void));
extern int        dbif_txn_commit  __P((void));
extern int        dbif_txn_abort  __P((void));
extern int        dbif_txn_checkpoint  __P((u_int32_t, u_int32_t, u_int32_t));
#else
extern int        dbif_sync  __P((void));
#endif /* CNID_BACKEND_DBD_TXN */

#endif
