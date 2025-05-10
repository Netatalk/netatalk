/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test206()
{
    uint16_t bitmap;
    uint16_t vol = VolID;
    struct afp_volume_parms parms;
    DSI *dsi = &Conn->dsi;
    ENTER_TEST
    bitmap = (1 << VOLPBIT_ATTR)
             | (1 << VOLPBIT_SIG)
             | (1 << VOLPBIT_CDATE)
             | (1 << VOLPBIT_MDATE)
             | (1 << VOLPBIT_BDATE)
             | (1 << VOLPBIT_VID)
             | (1 << VOLPBIT_BFREE)
             | (1 << VOLPBIT_BTOTAL)
             | (1 << VOLPBIT_NAME);
    FAIL(FPGetVolParam(Conn, vol, bitmap))
    /* FIXME: I don't understand why this is checking mdate before
       carrying out any modifications? Seems like a spurious test.
       Anyhow, disabling because it's causing a false positive
       when running the entire spectest suite on Debian Testing.
    */
#if 0

    if (parms.bdate == parms.mdate) {
        if (!Quiet) {
            fprintf(stdout, "Backup and modification date are the same!\n");
        }

        test_nottested();
        goto test_exit;
    }

#endif
    afp_volume_unpack(&parms, dsi->commands + sizeof(uint16_t), bitmap);
    bitmap = (1 << VOLPBIT_BDATE);
    FAIL(htonl(AFPERR_PARAM) != FPSetVolParam(Conn, vol + 1, bitmap, &parms))
    FAIL(htonl(AFPERR_BITMAP) != FPSetVolParam(Conn, vol, 0xffff, &parms))
    parms.bdate = parms.mdate;
    FAIL(FPSetVolParam(Conn, vol, bitmap, &parms))
    parms.bdate = 0;
    bitmap = (1 << VOLPBIT_BDATE) | (1 << VOLPBIT_MDATE);
    FAIL(FPGetVolParam(Conn, vol, bitmap))
    afp_volume_unpack(&parms, dsi->commands + sizeof(uint16_t), bitmap);

    if (parms.bdate != parms.mdate) {
        if (!Quiet) {
            fprintf(stdout,
                    "\tFAILED Backup %x and modification %x date are not the same!\n", parms.bdate,
                    parms.mdate);
        }

        test_failed();
    }

test_exit:
    exit_test("FPSetVolParms:test206: Set Volume parameters");
}

/* ----------- */
void FPSetVolParms_test()
{
    ENTER_TESTSET
    test206();
}
