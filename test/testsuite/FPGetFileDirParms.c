/* ----------------------------------------------
*/
#include "specs.h"

static char temp[MAXPATHLEN + 1];
static char temp1[MAXPATHLEN + 1];

/* ------------------------- */
STATIC void test44() {
    char *name = "t44 dir";
    unsigned int  dir, did;
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST
    dir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, dir, "/", 0, (1 << DIRPBIT_DID))) {
        test_failed();
        goto fin;
    }

    memcpy(&did, dsi->data + 3 * sizeof(uint16_t), sizeof(did));

    if (dir != did) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED DIDs differ\n");
        }

        test_failed();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, dir, "//", 0, (1 << DIRPBIT_DID))) {
        test_failed();
        goto fin;
    }

    memcpy(&did, dsi->data + 3 * sizeof(uint16_t), sizeof(did));

    if (DIRDID_ROOT != did) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED DID not DIRDID_ROOT\n");
        }

        test_failed();
        goto fin;
    }

fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetFileDirParms:test44: access .. folder");
}

/* -------------------------- */
STATIC void test58() {
    uint16_t vol = VolID;
    ENTER_TEST

    if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, DIRDID_ROOT_PARENT, "",
            0,
            (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
            (1 << DIRPBIT_UID) |
            (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS))
       ) {
        test_failed();
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT_PARENT, Vol, 0,
                           (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                           (1 << DIRPBIT_UID) |
                           (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS) | (1 << DIRPBIT_OFFCNT))
       ) {
        test_failed();
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0,
                           (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                           (1 << DIRPBIT_UID) |
                           (1 << DIRPBIT_GID) | (1 << DIRPBIT_ACCESS) | (1 << DIRPBIT_OFFCNT))
       ) {
        test_failed();
    }

    exit_test("FPGetFileDirParms:test58: folder 1 (DIRDID_ROOT_PARENT)");
}

/* ----------- */
STATIC void test70() {
    uint16_t vol = VolID;
    int ofs;
    uint16_t bitmap;
    int len;
    int did = DIRDID_ROOT;
    char *name = "t70 bogus cname";
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    if (!Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "GetFileDirParams Vol %d \n\n", vol);
    }

    memset(dsi->commands, 0, DSI_CMDSIZ);
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_CMD;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    ofs = 0;
    dsi->commands[ofs++] = AFP_GETFLDRPARAM;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &vol, sizeof(vol));
    ofs += sizeof(vol);
    memcpy(dsi->commands + ofs, &did, sizeof(did));
    ofs += sizeof(did);
    bitmap = htons(1 << FILPBIT_LNAME);
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    bitmap = htons(DIRPBIT_LNAME);;
    memcpy(dsi->commands + ofs, &bitmap, sizeof(bitmap));
    ofs += sizeof(bitmap);
    dsi->commands[ofs++] = 4;		/* ERROR !! long name */
    len = strlen(name);
    dsi->commands[ofs++] = len;
    u2mac(&dsi->commands[ofs], name, len);
    ofs += len;
    dsi->datalen = ofs;
    dsi->header.dsi_len = htonl(dsi->datalen);
    dsi->header.dsi_code = 0; // htonl(err);
    my_dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    /* ------------------ */
    my_dsi_stream_receive(dsi, dsi->data, DSI_DATASIZ, &dsi->datalen);
    dump_header(dsi);

    if (dsi->header.dsi_code != htonl(AFPERR_PARAM)) {
        test_failed();
    }

test_exit:
    exit_test("FPGetFileDirParms:test70: bogus cname (unknown type)");
}

/* ------------------------- */
STATIC void test94() {
    int  dir;
    char *name = "t94 invisible dir";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t vol = VolID;
    uint16_t bitmap = (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_MDATE) |
                      (1 << DIRPBIT_OFFCNT);
    DSI *dsi;
    int offcnt;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_nottested();
        goto end;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    offcnt = filedir.offcnt;

    if (!Quiet) {
        fprintf(stdout, "Modif date parent %x offcnt %d\n", filedir.mdate, offcnt);
    }

    sleep(4);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_failed();
        goto end;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (!Quiet) {
        fprintf(stdout, "Modif date dir %x \n", filedir.mdate);
    }

    sleep(5);
    filedir.attr = ATTRBIT_INVISIBLE | ATTRBIT_SETCLR ;
    bitmap = (1 << DIRPBIT_ATTR);

    if (FPSetDirParms(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir)) {
        test_failed();
        goto end;
    }

    bitmap = (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_MDATE) | (1 << DIRPBIT_OFFCNT);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_failed();
        goto end;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (!Quiet) {
        fprintf(stdout, "Modif date dir %x\n", filedir.mdate);
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
        test_failed();
        goto end;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (offcnt != filedir.offcnt) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED got %d want %d\n", filedir.offcnt, offcnt);
        }

        test_failed();
    }

    if (!Quiet) {
        fprintf(stdout, "Modif date parent %x\n", filedir.mdate);
    }

end:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetFileDirParms:test94: test invisible bit attribute");
}

/* --------------------- */
STATIC void test104() {
    char *name1 = "t104 dir1";
    char *name2 = "t104 dir2";
    char *name3 = "t104 dir3";
    char *name4 = "t104 dir4";
    char *name5 = "t104 file";
    unsigned int  dir1 = 0, dir2 = 0, dir3 = 0, dir4 = 0;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_DID) | (1 << DIRPBIT_LNAME);
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir1 = FPCreateDir(Conn, vol, DIRDID_ROOT, name1))) {
        test_nottested();
        goto fin;
    }

    if (!(dir2 = FPCreateDir(Conn, vol, dir1, name2))) {
        test_nottested();
        goto fin;
    }

    if (FPCreateFile(Conn, vol, 0, dir2, name5)) {
        test_nottested();
        goto fin;
    }

    if (!(dir3 = FPCreateDir(Conn, vol, dir2, name3))) {
        test_nottested();
        goto fin;
    }

    if (!(dir4 = FPCreateDir(Conn, vol, dir3, name4))) {
        test_nottested();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, dir3, "", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.did != dir3) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.did, dir3);
        }

        test_failed();
        goto fin;
    }

    if (strcmp(filedir.lname, name3)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %s should be %s\n", filedir.lname, name3);
        }

        test_failed();
        goto fin;
    }

    bitmap = (1 << DIRPBIT_DID) | (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_OFFCNT);

    if (FPGetFileDirParams(Conn, vol, dir3, "t104 dir4///", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.did != dir2) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.did, dir2);
        }

        test_failed();
        goto fin;
    }

    if (strcmp(filedir.lname, name2)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %s should be %s\n", filedir.lname, name2);
        }

        test_failed();
        goto fin;
    }

    if (filedir.offcnt != 2) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %d\n", filedir.offcnt);
        }

        test_failed();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, dir3, "t104 dir4/", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.did != dir4) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.did, dir4);
        }

        test_failed();
        goto fin;
    }

    if (strcmp(filedir.lname, name4)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %s should be %s\n", filedir.lname, name4);
        }

        test_failed();
    }

fin:
    FAIL(dir3 && FPDelete(Conn, vol, dir3, name4))
    FAIL(dir2 && FPDelete(Conn, vol, dir2, name3))
    FAIL(dir2 && FPDelete(Conn, vol, dir2, name5))
    FAIL(dir1 && FPDelete(Conn, vol, dir1, name2))
    FAIL(dir1 && FPDelete(Conn, vol, dir1, ""))
    exit_test("FPGetFileDirParms:test104: cname with trailing 0");
}

/* -------------------------- */
STATIC void test132() {
    int  dir;
    char *name = "t132 file";
    char *name1 = "t132 dir";
    uint16_t bitmap;
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name1))) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, name)) {
        test_nottested();
        goto fin;
    }

    FAIL(htonl(AFPERR_BITMAP) != FPGetFileDirParams(Conn, vol, dir, name, 0xffff,
            0))
    FAIL(htonl(AFPERR_BITMAP) != FPGetFileDirParams(Conn, vol, dir, "", 0, 0xffff))
    bitmap = (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_FINFO) |
             (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
             (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
             (1 << DIRPBIT_ACCESS);

    if (!strcmp("disk1", Vol)) {
        if (!Quiet) {
            fprintf(stdout, "Volume is disk1 choose other name!\n");
        }

        test_nottested();
        goto fin;
    }

    if (!FPGetFileDirParams(Conn, vol, DIRDID_ROOT_PARENT, "disk1", 0, bitmap)) {
        test_failed();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT_PARENT, Vol, 0, bitmap)) {
        test_failed();
    }

    FAIL(FPDelete(Conn, vol, dir, name))
fin:
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPGetFileDirParms:test132: GetFilDirParams errors");
}

