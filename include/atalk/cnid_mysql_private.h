#ifndef _ATALK_CNID_MYSQL_PRIVATE_H
#define _ATALK_CNID_MYSQL_PRIVATE_H 1

#include <stdbool.h>

#include <atalk/cnid_private.h>
#include <atalk/uuid.h>

#define CNID_MYSQL_FLAG_DEPLETED (1 << 0) /*!< CNID set overflowed */
#define CNID_MYSQL_FLAG_HINT_RANGE_LOGGED (1 << 1) /*!< out-of-range hint warned once */
#define CNID_MYSQL_FLAG_NEAR_DEPLETION (1 << 2) /*!< approaching-ceiling warned once */

typedef struct CNID_mysql_private {
    struct vol *vol;
    uint32_t      cnid_mysql_flags;
    MYSQL        *cnid_mysql_con;
    char         *cnid_mysql_voluuid_str;
    cnid_t        cnid_mysql_hint;
    MYSQL_STMT   *cnid_lookup_stmt;
    MYSQL_STMT   *cnid_add_stmt;
    MYSQL_STMT   *cnid_put_stmt;
    MYSQL_STMT   *cnid_get_stmt;
    MYSQL_STMT   *cnid_delete_stmt;
    MYSQL_STMT   *cnid_resolve_stmt;
    MYSQL_STMT   *cnid_purge_stmt;
    /* Server lacks recursive-CTE support (pre-8.0 / MariaDB pre-10.2);
     * scoped finds fall back to unscoped with a one-time warning. */
    bool          cnid_find_scoped_unsupported;
} CNID_mysql_private;

#endif
