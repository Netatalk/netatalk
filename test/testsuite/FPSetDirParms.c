/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test82() {
    int  dir;
    char *name = "t82 test dir";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) |
                      (1 << DIRPBIT_MDATE)
                      | (1 << DIRPBIT_ACCESS) | (1 << DIRPBIT_FINFO) | (1 << DIRPBIT_UID) |
                      (1 << DIRPBIT_GID) ;
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    filedir.access[0] = 0;

    if (FPSetDirParms(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir)) {
        test_failed();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    filedir.access[0] = 0;

    if (FPSetDirParms(Conn, vol, DIRDID_ROOT, "", bitmap, &filedir)) {
        test_failed();
        goto fin;
    }

fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetDirParms:test82: test set dir parameters");
}

/* ------------------------- */
STATIC void test84() {
    int  dir;
    char *name = "t84 no delete dir";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_FINFO) |
                      (1 << DIRPBIT_CDATE) |
                      (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) | (1 << DIRPBIT_UID) |
                      (1 << DIRPBIT_GID);
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
        FAIL(FPSetDirParms(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))

        if (ntohl(AFPERR_OLOCK) != FPDelete(Conn, vol, DIRDID_ROOT, name)) {
            test_failed();
            goto test_exit;
        }

        filedir.attr = ATTRBIT_NODELETE;
        FAIL(FPSetDirParms(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetDirParms:test84: test dir set no delete attribute");
}

/* ------------------------- */
STATIC void test88() {
    int  dir = 0;
    char *name = "t88 error setdirparams";
    char *name1 = "t88 error setdirparams file";
    char *name2 = "t88 error enoent file";
    char *ndir = "t88 no access";
    char *rodir = "t88 read only access";
    int  ofs =  3 * sizeof(uint16_t);
    int pdir = 0;
    int rdir = 0;
    struct afp_filedir_parms filedir;
    uint16_t bitmap = DIRPBIT_ATTR | (1 << DIRPBIT_FINFO) | (1 << DIRPBIT_CDATE) |
                      (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) | (1 << DIRPBIT_UID) |
                      (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS);
    uint16_t vol = VolID;
    DSI *dsi;
    unsigned int ret;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (!(pdir = no_access_folder(vol, DIRDID_ROOT, ndir))) {
        goto test_exit;
    }

    if (!(rdir = read_only_folder(vol, DIRDID_ROOT, rodir))) {
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, ndir, 0,
                           (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                           (1 << DIRPBIT_ACCESS))) {
        test_failed();
        goto fin;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_failed();
        goto fin;
    }

    if (FPCreateFile(Conn, vol, 0, dir, name1)) {
        test_failed();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        filedir.access[0] = 0;
        bitmap = (1 << DIRPBIT_ATTR);
        filedir.attr = ATTRBIT_INVISIBLE | ATTRBIT_SETCLR ;
        ret = FPSetDirParms(Conn, vol, DIRDID_ROOT, rodir, bitmap, &filedir);

        if (not_valid(ret, AFPERR_ACCESS, 0)) {
            test_failed();
        }

        bitmap = (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE);
        ret = FPSetDirParms(Conn, vol, DIRDID_ROOT, rodir, bitmap, &filedir);

        if (not_valid(ret, AFPERR_ACCESS, 0)) {
            test_failed();
        }

        bitmap = (1 << DIRPBIT_UID) | (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS);
        FAIL(ntohl(AFPERR_ACCESS) != FPSetDirParms(Conn, vol, DIRDID_ROOT, ndir, bitmap,
                &filedir))
        FAIL(ntohl(AFPERR_BADTYPE) != FPSetDirParms(Conn, vol, dir, name1, bitmap,
                &filedir))
        FAIL(ntohl(AFPERR_NOOBJ) != FPSetDirParms(Conn, vol, dir, name2, bitmap,
                &filedir))
    }

fin:
    delete_folder(vol, DIRDID_ROOT, ndir);

    if (rdir) {
        delete_folder(vol, DIRDID_ROOT, rodir);
    }

    FAIL(dir && FPDelete(Conn, vol, dir, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetDirParms:test88: test error setdirparam");
}

/* ------------------------- */
STATIC void test107() {
    int  dir;
    int  pdir;
    char *name = "t107 test dir";
    char *ndir = "t107 no access";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) |
                      (1 << DIRPBIT_MDATE)
                      | (1 << DIRPBIT_ACCESS) | (1 << DIRPBIT_FINFO) | (1 << DIRPBIT_UID) |
                      (1 << DIRPBIT_GID) ;
    uint16_t bitmap2 = (1 << DIRPBIT_ACCESS) | (1 << DIRPBIT_UID) |
                       (1 << DIRPBIT_GID) ;
    uint16_t vol2;
    int uid;
    int ret;
    uint16_t vol = VolID;
    DSI *dsi;
    DSI *dsi2;
    dsi = &Conn->dsi;
    ENTER_TEST

    // Not supported with the mysql backend
    if (Exclude) {
        test_skipped(T_EXCLUDE);
        goto test_exit;
    }

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (!(pdir = no_access_folder(vol, DIRDID_ROOT, ndir))) {
        goto test_exit;
    }

    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn2, vol2, DIRDID_ROOT, name))) {
        test_nottested();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi2->data + ofs, 0, bitmap);
    filedir.access[0] = 0;
    uid = filedir.uid;

    if (FPGetFileDirParams(Conn2, vol2,  DIRDID_ROOT, name, 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi2->data + ofs, 0, bitmap);
        filedir.access[0] = 0;
        filedir.uid = uid;
        /* change owner */
        FAIL(FPSetDirParms(Conn2, vol2, DIRDID_ROOT, name, bitmap2, &filedir))
    }

    if (FPGetFileDirParams(Conn2, vol2,  DIRDID_ROOT, ndir, 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi2->data + ofs, 0, bitmap);
        filedir.access[0] = 0;
        filedir.uid = uid;
        FAIL(FPSetDirParms(Conn2, vol2, DIRDID_ROOT, ndir, bitmap2, &filedir))
    }

    if (FPGetFileDirParams(Conn2, vol2,  DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi2->data + ofs, 0, bitmap);
        filedir.access[0] = 0;
        filedir.uid = uid;
        ret = FPSetDirParms(Conn2, vol2, DIRDID_ROOT, "", bitmap2, &filedir);

        if (not_valid(ret, /* MAC */AFPERR_ACCESS, 0)) {
            test_failed();
        }

        filedir.access[0] = 0;
        filedir.access[1] = 7;
        filedir.access[2] = 7;
        filedir.access[3] = 7;
        ret = FPSetDirParms(Conn2, vol2, DIRDID_ROOT, "", bitmap2, &filedir);

        if (not_valid(ret, /* MAC */AFPERR_ACCESS, 0)) {
            test_failed();
        }
    }

    FAIL(FPDelete(Conn2, vol2,  DIRDID_ROOT, name))
    FAIL(FPCloseVol(Conn2, vol2))
