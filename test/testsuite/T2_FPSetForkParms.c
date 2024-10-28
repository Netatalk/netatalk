#include "specs.h"
#include "ea.h"

/* -------------------------- */
STATIC void test9()
{
    int fork;
    int fork1 = 0;
    uint16_t bitmap = 0;
    char *name = "ta\314\210st9";
    int ret;
    uint16_t vol = VolID;
    DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto test_exit;
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		test_failed();
		goto fin;
	}
	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_RFLEN), 10)) {
		test_failed();
		goto fin;
	}

	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_RFLEN), 0)) {
		test_failed();
		goto fin;
	}

fin:
	FAIL (fork && FPCloseFork(Conn,fork))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPSetForkParms:test9: name encoding");
}

/* ----------- */
void T2FPSetForkParms_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
	test9();
}
