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
 * @brief Test capabilities: environment/build features a unit test may depend on.
 *
 * Some tests can only run where a feature is present (the LD_PRELOAD shim reaches
 * libatalk; the filesystem supports user extended attributes for an ea=sys
 * volume).  A test probes the capability it needs and returns TEST_SKIP when it is
 * absent, so coverage stays honest per platform rather than failing where the
 * feature is simply unavailable.  Each capability is probed once by actually
 * exercising it (e.g. an xattr round-trip), not by trusting a higher-level
 * operation that may succeed without the underlying support; the result is
 * cached for the run.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atalk/acl.h>
#include <atalk/ea.h>
#include <atalk/volume.h>

#include "faultinject.h"
#include "test_capabilities.h"

/* Cached probe results: -1 = not yet probed, 0 = absent, 1 = present. */
static int cap_cache[] = {
    [TEST_CAP_FAULT_INJECT] = -1,
    [TEST_CAP_EA_SYS]       = -1,
    [TEST_CAP_SOCKET_SPLICE] = -1,
    [TEST_CAP_ACL_SET]      = -1,
    [TEST_CAP_XATTR_WATCH]  = -1,
    [TEST_CAP_STAT_WATCH]  = -1,
    [TEST_CAP_FSTAT_WATCH]  = -1,
    [TEST_CAP_CLOSE_WATCH]  = -1,
    [TEST_CAP_PREAD_WATCH]  = -1,
};

/*!
 * @brief Short capability name for skip and log messages
 *
 * @param[in] cap  the capability
 * @returns its name
 */
const char *test_capability_name(enum test_capability cap)
{
    switch (cap) {
    case TEST_CAP_FAULT_INJECT:
        return "fault-injection (LD_PRELOAD)";

    case TEST_CAP_EA_SYS:
        return "user extended attributes (ea=sys)";

    case TEST_CAP_SOCKET_SPLICE:
        return "splice() from an AF_UNIX stream socket";

    case TEST_CAP_XATTR_WATCH:
        return "xattr call counting (LD_PRELOAD)";

    case TEST_CAP_STAT_WATCH:
        return "stat() call counting (LD_PRELOAD)";

    case TEST_CAP_FSTAT_WATCH:
        return "fstat() call counting (LD_PRELOAD)";

    case TEST_CAP_CLOSE_WATCH:
        return "close() call counting (LD_PRELOAD)";

    case TEST_CAP_PREAD_WATCH:
        return "pread() call counting (LD_PRELOAD)";

    case TEST_CAP_ACL_SET:
        return "a filesystem that stores ACLs";

    default:
        return "unknown";
    }
}

/*!
 * @brief Does the filesystem under vol->v_path support user extended attributes?
 *
 * Round-trips a real xattr on a scratch file: set, read back, remove.  An ea=sys
 * volume stores AppleDouble metadata in xattrs, so without this the volume opens
 * (openvol() does not write-probe) but every metadata write fails — which would
 * surface as a test failure rather than an honest skip.  Probing here keeps the
 * skip honest.
 */
static int probe_ea_sys(const struct vol *vol)
{
    char path[MAXPATHLEN + 1];
    char buf[4];
    int fd;
    int ok = 0;

    if (snprintf(path, sizeof(path), "%s/.cap_ea_probe", vol->v_path)
            >= (int)sizeof(path)) {
        return 0;
    }

    fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);

    if (fd < 0) {
        return 0;
    }

    close(fd);

    /* set -> read back -> verify; AD_EA_META is netatalk's own metadata EA name,
     * so the probe uses the exact namespace an ea=sys volume writes. */
    if (sys_setxattr(path, AD_EA_META, "ok", 2, 0) == 0
            && sys_getxattr(path, AD_EA_META, buf, sizeof(buf)) == 2) {
        ok = 1;
    }

    (void)sys_removexattr(path, AD_EA_META);
    (void)unlink(path);
    return ok;
}

/*!
 * @brief Is the LD_PRELOAD interposer active and reaching libc here?
 *
 * Arms a one-shot open() failure and checks it fired.  macOS's two-level
 * namespace ignores a plain symbol preload, so the injection never takes effect
 * there and fault-injection-dependent tests skip.
 */
static int probe_fault_inject(const struct vol *vol)
{
    char path[MAXPATHLEN + 1];

    if (snprintf(path, sizeof(path), "%s/.cap_fi_probe", vol->v_path)
            >= (int)sizeof(path)) {
        return 0;
    }

    return faultinject_open_works(path);
}

/*!
 * @brief Can splice() read from an AF_UNIX stream socket (Linux 4.2 on)
 *        into a pipe of the default splice size?
 *
 * A real uid past its pipe budget gets two-page pipes that cannot grow, and
 * afpd declines every recvfile write.
 */
static int probe_socket_splice(void)
{
#ifdef WITH_RECVFILE
    int s[2];
    int p[2];
    int ok = 0;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, s) != 0) {
        return 0;
    }

    if (pipe(p) == 0) {
        ok = fcntl(p[1], F_SETPIPE_SZ, DEFAULT_SPLICE_SIZE)
             == DEFAULT_SPLICE_SIZE
             && write(s[0], "x", 1) == 1
             && splice(s[1], NULL, p[1], NULL, 1, SPLICE_F_NONBLOCK) == 1;
        close(p[0]);
        close(p[1]);
    }

    close(s[0]);
    close(s[1]);
    return ok;
#else
    return 0;
#endif
}

/*!
 * @brief Does the preload count libatalk's xattr reads into fi?
 *
 * Watches one sys_getxattr() of the volume root: the interposers exist on
 * Linux only, and the call reaches them whatever the filesystem answers.
 */
