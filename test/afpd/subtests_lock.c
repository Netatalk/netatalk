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
 * @brief Unit tests for afpd fork/adouble open, close and locking paths.
 *
 * Uses the LD_PRELOAD fault-injection shim (faultinject.c) to force libc
 * failures the black-box spectest cannot reach.  Each test returns 0 on
 * success, or a small nonzero code identifying the failure point (surfaced by
 * the harness as "# got: N").
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <signal.h>

#include <atalk/adouble.h>
#include <atalk/directory.h>
#include <atalk/logger.h>
#include <atalk/volume.h>

#include "directory.h"
#include "faultinject.h"
#include "file.h"
#include "fork.h"
#include "subtests_lock.h"
#include "test.h"      /* TEST_SKIP */
#include "test_capabilities.h"

/*!
 * @brief Framework capability probe: does fault injection reach into libatalk?
 *
 * Category: framework integrity (meta-test).  Not a netatalk behavioural test —
 * it validates the harness.  The LD_PRELOAD shim interposes libc symbols at
 * dynamic-link resolution; this arms an EMFILE on the next open() and calls
 * ad_open() (whose open() lives in the shared libatalk.so), then checks the
 * open failed with the injected errno.
 *
 * Interception is not reliable on every platform — macOS's two-level namespace
 * ignores a plain symbol preload, so the injected fault never fires there.  In
 * that case this returns TEST_SKIP rather than failing: the result is then
 * honest per-platform (injection-based tests run where the mechanism works and
 * skip where it does not).
 *
 * The errno check lives only in the probe, which owns a direct open() it
 * controls; after ad_open() returns, errno is unreliable (its EC_* error path
 * runs close()/logging that can overwrite it), so the libatalk leg asserts only
 * that the open FAILED while armed, not the specific errno.
 */
int utest_faultinject_selftest(const struct vol *vol)
{
    struct adouble ad;
    char path[MAXPATHLEN + 1];
    snprintf(path, sizeof(path), "%s/fi_selftest", vol->v_path);

    /* Capability probe: does the preload intercept open() here, with the armed
     * errno?  If not (e.g. macOS two-level namespace), skip — injection-
     * dependent tests skip too. */
    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    /* It works; also confirm injection reaches open() *inside libatalk* (via
     * ad_open), not just a direct libc open from the executable. */
    ad_init(&ad, vol);

    if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                0666) != 0) {
        return 1;                    /* setup create failed */
    }

    ad_close(&ad, ADFLAGS_DF);
    ad_init(&ad, vol);
    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_after = 0;          /* fail the very next open */
    fi.open_errno = EMFILE;
    int ad_ret = ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR, 0666);
    fault_inject_reset();

    if (ad_ret == 0) {
        ad_close(&ad, ADFLAGS_DF);   /* libatalk open not intercepted */
        (void)unlink(path);
        return 2;
    }

    (void)unlink(path);
    return 0;
}

/* Is fd a currently-valid descriptor in this process? */
static int fd_is_open(int fd)
{
    return fd >= 0 && fcntl(fd, F_GETFD) != -1;
}

/*!
 * @brief afp_openfork() error cleanup closes an open fork's fd (no leak).
 *
 * Category: accounting invariant.  The invariant: after the open-error cleanup
 * ad_close(DF|RF|HF|SETSHRMD), no fork fd survives.  When afp_openfork() fails
 * partway it must close whatever the first ad_open() opened, or that fd leaks
 * into the long-lived child.  Verified by accounting (not an fd-count scan, which
 * is racy and Linux-only): capture the raw data-fork fd, run the cleanup, and
 * assert two independent ledgers agree the fd is gone — the ad layer
 * (ad_data_fileno == -1) and the kernel (fcntl(fd) -> EBADF).  A negative control
 * (fd open before cleanup) keeps the assertion from passing vacuously.
 */
int utest_openfork_no_fd_leak(const struct vol *vol)
{
    struct adouble ad;
    char path[MAXPATHLEN + 1];
    int dfd;
    snprintf(path, sizeof(path), "%s/fi_leak", vol->v_path);
    ad_init(&ad, vol);

    /* First ad_open(): opens a real data-fork fd (refcount 1). */
    if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                0666) != 0) {
        return 1;                    /* setup open failed */
    }

    dfd = ad_data_fileno(&ad);

    /* Negative control: before cleanup the fd must be open, else the leak
     * assertion below would pass vacuously. */
    if (!fd_is_open(dfd)) {
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 2;                    /* fd not open after a successful open?! */
    }

    /* Run afp_openfork()'s error-path cleanup (close keyed on open-state). */
    if (AD_DATA_OPEN(&ad) || AD_META_OPEN(&ad) || AD_RSRC_OPEN(&ad)) {
        ad_close(&ad, (ADFLAGS_DF | ADFLAGS_RF | ADFLAGS_HF) | ADFLAGS_SETSHRMD);
    }

    (void)unlink(path);

    /* ad-layer accounting: the data-fork fileno must be reset. */
    if (ad_data_fileno(&ad) != -1) {
        return 3;                    /* ad layer still records the fd open */
    }

    /* kernel truth: the captured fd must really be closed (the leak check). */
    if (fd_is_open(dfd)) {
        return 4;                    /* fd leaked: cleanup did not close it */
    }

    return 0;
}

/*!
 * @brief Shared-adouble open/close refcount + fileno ledger stays balanced.
 *
 * Category: accounting invariant (broad).  When several forks reference one
 * inode they share a single struct adouble; ad_open_df()'s already-open path
 * bumps the fd-level adf_refcount instead of opening a second fd, and ad_close()
 * decrements it, closing the fd only at the last reference.  This pins down that
 * whole family with one test by walking the ledger across a multi-reference
 * open/close sequence and asserting the two coupled invariants at every step:
 *   - the fd is open  iff  adf_refcount > 0   (fileno reset to -1 exactly at 0);
 *   - refcount tracks references exactly (N opens need N closes; an early close
 *     must not close the shared fd while a sibling reference remains).
 * It is the regression net that protects every open/close/cleanup path the
 * fork series touches.
 */
