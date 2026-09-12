/*
 * Tests for credential-preserving flat SRP verifier migration
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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "afppasswd_migrate.h"

/* Declared in <atalk/util.h>; avoid its unrelated bstring dependency here. */
extern const char *tmpdir(void);

#define SALT_HEX_LEN 32
#define VERIFIER_HEX_LEN 384

struct test_context {
    uid_t uid;
    int collide_uids;
    int fail_write;
    int sync_calls;
    int fail_sync_call;
};

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

static char *make_record(const char *username, char salt, char verifier)
{
    size_t username_length = strlen(username);
    size_t suffix_length = 1 + SALT_HEX_LEN + 1 + VERIFIER_HEX_LEN + 2;
    size_t length;
    char *record;
    char *p;

    if (username_length > SIZE_MAX - suffix_length) {
        return NULL;
    }

    length = username_length + suffix_length;
    record = malloc(length);

    if (record == NULL) {
        return NULL;
    }

    p = record;
    memcpy(p, username, username_length);
    p += username_length;
    *p++ = ':';
    memset(p, salt, SALT_HEX_LEN);
    p += SALT_HEX_LEN;
    *p++ = ':';
    memset(p, verifier, VERIFIER_HEX_LEN);
    p += VERIFIER_HEX_LEN;
    *p++ = '\n';
    *p = '\0';
    return record;
}

static int test_lookup(const char *name, uid_t *uid, void *opaque)
{
    const struct test_context *context = opaque;

    if (strcmp(name, "unknown") == 0) {
        return -1;
    }

    if (strcmp(name, "alice") == 0) {
        *uid = context->uid;
        return 0;
    }

    if (strcmp(name, "bob") == 0) {
        *uid = context->collide_uids ? context->uid : context->uid + 1;
        return 0;
    }

    return -1;
}

static ssize_t test_write(int fd, const void *data, size_t length, void *opaque)
{
    struct test_context *context = opaque;

    if (context->fail_write == 1) {
        size_t partial = length < 17 ? length : 17;
        context->fail_write = 2;
        return write(fd, data, partial);
    }

    if (context->fail_write == 2) {
        errno = EIO;
        return -1;
    }

    return write(fd, data, length);
}

static int test_sync(int fd, void *opaque)
{
    struct test_context *context = opaque;
    context->sync_calls++;

    if (context->fail_sync_call == context->sync_calls) {
        errno = EIO;
        return -1;
    }

    return fsync(fd);
}

static int write_file(const char *path, const char *data)
{
    size_t remaining = strlen(data);
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if (fd < 0) {
        return -1;
    }

    while (remaining > 0) {
        ssize_t written = write(fd, data, remaining);

        if (written <= 0) {
            close(fd);
            return -1;
        }

        data += written;
        remaining -= (size_t)written;
    }

    return close(fd);
}

static char *read_file(const char *path)
{
    struct stat st;
    char *data;
    ssize_t length;
    int fd = open(path, O_RDONLY);

    if (fd < 0 || fstat(fd, &st) < 0 || st.st_size < 0) {
        if (fd >= 0) {
            close(fd);
        }

        return NULL;
    }

    data = malloc((size_t)st.st_size + 1);

    if (data == NULL) {
        close(fd);
        return NULL;
    }

    length = read(fd, data, (size_t)st.st_size);
    close(fd);

    if (length != st.st_size) {
        free(data);
        return NULL;
    }

    data[length] = '\0';
    return data;
}

static int make_case(char directory[MAXPATHLEN + 1],
                     char path[MAXPATHLEN + 1], const char *source)
{
    int length = snprintf(directory, MAXPATHLEN + 1,
                          "%s/afppasswd-migrate.XXXXXX", tmpdir());

    if (length < 0 || length >= MAXPATHLEN + 1) {
        return -1;
    }

    if (mkdtemp(directory) == NULL || chmod(directory, 0700) < 0 ||
            snprintf(path, MAXPATHLEN + 1, "%s/afppasswd.srp", directory) < 0 ||
            write_file(path, source) < 0 || chmod(path, 0600) < 0) {
        return -1;
    }

    return 0;
}

static void remove_case(const char *directory)
{
    if (directory[0] == '\0') {
        return;
    }

    DIR *stream = opendir(directory);
    struct dirent *entry;

    if (stream == NULL) {
        return;
    }

    while ((entry = readdir(stream)) != NULL) {
        char path[MAXPATHLEN + 1];
        struct stat st;

        if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", directory, entry->d_name);

        if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            DIR *child = opendir(path);
            struct dirent *child_entry;

            if (child != NULL) {
                while ((child_entry = readdir(child)) != NULL) {
                    char child_path[MAXPATHLEN + 1];

                    if (strcmp(child_entry->d_name, ".") == 0 ||
                            strcmp(child_entry->d_name, "..") == 0) {
                        continue;
                    }

                    snprintf(child_path, sizeof(child_path), "%s/%s", path,
                             child_entry->d_name);
                    unlink(child_path);
                }

                closedir(child);
            }

            rmdir(path);
        } else {
            unlink(path);
        }
    }

    closedir(stream);
    rmdir(directory);
}

