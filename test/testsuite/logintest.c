/*
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 * Based on work by Rafal Lewczuk, Didier Gautheron, and Frank Lahm
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

#include <dlfcn.h>
#include <getopt.h>

#include "afpclient.h"
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
#include "logintest_uam.h"
#endif

uint16_t VolID;

CONN *Conn;
CONN *Conn2;

int ExitCode = 0;
int PassCount = 0;
int FailCount = 0;
int SkipCount = 0;
int NotTestedCount = 0;
char FailedTests[1024][256] = {{0}};
char NotTestedTests[1024][256] = {{0}};
char SkippedTests[1024][256] = {{0}};

DSI *Dsi;

char Data[300000] = "";
/* ------------------------------- */
char    *Server = "localhost";
char    *Server2;
int     Port = DSI_AFPOVERTCP_PORT;
char    *Password = "";
char    *Vol = "";
char    *User;
char    *User2;
char    *Path;
int     Version = 34;
int     List = 0;
int     Mac = 0;
int     EmptyVol = 0;
char    *Test;
char *vers = "AFP3.4";

#define FN(a) a
#define EXT_FN(a) extern void FN(a)(void)

EXT_FN(test_dsi_getstatus_no_session);
EXT_FN(test_dsi_open_close_session);
EXT_FN(test_dsi_open_session_ignore_param);
EXT_FN(test_dsi_command_roundtrip);
EXT_FN(test_connection_limit);
EXT_FN(test_guest_login);
EXT_FN(test_cleartext_login);
EXT_FN(test_loginext_guest_direct);
EXT_FN(test_logincont_roundtrip);

struct test_fn {
    const char *name;
    void (*fn)(void);
};

#define FN_N(a) { # a, FN(a) },

static struct test_fn Test_list[] = {
    FN_N(test_dsi_getstatus_no_session)
    FN_N(test_dsi_open_close_session)
    FN_N(test_dsi_open_session_ignore_param)
    FN_N(test_dsi_command_roundtrip)
    FN_N(test_connection_limit)
    FN_N(test_guest_login)
    FN_N(test_cleartext_login)
    FN_N(test_loginext_guest_direct)
    FN_N(test_logincont_roundtrip)
    { NULL, NULL },
};

#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
static void list_uam_tests(void)
{
    for (const struct uamtest_entry *entry = uamtest_entries;
            entry->selector; entry++) {
        fprintf(stdout, "%-10s %s (libafpclient UAM)\n", entry->selector,
                entry->uam);
    }
}

static const struct uamtest_entry *find_uam_test(const char *name)
{
    for (const struct uamtest_entry *entry = uamtest_entries;
            entry->selector; entry++) {
        if (!strcmp(entry->selector, name) || !strcmp(entry->uam, name)) {
            return entry;
        }
    }

    return NULL;
}
#endif



/* =============================== */
static void list_tests(void)
{
    int i = 0;
    fprintf(stdout, "Available tests. Run individually with the -f option.\n");

    while (Test_list[i].name != NULL) {
        fprintf(stdout, "%s\n", Test_list[i].name);
        i++;
    }

#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
    list_uam_tests();
#endif
}

/* ----------- */
static void run_one(char *name)
{
    int i = 0;
    void *handle = NULL;
    void (*fn)(void) = NULL;
    char *error;

    while (Test_list[i].name != NULL) {
        if (!strcmp(Test_list[i].name, name)) {
            break;
        }

        i++;
    }

    if (Test_list[i].name == NULL) {
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
        const struct uamtest_entry *uam_entry = find_uam_test(name);

        if (uam_entry) {
            uamtest_run_selected(uam_entry->selector);
            return;
        }

#endif
        handle = dlopen(NULL, RTLD_NOW);

        if (handle) {
            dlerror();
            fn = dlsym(handle, name);

            if ((error = dlerror()) != NULL) {
                fprintf(stdout, "%s\n", error);
                dlclose(handle);
                handle = NULL;
            }
        } else {
            fprintf(stdout, "%s\n", dlerror());
        }

        if (!handle || !fn) {
            test_nottested();
            return;
        }
    } else {
        fn = Test_list[i].fn;
    }

    (*fn)();

    if (handle) {
        dlclose(handle);
    }
}

/* ----------- */
static void run_all(void)
{
    int i = 0;

    while (Test_list[i].name != NULL) {
        Test_list[i].fn();
        i++;
    }

#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
    uamtest_run_selected(NULL);
#endif
}