/* ------------------------- */
STATIC void test194() {
    int  dir = 0;
    char *name = "t194 dir";
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

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0,
                           (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                           (1 << DIRPBIT_ACCESS))) {
        test_failed();
    }

    delete_folder(vol, DIRDID_ROOT, name);
test_exit:
    exit_test("FPGetFileDirParms:test194: dir without access");
}

/* ------------------------- */
STATIC void test229() {
    int  dir = 0;
    char *name = "t229 file";
    char *ndir = "t229 dir";
    uint16_t vol = VolID;
    DSI *dsi;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
        test_skipped(T_UNIX_PREV);
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

    if (FPGetFileDirParams(Conn, vol, dir, "", 0,
                           (1 <<  DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << DIRPBIT_DID) |
                           (1 << DIRPBIT_UNIXPR))) {
        test_failed();
    }

    if (FPGetFileDirParams(Conn, vol, dir, name,
                           (1 <<  FILPBIT_PDINFO) | (1 << FILPBIT_PDID) | (1 << FILPBIT_FNUM) |
                           (1 << DIRPBIT_UNIXPR),
                           0)) {
        test_failed();
    }

    FAIL(FPDelete(Conn, vol, dir, name))
fin:
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPGetFileDirParms:test229: unix access privilege");
}

/* ------------------------- */
STATIC void test307() {
    char *name = "t307 dir#2";
    uint16_t vol = VolID;
    DSI *dsi;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = 0;
    unsigned int dir;
    char *result;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_failed();
        goto test_exit;
    }

    if (Conn->afp_version >= 30) {
        bitmap = (1 << FILPBIT_PDINFO);
    } else {
        bitmap = (1 << DIRPBIT_LNAME);
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_nottested();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        result = (Conn->afp_version >= 30) ? filedir.utf8_name : filedir.lname;

        if (strcmp(result, name)) {
            test_failed();
        }
    }

    if (!FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "bad name #2", 0, bitmap)) {
        test_failed();
    }

    if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "???#2",
            0, bitmap)) {
        test_failed();
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetFileDirParms:test307: mangled dirname");
}

/* ------------------------- */
STATIC void test308() {
    char *name = "t308 dir";
    uint16_t vol = VolID;
    DSI *dsi;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = 0;
    unsigned int dir;
    char *result;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Conn->afp_version >= 30) {
        bitmap = (1 << FILPBIT_PDINFO);
    } else {
        if (Mac) { /* a Mac AFP 2.x can't create filename longer than 31 bytes*/
            test_skipped(T_MAC);
            goto test_exit;
        }

        bitmap = (1 << DIRPBIT_LNAME);
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_failed();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_nottested();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        result = (Conn->afp_version >= 30) ? filedir.utf8_name : filedir.lname;

        if (strcmp(result, name)) {
            test_failed();
        }
    }

    sprintf(temp, "t307#%X", ntohl(dir));

    if (!FPGetFileDirParams(Conn, vol, DIRDID_ROOT, temp, 0, bitmap)) {
        test_failed();
    }

    sprintf(temp, "t308#%X", ntohl(dir));

    if (!FPGetFileDirParams(Conn, vol, DIRDID_ROOT, temp, 0, bitmap)) {
        test_failed();
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetFileDirParms:test308: mangled dirname");
}

/* ------------------------- */
STATIC void test319() {
    uint16_t vol = VolID;
    ENTER_TEST

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, (1 << DIRPBIT_ACCESS))) {
        test_failed();
    }

    exit_test("FPGetFileDirParms:test319: get volume access right");
}

/* ------------------------- */
STATIC void test324() {
    char *name = "t324 very long filename more than 31 bytes.txt";
    uint16_t vol = VolID;
    DSI *dsi;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = 0;
    unsigned int dir;
    char *result;
    int ret;
    int id;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Conn->afp_version >= 30) {
        bitmap = (1 << FILPBIT_PDINFO);
    } else {
        if (Mac) { /* a Mac AFP 2.x can't create filename longer than 31 bytes*/
            test_skipped(T_MAC);
            goto test_exit;
        }

        bitmap = (1 << DIRPBIT_LNAME);
    }

    ret = FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name);

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    /* hack if filename < 255 it works with afp 2.x too */
    id = get_fid(Conn, vol, DIRDID_ROOT, name);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_nottested();
    }

#if 0
    else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        result = (Conn->afp_version >= 30) ? filedir.utf8_name : filedir.lname;

        if (strcmp(result, name)) {
            test_failed();
        }
    }

