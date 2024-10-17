/* ----------------------------------------------
*/
#include "specs.h"

static char temp[MAXPATHLEN];   

/* -------------------------- */
STATIC void test60()
{
char *name ="test60 illegal fork";
DSI *dsi;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test60: illegal fork\n");

	illegal_fork(dsi, AFP_BYTELOCK, name);
	exit_test("test60");
}

/* ------------------------- */
static void test_bytelock(uint16_t vol, char *name, int type)
{
int fork;
int fork1;
uint16_t bitmap = 0;
int len = (type == OPENFORK_RSCS)?(1<<FILPBIT_RFLEN):(1<<FILPBIT_DFLEN);

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		return;
	}

	fork = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		FPDelete(Conn, vol,  DIRDID_ROOT, name);
		return;
	}

	if (FPByteLock(Conn, fork, 0, 0 /* set */, 0, 100)) {
		failed();
	}
	else if (FPByteLock(Conn, fork, 0, 1 /* clear */ , 0, 100)) {
		failed();
	}

	FAIL (FPByteLock(Conn, fork, 0, 0 /* set */, 0, 100))
	
	/* some Mac OS do nothing here */
	FAIL (htonl(AFPERR_PARAM) != FPByteLock(Conn, fork, 0, 0 , -1, 75)) 

	FAIL (htonl(AFPERR_NORANGE) != FPByteLock(Conn, fork, 0, 1 /* clear */ , 0, 75)) 

	FAIL ( htonl(AFPERR_RANGEOVR) != FPByteLock(Conn, fork, 0, 0 /* set */, 80 , 100)) 

	fork1 = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork1)
		failed();
	else {
		FAIL (htonl(AFPERR_LOCK) != FPByteLock(Conn, fork1, 0, 0 /* set */ , 20, 60))
		FAIL (FPSetForkParam(Conn, fork, len , 50)) 
		FAIL (FPSetForkParam(Conn, fork1, len , 60)) 
		FAIL (htonl(AFPERR_LOCK) != FPRead(Conn, fork1, 0, 40, Data)) 
		FAIL (htonl(AFPERR_LOCK) != FPWrite(Conn, fork1, 10, 40, Data, 0)) 
		FAIL (FPCloseFork(Conn,fork1))
	}

	/* end */

    /* Netatalk 2 doesn't drop locks when a fork is closed, 3 does */
    FPByteLock(Conn, fork, 0, 1 /* clear */ , 0, 100);

	FAIL (htonl(AFPERR_NORANGE) != FPByteLock(Conn, fork, 0, 1 /* clear */ , 0, 50)) 

	FAIL (FPSetForkParam(Conn, fork, len , 200)) 

	if (FPByteLock(Conn, fork, 1 /* end */, 0 /* set */, 0, 100)) {
		failed();
	}
	else if (FPByteLock(Conn, fork, 0, 1 /* clear */ , 200, 100)) {
		failed();
	}

    /* RANGEOVR */
	if (FPByteLock(Conn, fork, 0 /* end */, 0 /* set */, 0, -1)) {
		failed();
	}
	else if (FPByteLock(Conn, fork, 0, 1 /* clear */ , 0, -1)) {
		failed();
	}
	FAIL (htonl(AFPERR_PARAM) != FPByteLock(Conn, fork, 0 /* start */, 0 /* set */, 0, 0)) 

	/* some Mac OS do nothing here */
	FAIL (htonl(AFPERR_PARAM) != FPByteLock(Conn, fork, 0 /* start */, 0 /* set */, 0, -2))

	if (FPCloseFork(Conn,fork)) {
		nottested();
	}
	FAIL (htonl (AFPERR_PARAM ) != FPByteLock(Conn, fork, 0, 0 /* set */, 0, 100)) 

	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) {
		nottested();
	}

}
/* ----------- */
STATIC void test63()
{
char *name = "test63 FPByteLock DF";

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test63: FPByteLock Data Fork\n");
	test_bytelock(VolID, name, OPENFORK_DATA);
	exit_test("test63");
}

/* ----------- */
STATIC void test64()
{
char *name = "test64 FPByteLock RF";

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test64: FPByteLock Resource Fork\n");
	test_bytelock(VolID, name, OPENFORK_RSCS);
	exit_test("test64");
}

