/* ----------------------------------------------
*/
#include "specs.h"
#include "adoublehelper.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>

static char temp[MAXPATHLEN];

/* ------------------------- */
STATIC void test3()
{
    char *name  = "t3.txt";
    uint16_t bitmap = 0;
    int fork1, fork2;
    uint16_t vol = VolID;

	ENTER_TEST

	if (!Quiet) {
		fprintf(stdout,"FPOpenFork:test3: Checks data fork / adouble metadata refcounting\n");
	}
	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		nottested();
		goto test_exit;
	}

	if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name, OPENACC_WR|OPENACC_RD)) == 0) {
		failed();
		goto fin;
	}
	FAIL (FPWrite(Conn, fork1, 0, 2000, Data, 0 ))
	FAIL (FPRead(Conn, fork1, 0, 2000, Data))
	FAIL (FPCloseFork(Conn, fork1))

	if (!Mac && delete_unix_md(Path, "", name)) {
		nottested();
		goto fin;
	}

	if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name, OPENACC_RD)) == 0) {
		failed();
		goto fin;
	}

	if ((fork2 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name, OPENACC_RD)) == 0) {
        FPCloseFork(Conn, fork1);
		failed();
		goto fin;
	}
	FAIL (FPCloseFork(Conn, fork2))

	FAIL (FPRead(Conn, fork1, 0, 2000, Data))
	FAIL (FPCloseFork(Conn, fork1))

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPOpenFork:test3: open data fork without metadata twice, close once, then read");
}

/* ------------------------- */
STATIC void test4()
{
    char *name  = "t4.txt";
    uint16_t bitmap = 0;
    int fork1, fork2;
    uint16_t vol = VolID;

	ENTER_TEST

	if (!Quiet) {
		fprintf(stdout,"FPOpenFork:test4: Checks reso fork / adouble metadata refcounting\n");
	}
	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		nottested();
		goto test_exit;
	}

	if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name, OPENACC_WR|OPENACC_RD)) == 0) {
		failed();
		goto fin;
	}
	FAIL (FPWrite(Conn, fork1, 0, 2000, Data, 0 ))
	FAIL (FPRead(Conn, fork1, 0, 2000, Data))
	FAIL (FPCloseFork(Conn, fork1))

	if (!Mac && delete_unix_md(Path, "", name)) {
		nottested();
		goto fin;
	}

	if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name, OPENACC_WR|OPENACC_RD)) == 0) {
		failed();
		goto fin;
	}

	if ((fork2 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name, OPENACC_WR|OPENACC_RD)) == 0) {
        FPCloseFork(Conn, fork1);
		failed();
		goto fin;
	}
	FAIL (FPCloseFork(Conn, fork2))

	FAIL (FPWrite(Conn, fork1, 0, 2000, Data, 0 ))
	FAIL (FPRead(Conn, fork1, 0, 2000, Data))

	FAIL (FPCloseFork(Conn, fork1))

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPOpenFork:test4: open reso fork without metadata twice, close once, then read");
}

/* ------------------------- */
STATIC void test7()
{
    char *name  = "t7.txt";
    uint16_t bitmap = 0;
    int fork1, fork2;
    uint16_t vol = VolID;

	ENTER_TEST

	if (!Quiet) {
		fprintf(stdout,"FPOpenFork:test7: Checks fork / adouble metadata refcounting\n");
	}
	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		nottested();
		goto test_exit;
	}

	if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name, OPENACC_WR|OPENACC_RD)) == 0) {
		failed();
		goto fin;
	}

	FAIL (FPWrite(Conn, fork1, 0, 2000, Data, 0 ))
	FAIL (FPRead(Conn, fork1, 0, 2000, Data))
	FAIL (FPCloseFork(Conn, fork1))

	if (!Mac && delete_unix_md(Path, "", name)) {
		nottested();
		goto fin;
	}

	if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap, DIRDID_ROOT, name, OPENACC_WR|OPENACC_RD)) == 0) {
		failed();
		goto fin;
	}
	if ((fork2 = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap, DIRDID_ROOT, name, OPENACC_RD|OPENACC_WR)) == 0) {
        FPCloseFork(Conn, fork1);
		failed();
		goto fin;
	}
	FAIL (FPCloseFork(Conn, fork2))

	FAIL (FPRead(Conn, fork1, 0, 2000, Data))
	FAIL (FPCloseFork(Conn, fork1))

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPOpenFork:test7: open data fork without metadata, then open and close resource fork");
}

