#ifndef SUBTESTS_CNID_H
#define SUBTESTS_CNID_H

struct vol;

extern int utest_cnid_volume_tag_identity(void);
extern int utest_cnid_wrapper_sets_errno(void);
extern int utest_cnid_error_codes_distinct(void);
extern int utest_cnid_valide_byteorder(void);
extern int utest_cnid_resolve_dotdot_rejected(void);
extern int utest_cnid_add_busy_not_fatal(struct vol *vol);
extern int utest_cnid_add_depletion_resets(struct vol *vol);
extern int utest_cnid_resolve_notfound_errno(struct vol *vol);
extern int utest_cnid_corrupt_row_classified(struct vol *vol);
extern int utest_cnid_find_no_truncated_id(struct vol *vol);
extern int utest_cnid_dup_row_no_truncated_delete(struct vol *vol);

#endif /* SUBTESTS_CNID_H */
