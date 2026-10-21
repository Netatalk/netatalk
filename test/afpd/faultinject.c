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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/sendfile.h>
#endif

#ifdef HAVE_POSIX_ACLS
#include <sys/acl.h>
#endif /* HAVE_POSIX_ACLS */

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

/*! @brief Count an armed call on c and tell whether it fails now */
static int fi_call_fire(struct fi_call *c)
{
    if (c->armed) {
        c->calls++;
    }

    return fault_should_fire(c->armed, &c->fail_after, c->errnum);
}

/*!
 * @brief The real call of an interposer: under 64-bit offsets glibc names the
 *        definition name64, and on a 32-bit build its plain name is the
 *        32-bit call, so name64 is looked up first
 */
static void *fi_real64(const char *name64, const char *name)
{
    void *real = dlsym(RTLD_NEXT, name64);
    return real != NULL ? real : dlsym(RTLD_NEXT, name);
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
 * the decision so count/flags reflect the attempt; only while armed.  An open
 * without the flags of open_fail_flags is neither recorded nor failed.  Returns
 * 1 when the caller should inject the failure (errno already set). */
static int open_armed_should_fire(int flags)
{
    if (!fi.open_armed) {
        return 0;
    }

    if (fi.open_fail_flags
            && (flags & fi.open_fail_flags) != fi.open_fail_flags) {
        return 0;
    }

    fi.open_calls++;
    fi.open_last_flags = flags;
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
        real_open = (open_fn)fi_real64("open64", "open");
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

/*! @brief count, cut to max when max is set */
static size_t fi_cap(size_t count, int max)
{
    return max > 0 && count > (size_t)max ? (size_t)max : count;
}

typedef ssize_t (*read_fn)(int, void *, size_t);
static read_fn real_read;

/*! @brief read() interposer: counts and fails armed calls, and reads no
 *         more than read_max bytes a call when it is set */
ssize_t read(int fd, void *buf, size_t count)
{
    if (!real_read) {
        real_read = (read_fn)dlsym(RTLD_NEXT, "read");
    }

    if (fi_call_fire(&fi.read)) {
        return -1;
    }

    return real_read(fd, buf, fi_cap(count, fi.read_max));
}

typedef ssize_t (*write_fn)(int, const void *, size_t);
static write_fn real_write;

/*!
 * @brief write() interposer: the next write_zero calls write nothing and
 *        return 0, and with write_max set a call writes no more than
 *        write_max bytes
 */
ssize_t write(int fd, const void *buf, size_t count)
{
    if (!real_write) {
        real_write = (write_fn)dlsym(RTLD_NEXT, "write");
    }

    if (fi.write_zero > 0) {
        fi.write_zero--;
        return 0;
    }

    return real_write(fd, buf, fi_cap(count, fi.write_max));
}

typedef ssize_t (*pread_fn)(int, void *, size_t, off_t);
static pread_fn real_pread;

/*! @brief pread() interposer: counts every call, and has read()'s countdown
 *         and cap */
ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
    if (!real_pread) {
        real_pread = (pread_fn)fi_real64("pread64", "pread");
    }

    fi.pread_calls++;

    if (fi_call_fire(&fi.read)) {
        return -1;
    }

    return real_pread(fd, buf, fi_cap(count, fi.read_max), offset);
}

typedef ssize_t (*pwrite_fn)(int, const void *, size_t, off_t);
static pwrite_fn real_pwrite;

/*! @brief pwrite() interposer: counts and fails armed calls */
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    if (!real_pwrite) {
        real_pwrite = (pwrite_fn)fi_real64("pwrite64", "pwrite");
    }

    if (fi_call_fire(&fi.pwrite)) {
        return -1;
    }

    return real_pwrite(fd, buf, count, offset);
}

typedef off_t (*lseek_fn)(int, off_t, int);
static lseek_fn real_lseek;

/*! @brief lseek() interposer: counts and fails armed calls */
off_t lseek(int fd, off_t offset, int whence)
{
    if (!real_lseek) {
        real_lseek = (lseek_fn)fi_real64("lseek64", "lseek");
    }

    if (fi_call_fire(&fi.lseek)) {
        return -1;
    }

    return real_lseek(fd, offset, whence);
}

