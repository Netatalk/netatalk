/*
 * FPSpotlightRPC — exercises the paginated cnid_find() path through
 * FPSpotlightRPC. test volume must be configured with `spotlight = yes`
 *
 * Determinism invariants:
 *
 *   1. cnid_find() does an UNSCOPED substring/prefix match across the
 *      entire CNID database. The Spotlight DSL filename predicate is
 *      filename-only, so the per-test sub-directory does NOT partition
 *      results by CNID. To avoid collisions with pre-existing files
 *      anywhere on the volume, both fixtures use deliberately long,
 *      test-unique prefixes (`paginate530-f-` / `paginate531-p-`).
 *
 *   2. The test corpus must NOT be mutated by another AFP client
 *      during the test. cnid_dbd pagination is stateless on the daemon
 *      side, so concurrent CNID ADDs/DELETEs between paginated batches
 *      can produce duplicates or misses — the deterministic raw-count
 *      assertion (`got == 50` / `got == 250`) relies on a static
 *      corpus.
 *
 *   3. T_AFP32 corresponds to `Conn->afp_version >= 5` (netatalk's
 *      internal index for AFP 3.2+). The Spotlight RPC subprotocol was
 *      introduced in AFP 3.2; lower versions return AFPERR_NOOP from
 *      FPSpotlightOpen, which the fixture treats as test_nottested().
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

#define PAG_CORPUS_SMALL    50
#define PAG_CORPUS_PAGED   250

/*
 * Long, test-unique filename prefixes. These are NOT directory names
 * (the DSL is filename-only); they are filename-prefix tokens chosen
 * to be unlikely to collide with anything on a typical AFP volume,
 * including AppleDouble shadows (`._foo`), Network Trash Folder
 * cruft, hidden config files, etc.
 */
#define PAG_PREFIX_530      "paginate530-f-"
#define PAG_PREFIX_531      "paginate531-p-"

#define PAG_DIR_SMALL  "pagtest_small"
#define PAG_DIR_PAGED  "pagtest_paged"

/*!
 * @brief Create a directory and return its DID
 *
 * Convention reminder: in the testsuite, FPCreate* / FPOpen* helpers
 * return the new DID (NETWORK byte order, treated as `int`) on
 * success and 0 on failure — they do NOT return AFP error codes.
 *
 * AFPCreateDir extracts the new DID from dsi->commands with a straight
 * memcpy (no byte swap). The returned value is therefore the
 * on-the-wire NETWORK-byte-order CNID stored as int. It must be
 * passed straight back to FPCreateFile()/FPOpenDir() without an
 * ntohl() — those helpers will memcpy it onto the next request's
 * wire as-is. Applying ntohl() here would silently corrupt the DID.
 *
 * @returns the network-byte-order new DID on success, 0 on error
 */
static int pag_create_dir_get_did(uint16_t vol, uint32_t parent_did,
                                  char *name)
{
    return FPCreateDir(Conn, vol, parent_did, name);
}

