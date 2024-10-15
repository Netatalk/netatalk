/* ----------------------------------------------
*/
#include "specs.h"

extern char *Server;
extern int  Port;
extern char *Password;
extern char *vers;
extern char *uam; 

static volatile int sigp = 0;

static void pipe_handler()
{
	sigp = 1;
}

/* ------------------------- */
STATIC void test223()
{
char *name = "t223 file";
uint16_t vol = VolID;
unsigned int ret;
struct sigaction action;    
DSI *dsi;
int sock;
uint32_t time= 12345;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPzzz:test223: AFP 3.x enter sleep mode\n");
	if (Conn->afp_version < 30 || Conn2) {
		test_skipped(T_AFP3_CONN2);
		goto test_exit;
	}

    action.sa_handler = pipe_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    if ((sigaction(SIGPIPE, &action, NULL) < 0)) {
		nottested();
		goto test_exit;
    }

    /* Get session token */
    FAIL( FPGetSessionToken(Conn, 3, time, strlen("test223"), "test223"))

	FAIL (FPzzz(Conn, 0)) 

	fprintf(stdout,"sleep more than 2 mn\n");
	sleep(60 * 3);

	ret = FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name);
	if (sigp || ret == (unsigned)-1) {
		fprintf(stdout,"\tFAILED disconnected %d\n", sigp);
		failed_nomsg();
        /* try to reconnect */
        Conn->dsi.socket = OpenClientSocket(Server, Port);
        if (Conn->dsi.socket < 0) {
            nottested();
            goto fin;
        }
        Conn->dsi.protocol = DSI_TCPIP; 
        if (Conn->afp_version < 30)
            ret = FPopenLogin(Conn, vers, uam, User, Password);
        else
            ret = FPopenLoginExt(Conn, vers, uam, User, Password);

        if (ret) {
            nottested();
            goto fin;
        }

        /* Get session token, killing above session which is possibly in disconnected state */
        FAIL( FPGetSessionToken(Conn, 3, time, strlen("test223"), "test223"))

        vol = VolID  = FPOpenVol(Conn, Vol);
        if (vol == 0xffff) {
            nottested();
            goto fin;
        }
        FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) 

	} else if (ret) {
		failed();
	}

	/* always there ? */
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))
fin:
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    if ((sigaction(SIGPIPE, &action, NULL) < 0)) {
		nottested();
    }
test_exit:
	exit_test("test223");
}

/* ------------------------- */
STATIC void test224()
{
char *name = "t224 file";
uint16_t vol = VolID;
unsigned int ret;
struct sigaction action;    
int sock;
uint32_t time= 12345;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPzzz:test224: disconnected after 2 mn\n");

	if (Conn->afp_version < 30 || Conn2) {
		test_skipped(T_AFP3_CONN2);
		goto test_exit;
	}

	sigp = 0;
    action.sa_handler = pipe_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    if ((sigaction(SIGPIPE, &action, NULL) < 0)) {
		nottested();
		goto test_exit;
    }

    /* Get session token */
    FAIL( FPGetSessionToken(Conn, 3, time, strlen("test224"), "test224"))

	fprintf(stdout,"sleep more than 2 mn\n");
	sleep(60 *3);

	ret = FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name);
	if (!sigp && ret != (unsigned)-1) {
		fprintf(stdout,"\tFAILED not disconnected \n");
		failed_nomsg();
	} else {
        /* try to reconnect */
        Conn->dsi.socket = OpenClientSocket(Server, Port);
        if (Conn->dsi.socket < 0) {
            nottested();
            goto fin;
        }
        Conn->dsi.protocol = DSI_TCPIP; 
        if (Conn->afp_version < 30)
            ret = FPopenLogin(Conn, vers, uam, User, Password);
        else
            ret = FPopenLoginExt(Conn, vers, uam, User, Password);

        if (ret) {
            nottested();
            goto fin;
        }

        /* Get session token, killing above session which is in disconnected state */
        FAIL( FPGetSessionToken(Conn, 3, time, strlen("test224"), "test224"))

        vol = VolID  = FPOpenVol(Conn, Vol);
        if (vol == 0xffff) {
            nottested();
            goto fin;
        }
        FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name)) 
    }

	/* always there ? */
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))

fin:
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    if ((sigaction(SIGPIPE, &action, NULL) < 0)) {
		nottested();
    }
test_exit:
	exit_test("test224");
	
}

/* ------------------------- */
STATIC void test239()
{
char *name = "t239 file";
uint16_t vol = VolID;
unsigned int ret;
struct sigaction action;    
DSI *dsi;
int sock;

	enter_test();
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPzzz:test239: AFP 3.x enter extended sleep\n");

	if (Conn->afp_version < 30 || Conn2) {
		test_skipped(T_AFP3_CONN2);
		goto test_exit;
	}

    action.sa_handler = pipe_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    if ((sigaction(SIGPIPE, &action, NULL) < 0)) {
		nottested();
		goto test_exit;
    }
	FAIL (FPzzz(Conn, 1))
	fprintf(stdout,"sleep more than 2 mn\n");
	sleep(60 *3);
	FAIL (FPzzz(Conn, 2))
    FAIL (FPCreateFile(Conn, vol,  0, DIRDID_ROOT , name))
	FAIL (FPDelete(Conn, vol,  DIRDID_ROOT, name))

fin:
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    if ((sigaction(SIGPIPE, &action, NULL) < 0)) {
		nottested();
    }
test_exit:
	exit_test("test239");
}

/* ----------- */
void FPzzz_test()
{
    fprintf(stdout,"===================\n");
    fprintf(stdout,"FPzzz\n");
    test223();
    test224();
    test239();
}

