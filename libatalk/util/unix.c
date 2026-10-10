/*
  Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>
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

/*!
 * @file
 * Netatalk utility functions: unix
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <poll.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <atalk/acl.h>
#include <atalk/adouble.h>
#include <atalk/afp.h>
#include <atalk/bstrlib_compat.h>
#include <atalk/compat.h>
#include <atalk/ea.h>
#include <atalk/logger.h>
#include <atalk/uam.h>
#include <atalk/unix.h>
#include <atalk/util.h>
#include <atalk/vfs.h>

/*! close all FDs >= a specified value */
static void closeall(int fd)
{
    int fdlimit = sysconf(_SC_OPEN_MAX);

    while (fd < fdlimit) {
        close(fd++);
    }
}

/*!
 * @brief Fork, exit parent, setsid(), chdir("/"), close all fds
 *
 * @returns -1 on failure, but you can't do much except exit in that case
 * since we may already have forked
 */
int daemonize(void)
{
    switch (fork()) {
    case 0:
        break;

    case -1:
        return -1;

    default:
        _exit(0);
    }

    if (setsid() < 0) {
        return -1;
    }

    switch (fork()) {
    case 0:
        break;

    case -1:
        return -1;

    default:
        _exit(0);
    }

    if (chdir("/") < 0) {
        LOG(log_error, logtype_default, "Can't chdir(/): %s", strerror(errno));
        return -1;
    }

    /* the logger's descriptors go with the rest; a process that logged
     * before daemonizing gets them back once 0-2 are /dev/null */
    log_close_all();
    closeall(0);
    open("/dev/null", O_RDWR);

    if (dup(0) < 0) {
        LOG(log_error, logtype_default, "Can't dup /dev/null first time: %s",
            strerror(errno));
        return -1;
    }

    if (dup(0) < 0) {
        LOG(log_error, logtype_default, "Can't dup /dev/null second time: %s",
            strerror(errno));
        return -1;
    }

    log_reopen();
    return 0;
}

#define FD_POLL_CHUNK 1024

/*!
 * @brief Count open descriptors in [base, base + n) with one fcntl(2) each
 *
 * @param[in] base  first descriptor
 * @param[in] n     descriptors to probe
 *
 * @returns number of open descriptors in the range
 */
static int count_fds_probed(int base, int n)
{
    int open_fds = 0;

    for (int fd = base; fd < base + n; fd++) {
        if (fcntl(fd, F_GETFD) != -1) {
            open_fds++;
        }
    }

    return open_fds;
}

/*!
 * @brief Count open descriptors below @p max, FD_POLL_CHUNK per poll(2)
 *
 * poll() marks a closed descriptor POLLNVAL without using a descriptor of
 * its own.  macOS poll() rejects device descriptors and DragonFly's
 * kqueue-based poll() miscounts them, so both probe instead.
 *
 * @param[in] max  number of descriptors to scan from 0
 *
 * @returns number of open descriptors below @p max
 */
static int count_fds_polled(int max)
{
#if defined(__APPLE__) || defined(__DragonFly__)
    return count_fds_probed(0, max);
#else
    struct pollfd pfd[FD_POLL_CHUNK];
    int open_fds = 0;

    for (int base = 0; base < max; base += FD_POLL_CHUNK) {
        int n = (max - base < FD_POLL_CHUNK) ? max - base : FD_POLL_CHUNK;

        for (int i = 0; i < n; i++) {
            pfd[i].fd = base + i;
            pfd[i].events = 0;
            pfd[i].revents = 0;
        }

        if (poll(pfd, (nfds_t)n, 0) < 0) {
            open_fds += count_fds_probed(base, n);
            continue;
        }

        for (int i = 0; i < n; i++) {
            if (!(pfd[i].revents & POLLNVAL)) {
                open_fds++;
            }
        }
    }

    return open_fds;
#endif
}

/*!
 * @brief Count the descriptors this process has open
 *
 * Lists /proc/self/fd where procfs enumerates every descriptor; otherwise,
 * or when no descriptor is free for the listing, scans the descriptors
 * below the limit with poll(2).  Meant for logging on rare events.
 *
 * @param[out] limit  set to getdtablesize() unless NULL
 *
 * @returns number of open descriptors
 */
int count_open_fds(int *limit)
{
    int max = getdtablesize();

    if (limit != NULL) {
        *limit = max;
    }

#if defined(__linux__) || defined(__sun)
    DIR *dp = opendir("/proc/self/fd");

    if (dp != NULL) {
        const struct dirent *de;
        int open_fds = 0;

        while ((de = readdir(dp)) != NULL) {
            if (de->d_name[0] != '.') {
                open_fds++;
            }
        }

        closedir(dp);
        /* the listing includes the descriptor opendir() holds */
        return open_fds - 1;
    }

#endif
    return count_fds_polled(max);
}

