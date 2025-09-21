/* ----------------------------------------------
*/
#include <stdarg.h>

#include "afphelper.h"
#include "specs.h"

/* -------------------------- */
#if 0
static char afp_cmd_with_fork[] = {
    AFP_BYTELOCK,
    AFP_CLOSEFORK,
    AFP_GETFORKPARAM,
    AFP_SETFORKPARAM,
    AFP_READ,
    AFP_FLUSHFORK,
    AFP_WRITE,
};
#endif

int Loglevel = AFP_LOG_INFO;

/*!
 * Helper for FPGetSessionToken, extracts token from reply buffer
 * @args buf    (r)  pointer to reply buffer
 * @args token  (w)  return allocated buffer with token here
 * @returns length of token, -1 on error
 */
ssize_t get_sessiontoken(const char *buf, char **token)
{
    uint32_t tmp;
    ssize_t len = -1;
    memcpy(&tmp, buf, sizeof(uint32_t));
    tmp = ntohl(tmp);
    len = tmp;

    if (len <= 0 || len > 200) {
        return -1;
    }

    if (!(*token = malloc(len))) {
        fprintf(stderr, "\tFAILED malloc(%ld) %s\n", len, strerror(errno));
        return -1;
    }

    memcpy(*token, buf + sizeof(uint32_t), len);
    return len;
}

void illegal_fork(DSI *dsi, char cmd, char *name)
{
    uint16_t vol = VolID;
    int ofs;
    int fork;
    uint16_t param;
    uint16_t bitmap = 0;

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_failed();
        return;
    }

    /* get an illegal fork descriptor */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
        return;
    }

    FAIL(FPCloseFork(Conn, fork))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    /* send the cmd with it */
    param  = fork;
    memset(dsi->commands, 0, DSI_CMDSIZ);
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_CMD;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    ofs = 0;
    dsi->commands[ofs++] = cmd;
    dsi->commands[ofs++] = 0;
    memcpy(dsi->commands + ofs, &param, sizeof(param));
    ofs += sizeof(param);
    dsi->datalen = ofs;
    dsi->header.dsi_len = htonl(dsi->datalen);
    dsi->header.dsi_code = 0;

    if (!Quiet) {
        fprintf(stdout, "---------------------\n");
        fprintf(stdout, "AFP call  %d\n\n", cmd);
    }

    my_dsi_stream_send(dsi, dsi->commands, dsi->datalen);
    my_dsi_cmd_receive(dsi);
    dump_header(dsi);

    if (ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
        if (!Quiet) {
            fprintf(stderr, "\tFAILED command %i\n", cmd);
        }

        test_failed();
    }
}

/* ---------------------- */
int get_did(CONN *conn, uint16_t vol, int dir, char *name)
{
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << DIRPBIT_DID);
    struct afp_filedir_parms filedir;
    DSI *dsi;
    dsi = &conn->dsi;
    filedir.did = 0;

    if (FPGetFileDirParams(conn, vol, dir, name, 0, bitmap)) {
        test_nottested();
        return 0;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (!filedir.did) {
        test_nottested();
    }

    return filedir.did;
}

/* ---------------------- */
int get_fid(CONN *conn, uint16_t vol, int dir, char *name)
{
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR);
    struct afp_filedir_parms filedir;
    DSI *dsi = &conn->dsi;
    filedir.did = 0;

    if (FPGetFileDirParams(conn, vol, dir, name, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

        if (!filedir.did) {
            test_failed();
        }
    }

    return filedir.did;
}

/* ---------------------- */
uint32_t get_forklen(DSI *dsi, int type)
{
    uint16_t bitmap = 0;
    int len = (type == OPENFORK_RSCS) ? (1 << FILPBIT_RFLEN) : (1 << FILPBIT_DFLEN);
    int  ofs =  sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint32_t flen;
    filedir.isdir = 0;
    bitmap = len;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    flen = (type == OPENFORK_RSCS) ? filedir.rflen : filedir.dflen;
    return flen;
}

/* ------------------------- */
void write_fork(CONN *conn, uint16_t vol, int dir, char *name, char *txt)
{
    int fork;
    uint16_t bitmap = 0;
    fork = FPOpenFork(conn, vol, OPENFORK_DATA, bitmap, dir, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        return;
    }

    if (FPWrite(conn, fork, 0, strlen(txt), txt, 0)) {
        test_failed();
    }

    FAIL(FPCloseFork(conn, fork))
}

/* ------------------------- */
void read_fork(CONN *conn, uint16_t vol, int dir, char *name, int len)
{
    int fork;
    uint16_t bitmap = 0;
    fork = FPOpenFork(conn, vol, OPENFORK_DATA, bitmap, dir, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        return;
    }

    memset(Data, 0, len + 1);

    if (FPRead(conn, fork, 0, len, Data)) {
        test_failed();
    }

    FAIL(FPCloseFork(conn, fork))
}

