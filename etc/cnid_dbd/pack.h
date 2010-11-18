/*
 * Copyright (C) Joerg Lenneis 2003
 * Copyright (C) Frank Lahm 2010
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_PACK_H
#define CNID_DBD_PACK_H 1

#include <db.h>
#include <atalk/cnid_dbd_private.h>

#define ntoh64(x)       (((uint64_t)(x) << 56) | \
                        (((uint64_t)(x) << 40) & 0xff000000000000ULL) | \
                        (((uint64_t)(x) << 24) & 0xff0000000000ULL) | \
                        (((uint64_t)(x) << 8)  & 0xff00000000ULL) | \
                        (((uint64_t)(x) >> 8)  & 0xff000000ULL) | \
                        (((uint64_t)(x) >> 24) & 0xff0000ULL) | \
                        (((uint64_t)(x) >> 40) & 0xff00ULL) | \
                        ((uint64_t)(x)  >> 56))

extern unsigned char *pack_cnid_data(struct cnid_dbd_rqst *);
extern int didname(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey);
extern int devino(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey);
extern int idxname(DB *dbp, const DBT *pkey, const DBT *pdata, DBT *skey);

#endif /* CNID_DBD_PACK_H */
