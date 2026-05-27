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
#include "test.h"
#include "volume.h"

unsigned char nologin = 0;
static AFPObj obj, aspobj;
static char *args[] = {"test", "-F", "test.conf"};
int test_output_tap = 0;
int test_case_num = 0;
FILE *test_report_stream = NULL;

int main(int argc, char *argv[])
{
    int reti;
    uint16_t vid;
    struct vol *vol;
    struct dir *retdir;
    struct path *path;
    int test_report_fd;
    signal(SIGPIPE, SIG_IGN);

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
    TEST(setuplog("default:note", "/dev/tty", true));
    TEST(afp_options_parse_cmdline(&obj, 3, &args[0]));
    TEST_int(afp_config_parse(&obj, NULL), 0);
    TEST_int(configinit(&obj, &aspobj), 0);
    TEST(cnid_init());
    TEST(load_volumes(&obj, LV_ALL));
    TEST_int(dircache_init(8192), 0);
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
    TEST_expr(vid = openvol(&obj, "afpd_test"), vid != 0);
    TEST_expr(vol = getvolbyvid(vid), vol != NULL);
    /* test directory.c stuff */
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT_PARENT), retdir != NULL);
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT), retdir != NULL);
    TEST_expr(path = cname(vol, retdir, cnamewrap("Network Trash Folder")),
              path != NULL);
    TEST_expr(retdir = dirlookup(vol, DIRDID_ROOT), retdir != NULL);
    TEST_int(getfiledirparms(&obj, vid, DIRDID_ROOT_PARENT, "afpd_test"), 0);
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
    /* cleanup */
    closevol(&obj, vol);
    unload_volumes(&obj);
    test_plan(test_case_num);
}
