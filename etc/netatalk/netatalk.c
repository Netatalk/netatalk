/*
   Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>
   Copyright (c) 2026 Andy Lemin (andylemin)

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
#include <inttypes.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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
#include <atalk/srp.h>
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
static pid_t dbus_pid = NETATALK_SRV_OPTIONAL;
static uint afpd_restarts, dbus_restarts _U_;
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

/* The lock path is normally compiled in for a system service. In single-user
 * mode it is supplied by the caller and must live in private user state. */
static const char *lockfile_path = PATH_NETATALK_LOCK;

/******************************************************************
 * Misc stuff
 ******************************************************************/

/*!
 * @brief Tell whether a service slot holds the pid of a running child
 *
 * @param[in] pid  pid slot, or NETATALK_SRV_NEEDED or NETATALK_SRV_OPTIONAL
 * @returns true for a pid, false for either marker
 */
static bool service_running(pid_t pid)
{
    if ((pid != NETATALK_SRV_NEEDED) && (pid != NETATALK_SRV_OPTIONAL)) {
        return true;
    }

    return false;
}

/*!
 * @brief Tell whether uamlist names uams_srp.so and nothing else
 *
 * The call splits the list on commas and spaces, as auth_load does.
 *
 * @param[in] uamlist  uam list of afp.conf, or NULL
 * @returns true for a list of only uams_srp.so, false otherwise or for a list
 *          of MAXPATHLEN bytes or more
 */
static bool srp_is_the_only_uam(const char *uamlist)
{
    char buf[MAXPATHLEN];
    char *last;
    const char *first;

    if (uamlist == NULL || strlcpy(buf, uamlist, sizeof(buf)) >= sizeof(buf)) {
        return false;
    }

    first = strtok_r(buf, ", ", &last);
    return first != NULL && strcmp(first, "uams_srp.so") == 0
           && strtok_r(NULL, ", ", &last) == NULL;
}

/*!
 * @brief Write the parent directory of path into dst
 *
 * The result follows dirname(3). Trailing slashes do not count, a path
 * without a slash gives ".", and a child of the root gives "/".
 *
 * @param[out] dst     receives the parent directory
 * @param[in]  dstlen  size of dst
 * @param[in]  path    path, or NULL
 * @returns 0, or -1 when path is NULL or does not fit dst
 */
static int path_parent(char *dst, size_t dstlen, const char *path)
{
    size_t len;
    char *slash;

    if (path == NULL || (len = strlcpy(dst, path, dstlen)) >= dstlen) {
        return -1;
    }

    while (len > 1 && dst[len - 1] == '/') {
        dst[--len] = '\0';
    }

    if ((slash = strrchr(dst, '/')) == NULL) {
        strlcpy(dst, ".", dstlen);
        return 0;
    }

    /* cut at the last slash, and at the run of slashes before it */
    while (slash > dst && slash[-1] == '/') {
        slash--;
    }

    slash[slash == dst ? 1 : 0] = '\0';
    return 0;
}

/*!
 * @brief Tell whether path is a directory of the caller without group or other
 * permission bits
 *
 * The last component must not be a symbolic link. The call drops trailing
 * slashes first, since lstat follows a final symbolic link through one.
 *
 * @param[in] path  directory
 * @returns true when the real uid owns the directory and its mode has no group
 *          or other bits
 */
static bool owned_private_dir(const char *path)
{
    char dir[MAXPATHLEN];
    struct stat st;
    size_t len = strlcpy(dir, path, sizeof(dir));

    if (len >= sizeof(dir)) {
        return false;
    }

    while (len > 1 && dir[len - 1] == '/') {
        dir[--len] = '\0';
    }

    return lstat(dir, &st) == 0 && S_ISDIR(st.st_mode)
           && st.st_uid == getuid() && (st.st_mode & 0077) == 0;
}

/*!
 * @brief Tell whether the parent directory of path is private to the caller, by
 * owned_private_dir
 *
 * @param[in] path  file or directory
 * @returns true when the parent passes owned_private_dir
 */
static bool parent_is_private(const char *path)
{
    char parent[MAXPATHLEN];
    return path_parent(parent, sizeof(parent), path) == 0
           && owned_private_dir(parent);
}

