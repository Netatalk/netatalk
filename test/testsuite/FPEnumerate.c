/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test28()
{
uint16_t vol = VolID;
char *name  = "t28 dir";
char *name1 = "t28 subdir";
char *name2 = "t28 file";
int  dir;
int  dir1;

	ENTER_TEST

	/* we need to empty the server cashe */
	FPCloseVol(Conn, vol);

	vol = VolID = FPOpenVol(Conn, Vol);
	if (vol == 0xffff) {
		nottested();
		goto test_exit;
	}

	dir   = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}

	dir1  = FPCreateDir(Conn,vol, dir , name1);
	if (!dir1) {
		nottested();
		goto fin;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir1 , name2))

	FAIL (FPCloseVol(Conn,vol))

	vol = VolID = FPOpenVol(Conn, Vol);
	if (vol == 0xffff) {
		nottested();
		goto test_exit;
	}

	if (FPEnumerate(Conn, vol,  dir1 , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		if (Mac) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED (IGNORED)\n");
			}
		}
		else {
			failed();
		}
		/* warm the cache */
		if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "", 0,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
			failed();
		}
		else if (FPEnumerate(Conn, vol,  dir, "", 0,
		     (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
			failed();
		}
	}

	FAIL (dir1 && FPDelete(Conn, vol,  dir1 , name2))
fin:
	FAIL (dir1 && FPDelete(Conn, vol,  dir1 , ""))
	FAIL (dir && FPDelete(Conn, vol,  dir, ""))
test_exit:
	exit_test("FPEnumerate:test28: test search by ID");
}

/* ----------------------- */
STATIC void test38()
{
uint16_t bitmap = (1<< DIRPBIT_DID);
int  rdir;
int  did;
uint16_t vol = VolID;
char *name = "t38 read only access dir";
char *nfile = "t38 read write file";
DSI *dsi;

	dsi = &Conn->dsi;
	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!(rdir = read_only_folder_with_file(vol, DIRDID_ROOT, name, nfile) ) ) {
		goto test_exit;
	}

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, name, 0, bitmap)) {
		failed();
		goto fin;
	}
	memcpy(&did, dsi->data +3 * sizeof( uint16_t ), sizeof(did));

	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , name,
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE) |
	         (1<<FILPBIT_DFLEN) | (1<<FILPBIT_RFLEN)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		failed();
		goto fin;
	}

	if (FPEnumerate(Conn, vol,  did , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) |  (1<<DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		failed();
		goto fin;
	}
#if 0
	bitmap = 0;
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap , did, "toto.txt",OPENACC_WR | OPENACC_RD);

	if (fork) {
		fprintf(stdout,"\tFAILED\n");
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap, did, "toto.txt", OPENACC_RD);

	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
		goto test_exit;
	}
	FPCloseFork(Conn,fork);
#endif
fin:
	delete_folder_with_file(vol, DIRDID_ROOT, name, nfile);
test_exit:
	exit_test("FPEnumerate:test38: enumerate folder with no write access");
}

/* ------------------------- */
STATIC void test34()
{
uint16_t vol = VolID;
char *name = "essai permission";

	ENTER_TEST

	if (ntohl(AFPERR_ACCESS) != FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0,
	    (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	    (1 << DIRPBIT_ACCESS)))
	{
		failed();
		goto test_exit;
	}

	FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
	     (1<< FILPBIT_LNAME) | (1<< FILPBIT_FNUM ),
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);

	if (ntohl(AFPERR_ACCESS) != FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0,
	     (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	     (1 << DIRPBIT_ACCESS))
	   )
	{
		failed();
		goto test_exit;
	}

test_exit:
	exit_test("FPEnumerate:test34: folder with --rwx-- perm");
}

/* ------------------------- */
STATIC void test40()
{
char *name  = "t40 dir";
char *name1 = "t40 file";
uint16_t vol = VolID;
unsigned int ret;
int  dir;

	ENTER_TEST

	dir   = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}

	FAIL (FPCreateFile(Conn, vol,  0, dir , name1))

	if (FPEnumerate(Conn, vol,  dir , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}

	FAIL (FPDelete(Conn, vol,  dir, name1))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))

	if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol,  dir, "", 0,
			(1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)))
	{
		failed();
	}
	ret = FPEnumerate(Conn, vol,  dir , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);
	if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NODIR, AFPERR_NODIR)) {
		failed();
	}
test_exit:
	exit_test("FPEnumerate:test40: enumerate deleted folder");

}

