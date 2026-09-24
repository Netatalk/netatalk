/*
 * Regression tests for NBP input validation.
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <atalk/nbp.h>

#include "nbp_conf.h"

static int failures;

#define CHECK(description, condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL: %s\n", (description)); \
        failures++; \
    } \
} while (0)

static size_t make_tuple(uint8_t *tuple, uint8_t objlen, uint8_t typelen,
                         uint8_t zonelen)
{
    uint8_t *data = tuple;
    memset(data, 0, SZ_NBPTUPLE);
    data += SZ_NBPTUPLE;
    *data++ = objlen;
    memset(data, 'o', objlen);
    data += objlen;
    *data++ = typelen;
    memset(data, 't', typelen);
    data += typelen;
    *data++ = zonelen;
    memset(data, 'z', zonelen);
    data += zonelen;
    return data - tuple;
}

static void test_valid_tuple(void)
{
    uint8_t tuple[SZ_NBPTUPLE + 3 * (NBPSTRLEN + 1) + 2];
    struct nbpnve nve = { 0 };
    size_t len;
    len = make_tuple(tuple, NBPSTRLEN, NBPSTRLEN, NBPSTRLEN);
    tuple[len++] = 0xaa;
    tuple[len++] = 0xbb;
    CHECK("maximum-length tuple is accepted",
          nbp_parse((char *)tuple, &nve, len) == 2);
    CHECK("object length is preserved", nve.nn_objlen == NBPSTRLEN);
    CHECK("type length is preserved", nve.nn_typelen == NBPSTRLEN);
    CHECK("zone length is preserved", nve.nn_zonelen == NBPSTRLEN);
}

static void test_overlong_strings(void)
{
    uint8_t tuple[SZ_NBPTUPLE + 3 * (NBPSTRLEN + 2)];
    struct nbpnve nve = { 0 };
    size_t len;
    len = make_tuple(tuple, NBPSTRLEN + 1, 1, 1);
    CHECK("overlong object is rejected",
          nbp_parse((char *)tuple, &nve, len) == -1);
    len = make_tuple(tuple, 1, NBPSTRLEN + 1, 1);
    CHECK("overlong type is rejected",
          nbp_parse((char *)tuple, &nve, len) == -1);
    len = make_tuple(tuple, 1, 1, NBPSTRLEN + 1);
    CHECK("overlong zone is rejected",
          nbp_parse((char *)tuple, &nve, len) == -1);
}

static void test_truncated_tuple(void)
{
    uint8_t tuple[SZ_NBPTUPLE + 3 * (NBPSTRLEN + 1)];
    struct nbpnve nve = { 0 };
    size_t len = make_tuple(tuple, NBPSTRLEN, NBPSTRLEN, NBPSTRLEN);

    for (size_t truncated_len = 0; truncated_len < len; truncated_len++) {
        CHECK("truncated tuple is rejected",
              nbp_parse((char *)tuple, &nve, truncated_len) == -1);
    }
}

static void test_match_rejects_invalid_lengths(void)
{
    struct nbpnve nve1 = { 0 };
    struct nbpnve nve2 = { 0 };
    nve1.nn_typelen = NBPSTRLEN + 1;
    nve2.nn_typelen = NBPSTRLEN + 1;
    CHECK("matching rejects an invalid type length",
          nbp_match(&nve1, &nve2, NBPMATCH_NOZONE | NBPMATCH_NOGLOB) == 0);
}

static void test_lookup_rejects_invalid_result_array(void)
{
    struct nbpnve nve = { 0 };
    errno = 0;
    CHECK("lookup rejects a zero result capacity",
          nbp_do_lookup_op(NULL, NULL, NULL, &nve, 0, NULL, NULL,
                           NBPOP_BRRQ) == -1);
    CHECK("zero result capacity sets EINVAL", errno == EINVAL);
    errno = 0;
    CHECK("lookup rejects a negative result capacity",
          nbp_do_lookup_op(NULL, NULL, NULL, &nve, -1, NULL, NULL,
                           NBPOP_BRRQ) == -1);
    CHECK("negative result capacity sets EINVAL", errno == EINVAL);
    errno = 0;
    CHECK("lookup rejects a null result array",
          nbp_do_lookup_op(NULL, NULL, NULL, NULL, 1, NULL, NULL,
                           NBPOP_BRRQ) == -1);
    CHECK("null result array sets EINVAL", errno == EINVAL);
}

int main(void)
{
    test_valid_tuple();
    test_overlong_strings();
    test_truncated_tuple();
    test_match_rejects_invalid_lengths();
    test_lookup_rejects_invalid_result_array();

    if (failures != 0) {
        fprintf(stderr, "%d test(s) FAILED.\n", failures);
        return 1;
    }

    puts("All NBP parser tests passed.");
    return 0;
}