int utest_shared_adouble_refcount_balance(const struct vol *vol)
{
    struct adouble ad;
    char path[MAXPATHLEN + 1];
    int dfd;
    snprintf(path, sizeof(path), "%s/fi_shared", vol->v_path);
    ad_init(&ad, vol);

    /* Reference 1: real open. */
    if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                0666) != 0) {
        return 1;                    /* setup open failed */
    }

    dfd = ad_data_fileno(&ad);

    if (dfd < 0 || ad.ad_data_fork.adf_refcount != 1 || !fd_is_open(dfd)) {
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 2;                    /* fresh open: expected refcount 1, fd open */
    }

    /* Reference 2: same adouble already open -> refcount++, same fd (no 2nd open). */
    if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR, 0666) != 0) {
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 3;                    /* second reference open failed */
    }

    if (ad.ad_data_fork.adf_refcount != 2 || ad_data_fileno(&ad) != dfd) {
        (void)unlink(path);
        return 4;                    /* 2nd ref must share the fd, refcount 2 */
    }

    /* Release reference 2: refcount 2->1, fd must stay open (sibling holds it). */
    ad_close(&ad, ADFLAGS_DF);

    if (ad.ad_data_fork.adf_refcount != 1) {
        (void)unlink(path);
        return 5;                    /* refcount did not drop to 1 */
    }

    if (ad_data_fileno(&ad) != dfd || !fd_is_open(dfd)) {
        (void)unlink(path);
        return 6;                    /* shared fd wrongly closed at refcount 1 */
    }

    /* Release reference 1 (last): refcount 1->0, fd closed and fileno reset. */
    ad_close(&ad, ADFLAGS_DF);
    (void)unlink(path);

    if (ad.ad_data_fork.adf_refcount != 0) {
        return 7;                    /* last close did not zero the refcount */
    }

    if (ad_data_fileno(&ad) != -1) {
        return 8;                    /* fileno not reset at last close */
    }

    if (fd_is_open(dfd)) {
        return 9;                    /* fd leaked: not closed at last reference */
    }

    return 0;
}

/*!
 * @brief ad_close() hard-fails on an adf_refcount underflow.
 *
 * Category: targeted (nuanced).  Accounting invariants are too broad to reach
 * this: the underflow guard's else-branch fires only in a desync no correct
 * open/close sequence produces — fileno still valid (>= 0) yet adf_refcount
 * already <= 0 (a balanced close clears the fileno exactly as the refcount hits
 * 0).  The guard is defense-in-depth against a future stray double-close on a
 * shared adouble, so the test deliberately manufactures that precise state:
 * open the data fork, force adf_refcount to 0, then ad_close() — the outer
 * fileno guard passes, the refcount else fires, AFP_ASSERT (live in
 * debugoptimized) abort()s.
 *
 * AFP_ASSERT abort()s the process, so run it in a fork()ed child and assert the
 * child died via SIGABRT; the parent (the harness) survives.  AFP_ASSERT
 * self-gates on NDEBUG: in a release/NDEBUG build it compiles to a no-op and the
 * guard cannot abort, so the test skips there rather than failing — its result
 * is correctly build-type-aware.
 */
int utest_adclose_underflow_aborts(const struct vol *vol)
{
#ifdef NDEBUG
    /* AFP_ASSERT is a no-op here; the guard logs but cannot abort. */
    (void)vol;
    return TEST_SKIP;
#else
    char path[MAXPATHLEN + 1];
    pid_t pid;
    int status;
    snprintf(path, sizeof(path), "%s/fi_underflow", vol->v_path);
    pid = fork();

    if (pid < 0) {
        LOG(log_error, logtype_afpd, "underflow: fork failed: %s", strerror(errno));
        return 1;
    }

    if (pid == 0) {
        /* Child: manufacture fileno>=0 with adf_refcount==0, then close. */
        struct adouble ad;
        ad_init(&ad, vol);

        if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                    0666) != 0) {
            _exit(2);                /* setup failure, distinct from SIGABRT */
        }

        /* Desync: fd still open, but the fd-level refcount is exhausted. */
        ad.ad_data_fork.adf_refcount = 0;
        ad_close(&ad, ADFLAGS_DF);   /* fileno>=0 && refcount<=0 -> guard abort */
        _exit(3);                    /* must not reach: guard did not fire */
    }

    /* Parent: reap and check the child aborted. */
    if (waitpid(pid, &status, 0) != pid) {
        LOG(log_error, logtype_afpd, "underflow: waitpid failed: %s",
            strerror(errno));
        (void)unlink(path);
        return 4;
    }

    (void)unlink(path);

    if (WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT) {
        return 0;                    /* guard fired as designed */
    }

    if (WIFEXITED(status)) {
        LOG(log_error, logtype_afpd,
            "underflow: child exited %d without aborting — guard did not fire",
            WEXITSTATUS(status));
    } else {
        LOG(log_error, logtype_afpd,
            "underflow: child died on signal %d, expected SIGABRT",
            WTERMSIG(status));
    }

    return 5;
#endif
}

/*!
 * @brief Read-only downgrade retry re-opens without destructive flags.
 *
 * Category: targeted (nuanced).  ad_open_df() retries a failed open() read-only
 * when the caller passed ADFLAGS_SETSHRMD | ADFLAGS_RDONLY and attempt 1 hit
 * EACCES/EPERM/EROFS; the retry must strip O_TRUNC/O_CREAT/O_EXCL, not just flip
 * the access mode, or a SETSHRMD|RDONLY|TRUNC caller would truncate on the retry
 * a file it asked to open read-only.  No fd/refcount ledger sees this -- the bug
 * is in which flags the second open() carries -- so drive the retry and inspect
 * the flags the shim recorded.  ad2openflags() makes attempt 1 O_RDWR|O_TRUNC;
 * arming EACCES on it forces the retry, whose flags land in open_last_flags
 * (open_calls == 2).  Skips where the shim cannot intercept libatalk's open().
 */
