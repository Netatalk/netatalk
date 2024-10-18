/* ----------------------------------------------
*/
#include "specs.h"

/* --------------------------------- */
static int is_there(CONN *conn, int did, char *name)
{
uint16_t vol = VolID;

	return FPGetFileDirParams(conn, vol,  did, name,
	         (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID)
	         ,
	         (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID)
		);
}

/* ------------------------- */
static void fatal_failed(void)
{
	failed();
}

/* ------------------------- */
STATIC void test5()
{
uint16_t bitmap = 0;
int fork, fork1;
char *name = "t5 file.txt";
uint16_t vol = VolID;
int size;
DSI *dsi;

	dsi = &Conn->dsi;
	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPRead:test5: read/write data fork\n");
	size = min(10000, dsi->server_quantum);
	if (size < 2000) {
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

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, size, Data)) {
		failed();
		goto fin1;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_WR | OPENACC_RD);
	if (!fork1) {
		failed();
		goto fin1;
	}

	if (FPWrite(Conn, fork1, size -2000, 2048, Data, 0 /*0x80 */)) {
		failed();
		goto fin2;
	}
	FAIL (FPFlushFork(Conn, fork1))

	if (FPRead(Conn, fork, 0, size, Data)) {
		failed();
		goto fin2;
	}

	if (FPWrite(Conn, fork1, 0, 100, Data, 0x80 )) {
		failed();
		goto fin2;
	}
	FAIL (FPFlush(Conn, vol))
	FAIL (FPCloseFork(Conn,fork1))
	FAIL (FPCloseFork(Conn,fork))
	/* ----------------- */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork1) {
		failed();
		goto fin2;
	}

	if (ntohl(AFPERR_ACCESS) != FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 0)) {
		failed();
		goto fin2;
	}

	if (FPSetForkParam(Conn, fork1, (1<<FILPBIT_DFLEN), 0)) {
		failed();
		goto fin2;
	}

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 10, size, Data)) {
		failed();
		goto fin2;
	}

	if (FPWrite(Conn, fork1, 100, 20, Data, 0 )) {
		failed();
		goto fin2;
	}

	if (FPRead(Conn, fork, 110, 10, Data)) {
		failed();
		goto fin2;
	}

fin2:
	FPCloseFork(Conn,fork1);

fin1:
	FAIL (FPCloseFork(Conn,fork))

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("test5");
}

/* ------------------------- */
STATIC void test46()
{
uint16_t bitmap = 0;
int fork, fork1;
char *name = "t46 file.txt";
uint16_t vol = VolID;
int size;
DSI *dsi;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPread:test46: read/write resource fork\n");
	size = min(10000, dsi->server_quantum);
	if (size < 2000) {
		fprintf(stdout,"\t server quantum (%d) too small\n", size);
		nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, size, Data)) {
		failed();
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork1) {
		failed();
		goto fin;
	}

	if (FPWrite(Conn, fork1, size -2000, 2048, Data, 0 /*0x80 */)) {
		failed();
		FPCloseFork(Conn,fork1);
		goto fin1;
	}
	FPFlushFork(Conn, fork1);

	if (FPRead(Conn, fork, 0, size, Data)) {
		failed();
	}

	if (FPWrite(Conn, fork1, 0, 100, Data, 0x80 )) {
		failed();
		goto fin1;
	}
	FPCloseFork(Conn,fork1);
	FPCloseFork(Conn,fork);
	/* ----------------- */
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork1) {
		failed();
		goto fin;
	}

	if (ntohl(AFPERR_ACCESS) != FPSetForkParam(Conn, fork, (1<<FILPBIT_RFLEN), 0)) {
		failed();
		goto fin1;
	}

	if (FPSetForkParam(Conn, fork1, (1<<FILPBIT_RFLEN), 0)) {
		failed();
		goto fin1;
	}

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 10, size, Data)) {
		failed();
		goto fin1;
	}

	if (FPWrite(Conn, fork1, 100, 20, Data, 0 )) {
		failed();
	}
	else if (FPRead(Conn, fork, 110, 10, Data)) {
		failed();
	}

