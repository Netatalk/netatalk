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
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Keep fault injection local to the migration implementation. */
static struct passwd *test_getpwnam(const char *name);
static ssize_t test_write(int fd, const void *data, size_t length);
static int test_sync(int fd);
static int test_mkdirat(int fd, const char *path, mode_t mode);
static int test_unlinkat(int fd, const char *path, int flags);
#define getpwnam test_getpwnam
#define write test_write
#define fsync test_sync
#define mkdirat test_mkdirat
#define unlinkat test_unlinkat
#include "../../bin/afppasswd/afppasswd_migrate.c"
#undef getpwnam
#undef write
#undef fsync
#undef mkdirat
#undef unlinkat

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
    int fail_second_sync_call;
    const char *fail_unlink_path;
    int unlink_failures;
    const char *staging_collision_file;
    int mkdir_calls;
};

static struct test_context *active_context;

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

static struct passwd *test_getpwnam(const char *name)
{
    static struct passwd account;

    if (strcmp(name, "alice") == 0) {
        account.pw_uid = active_context->uid;
        return &account;
    }

    if (strcmp(name, "bob") == 0) {
        account.pw_uid = active_context->collide_uids ? active_context->uid :
                         active_context->uid + 1;
        return &account;
    }

    return NULL;
}

static ssize_t test_write(int fd, const void *data, size_t length)
{
    if (active_context->fail_write == 1) {
        size_t partial = length < 17 ? length : 17;
        active_context->fail_write = 2;
        return write(fd, data, partial);
    }

    if (active_context->fail_write == 2) {
        errno = EIO;
        return -1;
    }

    return write(fd, data, length);
}

static int test_sync(int fd)
{
    active_context->sync_calls++;

    if (active_context->fail_sync_call == active_context->sync_calls ||
            active_context->fail_second_sync_call == active_context->sync_calls) {
        errno = EIO;
        return -1;
    }

    return fsync(fd);
}

static int test_unlinkat(int fd, const char *path, int flags)
{
    if (active_context->fail_unlink_path != NULL &&
            strcmp(path, active_context->fail_unlink_path) == 0) {
        active_context->unlink_failures++;
        errno = EACCES;
        return -1;
    }

    return unlinkat(fd, path, flags);
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

static int test_mkdirat(int fd, const char *path, mode_t mode)
{
    active_context->mkdir_calls++;

    if (active_context->staging_collision_file != NULL &&
            active_context->mkdir_calls == 1) {
        /* Introduce a collision after the stale-staging scan. */
        if (mkdirat(fd, path, mode) < 0 ||
                write_file(active_context->staging_collision_file,
                           "existing verifier\n") < 0) {
            return -1;
        }
    }

    return mkdirat(fd, path, mode);
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

static int run_migration(const char *path, struct test_context *context)
{
    int result;
    active_context = context;
    result = afppasswd_migrate_srp(path, getuid());
    active_context = NULL;
    return result;
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

static void test_success(mode_t mode, const char *description)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char verifier_path[MAXPATHLEN + 1], backup_path[MAXPATHLEN + 1];
    char *record = make_record("alice", 'A', 'B');
    char *migrated = NULL, *backup = NULL;
    struct stat directory_st, verifier_st;
    struct test_context context = {.uid = getuid()};
    int passed = record != NULL && make_case(directory, path, record) == 0 &&
                 chmod(path, mode) == 0;

    if (passed) {
        passed = run_migration(path, &context) == 0;
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

    report(passed, description);
    free(migrated);
    free(backup);
    free(record);
    remove_case(directory);
}

static void test_final_record_without_newline(void)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char verifier_path[MAXPATHLEN + 1], backup_path[MAXPATHLEN + 1];
    char *record = make_record("alice", 'A', 'B');
    char *legacy = record == NULL ? NULL : strdup(record);
    char *migrated = NULL, *backup = NULL;
    struct test_context context = {.uid = getuid()};
    int passed = legacy != NULL;

    if (passed) {
        legacy[strlen(legacy) - 1] = '\0';
        passed = make_case(directory, path, legacy) == 0;
    }

    if (passed) {
        passed = run_migration(path, &context) == 0;
        snprintf(verifier_path, sizeof(verifier_path), "%s/%ju", path,
                 (uintmax_t)getuid());
        snprintf(backup_path, sizeof(backup_path), "%s.legacy", path);
        migrated = read_file(verifier_path);
        backup = read_file(backup_path);
        passed = passed && migrated != NULL && backup != NULL &&
                 strcmp(migrated, record) == 0 && strcmp(backup, legacy) == 0;
    }

    report(passed, "final legacy record without newline is normalized");
    free(migrated);
    free(backup);
    free(legacy);
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
    int passed = make_case(directory, path, source) == 0;

    if (passed) {
        passed = run_migration(path, &context) < 0 &&
                 source_is_unchanged(path, source) &&
                 count_case_entries(directory) == 1;
    }

    report(passed, name);
    remove_case(directory);
}

static void test_disabled_ownership(void)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char verifier_path[MAXPATHLEN + 1];
    char *record = make_record("alice", '*', '*');
    char *migrated = NULL;
    struct stat st;
    /* The disabled user's uid differs from the migration administrator's. */
    struct test_context context = {.uid = getuid() + 1};
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        passed = run_migration(path, &context) == 0;
        snprintf(verifier_path, sizeof(verifier_path), "%s/%ju", path,
                 (uintmax_t)context.uid);
        migrated = read_file(verifier_path);
        passed = passed && migrated != NULL && strcmp(migrated, record) == 0 &&
                 stat(verifier_path, &st) == 0 && st.st_uid == getuid() &&
                 (st.st_mode & 07777) == 0600 && st.st_nlink == 1;
    }

    report(passed, "disabled migrated verifier remains administrator-owned");
    free(migrated);
    free(record);
    remove_case(directory);
}

