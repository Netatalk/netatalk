/*
   Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <bstrlib.h>

#ifdef WITH_LIBEV
#include <ev.h>
#else
#include <event2/event.h>
#endif

#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/compat.h>
#include <atalk/dsi.h>
#include <atalk/errchk.h>
#include <atalk/globals.h>
#include <atalk/iniparser_util.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/server_child.h>
#include <atalk/server_ipc.h>
#include <atalk/util.h>

#include "afp_zeroconf.h"

/*! how many seconds we wait to shutdown from SIGTERM before we send SIGKILL */
#define KILL_GRACETIME 5

/* defines that control whether services should run by default */
#define NETATALK_SRV_NEEDED  -1
#define NETATALK_SRV_OPTIONAL 0
#define NETATALK_SRV_ERROR    NETATALK_SRV_NEEDED

/* forward declarations */
static pid_t run_process(const char *path, ...);
static void kill_childs(int sig, ...);
static void netatalk_exit(int ret);

/* static variables */
static AFPObj obj;
static pid_t afpd_pid = NETATALK_SRV_NEEDED;
#ifdef CNID_BACKEND_DBD
static pid_t cnid_metad_pid = NETATALK_SRV_NEEDED;
#else
static pid_t cnid_metad_pid = NETATALK_SRV_OPTIONAL;
#endif
static pid_t dbus_pid = NETATALK_SRV_OPTIONAL;
static uint afpd_restarts, cnid_metad_restarts, dbus_restarts _U_;
#ifdef WITH_LIBEV
static struct ev_loop *loop;
static ev_signal sigterm_ev, sigquit_ev, sigchld_ev, sighup_ev;
static ev_timer timer_ev, kill_timer_ev;
#else
static struct event_base *base;
struct event *sigterm_ev, *sigquit_ev, *sigchld_ev, *sighup_ev, *timer_ev;
#endif
static int in_shutdown;
static const char *dbus_path _U_;

/******************************************************************
 * Misc stuff
 ******************************************************************/

static bool service_running(pid_t pid)
{
    if ((pid != NETATALK_SRV_NEEDED) && (pid != NETATALK_SRV_OPTIONAL)) {
        return true;
    }

    return false;
}

#ifdef WITH_SPOTLIGHT
/*! Create directory and all missing parent directories (like mkdir -p) */
static int makedirs(const char *path, mode_t mode)
{
    char tmp[PATH_MAX];
    char *p;

    if (strlcpy(tmp, path, sizeof(tmp)) >= sizeof(tmp)) {
        errno = ENAMETOOLONG;
        return -1;
    }

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';

            if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
                return -1;
            }

            *p = '/';
        }
    }

    if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
        return -1;
    }

    return 0;
}

/*! Set indexers to index all our volumes via a dconf keyfile */
static int set_sl_volumes(void)
{
    EC_INIT;
    const struct vol *volumes, *vol;
    FILE *fp = NULL;
    int sysret;
    bool first;
    EC_NULL_LOG(volumes = getvolumes());

    if (makedirs(INDEXER_DCONF_DB_DIR, 0755) != 0) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to create " INDEXER_DCONF_DB_DIR ": %s",
            strerror(errno));
        EC_FAIL;
    }

    if ((fp = fopen(INDEXER_DCONF_DB_DIR "/10-spotlight", "w")) == NULL) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to open " INDEXER_DCONF_DB_DIR "/10-spotlight: %s",
            strerror(errno));
        EC_FAIL;
    }

    fprintf(fp, "[" INDEXER_DCONF_PATH "]\n");

    /* Collect Spotlight-enabled volume paths */
    first = true;

    for (vol = volumes; vol; vol = vol->v_next) {
        if (vol->v_flags & AFPVOL_SPOTLIGHT) {
            if (first) {
                fprintf(fp, "index-recursive-directories=['%s'", vol->v_path);
                first = false;
            } else {
                fprintf(fp, ", '%s'", vol->v_path);
            }
        }
    }

    if (first) {
        /* No Spotlight volumes: emit typed empty array so dconf update parses it */
        fprintf(fp, "index-recursive-directories=@as []\n");
    } else {
        fprintf(fp, "]\n");
    }

    fprintf(fp, "index-single-directories=@as []\n");

    if (fflush(fp) != 0) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to write " INDEXER_DCONF_DB_DIR "/10-spotlight: %s",
            strerror(errno));
        EC_FAIL;
    }

    fclose(fp);
    fp = NULL;
    sysret = system(DCONF_UPDATE_COMMAND);

    if (sysret == -1) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: system() failed to run 'dconf update': %s",
            strerror(errno));
        EC_FAIL;
    } else if (sysret != 0) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: 'dconf update' exited with status %d",
            WEXITSTATUS(sysret));
        EC_FAIL;
    }

