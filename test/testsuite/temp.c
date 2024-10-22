/* ----------------------------------------------
*/
#include "afpclient.h"
#include "test.h"
extern int Noadouble;

uint16_t vol;
DSI *dsi, *dsi2;

#include <signal.h>

static void alarm_handler()
{
	fprintf(stdout,"\tFAILED\n");
	exit(1);
}

/* ------------------------- */
STATIC void test12()
{
uint16_t bitmap = 0;
int fork = 0;
int fork1;
int fork2;
uint16_t vol = VolID;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPRead:test12: read/write\n");

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT,
				"Network Trash Folder/Trash Can Usage Map",OPENACC_RD | OPENACC_DWR);
	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	fork2 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT,
				"Network Trash Folder/Trash Can Usage Map",OPENACC_WR | OPENACC_DWR);
	if (fork2) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork2);
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT,
				"Network Trash Folder/Trash Can Usage Map",OPENACC_RD | OPENACC_DWR);
	if (!fork1) {
		fprintf(stdout,"\tFAILED\n");
		goto fin;
	}
	FPCloseFork(Conn,fork1);

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT,
				"Network Trash Folder/Trash Can Usage Map",OPENACC_WR | OPENACC_DWR);
	if (fork1) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork1);
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT,
				"Network Trash Folder/Trash Can Usage Map",OPENACC_RD | OPENACC_DRD);
	if (fork1) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork1);
		goto fin;
	}

	FPCloseFork(Conn,fork);

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT,
				"Network Trash Folder/Trash Can Usage Map",OPENACC_RD | OPENACC_DRD);

	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT,
				"Network Trash Folder/Trash Can Usage Map",OPENACC_RD );
	if (fork1) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork1);
		goto fin;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT,
				"Network Trash Folder/Trash Can Usage Map",OPENACC_WR );
	if (!fork1) {
		fprintf(stdout,"\tFAILED\n");
		goto fin;
	}
	FPCloseFork(Conn,fork1);
fin:
	FPCloseFork(Conn,fork);
}


/* ------------------------- */
void test24()
{
int fork;
uint16_t bitmap = 0;
char *name  = "Un nom long 0123456789 0123456789 0123456789 0123456789.txt";
char *name1 = "nouveau nom long 0123456789 0123456789 0123456789 0123456789";
char *name2 = "Contents";
int  dir;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test24: mangled name\n");

	FPDelete(Conn, vol,  DIRDID_ROOT , name);
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	if (FPRename(Conn, vol, DIRDID_ROOT, name, "test1")) {fprintf(stdout,"\tFAILED\n");}
	if (FPRename(Conn, vol, DIRDID_ROOT, "test1", name)) {fprintf(stdout,"\tFAILED\n");}

	FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);



	if (FPRename(Conn, vol, DIRDID_ROOT, name, name1)) {
		fprintf(stdout,"\tFAILED\n");
	}
	FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name1, "", name);
	FPDelete(Conn, vol,  DIRDID_ROOT , name1);
	dir  = FPCreateDir(Conn,vol, DIRDID_ROOT , name2);
	if (!dir) {
		fprintf(stdout,"\tFAILED\n");
	}

	FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, name1);
	FPDelete(Conn, vol,  dir , name1);
	FPDelete(Conn, vol,  DIRDID_ROOT, name2);
	FPFlush(Conn, vol);
}


static char temp[MAXPATHLEN];
static char temp1[MAXPATHLEN];

/* ------------------------- */
void test47()
{
uint16_t bitmap = 0;
int fork, fork1;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test47: open read only file read only then read write\n");
    fprintf(stdout,"test47: in a read only folder\n");


	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, "new/toto.txt", OPENACC_RD);

	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, "new/toto.txt",OPENACC_WR | OPENACC_RD);

	if (fork1) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork1);
		FPCloseFork(Conn,fork);
		return;
	}

	FPCloseFork(Conn,fork);

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, "new/toto.txt", OPENACC_RD);

	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork);
		return;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, "new/toto.txt",OPENACC_WR | OPENACC_RD);

	if (fork1) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork1);
		FPCloseFork(Conn,fork);
		return;
	}

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork);
		return;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, "new/toto.txt", OPENACC_RD);

	if (!fork1) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork);
		return;
	}
	FPCloseFork(Conn,fork1);

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork);
		return;
	}

	FPCloseFork(Conn,fork);

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test47: in a read/write folder\n");


	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, "test folder/toto.txt", OPENACC_RD);

	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, "test folder/toto.txt",OPENACC_WR | OPENACC_RD);

	if (fork1) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	FPCloseFork(Conn,fork);

	strcpy(temp, Path);strcat(temp,"/test folder/.AppleDouble/toto.txt");
	if (unlink(temp)) {
		fprintf(stdout,"\tResource fork not there\n");
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, "test folder/toto.txt", OPENACC_RD);

	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, "test folder/toto.txt",OPENACC_WR | OPENACC_RD);

	if (fork1) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	FPCloseFork(Conn,fork);

	strcpy(temp, Path);strcat(temp,"/test folder/.AppleDouble/toto.txt");
	if (unlink(temp)) {
		fprintf(stdout,"\tResource fork not there\n");
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, "test folder/toto.txt", OPENACC_RD);
	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	if (ntohl(AFPERR_EOF) != FPRead(Conn, fork, 0, 100, Data)) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork);
		return;
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, "test folder/toto.txt",OPENACC_WR | OPENACC_RD);
    /* bad, but we are able to open read-write the resource for of a read-only file (data fork)
     * difficult to fix.
     */
	if (!fork1) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	if (FPWrite(Conn, fork1, 0, 10, Data, 0 )) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork);
		FPCloseFork(Conn,fork1);
		return;
	}

	if (FPRead(Conn, fork, 0, 10, Data)) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork);
		FPCloseFork(Conn,fork1);
		return;
	}

	FPCloseFork(Conn,fork);
	if (FPWrite(Conn, fork1, 0, 20, Data, 0x80 )) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork1);
		return;
	}

	if (ntohl(AFPERR_PARAM) != FPRead(Conn, fork, 0, 30, Data)) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork1);
		return;
	}

	FPCloseFork(Conn,fork1);
}

/* -------------------------- */
static char afp_cmd_with_fork[] = {
AFP_BYTELOCK, /* done */
AFP_CLOSEFORK, /* done */
AFP_GETFORKPARAM,
AFP_SETFORKPARAM,
AFP_READ,
AFP_FLUSHFORK,
AFP_WRITE,
};

void test60()
{

}

