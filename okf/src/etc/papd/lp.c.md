---
type: C Source File
title: "etc/papd/lp.c"
description: "19 functions, 1 type, includes 8 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/lp.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [file.h](file.h.md)
* [lp.h](lp.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [print_cups.h](print_cups.h.md)
* [printer.h](printer.h.md)
* System headers: `ctype.h`, `fcntl.h`, `netdb.h`, `netinet/in.h`, `pwd.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/file.h`, `sys/param.h`, `sys/socket.h`, `sys/stat.h`, `sys/time.h`, `sys/un.h`, `unistd.h`

# Functions

### rresvport

```c
int rresvport(int *alport)
```

Defined at lines 108 to 161.

### lp_conn_inet

```c
int lp_conn_inet()
```

Declared at etc/papd/lp.c line 165; no definition in the scanned sources.

### lp_disconn_inet

```c
int lp_disconn_inet(int)
```

Declared at etc/papd/lp.c line 166; no definition in the scanned sources.

### lp_conn_unix

```c
int lp_conn_unix()
```

Declared at etc/papd/lp.c line 167; no definition in the scanned sources.

Called by: [lp_print](lp.c.md#lp_print)

### lp_disconn_unix

```c
int lp_disconn_unix(int)
```

Declared at etc/papd/lp.c line 168; no definition in the scanned sources.

Called by: [lp_print](lp.c.md#lp_print)

### lp_origin

```c
void lp_origin(int origin)
```

Defined at lines 193 to 196.

Called by: [ch_creator](headers.c.md#ch_creator)

### convert_octal

```c
static void convert_octal(char *string, charset_t dest)
```

Defined at lines 199 to 238.

Called by: [translate](lp.c.md#translate)

### translate

```c
static void translate(charset_t from, charset_t dest, char **option)
```

Defined at lines 241 to 256.

Calls: [convert_octal](lp.c.md#convert_octal), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate)

Called by: [lp_setup_comments](lp.c.md#lp_setup_comments)

### lp_setup_comments

```c
static void lp_setup_comments(charset_t dest)
```

Defined at lines 259 to 287.

Calls: [translate](lp.c.md#translate)

Called by: [lp_open](lp.c.md#lp_open), [lp_print](lp.c.md#lp_print)

### lp_person

```c
void lp_person(char *person)
```

Defined at lines 398 to 410.

Called by: [cq_rbilogin](queries.c.md#cq_rbilogin), [lp_init](lp.c.md#lp_init)

### lp_host

```c
void lp_host(char *host)
```

Defined at lines 431 to 444.

### lp_job

```c
void lp_job(char *job)
```

Defined at lines 451 to 459.

Called by: [ch_title](headers.c.md#ch_title)

### lp_for

```c
void lp_for(char *lpfor)
```

Defined at lines 461 to 468.

Called by: [ch_for](headers.c.md#ch_for)

### lp_init

```c
static int lp_init(struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 471 to 660.

Calls: [cups_get_printer_status](print_cups.c.md#cups_get_printer_status), [lp_person](lp.c.md#lp_person), [spoolerror](file.c.md#spoolerror)

Called by: [lp_open](lp.c.md#lp_open)

Uses file-scope variables: `hostname`, `sat`

### lp_open

```c
int lp_open(struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 662 to 777.

open a file for spooling

Calls: [lp_init](lp.c.md#lp_init), [lp_print](lp.c.md#lp_print), [lp_setup_comments](lp.c.md#lp_setup_comments), [spoolerror](file.c.md#spoolerror)

Called by: [ps](magics.c.md#ps)

Uses file-scope variables: `hostname`, `sat`

### lp_close

```c
int lp_close(void)
```

Defined at lines 779 to 790.

close current spooling file

Called by: [lp_cancel](lp.c.md#lp_cancel), [lp_print](lp.c.md#lp_print), [parser_error](magics.c.md#parser_error), [ps](magics.c.md#ps)

### lp_write

```c
int lp_write(struct papfile *in, char *buf, size_t len)
```

Defined at lines 794 to 876.

open a buffer to the current open file

Calls: [lp_print](lp.c.md#lp_print)

Called by: [ch_creator](headers.c.md#ch_creator), [ch_endcomm](headers.c.md#ch_endcomm), [ch_endtranslate](headers.c.md#ch_endtranslate), [ch_for](headers.c.md#ch_for), [ch_starttranslate](headers.c.md#ch_starttranslate), [ch_title](headers.c.md#ch_title), [ch_translateone](headers.c.md#ch_translateone), [cm_psadobe](magics.c.md#cm_psadobe), [ps](magics.c.md#ps)

### lp_cancel

```c
int lp_cancel(void)
```

Defined at lines 878 to 907.

cancel current job

Calls: [lp_close](lp.c.md#lp_close)

Called by: [session](session.c.md#session)

Uses file-scope variables: `hostname`

### lp_print

```c
int lp_print(void)
```

Defined at lines 915 to 1048.

Create printcap control file, signal printer.

XXX piped?

Note: Errors here should remove queue files.

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [cups_get_language](print_cups.c.md#cups_get_language), [cups_print_job](print_cups.c.md#cups_print_job), [lp_close](lp.c.md#lp_close), [lp_conn_unix](lp.c.md#lp_conn_unix), [lp_disconn_unix](lp.c.md#lp_disconn_unix), [lp_setup_comments](lp.c.md#lp_setup_comments)

Called by: [lp_open](lp.c.md#lp_open), [lp_write](lp.c.md#lp_write), [session](session.c.md#session)

Uses file-scope variables: `hostname`

# Types

### struct lp

Defined at line 174.
* `int lp_flags`
* `FILE * lp_stream`
* `int lp_seq`
* `int lp_origin`
* `char lp_letter`
* `char * lp_person`
* `char * lp_created_for`
* `char * lp_host`
* `char * lp_job`
* `char * lp_spoolfile`

# Macros

* Undocumented: `BUFSIZE`, `LP_CONNECT`, `LP_INIT`, `LP_JOBPENDING`, `LP_OPEN`, `LP_PIPE`, `LP_QUEUE`, `is_var`

# File-scope variables

`hostname`, `lp`, `sat`