EC_CLEANUP:

    if (fp) {
        fclose(fp);
    }

    EC_EXIT;
}
#endif /* WITH_SPOTLIGHT */

/******************************************************************
 * event library helper functions
 ******************************************************************/

#ifdef WITH_LIBEV

/*! libev syserr callback */
static void libev_syserr_cb(const char *msg) EV_NOEXCEPT {
    LOG(log_error, logtype_default, "libev fatal: %s", msg);
    netatalk_exit(EXITERR_SYS);
}

#else

/*! libevent logging callback */
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

#endif

/******************************************************************
 * event callback implementations (library agnostic)
 ******************************************************************/

/*! SIGTERM implementation — caller must check in_shutdown before calling */
static void sigterm_impl(void)
{
    sigset_t sigs;
    LOG(log_info, logtype_afpd, "Exiting on SIGTERM");
    in_shutdown = 1;
    /* block any signal but SIGCHLD */
    sigfillset(&sigs);
    sigdelset(&sigs, SIGCHLD);
    sigprocmask(SIG_SETMASK, &sigs, NULL);
#ifdef WITH_SPOTLIGHT
    int sysret = system(INDEXER_COMMAND " -t");

    if (sysret == -1) {
        LOG(log_error, logtype_afpd,
            "sigterm_cb: system() failed to run Spotlight indexer stop: %s",
            strerror(errno));
    } else if (sysret != 0) {
        LOG(log_error, logtype_afpd,
            "sigterm_cb: Spotlight indexer stop exited with status %d",
            WEXITSTATUS(sysret));
    }

#endif
    kill_childs(SIGTERM, &afpd_pid, &cnid_metad_pid, &dbus_pid, NULL);
}

/*! SIGQUIT implementation */
static void sigquit_impl(void)
{
    LOG(log_note, logtype_afpd, "Exiting on SIGQUIT");
#ifdef WITH_SPOTLIGHT
    int sysret = system(INDEXER_COMMAND " -t");

    if (sysret == -1) {
        LOG(log_error, logtype_afpd,
            "sigquit_cb: system() failed to run Spotlight indexer stop: %s",
            strerror(errno));
    } else if (sysret != 0) {
        LOG(log_error, logtype_afpd,
            "sigquit_cb: Spotlight indexer stop exited with status %d",
            WEXITSTATUS(sysret));
    }

#endif
    kill_childs(SIGQUIT, &afpd_pid, &cnid_metad_pid, &dbus_pid, NULL);
}

/*! SIGHUP implementation */
static void sighup_impl(void)
{
    LOG(log_note, logtype_afpd,
        "Received SIGHUP, sending all processes signal to reload config");
    load_volumes(&obj, LV_ALL);

    if (!(obj.options.flags & OPTION_NOZEROCONF)) {
        zeroconf_deregister();
        zeroconf_register(&obj);
        LOG(log_note, logtype_default, "Re-registered with Zeroconf");
    }

    kill_childs(SIGHUP, &afpd_pid, &cnid_metad_pid, NULL);
}

