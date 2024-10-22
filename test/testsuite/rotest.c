#include "specs.h"

#include <getopt.h>

uint16_t VolID;
static DSI *dsi;
CONN *Conn;
CONN *Conn2;

int ExitCode = 0;

DSI *Dsi;

char Data[300000] = "";
/* ------------------------------- */
char    *Server = "localhost";
char    *Server2;
int     Proto = 0;
int     Port = 548;
char    *Password = "";
char    *Vol = "";
char    *User;
char    *User2;
char    *Path;
int     Version = 21;
int     List = 0;
int     Mac = 0;
char    *Test;
static char  *vers = "AFPVersion 2.1";


/* ------------------------------- */
static void connect_server(void)
{
    Conn->type = Proto;
    if (!Proto) {
	int sock;
    	Dsi = &Conn->dsi;
		dsi = Dsi;
	    sock = OpenClientSocket(Server, Port);
        if ( sock < 0) {
        	nottested();
	    	exit(ExitCode);
        }
     	Dsi->protocol = DSI_TCPIP;
	    Dsi->socket = sock;
    }
    else {
	}
}

/* -------------------------------- */
static int really_ro()
{
int dir;
char *ndir = "read only dir";

	if ((dir = FPCreateDir(Conn,VolID, DIRDID_ROOT , ndir))) {
		nottested();
		FAIL (FPDelete(Conn, VolID,  dir , ""))
		return 0;
	}
	return 1;
}

/* -------------------------------- */
static void check_test(unsigned int err)
{
	if (err != ntohl(AFPERR_VLOCK) && err != ntohl(AFPERR_ACCESS) ) {
		failed();
	}
}

