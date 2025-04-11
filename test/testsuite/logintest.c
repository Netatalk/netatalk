#include "specs.h"

#include <getopt.h>

uint16_t VolID;

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

DSI *Dsi;

char Data[300000] = "";
/* ------------------------------- */
char    *Server = "localhost";
char    *Server2;
int     Proto = 0;
int     Port = DSI_AFPOVERTCP_PORT;
char    *Password = "";
char    *Vol = "";
char    *User;
char    *User2;
char    *Path;
int     Version = 34;
int     List = 0;
int     Mac = 0;
char    *Test;
static char  *vers = "AFP3.4";

STATIC void connect_server(CONN *conn)
{
DSI *dsi;

    conn->type = Proto;
    if (!Proto) {
	int sock;
    	dsi = &conn->dsi;
	    sock = OpenClientSocket(Server, Port);
        if ( sock < 0) {
        	test_nottested();
	    	exit(ExitCode);
        }
	    dsi->socket = sock;
    }
}

/* ------------------------- */
STATIC void test1(void)
{
	ENTER_TEST

    connect_server(Conn);
    Dsi = &Conn->dsi;

	if (!Quiet) {
		fprintf(stdout, "DSIGetStatus\n");
	}
	if (DSIGetStatus(Conn)) {
		test_failed();
	}
	CloseClientSocket(Dsi->socket);

	exit_test("Logintest:test1: DSI with no open session");
}

/* ------------------------- */
STATIC void test2(void)
{
	ENTER_TEST

    connect_server(Conn);
    Dsi = &Conn->dsi;

	if (!Quiet) {
		fprintf(stdout,"DSIOpenSession\n");
	}
	if (DSIOpenSession(Conn)) {
		test_failed();
		goto test_exit;
	}
	if (Mac) {
		if (!Quiet) {
			fprintf(stdout,"DSIGetStatus\n");
		}
		if (DSIGetStatus(Conn)) {
			test_failed();
			goto test_exit;
		}
	}
	if (!Quiet) {
		fprintf(stdout,"DSICloseSession\n");
	}
	if (DSICloseSession(Conn)) {
		test_failed();
		goto test_exit;
	}
	CloseClientSocket(Dsi->socket);

test_exit:
	exit_test("Logintest:test2: DSI with open session");
}

/* ------------------------- */
STATIC void test3(void)
{
static char *uam = "No User Authent";
int ret;

	ENTER_TEST

    connect_server(Conn);
    Dsi = &Conn->dsi;

    if (Version >= 30) {
      	ret = FPopenLoginExt(Conn, vers, uam, "", "");
    }
    else {
      	ret = FPopenLogin(Conn, vers, uam, "", "");
	}
	if (ret) {
		test_failed();
		goto test_exit;
	}
    if (Mac) {
		fprintf(stdout,"DSIGetStatus\n");
		if (DSIGetStatus(Conn)) {
			test_failed();
			goto test_exit;
		}
	}
	if (FPLogOut(Conn)) {
		test_failed();
		goto test_exit;
    }

test_exit:
	CloseClientSocket(Dsi->socket);
	exit_test("Logintest:test3: Guest login");
}

/* ------------------------- */
// FIXME: when max connections is exceeded the server still returns
// code DSIERR_OK and not DSIERR_TOOMANY (Netatalk 4.0.3, 3.1.12)
STATIC void test4(void)
{
CONN conn[50];
int  i;
int  cnt = 0;
int  ret;

	ENTER_TEST

    for (i = 0; i < 50; i++) {
    	connect_server(&conn[i]);
        Dsi = &conn[i].dsi;

		if (!Quiet) {
	     	fprintf(stdout,"DSIOpenSession number %d\n", i);
		}
	    if ((ret = DSIOpenSession(&conn[i]))) {
	    	if (ret != DSIERR_TOOMANY) {
				test_failed();
				goto test_exit;
	    	}
	    	cnt = i;
	    	break;
	    }
		if (!Quiet) {
	     	fprintf(stdout,"DSIOpenSession return code %04x\n", ret);
		}
	}
	if (!cnt) {
		if (!Quiet) {
			fprintf(stdout,"All connections succeeded\n");
		}
		test_nottested();
		goto test_exit;
	}
	for (i = 0; i < cnt; i++) {
        Dsi = &conn[i].dsi;
		if (DSICloseSession(&conn[i])) {
			test_failed();
			goto test_exit;
		}
	}

test_exit:
	CloseClientSocket(Dsi->socket);
	exit_test("Logintest:test4: too many connections");
}

