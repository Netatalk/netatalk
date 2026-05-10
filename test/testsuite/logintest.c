#include <dlfcn.h>
#include <getopt.h>

#include "afpclient.h"
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

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
int     Port = DSI_AFPOVERTCP_PORT;
char    *Password = "";
char    *Vol = "";
char    *User;
char    *User2;
char    *Path;
int     Version = 34;
int     List = 0;
int     Mac = 0;
int     EmptyVol = 0;
char    *Test;
static char  *vers = "AFP3.4";

STATIC void connect_server(CONN *conn)
{
    int sock;
    sock = OpenClientSocket(Server, Port);

    if (sock < 0) {
        test_nottested();
        exit(ExitCode);
    }

    memset(&conn->dsi, 0, sizeof(conn->dsi));
    conn->dsi.socket = sock;
    /* Guard against a server that accepts the TCP connection but stalls before
     * replying (e.g. parent busy reaping prior children after a connection-limit
     * test). Without this, dsi_stream_read blocks in read() indefinitely. */
    struct timeval tv = { .tv_sec = 15, .tv_usec = 0 };
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
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
        fprintf(stdout, "DSIOpenSession\n");
    }

    if (DSIOpenSession(Conn)) {
        test_failed();
        goto test_exit;
    }

    if (Mac) {
        if (!Quiet) {
            fprintf(stdout, "DSIGetStatus\n");
        }

        if (DSIGetStatus(Conn)) {
            test_failed();
            goto test_exit;
        }
    }

    if (!Quiet) {
        fprintf(stdout, "DSICloseSession\n");
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
    } else {
        ret = FPopenLogin(Conn, vers, uam, "", "");
    }

    if (ret) {
        test_failed();
        goto test_exit;
    }

    if (Mac) {
        fprintf(stdout, "DSIGetStatus\n");

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

/* With Netatalk we define the default as 200 maximum connections,
 * while with the AFP server on Mac OS X 10.7 Lion the default is 10.
 * Therefore, test up to 201 to exercise either limit. */
#define TEST4_MAX_CONNS 201

/* The AFP Reference defines kFPNoMoreSessions (-1068) for FPLogin/FPLoginExt
 * when the server is at its session cap (Table 50: FPLoginExt result
 * codes), but says nothing about a DSI-layer error code for this case.
 * netatalk actually enforces the cap one layer down, in dsi_getsess.c,
 * by sending DSIERR_SERVBUSY (0xFBD1) on the DSIOpenSession reply — so
 * the client never reaches the FPLogin stage. This test accepts either
 * code as "limit hit". */
STATIC void test4(void)
{
    static char *uam = "No User Authent";
    CONN conn[TEST4_MAX_CONNS];
    int cnt = 0;
    unsigned int ret = 0;
    uint32_t code_host;
    int hit_limit = 0;
    ENTER_TEST

    for (int i = 0; i < TEST4_MAX_CONNS; i++) {
        if (!Quiet) {
            fprintf(stdout, "conn %d\n", i);
        }

        connect_server(&conn[i]);
        Dsi = &conn[i].dsi;
        cnt++;

        if (Version >= 30) {
            ret = FPopenLoginExt(&conn[i], vers, uam, "", "");
        } else {
            ret = FPopenLogin(&conn[i], vers, uam, "", "");
        }

        code_host = ntohl(ret);

        if (code_host == (uint32_t) AFPERR_MAXSESS
                || code_host == DSIERR_SERVBUSY) {
            hit_limit = 1;
            break;
        }

        if (ret) {
            if (!Quiet) {
                fprintf(stdout,
                        "Unexpected login error at conn %d: %d (%s)\n",
                        i, code_host, afp_error(ret));
            }

            test_failed();
            goto cleanup;
        }
    }

    if (!hit_limit) {
        if (!Quiet) {
            fprintf(stdout,
                    "All %d logins succeeded; server cap is > %d\n",
                    cnt, cnt);
        }

        test_nottested();
    }

cleanup:
    /* Send DSICloseSession on all connections that have an open DSI session,
     * then close sockets. DSIERR_SERVBUSY means DSIOpenSession itself was
     * rejected -- no DSI session was established, so skip DSICloseSession for
     * that slot and just shut its socket. AFPERR_MAXSESS means DSIOpenSession
     * succeeded but AFP rejected the login -- the DSI session IS open and must
     * be properly closed, or the server keeps the slot occupied. Sleep after to
     * let the server reap workers, otherwise the next test may fail with
     * SERVBUSY waiting for a free fork slot. */
    {
        int failed_idx = (hit_limit && code_host == DSIERR_SERVBUSY) ? cnt - 1 : -1;

        for (int j = 0; j < cnt; j++) {
            if (j != failed_idx) {
                DSICloseSession(&conn[j]);
            }

            CloseClientSocket(conn[j].dsi.socket);
        }
    }

    if (cnt > 0) {
        fflush(stdout);
        sleep(3);
    }

    exit_test("Logintest:test4: connection limit hit returns server-busy");
}

/* ------------------------- */
STATIC void test5(void)
{
    static char *uam = "Cleartxt Passwrd";
    int ret;
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;

    if (!User || Password[0] == '\0') {
        test_skipped(T_CRED);
        goto test_exit;
    }

    if (Version >= 30) {
        ret = FPopenLoginExt(Conn, vers, uam, User, Password);
    } else {
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
    dsi_send(dsi);
    dsi_cmd_receive(dsi);

    if (!User || Password[0] == '\0') {
        test_skipped(T_CRED);
        goto test_exit;
    }

    if (dsi->header.dsi_code) {
        test_failed();
        goto test_exit;
    }

    if (!Quiet) {
        fprintf(stdout, "DSICloseSession\n");
    }

    if (DSICloseSession(Conn)) {
        test_failed();
        goto test_exit;
    }

test_exit:
    CloseClientSocket(Dsi->socket);
    exit_test("Logintest:test6: DSIOpenSession non zero parameter should be ignored by the server");
}

/* Direct round-trip through the renamed dsi_stream_send / dsi_cmd_receive
 * primitives. Sends an unknown AFP command (255) to an open but
 * unauthenticated session; expect AFPERR_NOOP from the server's switch
 * dispatcher. Catches regressions in the I/O layer. */
STATIC void test7(void)
{
    DSI *dsi;
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;
    dsi = Dsi;

    if (DSIOpenSession(Conn)) {
        test_failed();
        goto test_exit;
    }

    memset(&dsi->header, 0, sizeof(dsi->header));
    dsi->header.dsi_flags = DSIFL_REQUEST;
    dsi->header.dsi_command = DSIFUNC_CMD;
    dsi->header.dsi_requestID = htons(dsi_clientID(dsi));
    dsi->commands[0] = 0xFF;            /* unknown AFP command */
    dsi->commands[1] = 0;
    dsi->cmdlen = 2;
    dsi_send(dsi);
    dsi_cmd_receive(dsi);

    /* Any non-zero error code means the round trip worked. A zero result
     * would mean the server accepted an unknown command, which would be a
     * server bug, not a client one. */
    if (dsi->header.dsi_code == 0) {
        test_failed();
        goto test_exit;
    }

    if (DSICloseSession(Conn)) {
        test_failed();
        goto test_exit;
    }

test_exit:
    CloseClientSocket(Dsi->socket);
    exit_test("Logintest:test7: DSI round-trip via renamed dsi_stream_send/dsi_cmd_receive");
}

/* FPLoginExt + No User Authent via AFPopenLoginExt_pwd directly, bypassing
 * the FPopenLoginExt wrapper. Validates the post-B1 wire format
 * independently of the wrapper's version-branching. AFP >= 3.0 only. */
STATIC void test8(void)
{
    static const char *uam = "No User Authent";
    unsigned int ret;
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;

    if (Version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    ret = AFPopenLoginExt_pwd(Conn, vers, uam, "", "");

    if (ret) {
        test_failed();
        goto test_exit;
    }

    if (FPLogOut(Conn)) {
        test_failed();
    }

test_exit:
    CloseClientSocket(Dsi->socket);
    exit_test("Logintest:test8: FPLoginExt + No User Authent (direct)");
}

/* Smoke test for AFPLoginCont primitive (Part B4). Drives DHCAST128 with
 * an empty UserAuthInfo to provoke kFPAuthContinue, then sends garbage
 * via AFPLoginCont to verify the wire format reaches the server's
 * logincont entry point. Skipped if the server does not load DHCAST128
 * (returns AFPERR_BADUAM) or doesn't return kFPAuthContinue. Does NOT
 * authenticate -- this only exercises the round-trip plumbing. */
STATIC void test9(void)
{
    static const char *uam = "DHCAST128";
    unsigned int ret;
    uint8_t garbage[16];
    ENTER_TEST
    connect_server(Conn);
    Dsi = &Conn->dsi;

    if (Version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    if (!User) {
        test_skipped(T_CRED);
        goto test_exit;
    }

    ret = AFPopenLoginExt(Conn, vers, uam, User, NULL, 0);

    if (ntohl(ret) != (uint32_t) AFPERR_AUTHCONT) {
        if (!Quiet) {
            fprintf(stdout,
                    "AFPopenLoginExt returned %d (%s), expected AFPERR_AUTHCONT\n",
                    ntohl(ret), afp_error(ret));
        }

        test_nottested();
        goto test_exit;
    }

    if (Conn->login_cont_len == 0) {
        /* Got AUTHCONT but no payload captured -- that's a real bug. */
        test_failed();
        goto test_exit;
    }

    memset(garbage, 0xAA, sizeof(garbage));
    ret = AFPLoginCont(Conn, garbage, sizeof(garbage));

    /* Any non-zero result is acceptable: the server rejected our garbage
     * response, which means AFPLoginCont reached the UAM. A zero result
     * would mean we authenticated with random bytes, which is impossible
     * unless something is very wrong. */
    if (ret == 0) {
        test_failed();
    }

test_exit:
    CloseClientSocket(Dsi->socket);
    exit_test("Logintest:test9: AFPLoginCont primitive round-trip");
}

/* UAM dispatch table. Each runner returns dsi_code (0 == success).
 * Plug-in point for DHX/DHX2/SRP later -- add a runner that drives the
 * corresponding crypto via AFPopenLoginExt + AFPLoginCont, then list it
 * here. */
struct uam_test {
    const char *uam_name;
    unsigned int (*run)(CONN *conn, const char *vers,
                        const char *user, const char *pwd);
    int needs_cred;       /* 1 = skip if !User/!Password */
};

static unsigned int run_nua(CONN *conn, const char *pvers,
                            const char *user _U_, const char *pwd _U_)
{
    return AFPopenLoginExt_pwd(conn, pvers, "No User Authent", "", "");
}

static unsigned int run_cleartxt(CONN *conn, const char *pvers,
                                 const char *user, const char *pwd)
{
    return AFPopenLoginExt_pwd(conn, pvers, "Cleartxt Passwrd", user, pwd);
}

static const struct uam_test uam_matrix[] = {
    { "No User Authent",  run_nua,      0 },
    { "Cleartxt Passwrd", run_cleartxt, 1 },
    /* Plug-in slots (need crypto helpers, see comment above):
     * { "Randnum Exchange", run_randnum,  1 },
     * { "2-Way Randnum exchange", run_randnum2,  1 },
     * { "DHCAST128", run_dhx,  1 },
     * { "DHX2",      run_dhx2, 1 },
     * { "SRP",       run_srp,  1 },
     */
    { NULL, NULL, 0 }
};

STATIC void test10(void)
{
    ENTER_TEST

    if (Version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    for (const struct uam_test *e = uam_matrix; e->uam_name; e++) {
        if (e->needs_cred && (!User || Password[0] == '\0')) {
            if (!Quiet) {
                fprintf(stdout, "  [%s] skipped (no credentials)\n",
                        e->uam_name);
            }

            continue;
        }

        connect_server(Conn);
        Dsi = &Conn->dsi;
        unsigned int ret = e->run(Conn, vers, User, Password);

        if (ret) {
            if (!Quiet) {
                fprintf(stdout, "  [%s] failed: dsi_code=0x%x\n",
                        e->uam_name, ntohl(ret));
            }

            test_failed();
        } else {
            Conn->afp_version = Version;
            FPLogOut(Conn);

            if (!Quiet) {
                fprintf(stdout, "  [%s] ok\n", e->uam_name);
            }
        }

        CloseClientSocket(Dsi->socket);
    }

test_exit:
    exit_test("Logintest:test10: UAM matrix walk");
}

struct test_fn {
    const char *name;
    void (*fn)(void);
};

static struct test_fn Test_list[] = {
    { "test1",  test1  },
    { "test2",  test2  },
    { "test3",  test3  },
    { "test4",  test4  },
    { "test5",  test5  },
    { "test6",  test6  },
    { "test7",  test7  },
    { "test8",  test8  },
    { "test9",  test9  },
    { "test10", test10 },
    { NULL, NULL },
};

/* =============================== */
static void list_tests(void)
{
    int i = 0;
    fprintf(stdout, "Available tests. Run individually with the -f option.\n");

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

    while (Test_list[i].name != NULL) {
        if (!strcmp(Test_list[i].name, name)) {
            break;
        }

        i++;
    }

    if (Test_list[i].name == NULL) {
        handle = dlopen(NULL, RTLD_NOW);

        if (handle) {
            dlerror();
            fn = dlsym(handle, name);

            if ((error = dlerror()) != NULL) {
                fprintf(stdout, "%s\n", error);
                dlclose(handle);
                handle = NULL;
            }
        } else {
            fprintf(stdout, "%s\n", dlerror());
        }

        if (!handle || !fn) {
            test_nottested();
            return;
        }
    } else {
        fn = Test_list[i].fn;
    }

    (*fn)();

    if (handle) {
        dlclose(handle);
    }
}

/* ----------- */
static void run_all(void)
{
    int i = 0;

    while (Test_list[i].name != NULL) {
        Test_list[i].fn();
        i++;
    }
}

/* =============================== */
void usage(char *av0)
{
    fprintf(stdout,
            "usage:\t%s [-1234567CmVv] [-h host] [-p port] [-u user] [-w password] [-f test]\n",
            av0);
    fprintf(stdout, "\t-m\tserver is a Mac\n");
    fprintf(stdout, "\t-h\tserver host name (default localhost)\n");
    fprintf(stdout, "\t-p\tserver port (default 548)\n");
    fprintf(stdout, "\t-u\tuser name (default uid)\n");
    fprintf(stdout, "\t-w\tpassword\n");
    fprintf(stdout, "\t-1\tAFP 2.1 version\n");
    fprintf(stdout, "\t-2\tAFP 2.2 version\n");
    fprintf(stdout, "\t-3\tAFP 3.0 version\n");
    fprintf(stdout, "\t-4\tAFP 3.1 version\n");
    fprintf(stdout, "\t-5\tAFP 3.2 version\n");
    fprintf(stdout, "\t-6\tAFP 3.3 version\n");
    fprintf(stdout, "\t-7\tAFP 3.4 version (default)\n");
    fprintf(stdout, "\t-v\tverbose\n");
    fprintf(stdout, "\t-V\tvery verbose\n");
    fprintf(stdout, "\t-C\tturn off terminal color output\n");
    fprintf(stdout, "\t-f\ttest to run\n");
    fprintf(stdout, "\t-l\tlist tests\n");
    exit(1);
}

/* ------------------------------- */
int main(int ac, char **av)
{
    int cc;

    if (ac == 1) {
        usage(av[0]);
    }

    while ((cc = getopt(ac, av, "1234567CmVvf:h:lp:u:w:")) != EOF) {
        switch (cc) {
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

        case 'f':
            Test = strdup(optarg);
            break;

        case 'h':
            Server = strdup(optarg);
            break;

        case 'l':
            List = 1;
            break;

        case 'm':
            Mac = 1;
            break;

        case 'p' :
            Port = atoi(optarg);

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
            usage(av[0]);
        }
    }

    if (List) {
        list_tests();
        exit(2);
    }

    if (!Quiet) {
        fprintf(stdout, "Connecting to host %s:%d\n", Server, Port);
    }

    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        return 1;
    }

    if (Test != NULL) {
        run_one(Test);
    } else {
        run_all();
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
