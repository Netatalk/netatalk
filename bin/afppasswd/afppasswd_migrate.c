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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <pwd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <atalk/compat.h>

#include "afppasswd_migrate.h"

#define SRP_HEX_SALT_LEN 32
#define SRP_HEX_V_LEN 384
#define SRP_FIELDS_LEN (SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN + 1)
#define USERNAME_MAX_LEN 255
#define MIGRATE_ATTEMPTS 10000

struct migrate_record {
    char *line;
    size_t length;
    char *username;
    uid_t uid;
};

static int default_lookup_uid(const char *name, uid_t *uid, void *context)
{
    const struct passwd *pwd;
    (void)context;
    errno = 0;
    pwd = getpwnam(name);

    if (pwd == NULL) {
        return -1;
    }

    *uid = pwd->pw_uid;
    return 0;
}

static ssize_t default_write_data(int fd, const void *data, size_t count,
                                  void *context)
{
    (void)context;
    return write(fd, data, count);
}

static int default_sync_fd(int fd, void *context)
{
    (void)context;
    return fsync(fd);
}

static int valid_username(const char *name)
{
    size_t length = strnlen(name, USERNAME_MAX_LEN + 1);
    return length > 0 && length <= USERNAME_MAX_LEN &&
           strchr(name, ':') == NULL && strchr(name, '\n') == NULL &&
           strchr(name, '\r') == NULL;
}

static int valid_srp_fields(const char *fields)
{
    int salt_disabled = 1, verifier_disabled = 1;

    if (strlen(fields) != SRP_FIELDS_LEN ||
            fields[SRP_HEX_SALT_LEN] != ':' ||
            fields[SRP_FIELDS_LEN - 1] != '\n') {
        return 0;
    }

    for (size_t i = 0; i < SRP_HEX_SALT_LEN; i++) {
        salt_disabled &= fields[i] == '*';
    }

    for (size_t i = 0; i < SRP_HEX_V_LEN; i++) {
        verifier_disabled &= fields[SRP_HEX_SALT_LEN + 1 + i] == '*';
    }

    if (salt_disabled || verifier_disabled) {
        return salt_disabled && verifier_disabled;
    }

    for (size_t i = 0; i < SRP_HEX_SALT_LEN; i++) {
        if (!isxdigit((unsigned char)fields[i])) {
            return 0;
        }
    }

    for (size_t i = 0; i < SRP_HEX_V_LEN; i++) {
        if (!isxdigit((unsigned char)fields[SRP_HEX_SALT_LEN + 1 + i])) {
            return 0;
        }
    }

    return 1;
}

static int split_path(const char *path, char parent[MAXPATHLEN + 1],
                      char basename[MAXPATHLEN + 1])
{
    char copy[MAXPATHLEN + 1];
    char *slash;
    size_t length;

    if (path == NULL || path[0] == '\0' ||
            strlcpy(copy, path, sizeof(copy)) >= sizeof(copy)) {
        fprintf(stderr, "afppasswd: migration path is empty or too long.\n");
        return -1;
    }

    length = strlen(copy);

    if (length > 1 && copy[length - 1] == '/') {
        fprintf(stderr, "afppasswd: migration path must name a file.\n");
        return -1;
    }

    slash = strrchr(copy, '/');

    if (slash == NULL) {
        strlcpy(parent, ".", MAXPATHLEN + 1);
        strlcpy(basename, copy, MAXPATHLEN + 1);
    } else if (slash == copy) {
        strlcpy(parent, "/", MAXPATHLEN + 1);
        strlcpy(basename, slash + 1, MAXPATHLEN + 1);
    } else {
        *slash = '\0';
        strlcpy(parent, copy, MAXPATHLEN + 1);
        strlcpy(basename, slash + 1, MAXPATHLEN + 1);
    }

    if (basename[0] == '\0' || strcmp(basename, ".") == 0 ||
            strcmp(basename, "..") == 0) {
        fprintf(stderr, "afppasswd: migration path must name a file.\n");
        return -1;
    }

    return 0;
}