int utest_ro_retry_strips_destructive_flags(const struct vol *vol)
{
    struct adouble ad;
    char path[MAXPATHLEN + 1];
    int ad_ret, retry_flags, calls;
    snprintf(path, sizeof(path), "%s/fi_ro_retry", vol->v_path);

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    /* Disarm any injection a prior test left armed before the pre-create setup,
     * so a stale fi.open_armed cannot fail it or skew open_calls. */
    fault_inject_reset();
    /* Pre-create: the RDONLY retry (no O_CREAT) needs the file to exist. */
    ad_init(&ad, vol);

    if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR | ADFLAGS_CREATE,
                0666) != 0) {
        (void)unlink(path);          /* may have been created before failing */
        return 1;                    /* setup create failed */
    }

    ad_close(&ad, ADFLAGS_DF);
    ad_init(&ad, vol);
    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_after = 0;           /* attempt 1 fails EACCES; retry runs */
    fi.open_errno = EACCES;
    ad_ret = ad_open(&ad, path,
                     ADFLAGS_DF | ADFLAGS_RDONLY | ADFLAGS_SETSHRMD | ADFLAGS_TRUNC,
                     0666);
    calls = fi.open_calls;
    retry_flags = fi.open_last_flags;
    fault_inject_reset();

    if (ad_ret == 0) {
        ad_close(&ad, ADFLAGS_DF);
    }

    (void)unlink(path);

    if (calls != 2) {
        return 2;                    /* negative control: retry did not run */
    }

    if (retry_flags & O_TRUNC) {
        return 3;                    /* destructive flag survived the RO retry */
    }

    if (retry_flags & (O_RDWR | O_WRONLY)) {
        return 4;                    /* retry not downgraded to read-only */
    }

    if (ad_ret != 0) {
        return 5;                    /* read-only retry open failed */
    }

    return 0;
}

/*!
 * @brief ad2openflags(): RDONLY+SETSHRMD on the data fork is promoted to O_RDWR
 *        (share-mode locks need a writable fd); plain RDONLY stays O_RDONLY.
 *
 * Category: targeted.  This promotion is load-bearing and easy to break silently
 * (O_RDONLY is 0, so a regression only surfaces as subtle lock behaviour or
 * strict-backend EINVAL).  The access mode has no fd/refcount ledger; observe it
 * via the shim, which records each armed open()'s flags in fi.open_last_flags.
 * Arm with open_fail_after = -1 to record without failing.  Skips where the shim
 * cannot intercept libatalk's open().
 */
int utest_ad2openflags_accmode(const struct vol *vol)
{
    struct adouble ad;
    char path[MAXPATHLEN + 1];
    int ad_ret, shrmd_flags, ro_flags;
    snprintf(path, sizeof(path), "%s/fi_ad2openflags", vol->v_path);

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    fault_inject_reset();
    ad_init(&ad, vol);

    if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR | ADFLAGS_CREATE, 0666) != 0) {
        (void)unlink(path);
        return 1;                    /* setup create failed */
    }

    ad_close(&ad, ADFLAGS_DF);
    /* (1) RDONLY + SETSHRMD must be promoted to O_RDWR */
    ad_init(&ad, vol);
    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_after = -1;         /* record flags, never fail */
    ad_ret = ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDONLY | ADFLAGS_SETSHRMD);
    shrmd_flags = fi.open_last_flags;
    fault_inject_reset();

    if (ad_ret == 0) {
        ad_close(&ad, ADFLAGS_DF);
    }

    /* (2) plain RDONLY (no SETSHRMD) must stay O_RDONLY */
    ad_init(&ad, vol);
    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_after = -1;
    ad_ret = ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDONLY);
    ro_flags = fi.open_last_flags;
    fault_inject_reset();

    if (ad_ret == 0) {
        ad_close(&ad, ADFLAGS_DF);
    }

    (void)unlink(path);

    if ((shrmd_flags & O_ACCMODE) != O_RDWR) {
        return 2;                    /* SETSHRMD|RDONLY not promoted to O_RDWR */
    }

    if ((ro_flags & O_ACCMODE) != O_RDONLY) {
        return 3;                    /* plain RDONLY wrongly opened writable */
    }

    return 0;
}

/* ====================================================================== *
 * Delete-conflict GET tests (ad_testlock_range / ad_testlock_whole /
 * of_get_locks / deletefile).
 *
 * F_GETLK never reports the calling process's own locks, so a conflict a
 * probe must see has to be held by ANOTHER process.  peer_hold_lock() forks a
 * child that takes a real fcntl lock on the backing file and parks until told
 * to exit, giving the parent a genuine cross-process holder to probe against.
 * ====================================================================== */

/* A forked peer holding a byte-range lock on a path until released. */
struct peer {
    pid_t pid;
    int   to_child;      /* parent writes 1 byte here to tell the peer to exit */
    int   from_child;    /* peer writes 1 byte here once the lock is held */
};

/* Fork a child that opens `path` and takes an fcntl lock of `type`
 * (F_RDLCK/F_WRLCK) over [start, start+len).  Returns 0 and fills *p on success
 * (the lock is held by the time this returns), -1 on failure. */
