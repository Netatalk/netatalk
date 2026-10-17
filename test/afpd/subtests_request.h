#ifndef SUBTESTS_REQUEST_H
#define SUBTESTS_REQUEST_H

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_write_reads_size_only_when_used(AFPObj *obj,
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
extern int utest_rfork_cache_skips_fill_while_written(AFPObj *obj,
                                                      struct vol *vol);
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
extern int utest_symlink_rfork_close_keeps_cnids(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_emptied_icon_close_deletes_file(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_emptied_icon_close_keeps_data(AFPObj *obj, struct vol *vol);
extern int utest_emptied_icon_close_keeps_symlink(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_virtual_icon_write_only_open_refuses_read(AFPObj *obj,
                                                           struct vol *vol);
extern int utest_icon_create_refused_replies_access(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_emptied_rfork_close_keeps_hard_link(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_rename_moves_open_forks(AFPObj *obj, struct vol *vol);
extern int utest_write_counts_bytes_appended(AFPObj *obj, struct vol *vol);
extern int utest_findnameat_follows_as_volume(AFPObj *obj, struct vol *vol);
extern int utest_rename_keeps_open_fork_length(AFPObj *obj,
                                               struct vol *vol);
extern int utest_virtual_icon_open_checks_root_access(AFPObj *obj,
                                                      struct vol *vol);

#endif /* SUBTESTS_REQUEST_H */
