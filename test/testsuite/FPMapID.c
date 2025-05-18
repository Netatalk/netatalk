/* ----------------------------------------------
*/
#include "specs.h"

STATIC void test208()
{
    int  dir;
    char *name = "t208 test Map ID";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) |
                      (1 << DIRPBIT_MDATE)
                      | (1 << DIRPBIT_ACCESS) | (1 << DIRPBIT_FINFO) | (1 << DIRPBIT_UID) |
                      (1 << DIRPBIT_GID) ;
    unsigned int ret;
    uint16_t vol = VolID;
    DSI *dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap))
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    /* user to Mac roman */
    FAIL(FPMapID(Conn, 1, 0))
    /* user to Mac roman */
    FAIL(FPMapID(Conn, 1, filedir.uid))
    /* user to Mac roman */
    ret = FPMapID(Conn, 1, -filedir.uid);

    if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NOITEM, AFPERR_NOITEM)) {
        test_failed();
    }

    /* group to Mac roman */
    ret = FPMapID(Conn, 2, -filedir.gid);

    /* sometime -filedir.gid is there */
    if (ret && not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NOITEM, AFPERR_NOITEM)) {
        test_failed();
    }

    /* group to Mac roman */
    FAIL(FPMapID(Conn, 2, filedir.gid))
    /* user to UTF8 */
    ret = FPMapID(Conn, 3, filedir.uid);

    if (Conn->afp_version >= 30 && ret) {
        test_failed();
    } else if (Conn->afp_version < 30 && ret != htonl(AFPERR_PARAM)) {
        test_failed();
    }

    /* group to UTF8 */
    ret = FPMapID(Conn, 4, filedir.gid);

    if (Conn->afp_version >= 30 && ret) {
        test_failed();
    } else if (Conn->afp_version < 30 && ret != htonl(AFPERR_PARAM)) {
        test_failed();
    }

    /* Older AFP versions only have 4 subfunctions */
    if (Conn->afp_version > 31) {
        FAIL((htonl(AFPERR_NOITEM) != FPMapID(Conn, 5, filedir.gid)))
    }

    /* --------------------- */
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPMapID:test208: test Map ID call");
}

/* ----------- */
void FPMapID_test()
{
    ENTER_TESTSET
    test208();
}