/* ----------------------
 * Use the second user for creating a folder with no access right
 * assume did are the same for != user
*/
int no_access_folder(uint16_t vol, int did, char *name)
{
    int ret = 0;
    int dir = 0;
    uint16_t vol2;
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << DIRPBIT_ACCESS) | (1 << DIRPBIT_UID) |
                      (1 << DIRPBIT_GID);
    struct afp_filedir_parms filedir;
    DSI *dsi, *dsi2;
    uint32_t uid;

    if (!Conn2) {
        return 0;
    }

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> Create no access folder <<<<<<<<<< \n");
    }

    dsi2 = &Conn2->dsi;
    dsi = &Conn->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        return 0;
    }

    ret = FPGetUserInfo(Conn, 1, 0, 1); /* who I am */

    if (ret) {
        test_nottested();
        goto fin;
    }

    ret = FPGetUserInfo(Conn2, 1, 0, 1); /* who I am */

    if (ret) {
        test_nottested();
        goto fin;
    }

    memcpy(&uid, dsi2->commands + sizeof(uint16_t), sizeof(uint32_t));
    uid = ntohl(uid);

    if (!(dir = FPCreateDir(Conn2, vol2, did, name))) {
        test_nottested();

        if (!(dir = get_did(Conn2, vol2, did, name))) {
            goto fin;
        }
    }

    if (FPGetFileDirParams(Conn2, vol2,  dir, "", 0, bitmap)) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi2->data + ofs, 0, bitmap);
    bitmap = (1 << DIRPBIT_ACCESS);
    filedir.access[0] = 0;
    filedir.access[1] = 0;
    filedir.access[2] = 0;
    filedir.access[3] = 3;  /* was 7 */

    if (FPSetDirParms(Conn2, vol2, dir, "", bitmap, &filedir)) {
        test_nottested();
        goto fin;
    }

    sleep(1);
    /* double check the first user can't create a dir in it */
    ret = get_did(Conn, vol, did, name);

    if (!ret) {
        if (ntohl(AFPERR_ACCESS) != dsi->header.dsi_code) {
            goto fin;
        }

        /* 1.6.x fails here, so cheat a little, it doesn't work with did=last though */
        ret = dir;
    }

    if (FPCreateDir(Conn, vol, ret, name)) {
        /* Mac OSX here does strange things
         * for when things go wrong */
        test_nottested();
        /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */
        FPEnumerate(Conn2, vol2,  DIRDID_ROOT, "",
                    (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR),
                    (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) |
                    (1 << DIRPBIT_DID) | (1 << DIRPBIT_ACCESS)
                    | (1 << DIRPBIT_UID) | (1 << DIRPBIT_GID));
        FPEnumerate(Conn, vol, ret, "",
                    (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_ATTR),
                    (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) |
                    (1 << DIRPBIT_DID) | (1 << DIRPBIT_ACCESS)
                    | (1 << DIRPBIT_UID) | (1 << DIRPBIT_GID));
        FPDelete(Conn, vol, ret, name);
        bitmap = (1 << DIRPBIT_ACCESS);
        filedir.access[0] = 0;
        filedir.access[1] = 0;
        filedir.access[2] = 0;
        filedir.access[3] = 7;  /* was 7 */
        FPSetDirParms(Conn2, vol2, dir, "", bitmap, &filedir);
        FPDelete(Conn2, vol2,  dir, name); /* dir and ret should be the same */
        ret = 0;
    }

fin:

    if (!ret && dir) {
        if (FPDelete(Conn2, vol2,  did, name)) {
            test_nottested();
        }
    }

    FPCloseVol(Conn2, vol2);

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> done <<<<<<<<<< \n");
    }

    return ret;
}

/* ---------------------- */
int group_folder(uint16_t vol, int did, char *name)
{
    int ret = 0;
    int dir = 0;
    uint16_t vol2;
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << DIRPBIT_ACCESS);
    struct afp_filedir_parms filedir;
    DSI *dsi, *dsi2;

    if (!Conn2) {
        return 0;
    }

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> Create ---rwx--- folder <<<<<<<<<< \n");
    }

    dsi2 = &Conn2->dsi;
    dsi = &Conn->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        return 0;
    }

    if (!(dir = FPCreateDir(Conn2, vol2, did, name))) {
        test_nottested();
        goto fin;
    }

    if (FPGetFileDirParams(Conn2, vol2,  dir, "", 0, bitmap)) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi2->data + ofs, 0, bitmap);
    filedir.access[0] = 0;
    filedir.access[1] = 0;
    filedir.access[2] = 7;
    filedir.access[3] = 0;

    if (FPSetDirParms(Conn2, vol2, dir, "", bitmap, &filedir)) {
        test_nottested();
        goto fin;
    }

    /* double check the first user can't create a dir in it */
    ret = get_did(Conn, vol, did, name);

    if (!ret) {
        if (ntohl(AFPERR_ACCESS) != dsi->header.dsi_code) {
            goto fin;
        }

        /* 1.6.x fails here, so cheat a little, it doesn't work with did=last though */
        ret = dir;
    }

    if (FPCreateDir(Conn, vol, ret, name)) {
        test_nottested();
        FPDelete(Conn, vol, ret, name);
        FPDelete(Conn, vol, did, name);
        ret = 0;
    }

