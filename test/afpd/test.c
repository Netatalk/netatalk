/*
  Copyright (c) 2010 Frank Lahm <franklahm@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/queue.h>
#include <atalk/util.h>
#include <atalk/volume.h>

#include "afp_config.h"
#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "filedir.h"
#include "hash.h"
#include "subtests.h"
#include "subtests_lock.h"
#include "test.h"
#include "test_capabilities.h"
#include "volume.h"

unsigned char nologin = 0;
static AFPObj obj, aspobj;
/* args[2] is filled with an absolute path to test.conf at startup: opening a
 * second volume triggers a load_volumes() config reload, and by then afpd has
 * fchdir()'d into a volume, so a relative "test.conf" would no longer resolve. */
static char  confpath[MAXPATHLEN + 1];
static char *args[] = {"test", "-F", confpath};
int test_output_tap = 0;
int test_case_num = 0;
FILE *test_report_stream = NULL;

int main(int argc, char *argv[])
{
    int reti;
    uint16_t vid;
    uint16_t vidsys;
    struct vol *vol;
    struct vol *volsys;
    struct dir *retdir;
    struct path *path;
    int test_report_fd;
    signal(SIGPIPE, SIG_IGN);

    /* Resolve test.conf to an absolute path before any chdir (see args[] note). */
    if (realpath("test.conf", confpath) == NULL) {
        /* fall back to the relative name; single-volume use still works */
        snprintf(confpath, sizeof(confpath), "test.conf");
    }

    if (argc == 2 && strcmp(argv[1], "--tap") == 0) {
        test_output_tap = 1;
    }

    test_report_fd = dup(STDOUT_FILENO);

    if (test_report_fd != -1) {
        test_report_stream = fdopen(test_report_fd, "w");

        if (test_report_stream == NULL) {
            close(test_report_fd);
        }
    }

    if (test_report_stream == NULL) {
        if (test_output_tap) {
            printf("Bail out! failed to initialize TAP output\n");
            fflush(stdout);
            exit(1);
        }

        test_report_stream = stdout;
    }

    if (test_output_tap) {
        setvbuf(test_report_stream, NULL, _IONBF, 0);
    }

    /* initialize */
    test_section("Initializing", "============");
    /* Log to stderr, not /dev/tty: under a non-interactive runner (CI, meson
     * test, docker run) there is no controlling terminal, so /dev/tty writes
     * fail with ENXIO and every LOG() is silently dropped.  stderr is captured. */
    TEST(setuplog("default:note", "/dev/stderr", true),
         "init logging to stderr");
    TEST(afp_options_parse_cmdline(&obj, 3, &args[0]),
         "parse afpd command-line options");
    TEST_int(afp_config_parse(&obj, NULL), 0,
             "parse afp.conf into AFPObj config");
    TEST_expr(reti = 0,
              obj.options.Cnid_srv != NULL
              && strcmp(obj.options.Cnid_srv, "127.0.0.1") == 0,
              "config: CNID server parsed as 127.0.0.1");
    TEST_expr(reti = 0,
              obj.options.Cnid_port != NULL
              && strcmp(obj.options.Cnid_port, "4700") == 0,
              "config: CNID port parsed as 4700");
    TEST_int(configinit(&obj, &aspobj), 0,
             "initialize server config state");
    TEST(cnid_init(), "initialize CNID subsystem");
    TEST(load_volumes(&obj, LV_ALL), "load all volumes from config");
    TEST_int(dircache_init(8192), 0, "initialize dircache (8192 entries)");
    obj.afp_version = 34;
    /* No IPC channel or dircache hint pipe in test harness */
    obj.ipc_fd = -1;
    obj.hint_fd = -1;
    /* Set global AFPobj — normally done by afp_over_dsi() which tests bypass */
    AFPobj = &obj;

    if (!test_output_tap) {
        fprintf(test_stream(), "\n");
    }

    /* now run tests */
    test_section("Running tests", "=============");
    TEST_expr(vid = openvol(&obj, "afpd_test"), vid != 0,
              "open the afpd_test volume");
    TEST_expr(vol = getvolbyvid(vid), vol != NULL,
              "resolve volume by volume id");
    /* Second volume on the ea=sys backend (metadata in EAs, not a ._ sidecar), so
     * the backend-specific tests can run both legs in-process — but only where the
     * filesystem actually supports user xattrs.  openvol() does not write-probe, so
     * gate on the capability (a real xattr round-trip) to skip honestly elsewhere. */
    volsys = NULL;

    if (test_capability(TEST_CAP_EA_SYS, vol)) {
        TEST_expr(vidsys = openvol(&obj, "afpd_test_sys"), vidsys != 0,
                  "open the afpd_test_sys (ea=sys) volume");
        TEST_expr(volsys = getvolbyvid(vidsys), volsys != NULL,
                  "resolve ea=sys volume by volume id");
    }

    /* No volume-level "cnid server" (only the dbd backend uses it), so the volume
     * inherits the Global value; the port exercises the no-port default. */
    TEST_expr(reti = 0,
              vol->v_cnidserver != NULL
              && strcmp(vol->v_cnidserver, "127.0.0.1") == 0,
              "volume: CNID server inherited from Global");
    TEST_expr(reti = 0,
              vol->v_cnidport != NULL
              && strcmp(vol->v_cnidport, "4700") == 0,
              "volume: CNID port defaults to 4700");
    /* test directory.c stuff */
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT_PARENT), retdir != NULL,
              "dirlookup: root parent directory");
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT), retdir != NULL,
              "dirlookup: root directory");
    TEST_expr(path = cname(vol, retdir, cnamewrap("Network Trash Folder")),
              path != NULL,
              "cname: resolve 'Network Trash Folder'");
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT), retdir != NULL,
              "dirlookup: root directory again");
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT_PARENT, "afpd_test"), 0,
             "getfiledirparms: volume root");
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, ""), 0,
             "getfiledirparms: root dir, empty name");
    TEST_expr(reti = createdir(&obj, vid, DIRDID_ROOT, "dir1"),
              reti == 0 || reti == AFPERR_EXIST,
              "createdir: 'dir1' (created or already exists)");
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, "dir1"), 0,
             "getfiledirparms: 'dir1'");
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, "\000\000"), 0,
             "getfiledirparms: null-name edge case");
    TEST_int(createfile(&obj, vid, DIRDID_ROOT, "dir1/file1"), 0,
             "createfile: 'dir1/file1'");
    TEST_int(delete (&obj, vid, DIRDID_ROOT, "dir1/file1"), 0,
             "delete: 'dir1/file1'");
    TEST_int(delete (&obj, vid, DIRDID_ROOT, "dir1"), 0,
             "delete: 'dir1' directory");
    TEST_int(createfile(&obj, vid, DIRDID_ROOT, "file1"), 0,
             "createfile: 'file1' at root");
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, "file1"), 0,
             "getfiledirparms: 'file1'");
    TEST_int(delete (&obj, vid, DIRDID_ROOT, "file1"), 0,
             "delete: 'file1'");
    /* test enumerate.c stuff */
    TEST_int(enumerate(&obj, vid, DIRDID_ROOT), 0,
             "enumerate: root directory contents");
    /* fork/adouble fault-injection unit tests (TEST_int_or_skip: tests that
     * cannot run on this platform/build return TEST_SKIP rather than failing) */
    TEST_int_or_skip(utest_faultinject_selftest(vol), 0,
                     "fault-injection: LD_PRELOAD interposer is active");
    TEST_int_or_skip(utest_openfork_no_fd_leak(vol), 0,
                     "afp_openfork() error path closes the leaked fork");
    TEST_int_or_skip(utest_shared_adouble_refcount_balance(vol), 0,
                     "shared adouble: ad_close() refcount balance, no early fd close");
    TEST_int_or_skip(utest_adclose_underflow_aborts(vol), 0,
                     "ad_close() hard-fails (abort) on adf_refcount underflow");
    TEST_int_or_skip(utest_ro_retry_strips_destructive_flags(vol), 0,
                     "read-only downgrade retry strips O_TRUNC (no truncate)");
    TEST_int_or_skip(utest_ad2openflags_accmode(vol), 0,
                     "ad2openflags: SETSHRMD|RDONLY promoted to O_RDWR, plain RDONLY stays RO");
    /* delete-conflict GET / of_get_locks / ad_testlock_range unit tests */
    TEST_int_or_skip(utest_testlock_range_clamp(vol), 0,
                     "ad_testlock_range: data-zone probe is clamped, never sweeps the band");
    TEST_int_or_skip(utest_testlock_whole(vol), 0,
                     "ad_testlock_whole: one unclamped F_GETLK spanning content + band");
    TEST_int_or_skip(utest_testlock_range_no_self_report(vol), 0,
                     "ad_testlock_range: F_GETLK does not report our own band entry");
    TEST_int_or_skip(utest_testlock_range_wrlck_sees_rdlck(vol), 0,
                     "ad_testlock_range: F_WRLCK probe sees a peer's F_RDLCK band entry");
    TEST_int_or_skip(utest_of_get_locks_contract(vol), 0,
                     "of_get_locks: positional band bitmap, independent df/rf, tri-valued");
    TEST_int_or_skip(utest_of_get_locks_fastpath(vol), 0,
                     "of_get_locks: >=2-dimension whole-fd fast path, per-bit on a hit");
    TEST_int_or_skip(utest_of_get_locks_failclosed(vol), 0,
                     "of_get_locks: fail-closed/NOENT/absent-rfork (v2 backend)");
    /* same fail-closed + NORF error-handling contract on the ea=sys backend */
    TEST_int_or_skip(volsys ? utest_of_get_locks_failclosed(volsys) : TEST_SKIP, 0,
                     "of_get_locks: fail-closed/NOENT/absent-rfork (ea=sys backend)");
    TEST_int_or_skip(utest_deletefile_quirk_hazard(vol), 0,
                     "POSIX close-drops-locks hazard is real here (negative control)");
    TEST_int_or_skip(utest_deletefile_quirk(vol), 0,
                     "deletefile conflict GET through a held fd preserves the held lock");
    TEST_int_or_skip(utest_deletefile_nodelete(vol), 0,
                     "deletefile honours NODELETE (v2 backend)");
    /* ea=sys leg: only where the filesystem supports user xattrs (else volsys is
     * NULL and the test skips, like any other unmet capability). */
    TEST_int_or_skip(volsys ? utest_deletefile_nodelete(volsys) : TEST_SKIP, 0,
                     "deletefile honours NODELETE (ea=sys backend)");
    /* release-path correctness: per-fork unlock, coalesced-union release, unlock-fail log */
    TEST_int_or_skip(utest_shared_rlock(vol), 0,
                     "shared read lock: kernel lock survives first release, drops on last");
    TEST_int_or_skip(utest_overlap_strand(vol), 0,
                     "overlapping read locks: union unlock leaves no stranded remainder");
    TEST_int_or_skip(utest_freelock_unlck_fail_logs(vol), 0,
                     "adf_freelock: a failed F_UNLCK is logged and the entry still dropped");
    TEST_int_or_skip(utest_fork_setmode_fdeny(vol), 0,
                     "fork_setmode_deny: each access mode maps to the correct deny bits");
    /* cleanup */
    closevol(&obj, vol);

    if (volsys) {
        closevol(&obj, volsys);
    }

    unload_volumes(&obj);
    test_plan(test_case_num);
}
