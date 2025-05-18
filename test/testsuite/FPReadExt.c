/* ----------------------------------------------
*/
#include "specs.h"



/* ------------------------- */
#define BUF_S 3000
static char w_buf[BUF_S];

STATIC void test22()
{
    int fork;
    int fork1 = 0;
    uint16_t bitmap = 0;
    char *name = "t22 file";
    uint16_t vol = VolID;
    char utf8name[20];
    int i;
    ENTER_TEST

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
        goto test_exit;
    }

    if (FPEnumerate_ext(Conn, vol, DIRDID_ROOT, "",
                        (1 << FILPBIT_PDINFO) | (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
                        | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN), 0)) {
        test_failed();
    }

#if 0
    FAIL(FPEnumerate_ext(Conn, vol, DIRDID_ROOT, "", 0, (1 << DIRPBIT_PDINFO)))
#endif
    /* > 2 Gb */
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin2g;
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name,
                       OPENACC_WR | OPENACC_RD);

    if (!fork1) {
        test_failed();
        goto fin2g;
    }

    if (Conn2) {
        FPGetSrvrMsg(Conn2, 0, 0);
    }

    memset(w_buf, 'b', BUF_S);
    FAIL(FPWrite_ext(Conn, fork, ((off_t)1 << 31) + 20, 2000, w_buf, 0))
    FAIL(FPWrite_ext(Conn, fork1, ((off_t)1 << 31) + 20, 1000, w_buf, 0))
    FAIL(FPWrite_ext(Conn, fork, ((off_t)1 << 31) + 1000, 3000, w_buf, 0))
    FAIL(FPWrite_ext(Conn, fork, 0, 200, w_buf, 0x80))
    FAIL(FPWrite_ext(Conn, fork1, 0, 200, w_buf, 0x80))

    if (Conn2) {
        FPGetSrvrMsg(Conn2, 0, 0);
    }

    if (FPRead_ext(Conn, fork, 10, 10000, Data)) {
        test_failed();
    } else for (i = 0; i < 10000; i++) {
            if (Data[i] != 0) {
                test_failed();
                break;
            }
        }

    if (FPRead_ext(Conn, fork, ((off_t)1 << 31) + 20, 3000, Data)) {
        test_failed();
    } else for (i = 0; i < 3000; i++) {
            if (Data[i] == 0) {
                test_failed();
                break;
            }
        }

    if (FPEnumerate_ext(Conn, vol, DIRDID_ROOT, "",
                        (1 << FILPBIT_PDINFO) | (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
                        | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN), 0)) {
        test_failed();
    }

fin2g:
    FAIL(FPCloseFork(Conn, fork))
    FAIL(FPCloseFork(Conn, fork1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    /* ==========> 4 Gb ============= */
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, "very big"))
    fork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, "very big",
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
    }

    fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, "very big",
                       OPENACC_WR | OPENACC_RD);

    if (!fork1) {
        test_failed();
    }

    FAIL(FPWrite_ext(Conn, fork, ((off_t)1 << 32) + 20, 2000, w_buf, 0))
    FAIL(FPWrite_ext(Conn, fork1, ((off_t)1 << 32) + 20, 1000, w_buf, 0))

    if (Conn2) {
        FPGetSrvrMsg(Conn2, 0, 0);
    }

    if (FPRead_ext(Conn, fork, 10, 10000, Data)) {
        test_failed();
    } else for (i = 0; i < 10000; i++) {
            if (Data[i] != 0) {
                if (!Quiet) {
                    fprintf(stdout, "\tFAILED Data != 0\n");
                }

                test_failed();
                break;
            }
        }

    if (FPRead_ext(Conn, fork, ((off_t)1 << 32) + 20, 1500, Data)) {
        test_failed();
    } else for (i = 0; i < 1500; i++) {
            if (Data[i] == 0) {
                test_failed();
                break;
            }
        }

    if (FPEnumerate_ext(Conn, vol, DIRDID_ROOT, "",
                        (1 << FILPBIT_PDINFO) | (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
                        | (1 << FILPBIT_DFLEN) | (1 << FILPBIT_RFLEN), 0)) {
        test_failed();
    }

    FAIL(FPCloseFork(Conn, fork))
    FAIL(FPCloseFork(Conn, fork1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, "very big"))
    /* ========= utf 8 name ======= */
    utf8name[0]  = 'a';
    utf8name[1]  = 0xcc;
    utf8name[2]  = 0x88;
    utf8name[3]  = 'o';
    utf8name[4]  = 0xcc;
    utf8name[5]  = 0x88;
    utf8name[6]  = 'u';
    utf8name[7]  = 0xcc;
    utf8name[8]  = 0x88;
    utf8name[9]  = '.';
    utf8name[10] = 't';
    utf8name[11] = 'x';
    utf8name[12] = 't';
    utf8name[13] = 0;
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, utf8name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, utf8name))
test_exit:
    exit_test("FPReadExt:test22: AFP 3.0 read/Write");
}

/* ----------- */
void FPReadExt_test()
{
    ENTER_TESTSET
    test22();
}
