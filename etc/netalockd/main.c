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
#include <atalk/tsocket.h>

static void sighandler(struct tevent_context *ev,
                       struct tevent_signal *se,
                       int signal,
                       int count,
                       void *siginfo,
                       void *private_data)
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


static void set_signal(struct tevent_context *event_ctx)
{
    /* catch SIGTERM, SIGINT */
    if (tevent_add_signal(event_ctx, event_ctx, SIGTERM, 0, sighandler, NULL) == NULL) {
        LOG(log_error, logtype_default, "failed to setup SIGTERM handler");
        exit(EXIT_FAILURE);
    }
    if (tevent_add_signal(event_ctx, event_ctx, SIGINT, 0, sighandler, NULL) == NULL) {
        LOG(log_error, logtype_default, "failed to setup SIGINT handler");
        exit(EXIT_FAILURE);
    }

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

    if ((event_ctx = tevent_context_init(talloc_autofree_context())) == NULL) {
        LOG(log_error, logtype_default, "Error initializing event lib");
        exit(1);
    }

    /* Setup signal stuff */
    set_signal(event_ctx);

    /* Setup listening socket */
    struct tsocket_address *addr;
    if (tsocket_address_inet_from_strings(event_ctx,
                                          "ip",
                                          "localhost",
                                          4701,
                                          &addr) != 0) {
        LOG(log_error, logtype_default, "Error initializing socket");
        exit(1);
    }
#if 0
    ssize_t tsocket_address_bsd_sockaddr(const struct tsocket_address *addr,
                                         struct sockaddr *sa,
                                         size_t sa_socklen)
#endif

    LOG(log_warning, logtype_default, "Running...");

    /* wait for events - this is where we sit for most of our life */
    tevent_loop_wait(event_ctx);

    talloc_free(event_ctx);

    return 0;
}
