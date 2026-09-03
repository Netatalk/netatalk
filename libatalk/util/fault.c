/*
   Unix SMB/CIFS implementation.
   Critical Fault handling
   Copyright (C) Andrew Tridgell 1992-1998

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBUNWIND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#elif defined(HAVE_BACKTRACE_SYMBOLS)
#include <execinfo.h>
#endif

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <atalk/logger.h>
#include <atalk/util.h>

#ifndef SIGNAL_CAST
#define SIGNAL_CAST (void (*)(int))
#endif
#ifndef SAFE_FREE
#define SAFE_FREE(x) do { if ((x) != NULL) {free(x); x=NULL;} } while(0)
#endif
#define BACKTRACE_STACK_SIZE 64

/* SIGSTKSZ is not a compile-time constant on current glibc */
#define FAULT_ALTSTACK_SIZE (64 * 1024)

static void (*cont_fn)(void *);

/*!
 * Catch a signal. This should implement the following semantics:
 *
 * 1. The handler remains installed after being called.
 * 2. The signal should be blocked during handler execution.
 */
static void (*CatchSignal(int signum, void (*handler)(int)))(int)
{
#ifdef HAVE_SIGACTION
    struct sigaction act;
    struct sigaction oldact;
    ZERO_STRUCT(act);
    act.sa_handler = handler;
#if 0

    /*
     * We *want* SIGALRM to interrupt a system call.
     */
    if (signum != SIGALRM) {
        act.sa_flags = SA_RESTART;
    }

#endif
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, signum);
    sigaction(signum, &act, &oldact);
    return oldact.sa_handler;
#else /* !HAVE_SIGACTION */
    /* FIXME: need to handle sigvec and systems with broken signal() */
    return signal(signum, handler);
#endif
}

/*!
 * Log a backtrace of the calling thread
 *
 * libunwind names frames on musl, which ships no execinfo; the execinfo path
 * stays for platforms that do. Deep runs of one frame are the signature of
 * runaway recursion, so log enough of them to see it.
 */
static void log_backtrace(void *uctx _U_)
{
#ifdef HAVE_LIBUNWIND
    unw_context_t uc;
    unw_cursor_t cursor;
    unsigned int frame = 0;
    int ret;

    if (uctx != NULL) {
        /* Unwind the frame that faulted. Starting from the handler's own
         * context instead stops at the signal trampoline, which musl does not
         * annotate, hiding the whole call chain that matters */
        ret = unw_init_local2(&cursor, (unw_context_t *) uctx,
                              UNW_INIT_SIGNAL_FRAME);
    } else if (unw_getcontext(&uc) == 0) {
        ret = unw_init_local(&cursor, &uc);
    } else {
        ret = -1;
    }

    if (ret != 0) {
        LOG(log_severe, logtype_default, "BACKTRACE: unavailable");
        return;
    }

    LOG(log_severe, logtype_default, "BACKTRACE:");

    do {
        unw_word_t ip = 0;
        unw_word_t off = 0;
        char name[256];
        unw_get_reg(&cursor, UNW_REG_IP, &ip);

        if (unw_get_proc_name(&cursor, name, sizeof(name), &off) != 0) {
            name[0] = '\0';
        }

        LOG(log_severe, logtype_default, " #%u 0x%llx %s+0x%llx",
            frame++, (unsigned long long)ip,
            name[0] ? name : "??", (unsigned long long)off);
    } while (frame < BACKTRACE_STACK_SIZE && unw_step(&cursor) > 0);

#elif defined(HAVE_BACKTRACE_SYMBOLS)
    void *backtrace_stack[BACKTRACE_STACK_SIZE];
    size_t backtrace_size;
    char **backtrace_strings;
    /* get the backtrace (stack frames) */
    backtrace_size = backtrace(backtrace_stack, BACKTRACE_STACK_SIZE);
    backtrace_strings = backtrace_symbols(backtrace_stack, backtrace_size);
    LOG(log_severe, logtype_default, "BACKTRACE: %d stack frames:", backtrace_size);

    if (backtrace_strings) {
        size_t i;

        for (i = 0; i < backtrace_size; i++) {
            LOG(log_severe, logtype_default, " #%u %s", i, backtrace_strings[i]);
        }

        SAFE_FREE(backtrace_strings);
    }

#else
    LOG(log_severe, logtype_default,
        "BACKTRACE: not built with an unwinder (libunwind or execinfo)");
#endif
}

