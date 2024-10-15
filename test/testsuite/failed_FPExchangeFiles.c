/* ----------------------------------------------
*/
#if 0
#define QUIRK
#endif
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
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPExchangeFiles:test108: exchange files\n");

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
#ifdef QUIRK	
		fprintf(stdout,"\tFAILED (IGNORED) %x should be %x\n", temp, fid_name);
#else		
		fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name);
		failed_nomsg();
#endif		
	}
	if ((temp = get_fid(Conn, vol, dir , name1)) != fid_name1) {
#ifdef QUIRK	
		fprintf(stdout,"\tFAILED (IGNORED) %x should be %x\n", temp, fid_name1);
#else
		fprintf(stdout,"\tFAILED %x should be %x\n", temp, fid_name1);
		failed_nomsg();
#endif		
	}

	read_fork(Conn, vol,  DIRDID_ROOT , name, 3);
	if (strcmp(Data,"red")) {
		fprintf(stdout,"\tFAILED should be red\n");
		failed_nomsg();
	}
	read_fork(Conn,  vol, dir , name1, 4);
	if (strcmp(Data,"blue")) {
		fprintf(stdout,"\tFAILED should be blue\n");
		failed_nomsg();
	}
	FAIL (FPDelete(Conn, vol,  dir , name1))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, ndir))
test_exit:
	exit_test("test108");
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
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPExchangeFiles:test111: exchange open files\n");

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
		fprintf(stdout,"\tFAILED should be red\n");
		failed_nomsg();
	}
	read_fork(Conn, vol, dir , name1, 4);
	if (strcmp(Data,"blue")) {
		fprintf(stdout,"\tFAILED should be blue\n");
		failed_nomsg();
	}

	FAIL (FPWrite(Conn, fork, 0, 3, "new", 0 ))

	read_fork(Conn, vol, dir , name1, 3);
	if (strcmp(Data,"new")) {
		fprintf(stdout,"\tFAILED should be new\n");
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
#ifdef QUIRK	
		fprintf(stdout,"\tFAILED (IGNORED) %x should be %x\n", ret, fid_name);
#else
		fprintf(stdout,"\tFAILED %x should be %x\n", ret, fid_name);
		failed_nomsg();
#endif		
	}

	if ((ret = get_fid(Conn, vol, dir , name1)) != fid_name1) {
#ifdef QUIRK	
		fprintf(stdout,"\tFAILED (IGNORED) %x should be %x\n", ret, fid_name1);
#else
		fprintf(stdout,"\tFAILED %x should be %x\n", ret, fid_name1);
		failed_nomsg();
#endif		
	}
		
fin:
	FAIL (FPDelete(Conn, vol,  dir , name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, ndir))
test_exit:
	exit_test("test111");
}


/* ----------- */
void FPExchangeFiles_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPExchangeFiles page 166\n");
	test108();
	test111();
}

