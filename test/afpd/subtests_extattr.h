#ifndef SUBTESTS_EXTATTR_H
#define SUBTESTS_EXTATTR_H

#include <atalk/globals.h>
#include <atalk/volume.h>

extern int utest_ea_param_reports_size(AFPObj *obj, struct vol *vol);
extern int utest_ea_protected_name_refused(AFPObj *obj, struct vol *vol);
extern int utest_ea_samba_size_matches(AFPObj *obj, struct vol *vol);
extern int utest_ea_missing_name_absent(AFPObj *obj, struct vol *vol);
extern int utest_ea_bad_name_refused(AFPObj *obj, struct vol *vol);
extern int utest_ea_missing_object_noobj(AFPObj *obj, struct vol *vol);
extern int utest_ea_names_cached(AFPObj *obj, struct vol *vol);
extern int utest_ea_names_follow_writes(AFPObj *obj, struct vol *vol);
extern int utest_ea_names_subsecond_write(AFPObj *obj, struct vol *vol);
extern int utest_ea_names_tier_lifecycle(AFPObj *obj, struct vol *vol);
extern int utest_ea_names_follow_access(AFPObj *obj, struct vol *vol);
extern int utest_ea_refused_write_quiet(AFPObj *obj, struct vol *vol);
extern int utest_ea_write_closed_dir(AFPObj *obj, struct vol *vol);
extern int utest_ea_values_cached(AFPObj *obj, struct vol *vol);
extern int utest_ea_values_follow_writes(AFPObj *obj, struct vol *vol);
extern int utest_ea_cache_counted(AFPObj *obj, struct vol *vol);
extern int utest_ea_values_cap(AFPObj *obj, struct vol *vol);
extern int utest_ea_hidden_name_uncached(AFPObj *obj, struct vol *vol);
extern int utest_ea_oversize_value_refused(AFPObj *obj, struct vol *vol);
extern int utest_ea_dir_lost_search_bit(AFPObj *obj, struct vol *vol);
extern int utest_ea_absent_name_from_list(AFPObj *obj, struct vol *vol);

#endif /* SUBTESTS_EXTATTR_H */
