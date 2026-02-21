/* ----------------------------------------------
*/
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "adoublehelper.h"
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

static char temp[MAXPATHLEN];

/* Forward declarations */
STATIC void test526(void);

/* ------------------------- */
STATIC void test3()
{
    char *name = "t3.txt";
    uint16_t bitmap = 0;
    uint16_t fork1;
    uint16_t fork2;
    uint16_t vol = VolID;
    ENTER_TEST

    if (!Quiet) {
        fprintf(stdout,
                "FPOpenFork:test3: Checks data fork / adouble metadata refcounting\n");
    }

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                            OPENACC_WR | OPENACC_RD)) == 0) {
        test_failed();
        goto fin;
    }

    FAIL(FPWrite(Conn, fork1, 0, 2000, Data, 0))
    FAIL(FPRead(Conn, fork1, 0, 2000, Data))
    FAIL(FPCloseFork(Conn, fork1))

    if (delete_unix_md(Path, "", name)) {
        test_nottested();
        goto fin;
    }

    if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                            OPENACC_RD)) == 0) {
        test_failed();
        goto fin;
    }

    if ((fork2 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                            OPENACC_RD)) == 0) {
        FPCloseFork(Conn, fork1);
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork2))
    FAIL(FPRead(Conn, fork1, 0, 2000, Data))
    FAIL(FPCloseFork(Conn, fork1))
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test3: open data fork without metadata twice, close once, then read");
}

/* ------------------------- */
STATIC void test4()
{
    char *name = "t4.txt";
    uint16_t bitmap = 0;
    uint16_t fork1;
    uint16_t fork2;
    uint16_t vol = VolID;
    ENTER_TEST

    if (!Quiet) {
        fprintf(stdout,
                "FPOpenFork:test4: Checks reso fork / adouble metadata refcounting\n");
    }

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                            OPENACC_WR | OPENACC_RD)) == 0) {
        test_failed();
        goto fin;
    }

    FAIL(FPWrite(Conn, fork1, 0, 2000, Data, 0))
    FAIL(FPRead(Conn, fork1, 0, 2000, Data))
    FAIL(FPCloseFork(Conn, fork1))

    if (delete_unix_md(Path, "", name)) {
        test_nottested();
        goto fin;
    }

    if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                            OPENACC_WR | OPENACC_RD)) == 0) {
        test_failed();
        goto fin;
    }

    if ((fork2 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                            OPENACC_WR | OPENACC_RD)) == 0) {
        FPCloseFork(Conn, fork1);
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork2))
    FAIL(FPWrite(Conn, fork1, 0, 2000, Data, 0))
    FAIL(FPRead(Conn, fork1, 0, 2000, Data))
    FAIL(FPCloseFork(Conn, fork1))
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test4: open reso fork without metadata twice, close once, then read");
}

/* ------------------------- */
STATIC void test7()
{
    char *name = "t7.txt";
    uint16_t bitmap = 0;
    uint16_t fork1;
    uint16_t fork2;
    uint16_t vol = VolID;
    ENTER_TEST

    if (!Quiet) {
        fprintf(stdout,
                "FPOpenFork:test7: Checks fork / adouble metadata refcounting\n");
    }

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                            OPENACC_WR | OPENACC_RD)) == 0) {
        test_failed();
        goto fin;
    }

    FAIL(FPWrite(Conn, fork1, 0, 2000, Data, 0))
    FAIL(FPRead(Conn, fork1, 0, 2000, Data))
    FAIL(FPCloseFork(Conn, fork1))

    if (delete_unix_md(Path, "", name)) {
        test_nottested();
        goto fin;
    }

    if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                            OPENACC_WR | OPENACC_RD)) == 0) {
        test_failed();
        goto fin;
    }

    if ((fork2 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                            OPENACC_RD | OPENACC_WR)) == 0) {
        FPCloseFork(Conn, fork1);
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork2))
    FAIL(FPRead(Conn, fork1, 0, 2000, Data))
    FAIL(FPCloseFork(Conn, fork1))
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test7: open data fork without metadata, then open and close resource fork");
}

/* ------------------------- */
STATIC void test47()
{
    char *name = "t47 folder";
    char *file = "t47 file.txt";
    uint16_t vol = VolID;
    uint16_t bitmap = 0;
    uint16_t fork = 0;
    uint16_t fork1 = 0;
    int dir;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!folder_with_ro_adouble(vol, DIRDID_ROOT, name, file)) {
        test_nottested();
        goto test_exit;
    }

    dir = get_did(Conn, vol, DIRDID_ROOT, name);
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, dir, file, OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, dir, file,
                       OPENACC_WR | OPENACC_RD);

    if (fork1) {
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, dir, file, OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
        test_failed();
        goto fin;
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, dir, file,
                       OPENACC_WR | OPENACC_RD);

    if (fork1) {
        test_failed();
        goto fin;
    }

    if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
        test_failed();
        goto fin;
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, dir, file, OPENACC_RD);

    if (!fork1) {
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork1))
    fork1 = 0;

    if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = 0;
