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

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPOpenVol:t205: Open Volume call\n");

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
		fprintf(stdout,"\tFAILED double open != volume id\n");
		failed_nomsg();
	}
    FAIL (FPCloseVol(Conn, ret));
    FAIL (!FPCloseVol(Conn, ret));
fin:
	ret = VolID = FPOpenVol(Conn, Vol);
	if (ret == 0xffff) {
		failed();
	}
	exit_test("test205");
}

/* -------------------------------- */
STATIC void test404()
{
uint16_t vol = VolID;
DSI *dsi = &Conn->dsi;
uint16_t ret;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPOpenVol:t404: lazy init of dbd cnid\n");

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
	exit_test("test404");
}

/* ----------- */
void FPOpenVol_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPOpenVol page 235\n");

    test205();
}