/*! SIGCHLD implementation, returns true if all services have exited during shutdown */
static bool sigchld_impl(void)
{
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status)) {
            if (WEXITSTATUS(status)) {
                LOG(log_info, logtype_default, "child[%d]: exited %d", pid,
                    WEXITSTATUS(status));
            } else {
                LOG(log_info, logtype_default, "child[%d]: done", pid);
            }
        } else {
            if (WIFSIGNALED(status)) {
                LOG(log_info, logtype_default, "child[%d]: killed by signal %d", pid,
                    WTERMSIG(status));
            } else {
                LOG(log_info, logtype_default, "child[%d]: died", pid);
            }
        }

        if (pid == afpd_pid) {
            afpd_pid = NETATALK_SRV_ERROR;
        } else if (pid == cnid_metad_pid) {
            cnid_metad_pid = NETATALK_SRV_ERROR;
        } else if (pid == dbus_pid) {
            dbus_pid = NETATALK_SRV_ERROR;
        } else {
            LOG(log_error, logtype_afpd, "Bad pid: %d", pid);
        }
    }

    return in_shutdown
           && !service_running(afpd_pid)
           && !service_running(cnid_metad_pid)
           && !service_running(dbus_pid);
}

/*! timer implementation */
static void timer_impl(void)
{
    if (in_shutdown) {
        return;
    }

    if (afpd_pid == NETATALK_SRV_NEEDED) {
        afpd_restarts++;
        LOG(log_note, logtype_afpd, "Restarting 'afpd' (restarts: %u)", afpd_restarts);

        if ((afpd_pid = run_process(_PATH_AFPD, "-d", "-F", obj.options.configfile,
                                    NULL)) == -1) {
            LOG(log_error, logtype_default, "Error starting 'afpd'");
        }
    }

    if (cnid_metad_pid == NETATALK_SRV_NEEDED) {
        cnid_metad_restarts++;
        LOG(log_note, logtype_afpd, "Restarting 'cnid_metad' (restarts: %u)",
            cnid_metad_restarts);

        if ((cnid_metad_pid = run_process(_PATH_CNID_METAD, "-d", "-F",
                                          obj.options.configfile, NULL)) == -1) {
            LOG(log_error, logtype_default, "Error starting 'cnid_metad'");
        }
    }

#ifdef WITH_SPOTLIGHT

    if (dbus_pid == NETATALK_SRV_NEEDED) {
        dbus_restarts++;
        LOG(log_note, logtype_afpd, "Restarting 'dbus' (restarts: %u)", dbus_restarts);

        if ((dbus_pid = run_process(dbus_path,
                                    "--config-file=" _PATH_CONFDIR "dbus-session.conf", NULL)) == -1) {
            LOG(log_error, logtype_default, "Error starting '%s'", dbus_path);
        }
    }

#endif
}

/******************************************************************
 * event library callback wrappers
 ******************************************************************/

#ifdef WITH_LIBEV

/*! kill timer callback for graceful shutdown timeout */
static void kill_timer_cb(struct ev_loop *ev_loop _U_, ev_timer *w _U_,
                          int revents _U_)
{
    ev_break(loop, EVBREAK_ALL);
}

static void sigterm_cb(struct ev_loop *ev_loop _U_, ev_signal *w _U_,
                       int revents _U_)
{
    if (in_shutdown) {
        return;
    }

    sigterm_impl();
    /* set gracetime timeout timer, remove all events but SIGCHLD */
    ev_timer_init(&kill_timer_ev, kill_timer_cb, KILL_GRACETIME, 0.0);
    ev_timer_start(loop, &kill_timer_ev);
    ev_signal_stop(loop, &sigterm_ev);
    ev_signal_stop(loop, &sigquit_ev);
    ev_signal_stop(loop, &sighup_ev);
    ev_timer_stop(loop, &timer_ev);
}

static void sigquit_cb(struct ev_loop *ev_loop _U_, ev_signal *w _U_,
                       int revents _U_)
{
    sigquit_impl();
}

static void sighup_cb(struct ev_loop *ev_loop _U_, ev_signal *w _U_,
                      int revents _U_)
{
    sighup_impl();
}

static void sigchld_cb(struct ev_loop *ev_loop _U_, ev_signal *w _U_,
                       int revents _U_)
{
    if (sigchld_impl()) {
        ev_break(loop, EVBREAK_ALL);
    }
}

static void timer_cb(struct ev_loop *ev_loop _U_, ev_timer *w _U_,
                     int revents _U_)
{
    timer_impl();
}

