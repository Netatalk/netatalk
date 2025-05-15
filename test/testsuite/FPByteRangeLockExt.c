/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
static void test_bytelock_ext(uint16_t vol, char *name, int type) {
    int fork;
    int fork1;
    uint16_t bitmap = 0;
    int len = (type == OPENFORK_RSCS) ? (1 << FILPBIT_RFLEN) : (1 << FILPBIT_DFLEN);

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto fin;
    }

    fork = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    if (FPByteLock_ext(Conn, fork, 0, 0 /* set */, 0, 100)) {
        test_failed();
    } else if (FPByteLock_ext(Conn, fork, 0, 1 /* clear */, 0, 100)) {
        test_failed();
    }

    FAIL(FPByteLock_ext(Conn, fork, 0, 0 /* set */, 0, 100))
    FAIL(htonl(AFPERR_PARAM) != FPByteLock_ext(Conn, fork, 0, 0, -1, 75))
    FAIL(htonl(AFPERR_NORANGE) != FPByteLock_ext(Conn, fork, 0, 1 /* clear */, 0,
            75))
    FAIL(htonl(AFPERR_RANGEOVR) != FPByteLock_ext(Conn, fork, 0, 0, 80 /* set */,
            100))
    fork1 = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                       OPENACC_WR | OPENACC_RD);

    if (!fork1) {
        test_failed();
    } else {
        FAIL(htonl(AFPERR_LOCK) != FPByteLock_ext(Conn, fork1, 0, 0 /* set */, 20, 60))
        FAIL(FPSetForkParam(Conn, fork, len, 50))
        FAIL(FPSetForkParam(Conn, fork1, len, 60))
        FAIL(htonl(AFPERR_LOCK) != FPRead(Conn, fork1, 0, 40, Data))
        FAIL(htonl(AFPERR_LOCK) != FPWrite(Conn, fork1, 10, 40, Data, 0))
        FAIL(FPCloseFork(Conn, fork1))
    }

    /* fin */
    /* Netatalk 2 doesn't drop locks when a fork is closed, 3 does */
    FPByteLock_ext(Conn, fork, 0, 1 /* clear */, 0, 100);
    FAIL(htonl(AFPERR_NORANGE) != FPByteLock_ext(Conn, fork, 0, 1 /* clear */, 0,
            50))
    FAIL(FPSetForkParam(Conn, fork, len, 200))

    if (FPByteLock_ext(Conn, fork, 1 /* end */, 0 /* set */, 0, 100)) {
        test_failed();
    } else if (FPByteLock_ext(Conn, fork, 0, 1 /* clear */, 200, 100)) {
        test_failed();
    }

    if (FPByteLock_ext(Conn, fork, 0 /* end */, 0 /* set */, 0, -1)) {
        test_failed();
    } else if (FPByteLock_ext(Conn, fork, 0, 1 /* clear */, 0, -1)) {
        test_failed();
    }

    FAIL(htonl(AFPERR_PARAM) != FPByteLock_ext(Conn, fork, 0 /* start */,
            0 /* set */, 0, 0))
    FAIL(htonl(AFPERR_PARAM) != FPByteLock_ext(Conn, fork, 0 /* start */,
            0 /* set */, 0, -2))
    FAIL(FPCloseFork(Conn, fork))
    FAIL(htonl(AFPERR_PARAM) != FPByteLock_ext(Conn, fork, 0, 0 /* set */, 0, 100))
fin:
    FPDelete(Conn, vol, DIRDID_ROOT, name);
}

/* ----------- */
// FIXME: broken since at least 3.1.12
STATIC void test66() {
    char *name = "t66 FPByteLock_ext DF";
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    test_bytelock_ext(VolID, name, OPENFORK_DATA);
test_exit:
    exit_test("FPByteRangeLockExt:test66: FPByteLock Data Fork");
}

/* ----------- */
// FIXME: broken since at least 3.1.12 - could not locate fork
STATIC void test67() {
    char *name = "t67 FPByteLock_ext RF";
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    test_bytelock_ext(VolID, name, OPENFORK_RSCS);
test_exit:
    exit_test("FPByteRangeLockExt:test67: FPByteLock Resource Fork");
}


/* -------------------------- */
STATIC void test195() {
    char *name = "test195 illegal fork";
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    illegal_fork(dsi, AFP_BYTELOCK_EXT, name);
test_exit:
    exit_test("FPByteRangeLockExt:test195: illegal fork");
}

/* ----------- */
void FPByteRangeLockExt_test() {
    ENTER_TESTSET
#if 0
    test66();
    test67();
#endif
    test195();
}
