#include "specs.h"
#include "ea.h"
#include "adoublehelper.h"

/* ------------------------- */
STATIC void test109()
{
    uint16_t bitmap = 0;
    int fork;
    char *name = "t109 file.txt";
    uint16_t vol = VolID;
    int size;
    DSI *dsi;
    struct stat st;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (Path[0] == '\0') {
		test_skipped(T_PATH);
		goto test_exit;
	}

	if (adouble == AD_EA) {
		test_skipped(T_ADV2);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		test_nottested();
		goto test_exit;
	}

    strcpy(Data, "hello");

	/* ----------------- */
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		test_failed();
		goto fin;
	}

	if (FPWrite(Conn, fork, 0, 5, Data, 0 )) {
		test_failed();
	}
    FPCloseFork(Conn,fork);
    fork = 0;

    /* ----------------- */
    if (chmod_unix_rfork(Path, "", name, 0400) != 0) {
        test_failed();
        goto fin;
    }
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_RD);
	if (!fork) {
		test_failed();
		goto fin;
	}
	if (FPRead(Conn, fork, 0, 5, Data)) {
		test_failed();
        goto fin;
	}
    if (strcmp(Data, "hello") != 0) {
		test_failed();
        goto fin;
    }
    FPCloseFork(Conn,fork);

    /* ----------------- */
    if (chmod_unix_rfork(Path, "", name, 0000) != 0) {
        test_failed();
        goto fin;
    }
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_RD);
    if (ntohl(AFPERR_ACCESS) != dsi->header.dsi_code) {
        if (fork)
            FPCloseFork(Conn,fork);
		test_failed();
		goto fin;
	}

    /* ----------------- */
    if (chmod_unix_rfork(Path, "", name, 0600) != 0) {
        test_failed();
        goto fin;
    }

    if (delete_unix_md(Path, "", name) != 0) {
		test_failed();
        goto fin;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		test_failed();
		goto fin;
	}
	if (FPWrite(Conn, fork, 0, 5, Data, 0 )) {
		test_failed();
	}
	if (FPRead(Conn, fork, 0, 5, Data)) {
		test_failed();
        goto fin;
	}
    if (strcmp(Data, "hello") != 0) {
		test_failed();
        goto fin;
    }

fin:
	if (fork)
        FPCloseFork(Conn,fork);

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))

test_exit:
	exit_test("FPread:test109: read resource fork that is ro (0400), inaccessible (0000) or without metadata EA");
}

/* ----------- */
void T2FPRead_test()
{
    ENTER_TESTSET
	test109();
}