typedef int (*asp_wrtcont_fn)(void *, char *, size_t *);
static asp_wrtcont_fn real_asp_wrtcont;

/*!
 * @brief asp_wrtcont() interposer: counts and fails armed calls, or with
 *        asp_wrtcont_fill hands back the bytes asked for, 0xa5 each, as an
 *        AppleTalk client's write continuation would
 */
int asp_wrtcont(void *asp, char *buf, size_t *buflen)
{
    if (fi_call_fire(&fi.asp_wrtcont)) {
        return -1;
    }

    if (fi.asp_wrtcont_fill) {
        memset(buf, 0xa5, *buflen);
        return 0;
    }

    if (!real_asp_wrtcont) {
        real_asp_wrtcont = (asp_wrtcont_fn)dlsym(RTLD_NEXT, "asp_wrtcont");
    }

    return real_asp_wrtcont(asp, buf, buflen);
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

        fi.lock_last_cmd   = cmd;
        fi.lock_last_type  = flk->l_type;
        fi.lock_last_start = flk->l_start;
        fi.lock_last_len   = flk->l_len;
    }

    /* another process's truncate, landing after the caller sized the file */
    if (fi.shrink_armed && (cmd == F_SETLK || cmd == F_SETLKW)) {
        fi.shrink_armed = 0;

        if (ftruncate(fi.shrink_fd, fi.shrink_to) != 0) {
            return -1;
        }
    }

    /* another process's rename onto a name, landing after the caller's stat
     * of it */
    if (fi.rename_armed && (cmd == F_SETLK || cmd == F_SETLKW)) {
        fi.rename_armed = 0;

        if (rename(fi.rename_from, fi.rename_to) != 0) {
            return -1;
        }
    }

    if (fault_should_fire(fi.fcntl_armed, &fi.fcntl_fail_after, fi.fcntl_errno)) {
        return -1;
    }

#ifdef F_SETPIPE_SZ

    /* over the pipe budget the kernel makes two-page pipes and grows none */
    if (fi.pipe_budget_spent && cmd == F_GETPIPE_SZ) {
        return 2 * (int)sysconf(_SC_PAGESIZE);
    }

    if (cmd == F_SETPIPE_SZ) {
        int size = (int)(intptr_t)arg;

        /* the kernel's refusal over the budget or above fs.pipe-max-size */
        if (fi.pipe_budget_spent
                || (fi.pipe_size_limit > 0 && size > fi.pipe_size_limit)) {
            errno = EPERM;
            return -1;
        }
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
typedef int (*pipe2_fn)(int[2], int);
static pipe2_fn real_pipe2;

/*! @brief pipe2() interposer: counts and fails armed calls */
int pipe2(int fds[2], int flags)
{
    if (!real_pipe2) {
        real_pipe2 = (pipe2_fn)dlsym(RTLD_NEXT, "pipe2");
    }

    if (fi_call_fire(&fi.pipe2)) {
        return -1;
    }

    return real_pipe2(fds, flags);
}

typedef ssize_t (*splice_fn)(int, loff_t *, int, loff_t *, size_t,
                             unsigned int);
static splice_fn real_splice;

/*!
 * @brief splice() interposer: counts and fails armed calls, a call with an
 *        output offset on the file side and one without on the socket side
 */
ssize_t splice(int fd_in, loff_t *off_in, int fd_out, loff_t *off_out,
               size_t len, unsigned int flags)
{
    struct fi_call *side = off_out != NULL ? &fi.splice_file
                               : &fi.splice_sock;

    if (!real_splice) {
        real_splice = (splice_fn)dlsym(RTLD_NEXT, "splice");
    }

    if (fi_call_fire(side)) {
        return -1;
    }

    return real_splice(fd_in, off_in, fd_out, off_out, len, flags);
}

typedef int (*poll_fn)(struct pollfd *, nfds_t, int);
static poll_fn real_poll;

/*!
 * @brief poll() interposer: with poll_urgent, a wait for POLLPRI on one
 *        descriptor reports an urgent byte and no data at once, as an urgent
 *        byte past a gap in the stream does, and every one-descriptor wait
 *        is counted
 */
int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    if (!real_poll) {
        real_poll = (poll_fn)dlsym(RTLD_NEXT, "poll");
    }

    if (fi.poll_urgent && nfds == 1) {
        fi.poll_waits++;

        if (fds[0].events & POLLPRI) {
            fds[0].revents = POLLPRI;
            fi.poll_urgent_wakeups++;
            return 1;
        }
    }

    return real_poll(fds, nfds, timeout);
}
#endif /* __linux__ */

