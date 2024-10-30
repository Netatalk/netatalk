/* ----------------------------------------------
*/
#include "specs.h"

/* ------------------------- */
static unsigned char afp_cmd_with_vol[] = {
AFP_CLOSEVOL,			/* 2 */
#if 0
AFP_CLOSEDIR,			/* 3 */
#endif
AFP_COPYFILE, 			/* 5 */
AFP_CREATEDIR,			/* 6 */
AFP_CREATEFILE,			/* 7 */
AFP_DELETE,				/* 8 */
AFP_ENUMERATE,			/* 9 */
AFP_FLUSH,				/* 10 */

AFP_GETVOLPARAM,		/* 17 */
AFP_MOVE,				/* 23 */
AFP_OPENDIR,			/* 25 */
AFP_OPENFORK,           /* 26 */
AFP_RENAME,				/* 28 */
AFP_SETDIRPARAM,		/* 29 */
AFP_SETFILEPARAM, 		/* 30 */
AFP_SETVOLPARAM,		/* 32 */
AFP_GETFLDRPARAM,		/* 34 */
AFP_SETFLDRPARAM,		/* 35 */
AFP_CREATEID,			/* 39 */
AFP_DELETEID, 			/* 40 */
AFP_RESOLVEID,			/* 41 */
AFP_EXCHANGEFILE,		/* 42 */
AFP_CATSEARCH,			/* 43 */
AFP_OPENDT,				/* 48 */
AFP_GETICON,			/* 51 */
AFP_GTICNINFO,			/* 52 */
AFP_ADDAPPL,			/* 53 */
AFP_RMVAPPL,			/* 54 */
AFP_GETAPPL,			/* 55 */
AFP_ADDCMT,				/* 56 */
AFP_RMVCMT,				/* 57 */
AFP_GETCMT,				/* 58 */
AFP_ADDICON,			/* 192 */
};

STATIC void test35()
{
int ofs;
uint16_t param = VolID+1;
DSI *dsi;
unsigned int ret;
unsigned char cmd;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (!Mac) {
		test_skipped(T_MAC);
		goto test_exit;
	}

	for (unsigned int i = 0 ;i < sizeof(afp_cmd_with_vol);i++) {
		memset(dsi->commands, 0, DSI_CMDSIZ);
		dsi->header.dsi_flags = DSIFL_REQUEST;
		dsi->header.dsi_command = DSIFUNC_CMD;
		dsi->header.dsi_requestID = htons(dsi_clientID(dsi));

		ofs = 0;
		cmd = afp_cmd_with_vol[i];
		dsi->commands[ofs++] = cmd;
		dsi->commands[ofs++] = 0;

		memcpy(dsi->commands +ofs, &param, sizeof(param));
		ofs += sizeof(param);

		dsi->datalen = ofs;
		dsi->header.dsi_len = htonl(dsi->datalen);
		dsi->header.dsi_code = 0;

   		my_dsi_stream_send(dsi, dsi->commands, dsi->datalen);
		my_dsi_cmd_receive(dsi);
		ret = dsi->header.dsi_code;

    	if (ntohl(AFPERR_PARAM) != ret) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED command %3i %s\t result %d %s\n", cmd, AfpNum2name(cmd),ntohl(ret), afp_error(ret));
		}
			test_failed();
    	}
    }
test_exit:
	exit_test("Error:test35: illegal volume (-5019 AFP_ERRPARAM)");
}

/* ----------------------- */
static unsigned char afp_cmd_with_vol_did[] = {
	AFP_COPYFILE, 			/* 5 */
	AFP_CREATEDIR,			/* 6 */
	AFP_CREATEFILE,			/* 7 */
	AFP_DELETE,				/* 8 */
	AFP_OPENDIR,			/* 25 */
	AFP_OPENFORK,           /* 26 */
	AFP_RENAME,				/* 28 */
	AFP_SETDIRPARAM,		/* 29 */
	AFP_SETFILEPARAM, 		/* 30 */
	AFP_GETFLDRPARAM,		/* 34 */
	AFP_SETFLDRPARAM,		/* 35 */
	AFP_ADDAPPL,			/* 53 */
	AFP_RMVAPPL,			/* 54 */
	AFP_ADDCMT,				/* 56 */
	AFP_RMVCMT,				/* 57 */
	AFP_GETCMT,				/* 58 */
};

STATIC void test36()
{
unsigned int i;
int ofs;
uint16_t param = VolID;
char *name = "t36 dir";
int  did;
uint16_t vol = VolID;
DSI *dsi;
unsigned char cmd;
unsigned int ret;

	dsi = &Conn->dsi;

	ENTER_TEST

	memset(dsi->commands, 0, sizeof(dsi->commands));
	did  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!did) {
		test_nottested();
		goto test_exit;
	}
	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) {
		test_nottested();
		goto test_exit;
	}

	for (i = 0 ;i < sizeof(afp_cmd_with_vol_did);i++) {
		memset(dsi->commands, 0, DSI_CMDSIZ);
		dsi->header.dsi_flags = DSIFL_REQUEST;
		dsi->header.dsi_command = DSIFUNC_CMD;
		dsi->header.dsi_requestID = htons(dsi_clientID(dsi));

		ofs = 0;
		cmd = afp_cmd_with_vol_did[i];
		dsi->commands[ofs++] = cmd;
		dsi->commands[ofs++] = 0;

		memcpy(dsi->commands +ofs, &param, sizeof(param));
		ofs += sizeof(param);

		memcpy(dsi->commands +ofs, &did, sizeof(did));  /* directory did */
		ofs += sizeof(did);

		dsi->datalen = ofs;
		dsi->header.dsi_len = htonl(dsi->datalen);
		dsi->header.dsi_code = 0; // htonl(err);

   		my_dsi_stream_send(dsi, dsi->commands, dsi->datalen);
		my_dsi_cmd_receive(dsi);
		ret = dsi->header.dsi_code;
    	if (ntohl(AFPERR_NOOBJ) != ret) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED command %3i %s\t result %d %s\n", cmd, AfpNum2name(cmd),ntohl(ret), afp_error(ret));
		}
			test_failed();
    	}
    }
