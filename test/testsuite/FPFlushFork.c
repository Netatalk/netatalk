/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test203() {
    uint16_t vol = VolID;
    uint16_t bitmap = (1 << FILPBIT_MDATE);
    struct afp_filedir_parms filedir;
    int fork = 0;
    char *name = "t203 file";
    uint32_t mdate;
    DSI *dsi = &Conn->dsi;
    ENTER_TEST

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_RD | OPENACC_WR);

    if (!fork) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->commands + 2 * sizeof(uint16_t), bitmap, 0);
    mdate = filedir.mdate;
    sleep(2);
    FAIL(FPFlushFork(Conn, fork))

    if (FPGetForkParam(Conn, fork, bitmap)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + sizeof(uint16_t), bitmap, 0);

    /* is that always true? ie over nfs */
    if (mdate != filedir.mdate) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED dates differ\n");
        }

        test_failed();
    }

    mdate = filedir.mdate;
    FAIL(FPWrite(Conn, fork, 0, 10, Data, 0))
    sleep(2);
    FAIL(FPFlushFork(Conn, fork))

    if (FPGetForkParam(Conn, fork, bitmap)) {
        test_failed();
        goto fin;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + sizeof(uint16_t), bitmap, 0);

    /* is that always true? ie over nfs */
    if (mdate == filedir.mdate) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED dates equal\n");
        }

        test_failed();
    }

    FAIL(htonl(AFPERR_PARAM) != FPFlushFork(Conn, fork + 1))
fin:
    FAIL(fork && FPCloseFork(Conn, fork))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
test_exit:
    exit_test("FPFlushFork:test203: flush fork call");
}

/* ----------- */
void FPFlushFork_test() {
    ENTER_TESTSET
    test203();
}