fin:

    if (!ret && dir) {
        if (FPDelete(Conn2, vol2,  did, name)) {
            test_nottested();
        }
    }

    FPCloseVol(Conn2, vol2);

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> done <<<<<<<<<< \n");
    }

    return ret;
}

/* ----------------------
 * Use the second user for creating a folder with read only access right
 * assume did are the same for != user
*/
int read_only_folder(uint16_t vol, int did, char *name)
{
    int ret = 0;
    int dir = 0;
    uint16_t vol2;
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << DIRPBIT_ACCESS);
    struct afp_filedir_parms filedir;
    DSI *dsi2;

    if (!Conn2) {
        return 0;
    }

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> Create read only folder <<<<<<<<<< \n");
    }

    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        return 0;
    }

    if (!(dir = FPCreateDir(Conn2, vol2, did, name))) {
        test_nottested();
        goto fin;
    }

    if (FPGetFileDirParams(Conn2, vol2,  dir, "", 0, bitmap)) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi2->data + ofs, 0, bitmap);
    filedir.access[0] = 0;
    filedir.access[1] = 3;
    filedir.access[2] = 3;
    filedir.access[3] = 3;

    if (FPSetDirParms(Conn2, vol2, dir, "", bitmap, &filedir)) {
        test_nottested();
        goto fin;
    }

    /* double check the first user can't create a dir in it */
    ret = get_did(Conn, vol, did, name);

    if (FPCreateDir(Conn, vol, ret, name)) {
        test_nottested();
        FPDelete(Conn2, vol2,  ret, name);
        FPDelete(Conn2, vol2,  did, name);
        ret = 0;
    }

fin:

    if (!ret && dir) {
        if (FPDelete(Conn2, vol2,  did, name)) {
            test_nottested();
        }
    }

    FPCloseVol(Conn2, vol2);

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> done <<<<<<<<<< \n");
    }

    return ret;
}

/* ----------------------
 * Use the second user for creating a folder with read only access right
 * assume did are the same for != user
*/
int read_only_folder_with_file(uint16_t vol, int did, char *name, char *file)
{
    int ret = 0;
    int dir = 0;
    uint16_t vol2;
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << DIRPBIT_ACCESS);
    struct afp_filedir_parms filedir;
    DSI *dsi2;

    if (!Conn2) {
        return 0;
    }

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> Create folder <<<<<<<<<< \n");
    }

    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        return 0;
    }

    if (!(dir = FPCreateDir(Conn2, vol2, did, name))) {
        test_nottested();
        goto fin;
    }

    if (FPGetFileDirParams(Conn2, vol2,  dir, "", 0, bitmap)) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi2->data + ofs, 0, bitmap);

    if (FPCreateFile(Conn2, vol2,  0, dir, file)) {
        test_nottested();
        goto fin;
    }

    filedir.access[0] = 0;
    filedir.access[1] = 3;
    filedir.access[2] = 3;
    filedir.access[3] = 3;

    if (FPSetDirParms(Conn2, vol2, dir, "", bitmap, &filedir)) {
        test_nottested();
        goto fin;
    }

    /* double check the first user can't create a dir in it */
    ret = get_did(Conn, vol, did, name);

    if (FPCreateDir(Conn, vol, ret, name)) {
        test_nottested();
        FPDelete(Conn, vol, ret, name);
        FPDelete(Conn, vol, did, name);
        ret = 0;
    }

fin:

    if (!ret && dir) {
        if (FPDelete(Conn2, vol2,  did, name)) {
            test_nottested();
        }
    }

    FPCloseVol(Conn2, vol2);

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> done <<<<<<<<<< \n");
    }

    return ret;
}

/* ------------------------
 * We need to set rw perm first for .AppleDouble
*/
int delete_folder(uint16_t vol, int did, char *name)
{
    uint16_t vol2;
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << DIRPBIT_ACCESS);
    struct afp_filedir_parms filedir;
    DSI *dsi2;

    if (!Conn2) {
        return 0;
    }

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> Delete folder <<<<<<<<<< \n");
    }

    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        return 0;
    }

    if (FPGetFileDirParams(Conn2, vol2,  did, name, 0, bitmap)) {
        test_nottested();
        FPCloseVol(Conn2, vol2);
        return 0;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi2->data + ofs, 0, bitmap);
    filedir.access[0] = 0;
    filedir.access[1] = 7;
    filedir.access[2] = 7;
    filedir.access[3] = 7;

    if (FPSetDirParms(Conn2, vol2, did, name, bitmap, &filedir)) {
        test_nottested();
        FPCloseVol(Conn2, vol2);
        return 0;
    }

    if (FPDelete(Conn2, vol2,  did, name)) {
        test_nottested();
        FPCloseVol(Conn2, vol2);
        return 0;
    }

    FPCloseVol(Conn2, vol2);

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> done <<<<<<<<<< \n");
    }

    return 1;
}