#endif
    sprintf(temp1, "#%X.txt", ntohl(id));
    memset(temp, 0, sizeof(temp));
    strncpy(temp, name, 31 - strlen(temp1));
    strcat(temp, temp1);
    /* for afp3 it was not valid mangled filename
     * Updated 2004-07-12
     * we changed mangled handling, now it's a valid filename for AFP3.x.
    */
    ret = FPGetFileDirParams(Conn, vol, DIRDID_ROOT, temp, bitmap, 0);
#ifdef OLD_MANGLING

    if ((Conn->afp_version >= 30 && ret != ntohl(AFPERR_NOOBJ))
            || (Conn->afp_version < 30 && ret)) {
        test_failed();
    }

#else

    if (ret) {
        test_failed();
    }

#endif
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetFileDirParms:test324: long file name >31 bytes");
}


/* ------------------------- */
STATIC void test326() {
    char *name = "t326 long filename and extension .longtxt";
    uint16_t vol = VolID;
    uint16_t bitmap = 0;
    int ret;
    int id;
    ENTER_TEST
    ret = FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name);

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    if (Conn->afp_version >= 30) {
        bitmap = (1 << FILPBIT_PDINFO);
    } else {
        // FIXME: fails with afp 2.x despite the comment below
        bitmap = (1 << DIRPBIT_LNAME);
        test_skipped(T_AFP3);
        goto test_exit;
    }

    /* hack if filename < 255 it works with afp 2.x too */
    id = get_fid(Conn, vol, DIRDID_ROOT, name);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_nottested();
        goto test_exit;
    }

#if 0
    else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
        result = (Conn->afp_version >= 30) ? filedir.utf8_name : filedir.lname;

        if (strcmp(result, name)) {
            test_failed();
        }
    }

#endif
    sprintf(temp1, "#%X.txt", ntohl(id));
    memset(temp, 0, sizeof(temp));
    strncpy(temp, name, 31 - strlen(temp1));
    strcat(temp, temp1);
    /* for afp3 it's not valid mangled filename */
    ret = FPGetFileDirParams(Conn, vol, DIRDID_ROOT, temp, bitmap, 0);

    if ((Conn->afp_version >= 30 && ret != ntohl(AFPERR_NOOBJ))
            || (Conn->afp_version < 30 && ret)) {
        test_failed();
    }

test_exit:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    exit_test("FPGetFileDirParms:test326: long file name >31 bytes");
}

/* ------------------------- */
STATIC void test333() {
    char *name = "t333 very long filename (more than 31 bytes).txt";
    uint16_t vol = VolID;
    DSI *dsi;
    uint16_t bitmap = 0;
    int ret;
    int id;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Conn->afp_version >= 30) {
        bitmap = (1 << FILPBIT_PDINFO);
    } else {
        if (Mac) { /* a Mac AFP 2.x can't create filename longer than 31 bytes*/
            test_skipped(T_MAC);
            goto test_exit;
        }

        bitmap = (1 << DIRPBIT_LNAME);
    }

    ret = FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name);

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    /* hack if filename < 255 it works with afp 2.x too */
    id = get_fid(Conn, vol, DIRDID_ROOT, name);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_nottested();
    }

    sprintf(temp1, "#%X.txt", ntohl(id));
    memset(temp, 0, sizeof(temp));
    strncpy(temp, name, 31 - strlen(temp1));
    strcat(temp, temp1);
    /* for afp3 it's not valid mangled filename */
    ret = FPGetFileDirParams(Conn, vol, DIRDID_ROOT, temp, bitmap, 0);
#ifdef OLD_MANGLING

    if ((Conn->afp_version >= 30 && ret != ntohl(AFPERR_NOOBJ))
            || (Conn->afp_version < 30 && ret)) {
        test_failed();
    }

#else

    if (ret) {
        test_failed();
    }

#endif
    sprintf(temp1, "#0%X.txt", ntohl(id));
    memset(temp, 0, sizeof(temp));
    strncpy(temp, name, 31 - strlen(temp1));
    strcat(temp, temp1);
    ret = FPGetFileDirParams(Conn, vol, DIRDID_ROOT, temp, bitmap, 0);

    if (!ret) {
        test_failed();
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetFileDirParms:test333: long file name >31 bytes");
}

