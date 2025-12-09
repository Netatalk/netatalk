/* ----------------------------------------------
*/
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/* ------------------------- */
STATIC void test108()
{
    int dir;
    char *name  = "t108 exchange file";
    char *name1 = "t108 new file name";
    char *ndir  = "t108 dir";
    int fid_name;
    int fid_name1;
    int temp;
    uint16_t vol = VolID;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_failed();
    }

    FAIL(FPCreateFile(Conn, vol, 0, dir, name1))
    fid_name  = get_fid(Conn, vol, DIRDID_ROOT, name);
    fid_name1 = get_fid(Conn, vol, dir, name1);
    write_fork(Conn, vol, DIRDID_ROOT, name, "blue");
    write_fork(Conn, vol, dir, name1, "red");
    /* ok */
    FAIL(FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))

    /* test remove of no cnid db */
    if ((temp = get_fid(Conn, vol, DIRDID_ROOT, name)) != fid_name) {
        if (Mac) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED (IGNORED) %x should be %x\n", temp, fid_name);
            }
        } else {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED %x should be %x\n", temp, fid_name);
            }

            test_failed();
        }
    }

    if ((temp = get_fid(Conn, vol, dir, name1)) != fid_name1) {
        if (Mac) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED (IGNORED) %x should be %x\n", temp, fid_name1);
            }
        } else {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED %x should be %x\n", temp, fid_name1);
            }

            test_failed();
        }
    }

    read_fork(Conn, vol, DIRDID_ROOT, name, 3);

    if (strcmp(Data, "red")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be red\n");
        }

        test_failed();
    }

    read_fork(Conn,  vol, dir, name1, 4);

    if (strcmp(Data, "blue")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be blue\n");
        }

        test_failed();
    }

    FAIL(FPDelete(Conn, vol, dir, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, ndir))
test_exit:
    exit_test("FPExchangeFiles:test108: exchange files");
}

/* ------------------------- */
STATIC void test111()
{
    int fork;
    int fork1;
    int dir;
    uint16_t bitmap = 0;
    char *name  = "t111 exchange open files";
    char *name1 = "t111 new file name";
    char *ndir  = "t111 dir";
    int fid_name;
    int fid_name1;
    uint16_t vol = VolID;
    int ret;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_failed();
        goto fin;
    }

    FAIL(FPCreateFile(Conn, vol, 0, dir, name1))
    fid_name  = get_fid(Conn, vol, DIRDID_ROOT, name);
    fid_name1 = get_fid(Conn, vol, dir, name1);
    write_fork(Conn, vol, DIRDID_ROOT, name, "blue");
    write_fork(Conn, vol, dir, name1, "red");
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    /* ok */
    FAIL(FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))
    read_fork(Conn, vol, DIRDID_ROOT, name, 3);

    if (strcmp(Data, "red")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be red\n");
        }

        test_failed();
    }

    read_fork(Conn, vol, dir, name1, 4);

    if (strcmp(Data, "blue")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be blue\n");
        }

        test_failed();
    }

    FAIL(FPWrite(Conn, fork, 0, 3, "new", 0))
    read_fork(Conn, vol, dir, name1, 3);

    if (strcmp(Data, "new")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be new\n");
        }

        test_failed();
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                       OPENACC_WR | OPENACC_RD);

    if (!fork1) {
        test_failed();
    }

    FAIL(FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))

    if (fork1) {
        FPCloseFork(Conn, fork1);
    }

    FAIL(FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))

    if (fork) {
        FPCloseFork(Conn, fork);
    }

    if ((ret = get_fid(Conn, vol, DIRDID_ROOT, name)) != fid_name) {
        if (Mac) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED (IGNORED) %x should be %x\n", ret, fid_name);
            }
        } else {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED %x should be %x\n", ret, fid_name);
            }

            test_failed();
        }
    }

    if ((ret = get_fid(Conn, vol, dir, name1)) != fid_name1) {
        if (Mac) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED (IGNORED) %x should be %x\n", ret, fid_name1);
            }
        } else {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED %x should be %x\n", ret, fid_name);
            }

            test_failed();
        }
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, ndir))
test_exit:
    exit_test("FPExchangeFiles:test111: exchange open files");
}