/* ----------- */
void test68()
{
int ofs;
uint16_t bitmap;
int len;
int did = DIRDID_ROOT;
char *name = "new/test.txt\0";

	if (Conn->afp_version >= 30)
		return;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test68: cname with a trailing 0??\n");

	fprintf(stdout,"---------------------\n");
	fprintf(stdout,"GetFileDirParams Vol %d \n\n", vol);
	memset(dsi->commands, 0, DSI_CMDSIZ);
	dsi->header.dsi_flags = DSIFL_REQUEST;
	dsi->header.dsi_command = DSIFUNC_CMD;
	dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
	ofs = 0;
	dsi->commands[ofs++] = AFP_GETFLDRPARAM;
	dsi->commands[ofs++] = 0;

	memcpy(dsi->commands +ofs, &vol, sizeof(vol));
	ofs += sizeof(vol);

	memcpy(dsi->commands +ofs, &did, sizeof(did));
	ofs += sizeof(did);

	bitmap = htons(1 << FILPBIT_LNAME);
	memcpy(dsi->commands +ofs, &bitmap, sizeof(bitmap));
	ofs += sizeof(bitmap);

	bitmap = htons(DIRPBIT_LNAME);;
	memcpy(dsi->commands +ofs, &bitmap, sizeof(bitmap));
	ofs += sizeof(bitmap);

	dsi->commands[ofs++] = 2;		/* long name */
	len = strlen(name) +1;
	dsi->commands[ofs++] = len;
	u2mac(&dsi->commands[ofs], name, len);
	ofs += len;

	dsi->datalen = ofs;
	dsi->header.dsi_len = htonl(dsi->datalen);
	dsi->header.dsi_code = 0; // htonl(err);

   	my_dsi_stream_send(dsi, dsi->commands, dsi->datalen);
	/* ------------------ */
	my_dsi_stream_receive(dsi, dsi->data, DSI_DATASIZ, &dsi->datalen);

	dump_header(dsi);
	if (dsi->header.dsi_code != 0) {
		fprintf(stdout,"\tFAILED\n");
	}
}


/* -------------------------
   dirA
     child --> dirB
                next --> dirB
                prev --> dirB

     child --> dirB
                next --> dirA1
                prev --> dirA1
                           next --> dirB
                           prev --> dirB
                
     child --> dirA2
                next --> dirA1                
   dirC
     child --> symX
                 next --> dirA1
                 prev --> dirA1


   dirA
     child --> dirB
                next --> dirB
                prev --> dirA1

     
*/
void test85()
{
struct sigaction action;
struct itimerval    it;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"t85: test bogus symlink\n");

   	it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 15;
    it.it_value.tv_usec = 0;

	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	strcpy(temp, Path);strcat(temp,"/dirC");
	/* need to ckeck we have
     * dirC/
          folder
          symlink to dirA/dirB
		  folder2
       an readdir return folder before symlink befor folder2!
     */
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGTERM);
    action.sa_flags = SA_RESTART | SA_ONESHOT;
    if ((sigaction(SIGALRM, &action, NULL) < 0) ||
            (setitimer(ITIMER_REAL, &it, NULL) < 0)) {
		fprintf(stdout,"\tFAILED\n");
		return;
    }

	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "dirA",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
    }


	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "dirC",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
    }


	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "dirA",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
    }


	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "dirC",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
    }


   	it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 0;
   	setitimer(ITIMER_REAL, &it, NULL);

}

/* ---------------------- */
test86() {
int  dir;
char *name = "t86 new dir";
char *name1 = "t86 admin create file";
uint16_t vol2;
int fork;
uint16_t bitmap = 0;
uint16_t dbitmap = (1 << DIRPBIT_ACCESS);
struct afp_filedir_parms filedir;
int  ofs =  3 * sizeof( uint16_t );


    fprintf(stdout,"===================\n");
    fprintf(stdout,"t86: test file/dir created by admin\n");

	if (!Conn2) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;

	}

	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (!(dir = FPCreateDir(Conn2,vol2, DIRDID_ROOT , name))) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	if (FPGetFileDirParams(Conn2, vol2,  DIRDID_ROOT , name, 0, dbitmap )) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi2->data +ofs, 0, dbitmap);
        filedir.access[2] = 3; /* group read only */
 		if (FPSetDirParms(Conn2, vol2, DIRDID_ROOT , name, dbitmap, &filedir)) {
			fprintf(stdout,"\tFAILED\n");
 		}
	}


	if (FPCreateFile(Conn2, vol2,  0, dir , name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap , dir, name1, OPENACC_WR );
	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
	}
    else {
		FPCloseFork(Conn,fork);
    }

	if (FPDelete(Conn2, vol2,  dir , name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPCreateFile(Conn, vol,  0, dir , name1)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (FPDelete(Conn, vol,  dir , name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPDelete(Conn2, vol2,  DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
	}
	FPCloseVol(Conn2,vol2);

}

/* ---------------------- */
test87() {
int  dir;
char *name = "t87 dir without .Appledoube";
char *name1 = "t87 file without .Appledoube";

    fprintf(stdout,"===================\n");
    fprintf(stdout,"t87: add comment file/dir without .Appledouble\n");

	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	strcpy(temp, Path);strcat(temp,"/");strcat(temp, name);strcat(temp,"/.AppleDouble/.Parent");
	if (unlink(temp)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPAddComment(Conn, vol,  DIRDID_ROOT , name, "Comment for test folder")) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPCreateFile(Conn, vol,  0, dir , name1)) {
		fprintf(stdout,"\tFAILED\n");
	}
	strcpy(temp, Path);strcat(temp,"/");strcat(temp, name);
	strcat(temp,"/.AppleDouble/");strcat(temp,name1);
	if (unlink(temp)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPAddComment(Conn, vol,  dir , name1, "Comment for test folder")) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPDelete(Conn, vol,  dir , name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
	}
}

/* ------------------------- */
void test89()
{
int  dir;
char *name = "t89 test error setfilparam";
char *name1 = "t89 error setfilparams dir";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap = (1<<FILPBIT_FINFO)| (1<<FILPBIT_CDATE) |
					(1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE);



    fprintf(stdout,"===================\n");
    fprintf(stdout,"t89: test set file setfilparam\n");

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name1))) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , "test/2.txt", bitmap,0)) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
 		if (ntohl(AFPERR_BADTYPE) != FPSetFileParams(Conn, vol, DIRDID_ROOT , name1, bitmap, &filedir)) {
			fprintf(stdout,"\tFAILED\n");
 		}
 		if (ntohl(AFPERR_ACCESS) != FPSetFileParams(Conn, vol, DIRDID_ROOT , "test/2.txt", bitmap, &filedir)) {
			fprintf(stdout,"\tFAILED\n");
 		}
 		if (ntohl(AFPERR_ACCESS) != FPSetFileParams(Conn, vol, DIRDID_ROOT , "bar/2.txt", bitmap, &filedir)) {
			fprintf(stdout,"\tFAILED\n");
 		}
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT , name1)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

}


/* -------------------------------------- */
void test92()
{
struct sigaction action;
struct itimerval    it;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"t92: test bogus symlink 2\n");

   	it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 15;
    it.it_value.tv_usec = 0;

	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	strcpy(temp, Path);strcat(temp,"/dirC");
	/* need to ckeck we have
     * dirC/
          folder
          symlink to dirA/dirB
		  folder2
       an readdir return folder before symlink befor folder2!
     */
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGHUP);
    sigaddset(&action.sa_mask, SIGTERM);
    action.sa_flags = SA_RESTART | SA_ONESHOT;
    if ((sigaction(SIGALRM, &action, NULL) < 0) ||
            (setitimer(ITIMER_REAL, &it, NULL) < 0)) {
		fprintf(stdout,"\tFAILED\n");
		return;
    }

	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "dirD",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
    }


	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "dirC",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
    }


	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "dirD",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
    }


   	it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 0;
   	setitimer(ITIMER_REAL, &it, NULL);

}

