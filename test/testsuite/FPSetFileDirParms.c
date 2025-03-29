/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test98()
{
int  dir = 0;
char *name = "t98 error setfildirparams";
char *name1 = "t98 error setfildirparams file";
char *ndir = "t98 no access";
char *rodir = "t98 read only access";
int  ofs =  3 * sizeof( uint16_t );
int pdir = 0;
int rdir = 0;
int ret;
struct afp_filedir_parms filedir;
uint16_t bitmap = (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)
					| (1<<DIRPBIT_UID) | (1 << DIRPBIT_GID) |(1 << DIRPBIT_ACCESS)
					| (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE);
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
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, ndir, 0,
	    (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
		(1<<DIRPBIT_UID) | (1 << DIRPBIT_GID)| (1 << DIRPBIT_ACCESS))) {
		test_failed();
		goto fin;
	}
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, rodir, 0,
	    (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
		(1<<DIRPBIT_UID) | (1 << DIRPBIT_GID)| (1 << DIRPBIT_ACCESS))) {
		test_failed();
		goto fin;
	}

	FAIL (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name)))
	FAIL (FPCreateFile(Conn, vol,  0, dir , name1))

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, 0,bitmap )) {
		test_failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		bitmap = (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE);
 		ret = FPSetFilDirParam(Conn, vol, DIRDID_ROOT , rodir, bitmap, &filedir);
        if (adouble == AD_EA) {
            if (not_valid(ret, /* MAC */0, AFPERR_ACCESS))
                test_failed();
        } else {
            if (ret)
                test_failed();
        }
 		ret = FPSetFilDirParam(Conn, vol, DIRDID_ROOT , ndir, bitmap, &filedir);
		if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
			test_failed();
		}
 		FAIL (FPSetFilDirParam(Conn, vol, dir , name1, bitmap, &filedir))
 		FAIL (ntohl(AFPERR_NOOBJ) != FPSetFilDirParam(Conn, vol, DIRDID_ROOT, name1, bitmap, &filedir))
 		FAIL (FPSetFilDirParam(Conn, vol, DIRDID_ROOT, name, bitmap, &filedir))
	}

fin:
	delete_folder(vol, DIRDID_ROOT, ndir);
	if (rdir) {
		delete_folder(vol, DIRDID_ROOT, rodir);
	}
	FAIL (FPDelete(Conn, vol,  dir , name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPSetFileDirParms:test98: test error setfildirparam");
}

/* ------------------------- */
STATIC void test230()
{
int  dir = 0;
char *name = "t230 file";
char *ndir = "t230 dir";
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;
int fork;

DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST
	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
		test_skipped(T_UNIX_PREV);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		test_nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		test_nottested();
		goto fin;
	}
	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);

	if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
	    test_failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (!(S_ISDIR(filedir.unix_priv))) {
			if (!Quiet) {
				fprintf(stdout, "\tFAILED %o not a dir\n", filedir.unix_priv);
			}
			test_failed();
		}
		bitmap = (1<< DIRPBIT_UNIXPR);
		filedir.unix_priv &= ~S_IWUSR;
        filedir.access[0] = 0;
        filedir.access[1] = filedir.access[2] = filedir.access[3] = 3;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))
 		FAIL (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap))

		filedir.unix_priv &= ~(S_IWUSR |S_IWGRP| S_IWOTH);
        filedir.access[0] = 0;
        filedir.access[1] = filedir.access[2] = filedir.access[3] = 3;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))

 		FAIL (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap))

 		if (!FPDelete(Conn, vol,  dir , name)) {
 		    /* FIXME OSX delete it*/
			if (FPCreateFile(Conn, vol,  0, dir , name)) {
				test_nottested();
			}
 		}

 		/* open fork read write in a read only folder */
		fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0 ,dir, name,OPENACC_WR | OPENACC_RD);
		if (!fork) {
			test_failed();
		}
		else {
			FPCloseFork(Conn, fork);
		}

		fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , 0 ,dir, name,OPENACC_WR | OPENACC_RD);
		if (!fork) {
            if (adouble == AD_V2)
                test_failed();
		}
		else {
			FPCloseFork(Conn, fork);
		}
	}

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR);

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
		bitmap = (1<< DIRPBIT_UNIXPR);
		filedir.unix_priv &= ~S_IWUSR;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))

		fork = FPOpenFork(Conn, vol, OPENFORK_DATA , 0 ,dir, name,OPENACC_WR | OPENACC_RD);
		if (fork) {
			test_failed();
			FPCloseFork(Conn, fork);
		}
		fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , 0 ,dir, name,OPENACC_WR | OPENACC_RD);
		if (fork) {
			test_failed();
			FPCloseFork(Conn, fork);
		}

	}

	/* ----------------- */
	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);
	if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
	    test_failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		bitmap = (1<< DIRPBIT_UNIXPR);
		filedir.unix_priv |= S_IWUSR;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))
	}

	FAIL (FPDelete(Conn, vol,  dir , name))
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test230: set unix access privilege");
}