fin:
    delete_folder(vol, DIRDID_ROOT, ndir);
test_exit:
    exit_test("FPSetDirParms:test107: test dir");
}


/* ------------------------- */
STATIC void test189() {
    int  dir;
    char *name = "t189 error setdirparams";
    char *name1 = "t189 error setdirparams file";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_FINFO) | (1 << DIRPBIT_CDATE) |
                      (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) | (1 << DIRPBIT_UID) |
                      (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS);
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_nottested();
        goto fin;
    }

    if (FPCreateFile(Conn, vol, 0, dir, name1)) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    filedir.access[0] = 0;
    FAIL(ntohl(AFPERR_BADTYPE) != FPSetDirParms(Conn, vol, dir, name1, bitmap,
            &filedir))
    FAIL(FPDelete(Conn, vol, dir, name1))
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetDirParms:test189: test error setdirparam");
}

/* ------------------------- */
STATIC void test193() {
    int  dir;
    char *name = "t193 no delete dir";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_ACCESS);
    uint16_t vol = VolID;
    DSI *dsi;
    int ret;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        ret = FPSetDirParms(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir);

        if (not_valid(ret, /* MAC */AFPERR_PARAM, 0)) {
            test_failed();
        }

        filedir.access[0] = 0;
        FAIL(FPSetDirParms(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetDirParms:test193: user permission set");
}

/* ------------------------- */
STATIC void test351() {
    int  dir = 0;
    char *ndir = "t351 dir";
    char *name = "t351 file";
    uint16_t vol = VolID;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = 0;
    uint8_t old_access[4];
    int ret;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_nottested();
        goto test_exit;
    }

    bitmap = (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) | (1 << DIRPBIT_ACCESS);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    memset(&filedir, 0, sizeof(filedir));
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs,  0, bitmap);
    bitmap = (1 << DIRPBIT_ACCESS);
    filedir.access[0] = 0;
    memcpy(old_access, filedir.access, sizeof(old_access));
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
    ret = FPSetDirParms(Conn, vol, DIRDID_ROOT, "", bitmap, &filedir);

    if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
        test_failed();
    }

    if (!ret) {
        memcpy(filedir.access, old_access, sizeof(old_access));
        FAIL(FPSetDirParms(Conn, vol, DIRDID_ROOT, "", bitmap, &filedir))
    }

    filedir.access[3] = 3; /* only owner */
    FAIL(FPSetDirParms(Conn, vol, DIRDID_ROOT, "", bitmap, &filedir))

    if (!FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_failed();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    }

    memcpy(filedir.access, old_access, sizeof(old_access));
    FAIL(FPSetDirParms(Conn, vol, DIRDID_ROOT, "", bitmap, &filedir))

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_failed();
    } else {
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPSetDirParms:test351: change root folder unix perm");
}

