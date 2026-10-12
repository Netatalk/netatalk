/*
  Unix SMB/CIFS implementation.
  Character set conversion Extensions
  Copyright (C) Igor Vergeichik <iverg@mail.ru> 2001
  Copyright (C) Andrew Tridgell 2001
  Copyright (C) Simo Sorce 2001
  Copyright (C) Martin Pool 2003

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

*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_USABLE_ICONV
#include <iconv.h>
#endif

#include <atalk/logger.h>
#include <atalk/unicode.h>
#include <atalk/util.h>
#include <atalk/compat.h>
#include <atalk/byteorder.h>

/*!
 * @file
 * Character-set conversion routines based on samba iconv wrappers.
 */


#define MAX_CHARSETS 20

#define CHECK_FLAGS(a,b) (((a)!=NULL) ? (*(a) & (b)) : 0 )

static atalk_iconv_t conv_handles[MAX_CHARSETS][MAX_CHARSETS];
static char *charset_names[MAX_CHARSETS];
static struct charset_functions *charsets[MAX_CHARSETS];
static size_t source_unit_sizes[MAX_CHARSETS];
static char hexdig[] = "0123456789abcdef";
#define MAX_CONVERT_SIZE (1 << 20)


/*!
 * @brief Return the iconv name of a charset
 *
 * UCS2, UTF8, and UTF8-MAC use fixed names, and the others come from the
 * name table.
 *
 * @param[in] ch  charset
 * @returns the name, or NULL for an unnamed table slot
 */
static const char *charset_name(charset_t ch)
{
    const char *ret = NULL;

    if (ch == CH_UCS2) {
        ret = "UCS-2";
    } else if (ch == CH_UTF8) {
        ret = "UTF8";
    } else if (ch == CH_UTF8_MAC) {
        ret = "UTF8-MAC";
    } else {
        ret = charset_names[ch];
    }

    return ret;
}

/*!
 * @brief Set the name of a fixed charset slot in the name table
 *
 * The call stores a strdup of the name and does not free an earlier one.
 *
 * @param[in] ch    charset below NUM_CHARSETS
 * @param[in] name  charset name
 * @returns 0, or -1 for a charset outside the fixed slots
 */
int set_charset_name(charset_t ch, const char *name)
{
    if (ch >= NUM_CHARSETS) {
        return -1;
    }

    charset_names[ch] = strdup(name);
    source_unit_sizes[ch] = 0;
    return 0;
}

/*!
 * @brief Free every name in the charset name table
 */
void free_charset_names(void)
{
    for (int ch = 0; ch < MAX_CHARSETS; ch++) {
        source_unit_sizes[ch] = 0;

        if (charset_names[ch]) {
            free(charset_names[ch]);
            charset_names[ch] = NULL;
        }
    }
}

/*!
 * @brief Return the built-in conversion functions of a charset, which the call
 * looks up once by name
 *
 * @param[in] ch  charset
 * @returns the functions, or NULL for a charset that only iconv knows
 */
static struct charset_functions *get_charset_functions(charset_t ch)
{
    if (charsets[ch] != NULL) {
        return charsets[ch];
    }

    charsets[ch] = find_charset_functions(charset_name(ch));
    return charsets[ch];
}


/*!
 * @brief Run init_iconv once, on the first conversion or charset lookup
 */
static void lazy_initialize_conv(void)
{
    static int initialized = 0;

    if (!initialized) {
        initialized = 1;
        init_iconv();
    }
}

/*!
 * @brief Find a charset by name, or register it and open its iconv descriptors
 * to and from UCS2
 *
 * The name match ignores case. Up to MAX_CHARSETS charsets fit, and a name
 * that iconv cannot convert fails.
 *
 * @param[in] name  charset name
 * @returns the charset, or `(charset_t)-1` for an error
 */
charset_t add_charset(const char *name)
{
    static charset_t max_charset_t = NUM_CHARSETS - 1;
    charset_t cur_charset_t = max_charset_t + 1;
    unsigned int c1;
    lazy_initialize_conv();

    for (c1 = 0; c1 <= max_charset_t; c1++) {
        if (strcasecmp(name, charset_name(c1)) == 0) {
            return c1;
        }
    }

    if (cur_charset_t >= MAX_CHARSETS)  {
        LOG(log_debug, logtype_default,
            "Adding charset %s failed, too many charsets (max. %u allowed)",
            name, MAX_CHARSETS);
        return (charset_t) -1;
    }

    /* First try to setup the required conversions */
    conv_handles[cur_charset_t][CH_UCS2] = atalk_iconv_open(charset_name(CH_UCS2),
                                                            name);

    if (conv_handles[cur_charset_t][CH_UCS2] == (atalk_iconv_t) -1) {
        LOG(log_error, logtype_default,
            "Required conversion from %s to %s not supported",
            name,  charset_name(CH_UCS2));
        conv_handles[cur_charset_t][CH_UCS2] = NULL;
        return (charset_t) -1;
    }

    conv_handles[CH_UCS2][cur_charset_t] = atalk_iconv_open(name,
                                                            charset_name(CH_UCS2));

    if (conv_handles[CH_UCS2][cur_charset_t] == (atalk_iconv_t) -1) {
        LOG(log_error, logtype_default,
            "Required conversion from %s to %s not supported",
            charset_name(CH_UCS2), name);
        conv_handles[CH_UCS2][cur_charset_t] = NULL;
        return (charset_t) -1;
    }

    /* register the new charset_t name */
    charset_names[cur_charset_t] = strdup(name);
    charsets[cur_charset_t] = get_charset_functions(cur_charset_t);
    max_charset_t++;
    LOG(log_debug9, logtype_default, "Added charset %s with handle %u", name,
        cur_charset_t);
    return cur_charset_t;
}

