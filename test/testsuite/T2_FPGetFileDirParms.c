/* ----------------------------------------------
*/

#include "specs.h"
#include "adoublehelper.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static char temp[MAXPATHLEN];
static char temp1[MAXPATHLEN];

STATIC void test32()
{
char *name = "t32 dir";
char *name1 = "t32 file";
int  dir,dir1;
int  ret;
uint16_t vol = VolID;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}
	/* so FPEnumerate doesn't return NOOBJ */

	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1))
	FAIL (FPCreateFile(Conn, vol,  0, dir , name1))

	if (FPEnumerate(Conn, vol,  dir, "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
		goto fin;
	}

	FAIL (FPDelete(Conn, vol,  dir , name1))
	if (Mac) {
		if (FPDelete(Conn, vol,  dir , "")) {
			failed();
		}
	}
	else if (delete_unix_dir(Path, name)) {
		failed();
		goto fin;
	}
	/* our curdir is in the deleted folder so no error!
	   or it's a nfs exported volume
	*/
	ret = FPGetFileDirParams(Conn, vol,  dir, "",
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS));

	if (not_valid(ret, AFPERR_NOOBJ, 0)) {
		failed();
	}

	if (FPEnumerate(Conn, vol,  DIRDID_ROOT, "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

	if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol,  dir, "",
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

	dir1  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir1) {
		failed();
		goto fin;
	}
	if (FPGetFileDirParams(Conn, vol,  dir1, "",
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

    /* dir and dir1 should be != but if inode reused they are the same */
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))
test_exit:
	exit_test("FPGetFileDirParms:test32: dir deleted by someone else, access with ID");
}

/* ------------------------- */
STATIC void test33()
{
char *name = "t33 dir";
char *name1 = "t33 file";
int  dir,dir1;
uint16_t vol = VolID;
int ret;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}

	/* so FPEnumerate doesn't return NOOBJ */
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1))
	FAIL (FPCreateFile(Conn, vol,  0, dir , name1))

	if (FPEnumerate(Conn, vol, DIRDID_ROOT, name,
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
		goto fin;
	}
	FAIL (FPDelete(Conn, vol,  dir , name1))
	if (Mac) {
		FAIL (FPDelete(Conn, vol,  dir , ""))
	}
	else if (delete_unix_dir(Path, name)) {
		failed();
		goto fin;
	}
	/* our curdir is in the deleted folder so no error! */
	ret = FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name,
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS));

	if (not_valid(ret, AFPERR_NOOBJ, 0)) {
		failed();
	}

	if (FPEnumerate(Conn, vol,  DIRDID_ROOT, "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

	if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name,
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

	dir1  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir1) {
		failed();
		goto fin;
	}
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, name,
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

fin:
    /* dir and dir1 should be != but if inode reused they are the same */
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPGetFileDirParms:test33: dir deleted by someone else, access with name");
}

/* ------------------------- */
STATIC void test42()
{
char *name = "t42 dir";
char *name1 = "t42 dir1";
int  dir,dir1;
uint16_t vol = VolID;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}
	dir1  = FPCreateDir(Conn,vol, DIRDID_ROOT , name1);
	if (!dir) {
		nottested();
		goto fin;
	}

	if (ntohl(AFPERR_NOOBJ) != FPEnumerate(Conn, vol,  dir, "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol,  dir1, "", 0,
			(1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS) | (1<<DIRPBIT_OFFCNT)))
	{
		failed();
	}

	if (Mac) {
		FAIL (FPDelete(Conn, vol,  dir , ""))
	}
	else if (delete_unix_dir(Path, name)) {
		failed();
		goto fin;
	}

	/* our curdir is in the deleted folder so no error! */
	if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol,  dir, "bar",
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		failed();
	}

fin:
	FAIL (dir && FPDelete(Conn, vol,  dir, ""))
	FAIL (dir1 && FPDelete(Conn, vol,  dir1, ""))
test_exit:
	exit_test("FPGetFileDirParms:test42: dir deleted by someone else, access with ID from another dir");
}

/* -------------------------- */
// FIXME
STATIC void test52()
{
uint16_t vol = VolID;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}
	if (!FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "new/.invisible",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
	         0
		))
	{
		failed();
	}
test_exit:
	exit_test("FPGetFileDirParms:test52: test .file without AppleDouble");
}

