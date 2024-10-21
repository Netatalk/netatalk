/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test199()
{
int dir;
char *name = "t199 dir";
uint16_t vol = VolID;
int ret;
DSI *dsi;


	enter_test();
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	dsi = &Conn->dsi;
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}

	dir = FPOpenDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto fin;
	}
	FAIL (FPCloseDir(Conn, vol, DIRDID_ROOT_PARENT))
	FAIL (FPCloseDir(Conn, vol, DIRDID_ROOT))
	ret = FPCloseDir(Conn, vol +1, dir);
	if (not_valid(ret, /* MAC */AFPERR_PARAM, 0)) {
		failed();
	}

	FAIL (FPCloseDir(Conn, vol, dir))

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPCloseDir:test199: FPCloseDir call");
}

/* ----------- */
void FPCloseDir_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPCloseDir page 127\n");
    fprintf(stdout,"-------------------\n");
	test199();
}