/*!
 * @brief A path only the caller can reach and replace
 *
 * @param[in] path           the file to judge
 * @param[in] file_mask      mode bits the file must not carry
 * @param[in] may_be_absent  whether an absent path passes
 *
 * @returns true when the parent is private to the caller and path is either
 *          absent-and-allowed or a regular file the caller owns
 */
static bool dir_is_private(const char *path, mode_t file_mask,
                           bool may_be_absent)
{
    struct stat st;

    if (!parent_is_private(path)) {
        return false;
    }

    if (lstat(path, &st) != 0) {
        return may_be_absent && errno == ENOENT;
    }

    return S_ISREG(st.st_mode) && st.st_uid == getuid()
           && (st.st_mode & file_mask) == 0;
}

/*!
 * @brief Tell whether a CNID directory belongs to the caller and is writable,
 * or can be created
 *
 * A symbolic link does not pass. The call drops trailing slashes first. A
 * [Global] vol dbpath gets the volume name and a slash appended, and a
 * trailing slash makes lstat follow a link. A missing directory passes when
 * its parent is writable and searchable.
 *
 * @param[in] path  CNID directory of a volume
 * @returns true for a usable or creatable directory
 */
static bool dbpath_is_creatable(const char *path)
{
    char dir[MAXPATHLEN];
    char parent[MAXPATHLEN];
    struct stat st;
    size_t len = strlcpy(dir, path, sizeof(dir));

    if (len >= sizeof(dir)) {
        return false;
    }

    while (len > 1 && dir[len - 1] == '/') {
        dir[--len] = '\0';
    }

    if (lstat(dir, &st) == 0) {
        return S_ISDIR(st.st_mode) && st.st_uid == getuid()
               && access(dir, W_OK | X_OK) == 0;
    }

    return errno == ENOENT && path_parent(parent, sizeof(parent), dir) == 0
           && access(parent, W_OK | X_OK) == 0;
}

/*!
 * @brief Tell whether the SRP verifier store is private to the calling user and
 * holds the verifier of that user
 *
 * The store has to pass owned_private_dir. The verifier file has the numeric
 * uid as its name, as the SRP UAM expects. It has to be a regular file with
 * 1 link that the user owns and can read, with a mode that
 * srp_verifier_mode_is_safe accepts. Symbolic links fail, as the UAM refuses
 * them.
 *
 * @param[in] path  verifier store, or NULL for _PATH_AFPSRPVERIFIERPATH
 * @returns true when the store and the verifier pass
 */
static bool srp_verifier_store_is_private(const char *path)
{
    char verifier[MAXPATHLEN];
    struct stat st;
    int len;

    if (path == NULL) {
        path = _PATH_AFPSRPVERIFIERPATH;
    }

    if (!owned_private_dir(path)) {
        return false;
    }

    len = snprintf(verifier, sizeof(verifier), "%s/%ju", path,
                   (uintmax_t)getuid());

    if (len < 0 || (size_t)len >= sizeof(verifier)) {
        return false;
    }

    /* the UAM's own file rule, plus owner-readable: a verifier the UAM cannot
     * open would fail every login instead of this start */
    return lstat(verifier, &st) == 0 && S_ISREG(st.st_mode) && st.st_nlink == 1
           && st.st_uid == getuid() && srp_verifier_mode_is_safe(st.st_mode)
           && (st.st_mode & S_IRUSR) != 0;
}

/*!
 * @brief Refuse a single-user configuration that the mode cannot serve
 *
 * The call runs after the volumes load and before daemonize, so every message
 * reaches the terminal of the operator. It refuses AppleTalk, afpstats,
 * forced identities, an admin group, and a uam list other than uams_srp.so. The
 * config file and the SRP verifier store must be private to the user, and
 * [Homes] is out.
 *
 * Every configured volume must load and use the sqlite CNID backend, and a
 * Spotlight volume no backend other than cnid. The user must reach each
 * volume, and its CNID directory must be another directory that the user owns
 * or can create. A port below 1024 only warns.
 *
 * @param[in] volumes_loaded  result of the volume load
 * @returns 0 when every clause passes, -1 after the message of the first
 *          failure
 */
