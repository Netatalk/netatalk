---
type: C Source File
title: "bin/netacnv/netacnv.c"
description: "The netacnv command: convert a string between two charsets."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/netacnv/netacnv.c"
tags: ["bin/netacnv"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-13T22:34:30+02:00 }
---

Part of the [bin/netacnv](../netacnv.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* System headers: `errno.h`, `stdint.h`, `stdio.h`, `stdlib.h`, `string.h`, `unistd.h`

# Functions

### usage

```c
static void usage(FILE *stream)
```

Defined at lines 62 to 79.

Print the usage of netacnv, its defaults, and the names of its conversion options to the chosen stream.

Parameters:
* `stream`: stdout for help, stderr for a usage error

Called by: [main](netacnv.c.md#main)

### validate_charset

```c
static int validate_charset(const char *name, const char *role)
```

Defined at lines 92 to 114.

Validate both conversion directions before registering a charset.

add_charset returns an existing fixed slot by name without checking that its descriptors loaded. Probe the actual converters first so a matching name alone does not make an unusable charset appear valid.

Parameters:
* `name`: charset name
* `role`: name of the command-line option for an error message

Returns: 0 on success, or -1 after a diagnostic

Calls: [atalk_iconv_close](../../libatalk/unicode/iconv.c.md#atalk_iconv_close), [atalk_iconv_open](../../libatalk/unicode/iconv.c.md#atalk_iconv_open)

Called by: [main](netacnv.c.md#main)

### decode_hex_input

```c
static int decode_hex_input(const char *text, char *bytes, size_t *length)
```

Defined at lines 124 to 175.

Decode hexadecimal byte pairs, optionally separated by ASCII whitespace.

Parameters:
* `text`: command-line hex input
* `bytes`: buffer with MAXPATHLEN bytes and a trailing NUL slot
* `length`: decoded byte count

Returns: 0 on success, or -1 after a diagnostic

Called by: [main](netacnv.c.md#main)

### print_bytes

```c
static void print_bytes(const char *label, const char *bytes, size_t length)
```

Defined at lines 184 to 193.

Print exact bytes and their length without interpreting the charset.

Parameters:
* `label`: label for the diagnostic line
* `bytes`: byte buffer
* `length`: byte count, excluding the terminator

Called by: [main](netacnv.c.md#main)

### print_flags

```c
static void print_flags(uint16_t flags)
```

Defined at lines 200 to 219.

Print option and result flags returned by convert_charset.

Parameters:
* `flags`: conversion flags after the call

Called by: [main](netacnv.c.md#main)

### main

```c
int main(int argc, char **argv)
```

Defined at lines 238 to 402.

Convert a string between two charsets with convert_charset and print both forms.

The -f and -t options name the source and the target charsets. Their defaults are UTF8-MAC and UTF8. The -m option names the legacy Mac charset, MAC_ROMAN by default. Each -o option adds a CONV_* flag by its name; unknown names and conflicting case or composition options fail. -x decodes hexadecimal byte pairs, allowing embedded NULs and wide input. The input and output are bounded by MAXPATHLEN bytes. -v prints byte lengths, hex bytes, and result flags to stderr; -h prints help.

Parameters:
* `argc`: argument count
* `argv`: argument vector

Returns: EXIT_SUCCESS, or EXIT_FAILURE for an argument, charset, conversion, or output error

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [convert_charset](../../libatalk/unicode/charcnv.c.md#convert_charset), [decode_hex_input](netacnv.c.md#decode_hex_input), [free_charset_names](../../libatalk/unicode/charcnv.c.md#free_charset_names), [print_bytes](netacnv.c.md#print_bytes), [print_flags](netacnv.c.md#print_flags), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name), [usage](netacnv.c.md#usage), [validate_charset](netacnv.c.md#validate_charset)

# Types

### struct flag_map

Defined at line 38.
* `uint16_t flag`
* `const char * flagname`

# Macros

* Undocumented: `FLAG`, `MACCHARSET`

# File-scope variables

`flag_map`
