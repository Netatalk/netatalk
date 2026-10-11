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
 * @brief Compare two buffers for equality in a time that depends only on their
 * length
 *
 * The call reads all n bytes of both buffers, whatever they hold. Unlike
 * memcmp, the result gives no order, so it cannot sort.
 *
 * @param[in] a  first buffer
 * @param[in] b  second buffer
 * @param[in] n  number of bytes
 * @returns 0 for equal buffers, 1 otherwise
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
