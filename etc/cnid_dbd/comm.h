/*
 * Copyright (C) Joerg Lenneis 2003
 * All Rights Reserved.  See COPYING.
 */

#ifndef CNID_DBD_COMM_H
#define CNID_DBD_COMM_H 1

/* number of seconds to try reading in readt */
#define CNID_DBD_TIMEOUT 1

#include <atalk/cnid_dbd_private.h>


int      comm_init  (struct db_param *, int, int);
int      comm_rcv  (struct cnid_dbd_rqst *,  time_t, const sigset_t *, time_t *);
int      comm_snd  (struct cnid_dbd_rply *);
int      comm_nbe  (void);

#endif /* CNID_DBD_COMM_H */

