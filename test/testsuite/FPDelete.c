/* ----------------------------------------------
*/
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/* ------------------------- */
STATIC void test27()
{
    char *name  = "t27 file";
    char *name2 = "t27 dir";
    uint16_t vol = VolID;
    int  dir;
    ENTER_TEST
    dir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name2);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_nottested();
    }

    FAIL(htonl(AFPERR_DIRNEMPT) != FPDelete(Conn, vol, DIRDID_ROOT, name2))
    FAIL(FPDelete(Conn, vol, dir, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name2))
test_exit:
    exit_test("FPDelete:test27: delete not empty dir");
}

/* -------------------------- */
STATIC  void test74()
{
    int fork;
    uint16_t bitmap = 0;
    uint16_t vol2;
    char *name = "t74 Delete File 2 users";
    int type = OPENFORK_DATA;
    int len = (type == OPENFORK_RSCS) ? (1 << FILPBIT_RFLEN) : (1 << FILPBIT_DFLEN);
    uint16_t vol = VolID;
    DSI *dsi2;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);
    /* deny-write claim: an open alone does not block a delete */
    fork = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD | OPENACC_DWR);

    if (!fork) {
        test_failed();
        goto fin;
    }

    FAIL(FPSetForkParam(Conn, fork, len, 50))
    FAIL(htonl(AFPERR_BUSY) != FPDelete(Conn2, vol2,  DIRDID_ROOT, name))
    FAIL(FPCloseFork(Conn, fork))
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPCloseVol(Conn2, vol2))
test_exit:
    exit_test("FPDelete:test74: Delete File 2 users");
}

/* ------------------------- */
STATIC void test90()
{
    int  dir = 0;
    char *name = "t90 dir";
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (!(dir = no_access_folder(vol, DIRDID_ROOT, name))) {
        goto test_exit;
    }

    FAIL(ntohl(AFPERR_ACCESS) != FPDelete(Conn, vol, DIRDID_ROOT, name))
    delete_folder(vol, DIRDID_ROOT, name);
test_exit:
    exit_test("FPDelete:test90: delete a dir without access");
}

