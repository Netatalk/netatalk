/*!
 * @file
 * @brief Icon path component test entry points
 */

#ifndef SUBTESTS_ICONS_H
#define SUBTESTS_ICONS_H

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_rfork_unchanged_close_skips_metadata(AFPObj *obj,
                                                      struct vol *vol);
extern int utest_rfork_readonly_close_sees_growth(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_rfork_close_refreshes_shared(AFPObj *obj, struct vol *vol);
extern int utest_icon_readonly_close_converts_first(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_rfork_open_measures_sidecar_once(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_rfork_join_measures_held_descriptor(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_folder_entry_keeps_metadata_read(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_catsearch_entry_keeps_metadata_read(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_rfork_open_probes_bands_for_deny_only(AFPObj *obj,
                                                       struct vol *vol);
extern int utest_rfork_open_skips_acl_read_mode_grants(AFPObj *obj,
                                                       struct vol *vol);
extern int utest_shared_rfork_length_survives_rsrc_close(AFPObj *obj,
                                                         struct vol *vol);
extern int utest_shared_rfork_length_survives_failed_open(AFPObj *obj,
                                                          struct vol *vol);
extern int utest_emptied_sidecar_goes_under_bad_metadata(AFPObj *obj,
                                                         struct vol *vol);
extern int utest_shared_rfork_length_survives_bad_sidecar(AFPObj *obj,
                                                          struct vol *vol);
extern int utest_readonly_volume_close_reads_nothing(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_closed_fork_drops_its_open_bit(AFPObj *obj, struct vol *vol);
extern int utest_open_forks_report_both(AFPObj *obj, struct vol *vol);
extern int utest_peer_data_fork_shows_in_attributes(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_cached_rfork_open_opens_data_file_only(AFPObj *obj,
                                                        struct vol *vol);
extern int utest_cached_rfork_read_stats_sidecar_once(AFPObj *obj,
                                                      struct vol *vol);
extern int utest_cached_rfork_close_closes_one_descriptor(AFPObj *obj,
                                                          struct vol *vol);
extern int utest_cached_rfork_lock_takes_own_reference(AFPObj *obj,
                                                       struct vol *vol);
extern int utest_cached_rfork_release_without_lock(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_cached_rfork_read_sees_rewrite(AFPObj *obj, struct vol *vol);
extern int utest_cached_rfork_read_after_buffer_drop(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_cached_rfork_meets_deny_read(AFPObj *obj, struct vol *vol);
extern int utest_cached_rfork_file_params_keep_metadata(AFPObj *obj,
                                                        struct vol *vol);
extern int utest_cached_rfork_close_drops_its_open_bit(AFPObj *obj,
                                                       struct vol *vol);
extern int utest_cached_rfork_second_open_joins(AFPObj *obj, struct vol *vol);
extern int utest_cached_rfork_missing_sidecar_reads_eof(AFPObj *obj,
                                                        struct vol *vol);
extern int utest_cached_rfork_short_sidecar_reads_eof(AFPObj *obj,
                                                      struct vol *vol);
extern int utest_cached_rfork_bad_sidecar_read_refused(AFPObj *obj,
                                                       struct vol *vol);
extern int utest_cached_rfork_unreadable_sidecar_read_refused(AFPObj *obj,
        struct vol *vol);
extern int utest_cached_rfork_renamed_file_read_refused(AFPObj *obj,
                                                        struct vol *vol);
extern int utest_hard_link_takes_base_open(AFPObj *obj, struct vol *vol);
extern int utest_root_icon_takes_base_open(AFPObj *obj, struct vol *vol);
extern int utest_cached_rfork_strict_locking_takes_base_open(AFPObj *obj,
        struct vol *vol);

#endif /* SUBTESTS_ICONS_H */
