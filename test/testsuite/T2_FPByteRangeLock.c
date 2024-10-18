/* ----------------------------------------------
*/
#include "specs.h"
#include "adoublehelper.h"

#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static char temp[MAXPATHLEN];

/* ------------------------- */
static void test_bytelock(uint16_t vol2, char *name, int type)
{
int fork;
int fork1;
uint16_t bitmap = 0;
int len = (type == OPENFORK_RSCS)?(1<<FILPBIT_RFLEN):(1<<FILPBIT_DFLEN);
struct flock lock;
int fd;
int ret;
uint16_t vol = VolID;

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		failed();
		return;
	}
	fork = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name, OPENACC_WR |OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
	if (!fork) {
		failed();
		FPDelete(Conn, vol,  DIRDID_ROOT, name);
		return;
	}

	if (FPSetForkParam(Conn, fork, len , 50)) {fprintf(stdout,"\tFAILED\n");}
	if (FPByteLock(Conn, fork, 0, 0 , 0 , 100)) {fprintf(stdout,"\tFAILED\n");}
	if (FPRead(Conn, fork, 0, 40, Data)) {fprintf(stdout,"\tFAILED\n");}
	if (FPWrite(Conn, fork, 10, 40, Data, 0)) {fprintf(stdout,"\tFAILED\n");}
	fork1 = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (fork1) {
		failed();
		FPCloseFork(Conn,fork);
	}

	strcpy(temp, Path);
    strcat(temp,(type == OPENFORK_RSCS && adouble == AD_V2) ? "/.AppleDouble/" : "/");
    strcat(temp, name);

	fprintf(stdout," \n---------------------\n");
	fprintf(stdout, "open(\"%s\", O_RDWR)\n", temp);
	fd = open(temp, O_RDWR, 0);
	if (fd >= 0) {
        if (adouble == AD_V2) {
            lock.l_start = 0;
            lock.l_len = 1024;
        } else {
            lock.l_start = AD_FILELOCK_BASE;
            lock.l_len = 8;
        }
        lock.l_type = F_WRLCK;
        lock.l_whence = SEEK_SET;

    	if ((ret = fcntl(fd, F_SETLK, &lock)) >= 0 || (errno != EACCES && errno != EAGAIN)) {
    	    if (!ret >= 0)
    	    	errno = 0;
    		perror("fcntl ");
			fprintf(stdout,"\tFAILED\n");
    	}
    	fcntl(fd, F_UNLCK, &lock);
    	close(fd);
	}
	else {
    	perror("open ");
		fprintf(stdout,"\tFAILED \n");
	}
	fork1 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (fork1) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn2,fork1);
	}

	FPCloseFork(Conn,fork);
	fork1 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork1) {
		failed();
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		FAIL (FPCloseFork(Conn2,fork1))
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
}

/* -------------------- */
void test117()
{
char *name = "t117 exclusive open DF";
uint16_t vol2;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test117: test open excl mode\n");

	if (!Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		nottested();
		goto test_exit;
	}
//	test_bytelock(vol2, name, OPENFORK_DATA);
	name = "t117 exclusive open RF";
	test_bytelock(vol2, name, OPENFORK_RSCS);

	FPCloseVol(Conn2,vol2);
test_exit:
	exit_test("test117");
}

/* ----------- */
void FPByteRangeLock_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock page 101\n");
    test117();
}
