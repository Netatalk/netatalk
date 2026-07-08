/*
  Copyright (c) 2012 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <math.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <talloc.h>

#include <atalk/byteorder.h>
#include <atalk/dalloc.h>
#include <atalk/dsi.h>
#include <atalk/errchk.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/spotlight.h>
#include <atalk/util.h>
#include <atalk/volume.h>

/* If you change this constant, also update SL_PACK_BUFLEN in
 * test/testsuite/afpcmd_spotlight.c — that file deliberately
 * cannot #include <atalk/dsi.h> and therefore hard-codes 65472. */
#define MAX_SLQ_DAT (DSI_DATASIZ - 64)
#define MAX_SLQ_TOC 8192

/**************************************************************************************************
 * RPC data marshalling and unmarshalling
 **************************************************************************************************/

/* Spotlight epoch is 1.1.2001 00:00 UTC */
#define SPOTLIGHT_TIME_DELTA 978307200 /* Diff from UNIX epoch to Spotlight epoch */

#define SQ_TYPE_NULL    0x0000
#define SQ_TYPE_COMPLEX 0x0200
#define SQ_TYPE_INT64   0x8400
#define SQ_TYPE_BOOL    0x0100
#define SQ_TYPE_FLOAT   0x8500
#define SQ_TYPE_DATA    0x0700
#define SQ_TYPE_CNIDS   0x8700
#define SQ_TYPE_UUID    0x0e00
#define SQ_TYPE_DATE    0x8600
#define SQ_TYPE_TOC     0x8800

#define SQ_CPX_TYPE_ARRAY           0x0a00
#define SQ_CPX_TYPE_STRING          0x0c00
#define SQ_CPX_TYPE_UTF16_STRING    0x1c00
#define SQ_CPX_TYPE_DICT            0x0d00
#define SQ_CPX_TYPE_CNIDS           0x1a00
#define SQ_CPX_TYPE_FILEMETA        0x1b00

#define SUBQ_SAFETY_LIM 20

/* Forward declarations */
static int sl_pack_loop(DALLOC_CTX *query, char *buf, int offset, char *toc_buf,
                        int *toc_idx);
static int sl_unpack_loop(DALLOC_CTX *query, const char *buf, int offset,
                          uint count, size_t buf_len, const uint toc_offset,
                          uint toc_count, const uint encoding, int depth);
static int sl_unpack_cpx(DALLOC_CTX *query, const char *buf, const int offset,
                         uint cpx_query_type, uint cpx_query_count,
                         size_t buf_len, const uint toc_offset, uint toc_count,
                         const uint encoding, int depth);
static int sl_unpack_r(DALLOC_CTX *query, const char *buf, size_t buf_len,
                       int depth);

/**************************************************************************************************
 * Wrapper functions for the *VAL macros with bound checking
 **************************************************************************************************/

static void sl_put_le32(char *buf, off_t off, uint32_t val)
{
    uint8_t *p = (uint8_t *)buf + off;
    p[0] = (uint8_t)val;
    p[1] = (uint8_t)(val >> 8);
    p[2] = (uint8_t)(val >> 16);
    p[3] = (uint8_t)(val >> 24);
}

static void sl_put_le64(char *buf, off_t off, uint64_t val)
{
    sl_put_le32(buf, off, (uint32_t)val);
    sl_put_le32(buf, off + 4, (uint32_t)(val >> 32));
}

static uint64_t sl_get_le64(const char *buf, int offset)
{
    const uint8_t *p = (const uint8_t *)buf + offset;
    return (uint64_t)p[0]
           | ((uint64_t)p[1] << 8)
           | ((uint64_t)p[2] << 16)
           | ((uint64_t)p[3] << 24)
           | ((uint64_t)p[4] << 32)
           | ((uint64_t)p[5] << 40)
           | ((uint64_t)p[6] << 48)
           | ((uint64_t)p[7] << 56);
}

static uint64_t sl_get_be64(const char *buf, int offset)
{
    const uint8_t *p = (const uint8_t *)buf + offset;
    return (uint64_t)p[7]
           | ((uint64_t)p[6] << 8)
           | ((uint64_t)p[5] << 16)
           | ((uint64_t)p[4] << 24)
           | ((uint64_t)p[3] << 32)
           | ((uint64_t)p[2] << 40)
           | ((uint64_t)p[1] << 48)
           | ((uint64_t)p[0] << 56);
}