static int validate_parent(int fd, const char *path, uid_t administrator_uid)
{
    struct stat st;

    if (fstat(fd, &st) < 0 || !S_ISDIR(st.st_mode) ||
            st.st_uid != administrator_uid ||
            (st.st_mode & (S_IWGRP | S_IWOTH))) {
        fprintf(stderr,
                "afppasswd: directory containing %s must be administrator-owned and not writable by group or other.\n",
                path);
        return -1;
    }

    return 0;
}

static int validate_source(int fd, const char *path, uid_t administrator_uid,
                           struct stat *st)
{
    if (fstat(fd, st) < 0) {
        fprintf(stderr, "afppasswd: can't inspect legacy SRP file %s: %s\n",
                path, strerror(errno));
        return -1;
    }

    if (!S_ISREG(st->st_mode) || st->st_uid != administrator_uid ||
            (st->st_mode & (S_IRWXG | S_IRWXO)) || st->st_nlink != 1) {
        fprintf(stderr,
                "afppasswd: legacy SRP file %s must be a single-link regular file owned by the administrator and accessible only by its owner.\n",
                path);
        return -1;
    }

    return 0;
}

static void free_records(struct migrate_record *records, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        free(records[i].line);
        free(records[i].username);
    }

    free(records);
}

static int add_record(struct migrate_record **records, size_t *count,
                      size_t *capacity, const char *line, size_t length,
                      const struct afppasswd_migrate_hooks *hooks,
                      size_t line_number)
{
    struct migrate_record *grown;
    struct migrate_record record = {0};
    const char *colon = memchr(line, ':', length);
    size_t username_length;

    if (colon == NULL || memchr(line, '\0', length) != NULL) {
        goto malformed;
    }

    username_length = (size_t)(colon - line);
    record.username = strndup(line, username_length);

    if (record.username == NULL) {
        fprintf(stderr, "afppasswd: out of memory while migrating.\n");
        return -1;
    }

    if (!valid_username(record.username) ||
            !valid_srp_fields(colon + 1)) {
        goto malformed;
    }

    if (hooks->lookup_uid(record.username, &record.uid,
                          hooks->context) < 0) {
        fprintf(stderr,
                "afppasswd: legacy SRP record %zu names unknown local user %s.\n",
                line_number, record.username);
        free(record.username);
        return -1;
    }

    for (size_t i = 0; i < *count; i++) {
        if (strcmp((*records)[i].username, record.username) == 0) {
            fprintf(stderr,
                    "afppasswd: legacy SRP file contains duplicate user %s.\n",
                    record.username);
            free(record.username);
            return -1;
        }

        if ((*records)[i].uid == record.uid) {
            fprintf(stderr,
                    "afppasswd: legacy SRP users %s and %s resolve to the same uid %ju.\n",
                    (*records)[i].username, record.username,
                    (uintmax_t)record.uid);
            free(record.username);
            return -1;
        }
    }

    record.line = malloc(length + 1);

    if (record.line == NULL) {
        fprintf(stderr, "afppasswd: out of memory while migrating.\n");
        free(record.username);
        return -1;
    }

    memcpy(record.line, line, length);
    record.line[length] = '\0';
    record.length = length;

    if (*count == *capacity) {
        size_t new_capacity = *capacity == 0 ? 16 : *capacity * 2;

        if (new_capacity < *capacity ||
                new_capacity > SIZE_MAX / sizeof(**records)) {
            fprintf(stderr, "afppasswd: too many legacy SRP records.\n");
            free(record.line);
            free(record.username);
            return -1;
        }

        grown = realloc(*records, new_capacity * sizeof(**records));

        if (grown == NULL) {
            fprintf(stderr, "afppasswd: out of memory while migrating.\n");
            free(record.line);
            free(record.username);
            return -1;
        }

        *records = grown;
        *capacity = new_capacity;
    }

    (*records)[(*count)++] = record;
    return 0;
malformed:
    fprintf(stderr, "afppasswd: malformed legacy SRP record %zu.\n",
            line_number);
    free(record.username);
    return -1;
}

