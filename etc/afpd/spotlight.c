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
#define SPOTLIGHT_CMD_FLAGS   2
#define SPOTLIGHT_CMD_RPC     3
#define SPOTLIGHT_CMD_VOLPATH 4

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

static uint64_t spotlight_ntoh64(const char *buf, int encoding)
{
	if (encoding == SL_ENC_LITTLE_ENDIAN)
		return LVAL(buf, 0);
	else
        return ntoh64(LVAL(buf, 0));
}

#if 0
static gdouble
spotlight_ntohieee_double(tvbuff_t *tvb, gint offset, guint encoding)
{
	if (encoding == ENC_LITTLE_ENDIAN)
		return tvb_get_letohieee_double(tvb, offset);
	else
		return tvb_get_ntohieee_double(tvb, offset);
}

/*
* Returns the UTF-16 string encoding, by checking the 2-byte byte order mark.
* If there is no byte order mark, -1 is returned.
*/
static guint
spotlight_get_utf16_string_encoding(tvbuff_t *tvb, gint offset, gint query_length, guint encoding) {
	guint utf16_encoding;

	/* check for byte order mark */
	utf16_encoding = ENC_BIG_ENDIAN;
	if (query_length >= 2) {
		guint16 byte_order_mark;
		if (encoding == ENC_LITTLE_ENDIAN)
			byte_order_mark = tvb_get_letohs(tvb, offset);
		else
			byte_order_mark = tvb_get_ntohs(tvb, offset);

		if (byte_order_mark == 0xFFFE) {
			utf16_encoding = ENC_BIG_ENDIAN | ENC_UTF_16;
		}
		else if (byte_order_mark == 0xFEFF) {
			utf16_encoding = ENC_LITTLE_ENDIAN | ENC_UTF_16;
		}
	}

	return utf16_encoding;
}

static gint
spotlight_int64(tvbuff_t *tvb, proto_tree *tree, gint offset, guint encoding)
{
	gint count, i;
	guint64 query_data64;

	query_data64 = spotlight_ntoh64(tvb, offset, encoding);
	count = query_data64 >> 32;
	offset += 8;

	i = 0;
	while (i++ < count) {
		query_data64 = spotlight_ntoh64(tvb, offset, encoding);
		proto_tree_add_text(tree, tvb, offset, 8, "int64: 0x%016" G_GINT64_MODIFIER "x", query_data64);
		offset += 8;
	}

	return count;
}

static gint
spotlight_date(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, gint offset, guint encoding)
{
	gint count, i;
	guint64 query_data64;
	nstime_t t;

	query_data64 = spotlight_ntoh64(tvb, offset, encoding);
	count = query_data64 >> 32;
	offset += 8;

	if (count > SUBQ_SAFETY_LIM) {
		expert_add_info_format(pinfo, tree, PI_MALFORMED, PI_ERROR,
							   "Subquery count (%d) > safety limit (%d)", count, SUBQ_SAFETY_LIM);
		return -1;
	}

	i = 0;
	while (i++ < count) {
		query_data64 = spotlight_ntoh64(tvb, offset, encoding) >> 24;
		t.secs = query_data64 - SPOTLIGHT_TIME_DELTA;
		t.nsecs = 0;
		proto_tree_add_time(tree, hf_afp_spotlight_date, tvb, offset, 8, &t);
		offset += 8;
	}

	return count;
}

static gint
spotlight_uuid(tvbuff_t *tvb, proto_tree *tree, gint offset, guint encoding)
{
	gint count, i;
	guint64 query_data64;

	query_data64 = spotlight_ntoh64(tvb, offset, encoding);
	count = query_data64 >> 32;
	offset += 8;

	i = 0;
	while (i++ < count) {
		proto_tree_add_item(tree, hf_afp_spotlight_uuid, tvb, offset, 16, ENC_BIG_ENDIAN);
		offset += 16;
	}

	return count;
}

static gint
spotlight_float(tvbuff_t *tvb, proto_tree *tree, gint offset, guint encoding)
{
	gint count, i;
	guint64 query_data64;
	gdouble fval;

	query_data64 = spotlight_ntoh64(tvb, offset, encoding);
	count = query_data64 >> 32;
	offset += 8;

	i = 0;
	while (i++ < count) {
		fval = spotlight_ntohieee_double(tvb, offset, encoding);
		proto_tree_add_text(tree, tvb, offset, 8, "float: %f", fval);
		offset += 8;
	}

	return count;
}