static int validate_singleuser_config(bool volumes_loaded)
{
    if (obj.options.flags & OPTION_DDP) {
        fprintf(stderr, "netatalk: --single-user does not support AppleTalk.\n");
        return -1;
    }

    if (obj.options.flags & OPTION_AFPSTATS) {
        fprintf(stderr, "netatalk: --single-user does not support afpstats.\n");
        return -1;
    }

    if (obj.options.force_user || obj.options.force_group
            || obj.options.admingid != 0) {
        fprintf(stderr,
                "netatalk: --single-user does not support admin or forced identities.\n");
        return -1;
    }

    if (!srp_is_the_only_uam(obj.options.uamlist)) {
        fprintf(stderr,
                "netatalk: --single-user requires 'uam list = uams_srp.so'.\n");
        return -1;
    }

    /* afpd re-parses -F on its own; a directory someone else can write into
     * lets the file be swapped between this validation and that parse. */
    if (!dir_is_private(obj.options.configfile, 0022, false)) {
        fprintf(stderr,
                "netatalk: --single-user requires a configuration file owned by the calling user, not writable by group or others, that lives in a mode-0700 directory owned by that user.\n");
        return -1;
    }

    if (!srp_verifier_store_is_private(obj.options.srpverifierpath)) {
        fprintf(stderr,
                "netatalk: --single-user requires 'srp verifier path' to be a mode-0700 directory owned by the calling user containing that user's verifier.\n");
        return -1;
    }

    if (iniparser_find_entry(obj.iniconfig, INISEC_HOMES)) {
        fprintf(stderr, "netatalk: --single-user does not support [Homes] volumes.\n");
        return -1;
    }

    if (obj.vols_skipped != 0) {
        fprintf(stderr,
                "netatalk: --single-user requires every configured volume to load: %d skipped, the log says why.\n",
                obj.vols_skipped);
        return -1;
    }

    if (!volumes_loaded || getvolumes() == NULL) {
        fprintf(stderr,
                "netatalk: --single-user requires at least one static volume.\n");
        return -1;
    }

    for (const struct vol *vol = getvolumes(); vol != NULL; vol = vol->v_next) {
        if (vol->v_cnidscheme == NULL || strcasecmp(vol->v_cnidscheme, "sqlite") != 0) {
            fprintf(stderr, "netatalk: --single-user volume '%s' must use sqlite CNID.\n",
                    vol->v_localname);
            return -1;
        }

        /* the cnid backend searches the volume's own sqlite database; the
         * others keep their index under system directories */
        if ((vol->v_flags & AFPVOL_SPOTLIGHT) && vol->v_sl_backend_name != NULL
                && strcasecmp(vol->v_sl_backend_name, "cnid") != 0) {
            fprintf(stderr,
                    "netatalk: --single-user supports only 'spotlight backend = cnid'; volume '%s' uses '%s'.\n",
                    vol->v_localname, vol->v_sl_backend_name);
            return -1;
        }

        int want = R_OK | X_OK | ((vol->v_flags & AFPVOL_RO) ? 0 : W_OK);

        if (access(vol->v_path, want) != 0) {
            fprintf(stderr,
                    "netatalk: --single-user cannot access volume '%s' as the calling user.\n",
                    vol->v_localname);
            return -1;
        }

        /* a volume root that is its own CNID directory would be closed to
         * everyone else by the owner-only rule; v_path is canonical, the
         * configured dbpath is not */
        char *dbreal = access(vol->v_dbpath, F_OK) == 0
                       ? realpath_safe(vol->v_dbpath) : NULL;
        bool nested = dbreal != NULL && strcmp(dbreal, vol->v_path) == 0;
        free(dbreal);

        if (nested) {
            fprintf(stderr,
                    "netatalk: --single-user volume '%s' names itself as its "
                    "CNID directory ('vol dbpath' %s): the mode would keep the "
                    "share owner-only.\n",
                    vol->v_localname, vol->v_dbpath);
            return -1;
        }

        /* CNID state is created at first mount, as the serving user; a
         * directory that cannot come into being, or that another account
         * owns, is refused here, with the fix, rather than at every mount by
         * the backend. */
        if (!dbpath_is_creatable(vol->v_dbpath)) {
            fprintf(stderr,
                    "netatalk: --single-user requires the CNID directory of volume '%s' (%s) to be a writable directory the calling user owns, not a symbolic link, or one the calling user can create: set 'vol dbpath' to a directory the calling user owns.\n",
                    vol->v_localname, vol->v_dbpath);
            return -1;
        }
    }

    {
        int port = atoi(obj.options.port);

        if (port > 0 && port < 1024) {
            fprintf(stderr,
                    "netatalk: warning: afp port %d is below 1024 (548 is the default "
                    "when 'afp port' is not set, and a port in 'afp listen' takes "
                    "precedence); binding it may need privileges this mode does not "
                    "have. Set 'afp port = 5548' or another port above 1023 if afpd "
                    "fails to start.\n", port);
        }
    }

    return 0;
}

