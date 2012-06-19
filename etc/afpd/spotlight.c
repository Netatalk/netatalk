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

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdbool.h>
#include <inttypes.h>

#include <atalk/errchk.h>
#include <atalk/util.h>
#include <atalk/logger.h>
#include <atalk/talloc.h>
#include <atalk/dalloc.h>
#include <atalk/byteorder.h>
#include <atalk/netatalk_conf.h>
#include <atalk/volume.h>

#include "spotlight.h"

/**************************************************************************************************
 * RPC data marshalling and unmarshalling
 **************************************************************************************************/

/* FPSpotlightRPC subcommand codes */
#define SPOTLIGHT_CMD_VOLPATH 1
#define SPOTLIGHT_CMD_FLAGS   2
#define SPOTLIGHT_CMD_RPC     3

/* Spotlight epoch is UNIX epoch minus SPOTLIGHT_TIME_DELTA */
#define SPOTLIGHT_TIME_DELTA INT64_C(280878921600U)

#define SQ_TYPE_NULL    0x0000
#define SQ_TYPE_COMPLEX 0x0200
#define SQ_TYPE_INT64   0x8400
#define SQ_TYPE_BOOL    0x0100
#define SQ_TYPE_FLOAT   0x8500
#define SQ_TYPE_DATA    0x0700
#define SQ_TYPE_CNIDS   0x8700
#define SQ_TYPE_UUID    0x0e00
#define SQ_TYPE_DATE    0x8600

#define SQ_CPX_TYPE_ARRAY    		0x0a00
#define SQ_CPX_TYPE_STRING   		0x0c00
#define SQ_CPX_TYPE_UTF16_STRING	0x1c00
#define SQ_CPX_TYPE_DICT     		0x0d00
#define SQ_CPX_TYPE_CNIDS    		0x1a00
#define SQ_CPX_TYPE_FILEMETA 		0x1b00

#define SUBQ_SAFETY_LIM 20

/* Can be ored and used as flags */
#define SL_ENC_LITTLE_ENDIAN 1
#define SL_ENC_BIG_ENDIAN    2
#define SL_ENC_UTF_16        4

/* Forward declarations */
static int dissect_spotlight(DALLOC_CTX *query, const char *buf);

/* Helper functions and stuff */
static const char *neststrings[] = {
    "",
    "    ",
    "        ",
    "            ",
    "                ",
    "                    ",
    "                        "
};

static int dd_dump(DALLOC_CTX *dd, int nestinglevel)
{
    const char *type;

    LOG(log_debug, logtype_sl, "%s1: %s(#%d): {", neststrings[nestinglevel], talloc_get_name(dd), talloc_array_length(dd->dd_talloc_array));

    for (int n = 0; n < talloc_array_length(dd->dd_talloc_array); n++) {

        type = talloc_get_name(dd->dd_talloc_array[n]);

        if (STRCMP(type, ==, "DALLOC_CTX")
                   || STRCMP(type, ==, "sl_array_t")
                   || STRCMP(type, ==, "sl_dict_t")) {
            dd_dump(dd->dd_talloc_array[n], nestinglevel + 1);
        } else if (STRCMP(type, ==, "uint64_t")) {
            uint64_t i;
            memcpy(&i, dd->dd_talloc_array[n], sizeof(uint64_t));
            LOG(log_debug, logtype_sl, "%s%u:\t0x%04x", neststrings[nestinglevel + 1], n + 1, i);
        } else if (STRCMP(type, ==, "int64_t")) {
            int64_t i;
            memcpy(&i, dd->dd_talloc_array[n], sizeof(int64_t));
            LOG(log_debug, logtype_sl, "%s%d:\t%" PRId64, neststrings[nestinglevel + 1], n + 1, i);
        } else if (STRCMP(type, ==, "uint32_t")) {
            uint32_t i;
            memcpy(&i, dd->dd_talloc_array[n], sizeof(uint32_t));
            LOG(log_debug, logtype_sl, "%s%d:\t%" PRIu32, neststrings[nestinglevel + 1], n + 1, i);
        } else if (STRCMP(type, ==, "char *")) {
            char *s;
            memcpy(&s, dd->dd_talloc_array[n], sizeof(char *));
            LOG(log_debug, logtype_sl, "%s%d:\t%s", neststrings[nestinglevel + 1], n + +1, s);
        } else if (STRCMP(type, ==, "sl_bool_t")) {
            sl_bool_t bl;
            memcpy(&bl, dd->dd_talloc_array[n], sizeof(sl_bool_t));
            LOG(log_debug, logtype_sl, "%s%d:\t%s", neststrings[nestinglevel + 1], n + +1, bl ? "true" : "false");
        } else if (STRCMP(type, ==, "sl_cnids_t")) {
            sl_cnids_t cnids;
            memcpy(&cnids, dd->dd_talloc_array[n], sizeof(sl_cnids_t));
            LOG(log_debug, logtype_sl, "%s%d:\tunkn1: %" PRIu16 ", unkn2: %" PRIu32,
                   neststrings[nestinglevel + 1], n + 1, cnids.ca_unkn1, cnids.ca_unkn2);
            if (cnids.ca_cnids)
                dd_dump(cnids.ca_cnids, nestinglevel + 1);
        }
    }
    LOG(log_debug, logtype_sl, "%s}", neststrings[nestinglevel]);
}