static int sivalc(char *buf, off_t off, off_t maxoff, uint32_t val)
{
    if (off + sizeof(val) >= maxoff) {
        LOG(log_error, logtype_sl, "sivalc: off: %zd, maxoff: %zd", off, maxoff);
        return -1;
    }

    /*
     * sl_pack() emits the "432130dm" Spotlight payload variant, whose
     * numeric fields are little-endian on the wire. Do not use Netatalk's
     * SIVAL/SLVAL here: in this tree they write host order, not fixed
     * little-endian, and that breaks Spotlight RPC on big-endian systems.
     */
    sl_put_le32(buf, off, val);
    return 0;
}

static int slvalc(char *buf, off_t off, off_t maxoff, uint64_t val)
{
    if (off + sizeof(val) >= maxoff) {
        LOG(log_error, logtype_sl, "slvalc: off: %zd, maxoff: %zd", off, maxoff);
        return -1;
    }

    sl_put_le64(buf, off, val);
    return 0;
}

/*!
 * @brief Returns the UTF-16 string encoding, by checking the 2-byte byte order mark.
 * @returns If there is no byte order mark, -1 is returned.
 */
static uint spotlight_get_utf16_string_encoding(const char *buf, int offset,
        int query_length, uint encoding)
{
    uint utf16_encoding;
    /* Assumed encoding in absence of a bom is little endian */
    utf16_encoding = SL_ENC_LITTLE_ENDIAN;

    if (query_length >= 2) {
        uint8_t le_bom[] = {0xff, 0xfe};
        uint8_t be_bom[] = {0xfe, 0xff};

        if (memcmp(le_bom, buf + offset, sizeof(uint16_t)) == 0) {
            utf16_encoding = SL_ENC_LITTLE_ENDIAN | SL_ENC_UTF_16;
        } else if (memcmp(be_bom, buf + offset, sizeof(uint16_t)) == 0) {
            utf16_encoding = SL_ENC_BIG_ENDIAN | SL_ENC_UTF_16;
        }
    }

    return utf16_encoding;
}

/**************************************************************************************************
 * marshalling functions
 **************************************************************************************************/

#define SL_OFFSET_DELTA 16

static uint64_t sl_pack_tag(uint16_t type, uint16_t size_or_count, uint32_t val)
{
    uint64_t tag = ((uint64_t)val << 32) | ((uint64_t)type << 16) | size_or_count;
    return tag;
}

static int sl_pack_float(double d, char *buf, int offset)
{
    EC_INIT;
    union {
        double d;
        uint64_t w;
    } ieee_fp_union;
    ieee_fp_union.d = d;
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_FLOAT, 2, 1)));
    EC_ZERO(slvalc(buf, offset + 8, MAX_SLQ_DAT, ieee_fp_union.w));
EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset + 2 * sizeof(uint64_t);
}

static int sl_pack_uint64(uint64_t u, char *buf, int offset)
{
    EC_INIT;
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_INT64, 2, 1)));
    EC_ZERO(slvalc(buf, offset + 8, MAX_SLQ_DAT, u));
EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset + 2 * sizeof(uint64_t);
}

static int sl_pack_bool(sl_bool_t bl, char *buf, int offset)
{
    EC_INIT;
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_BOOL, 1,
            bl ? 1 : 0)));
EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset + sizeof(uint64_t);
}

static int sl_pack_nil(char *buf, int offset)
{
    EC_INIT;
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_NULL, 1, 1)));
EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset + sizeof(uint64_t);
}

static int sl_pack_date(sl_time_t t, char *buf, int offset)
{
    EC_INIT;
    uint64_t data = 0;
    union {
        double d;
        uint64_t w;
    } ieee_fp_union;
    ieee_fp_union.d = (double)(t.tv_sec - SPOTLIGHT_TIME_DELTA);
    ieee_fp_union.d += (double)t.tv_usec / 1000000;
    data = ieee_fp_union.w;
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_DATE, 2, 1)));
    EC_ZERO(slvalc(buf, offset + 8, MAX_SLQ_DAT, data));
EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset + 2 * sizeof(uint64_t);
}

static int sl_pack_uuid(sl_uuid_t *uuid, char *buf, int offset)
{
    EC_INIT;
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_UUID, 3, 1)));

    if (offset + 8 + 16 >= MAX_SLQ_DAT) {
        EC_FAIL;
    }

    memcpy(buf + offset + 8, uuid, 16);
EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset + sizeof(uint64_t) + 16;
}

