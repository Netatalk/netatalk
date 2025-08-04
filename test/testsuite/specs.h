/* -------------------------------------
*/

#ifndef SPECS_H
#define SPECS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <signal.h>

#include "afpclient.h"
#include "afphelper.h"
#include "test.h"

/* Defines */
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

/* Types */
enum adouble {
    AD_EA = 1,
    AD_V2 = 2
};

/* Globals */
extern uint16_t VolID;
extern int Mac;
extern int ExitCode;
extern enum adouble adouble;

/* Functions */
extern void illegal_fork(DSI *dsi, char cmd, char *name);
extern int no_access_folder(uint16_t vol, int did, char *name);
extern int read_only_folder(uint16_t vol, int did, char *name);
extern int delete_folder(uint16_t vol, int did, char *name);

extern int get_did(CONN *conn, uint16_t vol, int dir, char *name);
extern int get_fid(CONN *conn, uint16_t vol, int dir, char *name);
extern uint32_t get_forklen(DSI *dsi, int type);

extern void write_fork(CONN *conn, uint16_t vol, int dir, char *name,
                       char *data);
extern void read_fork(CONN *conn, uint16_t vol, int dir, char *name, int len);

extern int read_only_folder_with_file(uint16_t vol, int did, char *name,
                                      char *file);
extern int delete_folder_with_file(uint16_t vol, int did, char *name,
                                   char *file);
extern int get_vol_attrib(uint16_t vol) ;
extern int group_folder(uint16_t vol, int did, char *name);
extern unsigned int get_vol_free(uint16_t vol) ;
extern ssize_t get_sessiontoken(const char *buf, char **token);

extern void test_failed(void);
extern void test_skipped(int why);
extern void test_nottested(void);
extern void enter_test(void);
extern void exit_test(char *name);

extern int not_valid(unsigned int ret, int mac_error, int afpd_error);
extern int not_valid_bitmap(unsigned int ret, unsigned int bitmap,
                            int afpd_error);

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
#define T_LOCKING    9
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

/* ---------------------------------
*/

#endif
