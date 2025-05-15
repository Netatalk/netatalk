/* ----------------------------------------------
*/
#include "specs.h"

extern char icon0_256[];

/* -------------------------- */
STATIC void test213() {
    uint16_t vol = VolID;
    uint16_t dt;
    unsigned int ret;
    u_char   u_null[] = { 0, 0, 0, 0 };
    ENTER_TEST

    // Not supported with the mysql backend
    if (Exclude) {
        test_skipped(T_EXCLUDE);
        goto test_exit;
    }

    dt = FPOpenDT(Conn, vol);
    ret = FPGetIconInfo(Conn,  dt, (unsigned char *) "ttxt", 1);

    if (ret == htonl(AFPERR_NOITEM)) {
        FAIL(FPAddIcon(Conn,  dt, "ttxt", "3DMF", 1, 0, 256, icon0_256))
        goto fin;
    }

    FAIL(FPGetIconInfo(Conn,  dt, (unsigned char *) "ttxt", 1))
    FAIL(htonl(AFPERR_NOITEM) != FPGetIconInfo(Conn,  dt, (unsigned char *) "ttxt",
            256))

    if (!Mac) {
        ret = FPGetIconInfo(Conn,  dt, (unsigned char *) "UNIX", 1);

        if (ret == htonl(AFPERR_NOITEM)) {
            FAIL(FPGetIconInfo(Conn,  dt, u_null, 1))
            FAIL(htonl(AFPERR_NOITEM) != FPGetIconInfo(Conn,  dt, u_null, 2))
            goto fin;
        } else if (ret) {
            test_failed();
            goto fin;
        } else {
            FAIL(htonl(AFPERR_NOITEM) != FPGetIconInfo(Conn,  dt, (unsigned char *) "UNIX",
                    2))
        }
    }

fin:
    FPCloseDT(Conn, dt);
test_exit:
    exit_test("FPGetIconInfo:test213: get Icon Info call");
}

/* ----------- */
void FPGetIconInfo_test() {
    ENTER_TESTSET
    test213();
}