fin1:
	FPCloseFork(Conn,fork1);
fin:
	if (fork) FPCloseFork(Conn,fork);

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("test46");
}

/* -------------------------- */
STATIC void test59()
{
int fork;
uint16_t bitmap = 0;
char *name = "test59 FPRead,FPWrite 2GB lim";
uint16_t vol = VolID;
int ret;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPRead:test59: 2 GBytes for offset limit FPRead, FPWrite\n");

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

    /* > 2 Gb */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}

	FAIL (ntohl(AFPERR_PARAM) != FPRead(Conn, fork, ((off_t)1 << 31) +20, 3000, Data))
	FAIL (ntohl(AFPERR_PARAM) != FPWrite(Conn, fork, ((off_t)1 << 31) +20, 3000, Data, 0))
	ret = FPWrite(Conn, fork, 0x7fffffff, 30, Data,0);
	if (not_valid(ret, /* MAC */AFPERR_MISC, AFPERR_DFULL)) {
		failed();
	}

	FAIL (FPCloseFork(Conn,fork))
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test59");
}

/* -------------------------- */
STATIC void test61()
{
int fork;
uint16_t bitmap = 0;
char *name = "test61 FPRead, FPWrite error";
uint16_t vol = VolID;
int size;
DSI *dsi;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPRread:test61: FPRead, FPWrite errors\n");
	size = min(10000, dsi->server_quantum);
	if (size < 2000) {
		nottested();
		fprintf(stdout,"\t server quantum (%d) too small\n", size);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR );
	if (!fork) {
		failed();
		goto fin;
	}
	FAIL (ntohl(AFPERR_ACCESS) != FPRead(Conn, fork, 0, 30, Data))
	FAIL (FPWrite(Conn, fork, 0, 0, Data, 0))
	FAIL (FPCloseFork(Conn,fork))

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_RD );
	if (!fork) {
		failed();
		goto fin;
	}
	FAIL (ntohl(AFPERR_ACCESS) != FPWrite(Conn, fork, 0, 30, Data,0))

	FAIL (FPRead(Conn, fork, 0, 0, Data))
	FAIL (FPCloseFork(Conn,fork))

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR|OPENACC_RD );
	if (!fork) {
		failed();
		goto fin;
	}
	FAIL (FPWrite(Conn, fork, 0, 300, Data, 0))
	FAIL (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 400, Data))
	FAIL (FPWrite(Conn, fork, 0, size -1000, Data, 0))
	FAIL (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, size, Data) )

	FPCloseFork(Conn,fork);
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test61");
}

extern char *Server;
extern int  Port;
extern char *Password;
extern char *vers;
extern char *uam;
extern int  Proto;

static volatile int sigp = 0;
static int sock = -1;

static void alarm_handler()
{
	sigp = 1;
	if (sock != -1)
		close(sock);
}