#if defined(__linux__) || defined(__FreeBSD__)
#ifdef __linux__
typedef ssize_t (*sendfile_fn)(int, int, off_t *, size_t);
#else
typedef int (*sendfile_fn)(int, int, off_t, size_t, struct sf_hdtr *, off_t *,
                           int);
#endif
static sendfile_fn real_sendfile;

/*! @brief Count an armed sendfile() call; does it fail? */
static int sendfile_should_fire(void)
{
    if (!real_sendfile) {
        real_sendfile = (sendfile_fn)fi_real64("sendfile64", "sendfile");
    }

    return fi_call_fire(&fi.sendfile);
}
#endif

#ifdef __linux__
typedef ssize_t (*send_fn)(int, const void *, size_t, int);
static send_fn real_send;

/*! @brief send() interposer: counts and fails armed calls, and sends no
 *         more than send_max bytes a call when it is set */
ssize_t send(int fd, const void *buf, size_t len, int flags)
{
    if (!real_send) {
        real_send = (send_fn)dlsym(RTLD_NEXT, "send");
    }

    if (fi_call_fire(&fi.send)) {
        return -1;
    }

    return real_send(fd, buf, fi_cap(len, fi.send_max), flags);
}

/*! @brief sendfile() interposer: counts and fails armed calls; with
 *         sendfile_fake reports a call that sent its count, moving none, and
 *         with sendfile_eof_einval fails a call at the end of the file with
 *         EINVAL, as illumos's sendfilev() does */
ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count)
{
    ssize_t len;

    if (sendfile_should_fire()) {
        return -1;
    }

    if (fi.sendfile_fake) {
        /* a native 32-bit kernel refuses a count no ssize_t holds */
        if (count > SSIZE_MAX) {
            errno = EINVAL;
            return -1;
        }

        /* the most one Linux call sends */
        if (count > 0x7ffff000) {
            count = 0x7ffff000;
        }

        *offset += (off_t)count;
        return (ssize_t)count;
    }

    len = real_sendfile(out_fd, in_fd, offset, count);

    if (len == 0 && count > 0 && fi.sendfile_eof_einval) {
        errno = EINVAL;
        return -1;
    }

    return len;
}
#elif defined(__FreeBSD__)
/*! @brief sendfile() interposer: counts and fails armed calls, sending none */
int sendfile(int fd, int s, off_t offset, size_t nbytes, struct sf_hdtr *hdtr,
             off_t *sbytes, int flags)
{
    if (sendfile_should_fire()) {
        if (sbytes != NULL) {
            *sbytes = 0;
        }

        return -1;
    }

    return real_sendfile(fd, s, offset, nbytes, hdtr, sbytes, flags);
}
#endif

typedef int (*fstat_fn)(int, void *);
static fstat_fn real_fstat;
static fstat_fn real_fstat64;
static fstat_fn real_fstat50;
static fstat_fn real_fstat64_time64;
static fstat_fn real_fstat_time64;

/* a large-file glibc build calls fstat64, NetBSD's headers rename fstat()
 * to __fstat50, a 32-bit build with a 64-bit time_t calls __fstat64_time64
 * (glibc) or __fstat_time64 (musl), other builds call fstat; each forwards
 * to its own name, so the caller's struct stat layout is kept */
int fi_fstat(int fd, void *st) __asm__("fstat");
int fi_fstat64(int fd, void *st) __asm__("fstat64");
int fi_fstat50(int fd, void *st) __asm__("__fstat50");
int fi_fstat64_time64(int fd, void *st) __asm__("__fstat64_time64");
int fi_fstat_time64(int fd, void *st) __asm__("__fstat_time64");