/*!
 * @brief Log a failed PAM step with the errno it left and the fds in use
 *
 * The descriptor count tells a module that cannot open its files from a
 * wrong password.  Takes pam_strerror() text, never a handle, which a
 * failed pam_start() leaves undefined.  Both afpd and papd load the PAM
 * UAMs.
 *
 * @param[in] level    log level of the line
 * @param[in] uam      UAM name that leads the log line
 * @param[in] user     user being authenticated
 * @param[in] step     PAM function that failed
 * @param[in] errtext  pam_strerror() text for @p code
 * @param[in] code     PAM return code
 * @param[in] err      errno as the failed step left it
 */
void uam_log_pam_failure(enum loglevels level, const char *uam,
                         const char *user, const char *step,
                         const char *errtext, int code, int err)
{
    int fdlimit;
    int fds;

    /* LOG()'s own level test, so a filtered line costs no count */
    if (level > type_configs[logtype_uams].level) {
        return;
    }

    fds = count_open_fds(&fdlimit);
    LOG(level, logtype_uams,
        "%s: PAM_Error: %s for %s: %s (PAM %d, errno %d: %s), fds %d open of %d",
        uam, step, user ? user : "?", errtext, code, err, strerror(err), fds,
        fdlimit);
}

static const int usage_warn_pct[] = { 50, 75, 90 };
#define USAGE_LEVELS ((int)(sizeof(usage_warn_pct) / sizeof(usage_warn_pct[0])))

/*!
 * @brief Count an event, reporting its 1st, 2nd, 4th, 8th... occurrence
 *
 * @param[in,out] count  occurrences so far
 *
 * @returns true when this occurrence should be logged
 */
bool log_backoff(unsigned int *count)
{
    unsigned int n = ++*count;
    return (n & (n - 1)) == 0;
}

/*!
 * @brief Track usage against the 50, 75 and 90 percent warning thresholds
 *
 * Raises @p level past each threshold @p used has reached, and lowers it
 * once @p used is ten points below the highest one reached, so a threshold
 * is reported once per climb rather than on every crossing.
 *
 * @param[in,out] level  thresholds reached so far, 0 to 3
 * @param[in]     used   current usage
 * @param[in]     limit  capacity the thresholds are percentages of
 *
 * @returns true when @p level rose
 */
bool usage_level_update(int *level, int64_t used, int64_t limit)
{
    bool rose = false;

    if (limit <= 0) {
        return false;
    }

    while (*level < USAGE_LEVELS
            && used * 100 >= usage_warn_pct[*level] * limit) {
        (*level)++;
        rose = true;
    }

    if (!rose && *level > 0
            && used * 100 < (usage_warn_pct[*level - 1] - 10) * limit) {
        (*level)--;
    }

    return rose;
}

/*!
 * @brief Threshold, in percent, that @p level has reached
 *
 * @param[in] level  a level raised by usage_level_update(), 1 to 3
 *
 * @returns the threshold in percent
 */
int usage_level_pct(int level)
{
    return usage_warn_pct[level - 1];
}

static uid_t saved_uid = -1;
static int root_nesting = 0;

/*!
 * seteuid(0) and back, if either fails and panic != 0 we PANIC.
 * Nesting is tracked: only the outermost become_root() elevates privileges
 * and only the matching unbecome_root() drops them.
 */
void become_root(void)
{
    if (getuid() == 0) {
        int outermost = (root_nesting == 0);
        root_nesting++;

        if (outermost) {
            saved_uid = geteuid();

            if (seteuid(0) != 0) {
                AFP_PANIC("Can't seteuid(0)");
            }
        }
    }
}

void unbecome_root(void)
{
    if (getuid() == 0) {
        if (root_nesting <= 0) {
            AFP_PANIC("unbecome_root: nesting underflow");
        }

        root_nesting--;

        if (root_nesting == 0) {
            if (saved_uid == -1 || seteuid(saved_uid) < 0) {
                AFP_PANIC("Can't seteuid back");
            }

            saved_uid = -1;
        }
    }
}

/*!
 * @brief get cwd in static buffer
 *
 * @returns pointer to path or pointer to error messages on error
 */
const char *getcwdpath(void)
{
    static char cwd[MAXPATHLEN + 1];
    char *p;

    if ((p = getcwd(cwd, MAXPATHLEN)) != NULL) {
        return p;
    } else {
        return strerror(errno);
    }
}

