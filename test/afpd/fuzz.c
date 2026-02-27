/*
  Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>

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
 * libFuzzer fuzz target for afpd file/directory operations
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/globals.h>
#include <atalk/logger.h>
#include <atalk/netatalk_conf.h>
#include <atalk/volume.h>

#include "afp_config.h"
#include "afpfunc_helpers.h"
#include "dircache.h"
#include "directory.h"
#include "file.h"
#include "filedir.h"
#include "hash.h"
#include "subtests.h"
#include "test.h"
#include "volume.h"

unsigned char nologin = 0;

static AFPObj obj, aspobj;
static uint16_t vid;
static int initialized;
static char start_cwd[4096];

int LLVMFuzzerInitialize(int *argc _U_, char ***argv _U_)
{
    static char *args[] = {"fuzz", "-F", "test.conf"};
    signal(SIGPIPE, SIG_IGN);

    /*
     * Save the working directory before initialization.
     * AFP operations change cwd into the volume path, which would
     * break libFuzzer's resolution of relative seed corpus paths.
     * We restore it after init and after every fuzz iteration.
     */
    if (getcwd(start_cwd, sizeof(start_cwd)) == NULL) {
        return 0;
    }

    setuplog("default:note", "/dev/null", true);
    afp_options_parse_cmdline(&obj, 3, &args[0]);

    if (afp_config_parse(&obj, NULL) != 0) {
        return 0;
    }

    if (configinit(&obj, &aspobj) != 0) {
        return 0;
    }

    cnid_init();
    load_volumes(&obj, LV_ALL);

    if (dircache_init(8192) != 0) {
        return 0;
    }

    obj.afp_version = 34;
    obj.ipc_fd = -1;
    obj.hint_fd = -1;
    AFPobj = &obj;
    vid = openvol(&obj, "afpd_test");

    if (vid != 0) {
        initialized = 1;
    }

    /* Restore cwd so libFuzzer can find the seed corpus */
    if (chdir(start_cwd) != 0) {
        return 0;
    }

    return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (!initialized) {
        return 0;
    }

    /*
     * Cap at 230 bytes — the helper functions use static char buf[256]
     * with up to 19 bytes of AFP protocol headers + 7 byte path header
     * prepended before the name.
     */
    if (size > 230) {
        size = 230;
    }

    /* Build a null-terminated name from the fuzzer input */
    char name[231];
    memcpy(name, data, size);
    name[size] = '\0';
    /* Exercise directory operations */
    createdir(&obj, vid, DIRDID_ROOT, name);
    getfiledirparms(&obj, vid, DIRDID_ROOT, name);
    delete (&obj, vid, DIRDID_ROOT, name);
    /* Exercise file operations */
    createfile(&obj, vid, DIRDID_ROOT, name);
    getfiledirparms(&obj, vid, DIRDID_ROOT, name);
    delete (&obj, vid, DIRDID_ROOT, name);
    /* Exercise enumerate */
    enumerate(&obj, vid, DIRDID_ROOT);
    /* Restore cwd — AFP operations may have changed it */
    chdir(start_cwd);
    return 0;
}
