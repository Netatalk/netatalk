/*
 * Copyright (C) Joerg Lenneis 2003
 * Copyright (C) Frank Lahm 2010
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_DB_PARAM_H
#define CNID_DBD_DB_PARAM_H 1

#include <sys/param.h>

struct db_param {
    char *dir;
    int logfile_autoremove;
    int cachesize;              /* in KB */
    int maxlocks;
    int maxlockobjs;
    int flush_interval;
    int flush_frequency;
    char usock_file[MAXPATHLEN + 1];    
    int fd_table_size;
    int idle_timeout;
    int max_vols;
};

extern struct db_param *db_param_read  (char *);

#endif /* CNID_DBD_DB_PARAM_H */