#if 0
    fprintf(stdout, "test47: in a read/write folder\n");
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT,
                      "test folder/toto.txt", OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT,
                       "test folder/toto.txt", OPENACC_WR | OPENACC_RD);

    if (fork1) {
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = 0;
    strcpy(temp, Path);
    strcat(temp, "/test folder/.AppleDouble/toto.txt");

    if (unlink(temp)) {
        fprintf(stdout, "\tFAILED Resource fork not there\n");
        test_failed();
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT,
                      "test folder/toto.txt", OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT,
                       "test folder/toto.txt", OPENACC_WR | OPENACC_RD);

    if (fork1) {
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = 0;
    strcpy(temp, Path);
    strcat(temp, "/test folder/.AppleDouble/toto.txt");

    if (unlink(temp)) {
        fprintf(stdout, "\tFAILED Resource fork not there\n");
        test_failed();
        goto fin;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT,
                      "test folder/toto.txt", OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
        test_failed();
        goto fin;
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT,
                       "test folder/toto.txt", OPENACC_WR | OPENACC_RD);

    /* bad, but we are able to open read-write the resource for of a read-only file (data fork)
     * difficult to fix.
     */
    if (!fork1) {
        test_failed();
        goto fin;
    }

    if (FPWrite(Conn, fork1, 0, 10, Data, 0)) {
        test_failed();
        goto fin;
    }

    if (FPRead(Conn, fork, 0, 10, Data)) {
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = 0;

    if (FPWrite(Conn, fork1, 0, 20, Data, 0x80)) {
        test_failed();
        goto fin;
    }

    if (ntohl(AFPERR_PARAM) != FPRead(Conn, fork, 0, 30, Data)) {
        test_failed();
    }

#endif
fin:

    if (fork1) {
        FPCloseFork(Conn, fork1);
    }

    if (fork) {
        FPCloseFork(Conn, fork);
    }

    delete_ro_adouble(vol, dir, file);
test_exit:
    exit_test("FPOpenFork:test47: open read only file read only then read write in a read only folder");
}

/* ------------------------- */
STATIC void test49()
{
    char *name = "t49 folder";
    char *file = "t49 file.txt";
    uint16_t bitmap = 0;
    uint16_t fork;
    uint16_t fork1;
    uint16_t vol = VolID;
    int dir;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, file)) {
        test_nottested();
        goto fin;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, dir, file, OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    snprintf(temp, sizeof(temp), "%s/%s/.AppleDouble/%s", Path, name, file);
    unlink(temp);
    fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, dir, file,
                       OPENACC_WR | OPENACC_RD);

    if (!fork1) {
        test_failed();
    }

    FAIL(FPCloseFork(Conn, fork))
    FAIL(FPCloseFork(Conn, fork1))

    if (!unlink(temp) && !Quiet) {
        fprintf(stdout, "\tResource fork there!\n");
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, dir, file,
                       OPENACC_WR | OPENACC_RD);

    if (!fork1) {
        test_failed();
    }

    FAIL(FPCloseFork(Conn, fork1))

    if (!unlink(temp) && !Quiet) {
        fprintf(stdout, "\tResource fork there!\n");
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, file))
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPOpenFork:test49: open read-write file without resource fork in a read-write folder");
}

/* ------------------------- */
STATIC void test152()
{
    int dir;
    uint16_t bitmap = 0;
    char *name = "t152 ro AppleDouble";
    char *file = "t152 test.pdf";
    uint16_t fork;
    uint16_t vol = VolID;
    const DSI *dsi = &Conn->dsi;
    unsigned int ret;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (adouble == AD_EA) {
        test_skipped(T_ADV2);
        goto test_exit;
    }

    if (!folder_with_ro_adouble(vol, DIRDID_ROOT, name, file)) {
        test_nottested();
        goto test_exit;
    }

    dir = get_did(Conn, vol, DIRDID_ROOT, name);
    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, dir, file,
                      OPENACC_RD | OPENACC_WR);
    ret = dsi->header.dsi_code;

    if (not_valid(ret, 0, AFPERR_LOCK)) {
        test_failed();
    }

    if (ret != htonl(AFPERR_LOCK)) {
        test_failed();
    }

    if (fork) {
        FPCloseFork(Conn, fork);
    }

    delete_ro_adouble(vol, dir, file);
test_exit:
    exit_test("FPOpenFork:test152: Error when no write access to .AppleDouble");
}

/* ------------------------- */
STATIC void test153()
{
    char *name = "t153.txt";
    uint16_t bitmap = 0;
    uint16_t fork;
    uint16_t vol = VolID;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if (delete_unix_md(Path, "", name)) {
        test_nottested();
        goto fin;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    FAIL(FPWrite(Conn, fork, 0, 2000, Data, 0))
    FAIL(FPRead(Conn, fork, 0, 2000, Data))
    FAIL(FPCloseFork(Conn, fork))
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test153: open data fork without resource fork");
}

/* ------------------------- */
STATIC void test157()
{
    char *name = "t157.txt";
    uint16_t bitmap = 0;
    uint16_t fork;
    uint16_t vol = VolID;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if (delete_unix_md(Path, "", name)) {
        test_nottested();
        goto fin;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    if (FPRead(Conn, fork, 0, 2000, Data) != ntohl(AFPERR_EOF)) {
        test_failed();
    }

    FAIL(FPCloseFork(Conn, fork))
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test157: open not existing resource fork read-only");
}

/* ------------------------- */
STATIC void test156()
{
    int dir;
    uint16_t bitmap = 0;
    uint16_t fork;
    char *name = "t156 ro AppleDouble";
    char *file = "t156 test.pdf";
    uint16_t vol = VolID;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!(dir = folder_with_ro_adouble(vol, DIRDID_ROOT, name, file))) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, dir, file,
                      OPENACC_RD | OPENACC_WR);

    if (!fork) {
        test_failed();
        goto fin;
    }

    if (fork && FPCloseFork(Conn, fork)) {
        test_failed();
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, dir, file,
                      OPENACC_RD | OPENACC_WR);

    if (!fork) {
        test_failed();
    }

    if (fork && FPCloseFork(Conn, fork)) {
        test_failed();
    }

fin:
    delete_ro_adouble(vol, dir, file);
test_exit:
    exit_test("FPOpenFork:test156: Open data fork with no write access to .AppleDouble");
}