/*! @brief Count an fstat() of the watched descriptor */
static void fstat_watched(int fd)
{
    if (fi.fstat_watch && fd == fi.fstat_fd) {
        fi.fstat_calls++;
    }
}

/*! @brief fstat() interposer: counts watched calls on fi.fstat_fd */
int fi_fstat(int fd, void *st)
{
    if (!real_fstat) {
        real_fstat = (fstat_fn)dlsym(RTLD_NEXT, "fstat");
    }

    fstat_watched(fd);
    return real_fstat(fd, st);
}

/*! @brief fstat64() interposer: counts watched calls on fi.fstat_fd */
int fi_fstat64(int fd, void *st)
{
    if (!real_fstat64) {
        real_fstat64 = (fstat_fn)dlsym(RTLD_NEXT, "fstat64");
    }

    fstat_watched(fd);
    return real_fstat64(fd, st);
}

/*! @brief __fstat50() interposer: counts watched calls on fi.fstat_fd */
int fi_fstat50(int fd, void *st)
{
    if (!real_fstat50) {
        real_fstat50 = (fstat_fn)dlsym(RTLD_NEXT, "__fstat50");
    }

    fstat_watched(fd);
    return real_fstat50(fd, st);
}

/*! @brief __fstat64_time64() interposer: counts watched calls on fi.fstat_fd */
int fi_fstat64_time64(int fd, void *st)
{
    if (!real_fstat64_time64) {
        real_fstat64_time64 = (fstat_fn)dlsym(RTLD_NEXT, "__fstat64_time64");
    }

    fstat_watched(fd);
    return real_fstat64_time64(fd, st);
}

/*! @brief __fstat_time64() interposer: counts watched calls on fi.fstat_fd */
int fi_fstat_time64(int fd, void *st)
{
    if (!real_fstat_time64) {
        real_fstat_time64 = (fstat_fn)dlsym(RTLD_NEXT, "__fstat_time64");
    }

    fstat_watched(fd);
    return real_fstat_time64(fd, st);
}

typedef time_t (*time_fn)(time_t *);
typedef int64_t (*time64_fn)(int64_t *);
static time_fn real_time;
static time_fn real_time50;
static time64_fn real_time64;

/* NetBSD's headers rename time() to __time50, a 32-bit build with a 64-bit
 * time_t calls __time64, whose time_t this build's may not be, and other
 * builds call time; pinned names keep a header's rename off the
 * definitions */
time_t fi_time(time_t *t) __asm__("time");
time_t fi_time50(time_t *t) __asm__("__time50");
int64_t fi_time64(int64_t *t) __asm__("__time64");

/*! @brief Count a time() call while fi.time_watch is set */
static void time_watched(void)
{
    if (fi.time_watch) {
        fi.time_calls++;
    }
}

/*! @brief time() interposer: counts calls while fi.time_watch is set */
time_t fi_time(time_t *t)
{
    if (!real_time) {
        real_time = (time_fn)dlsym(RTLD_NEXT, "time");
    }

    time_watched();
    return real_time(t);
}

/*! @brief __time50() interposer: counts calls while fi.time_watch is set */
time_t fi_time50(time_t *t)
{
    if (!real_time50) {
        real_time50 = (time_fn)dlsym(RTLD_NEXT, "__time50");
    }

    time_watched();
    return real_time50(t);
}

/*! @brief __time64() interposer: counts calls while fi.time_watch is set */
int64_t fi_time64(int64_t *t)
{
    if (!real_time64) {
        real_time64 = (time64_fn)dlsym(RTLD_NEXT, "__time64");
    }

    time_watched();
    return real_time64(t);
}

typedef int (*statvfs_fn)(const char *, struct statvfs *);
static statvfs_fn real_statvfs;

/*!
 * @brief statvfs() interposer: reports the filesystem read-only while armed
 *
 * The header renames this definition where libc versions the call (glibc
 * statvfs64, NetBSD __statvfs90); the real function with the same struct
 * carries that name too, so it is looked up under this definition's own
 * name first, when dladdr() names this very definition. A call whose real
 * function is not found fails with ENOSYS.
 */
