/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test201()
{
uint16_t  dir;
uint16_t vol = VolID;
int ret;

	enter_test();

	if (0xffff == (dir = FPOpenDT(Conn,vol))) {
		nottested();
		goto test_exit;
	}
	ret = FPCloseDT(Conn, dir +1);
	if (not_valid(ret, AFPERR_PARAM, 0)) {
		failed();
	}
	FAIL (FPCloseDT(Conn, dir))
test_exit:
	exit_test("FPCloseDT:test201: FPCloseDT call");
}

/* ----------- */
void FPCloseDT_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPCloseDT page 128\n");
    fprintf(stdout,"-------------------\n");
	test201();
}