#else /* libevent2 */

static void sigterm_cb(evutil_socket_t fd _U_, short what _U_, void *arg _U_)
{
    struct timeval tv;

    if (in_shutdown) {
        return;
    }

    sigterm_impl();
    /* set gracetime timeout timer, remove all events but SIGCHLD */
    tv.tv_sec = KILL_GRACETIME;
    tv.tv_usec = 0;
    event_base_loopexit(base, &tv);
    event_del(sigterm_ev);
    event_del(sigquit_ev);
    event_del(sighup_ev);
    event_del(timer_ev);
}

static void sigquit_cb(evutil_socket_t fd _U_, short what _U_, void *arg _U_)
{
    sigquit_impl();
}

static void sighup_cb(evutil_socket_t fd _U_, short what _U_, void *arg _U_)
{
    sighup_impl();
}

static void sigchld_cb(evutil_socket_t fd _U_, short what _U_, void *arg _U_)
{
    if (sigchld_impl()) {
        event_base_loopbreak(base);
    }
}

static void timer_cb(evutil_socket_t fd _U_, short what _U_, void *arg _U_)
{
    timer_impl();
}

#endif /* WITH_LIBEV */

/******************************************************************
 * helper functions
 ******************************************************************/

/*! kill processes passed as varargs of type "pid_t *", terminate list with NULL */
static void kill_childs(int sig, ...)
{
    va_list args;
    pid_t *pid;
    va_start(args, sig);

    while ((pid = va_arg(args, pid_t *)) != NULL) {
        if (*pid == NETATALK_SRV_ERROR || *pid == NETATALK_SRV_OPTIONAL) {
            continue;
        }

        kill(*pid, sig);
    }

    va_end(args);
}

/*! this get called when error conditions are met that require us to exit gracefully */
static void netatalk_exit(int ret)
{
    server_unlock(PATH_NETATALK_LOCK);
    exit(ret);
}

/*! this forks() and exec() "path" with varags as argc[] */
static pid_t run_process(const char *path, ...)
{
    int i = 0;
#define MYARVSIZE 64
    char *myargv[MYARVSIZE];
    va_list args;
    pid_t pid;

    if ((pid = fork()) < 0) {
        LOG(log_error, logtype_cnid, "error in fork: %s", strerror(errno));
        return -1;
    }

    if (pid == 0) {
        myargv[i++] = (char *)path;
        va_start(args, path);

        while (i < MYARVSIZE) {
            if ((myargv[i++] = va_arg(args, char *)) == NULL) {
                break;
            }
        }

        va_end(args);
        (void)execv(path, myargv);
        /* Yikes! We're still here, so exec failed... */
        LOG(log_error, logtype_cnid, "Fatal error in exec: %s", strerror(errno));
        exit(1);
    }

    return pid;
}

static void show_netatalk_version(void)
{
    int num _U_, i _U_;
    printf("netatalk %s - Netatalk AFP server service controller daemon\n\n",
           VERSION);
    puts("This program is free software; you can redistribute it and/or modify it under");
    puts("the terms of the GNU General Public License as published by the Free Software");
    puts("Foundation; either version 2 of the License, or (at your option) any later");
    puts("version. Please see the file COPYING for further information and details.\n");
    puts("netatalk has been compiled with support for these features:\n");
    printf("      Zeroconf support:\t");
#if defined (HAVE_MDNS)
    puts("mDNSResponder");
#elif defined (HAVE_AVAHI)
    puts("Avahi");
#else
    puts("No");
#endif
    printf("     Spotlight support:\t");
#ifdef WITH_SPOTLIGHT
    puts("Yes");
#else
    puts("No");
#endif
}

static void show_netatalk_paths(void)
{
    printf("              afp.conf:\t%s\n", _PATH_CONFDIR "afp.conf");
    printf("                  afpd:\t%s\n", _PATH_AFPD);
    printf("            cnid_metad:\t%s\n", _PATH_CNID_METAD);
#ifdef WITH_SPOTLIGHT
    printf("           dbus-daemon:\t%s\n", DBUS_DAEMON_PATH);
    printf("     dbus-session.conf:\t%s\n", _PATH_CONFDIR "dbus-session.conf");
    printf("       indexer manager:\t%s\n", INDEXER_COMMAND);
#endif
#ifndef SOLARIS
    printf("    netatalk lock file:\t%s\n", PATH_NETATALK_LOCK);
#endif
}

