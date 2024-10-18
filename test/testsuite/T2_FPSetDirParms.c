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

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPSetDirParms:t121: test set dir setfilparam (create .AppleDouble)\n");

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}
	dsi = &Conn->dsi;
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}

	memset(&filedir, 0, sizeof(filedir));
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, 0, bitmap)) {
		failed();
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
	exit_test("test121");
}

/* ----------- */
void FPSetDirParms_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPSetDirParms page 255\n");
    test121();
}
