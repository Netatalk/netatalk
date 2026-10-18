/*
 * Regression test for shell_quote()
 *
 * Copyright (c) 2026 Andy Lemin (andylemin)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/util.h>

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

static int quotes_as(const char *s, const char *want)
{
    char *word = shell_quote(s);
    int same = word != NULL && strcmp(word, want) == 0;

    if (!same) {
        printf("# shell_quote(\"%s\") gave %s, want %s\n", s,
               word ? word : "NULL", want);
    }

    free(word);
    return same;
}

int main(void)
{
    printf("TAP version 13\n");
    report(quotes_as("/srv/afp/cnid", "'/srv/afp/cnid'"),
           "a path is one word between single quotes");
    report(quotes_as("/srv/Bob's Files", "'/srv/Bob'\\''s Files'"),
           "a single quote in a path is written as '\\''");
    report(quotes_as("a\\b $HOME", "'a\\b $HOME'"),
           "a backslash and a dollar sign stay as written");
    report(quotes_as("", "''"), "an empty string is an empty word");
    printf("1..%d\n", tests_run);
    return tests_failed == 0 ? 0 : 1;
}
