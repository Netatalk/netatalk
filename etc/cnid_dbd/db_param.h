/*
 * $Id: db_param.h,v 1.5 2009-10-13 22:55:37 didg Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_DB_PARAM_H
#define CNID_DBD_DB_PARAM_H 1

#include <sys/param.h>
#include <sys/cdefs.h>

enum identity {
    METAD,
    CNID_DBD
};

struct db_param {
    char *dir;
    int logfile_autoremove;
    int cachesize;
    int flush_interval;
    int flush_frequency;
    char usock_file[MAXPATHLEN + 1];    
    int fd_table_size;
    int idle_timeout;
    int max_vols;
};

extern struct db_param *      db_param_read  (char *, enum identity);


#endif /* CNID_DBD_DB_PARAM_H */