#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH
/*!
 * @brief Create a directory and its missing parents, as mkdir -p does
 *
 * Every new component gets mode under the umask, and an existing one stays as
 * it is.
 *
 * @param[in] path  directory to create
 * @param[in] mode  mode of the new directories
 * @returns 0, or -1 with errno set, ENAMETOOLONG for a path of PATH_MAX bytes
 *          or more
 */
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

/*!
 * @brief Open a dconf input file for writing with mode 0644, whatever the umask
 *
 * The call truncates the file, and it sets the mode of an existing file too.
 *
 * @param[in] path  file to open
 * @returns the stream, or NULL with errno set
 */
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

/*!
 * @brief Point the localsearch indexer at the Spotlight volumes through a dconf
 * database
 *
 * The keyfile lists the path of every Spotlight volume as a recursive index
 * directory, and the locks file pins both index settings. A path with a
 * single quote stays out with a warning, since it would end the quoted
 * string. The call then runs dconf compile and waits for it.
 *
 * @returns 0, or -1 without volumes or when a write, the fork, or dconf compile
 *          fails
 */
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

/*!
 * @brief Log a fatal libev error and end the service controller
 *
 * @param[in] msg  error text from libev
 */
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
    kill_childs(SIGTERM, &afpd_pid, &dbus_pid, NULL);
}

/*! SIGQUIT implementation */
static void sigquit_impl(void)
{
    LOG(log_note, logtype_afpd, "Exiting on SIGQUIT");
    kill_childs(SIGQUIT, &afpd_pid, &dbus_pid, NULL);
}

/*! SIGHUP implementation */
static void sighup_impl(void)
{
    if (obj.cmdlineflags & OPTION_SINGLEUSER) {
        LOG(log_note, logtype_afpd,
            "Ignoring SIGHUP: configuration reload is disabled in single-user mode");
        return;
    }

    LOG(log_note, logtype_afpd,
        "Received SIGHUP, sending all processes signal to reload config");
    load_afp_conf_vols(&obj, LV_ALL);

    if (!(obj.options.flags & OPTION_NOZEROCONF)) {
        zeroconf_deregister();
        zeroconf_register(&obj);
        LOG(log_note, logtype_default, "Re-registered with Zeroconf");
    }

    kill_childs(SIGHUP, &afpd_pid, NULL);
}

/*!
 * @brief Reap the exited children and mark their services for a restart
 *
 * An exited afpd or dbus gets NETATALK_SRV_ERROR, and the timer restarts it
 * outside a shutdown.
 *
 * @returns true during a shutdown once neither service runs
 */
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
        } else if (pid == dbus_pid) {
            dbus_pid = NETATALK_SRV_ERROR;
        } else {
            LOG(log_error, logtype_afpd, "Bad pid: %d", pid);
        }
    }

    return in_shutdown
           && !service_running(afpd_pid)
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

/*!
 * @brief End the event loop when the shutdown grace time runs out
 *
 * @param[in] ev_loop  event loop, unused
 * @param[in] w        grace timer, unused
 * @param[in] revents  events, unused
 */
static void kill_timer_cb(struct ev_loop *ev_loop _U_, ev_timer *w _U_,
                          int revents _U_)
{
    ev_break(loop, EVBREAK_ALL);
}

/*!
 * @brief Start the shutdown on SIGTERM, with KILL_GRACETIME seconds for the
 * children to exit
 *
 * The call sends SIGTERM to afpd and dbus. It stops the SIGTERM, SIGQUIT,
 * and SIGHUP watchers and the restart timer, so only SIGCHLD and the grace
 * timer remain. A SIGTERM during a shutdown does nothing.
 *
 * @param[in] ev_loop  event loop, unused
 * @param[in] w        signal watcher, unused
 * @param[in] revents  events, unused
 */
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

/*!
 * @brief Pass SIGQUIT on to afpd and dbus
 *
 * @param[in] ev_loop  event loop, unused
 * @param[in] w        signal watcher, unused
 * @param[in] revents  events, unused
 */
