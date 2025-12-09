/* ----------------------------------------------
*/
#include "adoublehelper.h"
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

/* ------------------------- */
STATIC void test121()
{
    int  dir;
    char *name = "t121 test dir setdirparam";
    int  ofs =  3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1 << DIRPBIT_FINFO) | (1 << DIRPBIT_CDATE) |
                      (1 << DIRPBIT_BDATE) | (1 << DIRPBIT_MDATE);
    uint16_t vol = VolID;
    DSI *dsi;
    ENTER_TEST

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    dsi = &Conn->dsi;

    if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT, name))) {
        test_nottested();
        goto test_exit;
    }

    memset(&filedir, 0, sizeof(filedir));

    if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
        test_failed();
    } else {
        filedir.isdir = 1;
        afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

        if (delete_unix_adouble(Path, name)) {
            goto fin;
        }

        FAIL(FPSetDirParms(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
        FAIL(htonl(AFPERR_BITMAP) != FPSetDirParms(Conn, vol, DIRDID_ROOT, name, 0xffff,
                &filedir))
    }

fin:
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPSetDirParms:test121: test set dir setfilparam (create .AppleDouble)");
}

/* ------------------------- */
/* test528: External permission change detection
 *
 * This test validates that Netatalk properly detects permission changes made
 * externally (outside of AFP, e.g., via shell, SMB, NFS) and enforces them
 * without stale permission caching.
 *
 * Scenario:
 * 1. Client creates parent dir, child dir, and file
 * 2. Verifies file can be written to
 * 3. External process removes EXECUTE permission from parent (chmod 0444)
 * 4. Client attempts to access file (should fail - can't traverse parent)
 * 5. External process restores EXECUTE permission (chmod 0777)
 * 6. Client accesses file again (should succeed - can traverse parent)
 *
 * Why execute permission: Without execute on a directory, you cannot traverse
 * into it or access files in subdirectories, even if those files are writable.
 *
 * Note: This test requires localhost connection since it directly accesses
 * the filesystem to simulate external permission changes.
 */
STATIC void test528()
{
    char *parent_name = "t528_parent";
    char *child_name = "t528_child";
    char *file_name = "t528_file.txt";
    char parent_path[MAXPATHLEN];
    char child_path[MAXPATHLEN];
    char file_path[MAXPATHLEN];
    int parent_dir = 0;
    int child_dir = 0;
    uint16_t fork = 0;
    uint16_t vol = VolID;
    int  ofs = 3 * sizeof(uint16_t);
    uint16_t bitmap = (1 << DIRPBIT_ACCESS) | (1 << DIRPBIT_UNIXPR);
    struct afp_filedir_parms filedir;
    DSI *dsi = &Conn->dsi;
    ENTER_TEST

    if (!Quiet) {
        fprintf(stdout,
                "FPSetDirParms:test528: Permission cache invalidation on external changes\n");
    }

    if (Path[0] == '\0') {
        test_skipped(T_PATH);
        goto test_exit;
    }

    /* This test requires localhost connection for direct filesystem access.
     * It uses direct chmod() to simulate external permission changes from
     * other protocols (SMB, NFS) or shell commands. */
    if (strcmp(Server, "localhost") != 0 && strcmp(Server, "127.0.0.1") != 0) {
        test_skipped(T_LOCALHOST);
        goto test_exit;
    }

    /************************************
     * Step 1: Client 1 creates parent with rwxrwxrwx
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 1: Client 1 creates parent dir (rwxrwxrwx)\n");
    }

    parent_dir = FPCreateDir(Conn, vol, DIRDID_ROOT, parent_name);

    if (!parent_dir) {
        test_nottested();
        goto test_exit;
    }

    // Set parent permissions to rwxrwxrwx (0777)
    snprintf(parent_path, sizeof(parent_path), "%s/%s", Path, parent_name);

    if (chmod(parent_path, 0777) < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to chmod parent: %s\n", strerror(errno));
        }

        test_failed();
        goto fin;
    }

    /************************************
     * Step 2: Client 1 creates child dir (inherits)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 2: Client 1 creates child dir (inherits permissions)\n");
    }

    child_dir = FPCreateDir(Conn, vol, parent_dir, child_name);

    if (!child_dir) {
        test_failed();
        goto fin;
    }

    snprintf(child_path, sizeof(child_path), "%s/%s/%s", Path, parent_name,
             child_name);

    /************************************
     * Step 3: Client 1 creates file in child
     ************************************/
    if (!Quiet) {
        fprintf(stdout, "\t  Step 3: Client 1 creates file in child dir\n");
    }

    FAIL(FPCreateFile(Conn, vol, 0, child_dir, file_name))
    snprintf(file_path, sizeof(file_path), "%s/%s/%s/%s", Path, parent_name,
             child_name, file_name);
    // Verify can write to file
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, child_dir, file_name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    FAIL(FPWrite(Conn, fork, 0, 10, "test data", 0))
    FAIL(FPCloseFork(Conn, fork))
    fork = 0;

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    File created and writable\n");
    }

    /************************************
     * Step 4: External process removes execute permission
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 4: External chmod removes execute permission (r--r--r--)\n");
    }

    /* Remove execute permission - this prevents traversing into the directory,
     * which means accessing files in subdirectories will fail */
    if (chmod(parent_path, 0444) < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to chmod parent: %s\n", strerror(errno));
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout,
                "\t    Parent execute permission removed - directory not traversable\n");
    }

    /************************************
     * Step 5: Client 1 stats parent (trigger ctime check)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 5: Client 1 stats parent (should detect ctime change)\n");
    }

    /* Stat the parent directory to trigger dircache ctime validation.
     * This should detect the ctime change from external chmod and invalidate
     * any cached permission information. */
    FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, parent_name, 0, bitmap))
    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

    if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Parent stat completed, cache should be invalidated\n");
    }

    /************************************
     * Step 6: Client 1 attempts to write to file (should fail)
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 6: Client 1 attempts to write (should fail - parent not traversable)\n");
    }

    /* Try to open for write - should fail because parent lacks execute permission.
     * Unix chdir() will fail when attempting to traverse into child directory,
     * as parent is not traversable (mode 0444 = no execute bit).
     * Ctime-based cache ensures permission metadata stays synchronized. */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, child_dir, file_name,
                      OPENACC_WR);

    if (fork) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED should not be able to open for write after external chmod\n");
        }

        test_failed();
        FPCloseFork(Conn, fork);
        fork = 0;
    } else if (!Quiet && Verbose) {
        fprintf(stdout, "\t    Write access correctly denied\n");
    }

    /************************************
     * Step 7: External process restores execute permission
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 7: External chmod restores execute permission (rwxrwxrwx)\n");
    }

    /* Restore execute permission - this allows traversing the directory again */
    if (chmod(parent_path, 0777) < 0) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED unable to chmod parent back: %s\n", strerror(errno));
        }

        test_failed();
        goto fin;
    }

    if (!Quiet && Verbose) {
        fprintf(stdout,
                "\t    Parent execute permission restored - directory traversable again\n");
    }

    /************************************
     * Step 8: Client 1 stats parent again and then writes
     ************************************/
    if (!Quiet) {
        fprintf(stdout,
                "\t  Step 8: Client 1 stats parent and writes to file (should succeed)\n");
    }

    /* Stat parent again to trigger ctime check and cache refresh */
    FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, parent_name, 0, bitmap))
    /* Now should be able to write after cache refresh */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0, child_dir, file_name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED should be able to open for write after external chmod restore\n");
        }

        test_failed();
        goto fin;
    }

    FAIL(FPWrite(Conn, fork, 0, 15, "more test data", 0))
    FAIL(FPCloseFork(Conn, fork))
    fork = 0;

    if (!Quiet) {
        fprintf(stdout,
                "\t  ✓ Permission cache properly invalidated on external ctime changes\n");
    }

fin:

    // Cleanup
    if (fork) {
        FPCloseFork(Conn, fork);
    }

    // Restore permissions for cleanup
    chmod(parent_path, 0777);

    if (child_dir) {
        FPDelete(Conn, vol, child_dir, file_name);
        FPDelete(Conn, vol, parent_dir, child_name);
    }

    if (parent_dir) {
        FPDelete(Conn, vol, DIRDID_ROOT, parent_name);
    }

test_exit:
    exit_test("FPSetDirParms:test528: Permission cache invalidation on external changes");
}

/* ----------- */
void T2FPSetDirParms_test()
{
    ENTER_TESTSET
    test121();
    test528();
}
