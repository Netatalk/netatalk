---
type: C Source File
title: "libatalk/util/logger.c"
description: "19 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/logger.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-11T16:44:05+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/compat.h](../../include/atalk/compat.h.md)
* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/unix.h](../../include/atalk/unix.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `ctype.h`, `errno.h`, `fcntl.h`, `limits.h`, `signal.h`, `stdarg.h`, `stdbool.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/stat.h`, `sys/time.h`, `sys/types.h`, `sys/uio.h`, `syslog.h`, `time.h`, `unistd.h`

# Functions

### generate_message

```c
static int generate_message(char **message_details_buffer, char *user_message, int display_options, enum loglevels loglevel, enum logtypes logtype, bool log_us_timestamp)
```

Defined at lines 144 to 199.

Format a log file line around a message.

The line reads "time process[pid] {file:line} (level:type): message" and ends with a newline. The source file and line come from the static position that make_log_entry sets.

Parameters:
* `message_details_buffer`: receives the line, which the caller frees, or "" on failure
* `user_message`: message text
* `display_options`: unused
* `loglevel`: level of the message
* `logtype`: type of the message
* `log_us_timestamp`: add microseconds to the time

Returns: the line length, or -1 when asprintf fails

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [log_file_note_lost](logger.c.md#log_file_note_lost), [make_log_entry](logger.c.md#make_log_entry)

Uses file-scope variables: `arr_loglevel_strings`, `arr_logtype_strings`, `log_config`, `log_src_filename`, `log_src_linenumber`

### get_syslog_equivalent

```c
static int get_syslog_equivalent(enum loglevels loglevel)
```

Defined at lines 210 to 239.

Map a netatalk log level to a syslog priority.

Both log_severe and log_error map to LOG_ERR. Every debug level maps to LOG_DEBUG.

Parameters:
* `loglevel`: netatalk level

Returns: the syslog priority

Called by: [make_syslog_entry](logger.c.md#make_syslog_entry)

### make_syslog_entry

```c
static void make_syslog_entry(enum loglevels loglevel, enum logtypes logtype, char *message)
```

Defined at lines 250 to 261.

Send a message to syslog, opening the connection on first use.

The make_log_entry call sends the messages of syslog types here.

Parameters:
* `loglevel`: level of the message
* `logtype`: unused
* `message`: message text

Calls: [get_syslog_equivalent](logger.c.md#get_syslog_equivalent)

Called by: [make_log_entry](logger.c.md#make_log_entry)

Uses file-scope variables: `log_config`

### log_init

```c
static void log_init(void)
```

Defined at lines 268 to 274.

Send every log type to syslog at log_info.

The make_log_entry call runs it when the process logs before any setup.

Calls: [syslog_setup](logger.c.md#syslog_setup)

Called by: [make_log_entry](logger.c.md#make_log_entry)

### log_open_file

```c
static int log_open_file(const char *filename)
```

Defined at lines 284 to 293.

Open a log file for appending, as root and close-on-exec.

A new file gets mode 0640, because logs can hold user names and paths.

Parameters:
* `filename`: path of the log file

Returns: the descriptor, or -1 with errno set

Calls: [become_root](unix.c.md#become_root), [unbecome_root](unix.c.md#unbecome_root)

Called by: [log_file_recover](logger.c.md#log_file_recover), [log_file_reopen](logger.c.md#log_file_reopen), [log_reopen](logger.c.md#log_reopen), [log_setup](logger.c.md#log_setup)

### log_file_opened

```c
static void log_file_opened(enum logtypes logtype)
```

Defined at lines 318 to 329.

Record the device and inode of the file behind the new descriptor of a type.

They let log_file_reopen tell the file from another one under the same name or number. A failed fstat records 0 for both. The call also clears the mid-line mark of the type.

Parameters:
* `logtype`: type with the new descriptor

Called by: [log_file_recover](logger.c.md#log_file_recover), [log_file_reopen](logger.c.md#log_file_reopen), [log_reopen](logger.c.md#log_reopen), [log_setup](logger.c.md#log_setup)

Uses file-scope variables: `log_file_dev`, `log_file_ino`, `log_file_midline`, `type_configs`

### log_absolute_name

```c
static char * log_absolute_name(const char *filename)
```

Defined at lines 339 to 354.

Return the absolute name that reopens a log file after a chdir.

A relative name joins the current directory.

Parameters:
* `filename`: log file name

Returns: the new absolute path, or NULL on failure

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [log_setup](logger.c.md#log_setup)

### log_setup

```c
static void log_setup(const char *filename, enum loglevels loglevel, enum logtypes logtype, const bool log_us_timestamp)
```

Defined at lines 375 to 512.

Point a log type at a file, or turn the type off with a level of 0.

A level of 0 closes the file of a set type. For the default type it also turns off every type that has no setup. Any other level drops the old setup of the type and opens the new file. "/dev/tty" means stdout, and a name that ends in XXXXXX goes through mkstemp. Any other name opens as root. A file without an absolute name to reopen it by stays closed, and the type stays unset.

The default type passes its level and timestamp setting to every type that has no setup, so the LOG macro reads one level per type.

Parameters:
* `filename`: log file, "/dev/tty", or a mkstemp template
* `loglevel`: level, 0 to turn the type off
* `logtype`: type to set up
* `log_us_timestamp`: add microseconds to the time, passed on by the default type

Calls: [log_absolute_name](logger.c.md#log_absolute_name), [log_file_opened](logger.c.md#log_file_opened), [log_open_file](logger.c.md#log_open_file)

Called by: [setuplog_internal](logger.c.md#setuplog_internal)

Uses file-scope variables: `arr_loglevel_strings`, `arr_logtype_strings`, `log_config`, `log_lost`, `type_configs`

### log_close_all

```c
void log_close_all(void)
```

Defined at lines 523 to 537.

Close the syslog connection and every log file before a [closeall()](unix.c.md#closeall).

Called while the descriptors are still ours: closelog() on a closed and reused number would close the newcomer, and a stale file number kept in type_configs would be closed by [log_reopen()](logger.c.md#log_reopen) after another type had already been given it. The next syslog message opens a new connection; [log_reopen()](logger.c.md#log_reopen) reopens the files.

Called by: [daemonize](unix.c.md#daemonize)

Uses file-scope variables: `log_config`, `type_configs`

Mentioned in the documentation of: [log_reopen](logger.c.md#log_reopen)

### log_reopen

```c
void log_reopen(void)
```

Defined at lines 550 to 570.

Reopen the log files a previous [setuplog()](logger.c.md#setuplog) opened.

[daemonize()](unix.c.md#daemonize) closes every descriptor, so a process that set up logging before it daemonized would write to a closed or reused one. Files opened by name are reopened by it, a mkstemp() file by the name it got; fd 1 for "/dev/tty" is not ours to close. Only a type whose descriptor [log_close_all()](logger.c.md#log_close_all) has already closed is reopened, so a number another type was just given is never closed here. A file that cannot be reopened falls back to syslog, as a type with no file does.

Calls: [log_file_opened](logger.c.md#log_file_opened), [log_open_file](logger.c.md#log_open_file), [syslog_setup](logger.c.md#syslog_setup)

Called by: [daemonize](unix.c.md#daemonize)

Uses file-scope variables: `type_configs`

Mentioned in the documentation of: [log_close_all](logger.c.md#log_close_all)

### syslog_setup

```c
void syslog_setup(int loglevel, enum logtypes logtype, int display_options, int facility)
```

Defined at lines 584 to 616.

Send a log type to syslog at a level.

Setup the level and type of log that will be logged to syslog.

The display options and the facility apply to every type. For the default type, every type that has no setup takes the level and syslog. An open log file of the type stays open, unused.

Parameters:
* `loglevel`: level
* `logtype`: type to set up
* `display_options`: openlog options
* `facility`: syslog facility

Called by: [log_init](logger.c.md#log_init), [log_reopen](logger.c.md#log_reopen), [main](../../etc/atalkd/main.c.md#main), [main](../../etc/papd/main.c.md#main), [setuplog_internal](logger.c.md#setuplog_internal)

Uses file-scope variables: `arr_loglevel_strings`, `log_config`, `type_configs`

### setuplog_internal

```c
static void setuplog_internal(const char *loglevel, const char *logtype, const char *filename, const bool log_us_timestamp)
```

Defined at lines 631 to 675.

Set up one log type from its type and level names.

Both names match without regard to case, and an unknown name leaves the setup unchanged. A NULL level turns the type off. A NULL filename sends the type to syslog through syslog_setup, and any other name goes to log_setup.

Parameters:
* `loglevel`: level name, or NULL
* `logtype`: type name
* `filename`: log file, or NULL for syslog
* `log_us_timestamp`: add microseconds to the time

Calls: [log_setup](logger.c.md#log_setup), [syslog_setup](logger.c.md#syslog_setup)

Called by: [setuplog](logger.c.md#setuplog)

Uses file-scope variables: `arr_loglevel_strings`, `arr_logtype_strings`, `num_loglevel_strings`, `num_logtype_strings`

### log_file_reopen

```c
static bool log_file_reopen(enum logtypes owner, int fd, int err)
```

Defined at lines 692 to 730.

Reopen a log file by name after a failed write, where that can help.

A reopen helps when the name no longer names the open file, as after a delete or a rotation, or when the descriptor is no longer the file's; a full disk under the same name gains nothing from one. The new descriptor takes the slot before the old one is closed, so no slot holds a closed number.

Parameters:
* `owner`: type whose slot holds the descriptor
* `fd`: descriptor the write failed on
* `err`: errno of the failed write

Returns: true when the slot holds a new descriptor to retry on

Calls: [log_file_opened](logger.c.md#log_file_opened), [log_open_file](logger.c.md#log_open_file)

Called by: [log_file_write](logger.c.md#log_file_write)

Uses file-scope variables: `log_file_dev`, `log_file_ino`, `type_configs`

### log_file_write

```c
static int log_file_write(enum logtypes owner, const char *line, size_t len)
```

Defined at lines 747 to 785.

Write a formatted line to the log file of a type, retrying once on a reopened file.

A short write goes on with the rest of the line. After a reopen the new file gets the line from its start. A line that stops partway leaves the file mid-line, and the next line there starts with a newline. EINTR repeats the write.

Parameters:
* `owner`: type whose slot holds the descriptor
* `line`: formatted line
* `len`: length of line

Returns: 0, or the errno of the failed write, where a write of 0 bytes counts as EIO

Calls: [log_file_reopen](logger.c.md#log_file_reopen)

Called by: [log_file_note_lost](logger.c.md#log_file_note_lost), [make_log_entry](logger.c.md#make_log_entry)

Uses file-scope variables: `log_file_midline`, `type_configs`

### log_file_recover

```c
static bool log_file_recover(enum logtypes owner)
```

Defined at lines 798 to 816.

Give a type with an empty slot its file back, once the file opens again.

A failed reopen empties the slot, and make_log_entry calls this for every line until the open works.

Parameters:
* `owner`: type whose slot holds the descriptor

Returns: true when the slot holds a descriptor, false after a failed open or for a type without a file name

Calls: [log_file_opened](logger.c.md#log_file_opened), [log_open_file](logger.c.md#log_open_file)

Called by: [make_log_entry](logger.c.md#make_log_entry)

Uses file-scope variables: `type_configs`

### log_file_note_lost

```c
static void log_file_note_lost(enum logtypes owner, bool log_us_timestamp)
```

Defined at lines 829 to 854.

Write the count of unwritten lines and the first error to the log file of a type.

The make_log_entry call writes this note before the next message. The note carries the source position of this file, and the call restores the position of the caller afterwards. A written note resets the count.

Parameters:
* `owner`: type whose lines failed to reach its file
* `log_us_timestamp`: add microseconds to the time

Calls: [generate_message](logger.c.md#generate_message), [log_file_write](logger.c.md#log_file_write)

Called by: [make_log_entry](logger.c.md#make_log_entry)

Uses file-scope variables: `log_lost`, `log_lost_errno`, `log_src_filename`, `log_src_linenumber`, `type_configs`

### set_processname

```c
void set_processname(const char *processname)
```

Defined at lines 865 to 869.

Set the process name of log lines and syslog, cut to 15 characters.

Parameters:
* `processname`: name of the process

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [main](../../bin/misc/logger_test.c.md#main), [main](../../etc/atalkd/main.c.md#main), [main](../../etc/papd/main.c.md#main)

Uses file-scope variables: `log_config`

### make_log_entry

```c
void make_log_entry(enum loglevels loglevel, enum logtypes logtype, const char *file, const bool log_us_timestamp, int line, char *message,...)
```

Defined at lines 892 to 1007.

Write a log message to syslog or to the log file of its type, the worker of the LOG macro.

LOG macro worker: log the message to syslog or to a file.

A call from a signal handler during another call returns at once. The call leaves errno unchanged. A process without any setup logs to syslog at log_info. A type without a setup writes through the file of the default type.

A line that cannot reach its file adds to a count of unwritten lines. The next line that reaches the file follows a note with the count and the first error. A forked child drops the count of its parent.

Parameters:
* `loglevel`: level of the message
* `logtype`: type of the message
* `file`: source file of the LOG call
* `log_us_timestamp`: add microseconds to the time
* `line`: source line of the LOG call
* `message`: printf format
* `...`: format arguments

Calls: [generate_message](logger.c.md#generate_message), [log_file_note_lost](logger.c.md#log_file_note_lost), [log_file_recover](logger.c.md#log_file_recover), [log_file_write](logger.c.md#log_file_write), [log_init](logger.c.md#log_init), [make_syslog_entry](logger.c.md#make_syslog_entry), [vasprintf](../compat/misc.c.md#vasprintf)

Uses file-scope variables: `log_config`, `log_lost`, `log_lost_errno`, `log_lost_pid`, `log_src_filename`, `log_src_linenumber`, `type_configs`

### setuplog

```c
void setuplog(const char *logstr, const char *logfile, const bool log_us_timestamp)
```

Defined at lines 1021 to 1061.

Set up logging from a list of type:level pairs.

Commas or spaces separate the pairs, as in "default:note afpdaemon:debug". Type and level names match without regard to case, and the call skips a pair with an unknown name.

Parameters:
* `logstr`: list of type:level pairs
* `logfile`: log file, "/dev/tty", a mkstemp template, or NULL for syslog
* `log_us_timestamp`: add microseconds to the time

Calls: [setuplog_internal](logger.c.md#setuplog_internal)

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [main](../../bin/dbd/cmd_dbd.c.md#main), [main](../../bin/misc/logger_test.c.md#main), [main](../../bin/misc/uuidtest.c.md#main), [main](../../bin/nad/nad.c.md#main)

Mentioned in the documentation of: [log_reopen](logger.c.md#log_reopen)

# Macros

* `DEFAULT_LOG_CONFIG`: Default log config: log nothing to files.
* `LOGTYPE_STRING_IDENTIFIERS`: these are the string identifiers corresponding to each logtype
* Undocumented: `COUNT_ARRAY`, `LOGLEVEL_STRING_IDENTIFIERS`, `MAXLOGSIZE`

# File-scope variables

`arr_loglevel_chars`, `arr_loglevel_strings`, `arr_logtype_strings`, `log_config`, `log_file_dev`, `log_file_ino`, `log_file_midline`, `log_lost`, `log_lost_errno`, `log_lost_pid`, `log_src_filename`, `log_src_linenumber`, `num_loglevel_chars`, `num_loglevel_strings`, `num_logtype_strings`, `type_configs`
