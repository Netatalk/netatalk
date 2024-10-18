/* ----------------------------------------------
*/
#include "specs.h"

static char temp[MAXPATHLEN];

/* ------------------------- */
STATIC void test162()
{
char ndir[4];

int dir;
uint16_t vol = VolID;
DSI *dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test162: illegal UTF8 name\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}

    ndir[0] = 'e';
    ndir[1] = 0xc3;
    ndir[2] = 0;
	if ((dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		failed();
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , ndir))
	}
	else if (ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
		failed();
	}
test_exit:
	exit_test("test162");
}

/* ------------------------- */
STATIC void test166()
{
char nfile[8];
uint16_t bitmap;
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test166: utf8 precompose decompose\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}

    bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);
	strcpy(nfile, "ee.rtf");
	nfile[0] = 0xc3;         /* �.rtf precompose */
	nfile[1] = 0xa9;

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , nfile)) {
		nottested();
		goto test_exit;
	}
    bitmap = (1<< FILPBIT_PDID)|(1<< FILPBIT_LNAME)|(1 << FILPBIT_PDINFO );

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, nfile, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
	    if (strcmp(filedir.lname, "\216.rtf")) {
		    fprintf(stdout,"\tFAILED %s should be \\216.rtf\n",filedir.lname);
		    failed_nomsg();
	    }
	}
	FPEnumerate_ext(Conn, vol,  DIRDID_ROOT , "",
		                    (1 << FILPBIT_PDINFO )|(1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN), 0);

	strcpy(nfile, "eee.rtf");
	nfile[1] = 0xcc;         /* �.rtf decompose */
	nfile[2] = 0x81;
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , nfile))
test_exit:
	exit_test("test166");
}

/* ------------------------- */
STATIC void test167()
{
char nfile[8];
uint16_t bitmap;
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test167: utf8 precompose decompose\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}

    bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);
	strcpy(nfile, "laa");
	nfile[1] = 0xc3;         /* l� */
	nfile[2] = 0xa0;

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , nfile)) {
		nottested();
		goto test_exit;
	}
    bitmap = (1<< FILPBIT_PDID)|(1<< FILPBIT_LNAME)|(1 << FILPBIT_PDINFO );

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, nfile, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
	    if (strcmp(filedir.lname, "l\210")) {
		    fprintf(stdout,"\tFAILED %s should be l\\210\n",filedir.lname);
		    failed_nomsg();
	    }
	}
	FPEnumerate_ext(Conn, vol,  DIRDID_ROOT , "",
		                    (1 << FILPBIT_PDINFO )|(1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN), 0);

	strcpy(nfile, "laaa");
	nfile[2] = 0xcc;         /* l� */
	nfile[3] = 0x80;
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , nfile))
test_exit:
	exit_test("test167");
}

/* ------------------------- */
STATIC void test181()
{
char *name  = "t181 folder";
char *name1 = "t181 donne\314\201es"; /* decomposed donn�es */
char *name2 = "t181 foo";
uint16_t vol = VolID;
int  dir;
int  dir1;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test181: test search by ID UTF8\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}
	dir   = FPCreateDir(Conn, vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
	}

	dir1  = FPCreateDir(Conn,vol, dir , name1);
	if (!dir1) {
		failed();
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir1 , name2))

	FAIL (FPCloseVol(Conn,vol))

	vol = VolID = FPOpenVol(Conn, Vol);
	if (FPEnumerate(Conn, vol,  dir1 , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
		/* warm the cache */
		FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);
		FPEnumerate(Conn, vol,  dir , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);
	}

	FAIL (FPDelete(Conn, vol,  dir1 , name2))
	FAIL (FPDelete(Conn, vol,  dir , name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test181");
}

/* -------------------------
 */
STATIC void test185()
{
char *name = "t185.txt";
char *name1 = "t185 donne\314"; /* decomposed donn�es */
uint16_t vol = VolID;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test185: rename utf8 name\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	FAIL (ntohl(AFPERR_PARAM) != FPRename(Conn, vol, DIRDID_ROOT, name, name1))

	FAIL (ntohl(AFPERR_PARAM) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1))

	FAIL (FPDelete(Conn, vol, DIRDID_ROOT , name))

	FPFlush(Conn, vol);
test_exit:
	exit_test("test185");
}

/* ------------------------- */
STATIC void test233()
{
char *name = "t233 dire\314\201";
uint16_t vol = VolID;
DSI *dsi;
int  dir;
uint16_t bitmap = 0;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test233: false mangled UTF8 dirname\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}
	dir   = FPCreateDir(Conn, vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}
	bitmap =(1<< DIRPBIT_DID) | (1<< DIRPBIT_PDID)  | (1<< DIRPBIT_PDINFO) |(1<<DIRPBIT_LNAME);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, name, 0,bitmap )) {
		failed();
	}
	sprintf(temp,"t23#%X", ntohl(dir));

	if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, temp, 0, bitmap)) {
	    /* MAC OK */
		failed();
	}
	sprintf(temp,"t233 dire#%X", ntohl(dir));

	if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, temp, 0,bitmap )) {
	    /* MAC OK */
		failed();
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test233");
}

