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
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <unistd.h>

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
static pid_t run_afpd(void);
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
/* whether the volume list loaded; without it the cnid_metad
 * decisions fail conservative (run the daemon) */
static bool volumes_loaded;
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

/* The lock path is normally compiled in for a system service. In rootless
 * mode it is supplied by the caller and must live in private user state. */
static const char *lockfile_path = PATH_NETATALK_LOCK;

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

static bool srp_is_the_only_uam(const char *uamlist)
{
    const unsigned char *p = (const unsigned char *)uamlist;
    size_t count = 0;

    if (p == NULL) {
        return false;
    }

    while (*p != '\0') {
        const unsigned char *start;
        size_t len;

        while (*p != '\0' && (isspace(*p) || *p == ',')) {
            p++;
        }

        if (*p == '\0') {
            break;
        }

        start = p;

        while (*p != '\0' && !isspace(*p) && *p != ',') {
            p++;
        }

        len = (size_t)(p - start);

        if (len != strlen("uams_srp.so") || strncmp((const char *)start,
                "uams_srp.so", len) != 0) {
            return false;
        }

        count++;
    }

    return count == 1;
}

static bool dbpath_parent_is_writable(const char *path)
{
    char parent[MAXPATHLEN];
    char *slash;
    size_t len;

    if (path == NULL || strlcpy(parent, path, sizeof(parent)) >= sizeof(parent)) {
        return false;
    }

    len = strlen(parent);

    while (len > 1 && parent[len - 1] == '/') {
        parent[--len] = '\0';
    }

    slash = strrchr(parent, '/');

    if (slash == NULL) {
        strlcpy(parent, ".", sizeof(parent));
    } else if (slash == parent) {
        parent[1] = '\0';
    } else {
        *slash = '\0';
    }

    return access(parent, W_OK | X_OK) == 0;
}

static bool pidfile_path_is_private(const char *path)
{
    char parent[MAXPATHLEN];
    char *slash;
    struct stat st;
    size_t len;

    if (path == NULL || strlcpy(parent, path, sizeof(parent)) >= sizeof(parent)) {
        return false;
    }

    len = strlen(parent);

    while (len > 1 && parent[len - 1] == '/') {
        parent[--len] = '\0';
    }

    slash = strrchr(parent, '/');

    if (slash == NULL) {
        strlcpy(parent, ".", sizeof(parent));
    } else if (slash == parent) {
        parent[1] = '\0';
    } else {
        *slash = '\0';
    }

    if (stat(parent, &st) != 0 || !S_ISDIR(st.st_mode) || st.st_uid != getuid()
            || (st.st_mode & 0077) != 0) {
        return false;
    }

    if (lstat(path, &st) == 0) {
        return S_ISREG(st.st_mode) && st.st_uid == getuid();
    }

    return errno == ENOENT;
}

