#include "specs.h"

/*
   Test the following:

   test500()
   =========

   client 1:
      mkdir dir1
      mkdir dir2

   client 2:
      mv dir1 dir2/renamed

   client 1:
      ls dir2
      stat dir2/renamed

   Check: CNID must not change
   Targets: enumerate()

   test501()
   =========

   client 1:
      mkdir dir1
      mkdir dir2

   client 2:
      mv dir1 dir2/renamed

   client 1:
      stat renamed

   Check: CNID must not change
   Targets: getfildirparms()


   test502()
   =========

   client 1:
      mkdir dir1
      mkdir dir2

   client 2:
      mv dir1 dir2/renamed

   client 1:
      ls renamed

   Check: CNID must not change
   Targets: enumerate()


   test503()
   =========

   client 1:
      mkdir dir1
      mkdir dir2

   client 2:
      mv dir1 dir2/renamed

   client 1:
      stat renamed

   Check: CNID must not change
   Targets: getfildirparms()


   test504()
   =========

   client 1:
      mkdir -p dir1/dir2
      touch dir1/dir2/file

   client 2:
      mv dir1 renamed1

   client 1:
      stat file

   Check: CNID must not change
   Targets: getfildirparms()


   test505()
   =========

   client 1:
      mkdir -p dir1/dir2

   client 2:
      mv dir1 renamed1

   client 1:
      stat dir2

   Check: CNID must not change
   Targets: getfildirparms()


   test506()
   =========

   client 1:
      mkdir -p dir1/dir2

   client 2:
      mv dir1 renamed1
      mkdir dir1

   client 1:
      stat dir2


 */

/* move and rename dir, enumerate new parent, stat renamed dir */
STATIC void test500()
{
    DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t500 dir";
    char *subdir1 = "t500 subdir1";
    char *subdir2 = "t500 subdir2";
    char *renamedsubdir1 = "t500 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME);

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0)
        failed();
    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0)
        failed();
    if ((subdir2_id = FPCreateDir(Conn, vol1, dir_id, subdir2)) == 0)
        failed();

    /* Move and rename dir with second connection */
    FAIL( FPMoveAndRename(Conn2, vol2, dir_id, subdir2_id, subdir1, renamedsubdir1) );

    /* Enumerate with first connection, does it crash or similar ? */
    FAIL (FPEnumerate(Conn, vol1, subdir2_id, "", (1<<FILPBIT_FNUM), (1<< DIRPBIT_PDID)) );

    /* Manually check name and CNID */
	FAIL (FPGetFileDirParams(Conn, vol1, subdir2_id, renamedsubdir1, 0, bitmap) );

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

	if (filedir.did != subdir1_id) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %x should be %x\n",filedir.did, subdir1_id);
        }
        failed();
	}
	if (strcmp(filedir.lname, renamedsubdir1)) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, renamedsubdir1);
        }
        failed();
	}


fin:
	FAIL( FPCloseVol(Conn2, vol2) );
    FAIL( FPDelete(Conn, vol1, subdir1_id, "") );
    FAIL( FPDelete(Conn, vol1, subdir2_id, "") );
    FAIL( FPDelete(Conn, vol1, dir_id, "") );

test_exit:
	exit_test("Dircache:test500: move and rename dir, enumerate new parent, stat renamed dir");
}

/* move and rename dir, then stat it */
STATIC void test501()
{
    DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t501 dir";
    char *subdir1 = "t501 subdir1";
    char *subdir2 = "t501 subdir2";
    char *renamedsubdir1 = "t501 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME);

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0)
        failed();
    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0)
        failed();
    if ((subdir2_id = FPCreateDir(Conn, vol1, dir_id, subdir2)) == 0)
        failed();

    /* Move and rename dir with second connection */
    FAIL( FPMoveAndRename(Conn2, vol2, dir_id, subdir2_id, subdir1, renamedsubdir1) );

    /* Manually check name and CNID */
	FAIL (FPGetFileDirParams(Conn, vol1, subdir2_id, renamedsubdir1, 0, bitmap) );

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

	if (filedir.did != subdir1_id) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %x should be %x\n",filedir.did, subdir1_id);
        }
        failed();
	}
	if (strcmp(filedir.lname, renamedsubdir1)) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, renamedsubdir1);
        }
        failed();
	}


fin:
	FAIL( FPCloseVol(Conn2, vol2) );
    FAIL( FPDelete(Conn, vol1, subdir1_id, "") );
    FAIL( FPDelete(Conn, vol1, subdir2_id, "") );
    FAIL( FPDelete(Conn, vol1, dir_id, "") );