/*
* Returns the UTF-16 string encoding, by checking the 2-byte byte order mark.
* If there is no byte order mark, -1 is returned.
*/
static uint spotlight_get_utf16_string_encoding(const char *buf, int offset, int query_length, uint encoding) {
	uint utf16_encoding;

	/* check for byte order mark */
	utf16_encoding = SL_ENC_BIG_ENDIAN;
	if (query_length >= 2) {
		uint16_t byte_order_mark;
		if (encoding == SL_ENC_LITTLE_ENDIAN)
			byte_order_mark = SVAL(buf, offset);
		else
			byte_order_mark = RSVAL(buf, offset);

		if (byte_order_mark == 0xFFFE) {
			utf16_encoding = SL_ENC_BIG_ENDIAN | SL_ENC_UTF_16;
		}
		else if (byte_order_mark == 0xFEFF) {
			utf16_encoding = SL_ENC_LITTLE_ENDIAN | SL_ENC_UTF_16;
		}
	}

	return utf16_encoding;
}

/**************************************************************************************************
 * marshalling functions
 **************************************************************************************************/

static uint64_t sl_pack_tag(uint16_t type, uint16_t size, uint32_t val)
{
    uint64_t tag = ((uint64_t)val << 32) | ((uint64_t)type << 16) | size;
    return tag;
}

static int sl_pack_float(double d, char **buf, int *buf_len, char **toc_buf, int *toc_len, int *toc_idx)
{
    union {
        double d;
        uint64_t w;
    } ieee_fp_union;

    SLVAL(*buf, 0, sl_pack_tag(SQ_TYPE_FLOAT, 2, 1));
    SLVAL(*buf, 8, ieee_fp_union.w);
    *buf += 2 * sizeof(uint64_t);
    buf_len += 2 * sizeof(uint64_t);

    return 0;
}

static int sl_pack_uint64(uint64_t u, char **buf, int *buf_len, char **toc_buf, int *toc_len, int *toc_idx)
{
    SLVAL(*buf, 0, sl_pack_tag(SQ_TYPE_INT64, 2, 1));
    SLVAL(*buf, 8, u);
    *buf += 2 * sizeof(uint64_t);
    *buf_len += 2 * sizeof(uint64_t);
}

/**************************************************************************************************
 * unmarshalling functions
 **************************************************************************************************/

static uint64_t sl_unpack_uint64(const char *buf, int offset, uint encoding)
{
    if (encoding == SL_ENC_LITTLE_ENDIAN)
            return LVAL(buf, offset);
        else
            return RLVAL(buf, offset);
}