/* -------------------------- */
static void test_bytelock3(char *name, int type)
{
int fork;
int fork1;
uint16_t vol = VolID;
uint16_t bitmap = 0;
uint16_t vol2;
int len = (type == OPENFORK_RSCS)?(1<<FILPBIT_RFLEN):(1<<FILPBIT_DFLEN);
DSI *dsi2;

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		return;
	}

	fork = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
		return;
	}

	FAIL (FPSetForkParam(Conn, fork, len , 50))
	FAIL (FPByteLock(Conn, fork, 0, 0 , 0 , 100))

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		nottested();
		goto fin;
	}

	fork1 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork1) {
		failed();
	}
	else {
		FAIL (htonl(AFPERR_LOCK) != FPRead(Conn2, fork1, 0, 40, Data)) 
		FAIL (htonl(AFPERR_LOCK) != FPWrite(Conn2, fork1, 10, 40, Data, 0))
		FAIL (htonl(AFPERR_LOCK) != FPWrite(Conn2, fork1, 55, 40, Data, 0))
		FAIL (FPSetForkParam(Conn2, fork1, len , 60)) 
		FAIL (FPWrite(Conn, fork, 55, 40, Data, 0)) 
		FAIL (htonl(AFPERR_LOCK) != FPSetForkParam(Conn2, fork1, len , 60)) 
		FAIL (htonl(AFPERR_LOCK) != FPByteLock(Conn2, fork1, 0, 0 /* set */ , 20, 60))
		FAIL (FPSetForkParam(Conn, fork, len , 200))
		FAIL (FPSetForkParam(Conn2, fork1, len ,120))
		
	}
	FAIL (FPCloseFork(Conn,fork))
	if (fork1) FPCloseFork(Conn2,fork1);

	FAIL (FPCloseVol(Conn2,vol2))
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
}

/* --------------- */
STATIC void test65()
{
char *name = "t65 DF FPByteLock 2 users";

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test65: FPByteLock 2users DATA FORK\n");
    
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if (Locking) {
		test_skipped(T_LOCKING);
		goto test_exit;
	}		
    test_bytelock3(name, OPENFORK_DATA);

	name = "t65 RF FPByteLock 2 users";
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test65: FPByteLock 2users Resource FORK\n");
    test_bytelock3(name, OPENFORK_RSCS);
test_exit:
	exit_test("test65");
}

/* ---------------------------- */
static void test_bytelock2(char *name, int type)
{
int fork;
int fork1;
uint16_t bitmap = 0;
uint16_t vol = VolID;
DSI *dsi2;

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		return;
	}

	fork = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		failed();
		goto fin;
	}
	/* fin */
	if (FPByteLock(Conn, fork, 0 /* end */, 0 /* set */, 0, -1)) {
		failed();
        goto fin;
	}
    if (Conn->afp_version >= 30) {
        fork1 = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
        if (!fork1) {
            failed();
            goto fin;
        }
        FAIL (htonl(AFPERR_LOCK) != FPByteLock_ext(Conn, fork1, 0, 0, 20, 60));
        if (htonl(AFPERR_LOCK) != FPByteLock_ext(Conn, fork1, 0, 0, ((off_t)1<<32)+2, 60)) {
            failed();
            FAIL (FPByteLock_ext(Conn, fork1, 0, 1, ((off_t)1<<32)+2, 60));
        }
        FPCloseFork(Conn,fork1);
    }
    if (Conn2) {
        uint16_t vol2;
        if (Locking) {
            test_skipped(T_LOCKING);
            goto fin;
        }
        dsi2 = &Conn2->dsi;
        vol2  = FPOpenVol(Conn2, Vol);
        if (vol2 == 0xffff) {
            nottested();
            goto fin;
        }
        fork1 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
        if (!fork1) {
            failed();
            goto fin2;
        }
#if 0
        if (htonl(AFPERR_LOCK) != FPByteLock_ext(Conn2, fork1, 0, 0, ((off_t)1<<32)+2, 60)) {
            failed();
            FPByteLock_ext(Conn2, fork1, 0, 1, ((off_t)1<<32)+2, 60);
        }
#endif
        FAIL (htonl(AFPERR_LOCK)  != FPWrite_ext(Conn2, fork1, ((off_t)1<<32)+2, 40, Data, 0));
        FAIL (FPCloseFork(Conn2,fork1));
    fin2:
        FAIL (FPCloseVol(Conn2,vol2));
    }
    FAIL (FPByteLock(Conn, fork, 0, 1 /* clear */ , 0, -1));
	FAIL (FPCloseFork(Conn,fork));
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
}

