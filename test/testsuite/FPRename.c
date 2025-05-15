/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test69() {
    int  dir;
    char *name = "t69 rename file!name";
    char *name2 = "t69 new name";
    uint16_t vol = VolID;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPRename(Conn, vol, DIRDID_ROOT, name, name2))
    FAIL(!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name)))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name2))
    FAIL(!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name2)))
    FAIL(htonl(AFPERR_EXIST) != FPRename(Conn, vol, DIRDID_ROOT, name, name2))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name2))
    FAIL(FPRename(Conn, vol, DIRDID_ROOT, name, name2))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name2))
    /* other sens */
    FAIL(!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name2)))
    FAIL(FPRename(Conn, vol, DIRDID_ROOT, name2, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPRename:test69: rename a folder with Unix name != Mac name");
}

/* ------------------------- */
STATIC void test72() {
    int dir = 0;
    int dir1 = 0;
    uint16_t bitmap = 0;
    char *name  = "t72 check rename input";
    char *ndel = "t72 no delete dir";
    char *name1 = "t72 new folder";
    char *name2 = "t72 dir";
    unsigned int ret;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name2))) {
        test_nottested();
        goto test_exit;
    }

    if (!(dir1 = FPCreateDir(Conn, vol, DIRDID_ROOT, ndel))) {
        test_nottested();
        goto fin;
    }

    FAIL(ntohl(AFPERR_NORENAME) != FPRename(Conn, vol, DIRDID_ROOT, "", "volume"))
    bitmap = (1 << DIRPBIT_ATTR);

    if (FPGetFileDirParams(Conn, vol, dir1, "", 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        filedir.attr = ATTRBIT_NORENAME | ATTRBIT_SETCLR ;
        FAIL(FPSetDirParms(Conn, vol, dir1, "", bitmap, &filedir))
        ret = FPRename(Conn, vol, DIRDID_ROOT, ndel, "volume");

        if (ntohl(AFPERR_OLOCK) != ret) {
            test_failed();

            if (!ret) {
                FPRename(Conn, vol, DIRDID_ROOT, "volume", ndel);
            }
        }

        filedir.attr = ATTRBIT_NODELETE;
        FAIL(FPSetDirParms(Conn, vol, dir1, "", bitmap, &filedir))
    }

    ret = FPRename(Conn, vol, DIRDID_ROOT, name2, name2);

    if (not_valid(ret, /* MAC */0, AFPERR_EXIST)) {
        test_failed();
    }

    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(FPRename(Conn, vol, DIRDID_ROOT, name, name))
    FAIL(FPRename(Conn, vol, DIRDID_ROOT, name, name1))
    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, DIRDID_ROOT, name))
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, ndel))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name2))
test_exit:
    exit_test("FPRename:test72: check input parameter");
}

/* -------------------------- */
static int create_double_deleted_folder(uint16_t vol, char *name) {
    uint16_t vol2;
    int tdir;
    int tdir1 = 0;
    DSI *dsi2;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    DSI *dsi = &Conn->dsi;
    uint16_t bitmap;
    int tp, tp1;
    tdir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name);

    if (!tdir) {
        test_nottested();
        return 0;
    }

    tdir1  = FPCreateDir(Conn, vol, tdir, name);

    if (!tdir1) {
        test_nottested();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, tdir1, "", 0, (1 << DIRPBIT_OFFCNT))) {
        test_nottested();
        goto fin;
    }

    bitmap = (1 << DIRPBIT_ACCESS);

    if (FPGetFileDirParams(Conn, vol, tdir, "", 0, bitmap)) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    filedir.access[0] = 0;
    filedir.access[1] = 7;
    filedir.access[2] = 7;
    filedir.access[3] = 7;

    if (FPSetDirParms(Conn, vol, tdir, "", bitmap, &filedir)) {
        test_nottested();
        goto fin;
    }

    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto fin;
    }

    bitmap = (1 << DIRPBIT_ACCESS) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID);

    if (FPGetFileDirParams(Conn2, vol2,  DIRDID_ROOT, name, 0, bitmap)) {
        test_nottested();
        goto fin;
    }

    tp = get_did(Conn2, vol2, DIRDID_ROOT, name);

    if (!tp) {
        test_nottested();
        goto fin;
    }

    if (tp != tdir && !Quiet) {
        fprintf(stdout, "Warning DID connection1 0x%x ==> connection2 0x%x\n", tdir,
                tp);
    }

    tp1 = get_did(Conn2, vol2, tp, name);

    if (!tp1) {
        test_nottested();
        goto fin;
    }

    if (tp1 != tdir1 && !Quiet) {
        fprintf(stdout, "Warning DID connection1 0x%x ==> connection2 0x%x\n", tdir1,
                tp1);
    }

    if (FPDelete(Conn2, vol2,  tp1, "")) {
        test_nottested();
        FPDelete(Conn, vol, tdir1, "");
        tdir1 = 0;
    }

    if (FPDelete(Conn2, vol2,  tp, "")) {
        test_nottested();
        FPDelete(Conn, vol, tdir, "");
        tdir1 = 0;
    }

    FPCloseVol(Conn2, vol2);
    return tdir1;
