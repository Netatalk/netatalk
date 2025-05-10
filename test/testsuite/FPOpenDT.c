/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test200()
{
    uint16_t  dir;
    uint16_t vol = VolID;
    DSI *dsi;
    ENTER_TEST
    dsi = &Conn->dsi;
    dir = FPOpenDT(Conn, vol);

    if (dir == 0xffff) {
        test_failed();
    }

    dir = FPOpenDT(Conn, vol + 1);

    if (dir != 0xffff || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
        test_failed();
    }

    exit_test("FPOpenDT:test200: OpenDT call");
}

/* ----------- */
void FPOpenDT_test()
{
    ENTER_TESTSET
    test200();
}
