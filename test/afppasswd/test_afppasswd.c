/*
 * Tests for afppasswd's SRP verifier validation
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
#include <pwd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* Declared in <atalk/util.h>; avoid its unrelated bstring dependency here. */
extern const char *tmpdir(void);

/* Keep the implementation's small internal helpers testable without making
 * them part of afppasswd's public interface. */
static int test_fstat(int fd, struct stat *st);
static int test_fchown(int fd, uid_t owner, gid_t group);
static int test_fchmod(int fd, mode_t mode);
static int test_fcntl(int fd, int cmd, ...);
static int test_fsync(int fd);
static void test_setpwent(void);
static struct passwd *test_getpwent(void);
static void test_endpwent(void);
static struct passwd *test_getpwuid(uid_t uid);
static char *test_getpass(const char *prompt);
#define fstat test_fstat
#define fchown test_fchown
#define fchmod test_fchmod
#define fcntl test_fcntl
#define fsync test_fsync
#define setpwent test_setpwent
#define getpwent test_getpwent
#define endpwent test_endpwent
#define getpwuid test_getpwuid
#define getpass test_getpass
#define main afppasswd_program_main
#include "../../bin/afppasswd/afppasswd.c"
#undef main
#undef fstat
#undef fchown
#undef fchmod
#undef fcntl
#undef fsync
#undef setpwent
#undef getpwent
#undef endpwent
#undef getpwuid
#undef getpass

static struct stat verifier_directory;
static int mock_directory_owner;
static const char *mock_verifier_path;
static uid_t mock_verifier_owner;
static int fail_verifier_sync;
static int fail_verifier_chown;
static int fail_verifier_chmod;
static int fail_verifier_lock;
static int verifier_syncs;
static int enrollment_before_sync;
static struct passwd *mock_passwd;
static int passwd_returned;

static int is_mock_verifier(int fd)
{
    struct stat file_st, path_st;
    return mock_verifier_path != NULL && fstat(fd, &file_st) == 0 &&
           stat(mock_verifier_path, &path_st) == 0 &&
           file_st.st_dev == path_st.st_dev && file_st.st_ino == path_st.st_ino;
}

static int test_fstat(int fd, struct stat *st)
{
    int result = fstat(fd, st);

    /* Only the temporary verifier directory needs simulated root ownership. */
    if (result == 0 && mock_directory_owner &&
            st->st_dev == verifier_directory.st_dev &&
            st->st_ino == verifier_directory.st_ino) {
        st->st_uid = 0;
    }

    if (result == 0 && is_mock_verifier(fd)) {
        st->st_uid = mock_verifier_owner;
    }

    return result;
}

static int test_fchown(int fd, uid_t owner, gid_t group)
{
    if (!is_mock_verifier(fd)) {
        return fchown(fd, owner, group);
    }

    if (fail_verifier_chown) {
        errno = EPERM;
        return -1;
    }

    if (owner != 0 && verifier_syncs == 0) {
        enrollment_before_sync = 1;
    }

    mock_verifier_owner = owner;
    return 0;
}

static int test_fchmod(int fd, mode_t mode)
{
    if (is_mock_verifier(fd) && fail_verifier_chmod) {
        errno = EPERM;
        return -1;
    }

    return fchmod(fd, mode);
}

static int test_fcntl(int fd, int cmd, ...)
{
    va_list args;
    void *arg;
    va_start(args, cmd);
    arg = va_arg(args, void *);
    va_end(args);

    if (is_mock_verifier(fd) && fail_verifier_lock && cmd == F_SETLK) {
        errno = EACCES;
        return -1;
    }

    return fcntl(fd, cmd, arg);
}

static int test_fsync(int fd)
{
    if (is_mock_verifier(fd)) {
        if (fail_verifier_sync) {
            errno = EIO;
            return -1;
        }

        verifier_syncs++;
    }

    return fsync(fd);
}

static void test_setpwent(void)
{
    if (mock_passwd != NULL) {
        passwd_returned = 0;
    } else {
        setpwent();
    }
}