/* ------------------------- */
STATIC void test47()
{
char *name = "t47 folder";
char *file = "t47 file.txt";
uint16_t vol = VolID;
uint16_t bitmap = 0;
int fork = 0, fork1 = 0;
int dir;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

 	if (!folder_with_ro_adouble(vol, DIRDID_ROOT, name, file)) {
		nottested();
		goto test_exit;
 	}

	dir = get_did(Conn, vol, DIRDID_ROOT, name);

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap , dir, file, OPENACC_RD);

	if (!fork) {
		failed();
		goto fin1;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap , dir, file, OPENACC_WR | OPENACC_RD);

	if (fork1) {
		failed();
		goto fin1;
	}

	FAIL (FPCloseFork(Conn,fork))

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap , dir, file , OPENACC_RD);

	if (!fork) {
		failed();
		goto fin1;
	}

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
		failed();
		goto fin1;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap , dir, file,OPENACC_WR | OPENACC_RD);

	if (fork1) {
		failed();
		goto fin1;
	}

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
		failed();
		goto fin1;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap , dir, file, OPENACC_RD);

	if (!fork1) {
		failed();
		goto fin1;
	}
	FAIL (FPCloseFork(Conn,fork1))
	fork1 = 0;

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
		failed();
		goto fin1;
	}

	FAIL (FPCloseFork(Conn,fork))
	fork = 0;
#if 0
    fprintf(stdout,"===================\n");
    fprintf(stdout,"test47: in a read/write folder\n");


	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, "test folder/toto.txt", OPENACC_RD);

	if (!fork) {
		failed();
		goto fin1;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, "test folder/toto.txt",OPENACC_WR | OPENACC_RD);

	if (fork1) {
		failed();
		goto fin1;
	}

	FAIL (FPCloseFork(Conn,fork))
	fork = 0;

	strcpy(temp, Path);
	strcat(temp,"/test folder/.AppleDouble/toto.txt");
	if (unlink(temp)) {
		fprintf(stdout,"\tFAILED Ressource fork not there\n");
		failed_nomsg();
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, "test folder/toto.txt", OPENACC_RD);

	if (!fork) {
		failed();
		goto fin1;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, "test folder/toto.txt",OPENACC_WR | OPENACC_RD);

	if (fork1) {
		failed();
		goto fin1;
	}

	FAIL (FPCloseFork(Conn,fork))
	fork = 0;

	strcpy(temp, Path);strcat(temp,"/test folder/.AppleDouble/toto.txt");
	if (unlink(temp)) {
		fprintf(stdout,"\tFAILED Ressource fork not there\n");
		failed_nomsg();
		goto fin1;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, "test folder/toto.txt", OPENACC_RD);
	if (!fork) {
		failed();
		goto fin1;
	}

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
		failed();
		goto fin1;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, "test folder/toto.txt",OPENACC_WR | OPENACC_RD);
    /* bad, but we are able to open read-write the ressource for of a read-only file (data fork)
     * difficult to fix.
     */
	if (!fork1) {
		failed();
		goto fin1;
	}

	if (FPWrite(Conn, fork1, 0, 10, Data, 0 )) {
		failed();
		goto fin1;
	}

	if (FPRead(Conn, fork, 0, 10, Data)) {
		failed();
		goto fin1;
	}

	FAIL (FPCloseFork(Conn,fork))
	fork = 0;
	if (FPWrite(Conn, fork1, 0, 20, Data, 0x80 )) {
		failed();
		goto fin1;
	}

	if (ntohl(AFPERR_PARAM) != FPRead(Conn, fork, 0, 30, Data)) {
		failed();
	}
#endif
fin1:
	if (fork1) FPCloseFork(Conn,fork1);
	if (fork) FPCloseFork(Conn,fork);
fin:
	delete_ro_adouble(vol, dir, file);
test_exit:
	exit_test("FPOpenFork:test47: open read only file read only then read write in a read only folder");
}