/* ------------------------- */
STATIC void test352() {
    int  dir = 0;
    char *name = "t352 file";
    char *ndir = "t352 dir";
    uint16_t vol = VolID;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = 0;
    uint8_t old_access[4];
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_nottested();
        goto test_exit;
    }

    bitmap = (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) | (1 << DIRPBIT_ACCESS);

    if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    bitmap = (1 << DIRPBIT_ACCESS);
    filedir.unix_priv = 0;
    filedir.access[0] = 0;
    memcpy(old_access, filedir.access, sizeof(old_access));
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
    FAIL(FPSetDirParms(Conn, vol, dir, "", bitmap, &filedir))

    if (!FPCreateFile(Conn, vol, 0, dir, name)) {
        test_failed();
        FAIL(FPDelete(Conn, vol, dir, name))
    }

    /* double check with didn't screw the parent !*/
    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_failed();
    } else {
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    }

    memcpy(filedir.access, old_access, sizeof(old_access));
    FAIL(FPSetDirParms(Conn, vol, dir, "", bitmap, &filedir))

    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_failed();
    } else {
        FAIL(FPDelete(Conn, vol, dir, name))
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPSetDirParms:test352: Change Mac perm to no perm in root folder");
}

/* ------------------------- */
STATIC void test353() {
    int  dir = 0;
    char *name = "t353 file";
    char *ndir = "t353 dir";
    uint16_t vol = VolID;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = 0;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV) {
        test_skipped(T_NO_UNIX_PREV);
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_nottested();
        goto fin;
    }

    bitmap = (1 << DIRPBIT_PDINFO) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
             (1 << DIRPBIT_UNIXPR);

    if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
        test_failed();
        goto fin1;
    }

    bitmap = (1 << DIRPBIT_UNIXPR);
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    bitmap = (1 << DIRPBIT_UNIXPR);
    filedir.unix_priv = 1;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;

    if (htonl(AFPERR_BITMAP) != FPSetDirParms(Conn, vol, dir, "", bitmap,
            &filedir)) {
        test_failed();
        filedir.unix_priv = S_IRUSR | S_IWUSR;
        FAIL(FPSetDirParms(Conn, vol, dir, "", bitmap, &filedir))
    }

    bitmap = (1 <<  FILPBIT_PDINFO) | (1 << FILPBIT_PDID) | (1 << FILPBIT_FNUM) |
             (1 << DIRPBIT_UNIXPR);

    if (/* htonl(AFPERR_BITMAP) != */ FPGetFileDirParams(Conn, vol, dir, name,
                                      bitmap, 0)) {
        test_failed();
        goto fin1;
    }

    bitmap = (1 <<  FILPBIT_PDINFO) | (1 << FILPBIT_PDID) | (1 << FILPBIT_FNUM);

    if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
        test_failed();
        goto fin1;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    bitmap = (1 << DIRPBIT_UNIXPR);
    filedir.unix_priv = 0;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;

    if (htonl(AFPERR_BITMAP) != FPSetFilDirParam(Conn, vol, dir, name, bitmap,
            &filedir)) {
        test_failed();
        filedir.unix_priv = S_IRUSR | S_IWUSR;
        FAIL(FPSetFilDirParam(Conn, vol, dir, name, bitmap, &filedir))
    }