/* ------------------------- */
STATIC void test334() {
    char *name = "t334 very long filename (more than 31 bytes)";
    uint16_t vol = VolID;
    DSI *dsi;
    uint16_t bitmap = 0;
    int ret;
    int id;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Conn->afp_version >= 30) {
        bitmap = (1 << FILPBIT_PDINFO);
    } else {
        if (Mac) { /* a Mac AFP 2.x can't create filename longer than 31 bytes*/
            test_skipped(T_MAC);
            goto test_exit;
        }

        bitmap = (1 << DIRPBIT_LNAME);
    }

    ret = FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name);

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    /* hack if filename < 255 it works with afp 2.x too */
    id = get_fid(Conn, vol, DIRDID_ROOT, name);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_nottested();
    }

    sprintf(temp1, "#%X", ntohl(id));
    memset(temp, 0, sizeof(temp));
    strncpy(temp, name, 31 - strlen(temp1));
    strcat(temp, temp1);
    /* for afp3 it's not valid mangled filename */
    ret = FPGetFileDirParams(Conn, vol, DIRDID_ROOT, temp, bitmap, 0);
#ifdef OLD_MANGLING

    if ((Conn->afp_version >= 30 && ret != ntohl(AFPERR_NOOBJ))
            || (Conn->afp_version < 30 && ret)) {
        test_failed();
    }

#else

    if (ret) {
        test_failed();
    }

#endif
    sprintf(temp1, "#%X.", ntohl(id));
    memset(temp, 0, sizeof(temp));
    strncpy(temp, name, 31 - strlen(temp1));
    strcat(temp, temp1);
    ret = FPGetFileDirParams(Conn, vol, DIRDID_ROOT, temp, bitmap, 0);

    if (!ret) {
        test_failed();
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetFileDirParms:test334: long file name >31 bytes (no ext)");
}

/* ------------------------- */
STATIC void test335() {
    char *name = "t335 very long filename (more than 31 bytes).txt";
    char *ndir = "t335 dir";
    uint16_t vol = VolID;
    DSI *dsi;
    unsigned int  dir;
    uint16_t bitmap = 0;
    int ret;
    int id;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Conn->afp_version >= 30) {
        bitmap = (1 << FILPBIT_PDINFO);
    } else {
        if (Mac) { /* a Mac AFP 2.x can't create filename longer than 31 bytes*/
            test_skipped(T_MAC);
            goto test_exit;
        }

        bitmap = (1 << DIRPBIT_LNAME);
    }

    dir  = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    ret = FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name);

    if (ret) {
        test_nottested();
        goto fin;
    }

    /* hack if filename < 255 it works with afp 2.x too */
    id = get_fid(Conn, vol, DIRDID_ROOT, name);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_nottested();
    }

    sprintf(temp1, "#%X.txt", ntohl(id));
    memset(temp, 0, sizeof(temp));
    strncpy(temp, name, 31 - strlen(temp1));
    strcat(temp, temp1);
    /* for afp3 it's not valid mangled filename */
    ret = FPGetFileDirParams(Conn, vol, DIRDID_ROOT, temp, bitmap, 0);
#ifdef OLD_MANGLING

    if ((Conn->afp_version >= 30 && ret != ntohl(AFPERR_NOOBJ))
            || (Conn->afp_version < 30 && ret)) {
        test_failed();
    }

#else

    if (ret) {
        test_failed();
    }

#endif
    ret = FPCreateFile(Conn, vol, 0, dir, temp);

    if (ret) {
        test_failed();
    }

    FAIL(FPGetFileDirParams(Conn, vol, dir, temp, bitmap, 0))
    FAIL(FPDelete(Conn, vol, dir, temp))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
fin:
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPGetFileDirParms:test335: long file name >31 bytes");
}

/* -------------------------
 * for this test you need
.         "????"  "????"
.pdf      "PDF "  "CARO"
 * in extmap.conf
 */
STATIC void test371() {
    char *name  = "t371 file name";
    char *name1  = "t371 new name.pdf";
    uint16_t vol = VolID;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    DSI *dsi = &Conn->dsi;
    uint16_t bitmap;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto fin;
    }

    bitmap = (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_ATTR) | (1 << FILPBIT_FINFO) |
             (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
             (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
        goto fin;
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

        if (memcmp(filedir.finder_info, "????????", 8)) {
            if (!Quiet) {
                fprintf(stdout, "FAILED not default type\n");
            }

            test_failed();
            goto fin;
        }
    }

    FAIL(FPRename(Conn, vol, DIRDID_ROOT, name, name1))

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name1, bitmap, 0)) {
        test_failed();
        goto fin;
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

        if (memcmp(filedir.finder_info, "PDF CARO", 8)) {
            if (!Quiet) {
                fprintf(stdout, "FAILED not PDF\n");
            }

            test_failed();
        }
    }