int statvfs(const char *path, struct statvfs *buf)
{
    int ret;

    if (!real_statvfs) {
        Dl_info self;

        if (dladdr((void *)statvfs, &self) && self.dli_sname
                && self.dli_saddr == (void *)statvfs) {
            real_statvfs = (statvfs_fn)dlsym(RTLD_NEXT, self.dli_sname);
        }

        if (!real_statvfs) {
            real_statvfs = (statvfs_fn)fi_real64("statvfs64", "statvfs");
        }

        if (!real_statvfs) {
            errno = ENOSYS;
            return -1;
        }
    }

    ret = real_statvfs(path, buf);

    if (ret == 0 && fi.statvfs_rdonly) {
        buf->f_flag |= ST_RDONLY;
    }

    return ret;
}

typedef int (*access_fn)(const char *, int);
static access_fn real_access;

/*!
 * @brief access() interposer: fails an armed call with the armed errno
 */
int access(const char *path, int mode)
{
    if (!real_access) {
        real_access = (access_fn)dlsym(RTLD_NEXT, "access");
    }

    if (fault_should_fire(fi.access_armed, &fi.access_fail_after,
                          fi.access_errno)) {
        return -1;
    }

    return real_access(path, mode);
}

/* No <utime.h>: NetBSD renames utime() there, and a renamed definition
 * would take the 64-bit call yet forward it to the old one */
struct utimbuf;
typedef int (*utime_fn)(const char *, const struct utimbuf *);
static utime_fn real_utime;

/*!
 * @brief utime() interposer: counts watched calls and fails an armed one
 */
int utime(const char *path, const struct utimbuf *times)
{
    if (!real_utime) {
        real_utime = (utime_fn)dlsym(RTLD_NEXT, "utime");
    }

    if (fi.utime_watch) {
        fi.utime_calls++;
    }

    if (fi_call_fire(&fi.utime)) {
        return -1;
    }

    return real_utime(path, times);
}

typedef int (*utimensat_fn)(int, const char *, const struct timespec *, int);
static utimensat_fn real_utimensat;

/*!
 * @brief utimensat() interposer: counts watched calls with utime()'s, fails
 *        an armed one with them, and stamps a call without times
 *        utime_lag_ms behind the clock
 *
 * <sys/stat.h> renames this definition where libc versions the call (glibc
 * with 64-bit time on a 32-bit build), so the real function is looked up
 * under this definition's own name first, as statvfs() is.
 */
int utimensat(int dirfd, const char *path, const struct timespec *times,
              int flags)
{
    struct timespec lag[2];

    if (!real_utimensat) {
        Dl_info self;

        if (dladdr((void *)utimensat, &self) && self.dli_sname
                && self.dli_saddr == (void *)utimensat) {
            real_utimensat = (utimensat_fn)dlsym(RTLD_NEXT, self.dli_sname);
        }

        if (!real_utimensat) {
            real_utimensat = (utimensat_fn)dlsym(RTLD_NEXT, "utimensat");
        }
    }

    if (fi.utime_watch) {
        fi.utime_calls++;
    }

    if (fi_call_fire(&fi.utime)) {
        return -1;
    }

    if (times == NULL && fi.utime_lag_ms > 0
            && clock_gettime(CLOCK_REALTIME, &lag[0]) == 0) {
        lag[0].tv_sec -= fi.utime_lag_ms / 1000;
        lag[0].tv_nsec -= (fi.utime_lag_ms % 1000) * 1000L * 1000L;

        if (lag[0].tv_nsec < 0) {
            lag[0].tv_nsec += 1000L * 1000L * 1000L;
            lag[0].tv_sec--;
        }

        lag[1] = lag[0];
        times = lag;
    }

    return real_utimensat(dirfd, path, times, flags);
}

/* the clock coarse_time() reads, by its choice */
#if defined(CLOCK_REALTIME_COARSE)
#define FI_COARSE_CLOCK CLOCK_REALTIME_COARSE
#elif defined(CLOCK_REALTIME_FAST)
#define FI_COARSE_CLOCK CLOCK_REALTIME_FAST
#else
#define FI_COARSE_CLOCK CLOCK_REALTIME
#endif

