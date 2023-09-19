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
#include <unistd.h>

#include <atalk/logger.h>
#include <atalk/errchk.h>
#include <atalk/locking.h>
#include <atalk/cnid.h>
#include <atalk/paths.h>

/***************************************************************************
 * structures and defines
 ***************************************************************************/

#define LOCKTABLE_PATH         "/tmp/netatalk-afp-locks.tdb"

/*
 * Struct for building the the main database of file locks.
 * vid + cnid build the primary key for database access.
 */
typedef struct afp_lock {
    /* Keys */
    uint32_t l_vid;
    cnid_t   l_cnid;
    uint64_t l_dev;
    uint64_t l_ino;

    /* pid holding the lock, also secondary key */
    pid_t    l_pid;

    /* Refcounting access and deny modes */
    uint16_t l_amode_r;
    uint16_t l_amode_w;
    uint16_t l_dmode_r;
    uint16_t l_dmode_w;
} afp_lock_t;

#define PACKED_AFP_LOCK_SIZE                 \
    sizeof(((afp_lock_t *)0)->l_vid) +       \
    sizeof(((afp_lock_t *)0)->l_cnid) +      \
    sizeof(((afp_lock_t *)0)->l_dev) +       \
    sizeof(((afp_lock_t *)0)->l_ino) +       \
    sizeof(((afp_lock_t *)0)->l_pid) +       \
    sizeof(((afp_lock_t *)0)->l_amode_r) +   \
    sizeof(((afp_lock_t *)0)->l_amode_w) +   \
    sizeof(((afp_lock_t *)0)->l_dmode_r) +   \
    sizeof(((afp_lock_t *)0)->l_dmode_r)

/***************************************************************************
 * Data
 ***************************************************************************/

static struct smbdb_ctx *tdb;

/***************************************************************************
 * Private functions
 ***************************************************************************/

/***************************************************************************
 * Public functions
 ***************************************************************************/

/*!
 * Open locktable from path
 */
int locktable_init(const char *path)
{
    EC_INIT;

//    EC_NULL_LOG(tdb = smb_share_mode_db_open(path));

EC_CLEANUP:
    EC_EXIT;
}

