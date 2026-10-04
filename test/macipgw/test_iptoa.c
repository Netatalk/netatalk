/*
 * Regression tests for IPv4 address formatting and host byte order
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

#include <stdio.h>
#include <string.h>

#include "../../contrib/macipgw/util.h"

int main(void)
{
    static const struct {
        uint32_t address;
        const char *expected;
    } cases[] = {
        {UINT32_MAX, "255.255.255.255"}, //NOSONAR
        {0, "0.0.0.0"},                  //NOSONAR
        {1, "0.0.0.1"},                  //NOSONAR
        {0x01020304, "1.2.3.4"},         //NOSONAR
        {0x7f000001, "127.0.0.1"},       //NOSONAR
        {0xc0000201, "192.0.2.1"},       //NOSONAR
        {0xff000001, "255.0.0.1"},       //NOSONAR
        {0x010000ff, "1.0.0.255"},       //NOSONAR
    };
    int failures = 0;

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        const char *actual = iptoa(cases[i].address);

        if (strcmp(actual, cases[i].expected) != 0) {
            fprintf(stderr, "FAIL: expected %s, got %s\n", cases[i].expected, actual);
            failures++;
        }
    }

    return failures ? 1 : 0;
}
