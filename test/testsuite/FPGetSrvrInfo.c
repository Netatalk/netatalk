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
		test_failed();
	}

	exit_test("FPGetSrvrInfo:test1: GetSrvInfo");
}

/* ----------- */
void FPGetSrvrInfo_test()
{
    ENTER_TESTSET
	test1();
}
