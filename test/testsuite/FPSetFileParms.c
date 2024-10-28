/* ----------------------------------------------
*/
#include "specs.h"

STATIC void test83()
{
char *name = "t83 test file setfilparam";
char *name1 = "t83 test enoent file";
char *ndir = "t83 dir";
int  dir;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)| (1<<FILPBIT_CDATE) |
					(1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE);
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (!(dir =FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		test_nottested();
		goto test_exit;
	}


	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
 		FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir))
 		FAIL (htonl(AFPERR_NOOBJ) != FPSetFileParams(Conn, vol, DIRDID_ROOT , name1, bitmap, &filedir))
		FAIL (htonl(AFPERR_BADTYPE) != FPSetFileParams(Conn, vol, DIRDID_ROOT , ndir, bitmap, &filedir))
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileParms:test83: test set file setfilparam");
}

/* ------------------------- */
STATIC void test96()
{
char *name = "t96 invisible file";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_MDATE);
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto test_exit;
	}

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "", 0,bitmap )) {
		test_failed();
		goto end;
	}
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	if (!Quiet) {
		fprintf(stdout,"Modif date parent %x\n", filedir.mdate);
	}
	sleep(4);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name,bitmap, 0 )) {
		test_failed();
		goto end;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	if (!Quiet) {
		fprintf(stdout,"Modif date file %x\n", filedir.mdate);
	}
	sleep(5);

	filedir.attr = ATTRBIT_INVISIBLE | ATTRBIT_SETCLR ;
	bitmap = (1<<DIRPBIT_ATTR);
 	FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir))

	bitmap = (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_MDATE);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0 )) {
		test_failed();
		goto end;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	if (!Quiet) {
		fprintf(stdout,"Modif date file %x\n", filedir.mdate);
	}

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "", 0,bitmap )) {
		test_failed();
		goto end;
	}
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	if (!Quiet) {
		fprintf(stdout,"Modif date parent %x\n", filedir.mdate);
	}
end:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPSetFileParms:test96: test file's invisible bit");
}

/* ------------------------- */
STATIC void test118()
{
char *name = "t118 no delete file";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<FILPBIT_ATTR);
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto test_exit;
	}
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0 )) {
		test_nottested();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
 		FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir))
		FAIL (ntohl(AFPERR_OLOCK) != FPDelete(Conn, vol,  DIRDID_ROOT , name))
		filedir.attr = ATTRBIT_NODELETE;
 		FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir))
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPSetFileParms:test118: test file no delete attribute");
}

/* ------------------------- */
STATIC void test122()
{
char *name = "t122 setfilparam open fork";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
int fork;
int fork1;
int ret;
int type = OPENFORK_RSCS;

uint16_t bitmap =  (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)| (1<<FILPBIT_CDATE) |
					(1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE);
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	memset(&filedir, 0, sizeof(filedir));
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto test_exit;
	}
	fork = FPOpenFork(Conn, vol, type , 0 ,DIRDID_ROOT, name, OPENACC_WR |OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
	if (!fork) {
		test_nottested();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		/* wrong attrib (open fork set ) */
 		ret = FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir);
		if (not_valid(ret, /* MAC */AFPERR_PARAM, 0)) {
			test_failed();
		}
		bitmap =  (1<<FILPBIT_FINFO)| (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE);
 		FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir))

 		FAIL (htonl(AFPERR_BITMAP) != FPSetFileParams(Conn, vol, DIRDID_ROOT , name, 0xffff, &filedir))
	}
	fork1 = FPOpenFork(Conn, vol, type , 0 ,DIRDID_ROOT, name, OPENACC_RD);
	if (fork1) {
		FAIL (FPCloseFork(Conn,fork1))
		test_failed();
	}

	FPCloseFork(Conn,fork);
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPSetFileParms:test122: test setfilparam open fork");
}

/* ------------------------- */
STATIC void test318()
{
char *name = "t318 PDinfo error";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<FILPBIT_PDINFO );
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
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0 )) {
		test_nottested();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		FAIL (htonl(AFPERR_BITMAP) != FPSetFileParams(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir))
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPSetFileParms:test318: set UTF8 name(error)");
}

