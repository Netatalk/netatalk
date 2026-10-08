---
type: C Source File
title: "etc/spotlight/localsearch/sparql_parser.y"
description: "11 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/localsearch/sparql_parser.y"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 44 to 53.

### ascii_lower_dup

```c
static char * ascii_lower_dup(TALLOC_CTX *ctx, const char *s, size_t len)
```

Defined at lines 58 to 71.

Lowercase ASCII letters in-place in a talloc'd copy of `s`.

Called by: [map_expr](sparql_parser.y.md#map_expr)

### map_daterange

```c
static const char * map_daterange(const char *dateattr, time_t date1, time_t date2)
```

Defined at lines 73 to 108.

Uses file-scope variables: `sparqlvar`, `ssp_slq`

### map_type_search

```c
static char * map_type_search(const char *attr, char op, const char *val)
```

Defined at lines 110 to 139.

Called by: [map_expr](sparql_parser.y.md#map_expr)

Uses file-scope variables: `ssp_slq`

### map_expr

```c
static const char * map_expr(const char *attr, char op, const char *val)
```

Defined at lines 141 to 319.

Calls: [ascii_lower_dup](sparql_parser.y.md#ascii_lower_dup), [map_type_search](sparql_parser.y.md#map_type_search), [yyerror](sparql_parser.y.md#yyerror)

Uses file-scope variables: `sparqlvar`, `ssp_slq`

### yyerror

```c
void yyerror(char const *)
```

Defined at lines 321 to 328.

Called by: [map_expr](sparql_parser.y.md#map_expr)

### yywrap

```c
int yywrap()
```

Defined at lines 330 to 333.

### map_spotlight_to_sparql_query

```c
int map_spotlight_to_sparql_query(slq_t *slq, gchar **sparql_result)
```

Defined at lines 343 to 365.

Map a Spotlight RAW query string to a SPARQL query string

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
