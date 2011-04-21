#ifndef CMD_DBD_H
#define CMD_DBD_H

#include <signal.h>
#include <limits.h>

#include <atalk/volinfo.h>
#include "dbif.h"

enum logtype {LOGSTD, LOGDEBUG};
typedef unsigned int dbd_flags_t;

#define DBD_FLAGS_SCAN     (1 << 0)
#define DBD_FLAGS_FORCE    (1 << 1)
#define DBD_FLAGS_EXCL     (1 << 2)
#define DBD_FLAGS_CLEANUP  (1 << 3) /* Dont create AD stuff, but cleanup orphaned */
#define DBD_FLAGS_STATS    (1 << 4)

#define ADv2_DIRNAME ".AppleDouble"

#define DIR_DOT_OR_DOTDOT(a) \
        ((strcmp(a, ".") == 0) || (strcmp(a, "..") == 0))

#define STRCMP(a,b,c) \
        (strcmp(a,c) b 0)

extern int nocniddb; /* Dont open CNID database, only scan filesystem */
extern int db_locked; /* have we got the fcntl lock on lockfd ? */
extern volatile sig_atomic_t alarmed;

extern void dbd_log(enum logtype lt, char *fmt, ...);
extern int cmd_dbd_scanvol(DBD *dbd, struct volinfo *volinfo, dbd_flags_t flags);

/*
  Functions for querying the database which couldn't be reused from the existing
  funcs pool of dbd_* for one reason or another
*/
extern int cmd_dbd_add(DBD *dbd, struct cnid_dbd_rqst *rqst, struct cnid_dbd_rply *rply);
#endif /* CMD_DBD_H */
