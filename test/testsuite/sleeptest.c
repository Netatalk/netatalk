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

EXT_FN(FPzzz);

struct test_fn {
char *name;
void (*fn)(void);

};
#define FN_N(a) { # a , FN(a) },

static struct test_fn Test_list[] =
{
FN_N(FPzzz)
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
			nottested();
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
		nottested();
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
		nottested();
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
int     Port = 548;
char    *Password = "";
char    *Vol = "";
char    *Vol2 = "";
char    *User;
char    *User2;
char    *Path;
int     Version = 21;
int     List = 0;
int     Mac = 0;
char    *Test;
int		Locking;
enum adouble adouble = AD_EA;

/* =============================== */
void usage( char * av0 )
{
    fprintf( stdout, "usage:\t%s [-aLmn] [-h host] [-p port] [-s vol] [-u user] [-w password] -f [call]\n", av0 );
    fprintf( stdout,"\t-a\tvolume is appledouble = v2 instead of default appledouble = ea\n");
    fprintf( stdout,"\t-L\tserver without working fcntl locking, skip tests using it\n");
    fprintf( stdout,"\t-m\tserver is a Mac\n");
    fprintf( stdout,"\t-h\tserver host name (default localhost)\n");
    fprintf( stdout,"\t-p\tserver port (default 548)\n");
    fprintf( stdout,"\t-s\tvolume to mount (default home)\n");
    fprintf( stdout,"\t-S\tsecond volume to mount (default none)\n");
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
    fprintf( stdout,"\t-V\tvery verbose\n");

    fprintf( stdout,"\t-x\tdon't run tests with known bugs\n");
    fprintf( stdout,"\t-f\ttest to run\n");
    fprintf( stdout,"\t-l\tlist tests\n");
    fprintf( stdout,"\t-i\tinteractive mode, prompts before every test (debug purposes)\n");
    exit (1);
}

char *vers = "AFPVersion 2.1";
char *uam = "Cleartxt Passwrd";
/* ------------------------------- */
int main( int ac, char **av )
{
int cc;
int ret;

    while (( cc = getopt( ac, av, "vV1234567ah:H:p:s:S:u:d:w:c:f:Llmxi" )) != EOF ) {
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
		case 'c':
			Path = strdup(optarg);
			break;
		case 'L':
			Locking = 1;
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
        case 'S':
            Vol2 = strdup(optarg);
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
		Quiet = 0;
		break;
	case 'V':
		Quiet = 0;
		Verbose = 1;
		break;
	case 'i':
		Interactive = 1;
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