test_exit:
	exit_test("Error:test36: no folder error (ERR_NOOBJ)");
}


/* -------------------------------------------- */
static unsigned char afp_cmd_with_vol_did1[] = {
		AFP_MOVE,				/* 23 */
		AFP_CREATEID,			/* 39 */
		AFP_EXCHANGEFILE,		/* 42 */
};

STATIC void test37()
{
int ofs;
uint16_t param = VolID;
char *name = "t37 dir";
int  dir;
int  did;
uint16_t vol = VolID;
DSI *dsi;
unsigned int ret;
unsigned char cmd;

	dsi = &Conn->dsi;

	ENTER_TEST

	if (!Mac) {
		test_skipped(T_MAC);
		goto test_exit;
	}

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		test_nottested();
		goto fin;
	}
	did  = dir +1;

	for (unsigned int i = 0 ;i < sizeof(afp_cmd_with_vol_did1);i++) {
		memset(dsi->commands, 0, DSI_CMDSIZ);
		dsi->header.dsi_flags = DSIFL_REQUEST;
		dsi->header.dsi_command = DSIFUNC_CMD;
		dsi->header.dsi_requestID = htons(dsi_clientID(dsi));

		ofs = 0;
		cmd = afp_cmd_with_vol_did1[i];
		dsi->commands[ofs++] = cmd;
		dsi->commands[ofs++] = 0;

		memcpy(dsi->commands +ofs, &param, sizeof(param));
		ofs += sizeof(param);

		memcpy(dsi->commands +ofs, &did, sizeof(did));  /* directory did */
		ofs += sizeof(did);

		dsi->datalen = ofs;
		dsi->header.dsi_len = htonl(dsi->datalen);
		dsi->header.dsi_code = 0;

   		my_dsi_stream_send(dsi, dsi->commands, dsi->datalen);
		my_dsi_cmd_receive(dsi);
		ret = dsi->header.dsi_code;
    	if (ntohl(AFPERR_NOOBJ) != ret) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED command %3i %s\t result %d %s\n", cmd, AfpNum2name(cmd),ntohl(ret), afp_error(ret));
		}
			test_failed();
    	}
    }
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
test_exit:
	exit_test("Error:test37: no folder error ==> ERR_NOOBJ");
}

/* ----------------------
afp_openfork
afp_createfile
afp_setfilparams
afp_copyfile
afp_createid
afp_exchangefiles
dirlookup
afp_setdirparams
afp_createdir
afp_opendir
afp_getdiracl
afp_setdiracl
afp_openvol
afp_getfildirparams
afp_setfildirparams
afp_delete
afp_moveandrename
afp_enumerate
*/

static void cname_test(char *name)
{
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |
	               (1 << DIRPBIT_ACCESS);
uint16_t vol = VolID;
DSI *dsi;

	dsi = &Conn->dsi;

	FAIL (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0,bitmap ))
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);

	if (filedir.pdid != 2) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n",filedir.pdid, 2 );
		}
		test_failed();
	}
	if (strcmp(filedir.lname, name)) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, name );
		}
		test_failed();
	}
	FAIL (FPEnumerate(Conn, vol,  DIRDID_ROOT , "", 0, bitmap))
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0,bitmap )) {
		test_failed();
		return;
	}
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);

	if (filedir.pdid != 2) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n",filedir.pdid, 2 );
		}
		test_failed();
	}
	if (strcmp(filedir.lname, name)) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, name );
		}
		test_failed();
	}

	FAIL (ntohl(AFPERR_NOITEM) != FPGetComment(Conn, vol,  DIRDID_ROOT , name))
	FAIL (FPCloseVol(Conn,vol))
	vol  = FPOpenVol(Conn, Vol);
	if (vol == 0xffff) {
		test_nottested();
		return;
	}
	if (ntohl(AFPERR_NOITEM) != FPGetComment(Conn, vol,  DIRDID_ROOT , name)) {
		test_failed();
	}
	FAIL (FPEnumerate(Conn, vol,  DIRDID_ROOT , "", 0, bitmap))

	if (ntohl(AFPERR_NOITEM) != FPGetComment(Conn, vol,  DIRDID_ROOT , name)) {
		test_failed();
	}
}

/* ------------------------- */
// FIXME: afpd crash in dircache_search_by_did()
STATIC void test95()
{
int dir;
char *name  = "t95 exchange file";
char *name1 = "t95 new file name";
char *name2 = "t95 dir";
char *pname = "t95 --";
int pdir;
int ret;
uint16_t vol = VolID;

	ENTER_TEST

	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!(pdir = no_access_folder(vol, DIRDID_ROOT, pname))) {
		goto test_exit;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name2))) {
		test_nottested();
		goto test_exit;
	}

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name2))

	/* sdid bad */
	FAIL (ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, dir, DIRDID_ROOT, name, name1))

	/* name bad */
	ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1);
	if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_NOID)) {
		test_failed();
	}
	/* a dir */
	ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, "", name1);
	if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_BADTYPE)) {
		test_failed();
	}

	/* cname unchdirable */
	ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, "t95 --/2.txt", name1);
	if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_ACCESS)) {
		test_failed();
	}

	/* FIXME second time once bar is in the cache */
	FAIL (ntohl(AFPERR_ACCESS) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, "t95 --/2.txt", "", name1))

	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
#if 0
	if (!(dir1 = FPCreateDir(Conn,vol, DIRDID_ROOT , name2))) {fprintf(stdout,"\tFAILED\n");}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (fork) {
		if (ntohl(AFPERR_DENYCONF) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1)) {
			fprintf(stdout,"\tFAILED\n");
		}
		FPCloseFork(Conn,fork);
	}
