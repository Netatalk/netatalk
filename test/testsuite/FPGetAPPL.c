/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test169()
{
    uint16_t vol = VolID;
    uint16_t dt;
    char *file = "t169 file";
    int dir;
    ENTER_TEST

    /* Not supported with the mysql backend */
    if (Exclude) {
        test_skipped(T_EXCLUDE);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file)) {
        test_nottested();
        goto test_exit;
    }

    dt = FPOpenDT(Conn, vol);
    FAIL(htonl(AFPERR_NOITEM) != FPGetAppl(Conn,  dt, "ttxt", 1, 0x42))
    FAIL(FPAddAPPL(Conn, dt, DIRDID_ROOT, "ttxt", 0xafb471c0, file))
    FAIL(htonl(AFPERR_BITMAP) != FPGetAppl(Conn,  dt, "ttxt", 1, 0xffff))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, file))
    FAIL(htonl(AFPERR_NOITEM) != FPGetAppl(Conn,  dt, "ttxt", 1, 0x42))

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, file))) {
        test_failed();
        goto test_exit;
    }

    FAIL(htonl(AFPERR_NOITEM) != FPGetAppl(Conn,  dt, "ttxt", 1, 0x42))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, file))
    FAIL(FPRemoveAPPL(Conn, dt, DIRDID_ROOT, "ttxt", file))
    FAIL(FPCloseDT(Conn, dt))
test_exit:
    exit_test("FPGetAPPL:test169: test appl");
}

/* ----------- */
void FPGetAPPL_test()
{
    ENTER_TESTSET
    test169();
}