/* ------------------------- */
STATIC void test49()
{
char *name = "t49 folder";
char *file = "t49 file.txt";
uint16_t bitmap = 0;
int fork, fork1;
uint16_t vol = VolID;
int dir;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}
	if (FPCreateFile(Conn, vol,  0, dir , file)){
		nottested();
		goto fin;
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,dir, file, OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}
	if (!Mac) {
		sprintf(temp,"%s/%s/.AppleDouble/%s", Path, name, file);
		unlink(temp);
	}
	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,dir, file, OPENACC_WR | OPENACC_RD);

	if (!fork1) {
		failed();
	}

	FAIL (FPCloseFork(Conn,fork))
	FAIL (FPCloseFork(Conn,fork1))

	if (!unlink(temp) && !Quiet) {
		fprintf(stdout,"\tResource fork there!\n");
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,dir, file,OPENACC_WR | OPENACC_RD);

	if (!fork1) {
		failed();
	}
	FAIL (FPCloseFork(Conn,fork1))

	if (!unlink(temp) && !Quiet) {
		fprintf(stdout,"\tResource fork there!\n");
	}
fin:
	FAIL (FPDelete(Conn, vol,  dir , file))
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPOpenFork:test49: open read-write file without resource fork in a read-write folder");
}

/* ------------------------- */
STATIC void test152()
{
int dir;
uint16_t bitmap = 0;
char *name  = "t152 ro AppleDouble";
char *file  = "t152 test.pdf";
int fork;
uint16_t vol = VolID;
DSI *dsi = &Conn->dsi;
unsigned int ret;

	ENTER_TEST

	if ((!Mac && !Path)) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (adouble == AD_EA) {
		test_skipped(T_ADV2);
		goto test_exit;
	}

 	if (!folder_with_ro_adouble(vol, DIRDID_ROOT, name, file)) {
		nottested();
		goto test_exit;
 	}

	dir = get_did(Conn, vol, DIRDID_ROOT, name);

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,dir, file ,OPENACC_RD|OPENACC_WR);
	ret = dsi->header.dsi_code;
	if (not_valid(ret, 0, AFPERR_LOCK)) {
		failed();
	}
	if (!Mac && ret != htonl(AFPERR_LOCK)) {
		failed();
	}
	if (fork) {
		FPCloseFork(Conn, fork);
	}
	delete_ro_adouble(vol, dir, file);
test_exit:
	exit_test("FPOpenFork:test152: Error when no write access to .AppleDouble");
}

/* ------------------------- */
STATIC void test153()
{
char *name  = "t153.txt";
uint16_t bitmap = 0;
int fork;
uint16_t vol = VolID;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		nottested();
		goto test_exit;
	}
	if (!Mac && delete_unix_md(Path, "", name)) {
		nottested();
		goto fin;
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}
	FAIL (FPWrite(Conn, fork, 0, 2000, Data, 0 ))

	FAIL (FPRead(Conn, fork, 0, 2000, Data))

	FAIL (FPCloseFork(Conn, fork))
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPOpenFork:test153: open data fork without ressource fork");
}

/* ------------------------- */
STATIC void test157()
{
    char *name  = "t157.txt";
    uint16_t bitmap = 0;
    int fork;
    uint16_t vol = VolID;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		nottested();
		goto test_exit;
	}
	if (!Mac && delete_unix_md(Path, "", name)) {
		nottested();
		goto fin;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}

	if (FPRead(Conn, fork, 0, 2000, Data) != ntohl(AFPERR_EOF))
        failed();

	FAIL (FPCloseFork(Conn, fork))
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPOpenFork:test157: open not existing ressource fork read-only");
}

/* ------------------------- */
STATIC void test156()
{
int dir;
uint16_t bitmap = 0;
int fork;
char *name  = "t156 ro AppleDouble";
char *file  = "t156 test.pdf";
uint16_t vol = VolID;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

 	if (!(dir = folder_with_ro_adouble(vol, DIRDID_ROOT, name, file))) {
		nottested();
		goto test_exit;
 	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,dir, file ,OPENACC_RD|OPENACC_WR);

	if (!fork) {
		failed();
		goto fin;
	}
	if (fork && FPCloseFork(Conn, fork)) {
		failed();
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,dir, file ,OPENACC_RD|OPENACC_WR);
	if (!fork) {
		failed();
	}
	if (fork && FPCloseFork(Conn, fork)) {
		failed();
	}
fin:
	delete_ro_adouble(vol, dir, file);
test_exit:
	exit_test("FPOpenFork:test156: Open data fork with no write access to .AppleDouble");
}

