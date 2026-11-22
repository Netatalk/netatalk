/*!
 * @file
 * @brief Private SQLite CNID backend state and status flags
 */

#ifndef _ATALK_CNID_SQLITE_PRIVATE_H
#define _ATALK_CNID_SQLITE_PRIVATE_H 1

#include <atalk/cnid.h>
#include <atalk/uuid.h>

#define CNID_SQLITE_FLAG_DEPLETED (1 << 0) /*!< CNID set overflowed */
#define CNID_SQLITE_FLAG_NEAR_DEPLETION (1 << 1) /*!< depletion warning logged */
#define CNID_SQLITE_FLAG_HINT_RANGE_LOGGED (1 << 2) /*!< hint range warning logged */

typedef struct CNID_sqlite_private {
    struct vol *vol;
    uint32_t cnid_sqlite_flags;
    sqlite3 *cnid_sqlite_con;
    char *cnid_sqlite_voluuid_str;
    cnid_t cnid_sqlite_hint;
    sqlite3_stmt *cnid_lookup_stmt;
    sqlite3_stmt *cnid_add_stmt;
    sqlite3_stmt *cnid_put_stmt;
    sqlite3_stmt *cnid_get_stmt;
    sqlite3_stmt *cnid_resolve_stmt;
    sqlite3_stmt *cnid_delete_stmt;
    sqlite3_stmt *cnid_getstamp_stmt;
    sqlite3_stmt *cnid_find_stmt;
    sqlite3_stmt *cnid_find_scoped_stmt;
    sqlite3_stmt *cnid_update_stmt;
    sqlite3_stmt *cnid_del_didname_stmt;
    sqlite3_stmt *cnid_del_devino_stmt;
    sqlite3_stmt *cnid_dirlist_stmt;
    sqlite3_stmt *cnid_delete_row_stmt;
    sqlite3_stmt *cnid_update_row_stmt;
    sqlite3_stmt *cnid_del_devino_other_stmt;
    sqlite3_stmt *cnid_check_row_stmt;
    /*! Name index searches by seed count: [0] NULL without an index */
    sqlite3_stmt *cnid_find_index_stmt[CNID_FIND_MAX_SEEDS];
    /*! The same for folder searches */
    sqlite3_stmt *cnid_find_scoped_index_stmt[CNID_FIND_MAX_SEEDS];
    sqlite3_stmt *cnid_find_candidates_stmt;
    unsigned int cnid_sqlite_changes;   /*!< changes logged and writes since,
                                             toward CNID_SQLITE_INDEX_BATCH */
} CNID_sqlite_private;

#endif
