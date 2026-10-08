---
type: C Source File
title: "libatalk/util/fault.c"
description: "8 functions, includes 2 project headers."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/fault.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-08T21:56:42+02:00 }
---

Part of the [libatalk/util](../util.md) subsystem. Built from the commit recorded in [build](../../../build.md).

# Includes

* [atalk/logger.h](../../include/atalk/logger.h.md)
* [atalk/util.h](../../include/atalk/util.h.md)
* System headers: `errno.h`, `signal.h`, `stdio.h`, `stdlib.h`, `string.h`, `sys/types.h`, `unistd.h`

# Functions

### log_backtrace

```c
static void log_backtrace(void *uctx)
```

Defined at lines 96 to 161.

Log a backtrace of the calling thread

libunwind names frames on musl, which ships no execinfo; the execinfo path stays for platforms that do. Deep runs of one frame are the signature of runaway recursion, so log enough of them to see it.

Called by: [panic_report](fault.c.md#panic_report)

### panic_report

```c
static void panic_report(const char *why, void *uctx)
```

Defined at lines 166 to 170.

Something really nasty happened - panic !

Calls: [log_backtrace](fault.c.md#log_backtrace)

Called by: [fault_report](fault.c.md#fault_report), [netatalk_panic](fault.c.md#netatalk_panic)

### netatalk_panic

```c
void netatalk_panic(const char *why)
```

Defined at lines 172 to 175.

Calls: [panic_report](fault.c.md#panic_report)

### fault_report

```c
static void fault_report(int sig, siginfo_t *info, void *uctx)
```

Defined at lines 181 to 218.

report a fault

Calls: [panic_report](fault.c.md#panic_report)

Called by: [sig_fault](fault.c.md#sig_fault)

Uses file-scope variables: `CatchSignal`, `cont_fn`

### sig_fault

```c
static void sig_fault(int sig, siginfo_t *info, void *ctx)
```

Defined at lines 223 to 226.

catch serious errors

Calls: [fault_report](fault.c.md#fault_report)

Called by: [catch_fault_signal](fault.c.md#catch_fault_signal)

### catch_fault_signal

```c
static void catch_fault_signal(int signum)
```

Defined at lines 235 to 244.

Install sig_fault for a fault signal, on the alternate stack

SA_ONSTACK is required to report stack exhaustion: the guard page leaves no room for a signal frame, so without it the kernel cannot run the handler and kills the process with no diagnostic.

Calls: [sig_fault](fault.c.md#sig_fault)

Called by: [fault_setup](fault.c.md#fault_setup)

### fault_setup_thread

```c
void fault_setup_thread(void)
```

Defined at lines 266 to 272.

Give the calling thread its own alternate signal stack

sigaltstack() is per-thread and survives fork() but not pthread_create(), so a thread that exhausts its stack dies unreported unless it installs one.

### fault_setup

```c
void fault_setup(void(*fn)(void *))
```

Defined at lines 277 to 290.

setup our fault handlers

Calls: [catch_fault_signal](fault.c.md#catch_fault_signal)

Called by: [main](../../etc/afpd/main.c.md#main), [main](../../etc/netatalk/netatalk.c.md#main), [main](../../etc/papd/main.c.md#main)

Uses file-scope variables: `cont_fn`

# Macros

* Undocumented: `BACKTRACE_STACK_SIZE`, `FAULT_ALTSTACK_SIZE`, `SAFE_FREE`, `SIGNAL_CAST`

# File-scope variables

`CatchSignal`, `cont_fn`