/* -------------------------- */
STATIC void test172()
{
    uint16_t bitmap = 0;
    char *tname = "test172";
    char *name = "test172.txt";
    char *name1 = "newtest172.txt";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    int tdir;
    int fork;
    int dir = 0;
    unsigned int ret;
    uint16_t vol = VolID;
    const DSI *dsi = &Conn->dsi;
    int dt;
    ENTER_TEST
    memset(&filedir, 0, sizeof(filedir));
    tdir  = FPCreateDir(Conn, vol, DIRDID_ROOT, tname);

    if (!tdir) {
        test_nottested();
        goto test_exit;
    }

    if (FPDelete(Conn, vol, tdir, "")) {
        test_nottested();
        goto test_exit;
    }

    dt = FPOpenDT(Conn, vol);
    /* ---- fork.c ---- */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, tdir, tname,
                      OPENACC_WR | OPENACC_RD);

    if (fork || htonl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
        test_failed();

        if (fork) {
            FPCloseFork(Conn, fork);
        }
    }

    /* ---- file.c ---- */
    FAIL(htonl(AFPERR_NOOBJ) != FPCreateFile(Conn, vol, 0, tdir, tname))
    bitmap = (1 << FILPBIT_MDATE);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(Conn, &filedir, dsi->data + ofs, 0, bitmap);
        filedir.isdir = 0;
        FAIL(htonl(AFPERR_NOOBJ) != FPSetFileParams(Conn, vol, tdir, tname, bitmap,
                &filedir))
    }

    /* -------------------- */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))

    if (htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, tdir, vol, DIRDID_ROOT, tname,
                                          "", name1)) {
        test_failed();
        FPDelete(Conn, vol, DIRDID_ROOT, name1);
    }

    FAIL(htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, tdir, name,
                                           "", tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))

    /* -------------------- */
    if (get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) {
        ret = FPCreateID(Conn, vol, tdir, tname);

        if (htonl(AFPERR_NOOBJ) != ret && htonl(AFPERR_PARAM) != ret) {
            test_failed();
        }
    }

    /* -------------------- */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1))
    ret = FPExchangeFile(Conn, vol, tdir, dir, tname, name1);

    if (ntohl(AFPERR_NOOBJ) != ret) {
        if (ret == htonl(AFPERR_PARAM)) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED (IGNORED) not always the same error code!\n");
            }

            test_skipped(T_NONDETERM);
        } else {
            test_failed();
        }
    }

    FAIL(ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, DIRDID_ROOT, tdir, name,
            tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
    /* ---- directory.c ---- */
    filedir.isdir = 1;
    FAIL(ntohl(AFPERR_NOOBJ) != FPSetDirParms(Conn, vol, tdir, tname, bitmap,
            &filedir))
    /* ---------------- */
    dir  = FPCreateDir(Conn, vol, tdir, tname);

    if (dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
        test_failed();
    }

    /* ---------------- */
    dir = FPOpenDir(Conn, vol, tdir, tname);

    if (dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
        test_failed();
    }

    /* ---- filedir.c ---- */

    if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, tdir, tname, 0,
            (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
            (1 << DIRPBIT_UID) |
            (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS))
       ) {
        test_failed();
    }

    /* ---------------- */
    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(Conn, &filedir, dsi->data + ofs, 0, bitmap);
        FAIL(ntohl(AFPERR_NOOBJ) != FPSetFilDirParam(Conn, vol, tdir, tname, bitmap,
                &filedir))
    }

    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPRename(Conn, vol, tdir, tname, name1))
    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, tdir, tname))
    /* ---------------- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, tdir, DIRDID_ROOT, tname,
            name1))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, tdir, name,
            tname))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    /* ---- enumerate.c ---- */
    /* FIXME: FPEnumerate* uses dsi_data_receive. See afphelper.c:delete_directory_tree() */
    ret = FPEnumerate(Conn, vol, tdir, tname,
                      (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR) |
                      (1 << FILPBIT_FINFO) |
                      (1 << FILPBIT_CDATE) | (1 << FILPBIT_PDID)
                      ,
                      (1 << DIRPBIT_ATTR) |
                      (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                      (1 << DIRPBIT_ACCESS)
                     );

    if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NODIR, AFPERR_NODIR)) {
        test_failed();
    }

    /* ---- desktop.c ---- */
    FAIL(ntohl(AFPERR_NOOBJ) != FPAddComment(Conn, vol, tdir, tname, "Comment"))
    FAIL(ntohl(AFPERR_NOOBJ) != FPGetComment(Conn, vol, tdir, tname))
    FAIL(ntohl(AFPERR_NOOBJ) != FPRemoveComment(Conn, vol, tdir, tname))
    FAIL(FPCloseDT(Conn, dt))
test_exit:
    exit_test("FPDelete:test172: did error did=<deleted> name=test172 name");
}

/* -------------------------- */
/* Known to kill afpd 1.6.x servers */
STATIC void test196()
{
    char *name = "test196";
    char *name2 = "test196_subdir";
    char *name1 = "test196/test196_subdir";
    uint16_t vol = VolID;
    uint16_t vol2;
    int tdir;
    int tdir1 = 0;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    const DSI *dsi = &Conn->dsi;
    uint16_t bitmap = (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) |
                      (1 << DIRPBIT_DID) | (1 << DIRPBIT_UID) |
                      (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS);
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    tdir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name);

    if (!tdir) {
        test_nottested();
        goto test_exit;
    }

    tdir1  = FPCreateDir(Conn, vol, tdir, name2);

    if (!tdir1) {
        test_nottested();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, tdir1, "", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    bitmap = (1 << DIRPBIT_ACCESS);
    FAIL(FPGetFileDirParams(Conn, vol, tdir, "", 0, bitmap))
    filedir.isdir = 1;
    afp_filedir_unpack(Conn, &filedir, dsi->data + ofs, 0, bitmap);
    filedir.access[0] = 0;
    filedir.access[1] = 7;
    filedir.access[2] = 7;
    filedir.access[3] = 7;
    FAIL(FPSetDirParms(Conn, vol, tdir, "", bitmap, &filedir))
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto fin;
    }

    if (FPDelete(Conn2, vol2,  DIRDID_ROOT, name1)) {
        test_nottested();
        FPDelete(Conn, vol, tdir1, "");
        tdir1 = 0;
    }

    if (FPDelete(Conn2, vol2,  DIRDID_ROOT, name)) {
        test_nottested();
        FPDelete(Conn, vol, tdir, "");
        tdir = 0;
    }

    FPCloseVol(Conn2, vol2);
    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, tdir1, ""))
    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, tdir, ""))
    tdir = tdir1 = 0;