static struct passwd *test_getpwent(void)
{
    if (mock_passwd != NULL) {
        return passwd_returned++ == 0 ? mock_passwd : NULL;
    }

    return getpwent();
}

static void test_endpwent(void)
{
    if (mock_passwd == NULL) {
        endpwent();
    }
}

/*!
 * @brief getpwuid() over the file's account fixture
 *
 * @returns the fixture when one is set, else the real record
 */
static struct passwd *test_getpwuid(uid_t uid)
{
    return mock_passwd != NULL ? mock_passwd : getpwuid(uid);
}

static const char *mock_prompt_answers[2];
static int mock_prompts;

/*!
 * @brief getpass() over a scripted pair of answers
 *
 * getpass() hands out one static buffer, which afppasswd wipes after use, so
 * this hands out one too and counts the calls.
 *
 * @returns the next scripted answer, or NULL once they run out
 */
static char *test_getpass(const char *prompt)
{
    static char answer[SRP_PASSWDLEN + 1];
    const char *next = mock_prompts < 2 ? mock_prompt_answers[mock_prompts] : NULL;
    (void)prompt;
    mock_prompts++;

    if (next == NULL) {
        return NULL;
    }

    snprintf(answer, sizeof(answer), "%s", next);
    return answer;
}

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

static void test_rejected_verifier_does_not_change_metadata(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char verifier[MAXPATHLEN + 1];
    char alias[MAXPATHLEN + 1];
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
        fd = open_srp_verifier(dirfd, directory, getuid(), 0);

        if (fd >= 0) {
            close(fd);
        }

        if (fd < 0 && stat(verifier, &after) == 0 &&
                (before.st_mode & 0777) == (after.st_mode & 0777) &&
                before.st_uid == after.st_uid && before.st_nlink == after.st_nlink) {
            passed = 1;
        }
    }

    report(passed, "rejected verifier leaves metadata unchanged");
    passed = 0;

    if (dirfd >= 0 &&
            snprintf(alias, sizeof(alias), "%s/alias", directory) > 0 &&
            link(verifier, alias) == 0) {
        fd = open_srp_verifier(dirfd, directory, getuid(), 1);
        passed = fd < 0 && stat(verifier, &after) == 0 &&
                 (after.st_mode & 07777) == 0640 && after.st_nlink == 2;

        if (fd >= 0) {
            close(fd);
        }

        unlink(alias);
    }

    report(passed, "root rejects hard-linked verifier before permission repair");

    if (dirfd >= 0) {
        close(dirfd);
    }

    unlink(verifier);
    rmdir(directory);
}

static void test_verifier_permissions(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char verifier[MAXPATHLEN + 1] = {0};
    int fd = -1;
    const struct {
        mode_t mode;
        int safe;
        const char *description;
    } cases[] = {
        {0400, 1, "read-only owner verifier passes validation"},
        {0600, 1, "read-write owner verifier passes validation"},
        {0700, 1, "owner execute permission passes validation"},
        {0000, 1, "no access permissions passes metadata validation"},
        {0640, 0, "group read permission is rejected"},
        {0620, 0, "group write permission is rejected"},
        {0610, 0, "group execute permission is rejected"},
        {0604, 0, "other read permission is rejected"},
        {0602, 0, "other write permission is rejected"},
        {0601, 0, "other execute permission is rejected"},
    };

    if (make_case(directory) != 0 ||
            snprintf(verifier, sizeof(verifier), "%s/verifier", directory) <= 0 ||
            (fd = open(verifier, O_CREAT | O_EXCL | O_RDWR, 0600)) < 0) {
        report(0, "prepare verifier permission tests");
        rmdir(directory);
        return;
    }

    /* Keep the descriptor open so the test checks metadata, not open access. */
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        report(fchmod(fd, cases[i].mode) == 0 &&
               (validate_srp_verifier_file(fd, getuid(), verifier, 0) == 0) ==
               cases[i].safe, cases[i].description);
        report(validate_srp_verifier_file(fd, getuid(), verifier, 1) == 0,
               "administrative validation accepts existing permission mode");
    }

    mock_verifier_path = verifier;
    mock_verifier_owner = getuid() == 1 ? 2 : 1;
    report(validate_srp_verifier_file(fd, getuid(), verifier, 1) < 0,
           "administrative validation still rejects unrelated ownership");
    mock_verifier_path = NULL;
    close(fd);
    unlink(verifier);
    rmdir(directory);
}

