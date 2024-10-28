/* ----------------------------------------------
*/
#include "specs.h"

/* ----------------------- */
STATIC void test1(void)
{
int ret;
	ENTER_TEST

	ret = FPGetSrvrInfo(Conn);
	if (ret) {
		failed();
	}

	exit_test("FPGetSrvrInfo:test1: GetSrvInfo");
}

/* ----------- */
void FPGetSrvrInfo_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
	test1();
}
