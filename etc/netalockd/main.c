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
#include <atalk/adouble.h>
#include <atalk/paths.h>
#include <atalk/util.h>

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
    sigset_t            sigs;
    int                 ret;

    /* Log SIGBUS/SIGSEGV SBT */
    fault_setup(NULL);

    /* Default log setup: log to syslog */
    setuplog("default log_note");

    switch(server_lock("netalockd", _PATH_NETALOCKD_LOCK, 0)) {
    case -1: /* error */
        exit(EXITERR_SYS);
    case 0: /* child */
        break;
    default: /* server */
        exit(0);
    }

    set_signal();

    while (1) {
        if (reloadconfig) {
            reloadconfig = 0;
        }
    }

    return 0;
}
