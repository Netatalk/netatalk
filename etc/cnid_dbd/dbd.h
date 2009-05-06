/*
 * $Id: dbd.h,v 1.4 2009-05-06 11:54:24 franklahm Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * Copyright (C) Frank Lahm 2009
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_DBD_H
#define CNID_DBD_DBD_H 1

#include <atalk/cnid_dbd_private.h>

extern int dbd_stamp(DBD *dbd);
extern int dbd_add(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
extern int dbd_get(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
extern int dbd_resolve(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
extern int dbd_lookup(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
extern int dbd_update(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
extern int dbd_delete(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
extern int dbd_getstamp(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
extern int dbd_rebuild_add(DBD *dbd, struct cnid_dbd_rqst *, struct cnid_dbd_rply *);
extern int dbd_check_indexes(DBD *dbd, char *);

#endif /* CNID_DBD_DBD_H */
