---
type: C Source File
title: "bin/pap/papstatus.c"
description: "6 functions, includes 5 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/pap/papstatus.c"
tags: ["bin/pap"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Defined at lines 65 to 69.

Note SIGINT or SIGTERM, so the status loop stops after the current poll.

Parameters:
* `signo`: signal number, unused

Called by: [main](papstatus.c.md#main)

### usage

```c
static void usage(char *path)
```

Defined at lines 79 to 92.

Print the usage of papstatus to stderr and exit with 1.

Parameters:
* `path`: program path, whose base name the usage line shows

Called by: [main](papstatus.c.md#main)

### paprc

```c
static char * paprc(void)
```

Defined at lines 103 to 126.

Read the printer name from the first line of .

/.paprc that is not a comment

Any open error gives NULL. The name lives in a static buffer of 98 bytes.

Returns: the printer name, or NULL

Called by: [main](papstatus.c.md#main)

### print_status

```c
static void print_status(char status, char mask, char *message)
```

Defined at lines 141 to 150.

Print a label and True or False for one bit of an ImageWriter status byte.

Parameters:
* `status`: status byte
* `mask`: bit to test
* `message`: label before the value

Called by: [getstatus](papstatus.c.md#getstatus)

### main

```c
int main(int ac, char **av)
```

Defined at lines 165 to 276.

Print the status of a PAP printer once, or at a fixed interval.

The printer comes from -p or from ./.paprc as an NBP name, with the type LaserWriter and the zone `*` by default. Without an argument the tool prints the status once. A number argument sets the seconds between polls, and the query then repeats until SIGINT or SIGTERM. The -A option sets the local address.

Parameters:
* `ac`: argument count
* `av`: argument vector

Returns: 0, while errors exit with 1

Calls: [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [atp_close](../../libatalk/atp/atp_close.c.md#atp_close), [atp_open](../../libatalk/atp/atp_open.c.md#atp_open), [getstatus](papstatus.c.md#getstatus), [nbp_lookup](../../libatalk/nbp/nbp_lkup.c.md#nbp_lookup), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [paprc](papstatus.c.md#paprc), [sig_handler](papstatus.c.md#sig_handler), [usage](papstatus.c.md#usage)

### getstatus

```c
static void getstatus(ATP atp, struct sockaddr_at *sat, int is_imagewriter)
```

Defined at lines 289 to 338.

Ask a PAP printer for its status and print the status text.

For an ImageWriter the call first prints each bit of the status byte. A reply shorter than 8 bytes, or of another type, prints a message and nothing else. An ATP error exits with 1.

Parameters:
* `atp`: ATP endpoint
* `sat`: printer address
* `is_imagewriter`: nonzero for an ImageWriter

Calls: [atp_rresp](../../libatalk/atp/atp_rresp.c.md#atp_rresp), [atp_sreq](../../libatalk/atp/atp_sreq.c.md#atp_sreq), [print_status](papstatus.c.md#print_status)

Called by: [main](papstatus.c.md#main)

# Macros

* Undocumented: `COLOR_RIBBON_INSTALLED`, `COVER_OPEN_ERROR`, `IMAGEWRITER`, `IMAGEWRITER_LQ`, `PAPER_JAM_ERROR`, `PAPER_OUT_ERROR`, `PRINTER_ACTIVE`, `PRINTER_FAULT`, `PRINTER_OFF_LINE`, `SHEET_FEEDER_INSTALLED`, `_PATH_PAPRC`

# File-scope variables

`cbuf`, `nn`, `printer`, `stop_requested`
