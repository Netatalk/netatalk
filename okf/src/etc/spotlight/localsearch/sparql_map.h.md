---
type: C Header File
title: "etc/spotlight/localsearch/sparql_map.h"
description: "Spotlight attribute and content type mapping tables for SPARQL."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/localsearch/sparql_map.h"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-16T07:37:38+02:00 }
---

Part of the [etc/spotlight](../../spotlight.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Included by

* [etc/spotlight/localsearch/sparql_map.c](sparql_map.c.md)
* [etc/spotlight/localsearch/sparql_parser.y](sparql_parser.y.md)

# Types

### struct MDTypeMap

Defined at line 50.
* `const char * mdtm_value`: MD query value of attributes '_kMDItemGroupId' and 'kMDItemContentTypeTree'.
* `enum kMDTypeMap mdtm_type`: whether SPARQL query must search attribute rdf:type or nie:mime_Type
* `const char * mdtm_sparql`: the SPARQL query match string

### struct spotlight_sparql_map

Defined at line 43.
* `const char * ssm_spotlight_attr`
* `bool ssm_enabled`
* `enum ssm_type ssm_type`
* `const char * ssm_sparql_attr`

# Typedefs and enums

* `enum kMDTypeMap`: `kMDTypeMapNotSup`, `kMDTypeMapRDF`, `kMDTypeMapMime`
* `enum ssm_type`: `ssmt_bool`, `ssmt_num`, `ssmt_str`, `ssmt_fname`, `ssmt_fts`, `ssmt_date`, `ssmt_type`

# Macros

* Undocumented: `SPOTLIGHT_SPARQL_MAP_H`

# File-scope variables

`spotlight_sparql_date_map`
