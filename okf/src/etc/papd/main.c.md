---
type: C Source File
title: "etc/papd/main.c"
description: "10 functions, 1 type, includes 13 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/main.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
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

Defined at lines 90 to 94.

Note SIGINT or SIGTERM, so the accept loop of the server stops.

Parameters:
* `signo`: signal number, unused

Called by: [main](main.c.md#main)

### papd_exit

```c
static void papd_exit(const int i)
```

Defined at lines 110 to 115.

Remove the pid file, unload the UAMs, and exit the server process.

Print job children exit through exit() instead, since the pid file belongs to the server.

Parameters:
* `i`: exit status

Calls: [auth_unload](auth.c.md#auth_unload)

Called by: [die](main.c.md#die), [main](main.c.md#main), [papd_cleanup](main.c.md#papd_cleanup)

### papd_cleanup

```c
static void papd_cleanup(int n)
```

Defined at lines 125 to 153.

Unregister the NBP names of the printers and delete the PPD files fetched from CUPS.

A failed NBP removal exits papd with status n + 1.

Parameters:
* `n`: status that a failed NBP removal exits with, plus 1

Calls: [nbp_unrgstr](../../libatalk/nbp/nbp_unrgstr.c.md#nbp_unrgstr), [papd_exit](main.c.md#papd_exit)

Called by: [die](main.c.md#die), [main](main.c.md#main)

Uses file-scope variables: `printers`

### die

```c
static void die(int n)
```

Defined at lines 160 to 164.

Clean up the printer registrations and exit the server process.

Parameters:
* `n`: exit status

Calls: [papd_cleanup](main.c.md#papd_cleanup), [papd_exit](main.c.md#papd_exit)

Called by: [main](main.c.md#main)

### reap

```c
static void reap(int sig)
```

Defined at lines 172 to 196.

Collect exited print job children on SIGCHLD and log how each ended.

Parameters:
* `sig`: signal number, unused

Called by: [main](main.c.md#main)

### main

```c
int main(int ac, char **av)
```

Defined at lines 219 to 658.

Start papd: read the printers, register them with NBP, and serve PAP connections.

The options are -d for debug mode, -f for papd.conf, -p for the printcap file, and -P for the pid file. The -v or -V option prints the version, and papd accepts -a and ignores it. When papd.conf cannot open, papd serves one default printer named after the host.

Each printer gets an ATP socket and an NBP name. A PAP open request checks the printer and answers with a new ATP socket, the flow quantum, and the status. A forked child then runs the session. A printer that fails the check, or a lack of sockets, gets a busy reply. A status request gets the status alone.

SIGTERM or SIGINT unregisters the names and exits.

Parameters:
* `ac`: argument count
* `av`: argument vector

Returns: 0 after a shutdown signal, and a startup failure exits with 1

Calls: [atp_close](../../libatalk/atp/atp_close.c.md#atp_close), [atp_open](../../libatalk/atp/atp_open.c.md#atp_open), [atp_rreq](../../libatalk/atp/atp_rreq.c.md#atp_rreq), [atp_rsel](../../libatalk/atp/atp_rsel.c.md#atp_rsel), [atp_sresp](../../libatalk/atp/atp_sresp.c.md#atp_sresp), [auth_load](auth.c.md#auth_load), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate), [cups_get_printer_status](print_cups.c.md#cups_get_printer_status), [die](main.c.md#die), [fault_setup](../../libatalk/util/fault.c.md#fault_setup), [getprinters](main.c.md#getprinters), [getstatus](main.c.md#getstatus), [nbp_rgstr](../../libatalk/nbp/nbp_rgstr.c.md#nbp_rgstr), [papd_cleanup](main.c.md#papd_cleanup), [papd_exit](main.c.md#papd_exit), [reap](main.c.md#reap), [rprintcap](main.c.md#rprintcap), [server_lock](../../libatalk/util/server_lock.c.md#server_lock), [session](session.c.md#session), [set_charset_name](../../libatalk/unicode/charcnv.c.md#set_charset_name), [set_processname](../../libatalk/util/logger.c.md#set_processname), [sig_handler](main.c.md#sig_handler), [syslog_setup](../../libatalk/util/logger.c.md#syslog_setup)

Uses file-scope variables: `conffile`, `defprinter`, `printcap`, `printers`, `r_buf`, `uamlist`, `uampath`

### getstatus

```c
int getstatus(struct printer *pr, rbuf_t *buf)
```

Defined at lines 671 to 720.

Fill a PAP status reply with the status of a printer.

A pipe printer reports a fixed status. With CUPS a spooling printer reports the CUPS status, and without CUPS the status file in its spool directory. The status is a Pascal string of up to 255 bytes in buf.

Parameters:
* `pr`: printer
* `buf`: reply buffer, receives the length byte and the status

Returns: the number of bytes of the status, length byte included

Calls: [cups_get_printer_status](print_cups.c.md#cups_get_printer_status)

Called by: [main](main.c.md#main)

Uses file-scope variables: `cannedstatus`

### free_printer_fields

```c
static void free_printer_fields(struct printer *pr)
```

Defined at lines 728 to 765.

Free the strings of a printer entry that do not belong to the default printer.

Parameters:
* `pr`: printer entry, whose structure the caller frees

Called by: [getprinters](main.c.md#getprinters)

Uses file-scope variables: `defprinter`

### getprinters

```c
static void getprinters(char *cf)
```

Defined at lines 790 to 992.

Read the printer entries of papd.conf into the printer list.

Each entry starts with an NBP name, and the defaults fill in the host name, LaserWriter, and the zone `*`. The options are:

* `pd`: the PPD file
* `pr`: the lpd or CUPS printer, or a command after a leading `|`
* `ca`: the directory for CAP style authentication
* `sp`: print UAM authentication
* `am`: the UAM list, shared by every printer
* `op`: the operator of a spooling printer
* `pa`: the AppleTalk address
* `co`: CUPS options
* `fo`: the foomatic line end conversion

An entry named cupsautoadd adds every CUPS printer. An entry that does not parse only logs, and a missing file leaves the default printer as the only one.

Parameters:
* `cf`: path of papd.conf

Calls: [atalk_aton](../../libatalk/util/atalk_addr.c.md#atalk_aton), [cups_autoadd_printers](print_cups.c.md#cups_autoadd_printers), [cups_check_printer](print_cups.c.md#cups_check_printer), [endprent](printcap.c.md#endprent), [free_printer_fields](main.c.md#free_printer_fields), [getpname](printcap.c.md#getpname), [getprent](printcap.c.md#getprent), [nbp_name](../../libatalk/nbp/nbp_util.c.md#nbp_name), [pgetflag](printcap.c.md#pgetflag), [pgetstr](printcap.c.md#pgetstr), [pnchktc](printcap.c.md#pnchktc)

Called by: [main](main.c.md#main)

Uses file-scope variables: `defprinter`, `printers`, `uamlist`

### rprintcap

```c
int rprintcap(struct printer *pr)
```

Defined at lines 1006 to 1152.

Check the print system entry of a spooling printer and complete the printer from it.

With CUPS the printer has to exist in CUPS, unless cupsautoadd added it, and a printer without a PPD file takes the PPD of CUPS. Without CUPS the printcap entry supplies the spool directory, accounting, page cost, and lock file.

Parameters:
* `pr`: printer

Returns: 0, or -1 for an unknown printer or a failed allocation

Calls: [cups_get_printer_ppd](print_cups.c.md#cups_get_printer_ppd), [cups_printername_ok](print_cups.c.md#cups_printername_ok), [endprent](printcap.c.md#endprent), [pgetent](printcap.c.md#pgetent), [pgetnum](printcap.c.md#pgetnum), [pgetstr](printcap.c.md#pgetstr)

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