/* -------------------------- */
static void write_test(int size)
{
int fork = 0, fork1 = 0;
uint16_t bitmap = 0;
char *name = "t309 FPRead,FPWrite deadlock";
char *name1 = "t309 second file";
uint16_t vol = VolID;
uint16_t vol2;
int ret;
DSI *dsi;
DSI *dsi2;
int offset;
int quantum;
struct sigaction action;
struct itimerval    it;
CONN *myconn;

	dsi = &Conn->dsi;
	sock = -1;
	sigp = 0;

	quantum = min(size, dsi->server_quantum);
	if (quantum < size) {
		fprintf(stdout,"\t server quantum (%d) too small\n", quantum);
		nottested();
		return;
	}

   	it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 15;
    it.it_value.tv_usec = 0;

    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART | SA_ONESHOT;
    if ((sigaction(SIGALRM, &action, NULL) < 0) ||
            (setitimer(ITIMER_REAL, &it, NULL) < 0)) {
		nottested();
		return;
    }

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1)) {
		nottested();
		goto fin;
	}
    if ((myconn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
    	nottested();
    	goto fin;
	}
	myconn->type = Proto;
    dsi2 = &myconn->dsi;
	sock = OpenClientSocket(Server, Port);
    if ( sock < 0) {
    	nottested();
    	goto fin;
    }
    dsi2->protocol = DSI_TCPIP;
	dsi2->socket = sock;
	ret = FPopenLogin(myconn, vers, uam, User, Password);
	if (ret) {
    	nottested();
    	goto fin;
	}
	vol2 = VolID  = FPOpenVol(myconn, Vol);
	if (vol2 == 0xffff) {
    	nottested();
	    goto fin;
	}
	fork = FPOpenFork(myconn, vol2, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR|OPENACC_RD );
	if (!fork) {
		failed();
		goto fin;
	}
	fork1 = FPOpenFork(myconn, vol2, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name1,OPENACC_WR|OPENACC_RD );
	if (!fork1) {
		failed();
		goto fin;
	}
 	FAIL (FPSetForkParam(myconn, fork, (1<<FILPBIT_DFLEN), 10*128*1024))

	offset = 0;
	FAIL (FPReadHeader(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadHeader(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadHeader(dsi2, fork, offset, size, Data))

	FAIL (FPReadFooter(dsi2, fork, 0, size, Data))

	offset += size;
	FAIL (FPReadHeader(dsi2, fork, offset, size, Data))

	offset += size;
	FAIL (FPReadHeader(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadHeader(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadHeader(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadHeader(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadHeader(dsi2, fork, offset, size, Data))

	offset = 0;
	FAIL (FPWriteHeader(dsi2, fork1, offset, size, Data, 0))
	offset += size;
	FAIL (FPWriteHeader(dsi2, fork1, offset, size, Data, 0))
	offset += size;
	FAIL (FPWriteHeader(dsi2, fork1, offset, size, Data, 0))
	offset += size;
	FAIL (FPWriteHeader(dsi2, fork1, offset, size, Data, 0))

	offset = 0;
	offset += size;
	FAIL (FPReadFooter(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadFooter(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadFooter(dsi2, fork, offset, size, Data))

	offset += size;
	FAIL (FPReadFooter(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadFooter(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadFooter(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadFooter(dsi2, fork, offset, size, Data))
	offset += size;
	FAIL (FPReadFooter(dsi2, fork, offset, size, Data))

	offset = 0;
	FAIL (FPWriteFooter(dsi2, fork1, offset, size, Data, 0))
	offset += size;
	FAIL (FPWriteFooter(dsi2, fork1, offset, size, Data, 0))
	offset += size;
	FAIL (FPWriteFooter(dsi2, fork1, offset, size, Data, 0))
	offset += size;
	FAIL (FPWriteFooter(dsi2, fork1, offset, size, Data, 0))


fin:
	if (sigp ) {
		fprintf(stdout,"\tFAILED deadlock\n");
		failed_nomsg();
		sleep(5);
	}
	else {
		if (fork1) FPCloseFork(myconn,fork1);
		if (fork) FPCloseFork(myconn,fork);
	}
	free(myconn);
	myconn = NULL;

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 0;
   	setitimer(ITIMER_REAL, &it, NULL);
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    if ((sigaction(SIGALRM, &action, NULL) < 0)) {
		nottested();
    }
	sleep(1);
}

/* -------------------------- */
STATIC void test309()
{
	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPRread:test309: FPRead, FPWrite deadlock\n");
    write_test(1024);
	exit_test("test309");
}

/* -------------------------- */
STATIC void test327()
{
	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPRread:test327: FPRead, FPWrite deadlock\n");
    write_test(	128*1024);
	exit_test("test327");
}

/* ------------------------- */
void test328()
{
char *ndir;
int dir;
int i;
int fork;
char *data;
int nowrite;
int numread = /*2*/ 469;
uint16_t vol = VolID;
static char temp[MAXPATHLEN];
int	size;
DSI *dsi;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"test328: read speed\n");
	sprintf(temp,"test328 dir");

	if (get_vol_free(vol) < 17*1024*1024) {
		test_skipped(T_VOL_SMALL);
		goto test_exit;
	}

	ndir = strdup(temp);
	size = min(65536, dsi->server_quantum);
	data = calloc(1, size);
	if (ntohl(AFPERR_NOOBJ) != is_there(Conn, DIRDID_ROOT, ndir)) {
		nottested();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, "", 0
	         , (1<< DIRPBIT_DID) )) {
		nottested();
		goto fin;
	}
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		nottested();
		goto fin;
	}

	if (ntohl(AFPERR_NOOBJ) != is_there(Conn, dir, "File.big")) {
		failed();
		goto fin1;
	}
	if (FPGetFileDirParams(Conn, vol,  dir, "", 0
	         , (1<< DIRPBIT_DID) )) {
		failed();
		goto fin1;
	}
	if (FPCreateFile(Conn, vol,  0, dir , "File.big")){
		failed();
		goto fin1;
	}
	/* --------------- */
	strcpy(temp, "File.big");
	if (is_there(Conn, dir, temp)) {
		failed();
		goto fin1;
	}
	if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x72d,0)) {
		failed();
		goto fin1;
	}
	if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x73f, 0x133f )) {
		failed();
		goto fin1;
	}
	nowrite = 0;
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA ,
			            (1<<FILPBIT_PDID)|(1<< DIRPBIT_LNAME)|(1<<FILPBIT_FNUM)|(1<<FILPBIT_DFLEN)
			            , dir, temp, OPENACC_WR |OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
	if (!fork) {
		failed();
		goto fin1;
	}
	else {
		if (FPGetForkParam(Conn, fork, (1<<FILPBIT_PDID)|(1<< DIRPBIT_LNAME)|(1<<FILPBIT_DFLEN))) {
			failed();
			goto fin1;
		}
		for (i=0; i <= numread ; i++) {
			if (FPWrite(Conn, fork, i*size, size, data, 0 )) {
				nottested();
				nowrite = 1;
				break;
			}
		}
		if (FPCloseFork(Conn,fork)) {
			failed();
			goto fin1;
		}
	}

	if (is_there(Conn, dir, temp)) {fatal_failed();}
	if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x72d, 0)) {fatal_failed();}
	if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x73f, 0x133f )) {
		failed();
		goto fin1;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0x342 , dir, temp,OPENACC_RD);

	if (!fork) {
		failed();
		goto fin1;
	}
	else {
		if (FPGetForkParam(Conn, fork, (1<<FILPBIT_DFLEN))) {
			failed();
			goto fin1;
		}
		if (FPRead(Conn, fork, 0, 512, data)) {
			failed();
			goto fin1;
		}
		if (FPCloseFork(Conn,fork)) {
			failed();
			goto fin1;
		}
	}
	if (!nowrite) {
		fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0x342 , dir, temp,OPENACC_RD| OPENACC_DWR);
		if (!fork) {
			failed();
			goto fin1;
		}
		else {
			if (FPGetForkParam(Conn, fork, 0x242)) {
				failed();
				goto fin1;
			}
			if (FPGetFileDirParams(Conn, vol,  dir, temp, 0x72d,0)) {
				failed();
				goto fin1;
			}
			for (i=0; i <= numread ; i++) {
				if (FPRead(Conn, fork, i*size, size, data)) {
					failed();
					goto fin1;
				}
			}
			if (FPCloseFork(Conn,fork)) {
				failed();
				goto fin1;
			}
		}
	}