static gint
spotlight_CNID_array(tvbuff_t *tvb, proto_tree *tree, gint offset, guint encoding)
{
	gint count;
	guint64 query_data64;
	guint16 unknown1;
	guint32 unknown2;

	query_data64 = spotlight_ntoh64(tvb, offset, encoding);
	count = query_data64 & 0xffff;
	unknown1 = (query_data64 & 0xffff0000) >> 16;
	unknown2 = query_data64 >> 32;

	proto_tree_add_text(tree, tvb, offset + 2, 2, "unknown1: 0x%04" G_GINT16_MODIFIER "x",
		unknown1);
	proto_tree_add_text(tree, tvb, offset + 4, 4, "unknown2: 0x%08" G_GINT32_MODIFIER "x",
		unknown2);
	offset += 8;


	while (count --) {
		query_data64 = spotlight_ntoh64(tvb, offset, encoding);
		proto_tree_add_text(tree, tvb, offset, 8, "CNID: %" G_GINT64_MODIFIER "u",
			query_data64);
		offset += 8;
	}

	return 0;
}

static const char *spotlight_get_qtype_string(guint64 query_type)
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

static const char *spotlight_get_cpx_qtype_string(guint64 cpx_query_type)
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

static gint
spotlight_dissect_query_loop(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, gint offset,
                             guint64 cpx_query_type, gint count, gint toc_offset, guint encoding)
{
	gint i, j;
	gint subquery_count;
	gint toc_index;
	guint64 query_data64;
	gint query_length;
	guint64 query_type;
	guint64 complex_query_type;
	guint unicode_encoding;
	guint8 mark_exists;

	proto_item *item_query;
	proto_tree *sub_tree;

	/*
	 * This loops through a possibly nested query data structure.
	 * The outermost one is always without count and called from
	 * dissect_spotlight() with count = INT_MAX thus the while (...)
	 * loop terminates if (offset >= toc_offset).
	 * If nested structures are found, these will have an encoded element
	 * count which is used in a recursive call to
	 * spotlight_dissect_query_loop as count parameter, thus in this case
	 * the while (...) loop will terminate when count reaches 0.
	 */
	while ((offset < (toc_offset - 8)) && (count > 0)) {
		query_data64 = spotlight_ntoh64(tvb, offset, encoding);
		query_length = (query_data64 & 0xffff) * 8;
		if (query_length == 0) {
			/* XXX - report this as an error */
			break;
		}
		query_type = (query_data64 & 0xffff0000) >> 16;

		switch (query_type) {
		case SQ_TYPE_COMPLEX:
			toc_index = (gint)((query_data64 >> 32) - 1);
			query_data64 = spotlight_ntoh64(tvb, toc_offset + toc_index * 8, encoding);
			complex_query_type = (query_data64 & 0xffff0000) >> 16;

			switch (complex_query_type) {
			case SQ_CPX_TYPE_ARRAY:
			case SQ_CPX_TYPE_DICT:
				subquery_count = (gint)(query_data64 >> 32);
				item_query = proto_tree_add_text(tree, tvb, offset, query_length,
								 "%s, toc index: %u, children: %u",
								 spotlight_get_cpx_qtype_string(complex_query_type),
								 toc_index + 1,
								 subquery_count);
				break;
			case SQ_CPX_TYPE_STRING:
				subquery_count = 1;
				query_data64 = spotlight_ntoh64(tvb, offset + 8, encoding);
				query_length = (query_data64 & 0xffff) * 8;
				item_query = proto_tree_add_text(tree, tvb, offset, query_length + 8,
								 "%s, toc index: %u, string: '%s'",
								 spotlight_get_cpx_qtype_string(complex_query_type),
								 toc_index + 1,
								 tvb_get_ephemeral_string(tvb, offset + 16, query_length - 8));
				break;
			case SQ_CPX_TYPE_UTF16_STRING:
				/*
				* This is an UTF-16 string.
				* Dissections show the typical byte order mark 0xFFFE or 0xFEFF, respectively.
				* However the existence of such a mark can not be assumed.
				* If the mark is missing, big endian encoding is assumed.
				*/

				subquery_count = 1;
				query_data64 = spotlight_ntoh64(tvb, offset + 8, encoding);
				query_length = (query_data64 & 0xffff) * 8;

				unicode_encoding = spotlight_get_utf16_string_encoding(tvb, offset + 16, query_length - 8, encoding);
				mark_exists = (unicode_encoding & ENC_UTF_16);
				unicode_encoding &= ~ENC_UTF_16;

				item_query = proto_tree_add_text(tree, tvb, offset, query_length + 8,
								 "%s, toc index: %u, utf-16 string: '%s'",
								 spotlight_get_cpx_qtype_string(complex_query_type),
								 toc_index + 1,
								 tvb_get_ephemeral_unicode_string(tvb, offset + (mark_exists ? 18 : 16),
								 query_length - (mark_exists? 10 : 8), unicode_encoding));
				break;
			default:
				subquery_count = 1;
				item_query = proto_tree_add_text(tree, tvb, offset, query_length,
								 "type: %s (%s), toc index: %u, children: %u",
								 spotlight_get_qtype_string(query_type),
								 spotlight_get_cpx_qtype_string(complex_query_type),
								 toc_index + 1,
								 subquery_count);
				break;
			}

			sub_tree = proto_item_add_subtree(item_query, ett_afp_spotlight_query_line);
			offset += 8;
			offset = spotlight_dissect_query_loop(tvb, pinfo, sub_tree, offset, complex_query_type, subquery_count, toc_offset, encoding);
			count--;
			break;
		case SQ_TYPE_NULL:
			subquery_count = (gint)(query_data64 >> 32);
			if (subquery_count > count) {
				item_query = proto_tree_add_text(tree, tvb, offset, query_length, "null");
				expert_add_info_format(pinfo, item_query, PI_MALFORMED, PI_ERROR,
					"Subquery count (%d) > query count (%d)", subquery_count, count);
				count = 0;
			} else if (subquery_count > 20) {
				item_query = proto_tree_add_text(tree, tvb, offset, query_length, "null");
				expert_add_info_format(pinfo, item_query, PI_PROTOCOL, PI_WARN,
					"Abnormal number of subqueries (%d)", subquery_count);
				count -= subquery_count;
			} else {
				for (i = 0; i < subquery_count; i++, count--)
					proto_tree_add_text(tree, tvb, offset, query_length, "null");
			}
			offset += query_length;
			break;
		case SQ_TYPE_BOOL:
			proto_tree_add_text(tree, tvb, offset, query_length, "bool: %s",
							 (query_data64 >> 32) ? "true" : "false");
			count--;
			offset += query_length;
			break;
		case SQ_TYPE_INT64:
			item_query = proto_tree_add_text(tree, tvb, offset, 8, "int64");
			sub_tree = proto_item_add_subtree(item_query, ett_afp_spotlight_query_line);
			j = spotlight_int64(tvb, sub_tree, offset, encoding);
			count -= j;
			offset += query_length;
			break;
		case SQ_TYPE_UUID:
			item_query = proto_tree_add_text(tree, tvb, offset, 8, "UUID");
			sub_tree = proto_item_add_subtree(item_query, ett_afp_spotlight_query_line);
			j = spotlight_uuid(tvb, sub_tree, offset, encoding);
			count -= j;
			offset += query_length;
			break;
		case SQ_TYPE_FLOAT:
			item_query = proto_tree_add_text(tree, tvb, offset, 8, "float");
			sub_tree = proto_item_add_subtree(item_query, ett_afp_spotlight_query_line);
			j = spotlight_float(tvb, sub_tree, offset, encoding);
			count -= j;
			offset += query_length;
			break;
		case SQ_TYPE_DATA:
			switch (cpx_query_type) {
			case SQ_CPX_TYPE_STRING:
				proto_tree_add_text(tree, tvb, offset, query_length, "string: '%s'",
						    tvb_get_ephemeral_string(tvb, offset + 8, query_length - 8));
				break;
			case SQ_CPX_TYPE_UTF16_STRING: {
				/* description see above */
				unicode_encoding = spotlight_get_utf16_string_encoding(tvb, offset + 8, query_length, encoding);
				mark_exists = (unicode_encoding & ENC_UTF_16);
				unicode_encoding &= ~ENC_UTF_16;

				proto_tree_add_text(tree, tvb, offset, query_length, "utf-16 string: '%s'",
						    tvb_get_ephemeral_unicode_string(tvb, offset + (mark_exists ? 10 : 8),
								query_length - (mark_exists? 10 : 8), unicode_encoding));
				break;
			}
			case SQ_CPX_TYPE_FILEMETA:
				if (query_length <= 8) {
					/* item_query = */ proto_tree_add_text(tree, tvb, offset, query_length, "filemeta (empty)");
				} else {
					item_query = proto_tree_add_text(tree, tvb, offset, query_length, "filemeta");
					sub_tree = proto_item_add_subtree(item_query, ett_afp_spotlight_query_line);
					(void)dissect_spotlight(tvb, pinfo, sub_tree, offset + 8);
				}
				break;
			}
			count--;
			offset += query_length;
			break;
		case SQ_TYPE_CNIDS:
			if (query_length <= 8) {
				/* item_query = */ proto_tree_add_text(tree, tvb, offset, query_length, "CNID Array (empty)");
			} else {
				item_query = proto_tree_add_text(tree, tvb, offset, query_length, "CNID Array");
				sub_tree = proto_item_add_subtree(item_query, ett_afp_spotlight_query_line);
				spotlight_CNID_array(tvb, sub_tree, offset + 8, encoding);
			}
			count--;
			offset += query_length;
			break;
		case SQ_TYPE_DATE:
			if ((j = spotlight_date(tvb, pinfo, tree, offset, encoding)) == -1)
				return offset;
			count -= j;
			offset += query_length;
			break;
		default:
			proto_tree_add_text(tree, tvb, offset, query_length, "type: %s",
							 spotlight_get_qtype_string(query_type));
			count--;
			offset += query_length;
			break;
		}
	}

	return offset;
}

