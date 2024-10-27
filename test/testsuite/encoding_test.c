#include "afpclient.h"
#include "test.h"
#include "specs.h"
#include <time.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char    *Path;

uint16_t VolID;
static DSI *dsi;
CONN *Conn;
CONN *Conn2;

int ExitCode = 0;
char *Encoding = "western";
extern int Convert;
static char temp[MAXPATHLEN];

/* ------------------------- */
int empty_volume()
{
uint16_t vol = VolID;
uint16_t d_bitmap;
uint16_t f_bitmap;
unsigned int ret;
int  dir;
uint16_t  tp;
int  i, j;
unsigned char *b;
struct afp_filedir_parms filedir;
int *stack;
int cnt = 0;
int size = 1000;
DSI *dsi;

	dsi = &Conn->dsi;

    if (!Quiet) {
		fprintf(stdout,"Delete all files\n");
	}

    f_bitmap = (1<<FILPBIT_FNUM ) | (1<<FILPBIT_ATTR) | (1<<FILPBIT_FINFO)|
	         (1<<FILPBIT_CDATE) | (1<<FILPBIT_BDATE) | (1<<FILPBIT_MDATE);

	d_bitmap = (1<< DIRPBIT_ATTR) | (1<<DIRPBIT_FINFO) |  (1 << DIRPBIT_OFFCNT) |
	         (1<<DIRPBIT_CDATE) | (1<<DIRPBIT_BDATE) | (1<<DIRPBIT_MDATE) |
		    (1<< DIRPBIT_LNAME) | (1<< DIRPBIT_PDID) | (1<< DIRPBIT_DID)|(1<< DIRPBIT_ACCESS);

	if (Conn->afp_version >= 30) {
		f_bitmap |= (1<<FILPBIT_PDINFO);
		d_bitmap |= (1<<FILPBIT_PDINFO);
	}
	else {
		f_bitmap |= (1<<FILPBIT_LNAME);
		d_bitmap |= (1<<FILPBIT_LNAME);
	}
	dir = get_did(Conn, vol, DIRDID_ROOT, "");
	if (!dir) {
		nottested();
	    return -1;
	}
	if (!(stack = calloc(size, sizeof(int)) )) {
		nottested();
	    return -1;
	}
	stack[cnt] = dir;
	cnt++;

	while (cnt) {
	    cnt--;
	    dir = stack[cnt];
		i = 1;
		if (FPGetFileDirParams(Conn, vol,  dir , "", 0, d_bitmap)) {
			nottested();
			return -1;
		}
		while (!(ret = FPEnumerateFull(Conn, vol, i, 150, 8000,  dir , "", f_bitmap, d_bitmap))) {
			memcpy(&tp, dsi->data +4, sizeof(tp));
			tp = ntohs(tp);
		    i += tp;
		    b = dsi->data +6;
			for (j = 1; j <= tp; j++, b += b[0]) {
			    if (b[1]) {
	    		   filedir.isdir = 1;
	    		}
	    		else {
	    		   filedir.isdir = 0;
	    		}
	    		afp_filedir_unpack(&filedir, b + 2, f_bitmap, d_bitmap);
	    		if (FPDelete(Conn, vol,  DIRDID_ROOT , (Conn->afp_version >= 30)?filedir.utf8_name:filedir.lname)) {
	    		    nottested();
	    		    return -1;
	    		}
	    	}
	    }
	    if (ret != ntohl(AFPERR_NOOBJ)) {
			nottested();
			return -1;
	    }
	}
	return 0;
}

/* ------------------
 * client encoding western 1 byte [1..255]
*/
static void test_western()
{
uint16_t vol = VolID;
uint16_t f_bitmap;
int  ofs =  3 * sizeof( uint16_t );
struct afp_filedir_parms filedir;
char name[30];
char *result;
int  i, j = 0;
DSI *dsi;

	dsi = &Conn->dsi;

	ENTER_TEST

    for (i = 1; i < 8; i++) {
    	name[i -1] = i;
    }
    name[i -1] = 0;
	if (Conn->afp_version >= 30) {
		f_bitmap = (1<<FILPBIT_PDINFO);
	}
	else {
		f_bitmap = (1<<FILPBIT_LNAME);
	}
    while (j < 32) {
        j++;
		if (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) {
			nottested();
			goto test_exit;
		}
		if (FPGetFileDirParams(Conn, vol, DIRDID_ROOT, name, f_bitmap, 0)) {
			nottested();
			goto test_exit;
		}
		filedir.isdir = 0;
		afp_filedir_unpack(&filedir, dsi->data +ofs, f_bitmap, 0);
		result = (Conn->afp_version >= 30)?filedir.utf8_name:filedir.lname;
		if (strcmp(result, name)) {
			failed();
			goto test_exit;
		}

		/* don't use ':' char */
    	for (i = 0; i < 8; i++) {
    		name[i] = (j*8 +i != 0x3a)?j*8 +i:0x39;
	    }
    	name[i] = 0;
	}
	if (Path) {
	int fd;

		sprintf(temp,"%s/:test", Path);
		fd = open(temp, O_RDWR | O_CREAT, 0666);
		if (fd < 0) {
			if (!Quiet) {
				fprintf(stdout,"\tFAILED unable to create %s :%s\n", temp, strerror(errno));
			}
			failed_nomsg();
			goto test_exit;
		}
		else {
			close(fd);
		}
	}
test_exit:
	exit_test("Encoding:western");
}

/* ------------------ */
static void run_one()
{
	Convert = 0;
	dsi = &Conn->dsi;
	VolID = FPOpenVol(Conn, Vol);
	if (VolID == 0xffff) {
		nottested();
		return;
	}
	if (empty_volume() < 0) {
		nottested();
	    return;
	}
	if (strcmp(Encoding, "western") == 0) {
		if (!Quiet) {
			fprintf(stdout, "Testing encoding: %s\n", Encoding);
		}
	    test_western();
	}
	else {
		fprintf(stdout, "Unknown encoding: %s\n", Encoding);
		ExitCode = 1;
	}
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
    fprintf( stdout, "usage:\t%s [-1234567CdmVv] [-e encoding] [-h host] [-p port] [-s vol] [-c vol path] [-u user] [-w password]\n", av0 );
    fprintf( stdout,"\t-m\tserver is a Mac\n");
    fprintf( stdout,"\t-e\tclient encoding (default western)\n");
    fprintf( stdout,"\t-h\tserver host name (default localhost)\n");
    fprintf( stdout,"\t-p\tserver port (default 548)\n");
    fprintf( stdout,"\t-s\tvolume to mount (default home)\n");
    fprintf( stdout,"\t-c\tvolume path on the server\n");
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
    fprintf( stdout,"\t-C\tturn on terminal color output\n");

    exit (1);
}

/* ------------------------------- */
int main( int ac, char **av )
{
int cc;
static char *vers = "AFPVersion 2.1";
static char *uam = "Cleartxt Passwrd";

    while (( cc = getopt( ac, av, "1234567ClmRvVc:e:h:p:s:u:w:" )) != EOF ) {
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
		case 'c':
			Path = strdup(optarg);
			break;
		case 'e':
			Encoding = strdup(optarg);
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
