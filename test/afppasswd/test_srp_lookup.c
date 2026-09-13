/*
 * Tests for SRP UAM verifier-state classification.
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Declared in <atalk/util.h>; avoid its unrelated bstring dependency here. */
extern const char *tmpdir(void);

static int test_fstat(int fd, struct stat *st);
#define fstat test_fstat
#include "../../etc/uams/uams_srp.c"
#undef fstat

/* The lookup test includes the UAM implementation directly. These daemon
 * callbacks are not exercised, but are referenced by its login handlers. */
int uam_register(const int type _U_, const char *path _U_, const char *name _U_,
                 ...)
{
    return 0;
}

void uam_unregister(const int type _U_, const char *name _U_)
{
    /* This test provides only link-time stubs for UAM callbacks */
}

struct passwd *uam_getname(void *private _U_, char *name _U_, const int len _U_)
{
    return NULL;
}

int uam_checkuser(void *private _U_, const struct passwd *pwd _U_)
{
    return -1;
}

int uam_afpserver_option(void *private _U_, const int what _U_,
                         void *option _U_, size_t *len _U_)
{
    return -1;
}

static struct stat verifier_directory;
static struct stat verifier_file;

static int test_fstat(int fd, struct stat *st)
{
    int result = fstat(fd, st);

    if (result == 0 && st->st_dev == verifier_directory.st_dev &&
            st->st_ino == verifier_directory.st_ino) {
        st->st_uid = 0;
    }

    if (result == 0 && st->st_dev == verifier_file.st_dev &&
            st->st_ino == verifier_file.st_ino) {
        st->st_uid = 0;
    }

    return result;
}

int main(void)
{
    char directory[MAXPATHLEN + 1];
    char path[MAXPATHLEN + 32] = {0};
    char record[SRP_USERNAME_MAX_LEN + SRP_FORMAT_LEN + 1];
    unsigned char salt[SRP_SALT_LEN];
    gcry_mpi_t verifier = NULL;
    uid_t uid = getuid() == 0 ? 1 : getuid();
    enum srp_verifier_status status;
    size_t length = 0;
    int result;
    int fd = -1;
    int passed = 0;
    result = snprintf(directory, sizeof(directory),
                      "%s/netatalk-srp-lookup.XXXXXX", tmpdir());

    if (result > 0 && (size_t)result < sizeof(directory) &&
            mkdtemp(directory) != NULL &&
            (result = snprintf(path, sizeof(path), "%s/%ju", directory,
                               (uintmax_t)uid)) > 0 &&
            (size_t)result < sizeof(path) &&
            (fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600)) >= 0) {
        length = (size_t)snprintf(record, sizeof(record), "alice:");
        memset(record + length, SRP_DISABLED_CHAR, SRP_HEX_SALT_LEN);
        length += SRP_HEX_SALT_LEN;
        record[length++] = ':';
        memset(record + length, SRP_DISABLED_CHAR, SRP_HEX_V_LEN);
        length += SRP_HEX_V_LEN;
        record[length++] = '\n';

        if (write(fd, record, length) == (ssize_t)length &&
                close(fd) == 0 && stat(directory, &verifier_directory) == 0 &&
                stat(path, &verifier_file) == 0) {
            fd = -1;
            status = srp_lookup_verifier(directory, "alice", uid, salt,
                                         &verifier);
            passed = status == SRP_VERIFIER_NOT_ENROLLED && verifier == NULL;
        }
    }

    if (fd >= 0) {
        close(fd);
    }

    gcry_mpi_release(verifier);
    unlink(path);
    rmdir(directory);
    printf("%s 1 - root-owned disabled verifier is not enrolled\n",
           passed ? "ok" : "not ok");
    return passed ? 0 : 1;
}