/* -------------------------- */
void test78()
{
char *name = "t78 FPByteLock RF size -1";

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test78: test Byte Lock size -1 with no large file support\n");
	if (Locking) {
		test_skipped(T_LOCKING);
		goto test_exit;
	}		
	test_bytelock2(name, OPENFORK_RSCS);

    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test78: test Byte Lock size -1 with no large file support, DATA fork\n");
	name = "t78 FPByteLock DF size -1";
	test_bytelock2(name, OPENFORK_DATA);
test_exit:
	exit_test("test78");
}

/* ----------- */
STATIC void test79()
{
int fork;
int fork1;
uint16_t bitmap = 0;
uint16_t vol = VolID;
int type = OPENFORK_DATA;
char *name = "t79 FPByteLock Read";
int len = (type == OPENFORK_RSCS)?(1<<FILPBIT_RFLEN):(1<<FILPBIT_DFLEN);

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test79: test Byte Lock and read conflict\n");
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		FPDelete(Conn, vol,  DIRDID_ROOT, name);
		goto test_exit;
	}
	if (FPSetForkParam(Conn, fork, len , 60)) {
		nottested();
	}

	FAIL (FPByteLock(Conn, fork, 0, 0, 0, 100)) 

	fork1 = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork1) {
		failed();
	} 
	else {
		FAIL (htonl(AFPERR_LOCK) != FPRead(Conn, fork1, 0, 40, Data)) 
		FPCloseFork(Conn,fork1);
	}

	FAIL (FPCloseFork(Conn,fork))
	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) {
		nottested();
	}
test_exit:
	exit_test("test79");
}

/* -------------------------- */
static void test_bytelock5(uint16_t vol, char *name, int type)
{
int fork;
uint16_t bitmap = 0;
int len = (type == OPENFORK_RSCS)?(1<<FILPBIT_RFLEN):(1<<FILPBIT_DFLEN);

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		return;
	}

	fork = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		failed();
		FPDelete(Conn, vol,  DIRDID_ROOT, name);
		return;
	}
	FAIL (FPSetForkParam(Conn, fork, len , 50))

	FAIL (FPByteLock(Conn, fork, 0, 0 , 0 , 100)) 
	FAIL (FPRead(Conn, fork, 0, 40, Data))
	FAIL (FPWrite(Conn, fork, 10, 120, Data, 0)) 

	FAIL (FPByteLock(Conn, fork, 0, 1 , 0 , 100)) 
	FAIL (FPCloseFork(Conn,fork))
	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) {
		nottested();
	}
}

/* -------------------------- */
STATIC void test80()
{
char *name = "t80 RF FPByteLock Read write";

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test80: Resource Fork test Byte Lock and read write same user(file)\n");
	test_bytelock5(VolID, name, OPENFORK_RSCS);

    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test80: Data Fork test Byte Lock and read write same user(file)\n");
	name = "t80 DF FPByteLock Read write";
	test_bytelock5(VolID, name, OPENFORK_DATA);

	exit_test("test80");
}

/* --------------- */
STATIC void test329()
{
char *name = "t329 DF FPByteLock 2 users";
int fork;
int fork1 = 0;
int fork2 = 0;
uint16_t vol = VolID;
uint16_t bitmap = 0;
uint16_t vol2;
DSI *dsi2;
int type = OPENFORK_DATA;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test329: FPByteLock 2users DATA FORK\n");
    
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if (Locking) {
		test_skipped(T_LOCKING);
		goto test_exit;
	}
	
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
		goto test_exit;
	}

	fork1 = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork1) {
		nottested();
		FAIL (FPCloseFork(Conn,fork))
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
		goto test_exit;
	}

	fork2 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,0x33);
	if (!fork2) {
		failed();
	}
	
	FAIL (FPCloseFork(Conn,fork2))

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		nottested();
		goto fin;
	}
	if (FPGetFileDirParams(Conn2, vol2,  DIRDID_ROOT , name, (1<<FILPBIT_ATTR), 0 )) {
		nottested();
		goto fin;
	}

	fork2 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,OPENACC_DWR |OPENACC_RD);
	if (fork2) {
		FPCloseFork(Conn2,fork2);
		failed();
	}
	
	fork2 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,OPENACC_DWR |OPENACC_RD);
	if (fork2) {
		FPCloseFork(Conn2,fork2);
		failed();
	}
	
	fork2 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,/* OPENACC_WR | */OPENACC_RD);
	if (!fork2) {
		failed();
	}
	
	FAIL (FPCloseFork(Conn,fork))
	if (fork1) FPCloseFork(Conn,fork1);
	if (fork2) FPCloseFork(Conn2,fork2);

	FAIL (FPCloseVol(Conn2,vol2))
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test329");
}

