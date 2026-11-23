/*
 * Tests for the ftruncate() fallback of sys_ftruncate()
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

/*!
 * @file
 * @brief Tests for the ftruncate() fallback of sys_ftruncate()
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/adouble.h>
#include <atalk/ea.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/unix.h>
#include <atalk/util.h>

int sys_ftruncate_refused(int fd, off_t length);

/*
 * Rename libc calls only after the headers ad_write.c includes have been
 * parsed.  Applying these definitions as compiler arguments breaks fortified
 * libc headers that need to declare and wrap the original functions.
 */
#define ftruncate sys_ftruncate_refused
#include "../../libatalk/adouble/ad_write.c"
#undef ftruncate

static int tests_run;
static int tests_failed;

/*!
 * @brief ftruncate() of sys_ftruncate(): refused, as by a filesystem that
 *        will not change a file's length
 *
 * @param[in] fd      unused
 * @param[in] length  unused
 * @returns -1 with errno EPERM
 */
int sys_ftruncate_refused(int fd, off_t length)
{
    (void)fd;
    (void)length;
    errno = EPERM;
    return -1;
}

/*! @brief Print one TAP result line */
static void report(int passed, const char *name)
{
    tests_run++;
    printf("%s %d - %s\n", passed ? "ok" : "not ok", tests_run, name);

    if (!passed) {
        tests_failed++;
    }
}

/*!
 * @brief Open an unlinked scratch file that holds data
 *
 * @param[in] data  contents of the file
 * @returns the descriptor, or -1
 */
static int scratch_file(const char *data)
{
    char path[MAXPATHLEN];
    size_t len = strlen(data);
    int fd;
    snprintf(path, sizeof(path), "%s/netatalk-ftruncate-XXXXXX", tmpdir());

    if ((fd = mkstemp(path)) == -1) {
        return -1;
    }

    unlink(path);

    if (write(fd, data, len) != (ssize_t)len) {
        close(fd);
        return -1;
    }

    return fd;
}

/*!
 * @brief Tell whether fd holds the len bytes at want and nothing more
 *
 * @param[in] fd    file to read
 * @param[in] want  contents expected
 * @param[in] len   bytes at want
 * @returns 1 when it does, else 0
 */
static int holds(int fd, const char *want, size_t len)
{
    char got[16];
    struct stat st;
    return fstat(fd, &st) == 0 && (size_t)st.st_size == len
           && pread(fd, got, sizeof(got), 0) == (ssize_t)len
           && memcmp(got, want, len) == 0;
}

/*!
 * @brief Remove the folder tmpdir() made for this process, once it is empty,
 *        but never the system folder tmpdir() falls back to
 *
 * The folder is the one whose last name is the netatalk-PID tmpdir() gives.
 */
static void tmpdir_remove(void)
{
    const char *dir = tmpdir();
    const char *last = strrchr(dir, '/');
    char name[32];
    snprintf(name, sizeof(name), "netatalk-%u", (unsigned int)getpid());

    if (last != NULL && strcmp(last + 1, name) == 0) {
        rmdir(dir);
    }
}

/*!
 * @brief Run sys_ftruncate() to length on a scratch file that holds data
 *
 * @param[in] data    contents of the file before the call
 * @param[in] length  length asked for
 * @param[in] want    contents the file must hold afterwards
 * @param[in] len     bytes at want
 * @param[in] ret     result sys_ftruncate() must give, with errno EPERM for -1
 * @returns 1 when it does, the file holds want and its position is where it
 *          was, else 0
 */
static int truncate_case(const char *data, off_t length, const char *want,
                         size_t len, int ret)
{
    off_t pos;
    int fd;
    int ok;

    if ((fd = scratch_file(data)) == -1) {
        return 0;
    }

    pos = lseek(fd, 0, SEEK_CUR);
    errno = 0;
    ok = sys_ftruncate(fd, length) == ret && (ret == 0 || errno == EPERM)
         && lseek(fd, 0, SEEK_CUR) == pos && holds(fd, want, len);
    close(fd);
    return ok;
}

/*! @brief Run the tests and print their TAP report */
int main(void)
{
    int fd;
    printf("TAP version 13\n");

    if ((fd = scratch_file("abc")) == -1) {
        printf("Bail out! cannot create a scratch file in %s\n", tmpdir());
        tmpdir_remove();
        return 1;
    }

    close(fd);
    report(truncate_case("abc", 3, "abc", 3, 0),
           "sys_ftruncate keeps the bytes of a file that has the length");
    report(truncate_case("", 0, "", 0, 0),
           "sys_ftruncate leaves an empty file empty at a length of 0");
    report(truncate_case("abc", 10, "abc\0\0\0\0\0\0\0", 10, 0),
           "sys_ftruncate extends a file ftruncate does not");
    report(truncate_case("abc", 1, "abc", 3, -1),
           "sys_ftruncate leaves a longer file alone");
    tmpdir_remove();
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