static void test_single_record_updates(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char verifier[MAXPATHLEN + 1] = {0};
    unsigned char salt[SRP_SALT_LEN] = {0};
    unsigned char value[SRP_NBYTES] = {0};
    char hex[SRP_HEX_SALT_LEN + 1 + SRP_HEX_V_LEN];
    char matching[sizeof(hex) + 32];
    char stale[sizeof(hex) + 32];
    char stale_then_matching[2 * sizeof(matching)];
    char matching_then_stale[2 * sizeof(matching)];
    char actual[2 * sizeof(matching)];
    int root_flags = OPT_ISROOT | OPT_ADDUSER | OPT_NOCRACK;
    srp_encode_hex(hex, salt, value);
    snprintf(matching, sizeof(matching), "alice:%.*s\n", (int)sizeof(hex), hex);
    snprintf(stale, sizeof(stale), "previous_user:%.*s\n", (int)sizeof(hex), hex);
    snprintf(stale_then_matching, sizeof(stale_then_matching), "%s%s", stale,
             matching);
    snprintf(matching_then_stale, sizeof(matching_then_stale), "%s%s", matching,
             stale);
    const struct {
        const char *description;
        const char *contents;
        int flags;
        int succeeds;
    } cases[] = {
        {
            "matching final record after stale record is rejected unchanged",
            stale_then_matching, root_flags, 0
        },
        {
            "trailing record is rejected unchanged",
            matching_then_stale, root_flags, 0
        },
        { "root updates a single matching record", matching, root_flags, 1 },
        { "root replaces a longer stale username", stale, root_flags, 1 },
        { "root initializes an empty verifier", "", root_flags, 1 },
        { "root creates a missing verifier", NULL, root_flags, 1 },
        { "non-root rejects a stale username unchanged", stale, 0, 0 },
        { "non-root rejects an empty verifier unchanged", "", 0, 0 },
        {
            "malformed matching record is rejected unchanged",
            "alice:invalid\n", root_flags, 0
        },
    };

    if (make_case(directory) != 0 || stat(directory, &verifier_directory) != 0 ||
            snprintf(verifier, sizeof(verifier), "%s/%ju", directory,
                     (uintmax_t)getuid()) <= 0) {
        report(0, "prepare single-record update tests");
        rmdir(directory);
        return;
    }

    mock_directory_owner = 1;
    mock_verifier_path = verifier;

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        int passed = 0;
        mock_verifier_owner = cases[i].contents == NULL ? 0 : getuid();
        verifier_syncs = 0;

        if (cases[i].contents == NULL ||
                write_file(verifier, cases[i].contents) == 0) {
            int result = update_srp_passwd(directory, "alice", getuid(),
                                           cases[i].flags, "new-password");
            FILE *fp = fopen(verifier, "r");

            if (fp != NULL) {
                size_t length = fread(actual, 1, sizeof(actual) - 1, fp);
                actual[length] = '\0';

                if (!ferror(fp) && feof(fp)) {
                    if (cases[i].succeeds) {
                        passed = result == 0 && length == strlen(matching) &&
                                 strncmp(actual, "alice:", 6) == 0 &&
                                 srp_valid_fields(actual + 6) &&
                                 strcmp(actual, matching) != 0;
                    } else {
                        passed = result < 0 && length == strlen(cases[i].contents) &&
                                 memcmp(actual, cases[i].contents, length) == 0;
                    }
                }

                fclose(fp);
            }
        }

        report(passed, cases[i].description);
        unlink(verifier);
    }

    mock_directory_owner = 0;
    mock_verifier_path = NULL;
    rmdir(directory);
}