static int read_records(int source_fd, struct migrate_record **records,
                        size_t *count,
                        const struct afppasswd_migrate_hooks *hooks)
{
    FILE *stream;
    char *line = NULL;
    size_t size = 0, capacity = 0, line_number = 0;
    ssize_t length;
    int stream_fd = dup(source_fd);
    int result = -1;

    if (stream_fd < 0 || lseek(stream_fd, 0, SEEK_SET) < 0 ||
            (stream = fdopen(stream_fd, "r")) == NULL) {
        fprintf(stderr, "afppasswd: can't read legacy SRP file: %s\n",
                strerror(errno));

        if (stream_fd >= 0) {
            close(stream_fd);
        }

        return -1;
    }

    while ((length = getline(&line, &size, stream)) >= 0) {
        line_number++;

        if ((size_t)length > USERNAME_MAX_LEN + 1 + SRP_FIELDS_LEN) {
            fprintf(stderr, "afppasswd: malformed legacy SRP record %zu.\n",
                    line_number);
            goto done;
        }

        if (add_record(records, count, &capacity, line, (size_t)length,
                       hooks, line_number) < 0) {
            goto done;
        }
    }

    if (ferror(stream)) {
        fprintf(stderr, "afppasswd: can't read legacy SRP file: %s\n",
                strerror(errno));
        goto done;
    }

    result = 0;
done:
    free(line);
    fclose(stream);
    return result;
}