/* ------------------------- */
STATIC void test321()
{
uint16_t bitmap = 0;
int fork;
char *file  = "t321 test.txt";
uint16_t vol = VolID;
int fd;

	ENTER_TEST

	if ((!Mac && !Path)) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (adouble == AD_EA) {
		test_skipped(T_ADV2);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , file)){
		nottested();
		goto test_exit;
	}

	if (!Mac) {
		sprintf(temp,"%s/%s", Path, file);
		if (chmod(temp, 0444) < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to chmod %s :%s\n", temp, strerror(errno));
			}
			failed_nomsg();
			goto fin;
		}
		sprintf(temp,"%s/.AppleDouble/%s", Path, file);
		if (!Quiet) {
			fprintf(stdout,"unlink %s \n", temp);
		}
		unlink(temp);
		fd = open(temp, O_RDWR | O_CREAT, 0666);
		if (fd < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to create %s :%s\n", temp, strerror(errno));
			}
			failed_nomsg();
			goto fin;
		}
		close(fd);
		if (chmod(temp, 0444) < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to chmod %s :%s\n", temp, strerror(errno));
			}
			failed_nomsg();
			goto fin;
		}
	}
	bitmap = 0x693f;
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, file, bitmap, 0 )) {
		failed();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, file, bitmap, 0 )) {
		failed();
		goto fin;
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , 0 ,DIRDID_ROOT, file, OPENACC_RD|OPENACC_DWR);
	if (!fork) {
		failed();
	}
	if (fork && FPCloseFork(Conn, fork)) {
		failed();
	}

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, file))
test_exit:
	exit_test("FPOpenFork:test321: Bogus (empty) resource fork");
}

/* --------------------- */
STATIC void test372()
{
char *name  = "t372 file name.txt";
char data[20];
uint16_t vol = VolID;
int fork;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;
uint16_t bitmap;
int fd;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto fin;
	}
	bitmap = (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<FILPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		     (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		failed();
		goto fin;
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		memcpy(filedir.finder_info, "TEXTttxt", 8);

 		FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, (1<<FILPBIT_FINFO), &filedir))
	    FAIL (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0))
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}
	if (FPWrite(Conn, fork, 0, 5, "test\r", 0 )) {
		failed();
		goto fin1;
	}

	if (FPRead(Conn, fork, 0, 5, data)) {
		failed();
		goto fin1;
	}
	if (memcmp(data, "test\r", 5)) {
		if (!Quiet) {
			fprintf(stdout, "\tFAILED wrote \"test\\r\" get \"%s\"\n", data);
		}
	    failed_nomsg();
	}
	if (!Mac) {
		sprintf(temp,"%s/%s", Path, name);
		fd = open(temp, O_RDWR , 0666);
		if (fd < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to open %s :%s\n", temp, strerror(errno));
			}
			failed_nomsg();
			goto fin1;
		}
		if (read(fd, data, 5) != 5) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to read data:%s\n", strerror(errno));
			}
			failed_nomsg();
		}
		if (memcmp(data, "test\r", 5)) {
			if (!Quiet) {
				fprintf(stdout, "\tFAILED not \"test\\r\" get 0x%x 0x%x 0x%x 0x%x 0x%x\n",
				data[0],data[1],data[2],data[3],data[4]);
			}
		    failed_nomsg();
		}
		close(fd);
	}

fin1:
	FAIL (FPCloseFork(Conn,fork))
fin:
	FPDelete(Conn, vol,  DIRDID_ROOT , name);
test_exit:
	exit_test("FPRead:test372: no crlf convertion for TEXT file");
}

/* -------------------------
 * for this test you need a volume with options:crlf
 * in AppleVolumes.default
 *
 * not run by default (need -f test387)
*/

