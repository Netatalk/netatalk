/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test6()
{
int  dir;
char *name = "test6 dir";
uint16_t vol = VolID;

	ENTER_TEST

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		test_failed();
		goto test_exit;
	}

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		test_failed();
		goto test_exit;
	}

	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPCreateDir:test6: create dir");
}

/* ------------------------- */
STATIC void test26()
{
char *name = "test26 dir";
int pdir;
uint16_t vol = VolID;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}


	if (!(pdir = no_access_folder(vol, DIRDID_ROOT, name))) {
		goto test_exit;
	}

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0,
	    (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	    (1 << DIRPBIT_ACCESS))) {
	    test_failed();
	    goto fin;
	}

	FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
	     (1<< FILPBIT_LNAME) | (1<< FILPBIT_FNUM ),
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);

	if (FPGetFileDirParams(Conn, vol, pdir, "", 0,
	     (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	     (1 << DIRPBIT_ACCESS))
	   ) {
		test_failed();
	}
fin:
	delete_folder(vol, DIRDID_ROOT, name);
test_exit:
	exit_test("FPCreateDir:test26: folder without right access");
}

/* ------------------------- */
STATIC void test45()

{
int  dir = 0;
char *ndir = "t45 no access";
char *rodir = "t45 read only access";
int pdir = 0;
int rdir = 0;
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
	if (!(rdir = read_only_folder(vol, DIRDID_ROOT, rodir) ) ) {
		goto fin;
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , "t45 no access/file");
	if (dir || dsi->header.dsi_code != ntohl(AFPERR_ACCESS)) {
		test_failed();
		goto fin;
	}
	if (dir) {
		FAIL (FPDelete(Conn, vol,  dir , ""))
		goto fin;
	}

	dir  = FPCreateDir(Conn,vol, pdir , "t45 file");
	if (dir || dsi->header.dsi_code != ntohl(AFPERR_ACCESS)) {
		test_failed();
		goto fin;
	}
	if (dir) {
		FAIL (FPDelete(Conn, vol,  dir , ""))
		goto fin;
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , rodir);
	if (dir || dsi->header.dsi_code != htonl(AFPERR_EXIST)) {
		test_failed();
		goto fin;
	}
fin:
	delete_folder(vol, DIRDID_ROOT, ndir);
	if (rdir) {
		delete_folder(vol, DIRDID_ROOT, rodir);
	}
test_exit:
	exit_test("FPCreateDir:test45: Folder Creation");
}

/* -------------------------- */
static int create_deleted_folder(uint16_t vol, char *name)
{
uint16_t vol2;
int tdir;
DSI *dsi2;

	tdir  = FPCreateDir(Conn,vol, DIRDID_ROOT, name);
	if (!tdir) {
		test_nottested();
		return 0;
	}

	if (FPGetFileDirParams(Conn, vol,  tdir , "", 0, (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT))) {
		test_nottested();
		return 0;
	}

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (FPDelete(Conn2, vol2, DIRDID_ROOT, name)) {
		test_nottested();
		FAIL (FPDelete(Conn, vol,  tdir , ""))
		FAIL (FPCloseVol(Conn2,vol2))
		return 0;
	}
	FAIL (FPCloseVol(Conn2,vol2))

	return(tdir);
}

/* ----------- */
STATIC void test175()
{
char *tname = "test175";
int tdir,dir;
uint16_t vol = VolID;
DSI *dsi;
unsigned int ret;


	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	dsi = &Conn->dsi;
    /* ---- file.c ---- */
	if (!(tdir = create_deleted_folder(vol, tname))) {
		goto test_exit;
	}

	ret = FPCreateFile(Conn, vol,  0, tdir, tname);
	/* FIXME ? */
	if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_PARAM)) {
		test_failed();
	}
	/* ---- directory.c ---- */
	if (!(tdir = create_deleted_folder(vol, tname))) {
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, tdir, tname);
	ret = dsi->header.dsi_code;
	if (dir || not_valid(ret, AFPERR_NOOBJ, AFPERR_PARAM)) {
		test_failed();
		if (dir) { FPDelete(Conn, vol, dir , "");}
	}
	/* ----------- */
	tname = ".test175";
	if (!(tdir = create_deleted_folder(vol, tname))) {
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, tdir, tname);
	ret = dsi->header.dsi_code;
	if (dir || not_valid(ret, AFPERR_NOOBJ, AFPERR_PARAM)) {
		test_failed();
		if (dir) { FPDelete(Conn, vol, dir , "");}
	}

	/* ----------- */
	tname = ".test175b";
	if (!(tdir = create_deleted_folder(vol, tname))) {
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT, tname);
	if (!dir) {
		test_failed();
	}
	else {
	    FPDelete(Conn, vol, dir , "");
	}