fin1:
	if (FPDelete(Conn, vol,  dir, "File.big")) {
		failed();
	}
	if (FPDelete(Conn, vol,  dir, "")) {
		failed();
	}
fin:
	free(ndir);
	free(data);
test_exit:
	exit_test("test328");
}

/* ------------------------- */
STATIC void test343()
{
uint16_t bitmap = 0;
int fork;
char *name = "t343 file.txt";
uint16_t vol = VolID;
int size;
DSI *dsi;

	dsi = &Conn->dsi;
	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPRead:test343: read/write data fork (small size)\n");
	size = min(4096, dsi->server_quantum);
	if (size < 2000) {
		nottested();
		fprintf(stdout,"\t server quantum (%d) too small\n", size);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);

	if (!fork) {
		failed();
		goto fin;
	}

	if (FPWrite(Conn, fork, 0, size, Data, 0 )) {
		failed();
		goto fin1;
	}

	if (FPRead(Conn, fork, 0, size, Data)) {
		failed();
		goto fin1;
	}
	FAIL (FPFlush(Conn, vol))

fin1:
	FAIL (FPCloseFork(Conn,fork))

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("test343");
}

/* ------------------------- */
STATIC void test344()
{
uint16_t bitmap = 0;
int fork = 0;
int fork1;
char *name = "t344 file.txt";
uint16_t vol = VolID;
int size;
int offset;
DSI *dsi;
int ret;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPread:test344: read after EOF\n");
	size = 100;
	offset = 128;

	if (Locking) {
		test_skipped(T_LOCKING);
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

	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), size)) {
		failed();
		goto fin;
	}

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, offset, 10, Data)) {
		failed();
		goto fin;
	}
	if (FPByteLock(Conn, fork, 0, 0 /* set */, 0, 200)) {
		failed();
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork1) {
		failed();
		goto fin;
	}
	ret= FPRead(Conn, fork1, offset, 10, Data);
	if (not_valid(ret, /* Mac */ AFPERR_EOF, AFPERR_LOCK)) {
		failed();
	}

	FPCloseFork(Conn,fork1);
