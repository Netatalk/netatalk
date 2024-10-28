/* ----------------------------------------------
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "specs.h"
extern char *Vol2;

/* ----------------------- */
STATIC void test320(void)
{
int ret;
DSI *dsi = &Conn->dsi;
int nbe,i;
int found = 0;
unsigned char len;
uint16_t vol2;
unsigned char *b;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}
	if (*Vol2 == 0) {
		test_skipped(T_VOL2);
		goto test_exit;
	}
	if (!Interactive) {
		test_skipped(T_MANUAL);
		goto test_exit;
	}

	vol2 = FPOpenVol(Conn, Vol2);
	if (vol2 == 0xffff) {
		nottested();
		goto test_exit;
	}
	printf("Modify afp.conf and press enter\n");
	getchar();


	FAIL (FPCloseVol(Conn,vol2))

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

	vol2 = FPOpenVol(Conn, Vol2);
	if (vol2 == 0xffff) {
		nottested();
		goto test_exit;
	}
	FAIL (FPCloseVol(Conn,vol2))
test_exit:
	exit_test("FPGetSrvrParms:test320: GetSrvrParms after volume config file has been modified");
}

/* ----------- */
void T2FPGetSrvrParms_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPGetSrvrParms page 203\n");
    fprintf(stdout,"-------------------\n");
	test320();
}
