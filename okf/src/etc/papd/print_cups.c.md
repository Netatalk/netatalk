---
type: C Source File
title: "etc/papd/print_cups.c"
description: "CUPS printing backend of papd."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/print_cups.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 94 to 106.

Return the name of the charset of the default CUPS language.

Returns: the encoding name that CUPS reports, for add_charset

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers), [lp_print](lp.c.md#lp_print)

### cups_passwd_cb

```c
static const char * cups_passwd_cb(const char *prompt, http_t *http, const char *method, const char *resource, void *user_data)
```

Defined at lines 119 to 129.

Refuse every CUPS password prompt, so a CUPS request never waits for a password.

Parameters:
* `prompt`: prompt text, unused
* `http`: connection, unused
* `method`: HTTP method, unused
* `resource`: resource path, unused
* `user_data`: callback data, unused

Returns: NULL, no password

Called by: [cups_get_printer_ppd](print_cups.c.md#cups_get_printer_ppd), [cups_get_printer_status](print_cups.c.md#cups_get_printer_status), [cups_print_job](print_cups.c.md#cups_print_job), [cups_printername_ok](print_cups.c.md#cups_printername_ok)

### cups_printername_ok

```c
int cups_printername_ok(char *name)
```

Defined at lines 139 to 175.

Check that a CUPS destination with this name exists and accepts a connection.

Parameters:
* `name`: CUPS printer name

Returns: 1 for a usable printer, or 0

Calls: [cups_passwd_cb](print_cups.c.md#cups_passwd_cb)

Called by: [rprintcap](main.c.md#rprintcap)

### cups_get_printer_ppd

```c
const char * cups_get_printer_ppd(char *name)
```

Defined at lines 188 to 383.

Write a PPD file for a CUPS printer from its IPP attributes.

The file holds fixed PostScript level 3 defaults plus the make, model, and color support of the printer. A printer that CUPS only knows from DNS-SD gets the query directly, through its device URI.

Parameters:
* `name`: CUPS printer name

Returns: the path of the temporary PPD file in a static buffer, or NULL on failure

Calls: [cups_passwd_cb](print_cups.c.md#cups_passwd_cb)

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers), [rprintcap](main.c.md#rprintcap)

### cups_get_printer_status

```c
int cups_get_printer_status(struct printer *pr)
```

Defined at lines 397 to 559.

Ask CUPS for the state of a printer and store a PAP status line for it.

The line in p_status reads like `status: idle; info: "name" is ready`. A printer that does not answer the query counts as busy but keeps taking jobs, since the CUPS scheduler queues them.

Parameters:
* `pr`: printer, receives the status line

Returns: 0 when the printer rejects jobs or is unreachable, 1 when stopped or not answering, 2 when ready, or 3 when printing

Calls: [cups_passwd_cb](print_cups.c.md#cups_passwd_cb), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat)

Called by: [getstatus](main.c.md#getstatus), [lp_init](lp.c.md#lp_init), [main](main.c.md#main)

Uses file-scope variables: `cups_status_msg`

### cups_print_job

```c
int cups_print_job(char *name, const char *filename, char *job, char *username, char *cupsoptions)
```

Defined at lines 574 to 675.

Submit a spooled data file to CUPS as a job.

The call sends the data file `filename` of the spool directory to the CUPS printer `name` as the job `job`. The job carries `username` as the originating user and the CUPS options of the co entry, when they are not NULL. CUPS picks the document format, and a failed send only logs.

Returns: the CUPS job ID, or 0 for an unknown printer or a missing data file

Calls: [cups_passwd_cb](print_cups.c.md#cups_passwd_cb), [strlcat](../../libatalk/compat/strlcpy.c.md#strlcat), [strlcpy](../../libatalk/compat/strlcpy.c.md#strlcpy)

Called by: [lp_print](lp.c.md#lp_print)

### cups_autoadd_printers

```c
struct printer * cups_autoadd_printers(struct printer *defprinter, struct printer *printers)
```

Defined at lines 694 to 768.

Add a printer entry for every CUPS destination, for the cupsautoadd entry of papd.conf.

Each entry takes the settings of the cupsautoadd entry and a PPD file from cups_get_printer_ppd. Its NBP name comes from convert_to_mac_name. A name that clashes with another added printer gets a `#nn` suffix, and one that clashes with a printer of papd.conf goes unused.

Parameters:
* `defprinter`: cupsautoadd entry that supplies the settings
* `printers`: printer list

Returns: the printer list with the added printers in front

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [convert_to_mac_name](print_cups.c.md#convert_to_mac_name), [cups_check_printer](print_cups.c.md#cups_check_printer), [cups_free_printer](print_cups.c.md#cups_free_printer), [cups_get_language](print_cups.c.md#cups_get_language), [cups_get_printer_ppd](print_cups.c.md#cups_get_printer_ppd), [cups_mangle_printer_name](print_cups.c.md#cups_mangle_printer_name)

Called by: [getprinters](main.c.md#getprinters)

### cups_mangle_printer_name

```c
static int cups_mangle_printer_name(struct printer *pr, struct printer *printers)
```

Defined at lines 784 to 818.

Give an added printer a free NBP name by appending `#01` to `#99`.

The name keeps at most 28 bytes before the suffix, so it stays within the 31 bytes of a Chooser name.

Parameters:
* `pr`: added printer, receives the new name
* `printers`: printer list

Returns: 0 for a free name, or 2 when every suffix clashes or the allocation fails

Calls: [cups_check_printer](print_cups.c.md#cups_check_printer), [strnlen](../../libatalk/compat/misc.c.md#strnlen)

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers)

### to_ascii

```c
static size_t to_ascii(char *inptr, char **outptr)
```

Defined at lines 833 to 857.

Make an ASCII version of a name by turning every byte above 0x7f into `_`.

A failed allocation exits papd.

Parameters:
* `inptr`: NUL-terminated name
* `outptr`: receives the new name, to free

Returns: the length of the new name

Called by: [convert_to_mac_name](print_cups.c.md#convert_to_mac_name)

### convert_to_mac_name

```c
static int convert_to_mac_name(const char *encoding, char *inptr, char *outptr, size_t outlen)
```

Defined at lines 875 to 913.

Turn a CUPS printer name into a Chooser name in Mac Roman.

The name goes from the CUPS charset to Mac Roman, or through to_ascii when that fails. The call cuts it to outlen - 1 bytes, turns `_` into a space, and turns `@` and `:` into `_`, since NBP names cannot hold them.

Parameters:
* `encoding`: charset of the CUPS name
* `inptr`: CUPS name
* `outptr`: receives the Chooser name
* `outlen`: size of outptr

Returns: the length of the Chooser name

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [to_ascii](print_cups.c.md#to_ascii)

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers)

### cups_check_printer

```c
int cups_check_printer(struct printer *pr, struct printer *printers, int replace)
```

Defined at lines 933 to 976.

Check whether the printer list holds a printer with the same name, ignoring case.

For an added printer, a clash with another added printer returns -1 and a clash with a printer of papd.conf returns 1. Otherwise, with replace set, the new printer takes the place of the old one in the list. The call frees the old one, so a papd.conf entry overrides an added printer.

Parameters:
* `pr`: new printer
* `printers`: printer list
* `replace`: nonzero to replace a printer of the same name

Returns: 0 without a clash, 1 for a clash, or -1 for a clash between added printers

Calls: [cups_free_printer](print_cups.c.md#cups_free_printer)

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers), [cups_mangle_printer_name](print_cups.c.md#cups_mangle_printer_name), [getprinters](main.c.md#getprinters)

### cups_free_printer

```c
static void cups_free_printer(struct printer *pr)
```

Defined at lines 991 to 1032.

Free a printer entry and its strings.

An added printer shares its other strings with the cupsautoadd entry, so the call frees only its name, printer, and PPD file.

Parameters:
* `pr`: printer entry

Called by: [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers), [cups_check_printer](print_cups.c.md#cups_check_printer)

# Typedefs and enums

* `typedef int cups_len_t`

# Macros

* Undocumented: `MAXCHOOSERLEN`, `cupsCopyDestInfo`, `cupsCreateTempFile`, `cupsGetDests`, `cupsGetError`, `cupsGetErrorString`, `cupsParseOptions`, `cupsSetPasswordCB`

# File-scope variables

`cups_status_msg`