fin:
    FAIL(tdir1 && FPDelete(Conn, vol, tdir1, ""))
    FAIL(tdir && FPDelete(Conn, vol, tdir, ""))
test_exit:
    exit_test("FPDelete:test196: delete a folder in a deleted folder");
}

/* -------------------------- */
STATIC void test368()
{
    int fork;
    uint16_t bitmap = 0;
    uint16_t vol2 = 0;
    char *name = "t368 Delete File 2 users";
    char *name3 = "t368 new name";
    char *name2 = "t368 dir";
    int type = OPENFORK_DATA;
    int len = (type == OPENFORK_RSCS) ? (1 << FILPBIT_RFLEN) : (1 << FILPBIT_DFLEN);
    uint16_t vol = VolID;
    DSI *dsi2;
    int  dir;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    dir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name2);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        goto fin;
    }

    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);
    fork = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD | OPENACC_DRD | OPENACC_DWR);

    if (!fork) {
        test_failed();
        goto fin;
    }

    FAIL(FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, name3))
    FAIL(FPSetForkParam(Conn, fork, len, 50))
    FAIL(htonl(AFPERR_BUSY) != FPDelete(Conn2, vol2,  dir, name3))
    FAIL(FPCloseFork(Conn, fork))
fin:
    FPDelete(Conn, vol, DIRDID_ROOT, name);
    FPDelete(Conn, vol, dir, name3);
    FAIL(FPDelete(Conn, vol, dir, ""))
    FAIL(FPCloseVol(Conn2, vol2))
test_exit:
    exit_test("FPDelete:test368: Delete File 2 users after it has been moved");
}

/* -------------------------- */
STATIC void test369()
{
    int fork;
    uint16_t bitmap = 0;
    uint16_t vol2 = 0;
    char *name = "t369 Delete File 2 users";
    char *name2 = "t369 dir";
    int type = OPENFORK_DATA;
    int len = (type == OPENFORK_RSCS) ? (1 << FILPBIT_RFLEN) : (1 << FILPBIT_DFLEN);
    uint16_t vol = VolID;
    DSI *dsi2;
    int  dir;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    dir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name2);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        goto fin;
    }

    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);
    fork = FPOpenFork(Conn, vol, type, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD | OPENACC_DRD | OPENACC_DWR);

    if (!fork) {
        test_failed();
        goto fin;
    }

    FAIL(FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, ""))
    FAIL(FPSetForkParam(Conn, fork, len, 50))
    FAIL(htonl(AFPERR_BUSY) != FPDelete(Conn2, vol2,  dir, name))
    FAIL(FPCloseFork(Conn, fork))
fin:
    FPDelete(Conn, vol, DIRDID_ROOT, name);
    FPDelete(Conn, vol, dir, name);
    FAIL(FPDelete(Conn, vol, dir, ""))
    FAIL(FPCloseVol(Conn2, vol2))
test_exit:
    exit_test("FPDelete:test369: Delete File 2 users after it has been moved");
}

/* -------------------------- */
STATIC void test421()
{
    char *name = "test421";
    uint16_t vol = VolID;
    uint16_t vol2;
    int tdir;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    const DSI *dsi = &Conn->dsi;
    uint16_t bitmap = (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) |
                      (1 << DIRPBIT_DID) | (1 << DIRPBIT_UID) |
                      (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS);
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    tdir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name);

    if (!tdir) {
        test_nottested();
        goto test_exit;
    }

    bitmap = (1 << DIRPBIT_ACCESS);
    FAIL(FPGetFileDirParams(Conn, vol, tdir, "", 0, bitmap))
    filedir.isdir = 1;
    afp_filedir_unpack(Conn, &filedir, dsi->data + ofs, 0, bitmap);
    filedir.access[0] = 0;
    filedir.access[1] = 7;
    filedir.access[2] = 7;
    filedir.access[3] = 7;
    FAIL(FPSetDirParms(Conn, vol, tdir, "", bitmap, &filedir))
    FAIL(FPGetFileDirParams(Conn, vol, tdir, "", 0, bitmap))
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto fin;
    }

    if (FPDelete(Conn2, vol2,  DIRDID_ROOT, name)) {
        test_nottested();
        FPDelete(Conn, vol, tdir, "");
    }

    FPCloseVol(Conn2, vol2);
    FAIL(ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol, tdir, ""))
    tdir = 0;