test_exit:
	exit_test("FPCreateDir:test175: did error two users in  folder did=<deleted> name=test175");
}

/* ----------- */
STATIC void test198()
{
char *name = "test198 dir";
char *name1 = "test198 subdir";
char *name2 = "test198 dir/test198 subdir";
int tdir,tdir1 = 0;
uint16_t vol = VolID;
uint16_t bitmap = (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) | (1<<DIRPBIT_UID) |
	    	(1 << DIRPBIT_GID) |(1 << DIRPBIT_ACCESS);
uint16_t vol2;
DSI *dsi2;
unsigned int ret;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	tdir  = FPCreateDir(Conn,vol, DIRDID_ROOT, name);
	if (!tdir) {
		test_nottested();
		goto test_exit;
	}

	tdir1  = FPCreateDir(Conn,vol,tdir, name1);
	if (!tdir1) {
		test_nottested();
		goto fin;
	}
	FAIL (FPGetFileDirParams(Conn, vol,  tdir , "", 0, bitmap))
	FAIL (FPGetFileDirParams(Conn, vol,  tdir1 , "", 0, bitmap))

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		test_nottested();
		goto fin;
	}

	ret = FPDelete(Conn2, vol2,  DIRDID_ROOT , name2);
	if (not_valid(ret, AFPERR_ACCESS, 0)) {
		test_failed();
	}
	if (ret) {
		FAIL (FPDelete(Conn, vol, tdir1 , ""))
	}
	tdir1 = 0;
	if (FPDelete(Conn2, vol2,  DIRDID_ROOT , name)) {
		test_failed();
		FAIL (FPDelete(Conn, vol, tdir , ""))
	}
	tdir = 0;
fin:
	FAIL (tdir && FPDelete(Conn, vol, tdir , ""))
	FAIL (tdir1 && FPDelete(Conn, vol, tdir1 , ""))
test_exit:
	exit_test("FPCreateDir:test198: second user delete folder");
}

/* ----------- */
STATIC void test357()
{
char *name = "test357 dir";
int tdir;
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO)| (1<<DIRPBIT_CDATE) |
					(1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE)| (1<<DIRPBIT_UID) |
	    			(1 << DIRPBIT_GID);

uint16_t vol2;
DSI *dsi;
DSI *dsi2;
unsigned int ret;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	dsi = &Conn->dsi;
	tdir  = FPCreateDir(Conn,vol, DIRDID_ROOT, name);
	if (!tdir) {
		test_nottested();
		goto test_exit;
	}

	FAIL (FPGetFileDirParams(Conn, vol,  tdir , "", 0, bitmap))
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
    filedir.access[0] = 0;
    filedir.access[1] = 0;
    filedir.access[2] = 0;
    filedir.access[3] = 7; /* only owner */
 	FAIL (FPSetDirParms(Conn, vol, tdir , "", (1 << DIRPBIT_ACCESS), &filedir))

	FAIL (FPGetFileDirParams(Conn, vol,  tdir , "", 0, bitmap))
	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		test_nottested();
		FAIL (FPDelete(Conn, vol, tdir , ""))
		goto test_exit;
	}

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
 	FAIL (FPSetDirParms(Conn2, vol2, DIRDID_ROOT , name, bitmap, &filedir))
	if (ntohl(AFPERR_OLOCK) != FPDelete(Conn, vol,  DIRDID_ROOT , name)) {
		test_failed();
		goto test_exit;
	}
	filedir.attr = ATTRBIT_NODELETE;
 	FAIL (FPSetDirParms(Conn2, vol2, DIRDID_ROOT , name, bitmap, &filedir))
	ret = FPDelete(Conn2, vol2,  DIRDID_ROOT , name);
	if (ret) {
		test_failed();
		FAIL (FPDelete(Conn, vol, tdir , ""))
	}
test_exit:
	exit_test("FPCreateDir:test357: admin user");
}

/* ----------- */
void FPCreateDir_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
	test6();
	test26();
	test45();
	test175();
	test198();
}