/* ------------------------
 * We need to set rw perm first for .AppleDouble
*/
int delete_folder_with_file(uint16_t vol, int did, char *name, char *file)
{
    uint16_t vol2;
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << DIRPBIT_ACCESS) | (1 << DIRPBIT_DID);
    struct afp_filedir_parms filedir;
    DSI *dsi2;

    if (!Conn2) {
        return 0;
    }

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> Delete folder <<<<<<<<<< \n");
    }

    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        return 0;
    }

    if (FPGetFileDirParams(Conn2, vol2,  did, name, 0, bitmap)) {
        test_nottested();
        FPCloseVol(Conn2, vol2);
        return 0;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi2->data + ofs, 0, bitmap);
    filedir.access[0] = 0;
    filedir.access[1] = 7;
    filedir.access[2] = 7;
    filedir.access[3] = 7;
    bitmap = (1 << DIRPBIT_ACCESS);

    if (FPSetDirParms(Conn2, vol2, did, name, bitmap, &filedir)) {
        test_nottested();
        FPCloseVol(Conn2, vol2);
        return 0;
    }

    if (FPDelete(Conn2, vol2, filedir.did, file)) {
        test_nottested();
    }

    if (FPDelete(Conn2, vol2,  did, name)) {
        test_nottested();
        FPCloseVol(Conn2, vol2);
        return 0;
    }

    FPCloseVol(Conn2, vol2);

    if (!Quiet) {
        fprintf(stdout, "\t>>>>>>>> done <<<<<<<<<< \n");
    }

    return 1;
}

/* ---------------------- */
int get_vol_attrib(uint16_t vol)
{
    struct afp_volume_parms parms;
    DSI *dsi;
    dsi = &Conn->dsi;

    if (FPGetVolParam(Conn, vol, (1 << VOLPBIT_ATTR))) {
        test_nottested();
        return 0;
    }

    afp_volume_unpack(&parms, dsi->commands + sizeof(uint16_t),
                      (1 << VOLPBIT_ATTR));
    return parms.attr;
}

/* ---------------------- */
unsigned int get_vol_free(uint16_t vol)
{
    struct afp_volume_parms parms;
    DSI *dsi;
    dsi = &Conn->dsi;

    if (FPGetVolParam(Conn, vol, (1 << VOLPBIT_BFREE))) {
        test_nottested();
        return 0;
    }

    afp_volume_unpack(&parms, dsi->commands + sizeof(uint16_t),
                      (1 << VOLPBIT_BFREE));
    return parms.bfree;
}

/* ---------------------- */
int not_valid(unsigned int ret, int mac_error, int netatalk_error)
{
    if (htonl(mac_error) != ret) {
        if (!Mac) {
            if (!Quiet) {
                fprintf(stdout, "MAC RESULT: %d %s\n", mac_error, afp_error(htonl(mac_error)));
            }

            if (htonl(netatalk_error) != ret) {
                return 1;
            }
        } else if (htonl(netatalk_error) == ret) {
            if (!Quiet) {
                fprintf(stdout, "Warning MAC and Netatalk now same RESULT!\n");
            }

            return 0;
        } else {
            return 1;
        }
    } else if (!Mac && !Quiet) {
        fprintf(stdout, "Warning MAC and Netatalk now same RESULT!\n");
    }

    return 0;
}

/* ---------------------- */
static int error_in_list(unsigned int bitmap, unsigned int error)
{
    if ((BITERR_NOOBJ & bitmap) && htonl(error) == AFPERR_NOOBJ) {
        return 1;
    }

    if ((BITERR_NODIR & bitmap) && htonl(error) == AFPERR_NODIR) {
        return 1;
    }

    if ((BITERR_PARAM & bitmap) && htonl(error) == AFPERR_PARAM) {
        return 1;
    }

    if ((BITERR_BUSY & bitmap) && htonl(error) == AFPERR_BUSY) {
        return 1;
    }

    if ((BITERR_BADTYPE & bitmap) && htonl(error) == AFPERR_BADTYPE) {
        return 1;
    }

    if ((BITERR_NOITEM & bitmap) && htonl(error) == AFPERR_NOITEM) {
        return 1;
    }

    if ((BITERR_DENYCONF & bitmap) && htonl(error) == AFPERR_DENYCONF) {
        return 1;
    }

    if ((BITERR_NFILE & bitmap) && htonl(error) == AFPERR_NFILE) {
        return 1;
    }

    if ((BITERR_ACCESS & bitmap) && htonl(error) == AFPERR_ACCESS) {
        return 1;
    }

    if ((BITERR_NOID & bitmap) && htonl(error) == AFPERR_NOID) {
        return 1;
    }

    if ((BITERR_BITMAP & bitmap) && htonl(error) == AFPERR_BITMAP) {
        return 1;
    }

    if ((BITERR_MISC & bitmap) && htonl(error) == AFPERR_MISC) {
        return 1;
    }

    return 0;
}

