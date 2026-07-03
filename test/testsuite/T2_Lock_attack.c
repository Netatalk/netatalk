/* ----------------------------------------------
 * Tier-2 lock "attack" tests.
 *
 * Observe the kernel's lock state directly (raw fcntl on the backing file via
 * -c Path), independently of afpd's own bookkeeping.  Auto-skip without a local
 * path (-c) or a second connection.
 */
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

static char temp[MAXPATHLEN];

/* Local FS path of the data fork's backing file (data fork == the file itself
 * on both AD_V2 and AD_EA backends), for a file `name` inside test subdir `sub`.
 * Returns 0 on success, -1 if the path would not fit in `outlen`. */
static int data_fork_path(char *out, size_t outlen, char *sub, char *name)
{
    int n = snprintf(out, outlen, "%s/%s/%s", Path, sub, name);

    if (n < 0 || (size_t)n >= outlen) {
        if (!Quiet) {
            fprintf(stdout, "\tdata_fork_path: path too long for %s/%s/%s\n",
                    Path, sub, name);
        }

        return -1;
    }

    return 0;
}

/* F_GETLK probe from a throwaway fd: 1 = kernel reports a conflicting lock on
 * [off,len); 0 = range free; -1 = open/fcntl error.
 *
 * Note on self-interference: this helper opens and closes its own fd.  That is
 * safe here precisely because it runs in the spectest client process, which holds
 * no afpd locks - the close drops nothing of ours.  (This is the opposite of the
 * server-side bug, where afpd closing a probe fd dropped its own locks.) */
static int kernel_lock_held(char *path, off_t off, off_t len)
{
    struct flock fl;
    int fd;
    int held;
    fd = open(path, O_RDWR, 0);

    if (fd < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tkernel_lock_held: open(%s): %s\n", path, strerror(errno));
        }

        return -1;
    }

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = off;
    fl.l_len    = len;

    if (fcntl(fd, F_GETLK, &fl) < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tkernel_lock_held: F_GETLK: %s\n", strerror(errno));
        }

        close(fd);
        return -1;
    }

    held = (fl.l_type != F_UNLCK);
    close(fd);
    return held;
}

/* ------------------------------------------------------------------ *
 * test600  Cross-session delete-probe does not corrupt a holder's lock.
 *          Kernel-observed.
 *
 * Conn locks [0,100] on the data fork; F_GETLK confirms the kernel records it.
 * Conn2 attempts FPDelete (in Conn2's child this fires the ADFLAGS_CHECK_OF
 * probe open+close on the inode).  Assert the delete returns AFPERR_BUSY and
 * Conn's lock is still held afterwards.
 *
 * Note: Conn and Conn2 are separate afpd children, so Conn2's probe close cannot
 * drop Conn's locks (the close-drops-own-locks effect is same-process only) - this
 * test passes on both the buggy and fixed server.  Its value is that no other test
 * asserts "held byte lock -> cross-session FPDelete is BUSY and the deleter's probe
 * path leaves the holder's lock intact."
 * ------------------------------------------------------------------ */
STATIC void test600()
{
    char *dname = "t600";
    char *name = "f";
    uint16_t vol = VolID;
    uint16_t vol2;
    uint16_t fork;
    int dir;
    int held;
    unsigned int dret;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    dir = FPCreateDir(Conn, vol, DIRDID_ROOT, dname);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_nottested();
        goto cleanup;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, dir, name,
                      OPENACC_RD | OPENACC_WR);

    if (!fork) {
        test_nottested();
        goto cleanup;
    }

    FAIL(FPSetForkParam(Conn, fork, (1 << FILPBIT_DFLEN), 200))
    FAIL(FPByteLock(Conn, fork, 0, 0 /* set */, 0, 100))

    if (data_fork_path(temp, sizeof(temp), dname, name) < 0) {
        test_nottested();
        FPCloseFork(Conn, fork);
        goto cleanup;
    }

    held = kernel_lock_held(temp, 0, 100);

    if (held != 1) {
        if (!Quiet) {
            fprintf(stdout, "\tkernel did not record the byte lock (held=%d)\n", held);
        }

        test_nottested();
        FPCloseFork(Conn, fork);
        goto cleanup;
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        FPCloseFork(Conn, fork);
        goto cleanup;
    }

    dret = FPDelete(Conn2, vol2, dir, name);
    FAIL(ntohl(AFPERR_BUSY) != dret)
    /* the crux: the cross-session delete's CHECK_OF probe must not corrupt
     * Conn's lock - it should still be held in the kernel. */
    held = kernel_lock_held(temp, 0, 100);

    if (held != 1) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED: byte lock dropped by the cross-session delete "
                    "probe - held=%d; deleter's CHECK_OF must not touch a "
                    "holder's lock\n", held);
        }

        test_failed();
    }

    FAIL(FPCloseVol(Conn2, vol2))
    FAIL(FPCloseFork(Conn, fork))