static void test_enrollment_ownership(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char verifier[MAXPATHLEN + 1];
    char line[SRP_HEX_SALT_LEN + SRP_HEX_V_LEN + 32];
    struct passwd account = {.pw_name = "alice", .pw_uid = getuid() == 0 ? 1 : getuid()};
    int flags = OPT_ISROOT | OPT_ADDUSER | OPT_NOCRACK;
    FILE *fp;
    int result;

    if (make_case(directory) != 0 || stat(directory, &verifier_directory) != 0 ||
            snprintf(verifier, sizeof(verifier), "%s/%ju", directory,
                     (uintmax_t)account.pw_uid) <= 0) {
        report(0, "prepare enrollment tests");
        rmdir(directory);
        return;
    }

    mock_directory_owner = 1;
    mock_verifier_path = verifier;
    mock_verifier_owner = 0;
    mock_passwd = &account;
    verifier_syncs = 0;
    enrollment_before_sync = 0;
    result = disable_srp_verifier(directory, account.pw_name, account.pw_uid);
    fp = fopen(verifier, "r");
    int placeholder = fp != NULL && fgets(line, sizeof(line), fp) != NULL &&
                      fgetc(fp) == EOF && strncmp(line, "alice:*", 7) == 0 &&
                      srp_valid_fields(line + 6);

    if (fp != NULL) {
        fclose(fp);
    }

    report(result == 0 && placeholder && mock_verifier_owner == 0,
           "disable creates a root-owned placeholder for a missing verifier");
    report(update_srp_passwd(directory, "alice", account.pw_uid, 0,
                             "new-password") < 0 && mock_verifier_owner == 0,
           "non-root cannot enroll a root-owned placeholder");
    verifier_syncs = 0;
    fail_verifier_sync = 1;
    result = update_srp_passwd(directory, "alice", account.pw_uid, flags,
                               "new-password");
    fail_verifier_sync = 0;
    report(result < 0 && mock_verifier_owner == 0,
           "failed verifier sync does not grant enrollment");
    fail_verifier_chown = 1;
    result = update_srp_passwd(directory, "alice", account.pw_uid, flags,
                               "new-password");
    fail_verifier_chown = 0;
    report(result < 0 && mock_verifier_owner == 0,
           "failed ownership transfer does not report enrollment success");
    verifier_syncs = 0;
    result = update_srp_passwd(directory, "alice", account.pw_uid, flags,
                               "new-password");
    fp = fopen(verifier, "r");
    int active = fp != NULL && fgets(line, sizeof(line), fp) != NULL &&
                 fgetc(fp) == EOF && strncmp(line, "alice:", 6) == 0 &&
                 line[6] != '*' && srp_valid_fields(line + 6);

    if (fp != NULL) {
        fclose(fp);
    }

    report(result == 0 && active && mock_verifier_owner == account.pw_uid &&
           !enrollment_before_sync,
           "root enrolls only after synchronizing a real verifier");
    fail_verifier_lock = 1;
    result = update_srp_passwd(directory, "alice", account.pw_uid, flags,
                               "new-password");
    fail_verifier_lock = 0;
    report(result < 0 && mock_verifier_owner == account.pw_uid,
           "root -a fails promptly when a verifier lock is held");
    fail_verifier_lock = 1;
    result = disable_srp_verifier(directory, account.pw_name, account.pw_uid);
    fail_verifier_lock = 0;
    report(result < 0 && mock_verifier_owner == account.pw_uid,
           "disable fails promptly when a verifier lock is held");
    result = disable_srp_verifier(directory, account.pw_name, account.pw_uid);
    fp = fopen(verifier, "r");
    placeholder = fp != NULL && fgets(line, sizeof(line), fp) != NULL &&
                  fgetc(fp) == EOF && strncmp(line, "alice:*", 7) == 0 &&
                  srp_valid_fields(line + 6);

    if (fp != NULL) {
        fclose(fp);
    }

    report(result == 0 && placeholder && mock_verifier_owner == 0,
           "disable revokes an enrolled verifier");
    result = update_srp_passwd(directory, "alice", account.pw_uid, flags,
                               "new-password");
    report(result == 0 && mock_verifier_owner == account.pw_uid,
           "root re-enables a disabled verifier with -a");
    /* Owner read/write bits let this root workflow run without privileges;
     * validation of 0000 and 0400 is covered by test_verifier_permissions. */
    const mode_t modes[] = {0644, 0666, 0777, 07777};

    for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
        struct stat st;
        int ready = chmod(verifier, modes[i]) == 0;
        result = update_srp_passwd(directory, "alice", account.pw_uid, flags,
                                   "new-password");
        report(ready && result == 0 && stat(verifier, &st) == 0 &&
               (st.st_mode & 07777) == 0600 &&
               mock_verifier_owner == account.pw_uid,
               "root -a repairs verifier permissions to 0600");
        ready = chmod(verifier, modes[i]) == 0;
        result = disable_srp_verifier(directory, "alice", account.pw_uid);
        report(ready && result == 0 && stat(verifier, &st) == 0 &&
               (st.st_mode & 07777) == 0600 && mock_verifier_owner == 0,
               "root -d repairs verifier permissions to 0600");
    }

    int ready = chmod(verifier, 0666) == 0;
    fail_verifier_chmod = 1;
    result = update_srp_passwd(directory, "alice", account.pw_uid, flags,
                               "new-password");
    report(ready && result < 0 && mock_verifier_owner == 0,
           "root -a fails without enrollment when permission repair fails");
    result = disable_srp_verifier(directory, "alice", account.pw_uid);
    fail_verifier_chmod = 0;
    report(result < 0, "root -d fails when permission repair fails");
    result = update_srp_passwd(directory, "alice", account.pw_uid, flags,
                               "new-password");
    report(result == 0, "permission repair can be retried after failure");
    result = create_srp_directory(directory, account.pw_uid);
    report(result < 0 && mock_verifier_owner == account.pw_uid,
           "initialization refuses an existing verifier directory");
    mock_passwd = NULL;
    mock_verifier_path = NULL;
    mock_directory_owner = 0;
    unlink(verifier);
    rmdir(directory);
}

