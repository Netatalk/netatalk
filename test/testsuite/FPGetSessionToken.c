/* ----------------------------------------------
*/
#include "specs.h"

extern char *Server;
extern int  Port;
extern char *Password;
extern char *vers;
extern char *uam;

static volatile int sigp = 0;

static void pipe_handler()
{
	sigp = 1;
}

/* ------------------------- */
STATIC void test220()
{
char *name = "t23 dir";
char *name1 = "t23 subdir";
char *name2 = "t23 file";
int  dir,dir1;
uint16_t vol = VolID;

	ENTER_TEST
	if (Conn->afp_version != 30) {
		test_skipped(T_AFP30);
		goto test_exit;
	}

	FAIL (FPGetSessionToken(Conn,0 ,0 ,0 ,NULL))
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
	exit_test("FPGetSessionToken:test220: AFP 3.0 get session token");
}

/* ------------------------- */
STATIC void test221()
{
	ENTER_TEST
	if (Conn->afp_version <= 31) {
		test_skipped(T_AFP32);
		goto test_exit;
	}

	FAIL (FPGetSessionToken(Conn, 3, 0, 5, "token"))
        FAIL (FPzzz(Conn, 0))
test_exit:
	exit_test("FPGetSessionToken:test221: AFP 3.1 get session token");
}

/* ----------- */
void FPGetSessionToken_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"%s\n", __func__);
    fprintf(stdout,"-------------------\n");
    test220();
    test221();
}
