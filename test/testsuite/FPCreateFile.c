/* ----------------------------------------------
*/
#include "specs.h"

/* -------------------------- */
STATIC void test51()
{
uint16_t bitmap;
int fork = 0;
char *name1 = "t51 file";
char *name2 = "t51 other file.txt";
char *name3 = "t51 dir";
int  dir;
int  did;
char *rodir = "t51 read only access";
int rdir = 0;
uint16_t vol = VolID;
DSI *dsi;
int ret;

	dsi = &Conn->dsi;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!(rdir = read_only_folder(vol, DIRDID_ROOT, rodir) ) ) {
		goto test_exit;
	}

	bitmap = (1<< DIRPBIT_DID);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, rodir, 0, bitmap)) {
		failed();
		goto fin;
	}
	memcpy(&did, dsi->data +3 * sizeof( uint16_t ), sizeof(did));

	ret = FPCreateFile(Conn, vol,  0, DIRDID_ROOT , rodir);
	if (not_valid(ret, /* MAC */AFPERR_EXIST, AFPERR_BADTYPE)) {
		failed();
	}

	FAIL (ntohl(AFPERR_ACCESS) != FPCreateFile(Conn, vol,  0, did , name1))

	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name2))

	FAIL (ntohl(AFPERR_EXIST) != FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name2))

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA  , 0 ,DIRDID_ROOT, name2,
		OPENACC_RD| OPENACC_WR|  OPENACC_DWR );

	if (!fork) {
		failed();
		goto fin;
	}

	if (ntohl(AFPERR_EXIST) != FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name2)) {
		failed();
		FAIL (FPCloseFork(Conn,fork))
	}
	if (ntohl(AFPERR_BUSY) != FPCreateFile(Conn, vol,  1, DIRDID_ROOT , name2)) {
		failed();
	}
	FAIL (FPCloseFork(Conn,fork))

	FAIL ( ntohl(AFPERR_NOOBJ) != FPCreateFile(Conn, vol,  1, did , "folder/essai"))

	/* ----------- */
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name3))) {
		failed();
		goto fin;
	}

	if (FPCreateFile(Conn, vol,  1, dir , name2)) {
		failed();
		goto fin;
	}

	FAIL (FPCreateFile(Conn, vol,  1, dir , name2))
fin:
	if (rdir) {
		delete_folder(vol, DIRDID_ROOT, rodir);
	}
	FAIL (FPDelete(Conn, vol,  dir , name2))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name2))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name3))

test_exit:
	exit_test("FPCreateFile:test51:  Create file with errors");
}

/* --------------------------
 * test for japanese convertion
*/
STATIC void test393()
{
// char *name2 = "t393 \343\203\262\343\202\231.txt"; /* \231 is bogus */
// char *name2 = "t393 \343\203\262\343\202.txt";
// char *name2 = "\210m";
/*
SHIFT JIS
"\226\213   \226\226  \203o     \203b     \203N     _ \203f \201[ \203^  _ \214\251 \217-110713626";
UCS2
   U5E55     U672B    U30D0     U30C3      U30AF       U30C7  U30FC U30BF    U898B
UTF8
  e5 b9 95, e6 9c ab, e3 83 90, e3 83 83, e3 82 af, 5f,e3 83 87, e3 83 bc, e3 82 bf, 5f, e8 a6 8b, 5f
30 37 31 33 36 32 36 0a
*/
char *name  = "\226\213\226\226\203o\203b\203N_\203f\201[\203^_\214\251\217-110713626";
/* trailing error */
// char *name1 = "\226\213\226\226\203o\203b\203N_\203f\201[\203^_\214\251\217";
char *name1 = "\345\271\225\346\234\253\346\234\253\343\203\217\343\202\231\346\234\253\343\203\217\343\202\231\345\271\225\346\234\253\345";
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

getchar();
	ENTER_TEST
	if (Conn->afp_version >= 30) {
		test_skipped(T_AFP2);
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1))
	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE) |
	         (1<<FILPBIT_DFLEN) | (1<<FILPBIT_RFLEN)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		failed();
	}

getchar();
	FAIL( FPDelete(Conn, vol,  DIRDID_ROOT , name))
	FAIL( FPDelete(Conn, vol,  DIRDID_ROOT , name1))

test_exit:
	exit_test("FPCreateFile:test393:  Create file with Japanese name, illegal char");
}

/* ----------- */
void FPCreateFile_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPCreateFile page 138\n");
    fprintf(stdout,"-------------------\n");
    test51();
}