/*!
 * @brief A bootstrap that aborts at the confirmation leaves the record alone
 */
static void test_aborted_bootstrap_keeps_verifier(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char verifier[MAXPATHLEN + 1] = {0};
    char actual[64] = {0};
    const char *existing = "alice:existing-record\n";
    struct passwd account = {.pw_name = "alice", .pw_uid = getuid()};
    FILE *fp;
    size_t length = 0;
    int result;

    if (make_case(directory) != 0 ||
            snprintf(verifier, sizeof(verifier), "%s/%ju", directory,
                     (uintmax_t)getuid()) <= 0 ||
            write_file(verifier, existing) != 0) {
        report(0, "prepare aborted bootstrap test");
        unlink(verifier);
        rmdir(directory);
        return;
    }

    mock_passwd = &account;
    mock_prompt_answers[0] = "Password2xyz";
    mock_prompt_answers[1] = "MISMATCHED";
    mock_prompts = 0;
    result = create_private_srp_verifier(directory, getuid(),
                                         OPT_CREATE | OPT_FORCE | OPT_NOCRACK, "");
    mock_passwd = NULL;
    fp = fopen(verifier, "r");

    if (fp != NULL) {
        length = fread(actual, 1, sizeof(actual) - 1, fp);
        fclose(fp);
    }

    report(result < 0 && mock_prompts == 2 && length == strlen(existing) &&
           memcmp(actual, existing, length) == 0,
           "mismatched confirmation leaves the existing private verifier unchanged");
    unlink(verifier);
    rmdir(directory);
}

/*!
 * @brief Read the verifier's first line and whether a second one follows
 *
 * @returns 1 when the file holds exactly one line, copied to line
 */
static int read_single_record(const char *path, char *line, size_t size)
{
    FILE *fp = fopen(path, "r");
    int single;

    if (fp == NULL) {
        return 0;
    }

    single = fgets(line, size, fp) != NULL && fgetc(fp) == EOF;
    fclose(fp);
    return single;
}