#endif
	/* sdid bad */

	FAIL (ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, DIRDID_ROOT,dir, name, name1))

	/* name bad */
	ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1);
	if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_NOID)) {
		test_failed();
	}
	/* same object */
	FAIL (ntohl(AFPERR_SAMEOBJ) != FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name))

	/* a dir */
	FAIL (ntohl(AFPERR_BADTYPE) != FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, ""))

	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1))

	FAIL (FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1))

	delete_folder(vol, DIRDID_ROOT, pname);
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name1))
test_exit:
	exit_test("Error:test95: exchange files");
}

/* ----------------- */
STATIC void test99()
{
int  dir = 0;
char *name = "t99 dir no access";
uint16_t vol = VolID;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!(dir = no_access_folder(vol, DIRDID_ROOT, name))) {
		goto test_exit;
	}

    cname_test(name);
#if 0
    cname_test("essai permission");
#endif
	delete_folder(vol, DIRDID_ROOT, name);
test_exit:
	exit_test("Error:test99: test folder without access right");
}

/* --------------------- */
STATIC void test100()
{
int dir;
char *name = "t100 no obj error";
char *name1 = "t100 no obj error/none";
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<DIRPBIT_ATTR);
unsigned int ret;
uint16_t vol = VolID;
DSI *dsi;
int dt;

	dsi = &Conn->dsi;

	ENTER_TEST

	dt = FPOpenDT(Conn,vol);
	FAIL (ntohl(AFPERR_NOOBJ) != FPAddComment(Conn, vol,  DIRDID_ROOT , name1,"essai"))
	FAIL (ntohl(AFPERR_NOOBJ) != FPGetComment(Conn, vol,  DIRDID_ROOT , name1))
	FAIL (ntohl(AFPERR_NOOBJ) != FPRemoveComment(Conn, vol,  DIRDID_ROOT , name1))
	FAIL (FPCloseDT(Conn, dt))

	filedir.isdir = 1;
	filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
 	FAIL (ntohl(AFPERR_NOOBJ) != FPSetDirParms(Conn, vol, DIRDID_ROOT , name1, bitmap, &filedir))

	if ((dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		test_failed();
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))
	}
	else if (ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
		test_failed();
	}

	dir = FPOpenDir(Conn,vol, DIRDID_ROOT , name1);
    if (dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code)
		test_failed();

	ret = FPEnumerate(Conn, vol,  DIRDID_ROOT , name1,
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);
	if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NODIR, AFPERR_NODIR)) {
		test_failed();
	}

	if (ntohl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name1, "", name)) {
		test_failed();
	}

	/* FIXME afp_errno in file.c */
	if ((get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		ret = FPCreateID(Conn,vol, DIRDID_ROOT, name1);
		if (ret != ntohl(AFPERR_NOOBJ)) {
			test_failed();
		}
	}
	/* FIXME ? */
	if (ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name1, name)) {
		test_failed();
	}

 	FAIL (ntohl(AFPERR_NOOBJ) != FPSetFileParams(Conn, vol, DIRDID_ROOT , name1, bitmap, &filedir))
 	FAIL (ntohl(AFPERR_NOOBJ) != FPSetFilDirParam(Conn, vol, DIRDID_ROOT , name1, bitmap, &filedir))

	FAIL (ntohl(AFPERR_NOOBJ) != FPRename(Conn, vol, DIRDID_ROOT, name1, name))

	FAIL (ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol,  DIRDID_ROOT , name1))

	FAIL (ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name1, name))
	exit_test("Error:test100: no obj cname error (AFPERR_NOOBJ)");
}

/* --------------------- */
STATIC void test101()
{
int dir;
char *name = "t101 no obj error";
char *ndir = "t101 no";
char *name1 = "t101 no/none";
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<DIRPBIT_ATTR);
uint16_t vol = VolID;
unsigned int ret;
DSI *dsi;
int  dt;

	dsi = &Conn->dsi;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!(dir = no_access_folder(vol, DIRDID_ROOT, ndir))) {
		goto test_exit;
	}
	dt = FPOpenDT(Conn,vol);
	FAIL (ntohl(AFPERR_ACCESS) != FPAddComment(Conn, vol,  DIRDID_ROOT , name1,"essai"))

	ret = FPGetComment(Conn, vol,  DIRDID_ROOT , name1);
	if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_ACCESS)) {
		test_failed();
	}

	FAIL (ntohl(AFPERR_ACCESS) != FPRemoveComment(Conn, vol,  DIRDID_ROOT , name1))
	FAIL (FPCloseDT(Conn, dt))

	filedir.isdir = 1;
	filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
 	FAIL (ntohl(AFPERR_ACCESS) != FPSetDirParms(Conn, vol, DIRDID_ROOT , name1, bitmap, &filedir))

	if ((dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		test_failed();
		FPDelete(Conn, vol,  DIRDID_ROOT , name1);
	}
	else if (ntohl(AFPERR_ACCESS) != dsi->header.dsi_code) {
		test_failed();
	}

	dir = FPOpenDir(Conn,vol, DIRDID_ROOT , name1);
    if (dir || ntohl(AFPERR_ACCESS) != dsi->header.dsi_code) {
		test_failed();
	}

	ret = FPEnumerate(Conn, vol,  DIRDID_ROOT , name1,
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);
	if (not_valid(ret, /* MAC */AFPERR_NODIR, AFPERR_ACCESS)) {
		test_failed();
	}

	FAIL (ntohl(AFPERR_ACCESS) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name1, "", name))

	if ((get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		ret = FPCreateID(Conn,vol, DIRDID_ROOT, name1);
		if (ret != ntohl(AFPERR_ACCESS)) {
			test_failed();
		}
	}

	ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name1, name);
	if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_ACCESS)) {
		test_failed();
	}

 	FAIL (ntohl(AFPERR_ACCESS) != FPSetFileParams(Conn, vol, DIRDID_ROOT , name1, bitmap, &filedir))
	FAIL (ntohl(AFPERR_ACCESS) != FPRename(Conn, vol, DIRDID_ROOT, name1, name))
	FAIL (ntohl(AFPERR_ACCESS) != FPDelete(Conn, vol,  DIRDID_ROOT , name1))

	FAIL (ntohl(AFPERR_ACCESS) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name1, name))
	delete_folder(vol, DIRDID_ROOT, ndir);
