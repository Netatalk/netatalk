#ifndef SUBTESTS_VOLPOLLS_H
#define SUBTESTS_VOLPOLLS_H

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_volpoll_stamp_nanoseconds(const struct vol *vol);
extern int utest_volpoll_probe_keeps_mtime(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_probe_reports_ro(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_probe_mount_ro(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_mark_once_a_second(AFPObj *obj, struct vol *vol);
extern int utest_volpoll_foreign_mark_announced(AFPObj *obj, struct vol *vol);
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

#endif /* SUBTESTS_VOLPOLLS_H */
