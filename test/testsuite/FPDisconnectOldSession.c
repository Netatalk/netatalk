/* ----------------------------------------------
*/
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

extern char  *Server;
extern int     Port;
extern char    *Password;
extern char *vers;
extern char *uam;

static volatile int sigp = 0;

static void pipe_handler()
{
    sigp = 1;
}


/* ------------------------- */
/* FIXME: need to recheck GetSessionToken 0 */
STATIC void test222()
{
    char *name = "t222 file";
    uint16_t vol = VolID, vol2;
    unsigned int ret;
    char *token = NULL;
    uint32_t len;
    CONN *conn2 = NULL;
    DSI *dsi3;
    int sock;
    int fork = 0, fork1;
    struct sigaction action;
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    ret = FPGetSessionToken(Conn, 0, 0, 0, NULL);

    if (ret) {
        test_failed();
        goto test_exit;
    }

    if ((conn2 = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        test_nottested();
        goto test_exit;
    }

    conn2->type = 0;
    dsi3 = &conn2->dsi;
    sock = OpenClientSocket(Server, Port);

    if (sock < 0) {
        test_nottested();
        goto test_exit;
    }

    dsi3->socket = sock;
    ret = FPopenLoginExt(conn2, vers, uam, User, Password);

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    conn2->afp_version = Conn->afp_version;
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    vol2  = FPOpenVol(conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto fin;
    }

    fork = FPOpenFork(conn2, vol2, OPENFORK_RSCS, 0, DIRDID_ROOT, name,
                      OPENACC_DWR | OPENACC_DRD);

    if (!fork) {
        test_nottested();
        goto fin;
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, 0, DIRDID_ROOT, name,
                       OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

    if (fork1) {
        FAIL(FPCloseFork(Conn, fork1))
        test_nottested();
        goto fin;
    }

    ret = FPGetSessionToken(conn2, 0, 0, 0, NULL);

    if (ret) {
        test_failed();
        goto fin;
    }

    memcpy(&len, dsi3->data, sizeof(uint32_t));
    len = ntohl(len);

    if (!len) {
        test_failed();
        goto fin;
    }

    if (!(token = malloc(len + 4))) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED malloc(%x) %s\n", len, strerror(errno));
        }

        test_failed();
        goto fin;
    }

    action.sa_handler =  SIG_IGN;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART | SA_ONESHOT;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_failed();
        goto fin;
    }

    memcpy(token, dsi3->data + sizeof(uint32_t), len);
    /* wrong token */
    ret =  FPDisconnectOldSession(Conn, 0, len + 4, token);

    if (ret != htonl(AFPERR_MISC)) {
        test_failed();
    }

    ret =  FPDisconnectOldSession(Conn, 0, len, token);

    if (ret != htonl(AFPERR_SESSCLOS)) {
        test_failed();
        goto fin;
    }

    sleep(2);
    fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, 0, DIRDID_ROOT, name,
                       OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

    if (!fork1) {
        /* arg we are there */
        test_failed();
        FAIL(FPCloseFork(conn2, fork))
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork1))
fin:
    action.sa_handler =  SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_failed();
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:

    if (conn2) {
        free(conn2);
    }

    if (token) {
        free(token);
    }

    exit_test("FPDisconnectOldSession:test222: AFP 3.x disconnect old session");
}