test_exit:
	exit_test("Error:test101: access error cname");
}

/* --------------------- */
static void test_comment(uint16_t vol, int dir, char *name)
{
int ret;

	ret = FPAddComment(Conn, vol,  dir, name, "essai");
	if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
		test_failed();
	}

	ret = FPRemoveComment(Conn, vol,  dir , name);
	if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
		test_failed();
	}

	if (!ret) {
		FPAddComment(Conn, vol,  dir , name,"essai");
	}
}

/* -------------- */
STATIC void test102()
{
int dir;
char *name = "t102 access error";
char *name1 = "t102 dir --";
int ret;
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<DIRPBIT_ATTR);
uint16_t vol = VolID;
DSI *dsi;
int  dt;

	dsi = &Conn->dsi;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!(dir = no_access_folder(vol, DIRDID_ROOT, name1))) {
		goto test_exit;
	}

	dt = FPOpenDT(Conn,vol);
	test_comment(vol, DIRDID_ROOT, name1);
	ret = FPGetComment(Conn, vol,DIRDID_ROOT, name1);
	if (not_valid(ret, /* MAC */0, AFPERR_NOITEM)) {
		test_failed();
	}
	FAIL (FPCloseDT(Conn, dt))

	filedir.isdir = 1;
	filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
 	FAIL (ntohl(AFPERR_ACCESS) != FPSetDirParms(Conn, vol, DIRDID_ROOT , name1, bitmap, &filedir))

	if ((dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		test_failed();
		FPDelete(Conn, vol,  DIRDID_ROOT , name1);
	}
	else if (ntohl(AFPERR_EXIST) != dsi->header.dsi_code) {
		test_failed();
	}

	dir = FPOpenDir(Conn,vol, DIRDID_ROOT , name1);
    if (dir) {
    	ret = 0;
    }
    else {
    	ret = dsi->header.dsi_code;
    }
	if (not_valid(ret, /* MAC */0, AFPERR_ACCESS)) {
		test_failed();
	}

	if (ntohl(AFPERR_ACCESS) != FPEnumerate(Conn, vol,  DIRDID_ROOT , name1,
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		test_failed();
	}

	FAIL (ntohl(AFPERR_BADTYPE) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name1, "", name))
	if ((get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		FAIL (ntohl(AFPERR_BADTYPE) != FPCreateID(Conn,vol, DIRDID_ROOT, name1))
	}

	ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name1, name);
	if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_BADTYPE)) {
		test_failed();
	}

 	FAIL (ntohl(AFPERR_BADTYPE) != FPSetFileParams(Conn, vol, DIRDID_ROOT , name1, bitmap, &filedir))

	if (FPRename(Conn, vol, DIRDID_ROOT, name1, name)) {
		test_failed();
	}
	else {
		FPRename(Conn, vol, DIRDID_ROOT, name, name1);
	}

	if (FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name1, name)) {
		test_failed();
	}
	else {
		FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1);
	}
	ret = FPDelete(Conn, vol,  DIRDID_ROOT , name1);
	if (not_valid(ret,0, AFPERR_ACCESS) ) {
		test_failed();
	}
	if (ret)
		delete_folder(vol, DIRDID_ROOT, name1);
test_exit:
	exit_test("Error:test102: access error but not cname");
}

/* --------------------- */
STATIC void test103()
{
int dir;
char *name = "t103 did access error";
char *name1 = "t130 dir --";
int ret;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<< DIRPBIT_DID);
uint16_t vol = VolID;
DSI *dsi;
int  dt;

	dsi = &Conn->dsi;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	if (!(dir = no_access_folder(vol, DIRDID_ROOT, name1))) {
		goto test_exit;
	}

	FAIL (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name1, 0, bitmap))

	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	dir = filedir.did;

	dt = FPOpenDT(Conn,vol);
	test_comment(vol, dir, "");
	ret = FPGetComment(Conn, vol,  dir , "");
	if (not_valid(ret, /* MAC */AFPERR_ACCESS, AFPERR_NOITEM)) {
		test_failed();
	}
	FAIL (FPCloseDT(Conn, dt))

	filedir.isdir = 1;
	filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
	bitmap = (1<< DIRPBIT_ATTR);
 	FAIL (ntohl(AFPERR_ACCESS) != FPSetDirParms(Conn, vol, dir , "", bitmap, &filedir))

	if (ntohl(AFPERR_ACCESS) != FPEnumerate(Conn, vol,  dir , "",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		test_failed();
	}

	ret = FPCopyFile(Conn, vol, dir, vol, DIRDID_ROOT, "", "", name);
	if (not_valid(ret, AFPERR_PARAM, AFPERR_BADTYPE) ) {
		test_failed();
	}

	if ((get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		FAIL (ntohl(AFPERR_BADTYPE) != FPCreateID(Conn,vol, dir, ""))
	}

	ret = FPExchangeFile(Conn, vol, dir, DIRDID_ROOT, "", name);
	if (not_valid(ret, AFPERR_NOOBJ, AFPERR_BADTYPE) ) {
		test_failed();
	}

	ret = FPSetFileParams(Conn, vol, dir , "", bitmap, &filedir);
	if (not_valid(ret, AFPERR_PARAM, AFPERR_BADTYPE) ) {
		test_failed();
	}

	if (FPRename(Conn, vol, dir, "", name)) {
		test_failed();
	}
	else {
		if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
			test_failed();
		}
		FPRename(Conn, vol, DIRDID_ROOT, name, name1);
	}

	if (FPMoveAndRename(Conn, vol, dir, DIRDID_ROOT, "", name)) {
		test_failed();
	}
	else {
		if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, 0, bitmap)) {
			test_failed();
		}
		FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT, name, name1);
	}

	ret = FPDelete(Conn, vol,  dir , "");
	if (not_valid(ret,0, AFPERR_ACCESS) ) {
		test_failed();
	}
	if (ret)
		delete_folder(vol, DIRDID_ROOT, name1);
