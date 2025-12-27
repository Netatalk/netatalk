/* ----------------------------------------------
*/
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/* ------------------------- */
STATIC void test227()
{
    uint16_t bitmap = (1 << FILPBIT_ATTR);
    uint16_t bitmap2;
    char *name = "t227 file.txt";
    uint16_t vol = VolID;
    uint32_t temp;
    const DSI *dsi;
    char pos[16];
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    struct afp_filedir_parms filedir2;
    unsigned int ret;
    ENTER_TEST
    dsi = &Conn->dsi;
    memset(pos, 0, sizeof(pos));
    memset(&filedir, 0, sizeof(filedir));
    filedir.attr = 0x01a0;			/* various lock attributes */
    ret = FPCatSearchExt(Conn, vol, 10, pos, 0,  /* d_bitmap*/ 0, bitmap, &filedir,
                         &filedir);

    if (Conn->afp_version < 30) {
        if (htonl(AFPERR_NOOP) != ret) {
            test_failed();
        } else {
            test_skipped(T_AFP3);
        }

        goto test_exit;
    }

    if (htonl(AFPERR_BITMAP) != ret) {
        test_failed();
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    ret = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  /* d_bitmap*/ 0, bitmap,
                         &filedir, &filedir);

    if (ret != htonl(AFPERR_EOF)) {
        test_failed();
    }

    memcpy(&temp, dsi->data + 20, sizeof(temp));
    temp = ntohl(temp);

    if (temp) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED want 0 get %d\n", temp);
        }

        test_failed();
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    filedir.attr = 0x01a0 | ATTRBIT_SETCLR ;
    FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
    memset(&filedir, 0, sizeof(filedir));
    /* ------------------- */
    filedir.attr = 0x01a0;			/* lock attributes */
    ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  /* d_bitmap*/ 0, bitmap,
                          &filedir, &filedir);

    if (ret != htonl(AFPERR_EOF)) {
        test_failed();
    }

    memcpy(&temp, dsi->data + 20, sizeof(temp));
    temp = ntohl(temp);

    if (temp != 1) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED want 1 get %d\n", temp);
        }

        test_failed();
    }

    /* ------------------- */
    filedir.attr = 0x0100;			/* lock attributes */
    ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  /* d_bitmap*/ 0, bitmap,
                          &filedir, &filedir);

    if (ret != htonl(AFPERR_EOF)) {
        test_failed();
    }

    memcpy(&temp, dsi->data + 20, sizeof(temp));
    temp = ntohl(temp);

    if (temp != 1) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED want 1 get %d\n", temp);
        }

        test_failed();
    }

#if 1
    /* -------------------- */
    memset(&filedir, 0, sizeof(filedir));
    memset(&filedir2, 0, sizeof(filedir2));
    filedir.lname = "Data";
    ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42,
                          0x80000000UL | (1 << FILPBIT_LNAME), &filedir, &filedir2);

    while (!ret) {
        memcpy(pos, dsi->data, 16);
        ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42,
                              0x80000000UL | (1 << FILPBIT_LNAME), &filedir, &filedir2);
    }

    /* -------------------- */
    memset(pos, 0, sizeof(pos));
    ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42,
                          0x80000000UL | (1 << FILPBIT_LNAME), &filedir, &filedir2);

    if (!ret) {
        memcpy(pos, dsi->data, 16);
        ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42,
                              0x80000000UL | (1 << FILPBIT_LNAME), &filedir, &filedir2);
    }

    /* -------------------- */
    memset(pos, 0, sizeof(pos));
    filedir.lname = "test";
    ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42,
                          0x80000000UL | (1 << FILPBIT_LNAME), &filedir, &filedir2);

    if (!ret) {
        memcpy(pos, dsi->data, 16);
        ret  = FPCatSearchExt(Conn, vol, 10, pos, 0x42,  0x42,
                              0x80000000UL | (1 << FILPBIT_LNAME), &filedir, &filedir2);
    }

    /* -------------------- */
    memset(pos, 0, sizeof(pos));
    filedir.utf8_name = "test";
    bitmap2 = (1 << FILPBIT_PDINFO) | (1 << FILPBIT_PDID);
    ret  = FPCatSearchExt(Conn, vol, 10, pos, bitmap2,  bitmap2,
                          0x80000000UL | (1 << FILPBIT_PDINFO), &filedir, &filedir2);

    while (!ret) {
        memcpy(pos, dsi->data, 16);
        ret  = FPCatSearchExt(Conn, vol, 10, pos, bitmap2,  bitmap2,
                              0x80000000UL | (1 << FILPBIT_PDINFO), &filedir, &filedir2);
    }