fin1:
    FAIL(FPDelete(Conn, vol, dir, name))
fin:
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPSetDirParms:test353: no unix access privilege");
}

/* ------------------------- */
STATIC void test354() {
    int  dir = 0;
    char *ndir = "t354 dir";
    char *name = "t354 file";
    uint16_t vol = VolID;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = 0;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
        test_skipped(T_UNIX_PREV);
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_nottested();
        goto test_exit;
    }

    bitmap = (1 << DIRPBIT_PDINFO) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
             (1 << DIRPBIT_UNIXPR);

    if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    bitmap = (1 << DIRPBIT_UNIXPR);
    filedir.unix_priv = 0;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
    FAIL(FPSetDirParms(Conn, vol, dir, "", bitmap, &filedir))

    if (!FPCreateFile(Conn, vol, 0, dir, name)) {
        test_failed();
        FAIL(FPDelete(Conn, vol, dir, name))
    }

    /* double check with didn't screw the parent !*/
    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_failed();
    } else {
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    }

    filedir.unix_priv = S_IRUSR | S_IWUSR | S_IXUSR ;
    FAIL(FPSetDirParms(Conn, vol, dir, "", bitmap, &filedir))

    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_failed();
    } else {
        FAIL(FPDelete(Conn, vol, dir, name))
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPSetDirParms:test354: change a folder perm in root folder");
}

/* ------------------------- */
STATIC void test355() {
    int  dir = 0;
    int  dir1 = 0;
    char *ndir = "t355 dir";
    char *ndir1 = "t355 subdir";
    char *name = "t355 file";
    uint16_t vol = VolID;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = 0;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
        test_skipped(T_UNIX_PREV);
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_nottested();
        goto test_exit;
    }

    if (!(dir1 = FPCreateDir(Conn, vol, dir, ndir1))) {
        test_nottested();
        goto fin;
    }

    bitmap = (1 << DIRPBIT_PDINFO) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
             (1 << DIRPBIT_UNIXPR);

    if (FPGetFileDirParams(Conn, vol, dir1, "", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs,  0, bitmap);
    bitmap = (1 << DIRPBIT_UNIXPR);
    filedir.unix_priv = 0;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
    FAIL(FPSetDirParms(Conn, vol, dir1, "", bitmap, &filedir))

    if (!FPCreateFile(Conn, vol, 0, dir1, name)) {
        test_failed();
        FAIL(FPDelete(Conn, vol, dir1, name))
    }

    /* double check with didn't screw the parent !*/
    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_failed();
    } else {
        FAIL(FPDelete(Conn, vol, dir, name))
    }

    filedir.unix_priv = S_IRUSR | S_IWUSR | S_IXUSR ;
    FAIL(FPSetDirParms(Conn, vol, dir1, "", bitmap, &filedir))

    if (FPCreateFile(Conn, vol, 0, dir1, name)) {
        test_failed();
    } else {
        FAIL(FPDelete(Conn, vol, dir1, name))
    }

