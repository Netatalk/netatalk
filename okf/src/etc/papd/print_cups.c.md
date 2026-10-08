---
type: C Source File
title: "etc/papd/print_cups.c"
description: "12 functions, includes 8 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/print_cups.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/pap.h](../../include/atalk/pap.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [print_cups.h](print_cups.h.md)
* [printer.h](printer.h.md)
* System headers: `ctype.h`, `cups/cups.h`, `cups/ipp.h`, `cups/language.h`, `errno.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/param.h`, `sys/types.h`, `unistd.h`

# Functions

### cups_get_language

```c
const char * cups_get_language(void)
```

Defined at lines 84 to 96.

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers), [lp_print](lp.c.md#lp_print)

### cups_passwd_cb

```c
static const char * cups_passwd_cb(const char *prompt, http_t *http, const char *method, const char *resource, void *user_data)
```

Defined at lines 104 to 114.

The CUPS password callback...

Note: O - Password or NULL

Note: I - Prompt

Returns: NULL (not implemented)

Called by: [cups_get_printer_ppd](print_cups.c.md#cups_get_printer_ppd), [cups_get_printer_status](print_cups.c.md#cups_get_printer_status), [cups_print_job](print_cups.c.md#cups_print_job), [cups_printername_ok](print_cups.c.md#cups_printername_ok)

### cups_printername_ok

```c
int cups_printername_ok(char *name)
```

Defined at lines 123 to 159.

Verify supplied printer name is a valid cups printer.

Note: O - 1 if printer name OK

Note: I - Name of printer

Calls: [cups_passwd_cb](print_cups.c.md#cups_passwd_cb)

Called by: [rprintcap](main.c.md#rprintcap)

### cups_get_printer_ppd

```c
const char * cups_get_printer_ppd(char *name)
```

Defined at lines 161 to 356.

Calls: [cups_passwd_cb](print_cups.c.md#cups_passwd_cb)

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers), [rprintcap](main.c.md#rprintcap)

### cups_get_printer_status

```c
int cups_get_printer_status(struct printer *pr)
```

Defined at lines 359 to 521.

Calls: [cups_passwd_cb](print_cups.c.md#cups_passwd_cb), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat)

Called by: [getstatus](main.c.md#getstatus), [lp_init](lp.c.md#lp_init), [main](main.c.md#main)

Uses file-scope variables: `cups_status_msg`

### cups_print_job

```c
int cups_print_job(char *name, const char *filename, char *job, char *username, char *cupsoptions)
```

Defined at lines 528 to 629.

pass the job to cups

Calls: [cups_passwd_cb](print_cups.c.md#cups_passwd_cb), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [lp_print](lp.c.md#lp_print)

### cups_autoadd_printers

```c
struct printer * cups_autoadd_printers(struct printer *defprinter, struct printer *printers)
```

Defined at lines 635 to 709.

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [convert_to_mac_name](print_cups.c.md#convert_to_mac_name), [cups_check_printer](print_cups.c.md#cups_check_printer), [cups_free_printer](print_cups.c.md#cups_free_printer), [cups_get_language](print_cups.c.md#cups_get_language), [cups_get_printer_ppd](print_cups.c.md#cups_get_printer_ppd), [cups_mangle_printer_name](print_cups.c.md#cups_mangle_printer_name)

Called by: [getprinters](main.c.md#getprinters)

### cups_mangle_printer_name

```c
static int cups_mangle_printer_name(struct printer *pr, struct printer *printers)
```

Defined at lines 720 to 754.

Mangles the printer name if two CUPS printer provide the same Chooser Name.

Note: Append '[nn](../../bin/pap/pap.c.md)' to the chooser name, if it is longer than 28 char we overwrite the last three chars

Returns: 0 on Success, 2 on Error

Calls: [cups_check_printer](print_cups.c.md#cups_check_printer), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers)

### to_ascii

```c
static size_t to_ascii(char *inbuf, char **outbuf)
```

Defined at lines 761 to 785.

fallback ASCII conversion

Called by: [convert_to_mac_name](print_cups.c.md#convert_to_mac_name)

### convert_to_mac_name

```c
static int convert_to_mac_name(const char *encoding, char *inptr, char *outptr, size_t outlen)
```

Defined at lines 798 to 836.

Convert to Mac printer name.

Note: 1) Convert from encoding to MacRoman

Note: 2) Shorten to MAXCHOOSERLEN (31)

Note: 3) Replace @ and _ as they are illegal

Returns: -1 on failure, length of name on success; outpr contains name in MacRoman

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [to_ascii](print_cups.c.md#to_ascii)

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers)

### cups_check_printer

```c
int cups_check_printer(struct printer *pr, struct printer *printers, int replace)
```

Defined at lines 848 to 891.

check if a printer with this name already exists.

Note: if yes, and replace = 1 the existing printer is replaced with the new one. This allows to overwrite printer settings

Note: created by cupsautoadd. It also used by cups_mangle_printer.

Calls: [cups_free_printer](print_cups.c.md#cups_free_printer)

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers), [cups_mangle_printer_name](print_cups.c.md#cups_mangle_printer_name), [getprinters](main.c.md#getprinters)

### cups_free_printer

```c
static void cups_free_printer(struct printer *pr)
```

Defined at lines 898 to 939.

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers), [cups_check_printer](print_cups.c.md#cups_check_printer)

# Typedefs and enums

* `typedef int cups_len_t`

# Macros

* Undocumented: `MAXCHOOSERLEN`, `cupsCopyDestInfo`, `cupsCreateTempFile`, `cupsGetDests`, `cupsGetError`, `cupsGetErrorString`, `cupsParseOptions`, `cupsSetPasswordCB`

# File-scope variables

`cups_status_msg`