/* ------------------------- */
STATIC void test321()
{
    uint16_t bitmap = 0;
    uint16_t fork;
    char *file = "t321 test.txt";
    uint16_t vol = VolID;
    int fd;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (adouble == AD_EA) {
        test_skipped(T_ADV2);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file)) {
        test_nottested();
        goto test_exit;
    }

    snprintf(temp, sizeof(temp), "%s/%s", Path, file);

    if (chmod(temp, 0444) < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to chmod %s :%s\n", temp, strerror(errno));
        }

        test_failed();
        goto fin;
    }

    snprintf(temp, sizeof(temp), "%s/.AppleDouble/%s", Path, file);

    if (!Quiet) {
        fprintf(stdout, "unlink %s \n", temp);
    }

    unlink(temp);
    fd = open(temp, O_RDWR | O_CREAT, 0666);

    if (fd < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to create %s :%s\n", temp, strerror(errno));
        }

        test_failed();
        goto fin;
    }

    close(fd);

    if (chmod(temp, 0444) < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to chmod %s :%s\n", temp, strerror(errno));
        }

        test_failed();
        goto fin;
    }

    bitmap = 0x693f;

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, file, bitmap, 0)) {
        test_failed();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, file, bitmap, 0)) {
        test_failed();
        goto fin;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, 0, DIRDID_ROOT, file,
                      OPENACC_RD | OPENACC_DWR);

    if (!fork) {
        test_failed();
    }

    if (fork && FPCloseFork(Conn, fork)) {
        test_failed();
    }

fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, file))
test_exit:
    exit_test("FPOpenFork:test321: Bogus (empty) resource fork");
}

/* --------------------- */
STATIC void test372()
{
    char *name = "t372 file name.txt";
    char data[20];
    uint16_t vol = VolID;
    uint16_t fork;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    const DSI *dsi = &Conn->dsi;
    uint16_t bitmap;
    int fd;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

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
        memcpy(filedir.finder_info, "TEXTttxt", 8);
        FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, (1 << FILPBIT_FINFO),
                             &filedir))
        FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0))
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    if (FPWrite(Conn, fork, 0, 5, "test\r", 0)) {
        test_failed();
        goto fin1;
    }

    if (FPRead(Conn, fork, 0, 5, data)) {
        test_failed();
        goto fin1;
    }

    if (memcmp(data, "test\r", 5)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED wrote \"test\\r\" get \"%s\"\n", data);
        }

        test_failed();
    }

    snprintf(temp, sizeof(temp), "%s/%s", Path, name);
    fd = open(temp, O_RDWR, 0666);

    if (fd < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to open %s :%s\n", temp, strerror(errno));
        }

        test_failed();
        goto fin1;
    }

    if (read(fd, data, 5) != 5) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to read data:%s\n", strerror(errno));
        }

        test_failed();
    }

    if (memcmp(data, "test\r", 5)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED not \"test\\r\" get 0x%x 0x%x 0x%x 0x%x 0x%x\n",
                    data[0], data[1], data[2], data[3], data[4]);
        }

        test_failed();
    }

    close(fd);
fin1:
    FAIL(FPCloseFork(Conn, fork))
fin:
    FPDelete(Conn, vol, DIRDID_ROOT, name);
test_exit:
    exit_test("FPRead:test372: no crlf convertion for TEXT file");
}

/* --------------------- */
STATIC void test388()
{
    char *name = "t388 file name.rtf";
    char data[20];
    uint16_t vol = VolID;
    uint16_t fork;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    const DSI *dsi = &Conn->dsi;
    uint16_t bitmap;
    int fd;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

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
        memcpy(filedir.finder_info, "TEXTttxt", 8);
        FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, (1 << FILPBIT_FINFO),
                             &filedir))
        FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0))
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    if (FPWrite(Conn, fork, 0, 5, "test\r", 0)) {
        test_failed();
        goto fin1;
    }

    if (FPRead(Conn, fork, 0, 5, data)) {
        test_failed();
        goto fin1;
    }

    if (memcmp(data, "test\r", 5)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED wrote \"test\\r\" get \"%s\"\n", data);
        }

        test_failed();
    }

    snprintf(temp, sizeof(temp), "%s/%s", Path, name);
    fd = open(temp, O_RDWR, 0666);

    if (fd < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to open %s :%s\n", temp, strerror(errno));
        }

        test_failed();
        goto fin1;
    }

    if (read(fd, data, 5) != 5) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to read data:%s\n", strerror(errno));
        }

        test_failed();
    }

    if (memcmp(data, "test\n", 5)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED not \"test\\n\" get 0x%x 0x%x 0x%x 0x%x 0x%x\n",
                    data[0], data[1], data[2], data[3], data[4]);
        }

        test_failed();
    }

    close(fd);
fin1:
    FAIL(FPCloseFork(Conn, fork))
fin:
    FPDelete(Conn, vol, DIRDID_ROOT, name);
test_exit:
    exit_test("FPRead:test388: crlf convertion for TEXT file (not default type)");
}

/* --------------------- */
STATIC void test392()
{
    char *name = "t392 file name.pdf";
    char data[20];
    uint16_t vol = VolID;
    uint16_t fork;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    const DSI *dsi = &Conn->dsi;
    uint16_t bitmap;
    int fd;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

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
        memcpy(filedir.finder_info, "PDF CARO", 8);
        FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, (1 << FILPBIT_FINFO),
                             &filedir))
        FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0))
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    if (FPWrite(Conn, fork, 0, 5, "test\r", 0)) {
        test_failed();
        goto fin1;
    }

    if (FPRead(Conn, fork, 0, 5, data)) {
        test_failed();
        goto fin1;
    }

    if (memcmp(data, "test\r", 5)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED wrote \"test\\r\" get \"%s\"\n", data);
        }

        test_failed();
    }

    snprintf(temp, sizeof(temp), "%s/%s", Path, name);
    fd = open(temp, O_RDWR, 0666);

    if (fd < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to open %s :%s\n", temp, strerror(errno));
        }

        test_failed();
        goto fin1;
    }

    if (read(fd, data, 5) != 5) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to read data:%s\n", strerror(errno));
        }

        test_failed();
    }

    if (memcmp(data, "test\r", 5)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED not \"test\\r\" get 0x%x 0x%x 0x%x 0x%x 0x%x\n",
                    data[0], data[1], data[2], data[3], data[4]);
        }

        test_failed();
    }

    close(fd);
fin1:
    FAIL(FPCloseFork(Conn, fork))
fin:
    FPDelete(Conn, vol, DIRDID_ROOT, name);
test_exit:
    exit_test("FPRead:test392: no crlf convertion for no TEXT file");
}

