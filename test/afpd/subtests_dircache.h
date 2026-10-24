/*!
 * @file
 * @brief Dircache test entry points: the stat a request refreshes an entry
 *        with, and objects replaced or gone behind the cache
 */

#ifndef SUBTESTS_DIRCACHE_H
#define SUBTESTS_DIRCACHE_H

#include <stdint.h>

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_dircache_step_up_keeps_entry(AFPObj *obj, struct vol *vol,
                                              uint16_t vid);
extern int utest_dircache_replaced_file_by_name(AFPObj *obj, struct vol *vol,
                                                uint16_t vid);
extern int utest_dircache_rekeyed_folder_children(AFPObj *obj,
                                                  struct vol *vol,
                                                  uint16_t vid);
extern int utest_dircache_replaced_curdir_rekeyed(AFPObj *obj,
                                                  struct vol *vol,
                                                  uint16_t vid);
extern int utest_dircache_replaced_folder_by_name(AFPObj *obj,
                                                  struct vol *vol,
                                                  uint16_t vid);
extern int utest_dircache_folder_became_file(AFPObj *obj, struct vol *vol,
                                             uint16_t vid);
extern int utest_dircache_working_folder_became_file(AFPObj *obj,
                                                     struct vol *vol,
                                                     uint16_t vid);
extern int utest_dircache_rowed_file_takes_folder_name(AFPObj *obj,
                                                       struct vol *vol,
                                                       uint16_t vid);
extern int utest_dircache_renamed_folder_keeps_row(AFPObj *obj,
                                                   struct vol *vol,
                                                   uint16_t vid);
extern int utest_dircache_unanswered_kind_keeps_row(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_curdir_never_file(AFPObj *obj, struct vol *vol,
                                            uint16_t vid);
extern int utest_dircache_file_became_folder(AFPObj *obj, struct vol *vol,
                                             uint16_t vid);
extern int utest_dircache_file_takes_folder_name(AFPObj *obj, struct vol *vol,
                                                 uint16_t vid);
extern int utest_dircache_replaced_folder_params_fail(AFPObj *obj,
                                                      struct vol *vol,
                                                      uint16_t vid);
extern int utest_dircache_replaced_access_fails(AFPObj *obj, struct vol *vol,
                                                uint16_t vid);
extern int utest_dircache_replaced_folder_create_fails(AFPObj *obj,
                                                       struct vol *vol,
                                                       uint16_t vid);
extern int utest_dircache_gone_folder_children(AFPObj *obj, struct vol *vol,
                                               uint16_t vid);
extern int utest_dircache_hint_gone_folder_children(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_replaced_folder_listing(AFPObj *obj,
                                                  struct vol *vol,
                                                  uint16_t vid);
extern int utest_dircache_stale_child_path_kept(AFPObj *obj, struct vol *vol,
                                                uint16_t vid);
extern int utest_dircache_moved_parent_keeps_rows(AFPObj *obj,
                                                  struct vol *vol,
                                                  uint16_t vid);
extern int utest_dircache_uncached_parent_keeps_rows(AFPObj *obj,
                                                     struct vol *vol,
                                                     uint16_t vid);
extern int utest_dircache_search_skips_moved_child(AFPObj *obj,
                                                   struct vol *vol,
                                                   uint16_t vid);
extern int utest_dircache_resolve_moved_child_fails(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_rekeyed_folder_recounts(AFPObj *obj,
                                                  struct vol *vol,
                                                  uint16_t vid);
extern int utest_dircache_inplace_refresh_children(AFPObj *obj,
                                                   struct vol *vol,
                                                   uint16_t vid);
extern int utest_dircache_held_add_fails(AFPObj *obj, struct vol *vol,
                                         uint16_t vid);
extern int utest_dircache_refresh_hints_siblings(AFPObj *obj,
                                                 struct vol *vol,
                                                 uint16_t vid);
extern int utest_dircache_orphan_parent_no_row(AFPObj *obj, struct vol *vol,
                                               uint16_t vid);
extern int utest_dircache_orphan_move_fails(AFPObj *obj, struct vol *vol,
                                            uint16_t vid);

#endif /* SUBTESTS_DIRCACHE_H */
