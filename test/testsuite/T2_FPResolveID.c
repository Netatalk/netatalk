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
STATIC void test129()
{
int  dir;
uint16_t vol = VolID;
char *name = "t129 Resolve ID file";
char *name1 = "t129 Resolve ID dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		failed();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		FAIL ((FPResolveID(Conn, vol, filedir.did, bitmap)))
	}

    if (adouble == AD_V2) {
        sprintf(temp1, "%s/%s/.AppleDouble/%s", Path, name1, name);
        if (unlink(temp1) <0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unlink %s %s\n", temp, strerror(errno));
		}
            failed_nomsg();
        }
    }
	sprintf(temp1, "%s/%s/%s", Path, name1, name);
	if (unlink(temp1) <0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unlink %s %s\n", temp, strerror(errno));
		}
		failed_nomsg();
	}

	if (delete_unix_dir(Path, name1)) {
		failed();
	}

	FAIL (ntohl(AFPERR_NOID ) != FPResolveID(Conn, vol, filedir.did, bitmap))
	FPCloseVol(Conn,vol);
	vol  = FPOpenVol(Conn, Vol);
	FAIL (ntohl(AFPERR_NOID ) != FPResolveID(Conn, vol, filedir.did, bitmap))
test_exit:
	exit_test("FPResolveID:test129: Resolve ID");
}

/* -------------------------- */
STATIC void test130()
{
uint16_t vol = VolID;
int  dir;
char *name = "t130 Delete ID file";
char *name1 = "t130 Delete ID dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;
int ret;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		failed();
		goto test_exit;
	}

	FAIL (ntohl(AFPERR_BADTYPE ) != FPDeleteID(Conn, vol, dir))

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
	}

    if (adouble == AD_V2) {
        sprintf(temp1, "%s/%s/.AppleDouble/%s", Path, name1, name);
        if (unlink(temp1) <0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unlink %s %s\n", temp, strerror(errno));
		}
            failed_nomsg();
        }
    }
	sprintf(temp1, "%s/%s/%s", Path, name1, name);
	if (unlink(temp1) <0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unlink %s %s\n", temp, strerror(errno));
		}
		failed_nomsg();
	}

	if (delete_unix_dir(Path, name1)) {
		failed();
	}

	FPCloseVol(Conn,vol);
	vol  = FPOpenVol(Conn, Vol);

    ret = FPDeleteID(Conn, vol, filedir.did);
	if (not_valid_bitmap(ret, BITERR_PARAM | BITERR_NOOBJ, AFPERR_PARAM)) {
		failed();
	}

test_exit:
	exit_test("FPResolveID:test130: Delete ID");
}

/* -------------------------- */
STATIC void test131()
{
uint16_t vol = VolID;
int  dir;
char *name = "t131 Delete ID file";
char *name1 = "t131 Delete ID dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		failed();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
	}
    if (adouble == AD_V2) {
        sprintf(temp1, "%s/%s/.AppleDouble/%s", Path, name1, name);
        if (unlink(temp1) <0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unlink %s %s\n", temp, strerror(errno));
		}
            failed_nomsg();
        }
    }
	sprintf(temp1, "%s/%s/%s", Path, name1, name);
	if (unlink(temp1) <0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unlink %s %s\n", temp, strerror(errno));
		}
		failed_nomsg();
	}

	if (delete_unix_dir(Path, name1)) {
		failed();
	}

	FAIL (ntohl(AFPERR_NOOBJ ) != FPDeleteID(Conn, vol, filedir.did))
	FPCloseVol(Conn,vol);
	vol  = FPOpenVol(Conn, Vol);
	FAIL (ntohl(AFPERR_NOID ) != FPDeleteID(Conn, vol, filedir.did))
test_exit:
	exit_test("FPResolveID:test131: Resolve ID");
}


/* -------------------------- */
STATIC void test331()
{
uint16_t vol = VolID;
int  dir;
char *name  = "t331 file";
char *name2 = "t331 file new name";
char *name1 = "t331 dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM ) | (1<<DIRPBIT_FINFO);
struct afp_filedir_parms filedir;
int fid = 0;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		failed();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		fid = filedir.did;
		FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
	}
	if (!Mac) {
		sprintf(temp, "%s/%s/%s", Path, name1, name);
		sprintf(temp1,"%s/%s/%s", Path, name1, name2);
		if (!Quiet) {
			fprintf(stdout,"rename %s %s\n", temp, temp1);
		}
		if (rename(temp, temp1) < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
			}
			failed_nomsg();
		}

        if (adouble == AD_V2) {
            sprintf(temp, "%s/%s/.AppleDouble/%s", Path, name1, name);
            sprintf(temp1,"%s/%s/.AppleDouble/%s", Path, name1, name2);
		if (!Quiet) {
			fprintf(stdout,"rename %s %s\n", temp, temp1);
		}
            if (rename(temp, temp1) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
		}
                failed_nomsg();
            }
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
	FAIL (FPDelete(Conn, vol,  dir, name2))
	FPDelete(Conn, vol,  dir, name);
	FAIL (FPDelete(Conn, vol,  dir, ""))