test_exit:
	exit_test("Error:test103: did access error");
}

/* --------------------- */
STATIC void test105()
{
int dir;
unsigned int err;
char *name = "t105 bad did";
char *name1 = "t105 no obj error/none";
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<DIRPBIT_ATTR);
uint16_t vol = VolID;
DSI *dsi;
int ret;
int  dt;

	dsi = &Conn->dsi;

	ENTER_TEST

    dir = 0;
    err = ntohl(AFPERR_PARAM);
	dt = FPOpenDT(Conn,vol);
	FAIL (err != FPAddComment(Conn, vol, dir, name1,"essai"))
	FAIL (err != FPGetComment(Conn, vol, dir , name1))
	FAIL (err != FPRemoveComment(Conn, vol, dir  , name1))
	FAIL (FPCloseDT(Conn, dt))

	filedir.isdir = 1;
	filedir.attr = ATTRBIT_NODELETE | ATTRBIT_SETCLR ;
 	FAIL (err != FPSetDirParms(Conn, vol, dir , name1, bitmap, &filedir))

	if ((dir = FPCreateDir(Conn,vol, dir , name1))) {
		test_failed();
		FPDelete(Conn, vol,  dir , name1);
	}
	else if (err != dsi->header.dsi_code) {
		test_failed();
	}

	dir = FPOpenDir(Conn,vol, dir , name1);
    if (dir || err != dsi->header.dsi_code) {
		test_failed();
	}

	if (err  != FPEnumerate(Conn, vol,  dir , name1,
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		test_failed();
	}

	FAIL (err != FPCopyFile(Conn, vol, dir, vol, DIRDID_ROOT, name1, "", name))

	/* FIXME afp_errno in file.c */
	if ((get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		if (err != FPCreateID(Conn,vol, dir, name1)) {
			test_failed();
		}
	}
	ret = FPExchangeFile(Conn, vol, dir, DIRDID_ROOT, name1, name);
	if (not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_PARAM)) {
		test_failed();
	}

 	FAIL (err != FPSetFileParams(Conn, vol, dir , name1, bitmap, &filedir))
	FAIL (err != FPRename(Conn, vol, dir, name1, name))
	FAIL (err != FPDelete(Conn, vol,  dir , name1))
	FAIL (err != FPMoveAndRename(Conn, vol, dir, DIRDID_ROOT, name1, name))
	exit_test("Error:test105: bad DID in call");
}

/* -------------------------- */
// FIXME: afpd crash in dircache_search_by_did()
STATIC void test170()
{
uint16_t bitmap = 0;
char *name = "test170.txt";
char *name1 = "newtest170.txt";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
int fork;
int dir;
unsigned int ret;
uint16_t vol = VolID;
DSI *dsi;
int  dt;

	dsi = &Conn->dsi;

	ENTER_TEST

    /* ---- fork.c ---- */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT_PARENT, "",OPENACC_WR | OPENACC_RD);
	if (fork || htonl(AFPERR_PARAM) != dsi->header.dsi_code) {
		test_failed();
		if (fork) FPCloseFork(Conn,fork);
		goto test_exit;
	}

    /* ---- file.c ---- */
	FAIL (htonl(AFPERR_PARAM) != FPCreateFile(Conn, vol,  0, DIRDID_ROOT_PARENT , ""))

	bitmap = (1<<FILPBIT_MDATE);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "", 0, bitmap)) {
		test_failed();
		goto test_exit;
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		filedir.isdir = 0;
 		FAIL (htonl(AFPERR_PARAM) != FPSetFileParams(Conn, vol, DIRDID_ROOT_PARENT , "", bitmap, &filedir))
	}
	/* -------------------- */
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))

	if (htonl(AFPERR_PARAM) != FPCopyFile(Conn, vol, DIRDID_ROOT_PARENT, vol, DIRDID_ROOT, "", "", name1)) {
		test_failed();
		FPDelete(Conn, vol,  DIRDID_ROOT , name1);
		goto test_exit;
	}

	FAIL (htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT_PARENT, name, "", ""))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))

	/* -------------------- */
	if ((get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		ret = FPCreateID(Conn,vol, DIRDID_ROOT_PARENT, "");
		FAIL (htonl(AFPERR_NOOBJ) != ret && htonl(AFPERR_PARAM) != ret )
	}

	/* -------------------- */
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1))

	ret = FPExchangeFile(Conn, vol, DIRDID_ROOT_PARENT,dir, "", name1);
	if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_BADTYPE, AFPERR_NOOBJ)) {
		test_failed();
		goto fin;
	}

	ret = FPExchangeFile(Conn, vol, DIRDID_ROOT, DIRDID_ROOT_PARENT, name, "");
	if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_BADTYPE, AFPERR_NOOBJ)) {
		test_failed();
		goto fin;
	}

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))

	/* ---- directory.c ---- */
	filedir.isdir = 1;
 	FAIL (ntohl(AFPERR_NOOBJ) != FPSetDirParms(Conn, vol, DIRDID_ROOT_PARENT , "", bitmap, &filedir))
 	/* ---------------- */
	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT_PARENT , "");
	if (dir || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
		test_failed();
		goto test_exit;
	}

 	/* ---------------- */
	dir = FPOpenDir(Conn,vol, DIRDID_ROOT_PARENT , "");
    if (dir || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
		test_failed();
		goto test_exit;
	}
 	/* ---- filedir.c ---- */

	if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, DIRDID_ROOT_PARENT, "", 0,
	        (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) | (1<<DIRPBIT_UID) |
	    	(1 << DIRPBIT_GID) |(1 << DIRPBIT_ACCESS))
	) {
		test_failed();
		goto test_exit;
	}

 	/* ---------------- */
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "", 0,bitmap )) {
		test_failed();
		goto test_exit;
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
 		if (ntohl(AFPERR_NOOBJ) != FPSetFilDirParam(Conn, vol, DIRDID_ROOT_PARENT , "", bitmap, &filedir)) {
			test_failed();
			goto test_exit;
 		}
 	}
 	/* ---------------- */
	FAIL (ntohl(AFPERR_NOOBJ) != FPRename(Conn, vol, DIRDID_ROOT_PARENT, "", name1))
 	/* ---------------- */
	FAIL (ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol,  DIRDID_ROOT_PARENT , ""))
 	/* ---------------- */
	FAIL (ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT_PARENT, DIRDID_ROOT, "", name1))
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	FAIL (ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, DIRDID_ROOT_PARENT, name, ""))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))

	/* ---- enumerate.c ---- */
	if (ntohl(AFPERR_NOOBJ) != FPEnumerate(Conn, vol,  DIRDID_ROOT_PARENT , "",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		test_failed();
		goto test_exit;
	}
	/* ---- desktop.c ---- */
	dt = FPOpenDT(Conn,vol);
	FAIL (ntohl(AFPERR_NOOBJ) != FPAddComment(Conn, vol,  DIRDID_ROOT_PARENT , "", "Comment"))
	FAIL (ntohl(AFPERR_NOOBJ) != FPGetComment(Conn, vol,  DIRDID_ROOT_PARENT , ""))
	FAIL (ntohl(AFPERR_NOOBJ) != FPRemoveComment(Conn, vol,  DIRDID_ROOT_PARENT , ""))
	FAIL (FPCloseDT(Conn, dt))

fin:
	FAIL (FPDelete(Conn, vol, DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol, DIRDID_ROOT, name1))
test_exit:
	exit_test("Error:test170: cname error did=1 name=\"\"");
}

