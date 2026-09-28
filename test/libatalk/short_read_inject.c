/*
 * Test-only LD_PRELOAD interposer for the OS X AppleDouble header reader
 *
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * When armed by test_convert.c, make one AD_DATASZ_OSX header read return
 * only the fixed header and two-entry table. This deterministically models a
 * sidecar truncated from 114 bytes to 50 bytes after fstat() but before
 * pread(), without adding a timing-dependent race to the regression test.
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

#include "config.h"

#include <atalk/adouble.h>

#include <dlfcn.h>
#include <stddef.h>
#include <sys/types.h>

/* Strong definitions are exported by adouble_convert_test. The weak fallbacks
 * keep this preload library linkable and harmless in any other process. */
int adouble_test_short_header_read __attribute__((weak));
int adouble_test_short_header_read_fired __attribute__((weak));

typedef ssize_t (*adf_pread_fn)(struct ad_fd *, void *, size_t, off_t);

ssize_t adf_pread(struct ad_fd *adf, void *buf, size_t count, off_t offset)
{
    static adf_pread_fn real_adf_pread;

    if (real_adf_pread == NULL) {
        real_adf_pread = (adf_pread_fn)dlsym(RTLD_NEXT, "adf_pread");
    }

    if (adouble_test_short_header_read
            && count == AD_DATASZ_OSX
            && offset == 0) {
        adouble_test_short_header_read = 0;
        adouble_test_short_header_read_fired++;
        count = AD_HEADER_LEN + ADEID_NUM_OSX * AD_ENTRY_LEN;
    }

    return real_adf_pread(adf, buf, count, offset);
}