/* ------------------------- */
void test97()
{
int fork;
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "t97 mswindows �";
char *name1 = "t97 new file name";
char *name2 = "Contents";
char *newv = strdup(Vol);
int  l = strlen(newv);
int  vol1;

	newv[1] = newv[1] +1;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test97: options mswindows\n");
	vol1  = FPOpenVol(Conn, newv);
	if (ntohl(AFPERR_PARAM) != dsi->header.dsi_code) {
		fprintf(stdout,"\tFAILED\n");
		if (!dsi->header.dsi_code)
			FPCloseVol(Conn,vol1);
		return;
	}
	newv[1] = newv[1] -1;
    newv[l -1] += 2;		/* was test5 open test7 */
	vol1  = FPOpenVol(Conn, newv);
	if (dsi->header.dsi_code) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	if (FPEnumerate(Conn, vol1,  DIRDID_ROOT , "",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
    }
	if (FPCreateFile(Conn, vol1,  0, DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPEnumerate(Conn, vol1,  DIRDID_ROOT , "",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
    }

	if (FPDelete(Conn, vol1,  DIRDID_ROOT , name)) { fprintf(stdout,"\tFAILED\n");}
	goto end;
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name2))) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT , name2)) { fprintf(stdout,"\tFAILED\n");}

	/* sdid bad */
	if (ntohl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, dir, vol, DIRDID_ROOT, name, "", name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	FPCloseVol(Conn,vol);
	vol  = FPOpenVol(Conn, Vol);
	/* cname unchdirable */
	if (ntohl(AFPERR_ACCESS) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, "bar", "", name1)) {
		fprintf(stdout,"\tFAILED\n");
	}
	/* second time once bar is in the cache */
	if (ntohl(AFPERR_ACCESS) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, "bar", "", name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	if (!(dir1 = FPCreateDir(Conn,vol, DIRDID_ROOT , name2))) {fprintf(stdout,"\tFAILED\n");}

	/* source is a dir */
	if (ntohl(AFPERR_BADTYPE) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name2, "", name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name,OPENACC_WR | OPENACC_RD);
	if (fork) {
		if (ntohl(AFPERR_DENYCONF) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1)) {
			fprintf(stdout,"\tFAILED\n");
		}
		FPCloseFork(Conn,fork);
	}
	/* dvol bad */
	if (ntohl(AFPERR_PARAM) != FPCopyFile(Conn, vol, DIRDID_ROOT, vol +1, dir, name, "", name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	/* ddid bad */
	if (ntohl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, DIRDID_ROOT , vol, dir,  name, "", name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	/* ok */
	if (FPCopyFile(Conn, vol, DIRDID_ROOT, vol, DIRDID_ROOT, name, "", name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	FPDelete(Conn, vol,  DIRDID_ROOT, name);
	FPDelete(Conn, vol,  DIRDID_ROOT, name1);
	FPDelete(Conn, vol,  DIRDID_ROOT, name2);
	FPFlush(Conn, vol);
end:
	FPCloseVol(Conn,vol1);
	free(newv);
}

/* ------------------------- */
void test109()
{
int fork;
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "t109 exchange file cross dev";
char *name1 = "t109 new file name";
char *ndir  = "folder_symlink/dir";
int fid_name;
int fid_name1;
int ret;
struct stat st;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test109: exchange files cross dev\n");

	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}
	strcpy(temp, Path);strcat(temp,"/folder_symlink");
	if (stat(temp, &st)) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){fprintf(stdout,"\tFAILED\n");}
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {fprintf(stdout,"\tFAILED\n");}
	if (FPCreateFile(Conn, vol,  0, dir, name1)){fprintf(stdout,"\tFAILED\n");}
	fid_name  = get_fid( DIRDID_ROOT , name);
	fid_name1 = get_fid( dir , name1);
	write_fork( DIRDID_ROOT , name, "blue");
	write_fork( dir , name1, "red");

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

	/* ok */
	if (FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1)) {
		fprintf(stdout,"\tFAILED\n");
	}
	read_fork( DIRDID_ROOT , name, 3);
	if (strcmp(Data,"red")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be red\n");
		}

	}
	read_fork( dir , name1, 4);
	if (strcmp(Data,"blue")) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED should be blue\n");
		}
	}
	if ((ret = get_fid(DIRDID_ROOT , name)) != fid_name) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", ret, fid_name);
		}
	}

	if ((ret = get_fid(dir , name1)) != fid_name1) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", ret, fid_name);
		}
	}

	if (FPDelete(Conn, vol,  dir , name1)) { fprintf(stdout,"\tFAILED\n");}

	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) { fprintf(stdout,"\tFAILED\n");}
	FPDelete(Conn, vol,  DIRDID_ROOT, ndir);
	FPFlush(Conn, vol);
}

/* ------------------------- */
void test110()
{
int fork;
int fork1;
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "t110 exchange file cross dev";
char *name1 = "t110 new file name";
char *ndir  = "folder_symlink/dir";
int fid_name;
int fid_name1;
struct stat st;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test110: error exchange files cross dev\n");

	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}
	strcpy(temp, Path);strcat(temp,"/folder_symlink");
	if (stat(temp, &st)) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){fprintf(stdout,"\tFAILED\n");}
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {fprintf(stdout,"\tFAILED\n");}
	if (FPCreateFile(Conn, vol,  0, dir, name1)){fprintf(stdout,"\tFAILED\n");}
	fid_name  = get_fid( DIRDID_ROOT , name);
	fid_name1 = get_fid( dir , name1);
	write_fork( DIRDID_ROOT , name, "blue");
	write_fork( dir , name1, "red");

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (ntohl(AFPERR_MISC) != FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,dir, name1, OPENACC_WR | OPENACC_RD);
	if (!fork1) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (ntohl(AFPERR_MISC) != FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (fork) FPCloseFork(Conn,fork);

	if (ntohl(AFPERR_MISC) != FPExchangeFile(Conn, vol, DIRDID_ROOT, dir, name, name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (fork1) FPCloseFork(Conn,fork1);

	if (FPDelete(Conn, vol,  dir , name1)) { fprintf(stdout,"\tFAILED\n");}

	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) { fprintf(stdout,"\tFAILED\n");}
	if (FPDelete(Conn, vol,  DIRDID_ROOT, ndir)) { fprintf(stdout,"\tFAILED\n");}
	FPFlush(Conn, vol);
}

/* ------------------------- */
void test112()
{
int dir;
uint16_t bitmap = 0;
char *name  = "t112 move, rename across dev";
char *name1 = "t112 new file name";
char *ndir  = "folder_symlink/dir";
int fid_name;
int fid_name1;
int ret;
struct stat st;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test112: move and rename file across dev\n");

	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}
	strcpy(temp, Path);strcat(temp,"/folder_symlink");
	if (stat(temp, &st)) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){fprintf(stdout,"\tFAILED\n");}
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {fprintf(stdout,"\tFAILED\n");}

	fid_name  = get_fid( DIRDID_ROOT , name);

	if (FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if ((ret = get_fid(dir , name1)) != fid_name) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", ret, fid_name);
		}
	}

	if (FPDelete(Conn, vol,  dir , name1)) { fprintf(stdout,"\tFAILED\n");}

	if (!FPDelete(Conn, vol,  DIRDID_ROOT, name)) { fprintf(stdout,"\tFAILED\n");}
	if (FPDelete(Conn, vol,  DIRDID_ROOT, ndir)) { fprintf(stdout,"\tFAILED\n");}
	FPFlush(Conn, vol);
}

