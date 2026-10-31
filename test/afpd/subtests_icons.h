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

#endif /* SUBTESTS_ICONS_H */
