/*
   Copy from samba lib/replace.c

   Unix SMB/CIFS implementation.
   replacement routines for broken systems
   Copyright (C) Andrew Tridgell 1992-1998

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   strlcpy strlcat functions.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <atalk/util.h>

#ifndef HAVE_STRLCPY
/*!
 * @brief Write a string into a buffer with truncation and a terminating NUL,
 * for systems without strlcpy
 *
 * Unlike strncpy, the call leaves the rest of the buffer untouched.
 *
 * @param[out] d        destination buffer
 * @param[in]  s        source string
 * @param[in]  bufsize  size of d
 * @returns the length of s, or 0 for a bufsize of 0
 */
size_t strlcpy(char *d, const char *s, size_t bufsize)
{
    size_t len = strlen(s);
    size_t ret = len;

    if (bufsize <= 0) {
        return 0;
    }

    if (len >= bufsize) {
        len = bufsize - 1;
    }

    memcpy(d, s, len);
    d[len] = 0;
    return ret;
}
#endif

#ifndef HAVE_STRLCAT
/*!
 * @brief Append a string to a buffer with truncation and a terminating NUL, for
 * systems without strlcat
 *
 * @param[in,out] d        NUL-terminated buffer
 * @param[in]     s        string to append
 * @param[in]     bufsize  size of d
 * @returns the length that the joined string needs, or 0 when d holds no NUL
 *          within bufsize
 */
size_t strlcat(char *d, const char *s, size_t bufsize)
{
    size_t len1 = strlen(d);
    size_t len2 = strlen(s);
    size_t ret = len1 + len2;

    if (len1 >= bufsize) {
        return 0;
    }

    if (len1 + len2 >= bufsize) {
        len2 = bufsize - (len1 + 1);
    }

    if (len2 > 0) {
        memcpy(d + len1, s, len2);
        d[len1 + len2] = 0;
    }

    return ret;
}
#endif