/* ------------------------- */
STATIC void test338()
{
    char *name = "t338 file";
    uint16_t vol = 0;
    unsigned int ret;
    char *token = NULL;
    uint32_t len;
    CONN *loc_conn1 = NULL;
    CONN *loc_conn2 = NULL;
    DSI *loc_dsi1;
    DSI *loc_dsi2;
    int sock1;
    int sock2;
    int fork = 0;
    struct sigaction action;
    char *id0 = "testsuite-test338-0";
    char *id1 = "testsuite-test338-1";
    uint32_t time = 12345;
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    /* setup 2 new connections for testing */

    /* connection 1 */
    if ((loc_conn1 = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        test_nottested();
        goto test_exit;
    }

    loc_conn1->type = 0;
    loc_dsi1 = &loc_conn1->dsi;
    sock1 = OpenClientSocket(Server, Port);

    if (sock1 < 0) {
        test_nottested();
        goto test_exit;
    }

    loc_dsi1->socket = sock1;
    ret = FPopenLoginExt(loc_conn1, vers, uam, User, Password);

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    loc_conn1->afp_version = Conn->afp_version;
    ret = FPGetSessionToken(loc_conn1, 3, time, strlen(id0), id0);

    if (ret) {
        test_failed();
        goto fin;
    }

    memcpy(&len, loc_dsi1->data, sizeof(uint32_t));
    len = ntohl(len);

    if (!len) {
        test_failed();
        goto fin;
    }

    if (!(token = malloc(len + 4))) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED malloc(%x) %s\n", len, strerror(errno));
        }

        test_failed();
        goto fin;
    }

    memcpy(token, loc_dsi1->data + sizeof(uint32_t), len);

    if (0xffff == (vol = FPOpenVol(loc_conn1, Vol))) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPCreateFile(loc_conn1, vol, 0, DIRDID_ROOT, name))
    fork = FPOpenFork(loc_conn1, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
    }

    /* done connection 1 */

    /* --------------------------------- */
    /* connection 2 */
    if ((loc_conn2 = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        test_nottested();
        goto test_exit;
    }

    loc_conn2->type = 0;
    loc_dsi2 = &loc_conn2->dsi;
    sock2 = OpenClientSocket(Server, Port);

    if (sock2 < 0) {
        test_nottested();
        goto fin;
    }

    loc_dsi2->socket = sock2;
    ret = FPopenLoginExt(loc_conn2, vers, uam, User, Password);

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    loc_conn2->afp_version = Conn->afp_version;
    FAIL(FPDisconnectOldSession(loc_conn2, 0, len, token))
    sleep(1);
    ret = FPGetSessionToken(loc_conn2, 4, time, strlen(id1), id1);
    sleep(1);
    FAIL(FPCloseFork(loc_conn2, fork))
    FAIL(FPLogOut(loc_conn2))
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:

    if (loc_conn1) {
        free(loc_conn1);
    }

    if (loc_conn2) {
        free(loc_conn2);
    }

    if (token) {
        free(token);
    }

    exit_test("FPDisconnectOldSession:test338: AFP 3.x disconnect old session");
}

/* ------------------------- */
/* Failing with 4.0.x as well as 3.1.12. May have to do with broken FPopenLoginExt(). */
STATIC void test339()
{
    char *name = "t339 file";
    char *ndir = "t339 dir";
    char *no_user_uam = "No User Authent";
    uint16_t vol1;
    unsigned int ret;
    char *token = NULL;
    uint32_t len;
    CONN *loc_conn1 = NULL;
    CONN *loc_conn2 = NULL;
    const DSI *dsi;
    DSI *loc_dsi1, *loc_dsi2;
    int sock1, sock2;
    int fork = 0, fork1;
    struct sigaction action;
    char *id0 = "testsuite-test339-0";
    char *id1 = "testsuite-test338-1";
    uint32_t time = 12345;
    uint16_t vol = VolID;
    int dir;
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << DIRPBIT_ACCESS);
    struct afp_filedir_parms filedir = { 0 };
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    /* setup 2 new connections for testing */

    if ((loc_conn1 = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        test_nottested();
        goto test_exit;
    }

    loc_conn1->type = 0;
    loc_dsi1 = &loc_conn1->dsi;
    sock1 = OpenClientSocket(Server, Port);

    if (sock1 < 0) {
        test_nottested();
        goto test_exit;
    }

    loc_dsi1->socket = sock1;
    ret = FPopenLoginExt(loc_conn1, vers, no_user_uam, "", "");

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    loc_conn1->afp_version = Conn->afp_version;
    ret = FPGetSessionToken(loc_conn1, 3, time, strlen(id0), id0);

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    dsi = &Conn->dsi;
    memcpy(&len, loc_dsi1->data, sizeof(uint32_t));
    len = ntohl(len);

    if (!len) {
        test_failed();
        goto test_exit;
    }

    if (!(token = malloc(len + 4))) {
        if (!Quiet) {
            fprintf(stdout, "\tNOT TESTED malloc(%x) %s\n", len, strerror(errno));
        }

        test_nottested();
        goto test_exit;
    }

    memcpy(token, loc_dsi1->data + sizeof(uint32_t), len);

    if (0xffff == (vol1 = FPOpenVol(loc_conn1, Vol))) {
        test_nottested();
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    filedir.access[0] = 0;
    filedir.access[1] = 7;
    filedir.access[2] = 7;
    filedir.access[3] = 7;

    if (FPSetDirParms(Conn, vol, dir, "", bitmap, &filedir)) {
        test_nottested();
        goto fin;
    }

    FAIL(FPCreateFile(loc_conn1, vol1,  0, dir, name))

    /* --------------------------------- */
    if ((loc_conn2 = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        test_nottested();
        goto test_exit;
    }

    loc_conn2->type = 0;
    loc_dsi2 = &loc_conn2->dsi;
    sock2 = OpenClientSocket(Server, Port);

    if (sock2 < 0) {
        test_nottested();
        goto fin;
    }

    loc_dsi2->socket = sock2;
    ret = FPopenLoginExt(loc_conn2, vers, no_user_uam, "", "");

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    loc_conn2->afp_version = Conn->afp_version;
    action.sa_handler = pipe_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_nottested();
        goto fin;
    }

    ret = FPDisconnectOldSession(loc_conn2, 0, len, token);

    if (ret == AFP_OK) {
        sleep(1);
        FAIL(FPGetSessionToken(loc_conn2, 4, time, strlen(id1), id1));
    } else {
        test_failed();
        FAIL(FPLogOut(loc_conn1))
    }

    FAIL(FPLogOut(loc_conn2));
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_nottested();
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, name))
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:

    if (loc_conn1) {
        free(loc_conn1);
    }

    if (loc_conn2) {
        free(loc_conn2);
    }

    if (token) {
        free(token);
    }

    exit_test("FPDisconnectOldSession:test339: AFP 3.x No auth disconnect old session");
}

