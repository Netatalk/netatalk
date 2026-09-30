/*
 * Unit tests for vetoed-file deletion and its directory descriptor handling
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/util.h>
#include <atalk/volume.h>

#include "faultinject.h"
#include "filedir.h"
#include "subtests_veto.h"
#include "test.h"

static int create_file(const char *path)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);

    if (fd == -1) {
        return -1;
    }

    return close(fd);
}

static int make_case(char root[MAXPATHLEN + 1], const char *name)
{
    int length = snprintf(root, MAXPATHLEN + 1,
                          "%s/netatalk-veto-%s-XXXXXX", tmpdir(), name);

    if (length < 0 || length >= MAXPATHLEN + 1) {
        return -1;
    }

    return mkdtemp(root) == NULL ? -1 : 0;
}

static int case_path(char path[MAXPATHLEN + 1], const char *parent,
                     const char *name)
{
    int length = snprintf(path, MAXPATHLEN + 1, "%s/%s", parent, name);

    if (length < 0 || length >= MAXPATHLEN + 1) {
        path[0] = '\0';
        return -1;
    }

    return 0;
}

/* A veto-matching symlink is a leaf, even when its target is a directory. */
int utest_delete_veto_symlink_stays_in_volume(void)
{
    char root[MAXPATHLEN + 1] = {0};
    char share[MAXPATHLEN + 1] = {0};
    char deldir[MAXPATHLEN + 1] = {0};
    char outside[MAXPATHLEN + 1] = {0};
    char linkpath[MAXPATHLEN + 1] = {0};
    char secret[MAXPATHLEN + 1] = {0};
    struct stat st;
    struct vol vol = {0};
    int result = 1;

    if (make_case(root, "symlink") != 0) {
        return 1;
    }

    if (case_path(share, root, "share") != 0
            || case_path(deldir, share, "deldir") != 0
            || case_path(outside, root, "outside") != 0
            || case_path(linkpath, deldir, "vetodir") != 0
            || case_path(secret, outside, "secret") != 0
            || mkdir(share, 0700) != 0
            || mkdir(deldir, 0700) != 0
            || mkdir(outside, 0700) != 0
            || create_file(secret) != 0
            || symlink(outside, linkpath) != 0) {
        goto cleanup;
    }

    vol.v_flags = AFPVOL_DELVETO;
    vol.v_veto = "vetodir/";

    if (delete_vetoed_files(&vol, deldir, false) != 0
            || stat(secret, &st) != 0
            || lstat(deldir, &st) == 0
            || errno != ENOENT) {
        goto cleanup;
    }

    result = 0;
cleanup:
    unlink(linkpath);
    unlink(secret);
    rmdir(deldir);
    rmdir(share);
    rmdir(outside);
    rmdir(root);
    return result;
}

/* Remove matching children, including descendants, but preserve ordinary files. */
int utest_delete_veto_recursive(void)
{
    char root[MAXPATHLEN + 1] = {0};
    char dir[MAXPATHLEN + 1] = {0};
    char nested[MAXPATHLEN + 1] = {0};
    char veto[MAXPATHLEN + 1] = {0};
    char child[MAXPATHLEN + 1] = {0};
    char keep[MAXPATHLEN + 1] = {0};
    struct vol vol = {0};
    struct stat st;
    int result = 1;

    if (make_case(root, "recursive") != 0) {
        return 1;
    }

    if (case_path(dir, root, "delete") != 0
            || case_path(nested, dir, "vetodir") != 0
            || case_path(veto, dir, "veto") != 0
            || case_path(child, nested, "ordinary") != 0
            || case_path(keep, dir, "keep") != 0
            || mkdir(dir, 0700) != 0 || mkdir(nested, 0700) != 0
            || create_file(child) != 0 || create_file(veto) != 0
            || create_file(keep) != 0) {
        goto cleanup;
    }

    vol.v_flags = AFPVOL_DELVETO;
    vol.v_veto = "veto/vetodir/";

    /* The non-veto child prevents removal of the parent on the first pass. */
    if (delete_vetoed_files(&vol, dir, false) == 0
            || lstat(veto, &st) == 0 || errno != ENOENT
            || lstat(nested, &st) == 0 || errno != ENOENT
            || stat(keep, &st) != 0) {
        goto cleanup;
    }

    if (unlink(keep) != 0 || delete_vetoed_files(&vol, dir, false) != 0
            || lstat(dir, &st) == 0 || errno != ENOENT) {
        goto cleanup;
    }

    result = 0;
cleanup:
    unlink(child);
    unlink(veto);
    unlink(keep);
    rmdir(nested);
    rmdir(dir);
    rmdir(root);
    return result;
}

