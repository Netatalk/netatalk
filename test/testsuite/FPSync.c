/* ----------------------------------------------*/
#include "specs.h"

/* ------------------------- */
STATIC void test2()
{
    int  dir;
    uint16_t vol = VolID;
    DSI *dsi;
    char *name = "t2 sync dir";
    int  ofs =  3 * sizeof( uint16_t );
    struct afp_filedir_parms filedir;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPSyncDir(test2): sync dir\n");

 	if (FPSyncDir(Conn, vol, DIRDID_ROOT))
		failed();

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, 0, (1 << DIRPBIT_DID) )) {
		failed();
        goto fin;
	}

    filedir.isdir = 1;
    afp_filedir_unpack(&filedir, dsi->data +ofs, 0, (1 << DIRPBIT_DID));
 	if (FPSyncDir(Conn, vol, filedir.did))
		failed();

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("test2");
}
