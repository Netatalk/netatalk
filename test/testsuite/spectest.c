#include "specs.h"
#include "afpclient.h"
#include "test.h"

#include <dlfcn.h>
#include <getopt.h>

uint16_t VolID;
static DSI *dsi;
CONN *Conn;
CONN *Conn2;

int ExitCode = 0;
int PassCount = 0;
int FailCount = 0;
int SkipCount = 0;
int NotTestedCount = 0;

char FailedTests[1024][256] = {{0}};
char NotTestedTests[1024][256] = {{0}};
char SkippedTests[1024][256] = {{0}};

#define FN(a) a ## _test
#define EXT_FN(a) extern void FN(a) (void)

EXT_FN(FPAddAPPL);
EXT_FN(FPAddComment);
EXT_FN(FPAddIcon);
EXT_FN(FPByteRangeLock);
EXT_FN(FPByteRangeLockExt);
EXT_FN(FPCatSearch);
EXT_FN(FPCatSearchExt);
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
EXT_FN(FPGetACL);
EXT_FN(FPGetAPPL);
EXT_FN(FPGetComment);
EXT_FN(FPGetExtAttr);
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
EXT_FN(FPSync);
EXT_FN(FPWrite);
EXT_FN(FPWriteExt);
EXT_FN(FPzzz);

EXT_FN(T2FPByteRangeLock);
EXT_FN(T2FPCopyFile);
EXT_FN(T2FPCreateFile);
EXT_FN(T2FPDelete);
EXT_FN(T2FPGetFileDirParms);
EXT_FN(T2FPGetSrvrParms);
EXT_FN(T2FPMoveAndRename);
EXT_FN(T2FPOpenFork);
EXT_FN(T2FPSetDirParms);
EXT_FN(T2FPSetFileParms);
EXT_FN(T2FPResolveID);
EXT_FN(T2FPRead);
EXT_FN(T2FPSetForkParms);

EXT_FN(Dircache_attack);
EXT_FN(Encoding);
EXT_FN(Error);
EXT_FN(Readonly);
EXT_FN(Utf8);

struct test_fn {
char *name;
void (*fn)(void);

};
#define FN_N(a) { # a , FN(a) },

static struct test_fn Test_list[] =
{
FN_N(FPAddAPPL)
FN_N(FPAddComment)
FN_N(FPAddIcon)
FN_N(FPByteRangeLock)
FN_N(FPByteRangeLockExt)
FN_N(FPCatSearch)
FN_N(FPCatSearchExt)
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
FN_N(FPGetACL)
FN_N(FPGetAPPL)
FN_N(FPGetComment)
FN_N(FPGetExtAttr)
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
FN_N(FPSync)
FN_N(FPWrite)
FN_N(FPWriteExt)
FN_N(FPzzz)

FN_N(T2FPByteRangeLock)
FN_N(T2FPCreateFile)
FN_N(T2FPCopyFile)
FN_N(T2FPDelete)
FN_N(T2FPGetFileDirParms)
FN_N(T2FPGetSrvrParms)
FN_N(T2FPMoveAndRename)
FN_N(T2FPOpenFork)
FN_N(T2FPSetDirParms)
FN_N(T2FPSetFileParms)
FN_N(T2FPResolveID)
FN_N(T2FPRead)
FN_N(T2FPSetForkParms)

FN_N(Dircache_attack)
FN_N(Encoding)
FN_N(Error)
FN_N(Readonly)
FN_N(Utf8)

{NULL, NULL},
};


/* =============================== */
static void press_enter(char *s)
{
    if (!Interactive)
	return;

    if (s)
	fprintf(stdout, "--> Performing: %s\n", s);
    fprintf(stdout, "Press <ENTER> to continue.\n");

    while (fgetc(stdin) != '\n')
	;
}