cleanup:
    delete_directory_tree(Conn, vol, DIRDID_ROOT, dname);
test_exit:
    exit_test("T2LockAttack:test600: cross-session delete probe leaves holder lock intact");
}

/* ------------------------------------------------------------------ *
 * test603  Shared share-mode read-lock preservation (refcount > 1),
 *          kernel-observed.
 *
 * Two deny-none read opens of one fork in one session share the AD_FILELOCK_OPEN_RD
 * share-mode lock with a refcount of 2. Closing the first open must not drop the
 * kernel lock (refcount 2->1); closing the second must (refcount ->0).
 * ------------------------------------------------------------------ */
STATIC void test603()
{
    char *dname = "t603";
    char *name = "f";
    uint16_t vol = VolID;
    uint16_t fork = 0, fork1 = 0;
    int dir;
    int held;
    off_t off = AD_FILELOCK_OPEN_RD;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    dir = FPCreateDir(Conn, vol, DIRDID_ROOT, dname);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_nottested();
        goto cleanup;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, dir, name, OPENACC_RD);

    if (!fork) {
        test_nottested();
        goto cleanup;
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, dir, name, OPENACC_RD);

    if (!fork1) {
        test_nottested();
        goto cleanup;          /* cleanup closes fork */
    }

    if (data_fork_path(temp, sizeof(temp), dname, name) < 0) {
        test_nottested();
        goto cleanup;          /* cleanup closes fork/fork1 */
    }

    held = kernel_lock_held(temp, off, 1);

    if (held != 1) {
        if (!Quiet) {
            fprintf(stdout, "\tshare read lock not visible in kernel (held=%d)\n", held);
        }

        test_nottested();
        goto cleanup;          /* cleanup closes fork/fork1 */
    }

    FAIL(FPCloseFork(Conn, fork))     /* fork1 still references it */
    fork = 0;
    held = kernel_lock_held(temp, off, 1);

    if (held != 1) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED: shared read lock dropped after first close "
                    "(over-unlock) - held=%d\n", held);
        }

        test_failed();
    }

    FAIL(FPCloseFork(Conn, fork1))    /* now it must be gone */
    fork1 = 0;
    held = kernel_lock_held(temp, off, 1);

    if (held != 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED: read lock still held after last close - held=%d\n",
                    held);
        }

        test_failed();
    }

cleanup:

    if (fork) {
        FPCloseFork(Conn, fork);
    }

    if (fork1) {
        FPCloseFork(Conn, fork1);
    }

    delete_directory_tree(Conn, vol, DIRDID_ROOT, dname);
test_exit:
    exit_test("T2LockAttack:test603: shared read lock survives partial release (kernel)");
}

/* ------------------------------------------------------------------ *
 * test604  Last-close teardown releases every range, kernel-observed.
 * ------------------------------------------------------------------ */
STATIC void test604()
{
    char *dname = "t604";
    char *name = "f";
    uint16_t vol = VolID;
    uint16_t fork;
    int dir;
    int i, held;
    static const off_t ranges[][2] = { {0, 50}, {100, 50}, {1000, 200} };
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    dir = FPCreateDir(Conn, vol, DIRDID_ROOT, dname);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_nottested();
        goto cleanup;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, dir, name,
                      OPENACC_RD | OPENACC_WR);

    if (!fork) {
        test_nottested();
        goto cleanup;
    }

    FAIL(FPSetForkParam(Conn, fork, (1 << FILPBIT_DFLEN), 2000))

    for (i = 0; i < 3; i++) {
        FAIL(FPByteLock(Conn, fork, 0, 0 /* set */, (int)ranges[i][0],
                        (int)ranges[i][1]))
    }

    FAIL(FPCloseFork(Conn, fork))     /* last close drops every lock */

    if (data_fork_path(temp, sizeof(temp), dname, name) < 0) {
        test_nottested();
        goto cleanup;
    }

    for (i = 0; i < 3; i++) {
        held = kernel_lock_held(temp, ranges[i][0], ranges[i][1]);

        if (held != 0) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED: range [%jd,+%jd] still locked after last close "
                                "- held=%d\n", (intmax_t)ranges[i][0],
                        (intmax_t)ranges[i][1], held);
            }

            test_failed();
        }
    }