/* --------------- */
STATIC void test410()
{
    char *name = "t410 DF FPByteLock 2 users";
    int fork;
    int fork1 = 0;
    int fork2 = 0;
    uint16_t vol = VolID;
    uint16_t bitmap = 0;
    uint16_t vol2;
    DSI *dsi2;
    int type = OPENFORK_DATA;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test410: FPByteLock 2users DATA FORK\n");
    
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if (Locking) {
		test_skipped(T_LOCKING);
		goto test_exit;
	}

    if (adouble == AD_V2) {
        /* this fails on adouble v2, because in Netatalk 3 we now close locks on file close */
		test_skipped(T_ADEA);
		goto test_exit;
    }
	
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		goto fin;
	}

	if (FPByteLock(Conn, fork, 0, 0 /* set */, 0, 100)) {
		failed();
        FAIL (FPCloseFork(Conn,fork))
        goto fin;
    }

	fork1 = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork1) {
		failed();
		FPCloseFork(Conn,fork);
		goto fin;
	}

	if (FPCloseFork(Conn,fork1)) { /* this should drop the byterange lock! */
		failed();
		FPCloseFork(Conn,fork);
		goto fin;
    }

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		nottested();
		FPCloseFork(Conn,fork);
		goto fin;
	}

	fork1 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name, OPENACC_WR |OPENACC_RD);
	if (!fork1) {
		failed();
		FPCloseFork(Conn,fork);
        goto fin2;
	}
	
	if (FPByteLock(Conn2, fork1, 0, 0 /* set */, 0, 100)) {
		failed();
    }

	FAIL (FPCloseFork(Conn,fork))
    FAIL (FPCloseFork(Conn2,fork1))
fin2:
	FAIL (FPCloseVol(Conn2,vol2))
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test410");
}

/* ------------------- */
static int create_trash(CONN *conn, uint16_t vol)
{
char *trash = "Network Trash Folder";
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (DIRPBIT_ATTR)| (1<<DIRPBIT_FINFO)| (1<<DIRPBIT_CDATE) | 
					(1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE)| (1<<DIRPBIT_UID) |
	    			(1 << DIRPBIT_GID) |(1 << DIRPBIT_ACCESS);
DSI *dsi;
int dir;
struct afp_filedir_parms filedir;

	dsi = &conn->dsi;

	dir  = FPCreateDir(conn, vol, DIRDID_ROOT, trash);
	if (!dir) {
		return dir;
	}

	if (FPGetFileDirParams(conn, vol,  DIRDID_ROOT , trash, 0,bitmap )) {
		FPDelete(Conn, vol,  dir, "");
		return 0;
	}
	filedir.isdir = 1;

	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
    filedir.access[0] = 0; 
    filedir.access[1] = 7; 
    filedir.access[2] = 7; 
    filedir.access[3] = 7; 
    if (FPSetDirParms(conn, vol, dir , "", (1 << DIRPBIT_ACCESS),  &filedir)) {
		FPDelete(conn, vol,  dir, "");
		return 0;
    }

	filedir.attr = ATTRBIT_INVISIBLE| ATTRBIT_SETCLR ;
    if (FPSetDirParms(conn, vol, DIRDID_ROOT , trash, (1 << DIRPBIT_ATTR), &filedir)) {
		FPDelete(conn, vol,  dir, "");
		return 0;
    }
	return dir;
}

