/*!
 * @file
 * @brief Volume poll and change mark component test entry points
 */

#ifndef SUBTESTS_VOLPOLLS_H
#define SUBTESTS_VOLPOLLS_H

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_volpoll_stamp_nanoseconds(const struct vol *vol);
extern int utest_volpoll_birthtime_member(void);
extern int utest_volpoll_spotlight_no_birthtime(void);
extern int utest_volpoll_probe_keeps_mtime(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_probe_reports_ro(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_probe_mount_ro(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_mark_once_a_second(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_root_date_kept(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_attention_versions(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_foreign_mark_announced(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_announced_mark_known(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_refused_attention_kept(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_own_mark_quiet(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_foreign_mark_this_second(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_foreign_mark_seen_second(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_root_entry_own(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_hard_create_compares(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_link_create_compares(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_link_root_create_own(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_sublink_root_create_own(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_mark_trailing_written(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_foreign_mark_same_second(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_quota_cached(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_fast_path_no_passwd(AFPObj *obj);
extern int utest_volpoll_srvrparms_mode_first(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_name_unencodable(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_srvrparms_cap(AFPObj *obj);
extern int utest_volpoll_ea_name_not_utf8(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_volname_not_utf8(AFPObj *obj);
extern int utest_volpoll_volname_from_start(AFPObj *obj);
extern int utest_volpoll_unwritten_unmarked(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_mark_setforkparams(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_mark_exchangefiles(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_mark_setextattr(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_mark_remextattr(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_mark_setvolparms(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_setvolparms_unwritten(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_mark_setacl(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_setacl_group_uuid(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_readonly_refused(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_openvol_failed_keeps_curdir(AFPObj *obj,
                                                     struct vol *vol);
extern int utest_volpoll_quota_unfetched(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_write_attention_quiet(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_setvolparms_flush_error(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_exchange_failure_cached(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_name_global_charset(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_ea_name_field(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_ea_value_bound(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_ea_none_refused(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_ea_ad_long_listed(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_ea_name_listed(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_volname_nul(AFPObj *obj);
extern int utest_volpoll_setdirparms_partial_marks(AFPObj *obj,
                                                   struct vol *vol);
extern int utest_volpoll_attention_quota_fresh(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_attention_sibling_quota(AFPObj *obj, struct vol *vol);

#endif /* SUBTESTS_VOLPOLLS_H */