static int sl_pack_CNID(sl_cnids_t *cnids, char *buf, int offset, char *toc_buf,
                        int *toc_idx)
{
    EC_INIT;
    int len;
    int cnid_count = talloc_array_length(cnids->ca_cnids->dd_talloc_array);
    uint64_t id;
    EC_ZERO(slvalc(toc_buf, *toc_idx * 8, MAX_SLQ_TOC,
                   sl_pack_tag(SQ_CPX_TYPE_CNIDS, (offset + SL_OFFSET_DELTA) / 8, 0)));
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_COMPLEX, 1,
            *toc_idx + 1)));
    *toc_idx += 1;
    offset += 8;
    len = cnid_count + 1;

    if (cnid_count > 0) {
        len ++;
    }

    /* 3rd parameter of sl_pack_tag has unknown meaning, but always 8 */
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_CNIDS, len, 8)));
    offset += 8;

    if (cnid_count > 0) {
        EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(cnids->ca_unkn1,
                cnid_count, cnids->ca_context)));
        offset += 8;

        for (int i = 0; i < cnid_count; i++) {
            memcpy(&id, cnids->ca_cnids->dd_talloc_array[i], sizeof(uint64_t));
            EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, id));
            offset += 8;
        }
    }

EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset;
}

static int sl_pack_array(sl_array_t *array, char *buf, int offset,
                         char *toc_buf, int *toc_idx)
{
    EC_INIT;
    int count = talloc_array_length(array->dd_talloc_array);
    int octets = (offset + SL_OFFSET_DELTA) / 8;
    EC_ZERO(slvalc(toc_buf, *toc_idx * 8, MAX_SLQ_TOC,
                   sl_pack_tag(SQ_CPX_TYPE_ARRAY, octets, count)));
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_COMPLEX, 1,
            *toc_idx + 1)));
    *toc_idx += 1;
    offset += 8;
    EC_NEG1(offset = sl_pack_loop(array, buf, offset, toc_buf, toc_idx));
EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset;
}

static int sl_pack_dict(sl_array_t *dict, char *buf, int offset, char *toc_buf,
                        int *toc_idx)
{
    EC_INIT;
    EC_ZERO(slvalc(toc_buf,
                   *toc_idx * 8,
                   MAX_SLQ_TOC,
                   sl_pack_tag(SQ_CPX_TYPE_DICT,
                               (offset + SL_OFFSET_DELTA) / 8,
                               talloc_array_length(dict->dd_talloc_array))));
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_COMPLEX, 1,
            *toc_idx + 1)));
    *toc_idx += 1;
    offset += 8;
    EC_NEG1(offset = sl_pack_loop(dict, buf, offset, toc_buf, toc_idx));
EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset;
}

static int sl_pack_filemeta(sl_filemeta_t *fm, char *buf, int offset,
                            char *toc_buf, int *toc_idx)
{
    EC_INIT;
    int fmlen;                  /* lenght of filemeta */
    int saveoff = offset;
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_COMPLEX, 1,
            *toc_idx + 1)));
    offset += 16;
    EC_NEG1(fmlen = sl_pack(fm, buf + offset));
    /* Check for empty filemeta array, if it's only 40 bytes, it's only the header but no content */
    LOG(log_debug, logtype_sl, "fmlen: %d", fmlen);

    if (fmlen > 40) {
        offset += fmlen;
    } else {
        fmlen = 0;
    }

    /* 3rd parameter of sl_pack_tag has unknown meaning, but always 8 */
    EC_ZERO(slvalc(buf, saveoff + 8, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_DATA,
            (fmlen / 8) + 1, 8)));
    EC_ZERO(slvalc(toc_buf, *toc_idx * 8, MAX_SLQ_TOC,
                   sl_pack_tag(SQ_CPX_TYPE_FILEMETA, (saveoff + SL_OFFSET_DELTA) / 8, fmlen / 8)));
    *toc_idx += 1;
EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset;
}

static int sl_pack_string(char *s, char *buf, int offset, char *toc_buf,
                          int *toc_idx)
{
    EC_INIT;
    int len, octets, used_in_last_octet;
    size_t alloc_size = talloc_get_size(s);
    len = (int)strnlen(s, alloc_size);

    if (alloc_size == 0 || (size_t)len == alloc_size) {
        EC_FAIL;
    }

    octets = (len / 8) + (len & 7 ? 1 : 0);
    used_in_last_octet = 8 - (octets * 8 - len);
    EC_ZERO(slvalc(toc_buf, *toc_idx * 8, MAX_SLQ_TOC,
                   sl_pack_tag(SQ_CPX_TYPE_STRING, (offset + SL_OFFSET_DELTA) / 8,
                               used_in_last_octet)));
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_COMPLEX, 1,
            *toc_idx + 1)));
    *toc_idx += 1;
    offset += 8;
    EC_ZERO(slvalc(buf, offset, MAX_SLQ_DAT, sl_pack_tag(SQ_TYPE_DATA, octets + 1,
            used_in_last_octet)));
    offset += 8;

    if (offset + octets * 8 > MAX_SLQ_DAT) {
        EC_FAIL;
    }

    memset(buf + offset, 0, octets * 8);
    strncpy(buf + offset, s, len);
    offset += octets * 8;
EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset;
}

static int sl_pack_loop(DALLOC_CTX *query, char *buf, int offset, char *toc_buf,
                        int *toc_idx)
{
    EC_INIT;
    const char *type;

    for (int n = 0; n < talloc_array_length(query->dd_talloc_array); n++) {
        type = talloc_get_name(query->dd_talloc_array[n]);

        if (STRCMP(type, ==, "sl_array_t")) {
            EC_NEG1(offset = sl_pack_array(query->dd_talloc_array[n], buf, offset, toc_buf,
                                           toc_idx));
        } else if (STRCMP(type, ==, "sl_dict_t")) {
            EC_NEG1(offset = sl_pack_dict(query->dd_talloc_array[n], buf, offset, toc_buf,
                                          toc_idx));
        } else if (STRCMP(type, ==, "sl_filemeta_t")) {
            EC_NEG1(offset = sl_pack_filemeta(query->dd_talloc_array[n], buf, offset,
                                              toc_buf, toc_idx));
        } else if (STRCMP(type, ==, "uint64_t")) {
            uint64_t i;
            memcpy(&i, query->dd_talloc_array[n], sizeof(uint64_t));
            EC_NEG1(offset = sl_pack_uint64(i, buf, offset));
        } else if (STRCMP(type, ==, "char *")) {
            EC_NEG1(offset = sl_pack_string(query->dd_talloc_array[n], buf, offset, toc_buf,
                                            toc_idx));
        } else if (STRCMP(type, ==, "sl_bool_t")) {
            sl_bool_t bl;
            memcpy(&bl, query->dd_talloc_array[n], sizeof(sl_bool_t));
            EC_NEG1(offset = sl_pack_bool(bl, buf, offset));
        } else if (STRCMP(type, ==, "double")) {
            double d;
            memcpy(&d, query->dd_talloc_array[n], sizeof(double));
            EC_NEG1(offset = sl_pack_float(d, buf, offset));
        } else if (STRCMP(type, ==, "sl_nil_t")) {
            EC_NEG1(offset = sl_pack_nil(buf, offset));
        } else if (STRCMP(type, ==, "sl_time_t")) {
            sl_time_t t;
            memcpy(&t, query->dd_talloc_array[n], sizeof(sl_time_t));
            EC_NEG1(offset = sl_pack_date(t, buf, offset));
        } else if (STRCMP(type, ==, "sl_uuid_t")) {
            EC_NEG1(offset = sl_pack_uuid(query->dd_talloc_array[n], buf, offset));
        } else if (STRCMP(type, ==, "sl_cnids_t")) {
            EC_NEG1(offset = sl_pack_CNID(query->dd_talloc_array[n], buf, offset, toc_buf,
                                          toc_idx));
        }
    }

EC_CLEANUP:

    if (ret != 0) {
        return -1;
    }

    return offset;
}

/**************************************************************************************************
 * unmarshalling functions
 **************************************************************************************************/

static bool sl_unpack_range_valid(size_t buf_len, int offset, size_t length)
{
    return offset >= 0
           && (size_t)offset <= buf_len
           && length <= buf_len - (size_t)offset;
}

static uint64_t sl_unpack_uint64(const char *buf, int offset, uint encoding)
{
    if (encoding == SL_ENC_LITTLE_ENDIAN) {
        return sl_get_le64(buf, offset);
    } else {
        return sl_get_be64(buf, offset);
    }
}

static int sl_unpack_uint64_checked(const char *buf, size_t buf_len, int offset,
                                    uint encoding, uint64_t *value)
{
    if (!sl_unpack_range_valid(buf_len, offset, sizeof(uint64_t))) {
        return -1;
    }

    *value = sl_unpack_uint64(buf, offset, encoding);
    return 0;
}

static int sl_unpack_count(uint64_t query_data64, int query_length,
                           size_t element_size, int *count)
{
    uint64_t count64;

    if (query_length < (int)sizeof(uint64_t)) {
        return -1;
    }

    count64 = query_data64 >> 32;

    if (count64 > INT_MAX
            || count64 > (uint64_t)((query_length - (int)sizeof(uint64_t))
                                    / (int)element_size)) {
        LOG(log_error, logtype_sl,
            "Spotlight unmarshalling rejected element count: "
            "count=%" PRIu64 ", query_length=%d, element_size=%zu",
            count64, query_length, element_size);
        return -1;
    }

    *count = (int)count64;
    return 0;
}