/* ------------------------- */
void test113()
{
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "t113 move, dir across dev";
char *name1 = "t113 new dir name";
char *odir  = "t113 dir";
char *ndir  = "folder_symlink/dir";
int fid_name;
int fid_name1;
int ret;
struct stat st;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test113: move and rename file across dev\n");

	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}
	strcpy(temp, Path);strcat(temp,"/folder_symlink");
	if (stat(temp, &st)) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , odir))) {fprintf(stdout,"\tFAILED\n");}
	if (!(dir1 = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {fprintf(stdout,"\tFAILED\n");}

	if (FPCreateFile(Conn, vol,  0, dir , name)){fprintf(stdout,"\tFAILED\n");}

	fid_name  = get_fid( dir , name);

	if (FPMoveAndRename(Conn, vol, dir, dir1, "", name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if ((ret = get_fid(dir , name)) != fid_name) {
		if (!Quiet) {
			fprintf(stdout,"\tFAILED %x should be %x\n", ret, fid_name);
		}
	}

	if (FPDelete(Conn, vol,  dir , name)) { fprintf(stdout,"\tFAILED\n");}

	if (FPDelete(Conn, vol,  dir, "")) { fprintf(stdout,"\tFAILED\n");}
	if (FPDelete(Conn, vol,  dir1, "")) { fprintf(stdout,"\tFAILED\n");}
	if (!FPDelete(Conn, vol,  DIRDID_ROOT, odir)) { fprintf(stdout,"\tFAILED\n");}
	FPFlush(Conn, vol);
}

/* ------------------------- */
void test114()
{
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "bogus folder/t114 file";
char *name1 = "t113 new dir name";
char *odir  = "t113 dir";
char *ndir  = "folder_symlink/dir";
int fid_name;
int fid_name1;
int temp;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test114: Various errors\n");

	if (ntohl(AFPERR_ACCESS) != FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		fprintf(stdout,"\tFAILED\n");
	}
	if (!FPDelete(Conn, vol,  DIRDID_ROOT, name)) {
		fprintf(stdout,"\tFAILED create returned an error but the file is there (DF only)\n");
	}

	if (ntohl(AFPERR_BITMAP) != FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
		                    (1 << FILPBIT_PDINFO )|(1 << FILPBIT_EXTDFLEN) | (1 << FILPBIT_EXTRFLEN)
		                    |(1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN), 0xffff)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (ntohl(AFPERR_BITMAP) != FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
		                    0xffff,
		        (1<< DIRPBIT_ATTR) |
				(1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS))
	) {
		fprintf(stdout,"\tFAILED\n");
	}
    bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME)|(1<< DIRPBIT_OFFCNT);
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "new-rw", 0, bitmap)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (filedir.offcnt != 0) {
			fprintf(stdout,"\tFAILED %d\n",filedir.offcnt);
		}
	}
	if (ntohl(AFPERR_DIRNEMPT) != FPDelete(Conn, vol,  DIRDID_ROOT, "new-rw")) {
		fprintf(stdout,"\tFAILED\n");
	}
}

/* ------------------------- */
void test115()
{
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "bogus folder/t114 file";
char *name1 = "t113 new dir name";
char *odir  = "t113 dir";
char *ndir  = "folder_symlink/dir";
int fid_name;
int fid_name1;
int temp;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t dt;

	dt = FPOpenDT(Conn,vol);
	if (FPGetIcon(Conn,  dt, "SITx", "TEXT", 4, 64 )) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPGetIcon(Conn,  dt, "SITx", "APPL", 1, 256 )) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPGetIconInfo(Conn,  dt, "SITx", 1 )) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (htonl(AFPERR_NOITEM) != FPGetIconInfo(Conn,  dt, "SITx", 256 )) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPGetIconInfo(Conn,  dt, "UNIX", 1 )) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (htonl(AFPERR_NOITEM) != FPGetIconInfo(Conn,  dt, "UNIX", 2 )) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPGetIcon(Conn,  dt, "UNIX", "TEXT", 1, 256 )) {
		fprintf(stdout,"\tFAILED\n");
	}

	FPCloseDT(Conn,dt);

}

/* ------------------------- */
static void test_bytelock_ex(uint16_t vol2, char *name, int type)
{
int fork;
int fork1;
uint16_t bitmap = 0;
int len = (type == OPENFORK_RSCS)?(1<<FILPBIT_RFLEN):(1<<FILPBIT_DFLEN);
struct flock lock;
int fd;
int ret;

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	fork = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name, OPENACC_WR |OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
		FPDelete(Conn, vol,  DIRDID_ROOT, name);
		return;
	}

	if (FPSetForkParam(Conn, fork, len , 50)) {fprintf(stdout,"\tFAILED\n");}
	if (FPByteLock(Conn, fork, 0, 0 , 0 , 100)) {fprintf(stdout,"\tFAILED\n");}
	if (FPRead(Conn, fork, 0, 40, Data)) {fprintf(stdout,"\tFAILED\n");}
	if (FPWrite(Conn, fork, 10, 40, Data, 0)) {fprintf(stdout,"\tFAILED\n");}
	fork1 = FPOpenFork(Conn, vol, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (fork1) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn,fork);
	}

	strcpy(temp, Path);
	strcat(temp,(type == OPENFORK_RSCS)?"/.AppleDouble/":"/");
	strcat(temp, name);

	fd = open(temp, O_RDWR, 0);
	if (fd >= 0) {
	  	lock.l_start = 60;
    	lock.l_type = F_WRLCK;
	    lock.l_whence = SEEK_SET;
    	lock.l_len = 300;

    	if ((ret = fcntl(fd, F_SETLK, &lock)) >= 0 || (errno != EACCES && errno != EAGAIN)) {
    	    if (!ret >= 0)
    	    	errno = 0;
    		perror("fcntl ");
			fprintf(stdout,"\tFAILED \n");
    	}
    	fcntl(fd, F_UNLCK, &lock);
    	close(fd);
	}
	else {
    	perror("open ");
		fprintf(stdout,"\tFAILED \n");
	}
	fork1 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (fork1) {
		fprintf(stdout,"\tFAILED\n");
		FPCloseFork(Conn2,fork1);
	}

	FPCloseFork(Conn,fork);
	fork1 = FPOpenFork(Conn2, vol2, type , bitmap ,DIRDID_ROOT, name,OPENACC_WR |OPENACC_RD);
	if (!fork1) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		FPCloseFork(Conn2,fork1);
	}
	FPDelete(Conn, vol,  DIRDID_ROOT, name);
}

/* -------------------- */
void test117()
{
char *name = "t117 exclusive open DF";
uint16_t vol2;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"t117: test open excl mode, local access with fcntl()\n");
	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	if (!Conn2) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);

	test_bytelock_ex(vol2, name, OPENFORK_DATA);

	name = "t117 exclusive open RF";
	test_bytelock_ex(vol2, name, OPENFORK_RSCS);

	FPCloseVol(Conn2,vol2);
}

/* -------------------- */
void test119()
{
int dir;
char *ndir = "t119 bogus dir";
char *name = "t119 delete read only file";
uint16_t vol2;
struct flock lock;
int fd;
int err;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"t119: delete read only/local fcntl locked file\n");
	if (!Path || !Conn2) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	if (FPCreateFile(Conn, vol,  0, dir , name)) {fprintf(stdout,"\tFAILED\n");}

	sprintf(temp, "%s/%s/%s", Path, ndir, name);
	fd = open(temp, O_RDWR, 0);
	if (fd >= 0) {
	  	lock.l_start = 0;
    	lock.l_type = F_WRLCK;
	    lock.l_whence = SEEK_SET;
    	lock.l_len = 0;

    	if (fcntl(fd, F_SETLK, &lock) < 0) {
    		perror("fcntl ");
			fprintf(stdout,"\tFAILED \n");
    	}
	}
	else {
    	perror("open ");
		fprintf(stdout,"\tFAILED \n");
	}

	if (htonl(AFPERR_BUSY) != (err = FPDelete(Conn, vol,  dir , name))) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (fd >= 0) {
    	fcntl(fd, F_UNLCK, &lock);
    	close(fd);
    }

	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);

	if (FPDelete(Conn, vol,  dir , name)) { fprintf(stdout,"\tFAILED\n");}
	if (FPDelete(Conn, vol,  DIRDID_ROOT , ndir)) { fprintf(stdout,"\tFAILED\n");}
	FPCloseVol(Conn2,vol2);
}

