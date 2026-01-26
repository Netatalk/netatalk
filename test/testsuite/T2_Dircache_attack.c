#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/*!
  @file
  @brief Test the following:
  @code
   test500()
   =========

   client 1:
      mkdir dir1
      mkdir dir2

   client 2:
      mv dir1 dir2/renamed

   client 1:
      ls dir2
      stat dir2/renamed

   Check: CNID must not change
   Targets: enumerate()

   test501()
   =========

   client 1:
      mkdir dir1
      mkdir dir2

   client 2:
      mv dir1 dir2/renamed

   client 1:
      stat renamed

   Check: CNID must not change
   Targets: getfildirparms()


   test502()
   =========

   client 1:
      mkdir dir1
      mkdir dir2

   client 2:
      mv dir1 dir2/renamed

   client 1:
      ls renamed

   Check: CNID must not change
   Targets: enumerate()


   test503()
   =========

   client 1:
      mkdir dir1
      mkdir dir2

   client 2:
      mv dir1 dir2/renamed

   client 1:
      stat renamed

   Check: CNID must not change
   Targets: getfildirparms()


   test504()
   =========

   client 1:
      mkdir -p dir1/dir2
      touch dir1/dir2/file

   client 2:
      mv dir1 renamed1

   client 1:
      stat file

   Check: CNID must not change
   Targets: getfildirparms()


   test505()
   =========

   client 1:
      mkdir -p dir1/dir2

   client 2:
      mv dir1 renamed1

   client 1:
      stat dir2

   Check: CNID must not change
   Targets: getfildirparms()


   test506()
   =========

   client 1:
      mkdir -p dir1/dir2

   client 2:
      mv dir1 renamed1
      mkdir dir1

   client 1:
      stat dir2
  @endcode
 */

/*! move and rename dir, enumerate new parent, stat renamed dir */
STATIC void test500()
{
    const DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t500 dir";
    char *subdir1 = "t500 subdir1";
    char *subdir2 = "t500 subdir2";
    char *renamedsubdir1 = "t500 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_DID) | (1 << DIRPBIT_LNAME);
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        test_failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0) {
        test_failed();
    }

    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0) {
        test_failed();
    }

    if ((subdir2_id = FPCreateDir(Conn, vol1, dir_id, subdir2)) == 0) {
        test_failed();
    }

    /* Move and rename dir with second connection */
    FAIL(FPMoveAndRename(Conn2, vol2, dir_id, subdir2_id, subdir1, renamedsubdir1));
    /* Enumerate with first connection, does it crash or similar ? */
    FAIL(FPEnumerate(Conn, vol1, subdir2_id, "", (1 << FILPBIT_FNUM),
                     (1 << DIRPBIT_PDID)));
    /* Manually check name and CNID */
    FAIL(FPGetFileDirParams(Conn, vol1, subdir2_id, renamedsubdir1, 0, bitmap));
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.did != subdir1_id) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.did, subdir1_id);
        }

        test_failed();
    }

    if (strcmp(filedir.lname, renamedsubdir1)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %s should be %s\n", filedir.lname, renamedsubdir1);
        }

        test_failed();
    }

fin:
    FAIL(FPCloseVol(Conn2, vol2));
    FAIL(FPDelete(Conn, vol1, subdir1_id, ""));
    FAIL(FPDelete(Conn, vol1, subdir2_id, ""));
    FAIL(FPDelete(Conn, vol1, dir_id, ""));
test_exit:
    exit_test("Dircache:test500: move and rename dir, enumerate new parent, stat renamed dir");
}

/*! move and rename dir, then stat it */
STATIC void test501()
{
    const DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t501 dir";
    char *subdir1 = "t501 subdir1";
    char *subdir2 = "t501 subdir2";
    char *renamedsubdir1 = "t501 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_DID) | (1 << DIRPBIT_LNAME);
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        test_failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0) {
        test_failed();
    }

    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0) {
        test_failed();
    }

    if ((subdir2_id = FPCreateDir(Conn, vol1, dir_id, subdir2)) == 0) {
        test_failed();
    }

    /* Move and rename dir with second connection */
    FAIL(FPMoveAndRename(Conn2, vol2, dir_id, subdir2_id, subdir1, renamedsubdir1));
    /* Manually check name and CNID */
    FAIL(FPGetFileDirParams(Conn, vol1, subdir2_id, renamedsubdir1, 0, bitmap));
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.did != subdir1_id) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.did, subdir1_id);
        }

        test_failed();
    }

    if (strcmp(filedir.lname, renamedsubdir1)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %s should be %s\n", filedir.lname, renamedsubdir1);
        }

        test_failed();
    }

