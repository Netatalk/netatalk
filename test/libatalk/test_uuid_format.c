/*
 * Regression tests for UUID formatting and configured mask bounds
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

/* Expose MAP_ANONYMOUS when compiling in strict C mode. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <atalk/ldapconfig.h>
#include <atalk/uuid.h>

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

static int failures;

static void check(int passed, const char *name)
{
    if (!passed) {
        fprintf(stderr, "FAIL: %s\n", name);
        failures++;
    }
}

int main(void)
{
    const unsigned char bytes[UUID_BINSIZE] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
    };
    const char *standard = "01234567-89AB-CDEF-FEDC-BA9876543210";
    unsigned char decoded[UUID_BINSIZE];
    long pagesize = sysconf(_SC_PAGESIZE);
    unsigned char *mapping;
    unsigned char *uuid;

    if (pagesize <= 0) {
        return 1;
    }

    mapping = mmap(NULL, pagesize * 2, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mapping == MAP_FAILED) {
        return 1;
    }

    if (mprotect(mapping + pagesize, pagesize, PROT_NONE) != 0) {
        munmap(mapping, pagesize * 2);
        return 1;
    }

    /* Any read past the 16-byte UUID fails immediately. */
    uuid = mapping + pagesize - UUID_BINSIZE;
    memcpy(uuid, bytes, sizeof(bytes));
    check(strcmp(uuid_bin2string(uuid), standard) == 0, "default uppercase UUID");
    uuid_string2bin(uuid_bin2string(uuid), decoded);
    check(memcmp(decoded, bytes, sizeof(bytes)) == 0, "default round trip");
#ifdef HAVE_LDAP
    char *saved_mask = ldap_uuid_string;
    char longest_mask[64];
    char longest_expected[64];
    char oversized_mask[65];
    const char *digits = "0123456789ABCDEFFEDCBA9876543210";
    char invalid_masks[][sizeof(longest_mask)] = {
        "", "x", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
        "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
        "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxz",
    };
    ldap_uuid_string = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    check(strcmp(uuid_bin2string(uuid), digits) == 0, "undashed UUID");
    ldap_uuid_string = "x-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    check(strcmp(uuid_bin2string(uuid), "0-123456789ABCDEFFEDCBA9876543210") == 0,
          "separator between nibbles");

    /* All 32 digits separated by dashes fill the output buffer exactly. */
    for (size_t i = 0; i < 63; i++) {
        longest_mask[i] = i % 2 ? '-' : 'x';
        longest_expected[i] = i % 2 ? '-' : digits[i / 2];
    }

    longest_mask[63] = longest_expected[63] = '\0';
    ldap_uuid_string = longest_mask;
    check(strcmp(uuid_bin2string(uuid), longest_expected) == 0,
          "maximum valid mask length");
    uuid_string2bin(uuid_bin2string(uuid), decoded);
    check(memcmp(decoded, bytes, sizeof(bytes)) == 0, "custom mask round trip");
    memcpy(oversized_mask, longest_mask, 63);
    oversized_mask[63] = '-';
    oversized_mask[64] = '\0';
    ldap_uuid_string = oversized_mask;
    check(strcmp(uuid_bin2string(uuid), standard) == 0, "oversized mask fallback");

    for (size_t i = 0; i < sizeof(invalid_masks) / sizeof(invalid_masks[0]); i++) {
        ldap_uuid_string = invalid_masks[i];
        check(strcmp(uuid_bin2string(uuid), standard) == 0, "invalid mask fallback");
    }

    ldap_uuid_string = NULL;
    check(strcmp(uuid_bin2string(uuid), standard) == 0,
          "default after custom mask");
    ldap_uuid_string = saved_mask;
#endif
    munmap(mapping, pagesize * 2);
    return failures ? 1 : 0;
}
