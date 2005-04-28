/*
 * $Id: dbd_dbcheck.c,v 1.2 2005-04-28 20:49:48 bfernhomberg Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */


#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <errno.h>
#include <netatalk/endian.h>
#include <atalk/logger.h>
#include <atalk/cnid_dbd_private.h>

#include "pack.h"
#include "dbif.h"
#include "dbd.h"

#ifndef CNID_BACKEND_DBD_TXN
int dbd_check(char *dbdir)
{
    u_int32_t c_didname = 0, c_devino = 0, c_cnid = 0;
#if 0
    char dbdir[MAXPATHLEN];

    if (NULL == getcwd(dbdir, sizeof(dbdir)) )
        return -1;
#endif

    LOG(log_debug, logtype_cnid, "CNID database at `%s' is being checked (quick)", dbdir);

    if (dbif_count(DBIF_IDX_CNID, &c_cnid)) 
        return -1;

    if (dbif_count(DBIF_IDX_DEVINO, &c_devino))
        return -1;

    /* bailout after the first error */
    if ( c_cnid != c_devino) {
        LOG(log_error, logtype_cnid, "CNID database at `%s' corrupted (%u/%u)", dbdir, c_cnid, c_devino);
        return 1;
    }

    if (dbif_count(DBIF_IDX_DIDNAME, &c_didname)) 
        return -1;
    
    if ( c_cnid != c_didname) {
        LOG(log_error, logtype_cnid, "CNID database at `%s' corrupted (%u/%u)", dbdir, c_cnid, c_didname);
        return 1;
    }

    LOG(log_debug, logtype_cnid, "CNID database at `%s' seems ok, %u entries.", dbdir, c_cnid);
    return 0;  
}

#endif

