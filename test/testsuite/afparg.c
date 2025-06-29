#include "specs.h"
#include <dlfcn.h>
#include <getopt.h>

#define FN(a) a ## _arg
#define EXT_FN(a) extern void FN(a) (char **argv)

EXT_FN(FPResolveID);
EXT_FN(FPEnumerate);
EXT_FN(FPCopyFile);
EXT_FN(FPLockrw);
EXT_FN(FPLockw);

struct test_fn {
    char *name;
    void (*fn)(char **);
    char *helptext;
};
#define FN_N(a,b) { # a, FN(a) , # a " " # b},

static struct test_fn Test_list[] = {
    FN_N(FPResolveID, CNID)
    FN_N(FPEnumerate, dir)
    FN_N(FPCopyFile, source dest)
    FN_N(FPLockrw, d | r file)
    FN_N(FPLockw, d | r file)

    {NULL, NULL},
};

/* Some globals */
int        ExitCode = 0;
uint16_t   VolID;
static DSI *dsi;
CONN       *Conn;
DSI        *Dsi;

/* Default values for options */
char    *Server = "localhost";
int     Port = DSI_AFPOVERTCP_PORT;
char    *Password = "";
char    *Vol = "";
char    *User;
int     Version = 34;
int     List = 0;
char    *Test = "";

char *vers = "AFP3.4";
char *uam = "Cleartxt Passwrd";

/* Unused but required in afphelper.c. Argh. */
CONN       *Conn2;
int        Mac = 0;
char       Data[1] = "";
int PassCount = 0;
int FailCount = 0;
int SkipCount = 0;
int NotTestedCount = 0;
char FailedTests[1024][256] = {{0}};
char NotTestedTests[1024][256] = {{0}};
char SkippedTests[1024][256] = {{0}};

/* =============================== */
static void list_tests(void)
{
    int i = 0;

    while (Test_list[i].name != NULL) {
        fprintf(stdout, "%s\n", Test_list[i].helptext);
        i++;
    }
}

/* ----------- */
static void run_one(char *name, char **args)
{
    int i = 0;
    void *handle = NULL;
    void (*fn)(char **) = NULL;
    char *error;
    char *token;

    while (Test_list[i].name != NULL) {
        if (!strcmp(Test_list[i].name, name)) {
            break;
        }

        i++;
    }

    if (Test_list[i].name == NULL) {
        test_nottested();
        return;
    }

    fn = Test_list[i].fn;
    dsi = &Conn->dsi;
    VolID = FPOpenVol(Conn, Vol);

    if (VolID == 0xffff) {
        test_nottested();
        return;
    }

    (*fn)(args);
    FPCloseVol(Conn, VolID);
}

/* =============================== */
void usage(char *av0)
{
    fprintf(stdout,
            "usage:\t%s [-1234567lVv] [-h host] [-p port] [-s vol] [-u user] [-w password] [-f command args]\n",
            av0);
    fprintf(stdout, "\t-h\tserver host name (default localhost)\n");
    fprintf(stdout, "\t-p\tserver port (default 548)\n");
    fprintf(stdout, "\t-s\tvolume to mount\n");
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
    fprintf(stdout, "\t-f\ttest to run\n");
    fprintf(stdout, "\t-l\tlist tests\n");
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

    while ((cc = getopt(ac, av, "1234567lVvf:h:p:s:u:w:")) != EOF) {
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

        case 'f' :
            Test = strdup(optarg);
            break;

        case 'h':
            Server = strdup(optarg);
            break;

        case 'l' :
            List = 1;
            break;

        case 'p' :
            Port = atoi(optarg);

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

    if (User != NULL && User[0] == '\0') {
        fprintf(stdout, "Error: Define a user with -u\n");
    }

    if (Password != NULL && Password[0] == '\0') {
        fprintf(stdout, "Error: Define a password with -w\n");
    }

    if (Vol != NULL && Vol[0] == '\0') {
        fprintf(stdout, "Error: Define a volume with -s\n");
    }

    if (Test != NULL && Test[0] == '\0') {
        fprintf(stdout, "Error: Define an AFP command with -f\n");
        fprintf(stdout, "Available commands:\n");
        list_tests();
        exit(1);
    }

    if ((Conn = (CONN *)calloc(1, sizeof(CONN))) == NULL) {
        return 1;
    }

    int sock;
    Dsi = &Conn->dsi;
    dsi = Dsi;
    sock = OpenClientSocket(Server, Port);

    if (sock < 0) {
        return 2;
    }

    Dsi->socket = sock;

    /* login */
    if (Version >= 30) {
        ret = FPopenLoginExt(Conn, vers, uam, User, Password);
    } else {
        ret = FPopenLogin(Conn, vers, uam, User, Password);
    }

    if (ret) {
        printf("Login failed\n");
        exit(1);
    }

    Conn->afp_version = Version;
    run_one(Test, &av[optind]);
    FPLogOut(Conn);
    return ExitCode;
}
