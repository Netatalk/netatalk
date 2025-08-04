/* ----------------------------------------------
*/
#include "specs.h"

/* --------------------------
FIXME
*/
STATIC void test14()
{
    uint16_t bitmap = 0;
    int fork;
    uint16_t vol = VolID;
    char *name = "t14 file";
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (!fork) {
        test_nottested();
        goto fin;
    }

    bitmap = (1 <<  FILPBIT_ATTR);

    if (FPGetForkParam(Conn, fork, bitmap)) {
        test_nottested();
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
    }

    FAIL(FPCloseFork(Conn, fork))

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
    }

fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test14: get data fork open attribute same connection");
}

/* --------------------------
 FIXME need to check open attrib
*/

STATIC void test15()
{
    uint16_t bitmap = 0;
    uint16_t vol = VolID;
    int fork;
    char *name = "t15 file";
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (!fork) {
        test_nottested();
        goto fin;
    }

    bitmap = 1;

    if (FPGetForkParam(Conn, fork, bitmap)) {
        test_nottested();
    }

    FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0);
    FPCloseFork(Conn, fork);
    FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0);
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test15: get resource fork open attribute");
}

/* -------------------------- */
STATIC void test16()
{
    uint16_t bitmap = 0;
    uint16_t vol = VolID;
    int fork = 0;
    int fork2 = 0;
    char *name = "t16 file";
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR |  OPENACC_DWR);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fork2 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                       OPENACC_RD);

    if (!fork2) {
        test_failed();
        goto fin;
    }

    bitmap = 1;
    FAIL(FPGetForkParam(Conn, fork, bitmap))
    FAIL(FPCloseFork(Conn, fork))
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR |  OPENACC_DWR);
fin:
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(fork2 && FPCloseFork(Conn, fork2))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test16: open deny mode/ fork attrib");
}

/* -------------------------- */
STATIC void test17()
{
    uint16_t bitmap = 0;
    uint16_t vol = VolID;
    int fork = 0;
    int fork2 = 0;
    int fork3 = 0;
    char *name = "t17 file";
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name, 0);

    if (!fork) {
        test_failed();
    }

    FAIL(fork && FPCloseFork(Conn, fork))
    /* -------------- */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR);

    if (!fork) {
        test_failed();
    }

    fork3 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                       OPENACC_RD | OPENACC_WR |  OPENACC_DWR);

    if (fork3) {
        test_failed();
    }

    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(fork3 && FPCloseFork(Conn, fork3))
    fork3 = 0;
    /* -------------- */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR |  OPENACC_DWR);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fork2 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                       OPENACC_RD);

    if (!fork2) {
        test_failed();
        goto fin;
    }

    bitmap = 1;
    FAIL(FPGetForkParam(Conn, fork, bitmap))
    /* fail */
    fork3 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                       OPENACC_RD | OPENACC_WR |  OPENACC_DWR);

    if (fork3) {
        test_failed();
    }

fin:
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(fork2 && FPCloseFork(Conn, fork2))
    FAIL(fork3 && FPCloseFork(Conn, fork3))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test17: open deny mode/ fork attrib");
}

/* -------------------------- */
STATIC void test18()
{
    uint16_t bitmap = 0;
    uint16_t vol = VolID;
    int fork;
    int fork2 = 0;
    int fork3 = 0;
    char *name = "t18 file";
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR |  OPENACC_DWR);

    if (!fork) {
        test_failed();
        goto fin;
    }

    /* success */
    fork2 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                       OPENACC_RD);
    bitmap = 1;

    if (!fork2) {
        test_failed();
        goto fin;
    }

    FAIL(FPGetForkParam(Conn, fork, bitmap))
    FAIL(FPCloseFork(Conn, fork))
    /* success */
    fork3 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                       OPENACC_RD | OPENACC_WR |  OPENACC_DWR);

    if (!fork3) {
        test_failed();
    }