/* ------------------------- */
STATIC void test5(void)
{
static char *uam = "Cleartxt Passwrd";
int ret;

	ENTER_TEST

    connect_server(Conn);
    Dsi = &Conn->dsi;
    if (Version >= 30) {
		ret = FPopenLoginExt(Conn, vers, uam, User, Password);
	}
	else {
		ret = FPopenLogin(Conn, vers, uam, User, Password);
	}
	if (ret) {
		test_failed();
		goto test_exit;
	}
	Conn->afp_version = Version;

   	if (FPLogOut(Conn)) {
   		test_failed();
   	}

test_exit:
	CloseClientSocket(Dsi->socket);
	exit_test("Logintest:test5: Clear text login");
}

/* ------------------------- */
STATIC void test6(void)
{
DSI *dsi;
uint32_t i = 0;

    connect_server(Conn);
    Dsi = &Conn->dsi;

	dsi = Dsi;
	/* DSIOpenSession */
	memset(&dsi->header, 0, sizeof(dsi->header));
	dsi->header.dsi_flags = DSIFL_REQUEST;
	dsi->header.dsi_command = DSIFUNC_OPEN;
	dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
	dsi->header.dsi_code = 6;


	dsi->cmdlen = 2 + sizeof(i);
	dsi->commands[0] = DSIOPT_ATTNQUANT;
  	dsi->commands[1] = sizeof(i);
  	i = htonl(DSI_DEFQUANT);
  	memcpy(dsi->commands + 2, &i, sizeof(i));
	my_dsi_send(dsi);
	my_dsi_cmd_receive(dsi);

	if (dsi->header.dsi_code) {
		test_failed();
		goto test_exit;
	}

	if (!Quiet) {
		fprintf(stdout,"DSICloseSession\n");
	}
	if (DSICloseSession(Conn)) {
		test_failed();
		goto test_exit;
	}

test_exit:
	CloseClientSocket(Dsi->socket);
	exit_test("Logintest:test6: DSIOpenSession non zero parameter should be ignored by the server");
}

/* =============================== */
void usage( char * av0 )
{
    fprintf( stdout, "usage:\t%s [-1234567CmVv] [-h host] [-p port] [-s vol] [-u user] [-w password]\n", av0 );
    fprintf( stdout,"\t-m\tserver is a Mac\n");
    fprintf( stdout,"\t-h\tserver host name (default localhost)\n");
    fprintf( stdout,"\t-p\tserver port (default 548)\n");
    fprintf( stdout,"\t-u\tuser name (default uid)\n");
    fprintf( stdout,"\t-w\tpassword\n");
    fprintf( stdout,"\t-1\tAFP 2.1 version\n");
    fprintf( stdout,"\t-2\tAFP 2.2 version\n");
    fprintf( stdout,"\t-3\tAFP 3.0 version\n");
    fprintf( stdout,"\t-4\tAFP 3.1 version\n");
    fprintf( stdout,"\t-5\tAFP 3.2 version\n");
    fprintf( stdout,"\t-6\tAFP 3.3 version\n");
    fprintf( stdout,"\t-7\tAFP 3.4 version (default)\n");
    fprintf( stdout,"\t-v\tverbose\n");
    fprintf( stdout,"\t-V\tvery verbose\n");
    fprintf( stdout,"\t-C\tturn off terminal color output\n");

    exit (1);
}

/* ------------------------------- */
int main( int ac, char **av )
{
int cc;

    if (ac == 1) {
        usage(av[0]);
    }

    while (( cc = getopt( ac, av, "1234567CmVvh:p:u:w:" )) != EOF ) {
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
		case 'C':
			Color = 0;
			break;
        case 'h':
            Server = strdup(optarg);
            break;
		case 'm':
			Mac = 1;
			break;
        case 'p' :
            Port = atoi( optarg );
            if (Port <= 0) {
                fprintf(stdout, "Bad port.\n");
                exit(1);
            }
            break;
        case 'u':
            User = strdup(optarg);
            break;
		case 'v':
			Quiet = 0;
			break;
		case 'V':
			Quiet = 0;
			Verbose = 1;
			break;
        case 'w':
            Password = strdup(optarg);
            break;
        default :
            usage( av[ 0 ] );
        }
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

    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
    	return 1;
    }

	test1();
	test2();
    test3();
#if 0
	test4();
#endif
	test5();
	test6();

    fprintf(stdout,"===================\n");
    fprintf(stdout,"TEST RESULT SUMMARY\n");
    fprintf(stdout,"-------------------\n");
	fprintf(stdout, "  Passed:     %d\n", PassCount);
	fprintf(stdout, "  Failed:     %d\n", FailCount);
	fprintf(stdout, "  Skipped:    %d\n", SkipCount);
	fprintf(stdout, "  Not tested: %d\n", NotTestedCount);

	return ExitCode;
}
