/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test62()
{
int fork;
int fork1 = 0;
uint16_t bitmap = 0;
char *name = "test62 SetForkParams Errors";
int ret;
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto test_exit;
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		test_failed();
		goto fin;
	}
	ret = FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), (1<< 31));
	if (not_valid(ret, /* MAC */0, AFPERR_PARAM)) {
		test_failed();
		goto fin;
	}
	if (!ret) {
		FAIL (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data))
	}

	ret = FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), -2);
	if (not_valid(ret, /* MAC */0, AFPERR_PARAM)) {
		test_failed();
		goto fin;
	}
	if (!ret) {
		FAIL (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data))
	}

	FAIL (ntohl(AFPERR_BITMAP) != FPSetForkParam(Conn, fork, 1<<FILPBIT_ATTR, 0))
	FAIL (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 100))
	FAIL (FPRead(Conn, fork, 0, 100, Data))
	FAIL (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 99, 2, Data))
	FAIL (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 0))
	FAIL (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 1, Data))
	FAIL  (FPByteLock(Conn, fork, 0, 0 , 0 /* set */, 100))
	FAIL (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 50))

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork1) {
		test_failed();
		goto fin;
	}
	if (!FPByteLock(Conn, fork1, 0, 0 , 60 , 100)) {
		test_failed();
		FPByteLock(Conn, fork1, 0, 1 , 60 , 100);
	}
	FAIL (htonl(AFPERR_LOCK) != FPSetForkParam(Conn, fork1, (1<<FILPBIT_DFLEN), 0))
	FAIL (FPCloseFork(Conn,fork1))
	fork1 = 0;

    /* Netatalk 2 doesn't drop locks when a fork is closed, 3 does */
    FPByteLock(Conn, fork, 0, 1 /* clear */ , 0, 100);

	FAIL (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 200))
	FAIL (FPByteLock(Conn, fork, 0, 0 , 0 /* set */, 100))
	FAIL (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 50))
	FAIL (FPByteLock(Conn, fork, 0, 1 /* clear */ , 0, 100))

fin:
	FAIL (fork && FPCloseFork(Conn,fork))
	FAIL (fork1 && FPCloseFork(Conn,fork1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPSetForkParms:test62: SetForkParams errors");
}

/* ------------------------- */
STATIC void test141()
{
int fork;
uint16_t bitmap = 0;
char *name  = "t140 setforkmode file";
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		test_nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		test_nottested();
		goto fin;
	}

	FAIL (ntohl(AFPERR_BITMAP) != FPSetForkParam(Conn, fork, (1<<FILPBIT_RFLEN), 0))
	if (Conn->afp_version < 30) {
		FAIL (ntohl(AFPERR_BITMAP) != FPSetForkParam(Conn, fork, (1<<FILPBIT_EXTDFLEN), 0))
	}

	FAIL (FPCloseFork(Conn, fork))

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name ,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		test_failed();
		goto fin;
	}

	FAIL (ntohl(AFPERR_BITMAP) != FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 0))

	if (Conn->afp_version < 30) {
		FAIL (ntohl(AFPERR_BITMAP) != FPSetForkParam(Conn, fork, (1<<FILPBIT_EXTRFLEN), 0))
	}

	FAIL (FPCloseFork(Conn, fork))

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPSetForkParms:test141: Setforkmode error");
}


/* ------------------------- */
STATIC void test217()
{
int fork;
uint16_t bitmap = 0;
char *name  = "t217 setforkparms file";
uint16_t vol = VolID;
DSI *dsi;
unsigned int ret;

	dsi = &Conn->dsi;

	ENTER_TEST
	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		test_nottested();
		goto test_exit;
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		test_failed();
		goto fin;
	}
	FAIL (FPSetForkParam(Conn, fork, (1<<FILPBIT_EXTDFLEN), 0))
	FAIL (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 1, 1, Data))
	FAIL (ntohl(AFPERR_PARAM) != FPSetForkParam(Conn, fork, (1<<FILPBIT_EXTDFLEN), 1<<31))
	ret = FPSetForkParam(Conn, fork, (1<<FILPBIT_EXTDFLEN), (1UL<<31));
	if (ret) {
		test_failed();
		goto fin;
	}
	FAIL (ntohl(AFPERR_PARAM) != FPRead_ext(Conn, fork, (1 << 31) +1, 1, Data))
	FAIL (ntohl(AFPERR_EOF) != FPRead_ext(Conn, fork, (1UL << 31) +1, 1, Data))
	FAIL (FPRead(Conn, fork, (1UL << 31) -1, 1, Data))

fin:
	FAIL (fork && FPCloseFork(Conn,fork))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPSetForkParms:test217: Setfork size 64 bits");
}

/* ------------------------- */
STATIC void test306()
{
uint16_t bitmap = 0;
int fork;
char *name = "t306 file.txt";
uint16_t vol = VolID;
uint32_t data = (unsigned)-1;
DSI *dsi;

	dsi = &Conn->dsi;
	ENTER_TEST

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);

	if (!fork) {
		test_failed();
		goto fin;
	}
	FAIL (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 1024))
	if (FPRead(Conn, fork, 1020, 4, (char *)&data)) {
		test_failed();
		goto fin;
	}
	if (data != 0) {
		if (Mac && !Quiet) {
			fprintf(stdout,"\tMac result 0x%x but 0 expected\n", data);
		}
		else {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED got 0x%x but 0 expected\n", data);
			}
			test_failed();
			goto fin;
		}
	}

fin:

	FAIL (fork && FPCloseFork(Conn,fork))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPSetForkParms:test306: set fork size, new size > old size");
}

/* ----------- */
void FPSetForkParms_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
    test62();
    test141();
    test217();
    test306();
}
