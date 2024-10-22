/* ----------------------------------------------
*/
#include "specs.h"

/* ----------------------- */
STATIC void test209(void)
{
int ret;
	ENTER_TEST

	ret = FPGetSrvrParms(Conn);
	if (ret) {
		failed();
	}
	exit_test("FPGetSrvrParms:test209: GetSrvrParms call");

}

/* ----------------------- */
STATIC void test316(void)
{
int ret;
DSI *dsi = &Conn->dsi;
int nbe,i;
int found = 0;
unsigned char len;
unsigned char *b;

	ENTER_TEST

	FPCloseVol(Conn,VolID);

	ret = FPGetSrvrParms(Conn);
	if (ret) {
		failed();
	}
	nbe = *(dsi->data +4);
	b = dsi->data+5;
	for (i = 0; i < nbe; i++) {
	    b++; /* flags */
	    len = *b;
	    b++;
	    if (!memcmp(b, Vol, len))
	        found = 1;
	    b += len;
	}
	if (!found) {
		failed();
	}

	VolID = FPOpenVol(Conn, Vol);
	if (VolID == 0xffff) {
		nottested();
	}
	exit_test("FPGetSrvrParms:test316: GetSrvrParms for a volume with option nostat set");
}

/* ----------- */
void FPGetSrvrParms_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPGetSrvrParms page 203\n");
    fprintf(stdout,"-------------------\n");
	test209();
	test316();
}
