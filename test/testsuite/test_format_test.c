/*
 * Test the string formatting helper functions in the testhelper module
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

#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "afpclient.h"
#include "afphelper.h"
#include "testhelper.h"

/* Reporting globals needed by testhelper.c; no AFP connection is used. */
CONN *Conn;
CONN *Conn2;
uint16_t VolID;
int Color;
int EmptyVol;
int ExitCode;
int Quiet = 1;
int PassCount;
int FailCount;
int SkipCount;
int NotTestedCount;
char FailedTests[1024][256];
char NotTestedTests[1024][256];
char SkippedTests[1024][256];

void clear_volume(uint16_t vol, CONN *conn)
{
    (void)vol;
    (void)conn;
    abort();
}

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void)
{
    struct {
        char before;
        char buf[8];
        char after;
    } guarded = { 'L', "old", 'R' };
    char name[32];
    char probe[MB_LEN_MAX];
    char single = 'x';
    CHECK(test_format(guarded.buf, sizeof(guarded.buf), "%s/%s", "a", "b") == 0);
    CHECK(strcmp(guarded.buf, "a/b") == 0);
    /* Seven characters plus NUL fit; eight characters must be rejected. */
    CHECK(test_format(guarded.buf, sizeof(guarded.buf), "%s", "1234567") == 0);
    CHECK(strcmp(guarded.buf, "1234567") == 0);
    errno = 0;
    CHECK(test_format(guarded.buf, sizeof(guarded.buf), "%s", "12345678") == -1);
    CHECK(errno == ENAMETOOLONG);
    CHECK(guarded.buf[0] == '\0');
    CHECK(guarded.before == 'L' && guarded.after == 'R');
    CHECK(test_format(&single, sizeof(single), "%s", "") == 0);
    CHECK(single == '\0');
    CHECK(test_format(&single, sizeof(single), "%s", "x") == -1);
    CHECK(single == '\0');
    errno = 0;
    CHECK(test_format(NULL, 0, "%s", "") == -1);
    CHECK(errno == ENAMETOOLONG);
    CHECK(test_format(name, sizeof(name), "t312-#%X.mp3", UINT32_MAX) == 0);
    CHECK(strcmp(name, "t312-#FFFFFFFF.mp3") == 0);
    CHECK(test_format(name, sizeof(name), "Trash Can #%d", INT_MAX) == 0);
    CHECK(strncmp(name, "Trash Can #", 11) == 0);
    /* Exercise the error path only if libc rejects this wide character. */
    CHECK(setlocale(LC_CTYPE, "C") != NULL);

    if (snprintf(probe, sizeof(probe), "%lc", (wint_t)0x100) < 0) {
        errno = 0;
        CHECK(test_format(guarded.buf, sizeof(guarded.buf), "%lc",
                          (wint_t)0x100) == -1);
        CHECK(errno == EILSEQ);
        CHECK(guarded.buf[0] == '\0');
        CHECK(guarded.before == 'L' && guarded.after == 'R');
    } else {
        fprintf(stderr,
                "Skipping encoding-error case: libc accepts 0x100 in the C locale\n");
    }

    return 0;
}
