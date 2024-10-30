/* ----------------------------------------------
*/
#include "specs.h"

extern char icon0_256[];

/* -------------------------- */
STATIC void test213()
{
uint16_t vol = VolID;
uint16_t dt;
unsigned int ret;
u_char   u_null[] = { 0, 0, 0, 0 };

	ENTER_TEST

	dt = FPOpenDT(Conn,vol);

	ret = FPGetIconInfo(Conn,  dt, (unsigned char *) "ttxt", 1);
	if (ret == htonl(AFPERR_NOITEM)) {
		FAIL (FPAddIcon(Conn,  dt, "ttxt", "3DMF", 1, 0, 256, icon0_256 ))
		goto test_exit;
	}

	FAIL (FPGetIconInfo(Conn,  dt, (unsigned char *) "ttxt", 1))

	FAIL (htonl(AFPERR_NOITEM) != FPGetIconInfo(Conn,  dt, (unsigned char *) "ttxt", 256 ))

	if (!Mac) {
		ret = FPGetIconInfo(Conn,  dt, (unsigned char *) "UNIX", 1 );
		if (ret == htonl(AFPERR_NOITEM)) {
			FAIL (FPGetIconInfo(Conn,  dt, u_null, 1 ))
			FAIL (htonl(AFPERR_NOITEM) != FPGetIconInfo(Conn,  dt, u_null, 2 ))
			goto test_exit;
		}
		else if (ret) {
			test_failed();
			goto test_exit;
		}
		else {
			FAIL (htonl(AFPERR_NOITEM) != FPGetIconInfo(Conn,  dt, (unsigned char *) "UNIX", 2 ))
		}
	}

test_exit:
	FPCloseDT(Conn,dt);
	exit_test("FPGetIconInfo:test213: get Icon Info call");
}

/* ----------- */
void FPGetIconInfo_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
	test213();
}