test_exit:
	exit_test("FPResolveID:test331: Resolve ID file modified with local fs");

}

/* -------------------------- */
static int get_fs_lock(char *folder, char *file)
{
    int fd;
    struct flock lock;
    int ret;

    if (adouble == AD_V2)
        sprintf(temp, "%s/%s/.AppleDouble/%s", Path, folder, file);
    else
        sprintf(temp, "%s/%s/%s", Path, folder, file);

	if (!Quiet) {
		fprintf(stdout," \n---------------------\n");
		fprintf(stdout, "open(\"%s\", O_RDWR)\n", temp);
	}
	fd = open(temp, O_RDWR, 0);
	if (fd >= 0) {
		lock.l_start = 0;		/* after meta data */
    	lock.l_type = F_WRLCK;
	    lock.l_whence = SEEK_SET;
    	lock.l_len = 0;

		if (!Quiet) {
			fprintf(stdout, "fcntl(1024)\n");
		}
    	if ((ret = fcntl(fd, F_SETLK, &lock)) >= 0 || (errno != EACCES && errno != EAGAIN)) {
    		if (!ret >= 0)
    	    	errno = 0;
		if (!Quiet) {
			perror("fcntl ");
			fprintf(stdout,"\tFAILED\n");
		}
			failed_nomsg();
    	}
    	fcntl(fd, F_UNLCK, &lock);
    	close(fd);
    	return 0;
    }
    else {
	if (!Quiet) {
		perror("open ");
		fprintf(stdout,"\tFAILED\n");
	}
		failed_nomsg();
    }
    return -1;
}

/* -------------------------- */
STATIC void test360()
{
uint16_t vol = VolID;
int  dir;
char *name  = "t360 file";
char *name2 = "t360 file new name";
char *name1 = "t360 dir";
char *name3 = "t360 open file";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM ) | (1<<DIRPBIT_FINFO);
struct afp_filedir_parms filedir;
int fid = 0;
int fork = 0;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (Locking) {
		test_skipped(T_LOCKING);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		failed();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))
	FAIL (FPCreateFile(Conn, vol,  0, dir , name3))

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS, bitmap , dir, name3, OPENACC_WR |OPENACC_RD|OPENACC_DWR| OPENACC_DRD);
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

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		fid = filedir.did;
		FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
	}
	if (!Mac) {
		sprintf(temp, "%s/%s/%s", Path, name1, name);
		sprintf(temp1,"%s/%s/%s", Path, name1, name2);
		if (!Quiet) {
			fprintf(stdout,"rename %s %s\n", temp, temp1);
		}
		if (rename(temp, temp1) < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
			}
			failed_nomsg();
		}

        if (adouble == AD_V2) {
            sprintf(temp, "%s/%s/.AppleDouble/%s", Path, name1, name);
            sprintf(temp1,"%s/%s/.AppleDouble/%s", Path, name1, name2);
		if (!Quiet) {
			fprintf(stdout,"rename %s %s\n", temp, temp1);
		}
            if (rename(temp, temp1) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
		}
                failed_nomsg();
            }
            if (get_fs_lock(name1, name3) < 0) {
                goto fin;
            }
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
	if (!Mac) {
		if (get_fs_lock(name1, name3) < 0) {
			goto fin;
		}
	}

fin:
	FAIL (fork && FPCloseFork(Conn,fork))
	FAIL (FPDelete(Conn, vol,  dir, name3))
	FAIL (FPDelete(Conn, vol,  dir, name2))
	FPDelete(Conn, vol,  dir, name);
	FAIL (FPDelete(Conn, vol,  dir, ""))
test_exit:
	exit_test("FPResolveID:test360: Resolve ID file modified with local fs and a file is opened");

}

/* -------------------------- */
STATIC void test397()
{
uint16_t vol = VolID;
char *name = "t397 Resolve ID file";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	if (!Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		FAIL ((FPResolveID(Conn, vol, filedir.did, bitmap)))
	}

    if (adouble == AD_V2) {
        sprintf(temp1, "%s/.AppleDouble/%s", Path, name);
        if (unlink(temp1) <0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unlink %s %s\n", temp, strerror(errno));
		}
            failed_nomsg();
        }
    }
	sprintf(temp1, "%s/%s", Path, name);
	if (unlink(temp1) <0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unlink %s %s\n", temp, strerror(errno));
		}
		failed_nomsg();
	}
	FAIL (ntohl(AFPERR_NOID ) != FPResolveID(Conn, vol, filedir.did, bitmap))
	FAIL (ntohl(AFPERR_NOID ) != FPResolveID(Conn, vol, filedir.did, bitmap))

test_exit:
	exit_test("FPResolveID:test397: Resolve ID file deleted local fs");
}

