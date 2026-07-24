/* Raw DSI read/write interleaving tests split from FPRead.c. */
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

extern char *Server;
extern int  Port;
extern char *Password;
extern char *vers;
extern char *uam;

static volatile int sigp = 0;
static int sock = -1;

static void alarm_handler()
{
    sigp = 1;

    if (sock != -1) {
        close(sock);
    }
}

/* Exercise partial DSI replies and interleaved requests on a self-managed
 * legacy session. libafpclient intentionally owns this framing and socket. */
static void write_test(int size)
{
    int fork = 0, fork1 = 0;
    uint16_t bitmap = 0;
    char *name = "t309 FPRead,FPWrite deadlock";
    char *name1 = "t309 second file";
    uint16_t vol = VolID;
    uint16_t vol2;
    int ret;
    DSI *dsi;
    DSI *dsi2;
    int offset;
    int quantum;
    struct sigaction action;
    struct itimerval it;
    CONN *myconn = { 0 };
    dsi = &Conn->dsi;
    sock = -1;
    sigp = 0;
    quantum = min(size, dsi->server_quantum);

    if (quantum < size) {
        if (!Quiet) {
            fprintf(stdout, "\t server quantum (%d) too small\n", quantum);
        }

        test_nottested();
        return;
    }

    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 15;
    it.it_value.tv_usec = 0;
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART | SA_ONESHOT;

    if ((sigaction(SIGALRM, &action, NULL) < 0) ||
            (setitimer(ITIMER_REAL, &it, NULL) < 0)) {
        test_nottested();
        return;
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name)) {
        test_nottested();
    }

    if (FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name1)) {
        test_nottested();
        goto fin;
    }

    if ((myconn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        test_nottested();
        goto fin;
    }

    dsi2 = &myconn->dsi;
    sock = OpenClientSocket(Server, Port);

    if (sock < 0) {
        test_nottested();
        goto fin;
    }

    dsi2->socket = sock;
    ret = FPopenLogin(myconn, vers, uam, User, Password);

    if (ret) {
        test_nottested();
        goto fin;
    }

    vol2 = VolID = FPOpenVol(myconn, Vol);

    if (vol2 == 0xffff) {
        test_nottested();
        goto fin;
    }

    fork = FPOpenFork(myconn, vol2, OPENFORK_DATA, bitmap, DIRDID_ROOT, name,
                      OPENACC_WR | OPENACC_RD);

    if (!fork) {
        test_failed();
        goto fin;
    }

    fork1 = FPOpenFork(myconn, vol2, OPENFORK_DATA, bitmap, DIRDID_ROOT, name1,
                       OPENACC_WR | OPENACC_RD);

    if (!fork1) {
        test_failed();
        goto fin;
    }

    FAIL(FPSetForkParam(myconn, fork, (1 << FILPBIT_DFLEN), 10 * 128 * 1024))
    offset = 0;
    FAIL(FPReadHeader(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadHeader(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadHeader(dsi2, fork, offset, size, Data))
    FAIL(FPReadFooter(dsi2, fork, 0, size, Data))
    offset += size;
    FAIL(FPReadHeader(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadHeader(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadHeader(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadHeader(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadHeader(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadHeader(dsi2, fork, offset, size, Data))
    offset = 0;
    FAIL(FPWriteHeader(dsi2, fork1, offset, size, Data, 0))
    offset += size;
    FAIL(FPWriteHeader(dsi2, fork1, offset, size, Data, 0))
    offset += size;
    FAIL(FPWriteHeader(dsi2, fork1, offset, size, Data, 0))
    offset += size;
    FAIL(FPWriteHeader(dsi2, fork1, offset, size, Data, 0))
    offset = 0;
    offset += size;
    FAIL(FPReadFooter(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadFooter(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadFooter(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadFooter(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadFooter(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadFooter(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadFooter(dsi2, fork, offset, size, Data))
    offset += size;
    FAIL(FPReadFooter(dsi2, fork, offset, size, Data))
    offset = 0;
    FAIL(FPWriteFooter(dsi2, fork1, offset, size, Data, 0))
    offset += size;
    FAIL(FPWriteFooter(dsi2, fork1, offset, size, Data, 0))
    offset += size;
    FAIL(FPWriteFooter(dsi2, fork1, offset, size, Data, 0))
    offset += size;
    FAIL(FPWriteFooter(dsi2, fork1, offset, size, Data, 0))
fin:

    if (sigp) {
        if (!Quiet) {
            fprintf(stdout, "\tFAILED deadlock\n");
        }

        test_failed();
        sleep(5);
    } else if (myconn) {
        if (fork1) {
            FPCloseFork(myconn, fork1);
        }

        if (fork) {
            FPCloseFork(myconn, fork);
        }
    }

    if (myconn) {
        free(myconn);
        myconn = NULL;
    }

    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name1))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
    it.it_interval.tv_usec = 0;
    it.it_value.tv_sec = 0;
    it.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &it, NULL);
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGALRM, &action, NULL) < 0) {
        test_nottested();
    }

    sleep(1);
}

STATIC void test309()
{
    ENTER_TEST
    write_test(1024);
    exit_test("T3FPRead:test309: FPRead, FPWrite deadlock");
}

STATIC void test327()
{
    ENTER_TEST
    write_test(128 * 1024);
    exit_test("T3FPRead:test327: FPRead, FPWrite deadlock");
}

void T3FPRead_test()
{
    ENTER_TESTSET
    test309();
    test327();
}