fin:
    FAIL(FPCloseVol(Conn2, vol2));
    FAIL(FPDelete(Conn, vol1, subdir1_id, ""));
    FAIL(FPDelete(Conn, vol1, subdir2_id, ""));
    FAIL(FPDelete(Conn, vol1, dir_id, ""));
test_exit:
    exit_test("Dircache:test501: move and rename dir, then stat it");
}

/*! move and rename dir, enumerate renamed dir */
STATIC void test502()
{
    const DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t502 dir";
    char *subdir1 = "t502 subdir1";
    char *subdir2 = "t502 subdir2";
    char *renamedsubdir1 = "t502 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_DID) | (1 << DIRPBIT_LNAME);
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        test_failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0) {
        test_failed();
    }

    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0) {
        test_failed();
    }

    FAIL(FPCreateFile(Conn, vol1,  0, subdir1_id, "file1"))

    if ((subdir2_id = FPCreateDir(Conn, vol1, dir_id, subdir2)) == 0) {
        test_failed();
    }

    /* Move and rename dir with second connection */
    FAIL(FPMoveAndRename(Conn2, vol2, dir_id, subdir2_id, subdir1, renamedsubdir1));
    /* Enumerate with first connection, does it crash or similar ? */
    FAIL(FPEnumerate(Conn, vol1, subdir1_id, "", (1 << FILPBIT_FNUM),
                     (1 << DIRPBIT_PDID)));
    /* Manually check name and CNID */
    FAIL(FPGetFileDirParams(Conn, vol1, subdir1_id, "", 0, bitmap));
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.did != subdir1_id) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.did, subdir1_id);
        }

        test_failed();
    }

    if (strcmp(filedir.lname, renamedsubdir1)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %s should be %s\n", filedir.lname, renamedsubdir1);
        }

        test_failed();
    }

fin:
    FAIL(FPCloseVol(Conn2, vol2));
    FAIL(FPDelete(Conn, vol1, subdir1_id, "file1"))
    FAIL(FPDelete(Conn, vol1, subdir1_id, ""));
    FAIL(FPDelete(Conn, vol1, subdir2_id, ""));
    FAIL(FPDelete(Conn, vol1, dir_id, ""));
test_exit:
    exit_test("Dircache:test502: move and rename dir, enumerate renamed dir");
}

/*! move and rename dir, stat renamed dir */
STATIC void test503()
{
    const DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t503 dir";
    char *subdir1 = "t503 subdir1";
    char *subdir2 = "t503 subdir2";
    char *renamedsubdir1 = "t503 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_DID) | (1 << DIRPBIT_LNAME);
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        test_failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0) {
        test_failed();
    }

    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0) {
        test_failed();
    }

    if ((subdir2_id = FPCreateDir(Conn, vol1, dir_id, subdir2)) == 0) {
        test_failed();
    }

    /* Move and rename dir with second connection */
    FAIL(FPMoveAndRename(Conn2, vol2, dir_id, subdir2_id, subdir1, renamedsubdir1));
    /* Manually check name and CNID */
    FAIL(FPGetFileDirParams(Conn, vol1, subdir1_id, "", 0, bitmap));
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.did != subdir1_id) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.did, subdir1_id);
        }

        test_failed();
    }

    if (strcmp(filedir.lname, renamedsubdir1)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %s should be %s\n", filedir.lname, renamedsubdir1);
        }

        test_failed();
    }

fin:
    FAIL(FPCloseVol(Conn2, vol2));
    FAIL(FPDelete(Conn, vol1, subdir1_id, ""));
    FAIL(FPDelete(Conn, vol1, subdir2_id, ""));
    FAIL(FPDelete(Conn, vol1, dir_id, ""));
test_exit:
    exit_test("Dircache:test503: move and rename dir, enumerate renamed dir");
}

