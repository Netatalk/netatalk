/* ----------------------------------------------
*/
#include "specs.h"

STATIC void test73()
{
int fork;
int dir;
int dir1;
int dir2;
uint16_t bitmap = 0;
char *name  = "t73 Move and rename";
char *name1 = "T73 Move and rename";
char *name2 = "t73 dir";
uint16_t vol = VolID;
int ret;

	enter_test();

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name2))) {
		nottested();
		goto test_exit;
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT , name2)) {
		nottested();
		goto test_exit;
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		nottested();
		goto test_exit;
	}

	/* cname unchdirable */
	if (Conn2) {
		int pdir;

		if (!(pdir = no_access_folder(vol, DIRDID_ROOT, "t73 700"))) {
			FPDelete(Conn, vol,  dir, name);
			failed_nomsg();
			goto test_exit;
		}
		ret = FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, "t73 700/essay", name1);
		if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_ACCESS)) {
			failed();
		}
		delete_folder(vol, DIRDID_ROOT, "t73 700");
	}
	/* bad ddid */
	FAIL (ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, name1))
	FAIL (!(dir1 = FPCreateDir(Conn,vol, DIRDID_ROOT , name2)))

	FAIL (FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir1, name, ""))

	FAIL (ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol,  DIRDID_ROOT, name))

	FAIL (FPMoveAndRename(Conn, vol, dir1, DIRDID_ROOT, name, ""))
	/* dirty but well */
	FAIL (ntohl(AFPERR_EXIST) != FPCreateFile(Conn, vol,  0, DIRDID_ROOT, name1))

	FAIL (ntohl(AFPERR_EXIST) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1))

	FAIL (ntohl(AFPERR_EXIST) != FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
	}
	if (fork) {
		FAIL (ntohl(AFPERR_EXIST) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name1, name))
		FAIL (FPCloseFork(Conn,fork))
	}
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol,  DIRDID_ROOT, name1))

	FAIL (!(dir2 = FPCreateDir(Conn,vol, DIRDID_ROOT , name)))
	FAIL (!FPCreateDir(Conn,vol, dir2 , name1))

	FAIL (FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir1,  name, ""))
	FAIL (FPDelete(Conn, vol,  dir2, name1))
	FAIL (FPDelete(Conn, vol,  dir1, name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name2))
test_exit:
	exit_test("FPMoveAndRename:test73: Move and rename");
}

static char temp[MAXPATHLEN];
static char temp1[MAXPATHLEN];

/* ------------------------- */
STATIC void test302()
{
char *name = "t302 dir";
char *name1 = "t302 file";
char *name2 = "t302 file1";
int  dir;
uint16_t vol = VolID;
int id,id1;

	enter_test();

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name1))

	id = get_fid(Conn, vol, dir , name1);

	if (!Mac) {
		sprintf(temp,"%s/%s/%s", Path, name, name1);
		sprintf(temp1,"%s/%s/%s", Path, name, name2);
		if (rename(temp, temp1) < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
			}
			failed_nomsg();
		}
	}
	else {
		FAIL (FPMoveAndRename(Conn, vol, dir, dir, name1, name2))
	}
	id1 = get_fid(Conn, vol, dir , name2);
	if (id != id1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED id are not the same %d %d\n", ntohl(id), ntohl(id1));
		}
		failed_nomsg();
	}
	FAIL (FPDelete(Conn, vol,  dir , name2))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPMoveAndRename:test302: file renamed by someone else, cnid not updated");
}


/* ----------- */
void FPMoveAndRename_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPMoveAndRename page 223\n");
    fprintf(stdout,"-------------------\n");
    test73();
    test302();
}
