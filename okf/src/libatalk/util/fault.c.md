---
type: C Source File
title: "libatalk/util/fault.c"
description: "Fault signal handlers with backtraces, from Samba."
resource: "https://github.com/Netatalk/netatalk/blob/main/libatalk/util/fault.c"
tags: ["libatalk/util"]
status: stable
generated: { by: process:okf-from-doxygen/1, at: 2026-10-12T21:52:14+02:00 }
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

Defined at lines 106 to 171.

Log a backtrace of the calling thread, up to BACKTRACE_STACK_SIZE frames.

The libunwind path names frames on musl, which ships no execinfo. With a signal context it starts at the faulting frame, because musl leaves the signal trampoline without unwind data. The execinfo path always starts at the caller. Runaway recursion shows as one frame repeated, and the frame limit leaves room to see it.

Parameters:
* `uctx`: signal context of a fault, or NULL to start at the caller

Called by: [panic_report](fault.c.md#panic_report)

### panic_report

```c
static void panic_report(const char *why, void *uctx)
```

Defined at lines 179 to 183.

Log a panic reason and a backtrace.

Parameters:
* `why`: reason for the panic
* `uctx`: signal context of a fault, or NULL to start at the caller

Calls: [log_backtrace](fault.c.md#log_backtrace)

Called by: [fault_report](fault.c.md#fault_report), [netatalk_panic](fault.c.md#netatalk_panic)

### netatalk_panic

```c
void netatalk_panic(const char *why)
```

Defined at lines 192 to 195.

Log a panic reason and a backtrace of the caller.

The call returns, and the AFP_PANIC macro aborts after it.

Parameters:
* `why`: reason for the panic

Calls: [panic_report](fault.c.md#panic_report)

### fault_report

```c
static void fault_report(int sig, siginfo_t *info, void *uctx)
```

Defined at lines 211 to 248.

Log a fault signal with its address and a backtrace, then end the process.

A second fault in any thread aborts at once. With a continuation from fault_setup, the call runs it and restores the default action of SIGSEGV and SIGBUS. It then returns, so the faulting instruction runs again and dumps core. Without a continuation it aborts.

Parameters:
* `sig`: signal number
* `info`: signal information with the faulting address, or NULL
* `uctx`: signal context of the fault

Calls: [panic_report](fault.c.md#panic_report)

Called by: [sig_fault](fault.c.md#sig_fault)

Uses file-scope variables: `CatchSignal`, `cont_fn`

### sig_fault

```c
static void sig_fault(int sig, siginfo_t *info, void *ctx)
```

Defined at lines 258 to 261.

Pass a fault signal to fault_report, as the SA_SIGINFO handler that catch_fault_signal installs.

Parameters:
* `sig`: signal number
* `info`: signal information
* `ctx`: signal context of the fault

Calls: [fault_report](fault.c.md#fault_report)

Called by: [catch_fault_signal](fault.c.md#catch_fault_signal)

### catch_fault_signal

```c
static void catch_fault_signal(int signum)
```

Defined at lines 272 to 281.

Install sig_fault for a fault signal, on the alternate stack.

At stack exhaustion the guard page leaves no room for a signal frame. Without SA_ONSTACK the kernel cannot run the handler and kills the process without a report.

Parameters:
* `signum`: signal number

Calls: [sig_fault](fault.c.md#sig_fault)

Called by: [fault_setup](fault.c.md#fault_setup)

### install_altstack

```c
static void install_altstack(char *base, size_t size)
```

Defined at lines 292 to 302.

Make a buffer the alternate signal stack of the calling thread.

A failure only logs an error.

Parameters:
* `base`: stack memory
* `size`: size of base

Called by: [fault_setup](fault.c.md#fault_setup), [fault_setup_thread](fault.c.md#fault_setup_thread)

### fault_setup_thread

```c
void fault_setup_thread(void)
```

Defined at lines 311 to 317.

Give the calling thread its own alternate signal stack.

sigaltstack() is per-thread and survives fork() but not pthread_create(), so a thread that exhausts its stack dies unreported unless it installs one.

Calls: [install_altstack](fault.c.md#install_altstack)

Called by: [iw_main](../../etc/afpd/idle_worker.c.md#iw_main)

### fault_setup

```c
void fault_setup(void(*fn)(void *))
```

Defined at lines 328 to 341.

Install the SIGSEGV and SIGBUS handlers with an alternate signal stack.

The stack covers the calling thread and survives fork. A thread from pthread_create calls fault_setup_thread for a stack.

Parameters:
* `fn`: continuation that fault_report runs before a core dump, or NULL to abort

Calls: [catch_fault_signal](fault.c.md#catch_fault_signal), [install_altstack](fault.c.md#install_altstack)

Called by: [main](../../etc/afpd/main.c.md#main), [main](../../etc/netatalk/netatalk.c.md#main), [main](../../etc/papd/main.c.md#main)

Uses file-scope variables: `cont_fn`

# Macros

* Undocumented: `BACKTRACE_STACK_SIZE`, `FAULT_ALTSTACK_SIZE`, `SAFE_FREE`, `SIGNAL_CAST`

# File-scope variables

`CatchSignal`, `cont_fn`
