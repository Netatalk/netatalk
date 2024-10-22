#if 0
#define QUIRK
#endif
#include "specs.h"

#include <dlfcn.h>
#include <getopt.h>

uint16_t VolID;
static DSI *dsi;
CONN *Conn;
CONN *Conn2;

int ExitCode = 0;

#define FN(a) a ## _test
#define EXT_FN(a) extern void FN(a) (void)

EXT_FN(FPAddAPPL);
EXT_FN(FPAddComment);
EXT_FN(FPAddIcon);
EXT_FN(FPByteRangeLock);
EXT_FN(FPByteRangeLockExt);
EXT_FN(FPCloseDir);
EXT_FN(FPCloseDT);
EXT_FN(FPCloseFork);
EXT_FN(FPCloseVol);
EXT_FN(FPCopyFile);
EXT_FN(FPCreateDir);
EXT_FN(FPCreateFile);
EXT_FN(FPDelete);
EXT_FN(FPDisconnectOldSession);
EXT_FN(FPEnumerate);
EXT_FN(FPEnumerateExt);
EXT_FN(FPEnumerateExt2);
EXT_FN(FPExchangeFiles);
EXT_FN(FPFlush);
EXT_FN(FPFlushFork);
EXT_FN(FPGetAPPL);
EXT_FN(FPGetComment);
EXT_FN(FPGetFileDirParms);
EXT_FN(FPGetSessionToken);
EXT_FN(FPGetSrvrInfo);
EXT_FN(FPGetSrvrMsg);
EXT_FN(FPGetSrvrParms);
EXT_FN(FPGetForkParms);
EXT_FN(FPGetIcon);
EXT_FN(FPGetIconInfo);
EXT_FN(FPGetUserInfo);
EXT_FN(FPGetVolParms);
EXT_FN(FPGetUserInfo);
EXT_FN(FPMapID);
EXT_FN(FPMapName);
EXT_FN(FPMoveAndRename);
EXT_FN(FPOpenDir);
EXT_FN(FPOpenDT);
EXT_FN(FPOpenFork);
EXT_FN(FPOpenVol);
EXT_FN(FPRead);
EXT_FN(FPReadExt);
EXT_FN(FPRemoveAPPL);
EXT_FN(FPRemoveComment);
EXT_FN(FPRename);
EXT_FN(FPResolveID);
EXT_FN(FPSetDirParms);
EXT_FN(FPSetFileDirParms);
EXT_FN(FPSetFileParms);
EXT_FN(FPSetForkParms);
EXT_FN(FPSetVolParms);
EXT_FN(FPWrite);
EXT_FN(FPWriteExt);
EXT_FN(FPzzz);
EXT_FN(Error);


struct test_fn {
char *name;
void (*fn)(void);

};
#define FN_N(a) { # a , FN(a) },

static struct test_fn Test_list[] =
{
#ifdef QUIRK
FN_N(FPEnumerate)
FN_N(FPExchangeFiles)
FN_N(FPMoveAndRename)
FN_N(Error)
#else
FN_N(FPAddAPPL)
FN_N(FPAddComment)
FN_N(FPAddIcon)
FN_N(FPByteRangeLock)
FN_N(FPByteRangeLockExt)
FN_N(FPCloseDir)
FN_N(FPCloseDT)
FN_N(FPCloseFork)
FN_N(FPCloseVol)
FN_N(FPCreateDir)
FN_N(FPCreateFile)
FN_N(FPCopyFile)
FN_N(FPDelete)
FN_N(FPDisconnectOldSession)
FN_N(FPEnumerate)
FN_N(FPEnumerateExt)
FN_N(FPEnumerateExt2)
FN_N(FPExchangeFiles)
FN_N(FPFlush)
FN_N(FPFlushFork)
FN_N(FPGetAPPL)
FN_N(FPGetComment)
FN_N(FPGetFileDirParms)
FN_N(FPGetForkParms)
FN_N(FPGetIcon)
FN_N(FPGetIconInfo)
FN_N(FPGetSessionToken)
FN_N(FPGetSrvrInfo)
FN_N(FPGetSrvrMsg)
FN_N(FPGetSrvrParms)
FN_N(FPGetUserInfo)
FN_N(FPGetVolParms)
FN_N(FPMapID)
FN_N(FPMapName)
FN_N(FPMoveAndRename)
FN_N(FPOpenDir)
FN_N(FPOpenDT)
FN_N(FPOpenFork)
FN_N(FPOpenVol)
FN_N(FPRead)
FN_N(FPReadExt)
FN_N(FPRemoveAPPL)
FN_N(FPRemoveComment)
FN_N(FPRename)
FN_N(FPResolveID)
FN_N(FPSetDirParms)
FN_N(FPSetFileDirParms)
FN_N(FPSetFileParms)
FN_N(FPSetForkParms)
FN_N(FPSetVolParms)
FN_N(FPWrite)
FN_N(FPWriteExt)
FN_N(FPzzz)
FN_N(Error)
#endif

{NULL, NULL},
};