/* The new dirfd error path must close the DIR stream and leave files alone. */
int utest_delete_veto_dirfd_failure(void)
{
    char root[MAXPATHLEN + 1] = {0};
    char dir[MAXPATHLEN + 1] = {0};
    char veto[MAXPATHLEN + 1] = {0};
    struct vol vol = {0};
    struct stat st;
    struct stat dir_st;
    struct stat fd_st;
    DIR *probe = NULL;
    int stream_fd;
    int fd_status;
    int fd_errno;
    int rc;
    int calls;
    int fired;
    int result = 1;

    if (make_case(root, "dirfd") != 0) {
        return 1;
    }

    if (case_path(dir, root, "delete") != 0
            || case_path(veto, dir, "veto") != 0
            || mkdir(dir, 0700) != 0 || create_file(veto) != 0
            || stat(dir, &dir_st) != 0) {
        goto cleanup;
    }

    /* Probe this symbol specifically; an open() preload may work while dirfd()
     * is bound locally or bypasses dynamic interposition on some platforms. */
    probe = opendir(dir);

    if (!probe) {
        goto cleanup;
    }

    fault_inject_reset();
    fi.dirfd_armed = 1;
    fi.dirfd_errno = EIO;
    rc = dirfd(probe);
    calls = fi.dirfd_calls;
    fired = fi.dirfd_fail_after == -1;
    fault_inject_reset();
    closedir(probe);
    probe = NULL;

    if (calls == 0) {
        result = TEST_SKIP;
        goto cleanup;
    }

    if (calls != 1 || !fired || rc != -1) {
        fprintf(stderr, "veto dirfd probe: calls=%d fired=%d rc=%d\n",
                calls, fired, rc);
        result = 2;
        goto cleanup;
    }

    vol.v_flags = AFPVOL_DELVETO;
    vol.v_veto = "veto/";
    fi.dirfd_armed = 1;
    fi.dirfd_errno = EIO;
    rc = delete_vetoed_files(&vol, dir, false);
    calls = fi.dirfd_calls;
    fired = fi.dirfd_fail_after == -1;
    stream_fd = fi.dirfd_failed_fd;
    fault_inject_reset();

    if (calls == 0) {
        result = TEST_SKIP;
        goto cleanup;
    }

    if (!fired || rc != -1 || stream_fd < 0) {
        fprintf(stderr, "veto dirfd test: calls=%d fired=%d rc=%d fd=%d\n",
                calls, fired, rc, stream_fd);
        result = 3;
        goto cleanup;
    }

    errno = 0;
    fd_status = fstat(stream_fd, &fd_st);
    fd_errno = errno;

    if ((fd_status == 0 && fd_st.st_dev == dir_st.st_dev
            && fd_st.st_ino == dir_st.st_ino)
            || (fd_status == -1 && fd_errno != EBADF)) {
        fprintf(stderr, "veto dirfd test: stream fd %d still references "
                        "the directory or failed unexpectedly (fstat=%d errno=%d)\n",
                stream_fd, fd_status, fd_errno);
        result = 4;
        goto cleanup;
    }

    if (stat(veto, &st) != 0) {
        fprintf(stderr, "veto dirfd test: file was changed: %s\n",
                strerror(errno));
        result = 5;
        goto cleanup;
    }

    result = 0;
cleanup:
    fault_inject_reset();

    if (probe) {
        closedir(probe);
    }

    unlink(veto);
    rmdir(dir);
    rmdir(root);
    return result;
}
