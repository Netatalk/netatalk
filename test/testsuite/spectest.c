#include <dlfcn.h>
#include <getopt.h>

#include "afpclient.h"
#include "afpclient_transport.h"
#include "afpcmd.h"
#include "afphelper.h"
#include "testhelper.h"

uint16_t VolID;
static DSI *dsi;
CONN *Conn;
CONN *Conn2;
DSI *Dsi;
DSI *Dsi2;

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
#ifdef HAVE_SPOTLIGHT_TEST
EXT_FN(FPSpotlightRPC);
#endif
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
EXT_FN(FPSetACL);
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

EXT_FN(T2FPByteRangeLock);
EXT_FN(T2FPCopyFile);
EXT_FN(T2FPCreateFile);
EXT_FN(T2FPDelete);
EXT_FN(T2FPGetFileDirParms);
EXT_FN(T2FPGetSrvrParms);
EXT_FN(T2FPGetVolParms);
EXT_FN(T2FPMoveAndRename);
EXT_FN(T2FPOpenFork);
EXT_FN(T2FPSetDirParms);
EXT_FN(T2FPSetFileParms);
EXT_FN(T2FPResolveID);
EXT_FN(T2FPRead);
EXT_FN(T2FPSetForkParms);
EXT_FN(T2LockAttack);

EXT_FN(Dircache_attack);
EXT_FN(Encoding);
EXT_FN(Readonly);
EXT_FN(Utf8);

EXT_FN(T3Error);
EXT_FN(T3FPRead);
EXT_FN(T3FPzzz);

static void spectest_libafpclient_smoke_test(void);

enum spectest_tier {
    SPECTEST_TIER_T1,
    SPECTEST_TIER_T2,
    SPECTEST_TIER_T3,
};

enum spectest_backend {
    SPECTEST_BACKEND_LEGACY_DSI,
    SPECTEST_BACKEND_LIBAFPCLIENT,
    SPECTEST_BACKEND_LIBAFPCLIENT_RAW_AFP,
};

#define SPECTEST_FLAG_NONE              0U
#define SPECTEST_FLAG_PRIMARY_VOL       (1U << 0)
#define SPECTEST_FLAG_SECOND_USER       (1U << 1)
#define SPECTEST_FLAG_SECOND_VOL        (1U << 2)
#define SPECTEST_FLAG_LOCAL_PATH        (1U << 3)
#define SPECTEST_FLAG_LOCAL_FS          (1U << 4)
#define SPECTEST_FLAG_READ_LOCKS        (1U << 5)
#define SPECTEST_FLAG_READONLY_VOL      (1U << 6)
#define SPECTEST_FLAG_SLEEP             (1U << 7)
#define SPECTEST_FLAG_WIRE_PROTOCOL     (1U << 8)
#define SPECTEST_FLAG_NO_RUNNER_VOL     (1U << 9)

struct test_fn {
    const char *name;
    void (*fn)(void);
    enum spectest_tier tier;
    enum spectest_backend backend;
    unsigned int flags;
    const char *legacy_reason;
};