static gint
dissect_spotlight(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, gint offset)
{
	guint encoding;
	gint i;
	guint64 toc_offset;
	guint64 querylen;
	gint toc_entries;
	guint64 toc_entry;

	proto_item *item_queries_data;
	proto_tree *sub_tree_queries;
	proto_item *item_toc;
	proto_tree *sub_tree_toc;

	if (strncmp(tvb_get_ephemeral_string(tvb, offset, 8), "md031234", 8) == 0)
		encoding = ENC_BIG_ENDIAN;
	else
		encoding = ENC_LITTLE_ENDIAN;
	proto_tree_add_text(tree,
			    tvb,
			    offset,
			    8,
			    "Endianess: %s",
			    encoding == ENC_BIG_ENDIAN ?
			    "Big Endian" : "Litte Endian");
	offset += 8;

	toc_offset = (spotlight_ntoh64(tvb, offset, encoding) >> 32) * 8;
	if (toc_offset < 8) {
		proto_tree_add_text(tree,
				    tvb,
				    offset,
				    8,
				    "ToC Offset: %" G_GINT64_MODIFIER "u < 8 (bogus)",
				    toc_offset);
		return -1;
	}
	toc_offset -= 8;
	if (offset + toc_offset + 8 > G_MAXINT) {
		proto_tree_add_text(tree,
				    tvb,
				    offset,
				    8,
				    "ToC Offset: %" G_GINT64_MODIFIER "u > %u (bogus)",
				    toc_offset,
				    G_MAXINT - 8 - offset);
		return -1;
	}
	querylen = (spotlight_ntoh64(tvb, offset, encoding) & 0xffffffff) * 8;
	if (querylen < 8) {
		proto_tree_add_text(tree,
				    tvb,
				    offset,
				    8,
				    "ToC Offset: %" G_GINT64_MODIFIER "u Bytes, Query length: %" G_GINT64_MODIFIER "u < 8 (bogus)",
				    toc_offset,
				    querylen);
		return -1;
	}
	querylen -= 8;
	if (querylen > G_MAXINT) {
		proto_tree_add_text(tree,
				    tvb,
				    offset,
				    8,
				    "ToC Offset: %" G_GINT64_MODIFIER "u Bytes, Query length: %" G_GINT64_MODIFIER "u > %u (bogus)",
				    toc_offset,
				    querylen,
				    G_MAXINT);
		return -1;
	}
	proto_tree_add_text(tree,
			    tvb,
			    offset,
			    8,
			    "ToC Offset: %" G_GINT64_MODIFIER "u Bytes, Query length: %" G_GINT64_MODIFIER "u Bytes",
			    toc_offset,
			    querylen);
	offset += 8;

	toc_entries = (gint)(spotlight_ntoh64(tvb, offset + (gint)toc_offset, encoding) & 0xffff);

	item_queries_data = proto_tree_add_text(tree,
						tvb,
						offset,
						(gint)toc_offset,
						"Spotlight RPC data");
	sub_tree_queries = proto_item_add_subtree(item_queries_data, ett_afp_spotlight_queries);

	/* Queries */
	offset = spotlight_dissect_query_loop(tvb, pinfo, sub_tree_queries, offset, SQ_CPX_TYPE_ARRAY, INT_MAX, offset + (gint)toc_offset + 8, encoding);

	/* ToC */
	if (toc_entries < 1) {
		proto_tree_add_text(tree,
				    tvb,
				    offset,
				    (gint)querylen - (gint)toc_offset,
				    "Complex types ToC (%u < 1 - bogus)",
				    toc_entries);
		return -1;
	}
	toc_entries -= 1;
	item_toc = proto_tree_add_text(tree,
				       tvb,
				       offset,
				       (gint)querylen - (gint)toc_offset,
				       "Complex types ToC (%u entries)",
				       toc_entries);
	sub_tree_toc = proto_item_add_subtree(item_toc, ett_afp_spotlight_toc);
	proto_tree_add_text(sub_tree_toc, tvb, offset, 2, "Number of entries (%u)", toc_entries);
	proto_tree_add_text(sub_tree_toc, tvb, offset + 2, 2, "unknown");
	proto_tree_add_text(sub_tree_toc, tvb, offset + 4, 4, "unknown");

	offset += 8;
	for (i = 0; i < toc_entries; i++, offset += 8) {
		toc_entry = spotlight_ntoh64(tvb, offset, encoding);
		if ((((toc_entry & 0xffff0000) >> 16) == SQ_CPX_TYPE_ARRAY)
		    || (((toc_entry & 0xffff0000) >> 16) == SQ_CPX_TYPE_DICT)) {
			proto_tree_add_text(sub_tree_toc,
					    tvb,
					    offset,
					    8,
					    "%u: count: %" G_GINT64_MODIFIER "u, type: %s, offset: %" G_GINT64_MODIFIER "u",
					    i+1,
					    toc_entry >> 32,
					    spotlight_get_cpx_qtype_string((toc_entry & 0xffff0000) >> 16),
					    (toc_entry & 0xffff) * 8);
		} else if ((((toc_entry & 0xffff0000) >> 16) == SQ_CPX_TYPE_STRING)
			|| (((toc_entry & 0xffff0000) >> 16) == SQ_CPX_TYPE_UTF16_STRING)) {
			proto_tree_add_text(sub_tree_toc,
					    tvb,
					    offset,
					    8,
					    "%u: pad byte count: %" G_GINT64_MODIFIER "x, type: %s, offset: %" G_GINT64_MODIFIER "u",
					    i+1,
					    8 - (toc_entry >> 32),
					    spotlight_get_cpx_qtype_string((toc_entry & 0xffff0000) >> 16),
					    (toc_entry & 0xffff) * 8);
		}
		else {
			proto_tree_add_text(sub_tree_toc,
					    tvb,
					    offset,
					    8,
					    "%u: unknown: 0x%08" G_GINT64_MODIFIER "x, type: %s, offset: %" G_GINT64_MODIFIER "u",
					    i+1,
					    toc_entry >> 32,
					    spotlight_get_cpx_qtype_string((toc_entry & 0xffff0000) >> 16),
					    (toc_entry & 0xffff) * 8);
		}


	}

	return offset;
}
#endif

