/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test53()
{
int fork;
char *name  = "t53 dir no access";
char *name1 = "t53 file.txt";
char *name3  = "t53 --rwx-- dir";
int pdir;
int dir;
int ret;
int dt;
uint16_t vol = VolID;
uint16_t vol2;
DSI *dsi2;

	enter_test();
    fprintf(stdout,"===================\n");
	fprintf(stdout, "FPGetComment:test53: get comment\n");
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}		

	if (!(pdir = no_access_folder(vol, DIRDID_ROOT, name))) {
		goto test_exit;
	}
	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		nottested();
		goto fin;
	}
	if (!(dir = FPCreateDir(Conn2,vol2, DIRDID_ROOT , name3))) {
		nottested();
		goto fin;
	}

	dt = FPOpenDT(Conn,vol);
	if (ntohl(AFPERR_NOITEM) != FPGetComment(Conn, vol,  DIRDID_ROOT , name)) {
		failed();
	}
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1))
	ret = FPGetComment(Conn, vol,  DIRDID_ROOT , name1);
	if (not_valid(ret, /* MAC */AFPERR_NOITEM, 0)) {
		failed();
	}

	ret = FPGetComment(Conn, vol,  DIRDID_ROOT , name3);
	if (not_valid(ret, /* MAC */AFPERR_NOITEM, 0)) {
		failed();
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA  , 0 ,DIRDID_ROOT, name1, OPENACC_RD );
	if (!fork) {
		failed();
		goto fin;
	}
	ret = FPGetComment(Conn, vol,  DIRDID_ROOT , name1);
	if (not_valid(ret, /* MAC */AFPERR_NOITEM, 0)) {
		failed();
	}

	FAIL (FPCloseFork(Conn,fork))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1)) 
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name3)) 
	FAIL (FPCloseVol(Conn2,vol2))
fin:
	delete_folder(vol, DIRDID_ROOT, name);
	FAIL (FPCloseDT(Conn, dt))
test_exit:
	exit_test("test53");
}


/* -------------------------- */
STATIC void test394()
{
char *name = "t394 file.txt";
int dt;
int dir;
int ret;
uint16_t vol = VolID;

	enter_test();
    fprintf(stdout,"===================\n");
	fprintf(stdout, "FPGetComment:test394: no comment\n");

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}
	dt = FPOpenDT(Conn,vol);

	ret = FPGetComment(Conn, vol,  DIRDID_ROOT , name);
	if (not_valid(ret, /* MAC */AFPERR_NOITEM, 0)) {
		failed();
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name)) 
	FAIL (FPCloseDT(Conn, dt))

test_exit:
	exit_test("test394");
}

/* ----------- */
void FPGetComment_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPGetComment page 176\n");
	test53();
	test394();
}