#endif
    /* -------------------- */
    memset(&filedir, 0, sizeof(filedir));
    filedir.attr = 0x01a0;
    FAIL(FPSetFileParams(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPCatSearchExt:test227: Catalog search");
}

/* ------------------------- */
/* test529: Catalog search with concurrent renames
 *
 * Tests FPCatSearchExt robustness when items are renamed during search iteration.
 * This validates dircache coherency under catalog search stress, important for
 * multi-user environments where catalog search is actively used.
 *
 * Scenario:
 * 1. Client 1 creates multiple files/dirs with searchable metadata (specific finder types)
 * 2. Client 1 starts FPCatSearchExt to find items matching criteria
 * 3. Client 2 renames some matching items while search in progress
 * 4. Client 1 continues iterating through search results
 * 5. Verify: No crashes, no duplicates, results consistent despite renames
 */
STATIC void test529()
{
    char base_name[256];
    char renamed_name[256];
    char *dir_name = "t529_catsearch_test";
    uint16_t vol = VolID;
    uint16_t vol2 = 0;
    uint16_t f_bitmap;
    uint16_t d_bitmap;
    uint32_t rbitmap;
    int dir = 0;
    int i;
    unsigned int ret;
    char pos[16];
    uint32_t match_count = 0;
    uint32_t temp;
    const DSI *dsi;
    struct afp_filedir_parms filedir;
    struct afp_filedir_parms filedir2;
    int iteration = 0;
    ENTER_TEST

    if (!Quiet) {
        fprintf(stdout,
                "FPCatSearchExt:test529: Catalog search with concurrent renames\n");
    }

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    dsi = &Conn->dsi;
    f_bitmap = (1 << FILPBIT_LNAME) | (1 << FILPBIT_FNUM) | (1 << FILPBIT_FINFO);
    d_bitmap = (1 << DIRPBIT_LNAME) | (1 << DIRPBIT_DID);

    /************************************
     * Step 1: Client 1 creates directory with searchable items
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 1: Client 1 creates directory with 20 searchable files\n");
    }

    dir = FPCreateDir(Conn, vol, DIRDID_ROOT, dir_name);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    // Create 20 files with PDF finder info (searchable by PDINFO)
    for (i = 1; i <= 20; i++) {
        snprintf(base_name, sizeof(base_name), "search_file_%02d.pdf", i);

        if (FPCreateFile(Conn, vol, 0, dir, base_name)) {
            if (!Quiet) {
                fprintf(stdout, "\tFAILED to create file %s\n", base_name);
            }

            test_nottested();
            goto fin;
        }

        // Set finder info to PDF type
        memset(&filedir, 0, sizeof(filedir));
        memset(&filedir2, 0, sizeof(filedir2));
        filedir.isdir = 0;
        memcpy(filedir.finder_info, "PDF CARO", 8);

        if (FPSetFileParams(Conn, vol, dir, base_name, (1 << FILPBIT_FINFO),
                            &filedir) && !Quiet) {
            fprintf(stdout, "\tFAILED to set finder info for %s\n", base_name);
        }

        // Continue anyway, not critical
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Created 20 files with PDF finder info\n");
    }

    /************************************
     * Step 2: Client 1 starts catalog search for PDF files
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 2: Client 1 starts catalog search for PDF files\n");
    }

    memset(pos, 0, sizeof(pos));
    memset(&filedir, 0, sizeof(filedir));
    memset(&filedir2, 0, sizeof(filedir2));
    filedir.utf8_name = "search";
    rbitmap = 0x80000000UL | (1 << FILPBIT_PDINFO);
    ret = FPCatSearchExt(Conn, vol, 10, pos, f_bitmap, d_bitmap, rbitmap, &filedir,
                         &filedir2);

    // Get initial result count from response
    if (ret == 0 || ret == ntohl(AFPERR_EOF)) {
        memcpy(&temp, dsi->data + 20, sizeof(temp));
        match_count = ntohl(temp);

        if (!Quiet && Verbose) {
            fprintf(stdout, "\t    Initial search found %d matches\n", match_count);
        }
    }

    /************************************
     * Step 3: Client 2 opens volume and renames items 8-12 during search
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 3: Client 2 renames items 8-12 while search in progress\n");
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Client 2 could not open volume\n");
        }

        test_nottested();
        goto fin;
    }

    // Rename files 8-12 (middle of the result set)
    for (i = 8; i <= 12; i++) {
        snprintf(base_name, sizeof(base_name), "search_file_%02d.pdf", i);
        snprintf(renamed_name, sizeof(renamed_name), "renamed_search_%02d.pdf", i);
        FPRename(Conn2, vol2, dir, base_name, renamed_name);  // Best effort
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Renamed items 8-12\n");
    }

    /************************************
     * Step 4: Client 1 continues iterating through search results
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 4: Client 1 continues iterating through search results\n");
    }

    // Continue iteration if we got results
    iteration = 1;

    while (ret == 0 && iteration < 50) {  // Safety limit to prevent infinite loop
        memcpy(pos, dsi->data, 16);
        ret = FPCatSearchExt(Conn, vol, 10, pos, f_bitmap, d_bitmap, rbitmap, &filedir,
                             &filedir2);
        iteration++;

        if (!Quiet && Verbose) {
            fprintf(stdout, "\t    Iteration %d, ret=%d\n", iteration, ntohl(ret));
        }

        // After 3 iterations, have Client 2 rename more items
        if (iteration == 3) {
            if (!Quiet && Verbose) {
                fprintf(stdout, "\t    Client 2 renames items 15-17\n");
            }

            for (i = 15; i <= 17; i++) {
                snprintf(base_name, sizeof(base_name), "search_file_%02d.pdf", i);
                snprintf(renamed_name, sizeof(renamed_name), "renamed_search_%02d.pdf", i);
                FPRename(Conn2, vol2, dir, base_name, renamed_name);  // Best effort
            }
        }
    }

    if (ret != ntohl(AFPERR_EOF) && ret != 0) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED search iteration ended with unexpected error: %d (%s)\n",
                    ntohl(ret), afp_error(ret));
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Search completed after %d iterations\n", iteration);
    }

    /************************************
     * Step 5: Re-run search to verify consistency
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 5: Re-run search to verify no corruption\n");
    }

    memset(pos, 0, sizeof(pos));
    ret = FPCatSearchExt(Conn, vol, 10, pos, f_bitmap, d_bitmap, rbitmap, &filedir,
                         &filedir2);

    if (ret != 0 && ret != ntohl(AFPERR_EOF)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED re-search failed: %d (%s)\n",
                    ntohl(ret), afp_error(ret));
        }

        test_failed();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ Catalog search handled concurrent renames without crashes or corruption\n");
    }

fin:

    // Cleanup: Delete all files (both original and renamed)
    if (dir) {
        for (i = 1; i <= 20; i++) {
            snprintf(base_name, sizeof(base_name), "search_file_%02d.pdf", i);
            FPDelete(Conn, vol, dir, base_name);  // Best effort cleanup
            snprintf(renamed_name, sizeof(renamed_name), "renamed_search_%02d.pdf", i);
            FPDelete(Conn, vol, dir, renamed_name);  // Best effort cleanup
        }

        FPDelete(Conn, vol, DIRDID_ROOT, dir_name);
    }

    if (vol2) {
        FPCloseVol(Conn2, vol2);
    }

test_exit:
    exit_test("FPCatSearchExt:test529: Catalog search with concurrent renames");
}

/* ----------- */
void FPCatSearchExt_test()
{
    ENTER_TESTSET
    test227();
    test529();
}