/* --------------------- */
STATIC void test106()
{
char *name1 = "t104 dir1";
char *name2 = "t104 dir2";
char *name3 = "t104 dir3";
char *name4 = "t104 dir4";
char *name5 = "t104 file";
char *name6 = "t104 dir2_1";
uint16_t vol = VolID;
DSI *dsi = &Conn->dsi;
unsigned int  dir1, dir2, dir3, dir4;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME);

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir1 = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		nottested();
		goto test_exit;
	}
	if (!(dir2 = FPCreateDir(Conn,vol, dir1 , name2))) {
		nottested();
		goto fin;
	}

	if (FPCreateFile(Conn, vol,  0, dir2 , name5)) {
		nottested();
		goto fin;
	}

	if (!(dir3 = FPCreateDir(Conn,vol, dir2, name3))) {
		nottested();
		goto fin;
	}

	if (!(dir4 = FPCreateDir(Conn,vol, dir3, name4))) {
		nottested();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol, dir3, "", 0, bitmap)) {
		failed();
		goto fin;
	}

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);

	if (filedir.did != dir3) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n",filedir.did, dir3 );
		}
		failed_nomsg();
	}
	if (strcmp(filedir.lname, name3)) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, name3);
		}
		failed_nomsg();
	}

    sleep(1);
    if (!Mac) {
		sprintf(temp, "%s/t104 dir1/t104 dir2/t104 dir2_1", Path);
		if (!Quiet) {
			fprintf(stdout, "mkdir(%s)\n", temp);
		}
		if (mkdir(temp, 0777)) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED mkdir %s %s\n", temp, strerror(errno));
			}
			failed_nomsg();
		}
	}
	else if (!(FPCreateDir(Conn,vol, dir2, name6))) {
		nottested();
	}

    bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME)|(1<< DIRPBIT_OFFCNT);
	if (FPGetFileDirParams(Conn, vol, dir3, "t104 dir4///t104 dir2_1//", 0, bitmap)) {
		failed();
	}
	else {
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (filedir.did != dir2) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED %x should be %x\n",filedir.did, dir2 );
			}
			failed_nomsg();
		}
		if (strcmp(filedir.lname, name2)) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, name2);
			}
			failed_nomsg();
		}
		if (filedir.offcnt != 3) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED %d\n",filedir.offcnt);
			}
			failed_nomsg();
		}
	}

	if (FPGetFileDirParams(Conn, vol, dir3, "t104 dir4/", 0, bitmap)) {
			failed();
	}
	else {
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (filedir.did != dir4) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED %x should be %x\n",filedir.did, dir4 );
			}
			failed_nomsg();
		}
		if (strcmp(filedir.lname, name4)) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, name4);
			}
			failed_nomsg();
		}
	}
fin:
	FAIL (FPDelete(Conn, vol,  dir3 , name4))
	FAIL (FPDelete(Conn, vol,  dir2 , name3))
	FAIL (FPDelete(Conn, vol,  dir2 , name5))
	FAIL (FPDelete(Conn, vol,  dir2 , name6))

	FAIL (FPDelete(Conn, vol,  dir1 , name2))
	FAIL (FPDelete(Conn, vol,  dir1 , ""))
test_exit:
	exit_test("FPGetFileDirParms:test106: cname with trailing 0 and chdir");
}

/* ------------------------- */
STATIC void test127()
{
char *name  = "t127 smb afp dir1";
char *name1 = "t127 dir1_1";
int  dir1,dir;

uint16_t bitmap = (1<<FILPBIT_FNUM );
uint16_t vol = VolID;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}
	if (!(dir1 = FPCreateDir(Conn, vol, dir , name1))) {
		failed();
	}

	if (!Mac) {
		sprintf(temp, "%s/%s", name, name1);
		delete_unix_dir(Path, temp);
	}
	else {
		FAIL (FPDelete(Conn,vol, dir1,""))
	}


	FAIL (FPCloseVol(Conn,vol))

	vol  = VolID = FPOpenVol(Conn, Vol);
	if (vol == 0xffff) {
		failed();
	}
	if (FPDelete(Conn,vol, dir,"")) {
		failed();
		FAIL (FPDelete(Conn,vol, DIRDID_ROOT , name))
	}
    bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME)|(1<< DIRPBIT_OFFCNT);

	FAIL (htonl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, dir1, "", 0, bitmap))
test_exit:
	exit_test("FPGetFileDirParms:test127: dir removed with cnid not updated");
}

/* ------------------------- */
STATIC void test128()
{
char *name  = "t128 smb afp dir1";
char *name1 = "t128 dir1_1";
int  dir1,dir;

uint16_t bitmap = (1<<FILPBIT_FNUM );
uint16_t vol = VolID;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT , name))) {
		nottested();
		goto test_exit;
	}
	if (!(dir1 = FPCreateDir(Conn, vol, dir , name1))) {
		failed();
	}

	if (!Mac) {
		sprintf(temp, "%s/%s", name, name1);
		delete_unix_dir(Path, temp);
	}
	else {
		FAIL (FPDelete(Conn,vol, dir1,""))
	}

	FAIL (FPDelete(Conn,vol, dir,""))

	FAIL (FPCloseVol(Conn,vol))
	vol  = VolID = FPOpenVol(Conn, Vol);
	if (vol == 0xffff) {
		failed();
	}
    bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME)|(1<< DIRPBIT_OFFCNT);

	FAIL (htonl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, dir1, "", 0, bitmap))
	FAIL (!FPDelete(Conn,vol, dir,""))