fin:
    FAIL(dir1 && FPDelete(Conn, vol, dir1, ""))
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPSetDirParms:test355: change a folder perm in a folder");
}

/* ------------------------- */
STATIC void test356() {
    int  dir = 0;
    char *ndir = "t356 dir";
    char *name = "t356 file";
    uint16_t vol = VolID;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = 0;
    int old_unixpriv;
    int ret;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
        test_skipped(T_UNIX_PREV);
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_nottested();
        goto test_exit;
    }

    bitmap = (1 << DIRPBIT_PDINFO) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
             (1 << DIRPBIT_UNIXPR);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    memset(&filedir, 0, sizeof(filedir));
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs,  0, bitmap);
    bitmap = (1 << DIRPBIT_UNIXPR);
    old_unixpriv = filedir.unix_priv;
    filedir.unix_priv = 0;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
    ret = FPSetDirParms(Conn, vol, DIRDID_ROOT, "", bitmap, &filedir);

    if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
        test_failed();
    }

    if (!ret) {
        filedir.unix_priv = old_unixpriv;
        FAIL(FPSetDirParms(Conn, vol, DIRDID_ROOT, "", bitmap, &filedir))
    }

    filedir.unix_priv = S_IRUSR | S_IXUSR ;
    FAIL(FPSetDirParms(Conn, vol, DIRDID_ROOT, "", bitmap, &filedir))

    if (!FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_failed();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    }

    filedir.unix_priv = old_unixpriv;
    FAIL(FPSetDirParms(Conn, vol, DIRDID_ROOT, "", bitmap, &filedir))

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_failed();
    } else {
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPSetDirParms:test356: change root folder unix perm");
}

/* ---------------------- */
static gid_t   *groups;
#define GROUPS_SIZE sizeof(gid_t)
static int     ngroups;

static int init_groups(void) {
    if ((ngroups = getgroups(0, NULL)) < 0) {
        return 0;
    }

    if (ngroups < 2) {
        /* we need at least 2 groups for testing */
        return 0;
    }

    if (NULL == (groups = calloc(ngroups, GROUPS_SIZE))) {
        return 0;
    }

    if ((ngroups = getgroups(ngroups, groups)) < 0) {
        return 0;
    }

    return ngroups;
}

/* -----------------
 * only work if client and server share the same numeric ids
*/
static  int check_group(gid_t group) {
    int i, j;

    for (i = 0; i < ngroups; i++) {
        if (groups[i] == group) {
            for (j = 0; j < ngroups; j++) {
                if (groups[j] != group) {
                    /* find a different group */
                    return groups[j];
                }
            }

            return 0;
        }
    }

    return 0;
}


/* ------------------------- */
STATIC void test405() {
    int  dir = 0;
    char *ndir = "t405 dir";
    uint16_t vol = VolID;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = 0;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
        test_skipped(T_UNIX_PREV);
        goto test_exit;
    }

    if (!init_groups()) {
        test_skipped(T_UNIX_GROUP);
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_nottested();
        goto test_exit;
    }

    bitmap = (1 << DIRPBIT_PDINFO) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
             (1 << DIRPBIT_UNIXPR);

    if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (!(filedir.gid = check_group(filedir.gid))) {
        test_skipped(T_UNIX_GROUP);
        FPDelete(Conn, vol, dir, "");
        goto test_exit;
    }

    bitmap = (1 << DIRPBIT_UNIXPR);
    FAIL(FPSetDirParms(Conn, vol, dir, "", bitmap, &filedir))
fin:
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPSetDirParms:test405: change a folder group owner in the root folder");
}

/* ----------- */
void FPSetDirParms_test() {
    ENTER_TESTSET
    test82();
    test84();
    test88();
    test107();
    test189();
    test193();
    test351();
    test352();
    test353();
    test354();
    test355();
    test356();
    test405();
}
