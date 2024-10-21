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

	enter_test();

	if (!Path) {
		test_skipped(T_MAC_PATH);
		goto test_exit;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

    strcpy(Data, "hello");

	/* ----------------- */
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}

	if (FPWrite(Conn, fork, 0, 5, Data, 0 )) {
		failed();
	}
    FPCloseFork(Conn,fork);
    fork = 0;

    /* ----------------- */
    if (chmod_unix_rfork(Path, "", name, 0400) != 0) {
        failed();
        exit(1);
        goto fin;
    }
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}
	if (FPRead(Conn, fork, 0, 5, Data)) {
		failed();
        goto fin;
	}
    if (strcmp(Data, "hello") != 0) {
		failed();
        goto fin;
    }
    FPCloseFork(Conn,fork);

    /* ----------------- */
    if (chmod_unix_rfork(Path, "", name, 0000) != 0) {
        failed();
        goto fin;
    }
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_RD);
    if (ntohl(AFPERR_ACCESS) != dsi->header.dsi_code) {
        if (fork)
            FPCloseFork(Conn,fork);
		failed();
		goto fin;
	}

    /* ----------------- */
    if (chmod_unix_rfork(Path, "", name, 0600) != 0) {
        failed();
        goto fin;
    }

    if (delete_unix_md(Path, "", name) != 0) {
		failed();
        goto fin;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}
	if (FPWrite(Conn, fork, 0, 5, Data, 0 )) {
		failed();
	}
	if (FPRead(Conn, fork, 0, 5, Data)) {
		failed();
        goto fin;
	}
    if (strcmp(Data, "hello") != 0) {
		failed();
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
void FPRead_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPRead\n");
    fprintf(stdout,"-------------------\n");
	test109();
}
