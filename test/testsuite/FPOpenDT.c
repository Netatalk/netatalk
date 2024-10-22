/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test200()
{
uint16_t  dir;
uint16_t vol = VolID;
DSI *dsi;

	ENTER_TEST

	dsi = &Conn->dsi;

	dir = FPOpenDT(Conn,vol);
	if (dir == 0xffff) {
		failed();
	}

	dir = FPOpenDT(Conn,vol +1);
    if (dir != 0xffff || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
		failed();
	}
	exit_test("FPOpenDT:test200: OpenDT call");
}

/* ----------- */
void FPOpenDT_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPOpenDT page 229\n");
    fprintf(stdout,"-------------------\n");
	test200();
}