fin:
    FAIL(tdir && FPDelete(Conn, vol, tdir, ""))
    FAIL(tdir1 && FPDelete(Conn, vol, tdir1, ""))
    return 0;
}

/* ----------- */
STATIC void test183() {
    char *tname = "test183";
    char *name1 = "test183.new";
    int tdir;
    uint16_t vol = VolID;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    /* ---- directory.c ---- */
    if (!(tdir = create_double_deleted_folder(vol, tname))) {
        goto test_exit;
    }

    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, tdir, ""))

    /* ---- filedir.c ------------ */
    if (!(tdir = create_double_deleted_folder(vol, tname))) {
        goto test_exit;
    }

    FAIL(ntohl(AFPERR_NOOBJ) != FPRename(Conn, vol, tdir, "", name1))
test_exit:
    exit_test("FPRename:test183: did error two users in  folder did=<deleted> name=test183");
}

/* ------------------------- */
STATIC void test184() {
    char *name  = "t184.txt";
    char *name1 = "t184new.txt";
    uint16_t vol = VolID;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPRename(Conn, vol, DIRDID_ROOT, name, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
    FAIL(!FPDelete(Conn, vol, DIRDID_ROOT, name))
    FPFlush(Conn, vol);
test_exit:
    exit_test("FPRename:test184: rename");
}

/* ------------------------- */
STATIC void test191() {
    char *name = "t191 dir";
    char *name1 = "t191 subdir";
    char *dest = "t191 newname";
    uint16_t vol = VolID;
    int  dir = 0, dir1 = 0, dir2 = 0;
    ENTER_TEST
    dir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    dir1  = FPCreateDir(Conn, vol, DIRDID_ROOT, name1);

    if (!dir1) {
        test_nottested();
        goto fin;
    }

    dir2  = FPCreateDir(Conn, vol, dir1, name1);

    if (!dir2) {
        test_nottested();
        goto fin;
    }

    if (FPRename(Conn, vol, DIRDID_ROOT, name, dest)) {
        test_failed();
    }

fin:
    FAIL(dir2 && FPDelete(Conn, vol, dir2, ""))
    FAIL(dir1 && FPDelete(Conn, vol, dir1, ""))
    FAIL(dir && FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPRename:test191: rename folders");
}

/* ------------------------- */
STATIC void test219() {
    char *name = "t191 dir";
    char *dest = "t191 newname";
    uint16_t vol = VolID;
    uint16_t vol2, bitmap;
    int  dir = 0;
    DSI	*dsi2 = &Conn2->dsi;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    dir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto fin;
    }

    bitmap = (1 << FILPBIT_LNAME);
    FAIL(FPEnumerate(Conn, vol, DIRDID_ROOT, "", bitmap, bitmap))
    FAIL(FPEnumerate(Conn2, vol2,  DIRDID_ROOT, "", bitmap, bitmap))

    if (FPRename(Conn2, vol2, DIRDID_ROOT, name, dest)) {
        test_failed();
    }

    FAIL(FPEnumerate(Conn, vol, DIRDID_ROOT, "", bitmap, bitmap))
    FAIL(FPCloseVol(Conn2, vol2))
fin:
    FAIL(dir && FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPRename:test219: rename folders two users");
}

/* ------------------------- */
STATIC void test376() {
    char *name = "t376 name";
    char *name1 = "t376 new name";
    uint16_t vol = VolID;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_failed();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1)) {
        test_failed();
        goto fin;
    }

    FAIL(htonl(AFPERR_EXIST) != FPRename(Conn, vol, DIRDID_ROOT, name, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
fin:
    FPDelete(Conn, vol, DIRDID_ROOT, name);
test_exit:
    exit_test("FPRename:test376: dest file exist");
}


/* -------------------------
*/
STATIC void test377() {
    char *name =  "t377 name";
    char *name1 = "t377 Name";
    uint16_t vol = VolID;
    int ret;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_failed();
        goto test_exit;
    }

    if ((ret = FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1))
            && ret != htonl(AFPERR_EXIST)) {
        test_failed();
        goto fin;
    }

    if (ret == htonl(AFPERR_EXIST)) {
        FAIL(FPRename(Conn, vol, DIRDID_ROOT, name, name1))
    } else {
        FAIL(htonl(AFPERR_EXIST) != FPRename(Conn, vol, DIRDID_ROOT, name, name1))
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
fin:
    FPDelete(Conn, vol, DIRDID_ROOT, name);
test_exit:
    exit_test("FPRename:test377: dest file exist but diff only by case, is this one OK");
}


/* ----------- */
void FPRename_test() {
    ENTER_TESTSET
    test69();
    test72();
    test183();
    test184();
    test191();
    test219();
    test376();
    test377();
}