typedef int (*clock_gettime_fn)(clockid_t, struct timespec *);
static clock_gettime_fn real_clock_gettime;

/*!
 * @brief clock_gettime() interposer: while clock_fake is set, the wall clock
 *        and the clock coarse_time() reads answer clock_sec, and
 *        clock_calls counts those answers
 *
 * <time.h> renames this definition where libc versions the call (glibc with
 * 64-bit time on a 32-bit build), so the real function is looked up under
 * this definition's own name first, as utimensat()'s is.
 */
int clock_gettime(clockid_t clk, struct timespec *ts)
{
    if (!real_clock_gettime) {
        Dl_info self;

        if (dladdr((void *)clock_gettime, &self) && self.dli_sname
                && self.dli_saddr == (void *)clock_gettime) {
            real_clock_gettime = (clock_gettime_fn)dlsym(RTLD_NEXT,
                                                         self.dli_sname);
        }

        if (!real_clock_gettime) {
            real_clock_gettime = (clock_gettime_fn)dlsym(RTLD_NEXT,
                                                         "clock_gettime");
        }
    }

    if (fi.clock_fake
            && (clk == CLOCK_REALTIME || clk == FI_COARSE_CLOCK)) {
        fi.clock_calls++;
        ts->tv_sec = fi.clock_sec;
        ts->tv_nsec = 0;
        return 0;
    }

    return real_clock_gettime(clk, ts);
}

#ifdef HAVE_POSIX_ACLS
typedef acl_t (*acl_get_file_fn)(const char *, acl_type_t);
static acl_get_file_fn real_acl_get_file;

/*!
 * @brief acl_get_file() interposer: counts watched calls
 */
acl_t acl_get_file(const char *path, acl_type_t type)
{
    if (!real_acl_get_file) {
        real_acl_get_file = (acl_get_file_fn)dlsym(RTLD_NEXT, "acl_get_file");
    }

    if (fi.acl_get_file_watch) {
        fi.acl_get_file_calls++;
    }

    return real_acl_get_file(path, type);
}
#endif /* HAVE_POSIX_ACLS */

typedef int (*unlink_fn)(const char *);
static unlink_fn real_unlink;

/*!
 * @brief unlink() interposer: counts and fails armed calls
 */
int unlink(const char *path)
{
    if (!real_unlink) {
        real_unlink = (unlink_fn)dlsym(RTLD_NEXT, "unlink");
    }

    if (fi_call_fire(&fi.unlink)) {
        return -1;
    }

    return real_unlink(path);
}

typedef struct dirent *(*readdir_fn)(DIR *);
static readdir_fn real_readdir;

/*!
 * @brief readdir() interposer: counts and fails armed calls
 *
 * <dirent.h> renames this definition where libc versions the call (glibc
 * readdir64 under 64-bit offsets, NetBSD __readdir30), so the real function
 * is looked up under this definition's own name first, as statvfs() is.
 */
struct dirent *readdir(DIR *dp)
{
    if (!real_readdir) {
        Dl_info self;

        if (dladdr((void *)readdir, &self) && self.dli_sname
                && self.dli_saddr == (void *)readdir) {
            real_readdir = (readdir_fn)dlsym(RTLD_NEXT, self.dli_sname);
        }

        if (!real_readdir) {
            real_readdir = (readdir_fn)fi_real64("readdir64", "readdir");
        }
    }

    if (fi_call_fire(&fi.readdir)) {
        return NULL;
    }

    return real_readdir(dp);
}

typedef char *(*getcwd_fn)(char *, size_t);
static getcwd_fn real_getcwd;

/*!
 * @brief getcwd() interposer: counts and fails armed calls
 */
char *getcwd(char *buf, size_t size)
{
    if (!real_getcwd) {
        real_getcwd = (getcwd_fn)dlsym(RTLD_NEXT, "getcwd");
    }

    if (fi_call_fire(&fi.getcwd)) {
        return NULL;
    }

    return real_getcwd(buf, size);
}

#endif /* FAULTINJECT_PRELOAD */
