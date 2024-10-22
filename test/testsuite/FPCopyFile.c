/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test71()
{
int fork;
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "t71 Copy file";
char *name1 = "t71 new file name";
char *name2 = "t71 dir";
char *ndir = "t71 no access";
int pdir = 0;

uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!(pdir = no_access_folder(vol, DIRDID_ROOT, ndir))) {
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name2))) {
		nottested();
		goto test_exit;
	}

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name2))

	/* sdid bad */
	FAIL (ntohl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, dir, vol, DIRDID_ROOT, name, "", name1))

	FPCloseVol(Conn,vol);
	vol  = FPOpenVol(Conn, Vol);
	/* cname unchdirable */
	FAIL (ntohl(AFPERR_BADTYPE) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, ndir, "", name1))
	/* second time once bar is in the cache */
	FAIL (ntohl(AFPERR_BADTYPE) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, ndir, "", name1))

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		failed();
		goto fin;
	}

	FAIL (!(dir1 = FPCreateDir(Conn,vol, DIRDID_ROOT , name2)))

	/* source is a dir */
	FAIL (ntohl(AFPERR_BADTYPE) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name2, "", name1))

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (fork) {
		FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1))
		FAIL (FPCloseFork(Conn,fork))
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))
	}
	else {
		failed();
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD | OPENACC_DRD | OPENACC_DWR);
	if (fork) {
		FAIL (ntohl(AFPERR_DENYCONF) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1))
		FAIL (FPCloseFork(Conn,fork))
	}
	else {
		failed();
	}


	/* dvol bad */
	FAIL (ntohl(AFPERR_PARAM) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol +1, dir, name, "", name1))

	/* ddid bad */
	FAIL (ntohl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT , vol, dir,  name, "", name1))

	/* ok */
	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))
fin:
	delete_folder(vol, DIRDID_ROOT, ndir);
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name2))
test_exit:
	exit_test("FPCopyFile:test71: Copy file");
}

/* ------------------------- */
STATIC void test158()
{
char *name  = "t158 old file name";
char *name1 = "t158 new file name";
uint16_t vol = VolID;

	ENTER_TEST

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1)) {
		nottested();
	}

	/* sdid bad */
	FAIL (ntohl(AFPERR_EXIST) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))

	exit_test("FPCopyFile:test158: copyFile dest exist");
}

/* ------------------------- */
STATIC void test315()
{
uint16_t bitmap = 0;
char *name  = "t315 old file name";
char *name1 = "t315 new file name";
uint16_t vol = VolID;
int fork;

	ENTER_TEST

	if (get_vol_free(vol) < 130*1024*1024) {
	    /* assume sparse file for setforkparam, not for copyfile */
		test_skipped(T_VOL_SMALL);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork) {
		nottested();
		goto fin;
	}

	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 64*1024*1024)) {
		nottested();
		FPCloseFork(Conn,fork);
		goto fin;
	}

	FAIL (FPCloseFork(Conn,fork))

	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork) {
		nottested();
		goto fin;
	}

	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 129*1024*1024)) {
		nottested();
		FPCloseFork(Conn,fork);
		goto fin;
	}

	FAIL (FPCloseFork(Conn,fork))

	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPCopyFile:test315: copyFile");
}

/* ------------------------- */
static void test_meta(char *name, char *name1, uint16_t vol2)
{
uint16_t vol = VolID;
int tp,tp1;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;
uint16_t bitmap;
char finder_info[32];

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto fin;
	}
	tp = get_fid(Conn, vol, DIRDID_ROOT, name);
	if (!tp) {
		nottested();
		goto fin;
	}
	bitmap = (1 << FILPBIT_FINFO);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		memcpy(filedir.finder_info, "PDF CARO", 8);

		memcpy(finder_info, filedir.finder_info, 32);
 		FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir))
	    FAIL (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0))
	}

	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol2, DIRDID_ROOT, name, "", name1))

	if (FPGetFileDirParams(Conn, vol2,  DIRDID_ROOT , name1, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		if (memcmp(finder_info, filedir.finder_info, 32)) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED finder info differ\n");
			}
			failed_nomsg();
			goto fin;
		}
	}

	tp1 = get_fid(Conn, vol2, DIRDID_ROOT, name1);
	if (!tp1) {
		nottested();
		goto fin;
	}
	if (tp == tp1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED both files have same ID\n");
		}
	    failed_nomsg();
	}

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	FAIL (FPDelete(Conn, vol2,  DIRDID_ROOT , name1))
}


