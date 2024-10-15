/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test202()
{
uint16_t vol = VolID;

	enter_test();
    fprintf(stdout,"===================\n");
	fprintf(stdout, "FPFlush:test202: flush volume call\n");

	FAIL (FPFlush(Conn, vol))
	
	FAIL (htonl(AFPERR_PARAM) != FPFlush(Conn, vol +1))
	exit_test("test202");
}

/* ----------- */
void FPFlush_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPFlush page 169\n");
	test202();
}

