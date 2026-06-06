/*
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <atalk/constant_time.h>

/*!
 * @brief Constant-time memory equality check.
 *
 * Compares exactly @p n bytes and returns 0 if the buffers are equal,
 * non-zero if they differ.
 *
 * @note Unlike memcmp(), this function does not provide
 * lexicographic ordering and must not be used for sorting.
 *
 * @returns 0 if the buffers are equal, non-zero if they differ.
 */
int atalk_ct_memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *pa = a;
    const unsigned char *pb = b;
    volatile unsigned char diff = 0;

    for (size_t i = 0; i < n; i++) {
        diff |= pa[i] ^ pb[i];
    }

    return diff != 0;
}
