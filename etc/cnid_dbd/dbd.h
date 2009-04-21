/*
 * $Id: dbd.h,v 1.3 2009-04-21 08:55:44 franklahm Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_DBD_H
#define CNID_DBD_DBD_H 1

#include <atalk/cnid_dbd_private.h>

extern int      dbd_stamp __P((void));
extern int      dbd_add  __P((struct cnid_dbd_rqst *, struct cnid_dbd_rply *));
extern int      dbd_get  __P((struct cnid_dbd_rqst *, struct cnid_dbd_rply *));
extern int      dbd_resolve  __P((struct cnid_dbd_rqst *, struct cnid_dbd_rply *));
extern int      dbd_lookup  __P((struct cnid_dbd_rqst *, struct cnid_dbd_rply *));
extern int      dbd_update  __P((struct cnid_dbd_rqst *, struct cnid_dbd_rply *));
extern int      dbd_delete  __P((struct cnid_dbd_rqst *, struct cnid_dbd_rply *));
extern int      dbd_getstamp  __P((struct cnid_dbd_rqst *, struct cnid_dbd_rply *));
extern int      dbd_rebuild_add __P((struct cnid_dbd_rqst *, struct cnid_dbd_rply *));
extern int      dbd_check_indexes  __P((char *));

#endif /* CNID_DBD_DBD_H */