/*!
 * @brief Initialize iconv conversion descriptors
 *
 * This is called the first time it is needed, and also called again
 * every time the configuration is reloaded, because the charset or
 * codepage might have changed.
 */
void init_iconv(void)
{
    int c1;

    for (c1 = 0; c1 < NUM_CHARSETS; c1++) {
        const char *name = charset_name((charset_t)c1);
        conv_handles[c1][CH_UCS2] = atalk_iconv_open(charset_name(CH_UCS2), name);

        if (conv_handles[c1][CH_UCS2] == (atalk_iconv_t) -1) {
            LOG(log_error, logtype_default,
                "Required conversion from %s to %s not supported",
                name,  charset_name(CH_UCS2));
            conv_handles[c1][CH_UCS2] = NULL;
        }

        if (c1 != CH_UCS2) { /* avoid lost memory, make valgrind happy */
            conv_handles[CH_UCS2][c1] = atalk_iconv_open(name, charset_name(CH_UCS2));

            if (conv_handles[CH_UCS2][c1] == (atalk_iconv_t) -1) {
                LOG(log_error, logtype_default,
                    "Required conversion from %s to %s not supported",
                    charset_name(CH_UCS2), name);
                conv_handles[CH_UCS2][c1] = NULL;
            }
        }

        charsets[c1] = get_charset_functions(c1);
    }
}

/*!
 * @brief Terminate a converted string with one NUL byte, or two for UCS2
 *
 * @param[in]     to         target charset
 * @param[in,out] buf        output buffer
 * @param[in]     bytesleft  free bytes after the string
 * @param[in]     len        length of the string
 * @returns @p len, or `(size_t)-1` with E2BIG when the NUL does not fit
 */
static size_t add_null(charset_t to, char *buf, size_t bytesleft, size_t len)
{
    /* Terminate the string */
    if (to == CH_UCS2 && bytesleft >= 2) {
        buf[len]   = 0;
        buf[len + 1] = 0;
    } else if (to != CH_UCS2 && bytesleft > 0) {
        buf[len]   = 0;
    } else {
        errno = E2BIG;
        return (size_t) -1;
    }

    return len;
}


/*!
 * @brief Convert string from one encoding to another,
 * making error checking etc
 *
 * @param from source character set
 * @param to destination character set
 * @param src pointer to source string (multibyte or singlebyte)
 * @param srclen length of the source string in bytes
 * @param dest pointer to destination string (multibyte or singlebyte)
 * @param destlen maximal length allowed for string
 * @returns the number of bytes occupied in the destination
 */
static size_t convert_string_internal(charset_t from, charset_t to,
                                      void const *src, size_t srclen,
                                      void *dest, size_t destlen)
{
    size_t i_len, o_len;
    size_t retval;
    const char *inbuf = (const char *)src;
    char *outbuf = (char *)dest;
    char *o_save = outbuf;
    atalk_iconv_t descriptor;

    /* Fixed based on Samba 3.0.6 */
    if (srclen == (size_t) -1) {
        if (from == CH_UCS2) {
            srclen = (strlen_w((const ucs2_t *)src)) * 2;
        } else {
            srclen = strlen((const char *)src);
        }
    }

    lazy_initialize_conv();
    descriptor = conv_handles[from][to];

    if (descriptor == (atalk_iconv_t) -1 || descriptor == (atalk_iconv_t) 0) {
        return (size_t) -1;
    }

    i_len = srclen;
    o_len = destlen;
    retval = atalk_iconv(descriptor,  &inbuf, &i_len, &outbuf, &o_len);

    if (retval == (size_t) -1) {
        const char *reason = "unknown error";

        switch (errno) {
        case EINVAL:
            reason = "Incomplete multibyte sequence";
            break;

        case E2BIG:
            reason = "No more room";
            break;

        case EILSEQ:
            reason = "Illegal multibyte sequence";
            break;
        }

        LOG(log_debug, logtype_default, "Conversion error: %s", reason);
        return (size_t) -1;
    }

    /* Terminate the string */
    return add_null(to, o_save, o_len, destlen - o_len);
}


/*!
 * @brief Convert a string between two charsets through UCS2, with Unicode
 * composition
 *
 * A decomposed target charset gets decomposed text. Otherwise text from a
 * decomposed source, or from a charset without built-in functions, gets
 * precomposed. The intermediate buffers hold MAXPATHLEN UCS2 characters,
 * and the output ends with a NUL.
 *
 * @param[in]  from     source charset
 * @param[in]  to       target charset
 * @param[in]  src      source string
 * @param[in]  srclen   source length in bytes, or `(size_t)-1` for the string
 *                      length
 * @param[out] dest     output buffer
 * @param[in]  destlen  size of @p dest
 * @returns the output length without the NUL, or `(size_t)-1` for an error
 */
size_t convert_string(charset_t from, charset_t to,
                      void const *src, size_t srclen,
                      void *dest, size_t destlen)
{
    size_t i_len, o_len;
    ucs2_t *u;
    ucs2_t buffer[MAXPATHLEN];
    ucs2_t buffer2[MAXPATHLEN];

    /* convert from_set to UCS2 */
    if ((size_t) -1 == (o_len = convert_string_internal(from, CH_UCS2, src, srclen,
                                                        (char *) buffer, sizeof(buffer)))) {
        LOG(log_error, logtype_default, "Conversion failed ( %s to CH_UCS2 )",
            charset_name(from));
        return (size_t) -1;
    }

    /* Do pre/decomposition */
    i_len = sizeof(buffer2);
    u = buffer2;

    if (charsets[to] && (charsets[to]->flags & CHARSET_DECOMPOSED)) {
        if ((size_t) -1 == (i_len = decompose_w(buffer, o_len, u, &i_len))) {
            return (size_t) -1;
        }
    } else if (!charsets[from] || (charsets[from]->flags & CHARSET_DECOMPOSED)) {
        if ((size_t) -1 == (i_len = precompose_w(buffer, o_len, u, &i_len))) {
            return (size_t) -1;
        }
    } else {
        u = buffer;
        i_len = o_len;
    }

    /* Convert UCS2 to to_set */
    if ((size_t)(-1) == (o_len = convert_string_internal(CH_UCS2, to, (char *) u,
                                                         i_len, dest, destlen))) {
        LOG(log_error, logtype_default, "Conversion failed (CH_UCS2 to %s):%s",
            charset_name(to), strerror(errno));
        return (size_t) -1;
    }

    return o_len;
}



