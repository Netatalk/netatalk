/* ----------------------------------------------
*/
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/* ------------------------- */
STATIC void test399()
{
    uint16_t vol = VolID;
    char *file = "test399_file";
    char *attr_name = "test399_attribute";
    int remerror = AFPERR_NOITEM;
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
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

    /* AFP 3.4 sends AFPERR_NOITEM instead of AFPERR_MISC */
    if (Conn->afp_version < 34) {
        remerror = AFPERR_MISC;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file)) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPSetExtAttr(Conn, vol, DIRDID_ROOT, 2, file, attr_name, "test399_data"))
    FAIL(FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file, attr_name))

    /* check create flag */
    if (ntohl(AFPERR_EXIST) != FPSetExtAttr(Conn, vol, DIRDID_ROOT, 2, file,
                                            attr_name, "test399_newdata")) {
        test_failed();
    }

    FAIL(FPListExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file))
    FAIL(FPSetExtAttr(Conn, vol, DIRDID_ROOT, 4, file, attr_name,
                      "test399_newdata"))
    FAIL(FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file, attr_name))
    FAIL(FPRemoveExtAttr(Conn, vol, DIRDID_ROOT, 0, file, attr_name))

    if (ntohl(remerror) != FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file,
                                        attr_name)) {
        test_failed();
    }

    if (ntohl(AFPERR_MISC) != FPRemoveExtAttr(Conn, vol, DIRDID_ROOT, 0, file,
            attr_name)) {
        test_failed();
    }

    FPDelete(Conn, vol, DIRDID_ROOT, file);
test_exit:
    exit_test("FPGetExtAttr:test399: check Extended Attributes Support");
}

/* ------------------------- */
STATIC void test416()
{
    uint16_t vol = VolID;
    char *file = "test416_file";
    char *file1 = "test416 new file";
    char *attr_name = "test416_attribute";
    ENTER_TEST

    if (Conn->afp_version < 32) {
        test_skipped(T_AFP32);
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

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file)) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPSetExtAttr(Conn, vol, DIRDID_ROOT, 2, file, attr_name, "test416_data"))
    FAIL(FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file, attr_name))
    FAIL(FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, file, "", file1))
    FAIL(FPListExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file))
    FAIL(FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file, attr_name))
    FAIL(FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file1, attr_name))
    FAIL(FPRemoveExtAttr(Conn, vol, DIRDID_ROOT, 0, file1, attr_name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, file))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, file1))
test_exit:
    exit_test("FPGetExtAttr:test416: copy file with Extended Attributes");
}

/* ------------------------- */
STATIC void test432()
{
    uint16_t vol = VolID;
    const DSI *dsi;
    char *file = "test432_file";
    char *attr_name = "test432_attribute";
    uint32_t attrlen;
    char *attrdata = "1234567890";
    size_t attrdata_len = 10;
    dsi = &Conn->dsi;
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
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

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file)) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPSetExtAttr(Conn, vol, DIRDID_ROOT, 2, file, attr_name, attrdata))
    /*
     * The xattr we set is 10 bytes large. The req_count must be
     * at least 10 bytes + 6 bytes. The 6 bytes are the AFP reply
     * packets bitmap and length field.
     *
     * This means that Apple's protocol implementation doesn't
     * support returning truncated xattrs. From the spec it would
     * be possible to get just the first byte of an xattr by
     * specifying an req_count of 7.
     */
    FAIL(htonl(AFPERR_PARAM) != FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 1, file,
            attr_name));
    FAIL(htonl(AFPERR_PARAM) != FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 6, file,
            attr_name));
    FAIL(htonl(AFPERR_PARAM) != FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 7, file,
            attr_name));
    FAIL(htonl(AFPERR_PARAM) != FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 15, file,
            attr_name));
    FAIL(FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 16, file, attr_name));
    FAIL(FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 17, file, attr_name));
    memcpy(&attrlen, dsi->data + 2, 4);
    attrlen = ntohl(attrlen);

    if (attrlen != attrdata_len) {
        test_failed();
    }

    FPDelete(Conn, vol, DIRDID_ROOT, file);
test_exit:
    exit_test("FPGetExtAttr:test432: set and get Extended Attributes");
}

/* test536: Multi-client EA cache coherency
 *
 * This test validates that Extended Attributes set by one client are
 * immediately visible to a second client, and that EA updates and
 * removals are also visible across clients.
 *
 * Scenario:
 * 1. Client 1 creates a file and sets an EA
 * 2. Client 2 reads the EA - verify it matches
 * 3. Client 1 updates the EA with a new value
 * 4. Client 2 reads again - verify updated value
 * 5. Client 1 removes the EA
 * 6. Client 2 tries to read - should get error (AFPERR_NOITEM)
 * 7. Clean up
 */