/* =============================== */
static void list_tests(void)
{
int i = 0;
	while (Test_list[i].name != NULL) {
		fprintf(stdout, "%s\n", Test_list[i].name);
		i++;
	}
}

/* ----------- */
static void run_one(char *name)
{
int i = 0;
void *handle = NULL;
void (*fn)(void) = NULL;
char *error;
char *token;

    token = strtok(name, ",");

	while (Test_list[i].name != NULL) {
		if (!strcmp(Test_list[i].name, name))
			break;
		i++;
	}
	if (Test_list[i].name == NULL) {
		handle = dlopen (NULL, RTLD_LAZY);
        if (handle) {
			fn = dlsym(handle, token);
			if ((error = dlerror()) != NULL)  {
			    fprintf (stdout, "%s\n", error);
			}
        }
        else {
        	fprintf (stdout, "%s\n", dlerror());
        }
        if (!handle || !fn) {
			nottested();
			return;
		}
	}
	else {
		fn = Test_list[i].fn;
	}

	dsi = &Conn->dsi;
	VolID = FPOpenVol(Conn, Vol);
	if (VolID == 0xffff) {
		nottested();
		return;
	}

	while (token ) {
	    (*fn)();
	    token = strtok(NULL, ",");
	    if (token && handle) {
			fn = dlsym(handle, token);
			if ((error = dlerror()) != NULL)  {
			    fprintf (stdout, "%s\n", error);
			}
	    }
	}

	if (handle)
		dlclose(handle);

	FPCloseVol(Conn,VolID);
}

/* ----------- */
static void run_all()
{
int i = 0;

	dsi = &Conn->dsi;
	VolID = FPOpenVol(Conn, Vol);
	if (VolID == 0xffff) {
		nottested();
		return;
	}
	while (Test_list[i].name != NULL) {
		Test_list[i].fn();
		i++;
	}

	FPCloseVol(Conn,VolID);
}

DSI *Dsi, *Dsi2;

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
int     Exclude = 0;

/* =============================== */
void usage( char * av0 )
{
    fprintf( stdout, "usage:\t%s [-m] [-n] [-t] [-h host] [-p port] [-s vol] [-u user] [-w password] -f [call]\n", av0 );
    fprintf( stdout,"\t-m\tserver is a Mac\n");
    fprintf( stdout,"\t-h\tserver host name (default localhost)\n");
    fprintf( stdout,"\t-p\tserver port (default 548)\n");
    fprintf( stdout,"\t-s\tvolume to mount (default home)\n");
    fprintf( stdout,"\t-c\tvolume path on the server\n");
    fprintf( stdout,"\t-u\tuser name (default uid)\n");
    fprintf( stdout,"\t-d\tsecond user for two connections (same password!)\n");
    fprintf( stdout,"\t-H\tsecond server for two connections (default use only one server)\n");

    fprintf( stdout,"\t-w\tpassword (default none)\n");
    fprintf( stdout,"\t-1\tAFP 2.1 version (default)\n");
    fprintf( stdout,"\t-2\tAFP 2.2 version\n");
    fprintf( stdout,"\t-3\tAFP 3.0 version\n");
    fprintf( stdout,"\t-4\tAFP 3.1 version\n");
    fprintf( stdout,"\t-5\tAFP 3.2 version\n");
    fprintf( stdout,"\t-6\tAFP 3.3 version\n");
    fprintf( stdout,"\t-7\tAFP 3.4 version\n");
    fprintf( stdout,"\t-v\tverbose\n");

    fprintf( stdout,"\t-x\tdon't run tests known to kill very old afpd versions\n");
    fprintf( stdout,"\t-f\ttest to run\n");
    fprintf( stdout,"\t-l\tlist tests\n");
    exit (1);
}

