/*
 * Regression test for hton64() on a value whose byte-swapped low word has
 * bit 31 set
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

#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <atalk/util.h>

int main(int argc, char **argv _U_)
{
    static const unsigned char want[8] = { 0, 0, 0, 0, 0, 0, 0x0b, 0xb8 };
    /* argc is 1: a value known only at run time keeps the shift out of
     * constant folding */
    uint64_t v = 3000 * (uint64_t)argc;
    uint64_t be = hton64(v);

    if (memcmp(&be, want, sizeof(want)) != 0) {
        fprintf(stderr, "FAIL: hton64(3000) is not big-endian 3000\n");
        return 1;
    }

    printf("All tests passed.\n");
    return 0;
}
