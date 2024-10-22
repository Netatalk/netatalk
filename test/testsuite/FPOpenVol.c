/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test205()
{
uint16_t vol = VolID;
DSI *dsi = &Conn->dsi;
uint16_t ret;
char *tp;

	ENTER_TEST

    FAIL (FPCloseVol(Conn, vol));
	/* --------- */
    ret = FPOpenVolFull(Conn, Vol, 0);
    if (ret != 0xffff || dsi->header.dsi_code != htonl(AFPERR_BITMAP)) {
		failed();
    }

	if (ret != 0xffff) {
    	FAIL (FPCloseVol(Conn, ret));
	}
	/* --------- */
    ret = FPOpenVolFull(Conn, Vol, 0xffff);
    if (ret != 0xffff || dsi->header.dsi_code != htonl(AFPERR_BITMAP)) {
		failed();
    }
	if (ret != 0xffff) {
    	FAIL (FPCloseVol(Conn, ret));
	}
	/* --------- */
	tp = strdup(Vol);
	if (!tp) {
		goto fin;
	}
	*tp = *tp +1;
    ret = FPOpenVol(Conn, tp);
	free(tp);
    if (ret != 0xffff || dsi->header.dsi_code != htonl(AFPERR_NOOBJ)) {
		failed();
    }
	if (ret != 0xffff) {
    	FAIL (FPCloseVol(Conn, ret));
	}
	/* -------------- */
	ret = FPOpenVol(Conn, Vol);
	if (ret == 0xffff) {
		failed();
	}
	vol = FPOpenVol(Conn, Vol);
	if (vol != ret) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED double open != volume id\n");
		}
		failed_nomsg();
	}
    FAIL (FPCloseVol(Conn, ret));
    FAIL (!FPCloseVol(Conn, ret));
fin:
	ret = VolID = FPOpenVol(Conn, Vol);
	if (ret == 0xffff) {
		failed();
	}
	exit_test("FPOpenVol:test205: Open Volume call");
}

/* -------------------------------- */
STATIC void test404()
{
uint16_t vol = VolID;
DSI *dsi = &Conn->dsi;
uint16_t ret;

	ENTER_TEST

    FAIL (FPCloseVol(Conn, vol));

	ret = FPGetSrvrParms(Conn);
	if (ret) {
		failed();
		goto fin;
	}

	/* --------- */
    ret = FPOpenVolFull(Conn, Vol, 1<<VOLPBIT_VID);
	if (ret == 0xffff) {
		failed();
		goto fin;
	}
	FAIL(FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, 1 << DIRPBIT_ACCESS));

	/* --------- */
    FAIL (FPCloseVol(Conn, ret));
fin:
	ret = VolID = FPOpenVol(Conn, Vol);
	if (ret == 0xffff) {
		failed();
	}
	exit_test("FPOpenVol:test404: lazy init of dbd cnid");
}

/* ----------- */
void FPOpenVol_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPOpenVol page 235\n");
    fprintf(stdout,"-------------------\n");

    test205();
    test404();
}