/* ------------------------- */
STATIC void test411()
{
    char *name = "t411 folder";
    char *file = "t411 file.txt";
    uint16_t bitmap = 0;
    uint16_t fork;
    uint16_t vol = VolID;
    int dir;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, file)) {
        test_nottested();
        goto fin;
    }

    delete_unix_md(Path, name, file);
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, dir, file, OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork))

    if (delete_unix_md(Path, name, file) == 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Resource fork there!\n");
        }

        test_failed();
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, file))
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPOpenFork:test411: open read-only a file without resource fork");
}

/* -------------------------
   Didn't fail but help when tracing afpd
*/
STATIC void test415()
{
    char *name = "t415 folder";
    char *file = "t415 file.txt";
    uint16_t bitmap = 0;
    uint16_t fork;
    uint16_t fork1;
    uint16_t vol = VolID;
    int dir;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, dir, file)) {
        test_nottested();
        goto fin;
    }

    snprintf(temp, sizeof(temp), "%s/%s/.AppleDouble/%s", Path, name, file);

    if (unlink(temp)) {
        test_nottested();
        goto fin;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, dir, file,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, dir, file,
                       OPENACC_WR | OPENACC_RD);

    if (!fork1) {
        test_failed();
        goto fin1;
    }

    FAIL(FPCloseFork(Conn, fork1))
fin1:
    FAIL(FPCloseFork(Conn, fork))
fin:
    FAIL(FPDelete(Conn, vol, dir, file))
    FAIL(FPDelete(Conn, vol, dir, ""))
test_exit:
    exit_test("FPOpenFork:test415: don't set the name again in the resource fork if file open twice");
}

/* ------------------------- */
STATIC void test236()
{
    char *name1 = "t236 dir";
    char *name2 = "etc";
    char *name3 = "t236 dir/etc";
    char *name4 = "passwd";
    int testdir, etcdir;
    uint16_t fork = 0;
    uint16_t vol = VolID, bitmap;
    struct afp_filedir_parms filedir = { 0 };
    const DSI *dsi = &Conn->dsi;
    int ofs = 3 * sizeof(uint16_t);
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    bitmap = (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_ATTR) | (1 << FILPBIT_FINFO) |
             (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
             (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID);
    /* create "t236 dir" */
    testdir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name1);

    if (!testdir) {
        test_nottested();
        goto test_exit;
    }

    /* create "t236 dir/etc" */
    etcdir = FPCreateDir(Conn, vol, testdir, name2);

    if (!etcdir) {
        test_nottested();
        goto fin;
    }

    /* make sure the server has "etc" in the dircache */
    if (FPGetFileDirParams(Conn, vol, etcdir, "", 0, (1 << DIRPBIT_DID))) {
        test_failed();
    }

    /* make sure the server's curdir is *not* etc */
    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, (1 << DIRPBIT_DID))) {
        test_failed();
    }

    /* remove "etc" */
    if (delete_unix_dir(Path, name3)) {
        test_failed();
        goto fin;
    }

    /* symlink "etc" to "/etc/passwd" */
    if (symlink_unix_file("/etc", Path, name3)) {
        test_failed();
        goto fin;
    }

    /* Ok, we've malicously prepared the dircache and filesystem, now confront afpd with it */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, etcdir, name4, OPENACC_RD);

    if (fork) {
        /* Check if the volume uses option 'followsymlinks' */
        if (FPGetFileDirParams(Conn, vol, etcdir, "", 1 << FILPBIT_UNIXPR, 0)) {
            test_failed();
        }

        afp_filedir_unpack(&filedir, dsi->data + ofs, 1 << FILPBIT_UNIXPR, 0);

        if (!(S_ISLNK(filedir.unix_priv))) {
            test_skipped(T_NOSYML);
            goto fin;
        }

        test_failed();
        goto fin;
    }

fin:
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(unlink_unix_file(Path, name1, name2))
    FAIL(testdir && FPDelete(Conn, vol, testdir, ""))
test_exit:
    exit_test("FPOpenFork:test236: symlink attack: try reading /etc/passwd");
}