/* ------------------------------ */
void test124()
{
int  dir;
char *name = "t124 dir1";

int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test124: dangling symlink\n");

	if (!Path ) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name))) {fprintf(stdout,"\tFAILED\n");}

	sprintf(temp, "%s/%s/none", Path, name);
	sprintf(temp1,"%s/%s/link", Path, name);

	if (symlink(temp, temp1) < 0) {
		perror("symlink ");
		fprintf(stdout,"\tFAILED\n");
	}
    bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME)|(1<< DIRPBIT_OFFCNT);
	if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (filedir.offcnt != 1) {
			fprintf(stdout,"\tFAILED %d\n",filedir.offcnt);
		}
	}
	if (htonl(AFPERR_NOOBJ) != FPEnumerate(Conn, vol,  dir , "",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (filedir.offcnt != 0) {
			fprintf(stdout,"\tFAILED %d\n",filedir.offcnt);
		}
	}

	if (FPDelete(Conn, vol,  dir , "")) { fprintf(stdout,"\tFAILED\n");}
}

/* ------------------------- */
void test125()
{
int fork;
char *name  = "t125 Un nom long 0123456789 0123456789 0123456789 0123456789.txt";
int  dir;
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test125: mangled name\n");

		fprintf(stdout,"\tFIXME FAILED\n");
		return;

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID)
		 ,
		 0
		)) {
		fprintf(stdout,"\tFAILED\n");
	}

    bitmap = (1<< FILPBIT_PDID)|(1<< FILPBIT_LNAME);
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name , bitmap,0)) {
			fprintf(stdout,"\tFAILED\n");
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, filedir.lname , bitmap,0)) {
			fprintf(stdout,"\tFIXME FAILED\n");
	}
	FPDelete(Conn, vol,  DIRDID_ROOT, name);
	FPFlush(Conn, vol);
}

/* ------------------------- */
void test126()
{
int fork;
char *name  = "t126 Un rep long 0123456789 0123456789 0123456789 0123456789";
int  dir;
int  ofs =  3 * sizeof( uint16_t );
uint16_t bitmap = (1<<FILPBIT_FNUM );
struct afp_filedir_parms filedir;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test126: mangled name\n");

	if (!Conn2) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	if (!(dir = FPCreateDir(Conn, vol, DIRDID_ROOT , name))) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID)
		 ,
		 0
		)) {
		fprintf(stdout,"\tFAILED\n");
	}

    bitmap = (1<< FILPBIT_PDID)|(1<< FILPBIT_LNAME)|(1<< DIRPBIT_OFFCNT);
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name , bitmap,0)) {
			fprintf(stdout,"\tFAILED\n");
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, filedir.lname , bitmap,0)) {
			fprintf(stdout,"\tFAILED\n");
	}
	FPDelete(Conn, vol,  DIRDID_ROOT, name);
	FPFlush(Conn, vol);
}


/* ------------------------- */
void test133()
{
int fork;
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "t133 mswindows dir";
char *name1 = "t133 new name.txt";
char *newv = strdup(Vol);
int  l = strlen(newv);
int  vol1;
int  err;
char *buf = "essai\nsuite\n";
int  len  = strlen(buf);

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test133: options mswindows crlf\n");

    newv[l -1] += 2;		/* was test5 open test7 */
	vol1  = FPOpenVol(Conn, newv);
	if (dsi->header.dsi_code) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	if (!(dir = FPCreateDir(Conn,vol1 , DIRDID_ROOT , name))) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		if (FPRename(Conn, vol1, dir, "", "")) {fprintf(stdout,"\tFAILED\n");}
	}

	if (ntohl(AFPERR_PARAM) != FPCreateFile(Conn, vol1,  0, DIRDID_ROOT , " PRN")) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (ntohl(AFPERR_PARAM) != FPCreateFile(Conn, vol1,  0, DIRDID_ROOT , "P*RN")) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (ntohl(AFPERR_EXIST) != FPCreateFile(Conn, vol1,  0, DIRDID_ROOT , "icon")) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPCreateFile(Conn, vol1,  0, DIRDID_ROOT , "icone")) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPRename(Conn, vol1, DIRDID_ROOT, "icone", "icone")) {fprintf(stdout,"\tFAILED\n");}

	if (ntohl(AFPERR_PARAM) != FPRename(Conn, vol1, DIRDID_ROOT, "icone", " PRN")) {fprintf(stdout,"\tFAILED\n");}

	if (ntohl(AFPERR_PARAM) != FPMoveAndRename(Conn, vol1, DIRDID_ROOT, DIRDID_ROOT, "icone", " PRN")) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPDelete(Conn, vol1,  DIRDID_ROOT , "icone")) { fprintf(stdout,"\tFAILED\n");}

	/* -------------------- */
	if (FPCreateFile(Conn, vol1,  0, DIRDID_ROOT , name1)) {
		fprintf(stdout,"\tFAILED\n");
	}
	fork = FPOpenFork(Conn, vol1, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name1, OPENACC_WR | OPENACC_RD);
	if (!fork) {
	}
	else {
		if (FPWrite(Conn, fork, 0, len, buf, 0 )) {fprintf(stdout,"\tFAILED\n");}
		if (FPRead(Conn, fork, 0, len, Data)) {fprintf(stdout,"\tFAILED\n");}

		FPCloseFork(Conn, fork);
	}

	if (FPDelete(Conn, vol1,  DIRDID_ROOT , name1)) { fprintf(stdout,"\tFAILED\n");}

	if (dir) {
		if (FPDelete(Conn, vol1,  dir , "")) { fprintf(stdout,"\tFAILED\n");}
	}

end:
	FPCloseVol(Conn,vol1);
	free(newv);
}