cleanup:
    delete_directory_tree(Conn, vol, DIRDID_ROOT, dname);
test_exit:
    exit_test("T2LockAttack:test604: last-close releases all byte locks (kernel)");
}

/* ------------------------------------------------------------------ *
 * test605  A foreign-process share-mode band lock blocks delete.
 *
 * A non-afpd process (this test) takes a raw F_WRLCK at the AD_FILELOCK_OPEN_WR
 * band offset on the data fork's backing file, with no afpd fork held.  A
 * cross-session FPDelete must detect the open-fork band lock and return
 * AFPERR_BUSY.
 * ------------------------------------------------------------------ */
STATIC void test605()
{
    char *dname = "t605";
    char *name = "f";
    uint16_t vol = VolID;
    uint16_t vol2;
    int dir;
    int fd = -1;
    struct flock fl;
    unsigned int dret;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    dir = FPCreateDir(Conn, vol, DIRDID_ROOT, dname);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_nottested();
        goto cleanup;
    }

    if (data_fork_path(temp, sizeof(temp), dname, name) < 0) {
        test_nottested();
        goto cleanup;
    }

    fd = open(temp, O_RDWR, 0);

    if (fd < 0) {
        test_nottested();
        goto cleanup;
    }

    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = AD_FILELOCK_OPEN_WR;
    fl.l_len    = 1;

    if (fcntl(fd, F_SETLK, &fl) < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tcould not take foreign band lock: %s\n", strerror(errno));
        }

        test_nottested();
        goto cleanup;
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto cleanup;
    }

    /* the foreign band lock must block the cross-session delete */
    dret = FPDelete(Conn2, vol2, dir, name);
    FAIL(ntohl(AFPERR_BUSY) != dret)
    FAIL(FPCloseVol(Conn2, vol2))
cleanup:

    if (fd >= 0) {
        fl.l_type = F_UNLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = AD_FILELOCK_OPEN_WR;
        fl.l_len = 1;
        fcntl(fd, F_SETLK, &fl);
        close(fd);
    }

    delete_directory_tree(Conn, vol, DIRDID_ROOT, dname);
test_exit:
    exit_test("T2LockAttack:test605: foreign band lock blocks delete");
}

/* ------------------------------------------------------------------ *
 * test620  Delete contention: a directory holding an open, byte-locked file.
 *
 * Conn opens a file inside a test directory and takes a byte lock. While it is
 * held:
 *   - deleting the parent directory returns AFPERR_DIRNEMPT (offspring present);
 *   - deleting the file from a second session returns AFPERR_BUSY (open by a user).
 * After Conn closes the fork, both the file and the directory delete cleanly.
 * Exercises the delete-vs-lock contention surface and the recursive cleanup of a
 * directory whose contents were locked.
 * ------------------------------------------------------------------ */
STATIC void test620()
{
    char *dname = "t620";
    char *name = "f";
    uint16_t vol = VolID;
    uint16_t vol2;
    uint16_t fork;
    int dir;
    int have_vol2 = 0;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    dir = FPCreateDir(Conn, vol, DIRDID_ROOT, dname);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_nottested();
        goto cleanup;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, dir, name,
                      OPENACC_RD | OPENACC_WR);

    if (!fork) {
        test_nottested();
        goto cleanup;
    }

    FAIL(FPSetForkParam(Conn, fork, (1 << FILPBIT_DFLEN), 200))
    FAIL(FPByteLock(Conn, fork, 0, 0 /* set */, 0, 100))
    /* deleting the non-empty parent directory must report DIRNEMPT */
    FAIL(ntohl(AFPERR_DIRNEMPT) != FPDelete(Conn, vol, DIRDID_ROOT, dname))
    /* deleting the open file from another session must report BUSY */
    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        FPCloseFork(Conn, fork);
        goto cleanup;
    }

    have_vol2 = 1;
    FAIL(ntohl(AFPERR_BUSY) != FPDelete(Conn2, vol2, dir, name))
    /* after the holder closes, the file is deletable again */
    FAIL(FPCloseFork(Conn, fork))
    FAIL(FPDelete(Conn2, vol2, dir, name))