static int sl_unpack_ints(DALLOC_CTX *query, const char *buf, int offset,
                          int query_length, size_t buf_len, uint encoding)
{
    int count, i;
    uint64_t query_data64;

    if (sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                 &query_data64) < 0
            || sl_unpack_count(query_data64, query_length, sizeof(uint64_t),
                               &count) < 0) {
        return -1;
    }

    offset += 8;
    i = 0;

    while (i++ < count) {
        if (sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                     &query_data64) < 0) {
            return -1;
        }

        dalloc_add_copy(query, &query_data64, uint64_t);
        offset += 8;
    }

    return count;
}

static int sl_unpack_date(DALLOC_CTX *query, const char *buf, int offset,
                          int query_length, size_t buf_len, uint encoding)
{
    int count, i;
    uint64_t query_data64;
    union {
        double d;
        uint64_t w;
    } ieee_fp_union;
    double unix_time;
    sl_time_t t;

    if (sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                 &query_data64) < 0
            || sl_unpack_count(query_data64, query_length, sizeof(uint64_t),
                               &count) < 0) {
        return -1;
    }

    offset += 8;
    i = 0;

    while (i++ < count) {
        if (sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                     &query_data64) < 0) {
            return -1;
        }

        ieee_fp_union.w = query_data64;
        unix_time = ieee_fp_union.d + SPOTLIGHT_TIME_DELTA;
        t = (sl_time_t) {
            .tv_sec  = (time_t)floor(unix_time),
            .tv_usec = (suseconds_t)((unix_time - floor(unix_time)) * 1000000)
        };
        dalloc_add_copy(query, &t, sl_time_t);
        offset += 8;
    }

    return count;
}

static int sl_unpack_uuid(DALLOC_CTX *query, const char *buf, int offset,
                          int query_length, size_t buf_len, uint encoding)
{
    int count, i;
    uint64_t query_data64;
    sl_uuid_t uuid;

    if (sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                 &query_data64) < 0
            || sl_unpack_count(query_data64, query_length, sizeof(uuid),
                               &count) < 0) {
        return -1;
    }

    offset += 8;
    i = 0;

    while (i++ < count) {
        if (!sl_unpack_range_valid(buf_len, offset, sizeof(uuid))) {
            return -1;
        }

        memcpy(uuid.sl_uuid, buf + offset, 16);
        dalloc_add_copy(query, &uuid, sl_uuid_t);
        offset += 16;
    }

    return count;
}

static int sl_unpack_floats(DALLOC_CTX *query, const char *buf, int offset,
                            int query_length, size_t buf_len, uint encoding)
{
    int count, i;
    uint64_t query_data64;
    union {
        double d;
        uint64_t w;
    } ieee_fp_union;

    if (sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                 &query_data64) < 0
            || sl_unpack_count(query_data64, query_length, sizeof(uint64_t),
                               &count) < 0) {
        return -1;
    }

    offset += 8;
    i = 0;

    while (i++ < count) {
        if (sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                     &ieee_fp_union.w) < 0) {
            return -1;
        }

        dalloc_add_copy(query, &ieee_fp_union.d, double);
        offset += 8;
    }

    return count;
}

static int sl_unpack_CNID(DALLOC_CTX *query, const char *buf, int offset,
                          int length, size_t buf_len, uint encoding)
{
    EC_INIT;
    int count;
    uint64_t query_data64;
    sl_cnids_t *cnids;
    EC_NULL(cnids = talloc_zero(query, sl_cnids_t));
    EC_NULL(cnids->ca_cnids = talloc_zero(cnids, DALLOC_CTX));

    if (length <= 16)
        /* that's permitted, it's an empty array */
    {
        goto EC_CLEANUP;
    }

    if (!sl_unpack_range_valid(buf_len, offset, (size_t)(length - 8))
            || sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                        &query_data64) < 0) {
        EC_FAIL;
    }

    count = query_data64 & 0xffff;

    if (count > (length - 2 * (int)sizeof(uint64_t)) / (int)sizeof(uint64_t)) {
        EC_FAIL;
    }

    cnids->ca_unkn1 = (query_data64 & 0xffff0000) >> 16;
    cnids->ca_context = query_data64 >> 32;
    offset += 8;

    while (count --) {
        EC_NEG1(sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                         &query_data64));
        dalloc_add_copy(cnids->ca_cnids, &query_data64, uint64_t);
        offset += 8;
    }

    dalloc_add(query, cnids, sl_cnids_t);
EC_CLEANUP:
    EC_EXIT;
}

static const char *spotlight_get_qtype_string(uint64_t query_type)
{
    switch (query_type) {
    case SQ_TYPE_NULL:
        return "null";

    case SQ_TYPE_COMPLEX:
        return "complex";

    case SQ_TYPE_INT64:
        return "int64";

    case SQ_TYPE_BOOL:
        return "bool";

    case SQ_TYPE_FLOAT:
        return "float";

    case SQ_TYPE_DATA:
        return "data";

    case SQ_TYPE_CNIDS:
        return "CNIDs";

    default:
        return "unknown";
    }
}