/*!
 * @brief Request absolute path
 *
 * @returns Absolute filesystem path to object
 */
const char *fullpathname(const char *name)
{
    static char wd[MAXPATHLEN + 1];

    if (name[0] == '/') {
        return name;
    }

    if (getcwd(wd, MAXPATHLEN)) {
        strlcat(wd, "/", MAXPATHLEN);
        strlcat(wd, name, MAXPATHLEN);
    } else {
        strlcpy(wd, name, MAXPATHLEN);
    }

    return wd;
}

/*!
 * @brief Takes a buffer with a path, strips slashs, returns basename
 *
 * @param p (rw) path
 *
 * path may be
 * @code
 *   "[/][dir/[...]]file"
 * @endcode
 * or
 * @code
 *   "[/][dir/[...]]dir/[/]"
 * @endcode
 * Result is "file" or "dir"
 *
 * @returns pointer to basename in path buffer, buffer is possibly modified
 */
char *stripped_slashes_basename(char *p)
{
    int i = strlen(p) - 1;

    while (i > 0 && p[i] == '/') {
        p[i--] = 0;
    }

    return strrchr(p, '/') ? strrchr(p, '/') + 1 : p;
}

/*!
 * @brief Find a suitable temporary directory for Netatalk.
 *
 * Creates a subdirectory for current gid if it doesn't exist.
 * The result should be copied immediately
 * as it may be overwritten by a subsequent call.
 */
const char *tmpdir(void)
{
    static char netatalk_tmpdir[MAXPATHLEN + 1];
    char *systmp;
    struct stat st;
    mode_t oldmask;
    int tmpfd, dirfd;
    char dirname[64];

    if ((systmp = getenv("TMPDIR")))
        ;
    else {
        systmp = "/tmp";
    }

    if ((tmpfd = open(systmp, O_RDONLY | O_DIRECTORY)) == -1) {
        LOG(log_error, logtype_default, "tmpdir: cannot open %s: %s",
            systmp, strerror(errno));
        return systmp;
    }

    snprintf(dirname, sizeof(dirname), "netatalk-%u", getpid());
    snprintf(netatalk_tmpdir, MAXPATHLEN, "%s/%s", systmp, dirname);
    oldmask = umask(0077); //NOSONAR: Restrict temporary directory access to its owner; restore umask below.

    if (mkdirat(tmpfd, dirname, 0700) != 0) {
        if (errno != EEXIST) {
            LOG(log_error, logtype_default, "tmpdir: failed to create %s: %s",
                netatalk_tmpdir, strerror(errno));
            umask(oldmask);
            close(tmpfd);
            return systmp;
        }
    } else {
        LOG(log_debug, logtype_default, "tmpdir: created directory %s",
            netatalk_tmpdir);
        umask(oldmask);
        close(tmpfd);
        return netatalk_tmpdir;
    }

    if (fstatat(tmpfd, dirname, &st, 0) != 0) {
        LOG(log_error, logtype_default, "tmpdir: error accessing %s: %s",
            netatalk_tmpdir, strerror(errno));
        umask(oldmask);
        close(tmpfd);
        return systmp;
    }

    umask(oldmask);

    if ((dirfd = openat(tmpfd, dirname, O_RDONLY | O_DIRECTORY)) == -1) {
        LOG(log_error, logtype_default, "tmpdir: cannot open %s: %s",
            netatalk_tmpdir, strerror(errno));
        close(tmpfd);
        return systmp;
    }

    if (fstat(dirfd, &st) != 0) {
        LOG(log_error, logtype_default, "tmpdir: fstat failed on %s: %s",
            netatalk_tmpdir, strerror(errno));
        close(dirfd);
        close(tmpfd);
        return systmp;
    }

    if (!S_ISDIR(st.st_mode)) {
        LOG(log_error, logtype_default, "tmpdir: %s exists but is not a directory",
            netatalk_tmpdir);
        close(dirfd);
        close(tmpfd);
        return systmp;
    }

    if ((st.st_mode & 0777) != 0700) {
        LOG(log_warning, logtype_default, "tmpdir: fixing permissions on %s",
            netatalk_tmpdir);

        if (fchmod(dirfd, 0700) != 0) {
            LOG(log_error, logtype_default, "tmpdir: failed to chmod %s: %s",
                netatalk_tmpdir, strerror(errno));
        }
    }

    if (st.st_uid != getuid()) {
        LOG(log_warning, logtype_default, "tmpdir: %s not owned by current user",
            netatalk_tmpdir);
        close(dirfd);
        close(tmpfd);
        return systmp;
    }

    close(dirfd);
    close(tmpfd);
    return netatalk_tmpdir;
}

