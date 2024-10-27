#include "afpclient.h"
#include "test.h"
#include "specs.h"
#include <time.h>
#include <getopt.h>

char    *Dir = "";

uint16_t VolID;
static DSI *dsi;
CONN *Conn;
CONN *Conn2;

int ExitCode = 0;

static uint8_t buffer[DSI_DATASIZ];

/* ------------------------- */
static void test300_enumerate(void)
{
uint16_t vol = VolID;
uint16_t d_bitmap;
uint16_t f_bitmap;
unsigned int ret;
int  dir;
uint16_t  tp;
int  i, j;
DSI *dsi;
unsigned char *b;
struct afp_filedir_parms filedir;
int *stack;
int cnt = 0;
int size = 1000;
	dsi = &Conn->dsi;

	ENTER_TEST

    f_bitmap = (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE)|
	         (1 << FILPBIT_DFLEN) |(1 << FILPBIT_RFLEN);


	d_bitmap = (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |  (1 << DIRPBIT_OFFCNT) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS);

	if (Conn->afp_version >= 30) {
		f_bitmap |= (1<<FILPBIT_PDINFO) | (1<<FILPBIT_LNAME);
		d_bitmap |= (1<<FILPBIT_PDINFO) | (1<<FILPBIT_LNAME);
	}
	else {
		f_bitmap |= (1<<FILPBIT_LNAME);
		d_bitmap |= (1<<FILPBIT_LNAME);
	}

	if (!Quiet) {
		fprintf(stdout,"%lx\n", time(NULL));
	}

	dir = get_did(Conn, vol, DIRDID_ROOT, Dir);
	if (!dir) {
		nottested();
		goto test_exit;
	}
	if (!(stack = calloc(size, sizeof(int)) )) {
		nottested();
		goto test_exit;
	}
	stack[cnt] = dir;
	cnt++;

	while (cnt) {
	    cnt--;
	    dir = stack[cnt];
		i = 1;
		if (FPGetFileDirParams(Conn, vol,  dir , "", 0, d_bitmap)) {
			nottested();
			goto test_exit;
		}
		while (!(ret = FPEnumerateFull(Conn, vol, i, 150, 8000,  dir , "", f_bitmap, d_bitmap))) {
			/* FPResolveID will trash dsi->data */
			memcpy(buffer, dsi->data, sizeof(buffer));
			memcpy(&tp, buffer +4, sizeof(tp));
			tp = ntohs(tp);
		    i += tp;
		    if (Recurse) {
		    	b = buffer +6;
			    for (j = 1; j <= tp; j++, b += b[0]) {
			        if (b[1]) {
	    		    	filedir.isdir = 1;
	        		    afp_filedir_unpack(&filedir, b + 2, 0, d_bitmap);
						if (cnt > size) {
							size += 1000;
							if (!(stack = realloc(stack, size* sizeof(int)))) {
								nottested();
								return;
							}
						}
						stack[cnt] = filedir.did;
						cnt++;
			        }
			        else {
	    		    	filedir.isdir = 0;
	        		    afp_filedir_unpack(&filedir, b + 2, f_bitmap, 0);
			        }
			        if (!Quiet) {
			        	fprintf(stdout, "0x%08x %s%s\n", ntohl(filedir.did),
			        	      (Conn->afp_version >= 30)?filedir.utf8_name:filedir.lname,
			        	      filedir.isdir?"/":"");
					}
					else {
			        	fprintf(stdout, "%s%s\n",
			        	      (Conn->afp_version >= 30)?filedir.utf8_name:filedir.lname,
			        	      filedir.isdir?"/":"");
					}
					if (!filedir.isdir && FPResolveID(Conn, vol, filedir.did, f_bitmap) && Quiet) {
						fprintf(stdout, " Can't resolve ID!");
					}
		    	}
		    }
	    }
	    if (ret != ntohl(AFPERR_NOOBJ)) {
			nottested();
			goto test_exit;
	    }
	}

	if (!Quiet) {
		fprintf(stdout,"%lx\n", time(NULL));
	}
	FPEnumerateFull(Conn, vol, 1, 150, 8000,  DIRDID_ROOT , "", f_bitmap, d_bitmap);

test_exit:
	exit_test("FPEnumerate:test300: enumerate recursively a folder");
}


/* ------------------------- */
void test300()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPEnumerate page 150\n");
    fprintf(stdout,"-------------------\n");
    test300_enumerate();
    if (Twice)
        test300_enumerate();
}

/* ------------------ */
static void run_one()
{
	dsi = &Conn->dsi;
	VolID = FPOpenVol(Conn, Vol);
	if (VolID == 0xffff) {
		nottested();
		return;
	}
	test300();
}

/* =============================== */

DSI *Dsi;

char Data[300000] = "";
/* ------------------------------- */
char    *Server = "localhost";
int     Proto = 0;
int     Port = 548;
char    *Password = "";
char    *Vol = "";
char    *User;
int     Version = 21;
int     Mac = 0;

/* =============================== */
void usage( char * av0 )
{
    fprintf( stdout, "usage:\t%s [-CmnRtVv] [-h host] [-p port] [-s vol] [-u user] [-w password]\n", av0 );
    fprintf( stdout,"\t-m\tserver is a Mac\n");
    fprintf( stdout,"\t-h\tserver host name (default localhost)\n");
    fprintf( stdout,"\t-p\tserver port (default 548)\n");
    fprintf( stdout,"\t-s\tvolume to mount (default home)\n");
    fprintf( stdout,"\t-d\tdirectory to enumerate\n");
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
    fprintf( stdout,"\t-R\trun it recursively\n");
    fprintf( stdout,"\t-t\trun it twice (ie one with the cache warm)\n");
    fprintf( stdout,"\t-C\tturn on terminal color output\n");

    exit (1);
}

/* ------------------------------- */
int main( int ac, char **av )
{
int cc;
//	static char *vers = "AFP2.2";
static char *vers = "AFPVersion 2.1";
static char *uam = "Cleartxt Passwrd";

    while (( cc = getopt( ac, av, "1234567ClmnRtVvd:h:p:s:u:w:" )) != EOF ) {
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
        case 'd':
            Dir = strdup(optarg);
            break;
        case 'h':
            Server = strdup(optarg);
            break;
		case 'm':
			Mac = 1;
			break;
        case 'n':
            Proto = 1;
            break;
        case 'p' :
            Port = atoi( optarg );
            if (Port <= 0) {
                fprintf(stdout, "Bad port.\n");
                exit(1);
            }
            break;
		case 'R':
			Recurse = 1;
			break;
        case 's':
            Vol = strdup(optarg);
            break;
		case 't':
			Twice = 1;
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
	FPopenLogin(Conn, vers, uam, User, Password);
	Conn->afp_version = Version;


	run_one();

   	FPLogOut(Conn);
	return ExitCode;
}