fin:
    FAIL(fork2 && FPCloseFork(Conn, fork2))
    FAIL(fork3 && FPCloseFork(Conn, fork3))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test18: open deny mode/ fork attrib");
}

/* -------------------------- */
STATIC void test19()
{
    uint16_t bitmap = 0;
    uint16_t vol = VolID;
    int fork;
    int fork2 = 0;
    int fork3 = 0;
    char *name = "t19 file";
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    /* success */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_DWR);

    if (!fork) {
        test_failed();
        goto fin;
    }

    /* success */
    fork2 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                       OPENACC_RD | OPENACC_DWR);

    if (!fork2) {
        test_failed();
        goto fin;
    }

    /* fail */
    fork3 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                       OPENACC_WR);

    if (fork3) {
        test_failed();
        goto fin;
    }

    bitmap = 1;

    /* success */
    if (FPGetForkParam(Conn, fork, bitmap)) {
        test_failed();
        goto fin;
    }

    /* success */
    if (FPCloseFork(Conn, fork)) {
        fork = 0;
        test_failed();
        goto fin;
    }

    fork = 0;
    /* fail */
    fork3 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                       OPENACC_WR);

    if (fork3) {
        test_failed();
    }

fin:
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(fork2 && FPCloseFork(Conn, fork2))
    FAIL(fork3 && FPCloseFork(Conn, fork3))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test19: open deny mode/ fork attrib");
}

/* ------------------------- */
STATIC void test20()
{
    uint16_t bitmap = 0;
    uint16_t vol = VolID;
    int fork = 0;
    int fork2 = 0;
    char *name = "t20 file";
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fork2 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                       OPENACC_WR);

    if (!fork2) {
        test_failed();
        goto fin;
    }

    FAIL(FPWrite(Conn, fork2, 0, 10, Data, 0 /*0x80 */))
fin:
    FAIL(fork2 && FPCloseFork(Conn, fork2))
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test20: open file read only and read write");
}

/* ----------------------- */
STATIC void test39()
{
    uint16_t bitmap = 0;
    uint16_t vol = VolID;
    char *ndir = "t39 dir";
    int  dir;
    char *nf1  = "t39 file.txt";
    char *nf2  = "t39 file2";
    char *name = "t39 dir/t39 file2";
    char *name1 = "t39 dir//t39 file.txt";
    char *name2 = "t39 dir///t39 file.txt";
    int  fork;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, nf1)) {
        test_nottested();
        goto fin;
    }

    if (FPCreateFile(Conn, vol, 0, dir, nf2)) {
        test_nottested();
        goto fin;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name1,
                      OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name2,
                      OPENACC_RD);

    if (fork) {
        test_failed();
        FAIL(FPCloseFork(Conn, fork))
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, nf2))
    FAIL(FPDelete(Conn, vol, dir, ""))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, nf1))
test_exit:
    exit_test("FPOpenFork:test39: cname path folder + filename");
}

/* ------------------------- */
STATIC void test48()
{
    uint16_t bitmap = 0;
    int fork;
    char *ndir = "t48 dir";
    int  dir;
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, ndir,
                      OPENACC_RD);

    if (fork) {
        test_failed();
        FAIL(FPCloseFork(Conn, fork))
    } else if (ntohl(AFPERR_BADTYPE) != dsi->header.dsi_code) {
        test_failed();
    }

    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPOpenFork:test48: open fork a folder");
}

