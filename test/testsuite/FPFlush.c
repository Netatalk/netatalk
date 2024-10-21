/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test202()
{
uint16_t vol = VolID;

	enter_test();

	FAIL (FPFlush(Conn, vol))

	FAIL (htonl(AFPERR_PARAM) != FPFlush(Conn, vol +1))
	exit_test("FPFlush:test202: flush volume call");
}

/* ----------- */
void FPFlush_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPFlush page 169\n");
    fprintf(stdout,"-------------------\n");
	test202();
}