static void test_write_failure(void)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char *record = make_record("alice", '1', '2');
    struct test_context context = {.uid = getuid(), .fail_write = 1};
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        passed = run_migration(path, &context) < 0 &&
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
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        passed = run_migration(path, &context) < 0 &&
                 source_is_unchanged(path, record) &&
                 count_case_entries(directory) == 1;
    }

    report(passed, name);
    free(record);
    remove_case(directory);
}

static void test_rollback_cleanup_failure(int sync_call, int fail_unlink,
                                          const char *name)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char backup_path[MAXPATHLEN + 1], diagnostics[2048];
    char *record = make_record("alice", '1', '2');
    struct stat source_st, backup_st;
    struct test_context context = {
        .uid = getuid(),
        .fail_sync_call = sync_call,
        .fail_second_sync_call = fail_unlink ? 0 : sync_call + 2,
        .fail_unlink_path = fail_unlink ? "afppasswd.srp.legacy" : NULL
    };
    FILE *capture = tmpfile();
    int saved_stderr = dup(STDERR_FILENO);
    int passed = record != NULL && make_case(directory, path, record) == 0 &&
                 capture != NULL && saved_stderr >= 0;

    if (passed) {
        fflush(stderr);
        passed = dup2(fileno(capture), STDERR_FILENO) >= 0;

        if (passed) {
            int result = run_migration(path, &context);
            size_t length;
            fflush(stderr);
            passed = dup2(saved_stderr, STDERR_FILENO) >= 0;
            rewind(capture);
            length = fread(diagnostics, 1, sizeof(diagnostics) - 1, capture);
            diagnostics[length] = '\0';
            passed = passed && !ferror(capture) && feof(capture) && result < 0 &&
                     source_is_unchanged(path, record) &&
                     strstr(diagnostics, "recovery required") == NULL &&
                     strstr(diagnostics, "automatic rollback failed") == NULL;
            snprintf(backup_path, sizeof(backup_path), "%s.legacy", path);

            if (fail_unlink) {
                passed = passed && context.unlink_failures == 1 &&
                         context.sync_calls == sync_call + 1 &&
                         count_case_entries(directory) == 2 &&
                         stat(path, &source_st) == 0 &&
                         stat(backup_path, &backup_st) == 0 &&
                         source_st.st_dev == backup_st.st_dev &&
                         source_st.st_ino == backup_st.st_ino &&
                         strstr(diagnostics,
                                "rollback restored the source but could not remove afppasswd.srp.legacy") !=
                         NULL;
            } else {
                passed = passed && context.sync_calls == sync_call + 2 &&
                         count_case_entries(directory) == 1 &&
                         lstat(backup_path, &backup_st) < 0 && errno == ENOENT &&
                         strstr(diagnostics,
                                "rollback restored the source but could not synchronize removal of afppasswd.srp.legacy")
                         != NULL &&
                         strstr(diagnostics, "could not remove") == NULL;
            }
        }
    }

    if (saved_stderr >= 0) {
        close(saved_stderr);
    }

    if (capture != NULL) {
        fclose(capture);
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
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        snprintf(collision, sizeof(collision), "%s.legacy", path);
        snprintf(backup, sizeof(backup), "%s.legacy.1", path);
        passed = write_file(collision, "existing backup\n") == 0 &&
                 run_migration(path, &context) == 0;
        collision_data = read_file(collision);
        backup_data = read_file(backup);
        passed = passed && collision_data != NULL && backup_data != NULL &&
                 strcmp(collision_data, "existing backup\n") == 0 &&
                 strcmp(backup_data, record) == 0 &&
                 run_migration(path, &context) < 0;
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
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        passed = chmod(path, 0644) == 0 &&
                 run_migration(path, &context) < 0 &&
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
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        snprintf(link_path, sizeof(link_path), "%s/second-link", directory);
        passed = link(path, link_path) == 0 &&
                 run_migration(path, &context) < 0 &&
                 source_is_unchanged(path, record) &&
                 count_case_entries(directory) == 2;
    }

    report(passed, "hardlinked source is rejected without modification");
    free(record);
    remove_case(directory);
}