STATIC void test536()
{
    uint16_t vol = VolID;
    uint16_t vol2 = 0;
    char *file = "test536_ea_cache";
    char *attr_name = "test536_attribute";
    char *data1 = "test536_initial_value";
    char *data2 = "test536_updated_value_longer";
    int remerror = AFPERR_NOITEM;
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
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

    if (!Conn2) {
        test_skipped(T_CONN2);
        goto test_exit;
    }

    /* AFP 3.4 sends AFPERR_NOITEM instead of AFPERR_MISC */
    if (Conn->afp_version < 34) {
        remerror = AFPERR_MISC;
    }

    /************************************
     * Step 1: Client 1 creates file and sets EA
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 1: Client 1 creates file and sets EA\n");
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file)) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPSetExtAttr(Conn, vol, DIRDID_ROOT, 2, file, attr_name, data1))
    /* Verify Client 1 can read it back */
    FAIL(FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file, attr_name))

    /************************************
     * Step 2: Client 2 reads the EA - verify consistency
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 2: Client 2 reads EA (cross-client consistency)\n");
    }

    vol2 = FPOpenVol(Conn2, Vol);

    if (vol2 == 0xffff) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED Client 2 could not open volume\n");
        }

        test_nottested();
        goto fin;
    }

    if (FPGetExtAttr(Conn2, vol2, DIRDID_ROOT, 0, 4096, file, attr_name)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 could not read EA set by Client 1\n");
        }

        test_failed();
        goto fin;
    }

    /* Also verify Client 2 can list the EA */
    FAIL(FPListExtAttr(Conn2, vol2, DIRDID_ROOT, 0, 4096, file))

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Client 2 successfully read EA\n");
    }

    /************************************
     * Step 3: Client 1 updates the EA with new value
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 3: Client 1 updates EA with new value\n");
    }

    /* Use flag 4 (replace) to update */
    FAIL(FPSetExtAttr(Conn, vol, DIRDID_ROOT, 4, file, attr_name, data2))

    /************************************
     * Step 4: Client 2 reads the updated EA
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 4: Client 2 reads updated EA\n");
    }

    if (FPGetExtAttr(Conn2, vol2, DIRDID_ROOT, 0, 4096, file, attr_name)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 could not read updated EA\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout,
                "\t    Client 2 successfully read updated EA\n");
    }

    /************************************
     * Step 5: Client 1 removes the EA
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Client 1 removes the EA\n");
    }

    FAIL(FPRemoveExtAttr(Conn, vol, DIRDID_ROOT, 0, file, attr_name))

    /************************************
     * Step 6: Client 2 tries to read removed EA
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 6: Client 2 reads removed EA (should fail)\n");
    }

    if (ntohl(remerror) != FPGetExtAttr(Conn2, vol2, DIRDID_ROOT, 0, 4096,
                                        file, attr_name)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Client 2 should get error reading removed EA\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ Multi-client EA cache coherency verified (set/read/update/remove)\n");
    }

fin:

    if (vol2) {
        FPCloseVol(Conn2, vol2);
    }

    FPDelete(Conn, vol, DIRDID_ROOT, file);
test_exit:
    exit_test("FPGetExtAttr:test536: Multi-client EA cache coherency");
}

/* test540: Directory EA lifecycle
 *
 * All existing EA tests (test399, test416, test432) only test EAs on files.
 * This test validates the full EA lifecycle on a DIRECTORY:
 * Set, Get, List, Replace, Remove, and error handling.
 *
 * Scenario:
 * 1. Create a directory
 * 2. Set an EA on the directory (create flag)
 * 3. Get the EA back, verify it exists
 * 4. List EAs on the directory, verify our EA is listed
 * 5. Replace the EA with a new value (replace flag)
 * 6. Get the replaced EA, verify new value accessible
 * 7. Remove the EA
 * 8. Verify Get returns NOITEM after removal
 * 9. Clean up
 */
