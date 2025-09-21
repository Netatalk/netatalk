/* ----------------------------------------------
*/

#include "specs.h"

static int CurTestResult;
static char *Why;
#define SKIPPED_MSG_BUFSIZE 256
static char skipped_msg_buf[SKIPPED_MSG_BUFSIZE];

/* ------------------------- */
void test_skipped(int why)
{
    char *s;

    switch (why) {
    case T_CONN2:
        s = "second user";
        break;

    case T_PATH:
        s = "has to be run on localhost with -c volume path";
        break;

    case T_AFP2:
        s = "AFP 2.x";
        break;

    case T_AFP3:
        s = "AFP 3.x";
        break;

    case T_AFP3_CONN2:
        s = "AFP 3.x and no second user";
        break;

    case T_AFP30:
        s = "AFP 3.0 only";
        break;

    case T_AFP31:
        s = "AFP 3.1 or higher";
        break;

    case T_AFP32:
        s = "AFP 3.2 or higher";
        break;

    case T_UNIX_PREV:
        s = "volume with UNIX privileges";
        break;

    case T_NO_UNIX_PREV:
        s = "volume without UNIX privileges";
        break;

    case T_UNIX_GROUP:
        s = "server and client same groups mapping";
        break;

    case T_UTF8:
        s = "volume with UTF8 encoding";
        break;

    case T_VOL2:
        s = "a second volume";
        break;

    case T_VOL_SMALL:
        s = "a bigger volume";
        break;

    case T_ID:
        s = "AFP FileID calls";
        break;

    case T_MAC:
        s = "needs Mac OS AFP server";
        break;

    case T_ACL:
        s = "volume with ACL support";
        break;

    case T_EA:
        s = "volume with filesystem Extended Attributes support";
        break;

    case T_ADEA:
        s = "volume must not use AppleDouble metadata";
        break;

    case T_ADV2:
        s = "volume must use AppleDouble metadata";
        break;

    case T_NOSYML:
        s = "volume without option 'follow symlinks'";
        break;

    case T_SINGLE:
        s = "run individually with -f";
        break;

    case T_VOL_BIG:
        s = "a smaller volume";
        break;

    case T_MANUAL:
        s = "Interactive mode";
        break;

    case T_NONDETERM:
        s = "nondeterministic behavior";
        break;

    case T_BIGENDIAN:
        s = "not big-endian compatible";
        break;

    case T_CRED:
        s = "username/password for the AFP server";
        break;

    default:
        s = "UNKNOWN REASON - this is a bug";
        break;
    }

    if (Color) {
        snprintf(skipped_msg_buf, sizeof(skipped_msg_buf),
                 ANSI_BBLUE "SKIPPED (%s)" ANSI_NORMAL, s);
    } else {
        snprintf(skipped_msg_buf, sizeof(skipped_msg_buf), "SKIPPED (%s)", s);
    }

    CurTestResult = 3;
}

/* ------------------------- */
void test_failed(void)
{
    if (!Quiet) {
        fprintf(stderr, "\tFAILED\n");
    }

    ExitCode = 1;
    CurTestResult = 1;
}

/* ------------------------- */
void test_nottested(void)
{
    if (!Quiet) {
        fprintf(stderr, "\tNOT TESTED\n");
    }

    if (!ExitCode) {
        ExitCode = 2;
    }

    CurTestResult = 2;
}

/* ------------------------- */
void enter_test(void)
{
    CurTestResult = 0;
    Why = "";
}

/* ------------------------- */
void exit_test(char *name)
{
    char *s;

    switch (CurTestResult) {
    case 0:
        PassCount++;

        if (Color) {
            s = ANSI_BGREEN "PASSED" ANSI_NORMAL;
        } else {
            s = "PASSED";
        }

        break;

    case 1:
        strlcpy(FailedTests[FailCount], name, 255);
        FailedTests[FailCount][255] = '\0';
        FailCount++;

        if (Color) {
            s = ANSI_BRED "FAILED" ANSI_NORMAL;
        } else {
            s = "FAILED";
        }

        fprintf(stdout, "%s - ", name);
        fprintf(stdout, "%s%s\n", s, Why);
        fflush(stdout);
        return;

    case 2:
        strlcpy(NotTestedTests[NotTestedCount], name, 255);
        NotTestedTests[NotTestedCount][255] = '\0';
        NotTestedCount++;

        if (Color) {
            s = ANSI_BYELLOW "NOT TESTED" ANSI_NORMAL;
        } else {
            s = "NOT TESTED";
        }

        break;

    case 3:
        strlcpy(SkippedTests[SkipCount], name, 255);
        SkippedTests[SkipCount][255] = '\0';
        SkipCount++;
        s = skipped_msg_buf;
        break;

    default:
        fprintf(stdout, "%s - ", name);
        fprintf(stdout, "UNKNOWN RESULT (%d)%s\n", CurTestResult, Why);
        fflush(stdout);
        return;
    }

    fprintf(stdout, "%s - ", name);
    fprintf(stdout, "%s%s\n", s, Why);
    fflush(stdout);
}
