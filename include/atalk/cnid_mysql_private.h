#ifndef _ATALK_CNID_MYSQL_PRIVATE_H
#define _ATALK_CNID_MYSQL_PRIVATE_H 1

#include <atalk/cnid_private.h>
#include <atalk/uuid.h>

#define CNID_MYSQL_FLAG_DEPLETED (1 << 0) /* CNID set overflowed */

typedef struct CNID_mysql_private {
    struct vol *vol;
    uint32_t      cnid_mysql_flags;
    MYSQL        *cnid_mysql_con;
    char         *cnid_mysql_voluuid_str;
    cnid_t        cnid_mysql_hint;
    MYSQL_STMT   *cnid_lookup_stmt;
    MYSQL_STMT   *cnid_add_stmt;
    MYSQL_STMT   *cnid_put_stmt;
} CNID_mysql_private;

#endif
