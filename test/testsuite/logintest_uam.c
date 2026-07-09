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

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <afp.h>
#include <uams_def.h>

#include "testhelper.h"
#include "logintest_uam.h"
#include "uamtest_libafpclient.h"

enum test_result {
    RESULT_PASS,
    RESULT_FAIL,
    RESULT_NOTTESTED,
    RESULT_SKIP,
};

const struct uamtest_entry uamtest_entries[] = {
    { "guest",     "No User Authent",        false, false },
    { "cleartext", "Cleartxt Passwrd",       true,  true  },
    { "randnum",   "Randnum Exchange",       true,  true  },
    { "randnum2",  "2-Way Randnum Exchange", true,  true  },
    { "dhx",       "DHCAST128",              true,  true  },
    { "dhx2",      "DHX2",                   true,  true  },
    { "srp",       "SRP",                    true,  true  },
    { NULL, NULL, false, false }
};

extern char *Password;
extern int Port;

static unsigned int advertised_uams;
static bool advertised_known;

static const char *result_text(enum test_result result)
{
    switch (result) {
    case RESULT_PASS:
        return Color ? ANSI_BGREEN "PASSED" ANSI_NORMAL : "PASSED";

    case RESULT_FAIL:
        return Color ? ANSI_BRED "FAILED" ANSI_NORMAL : "FAILED";

    case RESULT_NOTTESTED:
        return Color ? ANSI_BYELLOW "NOT TESTED" ANSI_NORMAL : "NOT TESTED";

    case RESULT_SKIP:
        return Color ? ANSI_BBLUE "SKIPPED" ANSI_NORMAL : "SKIPPED";
    }

    return "UNKNOWN";
}

static void remember_test_name(char tests[1024][256], int index,
                               const struct uamtest_entry *entry)
{
    snprintf(tests[index], 256, "UAM %s (%s)", entry->selector, entry->uam);
}

static void record_result(const struct uamtest_entry *entry,
                          enum test_result result, const char *reason)
{
    switch (result) {
    case RESULT_PASS:
        PassCount++;
        break;

    case RESULT_FAIL:
        remember_test_name(FailedTests, FailCount, entry);
        FailCount++;
        ExitCode = 1;
        break;

    case RESULT_NOTTESTED:
        remember_test_name(NotTestedTests, NotTestedCount, entry);
        NotTestedCount++;

        if (!ExitCode) {
            ExitCode = 2;
        }

        break;

    case RESULT_SKIP:
        remember_test_name(SkippedTests, SkipCount, entry);
        SkipCount++;
        break;
    }

    fprintf(stdout, "UAM %s (%s) - %s", entry->selector, entry->uam,
            result_text(result));

    if (reason && *reason) {
        fprintf(stdout, " (%s)", reason);
    }

    fprintf(stdout, "\n");
}

static bool entry_matches(const struct uamtest_entry *entry, const char *filter)
{
    return !filter || strcmp(filter, entry->selector) == 0
           || strcmp(filter, entry->uam) == 0;
}

void uamtest_list_tests(void)
{
    for (const struct uamtest_entry *entry = uamtest_entries;
            entry->selector; entry++) {
        fprintf(stdout, "%-10s %s (libafpclient UAM)\n", entry->selector,
                entry->uam);
    }
}

static bool has_credentials(void)
{
    return User && *User && Password && *Password;
}

static void make_wrong_password(char *buf, size_t buflen)
{
    char replacement;

    if (!Password || !*Password) {
        snprintf(buf, buflen, "netatalk-wrong-password");
        return;
    }

    snprintf(buf, buflen, "%s", Password);

    if (buflen < 2) {
        return;
    }

    /* Randnum, 2-Way Randnum, and classic cleartext only use the first
     * eight password bytes. Mutate byte 0 so the negative test really is
     * different for those UAMs too. Pick values that differ by more than
     * DES parity/ignored-bit behavior. */
    replacement = (buf[0] == 'X') ? 'q' : 'X';
    buf[0] = replacement;

    if (strcmp(buf, Password) == 0) {
        snprintf(buf, buflen, "netatalk-wrong-password");
    }
}