/* ---------------------------- */
static void test_denymode(char *name, int type)
{
    int fork;
    int fork1;
    int fork2;
    int fork3;
    uint16_t bitmap = 0;
    uint16_t vol = VolID;
    DSI *dsi;
    DSI *dsi2;
    dsi = &Conn->dsi;

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        return;
    }

    fork = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_DWR);

    if (!fork) {
        test_failed();
    }

    fork2 = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                       OPENACC_RD | OPENACC_DWR);

    if (!fork2) {
        test_failed();
    }

    fork3 = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                       OPENACC_RD | OPENACC_DWR);

    if (!fork3) {
        test_failed();
    }

    fork1 = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                       OPENACC_WR | OPENACC_RD);

    if (fork1 || dsi->header.dsi_code != ntohl(AFPERR_DENYCONF)) {
        test_failed();

        if (fork1) {
            FAIL(FPCloseFork(Conn, fork1))
        }
    }

    if (fork3) {
        FAIL(FPCloseFork(Conn, fork3))
    }

    if (Conn2) {
        uint16_t vol2;
        dsi2 = &Conn2->dsi;
        vol2  = FPOpenVol(Conn2, Vol);

        if (vol2 == 0xffff) {
            test_nottested();
        }

        fork1 = FPOpenFork(Conn2, vol2, type, bitmap, DIRDID_ROOT, name,
                           OPENACC_WR | OPENACC_RD);

        if (fork1 || dsi2->header.dsi_code != ntohl(AFPERR_DENYCONF)) {
            test_failed();

            if (fork1) {
                FPCloseFork(Conn2, fork1);
            }
        }

        fork1 = FPOpenFork(Conn2, vol2, type, bitmap, DIRDID_ROOT, name, OPENACC_RD);

        if (!fork1) {
            test_failed();
        } else {
            FAIL(FPCloseFork(Conn2, fork1))
        }

        fork1 = FPOpenFork(Conn2, vol2, type, bitmap, DIRDID_ROOT, name,
                           OPENACC_RD | OPENACC_DWR);

        if (!fork1) {
            test_failed();
        } else {
            FAIL(FPCloseFork(Conn2, fork1))
        }

        fork1 = FPOpenFork(Conn2, vol2, type, bitmap, DIRDID_ROOT, name,
                           OPENACC_WR | OPENACC_RD);

        if (fork1 || dsi2->header.dsi_code != ntohl(AFPERR_DENYCONF)) {
            test_failed();

            if (fork1) {
                FPCloseFork(Conn2, fork1);
            }
        }

        FAIL(FPCloseVol(Conn2, vol2))
    }

    FAIL(FPCloseFork(Conn, fork))

    if (fork2) {
        FPCloseFork(Conn, fork2);
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
}

/* -------------------------- */
STATIC void test81()
{
    char *name = "t81 Denymode RF 2users";
    ENTER_TEST

    if (!Quiet) {
        fprintf(stdout, "FPOpenFork:test81: Deny mode 2 users RF\n");
    }

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    test_denymode(name, OPENFORK_RSCS);

    if (!Quiet) {
        fprintf(stdout, "FPOpenFork:test81: Deny mode 2 users DF\n");
    }

    name = "t81 Denymode DF 2users";
    test_denymode(name, OPENFORK_DATA);
test_exit:
    exit_test("FPOpenFork:test81: Deny mode 2 users");
}