static int peer_hold_lock(struct peer *p, const char *path, short type,
                          off_t start, off_t len)
{
    int up[2], down[2];
    char b;

    if (pipe(up) < 0) {
        return -1;
    }

    if (pipe(down) < 0) {
        close(up[0]);
        close(up[1]);
        return -1;
    }

    p->pid = fork();

    if (p->pid < 0) {
        close(up[0]);
        close(up[1]);
        close(down[0]);
        close(down[1]);
        return -1;
    }

    if (p->pid == 0) {
        /* child: take the lock, signal "held", wait for the release byte */
        struct flock fl = {0};
        int fd = open(path, O_RDWR);

        if (fd < 0) {
            _exit(1);
        }

        fl.l_type = type;
        fl.l_whence = SEEK_SET;
        fl.l_start = start;
        fl.l_len = len;

        if (fcntl(fd, F_SETLK, &fl) < 0) {
            _exit(2);
        }

        b = 1;

        if (write(up[1], &b, 1) != 1) {
            _exit(3);
        }

        (void)read(down[0], &b, 1);   /* park until released */
        _exit(0);
    }

    /* parent */
    close(up[1]);
    close(down[0]);
    p->to_child = down[1];
    p->from_child = up[0];

    /* block until the peer reports the lock is held */
    if (read(p->from_child, &b, 1) != 1) {
        close(p->to_child);
        close(p->from_child);
        return -1;
    }

    return 0;
}

/* Release the peer (let it exit) and reap it. */
static void peer_release(struct peer *p)
{
    char b = 1;
    int status;
    (void)write(p->to_child, &b, 1);
    close(p->to_child);
    close(p->from_child);
    (void)waitpid(p->pid, &status, 0);
}

/* In a forked child, try to take an fcntl lock of `type` over [start,start+len)
 * on `path`.  Returns 1 if the lock was acquired (range free), 0 if it conflicted
 * (range held by another process), -1 on a fork/open error.  Cross-process, so it
 * sees a lock this process holds (which our own F_GETLK never would).  The result
 * is reported over a pipe rather than the child's exit status. */
static int peer_try_lock(const char *path, short type, off_t start, off_t len)
{
    int pfd[2];
    pid_t pid;
    int status;
    char res;

    if (pipe(pfd) < 0) {
        return -1;
    }

    pid = fork();

    if (pid < 0) {
        close(pfd[0]);
        close(pfd[1]);
        return -1;
    }

    if (pid == 0) {
        struct flock fl = {0};
        int fd = open(path, O_RDWR);
        char r;

        if (fd < 0) {
            r = 2;                   /* could not open */
        } else {
            fl.l_type = type;
            fl.l_whence = SEEK_SET;
            fl.l_start = start;
            fl.l_len = len;
            /* 1 = acquired (range free), 0 = conflict (range held) */
            r = (fcntl(fd, F_SETLK, &fl) == 0) ? 1 : 0;
        }

        (void)write(pfd[1], &r, 1);
        _exit(0);
    }

    close(pfd[1]);

    if (read(pfd[0], &res, 1) != 1) {
        res = 2;                     /* child died without reporting */
    }

    close(pfd[0]);
    (void)waitpid(pid, &status, 0);

    if (res == 2) {
        return -1;                   /* child could not open the file */
    }

    return res == 1 ? 1 : 0;
}

/* Seed a struct path for the conflict helpers: u_name only, st_valid 0 so
 * of_findname()/of_get_locks() stat it fresh. */
static void seed_path(struct path *path, char *u_name)
{
    memset(path, 0, sizeof(*path));
    path->u_name = u_name;
}

/* Create a scratch file of `size` bytes under the volume, returning its full
 * path in out[]/outlen.  Returns 0 on success. */
static int make_scratch(const struct vol *vol, const char *leaf,
                        off_t size, char *out, size_t outlen)
{
    struct adouble ad;
    int n = snprintf(out, outlen, "%s/%s", vol->v_path, leaf);

    if (n < 0 || (size_t)n >= outlen) {
        return -1;
    }

    ad_init(&ad, vol);

    if (ad_open(&ad, out, ADFLAGS_DF | ADFLAGS_RDWR | ADFLAGS_CREATE, 0666) != 0) {
        return -1;
    }

    if (size > 0) {
        (void)ftruncate(ad_data_fileno(&ad), size);
    }

    ad_close(&ad, ADFLAGS_DF);
    return 0;
}

/*!
 * @brief ad_testlock_range() clamps a data-zone probe; never sweeps the band.
 *
 * Category: targeted.  A whole-data-zone request (len == 0) must be bounded to
 * the data zone (issued l_len != 0, l_start + l_len <= BYTELOCK_MAX), not turned
 * into a POSIX l_len == 0 ("to infinity") that would reach the share-mode band.
 * An explicit band probe (off >= AD_FILELOCK_BASE) must pass through unclamped.
 * Observed via the shim's lock-fcntl recorder.  Skips where the shim cannot
 * intercept libatalk's fcntl().
 */
int utest_testlock_range_clamp(const struct vol *vol)
{
    struct adouble ad;
    char path[MAXPATHLEN + 1];
    off_t whole_len;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (make_scratch(vol, "fi_clamp", 200, path, sizeof(path)) != 0) {
        return 1;
    }

    ad_init(&ad, vol);

    if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR, 0666) != 0) {
        (void)unlink(path);
        return 2;
    }

    /* whole data zone (len 0): must be clamped, one F_GETLK, l_len finite */
    fault_inject_reset();
    fi.fcntl_watch = 1;
    (void)ad_testlock_range(&ad, ADEID_DFORK, 0, 0);
    whole_len = fi.lock_last_len;

    if (fi.getlk_calls != 1) {
        fault_inject_reset();
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 3;                    /* expected exactly one F_GETLK */
    }

    if (fi.lock_last_type != F_WRLCK) {
        fault_inject_reset();
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 4;                    /* must probe with explicit F_WRLCK */
    }

    if (whole_len == 0) {
        fault_inject_reset();
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 5;                    /* len 0 == "to infinity": sweeps the band */
    }

    if (fi.lock_last_start + whole_len > BYTELOCK_MAX) {
        fault_inject_reset();
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 6;                    /* clamp must stay within the data zone */
    }

    /* explicit band probe: passes through unclamped (l_start at the sentinel) */
    fi.getlk_calls = 0;
    (void)ad_testlock_range(&ad, ADEID_DFORK, AD_FILELOCK_OPEN_WR, 1);

    if (fi.getlk_calls != 1 || fi.lock_last_start != AD_FILELOCK_OPEN_WR
            || fi.lock_last_len != 1) {
        fault_inject_reset();
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 7;                    /* band probe must read the sentinel offset */
    }

    fault_inject_reset();
    ad_close(&ad, ADFLAGS_DF);
    (void)unlink(path);
    return 0;
}

