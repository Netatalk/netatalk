/* ----------------------------------------------
*/
#include "specs.h"
#include "adoublehelper.h"

/* ------------------------ */
static int afp_symlink(char *oldpath, char *newpath)
{
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap;
    uint16_t vol = VolID;
    DSI *dsi;
    int fork = 0;
    dsi = &Conn->dsi;

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, newpath)) {
        return -1;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, newpath,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        return -1;
    }

    if (FPWrite(Conn, fork, 0, strlen(oldpath), oldpath, 0)) {
        return -1;
    }

    if (FPCloseFork(Conn, fork)) {
        return -1;
    }

    fork = 0;
    bitmap = (1 << DIRPBIT_ATTR) | (1 << FILPBIT_FINFO) |
             (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_MDATE) |
             (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << FILPBIT_FNUM);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, newpath, bitmap, 0)) {
        return -1;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    memcpy(filedir.finder_info, "slnkrhap", 8);
    bitmap = (1 << FILPBIT_FINFO);

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT, newpath, bitmap, &filedir)) {
        return -1;
    }

    return 0;
}

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
    DSI *dsi = &Conn->dsi;
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
    DSI *dsi = &Conn->dsi;
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
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap;
    uint16_t vol = VolID;
    DSI *dsi;
    int fork = 0;
    unsigned int ret;
    char temp[MAXPATHLEN];
    struct stat st;
    dsi = &Conn->dsi;
    ENTER_TEST

    // FIXME: This test hangs with AFP2.x (3.1.12 - 4.0.3)
    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        return;
    }

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (afp_symlink("t426 dest", name)) {
        test_nottested();
        goto test_exit;
    }

    /* Check if volume uses option 'followsymlinks' */
    sprintf(temp, "%s/%s", Path, name);

    if (lstat(temp, &st)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED stat( %s ) %s\n", temp, strerror(errno));
        }

        test_failed();
    }

    if (!S_ISLNK(st.st_mode)) {
        test_skipped(T_NOSYML);
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
    } else {
        char *ln2 = "t426 dest 2";
        ret = FPWrite_ext(Conn, fork, 0, strlen(ln2), ln2, 0);

        if (not_valid_bitmap(ret, BITERR_ACCESS | BITERR_MISC, AFPERR_MISC)) {
            test_failed();
        }

        FPCloseFork(Conn, fork);
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name, OPENACC_RD);

    if (!fork) {
        /* Trying to open the linked file? */
        test_failed();
    }

    if (fork) {
        FPCloseFork(Conn, fork);
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetFileParms:test426: Create a dangling symlink");
}

/* ----------- */
void T2FPSetFileParms_test()
{
    ENTER_TESTSET
    test89();
    test120();
    test426();
}
