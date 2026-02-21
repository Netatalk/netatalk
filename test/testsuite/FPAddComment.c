/* ----------------------------------------------
*/
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/* -------------------------- */
STATIC void test55()
{
    int fork;
    char *name  = "t55 dir no access";
    char *name1 = "t55 file.txt";
    char *name2 = "t55 ro dir";
    char *name3  = "t55 --rwx-- dir";
    int pdir = 0;
    int rdir = 0;
    int dir;
    int ret;
    uint16_t vol = VolID;
    uint16_t vol2;
    DSI *dsi2;
    DSI *dsi = &Conn->dsi;
    char *cmt;
    int  dt = 0;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    if (!(rdir = read_only_folder(vol, DIRDID_ROOT, name2))) {
        test_nottested();
        goto test_exit;
    }

    if (!(pdir = no_access_folder(vol, DIRDID_ROOT, name)) && !Quiet) {
        fprintf(stdout, "\tWARNING folder without access failed\n");
    }

    dsi2 = &Conn2->dsi;
    vol2  = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto fin;
    }

    if (!(dir = FPCreateDir(Conn2, vol2, DIRDID_ROOT, name3))) {
        test_nottested();
        goto fin;
    }

    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1))
    dt = FPOpenDT(Conn, vol);
    cmt = "essai";
    ret = FPAddComment(Conn, vol, DIRDID_ROOT, name, cmt);

    if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
        test_failed();
    }

    ret = FPAddComment(Conn, vol, DIRDID_ROOT, name2, cmt);

    if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
        test_failed();
    }

    if (!ret) {
        ret = FPGetComment(Conn, vol, DIRDID_ROOT, name2);

        if (ret || memcmp(cmt, dsi->commands + 1, strnlen(cmt, 199))) {
            test_failed();
        }
    }

    FAIL(FPAddComment(Conn, vol, DIRDID_ROOT, name1, "Comment for toto.txt"))

    if (FPAddComment(Conn, vol, DIRDID_ROOT, name3, "Comment for test folder")) {
        test_failed();
    }

    if (FPRemoveComment(Conn, vol, DIRDID_ROOT, name3)) {
        test_failed();
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, DIRDID_ROOT, name1, OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    FAIL(FPAddComment(Conn, vol, DIRDID_ROOT, name3, "Comment"))
    FAIL(FPGetComment(Conn, vol, DIRDID_ROOT, name3))
    FAIL(FPRemoveComment(Conn, vol, DIRDID_ROOT, name3))
    FAIL(FPCloseFork(Conn, fork))
#if 0

    if (ntohl(AFPERR_ACCESS) != FPAddComment(Conn, vol, DIRDID_ROOT,
            "bogus folder", "essai")) {
        fprintf(stdout, "\tFAILED\n");
        return;
    }

#endif
fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name3))

    if (pdir) {
        delete_folder(vol, DIRDID_ROOT, name);
    }

    if (rdir) {
        delete_folder(vol, DIRDID_ROOT, name2);
    }

    FAIL(dt && FPCloseDT(Conn, dt))
test_exit:
    exit_test("FPAddComment:test55: add comment");
}

/* test537: Multi-client comment cache coherency
 *
 * This test validates that comments set by one client are visible
 * to a second client, and that comment updates and removals are
 * also properly reflected across clients.
 *
 * Scenario:
 * 1. Client 1 creates a file, opens desktop DB, sets a comment
 * 2. Client 1 reads comment back to verify
 * 3. Client 2 opens volume + desktop DB, reads comment - verify match
 * 4. Client 1 updates comment with a new value
 * 5. Client 2 reads updated comment - verify new value
 * 6. Client 1 removes comment
 * 7. Client 2 tries to read removed comment - should get error
 * 8. Clean up
 */
STATIC void test537()
{
    char *file = "t537 comment cache";
    char *cmt1 = "Initial comment from Client 1";
    char *cmt2 = "Updated comment from Client 1";
    uint16_t vol = VolID;
    uint16_t vol2 = 0;
    uint16_t dt = 0;
    uint16_t dt2 = 0;
    int ret;
    const DSI *dsi2;
    ENTER_TEST

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    /************************************
     * Step 1: Client 1 creates file and sets comment
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 1: Client 1 creates file and sets comment\n");
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file)) {
        test_nottested();
        goto test_exit;
    }

    dt = FPOpenDT(Conn, vol);

    if (!dt) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPOpenDT for Client 1\n");
        }

        test_nottested();
        goto fin;
    }

    FAIL(FPAddComment(Conn, vol, DIRDID_ROOT, file, cmt1))

    /************************************
     * Step 2: Client 1 reads comment back
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 2: Client 1 reads comment back\n");
    }

    if (FPGetComment(Conn, vol, DIRDID_ROOT, file)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 1 could not read its own comment\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 3: Client 2 reads the comment
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 3: Client 2 reads comment (cross-client)\n");
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Client 2 could not open volume\n");
        }

        test_nottested();
        goto fin;
    }

    dt2 = FPOpenDT(Conn2, vol2);

    if (!dt2) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPOpenDT for Client 2\n");
        }

        test_nottested();
        goto fin;
    }

    dsi2 = &Conn2->dsi;

    if (FPGetComment(Conn2, vol2, DIRDID_ROOT, file)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 could not read comment set by Client 1\n");
        }

        test_failed();
        goto fin;
    }

    /* Verify comment content matches - AFP max comment length = 199 */
    size_t cmt1_len = strnlen(cmt1, 199);

    if (dsi2->cmdlen < 1 + cmt1_len ||
            memcmp(cmt1, dsi2->commands + 1, cmt1_len)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 got different comment content\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Client 2 read matching comment\n");
    }

    /************************************
     * Step 4: Client 1 updates the comment
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 4: Client 1 updates comment\n");
    }

    FAIL(FPAddComment(Conn, vol, DIRDID_ROOT, file, cmt2))

    /************************************
     * Step 5: Client 2 reads updated comment
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Client 2 reads updated comment\n");
    }

    if (FPGetComment(Conn2, vol2, DIRDID_ROOT, file)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 could not read updated comment\n");
        }

        test_failed();
        goto fin;
    }

    /* AFP max comment length = 199 (see desktop.c) */
    size_t cmt2_len = strnlen(cmt2, 199);

    if (dsi2->cmdlen < 1 + cmt2_len ||
            memcmp(cmt2, dsi2->commands + 1, cmt2_len)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 got stale comment content\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout,
                "\t    Client 2 read updated comment correctly\n");
    }

    /************************************
     * Step 6: Client 1 removes the comment
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 6: Client 1 removes comment\n");
    }

    FAIL(FPRemoveComment(Conn, vol, DIRDID_ROOT, file))

    /************************************
     * Step 7: Client 2 tries to read removed comment
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 7: Client 2 reads removed comment (should fail)\n");
    }

    ret = FPGetComment(Conn2, vol2, DIRDID_ROOT, file);

    if (ret == 0) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 should get error reading removed comment\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ Multi-client comment cache coherency verified (add/read/update/remove)\n");
    }

fin:

    if (dt2) {
        FPCloseDT(Conn2, dt2);
    }

    if (vol2) {
        FPCloseVol(Conn2, vol2);
    }

    if (dt) {
        FPCloseDT(Conn, dt);
    }

    FPDelete(Conn, vol, DIRDID_ROOT, file);
test_exit:
    exit_test("FPAddComment:test537: Multi-client comment cache coherency");
}

/* ----------- */
void FPAddComment_test()
{
    ENTER_TESTSET
    test55();
    test537();
}
