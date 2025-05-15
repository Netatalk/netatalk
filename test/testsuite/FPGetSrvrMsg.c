/* ----------------------------------------------
*/
#include "specs.h"

/* ----------------------- */
STATIC void test210(void) {
    ENTER_TEST
    FAIL(FPGetSrvrMsg(Conn, 0, 0))
    FAIL(FPGetSrvrMsg(Conn, 1, 0))
    exit_test("FPGetSrvrMsg:test210: GetSrvrParms call");
}

/* ----------- */
void FPGetSrvrMsg_test() {
    ENTER_TESTSET
    test210();
}