static int validate_unprivileged_config(void)
{
    struct stat st;
    uid_t uid = getuid();

    if (obj.options.flags & OPTION_DDP) {
        fprintf(stderr, "netatalk: --unprivileged does not support AppleTalk.\n");
        return -1;
    }

    if (obj.options.flags & OPTION_AFPSTATS) {
        fprintf(stderr, "netatalk: --unprivileged does not support afpstats.\n");
        return -1;
    }

    if (obj.options.force_user || obj.options.force_group
            || obj.options.admingid != 0) {
        fprintf(stderr,
                "netatalk: --unprivileged does not support admin or forced identities.\n");
        return -1;
    }

    if (obj.options.signatureopt == NULL || obj.options.signatureopt[0] == '\0') {
        fprintf(stderr,
                "netatalk: --unprivileged requires an explicit [Global] signature.\n");
        return -1;
    }

    if (!srp_is_the_only_uam(obj.options.uamlist)) {
        fprintf(stderr,
                "netatalk: --unprivileged requires 'uam list = uams_srp.so'.\n");
        return -1;
    }

    if (stat(obj.options.configfile, &st) != 0 || st.st_uid != uid
            || (st.st_mode & 0022) != 0) {
        fprintf(stderr,
                "netatalk: --unprivileged requires a configuration file owned by the calling user and not writable by group or others.\n");
        return -1;
    }

    if (stat(obj.options.srppasswdfile, &st) != 0 || !S_ISREG(st.st_mode)
            || st.st_uid != uid || (st.st_mode & 0077) != 0) {
        fprintf(stderr,
                "netatalk: --unprivileged requires a mode-0600 SRP verifier file owned by the calling user.\n");
        return -1;
    }

    if (INIPARSER_GETSTR(obj.iniconfig, INISEC_HOMES, "basedir regex",
                         NULL) != NULL) {
        fprintf(stderr, "netatalk: --unprivileged does not support [Homes] volumes.\n");
        return -1;
    }

    if (!volumes_loaded || getvolumes() == NULL) {
        fprintf(stderr,
                "netatalk: --unprivileged requires at least one static volume.\n");
        return -1;
    }

    for (const struct vol *vol = getvolumes(); vol != NULL; vol = vol->v_next) {
        if (vol->v_cnidscheme == NULL || strcasecmp(vol->v_cnidscheme, "sqlite") != 0) {
            fprintf(stderr, "netatalk: --unprivileged volume '%s' must use sqlite CNID.\n",
                    vol->v_localname);
            return -1;
        }

        if (vol->v_uuid == NULL || vol->v_uuid[0] == '\0') {
            fprintf(stderr,
                    "netatalk: --unprivileged volume '%s' requires an explicit volume uuid.\n",
                    vol->v_localname);
            return -1;
        }

        if (vol->v_flags & AFPVOL_SPOTLIGHT) {
            fprintf(stderr,
                    "netatalk: --unprivileged does not support Spotlight on volume '%s'.\n",
                    vol->v_localname);
            return -1;
        }

        if (access(vol->v_path, R_OK | X_OK) != 0
                || (!(vol->v_flags & AFPVOL_RO) && access(vol->v_path, W_OK | X_OK) != 0)) {
            fprintf(stderr,
                    "netatalk: --unprivileged cannot access volume '%s' as the calling user.\n",
                    vol->v_localname);
            return -1;
        }

        if (!dbpath_parent_is_writable(vol->v_dbpath)) {
            fprintf(stderr,
                    "netatalk: --unprivileged requires a writable parent directory for vol dbpath of volume '%s'.\n",
                    vol->v_localname);
            return -1;
        }
    }

    return 0;
}

#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH
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