/*!
 * @brief The bootstrap's other exits: refused before a prompt, cancelled, failed
 *        after the file was created, and the successful record
 */
static void test_bootstrap_exits(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char verifier[MAXPATHLEN + 1] = {0};
    char line[SRP_HEX_SALT_LEN + SRP_HEX_V_LEN + 32] = {0};
    char actual[64] = {0};
    const char *existing = "alice:existing-record\n";
    struct passwd account = {.pw_name = "alice", .pw_uid = getuid()};
    struct stat st;
    size_t length = 0;
    FILE *fp;
    int result;

    if (make_case(directory) != 0 ||
            snprintf(verifier, sizeof(verifier), "%s/%ju", directory,
                     (uintmax_t)getuid()) <= 0 ||
            write_file(verifier, existing) != 0) {
        report(0, "prepare bootstrap exit tests");
        unlink(verifier);
        rmdir(directory);
        return;
    }

    mock_passwd = &account;
    /* an existing verifier without -f is refused before any prompt runs */
    mock_prompt_answers[0] = "Password2xyz";
    mock_prompt_answers[1] = "Password2xyz";
    mock_prompts = 0;
    result = create_private_srp_verifier(directory, getuid(),
                                         OPT_CREATE | OPT_NOCRACK, "");
    fp = fopen(verifier, "r");

    if (fp != NULL) {
        length = fread(actual, 1, sizeof(actual) - 1, fp);
        fclose(fp);
    }

    report(result < 0 && mock_prompts == 0 && length == strlen(existing) &&
           memcmp(actual, existing, length) == 0,
           "existing verifier without -f is refused before the prompt");
    unlink(verifier);
    /* a cancelled first prompt on a fresh directory leaves no file behind */
    mock_prompt_answers[0] = NULL;
    mock_prompts = 0;
    result = create_private_srp_verifier(directory, getuid(),
                                         OPT_CREATE | OPT_NOCRACK, "");
    report(result < 0 && mock_prompts == 1 && stat(verifier, &st) != 0,
           "cancelled bootstrap on a fresh directory leaves no verifier");
    /* a write that fails after the file was created removes the file again */
    mock_prompt_answers[0] = "Password2xyz";
    mock_prompt_answers[1] = "Password2xyz";
    mock_prompts = 0;
    mock_verifier_path = verifier;
    mock_verifier_owner = getuid();
    fail_verifier_lock = 1;
    result = create_private_srp_verifier(directory, getuid(),
                                         OPT_CREATE | OPT_NOCRACK, "");
    fail_verifier_lock = 0;
    mock_verifier_path = NULL;
    report(result < 0 && mock_prompts == 2 && stat(verifier, &st) != 0,
           "bootstrap that fails after creating the file leaves no verifier");
    /* the successful bootstrap writes one owner-only record the UAM accepts */
    unlink(verifier);
    mock_prompts = 0;
    result = create_private_srp_verifier(directory, getuid(),
                                         OPT_CREATE | OPT_NOCRACK, "");
    report(result == 0 && mock_prompts == 2 && stat(verifier, &st) == 0 &&
           (st.st_mode & 07777) == 0600 &&
           read_single_record(verifier, line, sizeof(line)) &&
           strncmp(line, "alice:", 6) == 0 && line[6] != '*' &&
           srp_valid_fields(line + 6),
           "bootstrap writes a single owner-only verifier record");
    /* -w that is too long is refused before the file exists */
    unlink(verifier);
    memset(line, 'x', SRP_PASSWDLEN + 1);
    line[SRP_PASSWDLEN + 1] = '\0';
    mock_prompts = 0;
    result = create_private_srp_verifier(directory, getuid(),
                                         OPT_CREATE | OPT_NOCRACK, line);
    report(result < 0 && mock_prompts == 0 && stat(verifier, &st) != 0,
           "overlong -w bootstrap leaves no verifier");
    mock_passwd = NULL;
    unlink(verifier);
    rmdir(directory);
}