/*! rename topdir, stat file in subdir of renamed topdir */
STATIC void test504()
{
    const DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t504 dir";
    char *subdir1 = "t504 subdir1";
    char *subdir2 = "t504 subdir2";
    char *renamedsubdir1 = "t504 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id, file_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_DID) | (1 << DIRPBIT_LNAME);
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        test_failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0) {
        test_failed();
    }

    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0) {
        test_failed();
    }

    if ((subdir2_id = FPCreateDir(Conn, vol1, subdir1_id, subdir2)) == 0) {
        test_failed();
    }

    /* Create file and get CNID */
    FAIL(FPCreateFile(Conn, vol1,  0, subdir2_id, "file1"));
    FAIL(FPGetFileDirParams(Conn, vol1, subdir2_id, "file1", 0, bitmap));
    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    file_id = filedir.did;
    /* Move and rename dir with second connection */
    FAIL(FPMoveAndRename(Conn2, vol2, dir_id, dir_id, subdir1, renamedsubdir1));
    /* check CNID */
    FAIL(FPGetFileDirParams(Conn, vol1, subdir2_id, "file1", 0, bitmap));
    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.did != file_id) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.did, subdir1_id);
        }

        test_failed();
    }

fin:
    FAIL(FPCloseVol(Conn2, vol2));
    FAIL(FPDelete(Conn, vol1, subdir2_id, "file1"))
    FAIL(FPDelete(Conn, vol1, subdir2_id, ""));
    FAIL(FPDelete(Conn, vol1, subdir1_id, ""));
    FAIL(FPDelete(Conn, vol1, dir_id, ""));
test_exit:
    exit_test("Dircache:test504: rename topdir, stat file in subdir of renamed topdir");
}

/*! rename dir, stat subdir in renamed dir */
STATIC void test505()
{
    const DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t505 dir";
    char *subdir1 = "t505 subdir1";
    char *subdir2 = "t505 subdir2";
    char *renamedsubdir1 = "t505 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_DID) | (1 << DIRPBIT_LNAME);
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        test_failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0) {
        test_failed();
    }

    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0) {
        test_failed();
    }

    if ((subdir2_id = FPCreateDir(Conn, vol1, subdir1_id, subdir2)) == 0) {
        test_failed();
    }

    /* Move and rename dir with second connection */
    FAIL(FPMoveAndRename(Conn2, vol2, dir_id, dir_id, subdir1, renamedsubdir1));
    /* Manually check name and CNID */
    FAIL(FPGetFileDirParams(Conn, vol1, subdir2_id, "", 0, bitmap));
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.did != subdir2_id) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.did, subdir2_id);
        }

        test_failed();
    }

    if (strcmp(filedir.lname, subdir2
              )) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %s should be %s\n", filedir.lname, subdir2);
        }

        test_failed();
    }

fin:
    FAIL(FPCloseVol(Conn2, vol2));
    FAIL(FPDelete(Conn, vol1, subdir2_id, ""));
    FAIL(FPDelete(Conn, vol1, subdir1_id, ""));
    FAIL(FPDelete(Conn, vol1, dir_id, ""));
test_exit:
    exit_test("Dircache:test505: rename dir, stat subdir in renamed dir");
}

/*! stat subdir in poisened path */
STATIC void test506()
{
    const DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t506 dir";
    char *subdir1 = "t506 subdir1";
    char *subdir2 = "t506 subdir2";
    char *renamedsubdir1 = "t506 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id, poisondir_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_DID) | (1 << DIRPBIT_LNAME);
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        test_failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0) {
        test_failed();
    }

    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0) {
        test_failed();
    }

    if ((subdir2_id = FPCreateDir(Conn, vol1, subdir1_id, subdir2)) == 0) {
        test_failed();
    }

    /* Move and rename dir with second connection */
    FAIL(FPMoveAndRename(Conn2, vol2, dir_id, dir_id, subdir1, renamedsubdir1));

    /* Re-create renamed directory */
    if ((poisondir_id = FPCreateDir(Conn2, vol2, dir_id, subdir1)) == 0) {
        test_failed();
    }

    /* Manually check name and CNID */
    FAIL(FPGetFileDirParams(Conn, vol1, subdir2_id, "", 0, bitmap));
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (filedir.did != subdir2_id) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %x should be %x\n", filedir.did, subdir2_id);
        }

        test_failed();
    }

    if (strcmp(filedir.lname, subdir2
              )) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED %s should be %s\n", filedir.lname, subdir2);
        }

        test_failed();
    }