/*********************************************************************************
 * chdir(), chmod(), chown(), stat() wrappers taking an additional option.
 * Currently the only used options are O_NOFOLLOW, used to switch between symlink
 * behaviour, and O_NETATALK_ACL for ochmod() indicating chmod_acl() shall be
 * called which does special ACL handling depending on the filesytem
 *********************************************************************************/

int ostat(const char *path, struct stat *buf, int options)
{
    if (options & O_NOFOLLOW) {
        return lstat(path, buf);
    } else {
        return stat(path, buf);
    }
}

int ochown(const char *path, uid_t owner, gid_t group, int options)
{
    if (options & O_NOFOLLOW) {
        return lchown(path, owner, group);
    } else {
        return chown(path, owner, group);
    }
}

/*!
 * @brief chmod() wrapper for symlink and ACL handling
 *
 * @param[in] path       path
 * @param[in] mode       requested mode
 * @param[in] st         stat() of path or NULL
 * @param[in] options    O_NOFOLLOW | O_NETATALK_ACL
 *
 * Option descriptions:
 *
 * - O_NOFOLLOW: don't chmod() symlinks, do nothing, return 0
 * - O_NETATALK_ACL: call chmod_acl() instead of chmod()
 * - O_IGNORE: ignore chmod() request, directly return 0
 */
int ochmod(char *path, mode_t mode, const struct stat *st, int options)
{
    struct stat sb;

    if (options & O_IGNORE) {
        return 0;
    }

    if (!st) {
        if (lstat(path, &sb) != 0) {
            return -1;
        }

        st = &sb;
    }

    if (options & O_NOFOLLOW)
        if (S_ISLNK(st->st_mode)) {
            return 0;
        }

    if (options & O_NETATALK_ACL) {
        return chmod_acl(path, mode);
    } else {
        return chmod(path, mode);
    }
}

/*!
 * @brief ostat/fsstatat multiplexer
 *
 * ostatat mulitplexes ostat and fstatat.
 *
 * @param[in] dirfd    -1 gives AT_FDCWD
 * @param[in] path     pathname
 * @param[in,out] st   pointer to struct stat
 * @param[in] options  file options
 */
int ostatat(int dirfd, const char *path, struct stat *st, int options)
{
    if (dirfd == -1) {
        dirfd = AT_FDCWD;
    }

    return fstatat(dirfd, path, st,
                   (options & O_NOFOLLOW) ? AT_SYMLINK_NOFOLLOW : 0);
}

/*!
 * @brief symlink safe chdir replacement
 *
 * Only chdirs to dir if it doesn't contain symlinks or if symlink checking
 * is disabled
 *
 * @returns 1 if a path element is a symlink, 0 otherwise, -1 on syserror
 */
int ochdir(const char *dir, int options)
{
    char buf[MAXPATHLEN + 1];
    char cwd[MAXPATHLEN + 1];
    char *test;
    int  i;

    if (!(options & O_NOFOLLOW)) {
        return chdir(dir);
    }

    /*
     dir is a canonical path (without "../" "./" "//" )
     but may end with a /
    */
    *cwd = 0;

    if (*dir != '/') {
        if (getcwd(cwd, MAXPATHLEN) == NULL) {
            return -1;
        }
    }

    if (chdir(dir) != 0) {
        return -1;
    }

    /*
     * Cases:
     * chdir request   | realpath result | ret
     * (after getwcwd) |                 |
     * =======================================
     * /a/b/.          | /a/b            | 0
     * /a/b/.          | /c              | 1
     * /a/b/.          | /c/d/e/f        | 1
     */
    if (getcwd(buf, MAXPATHLEN) == NULL) {
        return 1;
    }

    i = 0;

    if (*cwd) {
        /* relative path requested,
         * Same directory?
        */
        for (; cwd[i]; i++) {
            if (buf[i] != cwd[i]) {
                return 1;
            }
        }

        if (buf[i]) {
            if (buf[i] != '/') {
                return 1;
            }

            i++;
        }
    }

    test = &buf[i];

    for (i = 0; test[i]; i++) {
        if (test[i] != dir[i]) {
            return 1;
        }
    }

    /* trailing '/' ? */
    if (!dir[i]) {
        return 0;
    }

    if (dir[i] != '/') {
        return 1;
    }

    i++;

    if (dir[i]) {
        return 1;
    }

    return 0;
}

/*!
 * Store n random bytes an buf
 */
