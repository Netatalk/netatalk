/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------
*/

/* ------------- */
static void check_forklen(DSI *dsi, int type, uint32_t  len) {
    uint32_t flen;
    flen = get_forklen(dsi, type);

    if (flen != len) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED got %d but %d expected\n", flen, len);
        }

        test_failed();
    }
}

/* ------------- */
STATIC void test_21(uint16_t vol, char *name, int type) {
    int fork = 0;
    int fork2 = 0;
    uint16_t bitmap = 0;
    int len = (type == OPENFORK_RSCS) ? (1 << FILPBIT_RFLEN) : (1 << FILPBIT_DFLEN);
    DSI *dsi;
    dsi = &Conn->dsi;
    fork = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fork2 = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                       OPENACC_WR | OPENACC_RD);

    if (!fork2) {
        test_failed();
        goto fin;
    }

    if (FPWrite(Conn, fork, 0, 100, Data, 0)) {
        test_failed();
        goto fin;
    }

    bitmap = len;

    if (FPGetForkParam(Conn, fork, bitmap)) {
        test_failed();
        goto fin;
    }

    check_forklen(dsi, type, 100);

    if (FPGetForkParam(Conn, fork2, bitmap)) {
        test_failed();
        goto fin;
    }

    check_forklen(dsi, type, 100);
    FAIL(FPFlushFork(Conn, fork))

    if (FPGetForkParam(Conn, fork, bitmap)) {
        test_failed();
        goto fin;
    }

    check_forklen(dsi, type, 100);

    if (FPGetForkParam(Conn, fork2, bitmap)) {
        test_failed();
        goto fin;
    }

    check_forklen(dsi, type, 100);
fin:
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(fork2 && FPCloseFork(Conn, fork2))
}

/* -------- */
STATIC void test21() {
    uint16_t vol = VolID;
    char *name = "t21 file";
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    test_21(VolID, name, OPENFORK_DATA);
    test_21(VolID, name, OPENFORK_RSCS);
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetForkParms:test21: setting/reading fork len");
}

/* --------------------------
FIXME set resource for size and check
*/
STATIC void test50() {
    uint16_t bitmap;
    int fork = 0;
    int fork2 = 0;
    char *name = "t50 file";
    char *name1 = "t50 new name";
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR |  OPENACC_DWR);

    if (!fork) {
        test_nottested();
        goto fin;
    }

    bitmap = (1 << FILPBIT_DFLEN);

    if (FPGetForkParam(Conn, fork, bitmap)) {
        test_failed();
        goto fin;
    }

    bitmap = (1 << FILPBIT_RFLEN);

    if (ntohl(AFPERR_BITMAP) != FPGetForkParam(Conn, fork, bitmap)) {
        test_failed();
        goto fin;
    }

    if (FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1)) {
        test_failed();
        goto fin;
    }

    fork2 = FPOpenFork(Conn, vol, OPENFORK_RSCS, 0, DIRDID_ROOT, name1,
                       OPENACC_RD | OPENACC_WR);

    if (!fork2) {
        test_failed();
        goto fin;
    }

    bitmap = (1 << FILPBIT_DFLEN) | (1 << FILPBIT_MDATE);

    if (ntohl(AFPERR_BITMAP) != FPGetForkParam(Conn, fork2, bitmap)) {
        test_failed();
    }

    bitmap = (1 << FILPBIT_RFLEN) | (1 << FILPBIT_MDATE);

    if (FPGetForkParam(Conn, fork2, bitmap)) {
        test_failed();
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = 0;
    bitmap = (1 << FILPBIT_DFLEN) | (1 << FILPBIT_MDATE);

    if (ntohl(AFPERR_BITMAP) != FPGetForkParam(Conn, fork2, bitmap)) {
        test_failed();
    }

    bitmap = (1 << FILPBIT_RFLEN) | (1 << FILPBIT_MDATE);

    if (FPGetForkParam(Conn, fork2, bitmap)) {
        test_failed();
    }

fin:
    FAIL(fork2 && FPCloseFork(Conn, fork2))
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
test_exit:
    exit_test("FPGetForkParms:test50: deny mode & move file");
}

/* -------------------------- */
STATIC void test188() {
    char *name = "t188 illegal fork";
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST
    illegal_fork(dsi, AFP_GETFORKPARAM, name);
    exit_test("FPGetForkParms:test188: illegal fork");
}

/* -------------------------- */
STATIC void test192() {
    uint16_t bitmap;
    int fork = 0;
    char *name = "t192 file";
    int ret;
    uint16_t vol = VolID;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name, OPENACC_WR);

    if (!fork) {
        test_nottested();
        goto fin;
    }

    bitmap = (1 << FILPBIT_DFLEN);

    if (FPGetForkParam(Conn, fork, bitmap)) {
        test_failed();
        goto fin;
    }

    bitmap = (1 << FILPBIT_RFLEN) | (1 << FILPBIT_MDATE);
    ret = FPGetForkParam(Conn, fork, bitmap);

    if (not_valid_bitmap(ret, BITERR_ACCESS | BITERR_BITMAP, AFPERR_BITMAP)) {
        test_failed();
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = 0;
fin:
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetForkParms:test192: open write only");
}

/* ------------------------- */
STATIC void test305() {
    uint16_t bitmap = 0;
    int fork;
    char *name = "t305 file.txt";
    uint16_t vol = VolID;
    int len = (1 << FILPBIT_DFLEN);
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    FAIL(FPSetForkParam(Conn, fork, (1 << FILPBIT_DFLEN), 1024))
    FAIL(FPWrite(Conn, fork, 2048, 0, Data, 0))
    bitmap = len;

    if (FPGetForkParam(Conn, fork, bitmap)) {
        test_failed();
        goto fin;
    }

    check_forklen(dsi, OPENFORK_DATA, 1024);
fin:
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetForkParms:test305: fork len after a 0 byte write");
}

/* ----------- */
void FPGetForkParms_test() {
    ENTER_TESTSET
    test21();
    test50();
    test188();
    test192();
    test305();
}
