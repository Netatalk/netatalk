/* ----------------------------------------------
*/
#include "specs.h"


/* ------------------------- */
STATIC void test216()
{
uint16_t bitmap = 0;
int fork;
char *name = "t216 file.txt";
uint16_t vol = VolID;
int size;
DSI *dsi;

	dsi = &Conn->dsi;
	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPWrite:test216: read/write data fork\n");
	size = min(0x20000, dsi->server_quantum);
	if (size < 0x20000) {
		fprintf(stdout,"\t server quantum (%d) too small\n", size);
		nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}
	memset(Data, 0xff, 0x20000);

	if (htonl(AFPERR_DFULL) != FPWrite(Conn, fork, 0x7fffffffL -20, 15000, Data, 0 )) {
		failed();
		goto fin;
	}

	if (htonl(AFPERR_DFULL) != FPWrite(Conn, fork, 0x7fffffffL -12000, 15000, Data, 0 )) {
		failed();
		goto fin;
	}

	if (htonl(AFPERR_DFULL) != FPWrite(Conn, fork, 0x7ffe0000L, size, Data, 0 )) {
		failed();
		goto fin;
	}

fin:
	FAIL (fork && FPCloseFork(Conn,fork))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("test216");
}

/* ------------------------- */
STATIC void test226()
{
uint16_t bitmap = 0;
int fork;
char *name = "t226 file.txt";
uint16_t vol = VolID;
int size;
struct afp_volume_parms parms;
DSI *dsi;
int i,j;

	dsi = &Conn->dsi;
	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPWrite:test226: disk full error\n");
	size = min(0x20000, dsi->server_quantum); /* 128 k */
	if (size < 0x20000) {
		fprintf(stdout,"\t server quantum (%d) too small\n", size);
		nottested();
		goto test_exit;
	}

 	if (FPGetVolParam(Conn, vol, (1 << VOLPBIT_BFREE ))) {
		nottested();
		goto test_exit;
 	}
	afp_volume_unpack(&parms, dsi->commands +sizeof( uint16_t ), (1 << VOLPBIT_BFREE));

	if (parms.bfree > 2*1024*1024) {
		fprintf(stdout,"\t Volume too big, skipped\n");
		/* FIXME */
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}
	memset(Data, 0xff, 0x20000);
	i = parms.bfree / size;
	for (j = 0; j < i; j++) {
		if (FPWrite(Conn, fork, 0, size, Data, 0x80)) {
			failed();
			goto fin;
		}
	}

	if (htonl(AFPERR_DFULL) != FPWrite(Conn, fork, 0, size, Data, 0x80 )) {
		failed();
		goto fin;
	}

 	if (FPGetVolParam(Conn, vol, (1 << VOLPBIT_BFREE ))) {
		nottested();
		goto fin;
 	}
	afp_volume_unpack(&parms, dsi->commands +sizeof( uint16_t ), (1 << VOLPBIT_BFREE));

	if (parms.bfree > 7000) {
		if (FPWrite(Conn, fork, 0, parms.bfree - 7000, Data, 0x80)) {
			failed();
			goto fin;
		}
	}
	if (htonl(AFPERR_DFULL) != FPWrite(Conn, fork, 0, 8000, Data, 0x80 )) {
		failed();
	}

fin:

	FAIL (fork && FPCloseFork(Conn,fork))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("test226");
}

/* ------------------------- */
STATIC void test303()
{
uint16_t bitmap = 0;
int fork;
char *name = "t303 file.txt";
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;
	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPWrite:test303: Write 0 byte to data fork \n");

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}
	FAIL (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 1024))

	FAIL (FPWrite(Conn, fork, 1024, 0, Data, 0 ))

fin:

	FAIL (fork && FPCloseFork(Conn,fork))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("test303");
}

/* ----------- */
void FPWrite_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPWrite page 270\n");
	test216();
	test226();
	test303();
}