/* -------------------------- */
STATIC void test412()
{
uint16_t vol = VolID;
int  dir1, dir2;
char *name  = "t412 file";
char *ndir2 = "t412 dir dest";
char *ndir1 = "t412 dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM ) | (1<<DIRPBIT_FINFO);
struct afp_filedir_parms filedir;
int fid = 0;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir1 = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir1))) {
		nottested();
		goto test_exit;
	}

	if (!(dir2 = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir2))) {
		nottested();
		goto fin;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir1, name))

	if (FPGetFileDirParams(Conn, vol,  dir1, name, bitmap,0)) {
		failed();
		goto fin;
	}

	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	fid = filedir.did;
	FAIL (FPResolveID(Conn, vol, fid, bitmap))

	if (!Mac) {
		sprintf(temp, "%s/%s", Path, ndir1);
		sprintf(temp1,"%s/%s/%s", Path, ndir2, ndir1);
		if (!Quiet) {
			fprintf(stdout,"rename %s %s\n", temp, temp1);
		}
		if (rename(temp, temp1) < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
			}
			failed_nomsg();
		}
	}
	else {
		/* FIXME */
		FAIL (FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir1, name, ndir2))
	}
	FPResolveID(Conn, vol, fid, bitmap);

	if (FPEnumerate(Conn, vol,  dir2, "",
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

	if (FPGetFileDirParams(Conn, vol,  dir1 , name, bitmap,0)) {
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
	FAIL (FPDelete(Conn, vol,  dir1, name))
	FAIL (FPDelete(Conn, vol,  dir1, ""))
	FAIL (FPDelete(Conn, vol,  dir2, ""))
test_exit:
	exit_test("FPResolveID:test412: Resolve ID file modified with local fs");

}

/* -------------------------- */
STATIC void test413()
{
uint16_t vol = VolID;
int  dir,dir2;
char *name  = "t413 file";
char *name2 = "t413 dir dest";
char *name1 = "t413 dir";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM ) | (1<<DIRPBIT_FINFO);
struct afp_filedir_parms filedir;
int fid = 0;
DSI *dsi = &Conn->dsi;

	ENTER_TEST

	if (!Mac && !Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		failed();
		goto test_exit;
	}

	if (!(dir2 = FPCreateDir(Conn,vol, DIRDID_ROOT , name2))) {
		failed();
		goto fin;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name))

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
		goto fin;
	}

	if (FPGetFileDirParams(Conn, vol,  dir , name, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		fid = filedir.did;
		FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))
	}
	if (!Mac) {
		sprintf(temp, "%s/%s/%s", Path, name1, name);
		sprintf(temp1,"%s/%s/%s", Path, name2, name);
		if (!Quiet) {
			fprintf(stdout,"rename %s %s\n", temp, temp1);
		}
		if (rename(temp, temp1) < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
			}
			failed_nomsg();
		}

        if (adouble == AD_V2) {
            sprintf(temp, "%s/%s/.AppleDouble/%s", Path, name1, name);
            sprintf(temp1,"%s/%s/.AppleDouble/%s", Path, name2, name);
		if (!Quiet) {
			fprintf(stdout,"rename %s %s\n", temp, temp1);
		}
            if (rename(temp, temp1) < 0) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED unable to rename %s to %s :%s\n", temp, temp1, strerror(errno));
		}
                failed_nomsg();
            }
        }
	}
	else {
		/* FIXME */
		FAIL (FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, name2))
	}
	if (FPGetFileDirParams(Conn, vol,  dir2 , name, bitmap,0)) {
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
	FAIL (FPDelete(Conn, vol,  dir2, name))
	FPDelete(Conn, vol,  dir, name);
	FAIL (FPDelete(Conn, vol,  dir, ""))
	FAIL (FPDelete(Conn, vol,  dir2, ""))
test_exit:
	exit_test("FPResolveID:test413: Resolve ID file modified with local fs");

}

/* -------------------------- */
STATIC void test418()
{
uint16_t vol = VolID;
int  dir;
char *name1 = "t418_file1";
char *name2 = "t418_file2";
char *dir1 = "t418_dir";
char *tp = "t418_temp";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<DIRPBIT_FINFO);
struct afp_filedir_parms filedir;
int fid1 = 0, fid2;
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
	FAIL (FPRename(Conn, vol, dir, name1, tp))

	/* rename file2 to file1 */
	FAIL (FPRename(Conn, vol, dir, name2, name1))

	/* rename tmp to file2 */
	FAIL (FPRename(Conn, vol, dir, tp, name2))

	/* check name1 has name2 id */
	if (FPGetFileDirParams(Conn, vol,  dir , name1, bitmap,0)) {
		failed();
		goto fin;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
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
	if (fid1 != filedir.did) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED FPGetFileDirParams id differ %x %x\n", fid1, filedir.did);
		}
		failed_nomsg();
	}
	FAIL (FPResolveID(Conn, vol, filedir.did, bitmap))

fin:
	FAIL (FPDelete(Conn, vol,  dir, name1))
	FAIL (FPDelete(Conn, vol,  dir, name2))
	FAIL (FPDelete(Conn, vol,  dir, ""))
test_exit:
	exit_test("FPResolveID:test418: Resolve ID files name swapped with AFP rename");
}


/* ----------- */
void FPResolveID_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPResolveID page 252\n");
    fprintf(stdout,"-------------------\n");
	test129();
	test130();
	test131();
	test331();
	test360();
	test397();
	test412();
	test413();
	test418();
}
