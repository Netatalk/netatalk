/*
 * Tests for afppasswd's trusted SRP path selection and file validation
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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Declared in <atalk/util.h>; avoid its unrelated bstring dependency here. */
extern const char *tmpdir(void);

/* Keep the implementation's small internal helpers testable without making
 * them part of afppasswd's public interface. */
#define main afppasswd_program_main
#include "../../bin/afppasswd/afppasswd.c"
#undef main

static int tests_run;
static int tests_failed;

static void report(int passed, const char *name)
{
    tests_run++;
    printf("%s %d - %s\n", passed ? "ok" : "not ok", tests_run, name);

    if (!passed) {
        tests_failed++;
    }
}

static int write_file(const char *path, const char *contents)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    size_t remaining = strlen(contents);

    if (fd < 0) {
        return -1;
    }

    while (remaining > 0) {
        ssize_t written = write(fd, contents, remaining);

        if (written <= 0) {
            close(fd);
            return -1;
        }

        contents += written;
        remaining -= (size_t)written;
    }

    return close(fd);
}

static int make_case(char directory[MAXPATHLEN + 1])
{
    int length = snprintf(directory, MAXPATHLEN + 1,
                          "%s/afppasswd-test.XXXXXX", tmpdir());

    if (length < 0 || length >= MAXPATHLEN + 1) {
        return -1;
    }

    return mkdtemp(directory) == NULL ? -1 : chmod(directory, 0700);
}

