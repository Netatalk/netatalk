#ifndef AFP_HELPER_H
#define AFP_HELPER_H

#include <inttypes.h>

#include "dsi.h"

/* log level */
#define AFP_LOG_DEBUG    0
#define AFP_LOG_INFO     1
#define AFP_LOG_WARNING  2
#define AFP_LOG_ERROR    3
#define AFP_LOG_CRITICAL 4
#define AFP_LOG_MAX      5

extern void afp_printf(int level, int loglevel, int color, const char *fmt,
                       ...);

#define AFP_PRINTF(level, fmt, ...)  afp_printf(level, Loglevel, Color, fmt, ##__VA_ARGS__)

/* Forward declarations */
typedef struct CONN CONN;

/* Function declarations */
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
extern int get_vol_attrib(uint16_t vol);
extern int group_folder(uint16_t vol, int did, char *name);
extern unsigned int get_vol_free(uint16_t vol);
extern int not_valid(unsigned int ret, int mac_error, int afpd_error);
extern int not_valid_bitmap(unsigned int ret, unsigned int bitmap,
                            int afpd_error);
extern int32_t is_there(CONN *conn, uint16_t volume, int32_t did, char *name);
extern int delete_directory_tree(CONN *conn, uint16_t volume,
                                 uint32_t parent_did, char *dirname);
extern void clear_volume(uint16_t vol, CONN *conn);

#endif