fin:
    FAIL(tdir && FPDelete(Conn, vol, tdir, ""))
test_exit:
    exit_test("FPDelete:test421: delete an already deleted curdir folder");
}

/* ------------------------- */
extern int Attention_received;

STATIC void test422()
{
    uint16_t vol = VolID;
    char *name = "t422 file";
    int ret;
    ENTER_TEST

    if (Mac) {
        test_skipped(T_MAC);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    sleep(2);
    ret = FPDelete(Conn, vol, DIRDID_ROOT, name);

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    if (Conn->afp_version < 32) {
        if (!Attention_received) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED no attention received\n");
            }

            test_failed();
        }
    } else if (Attention_received) {
        fprintf(stdout, "\tFAILED attention received\n");
        test_failed();
    }

test_exit:
    exit_test("FPDelete:test422: Server notification on volume date change if AFP < 3.2");
}


/* -------------------------------------------------------------------------
 * test609  Cross-session DeleteInhibit (kFPDeleteInhibitBit) lifecycle.
 *
 * Session 1 (Conn) sets ATTRBIT_NODELETE, session 2 (Conn2) deletes:
 *
 *   1. Conn  FPCreateFile, then FPSetFileParams ATTRBIT_NODELETE|SETCLR  (set)
 *   2. Conn2 FPDelete  -> assert AFPERR_OLOCK            (cross-session refuse)
 *   3. Conn  FPSetFileParams ATTRBIT_NODELETE (SETCLR off => clear)
 *   4. Conn2 FPDelete  -> assert success                (cross-session allow)
 */
STATIC void test609()
{
    char *name = "t609 deleteinhibit xsession";
    int  ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 << FILPBIT_ATTR);
    uint16_t vol = VolID;
    uint16_t vol2;
    const DSI *dsi;
    ENTER_TEST
    dsi = &Conn->dsi;

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto cleanup_file;
    }

    /* read current attrs so the unpacked struct is well-formed before we
     * OR in NODELETE */
    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_nottested();
        goto cleanup_vol2;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(Conn, &filedir, dsi->data + ofs, bitmap, 0);
    /* (1) session 1 sets DeleteInhibit */
    filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR;
    FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
    /* (2) session 2 delete must be refused cross-session */
    FAIL(ntohl(AFPERR_OLOCK) != FPDelete(Conn2, vol2, DIRDID_ROOT, name))
    /* (3) session 1 clears it (SETCLR off => clear the named bits) */
    filedir.attr = ATTRBIT_NODELETE;
    FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
    /* (4) session 2 delete must now succeed cross-session (also disposes the
     *     file, so no explicit cleanup delete below) */
    FAIL(FPDelete(Conn2, vol2, DIRDID_ROOT, name))
    FAIL(FPCloseVol(Conn2, vol2))
    goto test_exit;
cleanup_vol2:
    FPCloseVol(Conn2, vol2);
cleanup_file:
    /* clear any inhibit we may have set, then remove via session 1 */
    filedir.attr = ATTRBIT_NODELETE;
    FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir);
    FPDelete(Conn, vol, DIRDID_ROOT, name);
test_exit:
    exit_test("FPDelete:test609: cross-session DeleteInhibit lifecycle");
}

/* -------------------------------------------------------------------------
 * test608  An open with no access mode (no Read/Write/DenyRead/DenyWrite) does
 *          not block a cross-session delete.
 *
 * Conn opens the data fork with access == 0 — a fork that is open but holds no
 * deny mode and reserves no access. Per the AFP sharing model a "none" access
 * open makes no claim against other users, so Conn2 FPDelete must succeed.
 *
 * The delete-time conflict check is selective: a "none" access open plants only
 * the OPEN_NONE share-mode marker, which the delete gate ignores (it blocks only
 * on a deny mode or a held content lock). A bare open therefore does not refuse a
 * cross-session delete.
 */
