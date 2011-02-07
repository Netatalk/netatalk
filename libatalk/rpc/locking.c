/*
 * Copyright (c) 2011 Frank Lahm
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <sys/queue.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "event2/event-config.h"

#include "event2/event.h"
#include "event2/http.h"
#include "event2/event_compat.h"
#include "event2/http_compat.h"
#include "event2/rpc.h"
#include "event2/rpc_struct.h"

#include <atalk/logger.h>
#include <atalk/errchk.h>
#include <atalk/locking.h>
#include <atalk/adouble.h>
#include <atalk/bstrlib.h>
#include <atalk/bstradd.h>
#include <atalk/lockrpc.gen.h>

EVRPC_HEADER(lock_msg, lock_req, lock_rep)
EVRPC_GENERATE(lock_msg, lock_req, lock_rep)

static struct evrpc_pool *rpc_pool;
static struct event_base *ev_base;

static void ev_log_cb(int severity, const char *msg)
{
    LOG(log_warning, logtype_default, (char *)msg);
}

static void msg_rep_cb(struct evrpc_status *status,
                       struct lock_req *req,
                       struct lock_rep *rep,
                       void *arg)
{
	if (status->error != EVRPC_STATUS_ERR_NONE)
		goto done;

done:
    event_base_loopexit(ev_base, NULL);
}

static int rpc_dummy(const char *name)
{
    struct lock *lock = NULL;
	struct lock_req *lock_req = NULL;
	struct lock_rep *lock_rep = NULL;
    
    lock = lock_new();
    lock_req = lock_req_new();
    lock_rep = lock_rep_new();

    EVTAG_ASSIGN(lock_req, req_lock, lock);
    EVTAG_ASSIGN(lock_req, req_filename, name);

    EVRPC_MAKE_REQUEST(lock_msg, rpc_pool, lock_req, lock_rep, msg_rep_cb, NULL);

    event_dispatch();
}

int rpc_lock(struct adouble *ad, uint32_t eid, int type, off_t off, off_t len, int user)
{
    rpc_dummy(cfrombstr(ad->ad_fullpath));
    return 0;
}

void rpc_unlock(struct adouble *ad, int user)
{
    rpc_dummy(cfrombstr(ad->ad_fullpath));
}

int rpc_tmplock(struct adouble *ad, uint32_t eid, int type, off_t off, off_t len, int user)
{
    rpc_dummy(cfrombstr(ad->ad_fullpath));
    return 0;
}

/**************************************************************************************
 * Public functions
 **************************************************************************************/

int rpc_init(const char *addr, unsigned short port)
{
    EC_INIT;

    struct evhttp_connection *evcon;

    EC_NULL_LOG(ev_base = event_init());
    event_set_log_callback(ev_log_cb);
	EC_NULL_LOG(rpc_pool = evrpc_pool_new(NULL));
	EC_NULL_LOG(evcon = evhttp_connection_new(addr, port));
	evrpc_pool_add_connection(rpc_pool, evcon);

EC_CLEANUP:
    EC_EXIT;
}
