/* ----------------------------------------------
*/
#include "specs.h"
#include "adoublehelper.h"

/* ------------------------- */
STATIC void test373()
{
char *name  = "t373 old file name";
char *name1 = "t373 new file name";
uint16_t vol = VolID;
int tp,tp1;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;
uint16_t bitmap;
uint32_t mdate = 0;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto fin;
	}

	if (!Mac && delete_unix_md(Path, "", name)) {
		test_nottested();
		goto fin;
	}

	if (!Quiet) {
		fprintf(stdout,"sleep(2)\n");
	}
	sleep(2);
	tp = get_fid(Conn, vol, DIRDID_ROOT, name);
	if (!tp) {
		test_nottested();
		goto fin;
	}
	bitmap = (1<<DIRPBIT_MDATE);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		mdate = filedir.mdate;
	}

	FAIL (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1))

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name1, bitmap,0)) {
		test_failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		if (mdate != filedir.mdate)  {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED modification date differ\n");
		}
	        test_failed();
	        goto fin;
		}
	}

	tp1 = get_fid(Conn, vol, DIRDID_ROOT, name1);
	if (!tp1) {
		test_nottested();
		goto fin;
	}
	if (tp == tp1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED both files have same ID\n");
		}
	    test_failed();
	}

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))

test_exit:
	exit_test("FPCopyFile:test373: copyFile check meta data, file without resource fork");
}

/* ----------- */
void T2FPCopyFile_test()
{
    ENTER_TESTSET
	test373();
}
