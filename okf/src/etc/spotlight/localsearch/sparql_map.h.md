---
type: C Header File
title: "etc/spotlight/localsearch/sparql_map.h"
description: "2 types."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/localsearch/sparql_map.h"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/spotlight](../../spotlight.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Included by

* [etc/spotlight/localsearch/sparql_map.c](sparql_map.c.md)
* [etc/spotlight/localsearch/sparql_parser.y](sparql_parser.y.md)

# Types

### struct MDTypeMap

Defined at line 45.
* `const char * mdtm_value`
* `enum kMDTypeMap mdtm_type`
* `const char * mdtm_sparql`

### struct spotlight_sparql_map

Defined at line 38.
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
