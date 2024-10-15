/* ----------------------------------------------
*/
#include "specs.h"

extern char icon0_256[];

/* -------------------------- */
STATIC void test115()
{
uint16_t vol = VolID;
uint16_t dt;
unsigned int ret;
char   u_null[] = { 0, 0, 0, 0 };

	enter_test();
    fprintf(stdout,"===================\n");
	fprintf(stdout, "FPGetIcon:test115: get Icon call\n");

	dt = FPOpenDT(Conn,vol);

	ret = FPGetIcon(Conn,  dt, "ttxt", "3DMF", 1, 256);
	if (ret == htonl(AFPERR_NOITEM)) {
		FAIL (FPAddIcon(Conn,  dt, "ttxt", "3DMF", 1, 0, 256, icon0_256 ))
	}

	FAIL (FPGetIcon(Conn,  dt, "ttxt", "3DMF", 1, 256))
	
	if (!Mac) {
		FAIL (htonl(AFPERR_NOITEM) != FPGetIcon(Conn,  dt, "UNIX", "TEXT",  2 ,512))
#if 0
// netatalk 2.0.4 and above don't have a default icon	
		ret = FPGetIcon(Conn,  dt, "UNIX", "TEXT", 1, 256 );
		if (ret == htonl(AFPERR_NOITEM)) {
			FAIL(FPGetIcon(Conn,  dt, u_null, u_null, 1, 256 ))
		}
		else if (ret) {
			failed();
		}
#endif
	}
	
	FPCloseDT(Conn,dt);
	exit_test("test115");
}

/* ----------- */
void FPGetIcon_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPGetIcon page 186\n");
	test115();
}

