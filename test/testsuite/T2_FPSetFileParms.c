/* ----------------------------------------------
*/
#include "adoublehelper.h"
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/* ------------------------ */
static int afp_symlink(char *oldpath, char *newpath)
{
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap;
    uint16_t vol = VolID;
    const DSI *dsi;
    int fork = 0;
    dsi = &Conn->dsi;

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, newpath)) {
        return -1;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, newpath,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        return -1;
    }

    if (FPWrite(Conn, fork, 0, (int)strlen(oldpath), oldpath, 0)) {
        return -1;
    }

    if (FPCloseFork(Conn, fork)) {
        return -1;
    }

    fork = 0;
    bitmap = (1 << DIRPBIT_ATTR) | (1 << FILPBIT_FINFO) |
             (1 << DIRPBIT_CDATE) | (1 << DIRPBIT_MDATE) |
             (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_PDID) | (1 << FILPBIT_FNUM);

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, newpath, bitmap, 0)) {
        return -1;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    memcpy(filedir.finder_info, "slnkrhap", 8);
    bitmap = (1 << FILPBIT_FINFO);

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT, newpath, bitmap, &filedir)) {
        return -1;
    }

    return 0;
}

/* ------------------------- */
STATIC void test89()
{
    int  dir;
    char *file = "t89 test error setfilparam";
    char *name = "t89 error setfilparams dir";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 << FILPBIT_FINFO) | (1 << FILPBIT_CDATE) |
                      (1 << FILPBIT_BDATE) | (1 << FILPBIT_MDATE);
    uint16_t vol = VolID;
    const DSI *dsi = &Conn->dsi;
    unsigned ret;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!(dir = folder_with_ro_adouble(vol, DIRDID_ROOT, name, file))) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, dir, file, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
        ret = FPSetFileParams(Conn, vol, dir, file, bitmap, &filedir);

        if (not_valid(ret, 0, AFPERR_ACCESS)) {
            test_failed();
        }
    }

    bitmap = (1 << FILPBIT_MDATE);

    if (FPGetFileDirParams(Conn, vol, dir, file, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
        FAIL(FPSetFileParams(Conn, vol, dir, file, bitmap, &filedir))
    }

    delete_ro_adouble(vol, dir, file);
test_exit:
    exit_test("FPSetFileParms:test89: test set file setfilparam");
}

/* ------------------------- */
STATIC void test120()
{
    char *name = "t120 test file setfilparam";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap = (1 << FILPBIT_ATTR) | (1 << FILPBIT_FINFO) |
                      (1 << FILPBIT_CDATE) |
                      (1 << FILPBIT_BDATE) | (1 << FILPBIT_MDATE);
    uint16_t vol = VolID;
    const DSI *dsi = &Conn->dsi;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
    } else {
        filedir.isdir = 0;
        afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
        delete_unix_md(Path, "", name);
        FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetFileParms:test120: test set file setfilparam (create .AppleDouble)");
}

/* ------------------------- */
STATIC void test426()
{
    char *name = "t426 Symlink";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    uint16_t bitmap;
    uint16_t vol = VolID;
    DSI *dsi;
    int fork = 0;
    unsigned int ret;
    char temp[MAXPATHLEN];
    struct stat st;
    dsi = &Conn->dsi;
    ENTER_TEST

    /* FIXME: This test hangs with AFP2.x (3.1.12 - 4.0.3) */
    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        return;
    }

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (afp_symlink("t426 dest", name)) {
        test_nottested();
        goto test_exit;
    }

    /* Check if volume uses option 'followsymlinks' */
    sprintf(temp, "%s/%s", Path, name);

    if (lstat(temp, &st)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED stat( %s ) %s\n", temp, strerror(errno));
        }

        test_failed();
    }

    if (!S_ISLNK(st.st_mode)) {
        test_skipped(T_NOSYML);
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
    } else {
        char *ln2 = "t426 dest 2";
        ret = FPWrite_ext(Conn, fork, 0, (off_t)strlen(ln2), ln2, 0);

        if (not_valid_bitmap(ret, BITERR_ACCESS | BITERR_MISC, AFPERR_MISC)) {
            test_failed();
        }

        FPCloseFork(Conn, fork);
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name, OPENACC_RD);

    if (!fork) {
        /* Trying to open the linked file? */
        test_failed();
    }

    if (fork) {
        FPCloseFork(Conn, fork);
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetFileParms:test426: Create a dangling symlink");
}