/* =============================== */
static void list_tests(void)
{
	int i = 0;
	fprintf(stdout, "Available testsets. Run individually with the -f option.\n");
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
		handle = dlopen (NULL, RTLD_NOW);
        if (handle) {
			fn = dlsym(handle, token);
			if ((error = dlerror()) != NULL)  {
			    fprintf (stdout, "%s (%p)\n", error, fn);
			}
        }
        else {
        	fprintf (stdout, "%s\n", dlerror());
        }
        if (!handle || !fn) {
			test_nottested();
			return;
		}
	}
	else {
		fn = Test_list[i].fn;
	}

	dsi = &Conn->dsi;
	press_enter("Opening volume.");
	VolID = FPOpenVol(Conn, Vol);
	if (VolID == 0xffff) {
		test_nottested();
		return;
	}

	while (token ) {
	    press_enter(token);
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
	press_enter("Opening volume.");
	VolID = FPOpenVol(Conn, Vol);
	if (VolID == 0xffff) {
		test_nottested();
		return;
	}
	while (Test_list[i].name != NULL) {
		press_enter(Test_list[i].name);
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
int     Port = DSI_AFPOVERTCP_PORT;
char    *Password = "";
char    *Vol = "";
char    *Vol2 = "";
char    *User;
char    *User2;
char    *Path = "";
int     Version = 34;
int     List = 0;
int     Mac = 0;
char    *Test;
int		Locking;
enum adouble adouble = AD_EA;

char *vers = "AFP3.4";
char *uam = "Cleartxt Passwrd";

/* =============================== */
void usage( char * av0 )
{
    fprintf( stdout, "usage:\t%s [-1234567aCiLlmnVvXx] [-h host] [-H host2] [-p port] [-s vol] [-c vol path] [-S vol2] "
	"[-u user] [-d user2] [-w password] [-F testsuite] [-f test]\n", av0 );
    fprintf( stdout,"\t-a\tvolume is using AppleDouble metadata and not EA\n");
    fprintf( stdout,"\t-L\tserver without working fcntl locking, skip tests using it\n");
    fprintf( stdout,"\t-m\tserver is a Mac\n");
    fprintf( stdout,"\t-h\tserver host name (default localhost)\n");
    fprintf( stdout,"\t-p\tserver port (default 548)\n");
    fprintf( stdout,"\t-s\tvolume to mount\n");
    fprintf( stdout,"\t-S\tsecond volume to mount\n");
    fprintf( stdout,"\t-c\tvolume path on the server\n");
    fprintf( stdout,"\t-u\tuser name (default uid)\n");
    fprintf( stdout,"\t-w\tpassword\n");
    fprintf( stdout,"\t-d\tsecond user for two connections (same password!)\n");
    fprintf( stdout,"\t-H\tsecond server for two connections\n");

    fprintf( stdout,"\t-1\tAFP 2.1 version\n");
    fprintf( stdout,"\t-2\tAFP 2.2 version\n");
    fprintf( stdout,"\t-3\tAFP 3.0 version\n");
    fprintf( stdout,"\t-4\tAFP 3.1 version\n");
    fprintf( stdout,"\t-5\tAFP 3.2 version\n");
    fprintf( stdout,"\t-6\tAFP 3.3 version\n");
    fprintf( stdout,"\t-7\tAFP 3.4 version (default)\n");
    fprintf( stdout,"\t-v\tverbose\n");
    fprintf( stdout,"\t-V\tvery verbose\n");
    fprintf( stdout,"\t-x\tdon't run tests that require special setup\n");
    fprintf( stdout,"\t-X\tdon't run tests that fail on big-endian systems\n");
    fprintf( stdout,"\t-f\ttest or testset to run\n");
    fprintf( stdout,"\t-l\tlist testsets\n");
    fprintf( stdout,"\t-i\tinteractive mode, prompts before every test (debug purposes)\n");
    fprintf( stdout,"\t-C\tturn off terminal color output\n");
    exit (1);
}

/* ------------------------------- */
int main( int ac, char **av )
{
int cc;
int ret;

    while (( cc = getopt( ac, av, "1234567aCiLlmVvXxc:d:f:H:h:p:S:s:u:w:" )) != EOF ) {
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
		case 'a':
            adouble = AD_V2;
			break;
		case 'C':
			Color = 0;
			break;
		case 'c':
			Path = strdup(optarg);
			break;
        case 'd':
            User2 = strdup(optarg);
            break;
        case 'f' :
            Test = strdup(optarg);
            break;
        case 'H':
            Server2 = strdup(optarg);
            break;
        case 'h':
            Server = strdup(optarg);
            break;
		case 'i':
			Interactive = 1;
			break;
        case 'l' :
            List = 1;
            break;
		case 'm':
			Mac = 1;
			break;
#if 0
        case 'n':
            Proto = 1;
            break;
#endif
        case 'p' :
            Port = atoi( optarg );
            if (Port <= 0) {
                fprintf(stdout, "Bad port.\n");
                exit(1);
            }
            break;
        case 'S':
            Vol2 = strdup(optarg);
            break;
        case 's':
            Vol = strdup(optarg);
            break;
        case 'u':
            User = strdup(optarg);
            break;
		case 'V':
			Quiet = 0;
			Verbose = 1;
			break;
		case 'v':
			Quiet = 0;
			break;
        case 'w':
            Password = strdup(optarg);
            break;
        case 'x':
        	Exclude = 1;
        	break;
        case 'X':
          Bigendian = 1;
          break;

        default :
            usage( av[ 0 ] );
        }
    }

	Loglevel = AFP_LOG_INFO;

	if (List) {
		list_tests();
		exit (2);
	}

    if (!Quiet) {
        fprintf(stdout, "Connecting to host %s:%d\n", Server, Port);
    }
	if (User != NULL && User[0] == '\0') {
        fprintf(stdout, "Error: Define a user with -u\n");
	}
	if (Password != NULL && Password[0] == '\0') {
        fprintf(stdout, "Error: Define a password with -w\n");
	}
	if (Vol != NULL && Vol[0] == '\0') {
        fprintf(stdout, "Error: Define a volume with -s\n");
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
	    Dsi->socket = sock;
    }
    else {
	}

    /* login */
    if (Version >= 30) {
		ret = FPopenLoginExt(Conn, vers, uam, User, Password);
	}
	else {
		ret = FPopenLogin(Conn, vers, uam, User, Password);
	}
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
		    Dsi2->socket = sock;
	    }
    	else {
		}
    	/* login */
    	if (Version >= 30) {
			ret = FPopenLoginExt(Conn2, vers, uam, User2, Password);
		}
    	else {
			ret = FPopenLogin(Conn2, vers, uam, User2, Password);
		}
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

    fprintf(stdout, "=====================\n");
    fprintf(stdout, " TEST RESULT SUMMARY\n");
    fprintf(stdout, "---------------------\n");
	fprintf(stdout, "  Passed:     %d\n", PassCount);
	fprintf(stdout, "  Skipped:    %d\n", SkipCount);
	fprintf(stdout, "  Failed:     %d\n", FailCount);
	fprintf(stdout, "  Not tested: %d\n", NotTestedCount);

	if (SkipCount) {
		fprintf(stdout, "\n  Skipped tests (precondition not met):\n");
		for (int i = 0; i < SkipCount; i++) {
			fprintf(stdout, "    %s\n", SkippedTests[i]);
		}
	}
	if (FailCount) {
		fprintf(stdout, "\n  Failed tests:\n");
		for (int i = 0; i < FailCount; i++) {
			fprintf(stdout, "    %s\n", FailedTests[i]);
		}
	}
	if (NotTestedCount) {
		fprintf(stdout, "\n  Not tested tests (setup step failed):\n");
		for (int i = 0; i < NotTestedCount; i++) {
			fprintf(stdout, "    %s\n", NotTestedTests[i]);
		}
	}

	return ExitCode;
}