static void sigquit_cb(struct ev_loop *ev_loop _U_, ev_signal *w _U_,
                       int revents _U_)
{
    sigquit_impl();
}

/*!
 * @brief Reload the volumes on SIGHUP, register them with Zeroconf again, and
 * pass the signal on to afpd
 *
 * A single-user server ignores SIGHUP.
 *
 * @param[in] ev_loop  event loop, unused
 * @param[in] w        signal watcher, unused
 * @param[in] revents  events, unused
 */
static void sighup_cb(struct ev_loop *ev_loop _U_, ev_signal *w _U_,
                      int revents _U_)
{
    sighup_impl();
}

/*!
 * @brief Reap the children on SIGCHLD, and end the event loop once a shutdown
 * has no service left
 *
 * @param[in] ev_loop  event loop, unused
 * @param[in] w        signal watcher, unused
 * @param[in] revents  events, unused
 */
static void sigchld_cb(struct ev_loop *ev_loop _U_, ev_signal *w _U_,
                       int revents _U_)
{
    if (sigchld_impl()) {
        ev_break(loop, EVBREAK_ALL);
    }
}

/*!
 * @brief Restart afpd and dbus when they need it, on the 1-second timer
 *
 * The timer does nothing during a shutdown.
 *
 * @param[in] ev_loop  event loop, unused
 * @param[in] w        restart timer, unused
 * @param[in] revents  events, unused
 */
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

/*!
 * @brief Send a signal to the services whose pid slots follow, up to a NULL
 *
 * A slot that holds NETATALK_SRV_ERROR or NETATALK_SRV_OPTIONAL gets no
 * signal.
 *
 * @param[in] sig  signal number
 * @param[in] ...  pid_t pointers, then NULL
 */
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

/*!
 * @brief Remove the lock file and exit with a status
 *
 * @param[in] ret  exit status
 */
static void netatalk_exit(int ret)
{
    server_unlock(lockfile_path);
    exit(ret);
}

/*!
 * @brief Fork and run a program with a NULL-terminated list of arguments
 *
 * The path also serves as argv[0]. A failed exec logs and exits the child
 * with 1.
 *
 * @param[in] path  program to run
 * @param[in] ...   char pointer arguments, then NULL
 * @returns the pid of the child, or -1 when fork fails
 */
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

/*!
 * @brief Start afpd on the controller's configuration file
 *
 * A single-user controller passes the mode on with -u, so afpd applies the
 * uid check in login().
 *
 * @returns the child pid, or NETATALK_SRV_ERROR
 */
static pid_t run_afpd(void)
{
    if (obj.cmdlineflags & OPTION_SINGLEUSER) {
        return run_process(_PATH_AFPD, "-d", "-u", "-F", obj.options.configfile, NULL);
    }

    return run_process(_PATH_AFPD, "-d", "-F", obj.options.configfile, NULL);
}

/*!
 * @brief Print the version, the license note, and the Zeroconf and Spotlight
 * backends of the build
 */
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

/*!
 * @brief Print the paths of afp.conf, afpd, the dbus daemon and its
 * configuration, and the lock file
 *
 * The dbus lines need the localsearch backend, and Solaris builds have no
 * lock file line.
 */
static void show_netatalk_paths(void)
{
    printf("                  afp.conf:\t%s\n", _PATH_CONFDIR "afp.conf");
    printf("                      afpd:\t%s\n", _PATH_AFPD);
#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH
    printf("               dbus-daemon:\t%s\n", DBUS_DAEMON_PATH);
    printf("         dbus-session.conf:\t%s\n", _PATH_CONFDIR "dbus-session.conf");
#endif
#ifndef SOLARIS
    printf("        netatalk lock file:\t%s\n", PATH_NETATALK_LOCK);
#endif
}

/*!
 * @brief Print the command line forms of netatalk
 */
static void usage(void)
{
    printf("usage: netatalk [-d] [-F configfile] \n");
    printf("       netatalk -u -P pidfile [-d] [-F configfile] \n");
    printf("       netatalk -d \n");
    printf("       netatalk -v|-V \n");
}

#ifdef SPOTLIGHT_BACKEND_LOCALSEARCH
/*!
 * @brief Tell whether a Spotlight volume names the localsearch backend
 *
 * @returns true when one volume does, false otherwise
 */
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