STATIC void test608()
{
    char *name = "t608 open_none nonblock";
    uint16_t vol = VolID;
    uint16_t vol2;
    uint16_t fork;
    unsigned int dret;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      0 /* no access */);

    if (!fork) {
        /* server may reject access==0 */
        test_nottested();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        goto test_exit;
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto cleanup;
    }

    dret = FPDelete(Conn2, vol2, DIRDID_ROOT, name);
    FAIL(AFP_OK != dret)   /* a no-access open must not block delete */
    FAIL(FPCloseVol(Conn2, vol2))
    /* file is gone (Conn2 deleted it); close our fork, do not re-delete */
    FAIL(FPCloseFork(Conn, fork))
    goto test_exit;
cleanup:
    FPCloseFork(Conn, fork);
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPDelete:test608: open with no access mode does not block delete");
}

/* -------------------------
 * test610  A read-only open with no deny mode does not block a cross-session
 *          delete.
 *
 * Conn opens the data fork OPENACC_RD only (read access, no write, no deny).
 * Per the AFP sharing model "someone has it open read-only, no deny" is not a
 * claim that should refuse a delete, so Conn2 FPDelete must succeed.
 *
 * Mechanism: a read-only, no-deny open plants only the OPEN_RD share-mode marker
 * (and, for the rsrc case, RSRC_OPEN_RD).  DELETE_BLOCKING_BAND_BITS excludes both
 * OPEN_RD and OPEN_NONE, so the delete gate ignores them and blocks only on a deny
 * mode, DeleteInhibit, or a held content lock.  Companion to test608 (OPEN_NONE).
 */
STATIC void test610()
{
    char *name = "t610 open_rd nonblock";
    uint16_t vol = VolID;
    uint16_t vol2;
    uint16_t fork;
    unsigned int dret;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD /* read only, no deny */);

    if (!fork) {
        test_nottested();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        goto test_exit;
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto cleanup;
    }

    dret = FPDelete(Conn2, vol2, DIRDID_ROOT, name);
    FAIL(AFP_OK != dret)   /* a read-only, no-deny open must not block delete */
    FAIL(FPCloseVol(Conn2, vol2))
    /* file is gone (Conn2 deleted it); close our fork, do not re-delete */
    FAIL(FPCloseFork(Conn, fork))
    goto test_exit;
cleanup:
    FPCloseFork(Conn, fork);
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPDelete:test610: read-only open does not block delete");
}

/* -------------------------
 * test611  A write-open with no deny mode does not block a cross-session
 *          delete.
 *
 * Conn opens the data fork read-write with no deny claim; Conn2's FPDelete
 * must succeed.  An open — even for write — is access, not a claim; only
 * deny modes and content locks block (SMB share-mode semantics, where
 * existing WRITE access never blocks a delete).  The holder's fds stay
 * valid on the unlinked inode; nothing in the holder's session is
 * invalidated.  Companion to test610 (OPEN_RD) and test608 (OPEN_NONE).
 */
STATIC void test611()
{
    char *name = "t611 open_wr nonblock";
    uint16_t vol = VolID;
    uint16_t vol2;
    uint16_t fork;
    unsigned int dret;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR /* no deny */);

    if (!fork) {
        test_nottested();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        goto test_exit;
    }

    FAIL(FPWrite(Conn, fork, 0, 2000, Data, 0))
    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto cleanup;
    }

    dret = FPDelete(Conn2, vol2, DIRDID_ROOT, name);
    FAIL(AFP_OK != dret)   /* a no-deny write-open must not block delete */
    FAIL(FPCloseVol(Conn2, vol2))
    /* the holder's fork survives the peer's delete: still readable, and the
     * client-side close still works (POSIX unlinked-but-open semantics) */
    FAIL(FPRead(Conn, fork, 0, 100, Data))
    FAIL(FPCloseFork(Conn, fork))
    goto test_exit;
cleanup:
    FPCloseFork(Conn, fork);
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPDelete:test611: no-deny write-open does not block delete");
}

