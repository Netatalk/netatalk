---
type: Subsystem
title: "etc/spotlight"
description: "10 files, 68 functions."
resource: "https://github.com/Netatalk/netatalk/tree/main/etc/spotlight"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

# Files

* [etc/spotlight/cnid/sl_cnid.c](spotlight/cnid/sl_cnid.c.md): Spotlight CNID backend: file name searches in the CNID database.
* [etc/spotlight/localsearch/sl_localsearch.c](spotlight/localsearch/sl_localsearch.c.md): Spotlight LocalSearch backend: SPARQL queries to the indexer.
* [etc/spotlight/localsearch/sparql_map.c](spotlight/localsearch/sparql_map.c.md): Spotlight attribute to SPARQL mapping table.
* [etc/spotlight/localsearch/sparql_map.h](spotlight/localsearch/sparql_map.h.md): Spotlight attribute and content type mapping tables for SPARQL.
* [etc/spotlight/localsearch/sparql_parser.y](spotlight/localsearch/sparql_parser.y.md): Grammar that maps Spotlight raw queries to SPARQL.
* [etc/spotlight/localsearch/spotlight_rawquery_lexer.l](spotlight/localsearch/spotlight_rawquery_lexer.l.md): Lexer of the Spotlight raw query language.
* [etc/spotlight/sl_backends.h](spotlight/sl_backends.h.md): Operation table declarations for available Spotlight backends.
* [etc/spotlight/spotlight_private.h](spotlight/spotlight_private.h.md): Internal Spotlight file metadata, scope and CNID array helpers.
* [etc/spotlight/xapian/sl_xapian.c](spotlight/xapian/sl_xapian.c.md): Spotlight Xapian backend: index state, queries, and results.
* [etc/spotlight/xapian/sl_xapian.h](spotlight/xapian/sl_xapian.h.md): Xapian index maintenance and Spotlight query interface.

# Includes headers from

* [include/atalk](../include/atalk.md): 32 includes
* [etc/afpd](afpd.md): 3 includes

# Headers included by

* [etc/afpd](afpd.md): 3 includes

# Calls into

* [etc/afpd](afpd.md): 8 calls
* [libatalk/util](../libatalk/util.md): 5 calls
* [libatalk/compat](../libatalk/compat.md): 4 calls
* [libatalk/cnid](../libatalk/cnid.md): 2 calls

# Most called functions

* [sl_cnid_fill_results](spotlight/cnid/sl_cnid.c.md#sl_cnid_fill_results): 2 callers
* [sl_xapian_db_path](spotlight/xapian/sl_xapian.c.md#sl_xapian_db_path): 2 callers
* [sl_xapian_ensure_state_root](spotlight/xapian/sl_xapian.c.md#sl_xapian_ensure_state_root): 2 callers
* [sl_xapian_fill_results](spotlight/xapian/sl_xapian.c.md#sl_xapian_fill_results): 2 callers
* [tracker_cursor_cb](spotlight/localsearch/sl_localsearch.c.md#tracker_cursor_cb): 2 callers
* [ascii_lower_dup](spotlight/localsearch/sparql_parser.y.md#ascii_lower_dup): 1 callers
* [cnid32_comp_fn](spotlight/cnid/sl_cnid.c.md#cnid32_comp_fn): 1 callers
* [cnid_comp_fn](spotlight/cnid/sl_cnid.c.md#cnid_comp_fn): 1 callers
* [cnid_comp_fn](spotlight/localsearch/sl_localsearch.c.md#cnid_comp_fn): 1 callers
* [cnid_comp_fn](spotlight/xapian/sl_xapian.c.md#cnid_comp_fn): 1 callers

# Functions without a static caller

No call, table or documentation edge reaches these functions in the scanned sources. Entry points reached through dlopen, signals, callbacks passed as arguments, or code outside the scanned directories appear here too.

[isodate2unix](spotlight/localsearch/sparql_parser.y.md#isodate2unix), [map_daterange](spotlight/localsearch/sparql_parser.y.md#map_daterange), [map_expr](spotlight/localsearch/sparql_parser.y.md#map_expr), [yylex](spotlight/localsearch/spotlight_rawquery_lexer.l.md#yylex), [yywrap](spotlight/localsearch/sparql_parser.y.md#yywrap)
