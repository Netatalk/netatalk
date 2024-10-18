/* ----------------------------------------------
*/
#include "specs.h"

/* ----------------------- */
STATIC void test210(void)
{
	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPGetSrvrMsg:test210: GetSrvrParms call\n");

	FAIL (FPGetSrvrMsg(Conn, 0, 0))
	FAIL (FPGetSrvrMsg(Conn, 1, 0))
	exit_test("test210");
}

/* ----------- */
void FPGetSrvrMsg_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPGetSrvrMsg page 200\n");
	test210();
}
