/*
 * Copyright (C) Joerg Lenneis 2003
 * Copyright (C) Frank Lahm 2009, 2010
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_DBD_H
#define CNID_DBD_DBD_H 1

#include <arpa/inet.h>

#include <atalk/cnid_bdb_private.h>

#include "dbif.h"

int add_cnid(DBD *dbd, struct cnid_dbd_rqst *rqst, struct cnid_dbd_rply *rply);
int get_cnid(DBD *dbd, struct cnid_dbd_rply *rply);

int dbd_add(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
int dbd_lookup(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
int dbd_get(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
int dbd_resolve(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
int dbd_update(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
int dbd_delete(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *,
               int idx);
int dbd_getstamp(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
int dbd_rebuild_add(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
int dbd_search(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
int dbd_check_indexes(DBD *dbd, char *);

#endif /* CNID_DBD_DBD_H */