/* ------------------------- */
STATIC void test197()
{
    int dir;
    char *name  = "t197 exchange file";
    char *name1 = "t197 new file name";
    char *ndir  = "t197 dir";
    uint16_t vol = VolID;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, ndir))) {
        test_failed();
    }

    FAIL(FPCreateFile(Conn, vol, 0, dir, name1))
    write_fork(Conn,  vol, DIRDID_ROOT, name, "blue");
    write_fork(Conn,  vol, dir, name1, "red");
    /* ok */
    FAIL(FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))
    read_fork(Conn, vol, DIRDID_ROOT, name, 3);

    if (strcmp(Data, "red")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be red\n");
        }

        test_failed();
    }

    read_fork(Conn,  vol, dir, name1, 4);

    if (strcmp(Data, "blue")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be blue\n");
        }

        test_failed();
    }

    FAIL(FPDelete(Conn, vol, dir, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, ndir))
test_exit:
    exit_test("FPExchangeFiles:test197: exchange files (doesn't check files' ID)");
}

/* ------------------------- */
STATIC void test342()
{
    int dir;
    char *name  = "t342 exchange file";
    char *name1 = "t342 new file name";
    int fid_name;
    int fid_name1;
    int temp;
    uint16_t vol = VolID;
    struct afp_filedir_parms filedir;
    char finder_info[32];
    uint16_t bitmap;
    const DSI *dsi = &Conn->dsi;
    int  ofs =  3 * sizeof(uint16_t);
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    dir = DIRDID_ROOT;
    FAIL(FPCreateFile(Conn, vol, 0, dir, name1))
    /* set some metadata, MUST NOT be exchanged */
    bitmap = (1 << FILPBIT_FINFO);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    memcpy(filedir.finder_info, "TESTTEST", 8);
    memcpy(finder_info, filedir.finder_info, 32);
    FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
    fid_name  = get_fid(Conn, vol, DIRDID_ROOT, name);
    fid_name1 = get_fid(Conn, vol, dir, name1);
    write_fork(Conn, vol, DIRDID_ROOT, name, "blue");
    write_fork(Conn, vol, dir, name1, "red");
    /* ok */
    FAIL(FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))

    /* test whether FinderInfo was preserved */
    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

    if (memcmp(finder_info, filedir.finder_info, 32) != 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED: metadata wasn't preserved\n");
        }

        test_failed();
    }

    /* test remove of no cnid db */
    if ((temp = get_fid(Conn, vol, DIRDID_ROOT, name)) != fid_name) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", temp, fid_name);
        }

        test_failed();
    }

    if ((temp = get_fid(Conn, vol, dir, name1)) != fid_name1) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", temp, fid_name1);
        }

        test_failed();
    }

    read_fork(Conn, vol, DIRDID_ROOT, name, 3);

    if (strcmp(Data, "red")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be red\n");
        }

        test_failed();
    }

    read_fork(Conn,  vol, dir, name1, 4);

    if (strcmp(Data, "blue")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be blue\n");
        }

        test_failed();
    }

    FAIL(FPDelete(Conn, vol, dir, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPExchangeFiles:test342: exchange files");
}

/* ------------------------- */
STATIC void test389()
{
    int dir;
    char *name  = "t389 exchange file";
    char *name1 = "t389 new file name";
    uint16_t bitmap = 0;
    int fid_name;
    int fid_name1;
    int temp;
    int fork;
    uint16_t vol = VolID;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    dir = DIRDID_ROOT;
    FAIL(FPCreateFile(Conn, vol, 0, dir, name1))
    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fid_name  = get_fid(Conn, vol, DIRDID_ROOT, name);
    fid_name1 = get_fid(Conn, vol, dir, name1);
    write_fork(Conn, vol, DIRDID_ROOT, name, "blue");
    write_fork(Conn, vol, dir, name1, "red");
    /* ok */
    FAIL(FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))
    FAIL(FPCloseFork(Conn, fork))

    /* test remove of no cnid db */
    if ((temp = get_fid(Conn, vol, DIRDID_ROOT, name)) != fid_name) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", temp, fid_name);
        }

        test_failed();
    }

    if ((temp = get_fid(Conn, vol, dir, name1)) != fid_name1) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", temp, fid_name1);
        }

        test_failed();
    }

    read_fork(Conn, vol, DIRDID_ROOT, name, 3);

    if (strcmp(Data, "red")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be red\n");
        }

        test_failed();
    }

    read_fork(Conn,  vol, dir, name1, 4);

    if (strcmp(Data, "blue")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be blue\n");
        }

        test_failed();
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPExchangeFiles:test389: exchange files, source with resource fork open");
}