/* ------------------- */
static int create_map(CONN *conn, uint16_t vol, int dir, char *name)
{
DSI *dsi;
struct afp_filedir_parms filedir;
int  ofs =  3 * sizeof( uint16_t );
int fork;

	dsi = &conn->dsi;

	if (FPCreateFile(conn, vol,  0, dir , name)) {
		return 0;
	}

	if (FPGetFileDirParams(conn, vol,  dir , name, 0x73f, 0x133f )) {
	  	return 0;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0x73f, 0);
	filedir.attr = ATTRBIT_INVISIBLE| ATTRBIT_SETCLR ;
 	if (FPSetFilDirParam(conn, vol, dir , name, (1 << DIRPBIT_ATTR), &filedir)) {
	  	return 0;
 	}

	fork = FPOpenFork(conn, vol, OPENFORK_DATA , 0x342 , dir, name, OPENACC_DWR | OPENACC_RD);
	return fork;
}

/* ------------------- */
static int set_perm(CONN *conn, uint16_t vol, int dir)
{
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (DIRPBIT_ATTR)| (1<<DIRPBIT_FINFO)| (1<<DIRPBIT_CDATE) | 
					(1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE)| (1<<DIRPBIT_UID) |
	    			(1 << DIRPBIT_GID) |(1 << DIRPBIT_ACCESS);
DSI *dsi;
struct afp_filedir_parms filedir;

	dsi = &conn->dsi;

	if (FPGetFileDirParams(conn, vol,  dir , "", 0,bitmap )) {
		return 0;
	}
	filedir.isdir = 1;

	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
    filedir.access[0] = 0; 
    filedir.access[1] = 0; 
    filedir.access[2] = 0; 
    filedir.access[3] = 7; 
    if (FPSetDirParms(conn, vol, dir , "", (1 << DIRPBIT_ACCESS),  &filedir)) {
		return 0;
    }

	filedir.attr = ATTRBIT_INVISIBLE| ATTRBIT_SETCLR ;
    if (FPSetDirParms(conn, vol, dir , "", (1 << DIRPBIT_ATTR), &filedir)) {
		return 0;
    }
    return 1;
}

/* ------------------- */
static int write_access(CONN *conn, uint16_t  vol, int dir)
{
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (DIRPBIT_ATTR)| (1<<DIRPBIT_FINFO)| (1<<DIRPBIT_CDATE) | 
					(1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE)| (1<<DIRPBIT_UID) |
	    			(1 << DIRPBIT_GID) |(1 << DIRPBIT_ACCESS);
DSI *dsi;
struct afp_filedir_parms filedir;

	dsi = &conn->dsi;

	if (FPGetFileDirParams(conn, vol,  dir , "", 0,bitmap )) {
		return 0;
	}
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	if ((filedir.access[0] & 7) == 7)
		return 1;
	return 0;
}

/* --------------- */
static int init_trash(CONN *conn, uint16_t vol, int *result)
{
char *trash = "Network Trash Folder";
char *map = "Trash Can Usage Map";
int ret;
int dir;
int dir2;
int fork = 0;
DSI *dsi;
int indice = 1;

	dsi = &conn->dsi;
	*result = 0;

	/* ---------- check/create 'Network Trash Folder' -------- */
	ret = FPGetFileDirParams(conn, vol,  DIRDID_ROOT , trash, 0x73f, 0x133f);
	if (ret == ntohl(AFPERR_NOOBJ)) {
	    dir = create_trash(conn, vol);
	    
	    if (!dir) {
		    nottested();
		    return 0;
	    }
	}
	else if (ret) {
	    nottested();
	    return 0;
	}
	dir = get_did(conn, vol, DIRDID_ROOT , trash);
	if (!dir) {
	    nottested();
	    return 0;
	}

	/* --------- check/create 'Trash Can Usage Map' -------- */
	fork = FPOpenFork(conn, vol, OPENFORK_DATA , 0x342 , dir, map, OPENACC_DWR |OPENACC_RD);
	if (!fork && ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
	    nottested();
	    return 0;
	}
	if (!fork) {
		fork = create_map(conn, vol, dir, map);
	}
	if (!fork) {
	    nottested();
	    return 0;
	}

	if (FPGetForkParam(conn, fork, 0x242)) {
		nottested();
		return 0;
	}

	/* */
	while (1) {
		indice++;
		if (FPByteLock(conn, fork, 0, 0 /* set */, indice , 1))
			continue;
		sprintf(temp, "Trash Can #%d", indice);
		dir2 = FPCreateDir(conn, vol, dir, temp);
		if (dir2) {
		   if (!set_perm(conn, vol, dir2)) {
		       nottested();
		       return 0;
		   }
		   break;
		
		}
		else {
		    dir2 = get_did(conn, vol, dir , temp);
		    if (!dir2) {
		        return 0;
		    }
		    if (dir2 && write_access(conn, vol, dir2)) {
		        break;
		    }
		}
		
		if (FPByteLock(conn, fork, 0, 1 /* clear */, indice , 1)) {
			nottested();
			return 0;
		}
	}
	*result = dir2;
	return fork;
}