/* -------------------------
 * test612  A same-session stale fork does not block a delete.
 *
 * Conn opens the data fork read-only (Preview's shape), never closes it, and
 * deletes the file on the same connection.  The server closes the session's
 * own tracked fork and the delete succeeds: the deleting session owns every
 * fork in its child, so an unclosed fork must not make the file undeletable.
 */
STATIC void test612()
{
    char *name = "t612 selfclean";
    uint16_t vol = VolID;
    uint16_t fork;
    unsigned int dret;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (!fork) {
        test_nottested();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        goto test_exit;
    }

    dret = FPDelete(Conn, vol, DIRDID_ROOT, name);
    FAIL(dret != AFP_OK)   /* a same-session stale fork must not block delete */

    if (dret == AFP_OK) {
        /* the sweep invalidated the refnum: the close must miss */
        FAIL(ntohl(AFPERR_PARAM) != FPCloseFork(Conn, fork))
    } else {
        FPCloseFork(Conn, fork);
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    }

test_exit:
    exit_test("FPDelete:test612: same-session stale fork swept on delete");
}

/* -------------------------
 * test613  ALL same-session stale forks close on delete, not just one.
 *
 * As test612 but with two unclosed forks (data + rsrc, two refnums) on the
 * inode.  The delete must succeed, which requires the server to close every
 * tracked fork on the inode, covering the rsrc/adouble close branch.
 */
STATIC void test613()
{
    char *name = "t613 selfclean2";
    uint16_t vol = VolID;
    uint16_t fork;
    uint16_t fork2;
    unsigned int dret;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (!fork) {
        test_nottested();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        goto test_exit;
    }

    fork2 = FPOpenFork(Conn, vol, OPENFORK_RSCS, 0, DIRDID_ROOT, name,
                       OPENACC_RD);

    if (!fork2) {
        test_nottested();
        FAIL(FPCloseFork(Conn, fork))
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        goto test_exit;
    }

    dret = FPDelete(Conn, vol, DIRDID_ROOT, name);
    FAIL(dret != AFP_OK)   /* every same-session fork on the inode sweeps */

    if (dret == AFP_OK) {
        /* the sweep invalidated BOTH refnums: each close must miss */
        FAIL(ntohl(AFPERR_PARAM) != FPCloseFork(Conn, fork))
        FAIL(ntohl(AFPERR_PARAM) != FPCloseFork(Conn, fork2))
    } else {
        FPCloseFork(Conn, fork);
        FPCloseFork(Conn, fork2);
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    }

test_exit:
    exit_test("FPDelete:test613: all same-session forks swept on delete");
}

/* -------------------------
 * test614  A same-session deny-mode fork blocks the delete; a plain
 *          write-open does not.
 *
 * The delete-blocking policy is holder-agnostic and claim-based: only deny
 * modes and content locks refuse; an open — even write — is not a claim
 * (QuickLook leaks write-opens, and SMB/POSIX deletes succeed under them).
 * Phase 1: open with a deny-write claim, write, FPDelete on the same
 * connection: refused AFPERR_BUSY and the fork stays usable; delete
 * succeeds after close.  Phase 2: open plain read-write, write, FPDelete:
 * succeeds, the fork is swept.
 */
STATIC void test614()
{
    char *name = "t614 own deny blocks";
    uint16_t vol = VolID;
    uint16_t fork;
    unsigned int dret;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR | OPENACC_DWR);

    if (!fork) {
        test_nottested();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        goto test_exit;
    }

    FAIL(FPWrite(Conn, fork, 0, 2000, Data, 0))
    FAIL(ntohl(AFPERR_BUSY) != FPDelete(Conn, vol, DIRDID_ROOT, name))
    /* the refused delete must not have touched the open fork */
    FAIL(FPGetForkParam(Conn, fork, 1 << FILPBIT_DFLEN))

    if (FPCloseFork(Conn, fork)) {
        /* a still-open deny fork would make the file undeletable: retry */
        test_failed();
        FPCloseFork(Conn, fork);
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))

    /* phase 2: a plain write-open (no deny) does not block; the fork sweeps */
    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR);

    if (!fork) {
        test_nottested();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        goto test_exit;
    }

    FAIL(FPWrite(Conn, fork, 0, 2000, Data, 0))
    dret = FPDelete(Conn, vol, DIRDID_ROOT, name);
    FAIL(dret != AFP_OK)   /* a write-open with no deny must not block */

    if (dret == AFP_OK) {
        /* the sweep invalidated the refnum: the close must miss */
        FAIL(ntohl(AFPERR_PARAM) != FPCloseFork(Conn, fork))
    } else {
        FPCloseFork(Conn, fork);
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    }