static DALLOC_CTX *unpack_spotlight(TALLOC_CTX *mem_ctx, char *ibuf, size_t ibuflen)
{
    EC_INIT;
	int len;
    DALLOC_CTX *query;

    EC_NULL_LOG( query = talloc_zero(mem_ctx, DALLOC_CTX) );

    ibuf++;
    ibuflen--;


EC_CLEANUP:
    if (ret != 0) {
        talloc_free(query);
        query = NULL;
    }
	return query;
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

    *rbuflen = 0;

    ibuf += 2;
    ibuflen -= 2;

    vid = SVAL(ibuf, 0);
    LOG(logtype_default, log_note, "afp_spotlight_rpc(vid: %" PRIu16 ")", vid);

    if ((vol = getvolbyvid(vid)) == NULL) {
        LOG(logtype_default, log_error, "afp_spotlight_rpc: bad volume id: %" PRIu16 ")", vid);
        ret = AFPERR_ACCESS;
        goto EC_CLEANUP;
    }

    /*    IVAL(ibuf, 2): unknown, always 0x00008004, some flags ? */

    cmd = RIVAL(ibuf, 6);
    LOG(logtype_default, log_note, "afp_spotlight_rpc(cmd: %d)", cmd);

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
		break;

	case SPOTLIGHT_CMD_RPC:
        /* IVAL(buf, 14): our reply in SPOTLIGHT_CMD_FLAGS */
        /* IVAL(buf, 18): length */
        /* IVAL(buf, 22): endianess, ignored, we assume little endian */
		break;
	}