/* -------------------------- */
// FIXME: afpd crash in dircache_search_by_did()
STATIC void test171()
{
uint16_t bitmap = 0;
char *tname = "test171";
char *name = "test171.txt";
char *name1 = "newtest171.txt";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
int tdir = DIRDID_ROOT_PARENT;
int fork;
int dir;
unsigned int ret;
uint16_t vol = VolID;
DSI *dsi;
int  dt;

	dsi = &Conn->dsi;

	ENTER_TEST

    /* ---- fork.c ---- */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap , tdir, tname,OPENACC_WR | OPENACC_RD);
	if (fork || htonl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
		test_failed();
		if (fork) FPCloseFork(Conn,fork);
		goto test_exit;
	}

    /* ---- file.c ---- */
	FAIL (htonl(AFPERR_NOOBJ) != FPCreateFile(Conn, vol,  0, tdir, tname))

	bitmap = (1<<FILPBIT_MDATE);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "", 0, bitmap)) {
		test_failed();
		goto test_exit;
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		filedir.isdir = 0;
 		FAIL (htonl(AFPERR_NOOBJ) != FPSetFileParams(Conn, vol, tdir, tname, bitmap, &filedir))
	}
	/* -------------------- */
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	if (htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, tdir , vol, DIRDID_ROOT, tname, "", name1)) {
		test_failed();
		FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))
		goto fin;
	}

	FAIL (htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, tdir, name, "", tname))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))

	/* -------------------- */
	if ((get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		ret = FPCreateID(Conn,vol, tdir, tname);
		if (htonl(AFPERR_NOOBJ) != ret && htonl(AFPERR_PARAM) != ret ) {
			test_failed();
			goto fin;
		}
	}

	/* -------------------- */
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1))
	FAIL (ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, tdir,dir, tname, name1))
	FAIL (ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, DIRDID_ROOT, tdir, name, tname))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))

	/* ---- directory.c ---- */
	filedir.isdir = 1;
 	FAIL (ntohl(AFPERR_NOOBJ) != FPSetDirParms(Conn, vol, tdir, tname, bitmap, &filedir))
 	/* ---------------- */
	dir  = FPCreateDir(Conn,vol, tdir, tname);
	if (dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
		test_failed();
	}

 	/* ---------------- */
	dir = FPOpenDir(Conn,vol, tdir, tname);
    FAIL (dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code)
 	/* ---- filedir.c ---- */

	if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, tdir, tname, 0,
	        (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) | (1<<DIRPBIT_UID) |
	    	(1 << DIRPBIT_GID) |(1 << DIRPBIT_ACCESS))
	) {
		test_failed();
	}

 	/* ---------------- */
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "", 0,bitmap )) {
		test_failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
 		FAIL (ntohl(AFPERR_NOOBJ) != FPSetFilDirParam(Conn, vol, tdir, tname, bitmap, &filedir))
 	}
 	/* ---------------- */
	FAIL (ntohl(AFPERR_NOOBJ) != FPRename(Conn, vol, tdir, tname, name1))
 	/* ---------------- */
	FAIL (ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol,  tdir, tname))
 	/* ---------------- */
	FAIL (ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, tdir, DIRDID_ROOT, tname, name1))
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	FAIL (ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, tdir, name, tname))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))

	/* ---- enumerate.c ---- */
	ret = FPEnumerate(Conn, vol,  tdir, tname,
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);
	if (not_valid_bitmap(ret, BITERR_NOOBJ | BITERR_NODIR, AFPERR_NODIR)) {
		test_failed();
	}
	/* ---- desktop.c ---- */
	dt = FPOpenDT(Conn,vol);
	FAIL (ntohl(AFPERR_NOOBJ) != FPAddComment(Conn, vol, tdir, tname, "Comment"))
	FAIL (ntohl(AFPERR_NOOBJ) != FPGetComment(Conn, vol, tdir, tname))
	FAIL (ntohl(AFPERR_NOOBJ) != FPRemoveComment(Conn, vol, tdir, tname))
	FAIL (FPCloseDT(Conn, dt))