/* =============================== */
void usage(char *av0)
{
    fprintf(stdout,
            "usage:\t%s [-1234567CmVv] [-h host] [-p port] [-u user] [-w password] [-f test]\n",
            av0);
    fprintf(stdout, "\t-m\tserver is a Mac\n");
    fprintf(stdout, "\t-h\tserver host name (default localhost)\n");
    fprintf(stdout, "\t-p\tserver port (default 548)\n");
    fprintf(stdout, "\t-u\tuser name (default uid)\n");
    fprintf(stdout, "\t-w\tpassword\n");
    fprintf(stdout, "\t-1\tAFP 2.1 version\n");
    fprintf(stdout, "\t-2\tAFP 2.2 version\n");
    fprintf(stdout, "\t-3\tAFP 3.0 version\n");
    fprintf(stdout, "\t-4\tAFP 3.1 version\n");
    fprintf(stdout, "\t-5\tAFP 3.2 version\n");
    fprintf(stdout, "\t-6\tAFP 3.3 version\n");
    fprintf(stdout, "\t-7\tAFP 3.4 version (default)\n");
    fprintf(stdout, "\t-v\tverbose\n");
    fprintf(stdout, "\t-V\tvery verbose\n");
    fprintf(stdout, "\t-C\tturn off terminal color output\n");
    fprintf(stdout, "\t-f\ttest to run");
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
    fprintf(stdout, " (or UAM selector/name)");
#endif
    fprintf(stdout, "\n");
    fprintf(stdout, "\t-l\tlist tests\n");
    exit(1);
}

/* ------------------------------- */
int main(int ac, char **av)
{
    int cc;

    if (ac == 1) {
        usage(av[0]);
    }

    while ((cc = getopt(ac, av, "1234567CmVvf:h:lp:u:w:")) != EOF) {
        switch (cc) {
        case '1':
            vers = "AFPVersion 2.1";
            Version = 21;
            break;

        case '2':
            vers = "AFP2.2";
            Version = 22;
            break;

        case '3':
            vers = "AFPX03";
            Version = 30;
            break;

        case '4':
            vers = "AFP3.1";
            Version = 31;
            break;

        case '5':
            vers = "AFP3.2";
            Version = 32;
            break;

        case '6':
            vers = "AFP3.3";
            Version = 33;
            break;

        case '7':
            vers = "AFP3.4";
            Version = 34;
            break;

        case 'C':
            Color = 0;
            break;

        case 'f':
            Test = strdup(optarg);
            break;

        case 'h':
            Server = strdup(optarg);
            break;

        case 'l':
            List = 1;
            break;

        case 'm':
            Mac = 1;
            break;

        case 'p' :
            Port = atoi(optarg);

            if (Port <= 0) {
                fprintf(stdout, "Bad port.\n");
                exit(1);
            }

            break;

        case 'u':
            User = strdup(optarg);
            break;

        case 'v':
            Quiet = 0;
            break;

        case 'V':
            Quiet = 0;
            Verbose = 1;
            break;

        case 'w':
            Password = strdup(optarg);
            break;

        default :
            usage(av[0]);
        }
    }

    if (List) {
        list_tests();
        exit(2);
    }

    if (!Quiet) {
        fprintf(stdout, "Connecting to host %s:%d\n", Server, Port);
    }

    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        return 1;
    }

    if (Test != NULL) {
        run_one(Test);
    } else {
        run_all();
    }

    fprintf(stdout, "=====================\n");
    fprintf(stdout, " TEST RESULT SUMMARY\n");
    fprintf(stdout, "---------------------\n");
    fprintf(stdout, "  Passed:     %d\n", PassCount);
    fprintf(stdout, "  Skipped:    %d\n", SkipCount);
    fprintf(stdout, "  Failed:     %d\n", FailCount);
    fprintf(stdout, "  Not tested: %d\n", NotTestedCount);

    if (SkipCount) {
        fprintf(stdout, "\n  Skipped tests (precondition not met):\n");

        for (int i = 0; i < SkipCount; i++) {
            fprintf(stdout, "    %s\n", SkippedTests[i]);
        }
    }

    if (FailCount) {
        fprintf(stdout, "\n  Failed tests:\n");

        for (int i = 0; i < FailCount; i++) {
            fprintf(stdout, "    %s\n", FailedTests[i]);
        }
    }

    if (NotTestedCount) {
        fprintf(stdout, "\n  Not tested tests (setup step failed):\n");

        for (int i = 0; i < NotTestedCount; i++) {
            fprintf(stdout, "    %s\n", NotTestedTests[i]);
        }
    }

    return ExitCode;
}