struct spectest_transport_backend {
    enum spectest_backend backend;
    const char *name;
    int (*connect)(CONN **conn, DSI **dsi_slot, char *host, char *user);
    void (*disconnect)(CONN **conn);
};
#define TEST_ENTRY(a, tier_value, backend_value, flags_value) \
    { # a, FN(a), tier_value, backend_value, flags_value, NULL },
#define TEST_T1(a) \
    TEST_ENTRY(a, SPECTEST_TIER_T1, SPECTEST_BACKEND_LIBAFPCLIENT_RAW_AFP, \
               SPECTEST_FLAG_PRIMARY_VOL)
#define TEST_T1_FLAGS(a, flags_value) \
    TEST_ENTRY(a, SPECTEST_TIER_T1, SPECTEST_BACKEND_LIBAFPCLIENT_RAW_AFP, \
               SPECTEST_FLAG_PRIMARY_VOL | (flags_value))
#define TEST_T2(a) \
    TEST_ENTRY(a, SPECTEST_TIER_T2, \
               SPECTEST_BACKEND_LIBAFPCLIENT_RAW_AFP, \
               SPECTEST_FLAG_PRIMARY_VOL | SPECTEST_FLAG_LOCAL_PATH | \
               SPECTEST_FLAG_LOCAL_FS)
#define TEST_T2_FLAGS(a, flags_value) \
    TEST_ENTRY(a, SPECTEST_TIER_T2, \
               SPECTEST_BACKEND_LIBAFPCLIENT_RAW_AFP, \
               SPECTEST_FLAG_PRIMARY_VOL | SPECTEST_FLAG_LOCAL_PATH | \
               SPECTEST_FLAG_LOCAL_FS | (flags_value))
#define TEST_T3_FLAGS(a, flags_value, reason_value) \
    { # a, FN(a), SPECTEST_TIER_T3, SPECTEST_BACKEND_LEGACY_DSI, \
      SPECTEST_FLAG_PRIMARY_VOL | (flags_value), reason_value },

static struct test_fn Test_list[] = {
    TEST_T1_FLAGS(FPAddAPPL, SPECTEST_FLAG_SECOND_USER)
    TEST_T1_FLAGS(FPAddComment, SPECTEST_FLAG_SECOND_USER)
    TEST_T1(FPAddIcon)
    TEST_T1_FLAGS(FPByteRangeLock,
    SPECTEST_FLAG_SECOND_USER | SPECTEST_FLAG_READ_LOCKS)
    TEST_T1_FLAGS(FPByteRangeLockExt,
    SPECTEST_FLAG_SECOND_USER | SPECTEST_FLAG_READ_LOCKS)
    TEST_T1(FPCatSearch)
    TEST_T1_FLAGS(FPCatSearchExt, SPECTEST_FLAG_LOCAL_PATH)
#ifdef HAVE_SPOTLIGHT_TEST
    TEST_T1(FPSpotlightRPC)
#endif
    TEST_T1(FPCloseDir)
    TEST_T1(FPCloseDT)
    TEST_T1(FPCloseFork)
    TEST_T1(FPCloseVol)
    TEST_T1_FLAGS(FPCreateDir, SPECTEST_FLAG_SECOND_USER)
    TEST_T1(FPCreateFile)
    TEST_T1_FLAGS(FPCopyFile, SPECTEST_FLAG_SECOND_VOL)
    TEST_T1_FLAGS(FPDelete, SPECTEST_FLAG_SECOND_USER)
    TEST_T1_FLAGS(FPEnumerate, SPECTEST_FLAG_LOCAL_PATH)
    TEST_T1(FPEnumerateExt)
    TEST_T1(FPEnumerateExt2)
    TEST_T1(FPExchangeFiles)
    TEST_T1(FPFlush)
    TEST_T1(FPFlushFork)
    TEST_T1(FPGetACL)
    TEST_T1(FPSetACL)
    TEST_T1(FPGetAPPL)
    TEST_T1_FLAGS(FPGetComment, SPECTEST_FLAG_SECOND_USER)
    TEST_T1(FPGetExtAttr)
    TEST_T1(FPGetFileDirParms)
    TEST_T1(FPGetForkParms)
    TEST_T1(FPGetIcon)
    TEST_T1(FPGetIconInfo)
    TEST_T1(FPGetSessionToken)
    TEST_T1(FPGetSrvrInfo)
    TEST_T1(FPGetSrvrMsg)
    TEST_T1(FPGetSrvrParms)
    TEST_T1(FPGetUserInfo)
    TEST_T1(FPGetVolParms)
    TEST_T1(FPMapID)
    TEST_T1(FPMapName)
    TEST_T1_FLAGS(FPMoveAndRename, SPECTEST_FLAG_LOCAL_PATH)
    TEST_T1(FPOpenDir)
    TEST_T1(FPOpenDT)
    TEST_T1(FPOpenFork)
    TEST_T1(FPOpenVol)
    TEST_T1(FPRead)
    TEST_T1(FPReadExt)
    TEST_T1(FPRemoveAPPL)
    TEST_T1_FLAGS(FPRemoveComment, SPECTEST_FLAG_SECOND_USER)
    TEST_T1(FPRename)
    TEST_T1(FPResolveID)
    TEST_T1(FPSetDirParms)
    TEST_T1(FPSetFileDirParms)
    TEST_T1(FPSetFileParms)
    TEST_T1(FPSetForkParms)
    TEST_T1(FPSetVolParms)
    TEST_T1(FPSync)
    TEST_T1(FPWrite)
    TEST_T1(FPWriteExt)
    TEST_T2_FLAGS(T2FPByteRangeLock, SPECTEST_FLAG_SECOND_USER)
    TEST_T2(T2FPCreateFile)
    TEST_T2(T2FPCopyFile)
    TEST_T2(T2FPDelete)
    TEST_T2(T2FPGetFileDirParms)
    TEST_T2_FLAGS(T2FPGetSrvrParms, SPECTEST_FLAG_SECOND_VOL)
    TEST_T2(T2FPGetVolParms)
    TEST_T2(T2FPMoveAndRename)
    TEST_T2(T2FPOpenFork)
    TEST_T2(T2FPSetDirParms)
    TEST_T2(T2FPSetFileParms)
    TEST_T2(T2FPResolveID)
    TEST_T2(T2FPRead)
    TEST_T2(T2FPSetForkParms)
    TEST_T2_FLAGS(T2LockAttack, SPECTEST_FLAG_SECOND_USER)

    TEST_T1_FLAGS(Dircache_attack, SPECTEST_FLAG_SECOND_USER)
    TEST_T1(Encoding)
    TEST_T1_FLAGS(Readonly, SPECTEST_FLAG_READONLY_VOL)
    TEST_T1(Utf8)
    {
        "LibafpclientSmoke",
        spectest_libafpclient_smoke_test,
        SPECTEST_TIER_T1,
        SPECTEST_BACKEND_LIBAFPCLIENT,
        SPECTEST_FLAG_PRIMARY_VOL | SPECTEST_FLAG_NO_RUNNER_VOL,
        NULL
    },

    /* T3 testsets intentionally retain raw legacy DSI. */
    TEST_T3_FLAGS(FPDisconnectOldSession, SPECTEST_FLAG_WIRE_PROTOCOL,
                  "owns sockets and hand-drives session replacement")
    TEST_T3_FLAGS(T3Error, SPECTEST_FLAG_WIRE_PROTOCOL,
                  "constructs malformed DSI and AFP requests")
    TEST_T3_FLAGS(T3FPRead, SPECTEST_FLAG_WIRE_PROTOCOL,
                  "interleaves partial DSI replies on a self-managed socket")
    TEST_T3_FLAGS(T3FPzzz,
                  SPECTEST_FLAG_WIRE_PROTOCOL | SPECTEST_FLAG_SLEEP,
                  "tests idle disconnect and manually reconnects the session")

    {
        NULL, NULL, SPECTEST_TIER_T1, SPECTEST_BACKEND_LEGACY_DSI,
        SPECTEST_FLAG_NONE, NULL
    },
};

static const char *spectest_tier_name(enum spectest_tier tier)
{
    switch (tier) {
    case SPECTEST_TIER_T1:
        return "T1";

    case SPECTEST_TIER_T2:
        return "T2";

    case SPECTEST_TIER_T3:
        return "T3";
    }

    return "unknown";
}

static const char *spectest_backend_name(enum spectest_backend backend)
{
    switch (backend) {
    case SPECTEST_BACKEND_LEGACY_DSI:
        return "legacy-dsi";

    case SPECTEST_BACKEND_LIBAFPCLIENT:
        return "libafpclient";

    case SPECTEST_BACKEND_LIBAFPCLIENT_RAW_AFP:
        return "libafpclient-raw-afp";
    }

    return "unknown";
}

static const struct test_fn *spectest_find_test(const char *name)
{
    int i = 0;

    if (!name) {
        return NULL;
    }

    while (Test_list[i].name != NULL) {
        if (!strcmp(Test_list[i].name, name)) {
            return &Test_list[i];
        }

        i++;
    }

    return NULL;
}

static enum spectest_backend spectest_backend_for_run(const char *name)
{
    const struct test_fn *test;
    test = spectest_find_test(name);
    return test ? test->backend : SPECTEST_BACKEND_LEGACY_DSI;
}

static void spectest_print_flag(unsigned int flags, unsigned int flag,
                                const char *name, int *first)
{
    if (!(flags & flag)) {
        return;
    }

    fprintf(stdout, "%s%s", *first ? "" : ",", name);
    *first = 0;
}

static void spectest_print_preconditions(unsigned int flags)
{
    int first = 1;
    spectest_print_flag(flags, SPECTEST_FLAG_PRIMARY_VOL, "primary-volume",
                        &first);
    spectest_print_flag(flags, SPECTEST_FLAG_SECOND_USER, "second-user",
                        &first);
    spectest_print_flag(flags, SPECTEST_FLAG_SECOND_VOL, "second-volume",
                        &first);
    spectest_print_flag(flags, SPECTEST_FLAG_LOCAL_PATH, "local-path", &first);
    spectest_print_flag(flags, SPECTEST_FLAG_LOCAL_FS, "local-filesystem",
                        &first);
    spectest_print_flag(flags, SPECTEST_FLAG_READ_LOCKS, "read-locks", &first);
    spectest_print_flag(flags, SPECTEST_FLAG_READONLY_VOL, "readonly-volume",
                        &first);
    spectest_print_flag(flags, SPECTEST_FLAG_SLEEP, "sleep", &first);
    spectest_print_flag(flags, SPECTEST_FLAG_WIRE_PROTOCOL, "wire-protocol",
                        &first);
    spectest_print_flag(flags, SPECTEST_FLAG_NO_RUNNER_VOL,
                        "self-managed-volume", &first);

    if (first) {
        fprintf(stdout, "-");
    }
}

struct spectest_uam_alias {
    const char *alias;
    const char *uam;
};

static const struct spectest_uam_alias spectest_uam_aliases[] = {
    { "guest",     "No User Authent"        },
    { "clrtxt",    "Cleartxt Passwrd"       },
    { "randnum",   "Randnum Exchange"       },
    { "randnum2",  "2-Way Randnum Exchange" },
    { "dhx",       "DHCAST128"              },
    { "dhx2",      "DHX2"                   },
    { "srp",       "SRP"                    },
    { NULL, NULL }
};

static const char *spectest_resolve_uam(const char *name)
{
    int i = 0;

    if (!name || !*name) {
        return name;
    }

    while (spectest_uam_aliases[i].alias != NULL) {
        if (!strcmp(name, spectest_uam_aliases[i].alias)) {
            return spectest_uam_aliases[i].uam;
        }

        if (!strcmp(name, spectest_uam_aliases[i].uam)) {
            return spectest_uam_aliases[i].uam;
        }

        i++;
    }

    return name;
}

static void spectest_libafpclient_smoke_test(void)
{
    ENTER_TEST
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT

    if (afptest_libafpclient_smoke(Conn, Vol) != 0) {
        test_failed();
    }

#else
    test_nottested();
#endif
    exit_test("LibafpclientSmoke:test1: libafpclient smoke path");
}

/* =============================== */
static void press_enter(const char *s)
{
    if (!Interactive) {
        return;
    }

    if (s) {
        fprintf(stdout, "--> Performing: %s\n", s);
    }

    fprintf(stdout, "Press <ENTER> to continue.\n");

    while (fgetc(stdin) != '\n')
        ;
}

/* =============================== */
static void list_tests(void)
{
    int i = 0;
    fprintf(stdout, "Available testsets. Run individually with the -f option.\n");
    fprintf(stdout, "%-24s %-4s %-22s %s\n", "Testset", "Tier",
            "Backend", "Preconditions / legacy reason");

    while (Test_list[i].name != NULL) {
        fprintf(stdout, "%-24s %-4s %-22s ", Test_list[i].name,
                spectest_tier_name(Test_list[i].tier),
                spectest_backend_name(Test_list[i].backend));
        spectest_print_preconditions(Test_list[i].flags);

        if (Test_list[i].legacy_reason) {
            fprintf(stdout, "; legacy: %s", Test_list[i].legacy_reason);
        }

        fprintf(stdout, "\n");
        i++;
    }
}

/* ----------- */
static void run_one(const char *name)
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
            fn = dlsym(handle, name);

            if ((error = dlerror()) != NULL)  {
                fprintf(stdout, "%s (%p)\n", error, fn);
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

    dsi = &Conn->dsi;

    if (!(Test_list[i].flags & SPECTEST_FLAG_NO_RUNNER_VOL)) {
        press_enter("Opening volume.");
        VolID = FPOpenVol(Conn, Vol);

        if (VolID == 0xffff) {
            test_nottested();
            return;
        }
    }

    press_enter(name);
    (*fn)();

    if (handle) {
        dlclose(handle);
    }

    if (!(Test_list[i].flags & SPECTEST_FLAG_NO_RUNNER_VOL)) {
        FPCloseVol(Conn, VolID);
    }
}

/* ----------- */
static void run_connected_range(int first, int last)
{
    int i = first;
    int needs_runner_vol = 0;

    while (i < last) {
        if (!(Test_list[i].flags & SPECTEST_FLAG_NO_RUNNER_VOL)) {
            needs_runner_vol = 1;
            break;
        }

        i++;
    }

    dsi = &Conn->dsi;

    if (needs_runner_vol) {
        press_enter("Opening volume.");
        VolID = FPOpenVol(Conn, Vol);

        if (VolID == 0xffff) {
            test_nottested();
            return;
        }
    }

    i = first;

    while (i < last) {
        press_enter(Test_list[i].name);
        Test_list[i].fn();
        i++;
    }

    if (needs_runner_vol) {
        FPCloseVol(Conn, VolID);
    }
}

char Data[300000] = "";
/* ------------------------------- */
char    *Server = "localhost";
char    *Server2;
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
int     EmptyVol = 0;
enum ad_format adouble = AD_EA;

char *vers = "AFP3.4";
char *uam = "Cleartxt Passwrd";
static char legacy_uam[] = "Cleartxt Passwrd";

static int spectest_legacy_dsi_connect(CONN **conn, DSI **dsi_slot,
                                       char *host, char *user)
{
    int ret;
    int sock;

    if ((*conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        return 1;
    }

    sock = OpenClientSocket(host, Port);

    if (sock < 0) {
        free(*conn);
        *conn = NULL;
        return 2;
    }

    *dsi_slot = &(*conn)->dsi;
    (*dsi_slot)->socket = sock;

    if (Version >= 30) {
        ret = FPopenLoginExt(*conn, vers, legacy_uam, user, Password);
    } else {
        ret = FPopenLogin(*conn, vers, legacy_uam, user, Password);
    }

    if (ret) {
        printf("Login failed\n");
        exit(1);
    }

    (*conn)->afp_version = Version;
    return 0;
}

static void spectest_legacy_dsi_disconnect(CONN **conn)
{
    if (!conn || !*conn) {
        return;
    }

    AFPLogOut(*conn);
    CloseClientSocket((*conn)->dsi.socket);
    free(*conn);
    *conn = NULL;
}

static int spectest_libafpclient_connect(CONN **conn, DSI **dsi_slot,
        char *host, char *user)
{
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT
    int ret;

    if ((*conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        return 1;
    }

    if (!Quiet) {
        fprintf(stdout, "Connecting with libafpclient backend\n");
        fflush(stdout);
    }

    ret = afptest_libafpclient_login(*conn, host, Port, vers, uam, user,
                                     Password);

    if (ret) {
        printf("Login failed\n");
        free(*conn);
        *conn = NULL;
        return 1;
    }

    *dsi_slot = &(*conn)->dsi;

    if (!Quiet) {
        fprintf(stdout, "Connected with libafpclient backend\n");
        fflush(stdout);
    }

    return 0;
#else
    (void)conn;
    (void)dsi_slot;
    (void)host;
    (void)user;
    fprintf(stdout, "Spectest backend 'libafpclient' was not compiled in.\n");
    return 1;
#endif
}

static void spectest_libafpclient_disconnect(CONN **conn)
{
#ifdef HAVE_TESTSUITE_LIBAFPCLIENT

    if (!conn || !*conn) {
        return;
    }

    afptest_libafpclient_logout(*conn);
    afptest_libafpclient_close(*conn);
    free(*conn);
    *conn = NULL;
#else
    (void)conn;
#endif
}

static const struct spectest_transport_backend spectest_legacy_dsi_backend = {
    SPECTEST_BACKEND_LEGACY_DSI,
    "legacy-dsi",
    spectest_legacy_dsi_connect,
    spectest_legacy_dsi_disconnect,
};

static const struct spectest_transport_backend spectest_libafpclient_backend = {
    SPECTEST_BACKEND_LIBAFPCLIENT,
    "libafpclient",
    spectest_libafpclient_connect,
    spectest_libafpclient_disconnect,
};

static const struct spectest_transport_backend
    spectest_libafpclient_raw_afp_backend = {
    SPECTEST_BACKEND_LIBAFPCLIENT_RAW_AFP,
    "libafpclient-raw-afp",
    spectest_libafpclient_connect,
    spectest_libafpclient_disconnect,
};

static const struct spectest_transport_backend *spectest_transport_for_backend(
    enum spectest_backend backend)
{
    switch (backend) {
    case SPECTEST_BACKEND_LEGACY_DSI:
        return &spectest_legacy_dsi_backend;

    case SPECTEST_BACKEND_LIBAFPCLIENT:
        return &spectest_libafpclient_backend;

    case SPECTEST_BACKEND_LIBAFPCLIENT_RAW_AFP:
        return &spectest_libafpclient_raw_afp_backend;
    }

    return NULL;
}

static int spectest_connect_primary(
    const struct spectest_transport_backend *backend)
{
    int ret;

    if (!backend || !backend->connect) {
        fprintf(stdout, "Spectest backend '%s' is not implemented yet.\n",
                backend ? backend->name : "unknown");
        return 1;
    }

    ret = backend->connect(&Conn, &Dsi, Server, User);

    if (ret) {
        return ret;
    }

    dsi = Dsi;
    return 0;
}

static int spectest_connect_secondary(
    const struct spectest_transport_backend *backend)
{
    int ret;

    if (!User2) {
        return 0;
    }

    if (!backend || !backend->connect) {
        fprintf(stdout, "Spectest backend '%s' is not implemented yet.\n",
                backend ? backend->name : "unknown");
        return 1;
    }

    ret = backend->connect(&Conn2, &Dsi2, Server2 ? Server2 : Server, User2);

    if (ret == 2) {
        return 1;
    }

    return ret;
}

static void spectest_disconnect(const struct spectest_transport_backend
                                *backend,
                                CONN **conn)
{
    if (!backend || !backend->disconnect || !conn || !*conn) {
        return;
    }

    backend->disconnect(conn);
}

static int run_selected(const char *selectors)
{
    const struct spectest_transport_backend *backend;
    enum spectest_backend backend_id;
    char *selection;
    char *saveptr = NULL;
    char *name;
    int ret = 0;
    selection = strdup(selectors);

    if (!selection) {
        perror("strdup");
        return 1;
    }

    name = strtok_r(selection, ",", &saveptr);

    while (name) {
        backend_id = spectest_backend_for_run(name);
        backend = spectest_transport_for_backend(backend_id);

        if (!backend) {
            fprintf(stdout, "No spectest backend selected for '%s'.\n", name);
            ret = 1;
            break;
        }

        if (!Quiet) {
            fprintf(stdout, "Using spectest backend %s for %s\n",
                    backend->name, name);
            fflush(stdout);
        }

        ret = spectest_connect_primary(backend);

        if (ret) {
            break;
        }

        ret = spectest_connect_secondary(backend);

        if (ret) {
            spectest_disconnect(backend, &Conn);
            break;
        }

        run_one(name);
        spectest_disconnect(backend, &Conn);
        spectest_disconnect(backend, &Conn2);
        name = strtok_r(NULL, ",", &saveptr);
    }

    free(selection);
    return ret;
}

static int run_all(void)
{
    const struct spectest_transport_backend *backend;
    enum spectest_backend backend_id;
    int first = 0;
    int last;
    int ret;

    while (Test_list[first].name != NULL) {
        backend_id = Test_list[first].backend;
        last = first + 1;

        while (Test_list[last].name != NULL
                && Test_list[last].backend == backend_id) {
            last++;
        }

        backend = spectest_transport_for_backend(backend_id);

        if (!backend) {
            fprintf(stdout, "No spectest backend selected.\n");
            return 1;
        }

        if (!Quiet) {
            fprintf(stdout, "Using spectest backend %s\n", backend->name);
            fflush(stdout);
        }

        ret = spectest_connect_primary(backend);

        if (ret) {
            return ret;
        }

        ret = spectest_connect_secondary(backend);

        if (ret) {
            spectest_disconnect(backend, &Conn);
            return ret;
        }

        run_connected_range(first, last);
        spectest_disconnect(backend, &Conn);
        spectest_disconnect(backend, &Conn2);
        first = last;
    }

    return 0;
}

/* =============================== */
void usage(char *av0)
{
    fprintf(stdout,
            "usage:\t%s [-1234567aCEiLlmVv] [-A uam] [-h host] [-H host2] [-p port] [-s vol] [-c vol path] [-S vol2] "
            "[-u user] [-d user2] [-w password] [-F testsuite] [-f tests]\n", av0);
    fprintf(stdout, "\t-A\tUAM name or alias (default Cleartxt Passwrd)\n");
    fprintf(stdout, "\t-a\tvolume is using AppleDouble metadata and not EA\n");
    fprintf(stdout, "\t-m\tserver is a Mac\n");
    fprintf(stdout, "\t-h\tserver host name (default localhost)\n");
    fprintf(stdout, "\t-p\tserver port (default 548)\n");
    fprintf(stdout, "\t-s\tvolume to mount\n");
    fprintf(stdout, "\t-S\tsecond volume to mount\n");
    fprintf(stdout, "\t-c\tvolume path on the server\n");
    fprintf(stdout, "\t-u\tuser name (default uid)\n");
    fprintf(stdout, "\t-w\tpassword\n");
    fprintf(stdout, "\t-d\tsecond user for two connections (same password!)\n");
    fprintf(stdout, "\t-H\tsecond server for two connections\n");
    fprintf(stdout, "\t-1\tAFP 2.1 version\n");
    fprintf(stdout, "\t-2\tAFP 2.2 version\n");
    fprintf(stdout, "\t-3\tAFP 3.0 version\n");
    fprintf(stdout, "\t-4\tAFP 3.1 version\n");
    fprintf(stdout, "\t-5\tAFP 3.2 version\n");
    fprintf(stdout, "\t-6\tAFP 3.3 version\n");
    fprintf(stdout, "\t-7\tAFP 3.4 version (default)\n");
    fprintf(stdout, "\t-v\tverbose\n");
    fprintf(stdout, "\t-V\tvery verbose\n");
    fprintf(stdout, "\t-f\ttest(s) or testset(s) to run, comma-separated\n");
    fprintf(stdout, "\t-l\tlist testsets\n");
    fprintf(stdout,
            "\t-L\tserver has 'afp read locks = yes'; run byte-range read-lock conflict tests\n");
    fprintf(stdout,
            "\t-i\tinteractive mode, prompts before every test (debug purposes)\n");
    fprintf(stdout, "\t-C\tturn off terminal color output\n");
    fprintf(stdout,
            "\t-E\tempty test volume between tests (WARNING deletes all user data on volume)\n");
    exit(1);
}

/* ------------------------------- */
int main(int ac, char **av)
{
    int cc;
    int ret;

    if (ac == 1) {
        usage(av[0]);
    }

    while ((cc = getopt(ac, av, "1234567aCEiLlmVvA:c:d:f:H:h:p:S:s:u:w:")) != EOF) {
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

        case 'A':
            uam = strdup(spectest_resolve_uam(optarg));
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

        case 'E':
            EmptyVol = 1;
            break;

        case 'L':
            Locking = 1;
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

        case 'p' :
            Port = atoi(optarg);

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

        default :
            usage(av[0]);
        }
    }

    Loglevel = AFP_LOG_INFO;

    if (List) {
        list_tests();
        exit(2);
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

    if (Test != NULL) {
        ret = run_selected(Test);

        if (ret) {
            return ret;
        }
    } else {
        ret = run_all();

        if (ret) {
            return ret;
        }
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