/* ------------------------- */
STATIC void test234()
{
char *name = "t234 file\314\201";
uint16_t vol = VolID;
DSI *dsi;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test234: false mangled UTF8 filename\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM)|(1<<FILPBIT_LNAME);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, name, bitmap,0 )) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
		sprintf(temp,"t23#%X", ntohl(filedir.did));
		if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, temp, bitmap,0 )) {
			failed();
		}
		sprintf(temp,"t234 file#%X", ntohl(filedir.did));
		if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, temp, bitmap,0 )) {
			failed();
		}
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test234");
}

/* ------------------------- */
STATIC void test312()
{
char *name = "t312-\xd7\xa4\xd7\xaa\xd7\x99\xd7\x97\xd7\x94.mp3";
uint16_t vol = VolID;
DSI *dsi;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test312: mangled UTF8 filename\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) | (1<<FILPBIT_LNAME);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, name, bitmap,0 )) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);

        sprintf(temp,"t312-#%X", ntohl(filedir.did));
		if (ntohl(AFPERR_NOOBJ) != 	FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, temp, bitmap,0 )) {
			failed();
		}

        sprintf(temp,"t312-#%X.mp3", ntohl(filedir.did));
		if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, temp, bitmap,0 )) {
			failed();
		}
		else {
		    filedir.isdir = 0;
		    afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
		    if (strcmp(filedir.utf8_name, name)) {
				fprintf(stdout,"\tFAILED %s should be %s\n",filedir.utf8_name, name); 
		    }
		}
        sprintf(temp,"t3-#%X.mp3", ntohl(filedir.did));
		if (ntohl(AFPERR_NOOBJ) != 	FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, temp, bitmap,0 )) {
			failed();
		}
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test312");
}

/* ------------------------- */
STATIC void test313()
{
char *name = "t313-\xd7\xa4\xd7\xaa\xd7\x99\xd7\x97\xd7\x94 dir";
uint16_t vol = VolID;
DSI *dsi;
int  dir;
uint16_t bitmap = 0;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test313: mangled UTF8 dirname\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}
	dir   = FPCreateDir(Conn, vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}
	bitmap = (1<< DIRPBIT_DID) | (1<< DIRPBIT_PDID)  | (1<< DIRPBIT_PDINFO) | (1<<DIRPBIT_LNAME);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, name, 0, bitmap )) {
		failed();
	}
	sprintf(temp,"t313-#%X", ntohl(dir));

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, temp, 0, bitmap )) {
		failed();
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test313");
}

/* ------------------------- */
STATIC void test314()
{
char *name = "test314#1";
uint16_t vol = VolID;
DSI *dsi;
uint16_t bitmap = 0;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test314: invalid mangled UTF8 name\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	bitmap = (1<< FILPBIT_PDINFO) | (1<<FILPBIT_LNAME);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, name, bitmap, 0 )) {
		failed();
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test314");
}

/* -------------------------
 * MAC FAILED
*/
STATIC void test337()
{
char *name = "\xd7\xa4\xd7\xaa\xd7\x99\xd7\x97\xd7\x94 test 337.mp3";
uint16_t vol = VolID;
DSI *dsi;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = 0;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test337: mangled UTF8 filename\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	bitmap = (1 <<  FILPBIT_PDINFO) | (1<< FILPBIT_PDID) | (1<< FILPBIT_FNUM) | (1<<FILPBIT_LNAME);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, name, bitmap,0 )) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);

        sprintf(temp,"???#%X", ntohl(filedir.did));
		if (ntohl(AFPERR_NOOBJ) != 	FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, temp, bitmap,0 )) {
	    	/* MAC OK */
			failed();
		}

        sprintf(temp,"???#%X.mp3", ntohl(filedir.did));
		if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, temp, bitmap,0 )) {
			failed();
		}
		else {
		    filedir.isdir = 0;
		    afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
		    if (strcmp(filedir.utf8_name, name)) {
				fprintf(stdout,"\tFAILED %s should be %s\n",filedir.utf8_name, name); 
		    }
		}
        sprintf(temp,"t#%X.mp3", ntohl(filedir.did));
		if (ntohl(AFPERR_NOOBJ) != 	FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, temp, bitmap,0 )) {
	    	/* MAC OK */
			failed();
		}
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test337");
}

/* ------------------------- */
extern int Force_type2;

STATIC void test381()
{
char nfile[8];
uint16_t bitmap;
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test381: utf8 use type 2 name with AFP3 connection\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}

    bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);
	strcpy(nfile, "laaa");
	nfile[2] = 0xcc;         /* l� */
	nfile[3] = 0x80;

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , nfile)) {
		nottested();
		goto test_exit;
	}
    bitmap = (1<< FILPBIT_PDID)|(1<< FILPBIT_LNAME)|(1 << FILPBIT_PDINFO );

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, nfile, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
	    if (strcmp(filedir.lname, "l\210")) {
		    fprintf(stdout,"\tFAILED %s should be l\\210\n",filedir.lname);
		    failed_nomsg();
	    }
	}
	Force_type2 = 1;
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , filedir.lname))
	Force_type2 = 0;

	FAIL (ntohl(AFPERR_NOOBJ) !=  FPDelete(Conn, vol,  DIRDID_ROOT , nfile))
