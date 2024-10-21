/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test108()
{
int dir;
char *name  = "t108 exchange file";
char *name1 = "t108 new file name";
char *ndir  = "t108 dir";
int fid_name;
int fid_name1;
int temp;
uint16_t vol = VolID;

	enter_test();

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		failed();
	}
	FAIL (FPCreateFile(Conn, vol,  0, dir, name1))

	fid_name  = get_fid(Conn, vol, DIRDID_ROOT , name);
	fid_name1 = get_fid(Conn, vol, dir , name1);

	write_fork( Conn, vol, DIRDID_ROOT , name, "blue");
	write_fork( Conn, vol, dir , name1, "red");
	/* ok */
	FAIL (FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))

	/* test remove of no cnid db */
	if ((temp = get_fid(Conn, vol, DIRDID_ROOT , name)) != fid_name) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name);
		}
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name);
		}
		failed_nomsg();
	}
	if ((temp = get_fid(Conn, vol, dir , name1)) != fid_name1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name1);
		}
		failed_nomsg();
	}

	read_fork(Conn, vol,  DIRDID_ROOT , name, 3);
	if (strcmp(Data,"red")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be red\n");
		}
		failed_nomsg();
	}
	read_fork(Conn,  vol, dir , name1, 4);
	if (strcmp(Data,"blue")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be blue\n");
		}
		failed_nomsg();
	}
	FAIL (FPDelete(Conn, vol,  dir , name1))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, ndir))
test_exit:
	exit_test("FPExchangeFiles:test108: exchange files");
}

/* ------------------------- */
STATIC void test111()
{
int fork;
int fork1;
int dir;
uint16_t bitmap = 0;
char *name  = "t111 exchange open files";
char *name1 = "t111 new file name";
char *ndir  = "t111 dir";
int fid_name;
int fid_name1;
uint16_t vol = VolID;
int ret;

	enter_test();

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		nottested();
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		failed();
		goto fin;
	}
	FAIL (FPCreateFile(Conn, vol,  0, dir, name1))

	fid_name  = get_fid(Conn, vol, DIRDID_ROOT , name);
	fid_name1 = get_fid(Conn, vol, dir , name1);

	write_fork(Conn, vol, DIRDID_ROOT , name, "blue");
	write_fork(Conn, vol, dir , name1, "red");

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}

	/* ok */
	FAIL (FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))

	read_fork(Conn, vol, DIRDID_ROOT , name, 3);
	if (strcmp(Data,"red")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be red\n");
		}
		failed_nomsg();
	}
	read_fork(Conn, vol, dir , name1, 4);
	if (strcmp(Data,"blue")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be blue\n");
		}
		failed_nomsg();
	}

	FAIL (FPWrite(Conn, fork, 0, 3, "new", 0 ))

	read_fork(Conn, vol, dir , name1, 3);
	if (strcmp(Data,"new")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be new\n");
		}
		failed_nomsg();
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork1) {
		failed();
	}

	FAIL (FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))
	if (fork1) FPCloseFork(Conn,fork1);

	FAIL (FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))

	if (fork) FPCloseFork(Conn,fork);
	if ((ret = get_fid(Conn, vol, DIRDID_ROOT , name)) != fid_name) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", ret, fid_name);
		}
		failed_nomsg();
	}

	if ((ret = get_fid(Conn, vol, dir , name1)) != fid_name1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", ret, fid_name);
		}
		failed_nomsg();
	}

fin:
	FAIL (FPDelete(Conn, vol,  dir , name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, ndir))
test_exit:
	exit_test("FPExchangeFiles:test111: exchange open files");
}