EC_CLEANUP:
    talloc_free(tmp_ctx);
    if (ret != AFP_OK) {
        
    }
    EC_EXIT;
}

/**************************************************************************************************
 * Testing
 **************************************************************************************************/

#ifdef SPOT_TEST_MAIN

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

    printf("%sArray(#%d): {\n", neststrings[nestinglevel], talloc_array_length(dd->dd_talloc_array));

    for (int n = 0; n < talloc_array_length(dd->dd_talloc_array); n++) {

        type = talloc_get_name(dd->dd_talloc_array[n]);

        if (STRCMP(type, ==, "int64_t")) {
            int64_t i;
            memcpy(&i, dd->dd_talloc_array[n], sizeof(int64_t));
            printf("%s%d:\t%" PRId64 "\n", neststrings[nestinglevel + 1], n, i);
        } else if (STRCMP(type, ==, "uint32_t")) {
            uint32_t i;
            memcpy(&i, dd->dd_talloc_array[n], sizeof(uint32_t));
            printf("%s%d:\t%" PRIu32 "\n", neststrings[nestinglevel + 1], n, i);
        } else if (STRCMP(type, ==, "char *")) {
            char *s;
            memcpy(&s, dd->dd_talloc_array[n], sizeof(char *));
            printf("%s%d:\t%s\n", neststrings[nestinglevel + 1], n, s);
        } else if (STRCMP(type, ==, "_Bool")) {
            bool bl;
            memcpy(&bl, dd->dd_talloc_array[n], sizeof(bool));
            printf("%s%d:\t%s\n", neststrings[nestinglevel + 1], n, bl ? "true" : "false");
        } else if (STRCMP(type, ==, "dd_t")) {
            DALLOC_CTX *nested;
            memcpy(&nested, dd->dd_talloc_array[n], sizeof(DALLOC_CTX *));
            dd_dump(nested, nestinglevel + 1);
        } else if (STRCMP(type, ==, "cnid_array_t")) {
            cnid_array_t *cnids;
            memcpy(&cnids, dd->dd_talloc_array[n], sizeof(cnid_array_t *));
            printf("%s%d:\tunkn1: %" PRIu16 ", unkn2: %" PRIu32,
                   neststrings[nestinglevel + 1], n, cnids->ca_unkn1, cnids->ca_unkn2);
            if (cnids->ca_cnids)
                dd_dump(cnids->ca_cnids, nestinglevel + 1);
        }
    }
    printf("%s}\n", neststrings[nestinglevel]);
}