/* ----------------- */
static void run_all()
{
char *ndir = "read only dir";
char *nfile = "read only file";
int  ofs =  4 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
char *file;
char *file1;
char *dir;
int  fid;
int  did;
uint16_t bitmap;
uint16_t fork, fork1;
struct afp_volume_parms parms;
DSI *dsi = &Conn->dsi;
uint32_t flen;
unsigned int ret;

	ENTER_TEST
    fprintf(stdout,"===================\n");
    fprintf(stdout,"Read only volume\n");
	VolID = FPOpenVol(Conn, Vol);
	if (VolID == 0xffff) {
		nottested();
		goto test_exit;
	}
	if (!really_ro())
		goto test_exit;

	/* get a directory */
	bitmap = (1 << DIRPBIT_LNAME);
	ret = FPEnumerateFull(Conn, VolID, 1, 1, 800,  DIRDID_ROOT, "", 0 , bitmap);
	if (ret) {
		nottested();
		goto fin;
	}
	filedir.isdir = 1;
	afp_filedir_unpack(&filedir, dsi->data +ofs, 0, bitmap);
	dir = strdup(filedir.lname);
	if (!dir) {
		nottested();
		goto fin;
	}
	fprintf(stdout,"Directory %s\n", dir);
	/* get a file */
	bitmap = (1 << FILPBIT_LNAME);
	ret = FPEnumerateFull(Conn, VolID, 1, 1, 800,  DIRDID_ROOT, "", bitmap, 0);
	if (ret) {
		nottested();
		goto fin;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	file = strdup(filedir.lname);
	if (!file) {
		nottested();
		goto fin;
	}
	fprintf(stdout,"File %s\n", file);

	/* get a second file */
	bitmap = (1 << FILPBIT_LNAME);
	ret = FPEnumerateFull(Conn, VolID, 2, 1, 800,  DIRDID_ROOT, "", bitmap, 0);
	if (ret) {
		nottested();
		goto fin;
	}
	filedir.isdir = 0;
	afp_filedir_unpack(&filedir, dsi->data +ofs, bitmap, 0);
	file1 = strdup(filedir.lname);
	if (!file1) {
		nottested();
		goto fin;
	}
	fprintf(stdout,"Second file %s\n", file1);

	/* TESTS */
	/* -- file.c -- */
	ret = FPCreateFile(Conn, VolID,  0, DIRDID_ROOT , nfile);
	FAIL (ntohl(AFPERR_VLOCK) != ret)
	if (!ret) {
		FAIL (FPDelete(Conn, VolID, DIRDID_ROOT, nfile))
	}

	bitmap = (1<<FILPBIT_FINFO)| (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE);
	if (FPGetFileDirParams(Conn, VolID,  DIRDID_ROOT , file, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data + 3 * sizeof( uint16_t ), bitmap, 0);
 		ret  = FPSetFileParams(Conn, VolID,  DIRDID_ROOT , file, bitmap, &filedir);
 		check_test(ret);
	}
	FAIL (ntohl(AFPERR_VLOCK) != FPCopyFile(Conn, VolID, DIRDID_ROOT, VolID, DIRDID_ROOT, file, "", nfile))

	if (!(get_vol_attrib(VolID) & VOLPBIT_ATTR_FILEID) ) {
		fprintf(stdout,"FileID calls Not supported\n");
	}
	else {
		/* with cnid db, if the volume isn't define as readonly in AppleVolumes.default
		 * CreateID, DeleteID don't return VLOCK
		*/
		ret = FPCreateID(Conn,VolID, DIRDID_ROOT, file);
		if (not_valid(ret, /* MAC */AFPERR_VLOCK, AFPERR_EXISTID )) {
			failed();
		}
		fid = get_fid(Conn, VolID, DIRDID_ROOT, file);
		if (fid) {
			ret = FPDeleteID(Conn, VolID, filedir.did);
			if (not_valid(ret, /* MAC */AFPERR_VLOCK, AFPERR_NOID )) {
				failed();
			}
		}
	}
	ret = FPExchangeFile(Conn, VolID, DIRDID_ROOT, DIRDID_ROOT, file, file1);
	if (not_valid(ret, /* MAC */AFPERR_VLOCK, AFPERR_ACCESS )) {
		failed();
	}

	/* -- volume.c -- */

    bitmap = (1 << VOLPBIT_MDATE );
 	FAIL (FPGetVolParam(Conn, VolID, bitmap))
	afp_volume_unpack(&parms, dsi->commands +sizeof( uint16_t ), bitmap);
	bitmap = (1 << VOLPBIT_BDATE );
 	parms.bdate = parms.mdate;
 	ret = FPSetVolParam(Conn, VolID, bitmap, &parms);
 	check_test(ret);

	/* -- filedir.c -- */
	bitmap = (1<<FILPBIT_FINFO)| (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE);
	if (FPGetFileDirParams(Conn, VolID,  DIRDID_ROOT , file, bitmap,0)) {
		failed();
	}
	else {
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data + 3 * sizeof( uint16_t ), bitmap, 0);
 		ret = FPSetFilDirParam(Conn, VolID,  DIRDID_ROOT , file, bitmap, &filedir);
 		check_test(ret);
	}
	bitmap = (1<<DIRPBIT_FINFO)| (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE);
	if (FPGetFileDirParams(Conn, VolID,  DIRDID_ROOT , dir, 0, bitmap)) {
		failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data + 3 * sizeof( uint16_t ), 0,bitmap);
 		ret = FPSetFilDirParam(Conn, VolID,  DIRDID_ROOT , dir, bitmap, &filedir);
 		check_test(ret);
	}

	FAIL (htonl(AFPERR_VLOCK) != FPRename(Conn, VolID, DIRDID_ROOT, file, nfile))
	FAIL (htonl(AFPERR_VLOCK) != FPRename(Conn, VolID, DIRDID_ROOT, dir, ndir))

	bitmap = (1<<DIRPBIT_OFFCNT);
	if (FPGetFileDirParams(Conn, VolID,  DIRDID_ROOT , dir, 0, bitmap)) {
		failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data + 3 * sizeof( uint16_t ), 0,bitmap);
		ret = FPDelete(Conn, VolID, DIRDID_ROOT, dir);
		if (ret != htonl(AFPERR_VLOCK) ) {
			if (!filedir.offcnt) {
				failed();
			}
			else {
				fprintf(stdout,"\tWARNING \"%s\", not empty FPDelete skipped\n", dir);
			}
		}
	}
	FAIL (htonl(AFPERR_VLOCK) != FPDelete(Conn, VolID, DIRDID_ROOT, file))

	FAIL (htonl(AFPERR_VLOCK) != FPMoveAndRename(Conn, VolID, DIRDID_ROOT, DIRDID_ROOT, file, nfile))
	FAIL (htonl(AFPERR_VLOCK) != FPMoveAndRename(Conn, VolID, DIRDID_ROOT, DIRDID_ROOT, dir, ndir))

	/* -- directory.c -- */
	bitmap = (1<<DIRPBIT_FINFO)| (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE);
	if (FPGetFileDirParams(Conn, VolID,  DIRDID_ROOT , dir, 0, bitmap)) {
		failed();
	}
	else {
		filedir.isdir = 1;
		afp_filedir_unpack(&filedir, dsi->data + 3 * sizeof( uint16_t ), 0,bitmap);
 		ret = FPSetDirParms(Conn, VolID,  DIRDID_ROOT , dir, bitmap, &filedir);
 		check_test(ret);
 	}
	if ((did = FPCreateDir(Conn,VolID, DIRDID_ROOT , ndir))) {
		nottested();
		FAIL (FPDelete(Conn, VolID,  did , ""))
		goto test_exit;
	}
 
 	/* -- fork.c -- */
 	bitmap = 0;
	fork = FPOpenFork(Conn, VolID, OPENFORK_DATA , bitmap ,DIRDID_ROOT, file,OPENACC_RD );

    if (!fork) {
		failed();
    }
	fork1 = FPOpenFork(Conn, VolID, OPENFORK_DATA , bitmap ,DIRDID_ROOT, file, OPENACC_WR );
    if (fork1) {
		failed();
		FAIL (FPCloseFork(Conn, fork1))
    }
    FAIL (FPCloseFork(Conn, fork))
	fork1 = FPOpenFork(Conn, VolID, OPENFORK_DATA , bitmap ,DIRDID_ROOT, file, OPENACC_WR );
    if (fork1) {
		failed();
		FAIL (FPCloseFork(Conn, fork1))
    }

	/* --------- */
	fork = FPOpenFork(Conn, VolID, OPENFORK_DATA , bitmap ,DIRDID_ROOT, file,OPENACC_RD );

    if (!fork) {
		failed();
    }
	bitmap = (1<<FILPBIT_DFLEN);
	ret = FPSetForkParam(Conn, fork, bitmap, 0);
 	check_test(ret);

	if (FPGetForkParam(Conn, fork, bitmap)) {
		failed();
	}
	else if ((flen = get_forklen(dsi, OPENFORK_DATA))) {
		FAIL (FPRead(Conn, fork, 0, min(100, flen), Data))
	}

    FAIL (FPCloseFork(Conn, fork))

	/* ------------- */
	bitmap = 0;
	fork = FPOpenFork(Conn, VolID, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, file,OPENACC_RD );

    if (!fork) {
		failed();
    }
	fork1 = FPOpenFork(Conn, VolID, OPENFORK_RSCS  , bitmap ,DIRDID_ROOT, file, OPENACC_WR );
    if (fork1) {
		failed();
		FAIL (FPCloseFork(Conn, fork1))
    }
    FAIL (FPCloseFork(Conn, fork))
	fork1 = FPOpenFork(Conn, VolID, OPENFORK_RSCS  , bitmap ,DIRDID_ROOT, file, OPENACC_WR );
    if (fork1) {
		failed();
		FAIL (FPCloseFork(Conn, fork1))
    }

	/* --------- */
	fork = FPOpenFork(Conn, VolID, OPENFORK_RSCS , bitmap ,DIRDID_ROOT, file,OPENACC_RD );

    if (!fork) {
		failed();
    }

	ret = FPSetForkParam(Conn, fork, (1<<FILPBIT_RFLEN), 100);
 	check_test(ret);