static void test_staging_collision(void)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char staging[MAXPATHLEN + 1], verifier[MAXPATHLEN + 1];
    char *record = make_record("alice", 'E', 'F');
    char *existing = NULL;
    struct test_context context = {.uid = getuid()};
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        snprintf(staging, sizeof(staging), "%s/.afppasswd.srp.migrate.%ju",
                 directory, (uintmax_t)getpid());
        snprintf(verifier, sizeof(verifier), "%s/%ju", staging,
                 (uintmax_t)context.uid);
        context.staging_collision_file = verifier;
        passed = run_migration(path, &context) < 0 &&
                 context.mkdir_calls == 1 &&
                 source_is_unchanged(path, record) &&
                 count_case_entries(directory) == 2 &&
                 count_case_entries(staging) == 1;
        existing = read_file(verifier);
        passed = passed && existing != NULL &&
                 strcmp(existing, "existing verifier\n") == 0;
    }

    report(passed, "staging collision preserves existing directory and verifier");
    free(existing);
    free(record);
    remove_case(directory);
}

static void test_partial_destination(void)
{
    char directory[MAXPATHLEN + 1] = {0}, path[MAXPATHLEN + 1];
    char staging[MAXPATHLEN + 1];
    char *record = make_record("alice", 'E', 'F');
    struct test_context context = {.uid = getuid()};
    int passed = record != NULL && make_case(directory, path, record) == 0;

    if (passed) {
        snprintf(staging, sizeof(staging),
                 "%s/.afppasswd.srp.migrate.interrupted", directory);
        passed = mkdir(staging, 0700) == 0 &&
                 run_migration(path, &context) < 0 &&
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

    test_success(0600, "successful migration preserves verifier and original");
    test_success(0400, "read-only legacy verifier migrates successfully");
    test_final_record_without_newline();
    test_disabled_ownership();
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
    test_rollback_cleanup_failure(4, 1,
                                  "pre-install rollback reports leftover backup without requiring recovery");
    test_rollback_cleanup_failure(5, 1,
                                  "post-install rollback reports leftover backup without requiring recovery");
    test_rollback_cleanup_failure(4, 0,
                                  "pre-install rollback reports unsynchronized backup removal without requiring recovery");
    test_rollback_cleanup_failure(5, 0,
                                  "post-install rollback reports unsynchronized backup removal without requiring recovery");
    test_backup_collision_and_repeat();
    test_unsafe_metadata();
    test_hardlinked_source();
    test_staging_collision();
    test_partial_destination();
    printf("1..%d\n", tests_run);
    free(alice);
    free(bob);
    free(unknown);
    free(duplicate);
    free(collision);
    return tests_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
