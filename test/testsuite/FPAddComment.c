/* ----------------------------------------------
*/
#include "specs.h"
/* -------------------------- */
STATIC void test55()
{
int fork;
char *name  = "t55 dir no access";
char *name1 = "t55 file.txt";
char *name2 = "t55 ro dir";
char *name3  = "t55 --rwx-- dir";
int pdir = 0;
int rdir = 0;
int dir;
int ret;
uint16_t vol = VolID;
uint16_t vol2;
DSI *dsi2;
DSI *dsi = &Conn->dsi;
char *cmt;
int  dt;

	enter_test();
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!(rdir = read_only_folder(vol, DIRDID_ROOT, name2) ) ) {
		nottested();
		goto test_exit;
	}
	if (!(pdir = no_access_folder(vol, DIRDID_ROOT, name)) && !Quiet) {
		fprintf(stdout,"\tWARNING folder without access failed\n");
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

	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1))
	dt = FPOpenDT(Conn,vol);

	cmt = "essai";
	ret = FPAddComment(Conn, vol,  DIRDID_ROOT , name, cmt);
	if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
		failed();
	}

	ret = FPAddComment(Conn, vol,  DIRDID_ROOT , name2, cmt);
	if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
		failed();
	}
	if (!ret) {
		ret = FPGetComment(Conn, vol,  DIRDID_ROOT , name2);
		if (ret || memcmp(cmt, dsi->commands +1, strlen(cmt))) {
			failed();
		}
	}

	FAIL (FPAddComment(Conn, vol,  DIRDID_ROOT , name1, "Comment for toto.txt"))

	if (FPAddComment(Conn, vol,  DIRDID_ROOT , name3, "Comment for test folder")) {
		failed();
	}
	if (FPRemoveComment(Conn, vol,  DIRDID_ROOT , name3)) {
		failed();
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA  , 0 ,DIRDID_ROOT, name1,OPENACC_RD );
	if (!fork) {
		failed();
		goto fin;
	}

	FAIL (FPAddComment(Conn, vol,  DIRDID_ROOT , name3, "Comment"))
	FAIL (FPGetComment(Conn, vol,  DIRDID_ROOT , name3))
	FAIL (FPRemoveComment(Conn, vol,  DIRDID_ROOT , name3))
	FAIL (FPCloseFork(Conn,fork))
#if 0
	if (ntohl(AFPERR_ACCESS) != FPAddComment(Conn, vol,  DIRDID_ROOT , "bogus folder","essai")) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
#endif
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name3))
	if (pdir) {
		delete_folder(vol, DIRDID_ROOT, name);
	}
	if (rdir) {
		delete_folder(vol, DIRDID_ROOT, name2);
	}
	FAIL (FPCloseDT(Conn, dt))
test_exit:
	exit_test("FPAddComment:test55: add comment");
}

/* ----------- */
void FPAddComment_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPAddComment page 96\n");
    fprintf(stdout,"-------------------\n");
	test55();
}