/*!
 * @brief ad_testlock_whole() issues one unclamped F_GETLK over the whole fd.
 *
 * Category: targeted.  The fast-path probe must be deliberately unclamped
 * (l_start == 0, l_len == 0) so it spans the data zone and the share-mode band
 * in a single call, and report a peer's band lock as a conflict.  Uses a forked
 * peer holding a band-offset lock; skips without shim fcntl interception.
 */
int utest_testlock_whole(const struct vol *vol)
{
    struct adouble ad;
    char path[MAXPATHLEN + 1];
    struct peer peer;
    int held;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (make_scratch(vol, "fi_whole", 100, path, sizeof(path)) != 0) {
        return 1;
    }

    ad_init(&ad, vol);

    if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR, 0666) != 0) {
        (void)unlink(path);
        return 2;
    }

    /* clear fd: one unclamped F_GETLK, reports 0 */
    fault_inject_reset();
    fi.fcntl_watch = 1;
    held = ad_testlock_whole(&ad, ADEID_DFORK);

    if (fi.getlk_calls != 1 || fi.lock_last_start != 0 || fi.lock_last_len != 0) {
        fault_inject_reset();
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 3;                    /* must be exactly one l_start=0,l_len=0 GETLK */
    }

    if (held != 0) {
        fault_inject_reset();
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 4;                    /* clear file must report 0 */
    }

    fault_inject_reset();

    /* a peer holding a band-offset lock must be seen by the whole-fd probe */
    if (peer_hold_lock(&peer, path, F_WRLCK, AD_FILELOCK_OPEN_WR, 1) != 0) {
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 5;
    }

    held = ad_testlock_whole(&ad, ADEID_DFORK);
    peer_release(&peer);
    ad_close(&ad, ADFLAGS_DF);
    (void)unlink(path);

    if (held != 1) {
        return 6;                    /* unclamped probe must span the band */
    }

    return 0;
}

/*!
 * @brief ad_testlock_range() does not report this process's own band entry.
 *
 * Category: targeted (the crux of using a sibling primitive).  Plant an
 * OPEN_RD band entry owned by this process via ad_lock(), then ad_testlock_range
 * on the same offset must return 0 (kernel F_GETLK never reports our own locks),
 * while the array-scanning ad_testlock() returns 1 on the same handle.  No shim
 * needed.
 */
int utest_testlock_range_no_self_report(const struct vol *vol)
{
    struct adouble ad;
    char path[MAXPATHLEN + 1];
    int range_ret, scan_ret;

    if (make_scratch(vol, "fi_self", 100, path, sizeof(path)) != 0) {
        return 1;
    }

    ad_init(&ad, vol);

    if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR | ADFLAGS_SETSHRMD,
                0666) != 0) {
        (void)unlink(path);
        return 2;
    }

    /* plant an OPEN_RD band entry owned by this handle */
    if (ad_lock(&ad, ADEID_DFORK, ADLOCK_RD | ADLOCK_FILELOCK,
                AD_FILELOCK_OPEN_RD, 1, 0) < 0) {
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 3;
    }

    range_ret = ad_testlock_range(&ad, ADEID_DFORK, AD_FILELOCK_OPEN_RD, 1);
    scan_ret  = ad_testlock(&ad, ADEID_DFORK, AD_FILELOCK_OPEN_RD);
    ad_unlock(&ad, 0, 1);
    ad_close(&ad, ADFLAGS_DF);
    (void)unlink(path);

    if (range_ret != 0) {
        return 4;                    /* F_GETLK wrongly reported our own lock */
    }

    if (scan_ret != 1) {
        return 5;                    /* control: array scan should self-match */
    }

    return 0;
}

/*!
 * @brief ad_testlock_range()'s F_WRLCK probe sees a peer's F_RDLCK band entry.
 *
 * Category: targeted (the read-lock visibility the old detector missed).  The
 * legacy delete probe opened the fork read-only and so issued an F_RDLCK test,
 * which does not conflict with another holder's read lock — a peer read-lock (and
 * the F_RDLCK share-mode band entries) were invisible.  ad_testlock_range() always
 * probes with an explicit F_WRLCK, which conflicts with a peer's read OR write
 * lock, so it must report a peer's F_RDLCK as a conflict.  A forked peer holds an
 * F_RDLCK at a band offset; assert the range probe returns 1.  Control: the same
 * offset with no peer returns 0.  Cross-process (our own F_GETLK never reports our
 * own locks), no shim needed.
 */
int utest_testlock_range_wrlck_sees_rdlck(const struct vol *vol)
{
    struct adouble ad;
    char path[MAXPATHLEN + 1];
    struct peer peer;
    int seen, clear;

    if (make_scratch(vol, "fi_rdlck", 100, path, sizeof(path)) != 0) {
        return 1;
    }

    ad_init(&ad, vol);

    if (ad_open(&ad, path, ADFLAGS_DF | ADFLAGS_RDWR, 0666) != 0) {
        (void)unlink(path);
        return 2;
    }

    /* control: no peer -> the band offset is free */
    clear = ad_testlock_range(&ad, ADEID_DFORK, AD_FILELOCK_OPEN_RD, 1);

    if (clear != 0) {
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 3;                    /* offset must be free before the peer locks */
    }

    /* a peer holds a READ lock at the band offset */
    if (peer_hold_lock(&peer, path, F_RDLCK, AD_FILELOCK_OPEN_RD, 1) != 0) {
        ad_close(&ad, ADFLAGS_DF);
        (void)unlink(path);
        return 4;
    }

    seen = ad_testlock_range(&ad, ADEID_DFORK, AD_FILELOCK_OPEN_RD, 1);
    peer_release(&peer);
    ad_close(&ad, ADFLAGS_DF);
    (void)unlink(path);

    if (seen != 1) {
        return 5;                    /* F_WRLCK probe must see the peer's F_RDLCK */
    }

    return 0;
}