test_exit:
	exit_test("Dircache:test501: move and rename dir, then stat it");
}

/* move and rename dir, enumerate renamed dir */
STATIC void test502()
{
    DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t502 dir";
    char *subdir1 = "t502 subdir1";
    char *subdir2 = "t502 subdir2";
    char *renamedsubdir1 = "t502 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME);

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0)
        failed();
    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0)
        failed();
    FAIL (FPCreateFile(Conn, vol1,  0, subdir1_id, "file1"))
    if ((subdir2_id = FPCreateDir(Conn, vol1, dir_id, subdir2)) == 0)
        failed();

    /* Move and rename dir with second connection */
    FAIL( FPMoveAndRename(Conn2, vol2, dir_id, subdir2_id, subdir1, renamedsubdir1) );

    /* Enumerate with first connection, does it crash or similar ? */
	FAIL( FPEnumerate(Conn, vol1, subdir1_id, "", (1<<FILPBIT_FNUM), (1<< DIRPBIT_PDID)) );

    /* Manually check name and CNID */
	FAIL( FPGetFileDirParams(Conn, vol1, subdir1_id, "", 0, bitmap) );

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

	if (filedir.did != subdir1_id) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %x should be %x\n",filedir.did, subdir1_id);
        }
        failed();
	}
	if (strcmp(filedir.lname, renamedsubdir1)) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, renamedsubdir1);
        }
        failed();
	}


fin:
	FAIL( FPCloseVol(Conn2, vol2) );
    FAIL( FPDelete(Conn, vol1, subdir1_id, "file1"))
    FAIL( FPDelete(Conn, vol1, subdir1_id, "") );
    FAIL( FPDelete(Conn, vol1, subdir2_id, "") );
    FAIL( FPDelete(Conn, vol1, dir_id, "") );

test_exit:
	exit_test("Dircache:test502: move and rename dir, enumerate renamed dir");
}

/* move and rename dir, stat renamed dir */
STATIC void test503()
{
    DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t503 dir";
    char *subdir1 = "t503 subdir1";
    char *subdir2 = "t503 subdir2";
    char *renamedsubdir1 = "t503 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME);

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0)
        failed();
    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0)
        failed();
    if ((subdir2_id = FPCreateDir(Conn, vol1, dir_id, subdir2)) == 0)
        failed();

    /* Move and rename dir with second connection */
    FAIL( FPMoveAndRename(Conn2, vol2, dir_id, subdir2_id, subdir1, renamedsubdir1) );

    /* Manually check name and CNID */
	FAIL( FPGetFileDirParams(Conn, vol1, subdir1_id, "", 0, bitmap) );

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

	if (filedir.did != subdir1_id) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %x should be %x\n",filedir.did, subdir1_id);
        }
        failed();
	}
	if (strcmp(filedir.lname, renamedsubdir1)) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, renamedsubdir1);
        }
        failed();
	}


fin:
	FAIL( FPCloseVol(Conn2, vol2) );
    FAIL( FPDelete(Conn, vol1, subdir1_id, "") );
    FAIL( FPDelete(Conn, vol1, subdir2_id, "") );
    FAIL( FPDelete(Conn, vol1, dir_id, "") );

test_exit:
	exit_test("Dircache:test503: move and rename dir, enumerate renamed dir");
}

/* rename topdir, stat file in subdir of renamed topdir */
STATIC void test504()
{
    DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t504 dir";
    char *subdir1 = "t504 subdir1";
    char *subdir2 = "t504 subdir2";
    char *renamedsubdir1 = "t504 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id, file_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME);

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0)
        failed();
    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0)
        failed();
    if ((subdir2_id = FPCreateDir(Conn, vol1, subdir1_id, subdir2)) == 0)
        failed();

    /* Create file and get CNID */
    FAIL( FPCreateFile(Conn, vol1,  0, subdir2_id, "file1") );
	FAIL( FPGetFileDirParams(Conn, vol1, subdir2_id, "file1", 0, bitmap) );
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);
    file_id = filedir.did;

    /* Move and rename dir with second connection */
    FAIL( FPMoveAndRename(Conn2, vol2, dir_id, dir_id, subdir1, renamedsubdir1) );

    /* check CNID */
	FAIL( FPGetFileDirParams(Conn, vol1, subdir2_id, "file1", 0, bitmap) );
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

	if (filedir.did != file_id) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %x should be %x\n",filedir.did, subdir1_id);
        }
        failed();
	}