static int probe_xattr_watch(const struct vol *vol)
{
    char buf[4];
    int worked;
    fault_inject_reset();
    fi.xattr_watch = 1;
    (void)sys_getxattr(vol->v_path, AD_EA_META, buf, sizeof(buf));
    worked = fi.getxattr_calls == 1;
    fault_inject_reset();
    return worked;
}

/*!
 * @brief Does the preload count stat() and lstat() calls of a path by its
 *        suffix?
 *
 * The test binary's own lstat() takes the symbol libatalk's does; a libc
 * that routes the calls through another symbol leaves the count at zero.
 */
static int probe_stat_watch(const struct vol *vol)
{
    struct stat st;
    int worked;
    fault_inject_reset();
    fi.stat_suffix = vol->v_path;
    (void)lstat(vol->v_path, &st);
    worked = fi.stat_suffix_calls == 1;
    fault_inject_reset();
    return worked;
}

/*!
 * @brief Does the preload count fstat() calls of every descriptor while
 *        fstat_fd is -1?
 *
 * @returns 1 when one fstat() of a pipe counts once, else 0
 */
static int probe_fstat_watch(void)
{
    struct stat st;
    int p[2];
    int worked;

    if (pipe(p) != 0) {
        return 0;
    }

    fault_inject_reset();
    fi.fstat_watch = 1;
    fi.fstat_fd = -1;
    worked = fstat(p[0], &st) == 0 && fi.fstat_calls == 1;
    fault_inject_reset();
    close(p[0]);
    close(p[1]);
    return worked;
}

/*!
 * @brief Does the preload count close() calls through a countdown armed
 *        never to fire?
 *
 * @returns 1 when one close() of a pipe counts once, else 0
 */
static int probe_close_watch(void)
{
    int p[2];
    int worked;

    if (pipe(p) != 0) {
        return 0;
    }

    fault_inject_reset();
    fi.close.armed = 1;
    fi.close.fail_after = -1;
    worked = close(p[0]) == 0 && fi.close.calls == 1;
    fault_inject_reset();
    close(p[1]);
    return worked;
}

/*!
 * @brief Does the preload count pread() calls?
 *
 * @param[in] vol  volume whose root takes the probe's file
 * @returns 1 when one pread() of an empty file counts once, else 0
 */
static int probe_pread_watch(const struct vol *vol)
{
    char path[MAXPATHLEN + 1];
    char byte;
    int fd;
    int worked;

    if (snprintf(path, sizeof(path), "%s/.cap_pread_probe", vol->v_path)
            >= (int)sizeof(path)
            || (fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600)) < 0) {
        return 0;
    }

    fault_inject_reset();
    worked = pread(fd, &byte, 1, 0) == 0 && fi.pread_calls == 1;
    fault_inject_reset();
    close(fd);
    (void)unlink(path);
    return worked;
}

/*!
 * @brief Does the filesystem under vol->v_path store an ACL written to it?
 *
 * Writes the root's own ACL back, which leaves it as it was; a filesystem
 * that reads ACLs but stores none, as illumos tmpfs, refuses the write.
 */
static int probe_acl_set(const struct vol *vol _U_)
{
#if defined(HAVE_NFSV4_ACLS)
    ace_t *aces = NULL;
    int count = get_nfsv4_acl(vol->v_path, &aces);
    int ok = count >= 0 && acl(vol->v_path, ACE_SETACL, count, aces) == 0;
    free(aces);
    return ok;
#elif defined(HAVE_POSIX_ACLS)
    acl_t own = acl_get_file(vol->v_path, ACL_TYPE_ACCESS);
    int ok = own != NULL
             && acl_set_file(vol->v_path, ACL_TYPE_ACCESS, own) == 0;

    if (own) {
        acl_free(own);
    }

    return ok;
#else
    return 0;
#endif
}

/*!
 * @brief Probe a capability; the result is cached for the run
 *
 * @param[in] cap  the capability to probe
 * @param[in] vol  supplies a filesystem path for capabilities probed against
 *                 the volume (TEST_CAP_EA_SYS, TEST_CAP_FAULT_INJECT and
 *                 TEST_CAP_PREAD_WATCH use vol->v_path for a scratch file,
 *                 TEST_CAP_ACL_SET writes its ACL back); must not be NULL
 * @returns 1 if the capability is available, 0 otherwise
 */
int test_capability(enum test_capability cap, const struct vol *vol)
{
    if (cap < 0 || cap >= (int)(sizeof(cap_cache) / sizeof(cap_cache[0]))) {
        return 0;
    }

    if (cap_cache[cap] != -1) {
        return cap_cache[cap];
    }

    switch (cap) {
    case TEST_CAP_FAULT_INJECT:
        cap_cache[cap] = probe_fault_inject(vol);
        break;

    case TEST_CAP_EA_SYS:
        cap_cache[cap] = probe_ea_sys(vol);
        break;

    case TEST_CAP_SOCKET_SPLICE:
        cap_cache[cap] = probe_socket_splice();
        break;

    case TEST_CAP_XATTR_WATCH:
        cap_cache[cap] = probe_xattr_watch(vol);
        break;

    case TEST_CAP_STAT_WATCH:
        cap_cache[cap] = probe_stat_watch(vol);
        break;

    case TEST_CAP_FSTAT_WATCH:
        cap_cache[cap] = probe_fstat_watch();
        break;

    case TEST_CAP_CLOSE_WATCH:
        cap_cache[cap] = probe_close_watch();
        break;

    case TEST_CAP_PREAD_WATCH:
        cap_cache[cap] = probe_pread_watch(vol);
        break;

    case TEST_CAP_ACL_SET:
        cap_cache[cap] = probe_acl_set(vol);
        break;

    default:
        cap_cache[cap] = 0;
        break;
    }

    return cap_cache[cap];
}
