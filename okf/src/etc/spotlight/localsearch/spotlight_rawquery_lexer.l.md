---
type: C Source File
title: "etc/spotlight/localsearch/spotlight_rawquery_lexer.l"
description: "1 function, includes 1 project header."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/spotlight/localsearch/spotlight_rawquery_lexer.l"
tags: ["etc/spotlight"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:42:54+02:00 }
---

Part of the [etc/spotlight](../../spotlight.md) subsystem. Built from the commit recorded in [build](../../../../build.md).

Line numbers refer to the view produced by the Doxygen input filter for this file type, not to the file itself.

# Includes

* [atalk/spotlight.h](../../../include/atalk/spotlight.h.md)
* System headers: `gio/gio.h`, `sparql_parser.h`, `stdbool.h`, `talloc.h`

# Functions

### yylex

```c
int yylex(void)
```

Defined at lines 39 to 57. Declared in [etc/spotlight/localsearch/sparql_parser.y](sparql_parser.y.md).

Split a Spotlight raw query string into tokens for the SPARQL parser.

A word is a run of ASCII letters, digits, and the characters _ * : - ., or of UTF-8 multibyte sequences. Quotes, braces, the comparison and logic operators, InRange, and $time.iso are separate tokens. Spaces, tabs, and newlines separate tokens.

Returns: the next token, or 0 at the end of the string

# Macros

* Undocumented: `SLQ_VAR`