/* --------------------- */
STATIC void test387()
{
char *name  = "t387 file name.txt";
char data[20];
uint16_t vol = VolID;
int fork;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;
uint16_t bitmap;
int fd;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto fin;
	}
	bitmap = (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<FILPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		     (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		failed();
		goto fin;
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		memcpy(filedir.finder_info, "TEXTttxt", 8);

 		FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, (1<<FILPBIT_FINFO), &filedir))
	    FAIL (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0))
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}
	if (FPWrite(Conn, fork, 0, 5, "test\r", 0 )) {
		failed();
		goto fin1;
	}

	if (FPRead(Conn, fork, 0, 5, data)) {
		failed();
		goto fin1;
	}
	if (memcmp(data, "test\r", 5)) {
		if (!Quiet) {
			fprintf(stdout, "\tFAILED wrote \"test\\r\" get \"%s\"\n", data);
		}
	    failed_nomsg();
	}
	if (!Mac) {
		sprintf(temp,"%s/%s", Path, name);
		fd = open(temp, O_RDWR , 0666);
		if (fd < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to open %s :%s\n", temp, strerror(errno));
			}
			failed_nomsg();
			goto fin1;
		}
		if (read(fd, data, 5) != 5) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to read data:%s\n", strerror(errno));
			}
			failed_nomsg();
		}
		if (memcmp(data, "test\n", 5)) {
			if (!Quiet) {
				fprintf(stdout, "\tFAILED not \"test\\n\" get 0x%x 0x%x 0x%x 0x%x 0x%x\n",
				data[0],data[1],data[2],data[3],data[4]);
			}
		    failed_nomsg();
		}
		close(fd);
	}

fin1:
	FAIL (FPCloseFork(Conn,fork))
fin:
	FPDelete(Conn, vol,  DIRDID_ROOT , name);
test_exit:
	exit_test("FPRead:test387: crlf convertion for TEXT file");
}

/* --------------------- */
STATIC void test388()
{
char *name  = "t388 file name.rtf";
char data[20];
uint16_t vol = VolID;
int fork;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;
uint16_t bitmap;
int fd;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto fin;
	}
	bitmap = (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<FILPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		     (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		failed();
		goto fin;
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		memcpy(filedir.finder_info, "TEXTttxt", 8);

 		FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, (1<<FILPBIT_FINFO), &filedir))
	    FAIL (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0))
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}
	if (FPWrite(Conn, fork, 0, 5, "test\r", 0 )) {
		failed();
		goto fin1;
	}

	if (FPRead(Conn, fork, 0, 5, data)) {
		failed();
		goto fin1;
	}
	if (memcmp(data, "test\r", 5)) {
		if (!Quiet) {
			fprintf(stdout, "\tFAILED wrote \"test\\r\" get \"%s\"\n", data);
		}
	    failed_nomsg();
	}
	if (!Mac) {
		sprintf(temp,"%s/%s", Path, name);
		fd = open(temp, O_RDWR , 0666);
		if (fd < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to open %s :%s\n", temp, strerror(errno));
			}
			failed_nomsg();
			goto fin1;
		}
		if (read(fd, data, 5) != 5) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to read data:%s\n", strerror(errno));
			}
			failed_nomsg();
		}
		if (memcmp(data, "test\n", 5)) {
			if (!Quiet) {
				fprintf(stdout, "\tFAILED not \"test\\n\" get 0x%x 0x%x 0x%x 0x%x 0x%x\n",
				data[0],data[1],data[2],data[3],data[4]);
			}
		    failed_nomsg();
		}
		close(fd);
	}

fin1:
	FAIL (FPCloseFork(Conn,fork))
fin:
	FPDelete(Conn, vol,  DIRDID_ROOT , name);
test_exit:
	exit_test("FPRead:test388: crlf convertion for TEXT file (not default type)");
}

/* --------------------- */
STATIC void test392()
{
char *name  = "t392 file name.pdf";
char data[20];
uint16_t vol = VolID;
int fork;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;
uint16_t bitmap;
int fd;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto fin;
	}
	bitmap = (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<FILPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		     (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		failed();
		goto fin;
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		memcpy(filedir.finder_info, "PDF CARO", 8);

 		FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, (1<<FILPBIT_FINFO), &filedir))
	    FAIL (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0))
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}
	if (FPWrite(Conn, fork, 0, 5, "test\r", 0 )) {
		failed();
		goto fin1;
	}

	if (FPRead(Conn, fork, 0, 5, data)) {
		failed();
		goto fin1;
	}
	if (memcmp(data, "test\r", 5)) {
		if (!Quiet) {
			fprintf(stdout, "\tFAILED wrote \"test\\r\" get \"%s\"\n", data);
		}
	    failed_nomsg();
	}
	if (!Mac) {
		sprintf(temp,"%s/%s", Path, name);
		fd = open(temp, O_RDWR , 0666);
		if (fd < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to open %s :%s\n", temp, strerror(errno));
			}
			failed_nomsg();
			goto fin1;
		}
		if (read(fd, data, 5) != 5) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to read data:%s\n", strerror(errno));
			}
			failed_nomsg();
		}
		if (memcmp(data, "test\r", 5)) {
			if (!Quiet) {
				fprintf(stdout, "\tFAILED not \"test\\r\" get 0x%x 0x%x 0x%x 0x%x 0x%x\n",
				data[0],data[1],data[2],data[3],data[4]);
			}
		    failed_nomsg();
		}
		close(fd);
	}

