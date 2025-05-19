/* ----------------------------------------------
*/
#include "specs.h"
#include <inttypes.h>

/* ------------------------- */
STATIC void test398()
{
    uint16_t vol = VolID;
    DSI *dsi;
    char *file = "test398_file";
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_ACLS)) {
        test_skipped(T_ACL);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file)) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPGetACL(Conn, vol, DIRDID_ROOT, 7, file))
#if 0
    FPGetACL(Conn,vol, DIRDID_ROOT , 7, "testdir");
    FPGetACL(Conn,vol, DIRDID_ROOT , 8, "test");
    FPGetACL(Conn,vol, DIRDID_ROOT , 23, "test");
    FPGetACL(Conn,vol, DIRDID_ROOT , 23, "test2");
    FPGetACL(Conn,vol, DIRDID_ROOT , 23, "test3");
    FPGetACL(Conn,vol, DIRDID_ROOT , 23, "testdir");
#endif
    FPDelete(Conn, vol, DIRDID_ROOT, file);
test_exit:
    exit_test("FPGetACL:test398: check ACL support");
}

/* ----------- */
void FPGetACL_test()
{
    ENTER_TESTSET
    test398();
}
