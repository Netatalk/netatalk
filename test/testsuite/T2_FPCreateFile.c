/* ----------------------------------------------
*/
#include "specs.h"

static char temp[MAXPATHLEN];

/* -------------------------- */
STATIC void test325()
{
char *name = "t325 file";
uint16_t vol = VolID;
DSI *dsi;
int ret;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (Path[0] == '\0') {
        test_skipped(T_PATH);
		goto test_exit;
	}

	if (adouble == AD_EA) {
       test_skipped(T_ADV2);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto test_exit;
	}

	sprintf(temp,"%s/%s", Path, name);
	if (!Quiet) {
		fprintf(stdout,"unlink data fork\n");
	}
	if (unlink(temp) <0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unlink(%s) %s\n", temp, strerror(errno));
		}
		test_failed();
		goto fin;
	}

	sprintf(temp,"%s/.AppleDouble/%s", Path, name);
	if (!Quiet) {
		fprintf(stdout,"chmod 444 resource fork\n");
	}
	if (chmod(temp, 0444) <0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED chmod(%s) %s\n", temp, strerror(errno));
		}
		test_failed();
		goto fin;
	}

	ret = FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name);
	if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
		test_failed();
	}

	ret = FPDelete(Conn, vol,  DIRDID_ROOT , name);
	if (not_valid(ret, /* MAC */0, AFPERR_NOOBJ)) {
		test_failed();
	}

fin:
	FPDelete(Conn, vol,  DIRDID_ROOT , name);
	unlink(temp);

test_exit:
	exit_test("FPCreateFile:test325: recreate a file with dangling symlink and no right");
}

/* ----------- */
void T2FPCreateFile_test()
{
    ENTER_TESTSET
    test325();
}
