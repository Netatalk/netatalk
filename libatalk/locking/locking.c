/*
 * Copyright (c) 2011 Frank Lahm
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <atalk/logger.h>
#include <atalk/errchk.h>
#include <atalk/locking.h>
#include <atalk/cnid.h>
#define AFP_LOCKTABLE_SIZE (8*1024*1024)

/* 
 * Struct for building the the main database of file locks.
 * vid + cnid build the primary key for database access.
 */
struct afp_lock {
    /* Keys */
    uint32_t vid;
    cnid_t cnid;

    /* Refcounting access and deny modes */
    uint16_t amode_r;
    uint16_t amode_w;
    uint16_t dmode_r;
    uint16_t dmode_w;
};

/* 
 * Structure for building a table which provides the way to find locks by pid
 */
struct pid_lock {
    /* Key */
    pid_t pid;

    /* Key for afp_lock */
    uint32_t vid;
    cnid_t cnid;
};

/***************************************************************************
 * Public functios
 ***************************************************************************/

int locktable_init(void)
{
    EC_INIT;


EC_CLEANUP:
    EC_EXIT;
}

