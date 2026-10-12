---
type: C Source File
title: "etc/papd/lp.c"
description: "Interface to lpr system."
resource: "https://github.com/Netatalk/netatalk/blob/main/etc/papd/lp.c"
tags: ["etc/papd"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 119 to 172.

Open a TCP socket bound to a reserved port, for systems without [rresvport()](lp.c.md#rresvport).

The search starts at *alport, clamped to 512 to 1023, and counts down. Below 512 it goes on at 1023. It ends at the first port that binds or after a full circle. A bind error other than EADDRINUSE ends it early.

Parameters:
* `alport`: port to start at, receives the bound port

Returns: the socket, or -1 with errno set

### lp_conn_inet

```c
int lp_conn_inet()
```

Declared at etc/papd/lp.c line 176; no definition in the scanned sources.

### lp_disconn_inet

```c
int lp_disconn_inet(int)
```

Declared at etc/papd/lp.c line 177; no definition in the scanned sources.

### lp_conn_unix

```c
int lp_conn_unix()
```

Declared at etc/papd/lp.c line 178; no definition in the scanned sources.

Called by: [lp_print](lp.c.md#lp_print)

### lp_disconn_unix

```c
int lp_disconn_unix(int)
```

Declared at etc/papd/lp.c line 179; no definition in the scanned sources.

Called by: [lp_print](lp.c.md#lp_print)

### lp_origin

```c
void lp_origin(int origin)
```

Defined at lines 209 to 212.

Record the driver that made the job, as guess_creator reports it.

Parameters:
* `origin`: 1 for an Apple LaserWriter driver, 2 for cgpdftops, or 0

Called by: [ch_creator](headers.c.md#ch_creator)

### convert_octal

```c
static void convert_octal(char *string, charset_t dest)
```

Defined at lines 225 to 264.

Decode the backslash escapes of a DSC comment value in place.

With a source charset, a `\ddd` octal escape becomes its byte, and one of 0 or of 0xff and above becomes a period. Every other escape, a backslash and the next character, becomes a period, as does every escape without a source charset. The result is never longer than the input.

Parameters:
* `string`: NUL-terminated value
* `dest`: source charset of the value, or 0 when unknown

Called by: [translate](lp.c.md#translate)

### translate

```c
static void translate(charset_t from, charset_t dest, char **option)
```

Defined at lines 278 to 293.

Decode the escapes of a DSC comment value and convert it to another charset.

Without a source charset the value keeps its bytes after convert_octal. A failed conversion keeps the decoded value.

Parameters:
* `from`: charset of the value, or 0 when unknown
* `dest`: target charset
* `option`: value, replaced by the converted allocation

Calls: [convert_octal](lp.c.md#convert_octal), [convert_string_allocate](../../libatalk/unicode/charcnv.c.md#convert_string_allocate)

Called by: [lp_setup_comments](lp.c.md#lp_setup_comments)

### lp_setup_comments

```c
static void lp_setup_comments(charset_t dest)
```

Defined at lines 305 to 333.

Convert the job name, the user name the job is for, and the person to a charset.

The source charset comes from lp_origin: Mac Roman for 1 and decomposed UTF-8 for 2. With any other origin only the escapes change.

Parameters:
* `dest`: target charset

Calls: [translate](lp.c.md#translate)

Called by: [lp_open](lp.c.md#lp_open), [lp_print](lp.c.md#lp_print)

### lp_person

```c
void lp_person(char *person)
```

Defined at lines 452 to 464.

Record the authenticated user of the job.

The name sets the user of the printer command, the owner of the spool file, and the user of the lpd or CUPS job. A failed allocation exits papd.

Parameters:
* `person`: user name

Called by: [cq_rbilogin](queries.c.md#cq_rbilogin), [lp_init](lp.c.md#lp_init)

### lp_host

```c
void lp_host(char *host)
```

Defined at lines 493 to 506.

Record a host name for the job, a value that only the log line reads.

No caller sets it, and lp_print writes the server host name instead. A failed allocation exits papd.

Parameters:
* `host`: host name

### lp_job

```c
void lp_job(char *job)
```

Defined at lines 516 to 524.

Record the job name from the `%Title:` header comment.

Pipe and CUPS jobs convert the name with lp_setup_comments, and lpd jobs write it unchanged.

Parameters:
* `job`: job name

Called by: [ch_title](headers.c.md#ch_title)

### lp_for

```c
void lp_for(char *lpfor)
```

Defined at lines 533 to 540.

Record the user name the job is for, from the `%For:` header comment.

A CUPS job without an authenticated user goes out under this name.

Parameters:
* `lpfor`: user name

Called by: [ch_for](headers.c.md#ch_for)

### lp_init

```c
static int lp_init(struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 561 to 750.

Prepare the job state when the spool file first opens.

A printer with authentication needs an authenticated user. CAP style authentication reads the user name from a file whose name holds the net and node of the client. The file lives in the directory of the ca option. Print UAM authentication counts when lp_person holds a name. A second call aborts papd.

An lpd printer refuses jobs while the mode of its lock file has the 010 bit. Its job number comes from the .seq file, which counts modulo 1000. A CUPS printer refuses jobs when cups_get_printer_status reports queuing as off, and like a pipe printer it uses the process ID as the job number.

Parameters:
* `out`: reply buffer, receives the error for the client
* `sat`: client address

Returns: 0, or -1 after an error for the client

Calls: [cups_get_printer_status](print_cups.c.md#cups_get_printer_status), [lp_person](lp.c.md#lp_person), [spoolerror](file.c.md#spoolerror)

Called by: [lp_open](lp.c.md#lp_open)

Uses file-scope variables: `hostname`

### lp_open

```c
int lp_open(struct papfile *out, struct sockaddr_at *sat)
```

Defined at lines 765 to 880.

Open the spool file of a job, or the pipe to the printer command.

A pending job prints first, and the first call runs lp_init. A pipe printer switches the process to the authenticated user for good and converts the header values to the Unix charset. It then starts the printer command with popen. A spooling printer creates the next data file, such as `dfA042hostname`, owned by the user or the printer operator.

Parameters:
* `out`: reply buffer, receives the error for the client
* `sat`: client address

Returns: 0, or -1 when the job cannot open or is open already

Calls: [lp_init](lp.c.md#lp_init), [lp_print](lp.c.md#lp_print), [lp_setup_comments](lp.c.md#lp_setup_comments), [spoolerror](file.c.md#spoolerror)

Called by: [ps](magics.c.md#ps)

Uses file-scope variables: `hostname`

### lp_close

```c
int lp_close(void)
```

Defined at lines 890 to 901.

Close the spool file or the pipe of the job and mark the job pending.

The next lp_print, lp_open, or lp_write sends the pending job to the printer.

Returns: 0

Called by: [lp_cancel](lp.c.md#lp_cancel), [lp_print](lp.c.md#lp_print), [parser_error](magics.c.md#parser_error), [ps](magics.c.md#ps)

### lp_write

```c
int lp_write(struct papfile *in, char *buf, size_t len)
```

Defined at lines 919 to 1001.

Write job data to the spool file or the printer pipe.

A pending job prints first. Until the spool file opens, the data waits in a 32 KiB buffer. Past half of it the call sets PF_STW, so the job loop opens the file. With P_FOOMATIC_HACK, header lines that carry PF_TRANSLATE get an LF line end in place of a CR. A full buffer or a failed write aborts papd.

Parameters:
* `in`: input buffer of the job, receives PF_STW
* `buf`: data
* `len`: length of data

Returns: 0

Calls: [lp_print](lp.c.md#lp_print)

Called by: [ch_creator](headers.c.md#ch_creator), [ch_endcomm](headers.c.md#ch_endcomm), [ch_endtranslate](headers.c.md#ch_endtranslate), [ch_for](headers.c.md#ch_for), [ch_starttranslate](headers.c.md#ch_starttranslate), [ch_title](headers.c.md#ch_title), [ch_translateone](headers.c.md#ch_translateone), [cm_psadobe](magics.c.md#cm_psadobe), [ps](magics.c.md#ps)

### lp_cancel

```c
int lp_cancel(void)
```

Defined at lines 1011 to 1040.

Cancel the job of the session by closing and deleting its spool data files.

A pipe printer has no data files, so the call returns at once.

Returns: 0, or -1 when a file name is too long

Calls: [lp_close](lp.c.md#lp_close)

Called by: [session](session.c.md#session)

Uses file-scope variables: `hostname`

### lp_print

```c
int lp_print(void)
```

Defined at lines 1058 to 1191.

Hand the spooled job to the print system.

The call closes the spool file first. For lpd it writes a control file with the host, the user, the job name, and the data files. It links the file into place as cfA and tells lpd through its Unix socket to start the queue.

For CUPS it converts the header values to the CUPS charset and submits the data file. The job runs as the authenticated user, the user the job is for, or the operator, in that order. The call deletes the data file afterwards. A pipe printer needs nothing here.

Errors only log, and the spool and control files stay.

Returns: 0

Calls: [add_charset](../../libatalk/unicode/charcnv.c.md#add_charset), [cups_get_language](print_cups.c.md#cups_get_language), [cups_print_job](print_cups.c.md#cups_print_job), [lp_close](lp.c.md#lp_close), [lp_conn_unix](lp.c.md#lp_conn_unix), [lp_disconn_unix](lp.c.md#lp_disconn_unix), [lp_setup_comments](lp.c.md#lp_setup_comments)

Called by: [lp_open](lp.c.md#lp_open), [lp_write](lp.c.md#lp_write), [session](session.c.md#session)

Uses file-scope variables: `hostname`

# Types

### struct lp

Defined at line 185.
* `int lp_flags`
* `FILE * lp_stream`
* `int lp_seq`
* `int lp_origin`
* `char lp_letter`
* `char * lp_person`
* `char * lp_created_for`: Holds the content of the Postscript %For Comment if available.
* `char * lp_host`
* `char * lp_job`
* `char * lp_spoolfile`

# Macros

* Undocumented: `BUFSIZE`, `LP_CONNECT`, `LP_INIT`, `LP_JOBPENDING`, `LP_OPEN`, `LP_PIPE`, `LP_QUEUE`, `is_var`

# File-scope variables

`hostname`, `lp`, `sat`