static int sl_unpack_ints(DALLOC_CTX *query, const char *buf, int offset, uint encoding)
{
	int count, i;
	uint64_t query_data64;

	query_data64 = sl_unpack_uint64(buf, offset, encoding);
	count = query_data64 >> 32;
	offset += 8;

	i = 0;
	while (i++ < count) {
        query_data64 = sl_unpack_uint64(buf, offset, encoding);
        dalloc_add(query, &query_data64, uint64_t);
		offset += 8;
	}

	return count;
}

static int sl_unpack_date(DALLOC_CTX *query, const char *buf, int offset, uint encoding)
{
	int count, i;
	uint64_t query_data64;
	sl_time_t t;

	query_data64 = sl_unpack_uint64(buf, offset, encoding);
	count = query_data64 >> 32;
	offset += 8;

	i = 0;
	while (i++ < count) {
		query_data64 = sl_unpack_uint64(buf, offset, encoding) >> 24;
		t.tv_sec = query_data64 - SPOTLIGHT_TIME_DELTA;
		t.tv_usec = 0;
        dalloc_add(query, &t, sl_time_t);
		offset += 8;
	}

	return count;
}

static int sl_unpack_uuid(DALLOC_CTX *query, const char *buf, int offset, uint encoding)
{
	int count, i;
    uint64_t query_data64;
    sl_uuid_t uuid;
	query_data64 = sl_unpack_uint64(buf, offset, encoding);
	count = query_data64 >> 32;
	offset += 8;

	i = 0;
	while (i++ < count) {
        memcpy(uuid.sl_uuid, buf + offset, 16);
        dalloc_add(query, &uuid, sl_uuid_t);
		offset += 16;
	}

	return count;
}

static int sl_unpack_floats(DALLOC_CTX *query, const char *buf, int offset, uint encoding)
{
	int count, i;
	uint64_t query_data64;
	double fval;
    union {
        double d;
        uint32_t w[2];
    } ieee_fp_union;

	query_data64 = sl_unpack_uint64(buf, offset, encoding);
	count = query_data64 >> 32;
	offset += 8;

	i = 0;
	while (i++ < count) {
        if (encoding == SL_ENC_LITTLE_ENDIAN) {
#ifdef WORDS_BIGENDIAN
            ieee_fp_union.w[0] = IVAL(buf, offset + 4);
            ieee_fp_union.w[1] = IVAL(buf, offset);
#else
            ieee_fp_union.w[0] = IVAL(buf, offset);
            ieee_fp_union.w[1] = IVAL(buf, offset + 4);
#endif
        } else {
#ifdef WORDS_BIGENDIAN
            ieee_fp_union.w[0] = RIVAL(buf, offset);
            ieee_fp_union.w[1] = RIVAL(buf, offset + 4);
#else
            ieee_fp_union.w[0] = RIVAL(buf, offset + 4);
            ieee_fp_union.w[1] = RIVAL(buf, offset);
#endif
        }
        dalloc_add(query, &ieee_fp_union.d, double);
		offset += 8;
	}

	return count;
}

