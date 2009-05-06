#ifndef CMD_DBD_H
#define CMD_DBD_H

#include <atalk/volinfo.h>
#include "dbif.h"

enum logtype {LOGSTD, LOGDEBUG};
typedef unsigned int dbd_flags_t;

#define DBD_FLAGS_SCAN     (1 << 0)
#define DBD_FLAGS_FORCE    (1 << 0)

#define ADv2_DIRNAME ".AppleDouble"

#define DIR_DOT_OR_DOTDOT(a) \
        ((strcmp(a, ".") == 0) || (strcmp(a, "..") == 0))

#define STRCMP(a,b,c) \
        (strcmp(a,c) b 0)

extern void dbd_log(enum logtype lt, char *fmt, ...);
extern int cmd_dbd_scanvol(DBD *dbd, struct volinfo *volinfo, dbd_flags_t flags);

#endif /* CMD_DBD_H */