/*!
 * @brief Root's -a with an overlong -w on a missing verifier creates nothing
 */
static void test_overlong_password_creates_nothing(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char verifier[MAXPATHLEN + 1] = {0};
    char password[SRP_PASSWDLEN + 2];
    struct stat st;
    int result;

    if (make_case(directory) != 0 || stat(directory, &verifier_directory) != 0 ||
            snprintf(verifier, sizeof(verifier), "%s/%ju", directory,
                     (uintmax_t)getuid()) <= 0) {
        report(0, "prepare overlong password test");
        rmdir(directory);
        return;
    }

    memset(password, 'x', SRP_PASSWDLEN + 1);
    password[SRP_PASSWDLEN + 1] = '\0';
    mock_directory_owner = 1;
    mock_verifier_path = verifier;
    mock_verifier_owner = 0;
    result = update_srp_passwd(directory, "alice", getuid(),
                               OPT_ISROOT | OPT_ADDUSER | OPT_NOCRACK, password);
    mock_directory_owner = 0;
    mock_verifier_path = NULL;
    report(result < 0 && stat(verifier, &st) != 0,
           "root -a with an overlong -w creates no verifier");
    unlink(verifier);
    rmdir(directory);
}

static void test_unenrolled_user_message(void)
{
    char directory[MAXPATHLEN + 1] = {0};
    char verifier[MAXPATHLEN + 1];
    char uid_name[3 * sizeof(uid_t) + 1];
    char message[256] = {0};
    int dirfd = -1, fd, pipefd[2] = {-1, -1}, saved_stderr = -1;
    ssize_t length;
    int passed = 0;

    if (getuid() == 0) {
        report(1, "disabled verifier reports administrator message (skipped while running as root)");
        return;
    }

    if (make_case(directory) == 0 &&
            srp_uid_filename(getuid(), uid_name, sizeof(uid_name)) == 0 &&
            snprintf(verifier, sizeof(verifier), "%s/%s", directory, uid_name) > 0 &&
            write_file(verifier, "alice:*\n") == 0 && chmod(verifier, 0000) == 0 &&
            (dirfd = open(directory, O_RDONLY | O_DIRECTORY | O_CLOEXEC)) >= 0 &&
            pipe(pipefd) == 0 && (saved_stderr = dup(STDERR_FILENO)) >= 0) {
        fflush(stderr);

        if (dup2(pipefd[1], STDERR_FILENO) >= 0) {
            close(pipefd[1]);
            pipefd[1] = -1;
            fd = open_srp_verifier(dirfd, directory, getuid(), 0);
            fflush(stderr);
            dup2(saved_stderr, STDERR_FILENO);
            length = read(pipefd[0], message, sizeof(message) - 1);
            passed = fd < 0 && length > 0 &&
                     strstr(message,
                            "Your password is disabled. Please see your administrator.") != NULL;

            if (fd >= 0) {
                close(fd);
            }
        }
    }

    if (saved_stderr >= 0) {
        close(saved_stderr);
    }

    if (dirfd >= 0) {
        close(dirfd);
    }

    if (pipefd[0] >= 0) {
        close(pipefd[0]);
    }

    if (pipefd[1] >= 0) {
        close(pipefd[1]);
    }

    chmod(verifier, 0600);
    unlink(verifier);
    rmdir(directory);
    report(passed, "disabled verifier reports administrator message");
}