/* ------------------------- */
STATIC void test41()
{
int dir = 0;
uint16_t vol = VolID;
char *name  = "t41 dir";
char *name1 = "t41 dir/file";
char *name2 = "t41 dir/sub dir/foo";
char *name3 = "t41 dir/sub dir";
unsigned int ret;

	ENTER_TEST

	dir   = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1)) {
		nottested();
		goto fin;
	}

	if (FPEnumerate(Conn, vol,  dir , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		nottested();
		goto fin;
	}

	ret = FPEnumerate(Conn, vol,  DIRDID_ROOT , name2,
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);

	if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NODIR, AFPERR_NODIR)) {
		failed();
	}

	ret = FPEnumerate(Conn, vol,  DIRDID_ROOT , name3,
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);

	if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NODIR, AFPERR_NODIR)) {
		failed();
	}

	if (ntohl(AFPERR_PARAM) != FPEnumerate(Conn, vol,  0 , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		failed();
	}
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))
	FAIL (FPDelete(Conn, vol,  dir, ""))
test_exit:
	exit_test("FPEnumerate:test41: enumerate folder not there");

}

/* -------------------------------------- */
STATIC void test93()
{
char *name = "t93 bad enumerate file";
char *name1 = "t93 bad enumerate dir";
int dir = 0;
int dir1 = 0;
int ret;
uint16_t vol = VolID;

	ENTER_TEST

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		nottested();
		goto fin;
	}
	if (!(dir1 = FPCreateDir(Conn,vol, dir , name1))) {
		nottested();
		goto fin;
	}

	ret = FPDelete(Conn, vol,  DIRDID_ROOT , "");
	if (not_valid_bitmap(ret, BITERR_PARAM | BITERR_BUSY, AFPERR_ACCESS)) {
		failed();
	}

	ret = FPEnumerate(Conn, vol,  DIRDID_ROOT , name,
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);

	if (not_valid_bitmap(ret, BITERR_BADTYPE | BITERR_NODIR, AFPERR_BADTYPE)) {
		failed();
	}
	FAIL (htonl( AFPERR_DIRNEMPT) != FPDelete(Conn, vol,  DIRDID_ROOT , name1))

	FAIL (htonl(AFPERR_BITMAP) != FPEnumerate(Conn, vol,  DIRDID_ROOT , name1, 0,0))

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))

	FAIL (dir1 && FPDelete(Conn, vol,  dir , name1))
	FAIL (dir && FPDelete(Conn, vol,  DIRDID_ROOT , name1))
test_exit:
	exit_test("FPEnumerate:test93: enumerate error");
}

/* ------------------------- */
STATIC void test218()
{
uint16_t bitmap = 0;
char *base  = "t218 test dir";
char *name  = "t218 enumerate file";
char *ndir1  = "t218 enumerate dir1";
char *ndir  = "t218 enumerate dir";
uint16_t vol = VolID;
int  ofs =  4 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
DSI *dsi;
unsigned int ret;
int bdir;
int dir;
int dir1;
int isdir;

	dsi = &Conn->dsi;

	ENTER_TEST
	if (Conn->afp_version < 30) {
		test_skipped(T_AFP3);
		goto test_exit;
	}

	bdir  = FPCreateDir(Conn,vol, DIRDID_ROOT , base);
	if (!bdir) {
		nottested();
		goto test_exit;
	}

	bitmap = (1 << FILPBIT_LNAME);
	if (htonl(AFPERR_NOOBJ) != FPEnumerateFull(Conn, vol, 1, 1, 800,  bdir, "", bitmap, bitmap)) {
		nottested();
		goto fin;
	}
	if (FPCreateFile(Conn, vol,  0, bdir , name)){
		nottested();
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, bdir , ndir);
	if (!dir) {
		nottested();
		goto fin;
	}
	dir1  = FPCreateDir(Conn,vol, bdir , ndir1);
	if (!dir1) {
		nottested();
		goto fin;
	}

	FAIL (htonl(AFPERR_PARAM) != FPEnumerateFull(Conn, vol, 0, 1, 800,  bdir, "", bitmap, 0))
	FAIL (htonl(AFPERR_PARAM) != FPEnumerateFull(Conn, vol, 1, 1, 2,  bdir, "", bitmap, bitmap))

	FAIL (FPEnumerateFull(Conn, vol, 1, 5, 800,  bdir, "", bitmap, bitmap))
	FAIL (htonl(AFPERR_NOOBJ) != FPEnumerateFull(Conn, vol, 1, 0, 800,  bdir, "", bitmap, bitmap))
	FAIL (FPEnumerateFull(Conn, vol, 2, 5, 800,  bdir, "", bitmap, bitmap))
	FAIL (htonl(AFPERR_NOOBJ) != FPEnumerateFull(Conn, vol, 4, 1, 800,  bdir, "", bitmap, bitmap))
	FAIL (htonl(AFPERR_NOOBJ) != FPEnumerateFull(Conn, vol, 5, 1, 800,  bdir, "", bitmap, bitmap))

	/* get the third */
	isdir = 0;
	ret = FPEnumerateFull(Conn, VolID, 3, 1, 800,  bdir, "", bitmap, 0);
	if (htonl(AFPERR_NOOBJ) == ret) {
		isdir = 1;
		ret = FPEnumerateFull(Conn, VolID, 3, 1, 800,  bdir, "", 0,bitmap);
	}
	if (ret) {
		failed();
		goto fin;
	}
	filedir.isdir = isdir;
	if (isdir) {
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	}
	else {
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	}
	FAIL (FPDelete(Conn, vol,  bdir, filedir.lname))
	FAIL (FPEnumerateFull(Conn, vol, 2, 5, 800,  bdir, "", bitmap, bitmap))
	FAIL (FPEnumerateFull(Conn, vol, 2, 5, 800,  bdir, "", bitmap, bitmap))

fin:
	FPDelete(Conn, vol,  bdir, name);
	FPDelete(Conn, vol,  bdir, ndir);
	FPDelete(Conn, vol,  bdir, ndir1);
	FAIL (htonl(AFPERR_NOOBJ) != FPEnumerateFull(Conn, vol, 1, 1, 800,  bdir, "", bitmap, bitmap))
	FPDelete(Conn, vol,  DIRDID_ROOT, base);
test_exit:
	exit_test("FPEnumerate:test218: enumerate arguments");
}

