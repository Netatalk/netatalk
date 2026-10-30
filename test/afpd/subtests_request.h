/*!
 * @file
 * @brief Request path component test entry points
 */

#ifndef SUBTESTS_REQUEST_H
#define SUBTESTS_REQUEST_H

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_write_reads_size_only_when_used(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_hard_create_refused_under_peer_open(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_hard_create_refuses_racing_open(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_hard_create_keeps_data_without_metadata(AFPObj *obj,
                                                         struct vol *vol);
extern int utest_hard_create_v2_header_starts_over(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_hard_create_empties_late_file(AFPObj *obj, struct vol *vol);
extern int utest_hard_create_refuses_held_late_file(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_create_drops_stale_metadata(AFPObj *obj, struct vol *vol);
extern int utest_hard_create_starts_file_over(AFPObj *obj,
                                              struct vol *vol);
extern int utest_create_drops_stale_sidecar(AFPObj *obj, struct vol *vol);
extern int utest_create_refused_by_sidecar_folder(AFPObj *obj, struct vol *vol);
extern int utest_create_long_name(AFPObj *obj, struct vol *vol);
extern int utest_hard_create_refused_under_sidecar_lock(AFPObj *obj,
                                                        struct vol *vol);
extern int utest_hard_create_refused_unread_sidecar(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_ea_trunc_open_reads_header(AFPObj *obj, struct vol *vol);
extern int utest_soft_create_over_symlink_exists(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_hard_create_refreshes_entry(AFPObj *obj, struct vol *vol);
extern int utest_symlink_refusal_keeps_locks(AFPObj *obj, struct vol *vol);
extern int utest_setfileparms_held_symlink_kept(AFPObj *obj, struct vol *vol);
extern int utest_held_conversion_keeps_metadata(AFPObj *obj, struct vol *vol);
extern int utest_symlink_conversion_refused_under_deny(AFPObj *obj,
                                                       struct vol *vol);
extern int utest_setfileparms_cnid_failure_closes(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_setfileparms_readonly_fork_refused(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_copyfile_readonly_source(AFPObj *obj, struct vol *vol);
extern int utest_openfork_rsrc_beside_readonly(AFPObj *obj, struct vol *vol);
extern int utest_icon_write_inhibited_readonly_lender(AFPObj *obj,
                                                      struct vol *vol);
extern int utest_setfileparms_native_finderinfo_kept(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_osx_conversion_keeps_locks(AFPObj *obj, struct vol *vol);
extern int utest_osx_conversion_by_path(AFPObj *obj, struct vol *vol);
extern int utest_osx_conversion_readonly_kept(AFPObj *obj, struct vol *vol);
extern int utest_osx_conversion_readonly_with_metadata(AFPObj *obj,
                                                       struct vol *vol);
extern int utest_osx_conversion_of_symlink(AFPObj *obj, struct vol *vol);
extern int utest_osx_conversion_failed_write_retried(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_osx_conversion_repairs_metadata(AFPObj *obj, struct vol *vol);
extern int utest_osx_conversion_readonly_volume(AFPObj *obj, struct vol *vol);
extern int utest_osx_conversion_header_has_cnid(AFPObj *obj, struct vol *vol);
extern int utest_readonly_volume_open_adds_no_cnid(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_exchange_held_file_keeps_locks(AFPObj *obj, struct vol *vol);
extern int utest_exchange_bare_held_destination(AFPObj *obj, struct vol *vol);
extern int utest_exchange_held_symlink_kept(AFPObj *obj, struct vol *vol);
extern int utest_exchange_refused_header_has_cnid(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_exchange_osx_sidecar_finderinfo(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_exchange_hard_link_keeps_rlen(AFPObj *obj, struct vol *vol);
extern int utest_exchange_refused_sidecar_header(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_exchange_bare_hard_link_keeps_rlen(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_exchange_failure_restores_cnids(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_exchange_failed_rename_leaves_no_temp(AFPObj *obj,
                                                       struct vol *vol);
extern int utest_exchange_needs_read_access(AFPObj *obj, struct vol *vol);
extern int utest_exchange_v2_names_kept(AFPObj *obj, struct vol *vol);
extern int utest_sidecar_names_refused(AFPObj *obj, struct vol *vol);
extern int utest_listing_keeps_sidecar_locks(AFPObj *obj, struct vol *vol);
extern int utest_listing_passes_fifo(AFPObj *obj, struct vol *vol);
extern int utest_v2_bare_rsrc_open_takes_band(AFPObj *obj, struct vol *vol);
extern int utest_virtual_icon_write_takes_deny_lock(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_exchange_failed_undo_keeps_files(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_setfileparms_symlink_dated_itself(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_volume_scan_counts_no_band(AFPObj *obj, struct vol *vol);
extern int utest_volume_scan_resets_appended(AFPObj *obj, struct vol *vol);
extern int utest_rfork_cache_hit_sends_cached_bytes(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_rfork_cache_stale_length_reads_fork(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_rfork_cache_finds_name_on_disk(AFPObj *obj,
                                                struct vol *vol);
extern int utest_rfork_cache_checks_identity(AFPObj *obj, struct vol *vol);
extern int utest_hint_relay_drops_repeats(AFPObj *obj, struct vol *vol);
extern int utest_fork_first_change_hints_siblings(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_rfork_cache_drops_every_link_name(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_rfork_cache_skips_fill_while_written(AFPObj *obj,
                                                      struct vol *vol);
extern int utest_rfork_cache_skips_symlink_fill(AFPObj *obj, struct vol *vol);
extern int utest_rfork_cache_lookup_only_inside_fork(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_fce_ignore_directories(AFPObj *obj, struct vol *vol);
extern int utest_fce_clock_read_only_when_held(AFPObj *obj,
                                               struct vol *vol);
extern int utest_virtual_icon_write_takes_open_lock(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_virtual_icon_refused_write_stays_virtual(AFPObj *obj,
                                                          struct vol *vol);
extern int utest_virtual_icon_second_write_keeps_locks(AFPObj *obj,
                                                       struct vol *vol);
extern int utest_refused_open_keeps_sibling_fork(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_reader_close_keeps_writer(AFPObj *obj, struct vol *vol);
extern int utest_emptied_rfork_close_spares_new_file(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_emptied_rfork_close_keeps_sidecar(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_symlink_rfork_close_keeps_cnids(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_emptied_icon_close_deletes_file(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_emptied_icon_close_keeps_data(AFPObj *obj, struct vol *vol);
extern int utest_emptied_icon_close_keeps_symlink(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_emptied_icon_close_spares_new_file(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_ea_deletefile_returns_past_stuck_ea(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_set_ea_failed_close_replies_misc(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_virtual_icon_write_only_open_refuses_read(AFPObj *obj,
                                                           struct vol *vol);
extern int utest_virtual_icon_params_name_its_fork(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_icon_create_refused_replies_access(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_emptied_rfork_close_keeps_hard_link(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_emptied_icon_close_keeps_unwritten(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_emptied_icon_close_keeps_link_icon(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_emptied_icon_close_keeps_claimed(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_symlink_emptied_rfork_keeps_sidecar(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_icon_seed_refused_stays_virtual(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_icon_lock_error_logged(AFPObj *obj, struct vol *vol);
extern int utest_icon_cleanup_spares_swapped_file(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_virtual_icon_write_inhibited_stays_virtual(AFPObj *obj,
                                                            struct vol *vol);
extern int utest_icon_refused_borrow_keeps_length(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_delete_sweep_sends_no_refresh(AFPObj *obj, struct vol *vol);
extern int utest_emptied_icon_close_sees_peer_read_only(AFPObj *obj,
                                                        struct vol *vol);
extern int utest_icon_write_refuses_readonly_lender(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_icon_write_refuses_symlink(AFPObj *obj, struct vol *vol);
extern int utest_emptied_icon_close_after_sibling_cut(AFPObj *obj,
                                                      struct vol *vol);
extern int utest_rfork_cache_fills_by_own_link(AFPObj *obj, struct vol *vol);
extern int utest_rfork_without_sidecar_reads_eof(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_first_change_hint_spares_new_file(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_rfork_close_refreshes_by_own_link(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_rename_moves_open_forks(AFPObj *obj, struct vol *vol);
extern int utest_rename_keeps_link_forks_name(AFPObj *obj, struct vol *vol);
extern int utest_rename_takes_stale_fork_name(AFPObj *obj, struct vol *vol);
extern int utest_write_counts_bytes_appended(AFPObj *obj, struct vol *vol);
extern int utest_findnameat_follows_as_volume(AFPObj *obj, struct vol *vol);
extern int utest_refused_delete_keeps_open_lock(AFPObj *obj,
                                                struct vol *vol);
extern int utest_refused_delete_keeps_rfork_length(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_rename_symlink_keeps_cnids(AFPObj *obj, struct vol *vol);
extern int utest_emptied_icon_close_spares_swapped_file(AFPObj *obj,
                                                        struct vol *vol);
extern int utest_rename_keeps_symlink_fork(AFPObj *obj, struct vol *vol);
extern int utest_rename_keeps_open_fork_length(AFPObj *obj,
                                               struct vol *vol);
extern int utest_rename_onto_virtual_icon_refused(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_virtual_icon_open_checks_root_access(AFPObj *obj,
                                                      struct vol *vol);
extern int utest_rfork_close_survives_bad_metadata(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_rfork_close_keeps_metadata_past_bad_ea(AFPObj *obj,
                                                        struct vol *vol);
extern int utest_readonly_open_meets_deny_mode(AFPObj *obj, struct vol *vol);
extern int utest_rfork_no_access_opens_share_their_slot(AFPObj *obj,
                                                        struct vol *vol);
extern int utest_refused_delete_keeps_appledouble(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_tmplock_covers_uncovered_bytes_only(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_tmplock_read_lock_covers_read(AFPObj *obj, struct vol *vol);
extern int utest_tmplock_refused_stretch_undoes_the_taken(AFPObj *obj,
                                                          struct vol *vol);
extern int utest_delete_uncounted_drops_count(AFPObj *obj, struct vol *vol);
extern int utest_byte_locks_scan_in_start_order(AFPObj *obj,
                                                struct vol *vol);

#endif /* SUBTESTS_REQUEST_H */
