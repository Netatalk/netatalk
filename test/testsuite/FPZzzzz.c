/* ----------------------------------------------
*/
#include <limits.h>

#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

extern char *Server;
extern int  Port;
extern char *Password;
extern char *vers;
extern char *uam;
extern const char *afptest_uam;

static volatile int sigp = 0;

static void pipe_handler(int signum)
{
    (void)signum;
    sigp = 1;
}

static int get_positive_env_uint(const char *name, unsigned int default_value,
                                 unsigned int max_value, unsigned int *result)
{
    const char *value = getenv(name);
    char *end;
    unsigned long parsed;

    if (!value || !*value) {
        *result = default_value;
        return 0;
    }

    errno = 0;
    parsed = strtoul(value, &end, 10);

    if (value[0] == '-' || errno || end == value || *end || parsed == 0 ||
            parsed > max_value) {
        fprintf(stderr, "%s must be an integer between 1 and %u\n", name,
                max_value);
        return -1;
    }

    *result = (unsigned int)parsed;
    return 0;
}

/* Wait beyond the ordinary two-minute idle timeout. The server may skip
 * one tick after client traffic, so allow an extra tick plus scheduling
 * margin. CI uses tickleval=10, timeout=12 and a 140-second wait. */
static int wait_for_idle_timeout(void)
{
    unsigned int seconds = 180;
    unsigned int tickleval;
    unsigned int timeout;
    unsigned int minimum_wait;

    if (get_positive_env_uint("AFP_SLEEP_TEST_WAIT", seconds, UINT_MAX,
                              &seconds) ||
            get_positive_env_uint("AFP_TICKLEVAL", 30, INT_MAX,
                                  &tickleval) ||
            get_positive_env_uint("AFP_TIMEOUT", 4, INT_MAX, &timeout)) {
        return -1;
    }

    if (timeout == UINT_MAX || tickleval > UINT_MAX / (timeout + 1)) {
        fprintf(stderr, "configured idle timeout is too large\n");
        return -1;
    }

    minimum_wait = (timeout + 1) * tickleval;

    if (seconds <= 120 || seconds <= minimum_wait) {
        fprintf(stderr,
                "AFP_SLEEP_TEST_WAIT must exceed 120 seconds and %u seconds "
                "for AFP_TICKLEVAL=%u and AFP_TIMEOUT=%u\n",
                minimum_wait, tickleval, timeout);
        return -1;
    }

    fprintf(stdout, "sleep %u seconds\n", seconds);

    while ((seconds = sleep(seconds)) != 0) {
        /* A signal must not shorten the interval being tested. */
    }

    return 0;
}

/* ------------------------- */
STATIC void test223()
{
    char *name = "t223 file";
    uint16_t vol = VolID;
    unsigned int ret;
    struct sigaction action;
    uint32_t time = 12345;
    ENTER_TEST

    if (!Test) {
        if (!Quiet) {
            fprintf(stdout, "Run sleep tests with: -f FPZzzzz.\n");
        }

        test_skipped(T_SINGLE);
        goto test_exit;
    }

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    action.sa_handler = pipe_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_nottested();
        goto test_exit;
    }

    /* Get session token */
    FAIL(FPGetSessionToken(Conn, 3, time, strlen("test223"), "test223"))
    FAIL(FPZzzzz(Conn, 0))
    FAILEXIT(wait_for_idle_timeout(), fin)
    ret = FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name);

    if (sigp || ret == (unsigned) - 1) {
        fprintf(stdout, "\tFAILED disconnected %d\n", sigp);
        test_failed();
        /* try to reconnect */
        Conn->dsi.socket = OpenClientSocket(Server, Port);

        if (Conn->dsi.socket < 0) {
            test_nottested();
            goto fin;
        }

        ret = afptest_login(Conn, vers, uam, afptest_uam, User, Password);

        if (ret) {
            test_nottested();
            goto fin;
        }

        /* Get session token, killing above session which is possibly in disconnected state */
        FAIL(FPGetSessionToken(Conn, 3, time, strlen("test223"), "test223"))
        vol = VolID  = FPOpenVol(Conn, Vol);

        if (vol == 0xffff) {
            test_nottested();
            goto fin;
        }

        FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    } else if (ret) {
        test_failed();
    }

    /* always there ? */
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
fin:
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_nottested();
    }

test_exit:
    exit_test("FPZzzzz:test223: AFP 3.x enter sleep mode");
}

/* ------------------------- */
STATIC void test224()
{
    char *name = "t224 file";
    uint16_t vol = VolID;
    unsigned int ret;
    struct sigaction action;
    uint32_t time = 12345;
    ENTER_TEST

    if (!Test) {
        if (!Quiet) {
            fprintf(stdout, "Run sleep tests with: -f FPZzzzz.\n");
        }

        test_skipped(T_SINGLE);
        goto test_exit;
    }

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    sigp = 0;
    action.sa_handler = pipe_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_nottested();
        goto test_exit;
    }

    /* Get session token */
    FAIL(FPGetSessionToken(Conn, 3, time, strlen("test224"), "test224"))
    FAILEXIT(wait_for_idle_timeout(), fin)
    ret = FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name);

    if (!sigp && ret != (unsigned) - 1) {
        fprintf(stdout, "\tFAILED not disconnected \n");
        test_failed();
    } else {
        /* try to reconnect */
        Conn->dsi.socket = OpenClientSocket(Server, Port);

        if (Conn->dsi.socket < 0) {
            test_nottested();
            goto fin;
        }

        ret = afptest_login(Conn, vers, uam, afptest_uam, User, Password);

        if (ret) {
            test_nottested();
            goto fin;
        }

        /* Get session token, killing above session which is in disconnected state */
        FAIL(FPGetSessionToken(Conn, 3, time, strlen("test224"), "test224"))
        vol = VolID  = FPOpenVol(Conn, Vol);

        if (vol == 0xffff) {
            test_nottested();
            goto fin;
        }

        FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    }

    /* always there ? */
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
fin:
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_nottested();
    }

test_exit:
    exit_test("FPZzzzz:test224: disconnected after 2 mn");
}

/* ------------------------- */
STATIC void test239()
{
    char *name = "t239 file";
    uint16_t vol = VolID;
    struct sigaction action;
    ENTER_TEST

    if (!Test) {
        if (!Quiet) {
            fprintf(stdout, "Run sleep tests with: -f FPZzzzz.\n");
        }

        test_skipped(T_SINGLE);
        goto test_exit;
    }

    if (Conn->afp_version < 30) {
        test_skipped(T_AFP3);
        goto test_exit;
    }

    action.sa_handler = pipe_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_nottested();
        goto test_exit;
    }

    FAIL(FPZzzzz(Conn, 1))
    FAILEXIT(wait_for_idle_timeout(), fin)
    FAIL(FPZzzzz(Conn, 2))
    FAIL(FPCreateFile(Conn, vol, 0, DIRDID_ROOT, name))
    FAIL(FPDelete(Conn, vol, DIRDID_ROOT, name))
fin:
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGPIPE, &action, NULL) < 0) {
        test_nottested();
    }

test_exit:
    exit_test("FPZzzzz:test239: AFP 3.x enter extended sleep");
}

/* ----------- */
void FPZzzzz_test()
{
    ENTER_TESTSET
    test223();
    test224();
    test239();
}
