/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
#define BUF_S 3000
static char w_buf[BUF_S];

STATIC void test148()
{
int fork = 0;
int fork1 = 0;
uint16_t bitmap = 0;
char *name = "t148 test";
char *dir = "t148 dir";
uint32_t rep;
uint16_t vol = VolID;
int tdir;
DSI *dsi = &Conn->dsi;

	ENTER_TEST
 	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
 	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto test_exit;
	}

	tdir  = FPCreateDir(Conn,vol, DIRDID_ROOT, dir);
	if (!tdir) {
		test_failed();
	}

	if (FPEnumerate_ext(Conn, vol,  DIRDID_ROOT , "",
		                    (1 << FILPBIT_PDINFO )|(1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN), 0)) {
		test_failed();
	}

	FAIL (FPEnumerate_ext(Conn, vol,  DIRDID_ROOT , "", 0, (1 << DIRPBIT_PDINFO )))

    /* > 2 Gb */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		test_failed();
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork1) {
		test_failed();
		goto fin;
	}

	memset(w_buf, 'b', BUF_S);
	if (FPWrite_ext(Conn, fork, 0, 1714, w_buf, 0 )) {
		test_failed();
	}
	else {
		memcpy(&rep, dsi->commands, sizeof(rep));
		if (rep) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED size %d\n", rep);
			}
			test_failed();
		}
		memcpy(&rep, dsi->commands +sizeof(rep), sizeof(rep));
		if (ntohl(rep) != 1714) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED size %d\n", ntohl(rep));
			}
			test_failed();
		}
	}

	if (FPWrite_ext(Conn, fork1, 0, 1714, w_buf, 0 )) {
		test_failed();
	}
	else {
		memcpy(&rep, dsi->commands, sizeof(rep));
		if (rep) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED size %d\n", rep);
			}
			test_failed();
		}
		memcpy(&rep, dsi->commands +sizeof(rep), sizeof(rep));
		if (ntohl(rep) != 1714) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED size %d\n", ntohl(rep));
			}
			test_failed();
		}
	}

	if (ntohl(AFPERR_EOF) != FPRead_ext(Conn, fork, 0, 2000, Data) ||
		dsi->header.dsi_len != htonl(1714)
		) {
		test_failed();
	}

	if (FPEnumerate_ext(Conn, vol,  DIRDID_ROOT , "",
		                    (1 << FILPBIT_PDINFO )|(1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN), 0)) {
		test_failed();
	}

fin:
	FAIL (FPCloseFork(Conn,fork))
	FAIL (FPCloseFork(Conn,fork1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, dir))
test_exit:
	exit_test("FPWriteExt:test148: AFP 3.0 FPWriteExt");
}

/* --------------------- */
STATIC void test207()
{
int fork;
int fork1;
uint16_t bitmap = 0;
char *name = "t207 file";
uint16_t vol = VolID;
int i;

	ENTER_TEST
 	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
 	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto test_exit;
	}

	if (FPEnumerate_ext(Conn, vol,  DIRDID_ROOT , "",
		                    (1 << FILPBIT_PDINFO )|(1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN),
		                    0))
	{
		test_failed();
	}

    /* > 2 Gb */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		test_failed();
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork1) {
		test_failed();
		goto fin;
	}

	if (Conn2) {
		FPGetSrvrMsg(Conn2, 0, 0);
	}
	memset(w_buf, 'b', BUF_S);
	FAIL (FPWrite_ext(Conn, fork, ((off_t)1 << 31) +20, 2000, w_buf, 0 ))
	FAIL (FPWrite_ext(Conn, fork1, ((off_t)1 << 31) +20, 1000, w_buf, 0 ))
	FAIL (FPWrite_ext(Conn, fork, ((off_t)1 << 31) +1000 , 3000, w_buf, 0 ))
	FAIL (FPWrite_ext(Conn, fork, 0 , 200, w_buf, 0x80 ))
	FAIL (FPWrite_ext(Conn, fork1, 0 , 200, w_buf, 0x80 ))

	if (Conn2) {
		FPGetSrvrMsg(Conn2, 0, 0);
	}
	if (FPRead_ext(Conn, fork, 10, 10000, Data)) {
		test_failed();
	}
	else for (i = 0; i < 10000; i++) {
		if (Data[i] != 0) {
			test_failed();
			break;
		}
	}

	if (FPRead_ext(Conn, fork, ((off_t)1 << 31) +20, 3000, Data)) {
		test_failed();
	}
	else for (i = 0; i < 3000; i++) {
		if (Data[i] == 0) {
			test_failed();
			break;
		}
	}

	FPEnumerate_ext(Conn, vol,  DIRDID_ROOT , "",
		                    (1 << FILPBIT_PDINFO )|(1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN), 0);

	FAIL (FPCloseFork(Conn,fork))
	FAIL (FPCloseFork(Conn,fork1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))

    /* ==========> 4 Gb ============= */

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , "very big")) {
		test_failed();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, "very big",OPENACC_WR | OPENACC_RD);
	if (!fork) {
		test_failed();
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, "very big",OPENACC_WR | OPENACC_RD);
	if (!fork1) {
		test_failed();
		goto fin;
	}

	FAIL (FPWrite_ext(Conn, fork, ((off_t)1 << 32) +20, 2000, w_buf, 0 ))

	if (Conn2) {
		FPGetSrvrMsg(Conn2, 0, 0);
	}

	FAIL (FPWrite_ext(Conn, fork1, ((off_t)1 << 32) +20, 1000, w_buf, 0 ))

	if (Conn2) {
		FPGetSrvrMsg(Conn2, 0, 0);
	}
	if (FPRead_ext(Conn, fork, 10, 10000, Data)) {
		test_failed();
	}
	else for (i = 0; i < 10000; i++) {
		if (Data[i] != 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED Data != 0\n");
			}
			test_failed();
			break;
		}
	}

	if (FPRead_ext(Conn, fork, ((off_t)1 << 32) +20, 1500, Data)) {
		test_failed();
	}
	else for (i = 0; i < 1500; i++) {
		if (Data[i] == 0) {
			test_failed();
			break;
		}
	}

	FPEnumerate_ext(Conn, vol,  DIRDID_ROOT , "",
		                    (1 << FILPBIT_PDINFO )|(1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN), 0);

	FAIL (FPCloseFork(Conn,fork1))

fin:
	FAIL (FPCloseFork(Conn,fork))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, "very big"))
test_exit:
	exit_test("FPWriteExt:test207: AFP 3.0 read/Write");
}

/* ------------------------- */
STATIC void test304()
{
uint16_t bitmap = 0;
int fork;
char *name = "t304 file.txt";
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;
	ENTER_TEST
 	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
 	}

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

	FAIL (FPWrite_ext(Conn, fork, 1024, 0, Data, 0 ))


fin:

	FAIL (fork && FPCloseFork(Conn,fork))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPWriteExt:test304: Write 0 byte to data fork");
}


/* ----------- */
void FPWriteExt_test()
{
    ENTER_TESTSET
    test148();
	test207();
	test304();
}