test_exit:
    exit_test("FPDelete:test614: own deny blocks delete; write-open sweeps");
}

/* -------------------------
 * test615  A once-written file with only a no-claim fork left deletes.
 *
 * Write via a read-write fork, flush, close it (the OPEN_WR claim is
 * released), then leak a read-only fork; the same-session delete must
 * succeed.  Pins that having-been-written is history, not a claim: only a
 * live OPEN_WR/deny/content lock blocks, never dirty/modified state.
 */
STATIC void test615()
{
    char *name = "t615 written then rd leak";
    uint16_t vol = VolID;
    uint16_t fork;
    unsigned int dret;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR);

    if (!fork) {
        test_nottested();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        goto test_exit;
    }

    FAIL(FPWrite(Conn, fork, 0, 2000, Data, 0))
    FAIL(FPFlushFork(Conn, fork))
    FAIL(FPCloseFork(Conn, fork))
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (!fork) {
        test_nottested();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        goto test_exit;
    }

    dret = FPDelete(Conn, vol, DIRDID_ROOT, name);
    FAIL(dret != AFP_OK)   /* once-written is not a claim; the RD fork sweeps */

    if (dret == AFP_OK) {
        /* the sweep invalidated the refnum: the close must miss */
        FAIL(ntohl(AFPERR_PARAM) != FPCloseFork(Conn, fork))
    } else {
        FPCloseFork(Conn, fork);
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    }

test_exit:
    exit_test("FPDelete:test615: once-written file with rd-only fork deletes");
}

/* -------------------------
 * test616  A refused delete leaves the session's open fork intact.
 *
 * DeleteInhibit blocks the delete before any fork cleanup: with the bit set
 * and a same-session fork open, FPDelete returns AFPERR_OLOCK and the fork
 * refnum must still be usable.  Clearing the bit lets the delete succeed and
 * only then is the fork server-closed.
 */
STATIC void test616()
{
    char *name = "t616 olock keeps fork";
    int  ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 << FILPBIT_ATTR);
    uint16_t vol = VolID;
    uint16_t fork;
    unsigned int dret;
    const DSI *dsi;
    ENTER_TEST
    dsi = &Conn->dsi;

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (!fork) {
        test_nottested();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_nottested();
        goto cleanup;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(Conn, &filedir, dsi->data + ofs, bitmap, 0);
    filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR;

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir)) {
        test_nottested();
        goto cleanup;
    }

    FAIL(ntohl(AFPERR_OLOCK) != FPDelete(Conn, vol, DIRDID_ROOT, name))
    /* the refused delete must not have touched the open fork */
    FAIL(FPGetForkParam(Conn, fork, 1 << FILPBIT_DFLEN))
    filedir.attr = ATTRBIT_NODELETE;

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir)) {
        /* the clear must not be left set: retry it via cleanup */
        test_failed();
        goto cleanup;
    }

    dret = FPDelete(Conn, vol, DIRDID_ROOT, name);
    FAIL(dret != AFP_OK)

    if (dret == AFP_OK) {
        /* the sweep invalidated the refnum: the close must miss */
        FAIL(ntohl(AFPERR_PARAM) != FPCloseFork(Conn, fork))
        goto test_exit;
    }

cleanup:
    FPCloseFork(Conn, fork);
    /* clear any inhibit we may have set, then remove */
    filedir.attr = ATTRBIT_NODELETE;
    FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir);
    FPDelete(Conn, vol, DIRDID_ROOT, name);
test_exit:
    exit_test("FPDelete:test616: refused delete leaves open fork intact");
}

/* ----------- */
void FPDelete_test()
{
    ENTER_TESTSET
    test27();
    test74();
    test172();
    test196();
    test368();
    test369();
    test421();
    test422();
    test608();
    test609();
    test610();
    test611();
    test612();
    test613();
    test614();
    test615();
    test616();
}
