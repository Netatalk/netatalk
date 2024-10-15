/* -------------------------------------
*/

#ifndef SPECS_H
#define SPECS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>  

#include "afpclient.h"
#include "afphelper.h"
#include "test.h"

/* Defines */
#if 0
#define FAIL(a) if ((a)) { failed();}
#else
#define FAIL(a) \
    if ((a)) { \
        if (Color) { \
            fprintf(stdout, ANSI_BRED"[%s:%d] " #a "\n" ANSI_NORMAL, __FILE__, __LINE__);\
        } else {\
            fprintf(stdout, "[%s:%d] " #a "\n", __FILE__, __LINE__); \
        } \
        failed_nomsg(); \
    }

#endif
#define EXPECT_FAIL(a, b) do { int _experr;  _experr = (a); if (htonl(_experr) != (b)) { failed(); } } while(0);
#define FAILEXIT(a, label) if ((a)) { failed(); goto label;}
#define STATIC 

/* Types */
enum adouble {
    AD_EA = 1,
    AD_V2 = 2
};

/* Globals */
extern uint16_t VolID;
extern int Mac;
extern int ExitCode;
extern int Exclude;
extern enum adouble adouble;

/* Functions */
extern void illegal_fork(DSI * dsi, char cmd, char *name);
extern int no_access_folder(uint16_t vol, int did, char *name);
extern int read_only_folder(uint16_t vol, int did, char *name);
extern int delete_folder(uint16_t vol, int did, char *name);

extern int get_did(CONN *conn, uint16_t vol, int dir, char *name);
extern int get_fid(CONN *conn, uint16_t vol, int dir, char *name);
extern uint32_t get_forklen(DSI *dsi, int type);

extern void write_fork(CONN *conn, uint16_t vol,int dir, char *name, char *data);
extern void read_fork(CONN *conn, uint16_t vol,int dir, char *name,int len);

extern int read_only_folder_with_file(uint16_t vol, int did, char *name, char *file);
extern int delete_folder_with_file(uint16_t vol, int did, char *name, char *file);
extern int get_vol_attrib(uint16_t vol) ;
extern int group_folder(uint16_t vol, int did, char *name);
extern unsigned int get_vol_free(uint16_t vol) ;
extern ssize_t get_sessiontoken(const char *buf, char **token);

extern void failed_nomsg(void);
extern void skipped_nomsg(void);
extern void nottested_nomsg(void);
extern void failed(void);
extern void known_failure(char *why);
extern void enter_test(void);
extern void exit_test(char *name);

extern void nottested(void);
extern int not_valid(unsigned int ret, int mac_error, int afpd_error);
extern int not_valid_bitmap(unsigned int ret, unsigned int bitmap, int afpd_error);
extern void test_skipped(int why);

#define T_CONN2      1
#define T_PATH       2
#define T_AFP3       3
#define T_AFP3_CONN2 4
#define T_MAC_PATH   5
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

/* ---------------------------------
*/

#endif