/*! Open a dconf input file with permissions independent of the process umask */
static FILE *open_dconf_file(const char *path)
{
    int fd, saved_errno;
    FILE *fp;

    if ((fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
        return NULL;
    }

    /* open() does not update the mode of an existing file. */
    if (fchmod(fd, 0644) == -1) {
        saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return NULL;
    }

    if ((fp = fdopen(fd, "w")) == NULL) {
        saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return NULL;
    }

    return fp;
}

/*! Set indexers to index all our volumes via a dconf keyfile */
static int set_sl_volumes(void)
{
    EC_INIT;
    const struct vol *volumes, *vol;
    FILE *fp = NULL;
    int status;
    pid_t pid, waitret;
    bool first;
    EC_NULL_LOG(volumes = getvolumes());

    if (makedirs(INDEXER_DCONF_DB_DIR, 0755) != 0) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to create " INDEXER_DCONF_DB_DIR ": %s",
            strerror(errno));
        EC_FAIL;
    }

    if (makedirs(INDEXER_DCONF_DB_DIR "/locks", 0755) != 0) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to create " INDEXER_DCONF_DB_DIR "/locks: %s",
            strerror(errno));
        EC_FAIL;
    }

    if ((fp = open_dconf_file(INDEXER_DCONF_DB_DIR "/10-spotlight")) == NULL) {
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
            if (strchr(vol->v_path, '\'') != NULL) {
                LOG(log_warning, logtype_sl,
                    "set_sl_volumes: skipping volume with single quote in path: \"%s\"",
                    vol->v_path);
                continue;
            }

            if (first) {
                fprintf(fp, "index-recursive-directories=['%s'", vol->v_path);
                first = false;
            } else {
                fprintf(fp, ", '%s'", vol->v_path);
            }
        }
    }

    if (first) {
        /* No Spotlight volumes: emit typed empty array so dconf compile parses it */
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

    if (fclose(fp) != 0) {
        fp = NULL;
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to close " INDEXER_DCONF_DB_DIR
            "/10-spotlight: %s", strerror(errno));
        EC_FAIL;
    }

    fp = NULL;

    if ((fp = open_dconf_file(INDEXER_DCONF_DB_DIR "/locks/spotlight")) == NULL) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to open " INDEXER_DCONF_DB_DIR
            "/locks/spotlight: %s", strerror(errno));
        EC_FAIL;
    }

    fprintf(fp, "/" INDEXER_DCONF_PATH "/index-recursive-directories\n");
    fprintf(fp, "/" INDEXER_DCONF_PATH "/index-single-directories\n");

    if (fflush(fp) != 0) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to write " INDEXER_DCONF_DB_DIR
            "/locks/spotlight: %s", strerror(errno));
        EC_FAIL;
    }

    if (fclose(fp) != 0) {
        fp = NULL;
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to close " INDEXER_DCONF_DB_DIR
            "/locks/spotlight: %s", strerror(errno));
        EC_FAIL;
    }

    fp = NULL;

    if ((pid = fork()) == -1) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to fork for 'dconf compile': %s",
            strerror(errno));
        EC_FAIL;
    }

    if (pid == 0) {
        int exec_errno;
        execl(INDEXER_DCONF_COMMAND, INDEXER_DCONF_COMMAND, "compile",
              INDEXER_DCONF_DB, INDEXER_DCONF_DB_DIR, NULL);
        exec_errno = errno;
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to execute '%s compile': %s",
            INDEXER_DCONF_COMMAND, strerror(exec_errno));
        _exit(127);
    }

    do {
        waitret = waitpid(pid, &status, 0);
    } while (waitret == -1 && errno == EINTR);

    if (waitret == -1) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: failed to wait for 'dconf compile': %s",
            strerror(errno));
        EC_FAIL;
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: 'dconf compile' exited with status %d",
            WEXITSTATUS(status));
        EC_FAIL;
    } else if (WIFSIGNALED(status)) {
        LOG(log_error, logtype_sl,
            "set_sl_volumes: 'dconf compile' killed by signal %d",
            WTERMSIG(status));
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
    kill_childs(SIGTERM, &afpd_pid, &cnid_metad_pid, &dbus_pid, NULL);
}

/*! SIGQUIT implementation */
static void sigquit_impl(void)
{
    LOG(log_note, logtype_afpd, "Exiting on SIGQUIT");
    kill_childs(SIGQUIT, &afpd_pid, &cnid_metad_pid, &dbus_pid, NULL);
}

