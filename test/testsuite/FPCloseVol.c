/* ----------------------------------------------
*/
#include "specs.h"

/* ----------- */
STATIC void test204()
{
uint16_t vol = VolID;
int ret;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPCloseVol:test204: Close Volume call\n");

	FAIL (FPCloseVol(Conn,vol))
	/* double close */
	ret = FPCloseVol(Conn, vol);
	if (not_valid_bitmap(ret, BITERR_ACCESS | BITERR_PARAM, AFPERR_PARAM)) {
		failed();
	}
	FAIL (htonl(AFPERR_PARAM) != FPCloseVol(Conn, vol +1))

	vol = VolID = FPOpenVol(Conn, Vol);
	if (vol == 0xffff) {
		failed();
	}
	exit_test("test204");

}

/* ----------- */
void FPCloseVol_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPCloseVol page 130\n");
	test204();
}