fin:
	FAIL (FPCloseVol(Conn,VolID))
test_exit:
	exit_test("rotest");
}

/* =============================== */
void usage( char * av0 )
{
    fprintf( stdout,"%s test a read only volume\n", av0);
    fprintf( stdout, "usage:\t%s [-m] [-n] [-t] [-h host] [-p port] [-s vol] [-u user] [-w password] -f [call]\n", av0 );

    fprintf( stdout,"\t-m\tserver is a Mac\n");
    fprintf( stdout,"\t-h\tserver host name (default localhost)\n");
    fprintf( stdout,"\t-p\tserver port (default 548)\n");
    fprintf( stdout,"\t-u\tuser name (default uid)\n");
    fprintf( stdout,"\t-s\tvolume to mount (default home)\n");

    fprintf( stdout,"\t-w\tpassword (default none)\n");
    fprintf( stdout,"\t-1\tAFP 2.1 version (default)\n");
    fprintf( stdout,"\t-2\tAFP 2.2 version\n");
    fprintf( stdout,"\t-3\tAFP 3.0 version\n");
    fprintf( stdout,"\t-4\tAFP 3.1 version\n");
    fprintf( stdout,"\t-5\tAFP 3.2 version\n");
    fprintf( stdout,"\t-6\tAFP 3.3 version\n");
    fprintf( stdout,"\t-7\tAFP 3.4 version\n");
    fprintf( stdout,"\t-v\tverbose\n");
    fprintf( stdout,"\t-V\tvery verbose\n");

    exit (1);
}