static void usage(void)
{
    printf("usage: netatalk [-F configfile] \n");
    printf("       netatalk -d \n");
    printf("       netatalk -v|-V \n");
}

int main(int argc, char **argv)
{
    int c, ret, debug = 0;
    sigset_t blocksigs;
#ifndef WITH_LIBEV
    struct timeval tv;
#endif
    /* Log SIGBUS/SIGSEGV SBT */
    fault_setup(NULL);

    while ((c = getopt(argc, argv, ":dF:vV")) != -1) {
        switch (c) {
        case 'd':
            debug = 1;
            break;

        case 'F':
            obj.cmdlineconfigfile = strdup(optarg);
            break;

        case 'v':       /* version */
        case 'V':       /* version */
            show_netatalk_version();
            puts("");
            show_netatalk_paths();
            puts("");
            exit(0);

        default:
            usage();
            exit(EXIT_FAILURE);
        }
    }

    if (check_lockfile("netatalk", PATH_NETATALK_LOCK) != 0) {
        exit(EXITERR_SYS);
    }

    if (!debug && daemonize() != 0) {
        exit(EXITERR_SYS);
    }

    if (create_lockfile("netatalk", PATH_NETATALK_LOCK) != 0) {
        exit(EXITERR_SYS);
    }

    sigfillset(&blocksigs);
    sigprocmask(SIG_SETMASK, &blocksigs, NULL);

    if (afp_config_parse(&obj, "netatalk") != 0) {
        netatalk_exit(EXITERR_CONF);
    }

    load_volumes(&obj, LV_ALL);
#ifdef WITH_LIBEV
    ev_set_syserr_cb(libev_syserr_cb);
#else
    event_set_log_callback(libevent_logmsg_cb);
    event_set_fatal_callback(netatalk_exit);
#endif
    LOG(log_note, logtype_default, "Netatalk AFP server starting");

    if ((afpd_pid = run_process(_PATH_AFPD, "-d", "-F", obj.options.configfile,
                                NULL)) == NETATALK_SRV_ERROR) {
        LOG(log_error, logtype_afpd, "Error starting 'afpd'");
        netatalk_exit(EXITERR_CONF);
    }

#ifdef CNID_BACKEND_DBD

    if ((cnid_metad_pid = run_process(_PATH_CNID_METAD, "-d", "-F",
                                      obj.options.configfile, NULL)) == NETATALK_SRV_ERROR) {
        LOG(log_error, logtype_afpd, "Error starting 'cnid_metad'");
        netatalk_exit(EXITERR_CONF);
    }

#endif
#ifdef WITH_LIBEV

    if ((loop = ev_default_loop(EVFLAG_NOENV)) == NULL) {
        LOG(log_error, logtype_afpd, "Error starting event loop");
        netatalk_exit(EXITERR_CONF);
    }

    ev_signal_init(&sigterm_ev, sigterm_cb, SIGTERM);
    ev_signal_init(&sigquit_ev, sigquit_cb, SIGQUIT);
    ev_signal_init(&sighup_ev, sighup_cb, SIGHUP);
    ev_signal_init(&sigchld_ev, sigchld_cb, SIGCHLD);
    ev_signal_start(loop, &sigterm_ev);
    ev_signal_start(loop, &sigquit_ev);
    ev_signal_start(loop, &sigchld_ev);
    ev_signal_start(loop, &sighup_ev);
    ev_timer_init(&timer_ev, timer_cb, 1.0, 1.0);
    ev_timer_start(loop, &timer_ev);
#else

    if ((base = event_base_new()) == NULL) {
        LOG(log_error, logtype_afpd, "Error starting event loop");
        netatalk_exit(EXITERR_CONF);
    }

    sigterm_ev = event_new(base, SIGTERM, EV_SIGNAL, sigterm_cb, NULL);
    sigquit_ev = event_new(base, SIGQUIT, EV_SIGNAL | EV_PERSIST, sigquit_cb, NULL);
    sighup_ev = event_new(base, SIGHUP,  EV_SIGNAL | EV_PERSIST, sighup_cb, NULL);
    sigchld_ev = event_new(base, SIGCHLD, EV_SIGNAL | EV_PERSIST, sigchld_cb, NULL);
    timer_ev = event_new(base, -1, EV_PERSIST, timer_cb, NULL);
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    event_add(sigterm_ev, NULL);
    event_add(sigquit_ev, NULL);
    event_add(sigchld_ev, NULL);
    event_add(sighup_ev, NULL);
    event_add(timer_ev, &tv);
#endif
    sigfillset(&blocksigs);
    sigdelset(&blocksigs, SIGTERM);
    sigdelset(&blocksigs, SIGQUIT);
    sigdelset(&blocksigs, SIGCHLD);
    sigdelset(&blocksigs, SIGHUP);
    sigprocmask(SIG_SETMASK, &blocksigs, NULL);
#ifdef WITH_SPOTLIGHT
    if (obj.options.flags & OPTION_SPOTLIGHT) {
        setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=" _PATH_STATEDIR "spotlight.ipc",
               1);
        setenv("DCONF_PROFILE", INDEXER_DCONF_PROFILE, 1);
        setenv("XDG_DATA_HOME", _PATH_STATEDIR, 0);
        setenv("XDG_CACHE_HOME", _PATH_STATEDIR, 0);
        setenv("TRACKER_USE_LOG_FILES", "1", 0);
        dbus_path = INIPARSER_GETSTR(obj.iniconfig, INISEC_GLOBAL, "dbus daemon",
                                     DBUS_DAEMON_PATH);
        LOG(log_note, logtype_default, "Starting dbus: %s", dbus_path);

        if ((dbus_pid = run_process(dbus_path,
                                    "--config-file=" _PATH_CONFDIR "dbus-session.conf",
                                    NULL)) == NETATALK_SRV_ERROR) {
            LOG(log_error, logtype_default, "Error starting '%s'", dbus_path);
            netatalk_exit(EXITERR_CONF);
        }

        /* Allow dbus some time to start up */
        sleep(1);
        set_sl_volumes();
        LOG(log_note, logtype_default, "Starting indexer: " INDEXER_COMMAND " -s");
        int sysret = system(INDEXER_COMMAND " -s");

        if (sysret == -1) {
            LOG(log_error, logtype_default,
                "system() failed to run Spotlight indexer start: %s", strerror(errno));
            netatalk_exit(EXITERR_CONF);
        } else if (sysret != 0) {
            LOG(log_error, logtype_default, "Spotlight indexer start exited with status %d",
                WEXITSTATUS(sysret));
            netatalk_exit(EXITERR_CONF);
        }
    }

#endif

    /* Now register with zeroconf, we also need the volumes for that */
    if (!(obj.options.flags & OPTION_NOZEROCONF)) {
        zeroconf_register(&obj);
        LOG(log_note, logtype_default, "Registered with Zeroconf");
    }

    /* run the event loop */
#ifdef WITH_LIBEV
    ev_run(loop, 0);
    ret = 0;
#else
    ret = event_base_dispatch(base);
#endif

    if (service_running(afpd_pid) || service_running(cnid_metad_pid)
            || service_running(dbus_pid)) {
        if (service_running(afpd_pid)) {
            LOG(log_error, logtype_afpd, "AFP service did not shutdown, killing it");
        }

        if (service_running(cnid_metad_pid)) {
            LOG(log_error, logtype_afpd,
                "CNID database service did not shutdown, killing it");
        }

        if (service_running(dbus_pid)) {
            LOG(log_error, logtype_afpd, "DBUS session daemon still running, killing it");
        }

        kill_childs(SIGKILL, &afpd_pid, &cnid_metad_pid, &dbus_pid, NULL);
    }

    LOG(log_note, logtype_afpd, "Netatalk AFP server exiting");
    netatalk_exit(ret);
}
