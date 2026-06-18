/*
  Copyright (c) 2026 andylemin

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

/*!
 * @file
 * @brief Fault-injection framework for the afpdtest unit harness.
 *
 * One source, two roles selected by -DFAULTINJECT_PRELOAD:
 *
 *  - Linked into the afpdtest executable (no define): provides the `fi` control
 *    block and fault_inject_reset().  A test arms a failure on `fi` right before
 *    the operation under test and disarms it right after.
 *
 *  - Built as libfaultinject.so (-DFAULTINJECT_PRELOAD) and loaded via
 *    LD_PRELOAD: interposes open/close/fcntl/fchdir at dynamic-symbol
 *    resolution, so calls made INSIDE libatalk.so (ad_open/ad_close/ad_lock/…)
 *    are intercepted — which a link-time --wrap on the executable cannot reach,
 *    because libatalk is a shared library.  The interposer references the `fi`
 *    block exported by the executable (export_dynamic), forwarding to the real
 *    libc symbol via dlsym(RTLD_NEXT) unless a failure is armed.
 *
 * Trigger model (countdown + errno): the per-call "armed" flag enables
 * interception; "fail_after" = N lets the next N armed calls succeed and fails
 * call N+1 once, with the per-call "errno".  Because LD_PRELOAD interposition is
 * global, the armed flag MUST be set only around the operation under test (and
 * cleared with fault_inject_reset()), or unrelated infrastructure calls get hit.
 *
 * Interception is not reliable on every platform (notably macOS's two-level
 * namespace ignores plain symbol preloads); the self-test probes whether it
 * works and dependent tests skip when it does not.
 */

#include "faultinject.h"

#ifndef FAULTINJECT_PRELOAD

/* ---- Role 1: control block, linked into the afpdtest executable ---- */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

struct fault_inject fi;

void fault_inject_reset(void)
{
    fi = (struct fault_inject) {
        0
    };
}

/*!
 * @brief Is the LD_PRELOAD open() interposer active on this platform?
 *
 * Arms a one-shot open() failure and probes it against the given scratch path.
 * Returns 1 if the injected failure fired (interception works), 0 otherwise
 * (e.g. macOS two-level namespace, or the preload was not loaded).  Leaves `fi`
 * reset.  Injection-dependent tests should gate on this and return TEST_SKIP
 * when it is 0, so coverage stays honest per platform.
 */
int faultinject_open_works(const char *scratch_path)
{
    int worked;
    int fd;
    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_after = 0;          /* fail the very next open() */
    fi.open_errno = EMFILE;
    errno = 0;
    fd = open(scratch_path, O_RDONLY | O_CREAT, 0600);
    /* Interception works iff the armed failure fired (open returned -1/EMFILE);
     * if it returned a real fd the preload is not active on this platform. */
    worked = (fd < 0 && errno == EMFILE);

    if (fd >= 0) {
        close(fd);
        (void)unlink(scratch_path);
    }

    fault_inject_reset();
    return worked;
}

#else /* FAULTINJECT_PRELOAD */

/* ---- Role 2: LD_PRELOAD interposer (libfaultinject.so) ---- */

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>

/* `fi` is defined in the afpdtest executable and exported dynamically; this is
 * the same object the test code arms.  A weak fallback keeps the preload
 * self-consistent if ever loaded into a process that does not define it. */
struct fault_inject __attribute__((weak)) fi;

/* Returns 1 (and sets errno) when this armed counter should fire now. */
static int fault_should_fire(int armed, int *fail_after, int errnum)
{
    if (!armed) {
        return 0;
    }

    if (*fail_after > 0) {
        (*fail_after)--;
        return 0;
    }

    if (*fail_after == 0) {
        *fail_after = -1;       /* fire once, then stop */
        errno = errnum;
        return 1;
    }

    return 0;
}

typedef int (*open_fn)(const char *, int, ...);
typedef int (*close_fn)(int);
typedef int (*fcntl_fn)(int, int, ...);
typedef int (*fchdir_fn)(int);

static open_fn   real_open;
static close_fn  real_close;
static fcntl_fn  real_fcntl;
static fchdir_fn real_fchdir;

