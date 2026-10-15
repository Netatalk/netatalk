#ifndef SUBTESTS_SENDFILE_H
#define SUBTESTS_SENDFILE_H

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_sendfile_read_lands_intact(AFPObj *obj, struct vol *vol);
extern int utest_sendfile_header_failure_sends_nothing(AFPObj *obj,
                                                       struct vol *vol);
extern int utest_sendfile_partial_header_lands_intact(AFPObj *obj,
                                                      struct vol *vol);
extern int utest_dsi_close_frame_unless_mid_reply(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_sendfile_short_fork_ends_session(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_buffered_short_fork_ends_session(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_sendfile_large_reply_keeps_session(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_sendfile_read_ext_fits_dsi_length(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_buffered_read_ext_fits_dsi_length(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_sendfile_refused_file_sent_from_buffer(AFPObj *obj,
                                                        struct vol *vol);
extern int utest_virtual_icon_fork_frees_adouble(AFPObj *obj, struct vol *vol);
extern int utest_virtual_icon_read_from_reply_buffer(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_sendfile_reconnect_session_ends(AFPObj *obj, struct vol *vol);
extern int utest_dsi_broken_reply_ends_session(AFPObj *obj, struct vol *vol);
extern int utest_reconnect_sigterm_sends_attention(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_rfork_cache_held_open_append_read_whole(AFPObj *obj,
                                                         struct vol *vol);
extern int utest_rfork_cache_stale_length_read_whole(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_buffered_read_stays_in_quantum(AFPObj *obj, struct vol *vol);
extern int utest_rfork_cache_read_stays_in_quantum(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_buffered_short_first_piece_replies(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_buffered_read_error_replies_misc(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_asp_read_unlocks_its_range(AFPObj *obj, struct vol *vol);
extern int utest_symlink_read_from_offset(AFPObj *obj, struct vol *vol);

#endif /* SUBTESTS_SENDFILE_H */
