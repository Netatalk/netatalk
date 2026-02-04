/* -------------------------------- */
#ifndef TESTHELPER_H
#define TESTHELPER_H

/* Macros */
#define FAIL(a) \
    if ((a)) { \
        if (!Quiet) { \
            if (Color) { \
                fprintf(stdout, ANSI_BRED"[%s:%d] " #a "\n" ANSI_NORMAL, __FILE__, __LINE__);\
            } else {\
                fprintf(stdout, "[%s:%d] " #a "\n", __FILE__, __LINE__); \
            } \
        } \
        test_failed(); \
    }

#define FAILEXIT(a, label) if ((a)) { test_failed(); goto label;}
#define STATIC

#define ENTER_TESTSET \
    fprintf(stdout,"===================\n"); \
    fprintf(stdout,"Executing testset: %s\n", __func__); \

#define ENTER_TEST \
    if (!Quiet) { \
            fprintf(stdout, "############## entering %s ##############\n", __func__); \
        } \
    enter_test(); \

/* Skipped test status codes */
#define T_CONN2      1
#define T_PATH       2
#define T_AFP3       3
#define T_AFP3_CONN2 4
#if 0
#define T_MAC_PATH   5
#endif
#define T_UNIX_PREV  6
#define T_UTF8       7
#define T_VOL2       8
#if 0
#define T_LOCKING    9
#endif
#define T_VOL_SMALL  10
#define T_ID         11
#define T_AFP2       12
#define T_MAC        13
#define T_ACL        14
#define T_EA         15
#define T_UNIX_GROUP 16
#define T_ADEA       17
#define T_NOSYML     18
#define T_ADV2       19
#define T_AFP30      20
#define T_NO_UNIX_PREV 21
#define T_SINGLE     22
#define T_VOL_BIG    23
#if 0
#define T_EXCLUDE    24
#endif
#define T_MANUAL     25
#define T_AFP31      26
#define T_AFP32      27
#define T_NONDETERM  28
#define T_BIGENDIAN  29
#define T_CRED       30
#define T_LOCALHOST  31

/* Define ansi colors */
#define ANSI_RED      "\033[0;31m"
#define ANSI_GREEN    "\033[0;32m"
#define ANSI_YELLOW   "\033[0;33m"
#define ANSI_BLUE     "\033[0;34m"
#define ANSI_MAGENTA  "\033[0;35m"
#define ANSI_CYAN     "\033[0;36m"
#define ANSI_GREY     "\033[0;37m"
#define ANSI_DARKGREY "\033[01;30m"
#define ANSI_BRED     "\033[01;31m"
#define ANSI_BGREEN   "\033[01;32m"
#define ANSI_BYELLOW  "\033[01;33m"
#define ANSI_BBLUE    "\033[01;34m"
#define ANSI_BMAGENTA "\033[01;35m"
#define ANSI_BCYAN    "\033[01;36m"
#define ANSI_WHITE    "\033[01;37m"
#define ANSI_NORMAL   "\033[0m"
#define ANSI_BOLD     "\033[1m"

/* Function declarations */
extern void test_failed_at(const char *file, int line);
extern void test_skipped(int why);
extern void test_nottested(void);
extern void enter_test(void);
extern void exit_test(char *name);

/* Wrapper macro for automatic file/line tracking */
#define test_failed() test_failed_at(__FILE__, __LINE__)

/* Types */
enum adouble {
    AD_EA = 1,
    AD_V2 = 2
};

/* Global variables */
extern uint16_t VolID;
extern int Mac;
extern int ExitCode;
extern enum adouble adouble;

extern char Data[];
extern char FailedTests[1024][256];
extern char NotTestedTests[1024][256];
extern char SkippedTests[1024][256];

extern char *Vol;
extern char *Vol2;
extern char *Path;
extern char *User;
extern char *Test;
extern char *Server;

extern int Version;
extern int Verbose;
extern int Quiet;
extern int Locking;
extern int Loglevel;
extern int Color;
extern int Interactive;
extern int Throttle;
extern int Bigendian;
extern int EmptyVol;

extern int PassCount;
extern int FailCount;
extern int SkipCount;
extern int NotTestedCount;

#endif
