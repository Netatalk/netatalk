---
type: C Source File
title: "etc/spotlight/localsearch/sparql_parser.y"
description: "11 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/localsearch/sparql_parser.y"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [etc/spotlight](../../spotlight.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

Line numbers refer to the view produced by the Doxygen input filter for this file type, not to the file itself.

# Includes

* [atalk/errchk.h](../../../include/atalk/errchk.h.md)
* [atalk/logger.h](../../../include/atalk/logger.h.md)
* [atalk/spotlight.h](../../../include/atalk/spotlight.h.md)
* [sparql_map.h](sparql_map.h.md)
* System headers: `bstrlib.h`, `ctype.h`, `gio/gio.h`, `inttypes.h`, `stdbool.h`, `stdio.h`, `string.h`, `talloc.h`, `time.h`

# Functions

### yyterminate

```c
void * yyterminate(void)
```

Declared at etc/spotlight/localsearch/sparql_parser.y line 22; no definition in the scanned sources.

### yy_scan_string

```c
YY_BUFFER_STATE yy_scan_string(const char *str)
```

Declared at etc/spotlight/localsearch/sparql_parser.y line 23; no definition in the scanned sources.

Called by: [map_spotlight_to_sparql_query](sparql_parser.y.md#map_spotlight_to_sparql_query)

### yy_delete_buffer

```c
void yy_delete_buffer(YY_BUFFER_STATE buffer)
```

Declared at etc/spotlight/localsearch/sparql_parser.y line 24; no definition in the scanned sources.

Called by: [map_spotlight_to_sparql_query](sparql_parser.y.md#map_spotlight_to_sparql_query)

### isodate2unix

```c
static time_t isodate2unix(const char *s)
```

Defined at lines 53 to 62.

Convert an ISO 8601 date of the form YYYY-MM-DDTHH:MM:SSZ to a time_t.

The call reads the date with strptime and converts it with mktime. The conversion treats the time as local time despite the Z.

Parameters:
* `s`: date string

Returns: the time, or -1 for a string in another form

### ascii_lower_dup

```c
static char * ascii_lower_dup(TALLOC_CTX *ctx, const char *s, size_t len)
```

Defined at lines 75 to 88.

Return a lowercase duplicate of the first len bytes of s.

The call lowers each byte with tolower, so in the C locale only ASCII letters change.

Parameters:
* `ctx`: talloc context for the duplicate
* `s`: string
* `len`: byte limit

Returns: the duplicate, or NULL when talloc fails

Called by: [map_expr](sparql_parser.y.md#map_expr)

### map_daterange

```c
static const char * map_daterange(const char *dateattr, time_t date1, time_t date2)
```

Defined at lines 102 to 137.

Map an InRange date test on a Spotlight attribute to a SPARQL FILTER.

Both dates go out in the form YYYY-MM-DDTHH:MM:SSZ from localtime. Unlike map_expr, the call skips the ssm_enabled check of the attribute.

Parameters:
* `dateattr`: Spotlight date attribute
* `date1`: start of the range, excluded
* `date2`: end of the range, excluded

Returns: the SPARQL pattern, or NULL for an unknown attribute or a failed localtime

Uses file-scope variables: `sparqlvar`, `ssp_slq`

### map_type_search

```c
static char * map_type_search(const char *attr, char op, const char *val)
```

Defined at lines 149 to 178.

Map a content type value to an rdf:type or nie:mimeType pattern through [MDTypeMap](sparql_map.h.md#struct-mdtypemap).

Parameters:
* `attr`: Spotlight attribute, unused
* `op`: comparison operator, unused
* `val`: content type, such as public.image

Returns: the SPARQL pattern, or NULL for a type that [MDTypeMap](sparql_map.h.md#struct-mdtypemap) lacks or does not support

Called by: [map_expr](sparql_parser.y.md#map_expr)

Uses file-scope variables: `ssp_slq`

### map_expr

```c
static const char * map_expr(const char *attr, char op, const char *val)
```

Defined at lines 197 to 375.

Map one comparison on a Spotlight attribute of the SPARQL map to a SPARQL pattern.

The attribute type picks the form. A number or a date gets a FILTER with the operator, and a string gets an anchored regex in which a star matches any text. A file name with no star, 1 star at an end, or a star at each end gets a lowercase test. The test is an equality, a prefix, a suffix, or a substring match. Any other star pattern in a file name becomes a case-insensitive regex. Content types go through map_type_search.

Parameters:
* `attr`: Spotlight attribute
* `op`: comparison operator: '=', '<', '>', or `!` for unequal
* `val`: value of the comparison

Returns: the SPARQL pattern, or NULL for an unknown attribute, one that ssm_enabled leaves off, or an error

Calls: [ascii_lower_dup](sparql_parser.y.md#ascii_lower_dup), [map_type_search](sparql_parser.y.md#map_type_search), [yyerror](sparql_parser.y.md#yyerror)

Uses file-scope variables: `sparqlvar`, `ssp_slq`

### yyerror

```c
void yyerror(const char *str)
```

Defined at lines 382 to 389.

Log a SPARQL parser error.

Parameters:
* `str`: error message

Called by: [map_expr](sparql_parser.y.md#map_expr)

### yywrap

```c
int yywrap()
```

Defined at lines 396 to 399.

Tell the lexer that no input follows the query string.

Returns: 1

### map_spotlight_to_sparql_query

```c
int map_spotlight_to_sparql_query(slq_t *slq, gchar **sparql_result)
```

Defined at lines 409 to 431.

Map a Spotlight RAW query string to a SPARQL query string.

Parameters:
* `slq`: Spotlight query handle
* `sparql_result`: Mapped SPARQL query, string is allocated in talloc context of slq

Returns: 0 on success, -1 on error

Calls: [yy_delete_buffer](sparql_parser.y.md#yy_delete_buffer), [yy_scan_string](sparql_parser.y.md#yy_scan_string)

Called by: [sl_localsearch_open_query](sl_localsearch.c.md#sl_localsearch_open_query)

Uses file-scope variables: `sparqlvar`, `ssp_result`, `ssp_slq`

# Typedefs and enums

* `typedef struct yy_buffer_state * YY_BUFFER_STATE`

# File-scope variables

`result_limit`, `sparqlvar`, `ssp_result`, `ssp_slq`
