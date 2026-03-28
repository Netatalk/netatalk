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
#include <libgen.h>
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
#include <atalk/util.h>
#include <atalk/vfs.h>
#include <atalk/volume.h>

#include "nad.h"

static void usage_mkdir(void)
{
    printf(
        "Usage: nad mkdir [-pv] <dir> [<dir> ...]\n\n"
        "The mkdir utility creates the directories named as operands.\n"
        "If the directories reside on an AFP volume, the corresponding\n"
        "CNIDs are added to the volume's database and AppleDouble\n"
        "metadata is initialized.\n\n"
        "The options are as follows:\n\n"
        "   -p   Create intermediate directories as required.\n"
        "   -v   Be verbose when creating directories, showing them\n"
        "        as they are created.\n"
    );
    exit(EXIT_FAILURE);
}

/*!
 * @brief Create a single directory with CNID and AppleDouble metadata
 *
 * @param[in] path       absolute path of directory to create
 * @param[in] mode       permission mode for the new directory
 * @param[in] vol        open AFP volume
 *
 * @returns 0 on success, -1 on error
 */
static int mkdir_with_cnid(const char *path, mode_t mode, afpvol_t *vol)
{
    cnid_t did, pdid;
    struct adouble ad;
    struct stat st;

    if (mkdir(path, mode) != 0) {
        SLOG("mkdir: %s: %s", path, strerror(errno));
        return -1;
    }

    if (!vol->vol->v_path || !ADVOL_V2_OR_EA(vol->vol->v_adouble)) {
        return 0;
    }

    if (vol->vol->v_adouble == AD_VERSION2) {
        bstring addir = bfromcstr(path);
        bcatcstr(addir, "/.AppleDouble");
        mkdir(cfrombstr(addir), 0777);
        chmod(cfrombstr(addir), 02777);
        bdestroy(addir);
    }

    if ((did = cnid_for_path(vol->vol->v_cdb, vol->vol->v_path, path,
                             &pdid)) == CNID_INVALID) {
        SLOG("Error resolving CNID for %s", path);
        return -1;
    }

    if (lstat(path, &st) != 0) {
        SLOG("lstat: %s: %s", path, strerror(errno));
        return -1;
    }

    ad_init(&ad, vol->vol);

    if (ad_open(&ad, path,
                ADFLAGS_HF | ADFLAGS_DIR | ADFLAGS_RDWR | ADFLAGS_CREATE,
                0666) != 0) {
        SLOG("Error opening adouble for: %s", path);
        return -1;
    }

    ad_setid(&ad, st.st_dev, st.st_ino, did, pdid, vol->db_stamp);

    if (vol->vol->v_adouble == AD_VERSION2) {
        char buf[MAXPATHLEN + 1];
        strlcpy(buf, path, sizeof(buf));
        ad_setname(&ad, convert_utf8_to_mac(vol->vol, basename(buf)));
    }

    ad_setdate(&ad, AD_DATE_CREATE | AD_DATE_UNIX, (uint32_t)st.st_mtime);
    ad_setdate(&ad, AD_DATE_MODIFY | AD_DATE_UNIX, (uint32_t)st.st_mtime);
    ad_setdate(&ad, AD_DATE_ACCESS | AD_DATE_UNIX, (uint32_t)st.st_mtime);
    ad_setdate(&ad, AD_DATE_BACKUP, AD_DATE_START);
    ad_flush(&ad);
    ad_close(&ad, ADFLAGS_HF);
    return 0;
}

/*!
 * @brief Create directory and optionally intermediate parents
 *
 * When pflag is set, creates all missing intermediate directories
 * along the path, registering each with the CNID database.
 *
 * @param[in] path       path of directory to create
 * @param[in] vol        open AFP volume
 *
 * @returns 0 on success, 1 on error
 */
static int do_mkdir(const char *path, afpvol_t *vol)
{
    struct stat st;

    if (!pflag) {
        if (mkdir_with_cnid(path, 0777, vol) != 0) {
            return 1;
        }

        if (vflag) {
            printf("%s\n", path);
        }

        return 0;
    }

    /* -p mode: create intermediate directories as needed */
    char buf[MAXPATHLEN + 1];

    if (strlcpy(buf, path, sizeof(buf)) >= sizeof(buf)) {
        SLOG("Path too long: %s", path);
        return 1;
    }

    /* Walk the path from left to right, creating dirs as needed */
    for (char *p = buf + 1; *p; p++) {
        if (*p != '/') {
            continue;
        }

        *p = '\0';

        if (lstat(buf, &st) != 0) {
            if (errno != ENOENT) {
                SLOG("lstat: %s: %s", buf, strerror(errno));
                return 1;
            }

            if (mkdir_with_cnid(buf, 0777, vol) != 0) {
                return 1;
            }

            if (vflag) {
                printf("%s\n", buf);
            }
        } else if (!S_ISDIR(st.st_mode)) {
            SLOG("%s: Not a directory", buf);
            return 1;
        }

        *p = '/';
    }

    /* Create the final directory */
    if (lstat(buf, &st) != 0) {
        if (errno != ENOENT) {
            SLOG("lstat: %s: %s", buf, strerror(errno));
            return 1;
        }

        if (mkdir_with_cnid(buf, 0777, vol) != 0) {
            return 1;
        }

        if (vflag) {
            printf("%s\n", buf);
        }
    } else if (!S_ISDIR(st.st_mode)) {
        SLOG("%s: Not a directory", buf);
        return 1;
    }

    return 0;
}

int nad_mkdir(int argc, char *argv[], AFPObj *obj)
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
            usage_mkdir();
            break;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc < 1) {
        usage_mkdir();
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

        if (do_mkdir(argv[i], &volume) != 0) {
            rval = 1;
        }

        closevol(&volume);
    }

    return rval;
}
