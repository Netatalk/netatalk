/*
 * Regression tests for CUPS Chooser name collisions and allocation failure
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail_realloc;
static int realloc_calls;

static void *name_realloc(void *ptr, size_t size)
{
    realloc_calls++;
    return fail_realloc ? NULL : realloc(ptr, size);
}

/* Exercise the static helper and real collision checks without a CUPS server. */
#define realloc name_realloc
#include "../../etc/papd/print_cups.c"
#undef realloc

static int failures;

static void check(int passed, const char *name)
{
    if (!passed) {
        fprintf(stderr, "FAIL: %s\n", name);
        failures++;
    }
}

static void test_length(size_t length)
{
    char original[201];
    char expected[32];
    size_t prefix = length > 28 ? 28 : length;
    struct printer existing = {0};
    struct printer candidate = {0};
    memset(original, 'a', length);
    original[length] = '\0';
    memcpy(expected, original, prefix);
    memcpy(expected + prefix, "#01", sizeof("#01"));
    existing.p_name = original;
    existing.p_flags = P_CUPS_AUTOADDED;
    candidate.p_name = strdup(original);
    candidate.p_flags = P_CUPS_AUTOADDED;
    check(cups_mangle_printer_name(&candidate, &existing) == 0,
          "duplicate name can be mangled");
    check(strcmp(candidate.p_name, expected) == 0, "bounded prefix and #01 suffix");
    check(strlen(candidate.p_name) <= 31, "Chooser name length limit");
    free(candidate.p_name);
}

static void test_collisions(void)
{
    struct printer existing[100] = {0};
    char names[100][32];
    struct printer candidate = {0};
    strcpy(names[0], "Printer");

    for (size_t i = 0; i < 100; i++) {
        if (i) {
            snprintf(names[i], sizeof(names[i]), "Printer#%02zu", i);
        }

        existing[i].p_name = names[i];
        existing[i].p_flags = P_CUPS_AUTOADDED;
        existing[i].p_next = i < 98 ? &existing[i + 1] : NULL;
    }

    candidate.p_name = strdup("printer");
    candidate.p_flags = P_CUPS_AUTOADDED;
    check(cups_mangle_printer_name(&candidate, existing) == 0,
          "case-insensitive collisions leave #99 available");
    check(strcmp(candidate.p_name, "printer#99") == 0,
          "#99 is a valid last suffix");
    free(candidate.p_name);
    existing[98].p_next = &existing[99];
    candidate.p_name = strdup("Printer");
    check(cups_mangle_printer_name(&candidate, existing) == 2,
          "all 99 suffixes occupied returns failure");
    free(candidate.p_name);
}

static void test_allocation_failure(void)
{
    struct printer existing = {.p_name = "Printer", .p_flags = P_CUPS_AUTOADDED};
    struct printer candidate = {.p_name = strdup("Printer"), .p_flags = P_CUPS_AUTOADDED};
    const char *saved = candidate.p_name;
    fail_realloc = 1;
    check(cups_mangle_printer_name(&candidate, &existing) == 2,
          "allocation failure is reported");
    check(candidate.p_name == saved && strcmp(saved, "Printer") == 0,
          "allocation failure preserves original name");
    realloc_calls = 0;
    check(cups_mangle_printer_name(&candidate, NULL) == 0 && realloc_calls == 0,
          "unique name needs no allocation");
    check(candidate.p_name == saved, "unique name is unchanged");
    free(candidate.p_name);
    fail_realloc = 0;
}

int main(void)
{
    const size_t lengths[] = {0, 5, 27, 28, 29, 31, 200};

    for (size_t i = 0; i < sizeof(lengths) / sizeof(lengths[0]); i++) {
        test_length(lengths[i]);
    }

    test_collisions();
    test_allocation_failure();
    return failures ? 1 : 0;
}
