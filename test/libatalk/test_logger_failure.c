/*
 * Tests for the logger's handling of a failed log-file write
 *
 * Copyright (c) 2026 Andy Lemin (andylemin)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <atalk/logger.h>

static int tests_run;
static int tests_failed;

/*! @brief Print one TAP result line */
static void report(int passed, const char *name)
{
    tests_run++;
    printf("%s %d - %s\n", passed ? "ok" : "not ok", tests_run, name);

    if (!passed) {
        tests_failed++;
    }
}

/*! @brief Whether the file at path holds first, and then second if given */
static int file_has(const char *path, const char *first, const char *second)
{
    char buf[8192];
    const char *at;
    ssize_t n;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        return 0;
    }

    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n < 0) {
        return 0;
    }

    buf[n] = '\0';
    at = strstr(buf, first);
    return at != NULL && (second == NULL || strstr(at, second) != NULL);
}

/*! @brief Whether every line in the file at path carries exactly one log prefix */
static int lines_whole(const char *path)
{
    char buf[8192];
    char *save;
    ssize_t n;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        return 0;
    }

    n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n < 0) {
        return 0;
    }

    buf[n] = '\0';

    for (const char *line = strtok_r(buf, "\n", &save); line != NULL;
            line = strtok_r(NULL, "\n", &save)) {
        /* "pid] {file:line}" opens every generated line once */
        const char *at = strstr(line, "] {");

        if (at == NULL || strstr(at + 3, "] {") != NULL) {
            return 0;
        }
    }

    return 1;
}

/*!
 * @brief Cap the process's file size @p room bytes past the log file's, or lift the cap
 *
 * With SIGXFSZ ignored, a write past RLIMIT_FSIZE fails with EFBIG, so the
 * cap makes writes to the log file fail and lifting it lets them succeed
 * again, on the same descriptor and the same file.  Room for part of a
 * line makes a write short.
 */
static int cap_file_size(int fd, off_t room, const struct rlimit *saved)
{
    struct rlimit cap = *saved;
    struct stat st;

    if (fd >= 0) {
        if (fstat(fd, &st) != 0) {
            return -1;
        }

        cap.rlim_cur = (rlim_t)(st.st_size + room);
    }

    return setrlimit(RLIMIT_FSIZE, &cap);
}

