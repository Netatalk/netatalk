---
type: C Source File
title: "etc/papd/main.c"
description: "10 functions, 1 type, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/main.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [etc/papd](../papd.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/atp.h](../../include/atalk/atp.h.md)
* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/nbp.h](../../include/atalk/nbp.h.md)
* [atalk/pap.h](../../include/atalk/pap.h.md)
* [atalk/unicode.h](../../include/atalk/unicode.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* [netatalk/at.h](../../sys/netatalk/at.h.md)
* [print_cups.h](print_cups.h.md)
* [printcap.h](printcap.h.md)
* [printer.h](printer.h.md)
* [session.h](session.h.md)
* [uam_auth.h](uam_auth.h.md)
* System headers: `errno.h`, `fcntl.h`, `netdb.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/file.h`, `sys/param.h`, `sys/socket.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`, `sys/wait.h`, `unistd.h`

# Functions

### sig_handler

```c
static void sig_handler(int signo)
```

Defined at lines 85 to 89.

Called by: [main](main.c.md#main)

Uses file-scope variables: `stop_requested`

### papd_exit

```c
static void papd_exit(const int i)
```

Defined at lines 98 to 103.

Calls: [auth_unload](auth.c.md#auth_unload)

Called by: [die](main.c.md#die), [main](main.c.md#main), [papd_cleanup](main.c.md#papd_cleanup)

### papd_cleanup

```c
static void papd_cleanup(int n)
```

Defined at lines 105 to 133.

Calls: [nbp_unrgstr](../../libatalk/nbp/nbp_unrgstr.c.md#nbp_unrgstr), [papd_exit](main.c.md#papd_exit)

Called by: [die](main.c.md#die), [main](main.c.md#main)

Uses file-scope variables: `printers`

### die

```c
static void die(int n)
```

Defined at lines 135 to 139.

Calls: [papd_cleanup](main.c.md#papd_cleanup), [papd_exit](main.c.md#papd_exit)

Called by: [main](main.c.md#main)

### reap

```c
static void reap(int sig)
```

Defined at lines 142 to 166.

Called by: [main](main.c.md#main)

### main

```c
int main(int ac, char **av)
```

Defined at lines 168 to 607.

Calls: [atp_close](../../libatalk/atp/atp_close.c.md#atp_close), [atp_open](../../libatalk/atp/atp_open.c.md#atp_open), [atp_rreq](../../libatalk/atp/atp_rreq.c.md#atp_rreq), [atp_rsel](../../libatalk/atp/atp_rsel.c.md#atp_rsel), [atp_sresp](../../libatalk/atp/atp_sresp.c.md#atp_sresp), [auth_load](auth.c.md#auth_load), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [cups_get_printer_status](print_cups.c.md#cups_get_printer_status), [die](main.c.md#die), [fault_setup](../../libatalk/util/fault.c.md#fault_setup), [getprinters](main.c.md#getprinters), [getstatus](main.c.md#getstatus), [nbp_rgstr](../../libatalk/nbp/nbp_rgstr.c.md#nbp_rgstr), [papd_cleanup](main.c.md#papd_cleanup), [papd_exit](main.c.md#papd_exit), [reap](main.c.md#reap), [rprintcap](main.c.md#rprintcap), [server_lock](../../libatalk/util/server_lock.c.md#server_lock), [session](session.c.md#session), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name), [set_processname](../../libatalk/util/logger.c.md#set_processname), [sig_handler](main.c.md#sig_handler), [syslog_setup](../../libatalk/util/logger.c.md#syslog_setup)

Uses file-scope variables: `conffile`, `connid`, `defprinter`, `oquantum`, `printcap`, `printers`, `quantum`, `r_buf`, `sock`, `stop_requested`, `uamlist`, `uampath`

### getstatus

```c
int getstatus(struct printer *pr, rbuf_t *buf)
```

Defined at lines 613 to 662.

Calls: [cups_get_printer_status](print_cups.c.md#cups_get_printer_status)

Called by: [main](main.c.md#main)

Uses file-scope variables: `cannedstatus`

### free_printer_fields

```c
static void free_printer_fields(struct printer *pr)
```

Defined at lines 664 to 701.

Called by: [getprinters](main.c.md#getprinters)

Uses file-scope variables: `defprinter`

### getprinters

```c
static void getprinters(char *cf)
```

Defined at lines 705 to 907.

Calls: [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers), [cups_check_printer](print_cups.c.md#cups_check_printer), [endprent](printcap.c.md#endprent), [free_printer_fields](main.c.md#free_printer_fields), [getpname](printcap.c.md#getpname), [getprent](printcap.c.md#getprent), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [pgetflag](printcap.h.md#pgetflag), [pgetstr](printcap.h.md#pgetstr), [pnchktc](printcap.h.md#pnchktc)

Called by: [main](main.c.md#main)

Uses file-scope variables: `defprinter`, `printers`, `uamlist`

### rprintcap

```c
int rprintcap(struct printer *pr)
```

Defined at lines 909 to 1055.

Calls: [cups_get_printer_ppd](print_cups.c.md#cups_get_printer_ppd), [cups_printername_ok](print_cups.c.md#cups_printername_ok), [endprent](printcap.c.md#endprent), [pgetent](printcap.h.md#pgetent), [pgetnum](printcap.h.md#pgetnum), [pgetstr](printcap.h.md#pgetstr)

Called by: [main](main.c.md#main)

Uses file-scope variables: `defprinter`, `printcap`

# Types

### struct rbuf_t

Defined at line 57.
* `uint8_t user_bytes`
* `uint8_t data_bytes`
* `uint8_t buf_len`
* `char buf`

# Macros

* Undocumented: `MACCHARSET`, `PF_CONFBUFFER`, `WEXITSTATUS`, `WIFEXITED`

# File-scope variables

`cannedstatus`, `conffile`, `connid`, `debug`, `defprinter`, `oquantum`, `pidfile`, `printcap`, `printer`, `printers`, `quantum`, `r_buf`, `sock`, `stop_requested`, `uamlist`, `uampath`, `version`