/* ------------------------- */
STATIC void test237()
{
    char *name1 = "t237 dir";
    char *name2 = "passwd";
    char *name3 = "t237 dir/passwd";
    char *name4 = "/etc/passwd";
    int name4_len = 11;
    int testdir;
    uint16_t fork = 0;
    uint16_t vol = VolID, bitmap;
    struct afp_filedir_parms filedir = { 0 };
    const DSI *dsi = &Conn->dsi;
    int ofs = 3 * sizeof(uint16_t);
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    bitmap = (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_ATTR) | (1 << FILPBIT_FINFO) |
             (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
             (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID);
    /* create "t237 dir" */
    testdir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name1);

    if (!testdir) {
        test_nottested();
        goto test_exit;
    }

    /* symlink "etc" to "/etc/passwd" */
    if (symlink_unix_file(name4, Path, name3)) {
        test_failed();
        goto fin;
    }

    /* Check if the volume uses option 'followsymlinks' */
    if (FPGetFileDirParams(Conn, vol, testdir, name2, 1 << FILPBIT_UNIXPR, 0)) {
        test_failed();
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 1 << FILPBIT_UNIXPR, 0);

    if (!(S_ISLNK(filedir.unix_priv))) {
        test_skipped(T_NOSYML);
        goto fin;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, testdir, name2, OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    if (FPRead(Conn, fork, 0, name4_len, Data)) {
        test_failed();
        goto fin;
    }

    Data[11] = 0;

    if (!Quiet) {
        fprintf(stdout, "readlink: %s\n", Data);
    }

    if (strcmp(Data, name4) != 0) {
        test_failed();
        goto fin;
    }

fin:
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(unlink_unix_file(Path, name1, name2))
    FAIL(testdir && FPDelete(Conn, vol, testdir, ""))
test_exit:
    exit_test("FPOpenFork:test237: symlink reading and attack: try reading /etc/passwd");
}

/* ------------------------- */
STATIC void test238()
{
    char *name1 = "t238 dir";
    char *name2 = "link";
    char *name3 = "t238 dir/link";
    char *verylonglinkname = "verylonglinkname";
    int verylonglinkname_len = 16;
    int  testdir;
    uint16_t fork = 0;
    uint16_t vol = VolID, bitmap;
    struct afp_filedir_parms filedir = { 0 };
    const DSI *dsi = &Conn->dsi;
    int ofs = 3 * sizeof(uint16_t);
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    bitmap = (1 << DIRPBIT_ATTR) | (1 << DIRPBIT_ATTR) | (1 << FILPBIT_FINFO) |
             (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE) |
             (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID);
    /* create "t238 dir" */
    testdir  = FPCreateDir(Conn, vol, DIRDID_ROOT, name1);

    if (!testdir) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, testdir, verylonglinkname)) {
        test_nottested();
        goto fin;
    }

    if (symlink_unix_file(verylonglinkname, Path, name3)) {
        test_failed();
        goto fin;
    }

    /* Check if the volume uses option 'followsymlinks' */
    if (FPGetFileDirParams(Conn, vol, testdir, name2, 1 << FILPBIT_UNIXPR, 0)) {
        test_failed();
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 1 << FILPBIT_UNIXPR, 0);

    if (!(S_ISLNK(filedir.unix_priv))) {
        test_skipped(T_NOSYML);
        goto fin;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, testdir, name2, OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    if (ntohl(AFPERR_PARAM) != FPRead(Conn, fork, 0, 8, Data)) {
        test_failed();
        goto fin;
    }

    if (FPRead(Conn, fork, 0, verylonglinkname_len, Data)) {
        test_failed();
        goto fin;
    }

    Data[verylonglinkname_len] = 0;

    if (!Quiet) {
        fprintf(stdout, "readlink: %s\n", Data);
    }

    if (strcmp(Data, verylonglinkname) != 0) {
        test_failed();
        goto fin;
    }

fin:
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(unlink_unix_file(Path, name1, name2))
    FAIL(testdir && FPDelete(Conn, vol, testdir, verylonglinkname))
    FAIL(testdir && FPDelete(Conn, vol, testdir, ""))
test_exit:
    exit_test("FPOpenFork:test238: symlink reading with short reqcount");
}

/* ------------------------- */
STATIC void test431()
{
    char *name = "t431";
    uint16_t fork1 = 0;
    uint16_t vol = VolID;
    char cmd[8192];
    const char *teststring = "test\n";
    int teststring_len = 5;
    struct afp_filedir_parms filedir = { 0 };
    const DSI *dsi = &Conn->dsi;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (adouble == AD_V2) {
        test_skipped(T_ADEA);
        goto test_exit;
    }

    if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_EXTATTRS)) {
        test_skipped(T_EA);
        goto test_exit;
    }

    if (Conn->afp_version < 31) {
        test_skipped(T_AFP31);
        goto test_exit;
    }

    /* touch resource fork conversion test base file */
    if (snprintf(cmd, sizeof(cmd), "touch %s/%s", Path, name) > sizeof(cmd)) {
        if (!Quiet) {
            fprintf(stdout, "FPOpenFork:test431: path too long\n");
        }

        test_failed();
        goto fin;
    }

    if (system(cmd) != 0) {
        test_failed();
        goto fin;
    }

    if (snprintf(cmd, sizeof(cmd), "%s/%s", Path, name) > sizeof(cmd)) {
        if (!Quiet) {
            fprintf(stdout, "FPOpenFork:test431: path too long\n");
        }

        test_failed();
        goto fin;
    }

    if (chmod(cmd, 0666) != 0) {
        test_failed();
        goto fin;
    }

    /* Create .AppleDouble directory */
    if (snprintf(cmd, sizeof(cmd), "%s/.AppleDouble", Path) > sizeof(cmd)) {
        if (!Quiet) {
            fprintf(stdout, "FPOpenFork:test431: path too long\n");
        }

        test_failed();
        goto fin;
    }

    if (mkdir(cmd, 0777) != 0 && errno != EEXIST) {
        test_failed();
        goto fin;
    }

    /* Copy resource fork */
    if (snprintf(cmd, sizeof(cmd), "cp %s/test431_data %s/.AppleDouble/%s",
                 _PATH_TESTDATA_DIR, Path, name) > sizeof(cmd)) {
        if (!Quiet) {
            fprintf(stdout, "FPOpenFork:test431: path too long\n");
        }

        test_failed();
        goto fin;
    }

    if (system(cmd) != 0) {
        test_failed();
        goto fin;
    }

    /* Verify v2 sidecar file was created successfully before enumerate */
    if (snprintf(cmd, sizeof(cmd), "%s/.AppleDouble/%s", Path,
                 name) >= (int)sizeof(cmd)) {
        test_failed();
        goto fin;
    }

    {
        struct stat st_pre;

        if (stat(cmd, &st_pre) != 0) {
            if (!Quiet) {
                fprintf(stdout, "FPOpenFork:test431: v2 sidecar not created\n");
            }

            test_failed();
            goto fin;
        }
    }

    /* Enumerate triggers v2→EA conversion (if 'convert appledouble = yes').
     * NB: FPEnumerate* uses my_dsi_data_receive (should be normalised).
     * See afphelper.c:delete_directory_tree() */
    if (FPEnumerate_ext2(Conn, vol, DIRDID_ROOT, "",
                         (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN),
                         (1 << DIRPBIT_PDINFO) | (1 << DIRPBIT_OFFCNT))) {
        test_failed();
        goto fin;
    }

    /* Check if v2 sidecar was consumed by conversion. We verified it existed
     * before enumerate. If it still exists after, conversion is disabled.
     * Clean up filesystem artifacts directly to avoid FAIL() in fin:. */
    {
        struct stat st_post;

        if (stat(cmd, &st_post) == 0) {
            unlink(cmd);
            snprintf(cmd, sizeof(cmd), "%s/.AppleDouble", Path);
            rmdir(cmd);
            snprintf(cmd, sizeof(cmd), "%s/%s", Path, name);
            unlink(cmd);
            test_skipped(T_V2CONV);
            goto test_exit;
        }
    }

    if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, (1 << FILPBIT_FINFO),
                            DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD)) == 0) {
        test_failed();
        goto fin;
    }

    FAIL(FPRead(Conn, fork1, 0, teststring_len, Data))

    if (memcmp(Data, teststring, teststring_len) != 0) {
        if (!Quiet) {
            fprintf(stdout, "FPOpenFork:test431: conversion failed\n");
        }

        test_failed();
        goto fin;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, (1 << FILPBIT_FINFO), 0)) {
        test_failed();
    }

    afp_filedir_unpack(&filedir, dsi->data + 6, (1 << FILPBIT_FINFO), 0);

    if (memcmp(filedir.finder_info, "TEXTTEST", 8)) {
        if (!Quiet) {
            fprintf(stdout, "FAILED - bad type/creator\n");
        }

        test_failed();
    }