/* ------------------------- */
STATIC void test116()
{
    char *name = "t116 no write file";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_MDATE);
    int fork;
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_nottested();
        goto end;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    filedir.attr = ATTRBIT_NOWRITE | ATTRBIT_SETCLR ;
    bitmap = (1 << DIRPBIT_ATTR);

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir)) {
        test_failed();
        goto end;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (fork || dsi->header.dsi_code != ntohl(AFPERR_OLOCK)) {
        test_failed();
    }

    if (fork) {
        FPCloseFork(Conn, fork);
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (!fork) {
        test_failed();
    } else {
        FAIL(FPCloseFork(Conn, fork))
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0xffff, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (fork || dsi->header.dsi_code != ntohl(AFPERR_BITMAP)) {
        fprintf(stdout, "\tFAILED\n");
    }

    if (fork) {
        FAIL(FPCloseFork(Conn, fork))
    }

end:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test116: test file's no-write attribute");
}

/* -------------------------- */
STATIC void test145()
{
    uint16_t bitmap = 0;
    int fork;
    char *name = "t145 file.txt";
    uint16_t vol = VolID;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name, 0);

    if (!fork) {
        test_failed();
    }

    if (FPCloseFork(Conn, fork)) {
        test_failed();
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test145: open RF mode 0");
}

/* ------------------------- */
STATIC void test151()
{
    uint16_t bitmap = 0;
    char *name1  = "t151 file";
    int fork;
    int i;
    int maxf;
    int ok = 0;
    static int forkt[2049];
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!Test) {
        if (!Quiet) {
            fprintf(stdout, "Opens too many forks to be run with other tests.\n");
        }

        test_skipped(T_SINGLE);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1)) {
        test_nottested();
        goto test_exit;
    }

    maxf = 0;

    for (i = 0; i < 2048; i++) {
        forkt[i] = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name1,
                              OPENACC_RD | OPENACC_DWR);

        if (!forkt[i]) {
            maxf = i;
            ok = 1;

            if (not_valid_bitmap(dsi->header.dsi_code, BITERR_NFILE | BITERR_DENYCONF,
                                 AFPERR_NFILE)) {
                test_failed();
            }

            break;
        }
    }

    if (!ok) {
        test_nottested();
        maxf = 2048;
    }

    for (i = 0; i < maxf; i++) {
        if (forkt[i] && FPCloseFork(Conn, forkt[i])) {
            test_failed();
        }
    }

    if (!ok) {
        goto end;
    }

    for (i = 0; i < maxf; i++) {
        fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name1,
                          OPENACC_RD | OPENACC_WR);

        if (!fork) {
            test_failed();
            break;
        }

        if (fork && FPCloseFork(Conn, fork)) {
            test_failed();
            break;
        }
    }

end:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
test_exit:
    exit_test("FPOpenFork:test151: too many open files");
}

/* -------------------------- */
STATIC void test190()
{
    int fork = 0;
    int fork2 = 0;
    char *name = "t190 file";
    char *name1 = "t190 new name";
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR |  OPENACC_DWR);

    if (!fork) {
        test_nottested();
        goto fin;
    }

    if (FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1)) {
        test_failed();
        goto fin;
    }

    fork2 = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name1, OPENACC_WR);

    if (fork2) {
        test_failed();
        goto fin;
    }

    if (ntohl(AFPERR_DENYCONF) != dsi->header.dsi_code) {
        test_failed();
        goto fin;
    }

    fork2 = FPOpenFork(Conn, vol, OPENFORK_RSCS, 0, DIRDID_ROOT, name1, OPENACC_WR);

    if (!fork2) {
        test_failed();
        goto fin;
    }

fin:
    FAIL(fork2 && FPCloseFork(Conn, fork2))
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
test_exit:
    exit_test("FPOpenFork::test190: deny mode after file moved");
}


/* ---------------------------- */
static void test_openmode(char *name, int type)
{
    int fork;
    int fork1;
    uint16_t bitmap = (1 << FILPBIT_ATTR) | (1 << FILPBIT_FINFO);
    uint16_t vol = VolID;
    DSI *dsi;
    DSI *dsi2;
    uint16_t vol2;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    int what   = (type == OPENFORK_DATA) ? ATTRBIT_DOPEN : ATTRBIT_ROPEN;
    int nowhat = (type == OPENFORK_DATA) ? ATTRBIT_ROPEN : ATTRBIT_DOPEN;
    dsi = &Conn->dsi;
    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        return;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        return;
    }

    fork = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name, OPENACC_RD);

    if (!fork) {
        test_failed();
    }

    if (FPGetFileDirParams(Conn2,  vol2, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi2->data + ofs, bitmap, 0);

        if (!(filedir.attr & what)) {
            test_failed();
        }

        if (filedir.attr & nowhat) {
            test_failed();
        }
    }

    fork1 = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name, OPENACC_WR);

    if (!fork1) {
        test_failed();
    }

    if (FPGetFileDirParams(Conn2,  vol2, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi2->data + ofs, bitmap, 0);

        if (!(filedir.attr & what)) {
            test_failed();
        }

        if (filedir.attr & nowhat) {
            test_failed();
        }
    }

    FAIL(FPCloseFork(Conn, fork))

    if (FPGetFileDirParams(Conn2,  vol2, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi2->data + ofs, bitmap, 0);

        if (!(filedir.attr & what)) {
            test_failed();
        }

        if (filedir.attr & nowhat) {
            test_failed();
        }
    }

    FAIL(FPCloseFork(Conn, fork1))

    if (FPGetFileDirParams(Conn2,  vol2, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi2->data + ofs, bitmap, 0);

        if (filedir.attr & what) {
            test_failed();
        }

        if (filedir.attr & nowhat) {
            test_failed();
        }
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
}

