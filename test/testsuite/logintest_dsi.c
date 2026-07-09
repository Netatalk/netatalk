/*
 * Copyright (c) 2026 Daniel Markstedt <daniel@mindani.net>
 * Based on work by Rafal Lewczuk, Didier Gautheron, and Frank Lahm
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

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <atalk/afp.h>

#include "afpclient.h"
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

extern int Port;
extern char *Password;
extern char *User;
extern char *vers;
extern DSI *Dsi;

static void connect_server(CONN *conn)
{
    int sock;
    sock = OpenClientSocket(Server, Port);

    if (sock < 0) {
        test_nottested();
        exit(ExitCode);
    }

    memset(&conn->dsi, 0, sizeof(conn->dsi));
    conn->dsi.socket = sock;
    /* Guard against a server that accepts the TCP connection but stalls before
     * replying (e.g. parent busy reaping prior children after a connection-limit
     * test). Without this, dsi_stream_read blocks in read() indefinitely. */
    struct timeval tv = { .tv_sec = 15, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static int is_bad_uam(unsigned int ret)
{
    return ntohl(ret) == (uint32_t)AFPERR_BADUAM;
}

/* ------------------------- */
void test_dsi_getstatus_no_session(void)
{
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;

    if (!Quiet) {
        fprintf(stdout, "DSIGetStatus\n");
    }

    if (DSIGetStatus(Conn)) {
        test_failed();
    }

    CloseClientSocket(Dsi->socket);
    exit_test("Logintest:dsi_getstatus_no_session: DSI with no open session");
}

/* ------------------------- */
void test_dsi_open_close_session(void)
{
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;

    if (!Quiet) {
        fprintf(stdout, "DSIOpenSession\n");
    }

    if (DSIOpenSession(Conn)) {
        test_failed();
        goto test_exit;
    }

    if (Mac) {
        if (!Quiet) {
            fprintf(stdout, "DSIGetStatus\n");
        }

        if (DSIGetStatus(Conn)) {
            test_failed();
            goto test_exit;
        }
    }

    if (!Quiet) {
        fprintf(stdout, "DSICloseSession\n");
    }

    if (DSICloseSession(Conn)) {
        test_failed();
        goto test_exit;
    }

    CloseClientSocket(Dsi->socket);
test_exit:
    exit_test("Logintest:dsi_open_close_session: DSI with open session");
}

/* ------------------------- */
void test_dsi_open_session_ignore_param(void)
{
    DSI *dsi;
    uint32_t i = 0;
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;
    dsi = Dsi;
    /* DSIOpenSession */
    memset(&dsi->header, 0, sizeof(dsi->header));
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_OPEN;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    dsi->header.dsi_code = 6;
    dsi->cmdlen = 2 + sizeof(i);
    dsi->commands[0] = DSIOPT_ATTNQUANT;
    dsi->commands[1] = sizeof(i);
    i = htonl(DSI_DEFQUANT);
    memcpy(dsi->commands + 2, &i, sizeof(i));
    dsi_send(dsi);
    dsi_cmd_receive(dsi);

    if (dsi->header.dsi_code) {
        test_failed();
        goto test_exit;
    }

    if (!Quiet) {
        fprintf(stdout, "DSICloseSession\n");
    }

    if (DSICloseSession(Conn)) {
        test_failed();
        goto test_exit;
    }

test_exit:
    CloseClientSocket(Dsi->socket);
    exit_test("Logintest:dsi_open_session_ignore_param: DSIOpenSession non zero parameter should be ignored by the server");
}

/* Direct round-trip through the renamed dsi_stream_send / dsi_cmd_receive
 * primitives. Sends an unknown AFP command (255) to an open but
 * unauthenticated session; expect AFPERR_NOOP from the server's switch
 * dispatcher. Catches regressions in the I/O layer. */
void test_dsi_command_roundtrip(void)
{
    DSI *dsi;
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;
    dsi = Dsi;

    if (DSIOpenSession(Conn)) {
        test_failed();
        goto test_exit;
    }

    memset(&dsi->header, 0, sizeof(dsi->header));
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_CMD;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    dsi->commands[0] = 0xFF;            /* unknown AFP command */
    dsi->commands[1] = 0;
    dsi->cmdlen = 2;
    dsi_send(dsi);
    dsi_cmd_receive(dsi);

    /* Any non-zero error code means the round trip worked. A zero result
     * would mean the server accepted an unknown command, which would be a
     * server bug, not a client one. */
    if (dsi->header.dsi_code == 0) {
        test_failed();
        goto test_exit;
    }

    if (DSICloseSession(Conn)) {
        test_failed();
        goto test_exit;
    }

test_exit:
    CloseClientSocket(Dsi->socket);
    exit_test("Logintest:dsi_command_roundtrip: DSI round-trip via renamed dsi_stream_send/dsi_cmd_receive");
}

/* With Netatalk we define the default as 200 maximum connections,
 * while with the AFP server on Mac OS X 10.7 Lion the default is 10.
 * Therefore, test up to 201 to exercise either limit. */
#define TEST_MAX_CONNS 201

/* The AFP Reference defines kFPNoMoreSessions (-1068) for FPLogin/FPLoginExt
 * when the server is at its session cap (Table 50: FPLoginExt result
 * codes), but says nothing about a DSI-layer error code for this case.
 * netatalk actually enforces the cap one layer down, in dsi_getsess.c,
 * by sending DSIERR_SERVBUSY (0xFBD1) on the DSIOpenSession reply — so
 * the client never reaches the FPLogin stage. This test accepts either
 * code as "limit hit". */
void test_connection_limit(void)
{
    static char *uam = "No User Authent";
    CONN conn[TEST_MAX_CONNS];
    int cnt = 0;
    unsigned int ret = 0;
    uint32_t code_host;
    int hit_limit = 0;
    ENTER_TEST

    for (int i = 0; i < TEST_MAX_CONNS; i++) {
        if (!Quiet) {
            fprintf(stdout, "conn %d\n", i);
        }

        connect_server(&conn[i]);
        Dsi = &conn[i].dsi;
        cnt++;

        if (Version >= 30) {
            ret = FPopenLoginExt(&conn[i], vers, uam, "", "");
        } else {
            ret = FPopenLogin(&conn[i], vers, uam, "", "");
        }

        code_host = ntohl(ret);

        if (code_host == (uint32_t) AFPERR_MAXSESS
                || code_host == DSIERR_SERVBUSY) {
            hit_limit = 1;
            break;
        }

        if (ret) {
            if (is_bad_uam(ret)) {
                test_skipped(T_UAM);
                goto cleanup;
            }

            if (!Quiet) {
                fprintf(stdout,
                        "Unexpected login error at conn %d: %d (%s)\n",
                        i, code_host, afp_error(ret));
            }

            test_failed();
            goto cleanup;
        }
    }

    if (!hit_limit) {
        if (!Quiet) {
            fprintf(stdout,
                    "All %d logins succeeded; server cap is > %d\n",
                    cnt, cnt);
        }

        test_nottested();
    }

cleanup:
    /* Send DSICloseSession on all connections that have an open DSI session,
     * then close sockets. DSIERR_SERVBUSY means DSIOpenSession itself was
     * rejected -- no DSI session was established, so skip DSICloseSession for
     * that slot and just shut its socket. AFPERR_MAXSESS means DSIOpenSession
     * succeeded but AFP rejected the login -- the DSI session IS open and must
     * be properly closed, or the server keeps the slot occupied. Sleep after to
     * let the server reap workers, otherwise the next test may fail with
     * SERVBUSY waiting for a free fork slot. */
    {
        int failed_idx = (hit_limit && code_host == DSIERR_SERVBUSY) ? cnt - 1 : -1;

        for (int j = 0; j < cnt; j++) {
            if (j != failed_idx) {
                DSICloseSession(&conn[j]);
            }

            CloseClientSocket(conn[j].dsi.socket);
        }
    }

    if (cnt > 0) {
        fflush(stdout);
        sleep(3);
    }

    exit_test("Logintest:connection_limit: connection limit hit returns server-busy");
}

/* Raw AFP helper coverage. The libafpclient-backed UAM matrix also checks
 * guest authentication, but through a different client stack; keep this test
 * as a direct FPLogin/FPLoginExt smoke test, including AFP 2.x fallback. */
void test_guest_login(void)
{
    static char *uam = "No User Authent";
    int ret;
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;

    if (Version >= 30) {
        ret = FPopenLoginExt(Conn, vers, uam, "", "");
    } else {
        ret = FPopenLogin(Conn, vers, uam, "", "");
    }

    if (ret) {
        if (is_bad_uam(ret)) {
            test_skipped(T_UAM);
            goto test_exit;
        }

        test_failed();
        goto test_exit;
    }

    if (Mac) {
        fprintf(stdout, "DSIGetStatus\n");

        if (DSIGetStatus(Conn)) {
            test_failed();
            goto test_exit;
        }
    }

    if (FPLogOut(Conn)) {
        test_failed();
        goto test_exit;
    }

test_exit:
    CloseClientSocket(Dsi->socket);
    exit_test("Logintest:guest_login: Guest login");
}

/* Raw AFP helper coverage. The libafpclient-backed UAM matrix has broader
 * cleartext UAM assertions; this test stays as a minimal direct login/logout
 * check for the testsuite-local AFP packet helpers. */
void test_cleartext_login(void)
{
    static char *uam = "Cleartxt Passwrd";
    int ret;
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;

    if (!User || Password[0] == '\0') {
        test_skipped(T_CRED);
        goto test_exit;
    }

    if (Version >= 30) {
        ret = FPopenLoginExt(Conn, vers, uam, User, Password);
    } else {
        ret = FPopenLogin(Conn, vers, uam, User, Password);
    }

    if (ret) {
        if (is_bad_uam(ret)) {
            test_skipped(T_UAM);
            goto test_exit;
        }

        test_failed();
        goto test_exit;
    }

    Conn->afp_version = Version;

    if (FPLogOut(Conn)) {
        test_failed();
    }

test_exit:
    CloseClientSocket(Dsi->socket);
    exit_test("Logintest:cleartext_login: Clear text login");
}

/* FPLoginExt + No User Authent via AFPopenLoginExt_pwd directly, bypassing
 * the FPopenLoginExt wrapper. Validates the post-B1 wire format
 * independently of the wrapper's version-branching. AFP >= 3.0 only. */
void test_loginext_guest_direct(void)
{
    static const char *uam = "No User Authent";
    unsigned int ret;
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;

    if (Version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    ret = AFPopenLoginExt_pwd(Conn, vers, uam, "", "");

    if (ret) {
        if (is_bad_uam(ret)) {
            test_skipped(T_UAM);
            goto test_exit;
        }

        test_failed();
        goto test_exit;
    }

    if (FPLogOut(Conn)) {
        test_failed();
    }

test_exit:
    CloseClientSocket(Dsi->socket);
    exit_test("Logintest:loginext_guest_direct: FPLoginExt + No User Authent (direct)");
}

/* Smoke test for AFPLoginCont primitive (Part B4). Drives DHCAST128 with
 * an empty UserAuthInfo to provoke kFPAuthContinue, then sends garbage
 * via AFPLoginCont to verify the wire format reaches the server's
 * logincont entry point. Skipped if the server does not load DHCAST128
 * (returns AFPERR_BADUAM) or doesn't return kFPAuthContinue. Does NOT
 * authenticate -- this only exercises the round-trip plumbing. */
void test_logincont_roundtrip(void)
{
    static const char *uam = "DHCAST128";
    unsigned int ret;
    uint8_t garbage[16];
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;

    if (Version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    if (!User) {
        test_skipped(T_CRED);
        goto test_exit;
    }

    ret = AFPopenLoginExt(Conn, vers, uam, User, NULL, 0);

    if (ntohl(ret) != (uint32_t) AFPERR_AUTHCONT) {
        if (is_bad_uam(ret)) {
            test_skipped(T_UAM);
            goto test_exit;
        }

        if (!Quiet) {
            fprintf(stdout,
                    "AFPopenLoginExt returned %d (%s), expected AFPERR_AUTHCONT\n",
                    ntohl(ret), afp_error(ret));
        }

        test_nottested();
        goto test_exit;
    }

    if (Conn->login_cont_len == 0) {
        /* Got AUTHCONT but no payload captured -- that's a real bug. */
        test_failed();
        goto test_exit;
    }

    memset(garbage, 0xAA, sizeof(garbage));
    ret = AFPLoginCont(Conn, garbage, sizeof(garbage));

    /* Any non-zero result is acceptable: the server rejected our garbage
     * response, which means AFPLoginCont reached the UAM. A zero result
     * would mean we authenticated with random bytes, which is impossible
     * unless something is very wrong. */
    if (ret == 0) {
        test_failed();
    }

test_exit:
    CloseClientSocket(Dsi->socket);
    exit_test("Logintest:logincont_roundtrip: AFPLoginCont primitive round-trip");
}