/*!
 * @brief Convert between character sets,
 * allocating a new buffer for the result
 *
 * @param from source character set
 * @param to destination character set
 * @param src pointer to source string (multibyte or singlebyte)
 * @param srclen length of source buffer
 * @param dest always set at least to NULL
 * @note -1 is not accepted for srclen
 *
 * @returns Size in bytes of the converted string; or -1 in case of error
 */

static size_t convert_string_allocate_internal(charset_t from, charset_t to,
                                               void const *src, size_t srclen, char **dest)
{
    size_t i_len, o_len, destlen;
    size_t retval;
    const char *inbuf = (const char *)src;
    char *outbuf = NULL, *ob = NULL;
    atalk_iconv_t descriptor;
    *dest = NULL;

    if (src == NULL || srclen == (size_t) -1) {
        return (size_t) -1;
    }

    lazy_initialize_conv();
    descriptor = conv_handles[from][to];

    if (descriptor == (atalk_iconv_t) -1 || descriptor == (atalk_iconv_t) 0) {
        /* conversion not supported, return -1*/
        LOG(log_debug, logtype_default,
            "convert_string_allocate: conversion not supported!");
        return -1;
    }

    destlen = MAX(srclen, 512);
convert:

    if (destlen > MAX_CONVERT_SIZE) {
        SAFE_FREE(ob);
        return (size_t) -1;
    }

    destlen = destlen * 2;
    outbuf = (char *)realloc(ob, destlen);

    if (!outbuf) {
        LOG(log_debug, logtype_default, "convert_string_allocate: realloc failed!");
        SAFE_FREE(ob);
        return (size_t) -1;
    } else {
        ob = outbuf;
    }

    inbuf = src;   /* this restarts the whole conversion if buffer needed to be increased */
    i_len = srclen;
    o_len = destlen;
    retval = atalk_iconv(descriptor,
                         &inbuf, &i_len,
                         &outbuf, &o_len);

    if (retval == (size_t) -1)        {
        const char *reason = "unknown error";

        switch (errno) {
        case EINVAL:
            reason = "Incomplete multibyte sequence";
            break;

        case E2BIG:
            goto convert;

        case EILSEQ:
            reason = "Illegal multibyte sequence";
            break;
        }

        LOG(log_debug, logtype_default, "Conversion error: %s(%s)", reason, inbuf);
        SAFE_FREE(ob);
        return (size_t) -1;
    }

    destlen = destlen - o_len;

    /* Terminate the string */
    if (to == CH_UCS2 && o_len >= 2) {
        ob[destlen] = 0;
        ob[destlen + 1] = 0;
        *dest = (char *)realloc(ob, destlen + 2);
    } else if (to != CH_UCS2 && o_len > 0) {
        ob[destlen] = 0;
        *dest = (char *)realloc(ob, destlen + 1);
    } else {
        goto convert; /* realloc */
    }

    if (destlen && !*dest) {
        LOG(log_debug, logtype_default, "convert_string_allocate: out of memory!");
        SAFE_FREE(ob);
        return (size_t) -1;
    }

    return destlen;
}


/*!
 * @brief Convert a string between two charsets through UCS2 into a buffer from
 * malloc
 *
 * The composition rules match convert_string, and the buffer ends with a
 * NUL.
 *
 * @param[in]  from    source charset
 * @param[in]  to      target charset
 * @param[in]  src     source string
 * @param[in]  srclen  source length in bytes, or `(size_t)-1` for the string
 *                     length
 * @param[out] dest    receives the buffer for the caller to free, or NULL
 * @returns the output length without the NUL, or `(size_t)-1` for an error
 */
size_t convert_string_allocate(charset_t from, charset_t to,
                               void const *src, size_t srclen,
                               char **dest)
{
    size_t i_len, o_len;
    ucs2_t *u;
    ucs2_t buffer[MAXPATHLEN];
    ucs2_t buffer2[MAXPATHLEN];
    *dest = NULL;

    /* convert from_set to UCS2 */
    if ((size_t)(-1) == (o_len = convert_string_internal(from, CH_UCS2, src, srclen,
                                                         buffer, sizeof(buffer)))) {
        LOG(log_error, logtype_default, "Conversion failed ( %s to CH_UCS2 )",
            charset_name(from));
        return (size_t) -1;
    }

    /* Do pre/decomposition */
    i_len = sizeof(buffer2);
    u = buffer2;

    if (charsets[to] && (charsets[to]->flags & CHARSET_DECOMPOSED)) {
        if ((size_t) -1 == (i_len = decompose_w(buffer, o_len, u, &i_len))) {
            return (size_t) -1;
        }
    } else if (!charsets[from] || (charsets[from]->flags & CHARSET_DECOMPOSED)) {
        if ((size_t) -1 == (i_len = precompose_w(buffer, o_len, u, &i_len))) {
            return (size_t) -1;
        }
    } else {
        u = buffer;
        i_len = o_len;
    }

    /* Convert UCS2 to to_set */
    if ((size_t) -1 == (o_len = convert_string_allocate_internal(CH_UCS2, to,
        (char *)u, i_len, dest))) {
        LOG(log_error, logtype_default, "Conversion failed (CH_UCS2 to %s):%s",
            charset_name(to), strerror(errno));
    }

    return o_len;
}

