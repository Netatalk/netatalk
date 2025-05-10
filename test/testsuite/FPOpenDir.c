/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test57()
{
    unsigned int dir;
    int pdir;
    int rdir = 0;
    char *name = "t57 dir no access";
    char *name2 = "t57 ro dir";
    char *name4 = "t57 dir";
    char *name3 = "t57 file.txt";
    uint16_t vol = VolID;
    int ret;
    DSI *dsi;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (!(pdir = no_access_folder(vol, DIRDID_ROOT, name))) {
        goto test_exit;
    }

    if (!(rdir = read_only_folder(vol, DIRDID_ROOT, name2))) {
        goto fin;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name4))) {
        test_failed();
        goto fin;
    }

    dsi = &Conn->dsi;
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name3))
    dir = FPOpenDir(Conn, vol, DIRDID_ROOT, name);
    ret = dsi->header.dsi_code;

    if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
        test_failed();
    }

    dir = FPOpenDir(Conn, vol, DIRDID_ROOT, name2);

    if (!dir) {
        test_failed();
    }

    dir = FPOpenDir(Conn, vol, DIRDID_ROOT, name3);

    if (dir || ntohl(AFPERR_BADTYPE) != dsi->header.dsi_code) {
        test_failed();
    }

    dir = FPOpenDir(Conn, vol, DIRDID_ROOT_PARENT, "");

    if (dir || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
        test_failed();
    }

    dir = FPOpenDir(Conn, vol, DIRDID_ROOT_PARENT, Vol);

    if (dir != DIRDID_ROOT) {
        test_failed();
    }

    dir = FPOpenDir(Conn, vol + 1, DIRDID_ROOT_PARENT, "");

    if (dir || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
        test_failed();
    }

    dir = FPOpenDir(Conn, vol, DIRDID_ROOT, name4);

    if (!dir) {
        test_failed();
        goto fin;
    }

    FAIL(FPCloseDir(Conn, vol, dir))
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name4))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name3))
    delete_folder(vol, DIRDID_ROOT, name);

    if (rdir) {
        delete_folder(vol, DIRDID_ROOT, name2);
    }

test_exit:
    exit_test("FPOpenDir:test57: OpenDir call");
}

/* ----------- */
void FPOpenDir_test()
{
    ENTER_TESTSET
    test57();
}