fin:
    FAIL(FPCloseVol(Conn2, vol2));
    FAIL(FPDelete(Conn, vol1, subdir2_id, ""));
    FAIL(FPDelete(Conn, vol1, subdir1_id, ""));
    FAIL(FPDelete(Conn, vol1, poisondir_id, ""));
    FAIL(FPDelete(Conn, vol1, dir_id, ""));
test_exit:
    exit_test("Dircache:test506: stat subdir in poisoned path");
}

/*!
 * @brief test508: Dircache stress test with cache filling and evictions
 *
 * **Test Requirements:**
 * - Configure AFP_DIRCACHESIZE=1024 (small cache for reasonable test time)
 * - Works with both LRU and ARC modes
 *
 * This test exercises core dircache behaviors by creating more directories than
 * can fit in cache, forcing evictions and re-access patterns to test cache
 * management under stress.
 *
 * **Test Pattern:**
 * 1. Create 2100 directories (2x cache size)
 *    - During creation: periodically re-access early dirs to establish frequency patterns
 * 2. Multiple re-access passes over different ranges
 *    - Pass 1-3: Repeatedly access dirs 500-900 (tests frequent access handling)
 *    - Pass 4: Access dirs 800-1100 (overlapping range)
 *    - Pass 5: Access dirs 1000-1200 (different range for pattern adaptation)
 *
 * **Expected Behavior:**
 * - LRU: Simple eviction of least recently used entries, all re-accesses are cache hits if not evicted
 * - ARC: Evictions to ghost lists (B1/B2), ghost hits on re-access, parameter adaptation
 *
 * **Verification:** Check logs for evictions, cache hits, and (in ARC) ghost hits and adaptations
 */
STATIC void test508()
{
    uint16_t vol = VolID;
    const int NUM_DIRS =
        2100;  /* Cache=1024 + ghosts=1024 = 2048 capacity, 2100 causes minimal rotation */
    uint32_t *dir_ids = NULL;
    char dir_name[64];
    int created_count = 0;
    ENTER_TEST
    /* Allocate array for directory IDs */
    dir_ids = calloc(NUM_DIRS, sizeof(uint32_t));

    if (!dir_ids) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED to allocate memory for dir_ids\n");
        }

        test_failed();
        goto test_exit;
    }

    if (!Quiet) {
        printf("  Dircache stress test: %d dirs (AFP_DIRCACHESIZE=1024)\n", NUM_DIRS);
        printf("  LRU: ~1024 cached entries with evictions\n");
        printf("  ARC: ~1024 cached + ~1024 ghosts, ~52 fully rotated\n");
    }

    /* ============================================================
     * PHASE 1: Create many directories (> cache size)
     * Expected: Over-Fills T1 to trigger evictions to B1
     * ============================================================ */
    for (int i = 0; i < NUM_DIRS; i++) {
        snprintf(dir_name, sizeof(dir_name), "arc_stress_%04d", i);
        dir_ids[i] = FPCreateDir(Conn, vol, DIRDID_ROOT, dir_name);

        if (!dir_ids[i]) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED to create directory %d\n", i);
            }

            test_failed();
            goto cleanup;
        }

        created_count++;

        /* Every 10 dirs, access first 10 again to promote to T2 */
        if (i % 10 == 0 && i > 0) {
            int idx = (i / 10) % 10;
            snprintf(dir_name, sizeof(dir_name), "arc_stress_%04d", idx);
            /* Not using FAIL - this is optimization, not critical */
            FPGetFileDirParams(Conn, vol, DIRDID_ROOT, dir_name, 0,
                               (1 << DIRPBIT_DID));
        }
    }

    /* ============================================================
     * PHASE 2: Multiple reaccess passes to test ARC adaptation & drive the hit rate
     * Goal: Trigger ghost hits, adaptation, T1→T2 promotions, and cache hits
     * With 2100 dirs and cache=1024 (2c=2048), most dirs stay in cache or ghosts
     * ============================================================ */
    if (!Quiet) {
        printf("  Phase 2: Multiple reaccess passes to increase hit rate...\n");
    }

    /* Pass 1: Trigger ghost hits → promote to T2 */
    for (int i = 500; i < 900; i++) {
        snprintf(dir_name, sizeof(dir_name), "arc_stress_%04d", i);
        FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, dir_name, 0,
                                (1 << DIRPBIT_DID)));
    }

    /* Pass 2: T2 cache hits (same range) */
    for (int i = 500; i < 900; i++) {
        snprintf(dir_name, sizeof(dir_name), "arc_stress_%04d", i);
        FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, dir_name, 0,
                                (1 << DIRPBIT_DID)));
    }

    /* Pass 3: More T2 hits */
    for (int i = 500; i < 900; i++) {
        snprintf(dir_name, sizeof(dir_name), "arc_stress_%04d", i);
        FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, dir_name, 0,
                                (1 << DIRPBIT_DID)));
    }

    /* Pass 4: Concentrated hits on subset */
    for (int i = 800; i < 1100; i++) {
        snprintf(dir_name, sizeof(dir_name), "arc_stress_%04d", i);
        FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, dir_name, 0,
                                (1 << DIRPBIT_DID)));
    }

    /* Pass 5: Different range for adaptation */
    for (int i = 1000; i < 1200; i++) {
        snprintf(dir_name, sizeof(dir_name), "arc_stress_%04d", i);
        FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, dir_name, 0,
                                (1 << DIRPBIT_DID)));
    }

    if (!Quiet) {
        printf("  Phase 2 completed: 5 reaccess passes\n");
    }

    if (!Quiet) {
        printf("  ✓ Dircache stress test completed\n");
        printf("  \n");
        printf("  Expected activity in logs:\n");
        printf("    ✓ lookups: ~14000+ (comprehensive testing)\n");
        printf("    ✓ cache_hits > 0 (frequent re-access)\n");
        printf("    ✓ evictions > 0 (cache management under stress)\n");
        printf("    ✓ max_entries should reach 1024\n");
        printf("  \n");
        printf("  ARC-specific (if enabled):\n");
        printf("    ✓ ghost_hits > 0 (B1 and B2 ghost learning)\n");
        printf("    ✓ adaptations > 0 (p parameter tuning)\n");
        printf("    ✓ promotions_t1_to_t2 > 0 (frequency detection)\n");
        printf("    ✓ Note: Final table state (T1/T2/B1/B2) will be zero/low after cleanup.\n");
        printf("            Check operation counters to verify ARC was active.\n");
    }