static int source_unchanged(int source_fd,
                            const struct migrate_record *records,
                            size_t count)
{
    char buffer[4096];
    size_t record = 0, offset = 0;
    ssize_t length;

    if (lseek(source_fd, 0, SEEK_SET) < 0) {
        return 0;
    }

    while ((length = read(source_fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < length; i++) {
            if (record >= count ||
                    buffer[i] != records[record].line[offset++]) {
                return 0;
            }

            if (offset == records[record].length) {
                record++;
                offset = 0;
            }
        }
    }

    return length == 0 && record == count && offset == 0;
}

static int write_all(int fd, const char *data, size_t length,
                     const struct afppasswd_migrate_hooks *hooks)
{
    while (length > 0) {
        ssize_t written = hooks->write_data(fd, data, length, hooks->context);

        if (written < 0) {
            return -1;
        }

        if (written == 0) {
            errno = EIO;
            return -1;
        }

        data += written;
        length -= (size_t)written;
    }

    return 0;
}

static int uid_filename(uid_t uid, char *name, size_t size)
{
    int length = snprintf(name, size, "%ju", (uintmax_t)uid);
    return length < 0 || (size_t)length >= size ? -1 : 0;
}

static int create_verifiers(int directory_fd, const char *path,
                            const struct migrate_record *records, size_t count,
                            const struct afppasswd_migrate_hooks *hooks)
{
    char name[3 * sizeof(uid_t) + 1];

    for (size_t i = 0; i < count; i++) {
        struct stat st;
        int fd;

        if (uid_filename(records[i].uid, name, sizeof(name)) < 0) {
            return -1;
        }

        fd = openat(directory_fd, name,
                    O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW,
                    0600);

        if (fd < 0) {
            fprintf(stderr,
                    "afppasswd: can't create migrated verifier %s/%s: %s\n",
                    path, name, strerror(errno));
            return -1;
        }

        if (fchown(fd, records[i].uid, (gid_t) -1) < 0 ||
                fchmod(fd, 0600) < 0 ||
                write_all(fd, records[i].line, records[i].length, hooks) < 0 ||
                hooks->sync_fd(fd, hooks->context) < 0 ||
                fstat(fd, &st) < 0) {
            fprintf(stderr,
                    "afppasswd: can't create migrated verifier %s/%s: %s\n",
                    path, name, strerror(errno));
            close(fd);
            return -1;
        }

        if (!S_ISREG(st.st_mode) || st.st_uid != records[i].uid ||
                (st.st_mode & (S_IRWXG | S_IRWXO)) || st.st_nlink != 1) {
            fprintf(stderr,
                    "afppasswd: migrated verifier %s/%s has unsafe metadata.\n",
                    path, name);
            close(fd);
            return -1;
        }

        if (close(fd) < 0) {
            fprintf(stderr,
                    "afppasswd: can't close migrated verifier %s/%s: %s\n",
                    path, name, strerror(errno));
            return -1;
        }
    }

    return 0;
}

static void remove_staging(int parent_fd, const char *temporary_name,
                           const struct migrate_record *records, size_t count)
{
    char name[3 * sizeof(uid_t) + 1];
    int directory_fd = openat(parent_fd, temporary_name,
                              O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);

    if (directory_fd < 0) {
        return;
    }

    for (size_t i = 0; i < count; i++) {
        if (uid_filename(records[i].uid, name, sizeof(name)) == 0) {
            unlinkat(directory_fd, name, 0);
        }
    }

    close(directory_fd);
    unlinkat(parent_fd, temporary_name, AT_REMOVEDIR);
}

static int create_staging_directory(int parent_fd, const char *basename,
                                    uid_t administrator_uid,
                                    char temporary_name[MAXPATHLEN + 1])
{
    for (unsigned int attempt = 0; attempt < MIGRATE_ATTEMPTS; attempt++) {
        int length = snprintf(temporary_name, MAXPATHLEN + 1,
                              ".%s.migrate.%ju.%u", basename,
                              (uintmax_t)getpid(), attempt);

        if (length < 0 || length > MAXPATHLEN) {
            fprintf(stderr,
                    "afppasswd: migration path is too long for a temporary sibling.\n");
            return -1;
        }

        if (mkdirat(parent_fd, temporary_name, 0700) == 0) {
            int fd = openat(parent_fd, temporary_name,
                            O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);

            if (fd < 0 || fchown(fd, administrator_uid, (gid_t) -1) < 0 ||
                    fchmod(fd, 0700) < 0) {
                fprintf(stderr,
                        "afppasswd: can't prepare migration directory: %s\n",
                        strerror(errno));

                if (fd >= 0) {
                    close(fd);
                }

                unlinkat(parent_fd, temporary_name, AT_REMOVEDIR);
                return -1;
            }

            return fd;
        }

        if (errno != EEXIST) {
            fprintf(stderr,
                    "afppasswd: can't create migration directory: %s\n",
                    strerror(errno));
            return -1;
        }
    }

    fprintf(stderr, "afppasswd: can't allocate a migration directory name.\n");
    return -1;
}

static int source_still_at_path(int parent_fd, const char *basename,
                                const struct stat *source_st)
{
    struct stat st;
    return fstatat(parent_fd, basename, &st, AT_SYMLINK_NOFOLLOW) == 0 &&
           st.st_dev == source_st->st_dev && st.st_ino == source_st->st_ino &&
           st.st_nlink == 1;
}

static int reject_stale_staging(int parent_fd, const char *basename,
                                const char *path)
{
    char prefix[MAXPATHLEN + 1];
    DIR *directory;
    struct dirent *entry;
    int scan_fd, length;
    size_t prefix_length;
    length = snprintf(prefix, sizeof(prefix), ".%s.migrate.", basename);

    if (length < 0 || (size_t)length >= sizeof(prefix)) {
        fprintf(stderr,
                "afppasswd: migration path is too long to inspect temporary siblings.\n");
        return -1;
    }

    prefix_length = (size_t)length;
    scan_fd = openat(parent_fd, ".",
                     O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);

    if (scan_fd < 0 || (directory = fdopendir(scan_fd)) == NULL) {
        fprintf(stderr,
                "afppasswd: can't inspect migration directory for %s: %s\n",
                path, strerror(errno));

        if (scan_fd >= 0) {
            close(scan_fd);
        }

        return -1;
    }

    errno = 0;

    while ((entry = readdir(directory)) != NULL) {
        if (strncmp(entry->d_name, prefix, prefix_length) == 0) {
            fprintf(stderr,
                    "afppasswd: found partial migration sibling %s; inspect or remove it before retrying.\n",
                    entry->d_name);
            closedir(directory);
            return -1;
        }
    }

    if (errno != 0) {
        fprintf(stderr,
                "afppasswd: can't inspect migration directory for %s: %s\n",
                path, strerror(errno));
        closedir(directory);
        return -1;
    }

    closedir(directory);
    return 0;
}

static int link_backup(int parent_fd, const char *basename,
                       char backup_name[MAXPATHLEN + 1])
{
    for (unsigned int attempt = 0; attempt < MIGRATE_ATTEMPTS; attempt++) {
        int length;

        if (attempt == 0) {
            length = snprintf(backup_name, MAXPATHLEN + 1, "%s.legacy",
                              basename);
        } else {
            length = snprintf(backup_name, MAXPATHLEN + 1, "%s.legacy.%u",
                              basename, attempt);
        }

        if (length < 0 || length > MAXPATHLEN) {
            fprintf(stderr,
                    "afppasswd: migration path is too long for a backup sibling.\n");
            return -1;
        }

        if (linkat(parent_fd, basename, parent_fd, backup_name, 0) == 0) {
            return 0;
        }

        if (errno != EEXIST) {
            fprintf(stderr, "afppasswd: can't retain legacy SRP file: %s\n",
                    strerror(errno));
            return -1;
        }
    }

    fprintf(stderr, "afppasswd: can't allocate a legacy backup name.\n");
    return -1;
}

static int restore_source(int parent_fd, const char *basename,
                          const char *backup_name,
                          const struct afppasswd_migrate_hooks *hooks)
{
    if (linkat(parent_fd, backup_name, parent_fd, basename, 0) < 0 ||
            hooks->sync_fd(parent_fd, hooks->context) < 0) {
        fprintf(stderr,
                "afppasswd: automatic rollback failed; the original remains at %s.\n",
                backup_name);
        return -1;
    }

    if (unlinkat(parent_fd, backup_name, 0) < 0 ||
            hooks->sync_fd(parent_fd, hooks->context) < 0) {
        fprintf(stderr,
                "afppasswd: rollback restored the source but could not remove %s.\n",
                backup_name);
        return -1;
    }

    return 0;
}

int afppasswd_migrate_srp(const char *path, uid_t administrator_uid,
                          const struct afppasswd_migrate_hooks *supplied_hooks)
{
    struct afppasswd_migrate_hooks hooks = {
        default_lookup_uid, default_write_data, default_sync_fd, NULL
    };
    struct migrate_record *records = NULL;
    struct stat source_st;
    struct flock lock = {0};
    char parent[MAXPATHLEN + 1], basename[MAXPATHLEN + 1];
    char temporary_name[MAXPATHLEN + 1] = {0};
    char backup_name[MAXPATHLEN + 1] = {0};
    int parent_fd = -1, source_fd = -1, temporary_fd = -1;
    size_t count = 0;
    int result = -1, source_unlinked = 0, directory_installed = 0;

    if (supplied_hooks != NULL) {
        hooks = *supplied_hooks;

        if (hooks.lookup_uid == NULL) {
            hooks.lookup_uid = default_lookup_uid;
        }

        if (hooks.write_data == NULL) {
            hooks.write_data = default_write_data;
        }

        if (hooks.sync_fd == NULL) {
            hooks.sync_fd = default_sync_fd;
        }
    }

    if (split_path(path, parent, basename) < 0) {
        goto done;
    }

    parent_fd = open(parent, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);

    if (parent_fd < 0 || validate_parent(parent_fd, path,
                                         administrator_uid) < 0) {
        if (parent_fd < 0) {
            fprintf(stderr, "afppasswd: can't open directory for %s: %s\n",
                    path, strerror(errno));
        }

        goto done;
    }

    source_fd = openat(parent_fd, basename,
                       O_RDONLY | O_NONBLOCK | O_CLOEXEC | O_NOFOLLOW);

    if (source_fd < 0) {
        struct stat st;

        if (fstatat(parent_fd, basename, &st, AT_SYMLINK_NOFOLLOW) == 0 &&
                S_ISDIR(st.st_mode)) {
            fprintf(stderr,
                    "afppasswd: %s is already an SRP verifier directory; there is no legacy file to migrate.\n",
                    path);
        } else {
            fprintf(stderr, "afppasswd: can't open legacy SRP file %s: %s\n",
                    path, strerror(errno));
        }

        goto done;
    }

    if (fstat(source_fd, &source_st) == 0 && S_ISDIR(source_st.st_mode)) {
        fprintf(stderr,
                "afppasswd: %s is already an SRP verifier directory; there is no legacy file to migrate.\n",
                path);
        goto done;
    }

    if (validate_source(source_fd, path, administrator_uid, &source_st) < 0) {
        goto done;
    }

    lock.l_type = F_RDLCK;
    lock.l_whence = SEEK_SET;

    if (fcntl(source_fd, F_SETLK, &lock) < 0) {
        fprintf(stderr,
                "afppasswd: legacy SRP file is busy; stop afpd and retry migration.\n");
        goto done;
    }

    if (read_records(source_fd, &records, &count, &hooks) < 0) {
        goto done;
    }

    if (reject_stale_staging(parent_fd, basename, path) < 0) {
        goto done;
    }

    temporary_fd = create_staging_directory(parent_fd, basename,
                                            administrator_uid,
                                            temporary_name);

    if (temporary_fd < 0) {
        goto done;
    }

    if (create_verifiers(temporary_fd, path, records, count, &hooks) < 0 ||
            fchmod(temporary_fd, 0755) < 0 ||
            hooks.sync_fd(temporary_fd, hooks.context) < 0) {
        fprintf(stderr,
                "afppasswd: migration staging failed; the legacy file was not changed.\n");
        goto done;
    }

    if (!source_unchanged(source_fd, records, count) ||
            !source_still_at_path(parent_fd, basename, &source_st)) {
        fprintf(stderr,
                "afppasswd: legacy SRP file changed during migration; stop afpd and retry.\n");
        goto done;
    }

    if (link_backup(parent_fd, basename, backup_name) < 0) {
        goto done;
    }

    if (hooks.sync_fd(parent_fd, hooks.context) < 0) {
        fprintf(stderr,
                "afppasswd: can't synchronize legacy SRP backup: %s\n",
                strerror(errno));
        unlinkat(parent_fd, backup_name, 0);
        hooks.sync_fd(parent_fd, hooks.context);
        backup_name[0] = '\0';
        goto done;
    }

    if (unlinkat(parent_fd, basename, 0) < 0) {
        fprintf(stderr, "afppasswd: can't prepare SRP directory install: %s\n",
                strerror(errno));
        unlinkat(parent_fd, backup_name, 0);
        hooks.sync_fd(parent_fd, hooks.context);
        backup_name[0] = '\0';
        goto done;
    }

    source_unlinked = 1;

    if (hooks.sync_fd(parent_fd, hooks.context) < 0 ||
            renameat(parent_fd, temporary_name, parent_fd, basename) < 0) {
        int rollback_result;
        fprintf(stderr,
                "afppasswd: can't install SRP verifier directory; rolling back: %s\n",
                strerror(errno));
        rollback_result = restore_source(parent_fd, basename, backup_name,
                                         &hooks);

        if (rollback_result == 0) {
            source_unlinked = 0;
            backup_name[0] = '\0';
        }

        goto done;
    }

    directory_installed = 1;
    close(temporary_fd);
    temporary_fd = -1;

    if (hooks.sync_fd(parent_fd, hooks.context) < 0) {
        fprintf(stderr,
                "afppasswd: can't synchronize installed SRP directory; rolling back: %s\n",
                strerror(errno));

        if (renameat(parent_fd, basename, parent_fd, temporary_name) == 0) {
            int rollback_result;
            directory_installed = 0;
            rollback_result = restore_source(parent_fd, basename, backup_name,
                                             &hooks);

            if (rollback_result == 0) {
                source_unlinked = 0;
                backup_name[0] = '\0';
            }
        } else {
            fprintf(stderr,
                    "afppasswd: automatic rollback failed; the original remains at %s.\n",
                    backup_name);
        }

        goto done;
    }

    printf("afppasswd: migrated %zu SRP verifier%s; original retained as %s/%s\n",
           count, count == 1 ? "" : "s", parent, backup_name);
    result = 0;
done:

    if (temporary_fd >= 0) {
        close(temporary_fd);
    }

    if (result < 0 && temporary_name[0] != '\0' && !directory_installed) {
        remove_staging(parent_fd, temporary_name, records, count);
    }

    if (source_unlinked && !directory_installed) {
        fprintf(stderr,
                "afppasswd: recovery required; the original remains at %s/%s.\n",
                parent, backup_name);
    }

    if (source_fd >= 0) {
        close(source_fd);
    }

    if (parent_fd >= 0) {
        close(parent_fd);
    }

    free_records(records, count);
    return result;
}