/*!
 * @brief of_get_locks(): positional band bitmap, independent df/rf, tri-valued.
 *
 * Category: targeted.  With a peer holding a DENY_WR band lock, request a subset
 * of band bits plus the data range and assert: the held bit comes back in its own
 * position, an unrequested neighbour bit stays 0, df/rf flags are independent, and
 * the status is OF_LOCKS_OK (separate from the data).  Cross-process peer.
 */
int utest_of_get_locks_contract(const struct vol *vol)
{
    char path[MAXPATHLEN + 1];
    struct path dpath;
    struct peer peer;
    int rc, dfl = 0, rfl = 0;
    uint16_t band = 0;

    if (make_scratch(vol, "fi_contract", 100, path, sizeof(path)) != 0) {
        return 1;
    }

    if (peer_hold_lock(&peer, path, F_WRLCK, AD_FILELOCK_DENY_WR, 1) != 0) {
        (void)unlink(path);
        return 2;
    }

    seed_path(&dpath, path);
    /* request DENY_WR (held) and DENY_RD (free); no data/rf content */
    rc = of_get_locks(vol, -1, &dpath, 0, 0, 0, 0, 0, 0,
                      DENY_WR_BIT | DENY_RD_BIT, 1, &dfl, &rfl, &band);
    peer_release(&peer);
    (void)unlink(path);

    if (rc != OF_LOCKS_OK) {
        return 3;                    /* readable state must be OF_LOCKS_OK */
    }

    if (!(band & DENY_WR_BIT)) {
        return 4;                    /* the held bit must be reported... */
    }

    if (band & DENY_RD_BIT) {
        return 5;                    /* ...and an unrequested neighbour must not */
    }

    if (dfl != 0 || rfl != 0) {
        return 6;                    /* no content lock held: df/rf must be 0 */
    }

    return 0;
}

/*!
 * @brief of_get_locks(): >=2-dimension whole-fd fast path, per-bit on a hit.
 *
 * Category: targeted.  With >= 2 data-fd dimensions requested on a clear file,
 * exactly one whole-fd F_GETLK is issued and the per-bit band loop is skipped.
 * With a peer band lock present, the whole-fd probe hits and the per-bit loop then
 * runs and attributes the exact bit.  Counts F_GETLK via the shim.
 */
int utest_of_get_locks_fastpath(const struct vol *vol)
{
    char path[MAXPATHLEN + 1];
    struct path dpath;
    struct peer peer;
    int rc, dfl = 0, rfl = 0;
    uint16_t band = 0;

    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (make_scratch(vol, "fi_fast", 100, path, sizeof(path)) != 0) {
        return 1;
    }

    seed_path(&dpath, path);
    /* clear file, data range + all band bits (many dimensions): one whole-fd
     * GETLK, band loop skipped */
    fault_inject_reset();
    fi.fcntl_watch = 1;
    rc = of_get_locks(vol, -1, &dpath, 1, 0, 0, 0, 0, 0,
                      ALL_BAND_BITS, 1, &dfl, &rfl, &band);

    if (rc != OF_LOCKS_OK || band != 0 || dfl != 0) {
        fault_inject_reset();
        (void)unlink(path);
        return 2;
    }

    if (fi.getlk_calls != 1) {
        fault_inject_reset();
        (void)unlink(path);
        return 3;                    /* clear file must resolve in one whole-fd GETLK */
    }

    fault_inject_reset();

    /* peer holds DENY_WR: whole-fd probe hits, per-bit loop attributes the bit */
    if (peer_hold_lock(&peer, path, F_WRLCK, AD_FILELOCK_DENY_WR, 1) != 0) {
        (void)unlink(path);
        return 4;
    }

    band = 0;
    rc = of_get_locks(vol, -1, &dpath, 1, 0, 0, 0, 0, 0,
                      ALL_BAND_BITS, 1, &dfl, &rfl, &band);
    peer_release(&peer);
    (void)unlink(path);

    if (rc != OF_LOCKS_OK) {
        return 5;
    }

    if (!(band & DENY_WR_BIT)) {
        return 6;                    /* on a hit, the exact bit must be attributed */
    }

    return 0;
}

/*!
 * @brief of_get_locks(): indeterminate -> OF_LOCKS_ERROR; missing -> NOENT.
 *
 * Category: targeted (fail-closed contract).  A missing target returns
 * OF_LOCKS_NOENT (definite: no locks), distinct from an unreadable one.  A forced
 * F_GETLK hard error (shim fcntl -> EIO) must return OF_LOCKS_ERROR, never a
 * silent "no conflict".  Skips without shim fcntl interception for the error leg.
 */
