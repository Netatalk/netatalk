/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test215()
{
    uint16_t vol = VolID;
    uint16_t dt;
    char *file = "t215 file";
    char *file1 = "t215 file1";
    unsigned int ret;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file)) {
        test_nottested();
        goto test_exit;
    }

    dt = FPOpenDT(Conn, vol);
    FAIL(htonl(AFPERR_NOITEM) != FPGetAppl(Conn,  dt, "ttxt", 1, 0x42))
    FAIL(FPAddAPPL(Conn, dt, DIRDID_ROOT, "ttxt", 0xafb471c0, file))
    ret = FPRemoveAPPL(Conn, dt, DIRDID_ROOT_PARENT, "ttxt", file1);

    if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NOITEM, AFPERR_NOOBJ)) {
        test_failed();
    }

    ret = FPRemoveAPPL(Conn, dt, DIRDID_ROOT, "ttut", file1);

    if (not_valid(ret, /* MAC */AFPERR_NOITEM, AFPERR_NOOBJ)) {
        test_failed();
    }

    ret = FPRemoveAPPL(Conn, dt, DIRDID_ROOT, "ttxt", "");

    if (not_valid(ret, /* MAC */AFPERR_NOITEM, AFPERR_BADTYPE)) {
        test_failed();
    }

    FAIL(htonl(AFPERR_PARAM) != FPRemoveAPPL(Conn, dt, 0, "ttxt", file))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, file))
    FAIL(FPRemoveAPPL(Conn, dt, DIRDID_ROOT, "ttxt", file))
    FAIL(FPCloseDT(Conn, dt))
test_exit:
    exit_test("FPRemoveAPPL:test215: remove appl");
}

/* ----------- */
void FPRemoveAPPL_test()
{
    ENTER_TESTSET
    test215();
}
