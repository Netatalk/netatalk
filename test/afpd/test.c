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
#include <stdint.h>
#include <unistd.h>

#include <atalk/cnid.h>
#include <atalk/directory.h>
#include <atalk/dsi.h>
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

static void dsi_test_header(uint8_t *block, uint8_t command,
                            uint16_t request_id,
                            uint32_t code_or_doff, uint32_t len)
{
    uint16_t request_id_be;
    uint32_t code_or_doff_be;
    uint32_t len_be;
    uint32_t reserved_be;
    memset(block, 0, DSI_BLOCKSIZ);
    block[0] = DSIFL_REQUEST;
    block[1] = command;
    request_id_be = htons(request_id);
    code_or_doff_be = htonl(code_or_doff);
    len_be = htonl(len);
    reserved_be = 0;
    memcpy(block + 2, &request_id_be, sizeof(request_id_be));
    memcpy(block + 4, &code_or_doff_be, sizeof(code_or_doff_be));
    memcpy(block + 8, &len_be, sizeof(len_be));
    memcpy(block + 12, &reserved_be, sizeof(reserved_be));
}

static int dsi_test_receive(uint8_t command, uint32_t code_or_doff,
                            uint32_t len, const uint8_t *payload,
                            size_t payload_len, uint32_t quantum, DSI *dsi)
{
    uint8_t block[DSI_BLOCKSIZ];
    int fds[2];
    ssize_t written;
    memset(dsi, 0, sizeof(*dsi));
    dsi->socket = -1;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) {
        return -1;
    }

    dsi->socket = fds[1];
    dsi->server_quantum = quantum;
    dsi->commands = calloc(1, quantum);
    dsi->buffer = calloc(1, quantum);

    if (dsi->commands == NULL || dsi->buffer == NULL) {
        close(fds[0]);
        return -1;
    }

    dsi->start = dsi->buffer;
    dsi->eof = dsi->buffer;
    dsi->end = dsi->buffer + quantum;
    dsi_test_header(block, command, 0x1234, code_or_doff, len);
    written = write(fds[0], block, sizeof(block));

    if (written != sizeof(block)) {
        close(fds[0]);
        return -1;
    }

    if (payload_len > 0) {
        written = write(fds[0], payload, payload_len);

        if (written != (ssize_t)payload_len) {
            close(fds[0]);
            return -1;
        }
    }

    close(fds[0]);
    return dsi_stream_receive(dsi);
}

static void dsi_test_cleanup(DSI *dsi)
{
    if (dsi->socket != -1) {
        close(dsi->socket);
    }

    free(dsi->commands);
    free(dsi->buffer);
}

/* T1 (RED before the quantum-math fix): dsi_len = quantum + doff is
 * spec-legal; the quantum bounds write DATA, not the whole frame.  Only
 * the header drives accept/reject, so the harness sends just doff payload
 * bytes; the data portion is consumed later by dsi_write. */
static int utest_dsi_receive_accepts_full_quantum_write(uint32_t doff)
{
    DSI dsi;
    uint8_t cmd[DSI_WROFF_FPWRITEEXT];
    int ret;
    memset(cmd, 0, sizeof(cmd));
    ret = dsi_test_receive(DSIFUNC_WRITE, doff, 4096 + doff, cmd, doff,
                           4096, &dsi);
    dsi_test_cleanup(&dsi);
    return ret == DSIFUNC_WRITE ? 0 : -1;
}

/* T2 (guard): one byte of data beyond the quantum is rejected. */
static int utest_dsi_receive_rejects_overquantum_write(uint32_t doff)
{
    DSI dsi;
    uint8_t cmd[DSI_WROFF_FPWRITEEXT];
    int ret;
    memset(cmd, 0, sizeof(cmd));
    ret = dsi_test_receive(DSIFUNC_WRITE, doff, 4096 + doff + 1, cmd, doff,
                           4096, &dsi);
    dsi_test_cleanup(&dsi);
    return ret == 0 ? 0 : -1;
}

/* T3 (guard): non-write commands keep the strict whole-frame bound. */
static int utest_dsi_receive_rejects_overquantum_cmd(void)
{
    DSI dsi;
    uint8_t payload[1] = { AFP_LOGOUT };
    int ret;
    ret = dsi_test_receive(DSIFUNC_CMD, 0, 4097, payload, 1, 4096, &dsi);
    dsi_test_cleanup(&dsi);
    return ret == 0 ? 0 : -1;
}