/*!
 * Something really nasty happened - panic !
 */
static void panic_report(const char *why, void *uctx)
{
    LOG(log_severe, logtype_default, "PANIC: %s", why);
    log_backtrace(uctx);
}

void netatalk_panic(const char *why)
{
    panic_report(why, NULL);
}


/*!
 * report a fault
 */
static void fault_report(int sig, siginfo_t *info, void *uctx)
{
    /* Atomic: the main thread and the idle worker can fault concurrently */
    static int counter;

    if (__atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST) > 0) {
        abort();
    }

    LOG(log_severe, logtype_default,
        "===============================================================");
    LOG(log_severe, logtype_default, "INTERNAL ERROR: Signal %d in pid %d (%s)",
        sig, (int)getpid(), VERSION);

    if (info != NULL) {
        /* An address just past the deepest frame means the guard page, ie the
         * stack was exhausted rather than a bad pointer dereferenced */
        LOG(log_severe, logtype_default,
            "faulting address %p, si_code %d", info->si_addr, info->si_code);
    }

    LOG(log_severe, logtype_default,
        "===============================================================");
    panic_report("internal error", uctx);

    if (cont_fn) {
        cont_fn(NULL);
#ifdef SIGSEGV
        CatchSignal(SIGSEGV, SIGNAL_CAST SIG_DFL);
#endif
#ifdef SIGBUS
        CatchSignal(SIGBUS, SIGNAL_CAST SIG_DFL);
#endif
        return; /* this should cause a core dump */
    }

    abort();
}

/*!
 * catch serious errors
 */
static void sig_fault(int sig, siginfo_t *info, void *ctx)
{
    fault_report(sig, info, ctx);
}

/*!
 * Install sig_fault for a fault signal, on the alternate stack
 *
 * SA_ONSTACK is required to report stack exhaustion: the
 * guard page leaves no room for a signal frame, so without it the kernel
 * cannot run the handler and kills the process with no diagnostic.
 */
static void catch_fault_signal(int signum)
{
    struct sigaction act;
    ZERO_STRUCT(act);
    act.sa_sigaction = sig_fault;
    act.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, signum);
    sigaction(signum, &act, NULL);
}

#ifdef HAVE_SIGALTSTACK
static void install_altstack(char *base, size_t size)
{
    stack_t ss;
    ZERO_STRUCT(ss);
    ss.ss_sp = base;
    ss.ss_size = size;

    if (sigaltstack(&ss, NULL) != 0) {
        LOG(log_error, logtype_default, "sigaltstack: %s", strerror(errno));
    }
}
#endif

/*!
 * Give the calling thread its own alternate signal stack
 *
 * sigaltstack() is per-thread and survives fork() but not pthread_create(), so
 * a thread that exhausts its stack dies unreported unless it installs one.
 */
void fault_setup_thread(void)
{
#ifdef HAVE_SIGALTSTACK
    static _Thread_local char altstack[FAULT_ALTSTACK_SIZE];
    install_altstack(altstack, sizeof(altstack));
#endif
}

/*!
 * setup our fault handlers
 */
void fault_setup(void (*fn)(void *))
{
    cont_fn = fn;
#ifdef HAVE_SIGALTSTACK
    static char altstack[FAULT_ALTSTACK_SIZE];
    install_altstack(altstack, sizeof(altstack));
#endif
#ifdef SIGSEGV
    catch_fault_signal(SIGSEGV);
#endif
#ifdef SIGBUS
    catch_fault_signal(SIGBUS);
#endif
}