void randombytes(void *buf, int n)
{
    char *p = (char *)buf;
    int fd, i;
    struct timeval tv;

    if ((fd = open("/dev/urandom", O_RDONLY)) != -1) {
        /* generate from /dev/urandom */
        if (read(fd, buf, n) != n) {
            close(fd);
            fd = -1;
        } else {
            close(fd);
            /* fd now != -1, so srandom wont be called below */
        }
    }

    if (fd == -1) {
        gettimeofday(&tv, NULL);
        srandom((unsigned int)tv.tv_usec);

        for (i = 0; i < n; i++) {
            p[i] = random() & 0xFF;
        }
    }

    return;
}

int gmem(gid_t gid, int ngroups, gid_t *groups)
{
    int		i;

    for (i = 0; i < ngroups; i++) {
        if (groups[i] == gid) {
            return 1;
        }
    }

    return 0;
}

/*!
 * realpath() replacement that always allocates storage for returned path
 */
char *realpath_safe(const char *path)
{
    char *resolved_path;
#ifdef REALPATH_TAKES_NULL

    if ((resolved_path = realpath(path, NULL)) == NULL) {
        LOG(log_warning, logtype_afpd, "realpath() cannot resolve path \"%s\"", path);
        return NULL;
    }

    return resolved_path;
#else

    if ((resolved_path = malloc(MAXPATHLEN + 1)) == NULL) {
        return NULL;
    }

    if (realpath(path, resolved_path) == NULL) {
        free(resolved_path);
        LOG(log_warning, logtype_afpd, "realpath() cannot resolve path \"%s\"", path);
        return NULL;
    }

    /* Safe some memory */
    char *tmp;

    if ((tmp = strdup(resolved_path)) == NULL) {
        free(resolved_path);
        return NULL;
    }

    free(resolved_path);
    resolved_path = tmp;
    return resolved_path;
#endif
}

/*!
 * @brief safe basename() replacement
 * @returns pointer to static buffer with basename of path
 */
const char *basename_safe(const char *path)
{
    static char buf[MAXPATHLEN + 1];
    strlcpy(buf, path, MAXPATHLEN);
    return basename(buf);
}

/*!
 * @brief extended strtok allows the quoted strings
 *
 * modified strtok.c in glibc 2.0.6
 */
char *strtok_quote(char *s, const char *delim)
{
    static char *olds = NULL;
    char *token;

    if (s == NULL) {
        s = olds;
    }

    /* Return NULL if no string to parse */
    if (s == NULL) {
        return NULL;
    }

    /* Scan leading delimiters.  */
    s += strspn(s, delim);

    if (*s == '\0') {
        olds = NULL;
        return NULL;
    }

    /* Find the end of the token.  */
    token = s;

    if (token[0] == '\"') {
        token++;
        s = strpbrk(token, "\"");
    } else {
        s = strpbrk(token, delim);
    }

    if (s == NULL) {
        /* This token finishes the string.  */
        olds = strchr(token, '\0');
    } else {
        /* Terminate the token and make OLDS point past it.  */
        *s = '\0';
        olds = s + 1;
    }

    return token;
}

int set_groups(AFPObj *obj, struct passwd *pwd)
{
    /* only root may set the group list; a server started by its user
     * already carries that user's groups */
    if (getuid() == 0 && initgroups(pwd->pw_name, pwd->pw_gid) < 0) {
        LOG(log_error, logtype_afpd, "initgroups(%s, %d): %s", pwd->pw_name,
            pwd->pw_gid, strerror(errno));
    }

    if ((obj->ngroups = getgroups(0, NULL)) < 0) {
        LOG(log_error, logtype_afpd, "login: %s getgroups: %s", pwd->pw_name,
            strerror(errno));
        return -1;
    }

    if (obj->groups) {
        free(obj->groups);
    }

    if (NULL == (obj->groups = calloc(obj->ngroups, sizeof(gid_t)))) {
        LOG(log_error, logtype_afpd, "login: calloc(%d) failed", obj->ngroups);
        return -1;
    }

    if ((obj->ngroups = getgroups(obj->ngroups, obj->groups)) < 0) {
        LOG(log_error, logtype_afpd, "login: %s getgroups: %s", pwd->pw_name,
            strerror(errno));
        return -1;
    }

    return 0;
}

#define GROUPSTR_BUFSIZE 1024
const char *print_groups(int ngroups, gid_t *groups)
{
    static char groupsstr[GROUPSTR_BUFSIZE];
    int i;
    char *s = groupsstr;

    if (ngroups == 0) {
        return "-";
    }

    for (i = 0; (i < ngroups) && (s < &groupsstr[GROUPSTR_BUFSIZE]); i++) {
        s += snprintf(s, &groupsstr[GROUPSTR_BUFSIZE] - s, " %u", groups[i]);
    }

    return groupsstr;
}