/* ------------------------- */
STATIC void test317()
{
char *name  = "t317 old file name";
char *name1 = "t317 new file name";

	ENTER_TEST
    test_meta(name, name1, VolID);

	exit_test("FPCopyFile:test317: copyFile check meta data");
}

/* ------------------------- */
STATIC void test332()
{
char *name  = "t332 old file name";
char *name1 = "t332 new file name";
uint16_t vol = VolID;
int tp,tp1;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;
uint16_t bitmap;
uint32_t mdate = 0;

	ENTER_TEST

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto fin;
	}
	if (!Quiet) {
		fprintf(stdout,"sleep(2)\n");
	}
	sleep(2);
	tp = get_fid(Conn, vol, DIRDID_ROOT, name);
	if (!tp) {
		nottested();
		goto fin;
	}
	bitmap = (1<<DIRPBIT_MDATE);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		mdate = filedir.mdate;
	}

	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1))

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name1, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		if (mdate != filedir.mdate)  {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED modification date differ\n");
			}
			failed_nomsg();
			goto fin;
		}
	}

	tp1 = get_fid(Conn, vol, DIRDID_ROOT, name1);
	if (!tp1) {
		nottested();
		goto fin;
	}
	if (tp == tp1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED both files have same ID\n");
		}
	    failed_nomsg();
	}

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))

	exit_test("FPCopyFile:test332: copyFile check meta data");
}

/* ----------- */
STATIC void test374()
{
int fork;
uint16_t vol2;
uint16_t bitmap = 0;
char *name  = "t374 Copy file";
char *name1 = "t374 new file name";
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (Locking) {
		test_skipped(T_LOCKING);
		goto test_exit;
	}

	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		failed();
		goto fin;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_DRD);
	if (fork) {
		FAIL (ntohl(AFPERR_DENYCONF) != FPCopyFile(Conn2, vol2, DIRDID_ROOT, vol2, DIRDID_ROOT, name, "", name1))
		FAIL (FPCloseFork(Conn,fork))
	}
	else {
		failed();
	}

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FPDelete(Conn, vol,  DIRDID_ROOT, name1);
fin:
	FPCloseVol(Conn2,vol2);

test_exit:
	exit_test("FPCopyFile:test374: Copy open file (deny read), two clients");
}

/* ------------------------- */
STATIC void test375()
{
int fork;
int fork1;
uint16_t bitmap = 0;
char *name  = "t375 old file name";
char *name1 = "t375 new file name";
uint16_t vol = VolID;

	ENTER_TEST

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1)) {
		nottested();
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name1,OPENACC_WR | OPENACC_DWR);
	if (!fork) {
		failed();
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name1,OPENACC_WR);
	if (fork1) {
		failed();
		FAIL (FPCloseFork(Conn,fork1))
	}

	FAIL (ntohl(AFPERR_EXIST) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1))
	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name1,OPENACC_WR);
	if (fork1) {
		failed();
		FAIL (FPCloseFork(Conn,fork1))
	}

	FAIL (FPCloseFork(Conn,fork))

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))

	exit_test("FPCopyFile:test158: copyFile dest exist and is open");
}

/* ------------------------- */
STATIC void test401()
{
int  dir = 0;
char *name = "t401 file.pdf";
char *name1= "new t401 file.pdf";

char *ndir = "t401 dir";
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;
int fork;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
		test_skipped(T_UNIX_PREV);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		nottested();
		goto test_exit;
	}
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);

	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.unix_priv = 0770;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		nottested();
		goto fin;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0, dir, name, OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		goto fin;
	}
	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 400)) {
		FPCloseFork(Conn,fork);
		nottested();
		goto fin;
	}
	FPCloseFork(Conn,fork);

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , 0, dir, name, OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		goto fin;
	}
	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_RFLEN), 300)) {
		FPCloseFork(Conn,fork);
		nottested();
		goto fin;
	}
	FPCloseFork(Conn,fork);

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR) | (1<<FILPBIT_ATTR);

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    failed();
	    goto fin1;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);

	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.unix_priv = 0444;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR) | (1<<FILPBIT_ATTR);
	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    failed();
	    goto fin1;
	}
	FAIL (FPCopyFile(Conn, vol, dir, vol, dir, name, "", name1))

	FAIL (FPDelete(Conn, vol,  dir , name1))

