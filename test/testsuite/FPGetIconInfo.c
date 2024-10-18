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

	enter_test();
    fprintf(stdout,"===================\n");
	fprintf(stdout, "FPGetIconInfo:test213: get Icon Info call\n");

	dt = FPOpenDT(Conn,vol);

	ret = FPGetIconInfo(Conn,  dt, (unsigned char *) "ttxt", 1);
	if (ret == htonl(AFPERR_NOITEM)) {
		FAIL (FPAddIcon(Conn,  dt, "ttxt", "3DMF", 1, 0, 256, icon0_256 ))
	}

	FAIL (FPGetIconInfo(Conn,  dt, (unsigned char *) "ttxt", 1))

	FAIL (htonl(AFPERR_NOITEM) != FPGetIconInfo(Conn,  dt, (unsigned char *) "ttxt", 256 ))

	if (!Mac) {
		ret = FPGetIconInfo(Conn,  dt, (unsigned char *) "UNIX", 1 );
		if (ret == htonl(AFPERR_NOITEM)) {
			FAIL (FPGetIconInfo(Conn,  dt, u_null, 1 ))
			FAIL (htonl(AFPERR_NOITEM) != FPGetIconInfo(Conn,  dt, u_null, 2 ))
		}
		else if (ret) {
			failed();
		}
		else {
			FAIL (htonl(AFPERR_NOITEM) != FPGetIconInfo(Conn,  dt, (unsigned char *) "UNIX", 2 ))
		}
	}

	FPCloseDT(Conn,dt);
	exit_test("test213");
}

/* ----------- */
void FPGetIconInfo_test()
{
// FIXME: icon tests are broken in Netatalk 4.0
#if 0
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPGetIconInfo page 188\n");
	test213();
#endif
}