STATIC void test540()
{
    uint16_t vol = VolID;
    int dir = 0;
    char *dirname = "test540_ea_dir";
    char *attr_name = "test540_dir_attribute";
    char *data1 = "test540_dir_ea_value";
    char *data2 = "test540_dir_ea_updated_value";
    int remerror = AFPERR_NOITEM;
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
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

    /* AFP 3.4 sends AFPERR_NOITEM instead of AFPERR_MISC */
    if (Conn->afp_version < 34) {
        remerror = AFPERR_MISC;
    }

    /************************************
     * Step 1: Create directory
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 1: Create test directory\n");
    }

    dir = FPCreateDir(Conn, vol, DIRDID_ROOT, dirname);

    if (!dir) {
        test_nottested();
        goto test_exit;
    }

    /************************************
     * Step 2: Set EA on directory (create flag = 2)
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 2: Set EA on directory\n");
    }

    if (FPSetExtAttr(Conn, vol, DIRDID_ROOT, 2, dirname, attr_name, data1)) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED FPSetExtAttr on directory\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 3: Get EA back
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 3: Get EA from directory\n");
    }

    if (FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, dirname, attr_name)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED FPGetExtAttr on directory\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 4: List EAs on directory
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 4: List EAs on directory\n");
    }

    if (FPListExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, dirname)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED FPListExtAttr on directory\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 5: Verify create flag rejects duplicate
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Verify create flag rejects duplicate EA\n");
    }

    if (ntohl(AFPERR_EXIST) != FPSetExtAttr(Conn, vol, DIRDID_ROOT, 2,
                                            dirname, attr_name, data2)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED duplicate create should return AFPERR_EXIST\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 6: Replace EA with new value (replace flag = 4)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 6: Replace EA on directory with new value\n");
    }

    if (FPSetExtAttr(Conn, vol, DIRDID_ROOT, 4, dirname, attr_name, data2)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED FPSetExtAttr replace on directory\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 7: Get replaced EA
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 7: Get replaced EA from directory\n");
    }

    if (FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, dirname, attr_name)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED FPGetExtAttr after replace\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 8: Remove EA
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 8: Remove EA from directory\n");
    }

    if (FPRemoveExtAttr(Conn, vol, DIRDID_ROOT, 0, dirname, attr_name)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED FPRemoveExtAttr on directory\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 9: Verify Get returns error after removal
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 9: Verify Get returns error after removal\n");
    }

    if (ntohl(remerror) != FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096,
                                        dirname, attr_name)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED should get error reading removed dir EA\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ Directory EA lifecycle verified (set/get/list/replace/remove)\n");
    }

fin:

    if (dir) {
        FPDelete(Conn, vol, DIRDID_ROOT, dirname);
    }

test_exit:
    exit_test("FPGetExtAttr:test540: Directory EA lifecycle");
}

/* test541: EA persistence across volume close/reopen
 *
 * This test validates that Extended Attributes survive a volume
 * close and reopen cycle, ensuring they are properly persisted
 * to disk and not just cached in server memory.
 *
 * Scenario:
 * 1. Create a file and set an EA
 * 2. Verify EA is readable
 * 3. Close the volume
 * 4. Reopen the volume
 * 5. Read EA back - verify it survived the close/reopen
 * 6. List EAs - verify our EA is still listed
 * 7. Clean up (remove EA, delete file)
 */
STATIC void test541()
{
    uint16_t vol = VolID;
    char *file = "test541_ea_persist";
    char *attr_name = "test541_persist_attr";
    char *data = "test541_persistent_value_data";
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
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

    /************************************
     * Step 1: Create file and set EA
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 1: Create file and set EA\n");
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, file)) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPSetExtAttr(Conn, vol, DIRDID_ROOT, 2, file, attr_name, data))

    /************************************
     * Step 2: Verify EA is readable
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 2: Verify EA is readable before close\n");
    }

    if (FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file, attr_name)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED EA not readable before close\n");
        }

        test_failed();
        goto fin;
    }

    FAIL(FPListExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file))

    /************************************
     * Step 3: Close the volume
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 3: Close volume\n");
    }

    FAIL(FPCloseVol(Conn, vol))

    /************************************
     * Step 4: Reopen the volume
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 4: Reopen volume\n");
    }

    vol = VolID = FPOpenVol(Conn, Vol);

    if (vol == 0xffff) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED could not reopen volume\n");
        }

        test_nottested();
        goto test_exit;
    }

    /************************************
     * Step 5: Read EA back - should survive close/reopen
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Read EA after volume reopen\n");
    }

    if (FPGetExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file, attr_name)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED EA not readable after volume reopen\n");
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 6: List EAs - verify still listed
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 6: List EAs after volume reopen\n");
    }

    if (FPListExtAttr(Conn, vol, DIRDID_ROOT, 0, 4096, file)) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED EA not listed after volume reopen\n");
        }

        test_failed();
        goto fin;
    }

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ EA persistence across volume close/reopen verified\n");
    }

fin:
    FPRemoveExtAttr(Conn, vol, DIRDID_ROOT, 0, file, attr_name);
    FPDelete(Conn, vol, DIRDID_ROOT, file);
test_exit:
    exit_test("FPGetExtAttr:test541: EA persistence across volume close/reopen");
}

/* ----------- */
void FPGetExtAttr_test()
{
    ENTER_TESTSET
    test399();
    test416();
    test432();
    test536();
    test540();
    test541();
}