/* test543: FinderInfo round-trip verification
 *
 * This test validates that FinderInfo set via FPSetFileParams is correctly
 * stored and can be read back identically via FPGetFileDirParms.
 *
 * Tests all 32 bytes of FinderInfo:
 * - Bytes 0-3: File Type (e.g., "TEST")
 * - Bytes 4-7: Creator Code (e.g., "CRE8")
 * - Bytes 8-9: Finder Flags
 * - Bytes 10-31: Extended Finder Info (location, folder, etc.)
 *
 * Scenario:
 * 1. Create a file
 * 2. Read initial FinderInfo (should be zeroed)
 * 3. Set custom FinderInfo with known type+creator+flags
 * 4. Read back FinderInfo
 * 5. Compare all 32 bytes byte-for-byte
 * 6. Set different FinderInfo (overwrite)
 * 7. Read back and verify the overwrite
 * 8. Clean up
 */
STATIC void test543()
{
    char *name = "t533 FinderInfo roundtrip";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    char saved_finfo[32];
    uint16_t bitmap = (1 << FILPBIT_FINFO);
    uint16_t vol = VolID;
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
     * Step 2: Read initial FinderInfo (should be zeroed for new file)
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 2: Read initial FinderInfo\n");
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

    /************************************
     * Step 3: Set custom FinderInfo with known pattern
     *
     * Bytes 0-3:   Type     = "TEST"
     * Bytes 4-7:   Creator  = "CRE8"
     * Bytes 8-9:   Flags    = 0x04 0x00 (kHasCustomIcon)
     * Bytes 10-15: Location = {100, 200} (x=100, y=200)
     * Bytes 16-17: Folder   = 0x00 0x00
     * Bytes 18-31: Extended = pattern 0xAA fill
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 3: Set custom FinderInfo (type=TEST, creator=CRE8)\n");
    }

    /* Build custom FinderInfo */
    memset(filedir.finder_info, 0, 32);
    memcpy(filedir.finder_info + 0, "TEST", 4);      /* Type */
    memcpy(filedir.finder_info + 4, "CRE8", 4);      /* Creator */
    filedir.finder_info[8] = 0x04;                    /* Flags high byte */
    filedir.finder_info[9] = 0x00;                    /* Flags low byte */
    filedir.finder_info[10] = 0x00;                   /* Location X high */
    filedir.finder_info[11] = 0x64;                   /* Location X low (100) */
    filedir.finder_info[12] = 0x00;                   /* Location Y high */
    filedir.finder_info[13] = 0xC8;                   /* Location Y low (200) */

    /* Fill extended info bytes 18-31 with pattern */
    for (int i = 18; i < 32; i++) {
        filedir.finder_info[i] = (char)0xAA;
    }

    /* Save what we set for comparison */
    memcpy(saved_finfo, filedir.finder_info, 32);

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPSetFileParams\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 4: Read back FinderInfo
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 4: Read back FinderInfo\n");
    }

    memset(&filedir, 0, sizeof(filedir));

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPGetFileDirParams readback\n");
        }

        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

    /************************************
     * Step 5: Compare all 32 bytes byte-for-byte
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Compare FinderInfo (32 bytes)\n");
    }

    if (memcmp(saved_finfo, filedir.finder_info, 32) != 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FinderInfo mismatch!\n");
            fprintf(stdout, "\t  Expected: ");

            for (int i = 0; i < 32; i++) {
                fprintf(stdout, "%02x",
                        (unsigned char)saved_finfo[i]);
            }

            fprintf(stdout, "\n\t  Got:      ");

            for (int i = 0; i < 32; i++) {
                fprintf(stdout, "%02x",
                        (unsigned char)filedir.finder_info[i]);
            }

            fprintf(stdout, "\n");
        }

        test_failed();
        goto fin;
    }

    /* Verify specific fields */
    if (memcmp(filedir.finder_info, "TEST", 4) != 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Type field mismatch\n");
        }

        test_failed();
        goto fin;
    }

    if (memcmp(filedir.finder_info + 4, "CRE8", 4) != 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Creator field mismatch\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Type='%.4s' Creator='%.4s' - match\n",
                filedir.finder_info, filedir.finder_info + 4);
    }

    /************************************
     * Step 6: Set different FinderInfo (overwrite test)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 6: Overwrite with new FinderInfo (type=PDF, creator=CARO)\n");
    }

    memset(filedir.finder_info, 0, 32);
    memcpy(filedir.finder_info + 0, "PDF ", 4);      /* Type */
    memcpy(filedir.finder_info + 4, "CARO", 4);      /* Creator */
    memcpy(saved_finfo, filedir.finder_info, 32);

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPSetFileParams overwrite\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 7: Read back and verify the overwrite
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 7: Read back overwritten FinderInfo and verify\n");
    }

    memset(&filedir, 0, sizeof(filedir));

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED FPGetFileDirParams overwrite readback\n");
        }

        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

    if (memcmp(saved_finfo, filedir.finder_info, 32) != 0) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED overwritten FinderInfo mismatch!\n");
        }

        test_failed();
        goto fin;
    }

    if (memcmp(filedir.finder_info, "PDF ", 4) != 0 ||
            memcmp(filedir.finder_info + 4, "CARO", 4) != 0) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED overwritten type/creator mismatch\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ FinderInfo round-trip verified (set, read, compare, overwrite, verify)\n");
    }

fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetFileParms:test543: FinderInfo round-trip verification");
}

/* test534: File dates round-trip verification
 *
 * This test validates that file dates (creation, backup, modification)
 * set via FPSetFileParams are correctly stored and can be read back
 * identically via FPGetFileDirParms.
 *
 * AFP dates are uint32_t seconds since 2000-01-01 00:00:00 UTC.
 *
 * Tests:
 * - Backup date (bdate): stored purely in AppleDouble metadata
 * - Creation date (cdate): stored in AppleDouble metadata
 * - Modification date (mdate): may be influenced by filesystem, but
 *   should round-trip when no filesystem writes occur between set and get
 *
 * Scenario:
 * 1. Create a file
 * 2. Read initial dates
 * 3. Set custom date values (known timestamps)
 * 4. Read back dates
 * 5. Compare each date value
 * 6. Set different dates (overwrite)
 * 7. Read back and verify overwrite
 * 8. Clean up
 */
STATIC void test534()
{
    char *name = "t534 date roundtrip";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    uint32_t saved_cdate, saved_bdate, saved_mdate;
    uint16_t bitmap = (1 << FILPBIT_CDATE) | (1 << FILPBIT_BDATE) |
                      (1 << FILPBIT_MDATE);
    uint16_t vol = VolID;
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
     * Step 2: Read initial dates
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 2: Read initial dates\n");
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

    if (!Quiet && Verbose) {
        fprintf(stdout,
                "\t    Initial: cdate=0x%08x bdate=0x%08x mdate=0x%08x\n",
                filedir.cdate, filedir.bdate, filedir.mdate);
    }

    /************************************
     * Step 3: Set custom date values
     *
     * Use specific known timestamps:
     * - cdate: 0x1A2B3C4D (Jan 2014 range in AFP epoch)
     * - bdate: 0x2B3C4D5E (a different known value)
     * - mdate: 0x3C4D5E6F (yet another known value)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 3: Set custom dates (cdate=0x1A2B3C4D, bdate=0x2B3C4D5E, mdate=0x3C4D5E6F)\n");
    }

    filedir.cdate = htonl(0x1A2B3C4D);
    filedir.bdate = htonl(0x2B3C4D5E);
    filedir.mdate = htonl(0x3C4D5E6F);
    saved_cdate = filedir.cdate;
    saved_bdate = filedir.bdate;
    saved_mdate = filedir.mdate;

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPSetFileParams for dates\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 4: Read back dates
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 4: Read back dates\n");
    }

    memset(&filedir, 0, sizeof(filedir));

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPGetFileDirParams readback\n");
        }

        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

    /************************************
     * Step 5: Compare each date value
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 5: Compare date values\n");
    }

    if (!Quiet && Verbose) {
        fprintf(stdout,
                "\t    Readback: cdate=0x%08x bdate=0x%08x mdate=0x%08x\n",
                filedir.cdate, filedir.bdate, filedir.mdate);
    }

    if (filedir.cdate != saved_cdate) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED cdate mismatch: expected 0x%08x got 0x%08x\n",
                    saved_cdate, filedir.cdate);
        }

        test_failed();
        goto fin;
    }

    if (filedir.bdate != saved_bdate) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED bdate mismatch: expected 0x%08x got 0x%08x\n",
                    saved_bdate, filedir.bdate);
        }

        test_failed();
        goto fin;
    }

    if (filedir.mdate != saved_mdate) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED mdate mismatch: expected 0x%08x got 0x%08x\n",
                    saved_mdate, filedir.mdate);
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 6: Set different dates (overwrite test)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 6: Overwrite with different dates\n");
    }

    filedir.cdate = htonl(0x55667788);
    filedir.bdate = htonl(0x11223344);
    filedir.mdate = htonl(0x99AABBCC);
    saved_cdate = filedir.cdate;
    saved_bdate = filedir.bdate;
    saved_mdate = filedir.mdate;

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPSetFileParams overwrite\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 7: Read back and verify the overwrite
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 7: Read back overwritten dates and verify\n");
    }

    memset(&filedir, 0, sizeof(filedir));

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED FPGetFileDirParams overwrite readback\n");
        }

        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);

    if (filedir.cdate != saved_cdate) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED cdate overwrite mismatch: expected 0x%08x got 0x%08x\n",
                    saved_cdate, filedir.cdate);
        }

        test_failed();
        goto fin;
    }

    if (filedir.bdate != saved_bdate) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED bdate overwrite mismatch: expected 0x%08x got 0x%08x\n",
                    saved_bdate, filedir.bdate);
        }

        test_failed();
        goto fin;
    }

    if (filedir.mdate != saved_mdate) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED mdate overwrite mismatch: expected 0x%08x got 0x%08x\n",
                    saved_mdate, filedir.mdate);
        }

        test_failed();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ File dates round-trip verified (cdate, bdate, mdate set/read/compare/overwrite)\n");
    }

fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetFileParms:test534: File dates round-trip verification");
}

/* test538: Multi-client FinderInfo consistency
 *
 * This test validates that FinderInfo set by one client is immediately
 * visible to a second client reading the same file. Cross-process
 * cache hints (IPC_CACHE_HINT) ensure Client 2's dircache is
 * invalidated when Client 1 modifies the file's metadata.
 *
 * Scenario:
 * 1. Client 1 creates a file
 * 2. Client 1 sets custom FinderInfo (type=TSTA, creator=CRE1)
 * 3. Client 2 reads FinderInfo - verify 32-byte match
 * 4. Client 1 updates FinderInfo (type=TSTB, creator=CRE2)
 * 5. Client 2 reads again - verify updated 32-byte match
 */
STATIC void test538()
{
    char *name = "t538 multiclient finfo";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir = { 0 };
    struct afp_filedir_parms filedir2 = { 0 };
    char saved_finfo[32];
    uint16_t bitmap = (1 << FILPBIT_FINFO);
    uint16_t vol = VolID;
    uint16_t vol2 = 0;
    const DSI *dsi = &Conn->dsi;
    const DSI *dsi2;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    /************************************
     * Step 1: Client 1 creates file
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 1: Client 1 creates test file\n");
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    /************************************
     * Step 2: Client 1 sets custom FinderInfo
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 2: Client 1 sets FinderInfo (type=TSTA, creator=CRE1)\n");
    }

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    memset(filedir.finder_info, 0, 32);
    memcpy(filedir.finder_info + 0, "TSTA", 4);
    memcpy(filedir.finder_info + 4, "CRE1", 4);
    filedir.finder_info[8] = 0x04;
    filedir.finder_info[9] = 0x00;
    memcpy(saved_finfo, filedir.finder_info, 32);

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPSetFileParams\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 3: Client 2 reads FinderInfo - verify match
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 3: Client 2 reads FinderInfo (cross-client)\n");
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Client 2 could not open volume\n");
        }

        test_nottested();
        goto fin;
    }

    dsi2 = &Conn2->dsi;

    if (FPGetFileDirParams(Conn2, vol2, DIRDID_ROOT, name, bitmap, 0)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 FPGetFileDirParams\n");
        }

        test_failed();
        goto fin;
    }

    filedir2.isdir = 0;
    afp_filedir_unpack(&filedir2, dsi2->data + ofs, bitmap, 0);

    if (memcmp(saved_finfo, filedir2.finder_info, 32) != 0) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 FinderInfo mismatch!\n");
            fprintf(stdout, "\t  Client 1 set: ");

            for (int i = 0; i < 32; i++) {
                fprintf(stdout, "%02x",
                        (unsigned char)saved_finfo[i]);
            }

            fprintf(stdout, "\n\t  Client 2 got: ");

            for (int i = 0; i < 32; i++) {
                fprintf(stdout, "%02x",
                        (unsigned char)filedir2.finder_info[i]);
            }

            fprintf(stdout, "\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout,
                "\t    Client 2 read type='%.4s' creator='%.4s' - match\n",
                filedir2.finder_info, filedir2.finder_info + 4);
    }

    /************************************
     * Step 4: Client 1 updates FinderInfo
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 4: Client 1 updates FinderInfo (type=TSTB, creator=CRE2)\n");
    }

    memset(filedir.finder_info, 0, 32);
    memcpy(filedir.finder_info + 0, "TSTB", 4);
    memcpy(filedir.finder_info + 4, "CRE2", 4);
    filedir.finder_info[8] = 0x08;
    filedir.finder_info[9] = 0x00;
    memcpy(saved_finfo, filedir.finder_info, 32);

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPSetFileParams update\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 5: Client 2 reads updated FinderInfo
     *
     * Cross-process cache hints (IPC_CACHE_HINT) ensure Client 2's
     * dircache is invalidated when Client 1 modifies the file's
     * FinderInfo — no volume close/reopen workaround needed.
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Client 2 reads updated FinderInfo\n");
    }

    memset(&filedir2, 0, sizeof(filedir2));

    if (FPGetFileDirParams(Conn2, vol2, DIRDID_ROOT, name, bitmap, 0)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 FPGetFileDirParams update\n");
        }

        test_failed();
        goto fin;
    }

    filedir2.isdir = 0;
    afp_filedir_unpack(&filedir2, dsi2->data + ofs, bitmap, 0);

    if (memcmp(saved_finfo, filedir2.finder_info, 32) != 0) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 updated FinderInfo mismatch!\n");
        }

        test_failed();
        goto fin;
    }

    if (memcmp(filedir2.finder_info, "TSTB", 4) != 0 ||
            memcmp(filedir2.finder_info + 4, "CRE2", 4) != 0) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 type/creator mismatch in update\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ Multi-client FinderInfo consistency verified (set→read→update→read)\n");
    }

fin:

    if (vol2) {
        FPCloseVol(Conn2, vol2);
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetFileParms:test538: Multi-client FinderInfo consistency");
}

/* ----------- */
void T2FPSetFileParms_test()
{
    ENTER_TESTSET
    test89();
    test120();
    test426();
    test543();
    test534();
    test538();
}
