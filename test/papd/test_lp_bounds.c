/*
 * Regression tests for CAP authentication paths and spool filenames
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int open_calls;
static int unlink_calls;
static char opened_path[4096];
static char unlinked_path[4096];

static int bounds_open(const char *path, int flags, ...)
{
    (void)flags;
    open_calls++;
    snprintf(opened_path, sizeof(opened_path), "%s", path);
    errno = ENOENT;
    return -1;
}

static int bounds_unlink(const char *path)
{
    unlink_calls++;
    snprintf(unlinked_path, sizeof(unlinked_path), "%s", path);
    return 0;
}

static int bounds_gethostname(char *name, size_t size)
{
    /* Model a platform returning a truncated hostname without a terminator. */
    memset(name, 'h', size);
    return 0;
}

#define open bounds_open
#define unlink bounds_unlink
#define gethostname bounds_gethostname
#include "../../etc/papd/lp.c"
#undef open
#undef unlink
#undef gethostname

struct printer *printer;
struct sockaddr_at *sat;

#ifdef HAVE_CUPS
/* CUPS operations are outside these path-formatting tests. */
int cups_get_printer_status(struct printer *pr)
{
    (void)pr;
    return 1;
}

const char *cups_get_language(void)
{
    return "UTF-8";
}

int cups_print_job(char *name, const char *filename, char *job,
                   char *username, char *options)
{
    (void)name;
    (void)filename;
    (void)job;
    (void)username;
    (void)options;
    return 0;
}
#endif

static int failures;

static void check(int passed, const char *name)
{
    if (!passed) {
        fprintf(stderr, "FAIL: %s\n", name);
        failures++;
    }
}

static void test_auth_paths(void)
{
    const char *suffix = "/net255.255node255";
    char directory[257];
    char output[1024];
    struct papfile out = {
        .pf_buf = output, .pf_data = output, .pf_bufsize = sizeof(output),
    };
    struct sockaddr_at address = {0};
    struct printer pr = {0};
    size_t max_directory = 255 - strlen(suffix);
    address.sat_addr.s_net = htons(65535);
    address.sat_addr.s_node = 255;
    pr.p_flags = P_AUTH | P_AUTH_CAP;
    pr.p_authprintdir = directory;
    printer = &pr;
    memset(directory, 'a', max_directory);
    directory[max_directory] = '\0';
    memset(&lp, 0, sizeof(lp));
    open_calls = 0;
    check(lp_init(&out, &address) == -1, "missing CAP file rejects authentication");
    check(open_calls == 1 && strlen(opened_path) == 255 &&
          strcmp(opened_path + max_directory, suffix) == 0,
          "exact-fit CAP path reaches open intact");
    directory[max_directory] = 'a';
    directory[max_directory + 1] = '\0';
    out.pf_datalen = 0;
    open_calls = 0;
    check(lp_init(&out, &address) == -1 && open_calls == 0,
          "one-byte-too-long CAP path fails before open");
    memset(directory, 'a', sizeof(directory) - 1);
    directory[sizeof(directory) - 1] = '\0';
    out.pf_datalen = 0;
    check(lp_init(&out, &address) == -1 && open_calls == 0,
          "oversized CAP directory fails before open");
}

static void test_hostname_and_spool_names(void)
{
    char output[1024];
    char expected[MAXPATHLEN];
    struct papfile out = {
        .pf_buf = output, .pf_data = output, .pf_bufsize = sizeof(output),
    };
    struct sockaddr_at address = {0};
    struct printer pr = {0};
    printer = &pr;
    memset(&lp, 0, sizeof(lp));
    memset(hostname, 'z', sizeof(hostname));
    check(lp_init(&out, &address) == 0, "initialize with unterminated hostname");
    check(hostname[sizeof(hostname) - 1] == '\0' &&
          strlen(hostname) == sizeof(hostname) - 1, "hostname is terminated");
    lp.lp_flags = LP_INIT;
    lp.lp_seq = INT_MAX;
    lp.lp_letter = 'A';
    snprintf(expected, sizeof(expected), "dfA%d%s", INT_MAX, hostname);
    open_calls = 0;
    check(lp_open(&out, &address) == -1 && open_calls == 1 &&
          strcmp(opened_path, expected) == 0,
          "spool filename retains full sequence and host");
    unlink_calls = 0;
    check(lp_cancel() == 0 && unlink_calls == 1 &&
          strcmp(unlinked_path, expected) == 0, "cancel uses the same spool filename");
    lp.lp_seq = 7;
    lp.lp_letter = 'A';
    snprintf(expected, sizeof(expected), "dfA007%s", hostname);
    check(lp_open(&out, &address) == -1 && strcmp(opened_path, expected) == 0,
          "three-digit sequence padding is preserved");
}

int main(void)
{
    test_auth_paths();
    test_hostname_and_spool_names();
    return failures ? 1 : 0;
}