fin1:
	FAIL (FPDelete(Conn, vol,  dir , name))
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPCopyFile:test401: unix access privilege, read only file");
}

/* ------------------------- */
STATIC void test402()
{
int  dir = 0;
char *name = "t402 file.pdf";
char *name1= "new t402 file.pdf";

char *ndir = "t402 dir";
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;
int fork;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
		test_skipped(T_UNIX_PREV);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		nottested();
		goto fin;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0, dir, name, OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		goto fin;
	}
	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 400)) {
		FPCloseFork(Conn,fork);
		nottested();
		goto fin;
	}
	FPCloseFork(Conn,fork);

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , 0, dir, name, OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		goto fin;
	}
	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_RFLEN), 300)) {
		FPCloseFork(Conn,fork);
		nottested();
		goto fin;
	}
	FPCloseFork(Conn,fork);

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR) | (1<<FILPBIT_ATTR);

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    failed();
	    goto fin1;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);

	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.unix_priv = 0222;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR) | (1<<FILPBIT_ATTR);
	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    failed();
	    goto fin1;
	}
	FAIL (ntohl(AFPERR_DENYCONF) != FPCopyFile(Conn, vol, dir, vol, dir, name, "", name1))

fin1:
	FAIL (FPDelete(Conn, vol,  dir , name))
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPCopyFile:test402: unix access privilege");
}

/* ------------------------- */
STATIC void test403()
{
int  dir = 0;
char *name = "t403 file.pdf";
char *name1= "new t403 file.pdf";

char *ndir = "t403 dir";
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
		test_skipped(T_UNIX_PREV);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		nottested();
		goto test_exit;
	}
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);

	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.unix_priv = 0770;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		nottested();
		goto fin;
	}

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR) | (1<<FILPBIT_ATTR);

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    failed();
	    goto fin1;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);

	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.unix_priv = 0444;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR) | (1<<FILPBIT_ATTR);
	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    failed();
	    goto fin1;
	}

	FAIL (FPCopyFile(Conn, vol, dir, vol, dir, name, "", name1))

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR) | (1<<FILPBIT_ATTR);

	if (FPGetFileDirParams(Conn, vol, dir, name1, bitmap, 0)) {
	    failed();
	    goto fin1;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	if ((filedir.unix_priv & 0444) != 0444) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unix priv differ\n");
		}
	    failed_nomsg();
	}

	FAIL (FPDelete(Conn, vol,  dir , name1))

fin1:
	FAIL (FPDelete(Conn, vol,  dir , name))
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPCopyFile:test403: unix access privilege, same priv");
}


/* -------------------------
 * this one is not run by default
*/
STATIC void test406()
{
int  dir = DIRDID_ROOT;
char *name = "Ducky.tif";
char *name1= "new ducky.tif";

uint16_t vol = VolID;
uint16_t bitmap = 0;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) | (1<<FILPBIT_ATTR);

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)){
	    failed();
	    goto test_exit;
	}
	FPCopyFile(Conn, vol, dir, vol, dir, name, "", name1);

test_exit:
	exit_test("FPCopyFile:test406: unix access privilege, read only file");
}