/* ------------------------- */
STATIC void test197()
{
int dir;
char *name  = "t197 exchange file";
char *name1 = "t197 new file name";
char *ndir  = "t197 dir";
uint16_t vol = VolID;

	enter_test();

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		failed();
	}
	FAIL (FPCreateFile(Conn, vol,  0, dir, name1))

	write_fork(Conn,  vol, DIRDID_ROOT , name, "blue");
	write_fork(Conn,  vol, dir , name1, "red");
	/* ok */
	FAIL (FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))

	read_fork(Conn, vol,  DIRDID_ROOT , name, 3);
	if (strcmp(Data,"red")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be red\n");
		}
		failed_nomsg();
	}
	read_fork(Conn,  vol, dir , name1, 4);
	if (strcmp(Data,"blue")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be blue\n");
		}
		failed_nomsg();
	}
	FAIL (FPDelete(Conn, vol,  dir , name1))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, ndir))
test_exit:
	exit_test("FPExchangeFiles:test197: exchange files (doesn't check files' ID)");
}

/* ------------------------- */
STATIC void test342()
{
int dir;
char *name  = "t342 exchange file";
char *name1 = "t342 new file name";
int fid_name;
int fid_name1;
int temp;
uint16_t vol = VolID;
struct afp_filedir_parms filedir;
char finder_info[32];
uint16_t bitmap;
DSI *dsi = &Conn->dsi;
int  ofs =  3 * sizeof( uint16_t );

	enter_test();

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	dir = DIRDID_ROOT;
	FAIL (FPCreateFile(Conn, vol,  0, dir, name1))

    /* set some metadata, MUST NOT be exchanged */
	bitmap = (1 << FILPBIT_FINFO);
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
		failed();
	}
    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    memcpy(filedir.finder_info, "TESTTEST", 8);
    memcpy(finder_info, filedir.finder_info, 32);
    FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir))

	fid_name  = get_fid(Conn, vol, DIRDID_ROOT , name);
	fid_name1 = get_fid(Conn, vol, dir , name1);

	write_fork( Conn, vol, DIRDID_ROOT , name, "blue");
	write_fork( Conn, vol, dir , name1, "red");
	/* ok */
	FAIL (FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))

    /* test whether FinderInfo was preserved */
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap, 0)) {
		failed();
	}
    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, bitmap, 0);
    if (memcmp(finder_info, filedir.finder_info, 32) != 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED: metadata wasn't preserved\n");
		}
		failed_nomsg();
    }

	/* test remove of no cnid db */
	if ((temp = get_fid(Conn, vol, DIRDID_ROOT , name)) != fid_name) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name);
		}
		failed_nomsg();
	}
	if ((temp = get_fid(Conn, vol, dir , name1)) != fid_name1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name1);
		}
		failed_nomsg();
	}

	read_fork(Conn, vol,  DIRDID_ROOT , name, 3);
	if (strcmp(Data,"red")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be red\n");
		}
		failed_nomsg();
	}
	read_fork(Conn,  vol, dir , name1, 4);
	if (strcmp(Data,"blue")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be blue\n");
		}
		failed_nomsg();
	}
	FAIL (FPDelete(Conn, vol,  dir , name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPExchangeFiles:test342: exchange files");
}

/* ------------------------- */
STATIC void test389()
{
int dir;
char *name  = "t389 exchange file";
char *name1 = "t389 new file name";
uint16_t bitmap = 0;
int fid_name;
int fid_name1;
int temp;
int fork;
uint16_t vol = VolID;

	enter_test();

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	dir = DIRDID_ROOT;
	FAIL (FPCreateFile(Conn, vol,  0, dir, name1))

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}

	fid_name  = get_fid(Conn, vol, DIRDID_ROOT , name);

	fid_name1 = get_fid(Conn, vol, dir , name1);

	write_fork( Conn, vol, DIRDID_ROOT , name, "blue");
	write_fork( Conn, vol, dir , name1, "red");
	/* ok */
	FAIL (FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))
	FAIL (FPCloseFork(Conn,fork))

	/* test remove of no cnid db */
	if ((temp = get_fid(Conn, vol, DIRDID_ROOT , name)) != fid_name) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name);
		}
		failed_nomsg();
	}
	if ((temp = get_fid(Conn, vol, dir , name1)) != fid_name1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name1);
		}
		failed_nomsg();
	}

	read_fork(Conn, vol,  DIRDID_ROOT , name, 3);
	if (strcmp(Data,"red")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be red\n");
		}
		failed_nomsg();
	}
	read_fork(Conn,  vol, dir , name1, 4);
	if (strcmp(Data,"blue")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be blue\n");
		}
		failed_nomsg();
	}