fin:
	FAIL (FPDelete(Conn, vol, DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol, DIRDID_ROOT, name1))
test_exit:
	exit_test("Error:test171: cname error did=1 name=bad name");
}

/* -------------------------- */
// FIXME: afpd crash in dircache_search_by_did()
STATIC void test173()
{
uint16_t bitmap = 0;
char *tname = "test173";
char *name = "test173.txt";
char *name1 = "newtest173.txt";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
int tdir = 0;
int fork;
int dir;
unsigned int ret;
uint16_t vol = VolID;
DSI *dsi;
int  dt;

	dsi = &Conn->dsi;

	ENTER_TEST

    /* ---- fork.c ---- */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap , tdir, tname,OPENACC_WR | OPENACC_RD);
	if (fork || htonl(AFPERR_PARAM) != dsi->header.dsi_code) {
		test_failed();
		if (fork) FPCloseFork(Conn,fork);
		goto test_exit;
	}

    /* ---- file.c ---- */
	FAIL (htonl(AFPERR_PARAM) != FPCreateFile(Conn, vol,  0, tdir, tname))

	bitmap = (1<<FILPBIT_MDATE);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "", 0, bitmap)) {
		test_failed();
		goto fin;
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		filedir.isdir = 0;
 		FAIL (htonl(AFPERR_PARAM) != FPSetFileParams(Conn, vol, tdir, tname, bitmap, &filedir))
	}
	/* -------------------- */
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))

	if (htonl(AFPERR_PARAM) != FPCopyFile(Conn, vol, tdir , vol, DIRDID_ROOT, tname, "", name1)) {
		test_failed();
		goto fin;
	}

	FAIL (htonl(AFPERR_PARAM) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, tdir, name, "", tname))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))

	/* -------------------- */
	if ((get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		ret = FPCreateID(Conn,vol, tdir, tname);
		FAIL (htonl(AFPERR_PARAM) != ret)
	}

	/* -------------------- */
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1))
	ret = FPExchangeFile(Conn, vol, tdir,dir, tname, name1);
	if (htonl(AFPERR_PARAM) != ret && not_valid(ret, /* MAC */AFPERR_NOOBJ, AFPERR_PARAM)) {
		test_failed();
		goto fin;
	}
	FAIL (ntohl(AFPERR_PARAM) != FPExchangeFile(Conn, vol, DIRDID_ROOT, tdir, name, tname))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))

	/* ---- directory.c ---- */
	filedir.isdir = 1;
 	FAIL (ntohl(AFPERR_PARAM) != FPSetDirParms(Conn, vol, tdir, tname, bitmap, &filedir))
 	/* ---------------- */
	dir  = FPCreateDir(Conn,vol, tdir, tname);
	if (dir || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
		test_failed();
		goto test_exit;
	}

 	/* ---------------- */
	dir = FPOpenDir(Conn,vol, tdir, tname);
    if (dir || ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
		test_failed();
		goto test_exit;
	}
 	/* ---- filedir.c ---- */

	if (ntohl(AFPERR_PARAM) != FPGetFileDirParams(Conn, vol, tdir, tname, 0,
	        (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) | (1<<DIRPBIT_UID) |
	    	(1 << DIRPBIT_GID) |(1 << DIRPBIT_ACCESS))
	) {
		test_failed();
		goto test_exit;
	}

 	/* ---------------- */
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "", 0,bitmap )) {
		test_failed();
		goto test_exit;
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
 		FAIL (ntohl(AFPERR_PARAM) != FPSetFilDirParam(Conn, vol, tdir, tname, bitmap, &filedir))
 	}
 	/* ---------------- */
	FAIL (ntohl(AFPERR_PARAM) != FPRename(Conn, vol, tdir, tname, name1))
 	/* ---------------- */
	FAIL (ntohl(AFPERR_PARAM) != FPDelete(Conn, vol,  tdir, tname))
 	/* ---------------- */
	FAIL (ntohl(AFPERR_PARAM) != FPMoveAndRename(Conn, vol, tdir, DIRDID_ROOT, tname, name1))
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	FAIL (ntohl(AFPERR_PARAM) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, tdir, name, tname))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	/* ---- enumerate.c ---- */
	if (ntohl(AFPERR_PARAM) != FPEnumerate(Conn, vol,  tdir, tname,
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		test_failed();
		goto test_exit;
	}
	/* ---- desktop.c ---- */
	dt = FPOpenDT(Conn,vol);
	FAIL (ntohl(AFPERR_PARAM) != FPAddComment(Conn, vol, tdir, tname, "Comment"))
	FAIL (ntohl(AFPERR_PARAM) != FPGetComment(Conn, vol, tdir, tname))
	FAIL (ntohl(AFPERR_PARAM) != FPRemoveComment(Conn, vol, tdir, tname))
	FAIL (FPCloseDT(Conn, dt))

	/* ---- appl.c ---- */
fin:
	FAIL (FPDelete(Conn, vol, DIRDID_ROOT, name))
	FAIL (FPDelete(Conn, vol, DIRDID_ROOT, name1))
test_exit:
	exit_test("Error:test173: did error did=0 name=test173 name");
}