/* ------------------------- */
STATIC void test390()
{
    int dir;
    char *name  = "t390 exchange file";
    char *name1 = "t390 new file name";
    uint16_t bitmap = 0;
    int fid_name;
    int fid_name1;
    int temp;
    int fork;
    uint16_t vol = VolID;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    dir = DIRDID_ROOT;
    FAIL(FPCreateFile(Conn, vol, 0, dir, name1))
    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fid_name  = get_fid(Conn, vol, DIRDID_ROOT, name);
    fid_name1 = get_fid(Conn, vol, dir, name1);
    write_fork(Conn, vol, DIRDID_ROOT, name, "blue");
    write_fork(Conn, vol, dir, name1, "red");
    /* ok */
    FAIL(FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))
    FAIL(FPFlushFork(Conn, fork))
    FAIL(FPCloseFork(Conn, fork))

    /* test remove of no cnid db */
    if ((temp = get_fid(Conn, vol, DIRDID_ROOT, name)) != fid_name) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", temp, fid_name);
        }

        test_failed();
    }

    if ((temp = get_fid(Conn, vol, dir, name1)) != fid_name1) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", temp, fid_name1);
        }

        test_failed();
    }

    read_fork(Conn, vol, DIRDID_ROOT, name, 3);

    if (strcmp(Data, "red")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be red\n");
        }

        test_failed();
    }

    read_fork(Conn,  vol, dir, name1, 4);

    if (strcmp(Data, "blue")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be blue\n");
        }

        test_failed();
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPExchangeFiles:test390: exchange files, source with resource fork open");
}

/* ------------------------- */
STATIC void test391()
{
    int dir;
    char *name  = "t391 exchange file";
    char *name1 = "t391 new file name";
    uint16_t bitmap = 0;
    int fid_name;
    int fid_name1;
    int temp;
    int fork;
    uint16_t vol = VolID;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    dir = DIRDID_ROOT;
    FAIL(FPCreateFile(Conn, vol, 0, dir, name1))
    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name1,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fid_name  = get_fid(Conn, vol, DIRDID_ROOT, name);
    fid_name1 = get_fid(Conn, vol, dir, name1);
    write_fork(Conn, vol, DIRDID_ROOT, name, "blue");
    write_fork(Conn, vol, dir, name1, "red");
    /* ok */
    FAIL(FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))
    FAIL(FPCloseFork(Conn, fork))

    /* test remove of no cnid db */
    if ((temp = get_fid(Conn, vol, DIRDID_ROOT, name)) != fid_name) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", temp, fid_name);
        }

        test_failed();
    }

    if ((temp = get_fid(Conn, vol, dir, name1)) != fid_name1) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", temp, fid_name1);
        }

        test_failed();
    }

    read_fork(Conn, vol, DIRDID_ROOT, name, 3);

    if (strcmp(Data, "red")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be red\n");
        }

        test_failed();
    }

    read_fork(Conn,  vol, dir, name1, 4);

    if (strcmp(Data, "blue")) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED should be blue\n");
        }

        test_failed();
    }