fin1:
	FAIL (FPCloseFork(Conn,fork))
fin:
	FPDelete(Conn, vol,  DIRDID_ROOT , name);
test_exit:
	exit_test("FPRead:test392: no crlf convertion for no TEXT file");
}

/* ------------------------- */
STATIC void test411()
{
char *name = "t411 folder";
char *file = "t411 file.txt";
uint16_t bitmap = 0;
int fork;
uint16_t vol = VolID;
int dir;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}
	if (FPCreateFile(Conn, vol,  0, dir , file)){
		nottested();
		goto fin;
	}

	if (!Mac) {
		delete_unix_md(Path, name, file);
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,dir, file, OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}

	FAIL (FPCloseFork(Conn,fork))

    if (!Mac && (delete_unix_md(Path, name, file) == 0)) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED Ressource fork there!\n");
		}
		failed_nomsg();
	}

fin:
	FAIL (FPDelete(Conn, vol,  dir , file))
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPOpenFork:test411: open read-only a file without ressource fork");

}

/* -------------------------
   Didn't fail but help when tracing afpd
*/
STATIC void test415()
{
char *name = "t415 folder";
char *file = "t415 file.txt";
uint16_t bitmap = 0;
int fork;
int fork1;
uint16_t vol = VolID;
int dir;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}
	if (FPCreateFile(Conn, vol,  0, dir , file)){
		nottested();
		goto fin;
	}

	if (!Mac) {
		sprintf(temp,"%s/%s/.AppleDouble/%s", Path, name, file);
		if (unlink(temp)) {
		    nottested();
		    goto fin;
		}
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,dir, file, OPENACC_WR | OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,dir, file, OPENACC_WR | OPENACC_RD);

	if (!fork1) {
		failed();
		goto fin1;
	}

	FAIL (FPCloseFork(Conn,fork1))

fin1:
	FAIL (FPCloseFork(Conn,fork))

fin:
	FAIL (FPDelete(Conn, vol,  dir , file))
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPOpenFork:test415: don't set the name again in the resource fork if file open twice");
}

/* ------------------------- */
STATIC void test236()
{
    char *name1 = "t236 dir";
    char *name2 = "etc";
    char *name3 = "t236 dir/etc";
    char *name4 = "passwd";
    int  testdir, etcdir;
    int fork = 0;
    uint16_t vol = VolID, bitmap;
    struct afp_filedir_parms filedir;
    DSI *dsi = &Conn->dsi;
    int  ofs =  3 * sizeof( uint16_t );

	ENTER_TEST

	if (!Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	bitmap = (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<FILPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		     (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID);


    /* create "t236 dir" */
	testdir  = FPCreateDir(Conn, vol, DIRDID_ROOT , name1);
	if (!testdir) {
		nottested();
		goto test_exit;
	}

    /* create "t236 dir/etc" */
	etcdir = FPCreateDir(Conn, vol, testdir, name2);
	if (!etcdir) {
		nottested();
		goto fin;
	}

    /* make sure the server has "etc" in the dircache */
	if (FPGetFileDirParams(Conn, vol,  etcdir, "", 0, (1<< DIRPBIT_DID))) {
		failed();
	}

    /* make sure the server's curdir is *not* etc */
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, "", 0, (1<< DIRPBIT_DID))) {
		failed();
	}

    /* remove "etc" */
	if (delete_unix_dir(Path, name3)) {
		failed();
		goto fin;
	}

    /* symlink "etc" to "/etc/passwd" */
    if (symlink_unix_file("/etc", Path, name3)) {
        failed();
        goto fin;
    }

    /* Ok, we've malicously prepared the dircache and filesystem, now confront afpd with it */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap, etcdir, name4, OPENACC_RD);
	if (fork) {
        /* Check if the volume uses option 'followsymlinks' */
        if (FPGetFileDirParams(Conn, vol,  etcdir, "", 1 << FILPBIT_UNIXPR, 0))
            failed();
        afp_filedir_unpack(&filedir, dsi->data + ofs, 1 << FILPBIT_UNIXPR, 0);
        if (!(S_ISLNK(filedir.unix_priv))) {
            test_skipped(T_NOSYML);
            goto fin;
        }

		failed();
		goto fin;
	}

