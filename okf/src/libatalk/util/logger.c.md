---
type: C Source File
title: "libatalk/util/logger.c"
description: "19 functions, includes 4 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/logger.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
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

Defined at lines 128 to 183.

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [log_file_note_lost](logger.c.md#log_file_note_lost), [make_log_entry](logger.c.md#make_log_entry)

Uses file-scope variables: `arr_loglevel_strings`, `arr_logtype_strings`, `log_config`, `log_src_filename`, `log_src_linenumber`

### get_syslog_equivalent

```c
static int get_syslog_equivalent(enum loglevels loglevel)
```

Defined at lines 185 to 214.

Called by: [make_syslog_entry](logger.c.md#make_syslog_entry)

### make_syslog_entry

```c
static void make_syslog_entry(enum loglevels loglevel, enum logtypes logtype, char *message)
```

Defined at lines 217 to 228.

Called by the LOG macro for syslog messages

Calls: [get_syslog_equivalent](logger.c.md#get_syslog_equivalent)

Called by: [make_log_entry](logger.c.md#make_log_entry)

Uses file-scope variables: `log_config`

### log_init

```c
static void log_init(void)
```

Defined at lines 230 to 236.

Calls: [syslog_setup](logger.c.md#syslog_setup)

Called by: [make_log_entry](logger.c.md#make_log_entry)

### log_open_file

```c
static int log_open_file(const char *filename)
```

Defined at lines 243 to 252.

Open a log file for appending, as root and close-on-exec.

Returns: the descriptor, or -1 with errno set

Calls: [become_root](unix.c.md#become_root), [unbecome_root](unix.c.md#unbecome_root)

Called by: [log_file_recover](logger.c.md#log_file_recover), [log_file_reopen](logger.c.md#log_file_reopen), [log_reopen](logger.c.md#log_reopen), [log_setup](logger.c.md#log_setup)

### log_file_opened

```c
static void log_file_opened(enum logtypes logtype)
```

Defined at lines 270 to 281.

Remember which file a type's new descriptor was opened on.

Called by: [log_file_recover](logger.c.md#log_file_recover), [log_file_reopen](logger.c.md#log_file_reopen), [log_reopen](logger.c.md#log_reopen), [log_setup](logger.c.md#log_setup)

Uses file-scope variables: `log_file_dev`, `log_file_ino`, `log_file_midline`, `type_configs`

### log_absolute_name

```c
static char * log_absolute_name(const char *filename)
```

Defined at lines 288 to 303.

The name to reopen filename by once the process may have chdir()ed.

Returns: an allocated absolute path, or NULL

Calls: [asprintf](../compat/misc.c.md#asprintf)

Called by: [log_setup](logger.c.md#log_setup)

### log_setup

```c
static void log_setup(const char *filename, enum loglevels loglevel, enum logtypes logtype, const bool log_us_timestamp)
```

Defined at lines 305 to 442.

Calls: [log_absolute_name](logger.c.md#log_absolute_name), [log_file_opened](logger.c.md#log_file_opened), [log_open_file](logger.c.md#log_open_file)

Called by: [setuplog_internal](logger.c.md#setuplog_internal)

Uses file-scope variables: `arr_loglevel_strings`, `arr_logtype_strings`, `log_config`, `log_lost`, `type_configs`

### log_close_all

```c
void log_close_all(void)
```

Defined at lines 453 to 467.

Close the syslog connection and every log file before a [closeall()](unix.c.md#closeall)

Called while the descriptors are still ours: closelog() on a closed and reused number would close the newcomer, and a stale file number kept in type_configs would be closed by [log_reopen()](logger.c.md#log_reopen) after another type had already been given it. The next syslog message opens a new connection; [log_reopen()](logger.c.md#log_reopen) reopens the files.

Called by: [daemonize](unix.c.md#daemonize)

Uses file-scope variables: `log_config`, `type_configs`

Mentioned in the documentation of: [log_reopen](logger.c.md#log_reopen)

### log_reopen

```c
void log_reopen(void)
```

Defined at lines 480 to 500.

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

Defined at lines 503 to 535.

Setup syslog logging

Called by: [log_init](logger.c.md#log_init), [log_reopen](logger.c.md#log_reopen), [main](../../etc/atalkd/main.c.md#main), [main](../../etc/papd/main.c.md#main), [setuplog_internal](logger.c.md#setuplog_internal)

Uses file-scope variables: `arr_loglevel_strings`, `log_config`, `type_configs`

### setuplog_internal

```c
static void setuplog_internal(const char *loglevel, const char *logtype, const char *filename, const bool log_us_timestamp)
```

Defined at lines 549 to 593.

If filename == NULL its for syslog logging, otherwise its for file-logging.

"unsetuplog" calls with loglevel == NULL. loglevel == NULL means:

```
if logtype == default
   disable logging
else
   set to default logging
```

Calls: [log_setup](logger.c.md#log_setup), [syslog_setup](logger.c.md#syslog_setup)

Called by: [setuplog](logger.c.md#setuplog)

Uses file-scope variables: `arr_loglevel_strings`, `arr_logtype_strings`, `num_loglevel_strings`, `num_logtype_strings`

### log_file_reopen

```c
static bool log_file_reopen(enum logtypes owner, int fd, int err)
```

Defined at lines 610 to 648.

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

Defined at lines 659 to 697.

Write a formatted line to a type's log file, reopening it once if needed.

A short write continues with the rest of the line; a reopened file gets the whole line. A line whose rest never made it leaves the file mid-line, and the next line written there starts with a newline.

Returns: 0, or the errno of the write that failed

Calls: [log_file_reopen](logger.c.md#log_file_reopen)

Called by: [log_file_note_lost](logger.c.md#log_file_note_lost), [make_log_entry](logger.c.md#make_log_entry)

Uses file-scope variables: `log_file_midline`, `type_configs`

### log_file_recover

```c
static bool log_file_recover(enum logtypes owner)
```

Defined at lines 704 to 722.

Give a type whose slot is empty its file back, once it can be opened.

Returns: true when the slot holds a descriptor

Calls: [log_file_opened](logger.c.md#log_file_opened), [log_open_file](logger.c.md#log_open_file)

Called by: [make_log_entry](logger.c.md#make_log_entry)

Uses file-scope variables: `type_configs`

### log_file_note_lost

```c
static void log_file_note_lost(enum logtypes owner, bool log_us_timestamp)
```

Defined at lines 727 to 752.

Note in a type's log file how many lines it lost, before the next one.

Calls: [generate_message](logger.c.md#generate_message), [log_file_write](logger.c.md#log_file_write)

Called by: [make_log_entry](logger.c.md#make_log_entry)

Uses file-scope variables: `log_lost`, `log_lost_errno`, `log_src_filename`, `log_src_linenumber`, `type_configs`

### set_processname

```c
void set_processname(const char *processname)
```

Defined at lines 759 to 763.

This function sets up the processname

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [main](../../bin/misc/logger_test.c.md#main), [main](../../etc/atalkd/main.c.md#main), [main](../../etc/papd/main.c.md#main)

Uses file-scope variables: `log_config`

### make_log_entry

```c
void make_log_entry(enum loglevels loglevel, enum logtypes logtype, const char *file, const bool log_us_timestamp, int line, char *message,...)
```

Defined at lines 770 to 885.

Bug: make_log_entry has 1 main flaw: The message in its entirety, must fit into the tempbuffer. So it must be shorter than MAXLOGSIZE

Calls: [generate_message](logger.c.md#generate_message), [log_file_note_lost](logger.c.md#log_file_note_lost), [log_file_recover](logger.c.md#log_file_recover), [log_file_write](logger.c.md#log_file_write), [log_init](logger.c.md#log_init), [make_syslog_entry](logger.c.md#make_syslog_entry), [vasprintf](../compat/misc.c.md#vasprintf)

Uses file-scope variables: `log_config`, `log_lost`, `log_lost_errno`, `log_lost_pid`, `log_src_filename`, `log_src_linenumber`, `type_configs`

### setuplog

```c
void setuplog(const char *logstr, const char *logfile, const bool log_us_timestamp)
```

Defined at lines 887 to 927.

Calls: [setuplog_internal](logger.c.md#setuplog_internal)

Called by: [afp_config_parse](netatalk_conf.c.md#afp_config_parse), [afp_over_dsi](../../etc/afpd/afp_dsi.c.md#afp_over_dsi), [main](../../bin/dbd/cmd_dbd.c.md#main), [main](../../bin/misc/logger_test.c.md#main), [main](../../bin/misc/uuidtest.c.md#main), [main](../../bin/nad/nad.c.md#main)

Mentioned in the documentation of: [log_reopen](logger.c.md#log_reopen)

# Macros

* Undocumented: `COUNT_ARRAY`, `DEFAULT_LOG_CONFIG`, `LOGLEVEL_STRING_IDENTIFIERS`, `LOGTYPE_STRING_IDENTIFIERS`, `MAXLOGSIZE`

# File-scope variables

`arr_loglevel_chars`, `arr_loglevel_strings`, `arr_logtype_strings`, `log_config`, `log_file_dev`, `log_file_ino`, `log_file_midline`, `log_lost`, `log_lost_errno`, `log_lost_pid`, `log_src_filename`, `log_src_linenumber`, `num_loglevel_chars`, `num_loglevel_strings`, `num_logtype_strings`, `type_configs`
