/* ----------------------------------------------
*/
#include "specs.h"

/* ----------------------- */
STATIC void test1(void)
{
int ret;
	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPGetSrvrInfo:test1: GetSrvInfo\n");

	ret = FPGetSrvrInfo(Conn);
	if (ret) {
		failed();
	}

	exit_test("test1");
}

/* ----------- */
void FPGetSrvrInfo_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPGetSrvInfo page 194\n");
	test1();
}
