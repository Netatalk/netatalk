/*
 * $Id: pack.c,v 1.5 2009-05-04 09:09:43 franklahm Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <netatalk/endian.h>

#include <string.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#include <sys/param.h>
#include <sys/cdefs.h>
#include <db.h>

#include <atalk/cnid_dbd_private.h>
#include "pack.h"

/* --------------- */
/*
 *  This implementation is portable, but could probably be faster by using htonl
 *  where appropriate. Also, this again doubles code from the cdb backend.
 */
static void pack_devino(unsigned char *buf, dev_t dev, ino_t ino)
{
    buf[CNID_DEV_LEN - 1] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 2] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 3] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 4] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 5] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 6] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 7] = dev; dev >>= 8;
    buf[CNID_DEV_LEN - 8] = dev;

    buf[CNID_DEV_LEN + CNID_INO_LEN - 1] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 2] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 3] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 4] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 5] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 6] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 7] = ino; ino >>= 8;
    buf[CNID_DEV_LEN + CNID_INO_LEN - 8] = ino;    
}

/* --------------- */
int didname(dbp, pkey, pdata, skey)
DB *dbp _U_;
const DBT *pkey _U_, *pdata;
DBT *skey;
{
int len;
 
    memset(skey, 0, sizeof(DBT));
    skey->data = (char *)pdata->data + CNID_DID_OFS;
    /* FIXME: At least DB 4.0.14 and 4.1.25 pass in the correct length of
       pdata.size. strlen is therefore not needed. Also, skey should be zeroed
       out already. */
    len = strlen((char *)skey->data + CNID_DID_LEN);
    skey->size = CNID_DID_LEN + len + 1;
    return (0);
}
 
/* --------------- */
int devino(dbp, pkey, pdata, skey)
DB *dbp _U_;
const DBT *pkey _U_, *pdata;
DBT *skey;
{
    memset(skey, 0, sizeof(DBT));
    skey->data = (char *)pdata->data + CNID_DEVINO_OFS;
    skey->size = CNID_DEVINO_LEN;
    return (0);
}

/* The equivalent to make_cnid_data in the cnid library. Non re-entrant. We
   differ from make_cnid_data in that we never return NULL, rqst->name cannot
   ever cause start[] to overflow because name length is checked in libatalk. */

unsigned char *pack_cnid_data(struct cnid_dbd_rqst *rqst)
{
    static unsigned char start[CNID_HEADER_LEN + MAXPATHLEN + 1];
    unsigned char *buf = start +CNID_LEN;
    u_int32_t i;

    pack_devino(buf, rqst->dev, rqst->ino);
    buf += CNID_DEVINO_LEN;

    i = htonl(rqst->type);
    memcpy(buf, &i, sizeof(i));
    buf += sizeof(i);

    /* did is already in network byte order */
    buf = memcpy(buf, &rqst->did, sizeof(rqst->did));
    buf += sizeof(rqst->did);
    buf = memcpy(buf, rqst->name, rqst->namelen);
    *(buf + rqst->namelen) = '\0';

    return start;
}

