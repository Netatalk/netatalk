/*
 * NBP lookup status message output formatting routines
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
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <atalk/unicode.h>

#include "nbplkup_output.h"

size_t nbplkup_convert_field(charset_t from, const char *src, size_t src_len,
                             char **dest)
{
    size_t dest_len;

    if (src == NULL || dest == NULL) {
        errno = EINVAL;
        return (size_t) -1;
    }

    *dest = NULL;
    dest_len = convert_string_allocate(from, CH_UNIX, src, src_len, dest);

    if (dest_len != (size_t) -1) {
        return dest_len;
    }

    if (src_len == SIZE_MAX) {
        errno = EOVERFLOW;
        return (size_t) -1;
    }

    *dest = malloc(src_len + 1);

    if (*dest == NULL) {
        return (size_t) -1;
    }

    memcpy(*dest, src, src_len);
    (*dest)[src_len] = '\0';
    return src_len;
}
