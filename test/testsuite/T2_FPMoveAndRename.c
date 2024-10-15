/* ----------------------------------------------
*/
#include "specs.h"
#include "adoublehelper.h"

static char temp[MAXPATHLEN];   
static char temp1[MAXPATHLEN];

STATIC void test136()
{
int dir;
char *name  = "t136 move, rename ";
char *name1 = "t136 dir/new file name";
char *name2 = "new file name";
char *ndir  = "t136 dir";
uint16_t vol = VolID;
unsigned int ret;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPMoveAndRename:test136: move and rename in a dir without .AppleDouble\n");

	if (!Path && !Mac) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	if (!Mac) {
		sprintf(temp, "%s/%s", Path, ndir);
		fprintf(stdout, "mkdir(%s)\n", temp);
		if (mkdir(temp, 0777)) {
			fprintf(stdout,"\tFAILED mkdir %s %s\n", temp, strerror(errno));
			failed_nomsg();
		}
	}
	else {
		dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir);
		if (!dir) {
			failed();
		}
	}

	ret = FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1);
	if (not_valid(ret, AFPERR_MISC, AFPERR_PARAM)) {
		failed();
	}
	
	dir = get_did(Conn, vol, DIRDID_ROOT, ndir);

	FAIL (FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, name2)) 
	
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))
	FAIL (!FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  dir, ""))
test_exit:
	exit_test("test136");
}

/* ----------------------- */
STATIC void test137()
{
int fork;
int dir;
uint16_t bitmap = 0;
char *name  = "t137 move, rename ";
char *name1 = "t137 dir/new file name";
char *name2 = "new file name";
char *ndir  = "t137 dir";
uint16_t vol = VolID;
unsigned int ret;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPMoveAndRename:test137: move and rename open file in dir without .AppleDouble\n");

	if (!Path && !Mac) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	if (!Mac) {
		sprintf(temp, "%s/%s", Path, ndir);
		fprintf(stdout, "mkdir(%s)\n", temp);
		if (mkdir(temp, 0777)) {
			fprintf(stdout,"\tFAILED mkdir %s %s\n", temp, strerror(errno));
			failed_nomsg();
		}
	}
	else {
		dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir);
		if (!dir) {
			failed();
		}
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
	}
	ret = FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1);
	if (not_valid(ret, AFPERR_MISC, AFPERR_PARAM)) {
		failed();
	}
	
	dir = get_did(Conn, vol, DIRDID_ROOT, ndir);

	FAIL (FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, name2)) 
	
	FAIL (fork && FPCloseFork(Conn,fork)) 

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))
	FAIL (!FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  dir, ""))
test_exit:
	exit_test("test137");
}

/* -------------------------- */
STATIC void test139()
{
int  dir;
char *name = "t139 file";
char *name1 = "t139 dir";
uint16_t vol = VolID;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPMoveAndRename:test139: Move And Rename \n");

	if (!Path && !Mac) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		nottested();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) 
	
	if (!Mac) {
		delete_unix_md(Path,"", name);
	}
	FAIL (FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, "")) 

	FAIL (FPDelete(Conn, vol,  dir , name))
	FAIL (!FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  dir , ""))
test_exit:
	exit_test("test139");
}

/* ------------------------- */
STATIC void test323()
{
char *name  = "t323 dir";
char *name1 = "t323 dir1";
char *file = "t323 file";
int  dir1,dir;

uint16_t bitmap = (1<<FILPBIT_FNUM );
uint16_t vol = VolID;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPGetFileDirParms:test323: file moved with cnid not updated\n");

	if (!Path && !Mac) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}
	if (!(dir1 = FPCreateDir(Conn, vol, DIRDID_ROOT , name1))) {
		failed();
		goto fin;
	}
	FAIL (FPCreateFile(Conn, vol,  0, dir , file))
	FAIL (FPCreateFile(Conn, vol,  0, dir1, file))
    bitmap = (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM );
	FAIL (FPGetFileDirParams(Conn, vol, dir, file, bitmap, 0)) 
	FAIL (FPGetFileDirParams(Conn, vol, dir1, file, bitmap, 0)) 

	if (!Mac) {
		sprintf(temp,"%s/%s/%s", Path, name, file);
		sprintf(temp1,"%s/%s/%s", Path, name1, file);
		fprintf (stdout, "rename %s --> %s\n", temp, temp1);
		if (rename(temp, temp1) < 0) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
			failed_nomsg();
		}
		
	}
	else {
		FAIL (FPMoveAndRename(Conn, vol, dir, dir1, file, file))
	}
	
    bitmap = (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM );

	FAIL (FPGetFileDirParams(Conn, vol, dir1, file, bitmap, 0)) 

fin:
	FAIL (FPDelete(Conn,vol, dir1,file))
	FPDelete(Conn,vol, dir,file);
	FAIL (FPDelete(Conn,vol, dir,""))
	FAIL (FPDelete(Conn,vol, dir1,""))
test_exit:
	exit_test("test323");
}

/* ------------------------- */
STATIC void test365()
{
char *name  = "t365 in";
char *name1 = "t365 out";
char *file = "t365 file";
int  dir1,dir;

uint16_t bitmap = (1<<FILPBIT_FNUM );
uint16_t vol = VolID;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPGetFileDirParms:test365: file moved with cnid not updated\n");

	if (!Path && !Mac) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}
	if (!(dir1 = FPCreateDir(Conn, vol, DIRDID_ROOT , name1))) {
		failed();
		goto fin;
	}
	FAIL (FPCreateFile(Conn, vol,  0, dir , file))
    bitmap = (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM );
	FAIL (FPGetFileDirParams(Conn, vol, dir, file, bitmap, 0)) 

	if (!Mac) {
		sprintf(temp,"%s/%s/%s", Path, name, file);
		sprintf(temp1,"%s/%s/%s", Path, name1, file);
		fprintf (stdout, "rename %s --> %s\n", temp, temp1);
		if (rename(temp, temp1) < 0) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
			failed_nomsg();
		}

        if (adouble == AD_V2) {
            sprintf(temp,"%s/%s/.AppleDouble/%s", Path, name, file);
            sprintf(temp1,"%s/%s/.AppleDouble/%s", Path, name1, file);
            fprintf (stdout, "rename %s --> %s\n", temp, temp1);
            if (rename(temp, temp1) < 0) {
                fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
                failed_nomsg();
            }
        }
	}
	else {
		FAIL (FPMoveAndRename(Conn, vol, dir, dir1, file, file))
	}
	
    bitmap = (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM );

	FAIL (FPGetFileDirParams(Conn, vol, dir1, file, bitmap, 0)) 

fin:
	FAIL (FPDelete(Conn,vol, dir1,file))
	FPDelete(Conn,vol, dir,file);
	FAIL (FPDelete(Conn,vol, dir,""))
	FAIL (FPDelete(Conn,vol, dir1,""))
test_exit:
	exit_test("test365");
}

/* ----------- */
void FPMoveAndRename_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPMoveAndRename page 223\n");
    test136();
    test137();
    test139();
    test323();
    test365();
}