/* ---------------------- */
static char *bitmap2text(unsigned int bitmap)
{
    static char temp[4096];
    static char temp1[4096];
    temp[0] = 0;

    if (BITERR_NOOBJ & bitmap) {
        sprintf(temp, "%d %s ", AFPERR_NOOBJ, afp_error(htonl(AFPERR_NOOBJ)));
    }

    if (BITERR_NODIR & bitmap) {
        sprintf(temp1, "%d %s ", AFPERR_NODIR, afp_error(htonl(AFPERR_NODIR)));
        strcat(temp, temp1);
    }

    if (BITERR_PARAM & bitmap) {
        sprintf(temp1, "%d %s ", AFPERR_PARAM, afp_error(htonl(AFPERR_PARAM)));
        strcat(temp, temp1);
    }

    if (BITERR_BUSY & bitmap) {
        sprintf(temp1, "%d %s ", AFPERR_BUSY, afp_error(htonl(AFPERR_BUSY)));
        strcat(temp, temp1);
    }

    if (BITERR_BADTYPE & bitmap) {
        sprintf(temp1, "%d %s ", AFPERR_BADTYPE, afp_error(htonl(AFPERR_BADTYPE)));
        strcat(temp, temp1);
    }

    if (BITERR_NOITEM & bitmap) {
        sprintf(temp1, "%d %s ", AFPERR_NOITEM, afp_error(htonl(AFPERR_NOITEM)));
        strcat(temp, temp1);
    }

    if (BITERR_DENYCONF & bitmap) {
        sprintf(temp1, "%d %s ", AFPERR_DENYCONF, afp_error(htonl(AFPERR_DENYCONF)));
        strcat(temp, temp1);
    }

    if (BITERR_NFILE & bitmap) {
        sprintf(temp1, "%d %s ", AFPERR_NFILE, afp_error(htonl(AFPERR_NFILE)));
        strcat(temp, temp1);
    }

    if (BITERR_ACCESS & bitmap) {
        sprintf(temp1, "%d %s ", AFPERR_ACCESS, afp_error(htonl(AFPERR_ACCESS)));
        strcat(temp, temp1);
    }

    if (BITERR_NOID & bitmap) {
        sprintf(temp1, "%d %s ", AFPERR_NOID, afp_error(htonl(AFPERR_NOID)));
        strcat(temp, temp1);
    }

    if (BITERR_BITMAP & bitmap) {
        sprintf(temp1, "%d %s ", AFPERR_BITMAP, afp_error(htonl(AFPERR_BITMAP)));
        strcat(temp, temp1);
    }

    if (BITERR_MISC & bitmap) {
        sprintf(temp1, "%d %s ", AFPERR_MISC, afp_error(htonl(AFPERR_MISC)));
        strcat(temp, temp1);
    }

    return temp;
}

/* ---------------------- */
int not_valid_bitmap(unsigned int ret, unsigned int bitmap, int netatalk_error)
{
    if (!Mac && !Quiet) {
        fprintf(stdout, "MAC RESULT: %s\n", bitmap2text(bitmap));
    }

    if (!error_in_list(bitmap, ret)) {
        if (htonl(netatalk_error) == ret) {
            return 0;
        }

        return 1;
    }

    return 0;
}

static int CurTestResult;
static char *Why;
#define SKIPPED_MSG_BUFSIZE 256
static char skipped_msg_buf[SKIPPED_MSG_BUFSIZE];