test_exit:
	exit_test("test381");
}

/* ------------------------- */
STATIC void test382()
{
char nfile[8];
uint16_t bitmap;
uint16_t vol = VolID;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test382: utf8 use type 2 name with AFP3 connection\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}

    bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);
	strcpy(nfile, "laaa");
	nfile[2] = 0xcc;         /* l� */
	nfile[3] = 0x80;

	if (!FPCreateDir(Conn, vol, DIRDID_ROOT , nfile)) {
		nottested();
		goto test_exit;
	}
    bitmap = (1<< FILPBIT_PDID)|(1<< FILPBIT_LNAME)|(1 << FILPBIT_PDINFO );

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, nfile, 0, bitmap)) {
		failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	    if (strcmp(filedir.lname, "l\210")) {
		    fprintf(stdout,"\tFAILED %s should be l\\210\n",filedir.lname);
		    failed_nomsg();
	    }
	}
	Force_type2 = 1;
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , filedir.lname))
	Force_type2 = 0;

	FAIL (ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol,  DIRDID_ROOT , nfile))
test_exit:
	exit_test("test382");
}

/* ------------------------- */
STATIC void test383()
{
char *file = "test 383 la\xcc\x80";/* l� */
char *file2 = "test 383 l\210";
char *nfile2 = "test 383 new name l\210";
char *nfile = "test 383 new name la\xcc\x80";/* l� */
uint16_t bitmap;
uint16_t vol = VolID;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test383: utf8 rename type 2 name, AFP3 connection\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}

    bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , file)) {
		nottested();
		goto test_exit;
	}
	Force_type2 = 1;
	FAIL (FPRename(Conn, vol, DIRDID_ROOT, file2, nfile2))
	Force_type2 = 0;

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , nfile))
	FPDelete(Conn, vol,  DIRDID_ROOT , file);
test_exit:
	exit_test("test383");
}

/* ------------------------- */
STATIC void test384()
{
char *file = "test 384 la\xcc\x80";/* l� */
char *file2 = "test 384 l\210";
char *nfile2 = "test 384 /new name l\210";
char *nfile = "test 384 new name la\xcc\x80";/* l� */
uint16_t bitmap;
uint16_t vol = VolID;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test384: utf8 rename type 2 name, AFP3 connection wrong parameter\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}

    bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , file)) {
		nottested();
		goto test_exit;
	}
	Force_type2 = 1;
	FAIL (ntohl(AFPERR_PARAM) != FPRename(Conn, vol, DIRDID_ROOT, file2, nfile2))
	Force_type2 = 0;

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , file))
	FPDelete(Conn, vol,  DIRDID_ROOT , nfile);
test_exit:
	exit_test("test384");
}

/* ------------------------- */
STATIC void test385()
{
char *file = "test 385 la\xcc\x80";/* l� */
char *file2 = "test 385 l\210";
char *nfile2 = "test 385 new name l\210";
char *nfile = "test 385 new name la\xcc\x80";/* l� */
uint16_t bitmap;
uint16_t vol = VolID;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test385: utf8 copyfile type 2 name, AFP3 connection\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}

    bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , file)) {
		nottested();
		goto test_exit;
	}
	Force_type2 = 1;
	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, file2, "", nfile2))
	Force_type2 = 0;

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , nfile))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , file));
test_exit:
	exit_test("test385");
}

/* ------------------------- */
STATIC void test386()
{
char *file = "test 386 la\xcc\x80";/* l� */
char *file2 = "test 386 l\210";
char *nfile2 = "test 386 new name l\210";
char *nfile = "test 386 new name la\xcc\x80";/* l� */
uint16_t bitmap;
uint16_t vol = VolID;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test386: utf8 moveandrename type 2 name, AFP3 connection\n");

	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	if ( !(get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		test_skipped(T_UTF8);
		goto test_exit;
	}

    bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , file)) {
		nottested();
		goto test_exit;
	}
	Force_type2 = 1;
	FAIL (FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, file2, nfile2))
	Force_type2 = 0;

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , nfile))
	FPDelete(Conn, vol,  DIRDID_ROOT , file);
test_exit:
	exit_test("test386");
}

/* ------------------------- */
STATIC void test395()
{
uint16_t vol = VolID;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Utf8:test395: utf8 bit set if AFP < 3.0\n");

	if (Conn->afp_version >= 30) {
		test_skipped(T_AFP2);
		goto test_exit;
	}

	if ( (get_vol_attrib(vol) & VOLPBIT_ATTR_UTF8)) {
		failed();
	}

test_exit:
	exit_test("test395");
}


/* ----------- */
void Utf8_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"UTF8 tests\n");
    test162();
    test166();
    test167();
    test181();
    test185();
	test233();
	test234();
	test312();
	test313();
	test314();
	test337();
	test381();
	test382();
	test383();
	test384();
	test385();
	test386();
	test395();
}
