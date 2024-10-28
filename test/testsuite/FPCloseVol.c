/* ----------------------------------------------
*/
#include "specs.h"

/* ----------- */
STATIC void test204()
{
uint16_t vol = VolID;
int ret;

	ENTER_TEST

	FAIL (FPCloseVol(Conn,vol))
	/* double close */
	ret = FPCloseVol(Conn, vol);
	if (not_valid_bitmap(ret, BITERR_ACCESS | BITERR_PARAM, AFPERR_PARAM)) {
		test_failed();
	}
	FAIL (htonl(AFPERR_PARAM) != FPCloseVol(Conn, vol +1))

	vol = VolID = FPOpenVol(Conn, Vol);
	if (vol == 0xffff) {
		test_failed();
	}
	exit_test("FPCloseVol:test204: Close Volume call");

}

/* ----------- */
void FPCloseVol_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
	test204();
}