static const char *spotlight_get_cpx_qtype_string(uint64_t cpx_query_type)
{
    switch (cpx_query_type) {
    case SQ_CPX_TYPE_ARRAY:
        return "array";

    case SQ_CPX_TYPE_STRING:
        return "string";

    case SQ_CPX_TYPE_UTF16_STRING:
        return "utf-16 string";

    case SQ_CPX_TYPE_DICT:
        return "dictionary";

    case SQ_CPX_TYPE_CNIDS:
        return "CNIDs";

    case SQ_CPX_TYPE_FILEMETA:
        return "FileMeta";

    default:
        return "unknown";
    }
}

static int sl_unpack_cpx(DALLOC_CTX *query,
                         const char *buf,
                         const int offset,
                         uint cpx_query_type,
                         uint cpx_query_count,
                         size_t buf_len,
                         const uint toc_offset,
                         uint toc_count,
                         const uint encoding,
                         int depth)
{
    EC_INIT;

    if (depth >= SUBQ_SAFETY_LIM) {
        EC_FAIL;
    }

    int roffset = offset;
    uint64_t query_data64, used_in_last_block64;
    uint unicode_encoding;
    uint8_t mark_exists;
    char *p, *tmp;
    int qlen, used_in_last_block, slen;
    sl_array_t *sl_array;
    sl_dict_t *sl_dict;
    sl_filemeta_t *sl_fm;

    switch (cpx_query_type) {
    case SQ_CPX_TYPE_ARRAY:
        sl_array = talloc_zero(query, sl_array_t);
        EC_NEG1_LOG(roffset = sl_unpack_loop(sl_array, buf, offset, cpx_query_count,
                                             buf_len, toc_offset, toc_count,
                                             encoding, depth + 1));
        dalloc_add(query, sl_array, sl_array_t);
        break;

    case SQ_CPX_TYPE_DICT:
        sl_dict = talloc_zero(query, sl_dict_t);
        EC_NEG1_LOG(roffset = sl_unpack_loop(sl_dict, buf, offset, cpx_query_count,
                                             buf_len, toc_offset, toc_count,
                                             encoding, depth + 1));
        dalloc_add(query, sl_dict, sl_dict_t);
        break;

    case SQ_CPX_TYPE_STRING:
    case SQ_CPX_TYPE_UTF16_STRING:
        EC_NEG1(sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                         &query_data64));
        qlen = (query_data64 & 0xffff) * 8;
        used_in_last_block64 = query_data64 >> 32;

        if (used_in_last_block64 > 8) {
            EC_FAIL;
        }

        used_in_last_block = (int)used_in_last_block64;
        slen = qlen - 16 + used_in_last_block;

        if (qlen <= 8 || !sl_unpack_range_valid(buf_len, offset, qlen)) {
            EC_FAIL;
        }

        if (cpx_query_type == SQ_CPX_TYPE_STRING) {
            if (slen < 0 || offset + 8 + slen > (int)toc_offset) {
                EC_FAIL;
            }

            p = dalloc_strndup(query, buf + offset + 8, slen);
        } else {
            unicode_encoding = spotlight_get_utf16_string_encoding(buf, offset + 8, slen,
                encoding);
            mark_exists = (unicode_encoding & SL_ENC_UTF_16);

            if (unicode_encoding & SL_ENC_BIG_ENDIAN) {
                EC_FAIL_LOG("Unsupported big endian UTF16 string");
            }

            slen -= mark_exists ? 2 : 0;

            if (slen <= 0 || offset + (mark_exists ? 10 : 8) + slen > (int)toc_offset) {
                EC_FAIL;
            }

            size_t tmp_len;
            EC_NEG1(tmp_len = convert_string_allocate(CH_UCS2,
                CH_UTF8,
                buf + offset + (mark_exists ? 10 : 8),
                slen,
                &tmp));
            p = dalloc_strndup(query, tmp, tmp_len);
            free(tmp);
        }

        dalloc_add(query, p, char *);
        roffset += qlen;
        break;

    case SQ_CPX_TYPE_FILEMETA:
        EC_NEG1(sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                         &query_data64));
        qlen = (query_data64 & 0xffff) * 8;

        if (qlen <= 8 || !sl_unpack_range_valid(buf_len, offset, qlen)) {
            EC_FAIL_LOG("SQ_CPX_TYPE_FILEMETA: query_length <= 8: %d", qlen);
        } else {
            sl_fm = talloc_zero(query, sl_filemeta_t);
            EC_NEG1_LOG(sl_unpack_r(sl_fm, buf + offset + 8, qlen - 8,
                                    depth + 1));
            dalloc_add(query, sl_fm, sl_filemeta_t);
        }

        roffset += qlen;
        break;

    case SQ_CPX_TYPE_CNIDS:
        EC_NEG1(sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                         &query_data64));
        qlen = (query_data64 & 0xffff) * 8;

        if (qlen <= 8 || !sl_unpack_range_valid(buf_len, offset, qlen)) {
            EC_FAIL;
        }

        EC_NEG1_LOG(sl_unpack_CNID(query, buf, offset + 8, qlen, buf_len,
                                   encoding));
        roffset += qlen;
        break;

    default:
        EC_FAIL;
    }