/* ------------------------- */
void test_skipped(int why)
{
    char *s = "";

    switch (why) {
    case T_CONN2:
        s = "second user";
        break;

    case T_PATH:
        s = "has to be run on localhost with -c volume path";
        break;

    case T_AFP2:
        s = "AFP 2.x";
        break;

    case T_AFP3:
        s = "AFP 3.x";
        break;

    case T_AFP3_CONN2:
        s = "AFP 3.x and no second user";
        break;

    case T_AFP30:
        s = "AFP 3.0 only";
        break;

    case T_AFP31:
        s = "AFP 3.1 or higher";
        break;

    case T_AFP32:
        s = "AFP 3.2 or higher";
        break;

    case T_UNIX_PREV:
        s = "volume with UNIX privileges";
        break;

    case T_NO_UNIX_PREV:
        s = "volume without UNIX privileges";
        break;

    case T_UNIX_GROUP:
        s = "server and client same groups mapping";
        break;

    case T_UTF8:
        s = "volume with UTF8 encoding";
        break;

    case T_VOL2:
        s = "a second volume";
        break;

    case T_VOL_SMALL:
        s = "a bigger volume";
        break;

    case T_ID:
        s = "AFP FileID calls";
        break;

    case T_MAC:
        s = "needs Mac OS AFP server";
        break;

    case T_ACL:
        s = "volume with ACL support";
        break;

    case T_EA:
        s = "volume with filesystem Extended Attributes support";
        break;

    case T_ADEA:
        s = "volume must not use AppleDouble metadata";
        break;

    case T_ADV2:
        s = "volume must use AppleDouble metadata";
        break;

    case T_NOSYML:
        s = "volume without option 'follow symlinks'";
        break;

    case T_SINGLE:
        s = "run individually with -f";
        break;

    case T_VOL_BIG:
        s = "a smaller volume";
        break;

    case T_MANUAL:
        s = "Interactive mode";
        break;

    case T_NONDETERM:
        s = "nondeterministic behavior";
        break;

    case T_BIGENDIAN:
        s = "not big-endian compatible";
        break;

    case T_CRED:
        s = "username/password for the AFP server";
        break;
    }

    if (Color) {
        snprintf(skipped_msg_buf, sizeof(skipped_msg_buf),
                 ANSI_BBLUE "SKIPPED (%s)" ANSI_NORMAL, s);
    } else {
        snprintf(skipped_msg_buf, sizeof(skipped_msg_buf), "SKIPPED (%s)", s);
    }

    CurTestResult = 3;
}

/* ------------------------- */
void test_failed(void)
{
    if (!Quiet) {
        fprintf(stderr, "\tFAILED\n");
    }

    ExitCode = 1;
    CurTestResult = 1;
}

/* ------------------------- */
void test_nottested(void)
{
    if (!Quiet) {
        fprintf(stderr, "\tNOT TESTED\n");
    }

    if (!ExitCode) {
        ExitCode = 2;
    }

    CurTestResult = 2;
}

/* ------------------------- */
void enter_test(void)
{
    CurTestResult = 0;
    Why = "";
}

/* ------------------------- */
void exit_test(char *name)
{
    char *s = "";

    switch (CurTestResult) {
    case 0:
        PassCount++;

        if (Color) {
            s = ANSI_BGREEN "PASSED" ANSI_NORMAL;
        } else {
            s = "PASSED";
        }

        break;

    case 1:
        strlcpy(FailedTests[FailCount], name, 255);
        FailedTests[FailCount][255] = '\0';
        FailCount++;

        if (Color) {
            s = ANSI_BRED "FAILED" ANSI_NORMAL;
        } else {
            s = "FAILED";
        }

        fprintf(stdout, "%s - ", name);
        fprintf(stdout, "%s%s\n", s, Why);
        fflush(stdout);
        return;

    case 2:
        strlcpy(NotTestedTests[NotTestedCount], name, 255);
        NotTestedTests[NotTestedCount][255] = '\0';
        NotTestedCount++;

        if (Color) {
            s = ANSI_BYELLOW "NOT TESTED" ANSI_NORMAL;
        } else {
            s = "NOT TESTED";
        }

        break;

    case 3:
        strlcpy(SkippedTests[SkipCount], name, 255);
        SkippedTests[SkipCount][255] = '\0';
        SkipCount++;
        s = skipped_msg_buf;
        break;
    }

    fprintf(stdout, "%s - ", name);
    fprintf(stdout, "%s%s\n", s, Why);
    fflush(stdout);
}

static void afp_print_prefix(int level, int color)
{
    if (color) {
        switch (level) {
        case AFP_LOG_WARNING:
            printf(ANSI_BYELLOW);
            break;

        case AFP_LOG_ERROR:
        case AFP_LOG_CRITICAL:
            printf(ANSI_BRED);
            break;

        default:
            break;
        }
    }
}

static void afp_print_postfix(int level, int color)
{
    if (color) {
        switch (level) {
        case AFP_LOG_WARNING:
        case AFP_LOG_ERROR:
        case AFP_LOG_CRITICAL:
            printf(ANSI_NORMAL);
            break;

        default:
            break;
        }
    }
}

void afp_printf(int level, int loglevel, int color, const char *fmt, ...)
{
    va_list arg;

    if ((level >= AFP_LOG_MAX) || (level < AFP_LOG_DEBUG)) {
        return;
    }

    if (level < loglevel) {
        return;
    }

    afp_print_prefix(level, color);
    va_start(arg, fmt);
    vfprintf(stdout, fmt, arg);
    va_end(arg);
    afp_print_postfix(level, color);
}

