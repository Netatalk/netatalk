/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test201()
{
    uint16_t  dir;
    uint16_t vol = VolID;
    int ret;
    ENTER_TEST

    if (0xffff == (dir = FPOpenDT(Conn, vol))) {
        test_nottested();
        goto test_exit;
    }

    ret = FPCloseDT(Conn, dir + 1);

    if (not_valid(ret, AFPERR_PARAM, 0)) {
        test_failed();
    }

    FAIL(FPCloseDT(Conn, dir))
test_exit:
    exit_test("FPCloseDT:test201: FPCloseDT call");
}

/* ----------- */
void FPCloseDT_test()
{
    ENTER_TESTSET
    test201();
}