/* ------------------------- */
STATIC void test231()
{
int  dir = 0;
char *name = "t231 file";
char *name1 = "t231 file user 2";
char *ndir = "t231 dir";
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;
uint16_t vol2;
DSI *dsi2;

DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
		test_skipped(T_UNIX_PREV);
		goto test_exit;
	}
	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		test_nottested();
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		test_nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		test_nottested();
		goto fin;
	}

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR);

	FAIL (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0))

	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);

	if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
	    test_failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		bitmap = (1<< DIRPBIT_UNIXPR);
		filedir.unix_priv |= S_IWUSR |S_IWGRP| S_IRGRP | S_IWOTH | S_IROTH;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))
		if (FPCreateFile(Conn2, vol2,  0, dir , name1)) {
			test_nottested();
		}
	    bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR);

	    FAIL (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0))
	    FAIL (FPGetFileDirParams(Conn2, vol2, dir, name, bitmap, 0))
	    FAIL (FPGetFileDirParams(Conn, vol, dir, name1, bitmap, 0))
	}

	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);

	if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
	    test_failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		bitmap = (1<< DIRPBIT_UNIXPR);
		filedir.unix_priv &= ~(S_IWUSR |S_IWGRP| S_IWOTH);
 		FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))
 		FAIL (FPGetFileDirParams(Conn2, vol2, dir, "", 0, bitmap))

 		FAIL (htonl(AFPERR_ACCESS) != FPDelete(Conn2, vol2,  dir , name1))
 		FAIL (htonl(AFPERR_ACCESS) != FPDelete(Conn2, vol2,  dir , name))
	}

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR);

	FAIL (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0))
	FAIL (FPGetFileDirParams(Conn2, vol2, dir, name, bitmap, 0))

	/* ----------------- */
	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);
	if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
	    test_failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		bitmap = (1<< DIRPBIT_UNIXPR);
		filedir.unix_priv |= S_IWUSR;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))
	}


	FAIL (FPDelete(Conn, vol,  dir , name1))
	FAIL (FPDelete(Conn, vol,  dir , name))

fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test231: set unix access privilege two users");
}

/* ------------------------- */
STATIC void test232()
{
int  dir = 0;
char *name = "t232 file";
char *ndir = "t232 dir";
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;

DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
		test_skipped(T_UNIX_PREV);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		test_nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		test_nottested();
		goto fin;
	}
	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);

	FAIL (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap))

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR);

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
		bitmap = (1<< DIRPBIT_UNIXPR);
		filedir.unix_priv &= ~(S_IWUSR |S_IWGRP| S_IWOTH);
        filedir.access[0] = 0;
        filedir.access[1] = filedir.access[2] = filedir.access[3] = 3;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))
 		if (FPDelete(Conn, vol,  dir , name)) {
 			test_failed();
			bitmap = (1<< DIRPBIT_UNIXPR);
			filedir.unix_priv |= S_IWUSR |S_IWGRP| S_IWOTH;
        	filedir.access[0] = 0;
        	filedir.access[1] = filedir.access[2] = filedir.access[3] = 3;
 			FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))
			FAIL (FPDelete(Conn, vol,  dir , name))
		}
	}

fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test232: unix access privilege delete ro file in a rw folder");
}

/* ------------------------- */
STATIC void test345()
{
int  dir = 0;
char *name = "t345 file";
char *ndir = "t345 dir";
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
		test_nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		test_nottested();
		goto fin;
	}
	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);

	FAIL (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap))

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR);

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
		bitmap = (1<< DIRPBIT_UNIXPR);
		filedir.unix_priv = 0;
        filedir.access[0] = 0;
        filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))

		fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0 , dir , name, OPENACC_RD);
		if (fork) {
			test_failed();
			FAIL (FPCloseFork(Conn,fork))
		}

		fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, 0 , dir , name, OPENACC_RD);
		if (fork) {
			test_failed();
			FAIL (FPCloseFork(Conn,fork))
		}

		filedir.unix_priv = S_IRUSR | S_IWUSR;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))

		fork = FPOpenFork(Conn, vol, OPENFORK_DATA, 0 , dir , name, OPENACC_RD);
		if (!fork) {
			test_failed();
		}
		else {
			FAIL (FPCloseFork(Conn,fork))
		}

		fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, 0 , dir , name, OPENACC_RD);
		if (!fork) {
			test_failed();
		}
		else {
			FAIL (FPCloseFork(Conn,fork))
		}

	}
	FAIL (FPDelete(Conn, vol,  dir , name))
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test345: no unix access privilege");
}