fin:
	FAIL( FPCloseVol(Conn2, vol2) );
    FAIL( FPDelete(Conn, vol1, subdir2_id, "file1"))
    FAIL( FPDelete(Conn, vol1, subdir2_id, "") );
    FAIL( FPDelete(Conn, vol1, subdir1_id, "") );
    FAIL( FPDelete(Conn, vol1, dir_id, "") );

test_exit:
	exit_test("Dircache:test504: rename topdir, stat file in subdir of renamed topdir");
}

/* rename dir, stat subdir in renamed dir */
STATIC void test505()
{
    DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t505 dir";
    char *subdir1 = "t505 subdir1";
    char *subdir2 = "t505 subdir2";
    char *renamedsubdir1 = "t505 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME);

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0)
        failed();
    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0)
        failed();
    if ((subdir2_id = FPCreateDir(Conn, vol1, subdir1_id, subdir2)) == 0)
        failed();

    /* Move and rename dir with second connection */
    FAIL( FPMoveAndRename(Conn2, vol2, dir_id, dir_id, subdir1, renamedsubdir1) );

    /* Manually check name and CNID */
	FAIL( FPGetFileDirParams(Conn, vol1, subdir2_id, "", 0, bitmap) );

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

	if (filedir.did != subdir2_id) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %x should be %x\n",filedir.did, subdir2_id);
        }
        failed();
	}
	if (strcmp(filedir.lname, subdir2
)) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, subdir2);
        }
        failed();
	}


fin:
	FAIL( FPCloseVol(Conn2, vol2) );
    FAIL( FPDelete(Conn, vol1, subdir2_id, "") );
    FAIL( FPDelete(Conn, vol1, subdir1_id, "") );
    FAIL( FPDelete(Conn, vol1, dir_id, "") );

test_exit:
	exit_test("Dircache:test505: rename dir, stat subdir in renamed dir");
}

/* stat subdir in poisened path */
STATIC void test506()
{
    DSI *dsi = &Conn->dsi;
    uint16_t vol1 = VolID;
    uint16_t vol2;
    char *dir = "t506 dir";
    char *subdir1 = "t506 subdir1";
    char *subdir2 = "t506 subdir2";
    char *renamedsubdir1 = "t506 renamedsubdir1";
    uint32_t dir_id, subdir1_id, subdir2_id, poisondir_id;
    int ofs = 3 * sizeof(uint16_t);
    struct afp_filedir_parms filedir;
    uint16_t bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME);

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if ((vol2 = FPOpenVol(Conn2, Vol)) == 0xffff) {
        failed();
        goto test_exit;
    }

    /* Create directories with first connection */
    if ((dir_id = FPCreateDir(Conn, vol1, DIRDID_ROOT, dir)) == 0)
        failed();
    if ((subdir1_id = FPCreateDir(Conn, vol1, dir_id, subdir1)) == 0)
        failed();
    if ((subdir2_id = FPCreateDir(Conn, vol1, subdir1_id, subdir2)) == 0)
        failed();

    /* Move and rename dir with second connection */
    FAIL( FPMoveAndRename(Conn2, vol2, dir_id, dir_id, subdir1, renamedsubdir1) );
    /* Re-create renamed directory */
    if ((poisondir_id = FPCreateDir(Conn2, vol2, dir_id, subdir1)) == 0)
        failed();

    /* Manually check name and CNID */
	FAIL( FPGetFileDirParams(Conn, vol1, subdir2_id, "", 0, bitmap) );

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data + ofs, 0, bitmap);

	if (filedir.did != subdir2_id) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %x should be %x\n",filedir.did, subdir2_id);
        }
        failed();
	}
	if (strcmp(filedir.lname, subdir2
)) {
        if (!Quiet) {
    		fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, subdir2);
        }
        failed();
	}


fin:
	FAIL( FPCloseVol(Conn2, vol2) );
    FAIL( FPDelete(Conn, vol1, subdir2_id, "") );
    FAIL( FPDelete(Conn, vol1, subdir1_id, "") );
    FAIL( FPDelete(Conn, vol1, poisondir_id, "") );
    FAIL( FPDelete(Conn, vol1, dir_id, "") );

test_exit:
	exit_test("Dircache:test506: stat subdir in poisoned path");
}

void Dircache_attack_test()
{
    fprintf(stdout, "===================\n");
    fprintf(stdout, "Dircache attack\n");
    fprintf(stdout, "===================\n");
    test500();
    test501();
    test502();
    test503();
    test504();
    test505();
    test506();
}
