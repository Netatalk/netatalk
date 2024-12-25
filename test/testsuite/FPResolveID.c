/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test76()
{
int  dir;
char *name = "t76 Resolve ID file";
char *name1 = "t76 Resolve ID dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) {
		test_skipped(T_ID);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		test_nottested();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	FAIL (FPCreateID(Conn,vol, dir, name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
		FAIL (htonl(AFPERR_BITMAP) != FPResolveID(Conn, vol, filedir.did, 0xffff))
	}

	FAIL (FPDelete(Conn, vol,  dir , name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))
test_exit:
	exit_test("FPResolveID:test76: Resolve ID");
}

/* -------------------------
*/
STATIC void test91()
{
int  dir;
int  dir1;
char *name = "t91 test ID file";
char *name1 = "t91 test ID dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;
unsigned int ret;
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		test_skipped(T_ID);
		goto test_exit;
	}

	if (!(dir1 = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		test_nottested();
		goto test_exit;
	}

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))

	ret = FPDeleteID(Conn, vol,  dir1);
	if (htonl(AFPERR_NOID) != ret) {
		test_failed();
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		test_failed();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	FAIL (htonl(AFPERR_NOID) != FPResolveID(Conn, vol, dir1, bitmap))

	ret = FPResolveID(Conn, vol, dir, bitmap);
	if (not_valid_bitmap(ret, BITERR_BADTYPE | BITERR_NOID, AFPERR_BADTYPE)) {
		test_failed();
	}

//	FAIL (FPCreateID(Conn,vol, dir, name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		test_failed();
	} else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		FAIL (FPDeleteID(Conn, vol, filedir.did))
	}

	ret = FPCreateID(Conn,vol, dir, name);
	if (not_valid(ret, /* MAC */AFPERR_EXISTID, 0)) {
		test_failed();
	}

	FAIL (htonl(AFPERR_EXISTID) != FPCreateID(Conn,vol, dir, name))
	FAIL (FPDelete(Conn, vol,  dir , name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))
test_exit:
	exit_test("FPResolveID:test91: Resolve ID errors");
}

/* -------------------------
*/
STATIC void test310()
{
int  dir;
char *name = "t310 test ID file";
char *name1 = "t310 new name";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		test_skipped(T_ID);
		goto test_exit;
	}
	dir = DIRDID_ROOT;

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	}

	FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
	FAIL (FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1))
	FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))
test_exit:
	exit_test("FPResolveID:test310: Resolve ID after rename");
}

/* -------------------------
*/
STATIC void test311()
{
int  dir;
char *name =  "t311-\xd7\xa4\xd7\xaa\xd7\x99\xd7\x97\xd7\x94#113F.mp3";
char *name1 = "t311-\xd7\xa4\xd7\xaa\xd7\x99\xd7\x97\xd7\x94#11.mp3";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = 0x693f;
struct afp_filedir_parms filedir;
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		test_skipped(T_ID);
		goto test_exit;
	}
	dir = DIRDID_ROOT;

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	}

	FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
	FAIL (FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1))
	FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))
test_exit:
	exit_test("FPResolveID:test311: Resolve ID after rename");
}

/* -------------------------- */
STATIC void test362()
{
int  dir;
uint16_t vol = VolID;
char *name = "t362 Resolve ID file";
char *name1 = "t362 Resolve ID dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;
uint16_t vol2;
DSI *dsi2;

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!(get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		test_skipped(T_ID);
		goto test_exit;
	}

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		test_nottested();
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		test_failed();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		FAIL ((FPResolveID(Conn, vol, filedir.did, bitmap)))
	}
	FAIL (FPDelete(Conn2, vol2,  dir , name))
	FAIL (FPDelete(Conn2, vol2,  dir , ""))

	FAIL (ntohl(AFPERR_NOID ) != FPResolveID(Conn, vol, filedir.did, bitmap))
	FPCloseVol(Conn,vol);
	vol  = FPOpenVol(Conn, Vol);
	FAIL (ntohl(AFPERR_NOID ) != FPResolveID(Conn, vol, filedir.did, bitmap))
test_exit:
	exit_test("FPResolveID:test362: Resolve ID two users interactions");
}

/* -------------------------- */
STATIC void test417()
{
uint16_t vol = VolID;
int  dir;
char *name  = "t417 file";
char *name2 = "t417 file new name";
char *name1 = "t417 dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM ) | (1<<DIRPBIT_FINFO);
struct afp_filedir_parms filedir;
int fid = 0;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		test_failed();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		test_failed();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		fid = filedir.did;
		FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
	}
	FAIL (FPMoveAndRename(Conn, vol, dir, dir, name, name2))

	if (FPGetFileDirParams(Conn, vol,  dir , name2, bitmap,0)) {
		test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		if (fid != filedir.did) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED FPGetFileDirParams id differ %x %x\n", fid, filedir.did);
			}
			test_failed();

		}
		else {
			FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
		}
	}

fin:
	FAIL (FPDelete(Conn, vol,  dir, name2))
	FPDelete(Conn, vol,  dir, name);
	FAIL (FPDelete(Conn, vol,  dir, ""))
test_exit:
	exit_test("FPResolveID:test417: Resolve ID file modified with AFP command");

}


/* ----------- */
void FPResolveID_test()
{
    ENTER_TESTSET
	test76();
	test91();
	test310();
	test311();
	test362();
	test417();
}
