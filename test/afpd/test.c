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
#include <string.h>

#include <atalk/bstrlib.h>
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
#include "test.h"
#include "volume.h"

/* Stuff from main.c which of cource can't be added as source to testbin */
unsigned char nologin = 0;
static AFPObj obj, aspobj;
#define ARGNUM 3
static char *args[] = {"test", "-F", "test.conf"};
/* Static variables */

int main() {
    int reti;
    uint16_t vid;
    struct vol *vol;
    struct dir *retdir;
    struct path *path;
    /* initialize */
    printf("Initializing\n============\n");
    TEST(setuplog("default:note", "/dev/tty", true));
    TEST(afp_options_parse_cmdline(&obj, 3, &args[0]));
    TEST_int(afp_config_parse(&obj, NULL), 0);
    TEST_int(configinit(&obj, &aspobj), 0);
    TEST(cnid_init());
    TEST(load_volumes(&obj, LV_ALL));
    TEST_int(dircache_init(8192), 0);
    obj.afp_version = 34;
    printf("\n");
    /* now run tests */
    printf("Running tests\n=============\n");
    TEST_expr(vid = openvol(&obj, "test"), vid != 0);
    TEST_expr(vol = getvolbyvid(vid), vol != NULL);
    /* test directory.c stuff */
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT_PARENT), retdir != NULL);
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT), retdir != NULL);
    TEST_expr(path = cname(vol, retdir, cnamewrap("Network Trash Folder")),
              path != NULL);
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT), retdir != NULL);
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT_PARENT, "test"), 0);
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, ""), 0);
    TEST_expr(reti = createdir(&obj, vid, DIRDID_ROOT, "dir1"),
              reti == 0 || reti == AFPERR_EXIST);
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, "dir1"), 0);
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, "\000\000"), 0);
    TEST_int(createfile(&obj, vid, DIRDID_ROOT, "dir1/file1"), 0);
    TEST_int(delete (&obj, vid, DIRDID_ROOT, "dir1/file1"), 0);
    TEST_int(delete (&obj, vid, DIRDID_ROOT, "dir1"), 0);
    TEST_int(createfile(&obj, vid, DIRDID_ROOT, "file1"), 0);
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT, "file1"), 0);
    TEST_int(delete (&obj, vid, DIRDID_ROOT, "file1"), 0);
    /* test enumerate.c stuff */
    TEST_int(enumerate(&obj, vid, DIRDID_ROOT), 0);
}