/*!
 * @brief Uppercase a string through UCS2 and keep its charset
 *
 * An in-place call that changes nothing returns @p srclen and leaves the
 * buffer as it is.
 *
 * @param[in]  ch       charset of the string
 * @param[in]  src      source string
 * @param[in]  srclen   source length in bytes
 * @param[out] dest     output buffer, which may be @p src
 * @param[in]  destlen  size of @p dest
 * @returns the output length, or `(size_t)-1` for an error
 */
size_t charset_strupper(charset_t ch, const char *src, size_t srclen,
                        char *dest, size_t destlen)
{
    size_t size;
    char *buffer;
    size = convert_string_allocate_internal(ch, CH_UCS2, src, srclen,
                                            (char **) &buffer);

    if (size == (size_t) -1) {
        SAFE_FREE(buffer);
        return size;
    }

    if (!strupper_w((ucs2_t *)buffer) && (dest == src)) {
        free(buffer);
        return srclen;
    }

    size = convert_string_internal(CH_UCS2, ch, buffer, size, dest, destlen);
    free(buffer);
    return size;
}

/*!
 * @brief Lowercase a string through UCS2 and keep its charset
 *
 * An in-place call that changes nothing returns @p srclen and leaves the
 * buffer as it is.
 *
 * @param[in]  ch       charset of the string
 * @param[in]  src      source string
 * @param[in]  srclen   source length in bytes
 * @param[out] dest     output buffer, which may be @p src
 * @param[in]  destlen  size of @p dest
 * @returns the output length, or `(size_t)-1` for an error
 */
size_t charset_strlower(charset_t ch, const char *src, size_t srclen,
                        char *dest, size_t destlen)
{
    size_t size;
    char *buffer;
    size = convert_string_allocate_internal(ch, CH_UCS2, src, srclen,
                                            (char **) &buffer);

    if (size == (size_t) -1) {
        SAFE_FREE(buffer);
        return size;
    }

    if (!strlower_w((ucs2_t *)buffer) && (dest == src)) {
        free(buffer);
        return srclen;
    }

    size = convert_string_internal(CH_UCS2, ch, buffer, size, dest, destlen);
    free(buffer);
    return size;
}

/*!
 * @brief Copy a string from a UCS2 src to a unix char * destination,
 * allocating a buffer
 *
 * @param ch destination character set
 * @param src source UCS2 string
 * @param dest always set at least to NULL
 * @param destlen maximum length of destination buffer
 *
 * @returns The number of bytes occupied by the string in the destination
 */
size_t ucs2_to_charset(charset_t ch, const ucs2_t *src, char *dest,
                       size_t destlen)
{
    size_t src_len = (strlen_w(src)) * sizeof(ucs2_t);
    return convert_string(CH_UCS2, ch, src, src_len, dest, destlen);
}

/*!
 * @brief Convert a NUL-terminated UCS2 string to a charset, into a buffer from
 * malloc
 *
 * @param[in]  ch    target charset
 * @param[out] dest  receives the buffer for the caller to free, or NULL
 * @param[in]  src   UCS2 string
 * @returns the output length, or `(size_t)-1` for an error
 */
size_t ucs2_to_charset_allocate(charset_t ch, char **dest, const ucs2_t *src)
{
    size_t src_len = (strlen_w(src)) * sizeof(ucs2_t);
    *dest = NULL;
    return convert_string_allocate(CH_UCS2, ch, src, src_len, dest);
}

/*!
 * @brief Precompose a string through UCS2 and keep its charset
 *
 * @param[in]  ch      charset of the string
 * @param[in]  src     source string
 * @param[in]  inlen   source length in bytes
 * @param[out] dst     output buffer
 * @param[in]  outlen  size of @p dst
 * @returns the output length, or `(size_t)-1` for an error
 */
size_t charset_precompose(charset_t ch, char *src, size_t inlen, char *dst,
                          size_t outlen)
{
    char *buffer;
    ucs2_t u[MAXPATHLEN];
    size_t len;
    size_t ilen;

    if ((size_t)(-1) == (len = convert_string_allocate_internal(ch, CH_UCS2, src,
                                                                inlen, &buffer))) {
        return len;
    }

    ilen = sizeof(u);

    if ((size_t) -1 == (ilen = precompose_w((ucs2_t *)buffer, len, u, &ilen))) {
        free(buffer);
        return (size_t) -1;
    }

    if ((size_t)(-1) == (len = convert_string_internal(CH_UCS2, ch, (char *)u, ilen,
                                                       dst, outlen))) {
        free(buffer);
        return (size_t) -1;
    }

    free(buffer);
    return len;
}

/*!
 * @brief Decompose a string through UCS2 and keep its charset
 *
 * @param[in]  ch      charset of the string
 * @param[in]  src     source string
 * @param[in]  inlen   source length in bytes
 * @param[out] dst     output buffer
 * @param[in]  outlen  size of @p dst
 * @returns the output length, or `(size_t)-1` for an error
 */
