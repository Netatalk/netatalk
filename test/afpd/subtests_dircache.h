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
extern int utest_dircache_kind_change_drops_ea(AFPObj *obj, struct vol *vol,
                                               uint16_t vid);
extern int utest_dircache_removed_entry_takes_nothing(AFPObj *obj,
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
extern int utest_dircache_healed_path_keeps_rows(AFPObj *obj,
                                                 struct vol *vol,
                                                 uint16_t vid);
extern int utest_dircache_stale_name_path_repaired(AFPObj *obj,
                                                   struct vol *vol,
                                                   uint16_t vid);
extern int utest_dircache_search_keeps_other_row(AFPObj *obj,
                                                 struct vol *vol,
                                                 uint16_t vid);
extern int utest_dircache_search_finds_moved_folder(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_stale_path_takes_no_metadata(AFPObj *obj,
                                                       struct vol *vol,
                                                       uint16_t vid);
extern int utest_dircache_replaced_folder_takes_row(AFPObj *obj,
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
extern int utest_dircache_replaced_folder_by_did(AFPObj *obj,
                                                 struct vol *vol,
                                                 uint16_t vid);
extern int utest_dircache_replaced_file_by_cnid(AFPObj *obj, struct vol *vol,
                                                uint16_t vid);
extern int utest_dircache_stat_denied_by_did(AFPObj *obj, struct vol *vol,
                                             uint16_t vid);
extern int utest_dircache_ids_under_denied_folder(AFPObj *obj, struct vol *vol,
                                                  uint16_t vid);
extern int utest_dircache_moved_folder_stat_denied(AFPObj *obj,
                                                   struct vol *vol,
                                                   uint16_t vid);
extern int utest_dircache_folder_became_file_by_did(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_moved_folder_file_at_name(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_ids_in_moved_denied_folder(AFPObj *obj,
                                                     struct vol *vol,
                                                     uint16_t vid);
extern int utest_dircache_child_of_replaced_folder(AFPObj *obj,
                                                   struct vol *vol,
                                                   uint16_t vid);
extern int utest_dircache_row_rewritten_after_resolve(AFPObj *obj,
                                                      struct vol *vol,
                                                      uint16_t vid);
extern int utest_dircache_replacement_keeps_its_row(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_replacement_keeps_its_cnid(AFPObj *obj,
                                                     struct vol *vol,
                                                     uint16_t vid);
extern int utest_dircache_row_rewritten_at_retirement(AFPObj *obj,
                                                      struct vol *vol,
                                                      uint16_t vid);
extern int utest_dircache_row_moved_at_retirement(AFPObj *obj,
                                                  struct vol *vol,
                                                  uint16_t vid);
extern int utest_dircache_retirement_backend_failure(AFPObj *obj,
                                                     struct vol *vol,
                                                     uint16_t vid);
extern int utest_dircache_moved_folder_ids_backend_failure(AFPObj *obj,
                                                           struct vol *vol,
                                                           uint16_t vid);
extern int utest_dircache_followed_link_replacement(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_link_row_rewritten_at_retirement(AFPObj *obj,
                                                           struct vol *vol,
                                                           uint16_t vid);
extern int utest_dircache_rewritten_link_unreadable(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_followed_link_unreadable(AFPObj *obj,
                                                   struct vol *vol,
                                                   uint16_t vid);
extern int utest_dircache_path_crosses_file_by_did(AFPObj *obj,
                                                   struct vol *vol,
                                                   uint16_t vid);
extern int utest_dircache_enumerate_vanished_folder(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_walk_after_self_heal(AFPObj *obj, struct vol *vol,
                                               uint16_t vid);
extern int utest_dircache_walk_skips_denied_folder(AFPObj *obj,
                                                   struct vol *vol,
                                                   uint16_t vid);
extern int utest_dircache_walk_backend_failure(AFPObj *obj, struct vol *vol,
                                               uint16_t vid);
extern int utest_dircache_walk_paused_folder_denied(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_walk_unentered_folder(AFPObj *obj, struct vol *vol,
                                                uint16_t vid);
extern int utest_dircache_resolveid_folder_a_file_took(AFPObj *obj,
                                                       struct vol *vol,
                                                       uint16_t vid);
extern int utest_dircache_deleteid_folder_a_file_took(AFPObj *obj,
                                                      struct vol *vol,
                                                      uint16_t vid);
extern int utest_dircache_resolveid_getcwd_denied(AFPObj *obj,
                                                  struct vol *vol,
                                                  uint16_t vid);
extern int utest_dircache_search_after_self_heal(AFPObj *obj,
                                                 struct vol *vol,
                                                 uint16_t vid);
extern int utest_dircache_forkparms_after_self_heal(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_followed_link_row(AFPObj *obj, struct vol *vol,
                                            uint16_t vid);
extern int utest_dircache_file_entry_not_entered(AFPObj *obj,
                                                 struct vol *vol,
                                                 uint16_t vid);
extern int utest_dircache_file_of_replaced_folder(AFPObj *obj,
                                                  struct vol *vol,
                                                  uint16_t vid);
extern int utest_dircache_folder_link_by_cnid(AFPObj *obj, struct vol *vol,
                                              uint16_t vid);
extern int utest_dircache_other_device_by_did(AFPObj *obj, struct vol *vol,
                                              uint16_t vid);
extern int utest_dircache_setfileparms_link_by_cnid(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);
extern int utest_dircache_openfork_by_cnid_other_user(AFPObj *obj,
                                                      struct vol *vol,
                                                      uint16_t vid);
extern int utest_dircache_exchange_by_cnid(AFPObj *obj, struct vol *vol,
                                           uint16_t vid);
extern int utest_dircache_file_stat_fails_by_cnid(AFPObj *obj,
                                                  struct vol *vol,
                                                  uint16_t vid);
extern int utest_dircache_createid_unenterable_folder(AFPObj *obj,
                                                      struct vol *vol,
                                                      uint16_t vid);
extern int utest_dircache_fifo_by_cnid(AFPObj *obj, struct vol *vol,
                                       uint16_t vid);
extern int utest_dircache_file_replaced_after_lookup(AFPObj *obj,
                                                     struct vol *vol,
                                                     uint16_t vid);
extern int utest_dircache_file_deleted_after_lookup(AFPObj *obj,
                                                    struct vol *vol,
                                                    uint16_t vid);

#endif /* SUBTESTS_DIRCACHE_H */
