/* ----------------------------------------------
*/
#include "specs.h"
#include "adoublehelper.h"

static char temp[MAXPATHLEN];
static char temp1[MAXPATHLEN];

/* ------------------------- */
STATIC void test146()
{
int fork,fork1;
int  dir;
char *name = "t146 file";
char *name1 = "t146 dir";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap =  (1 << DIRPBIT_ACCESS);
uint16_t vol2;
uint16_t vol = VolID;
DSI *dsi = &Conn->dsi;
DSI *dsi2;
int ret;

	ENTER_TEST

	// FIXME: broken in Netatalk 4.0
	if (Exclude) {
		test_skipped(T_EXCLUDE);
		goto test_exit;
	}
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		nottested();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , "", 0,bitmap )) {
		failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
        filedir.access[0] = 0;
        filedir.access[1] = 3; /* everyone */
        filedir.access[2] = 3; /* group */
        filedir.access[3] = 7; /* owner */
 		FAIL (FPSetDirParms(Conn, vol, dir , "", bitmap, &filedir))
	}

	/* ------------------ */
	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		failed();
	}
	FAIL (FPEnumerate(Conn2, vol2,  DIRDID_ROOT , "", 0, bitmap))
	FAIL (ntohl(AFPERR_ACCESS) != FPDelete(Conn2, vol2,  dir , name))
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0 ,dir , name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		failed();
	}
	ret = FPDelete(Conn2, vol2,  dir , name);
	if (not_valid(ret, /* MAC */AFPERR_ACCESS, AFPERR_BUSY)) {
		failed();
	}

    filedir.access[1] = 3;
    filedir.access[2] = 7;
    filedir.access[3] = 7;
 	FAIL (FPSetDirParms(Conn, vol, dir , "", bitmap, &filedir))
	FAIL (ntohl(AFPERR_BUSY) != FPDelete(Conn2, vol2,  dir , name))
	if (!Mac && adouble == AD_V2) {
		sprintf(temp,"%s/%s/.AppleDouble/%s", Path, name1, name);
		if (chmod(temp, 0644) <0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED chmod(%s) %s\n", temp, strerror(errno));
			}
			failed_nomsg();
		}
	}
	if (ntohl(AFPERR_BUSY) != FPDelete(Conn2, vol2,  dir , name)) {
		if (!Quiet) {
			fprintf(stdout,"\tFIXME FAILED open but deleted\n");
		}
#if 0
		failed_nomsg();
#endif
		FAIL (FPCloseFork(Conn,fork))
		FAIL (FPCreateFile(Conn, vol,  0, dir , name))
		fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0 ,dir , name,OPENACC_WR |OPENACC_RD);
		if (!fork) {
			failed();
		}
	} else {
        if (!Mac && adouble == AD_V2) {
            if (chmod(temp, 0666) <0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED chmod(%s) %s\n", temp, strerror(errno));
		}
                failed_nomsg();
            }
        }
    }

	FAIL (FPCloseFork(Conn,fork))
	fork1 = FPOpenFork(Conn2, vol2, OPENFORK_DATA , 0 ,dir , name,OPENACC_WR |OPENACC_RD);
	if (!fork1) {
		failed();
	}
	FAIL (ntohl(AFPERR_BUSY) != FPDelete(Conn, vol,  dir , name))
	FAIL (FPCloseFork(Conn2,fork1))

	FAIL (FPDelete(Conn, vol,  dir , name))
	FAIL (FPDelete(Conn, vol,  dir , ""))
	FAIL (FPCloseVol(Conn2,vol2))
test_exit:
	exit_test("FPDelete:test146: delete read only open file");
}

/* -------------------------- */
STATIC void test507()
{
int  dir;
uint16_t vol = VolID;
char *name = "t507 Resolve ID file";
char *name1 = "t507 Resolve ID dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	// FIXME: broken in Netatalk 4.0
	if (Exclude) {
		test_skipped(T_EXCLUDE);
		goto test_exit;
	}
	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		failed();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		FAIL ((FPResolveID(Conn, vol, filedir.did, bitmap)))
	}

    delete_unix_file(Path, name1, name);
	delete_unix_dir(Path, name1);

	FAIL (ntohl(AFPERR_NOID ) != FPResolveID(Conn, vol, filedir.did, bitmap))
	FPCloseVol(Conn,vol);
	vol  = FPOpenVol(Conn, Vol);
	FAIL (ntohl(AFPERR_NOID ) != FPResolveID(Conn, vol, filedir.did, bitmap))
test_exit:
	exit_test("FPDelete:test507: Resolve ID in a deleted folder");
}

/* -------------------------- */
STATIC void test363()
{
int  dir;
uint16_t vol = VolID;
char *name = "t363 GetForkParams file";
char *name1 = "t363 GetForkParams dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;
int fork;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		failed();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS  , bitmap ,dir, name,OPENACC_RD);
	if (!fork) {
		nottested();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	}
	if (!Mac) {
		sprintf(temp1, "%s/%s/.AppleDouble/%s", Path, name1, name);

		if (unlink(temp1) <0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unlink %s %s\n", temp, strerror(errno));
			}
			failed_nomsg();
		}
		sprintf(temp1, "%s/%s/%s", Path, name1, name);
		if (unlink(temp1) <0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unlink %s %s\n", temp, strerror(errno));
			}
			failed_nomsg();
		}
		if (delete_unix_dir(Path, name1)) {
			failed();
		}
		getchar();
	}
	if (FPGetForkParam(Conn, fork, bitmap)) {
		failed();
	}
fin:
	FPDelete(Conn, vol,  dir , name);
	FPDelete(Conn, vol,  dir , "");
test_exit:
	exit_test("FPDelete:test363: Get fork param in a deleted folder");

}

/* -------------------------- */
STATIC void test364()
{
int  dir;
uint16_t vol = VolID;
char *name = "t364 Delete ID file";
char *name1 = "t364 Delete ID dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	// FIXME: broken in Netatalk 4.0
	if (Exclude) {
		test_skipped(T_EXCLUDE);
		goto test_exit;
	}
	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		failed();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
		goto fin;
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		FAIL ((FPResolveID(Conn, vol, filedir.did, bitmap)))
	}

	delete_unix_file(Path, name1, name);
	delete_unix_dir(Path, name1);

	FAIL (ntohl(AFPERR_NOOBJ) != FPDeleteID(Conn, vol, filedir.did))

fin:
	FPDelete(Conn, vol,  dir , name);
	FPDelete(Conn, vol,  dir , "");
test_exit:
	exit_test("FPDelete:test364: Delete ID in a deleted folder");

}

/* ----------- */
void T2FPDelete_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPDelete page 143\n");
    fprintf(stdout,"-------------------\n");
    test146();
    test507();
#if 0
    test363();
#endif
    test364();
}