size_t charset_decompose(charset_t ch, char *src, size_t inlen, char *dst,
                         size_t outlen)
{
    char *buffer;
    ucs2_t u[MAXPATHLEN];
    size_t len;
    size_t ilen;

    if ((size_t)(-1) == (len = convert_string_allocate_internal(ch, CH_UCS2, src,
                                                                inlen, &buffer))) {
        return len;
    }

    ilen = sizeof(u);

    if ((size_t) -1 == (ilen = decompose_w((ucs2_t *)buffer, len, u, &ilen))) {
        free(buffer);
        return (size_t) -1;
    }

    if ((size_t)(-1) == (len = convert_string_internal(CH_UCS2, ch, (char *)u, ilen,
                                                       dst, outlen))) {
        free(buffer);
        return (size_t) -1;
    }

    free(buffer);
    return len;
}

/*!
 * @brief Return an ASCII hex digit's value, or -1 for another Unicode character
 *
 * @param[in] c  decoded character
 * @returns 0 to 15 for an ASCII hex digit, or -1
 */
static int filename_hex_value(ucs2_t c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return -1;
}

/*!
 * @brief Find the source code-unit size for replacement after a decoding error
 *
 * A Unicode NUL occupies two bytes in UTF16 and four in UTF32. Encode one
 * NUL, then append another using the same descriptor. The output-length
 * difference measures one code unit without including the initial BOM.
 * Probe the converter instead of matching encoding names, so aliases keep
 * the same recovery alignment. Other encodings retain the existing
 * one-byte replacement policy. Cache the result until the charset name changes.
 *
 * @param[in] from_set  source charset
 * @returns the code-unit size in bytes, or 1 if the probe fails
 */
static size_t source_code_unit_size(charset_t from_set)
{
    if (source_unit_sizes[from_set] != 0) {
        return source_unit_sizes[from_set];
    }

    ucs2_t nul = 0;
    char encoded[16];
    const char *inbuf = (const char *)&nul;
    char *outbuf = encoded;
    size_t inleft = sizeof(nul);
    size_t outleft = sizeof(encoded);
    size_t unit = 1;
    atalk_iconv_t descriptor = atalk_iconv_open(charset_name(from_set), "UCS-2");

    if (descriptor == (atalk_iconv_t) -1) {
        return source_unit_sizes[from_set] = 1;
    }

    size_t result = atalk_iconv(descriptor, &inbuf, &inleft, &outbuf, &outleft);

    if (result != (size_t) -1 && inleft == 0) {
        size_t first_len = sizeof(encoded) - outleft;
        inbuf = (const char *)&nul;
        inleft = sizeof(nul);
        result = atalk_iconv(descriptor, &inbuf, &inleft, &outbuf, &outleft);

        if (result != (size_t) -1 && inleft == 0) {
            unit = sizeof(encoded) - outleft - first_len;
        }
    }

    atalk_iconv_close(descriptor);
    source_unit_sizes[from_set] = unit == 2 || unit == 4 ? unit : 1;
    return source_unit_sizes[from_set];
}

/*!
 * @brief Restore a BOM-selected byte order after the iconv error reset
 *
 * Test whether the first code unit is consumed without output. If so, replay
 * that BOM on the reset descriptor before continuing after a replacement.
 *
 * @param[in] descriptor  source decoding descriptor after an error reset
 * @param[in] from_set    source charset
 * @param[in] src         original source bytes
 * @param[in] srclen      original source length in bytes
 * @param[in] unit        source code-unit size in bytes
 * @returns 0 on success, or -1 with errno if a converter fails
 */
static int restore_source_bom(atalk_iconv_t descriptor, charset_t from_set,
                              const char *src, size_t srclen, size_t unit)
{
    if (unit == 1 || srclen < unit) {
        return 0;
    }

    atalk_iconv_t probe = atalk_iconv_open("UCS-2", charset_name(from_set));

    if (probe == (atalk_iconv_t) -1) {
        return -1;
    }

    ucs2_t output[2];
    const char *inbuf = src;
    char *outbuf = (char *)output;
    size_t inleft = unit;
    size_t outleft = sizeof(output);
    size_t result = atalk_iconv(probe, &inbuf, &inleft, &outbuf, &outleft);
    atalk_iconv_close(probe);

    if (result != (size_t) -1 && inleft == 0 && outleft == sizeof(output)) {
        inbuf = src;
        inleft = unit;
        return atalk_iconv(descriptor, &inbuf, &inleft, &outbuf, &outleft)
               == (size_t) -1 ? -1 : 0;
    }

    return 0;
}

/*!
 * @brief Decode source bytes to UCS2 with the source-error recovery flags
 *
 * CONV_IGNORE returns the decoded prefix and sets CONV_REQMANGLE.
 * CONV__EILSEQ replaces invalid source code units with IGNORE_CHAR.
 * CONV_FORCE accepts a partial result after a decoding error.
 *
 * @param[in]     from_set        source charset
 * @param[in]     src             source bytes
 * @param[in]     srclen          source length in bytes; (size_t)-1 infers length
 * @param[out]    decoded         UCS2 output buffer
 * @param[in]     decoded_len     size of decoded in bytes
 * @param[out]    remaining_input source bytes left after decoding succeeds
 * @param[in,out] flags           CONV_* options and result flags
 * @returns decoded UCS2 code-unit count, or (size_t)-1 with errno
 */