/* --------------- */
STATIC void test330()
{
uint16_t vol = VolID;
uint16_t vol2;
int fork = 0;
int fork2 = 0;
DSI *dsi2;
int dir, dir2;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test330: pre OSX trash folder\n");

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	fork = init_trash(Conn, vol, &dir);
	if (!fork)
	    goto fin;
	
	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		nottested();
		goto fin;
	}
	fork2 = init_trash(Conn2, vol2, &dir2);
	if (!fork2)
		goto fin;
	if (dir2 == dir) {
		fprintf(stdout,"\tFAILED both client are using the same folder for trash\n"); 
		failed_nomsg();
	}

	if (fork2) { 
	    FAIL (FPCloseFork(Conn2, fork2))
	    FAIL (FPDelete(Conn2, vol2, dir2, ""))
	}
	FAIL (FPCloseVol(Conn2,vol2))

fin:
	if (fork) {
		FAIL (FPCloseFork(Conn, fork))
	        FAIL (FPDelete(Conn, vol, dir, ""))
	}
test_exit:
	exit_test("test330");
}

/* --------------- */
extern char *Test;
STATIC void test366()
{
char *name = "t366 FPByteLock 2 users";
int fork;
int fork1 = 0;
int fork2 = 0;
uint16_t vol = VolID;
uint16_t bitmap = 0;
uint16_t vol2;
DSI *dsi2;
int type = OPENFORK_DATA;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock:test366: Locks released on exit\n");
    
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}
	if (Locking) {
		test_skipped(T_LOCKING);
		goto test_exit;
	}
	/* hack, it closes a connection */
	if (!Test) {
		fprintf(stdout,"\tSKIPPED (only with -f option)\n");
		goto test_exit;
	}
	
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		nottested();
		goto test_exit;
	}

	fork = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork) {
		nottested();
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
		goto test_exit;
	}

	fork1 = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork1) {
		nottested();
		FAIL (FPCloseFork(Conn,fork))
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
		goto test_exit;
	}

	fork2 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,0x33);
	if (!fork2) {
		failed();
	}
	
	FAIL (FPCloseFork(Conn,fork2))

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		nottested();
		goto fin;
	}
	if (FPGetFileDirParams(Conn2, vol2,  DIRDID_ROOT , name, (1<<FILPBIT_ATTR), 0 )) {
		nottested();
		goto fin;
	}

	fork2 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,OPENACC_DWR |OPENACC_RD);
	if (fork2) {
		FPCloseFork(Conn2,fork2);
		failed();
	}
	
	fork2 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,OPENACC_DWR |OPENACC_RD);
	if (fork2) {
		FPCloseFork(Conn2,fork2);
		failed();
	}
	
	fork2 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,/* OPENACC_WR | */OPENACC_RD);
	if (!fork2) {
		failed();
	}
	
   	FPLogOut(Conn);
	if (fork2) FPCloseFork(Conn2,fork2);

	FAIL (FPDelete(Conn2, vol2,  DIRDID_ROOT, name))
	FAIL (FPCloseVol(Conn2,vol2))
	goto test_exit;

fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("test366");
}

/* ----------- */
void FPByteRangeLock_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPByteRangeLock page 101\n");
    test60();
// FIXME: These tests are broken with Netatalk 4.0
#if 0
	test63();
	test64();
	test65();
//	test78(); /* badly broken, didn't bother fixing for adouble:ea */
	test79();
#endif
	test80();
	test329();
// FIXME: broken with Netatalk 4.0
#if 0
	test330();
#endif
    test410();
	/* must be the last one */
	test366();
}

