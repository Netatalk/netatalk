/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test214()
{
    uint16_t vol = VolID;
    uint16_t dt = 0;
    unsigned int ret;
    char *file = "t214 file";
    char *name = "t214 dir";
    char *ndir  = "t214 dir no access";
    unsigned int pdir;
    int dir;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    if (!(pdir = no_access_folder(vol, DIRDID_ROOT, ndir))) {
        goto fin;
    }

    dt = FPOpenDT(Conn, vol);
    ret = FPGetAppl(Conn,  dt, "ttxt", 1, 0x42);

    if (htonl(AFPERR_NOITEM) != ret) {
        goto fin;
    }

    FAIL(htonl(AFPERR_BADTYPE) != FPAddAPPL(Conn, dt, dir, "ttxt", 0xafb471c0, ""))
    FAIL(htonl(AFPERR_BADTYPE) != FPAddAPPL(Conn, dt, DIRDID_ROOT, "ttxt",
                                            0xafb471c0, name))
    FAIL(htonl(AFPERR_NOOBJ) != FPAddAPPL(Conn, dt, DIRDID_ROOT_PARENT, "ttxt",
                                          0xafb471c0, name))
    FAIL(htonl(AFPERR_ACCESS) != FPAddAPPL(Conn, dt, pdir, "ttxt", 0xafb471c0,
                                           file))
    /*"SimpleText"*/
    ret = FPAddAPPL(Conn, dt, DIRDID_ROOT, "ttxt", 0xafb471c0, file);

    if (not_valid(ret, /* MAC */AFPERR_NOOBJ, 0)) {
        test_failed();
    }

    if (!ret) {
        FAIL(FPRemoveAPPL(Conn, dt, DIRDID_ROOT, "ttxt", file))
    }

    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file))
    FAIL(FPAddAPPL(Conn, dt, DIRDID_ROOT, "ttxt", 0xafb471c0, file))
    FAIL(FPRemoveAPPL(Conn, dt, DIRDID_ROOT, "ttxt", file))
    delete_folder(vol, DIRDID_ROOT, ndir);
fin:
    FAIL(dir && FPDelete(Conn, vol, dir, ""))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, file))
    FAIL(dt && FPCloseDT(Conn, dt))
test_exit:
    exit_test("FPAddAPPL:test214: test appl");
}

/* ------------------------- */
/* FIXME: The first branch needs to get refactored. Always fails with 1 user. */
STATIC void test301()
{
    uint16_t vol = VolID;
    unsigned int ret;
    char *file = "t301 file";
    char *name = "t301 dir";
    DSI *dsi, *dsi2;
    uint16_t vol2;
    uint32_t  did = DIRDID_ROOT;
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << DIRPBIT_ACCESS) | (1 << DIRPBIT_DID);
    uint16_t fork;
    struct afp_filedir_parms filedir;
    int dir;
    ENTER_TEST

    if (!Conn2) {
        dir = get_did(Conn, vol, did, name);

        if (!dir) {
            test_failed();
        } else if (!FPCreateDir(Conn, vol, dir, name)) {
            goto fin;
        }

        test_failed();
        ret = FPCreateFile(Conn, vol, 0, dir, file);
        FPEnumerate(Conn, vol, dir, "",
                    (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR),
                    (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) |
                    (1 << DIRPBIT_DID) | (1 << DIRPBIT_ACCESS)
                    | (1 << DIRPBIT_UID) | (1 << DIRPBIT_GID));

        if (ret) {
            if (!Quiet) {
                fprintf(stdout, "OK can't create a file\n");
            }

            goto fin;
        }

        fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, dir, file,
                          OPENACC_RD | OPENACC_WR);

        if (fork) {
            if (!Quiet) {
                fprintf(stdout,
                        "Ouch now we have a fork open then read/write in a folder we no access\n");
            }

            FPCloseFork(Conn, fork);
        }
    } else {
        dsi2 = &Conn2->dsi;
        dsi = &Conn->dsi;
        vol2  = FPOpenVol(Conn2, Vol);

        if (vol2 == 0xffff) {
            test_nottested();
            goto fin;
        }

        if (!(dir = FPCreateDir(Conn2, vol2, did, name))) {
            test_nottested();
            goto fin;
        }

        if (FPGetFileDirParams(Conn2, vol2,  dir, "", 0, bitmap)) {
            test_nottested();
            goto fin;
        }

        bitmap = (1 << DIRPBIT_ACCESS);
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi2->data + ofs, 0, bitmap);
        filedir.access[0] = 0;
        filedir.access[1] = 0;
        filedir.access[2] = 0;
        filedir.access[3] = 7;

        if (FPSetDirParms(Conn2, vol2, dir, "", bitmap, &filedir)) {
            test_nottested();
            goto fin;
        }

        FPCloseVol(Conn2, vol2);
    }

fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    exit_test("FPAddAPPL:test301: test appl 2");
}

/* ----------- */
void FPAddAPPL_test()
{
    ENTER_TESTSET
    test214();
#if 0
    test301();
#endif
}