/* ------------------------- */
static void test_data(char *name, char *name1, uint16_t vol2)
{
int  dir = DIRDID_ROOT;
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;
int fork;
DSI *dsi;
char data[20];

	dsi = &Conn->dsi;

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		nottested();
		goto fin;
	}
	/* put something in the data fork */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0, dir, name, OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		goto fin1;
	}
	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 400)) {
		FPCloseFork(Conn,fork);
		nottested();
		goto fin1;
	}
	if (FPWrite(Conn, fork, 0, 9, "Data fork", 0 )) {
		FPCloseFork(Conn,fork);
		nottested();
		goto fin1;
	}
	FPCloseFork(Conn,fork);

	/* put something in the resource fork */
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , 0, dir, name, OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		goto fin;
	}
	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_RFLEN), 300)) {
		FPCloseFork(Conn,fork);
		nottested();
		goto fin;
	}

	if (FPWrite(Conn, fork, 0, 13, "Resource fork", 0 )) {
		FPCloseFork(Conn,fork);
		nottested();
		goto fin1;
	}
	FPCloseFork(Conn,fork);

	/* *************** */

	FAIL (FPCopyFile(Conn, vol, dir, vol2, dir, name, "", name1))

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) | (1<<FILPBIT_ATTR)
	  | (1<<FILPBIT_RFLEN) | (1<<FILPBIT_DFLEN);

	if (FPGetFileDirParams(Conn, vol2, dir, name1, bitmap, 0)){
	    failed();
	    goto fin2;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	if (filedir.dflen != 400 || filedir.rflen != 300 ) {
		if (!Quiet) {
			fprintf(stdout, "\tFAILED after copy wrong size (data %d, resource %d) \n",
				filedir.dflen, filedir.rflen);
		}
		failed_nomsg();
		goto fin2;
	}

	/* check data fork */
	memset(data, 0, sizeof(data));
	fork = FPOpenFork(Conn, vol2, OPENFORK_DATA , 0 ,DIRDID_ROOT, name1, OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin2;
	}

	if (FPRead(Conn, fork, 0, 9, data)) {
		failed();
		goto fin2;
	}
	FPCloseFork(Conn,fork);

	if (memcmp(data, "Data fork", 9)) {
		if (!Quiet) {
			fprintf(stdout, "\tFAILED not \"Data fork\" read\n");
		}
		failed_nomsg();
		goto fin2;
	}
	/* check resource fork */
	memset(data, 0, sizeof(data));
	fork = FPOpenFork(Conn, vol2, OPENFORK_RSCS , 0, dir, name1, OPENACC_WR |OPENACC_RD);
	if (!fork) {
		failed();
		goto fin2;
	}

	if (FPRead(Conn, fork, 0, 13, data)) {
		failed();
		goto fin2;
	}
	FPCloseFork(Conn,fork);

	if (memcmp(data, "Resource fork", 13)) {
		if (!Quiet) {
			fprintf(stdout, "\tFAILED not \"Resource fork\" read\n");
		}
		failed_nomsg();
		goto fin2;
	}

	/* other way */
	FAIL (FPDelete(Conn, vol,  dir , name))
	FAIL (FPCopyFile(Conn, vol2, dir, vol, dir, name1, "", name))

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)){
	    failed();
	    goto fin2;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	if (filedir.dflen != 400 || filedir.rflen != 300 ) {
		if (!Quiet) {
			fprintf(stdout, "\tFAILED after copy wrong size\n");
		}
		failed_nomsg();
		goto fin2;
	}


	/* check data fork */
	memset(data, 0, sizeof(data));
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0 ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin2;
	}

	if (FPRead(Conn, fork, 0, 9, data)) {
		failed();
		goto fin2;
	}
	FPCloseFork(Conn,fork);

	if (memcmp(data, "Data fork", 9)) {
		if (!Quiet) {
			fprintf(stdout, "\tFAILED not \"Data fork\" read\n");
		}
		failed_nomsg();
		goto fin2;
	}
	/* check resource fork */
	memset(data, 0, sizeof(data));
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , 0, dir, name, OPENACC_WR |OPENACC_RD);
	if (!fork) {
		failed();
		goto fin2;
	}

	if (FPRead(Conn, fork, 0, 13, data)) {
		failed();
		goto fin2;
	}
	FPCloseFork(Conn,fork);

	if (memcmp(data, "Resource fork", 13)) {
		if (!Quiet) {
			fprintf(stdout, "\tFAILED not \"Resource fork\" read\n");
		}
		failed_nomsg();
		goto fin2;
	}

fin2:
	FAIL (FPDelete(Conn, vol2,  dir , name1))
fin1:
	FAIL (FPDelete(Conn, vol,  dir , name))
fin:
	return;
}

/* ---------------------- */
STATIC void test407()
{
char *name = "t407 file.pdf";
char *name1= "new t407 file.pdf";
uint16_t vol2;

	ENTER_TEST
	if (!*Vol2) {
		test_skipped(T_VOL2);
		goto test_exit;
	}
	vol2  = FPOpenVol(Conn, Vol2);
	if (vol2 == 0xffff) {
		nottested();
		goto test_exit;
	}
    test_data(name, name1, vol2);
	FAIL (FPCloseVol(Conn, vol2))

test_exit:
	exit_test("FPCopyFile:test407: copy file between two volumes");
}

