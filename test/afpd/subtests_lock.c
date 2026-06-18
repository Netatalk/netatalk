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

#include <atalk/adouble.h>
#include <atalk/logger.h>
#include <atalk/volume.h>

#include "faultinject.h"
#include "subtests_lock.h"
#include "test.h"      /* TEST_SKIP */

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
    if (!faultinject_open_works(path)) {
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
 * (ad_data_fileno == -1) AND the kernel (fcntl(fd) -> EBADF).  A negative control
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

    /* Negative control: before cleanup the fd MUST be open, else the leak
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
 *     must NOT close the shared fd while a sibling reference remains).
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

    /* Release reference 2: refcount 2->1, fd MUST stay open (sibling holds it). */
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