void assert_equal(intmax_t expect, intmax_t real, const char *file, int line,
                  void (*fn)(), int log_level)
{
    if (expect != real) {
        AFP_PRINTF(log_level, "%s:%d expected %" PRIdMAX ", got %" PRIdMAX "\n", file,
                   line, expect, real);
        fn();
    }
}

void assert_equal_u(uintmax_t expect, uintmax_t real, const char *file,
                    int line, void (*fn)(), int log_level)
{
    if (expect != real) {
        AFP_PRINTF(log_level, "%s:%d expected %" PRIuMAX ", got %" PRIuMAX "\n", file,
                   line, expect, real);
        fn();
    }
}

void assert_not_equal(intmax_t expect, intmax_t real, const char *file,
                      int line, void (*fn)(), int log_level)
{
    if (expect == real) {
        AFP_PRINTF(log_level, "%s:%d should not be %" PRIdMAX "\n", file, line, real);
        fn();
    }
}

void assert_not_equal_u(uintmax_t expect, intmax_t real, const char *file,
                        int line, void (*fn)(), int log_level)
{
    if (expect == real) {
        AFP_PRINTF(log_level, "%s:%d should not be %" PRIuMAX "\n", file, line, real);
        fn();
    }
}

void assert_null(const void *real, const char *file, int line, void (*fn)(),
                 int log_level)
{
    if (real != NULL) {
        AFP_PRINTF(log_level, "%s:%d should be NULL\n", file, line);
        fn();
    }
}

void assert_not_null(const void *real, char *file, int line, void (*fn)(),
                     int log_level)
{
    if (real == NULL) {
        AFP_PRINTF(log_level, "%s:%d should not be NULL\n", file, line);
        fn();
    }
}

void assert_true(int real, char *file, int line, void (*fn)(), int log_level)
{
    if (!real) {
        AFP_PRINTF(log_level, "%s:%d should be true\n", file, line);
        fn();
    }
}

void assert_false(int real, char *file, int line, void (*fn)(), int log_level)
{
    if (real) {
        AFP_PRINTF(log_level, "%s:%d should be false\n", file, line);
        fn();
    }
}

/*!
 * @brief depth-first recursively delete a directory tree
 *
 * Algorithm:
 * 1. Try simple delete (works if directory is empty)
 * 2. If not empty, get directory ID
 * 3. Enumerate contents and for each entry:
 *    - If it's a file: delete it immediately
 *    - If it's a directory: recurse into it
 * 4. After all contents are deleted, delete the now-empty directory
 *
 * @param conn       AFP connection
 * @param volume     Volume ID
 * @param parent_did Parent directory ID
 * @param dirname    Directory name to delete
 *
 * @returns 0 on success, -1 on failure
 */
