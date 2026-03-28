/*
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <errno.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <bstrlib.h>

#include <atalk/adouble.h>
#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/util.h>
#include <atalk/vfs.h>
#include <atalk/volume.h>

#include "nad.h"

static void usage_rmdir(void)
{
    printf(
        "Usage: nad rmdir [-pv] <dir> [<dir> ...]\n\n"
        "The rmdir utility removes empty directories in an AFP volume.\n"
        "The corresponding CNIDs are deleted from the volume's database,\n"
        "and AppleDouble metadata is removed.\n\n"
        "The options are as follows:\n\n"
        "   -p   Each dir operand is treated as a pathname of which all\n"
        "        components will be removed, if they are empty, starting\n"
        "        with the last most component.\n"
        "   -v   Be verbose when removing directories, showing them\n"
        "        as they are removed.\n"
    );
    exit(EXIT_FAILURE);
}

/*!
 * @brief Check if path is the volume root or its parent
 *
 * @returns 1 if path is a protected directory, 0 otherwise
 */
static int is_protected_dir(const char *path, const afpvol_t *vol)
{
    char resolved[PATH_MAX];

    if (realpath(path, resolved) == NULL) {
        return 0;
    }

    /* Check if resolved path is a prefix of (or equal to) the volume path */
    const char *remainder = vol->vol->v_path;
    const char *r = resolved;

    while (*r && *r == *remainder) {
        r++;
        remainder++;
    }

    if (*r == '\0' && (*remainder == '/' || *remainder == '\0')) {
        if (*remainder == '\0') {
            SLOG("Refusing to remove volume root: %s", path);
        } else {
            SLOG("Refusing to remove volume root parent: %s", path);
        }

        return 1;
    }

    return 0;
}

/*!
 * @brief Remove a single empty directory with CNID and AppleDouble cleanup
 *
 * @param[in] path       absolute path of directory to remove
 * @param[in] vol        open AFP volume
 *
 * @returns 0 on success, -1 on error
 */
static int rmdir_with_cnid(const char *path, afpvol_t *vol)
{
    cnid_t did, pdid;

    if (is_protected_dir(path, vol)) {
        return -1;
    }

    if (vol->vol->v_path && ADVOL_V2_OR_EA(vol->vol->v_adouble)) {
        /* Resolve CNID before any modifications */
        if ((did = cnid_for_path(vol->vol->v_cdb, vol->vol->v_path, path,
                                 &pdid)) == CNID_INVALID) {
            SLOG("Error resolving CNID for %s", path);
            return -1;
        }

        /* Remove AD metadata before rmdir */
        if (vol->vol->v_adouble == AD_VERSION2) {
            bstring parent = bfromcstr(path);
            bcatcstr(parent, "/.AppleDouble/.Parent");
            unlink(cfrombstr(parent));
            bdestroy(parent);
            bstring addir = bfromcstr(path);
            bcatcstr(addir, "/.AppleDouble");
            rmdir(cfrombstr(addir));
            bdestroy(addir);
        } else {
            /* EA mode: remove ._dotfile metadata */
            const char *ad_p = vol->vol->ad_path(path, ADFLAGS_DIR);
            unlink(ad_p);
        }

        if (rmdir(path) != 0) {
            SLOG("rmdir: %s: %s", path, strerror(errno));
            return -1;
        }

        /* Only delete CNID after successful removal from disk */
        if (cnid_delete(vol->vol->v_cdb, did) != 0) {
            SLOG("Error removing CNID %u for %s", ntohl(did), path);
            return -1;
        }
    } else {
        if (rmdir(path) != 0) {
            SLOG("rmdir: %s: %s", path, strerror(errno));
            return -1;
        }
    }

    return 0;
}

/*!
 * @brief Remove directory and optionally empty parent directories
 *
 * When pflag is set, removes empty parent directories working
 * upward from the specified path, stopping at the volume root.
 *
 * @param[in] path       path of directory to remove
 * @param[in] vol        open AFP volume
 *
 * @returns 0 on success, 1 on error
 */
static int do_rmdir(const char *path, afpvol_t *vol)
{
    if (rmdir_with_cnid(path, vol) != 0) {
        return 1;
    }

    if (vflag) {
        printf("%s\n", path);
    }

    if (!pflag) {
        return 0;
    }

    /* -p mode: remove empty parent directories */
    char buf[MAXPATHLEN + 1];
    size_t len = strlcpy(buf, path, sizeof(buf));

    if (len >= sizeof(buf)) {
        SLOG("Path too long: %s", path);
        return 1;
    }

    /* Strip trailing slashes */
    char *end = buf + len - 1;

    while (end > buf && *end == '/') {
        *end-- = '\0';
    }

    /* Walk upward, removing empty parents */
    for (;;) {
        char *slash = strrchr(buf, '/');

        if (slash == NULL || slash == buf) {
            break;
        }

        *slash = '\0';

        if (is_protected_dir(buf, vol)) {
            break;
        }

        if (rmdir_with_cnid(buf, vol) != 0) {
            break;
        }

        if (vflag) {
            printf("%s\n", buf);
        }
    }

    return 0;
}

int nad_rmdir(int argc, char *argv[], AFPObj *obj)
{
    int ch;
    int rval = 0;
    afpvol_t volume;

    while ((ch = getopt(argc, argv, "pv")) != -1) {
        switch (ch) {
        case 'p':
            pflag = 1;
            break;

        case 'v':
            vflag = 1;
            break;

        default:
            usage_rmdir();
            break;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 1) {
        usage_rmdir();
    }

    set_signal();
    cnid_init();

    for (int i = 0; i < argc; i++) {
        if (alarmed) {
            SLOG("...break");
            break;
        }

        if (openvol(obj, argv[i], &volume) != 0) {
            rval = 1;
            continue;
        }

        if (do_rmdir(argv[i], &volume) != 0) {
            rval = 1;
        }

        closevol(&volume);
    }

    return rval;
}