fin:
    FPDelete(Conn, vol, DIRDID_ROOT, name);
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
test_exit:
    exit_test("FPGetFileDirParms:test371: check default type");
}

/* -------------------------
 * for this test you need
.doc      "WDBN"  "MSWD"      Word Document
in extmap.conf
*/
STATIC void test380() {
    char *name  = "t380 file name.doc";
    uint16_t vol = VolID;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    DSI *dsi = &Conn->dsi;
    uint16_t bitmap;
    uint16_t bitmap1 = (1 << FILPBIT_ATTR) | (1 << FILPBIT_FINFO) |
                       (1 << FILPBIT_CDATE) |
                       (1 << FILPBIT_BDATE) | (1 << FILPBIT_MDATE);
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto fin;
    }

    bitmap = (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_ATTR) | (1 << FILPBIT_FINFO) |
             (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
             (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
        goto fin;
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

        if (memcmp(filedir.finder_info, "WDBNMSWD", 8)) {
            if (!Quiet) {
                fprintf(stdout, "FAILED not default type\n");
            }

            test_failed();
            goto fin;
        }
    }

    FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap1, &filedir))

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
        goto fin;
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

        if (memcmp(filedir.finder_info, "WDBNMSWD", 8)) {
            if (!Quiet) {
                fprintf(stdout, "FAILED not WDBNMSWD\n");
            }

            test_failed();
        }
    }

fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetFileDirParms:test380: check type mapping");
}

/* ------------------------- */
STATIC void test396() {
    char *name = "t396 dir";
    uint16_t vol = VolID;
    DSI *dsi;
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t f_bitmap = 0x73f;
    uint16_t d_bitmap = 0x133f;
    unsigned int dir;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_failed();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT_PARENT, Vol, f_bitmap,
                           d_bitmap)) {
        test_nottested();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, d_bitmap);
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, f_bitmap, d_bitmap)) {
        test_nottested();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, d_bitmap);
    }

    FPCreateID(Conn, vol, dir, "");
    FPCreateID(Conn, vol, DIRDID_ROOT, "");
    FPCreateID(Conn, vol, DIRDID_ROOT_PARENT, "");
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPGetFileDirParms:test396: dir root attribute");
}

/* -------------------------- */
STATIC void test423() {
    uint16_t vol = VolID;
    int  dir;
    char *name  = "t423 file";
    char *name2 = "t423 file new name";
    char *name1 = "t423 dir";
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << FILPBIT_FNUM) | (1 << DIRPBIT_FINFO);
    struct afp_filedir_parms filedir;
    int fid = 0;
    uint16_t fork = 0;
    DSI *dsi = &Conn->dsi;
    ENTER_TEST

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name1))) {
        test_failed();
        goto test_exit;
    }

    FAIL(FPCreateFile(Conn, vol, 0, dir, name))
    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, dir, name,
                      OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    if (FPByteLock(Conn, fork, 0, 0 /* set */, 0, 100)) {
        test_failed();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
        test_failed();
        goto fin;
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
        fid = filedir.did;
        FAIL(FPResolveID(Conn, vol, filedir.did, bitmap))
    }

    FAIL(FPMoveAndRename(Conn, vol, dir, dir, name, name2))

    if (FPGetFileDirParams(Conn, vol, dir, name2, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

        if (fid != filedir.did) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED FPGetFileDirParams id differ %x %x\n", fid,
                        filedir.did);
            }

            test_failed();
        } else {
            FAIL(FPResolveID(Conn, vol, filedir.did, bitmap))
        }
    }

fin:
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(FPDelete(Conn, vol, dir, name2))
    FPDelete(Conn, vol, dir, name);
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPGetFileDirParms:test420: an open file is renamed");
}

/* ----------- */
void FPGetFileDirParms_test() {
    ENTER_TESTSET
    test44();
    test58();
    test70();
    test94();
    test104();
    test132();
    test194();
    test229();
    test307();
    test308();
    test319();
    test324();
    test326();
    test333();
    test334();
    test335();
    test371();
    test380();
    test396();
    test423();
}