EC_CLEANUP:

    if (ret != 0) {
        roffset = -1;
    }

    return roffset;
}

static int sl_unpack_loop(DALLOC_CTX *query,
                          const char *buf,
                          int offset,
                          uint count,
                          size_t buf_len,
                          const uint toc_offset,
                          uint toc_count,
                          const uint encoding,
                          int depth)
{
    EC_INIT;

    if (depth >= SUBQ_SAFETY_LIM) {
        EC_FAIL;
    }

    int i, query_length;
    uint subcount;
    uint64_t query_data64, query_type;
    uint64_t toc_index64;
    size_t toc_index;
    uint cpx_query_type, cpx_query_count;
    sl_nil_t nil;
    sl_bool_t b;

    while (count > 0 && (offset < toc_offset)) {
        EC_NEG1(sl_unpack_uint64_checked(buf, buf_len, offset, encoding,
                                         &query_data64));
        query_length = (query_data64 & 0xffff) * 8;
        query_type = (query_data64 & 0xffff0000) >> 16;

        if (query_length == 0) {
            EC_FAIL;
        }

        if (!sl_unpack_range_valid(buf_len, offset, (size_t)query_length)) {
            EC_FAIL;
        }

        switch (query_type) {
        case SQ_TYPE_COMPLEX:
            toc_index64 = query_data64 >> 32;

            if (toc_index64 == 0 || toc_index64 > toc_count) {
                LOG(log_error, logtype_sl,
                    "Spotlight unmarshalling rejected TOC index: "
                    "index=%" PRIu64 ", toc_count=%u",
                    toc_index64, toc_count);
                EC_FAIL;
            }

            toc_index = (size_t)(toc_index64 - 1);

            if (toc_index > (SIZE_MAX - toc_offset) / sizeof(uint64_t)
                    || toc_offset + toc_index * sizeof(uint64_t) > INT_MAX
                    || !sl_unpack_range_valid(buf_len,
                                              (int)(toc_offset
                                                    + toc_index * sizeof(uint64_t)),
                                              sizeof(uint64_t))) {
                LOG(log_error, logtype_sl,
                    "Spotlight unmarshalling rejected TOC offset: "
                    "index=%zu, toc_offset=%u, buf_len=%zu",
                    toc_index, toc_offset, buf_len);
                EC_FAIL;
            }

            query_data64 = sl_unpack_uint64(buf,
                                            (int)(toc_offset
                                                  + toc_index * sizeof(uint64_t)),
                                            encoding);
            cpx_query_type = (query_data64 & 0xffff0000) >> 16;
            cpx_query_count = query_data64 >> 32;
            EC_NEG1_LOG(offset = sl_unpack_cpx(query, buf, offset + 8, cpx_query_type,
                                               cpx_query_count, buf_len, toc_offset,
                                               toc_count, encoding, depth + 1));
            count--;
            break;

        case SQ_TYPE_NULL:
            subcount = query_data64 >> 32;

            if (subcount > 64) {
                EC_FAIL;
            }

            nil = 0;

            for (i = 0; i < subcount; i++) {
                dalloc_add_copy(query, &nil, sl_nil_t);
            }

            if (subcount > count) {
                EC_FAIL;
            }

            offset += query_length;
            count -= subcount;
            break;

        case SQ_TYPE_BOOL:
            b = query_data64 >> 32;
            dalloc_add_copy(query, &b, sl_bool_t);
            offset += query_length;
            count--;
            break;

        case SQ_TYPE_INT64:
            EC_NEG1_LOG(subcount = sl_unpack_ints(query, buf, offset, query_length,
                                                  buf_len, encoding));

            if (subcount > count) {
                EC_FAIL;
            }

            offset += query_length;
            count -= subcount;
            break;

        case SQ_TYPE_UUID:
            EC_NEG1_LOG(subcount = sl_unpack_uuid(query, buf, offset, query_length,
                                                  buf_len, encoding));

            if (subcount > count) {
                EC_FAIL;
            }

            offset += query_length;
            count -= subcount;
            break;

        case SQ_TYPE_FLOAT:
            EC_NEG1_LOG(subcount = sl_unpack_floats(query, buf, offset,
                                                    query_length, buf_len,
                                                    encoding));

            if (subcount > count) {
                EC_FAIL;
            }

            offset += query_length;
            count -= subcount;
            break;

        case SQ_TYPE_DATE:
            EC_NEG1_LOG(subcount = sl_unpack_date(query, buf, offset, query_length,
                                                  buf_len, encoding));

            if (subcount > count) {
                EC_FAIL;
            }

            offset += query_length;
            count -= subcount;
            break;

        default:
            EC_FAIL;
        }
    }

EC_CLEANUP:

    if (ret != 0) {
        offset = -1;
    }

    return offset;
}