static size_t decode_source_ucs2(charset_t from_set, const char *src,
                                 size_t srclen, ucs2_t *decoded, size_t decoded_len,
                                 size_t *remaining_input, uint16_t *flags)
{
    const uint16_t option = (flags ? *flags : 0);
    const char *input = src;
    char *decoded_end = (char *)decoded;
    size_t input_left = srclen;
    size_t decoded_left = decoded_len;
    atalk_iconv_t descriptor = conv_handles[from_set][CH_UCS2];

    if (descriptor == (atalk_iconv_t) -1 || descriptor == NULL) {
        errno = EINVAL;
        return (size_t) -1;
    }

    const size_t source_unit = source_code_unit_size(from_set);
#ifdef HAVE_USABLE_ICONV

    if (descriptor->cd_direct || descriptor->cd_pull) {
        /* Reopen for each filename: a reset can retain BOM byte-order state
         * in glibc, and musl's NULL-input reset leaves shift state intact. */
        atalk_iconv_t fresh = atalk_iconv_open("UCS-2", charset_name(from_set));

        if (fresh == (atalk_iconv_t) -1) {
            return (size_t) -1;
        }

        atalk_iconv_close(descriptor);
        descriptor = conv_handles[from_set][CH_UCS2] = fresh;
    }

#endif

    if (srclen == (size_t) -1) {
        /* Native UCS2 can infer its length; other wide sources need byte lengths. */
        if (from_set == CH_UCS2) {
            ucs2_t c;
            input_left = 0;

            do {
                memcpy(&c, src + input_left, sizeof(c));
                input_left += sizeof(c);
            } while (c != 0);
        } else {
            input_left = strlen(src) + 1;
        }
    }

    size_t source_len = input_left;

    while (input_left > 0) {
        /* The native UCS2 converter is a byte copy; exclude a trailing byte. */
        size_t chunk_left = from_set == CH_UCS2 ? input_left & ~(size_t)1
                            : input_left;
        size_t trailing = input_left - chunk_left;
        size_t result = atalk_iconv(descriptor, &input, &chunk_left,
                                    &decoded_end, &decoded_left);
        input_left = chunk_left + trailing;

        if (result != (size_t) -1 && input_left == 0) {
            break;
        }

        if (result != (size_t) -1 || errno == EINVAL || errno == EILSEQ) {
            errno = EILSEQ;

            if (option & CONV_IGNORE) {
                *flags |= CONV_REQMANGLE;
                break;
            }

            if (option & CONV__EILSEQ) {
                if (decoded_left < sizeof(ucs2_t)) {
                    errno = E2BIG;
                } else {
                    ucs2_t replacement = IGNORE_CHAR;
                    memcpy(decoded_end, &replacement, sizeof(replacement));
                    decoded_end += sizeof(replacement);
                    decoded_left -= sizeof(replacement);
                    size_t skip = input_left < source_unit ? input_left : source_unit;
                    input += skip;
                    input_left -= skip;

                    if (input_left > 0 && restore_source_bom(descriptor, from_set,
                                                             src, source_len, source_unit) != 0) {
                        if (!(option & CONV_FORCE)) {
                            return (size_t) -1;
                        }

                        break;
                    }

                    continue;
                }
            }
        }

        if (!(option & CONV_FORCE)) {
            return (size_t) -1;
        }

        break;
    }

    *remaining_input = input_left;
    return (decoded_end - (char *)decoded) / sizeof(ucs2_t);
}

/*!
 * @brief Emit decoded UCS2 characters with filename punctuation and escapes
 *
 * @param[in]     from_set   source charset, which picks slash mapping
 * @param[in]     to_set     target charset, which picks colon mapping
 * @param[in]     cap_set    charset of the bytes in :xx escapes
 * @param[in]     decoded    decoded UCS2 characters
 * @param[in]     count      decoded UCS2 code-unit count
 * @param[in]     input_left undecoded source bytes, for leading-dot escaping
 * @param[out]    dest       UCS2 output buffer
 * @param[in]     destlen    size of dest in bytes
 * @param[in,out] flags      CONV_* options and result flags
 * @returns bytes written, or (size_t)-1 with errno
 */
static size_t emit_filename_chars(charset_t from_set, charset_t to_set,
                                  charset_t cap_set, const ucs2_t *decoded,
                                  size_t count, size_t input_left, char *dest,
                                  size_t destlen, uint16_t *flags)
{
    const uint16_t option = (flags ? *flags : 0);
    char *outbuf = dest;
    size_t outleft = destlen;
    /* Source decoding may have replaced a descriptor shared with CAP. */
    atalk_iconv_t descriptor_cap = conv_handles[cap_set][CH_UCS2];
    size_t pos = 0;

    while (pos < count) {
        ucs2_t output[3];
        size_t output_count = 1;
        ucs2_t c = decoded[pos];

        if (pos == 0 && (count >= 2 || input_left > 0)
                && c == '.' && (option & CONV_ESCAPEDOTS)) {
            output[0] = ':';
            output[1] = '2';
            output[2] = 'e';
            output_count = 3;
            *flags |= CONV_REQESCAPE;
        } else if (c == ':' && (option & CONV_UNESCAPEHEX)) {
            char hex[MAXPATHLEN];
            size_t hexlen = 0;

            while (pos + 2 < count && decoded[pos] == ':'
                    && filename_hex_value(decoded[pos + 1]) >= 0
                    && filename_hex_value(decoded[pos + 2]) >= 0) {
                if (hexlen == sizeof(hex)) {
                    errno = E2BIG;
                    goto end;
                }

                memset(&hex[hexlen++],
                       (filename_hex_value(decoded[pos + 1]) << 4)
                       | filename_hex_value(decoded[pos + 2]), 1);
                pos += 3;
            }

            if (hexlen == 0 || descriptor_cap == NULL
                    || descriptor_cap == (atalk_iconv_t) -1) {
                errno = hexlen == 0 ? EILSEQ : EINVAL;
                goto escape_error;
            }

            const char *hexbuf = hex;

            if (atalk_iconv(descriptor_cap, &hexbuf, &hexlen, &outbuf,
                            &outleft) == (size_t) -1) {
                goto escape_error;
            }

            continue;
        } else if ((c == ':' || c == '/') && (option & CONV_ESCAPEHEX)) {
            output[0] = ':';
            output[1] = c == ':' ? '3' : '2';
            output[2] = c == ':' ? 'a' : 'f';
            output_count = 3;
        } else if (c == ':' && (to_set == CH_UTF8_MAC || to_set == CH_MAC)) {
            output[0] = '/';
        } else if (c == '/' && (from_set == CH_UTF8_MAC || from_set == CH_MAC)
                   && to_set != CH_UTF8_MAC && to_set != CH_MAC) {
            output[0] = ':';
        } else {
            output[0] = c;
        }

        if (outleft < output_count * sizeof(ucs2_t)) {
            errno = E2BIG;
            goto end;
        }

        memcpy(outbuf, output, output_count * sizeof(ucs2_t));
        outbuf += output_count * sizeof(ucs2_t);
        outleft -= output_count * sizeof(ucs2_t);
        pos++;
    }

    return destlen - outleft;
escape_error:

    if (errno == EILSEQ && (option & CONV_IGNORE)) {
        *flags |= CONV_REQMANGLE;
        return destlen - outleft;
    }

end:
    return option & CONV_FORCE ? destlen - outleft : (size_t) -1;
}