fin:

    if (fork1) {
        FPCloseFork(Conn, fork1);
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, ".AppleDouble"))
test_exit:
    exit_test("FPOpenFork:test431: check AppleDouble conversion from v2 to ea");
}


/* ------------------------- */
/* test526: Fork lifecycle with cache interactions
 *
 * This test validates cache coherency during long-running fork operations
 * with concurrent permission changes from another client.
 *
 * Scenario:
 * 1. Client 1 creates a file and opens data fork for read
 * 2. Client 1 reads file multiple times (5+ times to populate cache)
 * 3. Client 2 changes file permissions (chmod #1)
 * 4. Client 1 continues reading (tests permission cache update)
 * 5. Client 2 changes permissions again (chmod #2)
 * 6. Client 1 seeks and reads different offsets
 * 7. Client 1 closes fork
 * 8. Client 2 deletes file
 * 9. Client 1 attempts to stat file (should fail AFPERR_NOOBJ)
 * 10. Verify cache properly cleaned, no stale entries
 */
STATIC void test526()
{
    char *name = "t526_fork_lifecycle.txt";
    const char *testdata = "This is test data for fork lifecycle cache testing. ";
    uint16_t bitmap = (1 << FILPBIT_ATTR) | (1 << FILPBIT_MDATE) |
                      (1 << FILPBIT_UNIXPR);
    uint16_t fork1 = 0;
    uint16_t vol = VolID;
    uint16_t vol2 = 0;
    int fd = -1;
    struct stat st;
    int i;
    ENTER_TEST

    if (!Quiet) {
        fprintf(stdout,
                "FPOpenFork:test526: Fork lifecycle with cache interactions and permission changes\n");
    }

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    /************************************
     * Step 1: Client 1 creates file and writes initial data
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 1: Client 1 creates file\n");
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    /* Write test data using filesystem directly */
    snprintf(temp, sizeof(temp), "%s/%s", Path, name);
    fd = open(temp, O_WRONLY | O_CREAT | O_TRUNC, 0640);

    if (fd < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to open %s: %s\n", temp, strerror(errno));
        }

        test_failed();
        goto fin;
    }

    /* Write repeated test data (260+ bytes) */
    size_t testdata_len = strnlen(testdata, 512);

    for (i = 0; i < 5; i++) {
        if (write(fd, testdata, testdata_len) != (ssize_t)testdata_len) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED unable to write data: %s\n", strerror(errno));
            }

            test_failed();
            close(fd);
            goto fin;
        }
    }

    close(fd);
    fd = -1;

    /************************************
     * Step 2: Client 1 opens fork for read and reads multiple times
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 2: Client 1 opens data fork for read\n");
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                       OPENACC_RD);

    if (!fork1) {
        test_failed();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout, "\t  Step 3: Client 1 reads file 6 times to populate cache\n");
    }

    /* Read multiple times to populate cache */
    for (i = 0; i < 6; i++) {
        if (FPRead(Conn, fork1, i * 40, 40, Data)) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED read iteration %d failed\n", i);
            }

            test_failed();
            goto fin;
        }

        if (!Quiet && Verbose) {
            fprintf(stdout, "\t    Read iteration %d completed\n", i + 1);
        }
    }

    /************************************
     * Step 3: Client 2 changes permissions (chmod #1)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 4: Client 2 changes file permissions (chmod #1 -> 0444)\n");
    }

    if (chmod(temp, 0444) < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to chmod %s: %s\n", temp, strerror(errno));
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Permissions changed to 0444\n");
    }

    /************************************
     * Step 4: Client 1 continues reading (tests permission cache update)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Client 1 continues reading (permission cache should update)\n");
    }

    /* Read again - should still work (fork already open) */
    for (i = 0; i < 3; i++) {
        if (FPRead(Conn, fork1, 10 + i * 20, 20, Data)) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED read after chmod #1 failed at iteration %d\n", i + 1);
            }

            test_failed();
            goto fin;
        }

        if (!Quiet && Verbose) {
            fprintf(stdout, "\t    Post-chmod read iteration %d completed\n", i + 1);
        }
    }

    /************************************
     * Step 5: Client 2 changes permissions again (chmod #2)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 6: Client 2 changes permissions again (chmod #2 -> 0644)\n");
    }

    if (chmod(temp, 0644) < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to chmod %s: %s\n", temp, strerror(errno));
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Permissions changed back to 0644\n");
    }

    /************************************
     * Step 6: Client 1 seeks and reads different offsets
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 7: Client 1 seeks and reads different offsets\n");
    }

    /* Read from beginning */
    if (FPRead(Conn, fork1, 0, 50, Data)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED read from offset 0 failed\n");
        }

        test_failed();
        goto fin;
    }

    /* Read from middle */
    if (FPRead(Conn, fork1, 100, 50, Data)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED read from offset 100 failed\n");
        }

        test_failed();
        goto fin;
    }

    /* Read from end */
    if (FPRead(Conn, fork1, 200, 50, Data)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED read from offset 200 failed\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 7: Client 1 closes fork
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 8: Client 1 closes fork\n");
    }

    FAIL(FPCloseFork(Conn, fork1))
    fork1 = 0;

    /************************************
     * Step 8: Client 2 deletes file
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 9: Client 2 deletes file\n");
    }

    /* Open volume on Conn2 */
    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Client 2 could not open volume\n");
        }

        test_nottested();
        goto fin;
    }

    if (FPDelete(Conn2, vol2, DIRDID_ROOT, name)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Client 2 could not delete file\n");
        }

        test_failed();
        FPCloseVol(Conn2, vol2);
        goto fin;
    }

    FPCloseVol(Conn2, vol2);

    /************************************
     * Step 9: Client 1 attempts to stat deleted file
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 10: Client 1 attempts to stat deleted file (should fail)\n");
    }

    /* Should fail with AFPERR_NOOBJ */
    unsigned int ret = FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0);

    if (ret != ntohl(AFPERR_NOOBJ)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED stat should return AFPERR_NOOBJ, got %d (%s)\n",
                    ntohl(ret), afp_error(ret));
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 10: Verify cache cleaned
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 11: Verify no stale cache entries (file gone)\n");
    }

    /* Verify file is really gone on filesystem */
    if (stat(temp, &st) == 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED file still exists on filesystem: %s\n", temp);
        }

        test_failed();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout, "\t  ✓ All steps completed - cache coherency validated\n");
    }