#ifndef O_TMPFILE
#define O_TMPFILE 0
#endif

/* A single open() definition covers both symbols libatalk may reference: under
 * glibc with _FILE_OFFSET_BITS=64 (set globally) <fcntl.h> renames THIS
 * definition to open64() — which is exactly the symbol LFS callers reference —
 * while musl/non-LFS keeps it as open().  So there is no separate open64()
 * interposer (a second definition would collide with the renamed one).
 *
 * open() takes a third mode_t argument ONLY when creating; reading the vararg
 * otherwise is undefined behaviour. */
int open(const char *path, int flags, ...)
{
    mode_t mode = 0;

    if (flags & (O_CREAT | O_TMPFILE)) {
        va_list ap;
        va_start(ap, flags);
        mode = (mode_t)va_arg(ap, int);
        va_end(ap);
    }

    if (!real_open) {
        real_open = (open_fn)dlsym(RTLD_NEXT, "open");
    }

    if (fault_should_fire(fi.open_armed, &fi.open_fail_after, fi.open_errno)) {
        return -1;
    }

    return real_open(path, flags, mode);
}

int close(int fd)
{
    if (!real_close) {
        real_close = (close_fn)dlsym(RTLD_NEXT, "close");
    }

    if (fault_should_fire(fi.close_armed, &fi.close_fail_after, fi.close_errno)) {
        /* The fd IS still closed (mirror real close-on-error semantics); only
         * the reported result is the injected failure. */
        real_close(fd);
        return -1;
    }

    return real_close(fd);
}

/* fcntl()'s third argument is command-dependent: absent (F_GETFD/F_GETFL), an
 * int (F_SETFD/F_SETFL/F_DUPFD), or a pointer (F_SETLK/F_GETLK).  The vararg
 * MUST be read and forwarded with the type the caller passed: reading an int
 * arg as void* (or vice versa) is undefined behaviour, and on LP64 reads a
 * 64-bit slot for a 32-bit value.  Since interposition is global we forward
 * every fcntl() correctly, dispatching on the command's arg class -- not just
 * the lock commands we arm. */
int fcntl(int fd, int cmd, ...)
{
    int int_arg = 0;
    void *ptr_arg = NULL;
    /* Commands taking a pointer third arg (struct flock *). */
    int has_ptr = (cmd == F_SETLK || cmd == F_SETLKW || cmd == F_GETLK
#ifdef F_OFD_SETLK
                   || cmd == F_OFD_SETLK || cmd == F_OFD_SETLKW
                   || cmd == F_OFD_GETLK
#endif
                  );
    /* Commands taking an int third arg. */
    int has_int = (cmd == F_SETFD || cmd == F_SETFL || cmd == F_DUPFD
#ifdef F_DUPFD_CLOEXEC
                   || cmd == F_DUPFD_CLOEXEC
#endif
                  );
    va_list ap;

    if (has_ptr) {
        va_start(ap, cmd);
        ptr_arg = va_arg(ap, void *);
        va_end(ap);
    } else if (has_int) {
        va_start(ap, cmd);
        int_arg = va_arg(ap, int);
        va_end(ap);
    }

    if (!real_fcntl) {
        real_fcntl = (fcntl_fn)dlsym(RTLD_NEXT, "fcntl");
    }

    if (fault_should_fire(fi.fcntl_armed, &fi.fcntl_fail_after, fi.fcntl_errno)) {
        return -1;
    }

    if (has_ptr) {
        return real_fcntl(fd, cmd, ptr_arg);
    }

    if (has_int) {
        return real_fcntl(fd, cmd, int_arg);
    }

    return real_fcntl(fd, cmd);
}

int fchdir(int fd)
{
    if (!real_fchdir) {
        real_fchdir = (fchdir_fn)dlsym(RTLD_NEXT, "fchdir");
    }

    if (fault_should_fire(fi.fchdir_armed, &fi.fchdir_fail_after,
                          fi.fchdir_errno)) {
        return -1;
    }

    return real_fchdir(fd);
}

#endif /* FAULTINJECT_PRELOAD */
