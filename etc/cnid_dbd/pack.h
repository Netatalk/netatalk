/*
 * $Id: pack.h,v 1.2 2005-04-28 20:49:49 bfernhomberg Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_PACK_H
#define CNID_DBD_PACK_H 1


#include <atalk/cnid_dbd_private.h>

#define CNID_OFS                 0
#define CNID_LEN                 4
 
#define CNID_DEV_OFS             CNID_LEN
#define CNID_DEV_LEN             8
  
#define CNID_INO_OFS             (CNID_DEV_OFS + CNID_DEV_LEN)
#define CNID_INO_LEN             8
   
#define CNID_DEVINO_OFS          CNID_LEN
#define CNID_DEVINO_LEN          (CNID_DEV_LEN +CNID_INO_LEN)
    
#define CNID_TYPE_OFS            (CNID_DEVINO_OFS +CNID_DEVINO_LEN)
#define CNID_TYPE_LEN            4
     
#define CNID_DID_OFS             (CNID_TYPE_OFS +CNID_TYPE_LEN)
#define CNID_DID_LEN             CNID_LEN
      
#define CNID_NAME_OFS            (CNID_DID_OFS + CNID_DID_LEN)
#define CNID_HEADER_LEN          (CNID_NAME_OFS)

#if 0
#define CNID_DBD_DEVINO_LEN          8
#define CNID_DBD_DID_LEN             4
#define CNID_DBD_HEADER_LEN          (CNID_DBD_DEVINO_LEN + CNID_DBD_DID_LEN)
#endif

extern char      *pack_cnid_data  __P((struct cnid_dbd_rqst *));

#ifdef DEBUG
extern char      *stringify_devino  __P((dev_t dev, ino_t ino));
#endif

#endif /* CNID_DBD_PACK_H */