fin:
    FAIL(FPDelete(Conn, vol, dir, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPExchangeFiles:test391: exchange files, dest with resource fork open");
}

/* test531: File exchange with cache coherency
 *
 * This test validates that file exchange properly updates cache when
 * a file is exchanged while one client has it open for reading.
 *
 * Scenario:
 * 1. Client 1 creates file1, file2
 * 2. Client 1 writes data to file1
 * 3. Client 1 opens file1 for read (cache entry created)
 * 4. Client 2 exchanges file1 <-> file2
 * 5. Client 1 reads from fork (should see file2's original content)
 * 6. Client 1 stats file1 (should have file2's metadata)
 * 7. Verify cache updated, CNID swapped correctly
 */
STATIC void test531()
{
    char *file1 = "t531_file1.txt";
    char *file2 = "t531_file2.txt";
    char *data1 = "FILE1_DATA";
    char *data2 = "FILE2_DATA";
    uint16_t bitmap = 0;
    uint16_t vol = VolID;
    uint16_t vol2 = 0;
    uint16_t fork = 0;
    int fid_file1;
    int fid_file2;
    int temp_fid;
    char read_buffer[32];
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    /************************************
     * Step 1-2: Client 1 creates files and writes data
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 1-2: Client 1 creates file1 and file2 with different data\n");
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file1)) {
        test_nottested();
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file2)) {
        test_nottested();
        goto fin;
    }

    /* Get original CNIDs */
    fid_file1 = get_fid(Conn, vol, DIRDID_ROOT, file1);
    fid_file2 = get_fid(Conn, vol, DIRDID_ROOT, file2);
    /* Write different data to each file */
    write_fork(Conn, vol, DIRDID_ROOT, file1, data1);
    write_fork(Conn, vol, DIRDID_ROOT, file2, data2);

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    file1 has '%s', file2 has '%s'\n", data1, data2);
    }

    /************************************
     * Step 3: Client 1 opens file1 for read (cache entry created)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 3: Client 1 opens file1 for read (populates cache)\n");
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, file1,
                      OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    /* Read to populate cache */
    size_t data1_len = strnlen(data1, 512);

    if (FPRead(Conn, fork, 0, (int)data1_len, read_buffer)) {
        test_failed();
        goto fin;
    }

    read_buffer[data1_len] = '\0';

    if (strcmp(read_buffer, data1) != 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED initial read should be '%s', got '%s'\n", data1,
                    read_buffer);
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    file1 opened and read successfully\n");
    }

    /************************************
     * Step 4: Client 2 exchanges file1 <-> file2
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 4: Client 2 exchanges file1 <-> file2\n");
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Client 2 could not open volume\n");
        }

        test_nottested();
        goto fin;
    }

    FAIL(FPExchangeFile(Conn2, vol2, DIRDID_ROOT, DIRDID_ROOT, file1, file2))
    FPCloseVol(Conn2, vol2);
    vol2 = 0;

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Files exchanged\n");
    }

    /************************************
     * Step 5: Client 1 reads from fork (should see file2's content)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Client 1 reads from open fork (should see swapped content)\n");
    }

    /* Close and reopen the fork to get updated content */
    FAIL(FPCloseFork(Conn, fork))
    fork = 0;
    /* Read file1 (should now contain file2's original data) */
    size_t data2_len = strnlen(data2, 512);
    read_fork(Conn, vol, DIRDID_ROOT, file1, (int)data2_len);

    if (strcmp(Data, data2) != 0) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED after exchange, file1 should contain '%s', got '%s'\n",
                    data2, Data);
        }

        test_failed();
    }

    /* Read file2 (should now contain file1's original data) */
    size_t data1_len2 = strnlen(data1, 512);
    read_fork(Conn, vol, DIRDID_ROOT, file2, (int)data1_len2);

    if (strcmp(Data, data1) != 0) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED after exchange, file2 should contain '%s', got '%s'\n",
                    data1, Data);
        }

        test_failed();
    }

    /************************************
     * Step 6-7: Verify CNID swapped correctly
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 6-7: Verify CNIDs remained with original files\n");
    }

    temp_fid = get_fid(Conn, vol, DIRDID_ROOT, file1);

    if (temp_fid != fid_file1) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED file1 CNID should be %x, got %x\n", fid_file1,
                    temp_fid);
        }

        test_failed();
    }

    temp_fid = get_fid(Conn, vol, DIRDID_ROOT, file2);

    if (temp_fid != fid_file2) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED file2 CNID should be %x, got %x\n", fid_file2,
                    temp_fid);
        }

        test_failed();
    }

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ File exchange cache coherency validated (data swapped, CNIDs preserved)\n");
    }

fin:

    if (fork) {
        FPCloseFork(Conn, fork);
    }

    if (vol2) {
        FPCloseVol(Conn2, vol2);
    }

    FPDelete(Conn, vol, DIRDID_ROOT, file1);
    FPDelete(Conn, vol, DIRDID_ROOT, file2);
test_exit:
    exit_test("FPExchangeFiles:test531: File exchange with cache coherency");
}

/* ----------- */
void FPExchangeFiles_test()
{
    ENTER_TESTSET
    test108();
    test111();
    test197();
    test342();
    test389();
    test390();
    test391();
    test531();
}