/* T4 (RED before the doff whitelist): doff outside {12, 20} is malformed;
 * doff 0 maps to 12 (ASC 3.7). */
static int utest_dsi_receive_validates_doff(void)
{
    DSI dsi;
    uint8_t cmd[DSI_WROFF_FPWRITEEXT + 1];
    int ret;
    memset(cmd, 0, sizeof(cmd));
    ret = dsi_test_receive(DSIFUNC_WRITE, 13, 64, cmd, 13, 4096, &dsi);
    dsi_test_cleanup(&dsi);

    if (ret != 0) {
        return -1;
    }

    ret = dsi_test_receive(DSIFUNC_WRITE, 21, 64, cmd, 21, 4096, &dsi);
    dsi_test_cleanup(&dsi);

    if (ret != 0) {
        return -1;
    }

    ret = dsi_test_receive(DSIFUNC_WRITE, 0, DSI_WROFF_FPWRITE + 8, cmd,
                           DSI_WROFF_FPWRITE + 8, 4096, &dsi);
    dsi_test_cleanup(&dsi);
    return ret == DSIFUNC_WRITE ? 0 : -1;
}

/* T6 (RED before the dsi_writeinit pointer-handoff fix): buffered write
 * payload must be handed off, not copied out.  eof - start == 64 holds
 * because the peer closes before dsi_stream_receive, so one recv coalesces
 * the header and payload writes. */
static int utest_dsi_writeinit_no_duplicate_copy(void)
{
    DSI dsi;
    uint8_t frame[DSI_WROFF_FPWRITE + 64];
    char *wbuf = NULL;
    size_t i;
    size_t cc;
    int ret;
    memset(frame, 0, DSI_WROFF_FPWRITE);

    for (i = 0; i < 64; i++) {
        frame[DSI_WROFF_FPWRITE + i] = (uint8_t)i;
    }

    ret = dsi_test_receive(DSIFUNC_WRITE, DSI_WROFF_FPWRITE,
                           DSI_WROFF_FPWRITE + 64, frame, sizeof(frame),
                           4096, &dsi);

    if (ret != DSIFUNC_WRITE || dsi.eof - dsi.start != 64) {
        dsi_test_cleanup(&dsi);
        return -1;
    }

    memset(dsi.commands, 0xAA, 4096);
    cc = dsi_writeinit(&dsi, &wbuf);

    if (cc != 64 || dsi.datasize != 0) {
        dsi_test_cleanup(&dsi);
        return -1;
    }

    for (i = 0; i < 64; i++) {
        if (dsi.commands[i] != 0xAA) {
            dsi_test_cleanup(&dsi);
            return -1;
        }
    }

    if (wbuf == NULL || wbuf < dsi.buffer || wbuf + cc > dsi.end
            || wbuf[0] != 0 || (uint8_t)wbuf[63] != 63) {
        dsi_test_cleanup(&dsi);
        return -1;
    }

    dsi_test_cleanup(&dsi);
    return 0;
}

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
    /* DSI unit tests: frame acceptance and write handoff (critical backports). */
    TEST_int(utest_dsi_receive_accepts_full_quantum_write(DSI_WROFF_FPWRITE), 0);
    TEST_int(utest_dsi_receive_accepts_full_quantum_write(DSI_WROFF_FPWRITEEXT), 0);
    TEST_int(utest_dsi_receive_rejects_overquantum_write(DSI_WROFF_FPWRITE), 0);
    TEST_int(utest_dsi_receive_rejects_overquantum_write(DSI_WROFF_FPWRITEEXT), 0);
    TEST_int(utest_dsi_receive_rejects_overquantum_cmd(), 0);
    TEST_int(utest_dsi_receive_validates_doff(), 0);
    TEST_int(utest_dsi_writeinit_no_duplicate_copy(), 0);
    TEST(afp_options_parse_cmdline(&obj, 3, &args[0]));
    TEST_int(afp_config_parse(&obj, NULL), 0);
    TEST_expr(reti = 0,
              obj.options.Cnid_srv != NULL
              && strcmp(obj.options.Cnid_srv, "127.0.0.1") == 0);
    TEST_expr(reti = 0,
              obj.options.Cnid_port != NULL
              && strcmp(obj.options.Cnid_port, "4700") == 0);
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
    TEST_expr(reti = 0,
              vol->v_cnidserver != NULL
              && strcmp(vol->v_cnidserver, "localhost") == 0);
    TEST_expr(reti = 0,
              vol->v_cnidport != NULL
              && strcmp(vol->v_cnidport, "4700") == 0);
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