/* -------------------------- */
STATIC void test174()
{
uint16_t bitmap = 0;
char *tname = "test174";
char *name = "test174.txt";
char *name1 = "newtest174.txt";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
int tdir;
int fork;
int dir;
uint16_t vol2;
unsigned int ret;
uint16_t vol = VolID;
DSI *dsi = &Conn->dsi;
DSI *dsi2;
int  dt;

	ENTER_TEST
	if (!Conn2) {
		test_skipped(T_CONN2);
		goto test_exit;
	}

	tdir  = FPCreateDir(Conn,vol, DIRDID_ROOT, tname);
	if (!tdir) {
		test_nottested();
		goto test_exit;
	}

	FAIL (FPGetFileDirParams(Conn, vol,  tdir , "", 0, (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT)))

	FAIL (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "", 0, (1 << DIRPBIT_PDINFO ) | (1 << DIRPBIT_OFFCNT)))

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (vol2 == 0xffff) {
		test_failed();
	}

	if (FPDelete(Conn2, vol2,  DIRDID_ROOT , tname)) {
		test_failed();
		FPDelete(Conn, vol,  tdir , "");
		FPCloseVol(Conn2,vol2);
		goto test_exit;
	}
	FPCloseVol(Conn2,vol2);

    /* ---- fork.c ---- */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap , tdir, tname,OPENACC_WR | OPENACC_RD);
	if (fork || htonl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
		test_failed();
		if (fork) FPCloseFork(Conn,fork);
	}

    /* ---- file.c ---- */
	FAIL (htonl(AFPERR_NOOBJ) != FPCreateFile(Conn, vol,  0, tdir, tname))

	bitmap = (1<<FILPBIT_MDATE);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "", 0, bitmap)) {
		test_failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		filedir.isdir = 0;
 		FAIL (htonl(AFPERR_NOOBJ) != FPSetFileParams(Conn, vol, tdir, tname, bitmap, &filedir))
	}
	/* -------------------- */
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))

	if (htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, tdir , vol, DIRDID_ROOT, tname, "", name1)) {
		test_failed();
		FPDelete(Conn, vol,  DIRDID_ROOT , name1);
	}

	FAIL (htonl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, tdir, name, "", tname))

	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))

	/* -------------------- */
	if ((get_vol_attrib(vol) & VOLPBIT_ATTR_FILEID) ) {
		ret = FPCreateID(Conn,vol, tdir, tname);
		if (htonl(AFPERR_NOOBJ) != ret && htonl(AFPERR_PARAM) != ret ) {
			test_failed();
		}
	}

	/* -------------------- */
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))

	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1))

	ret = FPExchangeFile(Conn, vol, tdir,dir, tname, name1);
	if (ntohl(AFPERR_NOOBJ) != ret) {
		if (ret == htonl(AFPERR_PARAM)) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED (IGNORED) not always the same error code!\n");
			}
			test_skipped(T_NONDETERM);
		}
		else {
			test_failed();
		}
	}
	FAIL (ntohl(AFPERR_NOOBJ) != FPExchangeFile(Conn, vol, DIRDID_ROOT, tdir, name, tname))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name1))

	/* ---- directory.c ---- */
	filedir.isdir = 1;
 	FAIL (ntohl(AFPERR_NOOBJ) != FPSetDirParms(Conn, vol, tdir, tname, bitmap, &filedir))
 	/* ---------------- */
	dir  = FPCreateDir(Conn,vol, tdir, tname);
	if (dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
		test_failed();
	}

 	/* ---------------- */
	dir = FPOpenDir(Conn,vol, tdir, tname);
    if (dir || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
		test_failed();
	}
 	/* ---- filedir.c ---- */

	if (ntohl(AFPERR_NOOBJ) != FPGetFileDirParams(Conn, vol, tdir, tname, 0,
	        (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) | (1<<DIRPBIT_UID) |
	    	(1 << DIRPBIT_GID) |(1 << DIRPBIT_ACCESS))
	) {
		test_failed();
	}

 	/* ---------------- */
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "", 0,bitmap )) {
		test_failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
 		FAIL (ntohl(AFPERR_NOOBJ) != FPSetFilDirParam(Conn, vol, tdir, tname, bitmap, &filedir))
 	}
 	/* ---------------- */
	FAIL (ntohl(AFPERR_NOOBJ) != FPRename(Conn, vol, tdir, tname, name1))
 	/* ---------------- */
	FAIL (ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol,  tdir, tname))
 	/* ---------------- */
	FAIL (ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, tdir, DIRDID_ROOT, tname, name1))
	FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	FAIL (ntohl(AFPERR_NOOBJ) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, tdir, name, tname))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT , name))

	/* ---- enumerate.c ---- */
	if (ntohl(AFPERR_NODIR) != FPEnumerate(Conn, vol,  tdir, tname,
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		test_failed();
	}
	/* ---- desktop.c ---- */
	dt = FPOpenDT(Conn,vol);
	FAIL (ntohl(AFPERR_NOOBJ) != FPAddComment(Conn, vol, tdir, tname, "Comment"))
	FAIL (ntohl(AFPERR_NOOBJ) != FPGetComment(Conn, vol, tdir, tname))
	FAIL (ntohl(AFPERR_NOOBJ) != FPRemoveComment(Conn, vol, tdir, tname))
	FAIL (FPCloseDT(Conn, dt))
test_exit:
	exit_test("Error:test174: did error two users from parent folder did=<deleted> name=test174 name");
}

/* ----------- */
void Error_test()
{
    ENTER_TESTSET
	test35();
	test36();
	test37();
#if 0
	test95();
#endif
	test99();
	test100();
	test101();
	test102();
	test103();
	test105();
// FIXME: Because of an afpd crash, test data cleanup fails
#if 0
	test170();
	test171();
	test173();
#endif
	test174();
}
