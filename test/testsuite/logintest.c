#include "specs.h"

#include <getopt.h>

uint16_t VolID;

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

static void connect_server(CONN *conn)
{
DSI *dsi;

    conn->type = Proto;
    if (!Proto) {
	int sock;
    	dsi = &conn->dsi;
	    sock = OpenClientSocket(Server, Port);
        if ( sock < 0) {
        	nottested();
	    	exit(ExitCode);
        }
     	dsi->protocol = DSI_TCPIP;
	    dsi->socket = sock;
    }
}

/* ------------------------- */
static void test3(void)
{
static char *uam = "No User Authent";
int ret;

	ENTER_TEST

    if (Version >= 30) {
      	ret = FPopenLoginExt(Conn, vers, uam, "", "");
    }
    else {
      	ret = FPopenLogin(Conn, vers, uam, "", "");
	}
	if (ret) {
		failed();
		goto test_exit;
	}
    if (Mac) {
		fprintf(stdout,"DSIGetStatus\n");
		if (DSIGetStatus(Conn)) {
			failed();
			goto test_exit;
		}
	}
	if (FPLogOut(Conn)) {
		failed();
		goto test_exit;
    }

test_exit:
	exit_test("Logintest:test3: Guest login");
}

/* ------------------------- */
static void test4(void)
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
		// FIXME: Broken assumption here. All 50 connections succeed.
	    if ((ret = DSIOpenSession(&conn[i]))) {
	    	if (ret != DSIERR_TOOMANY) {
				failed();
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
		nottested();
		goto test_exit;
	}
	for (i = 0; i < cnt; i++) {
        Dsi = &conn[i].dsi;
		if (DSICloseSession(&conn[i])) {
			failed();
			goto test_exit;
		}
		CloseClientSocket(Dsi->socket);
	}

test_exit:
	exit_test("Logintest:test4: too many connections");
}

/* =============================== */
void usage( char * av0 )
{
    fprintf( stdout, "usage:\t%s [-1234567mVv] [-h host] [-p port] [-s vol] [-u user] [-w password]\n", av0 );
    fprintf( stdout,"\t-m\tserver is a Mac\n");
    fprintf( stdout,"\t-h\tserver host name (default localhost)\n");
    fprintf( stdout,"\t-p\tserver port (default 548)\n");
    fprintf( stdout,"\t-u\tuser name (default uid)\n");

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
static char *uam = "Cleartxt Passwrd";
unsigned int ret;

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
			Color = 1;
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
	/************************************
	 *                                  *
	 * Connection user 1                *
	 *                                  *
	 ************************************/

    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
    	return 1;
    }
    connect_server(Conn);
	/* dsi with no open session */
    Dsi = &Conn->dsi;

	if (!Quiet) {
		fprintf(stdout, "DSIGetStatus\n");
	}
	if (DSIGetStatus(Conn)) {
		failed();
		return ExitCode;
	}
	CloseClientSocket(Dsi->socket);

	/* ------------------------ */
    connect_server(Conn);
    Dsi = &Conn->dsi;

	if (!Quiet) {
		fprintf(stdout,"DSIOpenSession\n");
	}
	if (DSIOpenSession(Conn)) {
		failed();
		return ExitCode;
	}
	if (Mac) {
		if (!Quiet) {
			fprintf(stdout,"DSIGetStatus\n");
		}
		if (DSIGetStatus(Conn)) {
			failed();
			return ExitCode;
		}
	}
	if (!Quiet) {
		fprintf(stdout,"DSICloseSession\n");
	}
	if (DSICloseSession(Conn)) {
		failed();
		return ExitCode;
	}
	CloseClientSocket(Dsi->socket);
	/* -------------------------- */

	if (!Quiet) {
		fprintf(stdout,"DSIOpenSession non zero parameter should be ignored by the server\n");
	}

    connect_server(Conn);
    Dsi = &Conn->dsi;
{
DSI *dsi;
uint32_t i = 0;

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
		failed();
		return ExitCode;
	}
}

	if (!Quiet) {
		fprintf(stdout,"DSICloseSession\n");
	}
	if (DSICloseSession(Conn)) {
		failed();
		return ExitCode;
	}
	CloseClientSocket(Dsi->socket);

	/* ------------------------ */
    /* guest login */
    connect_server(Conn);
    Dsi = &Conn->dsi;
    test3();
	CloseClientSocket(Dsi->socket);

	/* ------------------------
	 * clear text login
	*/
    connect_server(Conn);
    Dsi = &Conn->dsi;
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
		failed();
		return ExitCode;
	}
	Conn->afp_version = Version;

   	if (FPLogOut(Conn)) {
   		failed();
   	}

	/* ------------------------
	 * too many login
	*/
	test4();

	return ExitCode;
}