/* ------------------------- */
STATIC void test346()
{
int  dir = 0;
char *name = "t346 file";
char *ndir = "t346 dir";
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
		test_nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		test_nottested();
		goto fin;
	}
	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);

	FAIL (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap))

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR);

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    test_failed();
		FAIL (FPDelete(Conn, vol,  dir , name))
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
		bitmap = (1<< DIRPBIT_UNIXPR);
		filedir.unix_priv = 0;
        filedir.access[0] = 0;
        filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))
 		if (FPDelete(Conn, vol,  dir , name)) {
 			test_failed();
			filedir.unix_priv = S_IRUSR | S_IWUSR;
 			FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))
			FAIL (FPDelete(Conn, vol,  dir , name))
		}
	}
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test346: delete a file with no unix access privilege");
}

/* ------------------------- */
STATIC void test347()
{
int  dir = 0;
char *name = "t347 file";
char *ndir = "t347 dir";
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV) {
		test_skipped(T_NO_UNIX_PREV);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		test_nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		test_nottested();
		goto fin;
	}
	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);

	if ( FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
		test_failed();
		goto fin1;
	}

	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.unix_priv = 1;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	if (htonl(AFPERR_BITMAP) != FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir)) {
	    test_failed();
		filedir.unix_priv = S_IRUSR | S_IWUSR;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))
	}

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR);

	if (/* htonl(AFPERR_BITMAP) != */ FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    test_failed();
	    goto fin1;
	}
	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM);
	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    test_failed();
	    goto fin1;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.unix_priv = 0;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	if (htonl(AFPERR_BITMAP) != FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir)) {
	    test_failed();
		filedir.unix_priv = S_IRUSR | S_IWUSR;
 		FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))
	}
fin1:
	FAIL (FPDelete(Conn, vol,  dir , name))
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test347: no unix access privilege");
}

/* ------------------------- */
STATIC void test348()
{
int  dir = 0;
char *ndir = "t348 dir";
char *name = "t348 file";
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
		test_nottested();
		goto test_exit;
	}

	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);

	if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
		test_failed();
		goto fin;
	}

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.unix_priv = 0;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))
	if (!FPCreateFile(Conn, vol,  0, dir , name)) {
		test_failed();
		FAIL (FPDelete(Conn, vol,  dir , name))
	}
	/* double check with didn't screw the parent !*/
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_failed();
	}
	else {
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	}

	filedir.unix_priv = S_IRUSR | S_IWUSR | S_IXUSR ;
 	FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))
	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		test_failed();
	}
	else {
		FAIL (FPDelete(Conn, vol,  dir , name))
	}
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test348: change a folder perm in root folder");
}

/* ------------------------- */
STATIC void test349()
{
int  dir = 0;
int  dir1 = 0;
char *ndir = "t349 dir";
char *ndir1 = "t349 subdir";
char *name = "t349 file";
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
		test_nottested();
		goto test_exit;
	}

	if (!(dir1 = FPCreateDir(Conn,vol, dir , ndir1))) {
		test_nottested();
		goto fin;
	}

	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);

	if (FPGetFileDirParams(Conn, vol, dir1, "", 0, bitmap)) {
		test_failed();
		goto fin;
	}

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs,  0, bitmap);
	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.unix_priv = 0;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	FAIL (FPSetFilDirParam(Conn, vol, dir1 , "", bitmap, &filedir))
	if (!FPCreateFile(Conn, vol,  0, dir1 , name)) {
		test_failed();
		FAIL (FPDelete(Conn, vol,  dir1 , name))
	}
	/* double check with didn't screw the parent !*/
	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		test_failed();
	}
	else {
		FAIL (FPDelete(Conn, vol,  dir , name))
	}

	filedir.unix_priv = S_IRUSR | S_IWUSR | S_IXUSR ;
 	FAIL (FPSetFilDirParam(Conn, vol, dir1 , "", bitmap, &filedir))
	if (FPCreateFile(Conn, vol,  0, dir1 , name)) {
		test_failed();
	}
	else {
		FAIL (FPDelete(Conn, vol,  dir1 , name))
	}
fin:
	FAIL (dir1 && FPDelete(Conn, vol,  dir1 , ""))
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test349: change a folder perm in a folder");
}

