/*!
 * @file
 * @brief Directory listing and working directory component test entry points
 */

#ifndef SUBTESTS_ENUMERATE_H
#define SUBTESTS_ENUMERATE_H

#include <stdint.h>

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_enumerate_rereads_counted(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_conversion_attempts(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_dehex_keeps_cnid(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_folder_rename_retried(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_unrenamed_keeps_cnid(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_failed_batch_keeps_index(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_failed_read_rereads(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_read_error_fails(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_interleaved_reads_once(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_one_kind_keeps_index(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_kind_change_rewinds(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_deleted_name_keeps_index(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_failed_stat_leaves_index(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_dehex_vetoed_leaves(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_dehex_gone_leaves(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_icon_ends_files(AFPObj *obj, struct vol *vol);
extern int utest_movecwd_skips_getcwd(AFPObj *obj, struct vol *vol);
extern int utest_movecwd_at_descriptor_limit(AFPObj *obj, struct vol *vol);
extern int utest_movecwd_volume_walks(const struct vol *vol);
extern int utest_movecwd_rechecks_root(AFPObj *obj, struct vol *vol);
extern int utest_movecwd_walk_ignores_case(AFPObj *obj, struct vol *vol);
extern int utest_movecwd_stale_root(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_long_name_skipped(AFPObj *obj, uint16_t vid);
extern int utest_enumerate_cnid_one_query(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_cnid_other_inode(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_cnid_changed_file(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_cnid_other_metadata(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_cnid_write_unloads(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_cnid_outside_listing(AFPObj *obj, struct vol *vol);
extern int utest_enumerate_cnid_reread_unarmed(AFPObj *obj, struct vol *vol);

#endif /* SUBTESTS_ENUMERATE_H */