static enum test_result run_one(const struct uamtest_entry *entry,
                                char *reason, size_t reason_len)
{
    struct uamtest_session *session = NULL;
    struct uamtest_session *wrong_session = NULL;
    const char *canonical;
    unsigned int mask;
    unsigned int using_uam;
    char wrong_password[AFP_MAX_PASSWORD_LEN];
    int using_version;
    canonical = uamtest_libafpclient_resolve_uam(entry->uam);
    mask = uamtest_libafpclient_uam_mask(entry->uam);

    if (mask == 0) {
        snprintf(reason, reason_len, "libafpclient does not know this UAM");
        return RESULT_NOTTESTED;
    }

    if (advertised_known && (advertised_uams & mask) == 0) {
        snprintf(reason, reason_len, "server did not advertise support");
        return RESULT_SKIP;
    }

    if (entry->needs_credentials && !has_credentials()) {
        snprintf(reason, reason_len, "username/password required");
        return RESULT_SKIP;
    }

    if (!Quiet) {
        fprintf(stdout, "Testing %s\n", canonical ? canonical : entry->uam);
    }

    if (uamtest_libafpclient_connect(&session, Server, Port, Version,
                                     entry->uam,
                                     entry->needs_credentials ? User : "",
                                     entry->needs_credentials ? Password : "")
            != 0) {
        if (advertised_known && (advertised_uams & mask) != 0) {
            snprintf(reason, reason_len, "advertised UAM failed to authenticate");
            return RESULT_FAIL;
        }

        snprintf(reason, reason_len, "UAM not supported by protocol");
        return RESULT_SKIP;
    }

    using_uam = uamtest_libafpclient_using_uam(session);

    if ((using_uam & mask) == 0) {
        uamtest_libafpclient_close(&session);
        snprintf(reason, reason_len, "libafpclient selected a different UAM");
        return RESULT_FAIL;
    }

    using_version = uamtest_libafpclient_using_version(session);

    if (using_version != 0 && using_version > Version) {
        uamtest_libafpclient_close(&session);
        snprintf(reason, reason_len, "negotiated AFP%d above requested AFP%d",
                 using_version, Version);
        return RESULT_FAIL;
    }

    if (uamtest_libafpclient_smoke(session) != 0) {
        uamtest_libafpclient_close(&session);
        snprintf(reason, reason_len, "authenticated AFP smoke failed");
        return RESULT_FAIL;
    }

    if (entry->check_wrong_password) {
        make_wrong_password(wrong_password, sizeof(wrong_password));

        if (uamtest_libafpclient_connect(&wrong_session, Server, Port,
                                         Version, entry->uam, User,
                                         wrong_password) == 0) {
            uamtest_libafpclient_close(&wrong_session);
            uamtest_libafpclient_close(&session);
            snprintf(reason, reason_len, "wrong password authenticated");
            return RESULT_FAIL;
        }
    }

    uamtest_libafpclient_close(&session);
    snprintf(reason, reason_len, "login, smoke, and failure checks passed");
    return RESULT_PASS;
}

int uamtest_has_test(const char *filter)
{
    for (const struct uamtest_entry *entry = uamtest_entries;
            entry->selector; entry++) {
        if (entry_matches(entry, filter)) {
            return 1;
        }
    }

    return 0;
}

int uamtest_run_selected(const char *filter)
{
    bool matched = false;
    advertised_uams = uamtest_libafpclient_discover_uams(Server, Port,
                      Version, User ? User : "", Password ? Password : "",
                      &advertised_known);

    if (Verbose && advertised_known) {
        char *uam_list = uam_bitmap_to_string(advertised_uams);
        fprintf(stdout, "Advertised UAMs: %s\n", uam_list ? uam_list : "(none)");
    } else if (Verbose) {
        fprintf(stdout, "Advertised UAMs: unavailable before per-UAM login\n");
    }

    for (const struct uamtest_entry *entry = uamtest_entries;
            entry->selector; entry++) {
        char reason[256] = "";
        enum test_result result;

        if (!entry_matches(entry, filter)) {
            continue;
        }

        matched = true;
        result = run_one(entry, reason, sizeof(reason));
        record_result(entry, result, reason);
    }

    if (!matched) {
        fprintf(stderr, "Unknown UAM test: %s\n", filter);
        return 1;
    }

    return ExitCode;
}