cleanup:

    /* Cleanup: delete all created directories */
    if (!Quiet && created_count > 0) {
        printf("  Cleanup: Deleting %d test directories...\n", created_count);
    }

    if (dir_ids) {
        for (int i = 0; i < created_count; i++) {
            if (dir_ids[i]) {
                /* Delete directory by its ID (consistent with test500-506) */
                FPDelete(Conn, vol, dir_ids[i], "");
            }
        }

        free(dir_ids);
    }

test_exit:
    exit_test("Dircache:test508: Dircache stress test with evictions");
}

/*!
 * @brief Helper: Access a working set of directories multiple times
 *
 * Accesses directories in the specified range multiple times to establish
 * them as "frequently accessed" which should promote them to T2 in ARC.
 *
 * @param[in] vol       Volume ID
 * @param[in] start_idx Starting directory index
 * @param[in] end_idx   Ending directory index (exclusive)
 * @param[in] passes    Number of times to access each directory
 * @param[in] prefix    Directory name prefix
 * @returns Number of successful accesses
 */
static int access_working_set(uint16_t vol, int start_idx, int end_idx,
                              int passes, const char *prefix)
{
    char dir_name[64];
    int access_count = 0;
    uint16_t bitmap = (1 << DIRPBIT_DID);

    for (int pass = 0; pass < passes; pass++) {
        for (int i = start_idx; i < end_idx; i++) {
            snprintf(dir_name, sizeof(dir_name), "%s_%04d", prefix, i);

            if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, dir_name, 0, bitmap) == 0) {
                access_count++;
            }
        }
    }

    return access_count;
}

/*!
 * @brief Helper: Perform sequential scan of directories
 *
 * Sequentially accesses a range of directories once, simulating a scan operation
 * that should pass through ARC's T1 without evicting T2 entries, but would
 * evict LRU cache entries.
 *
 * @param[in] vol       Volume ID
 * @param[in] start_idx Starting directory index
 * @param[in] end_idx   Ending directory index (exclusive)
 * @param[in] prefix    Directory name prefix
 * @returns Number of successful accesses
 */