/* ------------------------- */
void test134()
{
int fork;
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "t134 Copy file";
char *name1 = "t134 new file name";
char *name2 = "bogus folder";
struct afp_filedir_parms filedir;
int  ofs =  3 * sizeof( uint16_t );
int err;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test134: Copy file errors (right access)\n");

	bitmap = (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |(1 << DIRPBIT_ACCESS);
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name2, 0, bitmap)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	dir = filedir.did;

	if (htonl(AFPERR_ACCESS) != (err = FPCopyFile(Conn, vol, DIRDID_ROOT, vol , dir, name, "", name1))) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (ntohl(AFPERR_NOOBJ) !=  FPDelete(Conn, vol,  dir, name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (ntohl(AFPERR_ACCESS) != (err = FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, name1))) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (ntohl(AFPERR_NOOBJ) != FPDelete(Conn, vol,  dir, name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPCopyFile(Conn, vol, dir, vol , DIRDID_ROOT, "test.pdf", "", name1) ) {
		fprintf(stdout,"\tFAILED\n");
	}
	else if (FPDelete(Conn, vol,  DIRDID_ROOT, name1) ) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) {
		fprintf(stdout,"\tFAILED\n");
	}
}

/* ------------------------- */
void test135()
{
int fork;
int dir;
uint16_t bitmap = 0;
char *name  = "t135 move, rename across dev";
char *name1 = "t135 new file name";
char *ndir  = "folder_symlink/dir";
struct stat st;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test135: move and rename file across dev\n");

	if (!Path) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}
	strcpy(temp, Path);strcat(temp,"/folder_symlink");
	if (stat(temp, &st)) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){fprintf(stdout,"\tFAILED\n");}
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {fprintf(stdout,"\tFAILED\n");}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name, OPENACC_WR | OPENACC_RD);
	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (htonl(AFPERR_OLOCK) != FPMoveAndRename(Conn, vol, DIRDID_ROOT, dir, name, name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (fork && FPCloseFork(Conn,fork)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) { fprintf(stdout,"\tFAILED\n");}
	if (FPDelete(Conn, vol,  dir, "")) {
		fprintf(stdout,"\tFAILED\n");
		FPDelete(Conn, vol,  dir, name);
		FPDelete(Conn, vol,  dir, "");
	}
}

/* ------------------------- */
void test140()
{
int fork;
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "t140 Exchange file";
char *name1 = "test.pdf";
char *name2 = "bogus folder";
struct afp_filedir_parms filedir;
int  ofs =  3 * sizeof( uint16_t );
int err;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test140: Exchange file errors (right access)\n");

	bitmap = (1 <<  DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID) |(1 << DIRPBIT_ACCESS);

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name2, 0, bitmap)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	dir = filedir.did;
	if (ntohl(AFPERR_ACCESS) != FPExchangeFile(Conn, vol, DIRDID_ROOT,dir,  name, name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) {
		fprintf(stdout,"\tFAILED\n");
	}
}

/* ------------------------- */
void test142()
{
int fork;
uint16_t bitmap = 0;
char *name  = "dropbox/toto.txt";
struct afp_filedir_parms filedir;
int  ofs =  3 * sizeof( uint16_t );
uint16_t vol2;
int err;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test142: -wx folder\n");

	if (!Conn2) {
		fprintf(stdout,"\tNOT TESTED\n");
		return;
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_WR | OPENACC_RD);
	if (fork || dsi->header.dsi_code != ntohl(AFPERR_LOCK)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (fork) {
		if (FPCloseFork(Conn, fork)) {
			fprintf(stdout,"\tFAILED\n");
		}
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name ,OPENACC_WR | OPENACC_RD);
	if (fork || dsi->header.dsi_code != ntohl(AFPERR_LOCK)) {
		fprintf(stdout,"\tFIXME? FAILED\n");
	}
	if (fork) {
		if (FPCloseFork(Conn, fork)) {
			fprintf(stdout,"\tFAILED\n");
		}
	}

	/* ------------------ */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name , OPENACC_WR );
	if (!fork ) {
		fprintf(stdout,"\tFIXME? FAILED\n");
	}
	else if (FPCloseFork(Conn, fork)) {
		fprintf(stdout,"\tFAILED\n");
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name , OPENACC_WR );
	if (!fork ) {
		fprintf(stdout,"\tFAILED\n");
	}
	else if (FPCloseFork(Conn, fork)) {
		fprintf(stdout,"\tFAILED\n");
	}

	/* ------------------ */
	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	fork = FPOpenFork(Conn2, vol2, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_WR | OPENACC_RD);
	if (fork || dsi2->header.dsi_code != ntohl(AFPERR_ACCESS)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (fork) {
		if (FPCloseFork(Conn2, fork)) {
			fprintf(stdout,"\tFAILED\n");
		}
	}

	fork = FPOpenFork(Conn2, vol2, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name ,OPENACC_WR | OPENACC_RD);
	if (fork || dsi2->header.dsi_code != ntohl(AFPERR_ACCESS)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (fork) {
		if (FPCloseFork(Conn2, fork)) {
			fprintf(stdout,"\tFAILED\n");
		}
	}
	/* ------------------ */
	fork = FPOpenFork(Conn2, vol2, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_WR );
	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
	}
	else if (FPCloseFork(Conn2, fork)) {
		fprintf(stdout,"\tFAILED\n");
	}

	fork = FPOpenFork(Conn2, vol2, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name ,OPENACC_WR );
	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
	}
	else if (FPCloseFork(Conn2, fork)) {
		fprintf(stdout,"\tFAILED\n");
	}

	/* ------------------ */
	fork = FPOpenFork(Conn2, vol2, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_RD );
	if (fork || dsi2->header.dsi_code != ntohl(AFPERR_ACCESS)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (fork) {
		if (FPCloseFork(Conn2, fork)) {
			fprintf(stdout,"\tFAILED\n");
		}
	}

	fork = FPOpenFork(Conn2, vol2, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name ,OPENACC_RD );
	if (fork || dsi2->header.dsi_code != ntohl(AFPERR_ACCESS)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (fork) {
		if (FPCloseFork(Conn2, fork)) {
			fprintf(stdout,"\tFAILED\n");
		}
	}


	FPCloseVol(Conn2,vol2);

}

/* --------------------------------- */
static int is_there(int did, char *name)
{
	return FPGetFileDirParams(Conn, vol,  did, name,
	         (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID)
	         ,
	         (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID)
		);
}

/* ------------------------- */
void test144()
{
int forkd[128];
int fork;
int i;
char *ndir = "test144";
int dir;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test144: lot of open fork\n");

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		fprintf(stdout,"\tFAILED\n");
	}

	/* --------------- */
	for (i=0; i < 128; i++) {
		sprintf(temp, "File.small%d", i);
		if (FPCreateFile(Conn, vol,  0, dir , temp)){
			fprintf(stdout,"\tFAILED\n");
		}
		fork = forkd[i] = FPOpenFork(Conn, vol, OPENFORK_DATA ,
			            (1<<FILPBIT_PDID)|(1<< DIRPBIT_LNAME)|(1<<FILPBIT_FNUM)|(1<<FILPBIT_DFLEN)
			            , dir, temp, OPENACC_WR |OPENACC_RD| OPENACC_DWR| OPENACC_DRD);
		if (!fork) {fprintf(stdout,"\tFAILED\n");}
		if (FPGetForkParam(Conn, fork, (1<<FILPBIT_PDID)|(1<< DIRPBIT_LNAME)|(1<<FILPBIT_DFLEN))
			) {
				fprintf(stdout,"\tFAILED\n");
		}
		if (FPWrite(Conn, fork, 0, 20480, Data, 0 )) {
				fprintf(stdout,"\tFAILED\n");
		}
	}
	for (i=0; i < 128; i++) {
		fork = forkd[i];
		sprintf(temp, "File.small%d", i);
		if (FPCloseFork(Conn,fork)) {fprintf(stdout,"\tFAILED\n");}
		if (FPDelete(Conn, vol,  dir, temp)) {fprintf(stdout,"\tFAILED\n");}
	}

	if (FPDelete(Conn, vol,  dir, "")) {fprintf(stdout,"\tFAILED\n");}
}


/* ------------------------- */
void test147()
{
int fork;
int dir;
int dir1;
uint16_t bitmap = 0;
char *name = "t147 new file name\252 a";

char *newv = strdup(Vol);
int  l = strlen(newv);
int  vol1;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test147: options mswindows\n");
    newv[l -1] += 2;		/* was test5 open test7 */
	vol1  = FPOpenVol(Conn, newv);
	if (dsi->header.dsi_code) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	if (FPEnumerate(Conn, vol1,  DIRDID_ROOT , "",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
    }
	if (FPCreateFile(Conn, vol1,  0, DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
	}
#if 0
	if (FPEnumerate(Conn, vol1,  DIRDID_ROOT , "",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
    }

	if (FPDelete(Conn, vol1,  DIRDID_ROOT , name)) { fprintf(stdout,"\tFAILED\n");}
	goto end;
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , name2))) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	if (FPDelete(Conn, vol,  DIRDID_ROOT , name2)) { fprintf(stdout,"\tFAILED\n");}

	/* sdid bad */
	if (ntohl(AFPERR_NOOBJ) != FPCopyFile(Conn, vol, dir, vol, DIRDID_ROOT, name, "", name1)) {
		fprintf(stdout,"\tFAILED\n");
	}
#endif

	FPDelete(Conn, vol1,  DIRDID_ROOT, name);
end:
	FPCloseVol(Conn,vol1);
	free(newv);
}

/* ------------------------- */
void test149()
{
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "bogus folder/t149 file";
int fork, fork1, fork2, fork3;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test149: Error when no write access to .AppleDouble\n");

	if (ntohl(AFPERR_ACCESS) != FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		fprintf(stdout,"\tFAILED\n");
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_RD);
	if (fork) {
		fprintf(stdout,"\tFIXME FAILED create failed but it's there\n");
		fork1 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name ,OPENACC_RD);
		if (!fork1) {
			fprintf(stdout,"\tFAILED\n");
		}
		fork2 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_RD|OPENACC_WR);
		if (!fork2) {
			fprintf(stdout,"\tFAILED\n");
		}

		fork3 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name ,OPENACC_RD|OPENACC_WR);
		if (fork3) {
			fprintf(stdout,"\tFAILED\n");
		}
		else if (dsi->header.dsi_code != ntohl(AFPERR_LOCK)) {
			fprintf(stdout,"\tFAILED\n");
		}

		if (fork && FPCloseFork(Conn, fork)) {
			fprintf(stdout,"\tFAILED\n");
		}
		if (fork1 && FPCloseFork(Conn, fork1)) {
			fprintf(stdout,"\tFAILED\n");
		}
		if (fork2 && FPCloseFork(Conn, fork2)) {
			fprintf(stdout,"\tFAILED\n");
		}
		if (fork3 && FPCloseFork(Conn, fork3)) {
			fprintf(stdout,"\tFAILED\n");
		}

		if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) {
			fprintf(stdout,"\tFAILED\n");
		}
	}
}

