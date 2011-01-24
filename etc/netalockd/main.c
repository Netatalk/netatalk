/*
 * Copyright (c) 2010 Frank Lahm
 * All Rights Reserved.  See COPYRIGHT.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <errno.h>

#include <atalk/logger.h>
#include <atalk/paths.h>
#include <atalk/util.h>

#include "event2/event.h"
#include "event2/http.h"
#include "event2/rpc.h"

#include <atalk/lockrpc.gen.h>

EVRPC_HEADER(lock_msg, lock_req, lock_rep)
EVRPC_GENERATE(lock_msg, lock_req, lock_rep)

struct event_base *eventbase;
struct evrpc_base *rpcbase;
struct evhttp *http;

static void sighandler(int signal)
{
    switch( signal ) {

    case SIGTERM :
    case SIGINT:
        LOG(log_error, logtype_default, "shutting down on signal");
        exit(0);
        break;

    default :
        LOG(log_error, logtype_default, "bad signal" );
    }
    return;
}


static void set_signal(void)
{
    /* catch SIGTERM, SIGINT */

    /* Log SIGBUS/SIGSEGV SBT */
    fault_setup(NULL);

    /* Ignore the rest */
    struct sigaction sv;
    memset(&sv, 0, sizeof(struct sigaction));
    sv.sa_handler = SIG_IGN;
    sv.sa_flags = SA_RESTART;
    sigemptyset(&sv.sa_mask);

    if (sigaction(SIGABRT, &sv, NULL) < 0) {
        LOG(log_error, logtype_default, "error in sigaction(SIGABRT): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGHUP, &sv, NULL) < 0) {
        LOG(log_error, logtype_default, "error in sigaction(SIGHUP): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGQUIT, &sv, NULL) < 0) {
        LOG(log_error, logtype_default, "error in sigaction(SIGQUIT): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGALRM, &sv, NULL) < 0) {
        LOG(log_error, logtype_default, "error in sigaction(SIGALRM): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGUSR1, &sv, NULL) < 0) {
        LOG(log_error, logtype_default, "error in sigaction(SIGUSR1): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGUSR2, &sv, NULL) < 0) {
        LOG(log_error, logtype_default, "error in sigaction(SIGUSR2): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        

}

static void lock_msg_cb(EVRPC_STRUCT(lock_msg)* rpc, void *arg _U_)
{
    int ret = 0;
    char *filename;
    struct lock_req *request = rpc->request;
    struct lock_rep *reply = rpc->reply;

    if (EVTAG_GET(request, req_filename, &filename) == -1) {
        LOG(log_error, logtype_default, "lock_msg_cb: no filename");
        exit(1);
    }

    LOG(log_warning, logtype_default, "lock_msg_cb(file: \"%s\")", filename);

	/* we just want to fill in some non-sense */
	EVTAG_ASSIGN(reply, result, 0);

	/* no reply to the RPC */
	EVRPC_REQUEST_DONE(rpc);
}

static int rpc_setup(const char *addr, uint16_t port)
{
    eventbase = event_base_new();
    
	if ((http = evhttp_new(eventbase)) == NULL) {
        LOG(log_error, logtype_default, "rpc_setup: error in evhttp_new: %s", strerror(errno));
        return -1;
    }

	if (evhttp_bind_socket(http, addr, port) != 0) {
        LOG(log_error, logtype_default, "rpc_setup: error in evhttp_new: %s", strerror(errno));
        return -1;
    }

    rpcbase = evrpc_init(http);

    EVRPC_REGISTER(rpcbase, lock_msg, lock_req, lock_rep, lock_msg_cb, NULL);

	return 0;
}

static void rpc_teardown(struct evrpc_base *rpcbase)
{
	EVRPC_UNREGISTER(rpcbase, lock_msg);
	evrpc_free(rpcbase);
}

int main(int ac, char **av)
{
    int ret;

    /* Default log setup: log to syslog */
    set_processname("netalockd");
    setuplog("default log_note");

    /* Check lockfile and daemonize */
    switch(server_lock("netalockd", _PATH_NETALOCKD_LOCK, 0)) {
    case -1: /* error */
        exit(EXITERR_SYS);
    case 0: /* child */
        break;
    default: /* server */
        exit(0);
    }

    /* Setup signal stuff */
    set_signal();

    /* Start listening */
    if (rpc_setup("127.0.0.1", 4701) != 0) {
        LOG(log_error, logtype_default, "main: rpc setup error");
        exit(1);
    }

    LOG(log_warning, logtype_default, "Running...");

    /* wait for events - this is where we sit for most of our life */
    event_base_dispatch(eventbase);

exit:
    rpc_teardown(rpcbase);
    evhttp_free(http);

    return 0;
}