int delete_directory_tree(CONN *conn, uint16_t volume,
                          uint32_t parent_did, char *dirname)
{
    struct afp_filedir_parms filedir;
    DSI *dsi_ptr = &conn->dsi;
    uint32_t dir_id;
    uint16_t f_bitmap, d_bitmap;
    unsigned int ret;
    uint16_t entry_count;
    unsigned char *entry_ptr;
    const unsigned char *data_end;

    /* Step 1: Try simple delete - optimal case for empty directories */
    if (FPDelete(conn, volume, parent_did, dirname) == AFP_OK) {
        return 0;
    }

    /* Step 2: dirname directory not empty - get its ID for enumeration */
    if (FPGetFileDirParams(conn, volume, parent_did, dirname, 0,
                           (1 << DIRPBIT_DID)) != AFP_OK) {
        /* Directory doesn't exist or access denied */
        fprintf(stderr, "[delete_directory_tree] Failed to get directory ID for '%s'\n",
                dirname);
        return -1;
    }

    /* Extract directory ID for dirname from DSI following FPGetFileDirParams call */
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi_ptr->data + (3 * sizeof(uint16_t)), 0,
                       (1 << DIRPBIT_DID));
    dir_id = filedir.did;

    /* Invalid directory ID */
    if (dir_id == 0) {
        fprintf(stderr, "[delete_directory_tree] Got invalid directory ID for '%s'\n",
                dirname);
        return -1;
    }

    /* Step 3: Enumerate and process contents - names for deletion & dir IDs for recursion */
    f_bitmap = (1 << FILPBIT_LNAME);
    d_bitmap = (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_DID);
    /* Call FPEnumerate until AFPERR_NOOBJ (no more entries) */
    int total_files_deleted = 0;
    int total_dirs_deleted = 0;
    int enumeration_round = 0;
    int max_rounds = 100;

    while (enumeration_round < max_rounds) {
        enumeration_round++;
        /* FPEnumerate batch of entries */
        ret = FPEnumerate(conn, volume, dir_id, "", f_bitmap, d_bitmap);

        if (ret == htonl(AFPERR_NOOBJ)) {
            /* No more objects - done enumerating */
            break;
        } else if (ret != AFP_OK) {
            fprintf(stderr,
                    "[delete_directory_tree] Enumeration failed for '%s' (error: 0x%08x)\n",
                    dirname, ret);
            break;
        }

        /* Process batch */
        /* WARN: FPEnumerate and FPEnumerate* use my_dsi_data_receive, other commands use my_dsi_cmd_receive.
         my_dsi_data_receive stores data length in 'cmdlen', not 'datalen'?
         my_dsi_cmd_receive (line 321) - receives data into DSI_CMDSIZ buffer (x->commands)
         my_dsi_data_receive (line 327) - receives data into DSI_DATASIZ buffer (x->data)
         So my_dsi_data_receive data is received into dsi->data buffer, not dsi->commands.
         &conn->dsi->cmdlen = datalen
         &conn->dsi->data + 0 = fbitmap
         &conn->dsi->data + 2 = dbitmap
         &conn->dsi->data + 4 = count
         &conn->dsi->data + 6 = data */
        if (dsi_ptr->cmdlen < 6) {
            fprintf(stderr, "[delete_directory_tree] Response too small (%lu bytes)\n",
                    dsi_ptr->cmdlen);
            break;
        }

        /* Get entry count (skip bitmap fields at offset 0 (fbitmap) and 2(dbitmap)) */
        memcpy(&entry_count, dsi_ptr->data + 4, sizeof(entry_count));
        entry_count = ntohs(entry_count);

        /* If no entries, we're done */
        if (entry_count == 0) {
            break;
        }

        /* Process each entry data */
        if (dsi_ptr->cmdlen > 6) {
            entry_ptr = dsi_ptr->data + 6;
            data_end = dsi_ptr->data + dsi_ptr->cmdlen;
            int batch_files_deleted = 0;
            int batch_dirs_deleted = 0;

            for (int i = 0; i < entry_count; i++) {
                /* Bounds checking */
                if (entry_ptr >= data_end || entry_ptr + 2 > data_end) {
                    fprintf(stderr,
                            "[delete_directory_tree] Reached end of data at entry %d of %u\n",
                            i, entry_count);
                    break;
                }

                /* Validate entry length */
                uint8_t entry_len = entry_ptr[0];
                uint8_t is_dir = entry_ptr[1];

                /* Validate entry length */
                if (entry_len == 0) {
                    /* Zero length means end of entries */
                    break;
                }

                if (entry_ptr + entry_len > data_end) {
                    /* Might be padding at the end of the batch */
                    if (!Quiet) {
                        fprintf(stdout,
                                "[delete_directory_tree] Warning: Entry %d length %u extends beyond buffer\n",
                                i, entry_len);
                    }

                    break;
                }

                /* Unpack entry to get name */
                memset(&filedir, 0, sizeof(filedir)); /* Clear structure first */
                filedir.isdir = is_dir;

                if (is_dir) {
                    /* Recurse subdirectory */
                    afp_filedir_unpack(&filedir, entry_ptr + 2, 0, d_bitmap);

                    if (filedir.lname && filedir.lname[0] != '\0') {
                        /* Pass current dir ID as new parent, and subdir entry name as dirname */
                        delete_directory_tree(conn, volume, dir_id, filedir.lname);
                        batch_dirs_deleted++;
                    }
                } else {
                    /* Delete file */
                    afp_filedir_unpack(&filedir, entry_ptr + 2, f_bitmap, 0);

                    if (filedir.lname && filedir.lname[0] != '\0') {
                        FPDelete(conn, volume, dir_id, filedir.lname);
                        batch_files_deleted++;
                    }
                }

                entry_ptr += entry_len;
            }

            total_files_deleted += batch_files_deleted;
            total_dirs_deleted += batch_dirs_deleted;
        }
    }

    if (!Quiet) {
        fprintf(stdout,
                "[delete_directory_tree] Total for '%s': Deleted %d files, deleted %d dirs in %d rounds\n",
                dirname, total_files_deleted, total_dirs_deleted, enumeration_round);
    }

    /* Step 4: Delete the now-empty directory */
    ret = FPDelete(conn, volume, parent_did, dirname);

    if (ret != AFP_OK) {
        /* Trying to delete dirname by its own ID */
        ret = FPDelete(conn, volume, dir_id, "");
    }

    if (!Quiet) {
        if (ret == AFP_OK) {
            fprintf(stdout, "[delete_directory_tree] Successfully deleted '%s'\n", dirname);
        } else {
            fprintf(stderr,
                    "[delete_directory_tree] Failed to delete '%s' (error: 0x%08x)\n",
                    dirname, ret);
        }
    }

    return (ret == AFP_OK) ? 0 : -1;
}