test_exit:
	exit_test("FPGetFileDirParms:test128: dir removed with cnid not updated");
}

/* ------------------------- */
STATIC void test182()
{
char *name = "t182 Contents";
char *name1 = "t182 foo";
int  dir,dir1;
int  dir2;
uint16_t vol = VolID;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}

	dir1  = FPCreateDir(Conn,vol, dir , name);
	if (!dir) {
		failed();
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name1))

	if (FPEnumerate(Conn, vol,  dir, "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

	FAIL (FPDelete(Conn, vol,  dir , name1))
	if (!Mac) {
		sprintf(temp,"%s/%s", name, name);
		if (delete_unix_dir(Path, temp)) {
			failed();
		}
		else if (delete_unix_dir(Path, name)) {
			failed();
		}
	}
	else {
		FAIL (FPDelete(Conn, vol,  dir1 , ""))
		FAIL (FPDelete(Conn, vol,  dir , ""))
	}

	FAIL (FPCloseVol(Conn,vol))

	vol = VolID = FPOpenVol(Conn, Vol);
	if (vol == 0xffff) {
		nottested();
		goto test_exit;
	}
	if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol,  dir, "",
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

	dir2  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir2) {
		failed();
	}
	if (FPGetFileDirParams(Conn, vol,  dir2, "", 0,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

    /* dir and dir1 should be != but if inode reused they are the same */
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPGetFileDirParms:test182: dir deleted by someone else, access with ID (dirlookup)");
}

/* ------------------------- */
STATIC void test235()
{
char *name = "t235 dir";
char *name1 = "t235 file";
char *name2 = "t235 file1";
int  dir;
uint16_t vol = VolID;
uint32_t id,id1;
int fd;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
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
	    /* so it doesn't reuse the same inode */
//        sleep(2); /* FIXME: Ensure ctimes differ, this circumvents dircache caching which only has second granularity */
		sprintf(temp,"%s/%s/%s", Path, name, name2);
		fd = open(temp, O_RDWR | O_CREAT, 0666);
		if (fd < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to create %s :%s\n", temp, strerror(errno));
			}
			failed_nomsg();
			goto fin;
		}
		close(fd);
		delete_unix_file(Path, name, name1);

		sprintf(temp1,"%s/%s/%s", Path, name, name1);
		if (rename(temp, temp1) < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
			}
			failed_nomsg();
		}
	}
	else {
		FAIL (FPDelete(Conn, vol,  dir , name))
		FAIL (FPCreateFile(Conn, vol,  0, dir , name1))
	}
	id1 = get_fid(Conn, vol, dir , name1);
	if (id == id1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED ids are the same: %u/%u\n", ntohl(id), ntohl(id1));
		}
		failed_nomsg();
	}
fin:
	FAIL (FPDelete(Conn, vol,  dir , name1))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("FPGetFileDirParms:test235: file deleted and recreated by someone else, cnid not updated");
}

/* ------------------------- */
STATIC void test336()
{
char *name = "t336 very long dirname (more than 31 bytes)";
char *ndir = "t336 dir";
uint16_t vol = VolID;
DSI *dsi;
unsigned int  dir;
uint16_t bitmap = 0;
int ret;
int id;
uint16_t vol2;
DSI *dsi2;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		nottested();
		goto test_exit;
	}

	id = FPCreateDir(Conn2, vol2, DIRDID_ROOT, name);
	if (!id) {
		nottested();
		goto test_exit;
	}
	FPCloseVol(Conn2,vol2);

	dir  = FPCreateDir(Conn, vol, DIRDID_ROOT , ndir);
	if (!dir) {
		nottested();
		goto fin;
	}
	if (Conn->afp_version >= 30) {
		bitmap = (1<<FILPBIT_PDINFO);
	}
	else {
		bitmap = (1<<DIRPBIT_LNAME);
	}

	sprintf(temp1,"#%X",ntohl(id));
	memset(temp, 0, sizeof(temp));
	strncpy(temp, name, 31 - strlen(temp1));
	strcat(temp, temp1);
	ret = FPGetFileDirParams(Conn, vol, DIRDID_ROOT, temp, 0, bitmap);
