---
type: C Source File
title: "bin/pap/papstatus.c"
description: "6 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/pap/papstatus.c"
tags: ["bin/pap"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [bin/pap](../pap.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/pap.h](../../include/atalk/pap.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `errno.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/file.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### sig_handler

```c
static void sig_handler(int signo)
```

Defined at lines 59 to 63.

Called by: [main](papstatus.c.md#main)

Uses file-scope variables: `stop_requested`

### usage

```c
static void usage(char *path)
```

Defined at lines 68 to 81.

Called by: [main](papstatus.c.md#main)

### paprc

```c
static char * paprc(void)
```

Defined at lines 84 to 107.

Called by: [main](papstatus.c.md#main)

### print_status

```c
static void print_status(char status, char mask, char *message)
```

Defined at lines 114 to 123.

Called by: [getstatus](papstatus.c.md#getstatus)

### main

```c
int main(int ac, char **av)
```

Defined at lines 125 to 236.

Calls: [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [atp_close](../../libatalk/atp/atp_close.c.md#atp_close), [atp_open](../../libatalk/atp/atp_open.c.md#atp_open), [getstatus](papstatus.c.md#getstatus), [nbp_lookup](../../libatalk/nbp/nbp_lkup.c.md#nbp_lookup), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [paprc](papstatus.c.md#paprc), [sig_handler](papstatus.c.md#sig_handler), [usage](papstatus.c.md#usage)

Uses file-scope variables: `nn`, `stop_requested`

### getstatus

```c
static void getstatus(ATP atp, struct sockaddr_at *sat, int is_imagewriter)
```

Defined at lines 238 to 287.

Calls: [atp_rresp](../../libatalk/atp/atp_rresp.c.md#atp_rresp), [atp_sreq](../../libatalk/atp/atp_sreq.c.md#atp_sreq), [print_status](papstatus.c.md#print_status)

Called by: [main](papstatus.c.md#main)

Uses file-scope variables: `cbuf`

# Macros

* Undocumented: `COLOR_RIBBON_INSTALLED`, `COVER_OPEN_ERROR`, `IMAGEWRITER`, `IMAGEWRITER_LQ`, `PAPER_JAM_ERROR`, `PAPER_OUT_ERROR`, `PRINTER_ACTIVE`, `PRINTER_FAULT`, `PRINTER_OFF_LINE`, `SHEET_FEEDER_INSTALLED`, `_PATH_PAPRC`

# File-scope variables

`cbuf`, `nn`, `printer`, `stop_requested`