/* -------------------------- */
STATIC void test341()
{
    char *name = "t341 Attrib open mode RF";
    ENTER_TEST

    if (!Quiet) {
        fprintf(stdout, "FPOpenFork:test341: Attrib open mode 2 users RF\n");
    }

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    test_openmode(name, OPENFORK_RSCS);

    if (!Quiet) {
        fprintf(stdout, "FPOpenFork:test341: Attrib open mode 2 users DF\n");
    }

    name = "t341 Attrib open mode RF";
    test_openmode(name, OPENFORK_DATA);
test_exit:
    exit_test("FPOpenFork:test341: Attrib open mode 2 users");
}

/* ---------------------------- */
static void test_denymode1(char *name, int type)
{
    int fork;
    int fork1;
    uint16_t bitmap = 0;
    uint16_t vol = VolID;
    DSI *dsi;
    DSI *dsi2;
    dsi = &Conn->dsi;

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        return;
    }

    fork = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR | OPENACC_DRD | OPENACC_DWR);

    if (!fork) {
        test_failed();
    }

    fork1 = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                       OPENACC_WR | OPENACC_RD);

    if (fork1 || dsi->header.dsi_code != ntohl(AFPERR_DENYCONF)) {
        test_failed();

        if (fork1) {
            FAIL(FPCloseFork(Conn, fork1))
        }
    }

    if (Conn2) {
        uint16_t vol2;
        dsi2 = &Conn2->dsi;
        vol2  = FPOpenVol(Conn2, Vol);

        if (vol2 == 0xffff) {
            test_nottested();
        }

        fork1 = FPOpenFork(Conn2, vol2, type, bitmap, DIRDID_ROOT, name,
                           OPENACC_RD | OPENACC_WR | OPENACC_DRD | OPENACC_DWR);

        if (fork1 || dsi2->header.dsi_code != ntohl(AFPERR_DENYCONF)) {
            test_failed();

            if (fork1) {
                FPCloseFork(Conn2, fork1);
            }
        }

        FAIL(FPCloseVol(Conn2, vol2))
    }

    FAIL(FPCloseFork(Conn, fork))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
}

/* -------------------------- */
STATIC void test367()
{
    char *name = "t367 Denymode RF 2users";
    ENTER_TEST

    if (!Quiet) {
        fprintf(stdout, "FPOpenFork:test367: Deny mode 2 users RF\n");
    }

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    test_denymode1(name, OPENFORK_RSCS);

    if (!Quiet) {
        fprintf(stdout, "FPOpenFork:test367: Deny mode 2 users DF\n");
    }

    name = "t367 Denymode DF 2users";
    test_denymode1(name, OPENFORK_DATA);
test_exit:
    exit_test("FPOpenFork:test367: Deny mode 2 users");
}

/* ----------- */
void FPOpenFork_test()
{
    ENTER_TESTSET
    test14();
    test15();
    test16();
    test17();
    test18();
    test19();
    test20();
    test39();
    test48();
    test81();
    test116();
    test145();
    test151();
    test190();
    test341();
    test367();
}