int utest_of_get_locks_failclosed(const struct vol *vol)
{
    char path[MAXPATHLEN + 1];
    struct path dpath;
    int rc, dfl = 0, rfl = 0;
    uint16_t band = 0;

    /* NOENT: a name that does not exist */
    if (snprintf(path, sizeof(path), "%s/fi_absent",
                 vol->v_path) >= (int)sizeof(path)) {
        return 1;
    }

    (void)unlink(path);
    seed_path(&dpath, path);
    rc = of_get_locks(vol, -1, &dpath, 1, 0, 0, 0, 0, 0,
                      ALL_BAND_BITS, 1, &dfl, &rfl, &band);

    if (rc != OF_LOCKS_NOENT) {
        return 2;                    /* missing target must be NOENT, not ERROR/OK */
    }

    /* NOENT also for an rfork-only request (no data/band probe): the missing
     * target must not slip through as OF_LOCKS_OK. */
    seed_path(&dpath, path);
    rc = of_get_locks(vol, -1, &dpath, 0, 0, 0, 1, 0, 0,
                      0, 1, &dfl, &rfl, &band);

    if (rc != OF_LOCKS_NOENT) {
        return 5;                    /* rfork-only missing target must be NOENT */
    }

    /* ERROR: force a hard F_GETLK failure on a real file */
    if (!test_capability(TEST_CAP_FAULT_INJECT, vol)) {
        return TEST_SKIP;
    }

    if (make_scratch(vol, "fi_failclosed", 100, path, sizeof(path)) != 0) {
        return 3;
    }

    seed_path(&dpath, path);
    fault_inject_reset();
    fi.fcntl_armed = 1;
    fi.fcntl_fail_after = 0;          /* fail the next lock fcntl once */
    fi.fcntl_errno = EIO;             /* a hard error, not EACCES/EAGAIN */
    rc = of_get_locks(vol, -1, &dpath, 1, 0, 0, 0, 0, 0,
                      ALL_BAND_BITS, 1, &dfl, &rfl, &band);
    fault_inject_reset();
    (void)unlink(path);

    if (rc != OF_LOCKS_ERROR) {
        return 4;                    /* hard GETLK error must fail closed */
    }

    /* ERROR: a hard rfork-open failure (EIO, not the benign "no rfork" ENOENT)
     * must fail closed - ADFLAGS_NORF suppresses only a missing rfork. */
    if (make_scratch(vol, "fi_rf_failclosed", 100, path, sizeof(path)) != 0) {
        return 6;
    }

    seed_path(&dpath, path);
    fault_inject_reset();
    fi.open_armed = 1;
    fi.open_fail_after = 0;            /* fail the next open() once */
    fi.open_errno = EIO;             /* hard error, not ENOENT/EACCES */
    rc = of_get_locks(vol, -1, &dpath, 0, 0, 0, 1, 0, 0,
                      0, 1, &dfl, &rfl, &band);
    fault_inject_reset();
    (void)unlink(path);

    if (rc != OF_LOCKS_ERROR) {
        return 7;                    /* hard rfork-open error must fail closed */
    }

    /* Benign control (no fault injection): an existing file with NO resource fork
     * must read OK, not error - ADFLAGS_NORF must keep treating a *missing* rfork as
     * "nothing locked".  This is the no-regression guard for the NORF change. */
    if (make_scratch(vol, "fi_rf_absent", 100, path, sizeof(path)) != 0) {
        return 8;
    }

    seed_path(&dpath, path);
    rc = of_get_locks(vol, -1, &dpath, 0, 0, 0, 1, 0, 0,
                      0, 1, &dfl, &rfl, &band);
    (void)unlink(path);

    if (rc != OF_LOCKS_OK || rfl != 0) {
        return 9;                    /* absent rfork must be OK + unlocked */
    }

    return 0;
}

/*!
 * @brief deletefile()'s conflict GET through a held fd preserves the held lock.
 *
 * Category: targeted (the same-process lock-on-close repro).  Register a tracked
 * ofork holding a byte-range lock on an inode, then drive of_get_locks() the way
 * deletefile() does (reusing the held fd, opening nothing).  Assert the conflict
 * read succeeds and a post-call F_GETLK in this process still shows the held lock
 * (a transient open+close would have dropped it).  No peer / no shim needed.
 */
int utest_deletefile_quirk(struct vol *vol)
{
    char leaf[64];
    char full[MAXPATHLEN + 1];
    struct path dpath;
    struct ofork *of;
    struct stat st;
    uint16_t ofrefnum = 0;
    int rc, dfl = 0, rfl = 0;
    uint16_t band = 0;
    int held_before, held_after;
    snprintf(leaf, sizeof(leaf), "fi_quirk");

    if (make_scratch(vol, leaf, 200, full, sizeof(full)) != 0) {
        return 1;
    }

    if (stat(full, &st) != 0) {
        (void)unlink(full);
        return 2;
    }

    /* register a tracked ofork on the inode, like afp_openfork() */
    of = of_alloc(vol, curdir, leaf, &ofrefnum, ADEID_DFORK,
                  NULL, &st);

    if (of == NULL) {
        (void)unlink(full);
        return 3;
    }

    if (ad_open(of->of_ad, full, ADFLAGS_DF | ADFLAGS_RDWR | ADFLAGS_SETSHRMD,
                0666) != 0) {
        of_dealloc(of);
        (void)unlink(full);
        return 4;
    }

    /* the held fork takes a content byte-range lock */
    if (ad_lock(of->of_ad, ADEID_DFORK, ADLOCK_WR, 0, 100, ofrefnum) < 0) {
        ad_close(of->of_ad, ADFLAGS_DF);
        of_dealloc(of);
        (void)unlink(full);
        return 5;
    }

    /* a peer must see the held lock as a conflict BEFORE the GET (control) */
    held_before = peer_try_lock(full, F_WRLCK, 0, 100);

    if (held_before != 0) {
        ad_unlock(of->of_ad, ofrefnum, 1);
        ad_close(of->of_ad, ADFLAGS_DF);
        of_dealloc(of);
        (void)unlink(full);
        return 6;                    /* lock not actually held -> test invalid */
    }

    /* drive the delete-time conflict GET; it must reuse of->of_ad's fd and so
     * open+close no second fd to the inode (which would drop the held lock) */
    seed_path(&dpath, full);
    rc = of_get_locks(vol, -1, &dpath, 1, 0, 0, 1, 0, 0,
                      ALL_BAND_BITS, 1, &dfl, &rfl, &band);
    /* a peer must STILL see the lock after the GET: if the GET had opened and
     * closed a transient fd to the inode, our process's lock would be gone */
    held_after = peer_try_lock(full, F_WRLCK, 0, 100);
    ad_unlock(of->of_ad, ofrefnum, 1);
    ad_close(of->of_ad, ADFLAGS_DF);
    of_dealloc(of);
    (void)unlink(full);

    if (rc != OF_LOCKS_OK) {
        return 7;                    /* conflict read itself failed */
    }

    if (held_after != 0) {
        return 8;                    /* held lock was dropped by the conflict GET */
    }

    return 0;
}