/* ------------------------ */
static int afp_symlink(char *oldpath, char *newpath)
{
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap;
uint16_t vol = VolID;
DSI *dsi;
int fork = 0;

	dsi = &Conn->dsi;

    if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , newpath)) {
	    return -1;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0 ,DIRDID_ROOT, newpath , OPENACC_WR | OPENACC_RD);
	if (!fork) {
	    return -1;
	}

	if (FPWrite(Conn, fork, 0, strlen(oldpath), oldpath, 0 )) {
	    return -1;
	}

    if (FPCloseFork(Conn,fork)) {
	    return -1;
    }
    fork = 0;

	bitmap = (1<< DIRPBIT_ATTR) |  (1<<FILPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) |  (1<<DIRPBIT_MDATE) |
		     (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<<FILPBIT_FNUM );

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , newpath, bitmap,0 )) {
	    return -1;
    }

    filedir.isdir = 0;
    afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
    memcpy(filedir.finder_info, "slnkrhap", 8);
	bitmap = (1<<FILPBIT_FINFO);

    if (FPSetFileParams(Conn, vol, DIRDID_ROOT , newpath, bitmap, &filedir))
        return -1;
    return 0;
}

/* ------------------------- */
STATIC void test427()
{
char *name = "t427 Symlink";
char *dest = "t427 dest";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap;
uint16_t vol = VolID;
DSI *dsi;
int fork = 0;

	dsi = &Conn->dsi;

	ENTER_TEST

    if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , dest)) {
		test_nottested();
		goto test_exit;
	}

    if (afp_symlink(dest, name)) {
		test_nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0 ,DIRDID_ROOT, name , OPENACC_RD);
	if (!fork) {
	    /* Trying to open the linked file? */
		test_failed();
	}

test_exit:
    if (fork) {
	    FPCloseFork(Conn,fork);
    }
    FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , dest))
    FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	exit_test("FPSetFileParms:test427: Create a symlink");
}

/* ------------------------- */
STATIC void test428()
{
char *name = "t428 Symlink";
char *name2 = "t428 Symlink2";
char *dest = "t428 dest";
uint16_t vol = VolID;
uint16_t vol2 = 0xffff;

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		test_nottested();
		goto test_exit;
	}

    if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , dest)) {
		test_nottested();
		goto test_error;
	}

    if (afp_symlink(dest, name)) {
		test_nottested();
		goto test_error;
	}
    if (afp_symlink(dest, name2)) {
		test_nottested();
		goto test_error;
	}
    FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
    FAIL (FPDelete(Conn2, vol2,  DIRDID_ROOT , name2))

test_error:
	if (vol2 != 0xffff)
	    FPCloseVol(Conn2,vol2);
    FPDelete(Conn, vol,  DIRDID_ROOT , dest);
    FPDelete(Conn, vol,  DIRDID_ROOT , name);
    FPDelete(Conn, vol,  DIRDID_ROOT , name2);

test_exit:
	exit_test("FPSetFileParms:test428: Delete symlinks, two users");
}

/* ------------------------- */
STATIC void test429()
{
char *name = "t429 Symlink";
char *dest = "t429 dest";
int  ofs =  sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<FILPBIT_FNUM );
uint16_t vol = VolID;
DSI *dsi;
int fork = 0;
int id;

	dsi = &Conn->dsi;

	ENTER_TEST

    if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , dest)) {
		test_nottested();
		goto test_exit;
	}

    if (afp_symlink(dest, name)) {
		test_nottested();
		goto test_exit;
	}

	id = get_fid(Conn, vol, DIRDID_ROOT , name);

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0 ,DIRDID_ROOT, name , OPENACC_RD);
	if (!fork) {
	    /* Trying to open the linked file? */
		test_failed();
		goto test_exit;
	}

	filedir.did = 0;
	if (FPGetForkParam(Conn, fork, bitmap)) {
		test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		if (!filedir.did || filedir.did != id) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED cnids are not the same %x %x\n", filedir.did, id);
			}
			test_failed();
		}
	}

test_exit:
    if (fork) {
	    FPCloseFork(Conn,fork);
    }
    FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , dest))
    FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	exit_test("FPSetFileParms:test429: symlink File ID");
}

/* ------------------------- */
STATIC void test430()
{
    char *name = "t430 Symlink";
    char *dest = "t430 dest";
    int  ofs =  sizeof( uint16_t );
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1<<FILPBIT_FNUM );
    uint16_t vol = VolID;
    DSI *dsi;
    int fork = 0;
    int id;

	dsi = &Conn->dsi;

	ENTER_TEST

    if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , dest)) {
		test_nottested();
		goto test_exit;
	}

    if (afp_symlink(dest, name)) {
		test_nottested();
		goto test_exit;
	}

    FAIL (FPSetFileParams(Conn, vol, DIRDID_ROOT, name, 1<<FILPBIT_CDATE, &filedir))

test_exit:
    FAIL (FPDelete(Conn, vol, DIRDID_ROOT, dest))
    FAIL (FPDelete(Conn, vol, DIRDID_ROOT, name))
	exit_test("FPSetFileParms:test430: set creation date on symlink");
}


/* ----------- */
void FPSetFileParms_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
    test83();
    test96();
    test118();
    test122();
    test318();
    test427();
    test428();
    test429();
    test430();
}
