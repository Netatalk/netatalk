/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
STATIC void test23()
{
char *name = "t23 dir";
char *name1 = "t23 subdir";
char *name2 = "t23 file";
int  dir,dir1;
uint16_t vol = VolID;


	ENTER_TEST

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		test_nottested();
		goto test_exit;
	}

	if (Conn->afp_version < 30) {
		if (ntohl(AFPERR_NOOP) != FPEnumerate_ext(Conn, vol,  DIRDID_ROOT , "",
		                    (1 << FILPBIT_PDINFO )|(1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN), 0xffff)) {
			test_failed();
		}
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
		goto test_exit;
	}
	FAIL (FPGetSrvrInfo(Conn))
	FPEnumerate_ext(Conn, vol, DIRDID_ROOT  , "",
			                (1 << FILPBIT_PDINFO )| (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		        	            |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN),
	                        (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT));

	dir1 = FPCreateDir(Conn,vol, dir , name1);
	if (dir1) {
		FAIL (FPGetFileDirParams(Conn, vol,  dir , name1, 0, (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT)))
		FAIL (FPCreateFile(Conn, vol,  0, dir1 , name2))
		FAIL (FPGetFileDirParams(Conn, vol,  dir , name1, 0, (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT)))
		if (FPEnumerate_ext(Conn, vol,  dir1 , "",
			                (1 << FILPBIT_PDINFO )| (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		        	            |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN),
	                        	(1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT)
		                     )) {
			test_failed();
		}
		if (FPEnumerate_ext(Conn, vol,  dir , "",
		                (1 << FILPBIT_PDINFO )| (1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN) | (1 << FILPBIT_LNAME),
	                        (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT) | (1 << DIRPBIT_LNAME)
	                     )) {
			test_failed();
		}
		FAIL (FPDelete(Conn, vol,  dir1, name2))
	}
	else {
		test_failed();
	}
	if (dir) {
		FAIL (FPDelete(Conn, vol,  dir, name1))
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	}
test_exit:
	exit_test("FPEnumerateExt:test23: AFP 3.0 FPEnumerate ext");
}

/* ----------- */
void FPEnumerateExt_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
    test23();
}
