/*
 * Copyright (c) 1990,1993 Regents of The University of Michigan.
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
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include <atalk/logger.h>
#include <atalk/adouble.h>
#include <atalk/compat.h>
#include <atalk/dsi.h>
#include <atalk/afp.h>
#include <atalk/paths.h>
#include <atalk/util.h>
#include <atalk/server_child.h>
#include <atalk/server_ipc.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/netatalk_conf.h>

#include <event2/event.h>

/* forward declaration */
static pid_t run_process(const char *path, ...);

/* static variables */
static AFPObj obj;
static sig_atomic_t got_chldsig;
static pid_t afpd_pid = -1,  cnid_metad_pid = -1;
static uint afpd_restarts, cnid_metad_restarts;
static struct event_base *base;

/* event functions */
static void libevent_logmsg_cb(int severity, const char *msg)
{
    switch (severity) {
    case _EVENT_LOG_DEBUG:
        LOG(log_debug, logtype_default, "libevent: %s", msg);
        break;
    case _EVENT_LOG_MSG:
        LOG(log_info, logtype_default, "libevent: %s", msg);
        break;
    case _EVENT_LOG_WARN:
        LOG(log_warning, logtype_default, "libevent: %s", msg);
        break;
    case _EVENT_LOG_ERR:
        LOG(log_error, logtype_default, "libevent: %s", msg);
        break;
    default:
        LOG(log_error, logtype_default, "libevent: %s", msg);
        break; /* never reached */
    }
}

static void sigterm_cb(evutil_socket_t fd, short what, void *arg)
{
    LOG(log_note, logtype_afpd, "Exiting on SIGTERM");
    if (afpd_pid != -1)
        kill(afpd_pid, SIGTERM);
    if (cnid_metad_pid != -1)
        kill(cnid_metad_pid, SIGTERM);
    server_unlock(_PATH_NETATALK_LOCK);
    event_base_loopbreak(base);
}

static void sigquit_cb(evutil_socket_t fd, short what, void *arg)
{
    LOG(log_note, logtype_afpd, "Exiting on SIGQUIT");
    if (afpd_pid != -1)
        kill(afpd_pid, SIGTERM);
    if (cnid_metad_pid != -1)
        kill(cnid_metad_pid, SIGTERM);
    server_unlock(_PATH_NETATALK_LOCK);
    event_base_loopbreak(base);
}

static void sigchld_cb(evutil_socket_t fd, short what, void *arg)
{
    int status, i;
    pid_t pid;

    LOG(log_debug, logtype_afpd, "Got SIGCHLD event");
  
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status))
                LOG(log_info, logtype_afpd, "child[%d]: exited %d", pid, WEXITSTATUS(status));
            else
                LOG(log_info, logtype_afpd, "child[%d]: done", pid);
        } else {
            if (WIFSIGNALED(status))
                LOG(log_info, logtype_afpd, "child[%d]: killed by signal %d", pid, WTERMSIG(status));
            else
                LOG(log_info, logtype_afpd, "child[%d]: died", pid);
        }

        if (pid == afpd_pid) {
            sleep(1);
            afpd_restarts++;
            LOG(log_note, logtype_afpd, "Restarting 'afpd' (restarts: %u)", afpd_restarts);
            if ((afpd_pid = run_process(_PATH_AFPD, "-d", "-F", obj.options.configfile, NULL)) == -1) {
                LOG(log_error, logtype_afpd, "Error starting 'afpd'");
            }
        } else if (pid = cnid_metad_pid) {
            sleep(1);
            cnid_metad_restarts++;
            LOG(log_note, logtype_afpd, "Restarting 'cnid_metad' (restarts: %u)", cnid_metad_restarts);
            if ((cnid_metad_pid = run_process(_PATH_CNID_METAD, "-d", "-F", obj.options.configfile, NULL)) == -1) {
                LOG(log_error, logtype_afpd, "Error starting 'cnid_metad'");
            }
        } else {
            LOG(log_error, logtype_afpd, "Bad pid: %d", pid);
        }
    }
}

/* other functions */