/*!
 * @brief Decode a source string to UCS2, then apply filename punctuation flags
 *
 * Punctuation is scanned as Unicode characters, so wide and multibyte source
 * encodings cannot mistake part of an encoded character for filename syntax.
 * CONV_UNESCAPEHEX decodes :xx bytes through the CAP charset. CONV_ESCAPEHEX
 * writes :3a for a colon and :2f for a slash. Without these options, a colon
 * becomes a slash for a Mac target, and a slash from a Mac source becomes a
 * colon for a non-Mac target. CONV_ESCAPEDOTS escapes a leading dot as :2e.
 *
 * Invalid or incomplete source sequences fail with EILSEQ. CONV_IGNORE
 * returns the decoded prefix and sets CONV_REQMANGLE. CONV__EILSEQ replaces
 * invalid source code units with IGNORE_CHAR. CONV_FORCE accepts a partial
 * result after a decoding or filename-processing error.
 *
 * @param[in]     from_set  source charset
 * @param[in]     to_set    final target charset, which picks punctuation mapping
 * @param[in]     cap_set   charset of the bytes in :xx escapes
 * @param[in]     src       source string
 * @param[in]     srclen    source length in bytes; (size_t)-1 uses strlen plus NUL
 * @param[out]    dest      UCS2 output
 * @param[in]     destlen   size of dest in bytes
 * @param[in,out] flags     CONV_* options and result flags
 * @returns bytes written, or (size_t)-1 with errno
 */
static size_t pull_charset_flags(charset_t from_set, charset_t to_set,
                                 charset_t cap_set, const char *src, size_t srclen,
                                 char *dest, size_t destlen, uint16_t *flags)
{
    const uint16_t option = (flags ? *flags : 0);
    /* :xx escapes can shrink three decoded characters to one output character. */
    ucs2_t decoded[3 * (MAXPATHLEN + 1)];
    size_t decoded_len = (MAXPATHLEN + 1) * sizeof(ucs2_t);
    size_t input_left;

    if (option & CONV_UNESCAPEHEX) {
        decoded_len *= 3;
    }

    size_t count = decode_source_ucs2(from_set, src, srclen, decoded, decoded_len,
                                      &input_left, flags);

    if (count == (size_t) -1) {
        return (size_t) -1;
    }

    return emit_filename_chars(from_set, to_set, cap_set, decoded, count,
                               input_left, dest, destlen, flags);
}

/*!
 * @brief Convert a UCS2 string to a multibyte charset and escape what does not
 * convert, by the flags
 *
 * An unconvertible character stops the call with EILSEQ. CONV_IGNORE then
 * returns the converted part and sets CONV_REQMANGLE. CONV_ESCAPEHEX instead
 * writes the character in the CAP charset as `:xx` escapes and sets
 * CONV_REQESCAPE. CONV_FORCE returns the converted part after any error, and
 * a trailing odd byte fails with EINVAL.
 *
 * @param[in]     to_set   target charset
 * @param[in]     cap_set  charset of the bytes in `:xx` escapes
 * @param[in]     src      UCS2 string
 * @param[in]     srclen   source length in bytes
 * @param[out]    dest     output buffer
 * @param[in]     destlen  size of @p dest
 * @param[in,out] flags    CONV_* options, which can receive CONV_REQESCAPE and
 *                         CONV_REQMANGLE
 * @returns the number of bytes written, or `(size_t)-1` with errno
 */