/* ------------------------- */
STATIC void test350()
{
int  dir = 0;
char *ndir = "t350 dir";
char *name = "t350 file";
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;
int old_unixpriv;
int ret;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
		test_skipped(T_UNIX_PREV);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		test_nottested();
		goto test_exit;
	}

	bitmap = (1<< DIRPBIT_PDINFO) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	         (1<< DIRPBIT_UNIXPR);

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
		test_failed();
		goto fin;
	}
	memset(&filedir, 0, sizeof(filedir));
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs,  0, bitmap);
	bitmap = (1<< DIRPBIT_UNIXPR);
	old_unixpriv = filedir.unix_priv;
	filedir.unix_priv = 0;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	ret = FPSetFilDirParam(Conn, vol, DIRDID_ROOT , "", bitmap, &filedir);
    if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
        test_failed();
	}
	if (!ret) {
		filedir.unix_priv = old_unixpriv;
 		FAIL (FPSetFilDirParam(Conn, vol, DIRDID_ROOT , "", bitmap, &filedir))
	}

	filedir.unix_priv = S_IRUSR | S_IXUSR ;
 	FAIL (FPSetFilDirParam(Conn, vol, DIRDID_ROOT , "", bitmap, &filedir))
	if (!FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_failed();
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	}
	filedir.unix_priv = old_unixpriv;
 	FAIL (FPSetFilDirParam(Conn, vol, DIRDID_ROOT , "", bitmap, &filedir))
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_failed();
	}
	else {
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	}
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test350: change root folder perm");
}

/* ------------------------- */
STATIC void test358()
{
int  dir = 0;
char *name = "t358 file";
char *name1 = "t358 file user 2";
char *ndir = "t358 dir";
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;
uint16_t vol2;
uint8_t old_access[4];
DSI *dsi2;
DSI *dsi;
int maxattempts = 10;
int attempt = 0;
int ret;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (Bigendian) {
		test_skipped(T_BIGENDIAN);
		goto test_exit;
	}
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		test_nottested();
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		test_nottested();
		goto test_exit;
	}
	FAIL (FPCreateFile(Conn, vol,  0, dir , name))
	FAIL (FPCreateFile(Conn2, vol2,  0, dir , name1))

	bitmap = (1<<FILPBIT_ATTR) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) | (1 << DIRPBIT_ACCESS);

	FAIL (FPGetFileDirParams(Conn2, vol2, dir, "", bitmap, bitmap))

	bitmap = (1<< DIRPBIT_ACCESS);

	if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
	    test_failed();
	    goto fin;
	}
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
    filedir.access[0] = 0;
    memcpy(old_access, filedir.access, sizeof(old_access));
    filedir.access[1] = filedir.access[2] = 0;
    filedir.access[3] = 3; /* only owner */
	bitmap = (1<< DIRPBIT_ACCESS) | (1<<FILPBIT_ATTR);
 	FAIL (FPSetFilDirParam(Conn, vol, dir , "", bitmap, &filedir))
	FAIL (FPGetFileDirParams(Conn2, vol2, dir, "", bitmap, bitmap))

	bitmap = (1 << FILPBIT_LNAME);
	FAIL (FPEnumerateFull(Conn, vol, 1, 5, 800,  dir, "", bitmap, bitmap))
	FAIL (htonl(AFPERR_ACCESS) != FPEnumerateFull(Conn2, vol2, 1, 5, 800,  dir, "", bitmap, bitmap))

    memcpy(filedir.access, old_access, sizeof(old_access));
	bitmap = (1<< DIRPBIT_ACCESS);
 	FAIL (FPSetDirParms(Conn, vol, dir , "", bitmap, &filedir))

fin:
	FAIL (FPDelete(Conn, vol, dir, name))
	FAIL (FPDelete(Conn, vol, dir, name1))

	while (attempt < maxattempts) {
		ret = FPDelete(Conn, vol, dir, "");
		if (ret == AFP_OK) {
			if (!Quiet) {
				printf("FPDelete succeeded on attempt %d\n", attempt + 1);
			}
			break;
		} else if (!Quiet) {
			printf("FPDelete failed on attempt %d\n", attempt + 1);
		}
		attempt++;
		sleep(1);
	}

	if (ret != AFP_OK) {
		test_failed();
	}

test_exit:
	exit_test("FPSetFileDirParms:test358: set unix access privilege two users");
}

