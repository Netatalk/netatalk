/* ----------------------------------------------
*/
#include "specs.h"

STATIC void test180()
{
int  dir;
char *name = "t180 test Map name";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap =  (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE)
|(1 << DIRPBIT_ACCESS) | (1<<DIRPBIT_FINFO)| (1<<DIRPBIT_UID) | (1 << DIRPBIT_GID) ;
unsigned int ret;
uint16_t vol = VolID;
DSI *dsi = &Conn->dsi;
char *grp = NULL;
char *usr = NULL;

	ENTER_TEST

	// FIXME: encoding tests are broken in Netatalk 4.0
	if (Exclude) {
		test_skipped(T_EXCLUDE);
		goto test_exit;
	}
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		test_nottested();
		goto test_exit;
	}
	FAIL (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, 0,bitmap ))

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);

	FAIL (FPMapID(Conn, 1, 0))  /* user to Mac roman */

	ret = FPMapID(Conn, 1, filedir.uid);  /* user to Mac roman */
	if (ret) {
		test_failed();
	}
	else {
		usr = strp2cdup(dsi->commands);
	}

	ret = FPMapID(Conn, 1, -filedir.uid);  /* user to Mac roman */
	if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NOITEM, AFPERR_NOITEM)) {
		test_failed();
	}

	ret = FPMapID(Conn, 2, -filedir.gid);  /* group to Mac roman */
	/* sometime -filedir.gid is there */
	if (ret && not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NOITEM, AFPERR_NOITEM)) {
		test_failed();
	}

	ret = FPMapID(Conn, 2, filedir.gid);  /* group to Mac roman */
	if (ret) {
		test_failed();
	}
	else {
		grp = strp2cdup(dsi->commands);
	}

	ret = FPMapID(Conn, 3, filedir.uid); /* user to UTF8 */
	if (Conn->afp_version >= 30 && ret) {
		test_failed();
	}
	ret = FPMapID(Conn, 4, filedir.gid); /* group to UTF8 */
	if (Conn->afp_version >= 30 && ret) {
		test_failed();
	}

	FAIL ((htonl(AFPERR_NOITEM) != FPMapID(Conn, 5, filedir.gid)))
	/* ---------------------
		MapName subfunction
		1 Unicode to User ID
		2 Unicode to Group ID
		3 Mac roman to UID
		4 Mac roman to GID
	*/

	FAIL (htonl(AFPERR_NOITEM) != FPMapName(Conn, 5, "toto"))

#if 0
	/* fail with OSX and new netatalk */
	ret = FPMapName(Conn, 3, "");
	if (ret && not_valid_bitmap(ret, BITERR_NOOBJ, AFPERR_PARAM)) {
		test_failed();
	}
#endif

	ret = FPMapName(Conn, 3, "toto");
	if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NOITEM, AFPERR_NOITEM)) {
		test_failed();
	}
	ret = FPMapName(Conn, 4, "toto");
	if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NOITEM, AFPERR_NOITEM)) {
		test_failed();
	}

	ret =  FPMapName(Conn, 1, "toto");
	if (Conn->afp_version >= 30 && not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NOITEM, AFPERR_NOITEM)) {
		test_failed();
	}
	else if (Conn->afp_version < 30 && ret != htonl(AFPERR_PARAM)) {
		test_failed();
	}
	/* ------------------ */
	ret = FPMapName(Conn, 2, "toto");
	if (Conn->afp_version >= 30 && not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NOITEM, AFPERR_NOITEM)) {
		test_failed();
	}
	else if (Conn->afp_version < 30 && ret != htonl(AFPERR_PARAM)) {
		test_failed();
	}
	/* usr */
	if (usr) {
		ret = FPMapName(Conn, 1, usr);
		if (Conn->afp_version >= 30 && ret) {
			test_failed();
		}

		FAIL (FPMapName(Conn, 3, usr))
		free(usr);
	}

	/* group */
	if (grp) {
		ret = FPMapName(Conn, 2, grp);
		if (Conn->afp_version >= 30 && ret) {
			test_failed();
		}

		FAIL (FPMapName(Conn, 4, grp))
		free(grp);
	}

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
test_exit:
	exit_test("FPMapName:test180: test Map Name");
}

/* ----------- */
void FPMapName_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
	test180();
}
