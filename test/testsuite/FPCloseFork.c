/* ----------------------------------------------
*/
#include "specs.h"

/* ----------- */
STATIC void test186()
{
    int fork;
    uint16_t bitmap = 0;
    uint16_t vol = VolID;
    int type = OPENFORK_DATA;
    char *name = "t186 FPCloseFork";
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_nottested();
        FPDelete(Conn, vol, DIRDID_ROOT, name);
        goto test_exit;
    }

    FAIL(FPCloseFork(Conn, fork))
    /* double close */
    FAIL(htonl(AFPERR_PARAM) != FPCloseFork(Conn, fork))
    FAIL(htonl(AFPERR_PARAM) != FPCloseFork(Conn, 0))

    if (FPDelete(Conn, vol, DIRDID_ROOT, name)) {
        test_nottested();
    }

test_exit:
    exit_test("FPCloseFork:test186: FPCloseFork");
}

/* -------------------------- */
STATIC void test187()
{
    char *name = "t187 illegal fork";
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST
    illegal_fork(dsi, AFP_CLOSEFORK, name);
    exit_test("FPCloseFork:test187: illegal fork");
}

/* ----------- */
void FPCloseFork_test()
{
    ENTER_TESTSET
    test186();
    test187();
}