fin:

    if (fork1) {
        FPCloseFork(Conn, fork1);
    }

    if (fd >= 0) {
        close(fd);
    }

    /* Cleanup: try to delete file if it still exists */
    FPDelete(Conn, vol, DIRDID_ROOT, name);

    if (vol2) {
        FPCloseVol(Conn2, vol2);
    }

test_exit:
    exit_test("FPOpenFork:test526: Fork lifecycle with cache interactions and permission changes");
}

/* test542: OpenFork on file in externally created nested directories
 *
 * This test validates the server's ability to resolve paths through
 * directories that were created externally (not via AFP operations) and
 * are therefore NOT in the dircache. This tests the "invalid-on-use"
 * recovery path where the server must discover directories via cname()
 * traversal, create dircache entries on-the-fly, and successfully open
 * a fork on a file deep in the externally-created tree.
 *
 * Scenario:
 * 1. Use mkdir -p to create 3-level nested directory tree on filesystem
 * 2. Create a file with content as the leaf node
 * 3. Resolve each directory through AFP (forces dircache entry creation)
 * 4. FPOpenFork on the leaf file
 * 5. Read data to verify fork is functional
 * 6. Write additional data to verify write access
 * 7. Read back written data to verify round-trip
 * 8. Close fork and clean up via AFP
 *
 * Note: This test requires localhost connection since it directly
 * creates filesystem artifacts via mkdir/open.
 */
STATIC void test542()
{
    char *dir1_name = "t532_ext1";
    char *dir2_name = "t532_ext2";
    char *dir3_name = "t532_ext3";
    char *file_name = "t532_file.txt";
    char pathbuf[MAXPATHLEN];
    char cmdbuf[MAXPATHLEN];
    const char *testdata = "external file test data\n";
    int testdata_len = 24;
    uint16_t vol = VolID;
    uint16_t fork = 0;
    int dir1 = 0, dir2 = 0, dir3 = 0;
    int fd;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    /* This test requires localhost connection for direct filesystem access */
    if (strcmp(Server, "localhost") != 0 && strcmp(Server, "127.0.0.1") != 0) {
        test_skipped(T_LOCALHOST);
        goto test_exit;
    }

    /************************************
     * Step 1: Create nested directories externally (NOT via AFP)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 1: Create 3-level nested directory tree via mkdir -p\n");
    }

    snprintf(pathbuf, sizeof(pathbuf), "%s/%s/%s/%s",
             Path, dir1_name, dir2_name, dir3_name);
    snprintf(cmdbuf, sizeof(cmdbuf), "mkdir -p %s", pathbuf);

    if (system(cmdbuf) != 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED mkdir -p %s\n", pathbuf);
        }

        test_nottested();
        goto test_exit;
    }

    /************************************
     * Step 2: Create file with content externally
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 2: Create file with content in deepest directory\n");
    }

    snprintf(pathbuf, sizeof(pathbuf), "%s/%s/%s/%s/%s",
             Path, dir1_name, dir2_name, dir3_name, file_name);
    fd = open(pathbuf, O_WRONLY | O_CREAT | O_TRUNC, 0640);

    if (fd < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to create %s: %s\n",
                    pathbuf, strerror(errno));
        }

        test_nottested();
        goto fin;
    }

    if (write(fd, testdata, testdata_len) != testdata_len) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to write to %s: %s\n",
                    pathbuf, strerror(errno));
        }

        close(fd);
        test_nottested();
        goto fin;
    }

    close(fd);
    /* Ensure permissions allow AFP access */
    snprintf(cmdbuf, sizeof(cmdbuf), "chmod -R 0777 %s/%s",
             Path, dir1_name);
    system(cmdbuf);

    /************************************
     * Step 3: Resolve path through AFP, forcing dircache creation
     *
     * Each get_did() call forces the server's cname() to discover
     * a directory that exists on the filesystem but is NOT in the
     * dircache. The server must lstat() the path component, create
     * a new dircache entry, and return the directory ID.
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 3: Resolve nested dirs through AFP (dircache discovery)\n");
    }

    dir1 = get_did(Conn, vol, DIRDID_ROOT, dir1_name);

    if (!dir1) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED get_did for %s\n", dir1_name);
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    dir1 (%s) DID = 0x%x\n", dir1_name, dir1);
    }

    dir2 = get_did(Conn, vol, dir1, dir2_name);

    if (!dir2) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED get_did for %s\n", dir2_name);
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    dir2 (%s) DID = 0x%x\n", dir2_name, dir2);
    }

    dir3 = get_did(Conn, vol, dir2, dir3_name);

    if (!dir3) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED get_did for %s\n", dir3_name);
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    dir3 (%s) DID = 0x%x\n", dir3_name, dir3);
    }

    /************************************
     * Step 4: Open fork on externally created file
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 4: FPOpenFork on externally created file\n");
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, dir3, file_name,
                      OPENACC_RD | OPENACC_WR);

    if (!fork) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPOpenFork for %s in dir3\n",
                    file_name);
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 5: Read data to verify fork is functional
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Read data from fork (verify content)\n");
    }

    if (FPRead(Conn, fork, 0, testdata_len, Data)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPRead returned error\n");
        }

        test_failed();
        goto fin;
    }

    if (memcmp(Data, testdata, testdata_len) != 0) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED read data doesn't match written data\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Read %d bytes successfully\n",
                testdata_len);
    }

    /************************************
     * Step 6: Write additional data to verify write access
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 6: Write additional data to fork\n");
    }

    if (FPWrite(Conn, fork, testdata_len, 11, "extra data\n", 0)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPWrite returned error\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 7: Read back written data for round-trip verification
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 7: Read back written data (round-trip verify)\n");
    }

    if (FPRead(Conn, fork, testdata_len, 11, Data)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPRead of written data\n");
        }

        test_failed();
        goto fin;
    }

    if (memcmp(Data, "extra data\n", 11) != 0) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED written data doesn't match on re-read\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 8: Close fork
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 8: Close fork\n");
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = 0;

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ Successfully opened fork on file in externally created nested directories\n");
    }

fin:

    if (fork) {
        FPCloseFork(Conn, fork);
    }

    /* Cleanup via AFP - delete from deepest to shallowest */
    if (dir3) {
        FPDelete(Conn, vol, dir3, file_name);
    }

    if (dir2) {
        FPDelete(Conn, vol, dir2, dir3_name);
    }

    if (dir1) {
        FPDelete(Conn, vol, dir1, dir2_name);
    }

    FPDelete(Conn, vol, DIRDID_ROOT, dir1_name);
test_exit:
    exit_test("FPOpenFork:test542: open fork on file in externally created nested directories");
}