#if 0
	/* for afp3 it's not valid mangled filename
	 * changed
	*/
	if ((Conn->afp_version >= 30 && ret != ntohl(AFPERR_NOOBJ))
	    || ( Conn->afp_version < 30 && ret)) {
		failed();
	}
#else
	if (ret) {
		failed();
	}

#endif
	ret = FPCreateDir(Conn, vol, dir, temp);
	if (!ret || ret != get_did(Conn, vol, dir, temp)) {
		failed();
	}
	if (Path) {
	    struct stat st;

		sprintf(temp1, "%s/%s/%s", Path, ndir, temp);
		if (stat(temp1, &st)) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED stat( %s ) %s\n", temp1, strerror(errno));
			}
			failed_nomsg();
		}
	}

	bitmap |= (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID);
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
		nottested();
	}
	if (FPGetFileDirParams(Conn, vol, dir, temp, 0, bitmap)) {
		nottested();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (filedir.pdid != ntohl(dir)) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED %x should be %x\n", filedir.pdid, ntohl(dir) );
			}
			failed_nomsg();
		}
	}

	FAIL (FPDelete(Conn, vol,  dir, temp))
	FAIL (FPDelete(Conn, vol,  dir, ""))
fin:
	FAIL (FPDelete(Conn, vol,  id, ""))
test_exit:
	exit_test("FPGetFileDirParms:test336: long dirname >31 bytes");
}

/* ----------------------- */
STATIC void test340()
{
char *name = "t340 dir";
char *name1 = "t340 file";
int  dir,dir1;
int  ret;
uint16_t vol = VolID;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}
	/* so FPEnumerate doesn't return NOOBJ */
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1))
	FAIL (FPCreateFile(Conn, vol,  0, dir , name1))

	if (FPEnumerate(Conn, vol,  dir, "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
		goto fin;
	}

	FAIL (FPDelete(Conn, vol,  dir , name1))
	if (Mac) {
		if (FPDelete(Conn, vol,  dir , "")) {
			failed();
		}
	}
	else if (delete_unix_dir(Path, name)) {
		failed();
		goto fin;
	}
	/* our curdir is in the deleted folder so no error!
	   or it's a nfs exported volume
	*/
	ret = FPGetFileDirParams(Conn, vol,  dir, "",
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS));

	if (ret != ntohl(AFPERR_NOOBJ)) {
		failed();
	}

	ret = FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name,
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS));

	if (ret != ntohl(AFPERR_NOOBJ)) {
		failed();
	}

	if (FPEnumerate(Conn, vol,  DIRDID_ROOT, "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

	if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol,  dir, "",
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

	dir1  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir1) {
		failed();
		goto fin;
	}
	if (FPGetFileDirParams(Conn, vol,  dir1, "",
	         0
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

    /* dir and dir1 should be != but if inode reused they are the same */
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))
test_exit:
	exit_test("FPGetFileDirParms:test340: dir deleted by someone else, access with ID");
}

/* -------------------------- */
STATIC void test420()
{
uint16_t vol = VolID;
int  dir;
char *name  = "t420 file";
char *name2 = "t420 file new name";
char *name1 = "t420 dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM ) | (1<<DIRPBIT_FINFO);
struct afp_filedir_parms filedir;
int fid = 0;
int fork = 0;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	if (!Mac && Path[0] == '\0') {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		failed();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap , dir, name, OPENACC_WR |OPENACC_RD|OPENACC_DWR| OPENACC_DRD);
	if (!fork) {
		failed();
		goto fin;
	}
	if (FPByteLock(Conn, fork, 0, 0 /* set */, 0, 100)) {
		failed();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
		goto fin;
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		fid = filedir.did;
		FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
	}
	if (!Mac) {
		if (rename_unix_file(Path, name1, name, name2) < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", name, name2, strerror(errno));
			}
			failed_nomsg();
		}
	}
	else {
		FAIL (FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, name2))
	}
	if (FPGetFileDirParams(Conn, vol,  dir , name2, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		if (fid != filedir.did) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED FPGetFileDirParams id differ %x %x\n", fid, filedir.did);
			}
			failed_nomsg();

		}
		else {
			FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
		}
	}

fin:
	FAIL (fork && FPCloseFork(Conn,fork))
	FAIL (FPDelete(Conn, vol,  dir, name2))
	FPDelete(Conn, vol,  dir, name);
	FAIL (FPDelete(Conn, vol,  dir, ""))
test_exit:
	exit_test("FPGetFileDirParms:test420: FPGetFileDirParms an open file is renamed with local fs");
}


/* ----------- */
void T2FPGetFileDirParms_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
	test32();
	test33();
	test42();
#if 0
	test52();
#endif
	test106();
	test127();
	test128();
	test182();
	test235();
	test336();
	test340();
	test420();
}
