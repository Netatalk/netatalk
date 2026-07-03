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
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include <atalk/ea.h>
#include <atalk/volume.h>

#include "faultinject.h"
#include "test_capabilities.h"

/* Cached probe results: -1 = not yet probed, 0 = absent, 1 = present. */
static int cap_cache[] = {
    [TEST_CAP_FAULT_INJECT] = -1,
    [TEST_CAP_EA_SYS]       = -1,
};

/*! @brief Short capability name for skip/log messages. */
const char *test_capability_name(enum test_capability cap)
{
    switch (cap) {
    case TEST_CAP_FAULT_INJECT:
        return "fault-injection (LD_PRELOAD)";

    case TEST_CAP_EA_SYS:
        return "user extended attributes (ea=sys)";

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
 * @brief Probe a capability; the result is cached for the run.
 *
 * @param cap  the capability to probe
 * @param vol  supplies a filesystem path for capabilities probed against the
 *             volume (TEST_CAP_EA_SYS, TEST_CAP_FAULT_INJECT use vol->v_path for
 *             a scratch file); must not be NULL.
 * @returns    1 if the capability is available, 0 otherwise.
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

    default:
        cap_cache[cap] = 0;
        break;
    }

    return cap_cache[cap];
}
