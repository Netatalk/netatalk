#ifndef AFP_HELPER_H
#define AFP_HELPER_H

#include <inttypes.h>

/* define ansi color */
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
extern int delete_directory_tree(CONN *conn, uint16_t volume,
                                 uint32_t parent_did, char *dirname);

#endif