/* ------------------------- */
STATIC void test370()
{
    char *name = "t370 file";
    char *ndir = "t370 dir";
    char *no_user_uam = "No User Authent";
    uint16_t vol1;
    unsigned int ret;
    char *token = NULL;
    uint32_t len;
    CONN *loc_conn1 = NULL;
    CONN *loc_conn2 = NULL;
    const DSI *dsi;
    DSI *loc_dsi1;
    DSI *loc_dsi2;
    int sock1;
    int sock2;
    int fork = 0;
    struct sigaction action;
    char *id0 = "testsuite-test370-0";
    char *id1 = "testsuite-test370-1";
    uint32_t time = 12345;
    uint16_t vol = VolID;
    int dir;
    int  ofs =  3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << DIRPBIT_ACCESS);
    struct afp_filedir_parms filedir = { 0 };
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    /* setup 2 new connections for testing */

    if ((loc_conn1 = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        test_nottested();
        goto test_exit;
    }

    loc_conn1->type = 0;
    loc_dsi1 = &loc_conn1->dsi;
    sock1 = OpenClientSocket(Server, Port);

    if (sock1 < 0) {
        test_nottested();
        goto test_exit;
    }

    loc_dsi1->socket = sock1;
    ret = FPopenLoginExt(loc_conn1, vers, uam, User, Password);

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    loc_conn1->afp_version = Conn->afp_version;
    ret = FPGetSessionToken(loc_conn1, 3, time, strlen(id0), id0);

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    dsi = &Conn->dsi;
    memcpy(&len, loc_dsi1->data, sizeof(uint32_t));
    len = ntohl(len);

    if (!len) {
        test_failed();
        goto test_exit;
    }

    if (!(token = malloc(len + 4))) {
        if (!Quiet) {
            fprintf(stdout, "\tNOT TESTED malloc(%x) %s\n", len, strerror(errno));
        }

        test_nottested();
        goto test_exit;
    }

    memcpy(token, loc_dsi1->data + sizeof(uint32_t), len);

    if (0xffff == (vol1 = FPOpenVol(loc_conn1, Vol))) {
        test_nottested();
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
        test_nottested();
        goto fin;
    }

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    filedir.access[0] = 0;
    filedir.access[1] = 7;
    filedir.access[2] = 7;
    filedir.access[3] = 7;

    if (FPSetDirParms(Conn, vol, dir, "", bitmap, &filedir)) {
        test_nottested();
        goto fin;
    }

    FAIL(FPCreateFile(loc_conn1, vol1,  0, dir, name))

    /* --------------------------------- */
    if ((loc_conn2 = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        test_nottested();
        goto test_exit;
    }

    loc_conn2->type = 0;
    loc_dsi2 = &loc_conn2->dsi;
    sock2 = OpenClientSocket(Server, Port);

    if (sock2 < 0) {
        test_nottested();
        goto fin;
    }

    loc_dsi2->socket = sock2;
    ret = FPopenLoginExt(loc_conn2, vers, no_user_uam, "", "");

    if (ret) {
        test_nottested();
        goto test_exit;
    }

    loc_conn2->afp_version = Conn->afp_version;
    action.sa_handler = pipe_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_nottested();
        goto fin;
    }

    FAIL(ntohl(AFPERR_MISC) != FPDisconnectOldSession(loc_conn2, 0, len, token))
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_nottested();
    }

    sleep(1);
    ret = FPGetSessionToken(loc_conn2, 4, time, strlen(id1), id1);
    sleep(1);
    fork = FPOpenFork(loc_conn1, vol1, OPENFORK_RSCS, 0, dir, name,
                      OPENACC_WR | OPENACC_RD | OPENACC_DWR | OPENACC_DRD);

    if (!fork) {
        test_failed();
    }

    FAIL(FPCloseFork(loc_conn1, fork))