STATIC void test547()
{
    uint16_t vol = VolID;
    int got = 0;
    int dir_id;
    char fn[64];
    ENTER_TEST

    if (Conn->afp_version < 32) {
        test_skipped(T_AFP32);
        goto test_exit;
    }

    /* FPSpotlightOpen returns AFPERR_NOOP if `spotlight = no` in
     * afp.conf, or AFP_OK on success. We do not need the populated
     * vol_path, so pass NULL/0 to skip the optional path-extract. */
    if (FPSpotlightOpen(Conn, vol, NULL, 0) != AFP_OK) {
        test_nottested();
        goto test_exit;
    }

    /* Pre-test scrub: blow away any stale subdirectory from a prior run. */
    delete_directory_tree(Conn, vol, DIRDID_ROOT, PAG_DIR_SMALL);
    dir_id = pag_create_dir_get_did(vol, DIRDID_ROOT, PAG_DIR_SMALL);

    /* dir_id is a network-byte-order CNID stored as int; on x86/arm64
     * the high byte may sign-bit-set. Check for the failure sentinel
     * exactly, NOT `<= 0`. */
    if (dir_id == 0) {
        test_nottested();
        goto cleanup;
    }

    for (int i = 0; i < PAG_CORPUS_SMALL; i++) {
        snprintf(fn, sizeof(fn), PAG_PREFIX_530 "%05d.txt", i);

        if (FPCreateFile(Conn, vol, 0, dir_id, fn) != AFP_OK) {
            test_nottested();
            goto cleanup;
        }
    }

    /* Use the long, test-unique prefix so the unscoped substring search
     * across the volume cannot pick up unrelated files. */
    if (FPSpotlightOpenQuery(Conn, vol,
                             "kMDItemFSName==\"" PAG_PREFIX_530 "*\"cd",
                             0x1234) != AFP_OK) {
        test_failed();
        goto cleanup;
    }

    if (FPSpotlightDrainResults(Conn, vol, 0x1234, &got) != AFP_OK) {
        test_failed();
        goto close_query;
    }

    if (got != PAG_CORPUS_SMALL) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED: test547 expected %d, got %d "
                    "(possible volume contamination — check for "
                    "stray '" PAG_PREFIX_530 "*' filenames)\n",
                    PAG_CORPUS_SMALL, got);
        }

        test_failed();
    }

close_query:
    FPSpotlightCloseQuery(Conn, vol, 0x1234);
cleanup:
    delete_directory_tree(Conn, vol, DIRDID_ROOT, PAG_DIR_SMALL);
test_exit:
    exit_test("FPSpotlightRPC:test547: small-corpus single-batch search");
}

STATIC void test548()
{
    uint16_t vol = VolID;
    int got = 0;
    int dir_id;
    char fn[64];
    ENTER_TEST

    if (Conn->afp_version < 32) {
        test_skipped(T_AFP32);
        goto test_exit;
    }

    if (FPSpotlightOpen(Conn, vol, NULL, 0) != AFP_OK) {
        test_nottested();
        goto test_exit;
    }

    delete_directory_tree(Conn, vol, DIRDID_ROOT, PAG_DIR_PAGED);
    dir_id = pag_create_dir_get_did(vol, DIRDID_ROOT, PAG_DIR_PAGED);

    /* See test547: dir_id is a network-byte-order CNID stored as int. */
    if (dir_id == 0) {
        test_nottested();
        goto cleanup;
    }

    for (int i = 0; i < PAG_CORPUS_PAGED; i++) {
        snprintf(fn, sizeof(fn), PAG_PREFIX_531 "%05d.txt", i);

        if (FPCreateFile(Conn, vol, 0, dir_id, fn) != AFP_OK) {
            test_nottested();
            goto cleanup;
        }
    }

    if (FPSpotlightOpenQuery(Conn, vol,
                             "kMDItemFSName==\"" PAG_PREFIX_531 "*\"cd",
                             0x1235) != AFP_OK) {
        test_failed();
        goto cleanup;
    }

    if (FPSpotlightDrainResults(Conn, vol, 0x1235, &got) != AFP_OK) {
        test_failed();
        goto close_query;
    }

    if (got != PAG_CORPUS_PAGED) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED: test548 expected %d, got %d "
                    "(possible volume contamination — check for "
                    "stray '" PAG_PREFIX_531 "*' filenames)\n",
                    PAG_CORPUS_PAGED, got);
        }

        test_failed();
    }

close_query:
    FPSpotlightCloseQuery(Conn, vol, 0x1235);
cleanup:
    delete_directory_tree(Conn, vol, DIRDID_ROOT, PAG_DIR_PAGED);
test_exit:
    exit_test("FPSpotlightRPC:test548: paged search (>=3 SEARCH batches)");
}

void FPSpotlightRPC_test()
{
    ENTER_TESTSET
    test547();
    test548();
}
