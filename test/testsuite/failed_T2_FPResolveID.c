/* ----------------------------------------------
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "specs.h"
#include "adoublehelper.h"

static char temp[MAXPATHLEN];
static char temp1[MAXPATHLEN];

/* -------------------------- */
STATIC void test417()
{
uint16_t vol = VolID;
int  dir;
char *name1 = "t417_file1";
char *name2 = "t417_file2";
char *dir1 = "t417_dir";
int  ofs =  3 * sizeof( uint16_t );
int  ofs_res =  sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<DIRPBIT_FINFO);
struct afp_filedir_parms filedir;
int fid1 = 0, fid2 = 0;
int nfid1 = 0, nfid2 = 0;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , dir1))) {
		failed();
		goto test_exit;
	}

	/* create file1 */
	FAIL (FPCreateFile(Conn, vol,  0, dir , name1))
	if (FPGetFileDirParams(Conn, vol,  dir , name1, bitmap,0)) {
		failed();
		goto fin;
	}

	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	fid1 = filedir.did;

	FAIL (FPResolveID(Conn, vol, fid1, bitmap))
	afp_filedir_unpack(&filedir, dsi->data +ofs_res, bitmap, 0);
	if (fid1 != filedir.did) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED FPGetFileDirParams, FPResolveID id differ %x %x\n", fid1, filedir.did);
		}
		failed_nomsg();
	}

	/* create file2 */
	FAIL (FPCreateFile(Conn, vol,  0, dir , name2))

	if (FPGetFileDirParams(Conn, vol,  dir , name2, bitmap,0)) {
		failed();
		goto fin;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	fid2 = filedir.did;

	FAIL (FPResolveID(Conn, vol, fid2, bitmap))
	afp_filedir_unpack(&filedir, dsi->data +ofs_res, bitmap, 0);
	if (fid2 != filedir.did) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED FPGetFileDirParams, FPResolveID id differ %x %x\n", fid2, filedir.did);
		}
		failed_nomsg();
	}

	/* rename name1 --> tmp */
	sprintf(temp, "%s/%s/%s", Path, dir1, name1);
	sprintf(temp1,"%s/%s/tmp", Path, dir1);
	if (!Quiet) {
		fprintf(stdout,"rename %s %s\n", temp, temp1);
	}
	if (rename(temp, temp1) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
		}
		failed_nomsg();
	}

	sprintf(temp, "%s/%s/.AppleDouble/%s", Path, dir1, name1);
	sprintf(temp1,"%s/%s/.AppleDouble/tmp", Path, dir1);
	if (!Quiet) {
		fprintf(stdout,"rename %s %s\n", temp, temp1);
	}
	if (rename(temp, temp1) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
		}
		failed_nomsg();
	}

	/* rename file2 to file1 */
	sprintf(temp, "%s/%s/%s", Path, dir1, name2);
	sprintf(temp1,"%s/%s/%s", Path, dir1, name1);
	if (!Quiet) {
		fprintf(stdout,"rename %s %s\n", temp, temp1);
	}
	if (rename(temp, temp1) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
		}
		failed_nomsg();
	}

	sprintf(temp, "%s/%s/.AppleDouble/%s", Path, dir1, name2);
	sprintf(temp1,"%s/%s/.AppleDouble/%s", Path, dir1, name1);
	if (!Quiet) {
		fprintf(stdout,"rename %s %s\n", temp, temp1);
	}
	if (rename(temp, temp1) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
		}
		failed_nomsg();
	}

	/* rename tmp to file2 */
	sprintf(temp, "%s/%s/%s", Path, dir1, name2);
	sprintf(temp1,"%s/%s/tmp", Path, dir1);
	if (!Quiet) {
		fprintf(stdout,"rename %s %s\n", temp1, temp);
	}
	if (rename(temp1, temp) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp1, temp, strerror(errno));
		}
		failed_nomsg();
	}

	sprintf(temp, "%s/%s/.AppleDouble/%s", Path, dir1, name2);
	sprintf(temp1,"%s/%s/.AppleDouble/tmp", Path, dir1);
	if (!Quiet) {
		fprintf(stdout,"rename %s %s\n", temp1, temp);
	}
	if (rename(temp1, temp) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp1, temp, strerror(errno));
		}
		failed_nomsg();
	}

	/* check name1 has name2 id */
	if (FPGetFileDirParams(Conn, vol,  dir , name1, bitmap,0)) {
		failed();
		goto fin;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	nfid1 = filedir.did;
	if (fid2 != filedir.did) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED FPGetFileDirParams id differ %x %x\n", fid2, filedir.did);
		}
		failed_nomsg();
	}

	FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
	afp_filedir_unpack(&filedir, dsi->data +ofs_res, bitmap, 0);
	if (nfid1 != filedir.did) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED FPGetFileDirParams, FPResolveID id differ %x %x\n", nfid1, filedir.did);
		}
		failed_nomsg();
	}

	/* check name2 has name1 id */
	if (FPGetFileDirParams(Conn, vol,  dir , name2, bitmap,0)) {
		failed();
		goto fin;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	nfid2 = filedir.did;
	if (fid1 != filedir.did) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED FPGetFileDirParams id differ %x %x\n", fid1, filedir.did);
		}
		failed_nomsg();
	}

	FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
	afp_filedir_unpack(&filedir, dsi->data +ofs_res, bitmap, 0);
	if (nfid2 != filedir.did) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED FPGetFileDirParams, FPResolveID id differ %x %x\n", nfid2, filedir.did);
		}
		failed_nomsg();
	}

	if (!Quiet) {
		fprintf(stdout,"file %s old %x  new %x\n", name1, fid1, nfid1);
		fprintf(stdout,"file %s old %x  new %x\n", name2, fid2, nfid2);
	}

