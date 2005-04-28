/*
 * $Id: db_param.h,v 1.2 2005-04-28 20:49:47 bfernhomberg Exp $
 *
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_DB_PARAM_H
#define CNID_DBD_DB_PARAM_H 1

#include <sys/param.h>
#include <sys/cdefs.h>


struct db_param {
    int check;
    int logfile_autoremove;
    int cachesize;
    int nosync;
    int flush_frequency;
    int flush_interval;
    char usock_file[MAXPATHLEN + 1];    
    int fd_table_size;
    int idle_timeout;
};

extern struct db_param *      db_param_read  __P((char *));


#endif /* CNID_DBD_DB_PARAM_H */