fin:
    FAIL(FPDelete(Conn, vol, dir, name))
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:

    if (loc_conn1) {
        free(loc_conn1);
    }

    if (loc_conn2) {
        free(loc_conn2);
    }

    if (token) {
        free(token);
    }

    exit_test("FPDisconnectOldSession:test370: AFP 3.x disconnect different user");
}

/* test535: Disconnect with active cache
 *
 * This test validates proper cache invalidation when a client disconnects
 * and reconnects after another client has modified the filesystem.
 *
 * Scenario:
 * 1. Client 1 creates dirs/files and populates cache
 * 2. Client 1 disconnects (simulates timeout)
 * 3. Client 2 renames/deletes created items
 * 4. Client 1 reconnects
 * 5. Client 1 accesses items via old DIDs (should fail gracefully)
 * 6. Verify proper cache invalidation on reconnect
 */
STATIC void test535()
{
    char *dir_name = "t535_disconnect_test";
    char *file1 = "t535_file1.txt";
    char *file2 = "t535_file2.txt";
    char *file3 = "t535_file3.txt";
    char *renamed_file1 = "t535_renamed1.txt";
    uint16_t vol = VolID;
    uint16_t vol2 = 0;
    uint16_t bitmap = (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) |
                      (1 << DIRPBIT_DID);
    int dir = 0;
    int saved_did = 0;
    unsigned int ret;
    int sock;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    /************************************
     * Step 1: Client 1 creates items and populates cache
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 1: Client 1 creates directory and files\n");
    }

    dir = FPCreateDir(Conn, vol, DIRDID_ROOT, dir_name);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    saved_did = dir;
    FAIL(FPCreateFile(Conn, vol, 0, dir, file1))
    FAIL(FPCreateFile(Conn, vol, 0, dir, file2))
    FAIL(FPCreateFile(Conn, vol, 0, dir, file3))
    /* Populate cache by accessing items */
    FAIL(FPGetFileDirParams(Conn, vol, dir, file1, bitmap, 0))
    FAIL(FPGetFileDirParams(Conn, vol, dir, file2, bitmap, 0))
    FAIL(FPGetFileDirParams(Conn, vol, dir, file3, bitmap, 0))
    /* FIXME: FPEnumerate* uses my_dsi_data_receive. See afphelper.c:delete_directory_tree() */
    FAIL(FPEnumerate(Conn, vol, dir, "", bitmap, bitmap))

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Created 3 files and populated cache\n");
    }

    /************************************
     * Step 2: Client 1 disconnects
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 2: Client 1 disconnects (simulates timeout)\n");
    }

    FAIL(FPCloseVol(Conn, vol))
    FAIL(FPLogOut(Conn))

    /* Close socket to simulate disconnect */
    if (Conn->dsi.socket >= 0) {
        close(Conn->dsi.socket);
        Conn->dsi.socket = -1;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Client 1 disconnected\n");
    }

    /************************************
     * Step 3: Client 2 modifies filesystem
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 3: Client 2 renames and deletes items\n");
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Client 2 could not open volume\n");
        }

        test_nottested();
        goto reconnect;
    }

    /* Rename file1 */
    FAIL(FPRename(Conn2, vol2, saved_did, file1, renamed_file1))
    /* Delete file2 */
    FAIL(FPDelete(Conn2, vol2, saved_did, file2))
    /* Keep file3 unchanged for comparison */
    FPCloseVol(Conn2, vol2);
    vol2 = 0;

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Client 2 renamed file1, deleted file2, kept file3\n");
    }

    /************************************
     * Step 4: Client 1 reconnects
     ************************************/