/*!
 * @brief Run the netatalk service controller, which starts afpd and the
 * Spotlight dbus daemon and restarts them
 *
 * As root the controller serves every user. With --single-user it runs as one
 * user other than root, and it needs -P with a pid file in a private
 * directory. It validates the configuration before it daemonizes.
 *
 * The controller loads the volumes, daemonizes without -d, writes the lock
 * file, and starts afpd. A localsearch volume also starts dbus and writes the
 * indexer configuration. The controller registers with Zeroconf and runs the
 * event loop until a SIGTERM shutdown ends. Any service left after the grace
 * time gets SIGKILL.
 *
 * @param[in] argc  argument count
 * @param[in] argv  argument vector
 * @returns no value, since netatalk_exit ends the process with the status of
 *          the event loop
 */
int main(int argc, char **argv)
{
    static const struct option long_options[] = {
        { "debug",        no_argument,       NULL, 'd' },
        { "config",       required_argument, NULL, 'F' },
        { "pidfile",      required_argument, NULL, 'P' },
        { "single-user",  no_argument,       NULL, 'u' },
        { "version",      no_argument,       NULL, 'v' },
        { NULL,            0,                 NULL,  0  }
    };
    int c, ret, debug = 0, singleuser = 0;
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

            /* daemonize() chdir()s to "/" and run_afpd() passes this string
             * on as given, so a relative path is resolved here */
            if ((obj.cmdlineconfigfile = realpath_safe(optarg)) == NULL) {
                fprintf(stderr, "netatalk: cannot resolve config file '%s': %s\n",
                        optarg, strerror(errno));
                exit(EXITERR_CONF);
            }

            break;

        case 'P':
            pidfile = optarg;
            break;

        case 'u':
            singleuser = 1;
            obj.cmdlineflags |= OPTION_SINGLEUSER;
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

    if (singleuser) {
        if (getuid() == 0 || geteuid() == 0) {
            fprintf(stderr,
                    "netatalk: --single-user must be started by a non-root user.\n");
            exit(EXIT_FAILURE);
        }

        if (pidfile == NULL || pidfile[0] == '\0') {
            fprintf(stderr,
                    "netatalk: --single-user requires -P with a PID file in private user state.\n");
            exit(EXIT_FAILURE);
        }

        lockfile_path = pidfile;

        if (lockfile_path[0] != '/') {
            fprintf(stderr,
                    "netatalk: --single-user requires -P with an absolute PID file path.\n");
            exit(EXIT_FAILURE);
        }

        if (!dir_is_private(lockfile_path, 0, true)) {
            fprintf(stderr,
                    "netatalk: --single-user requires -P in a mode-0700 directory owned by the calling user.\n");
            exit(EXIT_FAILURE);
        }
    } else if (getuid() != 0 || geteuid() != 0) {
        fprintf(stderr,
                "netatalk: must run as root; use --single-user (-u) for a single-user AFP server.\n");
        exit(EXIT_FAILURE);
    } else if (pidfile != NULL) {
        fprintf(stderr, "netatalk: -P is only valid together with --single-user.\n");
        exit(EXIT_FAILURE);
    }

    if (check_lockfile("netatalk", lockfile_path) != 0) {
        exit(EXITERR_SYS);
    }

    if (afp_config_parse(&obj, "netatalk") != 0) {
        if (singleuser) {
            fprintf(stderr,
                    "netatalk: --single-user could not load the configuration; the log has the reason.\n");
        }

        exit(EXITERR_CONF);
    }

    bool volumes_loaded = (load_afp_conf_vols(&obj, LV_ALL) == 0);

    if (singleuser && validate_singleuser_config(volumes_loaded) != 0) {
        exit(EXITERR_CONF);
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

    if (service_running(afpd_pid) || service_running(dbus_pid)) {
        if (service_running(afpd_pid)) {
            LOG(log_error, logtype_afpd, "AFP service did not shutdown, killing it");
        }

        if (service_running(dbus_pid)) {
            LOG(log_error, logtype_afpd, "DBUS session daemon still running, killing it");
        }

        kill_childs(SIGKILL, &afpd_pid, &dbus_pid, NULL);
    }

    LOG(log_note, logtype_afpd, "Netatalk AFP server exiting");
    netatalk_exit(ret);
}