/**************************************************************************************************
 * Global functions for packing und unpacking
 **************************************************************************************************/

int sl_pack(DALLOC_CTX *query, char *buf)
{
    EC_INIT;
    char toc_buf[MAX_SLQ_TOC];
    int toc_index = 0;
    int len = 0;
    memcpy(buf, "432130dm", 8);
    EC_NEG1_LOG(len = sl_pack_loop(query, buf + 16, 0, toc_buf + 8, &toc_index));
    EC_ZERO(sivalc(buf, 8, MAX_SLQ_DAT, len / 8 + 1 + toc_index + 1));
    EC_ZERO(sivalc(buf, 12, MAX_SLQ_DAT, len / 8 + 1));
    EC_ZERO(slvalc(toc_buf, 0, MAX_SLQ_TOC, sl_pack_tag(SQ_TYPE_TOC, toc_index + 1,
            0)));

    if ((16 + len + ((toc_index + 1) * 8)) >= MAX_SLQ_DAT) {
        EC_FAIL;
    }

    memcpy(buf + 16 + len, toc_buf, (toc_index + 1) * 8);
    len += 16 + (toc_index + 1) * 8;
EC_CLEANUP:

    if (ret != 0) {
        len = -1;
    }

    return len;
}

static int sl_unpack_r(DALLOC_CTX *query, const char *buf, size_t buf_len,
                       int depth)
{
    EC_INIT;
    int encoding;
    uint64_t query_data64;
    uint64_t toc_offset64;
    uint64_t toc_entry_count64;
    size_t data_len;
    uint toc_offset;
    uint toc_entry_count;

    if (buf_len < 16) {
        EC_FAIL;
    }

    if (strncmp(buf, "md031234", 8) == 0) {
        encoding = SL_ENC_BIG_ENDIAN;
    } else {
        encoding = SL_ENC_LITTLE_ENDIAN;
    }

    buf += 8;
    buf_len -= 8;
    EC_NEG1(sl_unpack_uint64_checked(buf, buf_len, 0, encoding,
                                     &query_data64));
    toc_offset64 = ((query_data64 >> 32) - 1) * 8;
    buf += 8;
    buf_len -= 8;
    data_len = buf_len;

    /*
     * From here buf/buf_len cover only the post-header data region;
     * toc_offset is relative to this region and must point at the TOC tag.
     */
    if (toc_offset64 > 65000
            || !sl_unpack_range_valid(data_len, (int)toc_offset64,
                                      sizeof(uint64_t))) {
        EC_FAIL_LOG("Spotlight unmarshalling rejected TOC offset: "
                    "toc_offset=%" PRIu64 ", data_len=%zu",
                    toc_offset64, data_len);
    }

    toc_offset = (uint)toc_offset64;
    EC_NEG1(sl_unpack_uint64_checked(buf, buf_len, toc_offset, encoding,
                                     &query_data64));

    if (((query_data64 & 0xffff0000) >> 16) != SQ_TYPE_TOC) {
        EC_FAIL;
    }

    toc_entry_count64 = query_data64 & 0xffff;

    if (toc_entry_count64 == 0) {
        EC_FAIL;
    }

    toc_entry_count64--;

    if (toc_entry_count64 > UINT_MAX
            || toc_entry_count64 > (buf_len - (toc_offset + 8)) / 8) {
        EC_FAIL;
    }

    toc_entry_count = (uint)toc_entry_count64;
    EC_NEG1(sl_unpack_loop(query, buf, 0, 1, buf_len, toc_offset + 8,
                           toc_entry_count, encoding, depth));
EC_CLEANUP:
    EC_EXIT;
}

int sl_unpack(DALLOC_CTX *query, const char *buf)
{
    return sl_unpack_r(query, buf, MAX_SLQ_DAT, 0);
}

int sl_unpack_len(DALLOC_CTX *query, const char *buf, size_t buf_len)
{
    return sl_unpack_r(query, buf, buf_len, 0);
}