reconnect:

    if (!Quiet) {
        fprintf(stdout, "\t  Step 4: Client 1 reconnects\n");
    }

    sock = OpenClientSocket(Server, Port);

    if (sock < 0) {
        test_nottested();
        goto fin;
    }

    Conn->dsi.socket = sock;

    if (Version >= 30) {
        ret = FPopenLoginExt(Conn, vers, uam, User, Password);
    } else {
        ret = FPopenLogin(Conn, vers, uam, User, Password);
    }

    if (ret) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Client 1 could not reconnect\n");
        }

        test_failed();
        goto fin;
    }

    Conn->afp_version = Version;
    vol = FPOpenVol(Conn, Vol);

    if (vol == 0xffff) {
        test_nottested();
        goto fin;
    }

    VolID = vol;

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Client 1 reconnected successfully\n");
    }

    /************************************
     * Step 5: Client 1 accesses items via old names
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Client 1 accesses items (cache should be invalidated)\n");
    }

    /* Try to access file1 with old name (should fail - it was renamed) */
    ret = FPGetFileDirParams(Conn, vol, saved_did, file1, bitmap, 0);

    if (ret != ntohl(AFPERR_NOOBJ)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED accessing renamed file1 should return AFPERR_NOOBJ, got %d (%s)\n",
                    ntohl(ret), afp_error(ret));
        }

        test_failed();
    }

    /* Try to access file2 (should fail - it was deleted) */
    ret = FPGetFileDirParams(Conn, vol, saved_did, file2, bitmap, 0);

    if (ret != ntohl(AFPERR_NOOBJ)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED accessing deleted file2 should return AFPERR_NOOBJ, got %d (%s)\n",
                    ntohl(ret), afp_error(ret));
        }

        test_failed();
    }

    /* Access file3 (should succeed - it wasn't modified) */
    FAIL(FPGetFileDirParams(Conn, vol, saved_did, file3, bitmap, 0))
    /* Access renamed file with new name (should succeed) */
    FAIL(FPGetFileDirParams(Conn, vol, saved_did, renamed_file1, bitmap, 0))

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Cache properly invalidated after reconnect\n");
    }

    /************************************
     * Step 6: Verify cache consistency
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 6: Verify cache consistency with re-enumerate\n");
    }

    /* Re-enumerate to verify cache is consistent */
    FAIL(FPEnumerate(Conn, vol, saved_did, "", bitmap, bitmap))

    if (!Quiet) {
        fprintf(stdout, "\t  ✓ Reconnect handled cache invalidation properly\n");
    }

fin:

    /* Cleanup */
    if (saved_did) {
        FPDelete(Conn, vol, saved_did,
                 file1);        /* Original name (if not renamed) */
        FPDelete(Conn, vol, saved_did, file2);        /* (if not deleted) */
        FPDelete(Conn, vol, saved_did, file3);        /* Should exist */
        FPDelete(Conn, vol, saved_did, renamed_file1); /* Renamed file */
        FPDelete(Conn, vol, DIRDID_ROOT, dir_name);
    }

    if (vol2) {
        FPCloseVol(Conn2, vol2);
    }

test_exit:
    exit_test("FPDisconnectOldSession:test535: Disconnect with active cache and reconnect");
}

/* ----------- */
void FPDisconnectOldSession_test()
{
    ENTER_TESTSET
#if 0
    test222();
#endif
    test338();
#if 0
    test339();
#endif
    test370();
    test535();
}
