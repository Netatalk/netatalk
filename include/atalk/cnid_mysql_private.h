/*!
 * @file
 * @brief Private MySQL CNID backend state and status flags
 */

#ifndef _ATALK_CNID_MYSQL_PRIVATE_H
#define _ATALK_CNID_MYSQL_PRIVATE_H 1

#include <stdbool.h>

#include <atalk/uuid.h>

#define CNID_MYSQL_FLAG_DEPLETED (1 << 0) /*!< CNID set overflowed */
#define CNID_MYSQL_FLAG_HINT_RANGE_LOGGED (1 << 1) /*!< out-of-range hint warned once */
#define CNID_MYSQL_FLAG_NEAR_DEPLETION (1 << 2) /*!< approaching-ceiling warned once */

/*! How a conditional update holds its row */
enum cnid_mysql_hold_kind {
    CNID_MYSQL_HOLD_ROW,   /*!< an InnoDB transaction locks the row */
    CNID_MYSQL_HOLD_TABLE, /*!< a table lock, on any other engine */
    CNID_MYSQL_HOLD_NONE   /*!< none, where the user may not lock tables */
};

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
    MYSQL_STMT   *cnid_dirlist_stmt;
    MYSQL_STMT   *cnid_delete_row_stmt;
    MYSQL_STMT   *cnid_update_row_stmt;
    MYSQL_STMT   *cnid_del_devino_other_stmt;
    MYSQL_STMT   *cnid_check_row_stmt;
    /* Server lacks recursive-CTE support (pre-8.0 / MariaDB pre-10.2);
     * scoped finds fall back to unscoped with a one-time warning. */
    bool          cnid_find_scoped_unsupported;
    enum cnid_mysql_hold_kind cnid_mysql_hold_kind;
    /* Thread the statements are prepared on; a reconnected session has none */
    unsigned long cnid_mysql_stmt_thread;
    /* SQL comparing Name byte for byte, where the results charset allows */
    char          cnid_mysql_name_is[80];
} CNID_mysql_private;

#endif