static size_t push_charset_flags(charset_t to_set, charset_t cap_set, char *src,
                                 size_t srclen, char *dest, size_t destlen, uint16_t *flags)
{
    const uint16_t option = (flags ? *flags : 0);
    size_t i_len, o_len, i;
    size_t j = 0;
    const char *inbuf = (const char *)src;
    char *outbuf = (char *)dest;
    atalk_iconv_t descriptor;
    atalk_iconv_t descriptor_cap;
    descriptor = conv_handles[CH_UCS2][to_set];
    descriptor_cap = conv_handles[CH_UCS2][cap_set];

    if (descriptor == (atalk_iconv_t) -1 || descriptor == (atalk_iconv_t) 0) {
        errno = EINVAL;
        return (size_t) -1;
    }

    i_len = srclen;
    o_len = destlen;

    while (i_len >= 2) {
        while (i_len > 0 &&
                atalk_iconv(descriptor, &inbuf, &i_len, &outbuf, &o_len) == (size_t) -1) {
            if (errno == EILSEQ) {
                if (option & CONV_IGNORE) {
                    *flags |= CONV_REQMANGLE;
                    return destlen - o_len;
                }

                if (option & CONV_ESCAPEHEX) {
                    const size_t bufsiz = o_len / 3 + 1;
                    char *buf = malloc(bufsiz);
                    size_t buflen;

                    if (!buf) {
                        goto end;
                    }

                    i = i_len;

                    for (buflen = 1; buflen <= bufsiz; ++buflen) {
                        char *b = buf;
                        size_t o = buflen;

                        if (atalk_iconv(descriptor_cap, &inbuf, &i, &b, &o) != (size_t) -1) {
                            buflen -= o;
                            break;
                        } else if (errno != E2BIG) {
                            SAFE_FREE(buf);
                            goto end;
                        } else if (o < buflen) {
                            buflen -= o;
                            break;
                        }
                    }

                    if (o_len < buflen * 3) {
                        SAFE_FREE(buf);
                        errno = E2BIG;
                        goto end;
                    }

                    o_len -= buflen * 3;
                    i_len = i;

                    for (i = 0; i < buflen; ++i) {
                        *outbuf++ = ':';
                        *outbuf++ = hexdig[(buf[i] >> 4) & 0x0f];
                        *outbuf++ = hexdig[buf[i] & 0x0f];
                    }

                    SAFE_FREE(buf);
                    *flags |= CONV_REQESCAPE;
                    continue;
                }
            }

            goto end;
        }
    } /* while (i_len >= 2) */

    if (i_len > 0) {
        errno = EINVAL;
    }

end:
    return (i_len + j == 0
            || (option & CONV_FORCE)) ? destlen - o_len : (size_t) -1;
}

/*!
 * @brief Convert a string between character sets, with case, composition, and
 * escaping chosen by flags
 *
 * convert_charset pulls the source into UCS-2 and precomposes or decomposes it
 * as the flags and the target charset require. It then folds case for
 * CONV_TOUPPER or CONV_TOLOWER and pushes the result into the target charset.
 *
 * @bug Buffer sizing is fragile and wants malloc/free logic.
 * @note dest_len must include room for the two-byte terminator.
 *
 * @param[in]     from_set     charset of src
 * @param[in]     to_set       charset of dest
 * @param[in]     cap_charset  charset used for the :XX hex escaping of
 *                             unmappable characters
 * @param[in]     src          source string
 * @param[in]     src_len      length of src in bytes
 * @param[out]    dest         destination buffer
 * @param[in]     dest_len     size of dest in bytes, terminator included
 * @param[in,out] flags        CONV_* options on entry, conversion results on
 *                             return
 * @returns the length of dest in bytes, or (size_t)-1 with errno set
 */
size_t convert_charset(charset_t from_set, charset_t to_set,
                       charset_t cap_charset, const char *src, size_t src_len, char *dest,
                       size_t dest_len, uint16_t *flags)
{
    size_t i_len, o_len;
    size_t dest_capacity;
    ucs2_t *u;
    ucs2_t buffer[MAXPATHLEN + 2];
    ucs2_t buffer2[MAXPATHLEN + 2];
    size_t u_size;
    lazy_initialize_conv();
    /* convert from_set to UCS2 */
    errno = 0;

    if ((size_t)(-1) == (o_len = pull_charset_flags(from_set, to_set, cap_charset,
                                                    src, src_len,
                                                    (char *) buffer, sizeof(buffer) - 2, flags))) {
        LOG(log_error, logtype_default,
            "Conversion failed ( %s to CH_UCS2 ), errno %i, %.*s", charset_name(from_set),
            errno, src_len > MAXPATHLEN ? MAXPATHLEN : (int)src_len, src);
        return (size_t) -1;
    }

    if (dest_len < 2) {
        errno = E2BIG;
        return (size_t) -1;
    }

    if (o_len == 0) {
        dest[0] = 0;
        dest[1] = 0;
        return o_len;
    }

    /* Do pre/decomposition */
    i_len = sizeof(buffer2) - 2;
    u = buffer2;

    if (CHECK_FLAGS(flags, CONV_DECOMPOSE) || (charsets[to_set]
                                               && (charsets[to_set]->flags & CHARSET_DECOMPOSED))) {
        if ((size_t) -1 == (i_len = decompose_w(buffer, o_len, u, &i_len))) {
            return (size_t) -1;
        }
    } else if (CHECK_FLAGS(flags, CONV_PRECOMPOSE) || !charsets[from_set]
               || (charsets[from_set]->flags & CHARSET_DECOMPOSED)) {
        if ((size_t) -1 == (i_len = precompose_w(buffer, o_len, u, &i_len))) {
            return (size_t) -1;
        }
    } else {
        u = buffer;
        i_len = o_len;
    }

    /* null terminate */
    u_size = (u == buffer2) ? sizeof(buffer2) : sizeof(buffer);

    if (i_len > u_size - 2) {
        errno = E2BIG;
        return (size_t) -1;
    }

    ((char *)u)[i_len] = 0;
    ((char *)u)[i_len + 1] = 0;

    /* Do case conversions */
    if (CHECK_FLAGS(flags, CONV_TOUPPER)) {
        strupper_w(u);
    } else if (CHECK_FLAGS(flags, CONV_TOLOWER)) {
        strlower_w(u);
    }

    dest_capacity = dest_len - 2;

    /* Convert UCS2 to to_set */
    if ((size_t)(-1) == (o_len = push_charset_flags(to_set, cap_charset, (char *)u,
                                                    i_len, dest, dest_capacity, flags))) {
        LOG(log_error, logtype_default,
            "Conversion failed (CH_UCS2 to %s):%s", charset_name(to_set), strerror(errno));
        return (size_t) -1;
    }

    /* null terminate */
    dest[o_len] = 0;
    dest[o_len + 1] = 0;
    return o_len;
}
