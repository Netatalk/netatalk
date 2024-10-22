/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test25()
{
char *name = "t25 dir";
char *name1 = "t25 subdir";
char *name2 = "t25 file";
int  dir,dir1;
uint16_t vol = VolID;

	ENTER_TEST
	if (Conn->afp_version < 31) {
		test_skipped(T_AFP31);
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}
	FPEnumerate_ext2(Conn, vol,  DIRDID_ROOT , "",
			 	(1 << FILPBIT_PDINFO )| (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		        	|(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN),
	                        (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT)
		         );

	dir1 = FPCreateDir(Conn,vol, dir , name1);
	if (!dir1) {
		failed();
		goto fin;
	}

	FAIL (FPGetFileDirParams(Conn, vol,  dir , name1, 0, (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT)))
	FAIL (FPCreateFile(Conn, vol,  0, dir1 , name2))
	FAIL (FPGetFileDirParams(Conn, vol,  dir , name1, 0, (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT)))
	if (FPEnumerate_ext2(Conn, vol,  dir1 , "",
			         (1 << FILPBIT_PDINFO )| (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		        	 |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN),
	                 (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT))) {
		failed();
	}
	if (FPEnumerate_ext2(Conn, vol,  dir , "",
		             (1 << FILPBIT_PDINFO )| (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		             |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN) | (1 << FILPBIT_LNAME),
	                 (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT) | (1 << DIRPBIT_LNAME))) {
		failed();
	}
	FAIL (FPDelete(Conn, vol,  dir1, name2))
	FAIL (FPDelete(Conn, vol,  dir, name1))
fin:
	FPDelete(Conn, vol,  DIRDID_ROOT, name);
test_exit:
	exit_test("FPEnumerateExt2:test25: FPEnumerate ext2");
}

/* ------------------------- */
STATIC void test211()
{
char *name = "t211 dir";
char *name1 = "t211 subdir";
char *name2 = "t211 file";
int  dir,dir1;
uint16_t vol = VolID;


	ENTER_TEST

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto test_exit;
	}

	if (Conn->afp_version < 31) {
		if (ntohl(AFPERR_NOOP) != FPEnumerate_ext2(Conn, vol,  DIRDID_ROOT , "",
		                    (1 << FILPBIT_PDINFO )|(1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN), 0xffff)) {
			failed();
		}
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
		goto test_exit;
	}
	FAIL (FPGetSrvrInfo(Conn))
	dir1 = FPCreateDir(Conn,vol, dir , name1);
	if (dir1) {
		FAIL (FPGetFileDirParams(Conn, vol,  dir , name1, 0, (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT)))
		FAIL (FPCreateFile(Conn, vol,  0, dir1 , name2))
		FAIL (FPGetFileDirParams(Conn, vol,  dir , name1, 0, (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT)))
		if (FPEnumerate_ext2(Conn, vol,  dir1 , "",
			                (1 << FILPBIT_PDINFO )| (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		        	            |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN),
	                        	(1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT)
		                     )) {
			failed();
		}
		if (FPEnumerate_ext2(Conn, vol,  dir , "",
		                (1 << FILPBIT_PDINFO )| (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN) | (1 << FILPBIT_LNAME),
	                        (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT) | (1 << DIRPBIT_LNAME)
	                     )) {
			failed();
		}
		FAIL (FPDelete(Conn, vol,  dir1, name2))
	}
	else {
		failed();
	}
	if (dir) {
		FAIL (FPDelete(Conn, vol,  dir, name1))
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	}
test_exit:
	exit_test("FPEnumerateExt2:test211: AFP 3.1 FPEnumerate ext2");
}

/* ----------- */
void FPEnumerateExt2_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPEnumerateExt2 page 155\n");
    fprintf(stdout,"-------------------\n");
    test25();
    test211();
}
