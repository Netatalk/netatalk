#include "specs.h"

/* -------------------------------- */
static int really_ro() {
    int dir;
    char *ndir = "read only dir";

    if ((dir = FPCreateDir(Conn, VolID, DIRDID_ROOT, ndir))) {
        test_nottested();
        FAIL(FPDelete(Conn, VolID,  dir, ""))
        return 0;
    }

    return 1;
}

/* -------------------------------- */
static void check_test(unsigned int err) {
    if (err != ntohl(AFPERR_VLOCK) && err != ntohl(AFPERR_ACCESS)) {
        test_failed();
    }
}

/* ----------------- */
STATIC void test510() {
    char *ndir = "read only dir";
    char *nfile = "read only file";
    int  ofs =  4 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    char *file = NULL;
    char *file1 = NULL;
    char *dir = NULL;
    int  fid;
    int  did;
    uint16_t bitmap;
    uint16_t fork, fork1;
    struct afp_volume_parms parms;
    DSI *dsi = &Conn->dsi;
    uint32_t flen;
    unsigned int ret;
    ENTER_TEST

    if (!Test) {
        if (!Quiet) {
            fprintf(stdout,
                    "Has to be tested with a read only volume, two files and one dir.\n");
        }

        test_skipped(T_SINGLE);
        goto test_exit;
    }

    VolID = FPOpenVol(Conn, Vol);

    if (VolID == 0xffff) {
        test_nottested();
        goto test_exit;
    }

    if (!really_ro()) {
        goto test_exit;
    }

    /* get a directory */
    bitmap = (1 << DIRPBIT_LNAME);
    ret = FPEnumerateFull(Conn, VolID, 1, 1, 800,  DIRDID_ROOT, "", 0, bitmap);

    if (ret) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    dir = strdup(filedir.lname);

    if (!dir) {
        test_nottested();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout, "Directory %s\n", dir);
    }

    /* get a file */
    bitmap = (1 << FILPBIT_LNAME);
    ret = FPEnumerateFull(Conn, VolID, 1, 1, 800,  DIRDID_ROOT, "", bitmap, 0);

    if (ret) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    file = strdup(filedir.lname);

    if (!file) {
        test_nottested();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout, "File %s\n", file);
    }

    /* get a second file */
    bitmap = (1 << FILPBIT_LNAME);
    ret = FPEnumerateFull(Conn, VolID, 2, 1, 800,  DIRDID_ROOT, "", bitmap, 0);

    if (ret) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    file1 = strdup(filedir.lname);

    if (!file1) {
        test_nottested();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout, "Second file %s\n", file1);
    }

    /* TESTS */
    /* -- file.c -- */
    ret = FPCreateFile(Conn, VolID,  0, DIRDID_ROOT, nfile);
    FAIL(ntohl(AFPERR_VLOCK) != ret)

    if (!ret) {
        FAIL(FPDelete(Conn, VolID, DIRDID_ROOT, nfile))
    }

    bitmap = (1 << FILPBIT_FINFO) | (1 << FILPBIT_CDATE) | (1 << FILPBIT_BDATE) |
             (1 << FILPBIT_MDATE);

    if (FPGetFileDirParams(Conn, VolID,  DIRDID_ROOT, file, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + 3 * sizeof(uint16_t), bitmap, 0);
        ret  = FPSetFileParams(Conn, VolID,  DIRDID_ROOT, file, bitmap, &filedir);
        check_test(ret);
    }

    FAIL(ntohl(AFPERR_VLOCK) != FPCopyFile(Conn, VolID, DIRDID_ROOT, VolID,
                                           DIRDID_ROOT, file, "", nfile))

    if (!(get_vol_attrib(VolID) & VOLPBIT_ATTR_FILEID) && !Quiet) {
        fprintf(stdout, "FileID calls Not supported\n");
    } else {
        /* with cnid db, if the volume isn't defined as read only in afp.conf
         * CreateID, DeleteID don't return VLOCK
        */
        ret = FPCreateID(Conn, VolID, DIRDID_ROOT, file);

        if (not_valid(ret, /* MAC */AFPERR_VLOCK, AFPERR_EXISTID)) {
            test_failed();
        }

        fid = get_fid(Conn, VolID, DIRDID_ROOT, file);

        if (fid) {
            ret = FPDeleteID(Conn, VolID, filedir.did);

            if (not_valid(ret, /* MAC */AFPERR_VLOCK, AFPERR_NOID)) {
                test_failed();
            }
        }
    }

    ret = FPExchangeFile(Conn, VolID, DIRDID_ROOT, DIRDID_ROOT, file, file1);

    if (not_valid(ret, /* MAC */AFPERR_VLOCK, AFPERR_ACCESS)) {
        test_failed();
    }

    /* -- volume.c -- */
    bitmap = (1 << VOLPBIT_MDATE);
    FAIL(FPGetVolParam(Conn, VolID, bitmap))
    afp_volume_unpack(&parms, dsi->commands + sizeof(uint16_t), bitmap);
    bitmap = (1 << VOLPBIT_BDATE);
    parms.bdate = parms.mdate;
    ret = FPSetVolParam(Conn, VolID, bitmap, &parms);
    check_test(ret);
    /* -- filedir.c -- */
    bitmap = (1 << FILPBIT_FINFO) | (1 << FILPBIT_CDATE) | (1 << FILPBIT_BDATE) |
             (1 << FILPBIT_MDATE);

    if (FPGetFileDirParams(Conn, VolID,  DIRDID_ROOT, file, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + 3 * sizeof(uint16_t), bitmap, 0);
        ret = FPSetFilDirParam(Conn, VolID,  DIRDID_ROOT, file, bitmap, &filedir);
        check_test(ret);
    }

    bitmap = (1 << DIRPBIT_FINFO) | (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) |
             (1 << DIRPBIT_MDATE);

    if (FPGetFileDirParams(Conn, VolID,  DIRDID_ROOT, dir, 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + 3 * sizeof(uint16_t), 0, bitmap);
        ret = FPSetFilDirParam(Conn, VolID,  DIRDID_ROOT, dir, bitmap, &filedir);
        check_test(ret);
    }

    FAIL(htonl(AFPERR_VLOCK) != FPRename(Conn, VolID, DIRDID_ROOT, file, nfile))
    FAIL(htonl(AFPERR_VLOCK) != FPRename(Conn, VolID, DIRDID_ROOT, dir, ndir))
    bitmap = (1 << DIRPBIT_OFFCNT);

    if (FPGetFileDirParams(Conn, VolID,  DIRDID_ROOT, dir, 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + 3 * sizeof(uint16_t), 0, bitmap);
        ret = FPDelete(Conn, VolID, DIRDID_ROOT, dir);

        if (ret != htonl(AFPERR_VLOCK)) {
            if (!filedir.offcnt) {
                test_failed();
            } else if (!Quiet) {
                fprintf(stdout, "\tWARNING \"%s\", not empty FPDelete skipped\n", dir);
            }
        }
    }

    FAIL(htonl(AFPERR_VLOCK) != FPDelete(Conn, VolID, DIRDID_ROOT, file))
    FAIL(htonl(AFPERR_VLOCK) != FPMoveAndRename(Conn, VolID, DIRDID_ROOT,
            DIRDID_ROOT, file, nfile))
    FAIL(htonl(AFPERR_VLOCK) != FPMoveAndRename(Conn, VolID, DIRDID_ROOT,
            DIRDID_ROOT, dir, ndir))
    /* -- directory.c -- */
    bitmap = (1 << DIRPBIT_FINFO) | (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) |
             (1 << DIRPBIT_MDATE);

    if (FPGetFileDirParams(Conn, VolID,  DIRDID_ROOT, dir, 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + 3 * sizeof(uint16_t), 0, bitmap);
        ret = FPSetDirParms(Conn, VolID,  DIRDID_ROOT, dir, bitmap, &filedir);
        check_test(ret);
    }

    if ((did = FPCreateDir(Conn, VolID, DIRDID_ROOT, ndir))) {
        test_nottested();
        FAIL(FPDelete(Conn, VolID,  did, ""))
        goto test_exit;
    }

    /* -- fork.c -- */
    bitmap = 0;
    fork = FPOpenFork(Conn, VolID, OPENFORK_DATA, bitmap, DIRDID_ROOT, file,
                      OPENACC_RD);

    if (!fork) {
        test_failed();
    }

    fork1 = FPOpenFork(Conn, VolID, OPENFORK_DATA, bitmap, DIRDID_ROOT, file,
                       OPENACC_WR);

    if (fork1) {
        test_failed();
        FAIL(FPCloseFork(Conn, fork1))
    }

    FAIL(FPCloseFork(Conn, fork))
    fork1 = FPOpenFork(Conn, VolID, OPENFORK_DATA, bitmap, DIRDID_ROOT, file,
                       OPENACC_WR);

    if (fork1) {
        test_failed();
        FAIL(FPCloseFork(Conn, fork1))
    }

    /* --------- */
    fork = FPOpenFork(Conn, VolID, OPENFORK_DATA, bitmap, DIRDID_ROOT, file,
                      OPENACC_RD);

    if (!fork) {
        test_failed();
    }

    bitmap = (1 << FILPBIT_DFLEN);
    ret = FPSetForkParam(Conn, fork, bitmap, 0);
    check_test(ret);

    if (FPGetForkParam(Conn, fork, bitmap)) {
        test_failed();
    } else if ((flen = get_forklen(dsi, OPENFORK_DATA))) {
        FAIL(FPRead(Conn, fork, 0, min(100, flen), Data))
    }

    FAIL(FPCloseFork(Conn, fork))
    /* ------------- */
    bitmap = 0;
    fork = FPOpenFork(Conn, VolID, OPENFORK_RSCS, bitmap, DIRDID_ROOT, file,
                      OPENACC_RD);

    if (!fork) {
        test_failed();
    }

    fork1 = FPOpenFork(Conn, VolID, OPENFORK_RSCS, bitmap, DIRDID_ROOT, file,
                       OPENACC_WR);

    if (fork1) {
        test_failed();
        FAIL(FPCloseFork(Conn, fork1))
    }

    FAIL(FPCloseFork(Conn, fork))
    fork1 = FPOpenFork(Conn, VolID, OPENFORK_RSCS, bitmap, DIRDID_ROOT, file,
                       OPENACC_WR);

    if (fork1) {
        test_failed();
        FAIL(FPCloseFork(Conn, fork1))
    }

    /* --------- */
    fork = FPOpenFork(Conn, VolID, OPENFORK_RSCS, bitmap, DIRDID_ROOT, file,
                      OPENACC_RD);

    if (!fork) {
        test_failed();
    }

    ret = FPSetForkParam(Conn, fork, (1 << FILPBIT_RFLEN), 100);
    check_test(ret);
fin:
    free(dir);
    free(file);
    free(file1);
    FAIL(FPCloseVol(Conn, VolID))
test_exit:
    exit_test("Readonly:test510: Access files and directories on a read only volume");
}

void Readonly_test() {
    ENTER_TESTSET
    test510();
}
