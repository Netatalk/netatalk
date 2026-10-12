---
type: C Source File
title: "bin/pap/pap.c"
description: "The pap command: print files on a PAP printer."
resource: "https://github.com/Netatalk/netatalk/blob/main/bin/pap/pap.c"
tags: ["bin/pap"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
---

Part of the [bin/pap](../pap.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/pap.h](../../include/atalk/pap.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* System headers: `errno.h`, `fcntl.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`

# Functions

### copy_status

```c
static int copy_status(const struct iovec *response, char *status_buf, size_t status_buf_size, size_t *status_len)
```

Defined at lines 57 to 78.

Extract the status string of a PAP Status reply into a buffer.

The string length sits in byte 8 of the reply, and the text follows. A reply shorter than its header, or a string that does not fit the reply or the buffer, fails.

Parameters:
* `response`: reply data
* `status_buf`: receives the NUL-terminated string
* `status_buf_size`: size of `status_buf`
* `status_len`: receives the string length

Returns: 0, or -1 for a malformed reply

Called by: [main](pap.c.md#main), [send_file](pap.c.md#send_file)

### usage

```c
static void usage(char *path)
```

Defined at lines 85 to 110.

Print the usage and the options of pap to stderr and exit with 2.

Parameters:
* `path`: program path, whose base name the usage line shows

Called by: [main](pap.c.md#main)

### paprc

```c
static char * paprc(void)
```

Defined at lines 122 to 150.

Read the printer name from the first line of .

/.paprc that is not a comment

A missing file gives NULL, and another open error exits with 2. The name lives in a static buffer of 98 bytes.

Returns: the printer name, or NULL

Called by: [main](pap.c.md#main)

### main

```c
int main(int ac, char **av)
```

Defined at lines 232 to 583.

Print files on a PAP printer.

The printer comes from -p or from ./.paprc as an NBP name, with the type LaserWriter and the zone `*` by default. The tool looks the printer up, opens a PAP connection, sends each file, or stdin without files, and closes the connection.

The -W option waits for an idle status first, and -w waits for a waiting status before the data. The -c option claims the longest wait time in the open request, which moves the job ahead in the printer queue. The -E option skips the wait for the printer EOF after the last file, and -s writes each status to a file. The -A option sets the local address, -d prints the protocol steps, and -e sends stdout to stderr.

Parameters:
* `ac`: argument count
* `av`: argument vector

Returns: does not return, and exits with 0, 1 for a protocol error, or 2 for a usage or system error

Calls: [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [atp_open](../../libatalk/atp/atp_open.c.md#atp_open), [atp_rresp](../../libatalk/atp/atp_rresp.c.md#atp_rresp), [atp_sreq](../../libatalk/atp/atp_sreq.c.md#atp_sreq), [copy_status](pap.c.md#copy_status), [nbp_lookup](../../libatalk/nbp/nbp_lkup.c.md#nbp_lookup), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [paprc](pap.c.md#paprc), [send_file](pap.c.md#send_file), [updatestatus](pap.c.md#updatestatus), [usage](pap.c.md#usage)

Uses file-scope variables: `cbuf`, `connid`, `debug`, `nn`, `noeof`, `oquantum`, `quantum`, `rniov`, `sat`, `satp`, `status`, `waitforprinter`

### send_file

```c
static int send_file(int fd, ATP atp, int lastfile, int is_imagewriter)
```

Defined at lines 608 to 1048.

Send one file over the open PAP connection and pass the printer output to stdout.

The call asks the printer for its output with PAP Read requests. Each Read request that the printer sends gets up to quantum packets of file data in reply. The call sends a tickle every 60 seconds and exits after four select timeouts in a row without a valid request. While no data can go out, the call asks for the printer status. A Close request from the printer exits with 1.

Parameters:
* `fd`: open file, 0 for stdin
* `atp`: ATP endpoint of the connection
* `lastfile`: nonzero for the last file of the job
* `is_imagewriter`: nonzero for an ImageWriter, whose status replies skip the header checks

Returns: 0 after the EOF of the printer, or after the last file under -E, while errors exit

Calls: [atp_rreq](../../libatalk/atp/atp_rreq.c.md#atp_rreq), [atp_rresp](../../libatalk/atp/atp_rresp.c.md#atp_rresp), [atp_rsel](../../libatalk/atp/atp_rsel.c.md#atp_rsel), [atp_sreq](../../libatalk/atp/atp_sreq.c.md#atp_sreq), [atp_sresp](../../libatalk/atp/atp_sresp.c.md#atp_sresp), [copy_status](pap.c.md#copy_status), [updatestatus](pap.c.md#updatestatus)

Called by: [main](pap.c.md#main)

Uses file-scope variables: `cbuf`, `connid`, `debug`, `nn`, `noeof`, `oquantum`, `port`, `quantum`, `rfiov`, `rniov`, `sat`, `satp`, `seq`, `sfiov`, `sniov`, `waitforprinter`

### updatestatus

```c
static void updatestatus(char *s, int len)
```

Defined at lines 1060 to 1090.

Write a printer status line as `%%[ status ]%%`.

The line goes to the -s status file, which the call truncates first, or to stderr. A status file that does not open switches all later lines to stderr.

Parameters:
* `s`: status text
* `len`: length of `s`

Called by: [main](pap.c.md#main), [send_file](pap.c.md#send_file)

Uses file-scope variables: `status`

# Macros

* Undocumented: `IMAGEWRITER`, `IMAGEWRITER_LQ`, `PAP_STATUS_BUFFER_SIZE`, `PAP_STATUS_HEADER_SIZE`, `_PATH_PAPRC`

# File-scope variables

`cbuf`, `connid`, `data`, `debug`, `fbuf`, `nbuf`, `nn`, `noeof`, `oquantum`, `port`, `printer`, `quantum`, `rfiov`, `rniov`, `sat`, `satp`, `seq`, `sfiov`, `sniov`, `status`, `waitforprinter`