cleanup:

    if (have_vol2) {
        FPCloseVol(Conn2, vol2);
    }

    /* recursive safety net: removes the dir (and any residue) regardless of
     * which path we arrived from */
    delete_directory_tree(Conn, vol, DIRDID_ROOT, dname);
test_exit:
    exit_test("T2LockAttack:test620: delete contention on a dir with a locked file");
}

/* ------------------------------------------------------------------ *
 * test607  Delete vs a foreign-process data-content read lock.
 *
 * A non-afpd process (e.g. Samba) takes an fcntl(F_RDLCK) on content [0,100] of
 * the backing file, with no AFP fork open. The current server refuses a
 * cross-session FPDelete (AFPERR_BUSY) because its delete-time check takes a
 * whole-file trial lock that overlaps any byte-range lock — it blocks on any byte
 * lock, which is too coarse.
 *
 * The correct outcome for a foreign read lock is not yet finalised and depends on
 * an ownership-escalation rule still under design:
 *   - a write byte-range lock / a file open for writing is active modification and
 *     should block a delete;
 *   - a read byte-range lock should not block the file's Unix owner (ownership is
 *     the escalation), but blocking a non-owner whose target is being read may be
 *     acceptable.
 * Until that policy is decided this test is not wired into the testset (its body
 * is kept ready for when the expected outcome is finalised).
 * ------------------------------------------------------------------ */
STATIC void test607()
{
    char *dname = "t607";
    char *name = "f";
    uint16_t vol = VolID;
    uint16_t vol2;
    uint16_t fork;
    int dir;
    int fd = -1;
    struct flock fl;
    unsigned int dret;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    dir = FPCreateDir(Conn, vol, DIRDID_ROOT, dname);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_nottested();
        goto cleanup;
    }

    /* give the file > 100 bytes so the content range is real */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, dir, name,
                      OPENACC_RD | OPENACC_WR);

    if (!fork) {
        test_nottested();
        goto cleanup;
    }

    FAIL(FPSetForkParam(Conn, fork, (1 << FILPBIT_DFLEN), 200))
    FAIL(FPCloseFork(Conn, fork))

    if (data_fork_path(temp, sizeof(temp), dname, name) < 0) {
        test_nottested();
        goto cleanup;
    }

    fd = open(temp, O_RDWR, 0);

    if (fd < 0) {
        test_nottested();
        goto cleanup;
    }

    fl.l_type   = F_RDLCK;                  /* a foreign read content lock */
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 100;

    if (fcntl(fd, F_SETLK, &fl) < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tcould not take foreign read lock: %s\n", strerror(errno));
        }

        test_nottested();
        goto cleanup;
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto cleanup;
    }

    /* The assertion below encodes today's over-broad "any byte lock blocks"
     * outcome only as scaffolding; the real expected result is undecided (it
     * depends on a file-ownership escalation rule) and must be set once that
     * policy is finalised.  Until then this test is not wired into the testset. */
    dret = FPDelete(Conn2, vol2, dir, name);
    FAIL(ntohl(AFPERR_BUSY) != dret)
    FAIL(FPCloseVol(Conn2, vol2))
cleanup:

    if (fd >= 0) {
        fl.l_type = F_UNLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 100;
        fcntl(fd, F_SETLK, &fl);
        close(fd);
    }

    delete_directory_tree(Conn, vol, DIRDID_ROOT, dname);
test_exit:
    exit_test("T2LockAttack:test607: delete vs foreign content read lock");
}

/* ----------- */
void T2LockAttack_test()
{
    ENTER_TESTSET
    test600();
    test603();
    test604();
    test605();
    /* test607 (delete vs foreign content read lock) is intentionally not wired in
     * yet: its expected outcome depends on an undecided ownership-escalation
     * policy. */
    test620();
}
