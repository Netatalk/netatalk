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
		test_failed();
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
		test_failed();
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
		test_failed();
	}

	VolID = FPOpenVol(Conn, Vol);
	if (VolID == 0xffff) {
		test_nottested();
	}
	exit_test("FPGetSrvrParms:test316: GetSrvrParms for a volume with option nostat set");
}

/* ----------- */
void FPGetSrvrParms_test()
{
    ENTER_TESTSET
	test209();
	test316();
}