static int sequential_scan(uint16_t vol, int start_idx, int end_idx,
                           const char *prefix)
{
    char dir_name[64];
    int access_count = 0;
    uint16_t bitmap = (1 << DIRPBIT_DID);

    for (int i = start_idx; i < end_idx; i++) {
        snprintf(dir_name, sizeof(dir_name), "%s_%04d", prefix, i);

        if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, dir_name, 0, bitmap) == 0) {
            access_count++;
        }
    }

    return access_count;
}

/*!
 * @brief test509: Scan resistance test with realistic workload pattern
 *
 * **Test Requirements:**
 * - Configure AFP_DIRCACHESIZE=1024 (small cache, working set fits)
 * - Works with both LRU and ARC modes for performance comparison
 *
 * This test provides a realistic workload pattern that simulates common AFP usage:
 * frequently accessed directories (hot working set) mixed with sequential scans
 * (one-time directory traversals). This pattern is optimized for ARC's frequency
 * detection, but also exposes LRU's limitations for comparison purposes.
 *
 * **Realistic Workload Pattern:**
 * - Hot working set (300 dirs): User's active projects/folders (accessed 5x)
 * - Warm working set (200 dirs): Recently used folders (accessed 2x)
 * - Sequential scans (1500 dirs): Spotlight indexing, backups, virus scans
 *
 * **Test Pattern (5 cycles of working set + scan):**
 * 1. Create 2100 directories (cache=1024)
 * 2. Build hot working set: Access 300 directories 5x (very frequent)
 * 3. Build warm working set: Access 200 directories 2x (moderately frequent)
 * 4-13. Five cycles of:
 *    - Sequential scan: Access 1500 directories once (1.5x cache size)
 *    - Verify hot set: Access 300 hot directories 2x
 *    - Verify warm set: Access 200 warm directories 1x
 *
 * **Expected Behavior:**
 * - LRU: Each scan evicts all working sets → Re-access = misses every cycle
 * - ARC: Hot/warm sets stay in T2 → Re-access = hits every cycle
 *
 * **Key Differences from test508:**
 * - Separate hot (5x) and warm (2x) working sets to test frequency detection
 * - Multiple scan/verify cycles (5×) to show sustained advantage
 * - Larger scans (1500 dirs = 1.5x cache) to stress scan resistance
 * - Realistic workload pattern amplifies LRU vs ARC performance differences
 */