fin:
	FAIL (FPDelete(Conn, vol,  dir , name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPExchangeFiles:test389: exchange files, source with resource fork open");
}

/* ------------------------- */
STATIC void test390()
{
int dir;
char *name  = "t390 exchange file";
char *name1 = "t390 new file name";
uint16_t bitmap = 0;
int fid_name;
int fid_name1;
int temp;
int fork;
uint16_t vol = VolID;

	enter_test();

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	dir = DIRDID_ROOT;
	FAIL (FPCreateFile(Conn, vol,  0, dir, name1))

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}

	fid_name  = get_fid(Conn, vol, DIRDID_ROOT , name);

	fid_name1 = get_fid(Conn, vol, dir , name1);

	write_fork( Conn, vol, DIRDID_ROOT , name, "blue");
	write_fork( Conn, vol, dir , name1, "red");
	/* ok */
	FAIL (FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))
	FAIL (FPFlushFork(Conn, fork))
	FAIL (FPCloseFork(Conn,fork))

	/* test remove of no cnid db */
	if ((temp = get_fid(Conn, vol, DIRDID_ROOT , name)) != fid_name) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name);
		}
		failed_nomsg();
	}
	if ((temp = get_fid(Conn, vol, dir , name1)) != fid_name1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name1);
		}
		failed_nomsg();
	}

	read_fork(Conn, vol,  DIRDID_ROOT , name, 3);
	if (strcmp(Data,"red")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be red\n");
		}
		failed_nomsg();
	}
	read_fork(Conn,  vol, dir , name1, 4);
	if (strcmp(Data,"blue")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be blue\n");
		}
		failed_nomsg();
	}

fin:
	FAIL (FPDelete(Conn, vol,  dir , name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPExchangeFiles:test390: exchange files, source with resource fork open");
}

/* ------------------------- */
STATIC void test391()
{
int dir;
char *name  = "t391 exchange file";
char *name1 = "t391 new file name";
uint16_t bitmap = 0;
int fid_name;
int fid_name1;
int temp;
int fork;
uint16_t vol = VolID;

	enter_test();

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	dir = DIRDID_ROOT;
	FAIL (FPCreateFile(Conn, vol,  0, dir, name1))

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name1, OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}

	fid_name  = get_fid(Conn, vol, DIRDID_ROOT , name);

	fid_name1 = get_fid(Conn, vol, dir , name1);

	write_fork( Conn, vol, DIRDID_ROOT , name, "blue");
	write_fork( Conn, vol, dir , name1, "red");
	/* ok */
	FAIL (FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1))
	FAIL (FPCloseFork(Conn,fork))

	/* test remove of no cnid db */
	if ((temp = get_fid(Conn, vol, DIRDID_ROOT , name)) != fid_name) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name);
		}
		failed_nomsg();
	}
	if ((temp = get_fid(Conn, vol, dir , name1)) != fid_name1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name1);
		}
		failed_nomsg();
	}

	read_fork(Conn, vol,  DIRDID_ROOT , name, 3);
	if (strcmp(Data,"red")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be red\n");
		}
		failed_nomsg();
	}
	read_fork(Conn,  vol, dir , name1, 4);
	if (strcmp(Data,"blue")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be blue\n");
		}
		failed_nomsg();
	}

fin:
	FAIL (FPDelete(Conn, vol,  dir , name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPExchangeFiles:test391: exchange files, dest with resource fork open");
}

/* ----------- */
void FPExchangeFiles_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPExchangeFiles page 166\n");
    fprintf(stdout,"-------------------\n");
	test108();
	test111();
	test197();
	test342();
	test389();
	test390();
	test391();
}