char *vers = "AFPVersion 2.1";
char *uam = "Cleartxt Passwrd";
/* ------------------------------- */
int main( int ac, char **av )
{
int cc;
int ret;

    while (( cc = getopt( ac, av, "v1234567h:H:p:s:u:d:w:c:f:lmx" )) != EOF ) {
        switch ( cc ) {
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
		case 'c':
			Path = strdup(optarg);
			break;
		case 'm':
			Mac = 1;
			break;
        case 'n':
            Proto = 1;
            break;
        case 'h':
            Server = strdup(optarg);
            break;
        case 'H':
            Server2 = strdup(optarg);
            break;
        case 's':
            Vol = strdup(optarg);
            break;
        case 'u':
            User = strdup(optarg);
            break;
        case 'd':
            User2 = strdup(optarg);
            break;
        case 'w':
            Password = strdup(optarg);
            break;
        case 'l' :
            List = 1;
            break;
        case 'f' :
            Test = strdup(optarg);
            break;
        case 'x':
        	Exclude = 1;
        	break;
        case 'p' :
            Port = atoi( optarg );
            if (Port <= 0) {
                fprintf(stdout, "Bad port.\n");
                exit(1);
            }
            break;
		case 'v':
			Verbose = 1;
			break;
        default :
            usage( av[ 0 ] );
        }
    }
	if (List) {
		list_tests();
		exit (2);
	}
	/************************************
	 *                                  *
	 * Connection user 1                *
	 *                                  *
	 ************************************/

    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
    	return 1;
    }
    Conn->type = Proto;
    if (!Proto) {
	int sock;
    	Dsi = &Conn->dsi;
		dsi = Dsi;
	    sock = OpenClientSocket(Server, Port);
        if ( sock < 0) {
	    	return 2;
        }
     	Dsi->protocol = DSI_TCPIP;
	    Dsi->socket = sock;
    }
    else {
	}

    /* login */
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

	/***************************************
	 *                                     *
	 * User 2                              *
	 *                                     *
	 ***************************************/
	/* user 2 */
	if (User2) {
    	if ((Conn2 = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
    		return 1;
    	}
	    Conn2->type = Proto;
    	if (!Proto) {
		int sock;
    		Dsi2 = &Conn2->dsi;

	    	sock = OpenClientSocket(Server2?Server2:Server, Port);
	        if ( sock < 0) {
		    	return 1;
        	}
	     	Dsi2->protocol = DSI_TCPIP;
		    Dsi2->socket = sock;
	    }
    	else {
		}
    	/* login */
	// FIXME: workaround for FPopenLoginExt() being broken
#if 0
    	if (Version >= 30) {
			ret = FPopenLoginExt(Conn2, vers, uam, User2, Password);
		}
    	else {
			ret = FPopenLogin(Conn2, vers, uam, User2, Password);
		}
#else
	ret = FPopenLogin(Conn2, vers, uam, User2, Password);
#endif
	if (ret) {
		printf("Login failed\n");
		exit(1);
	}
		Conn2->afp_version = Version;
	}
	/*********************************
	*/
	if (Test != NULL) {
		run_one(Test);
	}
	else {
		run_all();
	}

   	FPLogOut(Conn);

	if (User2) {
		FPLogOut(Conn2);
	}
	return ExitCode;
}