/*!
 * @brief Negative control: demonstrate the POSIX close-drops-locks hazard is real.
 *
 * Category: targeted (the hazard utest_deletefile_quirk's cure defends against).
 * POSIX advisory locks are owned by the process, so closing ANY fd to an inode
 * drops every lock the process holds on it.  This test reproduces that hazard
 * directly: take a lock through one fd, then open and close a SECOND, transient fd
 * to the same inode, and observe (via a peer) that the lock is now gone — exactly
 * what the legacy delete-time transient open+close did to a held fork's lock.
 *
 * Without this control, utest_deletefile_quirk's "the lock survived" could pass
 * vacuously on a platform that never drops the lock in the first place.  Here, if
 * the transient close does NOT drop the lock, this platform does not exhibit the
 * quirk, so there is nothing to defend against: return TEST_SKIP (the cure is
 * trivially safe here and its positive result is not meaningful).  A reproduced
 * hazard returns 0 — the danger is real, and the cure test then proves 9B avoids
 * it.  Cross-process peer (our own F_GETLK never reports our own locks).
 */
int utest_deletefile_quirk_hazard(const struct vol *vol)
{
    char path[MAXPATHLEN + 1];
    struct flock fl = {0};
    int held_fd, transient_fd;
    int held_before, held_after;

    if (make_scratch(vol, "fi_quirk_hazard", 200, path, sizeof(path)) != 0) {
        return 1;
    }

    /* fd 1: take a write lock on [0,100) and keep it open */
    held_fd = open(path, O_RDWR);

    if (held_fd < 0) {
        (void)unlink(path);
        return 2;
    }

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 100;

    if (fcntl(held_fd, F_SETLK, &fl) < 0) {
        close(held_fd);
        (void)unlink(path);
        return 3;
    }

    /* a peer must see the lock as held (control on the control) */
    held_before = peer_try_lock(path, F_WRLCK, 0, 100);

    if (held_before != 0) {
        close(held_fd);
        (void)unlink(path);
        return 4;                    /* lock not actually held -> test invalid */
    }

    /* the hazard: open a SECOND fd to the same inode and close it.  Under POSIX
     * record-lock semantics this drops the lock taken on held_fd. */
    transient_fd = open(path, O_RDONLY);

    if (transient_fd < 0) {
        close(held_fd);
        (void)unlink(path);
        return 5;
    }

    close(transient_fd);
    /* did the transient close drop our lock?  Ask the peer again. */
    held_after = peer_try_lock(path, F_WRLCK, 0, 100);
    close(held_fd);
    (void)unlink(path);

    if (held_after == 0) {
        /* lock still held: this platform does NOT exhibit the quirk (e.g. OFD or
         * BSD per-fd semantics), so there is no hazard to defend against here. */
        return TEST_SKIP;
    }

    if (held_after != 1) {
        return 6;                    /* peer_try_lock error */
    }

    /* lock dropped by the transient close: the hazard is real on this platform */
    return 0;
}

/*!
 * @brief deletefile() honours the NODELETE attribute (per backend).
 *
 * Category: targeted (FU-D, the NODELETE read path).  Create a file, set
 * ATTRBIT_NODELETE, and assert deletefile(checkAttrib=1) returns AFPERR_OLOCK and
 * leaves the file; clear it and assert the delete succeeds.  Driven with the
 * volume's own backend (called once per backend from test.c).
 */
int utest_deletefile_nodelete(const struct vol *vol)
{
    struct adouble ad;
    char full[MAXPATHLEN + 1];
    char leaf[64];
    int rc;
    int dirfd;
    struct stat st;
    snprintf(leaf, sizeof(leaf), "fi_nodelete");

    if (make_scratch(vol, leaf, 0, full, sizeof(full)) != 0) {
        return 1;
    }

    /* deletefile() resolves `leaf` against `dirfd` (else the global curdir, which
     * points at whichever volume was last chdir'd into).  Pass the volume root fd
     * so the test is correct for any volume, not just the cwd one. */
    dirfd = open(vol->v_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);

    if (dirfd < 0) {
        (void)unlink(full);
        return 10;
    }

    /* set NODELETE via the metadata header */
    ad_init(&ad, vol);

    if (ad_open(&ad, full, ADFLAGS_HF | ADFLAGS_RDWR | ADFLAGS_CREATE, 0666) != 0) {
        (void)unlink(full);
        return 2;
    }

    if (ad_setattr(&ad, htons(ATTRBIT_NODELETE)) != 0) {
        ad_close(&ad, ADFLAGS_HF);
        (void)unlink(full);
        return 3;
    }

    ad_flush(&ad);
    ad_close(&ad, ADFLAGS_HF);
    /* delete must be refused while NODELETE is set */
    rc = deletefile(vol, dirfd, leaf, 1);

    if (rc != AFPERR_OLOCK) {
        close(dirfd);
        (void)unlink(full);
        return 4;                    /* NODELETE not honoured */
    }

    if (stat(full, &st) != 0) {
        close(dirfd);
        return 5;                    /* file wrongly removed despite OLOCK */
    }

    /* clear NODELETE, delete must now succeed */
    ad_init(&ad, vol);

    if (ad_open(&ad, full, ADFLAGS_HF | ADFLAGS_RDWR, 0666) != 0) {
        close(dirfd);
        (void)unlink(full);
        return 6;
    }

    if (ad_setattr(&ad, 0) != 0) {
        ad_close(&ad, ADFLAGS_HF);
        close(dirfd);
        (void)unlink(full);
        return 7;
    }

    ad_flush(&ad);
    ad_close(&ad, ADFLAGS_HF);
    rc = deletefile(vol, dirfd, leaf, 1);
    close(dirfd);

    if (rc != AFP_OK) {
        (void)unlink(full);
        return 8;                    /* delete refused after NODELETE cleared */
    }

    if (stat(full, &st) == 0) {
        (void)unlink(full);
        return 9;                    /* delete returned OK but file remains */
    }

    return 0;
}
