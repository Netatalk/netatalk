#ifndef SUBTESTS_RECVFILE_H
#define SUBTESTS_RECVFILE_H

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_recvfile_write_lands_intact(AFPObj *obj, struct vol *vol);
extern int utest_recvfile_dfull_drains_request(AFPObj *obj, struct vol *vol);
extern int utest_recvfile_error_drains_request(AFPObj *obj, struct vol *vol);
extern int utest_recvfile_error_leaves_no_residue(AFPObj *obj, struct vol *vol);
extern int utest_recvfile_peer_close_ends_write(AFPObj *obj, struct vol *vol);
extern int utest_write_fork_eof_fails_request(AFPObj *obj, struct vol *vol);
extern int utest_recvfile_urgent_byte_finishes_write(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_recvfile_urgent_byte_at_eof(AFPObj *obj, struct vol *vol);
extern int utest_recvfile_symlink_refused(AFPObj *obj, struct vol *vol);
extern int utest_recvfile_rfork_length(AFPObj *obj, struct vol *vol);
extern int utest_recvfile_rfork_failure_keeps_length(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_recvfile_high_fd_waits(AFPObj *obj, struct vol *vol);
extern int utest_recvfile_unsplicable_fs_falls_back(AFPObj *obj,
                                                    struct vol *vol);
extern int utest_recvfile_bad_offset_keeps_splice(AFPObj *obj,
                                                  struct vol *vol);
extern int utest_recvfile_no_socket_splice_falls_back(AFPObj *obj,
                                                      struct vol *vol);
extern int utest_recvfile_pipe_takes_splice_size(AFPObj *obj,
                                                 struct vol *vol);
extern int utest_recvfile_pipe_never_shrinks(AFPObj *obj, struct vol *vol);
extern int utest_recvfile_pipe_steps_down(AFPObj *obj, struct vol *vol);
extern int utest_write_fork_short_write_fails_request(AFPObj *obj,
                                                      struct vol *vol);

#endif /* SUBTESTS_RECVFILE_H */