/* test544: Resource fork length round-trip after write+close+re-read
 *
 * This test validates that the resource fork length reported by
 * FPGetFileDirParams (FILPBIT_RFLEN) correctly reflects data written
 * to the resource fork, both while the fork is open and after
 * close+reopen.
 *
 * Scenario:
 * 1. Create a file
 * 2. Open resource fork for write
 * 3. Write 500 bytes of data
 * 4. Close fork
 * 5. Verify RFLEN == 500 via FPGetFileDirParams
 * 6. Reopen resource fork for write
 * 7. Append 300 more bytes (total 800)
 * 8. Close fork
 * 9. Verify RFLEN == 800 via FPGetFileDirParams
 * 10. Reopen resource fork for read
 * 11. Read 800 bytes, verify content
 * 12. Close fork and clean up
 */
STATIC void test544()
{
    char *name = "t535 rfork length roundtrip";
    uint16_t bitmap = 0;
    uint16_t fork = 0;
    uint16_t vol = VolID;
    int  ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    uint16_t fbitmap = (1 << FILPBIT_RFLEN);
    const DSI *dsi = &Conn->dsi;
    ENTER_TEST

    /************************************
     * Step 1: Create file
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 1: Create test file\n");
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    /************************************
     * Step 2: Open resource fork for write
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 2: Open resource fork for write\n");
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPOpenFork resource fork\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 3: Write 500 bytes of data
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 3: Write 500 bytes to resource fork\n");
    }

    if (FPWrite(Conn, fork, 0, 500, Data, 0)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPWrite 500 bytes\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 4: Close fork
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 4: Close resource fork\n");
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = 0;

    /************************************
     * Step 5: Verify RFLEN == 500 via FPGetFileDirParams
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Verify RFLEN == 500 via FPGetFileDirParams\n");
    }

    memset(&filedir, 0, sizeof(filedir));

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, fbitmap, 0)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPGetFileDirParams for RFLEN\n");
        }

        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, fbitmap, 0);

    if (filedir.rflen != 500) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED RFLEN should be 500, got %u\n",
                    filedir.rflen);
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    RFLEN = %u (correct)\n",
                filedir.rflen);
    }

    /************************************
     * Step 6: Reopen resource fork for write
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 6: Reopen resource fork for write (append)\n");
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPOpenFork reopen\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 7: Append 300 more bytes (total 800)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 7: Append 300 bytes (offset 500, total 800)\n");
    }

    if (FPWrite(Conn, fork, 500, 300, Data, 0)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPWrite append 300 bytes\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 8: Close fork
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 8: Close resource fork\n");
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = 0;

    /************************************
     * Step 9: Verify RFLEN == 800 via FPGetFileDirParams
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 9: Verify RFLEN == 800 via FPGetFileDirParams\n");
    }

    memset(&filedir, 0, sizeof(filedir));

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, fbitmap, 0)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED FPGetFileDirParams for RFLEN after append\n");
        }

        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, fbitmap, 0);

    if (filedir.rflen != 800) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED RFLEN should be 800, got %u\n",
                    filedir.rflen);
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    RFLEN = %u (correct)\n",
                filedir.rflen);
    }

    /************************************
     * Step 10: Reopen resource fork for read and verify content
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 10: Reopen resource fork, read 800 bytes\n");
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD);

    if (!fork) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED FPOpenFork reopen for read\n");
        }

        test_failed();
        goto fin;
    }

    if (FPRead(Conn, fork, 0, 800, Data)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPRead 800 bytes\n");
        }

        test_failed();
        goto fin;
    }

    FAIL(FPCloseFork(Conn, fork))
    fork = 0;

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ Resource fork length round-trip verified (500 → 800 bytes)\n");
    }

fin:

    if (fork) {
        FPCloseFork(Conn, fork);
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPOpenFork:test544: Resource fork length round-trip after write+close+re-read");
}

/* ----------- */
void T2FPOpenFork_test()
{
    ENTER_TESTSET
    test3();
    test4();
    test7();
#if 0
    test47();
#endif
    test49();
    test152();
    test153();
    test156();
    test157();
    test321();
    test372();
    test392();
    test411();
    test236();
    test237();
    test238();
    test431();
    test526();
    test542();
    test544();
}