fin:
	FAIL (fork && FPCloseFork(Conn,fork))
    FAIL (unlink_unix_file(Path, name1, name2))
	FAIL (testdir && FPDelete(Conn, vol,  testdir, ""))
test_exit:
	exit_test("FPOpenFork:test236: symlink attack: try reading /etc/passwd");
}

/* ------------------------- */
STATIC void test237()
{
    char *name1 = "t237 dir";
    char *name2 = "passwd";
    char *name3 = "t237 dir/passwd";
    char *name4 = "/etc/passwd";
    int  testdir;
    int fork = 0;
    uint16_t vol = VolID, bitmap;
    struct afp_filedir_parms filedir;
    DSI *dsi = &Conn->dsi;
    int  ofs =  3 * sizeof( uint16_t );

	ENTER_TEST

	if (!Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	bitmap = (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<FILPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		     (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID);


    /* create "t237 dir" */
	testdir  = FPCreateDir(Conn, vol, DIRDID_ROOT , name1);
	if (!testdir) {
		nottested();
		goto test_exit;
	}

    /* symlink "etc" to "/etc/passwd" */
    if (symlink_unix_file(name4, Path, name3)) {
        failed();
        goto fin;
    }

    /* Check if the volume uses option 'followsymlinks' */
	if (FPGetFileDirParams(Conn, vol, testdir, name2, 1<< FILPBIT_UNIXPR, 0))
		failed();
    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 1<< FILPBIT_UNIXPR, 0);
    if (!(S_ISLNK(filedir.unix_priv))) {
		test_skipped(T_NOSYML);
		goto fin;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap, testdir, name2, OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}

	if (FPRead(Conn, fork, 0, strlen(name4), Data)) {
		failed();
		goto fin;
	}

    Data[11] = 0;
	if (!Quiet) {
		fprintf(stdout, "readlink: %s\n", Data);
	}

    if (strcmp(Data, name4) != 0) {
		failed();
		goto fin;
    }

fin:
	FAIL (fork && FPCloseFork(Conn,fork))
    FAIL (unlink_unix_file(Path, name1, name2))
	FAIL (testdir && FPDelete(Conn, vol,  testdir, ""))
test_exit:
	exit_test("FPOpenFork:test237: symlink reading and attack: try reading /etc/passwd");
}

/* ------------------------- */
STATIC void test238()
{
    char *name1 = "t238 dir";
    char *name2 = "link";
    char *name3 = "t238 dir/link";
    char *verylonglinkname = "verylonglinkname";
    int  testdir;
    int fork = 0;
    uint16_t vol = VolID, bitmap;
    struct afp_filedir_parms filedir;
    DSI *dsi = &Conn->dsi;
    int  ofs =  3 * sizeof( uint16_t );

	ENTER_TEST

	if (!Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	bitmap = (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<FILPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		     (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID);


    /* create "t238 dir" */
	testdir  = FPCreateDir(Conn, vol, DIRDID_ROOT , name1);
	if (!testdir) {
		nottested();
		goto test_exit;
	}
	if (FPCreateFile(Conn, vol, 0, testdir, verylonglinkname)){
		nottested();
		goto fin;
	}

    if (symlink_unix_file(verylonglinkname, Path, name3)) {
        failed();
        goto fin;
    }

    /* Check if the volume uses option 'followsymlinks' */
	if (FPGetFileDirParams(Conn, vol, testdir, name2, 1<< FILPBIT_UNIXPR, 0))
		failed();
    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data + ofs, 1<< FILPBIT_UNIXPR, 0);
    if (!(S_ISLNK(filedir.unix_priv))) {
		test_skipped(T_NOSYML);
		goto fin;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap, testdir, name2, OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}

	if (ntohl(AFPERR_PARAM) != FPRead(Conn, fork, 0, 8, Data)) {
		failed();
		goto fin;
	}

	if (FPRead(Conn, fork, 0, strlen(verylonglinkname), Data)) {
		failed();
		goto fin;
	}

    Data[strlen(verylonglinkname)] = 0;
	if (!Quiet) {
		fprintf(stdout, "readlink: %s\n", Data);
	}

    if (strcmp(Data, verylonglinkname) != 0) {
		failed();
		goto fin;
    }

fin:
	FAIL (fork && FPCloseFork(Conn,fork))
    FAIL (unlink_unix_file(Path, name1, name2))
	FAIL (testdir && FPDelete(Conn, vol,  testdir, verylonglinkname))
	FAIL (testdir && FPDelete(Conn, vol,  testdir, ""))
test_exit:
	exit_test("FPOpenFork:test238: symlink reading with short reqcount");
}

/* ------------------------- */
STATIC void test431()
{
    char *name  = "resource_fork_conversion_test";
    int fork1 = 0;
    uint16_t vol = VolID;
    char cmd[8192];
    char *teststring = "test\n";
    int  ofs =  3 * sizeof( uint16_t );
    struct afp_filedir_parms filedir = { 0 };
    DSI *dsi = &Conn->dsi;

	ENTER_TEST

	// FIXME: failing; might require manually moving data file to
	// data/resource_fork_conversion_test
	if (Exclude) {
		test_skipped(T_EXCLUDE);
		goto test_exit;
	}
	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

    /* touch resource_fork_conversion_test base file */
    if (snprintf(cmd, sizeof(cmd), "touch %s/resource_fork_conversion_test", Path) > sizeof(cmd)) {
	if (!Quiet) {
		fprintf(stdout,"FPOpenFork:test431: path too long\n");
	}
		failed();
		goto fin;
    }
    if (system(cmd) != 0) {
		failed();
		goto fin;
    }

    /* Create .AppleDouble directory */
    if (snprintf(cmd, sizeof(cmd), "%s/.AppleDouble", Path) > sizeof(cmd)) {
	if (!Quiet) {
		fprintf(stdout,"FPOpenFork:test431: path too long\n");
	}
		failed();
		goto fin;
    }
    if (mkdir(cmd, 0777) != 0 && errno != EEXIST) {
		failed();
		goto fin;
    }

    /* Copy resource fork */
    if (snprintf(cmd, sizeof(cmd), "cp data/resource_fork_conversion_test %s/.AppleDouble/", Path) > sizeof(cmd)) {
	if (!Quiet) {
		fprintf(stdout,"FPOpenFork:test431: path too long\n");
	}
		failed();
		goto fin;
    }
    if (system(cmd) != 0) {
		failed();
		goto fin;
    }

    /* Enumerating should convert it */
	if (FPEnumerate_ext2(Conn, vol,  DIRDID_ROOT , "",
                         (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN),
                         (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT))) {
		failed();
		goto fin;
    }

	if ((fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS, (1<<FILPBIT_FINFO), DIRDID_ROOT, name, OPENACC_WR|OPENACC_RD)) == 0) {
		failed();
		goto fin;
	}

	FAIL (FPRead(Conn, fork1, 0, strlen(teststring), Data))

    if (memcmp(Data, teststring, strlen(teststring)) != 0) {
	if (!Quiet) {
		fprintf(stdout,"FPOpenFork:test431: conversion failed\n");
	}
		failed();
		goto fin;
    }

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, (1<<FILPBIT_FINFO), 0)) {
		failed();
	}

    afp_filedir_unpack(&filedir, dsi->data + 6, (1<<FILPBIT_FINFO), 0);
    if (memcmp(filedir.finder_info, "TEXTTEST", 8)) {
	if (!Quiet) {
		fprintf(stdout,"FAILED - bad type/creator\n");
	}
        failed_nomsg();
    }

fin:
    if (fork1)
        FPCloseFork(Conn, fork1);
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPOpenFork:test431: check AppleDouble conversion from v2 to ea");
}


/* ----------- */
void FPOpenFork_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPOpenFork page 230\n");
    fprintf(stdout,"-------------------\n");
    test3();
    test4();
    test7();
#if 0
    test47();
#endif
    test49();
	test152();
	test153();
	test156();
	test157();
	test321();
	test372();
	test392();
	test411();
    test236();
    test237();
    test238();
    test431();
}