static void test_default_path(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char config[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    int passed = 0;

    if (make_case(directory) == 0 &&
            snprintf(config, sizeof(config), "%s/missing.conf", directory) > 0 &&
            configured_credential_path(config, getuid(), "srp verifier path",
                                       "srp passwd file",
                                       _PATH_AFPSRPVERIFIERPATH, path,
                                       sizeof(path), 1) == 0 &&
            strcmp(path, _PATH_AFPSRPVERIFIERPATH) == 0) {
        passed = 1;
    }

    report(passed, "SRP path defaults when standard afp.conf is absent");
    rmdir(directory);
}

static void test_configured_path(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char config[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    int passed = 0;

    if (make_case(directory) == 0 &&
            snprintf(config, sizeof(config), "%s/afp.conf", directory) > 0 &&
            write_file(config, "[Global]\n"
                       "srp passwd file = /legacy/verifiers\n"
                       "srp verifier path = /configured/verifiers\n") == 0 &&
            configured_credential_path(config, getuid(), "srp verifier path",
                                       "srp passwd file",
                                       _PATH_AFPSRPVERIFIERPATH, path,
                                       sizeof(path), 1) == 0 &&
            strcmp(path, "/configured/verifiers") == 0) {
        passed = 1;
    }

    report(passed, "SRP path is read from trusted Global configuration");
    unlink(config);
    rmdir(directory);
}

static void test_explicit_config_must_exist(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char config[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    int passed = 0;

    if (make_case(directory) == 0 &&
            snprintf(config, sizeof(config), "%s/missing.conf", directory) > 0 &&
            configured_credential_path(config, getuid(), "srp verifier path",
                                       "srp passwd file",
                                       _PATH_AFPSRPVERIFIERPATH, path,
                                       sizeof(path), 0) < 0) {
        passed = 1;
    }

    report(passed, "explicit afp.conf must exist");
    rmdir(directory);
}

static void test_nonstandard_config_is_not_consulted(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char standard[MAXPATHLEN + 1];
    char nonstandard[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    int passed = 0;

    if (make_case(directory) == 0 &&
            snprintf(standard, sizeof(standard), "%s/afp.conf", directory) > 0 &&
            snprintf(nonstandard, sizeof(nonstandard), "%s/alternate.conf", directory) > 0
            &&
            write_file(standard, "[Global]\n") == 0 &&
            write_file(nonstandard, "[Global]\n"
                       "srp verifier path = /alternate/verifiers\n") == 0 &&
            configured_credential_path(standard, getuid(), "srp verifier path",
                                       "srp passwd file",
                                       _PATH_AFPSRPVERIFIERPATH, path,
                                       sizeof(path), 1) == 0 &&
            strcmp(path, _PATH_AFPSRPVERIFIERPATH) == 0) {
        passed = 1;
    }

    report(passed, "only the standard afp.conf is consulted");
    unlink(nonstandard);
    unlink(standard);
    rmdir(directory);
}

static void test_randnum_configured_path(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char config[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    int passed = 0;

    if (make_case(directory) == 0 &&
            snprintf(config, sizeof(config), "%s/afp.conf", directory) > 0 &&
            write_file(config, "[Global]\n"
                       "passwd file = /configured/afppasswd\n") == 0 &&
            configured_credential_path(config, getuid(), "passwd file", NULL,
                                       _PATH_AFPDPWFILE, path,
                                       sizeof(path), 1) == 0 &&
            strcmp(path, "/configured/afppasswd") == 0) {
        passed = 1;
    }

    report(passed, "Randnum path is read from trusted Global configuration");
    unlink(config);
    rmdir(directory);
}

static void test_unsafe_config_rejected(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char config[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 1];
    int passed = 0;

    if (make_case(directory) == 0 &&
            snprintf(config, sizeof(config), "%s/afp.conf", directory) > 0 &&
            write_file(config, "[Global]\n"
                       "srp verifier path = /unsafe/verifiers\n") == 0 &&
            chmod(config, 0660) == 0 &&
            configured_credential_path(config, getuid(), "srp verifier path",
                                       "srp passwd file",
                                       _PATH_AFPSRPVERIFIERPATH, path,
                                       sizeof(path), 1) < 0) {
        passed = 1;
    }

    report(passed, "group-writable configuration is rejected");
    unlink(config);
    rmdir(directory);
}

static void test_nonroot_config_option_rejected(void)
{
    char *argv[] = { "afppasswd", "-F", "afp.conf", NULL };
    int passed;

    if (getuid() == 0) {
        report(1, "non-root -F is rejected (skipped while running as root)");
        return;
    }

    optind = 1;
    passed = afppasswd_program_main(3, argv) != 0;
    report(passed, "non-root -F is rejected");
}

static void test_path_option_removed(void)
{
    char *argv[] = { "afppasswd", "-p", "verifiers", NULL };
    int passed;
    optind = 1;
    passed = afppasswd_program_main(3, argv) != 0;
    report(passed, "legacy -p option is rejected");
}

static void test_rejected_verifier_does_not_change_metadata(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char verifier[MAXPATHLEN + 1];
    char uid_name[3 * sizeof(uid_t) + 1];
    struct stat before, after;
    int dirfd = -1;
    int fd;
    int passed = 0;

    if (make_case(directory) == 0 &&
            srp_uid_filename(getuid(), uid_name, sizeof(uid_name)) == 0 &&
            snprintf(verifier, sizeof(verifier), "%s/%s", directory, uid_name) > 0 &&
            write_file(verifier, "not a verifier\n") == 0 &&
            chmod(verifier, 0640) == 0 && stat(verifier, &before) == 0 &&
            (dirfd = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC)) >= 0) {
        fd = open_srp_verifier(dirfd, directory, getuid(), 1);

        if (fd >= 0) {
            close(fd);
        }

        if (fd < 0 && stat(verifier, &after) == 0 &&
                (before.st_mode & 0777) == (after.st_mode & 0777) &&
                before.st_uid == after.st_uid && before.st_nlink == after.st_nlink) {
            passed = 1;
        }
    }

    if (dirfd >= 0) {
        close(dirfd);
    }

    report(passed, "rejected verifier leaves metadata unchanged");
    unlink(verifier);
    rmdir(directory);
}

int main(void)
{
    test_default_path();
    test_configured_path();
    test_explicit_config_must_exist();
    test_nonstandard_config_is_not_consulted();
    test_randnum_configured_path();
    test_unsafe_config_rejected();
    test_nonroot_config_option_rejected();
    test_path_option_removed();
    test_rejected_verifier_does_not_change_metadata();
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