static int sl_unpack_CNID(DALLOC_CTX *query, const char *buf, int offset, int length, uint encoding)
{
    EC_INIT;
	int count;
	uint64_t query_data64;
    sl_cnids_t cnids;

    EC_NULL( cnids.ca_cnids = talloc_zero(query, DALLOC_CTX) );

    if (length <= 16)
        /* that's permitted, it's an empty array */
        goto EC_CLEANUP;
    
	query_data64 = sl_unpack_uint64(buf, offset, encoding);
	count = query_data64 & 0xffff;

	cnids.ca_unkn1 = (query_data64 & 0xffff0000) >> 16;
	cnids.ca_unkn2 = query_data64 >> 32;

	offset += 8;

	while (count --) {
		query_data64 = sl_unpack_uint64(buf, offset, encoding);
        dalloc_add(cnids.ca_cnids, &query_data64, uint64_t);
		offset += 8;
	}

    dalloc_add(query, &cnids, sl_cnids_t);

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

static int spotlight_dissect_loop(DALLOC_CTX *query,
                                  const char *buf,
                                  uint offset,
                                  uint count,
                                  const uint toc_offset,
                                  const uint encoding)
{
    EC_INIT;
	int i, toc_index, query_length;
    uint subcount, cpx_query_type, cpx_query_count;
	uint64_t query_data64, query_type;
	uint unicode_encoding;
	uint8_t mark_exists;
    char *p;
    int padding, slen;

	while (count > 0 && (offset < toc_offset)) {
		query_data64 = sl_unpack_uint64(buf, offset, encoding);
		query_length = (query_data64 & 0xffff) * 8;
		query_type = (query_data64 & 0xffff0000) >> 16;
		if (query_length == 0)
            EC_FAIL;

		switch (query_type) {
		case SQ_TYPE_COMPLEX:
			toc_index = (query_data64 >> 32) - 1;
			query_data64 = sl_unpack_uint64(buf, toc_offset + toc_index * 8, encoding);
			cpx_query_type = (query_data64 & 0xffff0000) >> 16;
            cpx_query_count = query_data64 >> 32;

            switch (cpx_query_type) {
			case SQ_CPX_TYPE_ARRAY: {
                sl_array_t *sl_arrary = talloc_zero(query, sl_array_t);
                EC_NEG1_LOG( offset = spotlight_dissect_loop(sl_arrary, buf, offset + 8, cpx_query_count, toc_offset, encoding) );
                dalloc_add(query, sl_arrary, sl_array_t);
                break;
            }

			case SQ_CPX_TYPE_DICT: {
                sl_dict_t *sl_dict = talloc_zero(query, sl_dict_t);
                EC_NEG1_LOG( offset = spotlight_dissect_loop(sl_dict, buf, offset + 8, cpx_query_count, toc_offset, encoding) );
                dalloc_add(query, sl_dict, sl_dict_t);
                break;
            }
            case SQ_CPX_TYPE_STRING:
                query_data64 = sl_unpack_uint64(buf, offset + 8, encoding);
                query_length += (query_data64 & 0xffff) * 8;
                if ((padding = 8 - (query_data64 >> 32)) < 0)
                    EC_FAIL;
                if ((slen = query_length - 16 - padding) < 1)
                    EC_FAIL;
                p = talloc_strndup(query, buf + offset + 16, slen);
                dalloc_add(query, &p, char *);
                break;

            case SQ_CPX_TYPE_UTF16_STRING:
                query_data64 = sl_unpack_uint64(buf, offset + 8, encoding);
                query_length += (query_data64 & 0xffff) * 8;
                if ((padding = 8 - (query_data64 >> 32)) < 0)
                    EC_FAIL;
                if ((slen = query_length - 16 - padding) < 1)
                    EC_FAIL;

                unicode_encoding = spotlight_get_utf16_string_encoding(buf, offset + 16, slen, encoding);
                mark_exists = (unicode_encoding & SL_ENC_UTF_16);
                unicode_encoding &= ~SL_ENC_UTF_16;

                EC_NEG1( convert_string_allocate(CH_UCS2, CH_UTF8, buf + offset + (mark_exists ? 18 : 16), slen, &p) );
                dalloc_add(query, &p, char *);
                break;

            case SQ_CPX_TYPE_FILEMETA:
                query_data64 = sl_unpack_uint64(buf, offset + 8, encoding);
                query_length += (query_data64 & 0xffff) * 8;

                if (query_length <= 8) {
                    EC_FAIL_LOG("SQ_CPX_TYPE_FILEMETA: query_length <= 8%s", "");
                } else {
                    EC_NEG1_LOG( dissect_spotlight(query, buf + offset + 16) );
                }
                break;

            case SQ_CPX_TYPE_CNIDS:
                query_data64 = sl_unpack_uint64(buf, offset + 8, encoding);
                query_length += (query_data64 & 0xffff) * 8;
                EC_NEG1_LOG( sl_unpack_CNID(query, buf, offset + 16, query_length, encoding) );
                break;
            } /* switch (cpx_query_type) */

			count--;
			break;

        case SQ_TYPE_NULL: {
            subcount = query_data64 >> 32;
            if (subcount > 64)
                EC_FAIL;
            sl_nil_t nil = 0;
            for (i = 0; i < subcount; i++)
                dalloc_add(query, &nil, sl_nil_t);
            count -= subcount;
            break;
        }
        case SQ_TYPE_BOOL: {
            sl_bool_t b = query_data64 >> 32;
            dalloc_add(query, &b, sl_bool_t);
            count--;
            break;
        }
        case SQ_TYPE_INT64:
            EC_NEG1_LOG( subcount = sl_unpack_ints(query, buf, offset, encoding) );
            count -= subcount;
            break;
        case SQ_TYPE_UUID:
            EC_NEG1_LOG( subcount = sl_unpack_uuid(query, buf, offset, encoding) );
            count -= subcount;
            break;
        case SQ_TYPE_FLOAT:
            EC_NEG1_LOG( subcount = sl_unpack_floats(query, buf, offset, encoding) );
            count -= subcount;
            break;
        case SQ_TYPE_DATE:
            EC_NEG1_LOG( subcount = sl_unpack_date(query, buf, offset, encoding) );
            count -= subcount;
            break;
        default:
            EC_FAIL;
        }

        offset += query_length;
    }

EC_CLEANUP:
    if (ret != 0) {
        offset = -1;
    }
	return offset;
}

static int dissect_spotlight(DALLOC_CTX *query, const char *buf)
{
    EC_INIT;
	int encoding, i, toc_entries;
	uint64_t toc_offset, tquerylen, toc_entry;

	if (strncmp(buf, "md031234", 8) == 0)
		encoding = SL_ENC_BIG_ENDIAN;
	else
		encoding = SL_ENC_LITTLE_ENDIAN;

	buf += 8;

	toc_offset = ((sl_unpack_uint64(buf, 0, encoding) >> 32) - 1 ) * 8;
	if (toc_offset < 0 || (toc_offset > 65000)) {
        EC_FAIL;
	}

	buf += 8;

	toc_entries = (int)(sl_unpack_uint64(buf, toc_offset, encoding) & 0xffff);

	EC_NEG1( spotlight_dissect_loop(query, buf, 0, 1, toc_offset + 8, encoding) );

EC_CLEANUP:
    EC_EXIT;
}

/**************************************************************************************************
 * AFP functions
 **************************************************************************************************/
int afp_spotlight_rpc(AFPObj *obj, char *ibuf, size_t ibuflen, char *rbuf, size_t *rbuflen)
{
    EC_INIT;
    TALLOC_CTX *tmp_ctx = talloc_new(NULL);
    uint16_t vid;
    int cmd;
    int endianess = SL_ENC_LITTLE_ENDIAN;
    struct vol      *vol;
    DALLOC_CTX *query;

    *rbuflen = 0;

    ibuf += 2;
    ibuflen -= 2;

    vid = SVAL(ibuf, 0);
    LOG(log_debug, logtype_sl, "afp_spotlight_rpc(vid: %" PRIu16 ")", vid);

    if ((vol = getvolbyvid(vid)) == NULL) {
        LOG(log_error, logtype_sl, "afp_spotlight_rpc: bad volume id: %" PRIu16 ")", vid);
        ret = AFPERR_ACCESS;
        goto EC_CLEANUP;
    }

    /*    IVAL(ibuf, 2): unknown, always 0x00008004, some flags ? */

    cmd = RIVAL(ibuf, 6);
    LOG(log_debug, logtype_sl, "afp_spotlight_rpc(cmd: %d)", cmd);

    /*    IVAL(ibuf, 10: unknown, always 0x00000000 */

	switch (cmd) {

	case SPOTLIGHT_CMD_VOLPATH: {
        RSIVAL(rbuf, 0, ntohs(vid));
        RSIVAL(rbuf, 4, 0);
        int len = strlen(vol->v_path) + 1;
        strncpy(rbuf + 8, vol->v_path, len);
        *rbuflen += 8 + len;
		break;
    }
	case SPOTLIGHT_CMD_FLAGS:
        RSIVAL(rbuf, 0, 0x0100006b); /* Whatever this value means... flags? */
        *rbuflen += 4;
		break;

	case SPOTLIGHT_CMD_RPC: {
        DALLOC_CTX *query;
        EC_NULL( query = talloc_zero(tmp_ctx, DALLOC_CTX) );
        (void)dissect_spotlight(query, ibuf + 22);
        dd_dump(query, 0);
		break;
    }
	}

EC_CLEANUP:
    talloc_free(tmp_ctx);
    if (ret != AFP_OK) {
        return AFPERR_MISC;
    }
    EC_EXIT;
}

/**************************************************************************************************
 * Testing
 **************************************************************************************************/

#ifdef SPOT_TEST_MAIN

int main(int argc, char **argv)
{
    EC_INIT;
    TALLOC_CTX *mem_ctx = talloc_new(NULL);
    DALLOC_CTX *dd = talloc_zero(mem_ctx, DALLOC_CTX);
    int64_t i;

    set_processname("spot");
    setuplog("default:info", "/dev/tty");

    LOG(log_info, logtype_sl, "Start");

#if 0
    i = 2;
    dalloc_add(dd, &i, int64_t);

    i = 1;
    dalloc_add(dd, &i, int64_t);


    char *str = talloc_strdup(dd, "hello world");
    dalloc_add(dd, &str, char *);

    sl_bool_t b = true;
    dalloc_add(dd, &b, sl_bool_t);

    b = false;
    dalloc_add(dd, &b, sl_bool_t);


    /* add a nested array */
    DALLOC_CTX *nested = talloc_zero(dd, DALLOC_CTX);
    i = 3;
    dalloc_add(nested, &i, int64_t);
    dalloc_add(dd, nested, DALLOC_CTX);

    /* test an allocated CNID array */
    uint32_t id = 16;
    sl_cnids_t *cnids = talloc_zero(dd, sl_cnids_t);

    cnids->ca_cnids = talloc_zero(cnids, DALLOC_CTX);

    cnids->ca_unkn1 = 1;
    cnids->ca_unkn2 = 2;

    dalloc_add(cnids->ca_cnids, &id, uint32_t);
    dalloc_add(dd, cnids, sl_cnids_t);

    /* Now the Spotlight types */
    sl_array_t *sl_arrary = talloc_zero(dd, sl_array_t);
    i = 1234;
    dalloc_add(sl_arrary, &i, int64_t);

    sl_dict_t *sl_dict = talloc_zero(dd, sl_dict_t);
    i = 5678;
    dalloc_add(sl_dict, &i, int64_t);
    dalloc_add(sl_arrary, sl_dict, sl_dict_t);

    dalloc_add(dd, sl_arrary, sl_array_t);
#endif

    /* now parse a real spotlight packet */
    char ibuf[8192];
    char rbuf[8192];
    int fd;
    size_t len;
    DALLOC_CTX *query;

    EC_NULL( query = talloc_zero(mem_ctx, DALLOC_CTX) );

    EC_NEG1_LOG( fd = open("/home/ralph/netatalk/spot/etc/afpd/spotlight-packet.bin", O_RDONLY) );
    EC_NEG1_LOG( len = read(fd, ibuf, 8192) );
    EC_NEG1_LOG( dissect_spotlight(query, ibuf + 24) );

    /* Now dump the whole thing */
    dd_dump(query, 0);

EC_CLEANUP:
    if (mem_ctx) {
        talloc_free(mem_ctx);
        mem_ctx = NULL;
    }
    EC_EXIT;
}
#endif
