/*
  Copyright (c) 2026 Andy Lemin (andylemin)

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
 *    LD_PRELOAD: interposes the libc calls defined below at dynamic-symbol
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
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
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
typedef int (*open2_fn)(const char *, int);
typedef int (*close_fn)(int);
typedef int (*fcntl_fn)(int, int, ...);
typedef int (*fchdir_fn)(int);
#ifndef dirfd
typedef int (*dirfd_fn)(DIR *);
#endif

static open_fn   real_open;
static open2_fn  real_open_2;
static open2_fn  real_open64_2;
static close_fn  real_close;
static fcntl_fn  real_fcntl;
static fchdir_fn real_fchdir;
#ifndef dirfd
static dirfd_fn  real_dirfd;
#endif

#ifndef O_TMPFILE
#define O_TMPFILE 0
#endif

/* Record then decide, shared by every open-family interposer.  Record before
 * the decision so count/flags reflect the attempt; only while armed.  Returns 1
 * when the caller should inject the failure (errno already set). */
static int open_armed_should_fire(int flags)
{
    if (fi.open_armed) {
        fi.open_calls++;
        fi.open_last_flags = flags;
    }

    return fault_should_fire(fi.open_armed, &fi.open_fail_after, fi.open_errno);
}

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

    if (open_armed_should_fire(flags)) {
        return -1;
    }

    return real_open(path, flags, mode);
}

/* _FORTIFY_SOURCE (default on Ubuntu's GCC, glibc >= 2.42) rewrites a no-mode
 * open(path, flags) to __open_2()/__open64_2() instead of open()/open64().
 * Every of_get_locks/deletefile probe open is no-O_CREAT, so without these
 * interposers an armed failure never fires on a fortified build and a
 * fail-closed test reads a real ENOENT as "no rfork".  No O_CREAT here means no
 * mode argument.  The __asm__ labels pin the real symbol names: under
 * _FILE_OFFSET_BITS=64 the identifier __open_2 would itself emit __open64_2 and
 * collide, so private identifiers keep both interposers distinct (LFS + 32-bit). */
int fi_open_2(const char *path, int flags) __asm__("__open_2");
int fi_open64_2(const char *path, int flags) __asm__("__open64_2");

int fi_open_2(const char *path, int flags)
{
    if (!real_open_2) {
        real_open_2 = (open2_fn)dlsym(RTLD_NEXT, "__open_2");
    }

    if (open_armed_should_fire(flags)) {
        return -1;
    }

    return real_open_2(path, flags);
}

int fi_open64_2(const char *path, int flags)
{
    if (!real_open64_2) {
        real_open64_2 = (open2_fn)dlsym(RTLD_NEXT, "__open64_2");
    }

    if (open_armed_should_fire(flags)) {
        return -1;
    }

    return real_open64_2(path, flags);
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

/* fcntl()'s third argument is absent, an int or a pointer by command; it is
 * read and forwarded as one pointer-sized slot, as libc's own fcntl() reads
 * it, so every command keeps its argument. */
int fcntl(int fd, int cmd, ...)
{
    va_list ap;
    void *arg;
    va_start(ap, cmd);
    arg = va_arg(ap, void *);
    va_end(ap);

    if (!real_fcntl) {
        real_fcntl = (fcntl_fn)dlsym(RTLD_NEXT, "fcntl");
    }

    /* Record lock-fcntl calls so a test can assert HOW a probe locked (count,
     * range, type) — e.g. that a range probe never issues l_len == 0 (which would
     * sweep the share-mode band).  Only while watching, to stay inert otherwise. */
    if (fi.fcntl_watch && arg != NULL
            && (cmd == F_GETLK || cmd == F_SETLK || cmd == F_SETLKW)) {
        const struct flock *flk = arg;

        if (cmd == F_GETLK) {
            fi.getlk_calls++;
        } else {
            fi.setlk_calls++;
        }

        fi.lock_last_type  = flk->l_type;
        fi.lock_last_start = flk->l_start;
        fi.lock_last_len   = flk->l_len;
    }

    if (fault_should_fire(fi.fcntl_armed, &fi.fcntl_fail_after, fi.fcntl_errno)) {
        return -1;
    }

#ifdef F_SETPIPE_SZ

    /* the kernel's refusal above fs.pipe-max-size */
    if (cmd == F_SETPIPE_SZ && fi.pipe_size_limit > 0
            && (int)(intptr_t)arg > fi.pipe_size_limit) {
        errno = EPERM;
        return -1;
    }

#endif
    return real_fcntl(fd, cmd, arg);
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

/* Some systems (notably NetBSD) define dirfd as a DIR-field macro.  Those
 * calls cannot reach an interposer; the veto fault test probes and skips. */
#ifndef dirfd
int dirfd(DIR *dirp)
{
    int fd;

    if (!real_dirfd) {
        real_dirfd = (dirfd_fn)dlsym(RTLD_NEXT, "dirfd");
    }

    fd = real_dirfd(dirp);

    if (fi.dirfd_armed) {
        fi.dirfd_calls++;
    }

    if (fault_should_fire(fi.dirfd_armed, &fi.dirfd_fail_after,
                          fi.dirfd_errno)) {
        fi.dirfd_failed_fd = fd;
        return -1;
    }

    return fd;
}
#endif

#ifdef __linux__
typedef ssize_t (*splice_fn)(int, loff_t *, int, loff_t *, size_t,
                             unsigned int);
static splice_fn real_splice;

/*!
 * @brief splice() interposer: fails armed calls
 */
ssize_t splice(int fd_in, loff_t *off_in, int fd_out, loff_t *off_out,
               size_t len, unsigned int flags)
{
    if (!real_splice) {
        real_splice = (splice_fn)dlsym(RTLD_NEXT, "splice");
    }

    if (off_out != NULL) {
        if (fault_should_fire(fi.splice_file_armed, &fi.splice_file_fail_after,
                              fi.splice_file_errno)) {
            return -1;
        }

        return real_splice(fd_in, off_in, fd_out, off_out, len, flags);
    }

    if (fault_should_fire(fi.splice_sock_armed, &fi.splice_sock_fail_after,
                          fi.splice_sock_errno)) {
        return -1;
    }

    return real_splice(fd_in, off_in, fd_out, off_out, len, flags);
}
#endif /* __linux__ */

#endif /* FAULTINJECT_PRELOAD */