fin:
	FAIL (FPDelete(Conn, vol,  dir, name1))
	FAIL (FPDelete(Conn, vol,  dir, name2))
	FAIL (FPDelete(Conn, vol,  dir, ""))
test_exit:
	exit_test("FPResolveID:test417: Resolve ID files swapped by local fs");

}

/* -------------------------- */
STATIC void test419()
{
uint16_t vol = VolID;
int  dir;
char *name1 = "t419_file1";
char *name2 = "t419_file2";
char *dir1 = "t419_dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<DIRPBIT_FINFO);
struct afp_filedir_parms filedir;
int fid1 = 0, fid2;
int nfid1 = 0, nfid2 = 0;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , dir1))) {
		failed();
		goto test_exit;
	}

	/* create file1 */
	FAIL (FPCreateFile(Conn, vol,  0, dir , name1))
	if (FPGetFileDirParams(Conn, vol,  dir , name1, bitmap,0)) {
		failed();
		goto fin;
	}

	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	fid1 = filedir.did;
	FAIL (FPResolveID(Conn, vol, fid1, bitmap))

	/* create file2 */
	FAIL (FPCreateFile(Conn, vol,  0, dir , name2))
	if (FPGetFileDirParams(Conn, vol,  dir , name2, bitmap,0)) {
		failed();
		goto fin;
	}

	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	fid2 = filedir.did;
	FAIL (FPResolveID(Conn, vol, fid2, bitmap))

	/* rename name1 --> tmp */
	sprintf(temp, "%s/%s/%s", Path, dir1, name1);
	sprintf(temp1,"%s/%s/tmp", Path, dir1);
	if (!Quiet) {
		fprintf(stdout,"rename %s %s\n", temp, temp1);
	}
	if (rename(temp, temp1) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
		}
		failed_nomsg();
	}

	/* rename file2 to file1 */
	sprintf(temp, "%s/%s/%s", Path, dir1, name2);
	sprintf(temp1,"%s/%s/%s", Path, dir1, name1);
	if (!Quiet) {
		fprintf(stdout,"rename %s %s\n", temp, temp1);
	}
	if (rename(temp, temp1) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
		}
		failed_nomsg();
	}

	/* rename tmp to file2 */
	sprintf(temp, "%s/%s/%s", Path, dir1, name2);
	sprintf(temp1,"%s/%s/tmp", Path, dir1);
	if (!Quiet) {
		fprintf(stdout,"rename %s %s\n", temp1, temp);
	}
	if (rename(temp1, temp) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp1, temp, strerror(errno));
		}
		failed_nomsg();
	}

	/* check name1 has name2 id */
	if (FPGetFileDirParams(Conn, vol,  dir , name1, bitmap,0)) {
		failed();
		goto fin;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	nfid1 = filedir.did;
	if (fid2 != filedir.did) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED FPGetFileDirParams id differ %x %x\n", fid2, filedir.did);
		}
		failed_nomsg();
	}
	FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))

	/* check name2 has name1 id */
	if (FPGetFileDirParams(Conn, vol,  dir , name2, bitmap,0)) {
		failed();
		goto fin;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	nfid2 = filedir.did;
	if (fid1 != filedir.did) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED FPGetFileDirParams id differ %x %x\n", fid1, filedir.did);
		}
		failed_nomsg();
	}
	FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))

	if (!Quiet) {
		fprintf(stdout,"file %s old %x  new %x\n", name1, fid1, nfid1);
		fprintf(stdout,"file %s old %x  new %x\n", name2, fid2, nfid2);
	}

fin:
	FAIL (FPDelete(Conn, vol,  dir, name1))
	FAIL (FPDelete(Conn, vol,  dir, name2))
	FAIL (FPDelete(Conn, vol,  dir, ""))
test_exit:
	exit_test("FPResolveID:test419: Resolve ID files swapped by local fs but not their resource forks");
}



/* ----------- */
void FPResolveID_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPResolveID page 252\n");
    fprintf(stdout,"-------------------\n");
	test417();
	test419();
}