/* ------------------------- */
STATIC void test359()
{
int  dir = 0;
char *name = "t359 file.pdf";
char *ndir = "t359 dir";
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;
int ret;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
		test_skipped(T_UNIX_PREV);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		test_nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		test_nottested();
		goto fin;
	}

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR) | (1<<FILPBIT_ATTR);

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    test_failed();
	    goto fin1;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
 	FAIL (FPSetFileParams(Conn, vol, dir,  name, (1<<FILPBIT_ATTR), &filedir))

	FAIL (ntohl(AFPERR_OLOCK) != FPDelete(Conn, vol,  dir , name))

	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.unix_priv = 0;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))

	bitmap = (1<<FILPBIT_ATTR);
	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    test_failed();
	    goto fin2;
	}
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
	if ((filedir.attr & ATTRBIT_NODELETE) != ATTRBIT_NODELETE) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED attribute not set\n");
		}
		test_failed();
	}
	ret = FPDelete(Conn, vol,  dir , name);
	if (ntohl(AFPERR_OLOCK) != ret) {
		test_failed();
		if (!ret) {
			goto fin;
		}
	}

fin2:
	filedir.unix_priv = S_IRUSR | S_IWUSR;
	bitmap = (1<< DIRPBIT_UNIXPR);
 	FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))
	filedir.attr = ATTRBIT_NODELETE;
 	FAIL (FPSetFileParams(Conn, vol, dir , name, (1<<FILPBIT_ATTR), &filedir))

fin1:
	FAIL (FPDelete(Conn, vol,  dir , name))
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test359: no unix access privilege with finder info");
}

/* ------------------------- */
STATIC void test361()
{
int  dir = 0;
char *name = "t361 file.pdf";
char *ndir = "t361 dir";
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;
int ret;
DSI *dsi;
DSI *dsi2;
uint16_t vol2;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UNIXPRIV)) {
		test_skipped(T_UNIX_PREV);
		goto test_exit;
	}

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		test_nottested();
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		test_nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		test_nottested();
		goto fin;
	}

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO);

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    test_failed();
	    goto fin1;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
 	FAIL (FPSetFileParams(Conn, vol, dir,  name, (1<<FILPBIT_ATTR), &filedir))

	FAIL (ntohl(AFPERR_OLOCK) != FPDelete(Conn, vol,  dir , name))

	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.unix_priv = 0;
    filedir.access[0] = 0;
    filedir.access[1] = filedir.access[2] = filedir.access[3] = 0;
 	FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))
	bitmap = (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO);
	if (FPGetFileDirParams(Conn2, vol2, dir, name, bitmap, 0)) {
	    test_failed();
	    goto fin2;
	}
	afp_filedir_unpack(&filedir, dsi2->data +ofs, bitmap,0);
	if ((filedir.attr & ATTRBIT_NODELETE) != ATTRBIT_NODELETE) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED attribute not set\n");
		}
		test_failed();
	}
	ret = FPDelete(Conn2, vol2,  dir , name);
	if (ntohl(AFPERR_OLOCK) != ret) {
		test_failed();
		if (!ret) {
			goto fin;
		}
	}

fin2:
	filedir.unix_priv = S_IRUSR | S_IWUSR;
	bitmap = (1<< DIRPBIT_UNIXPR);
 	FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))
	filedir.attr = ATTRBIT_NODELETE;
 	FAIL (FPSetFileParams(Conn, vol, dir , name, (1<<FILPBIT_ATTR), &filedir))

fin1:
	FAIL (FPDelete(Conn, vol,  dir , name))
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test361: no unix access privilege two users with finder info");
}

/* ------------------------- */
STATIC void test400()
{
int  dir = 0;
char *name = "t400 file.pdf";
char *ndir = "t400 dir";
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
		test_nottested();
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, dir , name)) {
		test_nottested();
		goto fin;
	}

	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) |
		(1 << DIRPBIT_UNIXPR) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO);

	if (FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0)) {
	    test_nottested();
	    goto fin1;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);

	bitmap = (1<< DIRPBIT_UNIXPR);
	filedir.uid = 100000;
	filedir.gid = 125000;
	filedir.unix_priv = 0660;
 	FAIL (FPSetFilDirParam(Conn, vol, dir , name, bitmap, &filedir))

	FAIL(FPGetFileDirParams(Conn, vol, dir, name, bitmap, 0))

fin1:
	FAIL (FPDelete(Conn, vol,  dir , name))
fin:
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("FPSetFileDirParms:test400: set file owner");
}

/* ----------- */
void FPSetFileDirParms_test()
{
    ENTER_TESTSET
    test98();
    test230();
    test231();
    test232();
    test345();
    test346();
    test347();
    test348();
    test349();
    test350();
    test358();
    test359();
    test361();
    test400();
}