/*! SIGHUP implementation */
static void sighup_impl(void)
{
    if (obj.cmdlineflags & OPTION_UNPRIVILEGED) {
        LOG(log_note, logtype_afpd,
            "Ignoring SIGHUP: configuration reload is disabled in unprivileged mode");
        return;
    }

    LOG(log_note, logtype_afpd,
        "Received SIGHUP, sending all processes signal to reload config");

    if (load_afp_conf_vols(&obj, LV_ALL) == 0) {
        volumes_loaded = true;
    }

    if (!(obj.options.flags & OPTION_NOZEROCONF)) {
        zeroconf_deregister();
        zeroconf_register(&obj);
        LOG(log_note, logtype_default, "Re-registered with Zeroconf");
    }

    kill_childs(SIGHUP, &afpd_pid, &cnid_metad_pid, NULL);
#ifdef CNID_BACKEND_DBD

    if (volumes_loaded) {
        bool dbd_in_use = conf_cnid_scheme_in_use(&obj, "dbd");

        if (service_running(cnid_metad_pid) && !dbd_in_use) {
            LOG(log_note, logtype_afpd,
                "Stopping 'cnid_metad': no volume uses the dbd CNID scheme");
            kill_childs(SIGTERM, &cnid_metad_pid, NULL);
        } else if (!service_running(cnid_metad_pid) && dbd_in_use) {
            LOG(log_note, logtype_afpd,
                "Starting 'cnid_metad': a volume now uses the dbd CNID scheme");
            cnid_metad_pid = NETATALK_SRV_NEEDED;
        }
    }

#endif
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

        if ((afpd_pid = run_afpd()) == -1) {
            LOG(log_error, logtype_default, "Error starting 'afpd'");
        }
    }

    if (cnid_metad_pid == NETATALK_SRV_NEEDED) {
        if (volumes_loaded && !conf_cnid_scheme_in_use(&obj, "dbd")) {
            cnid_metad_pid = NETATALK_SRV_OPTIONAL;
        } else {
            cnid_metad_restarts++;
            LOG(log_note, logtype_afpd, "Restarting 'cnid_metad' (restarts: %u)",
                cnid_metad_restarts);

            if ((cnid_metad_pid = run_process(_PATH_CNID_METAD, "-d", "-F",
                                              obj.options.configfile, NULL)) == -1) {
                LOG(log_error, logtype_default, "Error starting 'cnid_metad'");
            }
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
    server_unlock(lockfile_path);
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

static pid_t run_afpd(void)
{
    if (obj.cmdlineflags & OPTION_UNPRIVILEGED) {
        return run_process(_PATH_AFPD, "-d", "-u", "-F", obj.options.configfile, NULL);
    }

    return run_process(_PATH_AFPD, "-d", "-F", obj.options.configfile, NULL);
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
    printf("          Zeroconf backend:\t");
#if defined (HAVE_MDNS)
    puts("mDNSResponder");
#elif defined (HAVE_AVAHI)
    puts("Avahi");
#else
    puts("None");
#endif
    printf(" Spotlight search backends:\t");
#ifdef WITH_SPOTLIGHT
    puts(SPOTLIGHT_BACKENDS);
#else
    puts("None");
#endif
}

static void show_netatalk_paths(void)
{
    printf("                  afp.conf:\t%s\n", _PATH_CONFDIR "afp.conf");
    printf("                      afpd:\t%s\n", _PATH_AFPD);
    printf("                cnid_metad:\t%s\n", _PATH_CNID_METAD);
#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH
    printf("               dbus-daemon:\t%s\n", DBUS_DAEMON_PATH);
    printf("         dbus-session.conf:\t%s\n", _PATH_CONFDIR "dbus-session.conf");
#endif
#ifndef SOLARIS
    printf("        netatalk lock file:\t%s\n", PATH_NETATALK_LOCK);
#endif
}

static void usage(void)
{
    printf("usage: netatalk [-d] [-F configfile] \n");
    printf("       netatalk -u -P pidfile [-d] [-F configfile] \n");
    printf("       netatalk -d \n");
    printf("       netatalk -v|-V \n");
}

#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH
static bool any_volume_uses_localsearch(void)
{
    const struct vol *vol;

    for (vol = getvolumes(); vol != NULL; vol = vol->v_next) {
        if ((vol->v_flags & AFPVOL_SPOTLIGHT)
                && vol->v_sl_backend_name != NULL
                && strcasecmp(vol->v_sl_backend_name, "localsearch") == 0) {
            return true;
        }
    }

    return false;
}
#endif /* SPOTLIGHT_BACKEND_LOCALSEARCH */

int main(int argc, char **argv)
{
    static const struct option long_options[] = {
        { "debug",        no_argument,       NULL, 'd' },
        { "config",       required_argument, NULL, 'F' },
        { "pidfile",      required_argument, NULL, 'P' },
        { "unprivileged", no_argument,       NULL, 'u' },
        { "version",      no_argument,       NULL, 'v' },
        { NULL,            0,                 NULL,  0  }
    };
    int c, ret, debug = 0, unprivileged = 0;
    const char *pidfile = NULL;
    sigset_t blocksigs;
#ifndef WITH_LIBEV
    struct timeval tv;
#endif
    /* Log SIGBUS/SIGSEGV SBT */
    fault_setup(NULL);

    while ((c = getopt_long(argc, argv, ":dF:P:uvV", long_options, NULL)) != -1) {
        switch (c) {
        case 'd':
            debug = 1;
            break;

        case 'F':
            obj.cmdlineconfigfile = strdup(optarg);
            break;

        case 'P':
            pidfile = optarg;
            break;

        case 'u':
            unprivileged = 1;
            obj.cmdlineflags |= OPTION_UNPRIVILEGED;
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

    if (unprivileged) {
        if (getuid() == 0 || geteuid() == 0) {
            fprintf(stderr,
                    "netatalk: --unprivileged must be started by a non-root user.\n");
            exit(EXIT_FAILURE);
        }

        if (pidfile == NULL || pidfile[0] == '\0') {
            fprintf(stderr,
                    "netatalk: --unprivileged requires -P with a PID file in private user state.\n");
            exit(EXIT_FAILURE);
        }

        lockfile_path = pidfile;

        if (lockfile_path[0] != '/') {
            fprintf(stderr,
                    "netatalk: --unprivileged requires -P with an absolute PID file path.\n");
            exit(EXIT_FAILURE);
        }

        if (!pidfile_path_is_private(lockfile_path)) {
            fprintf(stderr,
                    "netatalk: --unprivileged requires -P in a mode-0700 directory owned by the calling user.\n");
            exit(EXIT_FAILURE);
        }
    } else if (getuid() != 0 || geteuid() != 0) {
        fprintf(stderr,
                "netatalk: must run as root; use --unprivileged (-u) for a single-user AFP server.\n");
        exit(EXIT_FAILURE);
    } else if (pidfile != NULL) {
        fprintf(stderr, "netatalk: -P is only valid together with --unprivileged.\n");
        exit(EXIT_FAILURE);
    }

    if (afp_config_parse(&obj, "netatalk") != 0) {
        exit(EXITERR_CONF);
    }

    volumes_loaded = (load_afp_conf_vols(&obj, LV_ALL) == 0);

    if (unprivileged && validate_unprivileged_config() != 0) {
        exit(EXITERR_CONF);
    }

    if (check_lockfile("netatalk", lockfile_path) != 0) {
        exit(EXITERR_SYS);
    }

    if (!debug && daemonize() != 0) {
        exit(EXITERR_SYS);
    }

    if (create_lockfile("netatalk", lockfile_path) != 0) {
        exit(EXITERR_SYS);
    }

    sigfillset(&blocksigs);
    sigprocmask(SIG_SETMASK, &blocksigs, NULL);
#ifdef WITH_LIBEV
    ev_set_syserr_cb(libev_syserr_cb);
#else
    event_set_log_callback(libevent_logmsg_cb);
    event_set_fatal_callback(netatalk_exit);
#endif
    LOG(log_note, logtype_default, "Netatalk AFP server starting");

    if ((afpd_pid = run_afpd()) == NETATALK_SRV_ERROR) {
        LOG(log_error, logtype_afpd, "Error starting 'afpd'");
        netatalk_exit(EXITERR_CONF);
    }

#ifdef CNID_BACKEND_DBD

    if (volumes_loaded && !conf_cnid_scheme_in_use(&obj, "dbd")) {
        cnid_metad_pid = NETATALK_SRV_OPTIONAL;
    } else if ((cnid_metad_pid = run_process(_PATH_CNID_METAD, "-d", "-F",
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
#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH

    if (any_volume_uses_localsearch()) {
        setenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=" _PATH_STATEDIR "spotlight.ipc",
               1);
        setenv("DCONF_PROFILE", INDEXER_DCONF_PROFILE, 1);
        setenv("XDG_CONFIG_HOME", _PATH_STATEDIR, 1);
        setenv("XDG_DATA_HOME", _PATH_STATEDIR, 1);
        setenv("XDG_CACHE_HOME", _PATH_STATEDIR, 1);
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
