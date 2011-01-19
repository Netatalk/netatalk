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
#include <atalk/tevent.h>

static int reloadconfig;

static void sighandler(int sig)
{
    switch( sig ) {

    case SIGTERM :
        LOG(log_note, logtype_default, "netalockd shutting down on SIGTERM");
        exit(0);
        break;

    case SIGHUP :
        reloadconfig = 1;
        break;

    default :
        LOG(log_error, logtype_default, "bad signal" );
    }
    return;
}


static void set_signal(void)
{
    struct sigaction sv;

    sv.sa_handler = sighandler;
    sv.sa_flags = SA_RESTART;
    sigemptyset(&sv.sa_mask);
    if (sigaction(SIGTERM, &sv, NULL) < 0) {
        LOG(log_error, logtype_default, "error in sigaction(SIGTERM): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        
    if (sigaction(SIGINT, &sv, NULL) < 0) {
        LOG(log_error, logtype_default, "error in sigaction(SIGINT): %s", strerror(errno));
        exit(EXIT_FAILURE);
    }        

    memset(&sv, 0, sizeof(struct sigaction));
    sv.sa_handler = SIG_IGN;
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

int main(int ac, char **av)
{
    struct tevent_context *event_ctx;
    sigset_t sigs;
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
    /* Log SIGBUS/SIGSEGV SBT */
    fault_setup(NULL);

    if ((event_ctx = tevent_context_init(talloc_autofree_context())) == NULL) {
        LOG(log_error, logtype_default, "Error initializing event lib");
        exit(1);
    }

    LOG(log_warning, logtype_default, "Running...");

    /* wait for events - this is where we sit for most of our life */
    tevent_loop_wait(event_ctx);

    talloc_free(event_ctx);

    return 0;
}
