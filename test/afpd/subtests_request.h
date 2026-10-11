#ifndef SUBTESTS_REQUEST_H
#define SUBTESTS_REQUEST_H

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_write_reads_size_only_when_used(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_volume_scan_resets_appended(AFPObj *obj, struct vol *vol);
extern int utest_write_end_past_limit_is_disk_full(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_rfork_cache_hit_sends_cached_bytes(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_rfork_cache_stale_length_reads_fork(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_rfork_cache_finds_name_on_disk(AFPObj *obj,
                                                struct vol *vol);
extern int utest_hint_relay_drops_repeats(AFPObj *obj, struct vol *vol);
extern int utest_fork_first_change_hints_siblings(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_rfork_cache_skips_fill_while_written(AFPObj *obj,
                                                      struct vol *vol);
extern int utest_rfork_cache_lookup_only_inside_fork(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_fce_register_keeps_no_path(AFPObj *obj, struct vol *vol);
extern int utest_fce_clock_read_only_when_held(AFPObj *obj,
                                               struct vol *vol);
extern int utest_read_pieces_fit_command_buffer(AFPObj *obj,
                                                struct vol *vol);
extern int utest_read_short_fork_sends_bytes_read(AFPObj *obj,
                                                  struct vol *vol);

#endif /* SUBTESTS_REQUEST_H */