/* ------------------------- */
void test150()
{
int dir;
int dir1;
uint16_t bitmap = 0;
char *name  = "bogus folder/test.pdf";
int fork, fork1, fork2, fork3;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test150: Error when no write access to .AppleDouble\n");

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_RD|OPENACC_WR);
	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	if (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 0)) {
		fprintf(stdout,"\tFAILED\n");
	}

	bitmap = 1 << FILPBIT_DFLEN;
	if (FPGetForkParam(Conn, fork, bitmap)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPWrite(Conn, fork, 0, 2000, Data, 0 )) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (FPRead(Conn, fork, 0, 2000, Data)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (fork && FPCloseFork(Conn, fork)) {
		fprintf(stdout,"\tFAILED\n");
	}

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name ,OPENACC_RD|OPENACC_WR);
	if (!fork) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		if (FPRead(Conn, fork, 0, 2000, Data)) {
			fprintf(stdout,"\tFAILED\n");
		}
		if (FPSetForkParam(Conn, fork, (1<<FILPBIT_DFLEN), 0)) {
			fprintf(stdout,"\tFAILED\n");
		}
	}
	if (fork && FPCloseFork(Conn, fork)) {
		fprintf(stdout,"\tFAILED\n");
	}
}


/* ------------------------- */
void test155()
{
char *name  = "t155.doc";
char *name1 = "t155 new.oc";
uint16_t bitmap = 0;
int fork = 0, fork1 = 0;
int fork2, fork3;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test155: Word save NOT USED\n");

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)){
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_RD);
	FPCloseFork(Conn, fork);

	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name, OPENACC_RD);
	FPCloseFork(Conn, fork);
	/* 1024 */
	fork = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name, OPENACC_RD|OPENACC_WR);
	/* 1280 */
	fork1 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name, OPENACC_RD|OPENACC_WR);


	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1)){
		fprintf(stdout,"\tFAILED\n");
	}

	/* 2048 */
	fork2 = FPOpenFork(Conn, vol, OPENFORK_DATA , bitmap ,DIRDID_ROOT, name1, OPENACC_RD|OPENACC_WR);

	/* 2304 */
	fork3 = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name, OPENACC_RD);

	if (FPDelete(Conn, vol,  DIRDID_ROOT, name)) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (FPDelete(Conn, vol,  DIRDID_ROOT, name1)) {
		fprintf(stdout,"\tFAILED\n");
	}
}


/* ------------------------- */
void test157()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"test157: bad .AppleDouble resource fork\n");

	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)) {
		fprintf(stdout,"\tFAILED\n");
	}

}

/* ------------------------- */
void test159()
{
char *name  = "test.txt";
uint16_t bitmap;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;


    fprintf(stdout,"===================\n");
    fprintf(stdout,"test159: AppleDouble V1 to V2\n");
    bitmap = (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE) |
	         (1<<FILPBIT_DFLEN) | (1<<FILPBIT_RFLEN);

	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT, name, bitmap, 0)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
}

/* ------------------------- */
void test160()
{
char *name  = "OpenFolderListDF\r";
char ndir[4];
char nfile[6];

uint16_t bitmap;
int fork;
int dir;
int  ofs =  3 * sizeof( uint16_t );
int offcnt;
struct afp_filedir_parms filedir;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test160: utf8 name with \\r and Mac Code\n");
    bitmap = (1<< FILPBIT_PDID) | (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_RFLEN);

    fork = FPOpenFork(Conn, vol, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, name,0x33);
    if (fork || ntohl(AFPERR_NOOBJ) != dsi->header.dsi_code) {
		fprintf(stdout,"\tFAILED\n");
		if (fork) FPCloseFork(Conn, fork);
	}
	if (Conn->afp_version >= 30) {
	    return;
	}
	/* ------------------ */

    bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME)|(1<< DIRPBIT_OFFCNT);
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		offcnt = filedir.offcnt;
	}

	ndir[0] = 'a';
	ndir[1] = 0xaa;
	ndir[2] = 'e';
	ndir[3] = 0;
	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (filedir.offcnt != offcnt +1) {
			fprintf(stdout,"\tFAILED is %d want %d\n", filedir.offcnt, offcnt +1);
		}
	}
	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (filedir.offcnt != offcnt +1) {
			fprintf(stdout,"\tFAILED is %d want %d\n", filedir.offcnt, offcnt +1);
		}
	}
	if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	    if (strcmp(filedir.lname, ndir)) {
		    fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, ndir );
	    }
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT , ndir)) {
		fprintf(stdout,"\tFAILED\n");
	}
	/* ----------------- */
	strcpy(nfile, "e.rtf");
	nfile[0] = 0x8e;         /* �.rtf */

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , nfile)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
    bitmap = (1<< FILPBIT_PDID)|(1<< FILPBIT_LNAME);

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, nfile, bitmap,0)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
	    if (strcmp(filedir.lname, nfile)) {
		    fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, nfile);
	    }
	}
	if (FPDelete(Conn, vol,  DIRDID_ROOT , nfile)) {
		fprintf(stdout,"\tFAILED\n");
	}
}

