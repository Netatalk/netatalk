#ifndef _ATALK_CNID_SQLITE_PRIVATE_H
#define _ATALK_CNID_SQLITE_PRIVATE_H 1

#include <atalk/cnid_private.h>
#include <atalk/uuid.h>

/* CNID set overflowed */
#define CNID_SQLITE_FLAG_DEPLETED (1 << 0)

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
} CNID_sqlite_private;

#endif