/* ------------------------- */
STATIC void test408()
{
char *name  = "t408 old file name";
char *name1 = "t408 new file name";
uint16_t vol2;

	ENTER_TEST
	if (!*Vol2) {
		test_skipped(T_VOL2);
		goto test_exit;
	}
	vol2  = FPOpenVol(Conn, Vol2);
	if (vol2 == 0xffff) {
		nottested();
		goto test_exit;
	}

    test_meta(name, name1, vol2);

	FAIL (FPCloseVol(Conn, vol2))
test_exit:
	exit_test("FPCopyFile:test408: copyFile check meta data, two volumes");
}


/* ------------------------- */
STATIC void test409()
{
char *name  = "t409 old file name";
char *name1 = "t409 new file name";

	ENTER_TEST

    test_data(name, name1, VolID);

	exit_test("FPCopyFile:test409: copyFile check meta data, one volume");
}

/* ------------------------- */
STATIC void test416()
{
uint16_t vol = VolID;
uint16_t bitmap;
DSI *dsi;
char *file="test416_file";
char *file1="test416 new file";
char *attr_name="test416_attribute";

    dsi = &Conn->dsi;

	ENTER_TEST

    if (Conn->afp_version < 32) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_EXTATTRS)) {
        test_skipped(T_UTF8);
        goto test_exit;
    }

    bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);

    if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , file)) {
    	nottested();
        goto test_exit;
    }

	FAIL(FPSetExtAttr(Conn,vol, DIRDID_ROOT, 2, file, attr_name, "test416_data"))
	FAIL(FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file, attr_name))

	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, file, "", file1))

	FAIL(FPListExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file))

	FAIL(FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file, attr_name))
	FAIL(FPGetExtAttr(Conn,vol, DIRDID_ROOT , 0, 4096, file1, attr_name))

	FAIL(FPRemoveExtAttr(Conn,vol, DIRDID_ROOT , 0, file1, attr_name))

    FAIL(FPDelete(Conn, vol,  DIRDID_ROOT , file))
    FAIL(FPDelete(Conn, vol,  DIRDID_ROOT , file1))

test_exit:
	exit_test("FPCopyFile:test416: copy xattr");
}

/* ------------------------- */
STATIC void test414()
{
int dir;
char *name  = "t414 old file name";
char *name1 = "t414 new file name";
char *name2 = "t414 dir";
uint16_t vol = VolID;

	ENTER_TEST

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	/* wrong destdir */
	FAIL (ntohl(AFPERR_BADTYPE) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, name, name1))

	FAIL (ntohl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, name2, name1))

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name2))) {
		nottested();
		goto test_exit;
	}

	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, name2, name1))
	FAIL (FPDelete(Conn, vol,  dir , name1))

test_exit:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name2))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))

	exit_test("FPCopyFile:test414: copyFile with bad dest directory");
}

/* ------------------------- */
STATIC void test424()
{
int dir;
char *name  = "t424 Copy file";
char *name1 = "t424 new file name";
char *name2 = "t424 dir";
char *ndir = "t424 no access";
int pdir = 0;

uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!(pdir = no_access_folder(vol, DIRDID_ROOT, ndir))) {
		goto test_exit;
	}

	FPCloseVol(Conn,vol);
	vol  = FPOpenVol(Conn, Vol);

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		failed();
		goto fin;
	}

	/* sdid bad */
	FAIL (ntohl(AFPERR_ACCESS) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, ndir, name1))

	FAIL (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name2)))

	/* ok */
	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, name2, name1))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  dir, name1))
fin:
	delete_folder(vol, DIRDID_ROOT, ndir);
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name2))
test_exit:
	exit_test("FPCopyFile:test424: Copy file with dest directory");
}


/* ----------- */
void FPCopyFile_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPCopyFile page 131\n");
    fprintf(stdout,"-------------------\n");
    test71();
	test158();
	test315();
	test317();
	test332();
	test374();
	test375();
	test401();
	test402();
	test403();
	test407();
	test408();
	test409();
	test416();
	test414();
	test424();
}