/* ------------------------------- */
int main( int ac, char **av )
{
int cc;
int ret;
static char *uam = "Cleartxt Passwrd";

    while (( cc = getopt( ac, av, "vV1234567h:p:u:w:ms:" )) != EOF ) {
        switch ( cc ) {
        case 's':
            Vol = strdup(optarg);
            break;
        case '1':
			vers = "AFPVersion 2.1";
			Version = 21;
			break;
        case '2':
			vers = "AFP2.2";
			Version = 22;
			break;
        case '3':
			vers = "AFPX03";
			Version = 30;
			break;
        case '4':
			vers = "AFP3.1";
			Version = 31;
			break;
        case '5':
			vers = "AFP3.2";
			Version = 32;
			break;
        case '6':
			vers = "AFP3.3";
			Version = 33;
			break;
        case '7':
			vers = "AFP3.4";
			Version = 34;
			break;
		case 'm':
			Mac = 1;
			break;
        case 'h':
            Server = strdup(optarg);
            break;
        case 'u':
            User = strdup(optarg);
            break;
        case 'w':
            Password = strdup(optarg);
            break;
        case 'p' :
            Port = atoi( optarg );
            if (Port <= 0) {
                fprintf(stdout, "Bad port.\n");
                exit(1);
            }
	case 'v':
		Quiet = 0;
		break;
	case 'V':
		Quiet = 0;
		Verbose = 1;
		break;
        default :
            usage( av[ 0 ] );
        }
    }
	/************************************
	 *                                  *
	 * Connection user 1                *
	 *                                  *
	 ************************************/

    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
    	return 1;
    }
	/* ------------------------ */
    connect_server();
	// FIXME: workaround for FPopenLoginExt() being broken
#if 0
    if (Version >= 30) {
		ret = FPopenLoginExt(Conn, vers, uam, User, Password);
	}
	else {
		ret = FPopenLogin(Conn, vers, uam, User, Password);
	}
#else
	ret = FPopenLogin(Conn, vers, uam, User, Password);
#endif
	if (ret) {
		printf("Login failed\n");
		exit(1);
	}
	Conn->afp_version = Version;
	run_all();
   	FPLogOut(Conn);

	return ExitCode;
}
