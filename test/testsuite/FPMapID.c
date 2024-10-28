/* ----------------------------------------------
*/
#include "specs.h"

STATIC void test208()
{
int  dir;
char *name = "t208 test Map ID";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap =  (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE)
|(1 << DIRPBIT_ACCESS) | (1<<DIRPBIT_FINFO)| (1<<DIRPBIT_UID) | (1 << DIRPBIT_GID) ;
unsigned int ret;
uint16_t vol = VolID;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	// FIXME: encoding tests are broken in Netatalk 4.0
	if (Exclude) {
		test_skipped(T_EXCLUDE);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}
	FAIL (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, 0,bitmap ))

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);

	FAIL (FPMapID(Conn, 1, 0))  /* user to Mac roman */

	FAIL (FPMapID(Conn, 1, filedir.uid))  /* user to Mac roman */

	ret = FPMapID(Conn, 1, -filedir.uid);  /* user to Mac roman */
	if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NOITEM, AFPERR_NOITEM)) {
		failed();
	}

	ret = FPMapID(Conn, 2, -filedir.gid);  /* group to Mac roman */
	/* sometime -filedir.gid is there */
	if (ret && not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NOITEM, AFPERR_NOITEM)) {
		failed();
	}

	FAIL (FPMapID(Conn, 2, filedir.gid))  /* group to Mac roman */
	ret = FPMapID(Conn, 3, filedir.uid); /* user to UTF8 */
	if (Conn->afp_version >= 30 && ret) {
		failed();
	}
	else if (Conn->afp_version < 30 && ret != htonl(AFPERR_PARAM)) {
		failed();
	}
	ret = FPMapID(Conn, 4, filedir.gid); /* group to UTF8 */
	if (Conn->afp_version >= 30 && ret) {
		failed();
	}
	else if (Conn->afp_version < 30 && ret != htonl(AFPERR_PARAM)) {
		failed();
	}

	FAIL ((htonl(AFPERR_NOITEM) != FPMapID(Conn, 5, filedir.gid)))
	/* --------------------- */
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPMapID:test208: test Map ID call");
}

/* ----------- */
void FPMapID_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
	test208();
}
