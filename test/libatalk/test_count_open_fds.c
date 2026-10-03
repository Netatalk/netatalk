/*
 * Tests for the count_open_fds() descriptor counter
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>

#include <atalk/unix.h>

#define EXHAUST_MAX 64

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

/*! @brief The reported limit is getdtablesize() */
static int test_reports_limit(void)
{
    int limit = -1;
    (void)count_open_fds(&limit);
    return limit == getdtablesize();
}

/*! @brief A NULL limit pointer is accepted */
static int test_null_limit(void)
{
    return count_open_fds(NULL) >= 0;
}

/*! @brief A pipe raises the count by two and closing it restores it */
static int test_tracks_open_and_close(void)
{
    int p[2];
    int before = count_open_fds(NULL);

    if (pipe(p) != 0) {
        return 0;
    }

    int during = count_open_fds(NULL);
    close(p[0]);
    close(p[1]);
    int after = count_open_fds(NULL);
    return during == before + 2 && after == before;
}

/*!
 * @brief The count holds when no descriptor is free
 *
 * Lowers the soft limit just above current use and opens until EMFILE, so
 * every descriptor below the limit is taken and the count must equal it.
 * With no descriptor free, /proc/self/fd cannot be listed, so the count
 * comes from the poll(2) scan; the test checks that on Linux and Solaris.
 */
static int test_counts_when_exhausted(void)
{
    struct rlimit saved;
    struct rlimit low;
    int fds[EXHAUST_MAX];
    int opened = 0;
    int ok;

    if (getrlimit(RLIMIT_NOFILE, &saved) != 0) {
        return 0;
    }

    low = saved;
    low.rlim_cur = (rlim_t)count_open_fds(NULL) + 8;

    if (low.rlim_cur >= saved.rlim_cur || setrlimit(RLIMIT_NOFILE, &low) != 0) {
        return 0;
    }

    errno = 0;

    while (opened < EXHAUST_MAX) {
        int fd = open("/dev/null", O_RDONLY);

        if (fd < 0) {
            break;
        }

        fds[opened++] = fd;
    }

    ok = opened < EXHAUST_MAX && errno == EMFILE
         && count_open_fds(NULL) == getdtablesize();
#if defined(__linux__) || defined(__sun)
    DIR *dp = opendir("/proc/self/fd");
    ok = ok && dp == NULL && errno == EMFILE;

    if (dp != NULL) {
        closedir(dp);
    }

#endif

    while (opened > 0) {
        close(fds[--opened]);
    }

    if (setrlimit(RLIMIT_NOFILE, &saved) != 0) {
        return 0;
    }

    return ok;
}

int main(void)
{
    printf("TAP version 13\n");
    report(test_reports_limit(), "count_open_fds reports getdtablesize()");
    report(test_null_limit(), "count_open_fds accepts a NULL limit");
    report(test_tracks_open_and_close(),
           "count_open_fds tracks a pipe opened and closed");
    report(test_counts_when_exhausted(),
           "count_open_fds counts every descriptor when none is free");
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
