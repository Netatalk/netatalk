#ifndef SUBTESTS_SENDFILE_H
#define SUBTESTS_SENDFILE_H

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_sendfile_read_lands_intact(AFPObj *obj, struct vol *vol);
extern int utest_sendfile_header_failure_sends_nothing(AFPObj *obj,
                                                       struct vol *vol);
extern int utest_dsi_close_sends_nothing_mid_reply(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_sendfile_short_fork_ends_session(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_dsi_reply_refuses_unframed_length(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_sendfile_read_ext_fits_dsi_length(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_sendfile_refused_file_sent_from_buffer(AFPObj *obj,
                                                        struct vol *vol);

#endif /* SUBTESTS_SENDFILE_H */