static int count_case_entries(const char *directory)
{
    DIR *stream = opendir(directory);
    const struct dirent *entry;
    int count = 0;

    if (stream == NULL) {
        return -1;
    }

    while ((entry = readdir(stream)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 &&
                strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }

    closedir(stream);
    return count;
}

static struct afppasswd_migrate_hooks hooks_for(struct test_context *context)
{
    struct afppasswd_migrate_hooks hooks = {
        test_lookup, test_write, test_sync, context
    };
    return hooks;
}

static int source_is_unchanged(const char *path, const char *expected)
{
    struct stat st;
    char *actual = read_file(path);
    int result = actual != NULL && strcmp(actual, expected) == 0 &&
                 lstat(path, &st) == 0 && S_ISREG(st.st_mode);
    free(actual);
    return result;
}

static void test_success(void)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char verifier_path[MAXPATHLEN + 1], backup_path[MAXPATHLEN + 1];
    char *record = make_record("alice", 'A', 'B');
    char *migrated = NULL, *backup = NULL;
    struct stat directory_st, verifier_st;
    struct test_context context = {.uid = getuid()};
    struct afppasswd_migrate_hooks hooks = hooks_for(&context);
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        passed = afppasswd_migrate_srp(path, getuid(), &hooks) == 0;
        snprintf(verifier_path, sizeof(verifier_path), "%s/%ju", path,
                 (uintmax_t)getuid());
        snprintf(backup_path, sizeof(backup_path), "%s.legacy", path);
        migrated = read_file(verifier_path);
        backup = read_file(backup_path);
        passed = passed && migrated != NULL && backup != NULL &&
                 strcmp(migrated, record) == 0 && strcmp(backup, record) == 0 &&
                 stat(path, &directory_st) == 0 &&
                 S_ISDIR(directory_st.st_mode) &&
                 (directory_st.st_mode & 0777) == 0755 &&
                 stat(verifier_path, &verifier_st) == 0 &&
                 S_ISREG(verifier_st.st_mode) &&
                 verifier_st.st_uid == getuid() &&
                 (verifier_st.st_mode & 0777) == 0600 &&
                 verifier_st.st_nlink == 1;
    }

    report(passed, "successful migration preserves verifier and original");
    free(migrated);
    free(backup);
    free(record);
    remove_case(directory);
}

static void test_rejected_source(const char *name, const char *source,
                                 int collide_uids)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    struct test_context context = {
        .uid = getuid(), .collide_uids = collide_uids
    };
    struct afppasswd_migrate_hooks hooks = hooks_for(&context);
    int passed = make_case(directory, path, source) == 0;

    if (passed) {
        passed = afppasswd_migrate_srp(path, getuid(), &hooks) < 0 &&
                 source_is_unchanged(path, source) &&
                 count_case_entries(directory) == 1;
    }

    report(passed, name);
    remove_case(directory);
}

static void test_write_failure(void)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char *record = make_record("alice", '1', '2');
    struct test_context context = {.uid = getuid(), .fail_write = 1};
    struct afppasswd_migrate_hooks hooks = hooks_for(&context);
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        passed = afppasswd_migrate_srp(path, getuid(), &hooks) < 0 &&
                 source_is_unchanged(path, record) &&
                 count_case_entries(directory) == 1;
    }

    report(passed, "interrupted verifier write leaves source intact");
    free(record);
    remove_case(directory);
}

static void test_sync_failure(int sync_call, const char *name)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char *record = make_record("alice", '1', '2');
    struct test_context context = {
        .uid = getuid(), .fail_sync_call = sync_call
    };
    struct afppasswd_migrate_hooks hooks = hooks_for(&context);
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        passed = afppasswd_migrate_srp(path, getuid(), &hooks) < 0 &&
                 source_is_unchanged(path, record) &&
                 count_case_entries(directory) == 1;
    }

    report(passed, name);
    free(record);
    remove_case(directory);
}

