/* ----------------------------------------------
*/
#include "specs.h"
#include "adoublehelper.h"

/* ------------------------- */
STATIC void test121()
{
int  dir;
char *name = "t121 test dir setdirparam";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<DIRPBIT_FINFO)| (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE);
uint16_t vol = VolID;
DSI *dsi;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}
	dsi = &Conn->dsi;
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		test_nottested();
		goto test_exit;
	}

	memset(&filedir, 0, sizeof(filedir));
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, 0, bitmap)) {
		test_failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (!Mac) {
			if (delete_unix_adouble(Path, name)) {
				goto fin;
			}
		}
 		FAIL (FPSetDirParms(Conn, vol, DIRDID_ROOT , name, bitmap, &filedir))
 		FAIL (htonl(AFPERR_BITMAP) != FPSetDirParms(Conn, vol, DIRDID_ROOT , name, 0xffff, &filedir))
	}
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPSetDirParms:test121: test set dir setfilparam (create .AppleDouble)");
}

/* ----------- */
void T2FPSetDirParms_test()
{
    ENTER_TESTSET
    test121();
}
