/* ----------------------------------------------
*/
#include "adoublehelper.h"
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/* ------------------------- */
STATIC void test89()
{
    int  dir;
    char *file = "t89 test error setfilparam";
    char *name = "t89 error setfilparams dir";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << FILPBIT_FINFO) | (1 << FILPBIT_CDATE) |
                      (1 << FILPBIT_BDATE) | (1 << FILPBIT_MDATE);
    uint16_t vol = VolID;
    const DSI *dsi = &Conn->dsi;
    unsigned ret;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!(dir = folder_with_ro_adouble(vol, DIRDID_ROOT, name, file))) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, dir, file, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
        ret = FPSetFileParams(Conn, vol, dir, file, bitmap, &filedir);

        if (not_valid(ret, 0, AFPERR_ACCESS)) {
            test_failed();
        }
    }

    bitmap = (1 << FILPBIT_MDATE);

    if (FPGetFileDirParams(Conn, vol, dir, file, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
        FAIL(FPSetFileParams(Conn, vol, dir, file, bitmap, &filedir))
    }

    delete_ro_adouble(vol, dir, file);
test_exit:
    exit_test("FPSetFileParms:test89: test set file setfilparam");
}

/* ------------------------- */
STATIC void test120()
{
    char *name = "t120 test file setfilparam";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << FILPBIT_ATTR) | (1 << FILPBIT_FINFO) |
                      (1 << FILPBIT_CDATE) |
                      (1 << FILPBIT_BDATE) | (1 << FILPBIT_MDATE);
    uint16_t vol = VolID;
    const DSI *dsi = &Conn->dsi;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
        delete_unix_md(Path, "", name);
        FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetFileParms:test120: test set file setfilparam (create .AppleDouble)");
}

/* ------------------------- */
STATIC void test426()
{
    char *name = "t426 Symlink";
    char *target = "t426 dest";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap;
    uint16_t vol = VolID;
    const DSI *dsi;
    int fork = 0;
    int created = 0;
    int len;
    unsigned int ret;
    char temp[MAXPATHLEN];
    struct stat st;
    dsi = &Conn->dsi;
    ENTER_TEST

    /* FIXME: This test hangs with AFP2.x (3.1.12 - 4.0.3) */
    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        return;
    }

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    created = 1;
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_nottested();
        goto test_exit;
    }

    if (FPWrite(Conn, fork, 0, (int)strlen(target), target, 0)) {
        test_nottested();
        goto test_exit;
    }

    if (FPCloseFork(Conn, fork)) {
        test_nottested();
        goto test_exit;
    }

    fork = 0;
    bitmap = (1 << DIRPBIT_ATTR) | (1 << FILPBIT_FINFO) |
             (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_MDATE) |
             (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << FILPBIT_FNUM);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_nottested();
        goto test_exit;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    memcpy(filedir.finder_info, "slnkrhap", 8);
    bitmap = (1 << FILPBIT_FINFO);
    ret = FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir);

    if (ret != htonl(AFPERR_ACCESS)) {
        test_failed();
    }

    len = snprintf(temp, sizeof(temp), "%s/%s", Path, name);

    if (len < 0 || len >= (int)sizeof(temp)) {
        test_failed();
        goto test_exit;
    }

    if (lstat(temp, &st) == 0) {
        if (S_ISLNK(st.st_mode)) {
            test_failed();
        }
    } else if (errno != ENOENT) {
        test_failed();
    }

test_exit:

    if (fork) {
        FPCloseFork(Conn, fork);
    }

    if (created) {
        ret = FPDelete(Conn, vol, DIRDID_ROOT, name);

        if (ret && ret != htonl(AFPERR_NOOBJ)) {
            test_failed();
        }
    }

    exit_test("FPSetFileParms:test426: Reject a dangling symlink");
}

/* ----------- */
void T2FPSetFileParms_test()
{
    ENTER_TESTSET
    test89();
    test120();
    test426();
}