static void test_backup_collision_and_repeat(void)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char collision[MAXPATHLEN + 1], backup[MAXPATHLEN + 1];
    char *record = make_record("alice", 'C', 'D');
    char *collision_data = NULL, *backup_data = NULL;
    struct test_context context = {.uid = getuid()};
    struct afppasswd_migrate_hooks hooks = hooks_for(&context);
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        snprintf(collision, sizeof(collision), "%s.legacy", path);
        snprintf(backup, sizeof(backup), "%s.legacy.1", path);
        passed = write_file(collision, "existing backup\n") == 0 &&
                 afppasswd_migrate_srp(path, getuid(), &hooks) == 0;
        collision_data = read_file(collision);
        backup_data = read_file(backup);
        passed = passed && collision_data != NULL && backup_data != NULL &&
                 strcmp(collision_data, "existing backup\n") == 0 &&
                 strcmp(backup_data, record) == 0 &&
                 afppasswd_migrate_srp(path, getuid(), &hooks) < 0;
    }

    report(passed, "backup collision is safe and repeated migration is refused");
    free(collision_data);
    free(backup_data);
    free(record);
    remove_case(directory);
}

static void test_unsafe_metadata(void)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char *record = make_record("alice", 'E', 'F');
    struct test_context context = {.uid = getuid()};
    struct afppasswd_migrate_hooks hooks = hooks_for(&context);
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        passed = chmod(path, 0644) == 0 &&
                 afppasswd_migrate_srp(path, getuid(), &hooks) < 0 &&
                 source_is_unchanged(path, record) &&
                 count_case_entries(directory) == 1;
    }

    report(passed, "unsafe source metadata is rejected without modification");
    free(record);
    remove_case(directory);
}

static void test_hardlinked_source(void)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char link_path[MAXPATHLEN + 1];
    char *record = make_record("alice", 'E', 'F');
    struct test_context context = {.uid = getuid()};
    struct afppasswd_migrate_hooks hooks = hooks_for(&context);
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        snprintf(link_path, sizeof(link_path), "%s/second-link", directory);
        passed = link(path, link_path) == 0 &&
                 afppasswd_migrate_srp(path, getuid(), &hooks) < 0 &&
                 source_is_unchanged(path, record) &&
                 count_case_entries(directory) == 2;
    }

    report(passed, "hardlinked source is rejected without modification");
    free(record);
    remove_case(directory);
}

static void test_partial_destination(void)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char staging[MAXPATHLEN + 1];
    char *record = make_record("alice", 'E', 'F');
    struct test_context context = {.uid = getuid()};
    struct afppasswd_migrate_hooks hooks = hooks_for(&context);
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        snprintf(staging, sizeof(staging),
                 "%s/.afppasswd.srp.migrate.interrupted", directory);
        passed = mkdir(staging, 0700) == 0 &&
                 afppasswd_migrate_srp(path, getuid(), &hooks) < 0 &&
                 source_is_unchanged(path, record) &&
                 count_case_entries(directory) == 2;
    }

    report(passed, "partial migration destination is rejected");
    free(record);
    remove_case(directory);
}

int main(void)
{
    char *alice = make_record("alice", '0', '1');
    char *bob = make_record("bob", '2', '3');
    char *unknown = make_record("unknown", '4', '5');
    char *duplicate = NULL, *collision = NULL;
    printf("TAP version 13\n");

    if (alice != NULL && bob != NULL) {
        size_t duplicate_length = strlen(alice) * 2 + 1;
        size_t collision_length = strlen(alice) + strlen(bob) + 1;
        duplicate = malloc(duplicate_length);
        collision = malloc(collision_length);

        if (duplicate != NULL) {
            snprintf(duplicate, duplicate_length, "%s%s", alice, alice);
        }

        if (collision != NULL) {
            snprintf(collision, collision_length, "%s%s", alice, bob);
        }
    }

    test_success();
    test_rejected_source("malformed record leaves source intact",
                         "alice:not-a-verifier\n", 0);
    test_rejected_source("duplicate user leaves source intact",
                         duplicate != NULL ? duplicate : "", 0);
    test_rejected_source("uid collision leaves source intact",
                         collision != NULL ? collision : "", 1);
    test_rejected_source("unknown user leaves source intact",
                         unknown != NULL ? unknown : "", 0);
    test_write_failure();
    test_sync_failure(2, "failed staging sync leaves source intact");
    test_sync_failure(3, "failed backup sync leaves source intact");
    test_sync_failure(4, "failed pre-install sync rolls source back");
    test_sync_failure(5, "failed install sync rolls source back");
    test_backup_collision_and_repeat();
    test_unsafe_metadata();
    test_hardlinked_source();
    test_partial_destination();
    printf("1..%d\n", tests_run);
    free(alice);
    free(bob);
    free(unknown);
    free(duplicate);
    free(collision);
    return tests_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