/* ------------------------- */
static void test300(void)
{
uint8_t buffer[DSI_DATASIZ];
uint16_t vol = VolID;
uint16_t d_bitmap;
uint16_t f_bitmap;
unsigned int ret;
int dir;
uint16_t tp;
uint16_t i;
const DSI *dsi;
unsigned char *b;
struct afp_filedir_parms filedir;
int *stack = NULL;
int cnt = 0;
int size = 1000;

	dsi = &Conn->dsi;

	ENTER_TEST

    f_bitmap = (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)|
	         (1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN);

	d_bitmap = (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |  (1 << DIRPBIT_OFFCNT) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS);

	if (Conn->afp_version >= 30) {
		f_bitmap |= (1<<FILPBIT_PDINFO) | (1<<FILPBIT_LNAME);
		d_bitmap |= (1<<FILPBIT_PDINFO) | (1<<FILPBIT_LNAME);
	}
	else {
		f_bitmap |= (1<<FILPBIT_LNAME);
		d_bitmap |= (1<<FILPBIT_LNAME);
	}

	dir = get_did(Conn, vol, DIRDID_ROOT, "");
	if (!dir) {
		nottested();
		goto test_exit;
	}
	if (!(stack = calloc(size, sizeof(int)) )) {
		nottested();
		goto test_exit;
	}
	stack[cnt] = dir;
	cnt++;

	while (cnt) {
	    cnt--;
	    dir = stack[cnt];
		i = 1;
		if (FPGetFileDirParams(Conn, vol,  dir , "", 0, d_bitmap)) {
			nottested();
			goto test_exit;
		}
		while (!(ret = FPEnumerateFull(Conn, vol, i, 150, 8000,  dir , "", f_bitmap, d_bitmap))) {
			/* FPResolveID will trash dsi->data */
			memcpy(buffer, dsi->data, sizeof(buffer));
			memcpy(&tp, buffer +4, sizeof(tp));
			tp = ntohs(tp);
		    i += tp;
			b = buffer +6;
			for (int j = 1; j <= tp; j++, b += b[0]) {
				if (b[1]) {
					filedir.isdir = 1;
					afp_filedir_unpack(&filedir, b + 2, 0, d_bitmap);
					if (cnt > size) {
						size += 1000;
						if (!(stack = realloc(stack, size* sizeof(int)))) {
							nottested();
							return;
						}
					}
					stack[cnt] = filedir.did;
					cnt++;
				}
				else {
					filedir.isdir = 0;
					afp_filedir_unpack(&filedir, b + 2, f_bitmap, 0);
				}
				if (!Quiet) {
					fprintf(stdout, "0x%08x %s%s\n", ntohl(filedir.did),
							(Conn->afp_version >= 30)?filedir.utf8_name:filedir.lname,
							filedir.isdir?"/":"");
				}
				if (!filedir.isdir && FPResolveID(Conn, vol, filedir.did, f_bitmap) && !Quiet) {
					fprintf(stdout, " Can't resolve ID!");
				}
			}
	    }
	    if (ret != ntohl(AFPERR_NOOBJ)) {
			nottested();
			goto test_exit;
	    }
	}

	FPEnumerateFull(Conn, vol, 1, 150, 8000,  DIRDID_ROOT , "", f_bitmap, d_bitmap);

test_exit:
    free(stack);
	exit_test("FPEnumerate:test300: enumerate recursively a folder");
}

/* ----------- */
void FPEnumerate_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");

    test28();
#if 0
    test34();
#endif
    test38();
    test40();
    test41();
    test93();
    test218();
	test300();
}
