/*!
 * @file
 * @brief CNID database and volume identity component test entry points
 */

#ifndef SUBTESTS_CNID_H
#define SUBTESTS_CNID_H

struct AFPObj;
struct vol;

extern int utest_cnid_volume_tag_identity(void);
extern int utest_cnid_wrapper_sets_errno(void);
extern int utest_cnid_error_codes_distinct(void);
extern int utest_cnid_valide_byteorder(void);
extern int utest_cnid_resolve_dot_names_rejected(void);
extern int utest_cnid_dirtab_rows(void);
extern int utest_cnid_dirtab_collisions(void);
extern int utest_cnid_find_name_bound(void);
extern int utest_cnid_sqlite_symlinks_rejected(void);
extern int utest_cnid_open_unknown_scheme_uses_sqlite(void);
extern int utest_cnid_open_scheme_case_insensitive(void);
extern int utest_cnid_open_missing_backend_refused(void);
extern int utest_cnid_scheme_registered(void);
extern int utest_cnid_sqlite_orphan_reported(void);
extern int utest_cnid_sqlite_creation_logged(void);
extern int utest_cnid_add_busy_not_fatal(struct vol *vol);
extern int utest_cnid_add_depletion_resets(struct vol *vol);
extern int utest_cnid_uuid_case_keeps_table(struct vol *vol);
extern int utest_cnid_resolve_notfound_errno(struct vol *vol);
extern int utest_cnid_corrupt_row_classified(struct vol *vol);
extern int utest_cnid_find_no_truncated_id(struct vol *vol);
extern int utest_cnid_dup_row_no_truncated_delete(struct vol *vol);
extern int utest_cnid_dup_row_parent_out_of_range(struct vol *vol);
extern int utest_cnid_delete_row_conditional(struct vol *vol);
extern int utest_cnid_update_row_conditional(struct vol *vol);
extern int utest_cnid_update_row_moved_keeps_other(struct vol *vol);
extern int utest_cnid_update_row_replace_calls(struct vol *vol);
extern int utest_cnid_update_row_after_reconnect(struct vol *vol);
extern int utest_cnid_mysql_failed_open_closes(struct vol *vol);
extern int utest_cnid_update_row_dropped_in_hold(struct vol *vol);
extern int utest_cnid_lookup_row_changed(struct vol *vol);
extern int utest_cnid_lookup_rows_changed(struct vol *vol);
extern int utest_cnid_row_case_renamed(struct vol *vol);
extern int utest_cnid_sqlite_owner_only(struct vol *vol);
extern int utest_cnid_desktop_folder_as_owner(struct AFPObj *obj,
                                              struct vol *vol);

#endif /* SUBTESTS_CNID_H */
