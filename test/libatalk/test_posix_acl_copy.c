/*
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

/*!
 * @file
 * @brief Exercise native POSIX access ACL copying on open files
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(__linux__) && defined(HAVE_POSIX_ACLS)

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include <atalk/acl.h>
#include <atalk/util.h>
#include <acl/libacl.h>

#include "faultinject.h"

/*!
 * @brief Check the destination access ACL after copying an open file's ACL
 *
 * Create temporary source and destination files and compare the copied ACL
 * with the expected ACL. With a configured error, also require that the ACL
 * write hits the fault injector. Close and remove the test files afterwards.
 *
 * @param[in] dirfd          open scratch directory, kept open
 * @param[in] source_text    source access ACL in text form
 * @param[in] expected_text  expected destination access ACL in text form
 * @param[in] acl_set_error  errno to inject on the ACL write, or 0
 * @returns 1 if the copy meets the expectations, or 0 on setup or check failure
 */
static int check_copy(int dirfd, const char *source_text,
                      const char *expected_text, int acl_set_error)
{
    acl_t source = acl_from_text(source_text);
    acl_t expected = acl_from_text(expected_text);
    acl_t got = NULL;
    int srcfd = openat(dirfd, "source", O_CREAT | O_EXCL | O_RDWR, 0600);
    int dstfd = openat(dirfd, "copy", O_CREAT | O_EXCL | O_RDWR, 0640);
    int ok = 0;

    if (source == NULL || expected == NULL || srcfd < 0 || dstfd < 0
            || fchmod(dstfd, 0640) != 0
            || acl_set_fd(srcfd, source) != 0) {
        goto done;
    }

    fi.fsetxattr.armed = acl_set_error != 0;
    fi.fsetxattr.errnum = acl_set_error;
    int result = copy_posix_acl(srcfd, dstfd);
    int injected = fi.fsetxattr.calls == 1;
    fault_inject_reset();

    if (result != 0 || (acl_set_error && !injected)) {
        goto done;
    }

    got = acl_get_fd(dstfd);
    ok = got != NULL && acl_cmp(got, expected) == 0;
done:

    if (!ok) {
        fprintf(stderr, "ACL copy failed for %s (injected errno %d)\n",
                source_text, acl_set_error);
    }

    if (got) {
        acl_free(got);
    }

    if (source) {
        acl_free(source);
    }

    if (expected) {
        acl_free(expected);
    }

    if (srcfd >= 0) {
        close(srcfd);
        unlinkat(dirfd, "source", 0);
    }

    if (dstfd >= 0) {
        close(dstfd);
        unlinkat(dirfd, "copy", 0);
    }

    return ok;
}

/*!
 * @brief Run the Linux POSIX access ACL copy tests
 *
 * Use a temporary directory to exercise named entries, mode limits, inherited
 * ACL replacement, and the mode-bit fallback for a basic ACL.
 *
 * @returns 0 on success, 1 on a setup or test failure, or 77 if the filesystem
 *          lacks ACL support
 */
int main(void)
{
    char dir[MAXPATHLEN + 1];
    acl_t inherited = NULL;
    int dirfd = -1;
    int ret = 1;
    int len = snprintf(dir, sizeof(dir), "%s/netatalk-acl-copy-XXXXXX",
                       tmpdir());

    if (len < 0 || (size_t)len >= sizeof(dir) || mkdtemp(dir) == NULL) {
        return 1;
    }

    inherited = acl_from_text("u::rwx,u:12346:rwx,g::r-x,m::r-x,o::---");

    if (inherited == NULL
            || acl_set_file(dir, ACL_TYPE_DEFAULT, inherited) != 0) {
        if (errno == EOPNOTSUPP || errno == ENOTSUP) {
            ret = 77;
        }

        goto done;
    }

    dirfd = open(dir, O_RDONLY | O_DIRECTORY);

    if (dirfd == -1) {
        goto done;
    }

    if (check_copy(dirfd, "u::rw-,u:12345:rwx,g::r--,m::r--,o::---",
                   "u::rw-,u:12345:rwx,g::r--,m::r--,o::---", 0)
            && check_copy(dirfd, "u::rw-,g::r--,o::---", "u::rw-,g::r--,o::---", 0)
            && check_copy(dirfd, "u::rwx,u:12345:rwx,g::rwx,m::rwx,o::rwx",
                          "u::rw-,u:12345:rwx,g::rwx,m::r--,o::---", 0)
            && acl_delete_def_file(dir) == 0
            && check_copy(dirfd, "u::r--,g::rw-,o::r--", "u::r--,g::r--,o::---",
                          EOPNOTSUPP)) {
        ret = 0;
    }

done:

    if (dirfd != -1) {
        close(dirfd);
    }

    if (inherited) {
        acl_free(inherited);
    }

    rmdir(dir);
    return ret;
}

#else

/*!
 * @brief Skip the POSIX access ACL copy tests
 *
 * This build lacks the Linux POSIX ACL APIs needed by the test cases.
 *
 * @returns 77 to mark the test skipped
 */
int main(void)
{
    return 77;
}

#endif /* __linux__ && HAVE_POSIX_ACLS */