static void test_shared_srp_validation(void)
{
    char fields[SRP_FIELDS_LEN + 2];
    char username[SRP_USERNAME_MAX_LEN + 2];
    report(srp_valid_username("alice") && !srp_valid_username("") &&
           !srp_valid_username("alice:bob") &&
           !srp_valid_username("alice\nbob") &&
           !srp_valid_username("alice\rbob"),
           "shared username validation rejects empty names and delimiters");
    memset(username, 'a', sizeof(username));
    username[SRP_USERNAME_MAX_LEN] = '\0';
    report(srp_valid_username(username), "maximum-length username is valid");
    username[SRP_USERNAME_MAX_LEN] = 'a';
    username[SRP_USERNAME_MAX_LEN + 1] = '\0';
    report(!srp_valid_username(username), "overlong username is invalid");
    /* Exercise the same validator used by the writer, migration, and UAM. */
    memset(fields, 'a', SRP_HEX_SALT_LEN);
    fields[SRP_HEX_SALT_LEN] = ':';
    memset(fields + SRP_HEX_SALT_LEN + 1, 'F', SRP_HEX_V_LEN);
    fields[SRP_FIELDS_LEN - 1] = '\n';
    fields[SRP_FIELDS_LEN] = '\0';
    report(srp_valid_fields(fields), "mixed-case hexadecimal fields are valid");
    const struct {
        size_t offset;
        char replacement;
        const char *description;
    } invalid[] = {
        {0, 'g', "non-hexadecimal salt is invalid"},
        {0, '\0', "embedded NUL is invalid"},
        {0, '*', "partially disabled salt is invalid"},
        {SRP_HEX_SALT_LEN, ';', "incorrect field separator is invalid"},
        {SRP_HEX_SALT_LEN + 1, '*', "partially disabled verifier is invalid"},
        {SRP_FIELDS_LEN - 2, 'g', "non-hexadecimal verifier is invalid"},
        {SRP_FIELDS_LEN - 1, '\0', "missing newline is invalid"},
        {SRP_FIELDS_LEN - 1, '\r', "incorrect line ending is invalid"},
    };

    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        char saved = fields[invalid[i].offset];
        fields[invalid[i].offset] = invalid[i].replacement;
        report(!srp_valid_fields(fields), invalid[i].description);
        fields[invalid[i].offset] = saved;
    }

    report(!srp_valid_fields(fields + 1), "short salt is invalid");
    fields[SRP_FIELDS_LEN] = 'x';
    fields[SRP_FIELDS_LEN + 1] = '\0';
    report(!srp_valid_fields(fields), "trailing data is invalid");
    fields[SRP_FIELDS_LEN] = '\0';
    memset(fields, '*', SRP_HEX_SALT_LEN);
    report(!srp_valid_fields(fields),
           "disabled salt with active verifier is invalid");
    memset(fields + SRP_HEX_SALT_LEN + 1, '*', SRP_HEX_V_LEN);
    report(srp_valid_fields(fields), "fully disabled fields are well-formed");
    memset(fields, 'a', SRP_HEX_SALT_LEN);
    report(!srp_valid_fields(fields),
           "active salt with disabled verifier is invalid");
}

static void test_minimum_uid_parser(void)
{
    uid_t uid = 0;
    report(parse_minimum_uid("100", &uid) == 0 && uid == 100,
           "minimum uid accepts a decimal uid");
    report(parse_minimum_uid("", &uid) < 0 &&
           parse_minimum_uid("-1", &uid) < 0 &&
           parse_minimum_uid("100users", &uid) < 0 &&
           parse_minimum_uid(" 100", &uid) < 0,
           "minimum uid rejects malformed values");
}

static void test_disable_option_argument(void)
{
    char program[] = "afppasswd";
    char disable[] = "-d";
    char username[] = "alice";
    char *argv[] = {program, disable, username, NULL};
    int option;
    optind = 1;
    option = getopt(3, argv, AFPPASSWD_OPTSTRING);
    report(option == 'd' && optarg != NULL && strcmp(optarg, username) == 0,
           "-d consumes its required username argument");
}

int main(void)
{
    if (initialize_libgcrypt() != 0) {
        return 1;
    }

    test_shared_srp_validation();
    test_rejected_verifier_does_not_change_metadata();
    test_verifier_permissions();
    test_single_record_updates();
    test_enrollment_ownership();
    test_aborted_bootstrap_keeps_verifier();
    test_bootstrap_exits();
    test_overlong_password_creates_nothing();
    test_unenrolled_user_message();
    test_minimum_uid_parser();
    test_disable_option_argument();
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
