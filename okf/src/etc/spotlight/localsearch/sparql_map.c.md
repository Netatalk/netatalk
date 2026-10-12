---
type: C Source File
title: "etc/spotlight/localsearch/sparql_map.c"
description: "Spotlight attribute to SPARQL mapping table."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/localsearch/sparql_map.c"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [etc/spotlight](../../spotlight.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

# Includes

* [atalk/logger.h](../../../include/atalk/logger.h.md)
* [sparql_map.h](sparql_map.h.md)
* System headers: `stdlib.h`, `string.h`, `unistd.h`

# Functions

### configure_spotlight_attributes

```c
void configure_spotlight_attributes(const char *attributes_in)
```

Defined at lines 124 to 151.

Turn on only the Spotlight attributes that the spotlight attributes option lists.

The option is a comma-separated list, matched exactly against the attribute names of the SPARQL map, without trimming spaces. Every other attribute of the map turns off.

Parameters:
* `attributes_in`: comma-separated attribute names

Called by: [sl_localsearch_init](sl_localsearch.c.md#sl_localsearch_init)

# Macros

* Undocumented: `NOTSUPPORTED`, `SPECIAL`

# File-scope variables

`MDTypeMap`, `spotlight_sparql_map`
