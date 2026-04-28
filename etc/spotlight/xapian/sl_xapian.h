/*
 * C wrapper boundary for the Spotlight Xapian backend.
 *
 * The implementation is C++ because libxapian exposes a C++ API.  All
 * functions here catch C++ exceptions before returning to C callers.
 */

#ifndef SL_XAPIAN_H
#define SL_XAPIAN_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

int sl_xapian_reconcile(const char *db_path,
                        const char *volume_path,
                        const char *volume_uuid,
                        char *errbuf,
                        size_t errlen);
int sl_xapian_index_ready(const char *db_path,
                          const char *volume_path,
                          const char *volume_uuid,
                          char *errbuf,
                          size_t errlen);
int sl_xapian_query(const char *db_path,
                    const char *scope,
                    const char *qstring,
                    size_t offset,
                    size_t limit,
                    char ***paths,
                    size_t *count,
                    int *more,
                    char *errbuf,
                    size_t errlen);
int sl_xapian_upsert_path(const char *db_path,
                          const char *path,
                          char *errbuf,
                          size_t errlen);
int sl_xapian_delete_path(const char *db_path,
                          const char *path,
                          char *errbuf,
                          size_t errlen);
int sl_xapian_delete_prefix(const char *db_path,
                            const char *path,
                            char *errbuf,
                            size_t errlen);
int sl_xapian_reindex_subtree(const char *db_path,
                              const char *volume_path,
                              const char *path,
                              const char *oldpath,
                              char *errbuf,
                              size_t errlen);
int sl_xapian_mark_dirty(const char *db_path,
                         char *errbuf,
                         size_t errlen);
void sl_xapian_free_paths(char **paths, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* SL_XAPIAN_H */