#include <stdarg.h>

int main(int argc, char **argv)
{
    TALLOC_CTX *mem_ctx = talloc_new(NULL);
    DALLOC_CTX *dd = talloc_zero(mem_ctx, DALLOC_CTX);
    int64_t i;

    set_processname("spot");
    setuplog("default:info", "/dev/tty");

    LOG(logtype_default, log_info, "Start");

    i = 2;
    dalloc_add(dd, &i, int64_t);

    i = 1;
    dalloc_add(dd, &i, int64_t);


    char *str = talloc_strdup(dd, "hello world");
    dalloc_add(dd, &str, char *);

    bool b = true;
    dalloc_add(dd, &b, bool);

    b = false;
    dalloc_add(dd, &b, bool);


    /* add a nested array */
    DALLOC_CTX *nested = talloc_zero(dd, DALLOC_CTX);
    i = 3;
    dalloc_add(nested, &i, int64_t);
    dalloc_add(dd, &nested, DALLOC_CTX);

    /* test a CNID array */
    uint32_t id = 16;
    cnid_array_t *cnids = talloc_zero(dd, cnid_array_t);

    cnids->ca_cnids = talloc_zero(cnids, DALLOC_CTX);

    cnids->ca_unkn1 = 1;
    cnids->ca_unkn2 = 2;

    dalloc_add(cnids->ca_cnids, &id, uint32_t);
    dalloc_add(dd, &cnids, cnid_array_t);

    dd_dump(dd, 0);

    talloc_free(mem_ctx);
    return 0;
}
#endif