fin:
	if (fork) FPCloseFork(Conn,fork);

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("test344");
}

/* ------------------------- */
STATIC void test8()
{
    uint16_t bitmap = 0;
    int dfork = 0, rfork = 0;
    char *name1 = "t8 file1.txt";
    char *name2 = "t8 file2.txt";
    uint16_t vol = VolID;
    int rsize = 65000, dsize = 12000;
    DSI *dsi;
    int ret;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPRead:test8: open data and rfork, datafork size is 0, rfork is 65k, read from datafork after EOF\n");

	if (Locking) {
		test_skipped(T_LOCKING);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT, name1)) {
		failed();
		goto fin;
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT, name2)) {
		failed();
		goto fin;
	}

    /*****/

	rfork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap ,DIRDID_ROOT, name1, OPENACC_RD|OPENACC_WR);
	if (!rfork) {
		failed();
		goto fin;
	}
	if (FPSetForkParam(Conn, rfork, (1<<FILPBIT_RFLEN), rsize)) {
		failed();
		goto fin;
	}
    if (FPCloseFork(Conn, rfork)) {
		failed();
		goto fin;
    }

    /*****/

	dfork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap ,DIRDID_ROOT, name2, OPENACC_RD|OPENACC_WR);
	if (!dfork) {
		failed();
		goto fin;
	}
	if (FPSetForkParam(Conn, dfork, (1<<FILPBIT_DFLEN), dsize)) {
		failed();
		goto fin;
	}
    if (FPCloseFork(Conn, dfork)) {
		failed();
		goto fin;
    }

    /******/

	rfork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap ,DIRDID_ROOT, name1, OPENACC_RD);
	if (!rfork) {
		failed();
		goto fin;
	}

	dfork = FPOpenFork(Conn, vol, OPENFORK_DATA, bitmap ,DIRDID_ROOT, name2, OPENACC_RD);
	if (!dfork) {
		failed();
		goto fin;
	}

	if (ntohl(AFP_OK) != FPRead(Conn, rfork, 0, rsize, Data)) {
		failed();
		goto fin;
	}

	if (ntohl(AFP_OK) != FPRead(Conn, dfork, 0, dsize, Data)) {
		failed();
		goto fin;
	}

fin:
	if (dfork) FPCloseFork(Conn, dfork);
	if (rfork) FPCloseFork(Conn, rfork);

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name2))
test_exit:
	exit_test("test8");
}


/* ----------- */
void FPRead_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPRead page 238\n");
	test5();
	test8();
	test46();
	test59();
	test61();
	test309();
	test327();
	test328();
	test343();
	test344();
}