int main(void)
{
    /* in the working directory, the build tree under meson, not a shared /tmp */
    char dir[] = "logger_failure_XXXXXX";
    char path[PATH_MAX];
    char other[PATH_MAX];
    struct rlimit saved;
    struct stat st;
    int logfd;
    int ofd = -1;
    int stranger = -1;
    int stranger2 = -1;
    char away[PATH_MAX];
    int status = -1;
    pid_t child;
    int capped;
    int old;
    printf("TAP version 13\n");
    signal(SIGXFSZ, SIG_IGN);

    if (mkdtemp(dir) == NULL || getrlimit(RLIMIT_FSIZE, &saved) != 0) {
        printf("Bail out! no scratch directory or file size limit\n");
        return 1;
    }

    snprintf(path, sizeof(path), "%s/afpd.log", dir);
    snprintf(other, sizeof(other), "%s/other_XXXXXX", dir);
    setuplog("default:info", path, false);
    logfd = type_configs[logtype_default].fd;

    if (logfd < 0) {
        printf("Bail out! setuplog() opened no log file\n");
        return 1;
    }

    LOG(log_error, logtype_afpd, "line zero: written");

    /* writes fail while the name still names the open file */
    if (cap_file_size(logfd, 0, &saved) != 0) {
        printf("1..0 # SKIP RLIMIT_FSIZE cannot be capped here\n");
        unlink(path);
        rmdir(dir);
        return 0;
    }

    LOG(log_error, logtype_afpd, "line one: lost");

    /* some file systems, such as OpenBSD's sshfs, ignore RLIMIT_FSIZE */
    if (file_has(path, "line one: lost", NULL)) {
        cap_file_size(-1, 0, &saved);
        printf("1..0 # SKIP this file system does not enforce RLIMIT_FSIZE\n");
        unlink(path);
        rmdir(dir);
        return 0;
    }

    report(type_configs[logtype_default].fd == logfd
           && !type_configs[logtype_default].syslog,
           "a failed write keeps the log file's descriptor and destination");
    ofd = mkstemp(other);
    LOG(log_error, logtype_afpd, "line two: lost");
    report(ofd >= 0 && fstat(ofd, &st) == 0 && st.st_size == 0,
           "a descriptor opened after a failed write receives no log line");
    /* a forked process lifts its own cap and logs while the parent's lines
     * are still lost */
    fflush(stdout);
    child = fork();

    if (child == 0) {
        cap_file_size(-1, 0, &saved);
        LOG(log_error, logtype_afpd, "line from a forked process: written");
        _exit(file_has(path, "line from a forked process: written", NULL)
              && !file_has(path, "lost 2 log lines", NULL) ? 0 : 1);
    }

    if (child > 0) {
        waitpid(child, &status, 0);
    }

    report(child > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0,
           "a forked process does not report its parent's lost lines");
    cap_file_size(-1, 0, &saved);
    LOG(log_error, logtype_afpd, "line three: written");
    report(file_has(path, "lost 2 log lines", "line three: written"),
           "logging resumes in the same file after a note of the lines lost");
    /* room for part of a line only: its prefix lands, the rest does not */
    capped = cap_file_size(type_configs[logtype_default].fd, 40, &saved) == 0;
    LOG(log_error, logtype_afpd, "line cut short: lost");
    cap_file_size(-1, 0, &saved);
    LOG(log_error, logtype_afpd, "line after the cut one: written");
    report(capped && file_has(path, "lost 1 log lines",
                              "line after the cut one: written"),
           "a line cut short by a short write is counted lost");
    report(lines_whole(path),
           "the part of a cut line that landed ends before the next line");
    /* the descriptor is closed elsewhere and its number reused */
    old = type_configs[logtype_default].fd;
    close(old);
    stranger = open("/dev/null", O_RDONLY);
    LOG(log_error, logtype_afpd, "line four: written");
    report(stranger == old && fcntl(stranger, F_GETFD) != -1
           && type_configs[logtype_default].fd != stranger
           && file_has(path, "line four: written", NULL),
           "a descriptor that is no longer the log file's is replaced, not closed");
    /* the descriptor is gone and the file cannot be reopened for a while */
    old = type_configs[logtype_default].fd;
    close(old);
    stranger2 = open("/dev/null", O_RDONLY);
    snprintf(away, sizeof(away), "%s.away", dir);
    rename(dir, away);
    LOG(log_error, logtype_afpd, "line six: lost");
    LOG(log_error, logtype_afpd, "line seven: lost");
    rename(away, dir);
    LOG(log_error, logtype_afpd, "line eight: written");
    report(stranger2 == old && fcntl(stranger2, F_GETFD) != -1
           && type_configs[logtype_default].fd >= 0
           && file_has(path, "lost 2 log lines", "line eight: written"),
           "a file that cannot be reopened for a while is reopened once it can be");
    /* the file is deleted while writes to it fail */
    old = type_configs[logtype_default].fd;
    unlink(path);
    capped = cap_file_size(old, 0, &saved) == 0;
    LOG(log_error, logtype_afpd, "line five: written");
    cap_file_size(-1, 0, &saved);
    report(capped && file_has(path, "line five: written", NULL)
           && fcntl(old, F_GETFD) == -1 && errno == EBADF,
           "a log file deleted while writes fail is created again, and the old one let go");

    if (stranger >= 0) {
        close(stranger);
    }

    if (stranger2 >= 0) {
        close(stranger2);
    }

    if (ofd >= 0) {
        close(ofd);
        unlink(other);
    }

    unlink(path);
    rmdir(dir);
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
