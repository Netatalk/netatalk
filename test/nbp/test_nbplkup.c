/*
 * Regression tests for nbplkup output handling.
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/nbp.h>
#include <atalk/unicode.h>

#include "nbplkup_output.h"

static int failures;

#define CHECK(description, condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL: %s\n", (description)); \
        failures++; \
    } \
} while (0)

static void test_conversion_failure_uses_bounded_fallback(void)
{
    char source[NBPSTRLEN + 2];
    char *dest = NULL;
    size_t dest_len;
    memset(source, 0xff, NBPSTRLEN);
    source[NBPSTRLEN] = 'X';
    source[NBPSTRLEN + 1] = '\0';
    set_charset_name(CH_UNIX, "UTF8");
    set_charset_name(CH_MAC, "MAC_ROMAN");
    dest_len = nbplkup_convert_field(CH_UNIX, source, NBPSTRLEN, &dest);
    CHECK("failed conversion falls back to the source length",
          dest_len == NBPSTRLEN);
    CHECK("fallback allocation succeeds", dest != NULL);

    if (dest != NULL) {
        CHECK("fallback preserves the counted field bytes",
              memcmp(dest, source, NBPSTRLEN) == 0);
        CHECK("fallback is terminated at the field boundary",
              dest[NBPSTRLEN] == '\0');
    }

    free(dest);
}

int main(void)
{
    test_conversion_failure_uses_bounded_fallback();

    if (failures != 0) {
        fprintf(stderr, "%d test(s) FAILED.\n", failures);
        return 1;
    }

    puts("All nbplkup output tests passed.");
    return 0;
}
