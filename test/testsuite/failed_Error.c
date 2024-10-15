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
unsigned int i;
int ofs;
uint16_t param = VolID+1;
DSI *dsi;
unsigned int ret;
unsigned char cmd;

	dsi = &Conn->dsi;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Error:test35: illegal volume (-5019 AFP_ERRPARAM)\n");

	for (i = 0 ;i < sizeof(afp_cmd_with_vol);i++) {
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
			fprintf(stdout,"\tFAILED command %3i %s\t result %d %s\n", cmd, AfpNum2name(cmd),ntohl(ret), afp_error(ret));
			failed_nomsg();
    	}
    }
	exit_test("test35");
}

/* -------------------------------------------- */
static unsigned char afp_cmd_with_vol_did1[] = {
		AFP_MOVE,				/* 23 */
		AFP_CREATEID,			/* 39 */
		AFP_EXCHANGEFILE,		/* 42 */
};

STATIC void test37()
{
unsigned int i;
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

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Errror:test37: no folder error ==> ERR_NOOBJ\n");

	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name);
	if (!dir) {
		nottested();
		goto fin;
	}
	did  = dir +1;

	for (i = 0 ;i < sizeof(afp_cmd_with_vol_did1);i++) {
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
			fprintf(stdout,"\tFAILED command %3i %s\t result %d %s\n", cmd, AfpNum2name(cmd),ntohl(ret), afp_error(ret));
			failed_nomsg();
    	}
    }
fin:
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name)) 
	exit_test("test37");
}


/* ----------- */
void Error_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Various errors\n");
	test35();
	test37();
}