/* ------------------------- */
void test161()
{
char *nfile = ".Essai";
char *ndir  = ".Dir";

uint16_t bitmap;
int fork;
int dir;
int  ofs =  3 * sizeof( uint16_t );
int offcnt;
struct afp_filedir_parms filedir;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test161: leading . in name UTF8 and unix\n");

    bitmap = (1<< DIRPBIT_DID)|(1<< DIRPBIT_LNAME)|(1<< DIRPBIT_OFFCNT);
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		offcnt = filedir.offcnt;
	}

	if (!(dir = FPCreateDir(Conn,vol, DIRDID_ROOT , ndir))) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (filedir.offcnt != offcnt +1) {
			fprintf(stdout,"\tFAILED is %d want %d\n", filedir.offcnt, offcnt +1);
		}
	}
	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, "", 0, bitmap)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (filedir.offcnt != offcnt +1) {
			fprintf(stdout,"\tFAILED is %d want %d\n", filedir.offcnt, offcnt +1);
		}
	}
	if (FPGetFileDirParams(Conn, vol, dir, "", 0, bitmap)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	    if (strcmp(filedir.lname, ndir)) {
		    fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, ndir );
	    }
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT , ndir)) {
		fprintf(stdout,"\tFAILED\n");
	}
	/* ----------------- */
	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , nfile)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
    bitmap = (1<< FILPBIT_PDID)|(1<< FILPBIT_LNAME);

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, nfile, bitmap,0)) {
			fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
	    if (strcmp(filedir.lname, nfile)) {
		    fprintf(stdout,"\tFAILED %s should be %s\n",filedir.lname, nfile);
	    }
	}
	if (FPDelete(Conn, vol,  DIRDID_ROOT , nfile)) {
		fprintf(stdout,"\tFAILED\n");
	}
}

/* ------------------------- */
void test163()
{
int fork;
uint16_t bitmap = 0;
char *name  = "Un nom long 0123456789 0123456789 0123456789 0123456789.txt";
char *name1 = "Un nom long 0123456789 0123456789 0123456789 0123456790.txt";

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test163: mangled names\n");

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);

	if (FPDelete(Conn, vol,  DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPDelete(Conn, vol,  DIRDID_ROOT , name1)) {
		fprintf(stdout,"\tFAILED\n");
	}
}
/* ------------------------- */
void test164()
{
int fork;
uint16_t bitmap = 0;
char *name   = "Un nom long 0123456789 0123456789 0123456789 0123456789.txt";
char *name1  = "Un nom long 0123456789 ~000.txt";
int  ofs =  3 * sizeof( uint16_t );
int offcnt;
struct afp_filedir_parms filedir;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test164: mangled names\n");

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}

	FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
	     (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	     (1<<FILPBIT_CDATE) | (1<< FILPBIT_PDID)
	      ,
		 (1<< DIRPBIT_ATTR) |
		 (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		);

    bitmap = (1<< FILPBIT_PDID)|(1<< FILPBIT_LNAME);

	if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, bitmap,0)) {
			fprintf(stdout,"\tFAILED\n");
		if (FPDelete(Conn, vol,  DIRDID_ROOT , name)) {
			fprintf(stdout,"\tFAILED\n");
		}
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap,0);
		if (FPDelete(Conn, vol,  DIRDID_ROOT , filedir.lname)) {
			fprintf(stdout,"\tFIXME FAILED lname\n");
			if (FPDelete(Conn, vol, DIRDID_ROOT, name1)) {
				fprintf(stdout,"\tFAILED mangle\n");
				FPDelete(Conn, vol,  DIRDID_ROOT , name);
			}
		}
	}
}

/* ------------------------- */
void test177()
{
int  dir;
char *name = ".t177 test nohex,usedots";
char *name1 = "t177.txt";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"t177: tests nohex,usedots options\n");

	dir = FPCreateDir(Conn, vol, DIRDID_ROOT , ".Apple");
	if (dir || ntohl(AFPERR_EXIST) != dsi->header.dsi_code) {
		fprintf(stdout,"\tFAILED\n");
		if (dir) { FPDelete(Conn, vol, dir , "");}
	}

	dir = FPCreateDir(Conn, vol, DIRDID_ROOT , name);
	if (!dir) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	bitmap = (1<< DIRPBIT_ATTR) |(1<<DIRPBIT_FINFO)| (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE);
	if (FPGetFileDirParams(Conn, vol,  dir , "", 0, bitmap)) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
		if (!(filedir.attr & ATTRBIT_INVISIBLE)) {
			fprintf(stdout,"\tFAILED visible\n");
		}
	}
	if (FPCreateFile(Conn, vol,  0, dir , name1)) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , name,
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		fprintf(stdout,"\tFAILED\n");
	}

	if (FPEnumerate(Conn, vol,  DIRDID_ROOT , "",
	         (1<<FILPBIT_LNAME) | (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)
	         ,
		     (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS)
		)
	) {
		fprintf(stdout,"\tFAILED\n");
	}
	if (FPDelete(Conn, vol, dir , name1)) {fprintf(stdout,"\tFAILED\n");}
	if (FPDelete(Conn, vol, dir , "")) {fprintf(stdout,"\tFAILED\n");}
}

/* ------------------------- */
void test178()
{
int  dir;
char *name = ".t178 file.txt";
char *name1= ".t178!file.txt";
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
uint16_t bitmap;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"t178: tests nohex,usedots options\n");

	if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
		fprintf(stdout,"\tFAILED\n");
		return;
	}
	bitmap = (1<< FILPBIT_ATTR) |(1<<FILPBIT_FINFO)| (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE);
	if (FPGetFileDirParams(Conn, vol,  DIRDID_ROOT , name, bitmap, 0)) {
		fprintf(stdout,"\tFAILED\n");
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
		if (!(filedir.attr & ATTRBIT_INVISIBLE)) {
			fprintf(stdout,"\tFAILED\n");
		}
	}
	FPDelete(Conn, vol, DIRDID_ROOT , name);

	if (htonl(AFPERR_PARAM) != FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name1)) {
		fprintf(stdout,"\tFAILED\n");
		FPDelete(Conn, vol, DIRDID_ROOT , name1);
	}
}


/* ------------------------- */
void test997()
{
int ret;
uint32_t len;
uint32_t pid;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test997: send SIGHUP\n");

	if (FPGetSessionToken(Conn,0 ,0 ,0 ,NULL)) {
		fprintf(stdout,"\tFAILED\n");
	}
	memcpy(&len, dsi->data, sizeof(len));
	len = ntohl(len);
	if (len != 4) {
		fprintf(stdout,"\tFAILED\n");
	}
	else if (getuid() == 0) {
		memcpy(&pid, dsi->data +4, sizeof(pid));
		sleep(3);
		if (kill(pid, 1) < 0) {
			fprintf(stdout,"\tFAILED kill(%d,1) %s\n",pid, strerror(errno) );
		}
	}
}

/* ------------------------- */
void test998()
{
int fork;
uint16_t vol2;

	if (!Conn2) {
		return;
	}
    fprintf(stdout,"===================\n");
    fprintf(stdout,"test998: bad packet disconnect\n");
	dsi2 = &Conn2->dsi;
	vol2  = FPOpenVol(Conn2, Vol);
	if (FPBadPacket(Conn2, 4, "staff")) {
		fprintf(stdout,"\tFAILED\n");
	}
	FPCloseVol(Conn2,vol2);
}

/* ------------------------- */
void test999()
{
int fork;
uint16_t bitmap = 0;
char *name  = "dropbox/toto.txt";
struct afp_filedir_parms filedir;
int  ofs =  3 * sizeof( uint16_t );
uint16_t vol2;
int err;

    fprintf(stdout,"===================\n");
    fprintf(stdout,"test999: timeout disconnect \n");
    sleep(60*3);
}