STATIC void test509()
{
    uint16_t vol = VolID;
    const int NUM_DIRS = 2100;
    const int HOT_SET_START = 0;
    const int HOT_SET_END = 300;      /* Hot working set: 300 dirs, accessed 5x */
    const int WARM_SET_START = 300;
    const int WARM_SET_END = 500;     /* Warm working set: 200 dirs, accessed 2x */
    const int SCAN_START = 600;
    const int SCAN_END = 2100;        /* Scan: 1500 dirs (1.5x cache size) */
    const int NUM_CYCLES = 5;         /* Repeat scan/verify pattern */
    const char *prefix = "scan_resist";
    uint32_t *dir_ids = NULL;
    char dir_name[64];
    int created_count = 0;
    int accesses = 0;
    ENTER_TEST
    /* Allocate array for directory IDs */
    dir_ids = calloc(NUM_DIRS, sizeof(uint32_t));

    if (!dir_ids) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED to allocate memory for dir_ids\n");
        }

        test_failed();
        goto test_exit;
    }

    if (!Quiet) {
        printf("  Scan resistance test: %d dirs, AFP_DIRCACHESIZE=1024\n", NUM_DIRS);
        printf("  Hot set: %d dirs (5x), Warm set: %d dirs (2x), Scan: %d dirs (1x)\n",
               HOT_SET_END - HOT_SET_START,
               WARM_SET_END - WARM_SET_START,
               SCAN_END - SCAN_START);
        printf("  Pattern: %d cycles of (working sets + scan) - realistic workload\n",
               NUM_CYCLES);
    }

    /* ============================================================
     * PHASE 1: Create all directories
     * ============================================================ */
    if (!Quiet) {
        printf("  Phase 1: Creating %d directories...\n", NUM_DIRS);
    }

    for (int i = 0; i < NUM_DIRS; i++) {
        snprintf(dir_name, sizeof(dir_name), "%s_%04d", prefix, i);
        dir_ids[i] = FPCreateDir(Conn, vol, DIRDID_ROOT, dir_name);

        if (!dir_ids[i]) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED to create directory %d\n", i);
            }

            test_failed();
            goto cleanup;
        }

        created_count++;
    }

    /* ============================================================
     * PHASE 2: Build HOT working set (5x access → strong T2 promotion)
     * ============================================================ */
    if (!Quiet) {
        printf("  Phase 2: Building HOT working set (5x access)...\n");
    }

    accesses = access_working_set(vol, HOT_SET_START, HOT_SET_END, 5, prefix);

    if (!Quiet) {
        printf("    Hot set accesses: %d (expected: %d)\n",
               accesses, (HOT_SET_END - HOT_SET_START) * 5);
    }

    /* ============================================================
     * PHASE 3: Build WARM working set (2x access → moderate T2 promotion)
     * ============================================================ */
    if (!Quiet) {
        printf("  Phase 3: Building WARM working set (2x access)...\n");
    }

    accesses = access_working_set(vol, WARM_SET_START, WARM_SET_END, 2, prefix);

    if (!Quiet) {
        printf("    Warm set accesses: %d (expected: %d)\n",
               accesses, (WARM_SET_END - WARM_SET_START) * 2);
    }

    /* ============================================================
     * PHASES 4-13: Five cycles of scan + verify pattern
     * This demonstrates sustained ARC advantage over LRU
     * ============================================================ */
    for (int cycle = 1; cycle <= NUM_CYCLES; cycle++) {
        if (!Quiet) {
            printf("  \n");
            printf("  === CYCLE %d/%d ===\n", cycle, NUM_CYCLES);
        }

        /* Sequential scan (should pollute T1, not T2 in ARC) */
        if (!Quiet) {
            printf("    Scan: %d dirs (1.5x cache size)...\n",
                   SCAN_END - SCAN_START);
        }

        (void)sequential_scan(vol, SCAN_START, SCAN_END, prefix);

        /* Verify hot working set (should HIT in ARC T2, MISS in LRU) */
        if (!Quiet) {
            printf("    Verify hot set: %d dirs (2x access)...\n",
                   HOT_SET_END - HOT_SET_START);
        }

        accesses = access_working_set(vol, HOT_SET_START, HOT_SET_END, 2, prefix);

        if (!Quiet) {
            printf("      Hot set accesses: %d\n", accesses);
        }

        /* Verify warm working set (should HIT in ARC T2, MISS in LRU) */
        if (!Quiet) {
            printf("    Verify warm set: %d dirs (1x access)...\n",
                   WARM_SET_END - WARM_SET_START);
        }

        accesses = access_working_set(vol, WARM_SET_START, WARM_SET_END, 1, prefix);

        if (!Quiet) {
            printf("      Warm set accesses: %d\n", accesses);
        }
    }

    if (!Quiet) {
        printf("  \n");
        printf("  ✓ Scan resistance test completed\n");
        printf("  \n");
        printf("  Expected results in logs (for comparison):\n");
        printf("    LRU behavior:\n");
        printf("      - Working sets evicted by each scan\n");
        printf("      - Low cache hits on working sets after scans\n");
        printf("      - No frequency vs recency distinction\n");
        printf("    ARC advantages (if enabled):\n");
        printf("      - High cache hits on working sets in all cycles\n");
        printf("      - T2 size ~500 (hot 300 + warm 200) stable\n");
        printf("      - T1 fluctuates as scans pass through\n");
        printf("      - Scans don't evict working sets from T2\n");
        printf("      - Demonstrates frequency-based cache retention\n");
    }

cleanup:

    /* Cleanup: delete all created directories */
    if (!Quiet && created_count > 0) {
        printf("  Cleanup: Deleting %d test directories...\n", created_count);
    }

    if (dir_ids) {
        for (int i = 0; i < created_count; i++) {
            if (dir_ids[i]) {
                FPDelete(Conn, vol, dir_ids[i], "");
            }
        }

        free(dir_ids);
    }

test_exit:
    exit_test("Dircache:test509: Scan resistance test (realistic workload pattern)");
}

void Dircache_attack_test()
{
    ENTER_TESTSET
    test500();
    test501();
    test502();
    test503();
    test504();
    test505();
    test506();
    test508();  /* Dircache stress test - exercises evictions (both LRU and ARC) */
    test509();  /* Scan resistance test - realistic workload exposes frequency detection benefits */
}