static void netatalk_exit(int ret)
{
    if (afpd_pid != -1)
        kill(afpd_pid, SIGTERM);
    if (cnid_metad_pid != -1)
        kill(cnid_metad_pid, SIGTERM);
    server_unlock(_PATH_NETATALK_LOCK);
    exit(ret);
}

static pid_t run_process(const char *path, ...)
{
    int ret, i = 0;
    char *myargv[10];
    va_list args;
    pid_t pid;

    if ((pid = fork()) < 0) {
        LOG(log_error, logtype_cnid, "error in fork: %s", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        myargv[i++] = (char *)path;
        va_start(args, path);
        while ((myargv[i++] = va_arg(args, char *)) != NULL)
            ;
        va_end(args);

        ret = execv(path, myargv);

        /* Yikes! We're still here, so exec failed... */
        LOG(log_error, logtype_cnid, "Fatal error in exec: %s", strerror(errno));
        exit(1);
    }
    return pid;
}

static void usage(void)
{
    printf("usage: netatalk [-F configfile] \n");
}

int main(int argc, char **argv)
{
    const char *configfile = NULL;
    int c, ret, debug = 0;
    sigset_t blocksigs;

    /* Log SIGBUS/SIGSEGV SBT */
    fault_setup(NULL);

    while ((c = getopt(argc, argv, ":dF:")) != -1) {
        switch(c) {
        case 'd':
            debug = 1;
            break;
        case 'F':
            obj.cmdlineconfigfile = strdup(optarg);
            break;
        default:
            usage();
            exit(EXIT_FAILURE);
        }
    }

    if (check_lockfile("netatalk", _PATH_NETATALK_LOCK) != 0)
        exit(EXITERR_SYS);

    if (!debug && daemonize(0, 0) != 0)
        exit(EXITERR_SYS);

    if (create_lockfile("netatalk", _PATH_NETATALK_LOCK) != 0)
        exit(EXITERR_SYS);

    sigfillset(&blocksigs);
    sigprocmask(SIG_SETMASK, &blocksigs, NULL);
    
    if (afp_config_parse(&obj) != 0)
        netatalk_exit(EXITERR_CONF);

    set_processname("netatalk");
    setuplog(obj.options.logconfig, obj.options.logfile);
    event_set_log_callback(libevent_logmsg_cb);
    event_set_fatal_callback(netatalk_exit);

    LOG(log_note, logtype_default, "Netatalk AFP server starting");

    if ((afpd_pid = run_process(_PATH_AFPD, "-d", "-F", obj.options.configfile, NULL)) == -1) {
        LOG(log_error, logtype_afpd, "Error starting 'cnid_metad'");
        netatalk_exit(EXITERR_CONF);
    }

    if ((cnid_metad_pid = run_process(_PATH_CNID_METAD, "-d", "-F", obj.options.configfile, NULL)) == -1) {
        LOG(log_error, logtype_afpd, "Error starting 'cnid_metad'");
        netatalk_exit(EXITERR_CONF);
    }

    if ((base = event_base_new()) == NULL) {
        LOG(log_error, logtype_afpd, "Error starting event loop");
        netatalk_exit(EXITERR_CONF);
    }

    struct event *sigterm_ev, *sigquit_ev, *sigchld_ev;
    sigterm_ev = event_new(base, SIGTERM, EV_SIGNAL|EV_PERSIST, sigterm_cb, NULL);
    sigquit_ev = event_new(base, SIGQUIT, EV_SIGNAL|EV_PERSIST, sigquit_cb, NULL);
    sigchld_ev = event_new(base, SIGCHLD, EV_SIGNAL|EV_PERSIST, sigchld_cb, NULL);

    event_add(sigterm_ev, NULL);
    event_add(sigquit_ev, NULL);
    event_add(sigchld_ev, NULL);

    sigfillset(&blocksigs);
    sigdelset(&blocksigs, SIGTERM);
    sigdelset(&blocksigs, SIGQUIT);
    sigdelset(&blocksigs, SIGCHLD);
    sigprocmask(SIG_SETMASK, &blocksigs, NULL);

    /* run the event loop */
    ret = event_base_dispatch(base);

    return ret;
}
